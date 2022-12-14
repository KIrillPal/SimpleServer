#pragma once
#include "threadpool.hpp"
#include "log.hpp"

namespace Config {
    extern unsigned short PORT;
    extern std::string ROOT_DIRECTORY;
    extern std::string LOG_FILE;
    extern float DEFAULT_HTTP_VERSION;

    extern size_t USER_THREADS;
    extern size_t SCRIPT_THREADS;

    extern ThreadPool userThreadPool;
    extern ThreadPool scriptThreadPool;
    extern Log log;
}