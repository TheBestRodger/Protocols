#pragma once
/*
	1) Acts as a server in the client-server communication model
	2) Communicates with client applications over TCP protocol
	3) Uses I/O and control operations that block the thread of execution until the
	   corresponding operation completes, or an error occurs
	4) May handle more than one client simultaneously
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
class Service {
public:
	Service() {}

	void StartHandligClient(std::shared_ptr<tcp::socket> sock) {
		std::thread th(([this, sock]() {
			HandleClient(sock);
			}));
		th.detach();
	}
private:
	void HandleClient(socket_ptr sock) {
		try {
			net::streambuf request;
			net::read_until(*sock.get(), request, "\n");

			std::istream strm(&request);
			std::string g;
			std::getline(strm, g);

			std::cout << g << std::endl;

			std::this_thread::sleep_for(std::chrono::microseconds(500));

			std::string response = "Hello SIM1-DEV\n";
			net::write(*sock.get(), net::buffer(response));
		}
		catch (system::system_error& e) {
			std::cout << "Error occured! Error code = " << e.code() << " . Message: " << e.what() << std::endl;;
		}
		delete this;
	}
};

class Acceptor {
	net::io_context& m_ios;
	tcp::acceptor m_acceptor;
public:
	Acceptor(net::io_context& ios, unsigned short port) :
		m_ios(ios),
		m_acceptor(m_ios, tcp::endpoint(net::ip::address_v4::any(), port))
	{
		m_acceptor.listen();
	}

	void Accept() {
		socket_ptr sock(new tcp::socket(m_ios));
		m_acceptor.accept(*sock.get());

		(new Service)->StartHandligClient(sock);
	}
};

class Server {
	std::unique_ptr<std::thread> m_thread;
	std::atomic<bool> m_stop;
	net::io_context m_ios;
public:

	Server() : m_stop(false) {}

	void Start(unsigned short port) {
		m_thread.reset(new std::thread([this, port]() {
			Run(port);
			}));
	}
	void Stop() {
		m_stop.store(true);
		m_thread->join();
	}
private:
	void Run(unsigned short port) {
		Acceptor acc(m_ios, port);
		while (!m_stop.load())
			acc.Accept();
	}
};

int StartTCPSynPar() {
	
	unsigned short port = 12;
	try {
		Server srv;

		srv.Start(port);
		std::this_thread::sleep_for(std::chrono::seconds(60));
		srv.Stop();
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = "
			<< e.code() << ". Message: "
			<< e.what();
	}
	return 0;
}