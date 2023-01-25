#pragma once
#include <string>
#include "threadpool.hpp"
#include "log.hpp"

namespace Config {
    extern std::string NAME;
    extern unsigned short PORT;
    extern std::string ROOT_DIRECTORY;

    // has default value
    extern std::string DEFAULT_CONFIG_PATH;
    extern float       DEFAULT_HTTP_VERSION;
    extern std::string LOG_DIRECTORY;
    extern size_t USER_THREADS;
    extern size_t SCRIPT_THREADS;

    extern ThreadPool userThreadPool;
    extern ThreadPool scriptThreadPool;
    extern Log log;

    void Configure(const std::string& path);
}