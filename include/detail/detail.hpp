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
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#define trace std::cout << __FILE__ << " "<< __LINE__<< std::endl;std::cout.flush();
#include "common/guard.hpp"
#include "common/no_copy_able.hpp"
#include "timer.hpp"
#define SELECT 1
#if defined _WIN32 
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE      1024
#define IOCP 0
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>//socklen_t 
#include <winsock2.h>
#include <mswsock.h>
#include "exceptions.hpp"
#include "functional.hpp"
#include "iocp.hpp"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#else
#define _LINUX_
#define EPOLL 0
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#ifndef max_io_events
#define max_io_events 256
#define  SD_SEND SHUT_WR 
#define  SD_RECEIVE SHUT_RD 
#endif
#include "exceptions.hpp"
#include "functional.hpp"
#include "epoll.hpp"
#endif

#include "select.hpp"

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
		typedef epoll::connection_impl connection_impl;
		typedef epoll::acceptor_impl acceptor_impl;
		typedef epoll::proactor_impl proactor_impl;
		typedef epoll::connector_impl connector_impl;
#elif SELECT
		typedef select::connection_impl connection_impl;
		typedef select::acceptor_impl acceptor_impl;
		typedef select::proactor_impl proactor_impl;
		typedef select::connector_impl connector_impl;
#endif

	}
}
