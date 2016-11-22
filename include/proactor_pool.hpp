#pragma once
#include "xnet.hpp"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
namespace xnet
{
	class proactor_pool
	{
	private:
		struct mailbox
		{
			mailbox() {}
			mailbox(const mailbox &) 
			{
			}
			mailbox(mailbox && self) 
			{
				if (&self == this)
					return;
				sender_ = std::move(self.sender_);
				recevier_= std::move(self.recevier_);
				queue_ = std::move(self.queue_);
				proactor_ = self.proactor_;
			}
			void push(connection &&conn)
			{
				std::lock_guard<std::mutex> lg(mtx_);
				queue_.emplace_back(std::move(conn));
				char ch = 1;
				sender_.send(&ch, 1);
			}
			bool pop(connection &conn)
			{
				std::lock_guard<std::mutex> lg(mtx_);
				if (queue_.empty())
					return false;
				conn = std::move(queue_.back());
				queue_.pop_back();
				return true;
			}
			~mailbox()
			{
				sender_.close();
				recevier_.close();
				queue_.clear();
			}
			connection sender_;
			connection recevier_;
			std::mutex mtx_;
			std::vector<connection> queue_;
			proactor *proactor_;
		};
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		proactor_pool()
			:proactors_(1),
			acceptor_(std::move(proactors_[0].get_acceptor()))
		{

		}
		proactor_pool &set_size(int size)
		{
			size_ = size;
			return *this;
		}
		int get_size()
		{
			return size_;
		}
		proactor &get_proactor(std::size_t index)
		{
			if (index >= proactors_.size())
				throw std::out_of_range(
					"index >= proactors_.size()");
			return proactors_[index];
		}
		proactor &get_current_proactor()
		{
			return *current_proactor_store(nullptr);
		}
		proactor_pool &bind(const std::string &ip, int port)
		{
			ip_ = ip;
			port_ = port;
			return *this;
		}
		proactor_pool &regist_accept_callback(accept_callback_t cb )
		{
			accept_callback_ = cb;
			return *this;
		}
		void start()
		{
			do_start();
		}
		void stop()
		{
			do_stop();
		}
	private:
		proactor* current_proactor_store(proactor *_proactor)
		{
			static thread_local proactor *current_proactor_ = nullptr;
			if (_proactor)
				current_proactor_ = _proactor;
			return _proactor;
		}
		void do_start()
		{
			if (!size_) 
				size_ = std::thread::hardware_concurrency() * 2;
			proactors_.reserve(size_);
			for (int i = 1 ; i < size_; i++)
				proactors_.emplace_back();
			acceptor_.regist_accept_callback(std::bind(&
				proactor_pool::accept_callback, 
				this, std::placeholders::_1));
			xnet_assert(acceptor_.bind(ip_, port_));
			init_worker();
		}
		void init_worker()
		{
			std::mutex mtx;
			std::condition_variable sync;
			mailboxs_.reserve(size_);
			workers_.reserve(size_);
			for (int i = 0; i < size_; ++i)
			{
				proactor &pro = proactors_[i];
				workers_.emplace_back([&] {
					int count = 0;
					std::string ip;
					int port;
					mailboxs_.emplace_back();
					auto acceptor = pro.get_acceptor();
					acceptor.bind("127.0.0.1", 0);
					xnet_assert(acceptor.get_addr(ip, port));
					acceptor.regist_accept_callback([&](connection &&conn) {
						std::cout <<"regist_accept_callback"<<std::endl;
						mailboxs_.back().proactor_ = &pro;
						mailboxs_.back().recevier_ = std::move(conn);
						if (++count == 2)
						{
							init_mailbox(mailboxs_.back());
							std::unique_lock<std::mutex> locker_(mtx);
							sync.notify_one();
						}
						acceptor.close();
					});
					auto connector = pro.get_connector();
					connector.bind_success_callback([&](connection &&connn) {
						std::cout << "bind_success_callback" <<std::endl;
						mailboxs_.back().sender_ = std::move(connn);
						if (++count == 2)
						{
							init_mailbox(mailboxs_.back());
							std::unique_lock<std::mutex> locker_(mtx);
							sync.notify_one();
						}
						connector.close();
					});
					connector.bind_fail_callback([](std::string &&str) {
						std::cout << "bind_fail_callback" <<std::endl;
						std::cout << str << std::endl;
					});
					connector.async_connect(ip, port);
					current_proactor_store(&pro);
					pro.run();
				});
				

				std::unique_lock<std::mutex> locker(mtx);
				sync.wait(locker);
			}
			std::cout << "init_mailbox done"<<std::endl;
		}
		void accept_callback(connection && conn)
		{
			mailboxs_[++mbox_index_ % mailboxs_.size()].push(std::move(conn));
		}
		void do_stop()
		{
			for (auto &itr: proactors_)
			{
				itr.stop();
			}
			for (auto &itr: workers_)
			{
				itr.join();
			}
		}
		void init_mailbox(mailbox &mbox)
		{
			mbox.recevier_.regist_recv_callback(
				[this,&mbox](char *, std::size_t len)
			{
				xnet_assert(len);
				xnet_assert(accept_callback_);
				connection conn;
				do 
				{
					if (!mbox.pop(conn))
						break;
					mbox.proactor_->regist_connection(conn);
					accept_callback_(std::move(conn));
				} while (true);
				mbox.recevier_.async_recv_some();
			});
			mbox.recevier_.async_recv_some();
		}

		std::string ip_;
		int port_ = 0;
		int size_ = 0;
		std::vector<std::thread> workers_;
		std::vector<proactor> proactors_;
		accept_callback_t accept_callback_;
		acceptor acceptor_;
		int64_t mbox_index_ = 0;
		std::vector<mailbox> mailboxs_;
	};
}
