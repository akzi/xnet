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
	if (!is)
		return 0;
	std::string rsp((std::istreambuf_iterator<char>(is)),
		std::istreambuf_iterator<char>());

	int send = 0;
	int recv = 0;
	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn)
	{
		//accept connection
		conn.regist_recv_callback([id, &conns, &rsp, &send, &recv](void *data, int len)
		{
			//recv callback .
			if (len <= 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}

			conns[id].async_send(rsp.c_str(), (int)rsp.size());
			conns[id].async_recv_some();
			if (conns.size() > 1)
			{
				conns.begin()->second.close();
				conns.erase(conns.begin());
			}

		});
		conn.regist_send_callback([&](int len) {

			if (len == -1)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
		});

		conns.emplace(id, std::move(conn));
		conns[id].async_recv_some();
		id++;
	});
	int i = 0;
	auto timerid = proactor.set_timer(1000, [&] {
		std::cout << "timer callback" << std::endl;
		i++;
		if (i == 3)
			return false;//not repeat
		return true;//repeat timer
	});

	acceptor.bind("0.0.0.0", 9001);
	proactor.run();

	return 0;
}