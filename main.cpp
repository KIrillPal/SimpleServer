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

std::string getLogName() {
    return Config::LOG_DIRECTORY + '/' + Config::NAME + "---" + Log::getCurrentTime() + ".log";
}

int main(int argc, char* argv[]) {
    std::string config_path = Config::DEFAULT_CONFIG_PATH;
    if (argc >= 2) {
        config_path = argv[1];
    }

    try {
        Config::Configure(config_path);
    } catch (std::exception& ex) {
        std::cerr << "Configuration failed: " << ex.what() << '\n';
        return 0;
    }

    try
    {
        Config::log.start(getLogName());
        Config::userThreadPool.start(Config::USER_THREADS);
        Config::scriptThreadPool.start(Config::SCRIPT_THREADS);
        setRoot(Config::ROOT_DIRECTORY);
        Server s(Config::PORT);
    }
    catch (std::exception& e)
    {
        Config::log.logTime();
        Config::log << "Exception: " << e.what() << "\n";
        std::cerr << "Exception: " << e.what() << "\n";
    }

    Config::userThreadPool.join();
    Config::scriptThreadPool.join();
    return 0;
}