#include <algorithm>
#ifdef _WIN32
#define fd_t SOCKET
#elif
#define INVALID_SOCKET -1
typedef int fd_t;
#endif


namespace xnet
{
namespace select
{
	class socket_exception :std::exception
	{
	public:
		socket_exception()
		{

		}
		const char *str()
		{
			return error_str_.c_str();
		}
	private:
		std::string error_str_;
	};
	
	class io_context
	{
	public:
		enum
		{
			e_read = 1,
			e_write = 2,
			e_connect = 4,
			e_accept = 8,
			e_close = 16,
			e_idle = 32,
			e_stop = 64

		}status_;

		fd_t fd_ = INVALID_SOCKET;
		std::vector<uint8_t> buffer_;
		uint32_t to_read_;
		uint32_t read_bytes_;
		uint32_t read_pos_;

		uint32_t to_write_;
		uint32_t write_bytes_;
		uint32_t write_pos_;

		class connection_impl *connection_ = NULL;
		class acceptor_impl *acceptor_ = NULL;
		class connector_impl *connector_ = NULL;
	};

	class connection_impl
	{
	public:
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

		}
		void async_recv(uint32_t len)
		{

		}
	private:
		std::function<void(void *, int)> recv_callback_;
		std::function<void(int)> send_callback_;
	};
	class acceptor_impl
	{
	public:
		template<class accept_callback_t>
		void regist_accept_callback(accept_callback_t callback)
		{
			acceptor_callback_ = callback;
		}
		void bind(const std::string &ip, int port)
		{

		}
		void close()
		{

		}
	private:
		friend class proactor_impl;

		void on_accept(bool result)
		{ 

		}
		fd_t fd_;
		
		std::function<void(connection_impl*)> acceptor_callback_;
	};
	class connector_impl
	{
	public:
		connector_impl()
		{ }
		~connector_impl()
		{ }
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
		void sync_connect(const std::string &ip, int port)
		{

		}
		void close()
		{

		}
	private:
		friend class proactor_impl;

		void on_connect(bool result)
		{

		}
		std::function<void(connection_impl *)> success_callback_;
		std::function<void(std::string)> failed_callback_;
	};

	class proactor_impl
	{
	private:

		struct fd_context
		{
			fd_t fd;
			io_context io_context_;
		};

	public:
		proactor_impl()
		{

		}
		void init()
		{

		}
		void run()
		{
			while(!is_stop_)
			{
				uint32_t timeout = 0;
				memcpy(&readfds, &source_set_in, sizeof source_set_in);
				memcpy(&writefds, &source_set_out, sizeof source_set_out);
				memcpy(&exceptfds, &source_set_err, sizeof source_set_err);

				//  Wait for events.
#ifdef _OSX
				struct timeval tv = { (long)(timeout / 1000), timeout % 1000 * 1000 };
#else
				struct timeval tv = { (long)(timeout / 1000),
					(long)(timeout % 1000 * 1000) };
#endif

#ifdef _WIN32
				int rc = ::select(0, &readfds, &writefds, &exceptfds,
								timeout ? &tv : NULL);
				assert(rc != SOCKET_ERROR);
#else
				int rc = ::select(maxfd + 1, &readfds, &writefds, &exceptfds,
								timeout ? &tv : NULL);
				if(rc == -1)
				{
					assert(errno == EINTR);
					continue;
				}
#endif
				if(rc == 0)
					continue;

				for(fd_set_t::size_type i = 0; i < fds.size(); i++)
				{
					if(fds[i].fd == INVALID_SOCKET)
						continue;
					if(FD_ISSET(fds[i].fd, &exceptfds))
					if(fds[i].fd == INVALID_SOCKET)
						continue;
					if(FD_ISSET(fds[i].fd, &writefds))
						writeable_callback(fds[i]);
					if(fds[i].fd == INVALID_SOCKET)
						continue;
					if(FD_ISSET(fds[i].fd, &readfds))
						readable_callback(fds[i]);
				}

				if(retired)
				{
					fds.erase(std::remove_if(fds.begin(), fds.end(), [](fd_context &entry)
					{
						return entry.fd == INVALID_SOCKET;
					}), fds.end());
					retired = false;
				}
			}
		}
		void stop()
		{

		}
		acceptor_impl *get_acceptor()
		{
			return NULL;
		}
		connector_impl *get_connector()
		{
			return NULL;
		}

		void writeable_callback(fd_context entry)
		{
			io_context &io_ctx = entry.io_context_;

			if(io_ctx.status_ == io_context::e_write)
			{

			}
			else if(io_ctx.status_ == io_context::e_accept)
			{
				io_ctx.acceptor_->on_accept(true);
			}
			else if(io_ctx.status_ == io_context::e_connect)
			{
				io_ctx.connector_->on_connect(true);
			}
		}

		void readable_callback(fd_context fds)
		{

		}


	private:
		typedef std::vector <fd_context> fd_set_t;
		fd_set_t fds;

		fd_set source_set_in;
		fd_set source_set_out;
		fd_set source_set_err;

		fd_set readfds;
		fd_set writefds;
		fd_set exceptfds;

		fd_t maxfd;

		bool retired;

		bool is_stop_;
	};

}
}