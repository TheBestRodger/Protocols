#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501
#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 or earlier.
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include <map>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

namespace net   = boost::asio;
using tcp = net::ip::tcp;
using namespace boost;
using socket_ptr = std::shared_ptr<tcp::socket>;
using port = unsigned short;
using ref_sys_error = const boost::system::error_code&;
class Service{
    static const std::map<unsigned int, std::string> http_status_table;
public:
    Service(socket_ptr sock):
        m_sock(sock),
        m_request(4096),
        m_response_status_code(200),
        m_resource_size_bytes(0)
        {}
    void start_handling()
    {
        net::async_read_until(*m_sock.get(), m_request, "\r\n",[this](ref_sys_error ec, std::size_t bytes_transferred)
        {
            on_request_line_received(ec, bytes_transferred);
        });
    }
private:
    void on_request_line_received(ref_sys_error ec, std::size_t bytes_transferred)
    {
        if(ec.value())
        {
            std::cout << "Error occured! Error code = "
            << ec.value()
            << ". Message: " << ec.message();
            if (ec == asio::error::not_found) {
                // No delimiter has been found in the
                // request message.
                m_response_status_code = 413;
                send_response();
                return;
            }
            else
            {
            // In case pf any other error
            // close the socket and clean up.
            on_finish();
            return;
            }
        }
        std::string request_line;
        std::istream request_stream(&m_request);
        std::getline(request_stream, request_line, '\r');
        // Remove symbol '\n' from the buf.
        request_stream.get();

        // Parse the request line.
        std::string request_method;
        std::istringstream requset_line_stream(request_line);
        requset_line_stream >> request_method;

        // Only supp GET method (for now 01.03.24).
        if(request_method.compare("GET") != 0){
            // Unsupported method.
            m_response_status_code = 501;
            send_response();
            return;
        }

        requset_line_stream >> m_request_resource;

        std::string request_http_version;
        requset_line_stream >>request_http_version;

        if(request_http_version.compare("HTTP/1.1") != 0){
            // Unsupp HTTP vesrion or bad request.

            m_response_status_code = 505;
            send_response();
            return;
        }

        // At this point the request linr is successfully
        // received and parsed. Read the req head.

        net::async_read_until(*m_sock.get(), m_request, "\r\n\r\n",
                            [this](ref_sys_error ec, std::size_t bytes_transferred)
                            {
                                on_headers_received(ec, bytes_transferred);
                            });
        return;
    }
    void on_headers_received(ref_sys_error ec, std::size_t bytes_transferred)
    {
        if (ec.value()) {
            std::cout << "Error occured! Error code = "
            << ec.value()
            << ". Message: " << ec.message();
            if (ec == asio::error::not_found) {
                // No delimiter has been fonud in the
                // request message.
                m_response_status_code = 413;
                send_response();
                return;
            }
            else {
                // In case of any other error - close the
                // socket and clean up.
                on_finish();
                return;
            }        
        }
        std::istream request_stream(&m_request);
        std::string header_name, header_value;

        while(!request_stream.eof()){
            std::getline(request_stream, header_name, ':');
            if(!request_stream.eof()){
                std::getline(request_stream, header_value,'\r');
                request_stream.get();
                m_request_headers[header_name] = header_value;
            }
        }
        process_request();
        send_response();

        return;
    }
    void process_request()
    {
        // Read file.
        std::string resource_file_path = std::string("D:/Projects/Protocols/Server/Files") + m_request_resource;

        if(!filesystem::exists(resource_file_path)){
            m_response_status_code = 404;
            return;
        }
        std::ifstream resource_fstream(resource_file_path, std::ifstream::binary);
        if(!resource_fstream.is_open()){
            // Could not open file.
            // Smth goes wrong.
            m_response_status_code = 500;
            return;
        }

        // Find out file size.
        resource_fstream.seekg(0, std::ifstream::end);
        m_resource_size_bytes = static_cast<std::size_t>(resource_fstream.tellg());// metka

        m_resource_buffer.reset(new  char[m_resource_size_bytes]);
        resource_fstream.seekg(std::ifstream::beg);
        resource_fstream.read(m_resource_buffer.get(), m_resource_size_bytes);

        m_response_headers += std::string("From HTPP SERVER") + ": " + std::to_string(m_resource_size_bytes) + "\r\n";
    }
    void send_response()
    {
        m_sock->shutdown(tcp::socket::shutdown_receive);

        auto status_line = http_status_table.at(m_response_status_code);

        m_response_status_line = std::string("HTTP/1.1 ") + status_line + "\r\n";

        m_response_headers += "\r\n";

        std::vector<net::const_buffer> response_buffers;
        response_buffers.push_back(net::buffer(m_response_status_line));

        if(m_response_headers.length() >0){
            response_buffers.push_back(net::buffer(m_response_headers));
        }

        if(m_resource_size_bytes > 0){
            response_buffers.push_back(net::buffer(m_resource_buffer.get(), m_resource_size_bytes));
        }

        // Ini asyn write operation.

        net::async_write(*m_sock.get(), response_buffers, 
                        [this](ref_sys_error ec, std::size_t bytes_transferred)
                        {
                            on_response_sent(ec, bytes_transferred);
                        });
    }
    void on_response_sent(ref_sys_error ec, std::size_t bytes_transferred)
    {
        if (ec.value()) {
            std::cout << "Error occured! Error code = "
            << ec.value()
            << ". Message: " << ec.message();
        }
        m_sock->shutdown(tcp::socket::shutdown_both);
        on_finish();
    }
    void on_finish()
    {
        delete this;
    }
private:
    socket_ptr m_sock;
    net::streambuf m_request;
    std::map<std::string, std::string> m_request_headers;
    std::string m_request_resource;

