#include <iostream>
#include <asio/ts/internet.hpp>
#include "session.hpp"
#include "server.hpp"

using asio::ip::tcp;

asio::ip::address Server::getLocalIP() {
asio::io_context ic;
asio::ip::udp::resolver   resolver(ic);
asio::ip::udp::resolver::query query(asio::ip::udp::v4(), "1.1.1.1", "");
asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
asio::ip::udp::endpoint ep = *endpoints;
asio::ip::udp::socket socket(ic);
socket.connect(ep);
return socket.local_endpoint().address();
}

Server::Server(short port) :
    acceptor_(context_, tcp::endpoint(tcp::v4(), port)),
    socket_(context_)
{
    std::cout << "Server started with address " << getLocalIP() << ":" << (unsigned short)port << '\n';
    do_accept();
    context_.run();
}

void Server::do_accept()
{
    acceptor_.async_accept(socket_,
        [this](std::error_code ec) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket_))->start();
            }
            do_accept();
        });
}