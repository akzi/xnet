#include "../../include/proactor_pool.hpp"
#include <list>

int main()
{
	const char *rsp = "HTTP/1.1 200 OK\r\n"
		"Date: Fri, 28 Oct 2016 12:43:43 GMT\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: 11\r\n"
		"\r\n"
		"hello world";

	using namespace xnet;
	proactor_pool pp;
	std::mutex mtx;
	std::vector<connection> connections;
	std::list<std::size_t> index_;

	pp.bind("0.0.0.0", 9001).
		set_size(0).
		regist_accept_callback([&](connection &&conn) {
		std::lock_guard<std::mutex> lg(mtx);
		std::size_t index;
		connections.reserve(100000);
		connections.emplace_back(std::move(conn));
		index = connections.size() - 1;

		connections[index].regist_recv_callback(
			[index,rsp,&connections,&index_](char *data, std::size_t len) {
			auto id = index;
			if (len == 0)
			{
				connections[id].close();
				return;
			}
			//std::cout << data << std::endl;
			//std::cout << std::this_thread::get_id() << std::endl;
			//std::this_thread::sleep_for(std::chrono::nanoseconds(10));
			connections[id].async_send(rsp, (int)strlen(rsp));
			connections[id].async_recv_some();
		});
		connections[index].regist_send_callback(
			[index, &connections, &index_](std::size_t len) {
			int id = index;
			if (len == 0)
			{
				connections[id].close();
			}
		});
		connections[index].async_recv_some();

	});

	pp.start();
	getchar();
	pp.stop();

	return 0;
}