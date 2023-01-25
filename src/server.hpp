#pragma once
#include <asio/ts/internet.hpp>
#include "session.hpp"

class Server
{
public:
    Server(unsigned short port);
    static asio::ip::address getLocalIP();
private:
    void do_accept();

    asio::io_context        context_;
    asio::ip::tcp::acceptor acceptor_;
    asio::ip::tcp::socket   socket_;
};