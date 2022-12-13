#pragma once
#include "threadpool.hpp"

namespace Config {
    extern unsigned short PORT;
    extern std::string ROOT_DIRECTORY;
    extern float DEFAULT_HTTP_VERSION;

    extern size_t USER_THREADS;
    extern size_t SCRIPT_THREADS;

    extern ThreadPool userThreadPool;
    extern ThreadPool scriptThreadPool;
}
