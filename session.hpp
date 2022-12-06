#pragma once
#include <asio/ts/internet.hpp>

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(asio::ip::tcp::socket socket);

    void start();

    ~Session();

private:
    void send_page();

    void do_read();
    void do_write(char* allocated_buffer, std::size_t buffer_size);

    asio::ip::tcp::socket socket_;
    asio::ip::address remote_ip_;
};