#include <string>
#include <memory>
#include <functional>
#ifdef _WIN32
#include <winsock.h>
#endif
#include <iostream>
#include <mutex>
#include <vector>

class xnet
{

public:
	class socket_t :std::enable_shared_from_this<socket_t>
	{
	public:
		int send(const char *data, int len)
		{

		}
		template<class CompletonHandler >
		void async_send(const char *data, int len, CompletonHandler handle)
		{

		}
		int recv(char *buf, int len)
		{

		}
		template<class CompletonHandler >
		void async_recv(char *buf, int len, CompletonHandler handle)
		{

		}
	public:
		xnet &xnet_;
	};
	xnet()
	{

	}
	template<class AcceptCallBack>
	bool bind(AcceptCallBack handle, const std::string &ip, int port)
	{
		SOCKET listenSocket = WSASocket(
			AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if(listenSocket == INVALID_SOCKET)
		{
			std::cout << " WSASocket() failed. Error:" << GetLastError() << std::endl;
			return false;
		}
		CreateIoCompletionPort((HANDLE)listenSocket, completion_handle_, (u_long)0, 0);
		SOCKADDR_IN internetAddr;
		internetAddr.sin_family = AF_INET;
		internetAddr.sin_addr.s_addr = inet_addr(ip.c_str());
		internetAddr.sin_port = htons(port);
		if(bind(listenSocket, (PSOCKADDR)&internetAddr, sizeof(internetAddr)) == SOCKET_ERROR)
		{
			std::cout << "Bind failed. Error:" << GetLastError() << std::endl;
			return;
		}
		if(listen(listenSocket, 5) == SOCKET_ERROR)
		{
			std::cout << "listen failed. Error:" << GetLastError() << std::endl;
			return;
		}
	}
	bool run()
	{
		for(;;)
		{
			void *key = NULL;
			OVERLAPPED *overlapped = NULL;
			DWORD bytesTransferred = 0;

			BOOL completionStatus = GetQueuedCompletionStatus(
				completion_handle_,
				&bytesTransferred,
				(LPDWORD)&key,
				&overlapped,
				INFINITE);

			if(FALSE == completionStatus)
			{
				continue;
			}

			// NULL key packet is a special status that unblocks the worker
			// thread to initial a shutdown sequence. The thread should be going
			// down soon.
			if(NULL == key)
			{
				break;
			}

		}
	}
private:
	typedef std::shared_ptr<socket_t> socket_ptr_t;

	void init()
	{
		static std::once_flag once;
		std::call_once(once, [] {
			WSADATA wsaData;
			DWORD ret;
			if(ret = WSAStartup(0x0202, &wsaData) != 0)
			{
				std::cout << "WSAStartup failed. Error:" << ret << std::endl;
				return;
			}
		});

		completion_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if(completion_handle_ == NULL)
		{
			std::cout << "CreateIoCompletionPort failed. Error:" << GetLastError() << std::endl;
			return;
		}
	}
	struct listener
	{
		std::function<bool(socket_ptr_t)> accept_callback_;
		std::string ip;
		int port;
		SOCKET fd_;
	};
	HANDLE completion_handle_ = NULL;
	std::vector<std::shared_ptr<listener>> listeners_;
};




int main()
{

}