#include "config.hpp"

namespace Config {
    unsigned short PORT = 65000;
    std::string ROOT_DIRECTORY = "./root";
    float DEFAULT_HTTP_VERSION = 2;

    size_t USER_THREADS   = 4;
    size_t SCRIPT_THREADS = 2;

    ThreadPool userThreadPool(USER_THREADS);
    ThreadPool scriptThreadPool(SCRIPT_THREADS);
}
