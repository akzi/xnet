#pragma once
namespace xnet
{
	
	class proactor;
	class connection
	{
	public:
		connection(detail::connection_impl *impl)
			:impl_(impl)
		{
			assert(impl);
		}
		~connection()
		{
			close();
		}
		template<typename SEND_CALLBACK>
		void async_send(const void *data, int len, SEND_CALLBACK callback)
		{
			try
			{
				send_callback_ = callback;
				std::vector<uint8_t> buffer_;
				buffer_.resize(len);
				memcpy(buffer_.data(), data, len);
				impl_->async_send(data);
			}
			catch (std::exception* e)
			{
				send_callback_(-1);
			}
		}

		template<typename RECV_CALLBACK>
		void async_recv(uint32_t len, RECV_CALLBACK callback)
		{
			try
			{
				assert(len);
				recv_callback_ = callback;
				impl_->async_recv(len);
			}
			catch (std::exception &e)
			{
				std::cout << e.what() << std::endl;
				recv_callback_(NULL, -1);
			}
		}
		void close()
		{
			if (impl_)
			{
				impl_->close();
				impl_ = NULL;
			}
		}
	private:
		detail::connection_impl *impl_;
		std::function<void(int)> send_callback_;
		std::function<void(void *, int)> recv_callback_;
	};

	class acceptor
	{
	public:
		~acceptor()
		{
			close();
		}
		acceptor(acceptor &&acceptor_)
			:impl(acceptor_.impl)
		{
			acceptor_.impl = NULL;
		}

		template<class accept_callback_t>
		bool bind(const std::string &ip, int port, accept_callback_t callback)
		{
			try
			{
				accept_callback_ = callback;
				impl->bind(ip, port, [this](detail::connection_impl *conn)
				{
					accept_callback(conn);
				});
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		void close()
		{
			if (impl)
			{
				impl->close();
				impl = NULL;
			}
		}
	private:
		acceptor()
		{

		}
		friend proactor;

		void accept_callback(detail::connection_impl *conn)
		{
			assert(conn);
			accept_callback_(connection(conn));
		}
		std::function<void(connection &&) > accept_callback_;
		detail::acceptor_impl *impl = NULL;
	};
	class connector
	{
	public:
		connector(connector && _connector)
		{
			move(_connector);
		}
		bool sync_connect(const std::string &ip, int port)
		{
			ip_ = ip;
			port_ = port;
			try
			{
				impl_->connect(ip_, port_);
			}
			catch(std::exception &e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		template<typename SUCCESS_CALLBACK>
		connector& bind_success_callback(SUCCESS_CALLBACK success_callback)
		{
			success_callback_ = success_callback;
			impl_->bind_success_callback([this](detail::connection_impl *conn){ 
				success_callback_(connection(conn));
			});
			return *this;
		}
		template<typename FAILED_CALLBACK>
		connector& bind_fail_callback(FAILED_CALLBACK failed_callback)
		{
			failed_callback_ = failed_callback;
			impl_->bind_failed_callback([this](std::string error_code)
			{
				failed_callback_(std::move(error_code));
			});
			return *this;
		}
	private:
		void move(connector &_connector)
		{
			assert(_connector.impl_);
			impl_ = _connector.impl_;
			_connector.impl_ = NULL;
			success_callback_ = _connector.success_callback_;
			failed_callback_ = _connector.failed_callback_;
			_connector.success_callback_ = nullptr;
			_connector.failed_callback_ = nullptr;
			impl_->bind_success_callback([this](detail::connection_impl *conn)
			{
				success_callback_(connection(conn));
			});
			impl_->bind_failed_callback([this](std::string error_code)
			{
				failed_callback_(error_code);
			});
		}
		connector(const connector &) = delete;
		connector()
		{
		}
		friend proactor;
		std::function<void(connection &&)> success_callback_;
		std::function<void(std::string)> failed_callback_;
		detail::connector_impl *impl_;
		std::string ip_;
		int port_;

	};
	class proactor
	{
	public:
		proactor()
		{
			impl = new detail::proactor_impl;
		}
		~proactor()
		{
			if(impl)
			{
				impl->stop();
				delete impl;
			}
		}
		bool run()
		{
			try
			{
				impl->init();
				impl->run();
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				return false;
			}
			return true;
		}
		void stop()
		{
			impl->stop();
		}
		
		acceptor get_acceptor()
		{
			acceptor acc;
			acc.impl = impl->get_acceptor();
			return acc;
		}
		connector get_connector()
		{
			connector connector_;
			connector_.impl_ = impl->get_connector();
			return connector_;
		}
	private:
		proactor(const proactor &) = delete;
		proactor &operator =(const proactor &) = delete;
		detail::proactor_impl *impl;
	};
	
}