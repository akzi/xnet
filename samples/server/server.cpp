#include "../../include/xnet.hpp"

int main()
{
	xnet::proactor proactor;
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto acceptor = proactor.get_acceptor();
	std::string rsp =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";
	int count = 0;
	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn) 
	{
		conn.regist_recv_callback([id, &conns,&rsp, &count](void *data,int len) 
		{
			if (len == -1)
			{
				std::cout << "recv failed" << std::endl;
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
			if (len == 0);

			std::string str((char*)data, len);
			std::cout << str.c_str();
			//conns[id].async_send(rsp.c_str(), rsp.size());
			count += len;
			conns[id].async_recv_some();
			//conns[id].close();
			
		});
		conn.regist_send_callback([](int len) {
			std::cout << "send callback," << len << std::endl;
		});

		conns.emplace(id,std::move(conn));
		conns[id].async_recv_some();
		id++;
	});
	acceptor.bind("0.0.0.0", 9001);

	proactor.run();

	return 0;
}