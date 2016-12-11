#pragma once
#include "xnet.hpp"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <list>
namespace xnet
{
	class proactor_pool
	{
		
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		typedef std::function<void(void)> callbck_t;
		proactor_pool(std::size_t io_threads = std::thread::hardware_concurrency())
			:size_(io_threads)
		{
			if (!size_)
				size_ = std::thread::hardware_concurrency() * 2;
			proactors_.reserve(size_);
		}
		std::size_t get_size()
		{
			return size_;
		}
		proactor &get_proactor(std::size_t index)
		{
			if (index >= proactors_.size())
				throw std::out_of_range("index >= proactors_.size()");
			return proactors_[index];
		}
		proactor &get_current_proactor()
		{
			return *current_proactor_store(nullptr);
		}
		proactor_pool &bind(const std::string &ip, int port)
		{
			is_bind_ = true;
			ip_ = ip;
			port_ = port;
			xnet_assert(conn_boxs_.size());
			if (is_start_)
			{
				auto func = [this] {
					acceptor_.reset(new xnet::acceptor(get_current_proactor().get_acceptor()));
					acceptor_->regist_accept_callback(std::bind(&
						proactor_pool::accept_callback,
						this, std::placeholders::_1));
					xnet_assert(acceptor_->bind(ip_, port_));
				};
				conn_boxs_[0]->send(func);
			}
			return *this;
		}
		void start()
		{
			for (std::size_t i = 0; i < size_; i++)
				proactors_.emplace_back();
			start_proactors();
			is_start_ = true;
			if(is_bind_)
				bind(ip_, port_);
		}
		void stop()
		{
			for (auto &itr : proactors_)
				itr.stop();
			for (auto &itr : workers_)
				itr.join();
		}
		proactor_pool &regist_accept_callback(accept_callback_t callback )
		{
			accept_callback_ = callback;
			return *this;
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
		using msgbox_t = msgbox<std::function<void()>>;
		proactor* current_proactor_store(proactor *_proactor)
		{
			static thread_local proactor *current_proactor_ = nullptr;
			if (_proactor)
				current_proactor_ = _proactor;
			return current_proactor_;
		}
		
		void start_proactors()
		{
			std::mutex mtx;
			std::condition_variable sync;
			conn_boxs_.reserve(size_);
			workers_.reserve(size_);
			for (std::size_t i = 0; i < size_; ++i)
			{
				proactor &pro = proactors_[i];
				workers_.emplace_back([&] 
				{
					current_proactor_store(&pro);
					conn_boxs_.emplace_back(new msgbox_t(pro));
					conn_boxs_.back()->regist_notify([&pro,this,i]
					{
						do
						{
							auto &msgbox = conn_boxs_[i];
							auto item = msgbox->recv();
							if (!item.first)
								break;
							item.second();
						} while (true);
					});
					conn_boxs_.back()->regist_inited_callback([&] 
					{
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
		connection get_connection()
		{
			std::lock_guard<std::mutex> locker(connections_mutex_);
			xnet::connection _conn = std::move(connections_.back());
			connections_.pop_back();
			return std::move(_conn);
		}
		void add_connection(connection && conn)
		{
			std::lock_guard<std::mutex> locker(connections_mutex_);
			connections_.emplace_back(std::move(conn));
		}
		void accept_callback(xnet::connection && conn)
		{
			add_connection(std::move(conn));
			auto func = [this]()mutable {
				auto _conn = get_connection();
				get_current_proactor().regist_connection(_conn);
				accept_callback_(std::move(_conn));
			};
			conn_boxs_[++mbox_index_ % conn_boxs_.size()]->send(std::move(func));
		}
		std::atomic_bool is_bind_ = false;
		std::atomic_bool is_start_ = false;
		std::string ip_;
		int port_ = 0;
		std::size_t size_ = 0;
		
		std::vector<std::thread> workers_;
		std::vector<proactor> proactors_;
		
		accept_callback_t accept_callback_;
		std::unique_ptr<acceptor> acceptor_;
		
		int64_t mbox_index_ = 0;
		std::vector<std::unique_ptr<msgbox_t>> conn_boxs_;

		callbck_t run_before_callbck_;
		callbck_t run_end_callbck_;

		std::mutex connections_mutex_;
		std::list<xnet::connection> connections_;
	};
}
