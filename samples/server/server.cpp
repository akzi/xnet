#include "../../include/xnet.hpp"
#include <fstream>
int main()
{
	xnet::proactor proactor;
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto acceptor = proactor.get_acceptor();
	std::fstream is;
	is.open("rsp.txt");
	if(!is)
		return 0;
	std::string rsp((std::istreambuf_iterator<char>(is)),
					std::istreambuf_iterator<char>());

	int send = 0;
	int recv = 0;
	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn) 
	{
		//accept connection
		conn.regist_recv_callback([id, &conns,&rsp, &send, &recv](void *data,int len)
		{
			//recv callback .
			if (len <= 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}

			//std::string str((char*)data, len);
			//std::cout << (char*)data;
			//async send data.
			//conns[id].async_send(data,len);
			conns[id].async_send(rsp.c_str(), (int)rsp.size());
			conns[id].async_recv_some();
 			if(conns.size() > 1)
 			{
 				conns.begin()->second.close();
 				conns.erase(conns.begin());
 			}
 			
		});
		conn.regist_send_callback([&](int len) {
			
			if(len == -1)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
			//async send data call back here
		});

		conns.emplace(id,std::move(conn));
		conns[id].async_recv_some();
		//std::cout << "id " << id << "recv " << ++recv << std::endl;
		id++;
		//acceptor.close();
	});
	//bind 
	acceptor.bind("0.0.0.0", 9001);
	//run .
	proactor.run();

	return 0;
}