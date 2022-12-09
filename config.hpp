#pragma once
#include "threadpool.hpp"

namespace Config {
    const size_t user_threads   = 4;
    const size_t script_threads = 2;

    ThreadPool userThreadPool(user_threads);
    ThreadPool scriptThreadPool(script_threads);
}