    std::unique_ptr<char[]> m_resource_buffer;
    unsigned int m_response_status_code;
    std::size_t m_resource_size_bytes;
    std::string m_response_headers;
    std::string m_response_status_line;
};

const std::map<unsigned int, std::string> Service::http_status_table =
{
    { 200, "200 OK" },
    { 404, "404 Not Found" },
    { 413, "413 Request Entity Too Large" },
    { 500, "500 Server Error" },
    { 501, "501 Not Implemented" },
    { 505, "505 HTTP Version Not Supported" }
};

class Acceptor {
	net::io_context& m_ios;
	tcp::acceptor m_acceptor;
	std::atomic<bool> m_isStopped;
public:
	Acceptor(net::io_context &ios, port port):
		m_ios(ios),
		m_acceptor(m_ios, tcp::endpoint(net::ip::address_v4::any(), port)),
		m_isStopped(false){}

	void Start() {
		m_acceptor.listen();
		InitAccept();
	}

	void Stop() {
		m_isStopped.store(true);
	}

private:
	void InitAccept() {
		socket_ptr sock(new tcp::socket(m_ios));
		m_acceptor.async_accept(*sock.get(), [this, sock](const system::error_code& error)
			{
				onAccept(error, sock);
			});
	}

	void onAccept(const system::error_code& ec, socket_ptr sock)
	{
		if (ec.value() == 0) {
			(new Service(sock))->start_handling();
		}
		else {
			std::cout << "( onAccept )Error occured! Error code = "
				<< ec.value()
				<< ". Message: " << ec.message();
		}
		// Init next async accept operation if
		// acceptor has not been stopped yet.
		if (!m_isStopped.load()) {
			InitAccept();
		}
		else {
			// Stop accepting incoming connections
			// and free allocated resources.
			m_acceptor.close();
		}
	}
};

class Server {
	net::io_context m_ios;
	std::unique_ptr<net::io_context::work> m_work;
	std::unique_ptr<Acceptor> acc;
	std::vector<std::unique_ptr<std::thread>> m_thread_pool;
public:
	Server()
	{
		m_work.reset(new net::io_context::work(m_ios));
	}

	void Start(unsigned short port, unsigned thread_pool_size)
	{
		assert(thread_pool_size > 0);
        std::cout << "Server Starting...\n";
		acc.reset(new Acceptor(m_ios, port));
		acc->Start();

		for (int i = 0; i < thread_pool_size; ++i)
		{
			std::unique_ptr<std::thread> th(
				new std::thread([this]()
					{
						m_ios.run();
					}));
			m_thread_pool.push_back(std::move(th));
		}
	}

	void Stop() {
		acc->Stop();
		m_ios.stop();

		for (auto& th : m_thread_pool) {
			th->join();
		}
	}
};

void StartHTTPServer()
{
    setlocale(LC_ALL, "Russian");
	const unsigned DEFAULT_THREAD_POOL_SIZE = 2;
	unsigned short port = 12;
	try {
		Server srv;
		unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
		if (thread_pool_size == 0)
			thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
		srv.Start(port, thread_pool_size);
		std::this_thread::sleep_for(std::chrono::seconds(60));
		srv.Stop();
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = "
			<< e.code() << ". Message: "
			<< e.what();
	}
}