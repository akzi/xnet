#pragma once
#ifdef _WIN32
namespace xnet
{
	namespace detail
	{
		typedef iocp::connection_impl connection_impl;
		typedef iocp::acceptor_impl acceptor_impl;
		typedef iocp::proactor_impl proactor_impl;
		typedef iocp::socket_exception socket_exception;
	}
}
#endif