#pragma once
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <cassert>
#include <exception>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>
#include "common/guard.hpp"
#include "common/no_copy_able.hpp"
#include "detail/timer.hpp"
#define SELECT 1
#define FD_SETSIZE      1024
#if defined _WIN32 
#define IOCP 0
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>//socklen_t 
#include <winsock2.h>
#include <mswsock.h>
#include "exceptions.hpp"
#include "detail/iocp.hpp"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#elif £¨__GNUC__ > 4£© && £¨__GNUC_MINOR__ > 8£©
#define _LINUX_
#define EPOLL
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
typedef int SOCKET
#ifndef max_io_events
#define max_io_events 256
#endif
#include "exceptions.hpp"
#include "detail/epoll.hpp"
#endif
#include "detail/select.hpp"
#include "xnet.hpp"

namespace xnet
{
	namespace detail
	{

#if IOCP
		typedef iocp::connection_impl connection_impl;
		typedef iocp::acceptor_impl acceptor_impl;
		typedef iocp::proactor_impl proactor_impl;
		typedef iocp::connector_impl connector_impl;
		typedef iocp::socket_exception socket_exception;
#elif EPOLL
		
#elif SELECT
		typedef select::connection_impl connection_impl;
		typedef select::acceptor_impl acceptor_impl;
		typedef select::proactor_impl proactor_impl;
		typedef select::connector_impl connector_impl;
#endif
		
	}
}
