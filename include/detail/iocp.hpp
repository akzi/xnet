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
			E_CONNECT,
			E_READ,
			E_WRITE
		} type_;

		std::vector<uint8_t> buffer_;

		WSABUF WSABuf_;

		class acceptor_impl *acceptor_ = NULL;

		class connection_impl *connection_ = NULL;

		class connector_impl *connector_ = NULL;

		void reload(std::vector<uint8_t> &data)
		{
			buffer_.swap(data);
			WSABuf_.buf = (CHAR*)buffer_.data();
			WSABuf_.len = (ULONG)buffer_.size();
		}
		void reload(uint32_t len)
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
			overlapped_context_->reload(data);
			if(WSASend(socket_,
				&overlapped_context_->WSABuf_,
				1,
				&dwBytes,
				0,
				(LPOVERLAPPED)overlapped_context_,
				NULL) == SOCKET_ERROR)
			{
				if(WSAGetLastError() != WSA_IO_PENDING)
					throw socket_exception(WSAGetLastError());
			}
		}
		void async_recv(uint32_t len)
		{
			DWORD dwBytes = 0, dwFlags = 0;
			overlapped_context_->reload(len);
			if(WSARecv(
				socket_,
				&overlapped_context_->WSABuf_,
				1,
				&dwBytes,
				&dwFlags,
				(LPOVERLAPPED)overlapped_context_,
				NULL) == SOCKET_ERROR)
			{
				if(WSAGetLastError() != WSA_IO_PENDING)
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

		overLapped_context *overlapped_context_ = NULL;
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
			if(listener_sock_ != INVALID_SOCKET)
				closesocket(listener_sock_);
			if(accept_socket_ != INVALID_SOCKET)
				closesocket(accept_socket_);
		}
		template<class accept_callback_t>
		void bind(const std::string &ip, int port, accept_callback_t callback)
		{
			acceptor_callback_ = callback;
			if(listener_sock_ != INVALID_SOCKET)
				closesocket(listener_sock_);
			listener_sock_ = WSASocket(AF_INET,
										SOCK_STREAM,
										IPPROTO_TCP,
										NULL,
										0,
										WSA_FLAG_OVERLAPPED);

			if(INVALID_SOCKET == listener_sock_)
			{
				throw socket_exception(WSAGetLastError());
			}

			struct sockaddr_in addr;
			ZeroMemory((char *)&addr, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(ip.c_str());
			addr.sin_port = htons(port);

			if(SOCKET_ERROR == ::bind(listener_sock_,
				(struct sockaddr *) &addr,
				sizeof(addr)))
			{
				throw socket_exception(WSAGetLastError());
			}

			if(SOCKET_ERROR == listen(listener_sock_, SOMAXCONN))
			{
				throw socket_exception(WSAGetLastError());
			}

			DWORD dwBytes = 0;
			GUID GuidAcceptEx = WSAID_ACCEPTEX;
			if(WSAIoctl(listener_sock_,
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

			if(::CreateIoCompletionPort((HANDLE)listener_sock_,
				IOCompletionPort_,
				0,
				0) != IOCompletionPort_)
			{
				throw socket_exception(WSAGetLastError());
			}
			
		}
		void accept_callback()
		{
			if(setsockopt(accept_socket_,
				SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&listener_sock_,
				sizeof(listener_sock_)) != 0)
			{
				throw socket_exception(WSAGetLastError());
				return;
			}
			connection_impl *con = new connection_impl(accept_socket_);
			accept_socket_ = INVALID_SOCKET;
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
			if(accept_socket_ != INVALID_SOCKET)
				closesocket(accept_socket_);
			accept_socket_ = WSASocket(AF_INET,
										SOCK_STREAM,
										IPPROTO_TCP,
										NULL,
										0,
										WSA_FLAG_OVERLAPPED);

			if(accept_socket_ == INVALID_SOCKET)
			{
				throw socket_exception(GetLastError());
			}
			
			DWORD address_size = sizeof(sockaddr_in) + 16;
			char addr_buffer[128];
			if(acceptex_func_(listener_sock_,
				accept_socket_,
				addr_buffer,
				128 - address_size * 2,
				address_size,
				address_size,
				&bytesReceived_,
				(OVERLAPPED*)context_) == false)
			{
				DWORD lastError = GetLastError();
				if(lastError != ERROR_IO_PENDING)
				{
					throw socket_exception(GetLastError());
				}
			}
		}
		DWORD bytesReceived_ = 0;
		LPFN_ACCEPTEX acceptex_func_ = NULL;
		SOCKET listener_sock_ = INVALID_SOCKET;
		SOCKET accept_socket_ = INVALID_SOCKET;
		HANDLE IOCompletionPort_ = NULL;
		overLapped_context *context_ = NULL;
		std::function<void(connection_impl*)> acceptor_callback_;
	};

	class connector_impl
	{
	public:
		connector_impl()
		{
			overLapped_context_ = new overLapped_context;
		}
		~connector_impl()
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);
		}
		template<typename SUCCESS_CALLBACK>
		void bind_success_callback(SUCCESS_CALLBACK callback)
		{
			success_callback_ = callback;
		}
		template<typename FAILED_CALLBACK>
		void bind_failed_callback(FAILED_CALLBACK callback)
		{
			failed_callback_ = callback;
		}
		void connect(const std::string &ip, int port)
		{
			if(socket_ != INVALID_SOCKET)
				closesocket(socket_);

			socket_ = WSASocket(AF_INET,
									  SOCK_STREAM,
									  IPPROTO_TCP,
									  NULL,
									  0,
									  WSA_FLAG_OVERLAPPED);
			if(socket_ == INVALID_SOCKET)
			{
				throw socket_exception(WSAGetLastError());
			}
			
			if(CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket_),
			   IOCompletionPort_, 
			   0, 
			   0) != IOCompletionPort_)
			{
				throw socket_exception(WSAGetLastError());
			}
			GUID connectex_guid = WSAID_CONNECTEX;
			DWORD bytes_returned;

			if(WSAIoctl(socket_,
			   SIO_GET_EXTENSION_FUNCTION_POINTER,
			   &connectex_guid,
			   sizeof(connectex_guid),
			   &connectex_func_,
			   sizeof(connectex_func_),
			   &bytes_returned,
			   NULL,
			   NULL) == SOCKET_ERROR)
			{
				throw socket_exception(WSAGetLastError());
			}

			struct sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = 0;
			if(bind(socket_, 
			   reinterpret_cast<SOCKADDR*>(&addr), 
			   sizeof(addr)) == SOCKET_ERROR)
			{
				throw socket_exception(WSAGetLastError());
			}

			struct sockaddr_in socket_address;
			ZeroMemory((char *)&socket_address, sizeof(socket_address));
			socket_address.sin_family = AF_INET;
			socket_address.sin_addr.s_addr = inet_addr(ip.c_str());
			socket_address.sin_port = htons(port);

			DWORD bytes = 0;
			const int connect_ex_result = connectex_func_(socket_,
				reinterpret_cast<SOCKADDR*>(&socket_address),
				sizeof(socket_address),
				NULL,
				0,
				&bytes,
				reinterpret_cast<LPOVERLAPPED>(overLapped_context_)
			);
		}
		void close()
		{
			if(socket_!= INVALID_SOCKET)
				closesocket(socket_);

			socket_ = INVALID_SOCKET;
			overLapped_context_->connector_ = NULL;
			overLapped_context_->type_ = overLapped_context::E_CLOSE;
			delete this;
		}
	private:
		friend class proactor_impl;
		void connect_callback(bool result_)
		{
			if(result_ == false)
			{
				failed_callback_("connect failed");
				return;
			}
			SOCKET sock = socket_;
			socket_ = INVALID_SOCKET;
			success_callback_(new connection_impl(sock));
		}
		overLapped_context *overLapped_context_;
		LPFN_CONNECTEX connectex_func_ = NULL;
		SOCKET socket_ = INVALID_SOCKET;
		std::function<void(connection_impl *)> success_callback_;
		std::function<void(std::string)> failed_callback_;
		HANDLE IOCompletionPort_ = NULL;
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
			std::call_once(once, []
			{
				WSADATA wsaData;
				int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if(NO_ERROR != nResult)
				{
					throw socket_exception(nResult);
				}
			});
			IOCompletionPort_ = CreateIoCompletionPort(
				INVALID_HANDLE_VALUE, NULL, 0, 0);
			if(IOCompletionPort_ == NULL)
			{
				throw socket_exception(GetLastError());
			}
		}
		void run()
		{
			while(is_stop_ == false)
			{
				void *key = NULL;
				overLapped_context *overlapped = NULL;
				DWORD bytes = 0;
				DWORD timeout = INFINITE;

				if(FALSE == GetQueuedCompletionStatus(
					IOCompletionPort_,
					&bytes,
					(PULONG_PTR)&key,
					(LPOVERLAPPED*)&overlapped,
					timeout))
				{
					if(GetLastError() == WAIT_TIMEOUT)
						continue;
				}
				assert(overlapped);

				switch(overlapped->type_)
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
			assert(IOCompletionPort_);
			acceptor->IOCompletionPort_ = IOCompletionPort_;
			return acceptor;
		}
		connector_impl *get_connector()
		{
			connector_impl *acceptor = new connector_impl;
			assert(IOCompletionPort_);
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