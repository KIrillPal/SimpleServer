#include <cstdlib>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <filesystem>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/error.hpp>
#include "session.hpp"
#include "config.hpp"

#define BUFFER_SIZE 65536
using asio::ip::tcp;

Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start()
{
    remote_ip_ = socket_.remote_endpoint().address();
    std::cout << "Accepted " << remote_ip_ << '\n';
    closed_.store(false);
    do_read();
    //send_page();
}

Session::~Session() {
    std::cout << "Disconnected\n";
}




void Session::do_read()
{
    if (closed_) return;

    char* buffer = new char[BUFFER_SIZE];
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(buffer, BUFFER_SIZE),
        [this, self, buffer](std::error_code ec, std::size_t transferred) {
            if (!ec) {
                //std::cout << "Package " << transferred << " bytes received from " << remote_ip_ << '\n';
                //std::cout.write(buffer, transferred);

                doRequest(buffer, transferred);
            }
            else {
                std::cout << "Error with package receiving from " << remote_ip_ << ": "
                            << ec << ". " << ec.message() << '\n';
                if (ec.value() == asio::error::eof || ec.value() == asio::error::connection_reset) {
                    std::error_code ec;
                    socket_.shutdown(tcp::socket::shutdown_both, ec);
                    closed_.store(true);
                }
            }

            delete[] buffer;
            do_read();
        });
}

void Session::do_write(char* allocated_buffer, std::size_t buffer_size)
{
    if (closed_) {
        delete[] allocated_buffer;
        return;
    }
    auto self(shared_from_this());

    asio::async_write(socket_, asio::buffer(allocated_buffer, buffer_size),
        [this, self, allocated_buffer](std::error_code ec, std::size_t transferred) {
            if (!ec) {
                std::cout << "Package " << transferred << " bytes sent to " << remote_ip_ << '\n';
                //std::cout << '\n';
                //std::cout.write(allocated_buffer, transferred);
            }
            else std::cout << "Failed to send package " << transferred << " bytes to " << remote_ip_ << '\n';

            delete[] allocated_buffer;
            do_read();
        });
}


void Session::sendResponse(const HttpResponse& response) {
    std::string response_header = std::move(HttpResponse::getResponseHeader(response));
    size_t header_size = response_header.size();
    size_t total_size = header_size + response.data.size();
    //std::cout << response_header;

    char* buffer = new char[total_size];
    std::copy(response_header.begin(), response_header.end(), buffer);
    std::copy(response.data.begin(), response.data.end(), buffer + header_size);

    do_write(buffer, total_size);
}

HttpResponse Session::doGetRequest(const HttpRequest& request) {
    HttpResponse response;
    response.version = request.version;

    std::string file_path = request.url.path;
    if (file_path.empty() || file_path == "/") {
        file_path = "/index.html";
    }

    if (file_path[0] == '/')
        file_path = "." + file_path;
    else file_path = "./" + file_path;

    std::cout << "find: " << file_path << '\n';
    if (!std::filesystem::exists(file_path)) {
        response.status = HttpResponse::NOT_FOUND;
        return std::move(response);
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.good()) {
        response.status = HttpResponse::FORBIDDEN;
        return std::move(response);
    }

    size_t file_size = std::filesystem::file_size(file_path);
    response.status = HttpResponse::OK;
    response.content_length = file_size;

    std::string file_extension = std::filesystem::path(file_path).extension();
    if (!file_extension.empty() && file_extension[0] == '.') {
        file_extension.erase(0, 1);
    }

    HttpResponse::setTypeFromExtension(response, file_extension);

    try {
        response.data.resize(file_size);
        file.read(response.data.data(), file_size);
    } catch (std::exception& ex) {
        std::cout << "Failed to load opened file '" << file_path << "': " << ex.what() << '\n';
    }

    return std::move(response);
}

void Session::doRequest(const HttpRequest& request) {
    try {
        HttpResponse response;

        if (request.method == HttpRequest::GET) {
            response = std::move(doGetRequest(request));
        } else {
            std::cout << "Note: request ignored: unknown method";
            return;
        }
        sendResponse(response);
    }
    catch (std::exception& ex) {
        std::cout << "Failed to response: " << ex.what() << '\n';
        HttpResponse response;
        response.status = HttpResponse::INTERNAL_ERROR;
        sendResponse(response);
        return;
    }
}

void Session::doRequest(char* buffer, size_t size) {
    struct membuf : std::streambuf
    {
        membuf(char* begin, char* end) {
            this->setg(begin, begin, end);
        }
    };
    membuf stream_buffer(buffer, buffer + size);
    std::istream buffer_stream(&stream_buffer);
    HttpRequest request;
    try {
        request = std::move(HttpRequest::readRequest(buffer_stream));
    }
     catch (std::exception& ex) {
        std::cout << "Invalid request: " << ex.what() << '\n';
        HttpResponse response;
        response.status = HttpResponse::BAD_REQUEST;
        response.version = 2;
        sendResponse(response);
        return;
    }

    auto self(shared_from_this());

    Config::userThreadPool.submit([this, self, request = std::move(request)]() {
        doRequest(request);
    });
}