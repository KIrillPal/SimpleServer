#include <cstdlib>
#include <iostream>
#include <memory>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#define BUFFER_SIZE 1024
using asio::ip::tcp;


class session : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start()
    {
        std::cout << "Accepted " << socket_.remote_endpoint().address().to_string() << '\n';
        send_page();
    }

    ~session() {
        std::cout << "Disconnected\n";
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(buffer_, BUFFER_SIZE),
            [this, self](std::error_code ec, std::size_t length){
                if (!ec)
                     std::cout << "Package received from " << socket_.remote_endpoint().address().to_string() << '\n';
                else std::cout << "Error with package receiving from " << socket_.remote_endpoint().address().to_string() << '\n';
            });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(buffer_, length),
            [this, self](std::error_code ec, std::size_t transferred) {
                if (!ec)
                     std::cout << "Package " << transferred << " bytes sent to " << socket_.remote_endpoint().address().to_string() << '\n';
                else std::cout << "Failed to send package " << transferred << " bytes to " << socket_.remote_endpoint().address().to_string() << '\n';
                do_read();
            });
    }

    void send_page() {
        std::stringstream response;
        std::string html_page = "<!DOCTYPE html>"
                              "<html>"
                                "<body>"
                                    "<h1>My First Heading</h1>"
                                    "<p>My first paragraph.</p>"
                                "</body>"
                              "</html>";

        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/html; charset=utf-8\r\n";
        response << "Content-Length: " << html_page.length();
        response << "\r\n\r\n";
        response << html_page;

        response.read(buffer_, response.str().length());
        do_write(response.str().length());
    }


    char buffer_[BUFFER_SIZE];
    tcp::socket socket_;
    std::atomic<void*> sending_ptr;
};

class server
{
public:
    server(asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        socket_(io_context)
    {

        do_accept();
    }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](std::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket_))->start();
          }

          do_accept();
        });

  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();

  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}