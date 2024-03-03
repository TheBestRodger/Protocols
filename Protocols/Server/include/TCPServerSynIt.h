#pragma once
/*
* Synchoronous iterative TCP server mus satisfies the following criteria
* 
* 1) Acts as a server in the client-server communication model
* 2) Communicates with client app over TCP protocol
* 3) Uses I/O and control operation thar block the thread of execution until op completes/err
* 4) Handles clients in a serial? one-bu-one fashion
*/
#include <boost/asio.hpp>

#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
using namespace boost;
using tcp = asio::ip::tcp;
namespace net = asio;
class Service {
public:
	Service() {}

	void HandleClient(tcp::socket& sock) {
		try {
			net::streambuf request;
			net::read_until(sock, request, '\n');

			std::istream strm(&request);
			std::string g;
			std::getline(strm, g);
			std::cout << g << std::endl;

			std::this_thread::sleep_for(std::chrono::microseconds(500));

			std::string response = "Hello From SIM-DEV\n";
			net::write(sock, net::buffer(response));
			
		}
		catch (system::system_error& e) {
			std::cout << "Error occured! Error code = " << e.code() << " . Message: " << e.what() << std::endl;;
		}
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
		tcp::socket sock(m_ios);
		m_acceptor.accept(sock);

		Service svc;
		svc.HandleClient(sock);
	}
};

class Server {
	std::unique_ptr<std::thread> m_thread;
	std::atomic<bool> m_stop;
	net::io_context m_ios;
public:
	Server():m_stop(false){}

	void Start(unsigned short port) {
		m_thread.reset(new std::thread([this, port]()
			{
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


void StartTCPServerSyn() {
	unsigned short port = 12;
	setlocale(LC_ALL, "Russian");
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
}