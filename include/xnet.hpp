#pragma once
#include "detail.hpp"
namespace xnet
{
	struct error_t
	{
	public:
		error_t(const std::string &error_str)
			:error_string(error_str)
		{

		}
		operator bool()
		{
			return error_string.empty();
		}
		std::string to_string()
		{
			return error_string;
		}
	private:
		std::string error_string;
	};
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
		void async_recv(int len, RECV_CALLBACK callback)
		{

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
	};

	class acceptor
	{
	public:
		acceptor()
		{

		}
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
		void bind(const std::string &ip, int port, accept_callback_t callback)
		{
			accept_callback_ = callback;
			impl->bind(ip, port, [this](detail::connection_impl *conn) {
				accept_callback(conn);
			});
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
		friend proactor;

		void accept_callback(detail::connection_impl *conn)
		{
			assert(conn);
			accept_callback_(connection(conn));
		}
		std::function<void(connection &&) > accept_callback_;
		detail::acceptor_impl *impl = NULL;
	};

	class proactor
	{
	public:
		proactor()
		{
			impl = new detail::proactor_impl;
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
			}
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
	private:
		detail::proactor_impl *impl;
	};
	
}