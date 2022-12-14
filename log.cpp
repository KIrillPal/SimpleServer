#include <iostream>
#include <chrono>
#include <ctime>
#include "log.hpp"

std::string Log::getCurrentTime() {
    auto current_time = std::chrono::system_clock::now();
    std::time_t cur_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::string result = std::move(std::ctime(&cur_time_t));
    result.pop_back();

    auto duration = current_time.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    result += " " + std::to_string(millis % 1000);
    return std::move(result);
}

Log::Log() = default;

Log::Log(const std::string& path) {
    start(path);
}

void Log::start(const std::string& path) {
    path_ = path;
    file_.open(path_);
    logTime();
    *this << "Log started\n";
}

void Log::logTime() const {
    *this << "[" << Log::getCurrentTime() << "] ";
}

void Log::write(const char* buffer, size_t size) const {
    if (!file_.good()) {
        alertLogFail();
        return;
    }
    file_.write(buffer, size);
    file_.flush();
}

void Log::alertLogFail() const{
    std::cerr << "[" << Log::getCurrentTime() << "] " << "Failed to write to Log\n";
    std::cerr.flush();
}