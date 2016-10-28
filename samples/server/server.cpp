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
		//accept connection
		std::cout << "new conn" << std::endl;
		conn.regist_recv_callback([id, &conns,&rsp, &count](void *data,int len) 
		{
			//recv callback .
			if (len == -1)
			{
				std::cout << "recv failed" << std::endl;
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}

			std::string str((char*)data, len);
			//std::cout << str.c_str();
			//async send data.
			conns[id].async_send(rsp.c_str(), rsp.size());
			count += len;
			conns[id].close();
			
		});
		conn.regist_send_callback([](int len) {
			//async send data call back here
			std::cout << "send callback," << len << std::endl;
		});

		conns.emplace(id,std::move(conn));
		conns[id].async_recv_some();
		id++;
		//acceptor.close();
	});
	//bind 
	acceptor.bind("0.0.0.0", 9001);
	//run .
	proactor.run();

	return 0;
}