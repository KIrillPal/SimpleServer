#include <sstream>
#include <filesystem>
#include "config.hpp"

namespace Config {
    std::string NAME;
    unsigned short PORT = 0;
    std::string ROOT_DIRECTORY;
    std::string LOG_DIRECTORY  = "./logs";
    float DEFAULT_HTTP_VERSION = 2;

    size_t USER_THREADS   = 4;
    size_t SCRIPT_THREADS = 2;

    ThreadPool userThreadPool;
    ThreadPool scriptThreadPool;
    Log log;

    std::string DEFAULT_CONFIG_PATH = "./config.conf";
}

bool setArgument(const std::string& line) {
    size_t delim_pos = line.find('=');
    if (delim_pos == std::string::npos) {
        return true;
    }
    std::string argument = line.substr(0, delim_pos);
    std::stringstream(argument) >> argument;
    std::string value = line.substr(delim_pos + 1);

    if (argument == "NAME") {
        Config::NAME = value;
    }
    else if (argument == "LOG_DIRECTORY") {
        Config::LOG_DIRECTORY = std::filesystem::path(value).c_str();
    }
    else if (argument == "DEFAULT_HTTP_VERSION") {
        std::stringstream(value) >> Config::DEFAULT_HTTP_VERSION;
    }
    else if (argument == "USER_THREADS") {
        std::stringstream(value) >> Config::USER_THREADS;
    }
    else if (argument == "SCRIPT_THREADS") {
        std::stringstream(value) >> Config::SCRIPT_THREADS;
    }
    else {
        std::cerr << "Config: unknown parameter '" + argument + "'\n";
    }

    return false;
}

void Config::Configure(const std::string& path) {
    std::ifstream config(path);
    if (!config.good()) {
        throw std::runtime_error("couldn't open config file '" + path + "'");
    }
    config >> ROOT_DIRECTORY >> PORT;
    std::string line;
    std::getline(config, line);

    size_t line_number = 2;
    while (std::getline(config, line) && !line.empty()) {
        if (setArgument(line)) {
            std::string message = "invalid format of argument line ";
            message += std::to_string(line_number) + " '" + line + "'";
            throw std::runtime_error(message);
        }
        line.clear();
        ++line_number;
    }

    if (Config::PORT == 0 || Config::ROOT_DIRECTORY.empty()) {
        throw std::runtime_error("invalid port or root directory\nTip: first line of config format: <root> <port>");
    }
    if (Config::NAME.empty()) {
        Config::NAME = "Unnamed server, root: '" + Config::ROOT_DIRECTORY;
        Config::NAME += "', port: " + std::to_string(Config::PORT);
    }
}