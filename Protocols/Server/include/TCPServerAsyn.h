#pragma once
/*
 1) Acts as a server in the client-server communication model
 2) Communicates with client applications over TCP protocol
 3) Uses the asynchronous I/O and control operations
 4) May handle multiple clients simultaneously
*/
#include <boost/asio.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>

using namespace boost;
using tcp = asio::ip::tcp;
namespace net = asio;
using socket_ptr = std::shared_ptr<tcp::socket>;
using port = unsigned short;

class Service {
	socket_ptr m_sock;
	std::string m_response;
	net::streambuf m_request;
public:
	Service(socket_ptr sock) : m_sock(sock) {}

	void StartHandling() {
		net::async_read_until(*m_sock.get(), m_request, "\n", [this](
			const boost::system::error_code& ec, std::size_t bytes_trans
			) {
				onRequestReceived(ec, bytes_trans);
			});
	}
private:
	void onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_trans)
	{
		if (ec) {
			std::cout << "Error occured! Error code = "
				<< ec.value()
				<< ". Message: " << ec.message();
			onFinish();
			return;
		}
		m_response = ProcessRequest(m_request);

		net::async_write(*m_sock.get(), net::buffer(m_response), [this](
			const boost::system::error_code& ec, std::size_t bytes_trans
			)
			{
				onResponseSent(ec, bytes_trans);
			});
	}
	void onResponseSent(const boost::system::error_code& ec, std::size_t bytes_trans)
	{
		if (ec) {
			std::cout << "Error occured! Error code = "
				<< ec.value()
				<< ". Message: " << ec.message();
		}
		onFinish();
	}
	void onFinish() {
		delete this;
	}

	std::string ProcessRequest(net::streambuf& request) {
		int i = 0;
		while (i != 1000000) {
			++i;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			std::string response = "ResponseFromLocale\n";
			return response;
		}
	}
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
		if (!ec) {
			(new Service(sock))->StartHandling();
		}
		else {
			std::cout << "Error occured! Error code = "
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

void StartAsynServer()
{
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