#include <iostream>
#include <filesystem>
#include "server.hpp"

void setRoot(const std::filesystem::path& root) {
    std::error_code error_code;
    std::filesystem::current_path(root, error_code);

    if (error_code) {
        throw std::runtime_error("root directory doesn't exist");
    }
}

int main(int argc, char* argv[]) {
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: server <port>\n";
            return 1;
        }
        setRoot("./root");
        Server s(std::atoi(argv[1]));
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}