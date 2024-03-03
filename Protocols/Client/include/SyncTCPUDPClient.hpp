#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/array.hpp>

namespace json  = boost::json;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

using tcp = net::ip::tcp;
using namespace boost;
namespace syn{
class SyncTCPClient{
private: 
    net::io_service m_ios;
    tcp::endpoint m_ep;
    tcp::socket m_sock;
public:
    SyncTCPClient(const std::string& ip_address, unsigned short port):
    m_ep(net::ip::address::from_string(ip_address), port), m_sock(m_ios)
    {
        m_sock.open(m_ep.protocol());
    }
    void connect()
    {
        m_sock.connect(m_ep);
    }

    void close(){
        m_sock.shutdown(
            tcp::socket::shutdown_both
        );
        m_sock.close();
    }

    std::string emulateLongComputationOp(unsigned int duration_sec){
        std::string request = "Client Send this: " + std::to_string(duration_sec) + "\n";

        sendRequest(request);
        return recieveResponse();
    }
private:
    void sendRequest(const std::string& request){
        net::write(m_sock, net::buffer(request));
    }
    std::string recieveResponse(){
        net::streambuf buf;
        net::read_until(m_sock, buf, '\n');
        std::istream input(&buf);
        std::string response;
        std::getline(input, response);

        return response;
    }

};
class SyncUDPClient{
public:
    SyncUDPClient() : m_sock(m_ios) {
        m_sock.open(asio::ip::udp::v4());
    }
    std::string emulateLongComputationOp( unsigned int duration_sec, const std::string& raw_ip_address,
        unsigned short port_num) 
        {
            std::string request = "EMULATE_LONG_COMP_OP " + std::to_string(duration_sec) + "\n";
            asio::ip::udp::endpoint ep( asio::ip::address::from_string(raw_ip_address), port_num);
            sendRequest(ep, request);
            return receiveResponse(ep);
        }
private:
    void sendRequest(const asio::ip::udp::endpoint& ep, const std::string& request) {
        m_sock.send_to(asio::buffer(request), ep);
    }
    std::string receiveResponse(asio::ip::udp::endpoint& ep) { 
        char response[6]; 
        std::size_t bytes_recieved =
        m_sock.receive_from(asio::buffer(response), ep);
        m_sock.shutdown(asio::ip::udp::socket::shutdown_both);
        return std::string(response, bytes_recieved);
    }
private:
    asio::io_service m_ios;
    asio::ip::udp::socket m_sock;
};
}
int StartTCPSync(){
    const std::string ip = "10.25.106.45";
    unsigned short port = 12;
    try
    {
        syn::SyncTCPClient client(ip, port);
        client.connect();
        std::cout << "Sending request to the server... " << std::endl;
        
        std::string response = client.emulateLongComputationOp(100);
        
        std::cout << "Response received: " << response << std::endl;
        // Close the connection and free resources.
        client.close();
    }
    catch (system::system_error &e) {
        std::cout << "Error occured! Error code = " << e.code()
        << ". Message: " << e.what();
        return e.code().value();
    }
    return 0;
}

int StartUDPClient()
{
    const std::string server1_raw_ip_address = "10.25.106.45";
    const unsigned short server1_port_num = 12;
    const std::string server2_raw_ip_address = "10.25.106.45";
    const unsigned short server2_port_num = 12;
    try {
        syn::SyncUDPClient client;
        std::cout << "Sending request to the server #1 ... " << std::endl;
        std::string response = client.emulateLongComputationOp(10, server1_raw_ip_address, server1_port_num);
        std::cout << "Response from the server #1 received: " << response << std::endl;
        std::cout << "Sending request to the server #2... " << std::endl;
        response = client.emulateLongComputationOp(10, server2_raw_ip_address, server2_port_num);
        std::cout << "Response from the server #2 received: " << response << std::endl;
    }
    catch (system::system_error &e) {
        std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
        return e.code().value();
    }
    return 0;
}