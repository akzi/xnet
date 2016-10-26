#include <map>
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <cassert>
#include <exception>
#include <iostream>
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#include "iocp.hpp"
#endif
#include "detail.hpp"
#include "xnet.hpp"



int main()
{
	xnet::proactor proactor_;
	auto acceptor = proactor_.get_acceptor();
	acceptor.bind("0.0.0.0", 9001, [](xnet::connection && conn) {
		conn.async_recv(0, [] {});
	});
}


