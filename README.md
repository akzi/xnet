# xnet
sample async network lib

easy to use.

int main()
{
	xnet::proactor proactor_;
	auto acceptor = proactor_.get_acceptor();
  auto ret = acceptor.bind("0.0.0.0", 9001);
	assert(ret);
	acceptor.regist_accept_callback([](xnet::connection && conn) {
     //conn accept
		conn.regist_recv_callback([](void *data, int) {
    //recv data .
		});
		conn.async_recv(10);
	});
	auto connector = proactor_.get_connector();
	connector.
		bind_fail_callback([](std::string str) {
    //connect fail
		std::cout << str.c_str() << std::endl;
	}).
		bind_success_callback([](xnet::connection &&conn) 
	{ 
    //connect ok;
		conn.close();
	}).
		sync_connect("127.0.0.1", 9002);

	proactor_.run();
	return 0;
}
