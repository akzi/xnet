#pragma once
#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <cassert>
#include <exception>
#include <iostream>
#include "common/guard.hpp"
#include "common/no_copy_able.hpp"
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#include "detail/iocp.hpp"
#endif
#include "detail/detail.hpp"
#include "xnet.hpp"
#ifdef _WIN32
namespace xnet
{
	namespace detail
	{
		typedef iocp::connection_impl connection_impl;
		typedef iocp::acceptor_impl acceptor_impl;
		typedef iocp::proactor_impl proactor_impl;
		typedef iocp::connector_impl connector_impl;
		typedef iocp::socket_exception socket_exception;
	}
}
#endif