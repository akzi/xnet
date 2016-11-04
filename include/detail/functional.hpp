#pragma once
namespace xnet
{
	namespace detail
	{
		struct win32
		{
			char dumy;
		};
		struct linux
		{
			char dumy;
		};

		template<typename T, typename OS>
		struct _socket_closer
		{
			void operator()(T s)
			{
				assert(false);
			}
		};
		template<typename T>
		struct _socket_closer<T, win32>
		{
			void operator()(T s)
			{
				closesocket(s);
			}
		};
		template<typename T>
		struct _socket_closer<T, linux>
		{
			void operator()(T s)
			{
				close(s);
			}
		};

		template<class T>
		struct _socket_initer
		{
			static _socket_initer &get_instance()
			{
				static _socket_initer inst;
				return inst;
			}
			~_socket_initer()
			{

			}
		};

		template<>
		struct _socket_initer<win32>
		{
			static _socket_initer &get_instance()
			{
				WSADATA wsaData;
				int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (NO_ERROR != nResult)
				{
					assert(false);
				}
				static _socket_initer inst;
				return inst;
			}
			~_socket_initer()
			{
				WSACleanup();
			}
		};
		template<typename T, typename F>
		struct _selecter;
		template<typename T>
		struct _selecter<T, win32>
		{
			int operator()(
				SOCKET maxfds ,
				fd_set *readfds, 
				fd_set *writefds, 
				fd_set *exceptfds,
				uint32_t timeout)
			{
				struct timeval tv = { 
					(long)(timeout / 1000),
					(long)(timeout % 1000 * 1000) 
				};
				int rc = ::select(0 ,readfds, writefds, exceptfds, timeout ? &tv : NULL);
				xnet_assert(rc != SOCKET_ERROR);
				return rc;
			}
		};

		template<typename T>
		struct _selecter<T, linux>
		{
			int operator()(
				SOCKET maxfds, 
				fd_set *readfds, 
				fd_set *writefds, 
				fd_set *exceptfds, 
				uint32_t timeout)
			{
				struct timeval tv = { 
					(long)(timeout / 1000),
					(long)(timeout % 1000 * 1000) 
				};
				int rc = ::select(maxfds + 1, readfds, writefds, exceptfds, timeout ? &tv : NULL);
				if (rc == -1) {
					xnet_assert(errno == EINTR);
					return 0;
				}
				return rc;
			}
		};

		template <typename T, typename U>
		struct _get_last_errorer;

		template <typename T>
		struct _get_last_errorer<T,win32>
		{
			int operator()()
			{
				return GetLastError();
			}
		};

		template <typename T>
		struct _get_last_errorer<T, linux>
		{
			int operator()()
			{
				return errno;
			}
		};
	}

#ifdef _WIN32
	template<typename T>
	struct socket_closer:
		detail::_socket_closer<T, detail::win32>
	{
	};
	struct socket_initer:
		detail::_socket_initer<detail::win32>
	{
	};
	struct selecter :
		detail::_selecter<nullptr_t, detail::win32>
	{
	};
	struct get_last_errorer :
		detail::_get_last_errorer<nullptr_t, detail::win32>
	{

	};
#else
	template<typename T>
	struct socket_closer :
		detail::_socket_closer<T, detail::linux>
	{
	};
	struct socket_initer :
		detail::_socket_initer<detail::linux>
	{
	};
	struct selecter :detail::_selecter<nullptr_t, detail::linux>
	{
	};
	struct get_last_errorer :
		detail::_get_last_errorer<nullptr_t, detail::linux>
	{

	};
#endif

}