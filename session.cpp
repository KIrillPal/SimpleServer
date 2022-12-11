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
                std::cout << "Package " << transferred << " bytes received from " << remote_ip_ << '\n';
                std::cout << '\n';
                std::cout.write(buffer, transferred);

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
    std::cout << response_header;

    char* buffer = new char[total_size];
    std::copy(response_header.begin(), response_header.end(), buffer);
    std::copy(response.data.begin(), response.data.end(), buffer + header_size);

    do_write(buffer, total_size);
}

HttpResponse Session::doGetRequest(const HttpRequest& request) {
    HttpResponse response;
    response.version = request.version;

    std::string file_path = request.url.path;
    if (file_path[0] == '/')
        file_path = "." + file_path;
    else file_path = "./" + file_path;

    if (std::filesystem::is_directory(file_path)) {
        file_path += "/index.html";
    }

    std::cout << "find: " << file_path << '\n';
    if (!std::filesystem::exists(file_path)) {
        response.status = HttpResponse::NOT_FOUND;
        return std::move(response);
    }

    if (!request.url.query_string.empty()) {
        return std::move(doScript(file_path, request.url.query_string));
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.good()) {
        response.status = HttpResponse::FORBIDDEN;
        return std::move(response);
    }

    std::string file_extension = std::filesystem::path(file_path).extension();
    if (!file_extension.empty() && file_extension[0] == '.') {
        file_extension.erase(0, 1);
    }
    HttpResponse::setTypeFromExtension(response, file_extension);

    size_t file_size = std::filesystem::file_size(file_path);
    if (file_size > 0) {
        response.status = HttpResponse::OK;
        response.content_length = file_size;
        try {
            response.data.resize(file_size);
            file.read(response.data.data(), file_size);
        } catch (std::exception& ex) {
            std::cout << "Failed to load opened file '" << file_path << "': " << ex.what() << '\n';
        }
    } else {
        response.status = HttpResponse::NO_CONTENT;
    }
    file.close();
    return std::move(response);
}

HttpResponse Session::doPutRequest(const HttpRequest& request) {
    HttpResponse response;
    response.version = request.version;

    std::string file_path = request.url.path;
    if (file_path.empty() || file_path == "/") {
        response.status = HttpResponse::BAD_REQUEST;
        return std::move(response);
    }

    if (file_path[0] == '/')
        file_path = "." + file_path;
    else file_path = "./" + file_path;

    if (!std::filesystem::path(file_path).has_filename()) {
        response.status = HttpResponse::BAD_REQUEST;
        return std::move(response);
    }
    std::cout << "putting: " << file_path << '\n';
    if (std::filesystem::exists(file_path)) {
        response.status = HttpResponse::CONFLICT;
        return std::move(response);
    }

    std::ofstream file(file_path, std::ios::binary);
    if (!file.good()) {
        response.status = HttpResponse::CONFLICT;
        return std::move(response);
    }

    response.status = HttpResponse::CREATED;

    try {
        std::cout << "Writing " << request.content_length << " bytes to " << file_path << '\n';
        file.write(request.data.data(), request.content_length);
    } catch (std::exception& ex) {
        std::cout << "Failed to save created file '" << file_path << "': " << ex.what() << '\n';
    }
    file.close();
    return std::move(response);
}

void Session::doRequest(const HttpRequest& request) {
    try {
        HttpResponse response;

        if (request.method == HttpRequest::GET) {
            response = std::move(doGetRequest(request));
        }
        else if (request.method == HttpRequest::PUT) {
            response = std::move(doPutRequest(request));
        }
        else {
            response.status  = HttpResponse::METHOD_NOT_ALLOWED;
            response.version = Config::DEFAULT_HTTP_VERSION;
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

void Session::doRequest(char* buffer, size_t size)
{
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
        if (request.data.size() < request.content_length)
            readRequestData(request);
    }
     catch (std::exception& ex) {
        std::cout << "Invalid request: " << ex.what() << '\n';
        HttpResponse response;
        response.status = HttpResponse::BAD_REQUEST;
        response.version = Config::DEFAULT_HTTP_VERSION;
        sendResponse(response);
        return;
    }

    auto self(shared_from_this());

    Config::userThreadPool.submit([this, self, request = std::move(request)]() {
        doRequest(request);
    });
}

void Session::readRequestData(HttpRequest& request) {
    size_t rest_size = request.content_length - request.data.size();

    request.data.resize(request.content_length);
    asio::error_code ec;
    asio::read(socket_, asio::buffer(request.data.data(), rest_size), ec);
    if (ec) {
        throw std::runtime_error("Failed to get " + std::to_string(rest_size) + " bytes of package");
    }
}


HttpResponse Session::doPhpScript(const std::string& path, std::string query) {
    HttpResponse response;
    response.version = Config::DEFAULT_HTTP_VERSION;

    if (query[0] == '?')
        query[0] = ' ';
    std::replace( query.begin(), query.end(), '&', ' ');

    std::string command = "php-cgi -f " +  path + " " + query;
    FILE* exec_pipe = popen(command.c_str(), "r");
    if (!exec_pipe) {
        response.status = HttpResponse::INTERNAL_ERROR;
        return std::move(response);
    }

    char buffer[BUFFER_SIZE];
    while (!feof(exec_pipe)) {
        size_t got = std::fread(buffer, sizeof(char), BUFFER_SIZE, exec_pipe);
        if (got == 0)
            break;
        size_t offset = response.data.size();
        response.data.resize(offset + got);
        std::copy(buffer, buffer + got, response.data.begin() + offset);
    }
    response.status = HttpResponse::OK;
    response.content_length = response.data.size();
    HttpResponse::setTypeFromExtension(response, ".html");
    response.content_disposition = "inline";
    return std::move(response);
}

HttpResponse Session::doScript(const std::string& path, const std::string& query) {
    std::string extension = std::filesystem::path(path).extension();
    std::cout << "Executing script '" << path << "'\n";

    if (extension == ".php") {
        return std::move(doPhpScript(path, query));
    }
    HttpResponse response;
    response.status = HttpResponse::INTERNAL_ERROR;
    response.version = Config::DEFAULT_HTTP_VERSION;
    return std::move(response);
}
