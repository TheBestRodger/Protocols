#pragma once

#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINT 0x0501
#if _WIN32_WINT <= 0x0502
    #define BOOST_ASIO_DISABLE_IOCP
    #define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <iostream>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>

// namespace json  = boost::json;
// namespace beast = boost::beast;
// namespace http  = beast::http;
namespace net   = boost::asio;
using tcp = net::ip::tcp;
using namespace boost;


typedef void(*Callback)(unsigned int request_id,
const std::string& response,
const system::error_code& ec);
/*
    Structure represent a context of single request
*/
struct Session{
    /*
        ctor
    */
    Session(net::io_context& ios,
    const std::string& ip_address,
    unsigned short port,
    const std::string& request,
    unsigned int id,
    Callback callback
    ):m_sock(ios),  m_ep(net::ip::address::from_string(ip_address), port),
                    m_request(request),m_id(id),m_callback(callback),m_was_cancelled(false){}

    tcp::socket m_sock; // Socket for communication
    tcp::endpoint m_ep; // Remote endpoint
    std::string m_request; // Request string

    net::streambuf m_response_buf; // where the response will be stored
    std::string m_response; // represented as a string

    system::error_code m_ec;// Contains the description of an error

    unsigned int m_id;// Unique ID assigned to the request

    Callback m_callback; // Pointer to the function to be called when the request completes

    bool m_was_cancelled;
    std::mutex m_cancel_guard;
};

class AsyncTCPClient{
private:
    net::io_context m_ios;
    std::map<int, std::shared_ptr<Session>> m_active_sessions;
    std::mutex m_active_sessions_guard;
    std::unique_ptr<net::io_context::work> m_work;
    std::list<std::unique_ptr<std::thread>> m_threads;
public:
    AsyncTCPClient(unsigned char num_of_threads){
        
        m_work.reset(new net::io_context::work(m_ios));
        for(unsigned char i = 1; i <= num_of_threads; ++i)
        {
            std::unique_ptr<std::thread> th
            {
                new std::thread([this](){m_ios.run();})
            };
            m_threads.push_back(std::move(th));
        }
    }

    void emulateLongComputationOp(unsigned duration_sec, const std::string& ip, unsigned short port, Callback callback, unsigned int request_id)
    {
        std::string request = "EMULATE_LONG_CALC_OP: " + std::to_string(duration_sec) + "\n";

        std::shared_ptr<Session> session = std::shared_ptr<Session>(new Session(m_ios, ip, port, request, request_id, callback));

        session->m_sock.open(session->m_ep.protocol());

        std::unique_lock<std::mutex> lock(m_active_sessions_guard);
        m_active_sessions[request_id] = session;
        lock.unlock();

        session->m_sock.async_connect(session->m_ep,[this, session](const system::error_code& ec)
        {
            if(ec){
                session->m_ec = ec;
                onRequestComplete(session);
                return;
            }
            
            std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);

            if(session->m_was_cancelled){
                onRequestComplete(session);
                return;
            }
            net::async_write(session->m_sock,
            net::buffer(session->m_request),
            [this, session](const boost::system::error_code& ec,
            std::size_t bytes_transferred)
            {
                if (ec) {
                    session->m_ec = ec;
                    onRequestComplete(session);
                    return;
                }
                std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);
                if (session->m_was_cancelled) {
                    onRequestComplete(session);
                    return;
                }
                net::async_read_until(session->m_sock, session->m_response_buf, '\n',
                [this, session](const boost::system::error_code& ec, std::size_t bytes_transferred)
                {
                    if (ec) {
                        session->m_ec = ec;
                    } 
                    else {
                        std::istream strm(&session->m_response_buf);
                        std::getline(strm, session->m_response);
                    }
                        onRequestComplete(session);
                });
            });
        });
    }

    void cancelRequest(unsigned request_id){
        std::unique_lock<std::mutex> lock(m_active_sessions_guard);
        auto it = m_active_sessions.find(request_id);
        if(it != m_active_sessions.end()){
            std::unique_lock<std::mutex> cancel_lock(it->second->m_cancel_guard);
            it->second->m_was_cancelled = true;
            it->second->m_sock.cancel();
        }
    }

    void close(){
        m_work.reset(NULL);
        for(auto& thread : m_threads)
            thread->join();
    }
private:
    void onRequestComplete(std::shared_ptr<Session> session){
        system::error_code ign_ec;
        session->m_sock.shutdown(tcp::socket::shutdown_both, ign_ec);
        std::unique_lock<std::mutex> lock(m_active_sessions_guard);

        auto it = m_active_sessions.find(session->m_id);
        if(it != m_active_sessions.end())
            m_active_sessions.erase(it);
        
        lock.unlock();
        system::error_code ec;

        if(!session->m_ec && session->m_was_cancelled)
            ec = net::error::operation_aborted;
        else
            ec = session->m_ec;
        
        session->m_callback(session->m_id, session->m_response, ec);
    }
};

void handler(unsigned int request_id,
            const std::string& response,
            const system::error_code& ec)
{
    if (!ec) {
        std::cout << "Request #" << request_id << " has completed. Response: " << response << std::endl;
    } 
    else if (ec == asio::error::operation_aborted) { 
        std::cout << "Request #" << request_id << " has been cancelled by the user." << std::endl;
    } 
    else {
        std::cout << "Request #" << request_id << " failed! Error code = " << ec.value() << ". Error message = " << ec.message() << std::endl;
    }
return;
}

int StartAsyncTCP(){
    setlocale(LC_ALL, "Russian");// for normal err in consl
    try {
        AsyncTCPClient client = 3;
        // Here we emulate the user's behavior.
        // User initiates a request with id 1.
        client.emulateLongComputationOp(10, "127.0.0.1", 12, handler, 1);
        client.emulateLongComputationOp(10, "10.25.106.45", 12, handler, 2);    
        client.emulateLongComputationOp(10, "10.25.106.45", 12, handler, 3);

        client.close();
    }
    catch (system::system_error &e) {
        std::cout << "Error occured! Error code = " << e.code()
        << ". Message: " << e.what();
        return e.code().value();
    }
return 0;
}