#pragma once
namespace xnet
{
namespace detail
{
namespace iocp
{
class socket_exception :std::exception
{
public:
	explicit socket_exception(int error_code)
		: error_code_(error_code)
	{

	}
	explicit socket_exception(const std::string &error_str)
		:error_str_(error_str)
	{

	}
private:
	std::string error_str_;
	int error_code_;
};

class overLapped_context : OVERLAPPED
{
public:
	overLapped_context()
	{
		memset(this, 0, sizeof(OVERLAPPED));
	}

	enum
	{
		E_CLOSE = -1,
		E_ACCEPT = 10,
		E_READ,
		E_WRITE
	} type_;

	std::vector<uint8_t> buffer_;

	WSABUF WSABuf_;

	class acceptor_impl *acceptor_;

	class connection_impl *connection_;

	void load(std::vector<uint8_t> &data)
	{
		buffer_.swap(data);
		WSABuf_.buf = (CHAR*)buffer_.data();
		WSABuf_.len = (ULONG)buffer_.size();
	}
	void load(int len)
	{
		buffer_.resize(len);
		WSABuf_.buf = (CHAR*)buffer_.data();
		WSABuf_.len = (ULONG)buffer_.size();
	}
};


class connection_impl
{
public:
	connection_impl(SOCKET sock)
	{
		overlapped_context_ = new overLapped_context;
		overlapped_context_->connection_ = this;
	}
	~connection_impl()
	{

	}
	void close()
	{

	}
	template<typename READCALLBACK>
	void bind_recv_callback(READCALLBACK callback)
	{
		recv_callback_ = callback;
	}
	template<typename SENDCALLBACK>
	void bind_send_callback(SENDCALLBACK callback)
	{
		send_callback_ = callback;
	}
	void async_send(std::vector<uint8_t> &data)
	{
		DWORD dwBytes = 0;
		overlapped_context_->load(data);
		if (WSASend(socket_, 
			&overlapped_context_->WSABuf_, 
			1, 
			&dwBytes, 
			0, 
			(LPOVERLAPPED)overlapped_context_, 
			NULL) == SOCKET_ERROR)
		{
			throw socket_exception(WSAGetLastError());
		}
	}
	void async_recv(int len)
	{
		DWORD dwBytes = 0, dwFlags = 0;

		if (WSARecv(
			socket_,
			&overlapped_context_->WSABuf_,
			1,
			&dwBytes,
			&dwFlags,
			(LPOVERLAPPED)overlapped_context_,
			NULL) == SOCKET_ERROR)
		{
			throw socket_exception(WSAGetLastError());
		}

	}
private:
	friend class proactor_impl;
	void write_callbak(DWORD bytes)
	{
	
	}
	
	void send_callback(DWORD bytes)
	{
	
	}
	std::function<void(void *, int)> recv_callback_;
	std::function<void(int)> send_callback_;

