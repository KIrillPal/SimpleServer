#pragma once
#include <asio/ts/internet.hpp>
#include "request.hpp"

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(asio::ip::tcp::socket socket);

    void start();

    ~Session();

private:
    void sendResponse(const HttpResponse& response);
    HttpResponse doGetRequest(const HttpRequest& request);

    void doRequest(const HttpRequest& request);
    void doRequest(char* buffer, size_t size);

    void do_read();
    void do_write(char* allocated_buffer, std::size_t buffer_size);

    asio::ip::tcp::socket socket_;
    asio::ip::address remote_ip_;
};