#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include "session.hpp"

#define BUFFER_SIZE 65536
using asio::ip::tcp;

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start()
{
    remote_ip_ = socket_.remote_endpoint().address();
    std::cout << "Accepted " << remote_ip_ << '\n';
    do_read();
    //send_page();
}

Session::~Session() {
    std::cout << "Disconnected\n";
}


void Session::do_read()
{
    char* buffer = new char[BUFFER_SIZE];
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(buffer, BUFFER_SIZE),
        [this, self, buffer](std::error_code ec, std::size_t transferred) {
            if (!ec) {
                std::cout << "Package " << transferred << " bytes received from " << remote_ip_ << '\n';
                std::cout << '\n';
                std::cout.write(buffer, transferred);
            }
            else
                std::cout << "Error with package receiving from " << remote_ip_ << ": "
                    << ec << ". " << ec.message() << '\n';
            delete[] buffer;
            send_page();1
            //do_read();
        });
}

void Session::do_write(char* allocated_buffer, std::size_t buffer_size)
{
    auto self(shared_from_this());

    asio::async_write(socket_, asio::buffer(allocated_buffer, buffer_size),
        [this, self, allocated_buffer](std::error_code ec, std::size_t transferred) {
            if (!ec)
                    std::cout << "Package " << transferred << " bytes sent to " << remote_ip_ << '\n';
            else std::cout << "Failed to send package " << transferred << " bytes to " << remote_ip_ << '\n';
            delete[] allocated_buffer;

            do_read();
        });
}

void Session::send_page() {
    const char INDEX_PATH[] = "index.html";

    std::ifstream file(INDEX_PATH, std::ios::binary);
    size_t file_size = std::filesystem::file_size(INDEX_PATH);

    std::stringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html; charset=utf-8\r\n";
    response << "Content-Length: " << file_size;
    response << "\r\n\r\n";

    size_t response_size = response.str().length();
    char* buffer = new char[response_size + file_size];
    response.read(buffer, response_size);
    file.read(buffer + response_size, file_size);
    do_write(buffer, response_size + file_size);
}