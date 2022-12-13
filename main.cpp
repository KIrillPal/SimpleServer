#include <iostream>
#include <filesystem>
#include "server.hpp"
#include "config.hpp"

void setRoot(const std::filesystem::path& root) {
    std::error_code error_code;
    std::filesystem::current_path(root, error_code);

    if (error_code) {
        throw std::runtime_error("root directory doesn't exist");
    }
}

int main(int argc, char* argv[]) {
    Config::PORT = std::atoi(argv[1]);
    Config::ROOT_DIRECTORY = "./root";
    Config::DEFAULT_HTTP_VERSION = 2;
    Config::USER_THREADS   = 4;
    Config::SCRIPT_THREADS = 2;

    Config::userThreadPool.start(Config::USER_THREADS);
    Config::scriptThreadPool.start(Config::SCRIPT_THREADS);
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }
        setRoot(Config::ROOT_DIRECTORY);
        Server s(Config::PORT);
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    Config::userThreadPool.join();
    Config::scriptThreadPool.join();
    return 0;
}