	overLapped_context *overlapped_context_;
	SOCKET socket_ = INVALID_SOCKET;
};
class acceptor_impl
{
public:
	acceptor_impl()
	{
		context_ = new overLapped_context;
		context_->type_ = overLapped_context::E_ACCEPT;
	}
	~acceptor_impl()
	{
		if (listenSocket_ != INVALID_SOCKET)
			closesocket(listenSocket_);
		if(accept_socket_ != INVALID_SOCKET)
			closesocket(accept_socket_);
	}
	template<class accept_callback_t>
	void bind(const std::string &ip, int port, accept_callback_t callback)
	{
		acceptor_callback_ = callback;
		listenSocket_ = WSASocket(AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,
			0,
			WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == listenSocket_)
		{
			throw socket_exception(WSAGetLastError());
		}

		struct sockaddr_in serverAddress;
		ZeroMemory((char *)&serverAddress, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
		serverAddress.sin_port = htons(port);

		if (SOCKET_ERROR == ::bind(listenSocket_,
			(struct sockaddr *) &serverAddress,
			sizeof(serverAddress)))
		{
			closesocket(listenSocket_);
			listenSocket_ = INVALID_SOCKET;
			throw socket_exception(WSAGetLastError());
		}

		if (SOCKET_ERROR == listen(listenSocket_, SOMAXCONN))
		{
			closesocket(listenSocket_);
			listenSocket_ = INVALID_SOCKET;
			throw socket_exception(WSAGetLastError());
		}

		DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		if (WSAIoctl(listenSocket_,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx,
			sizeof(GuidAcceptEx),
			&acceptex_func_,
			sizeof(acceptex_func_),
			&dwBytes,
			NULL,
			NULL) == SOCKET_ERROR)
		{
			throw socket_exception(WSAGetLastError());
		}
		assert(acceptex_func_);

		if (::CreateIoCompletionPort((HANDLE)listenSocket_,
			IOCompletionPort_,
			0,
			0) != IOCompletionPort_)
		{
			assert(false);
		}
	}
	void accept_callback()
	{
		if (setsockopt(
			accept_socket_,
			SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			(char *)&listenSocket_,
			sizeof(listenSocket_)) != 0)
		{
			throw socket_exception(WSAGetLastError());
			return;
		}
		connection_impl *con = new connection_impl(accept_socket_);
		do_accept();
	}
	void close()
	{
		context_->type_ = overLapped_context::E_CLOSE;
		delete this;
	}
private:
	friend class proactor_impl;
	void do_accept()
	{
		accept_socket_ = WSASocket(AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,
			0,
			WSA_FLAG_OVERLAPPED);

		if (accept_socket_ == INVALID_SOCKET)
		{
			throw socket_exception(GetLastError());
		}

		DWORD address_size = sizeof(sockaddr_in) + 16;
		char addr_buffer[128];
		if (acceptex_func_(listenSocket_,
			accept_socket_, 
			addr_buffer, 
			128 - address_size * 2,
			address_size, 
			address_size, 
			&bytesReceived_, 
			(OVERLAPPED*)context_) == false)
		{
			DWORD lastError = GetLastError();
			if (lastError != ERROR_IO_PENDING)
			{
				throw socket_exception(GetLastError());
			}
		}
	}
	DWORD bytesReceived_ = 0;
	LPFN_ACCEPTEX acceptex_func_ = NULL;
	SOCKET listenSocket_ = INVALID_SOCKET;
	SOCKET accept_socket_ = INVALID_SOCKET;
	HANDLE IOCompletionPort_ = NULL;
	overLapped_context *context_ = NULL;
	std::function<void(connection_impl*)> acceptor_callback_;
};

class proactor_impl
{
public:
	proactor_impl()
	{

	}
	void init()
	{
		static std::once_flag once;
		std::call_once(once, [] {
			WSADATA wsaData;
			int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (NO_ERROR != nResult)
			{
				throw socket_exception(nResult);
			}
		});
		IOCompletionPort_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (IOCompletionPort_ == NULL)
		{
			throw socket_exception(GetLastError());
		}
	}
	void run()
	{
		while (is_stop_ == false)
		{
			void *key = NULL;
			overLapped_context *overlapped = NULL;
			DWORD bytes = 0;
			DWORD timeout = INFINITE;

			if (FALSE == GetQueuedCompletionStatus(
				IOCompletionPort_,
				&bytes,
				(PULONG_PTR)&key,
				(LPOVERLAPPED*)&overlapped,
				timeout))
			{
				if (GetLastError() == WAIT_TIMEOUT)
					continue;
			}
			assert(overlapped);

			switch (overlapped->type_)
			{
			case overLapped_context::E_ACCEPT:
				overlapped->acceptor_->accept_callback();
				break;
			case overLapped_context::E_READ:
				overlapped->connection_->send_callback(bytes);
				break;
			case overLapped_context::E_WRITE:
				overlapped->connection_->write_callbak(bytes);
				break;
			case overLapped_context::E_CLOSE:
				delete overlapped;
			default:
				break;
			}
		}
	}
	void stop()
	{
		is_stop_ = true;
	}
	acceptor_impl *get_acceptor()
	{
		acceptor_impl *acceptor = new acceptor_impl;
		if (IOCompletionPort_ == NULL)
			throw socket_exception("IOCompletionPort_ null");
		acceptor->IOCompletionPort_ = IOCompletionPort_;
		return acceptor;
	}
private:
	bool is_stop_ = false;
	HANDLE IOCompletionPort_ = NULL;

};
}
}
}