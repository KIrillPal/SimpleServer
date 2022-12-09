#pragma once
#include "threadpool.hpp"

namespace Config {
    const size_t USER_THREADS   = 4;
    const size_t SCRIPT_THREADS = 2;
    const float DEFAULT_HTTP_VERSION = 2;

    ThreadPool userThreadPool(USER_THREADS);
    ThreadPool scriptThreadPool(SCRIPT_THREADS);
}
