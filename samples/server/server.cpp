#include "../../include/xnet.hpp"
#include <fstream>
int main()
{
	xnet::proactor proactor;
	std::map<int, xnet::connection> conns;
	int id = 0;
	auto acceptor = proactor.get_acceptor();
	std::fstream is;
	is.open("./rsp.txt");
	if (!is)
	{
		std::cout << "open file rsp.txt failed, exit()..." << std::endl;
		return 0;
	}
	std::string rsp((std::istreambuf_iterator<char>(is)),
		std::istreambuf_iterator<char>());

	acceptor.regist_accept_callback(
		[&](xnet::connection &&conn)
	{
		//accept connection
		conns.emplace(id, std::move(conn));
		conns[id].regist_recv_callback([id, &conns](void *data, int len)
		{
			//recv callback .
			(void)data;
			if (len <= 0)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}

			
			conns[id].async_recv_some();
// 			if (conns.size() > 1)
// 			{
// 				conns.begin()->second.close();
// 				conns.erase(conns.begin());
// 			}

		});
		conns[id].regist_send_callback([&](int len) {

			if (len == -1)
			{
				conns[id].close();
				conns.erase(conns.find(id));
				return;
			}
			conns[id].async_send("hello world", len);
		});
		conns[id].async_recv_some();
		conns[id].async_send("hello world", 1);
	});
	int i = 0;
	proactor.set_timer(1000, [&] {
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