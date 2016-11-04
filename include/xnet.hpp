#pragma once
#include "detail/detail.hpp"
namespace xnet
{
	
	class proactor;
	class connection:public no_copy_able
	{
	public:
		typedef std::function<void(void *, int)> recv_callback_t;
		typedef std::function<void(int)> send_callback_t;

		connection()
		{

		}
		connection(detail::connection_impl *impl)
			:impl_(impl)
		{
			assert(impl);
			init();
		}
		connection(connection &&_connection)
		{
			reset_move(std::move(_connection));
		}
		connection &operator=(connection &&_connection)
		{
			reset_move(std::move(_connection));
			return *this;
		}
		~connection()
		{
			close();
		}
		connection &regist_send_callback(send_callback_t callback)
		{
			send_callback_ = callback;
			return *this;
		}

		connection &regist_recv_callback(recv_callback_t callback)
		{
			recv_callback_ = callback;
			return *this;
		}
		void async_send(const void *data, int len)
		{
			assert(len);
			assert(data);
			assert(impl_);

			std::vector<uint8_t> buffer_;
			buffer_.resize(len);
			memcpy(buffer_.data(), data, len);

			try
			{
				impl_->async_send(buffer_);
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				send_callback_(-1);
			}
		}

		void async_recv(uint32_t len)
		{
			try
			{
				assert(len);
				assert(impl_);
				 impl_->async_recv(len);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str() << std::endl;
				recv_callback_(NULL, -1);
			}
		}
		void async_recv_some()
		{
			try
			{
				assert(impl_);
				impl_->async_recv(0);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str() << std::endl;
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
		bool valid()
		{
			return !!impl_;
		}
	private:
		void reset_move(connection && _conn)
		{
			if (this != &_conn)
			{
				impl_ = _conn.impl_;
				recv_callback_ = std::move(_conn.recv_callback_);
				send_callback_ = std::move(_conn.send_callback_);
				init();
				_conn.impl_ = NULL;
			}
		}
		void init()
		{
			impl_->bind_recv_callback(
				[this](void *data, int len) {
				assert(recv_callback_);
				recv_callback_(data, len);
			});
			impl_->bind_send_callback(
				[this](int len) {
				assert(send_callback_);
				send_callback_(len);
			});
		}

		detail::connection_impl *impl_ = NULL;
		send_callback_t send_callback_;
		recv_callback_t  recv_callback_;
		
	};

	class acceptor : public no_copy_able
	{
	public:
		typedef std::function<void(connection &&) > accept_callback_t;
		~acceptor()
		{
			close();
		}
		acceptor(acceptor &&acceptor_)
		{
			reset_move(std::move(acceptor_));
		}
		acceptor &operator =(acceptor &&acceptor_)
		{
			reset_move(std::move(acceptor_));
		}
		void regist_accept_callback(accept_callback_t callback)
		{
			accept_callback_ = callback;
			
		}
		bool bind(const std::string &ip, int port)
		{
			try
			{
				impl_->bind(ip, port);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str()<< std::endl;
				return false;
			}
			return true;
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
		acceptor()
		{
			
		}
		void reset_move(acceptor &&acceptor_)
		{
			if (this != &acceptor_)
			{
				init(acceptor_.impl_);
				acceptor_.impl_ = NULL;
				accept_callback_= std::move(acceptor_.accept_callback_);

			}
		}
		void init(detail::acceptor_impl *impl)
		{
			impl_ = impl;
			impl_->regist_accept_callback(
				[this](detail::connection_impl *conn)
			{
				assert(conn);
				accept_callback_(connection(conn));
			});
		}
		friend proactor;
		accept_callback_t accept_callback_;
		detail::acceptor_impl *impl_ = NULL;
	};
	class connector: no_copy_able
	{
	public:
		typedef std::function<void(connection &&)> success_callback_t;
		typedef std::function<void(std::string)> failed_callback_t;

		connector(connector && _connector)
		{
			reset_move(std::move(_connector));
		}
		connector & operator =(connector && _connector)
		{
			reset_move(std::move(_connector));
		}
		~connector()
		{

		}
		void sync_connect(const std::string &ip, int port)
		{
			ip_ = ip;
			port_ = port;
			try
			{
				impl_->sync_connect(ip_, port_);
			}
			catch (detail::socket_exception &e)
			{
				if (!failed_callback_)
					throw e;
				else
					failed_callback_(e.str());
			}
		}
		connector& bind_success_callback(success_callback_t callback)
		{
			success_callback_ = callback;
			impl_->bind_success_callback(
				[this](detail::connection_impl *conn)
			{ 
				success_callback_(connection(conn));
			});
			return *this;
		}
		connector& bind_fail_callback(failed_callback_t callback)
		{
			failed_callback_ = callback;
			impl_->bind_failed_callback(
				[this](std::string error_code)
			{
				failed_callback_(std::move(error_code));
			});
			return *this;
		}
		void close()
		{
			impl_->close();
		}
	private:
		friend proactor;

		void init(detail::connector_impl *impl)
		{
			impl_ = impl;
		}
		void reset_move(connector &&_connector)
		{
			if (this != &_connector)
			{
				assert(_connector.impl_);
				impl_ = _connector.impl_;
				success_callback_ = std::move(_connector.success_callback_);
				failed_callback_ = std::move(_connector.failed_callback_);
				impl_->bind_success_callback(
					[this](detail::connection_impl *conn) {
					success_callback_(connection(conn));
				});
				impl_->bind_failed_callback(
					[this](std::string error_code) {
					failed_callback_(error_code);
				});
				_connector.impl_ = NULL;
			}
		}
		connector()
		{
		}
		std::function<void(connection &&)> success_callback_;
		 failed_callback_t failed_callback_;
		detail::connector_impl *impl_ = NULL;
		std::string ip_;
		int port_;

	};
	class proactor: public no_copy_able
	{
	public:
		typedef timer_manager::timer_id timer_id;
		proactor()
		{
			impl = new detail::proactor_impl;
			impl->init();
		}
		proactor(proactor && _proactor)
		{
			impl = _proactor.impl;
			_proactor.impl = NULL;
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
			assert(impl);
			try
			{
				impl->run();
			}
			catch (detail::socket_exception& e)
			{
				std::cout << e.str() << std::endl;
				return false;
			}
			return true;
		}
		void stop()
		{
			assert(impl);
			impl->stop();
		}
		
		acceptor get_acceptor()
		{
			assert(impl);
			acceptor acc;
			acc.init(impl->get_acceptor());
			return acc;
		}
		connector get_connector()
		{
			assert(impl);
			connector connector_;
			connector_.init(impl->get_connector());
			return connector_;
		}
		timer_id set_timer(uint32_t timeout, std::function<bool()> func)
		{
			return impl->set_timer(timeout, func);
		}
		void cancel_timer(timer_id id)
		{
			impl->cancel_timer(id);
		}
	private:
		detail::proactor_impl *impl;
	};
	
}