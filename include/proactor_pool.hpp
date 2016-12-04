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
		
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		typedef std::function<void(void)> callbck_t;
		proactor_pool()
			:proactors_(1),
			acceptor_(std::move(proactors_[0].get_acceptor()))
		{

		}
		proactor_pool &set_size(std::size_t size)
		{
			size_ = size;
			return *this;
		}
		std::size_t get_size()
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
		proactor_pool & regist_run_before(callbck_t callback)
		{
			run_before_callbck_ = callback;
			return *this;
		}
		proactor_pool &regist_run_end(callbck_t callbck)
		{
			run_end_callbck_ = callbck;
			return *this;
		}
	private:
		proactor* current_proactor_store(proactor *_proactor)
		{
			static thread_local proactor *current_proactor_ = nullptr;
			if (_proactor)
				current_proactor_ = _proactor;
			return current_proactor_;
		}
		void do_start()
		{
			if (!size_) 
				size_ = std::thread::hardware_concurrency() * 2;
			proactors_.reserve(size_);
			for (std::size_t i = 1 ; i < size_; i++)
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
			conn_boxs_.reserve(size_);
			workers_.reserve(size_);
			for (std::size_t i = 0; i < size_; ++i)
			{
				proactor &pro = proactors_[i];
				workers_.emplace_back([&] {
					current_proactor_store(&pro);
					conn_boxs_.emplace_back(new msgbox<connection >(pro));
					conn_boxs_.back()->regist_notify([&pro,this,i] {
						do
						{
							auto &msgbox = conn_boxs_[i];
							auto item = msgbox->recv();
							if (!item.first)
								break;
							pro.regist_connection(item.second);
							accept_callback_(std::move(item.second));
						} while (true);
					});
					conn_boxs_.back()->regist_inited_callback([&] {
						std::unique_lock<std::mutex> locker(mtx);
						sync.notify_one();
					});
					if(run_before_callbck_)
						run_before_callbck_();
					pro.run();
					if (run_end_callbck_)
						run_end_callbck_();
				});
				std::unique_lock<std::mutex> locker(mtx);
				sync.wait(locker);
			}
		}
		void accept_callback(connection && conn)
		{
			conn_boxs_[++mbox_index_ % conn_boxs_.size()]->send(std::move(conn));
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

		std::string ip_;
		int port_ = 0;
		std::size_t size_ = 0;
		std::vector<std::thread> workers_;
		std::vector<proactor> proactors_;
		accept_callback_t accept_callback_;
		acceptor acceptor_;
		int64_t mbox_index_ = 0;
		std::vector<std::unique_ptr<msgbox<connection>>> conn_boxs_;
		callbck_t run_before_callbck_;
		callbck_t run_end_callbck_;
	};
}
