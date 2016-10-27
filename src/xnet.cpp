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



int main()
{
	xnet::proactor proactor_;
	auto acceptor = proactor_.get_acceptor();
	assert(acceptor.bind("0.0.0.0", 9001));
	acceptor.regist_accept_callback([](xnet::connection && conn) {
		conn.regist_recv_callback([](void *data, int) {

		});
		conn.async_recv(10);
	});
	auto connector = proactor_.get_connector();
	connector.
		bind_fail_callback([](std::string str) {
		std::cout << str.c_str() << std::endl;
	}).
		bind_success_callback([](xnet::connection &&conn) 
	{ 
		conn.close();
	}).
		sync_connect("127.0.0.1", 9002);

	proactor_.run();
	return 0;
}


