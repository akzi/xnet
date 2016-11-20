#pragma once
#include "detail/detail.hpp"
namespace xnet
{

	class proactor;
	class connection :public no_copy_able
	{
	public:
		typedef std::function<void(char*, std::size_t)> recv_callback_t;
		typedef std::function<void(std::size_t)> send_callback_t;

		connection()
		{

		}
		connection(detail::connection_impl *impl)
			:impl_(impl)
		{
			xnet_assert(impl);
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
		int send(const char *data, int len)
		{
			xnet_assert(impl_);
			return impl_->send(data, len);
		}
		void async_send(std::string &&buffer)
		{
			xnet_assert(impl_);
			xnet_assert(buffer.size());
			try
			{
				impl_->async_send(std::move(buffer));
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				send_callback_(-1);
			}
		}

		void async_send(const char *data, int len)
		{
			xnet_assert(len);
			xnet_assert(data);
			xnet_assert(impl_);

			try
			{
				impl_->async_send({ data, (uint32_t)len });
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				send_callback_(-1);
			}
		}

		void async_recv(std::size_t len)
		{
			try
			{
				xnet_assert(len);
				xnet_assert(impl_);
				impl_->async_recv(len);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str() << std::endl;
				recv_callback_(NULL, 0);
			}
		}
		void async_recv_some()
		{
			try
			{
				xnet_assert(impl_);
				impl_->async_recv(0);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str() << std::endl;
				recv_callback_(NULL, 0);
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
		friend proactor;
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
				[this](char *data, std::size_t len) {
				xnet_assert(recv_callback_);
				recv_callback_(data, len);
			});
			impl_->bind_send_callback(
				[this](std::size_t len) {
				xnet_assert(send_callback_);
				send_callback_(len);
			});
		}
		proactor *pro_;
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
			return *this;
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
				std::cout << e.str() << std::endl;
				return false;
			}
			return true;
		}
		bool get_addr(std::string &ip, int &port)
		{
			try
			{
				assert(impl_);
				impl_->get_addr(ip, port);
			}
			catch (detail::socket_exception &e)
			{
				std::cout << e.str() << std::endl;
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
		proactor &get_proactor()
		{
			return *proactor_;
		}
	private:
		acceptor()
		{

		}
		void reset_move(acceptor &&acceptor_)
		{
			if (this == &acceptor_)
				return;
			init(acceptor_.impl_);
			acceptor_.impl_ = NULL;
			accept_callback_ = std::move(acceptor_.accept_callback_);
		}
		void init(detail::acceptor_impl *impl)
		{
			impl_ = impl;
			impl_->regist_accept_callback(
				[this](detail::connection_impl *conn)
			{
				xnet_assert(conn);
				accept_callback_(std::move(connection(conn)));
			});
		}
		friend proactor;
		proactor *proactor_;
		accept_callback_t accept_callback_;
		detail::acceptor_impl *impl_ = nullptr;
	};
	class connector : no_copy_able
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
			return *this;
		}
		~connector()
		{
			close();
		}
		void async_connect(const std::string &ip, int port)
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
			xnet_assert(impl_);
			impl_->bind_failed_callback(
				[this](std::string error_code)
			{
				failed_callback_(std::move(error_code));
			});
			return *this;
		}
		void close()
		{
			if (impl_)
			{
				impl_->close();
				impl_ = nullptr;
			}
		}
	private:
		friend proactor;

		void init(detail::connector_impl *impl)
		{
			impl_ = impl;
		}
		void reset_move(connector &&_connector)
		{
			if (this == &_connector)
				return;
			xnet_assert(_connector.impl_);
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
			_connector.impl_ = nullptr;
		}
		connector()
		{
		}
		std::function<void(connection &&)> success_callback_;
		failed_callback_t failed_callback_;
		detail::connector_impl *impl_ = nullptr;
		std::string ip_;
		proactor *proactor_ = nullptr;
		int port_;

	};
	class proactor : public no_copy_able
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
			if (impl)
			{
				impl->stop();
				delete impl;
			}
		}
		bool run()
		{
			xnet_assert(impl);
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
			xnet_assert(impl);
			impl->stop();
		}

		acceptor get_acceptor()
		{
			xnet_assert(impl);
			acceptor acc;
			acc.init(impl->get_acceptor());
			return acc;
		}
		connector get_connector()
		{
			xnet_assert(impl);
			connector connector_;
			connector_.init(impl->get_connector());
			return std::move(connector_);
		}
		timer_id set_timer(uint32_t timeout, std::function<bool()> func)
		{
			return impl->set_timer(timeout, func);
		}
		void cancel_timer(timer_id id)
		{
			impl->cancel_timer(id);
		}
		void regist_connection(connection &conn)
		{
			xnet_assert(conn.impl_);
			impl->regist_connection(*conn.impl_);
		}
	private:
		detail::proactor_impl *impl;
	};

}
