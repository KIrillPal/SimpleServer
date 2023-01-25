#include <iostream>
#include <fstream>
#include <string>
#pragma once

class Log {
public:
    static std::string getCurrentTime();

    Log();
    Log(const std::string& path);
    void start(const std::string& path);
    void logTime() const;
    void write(const char* buffer, size_t size) const;

    template<typename T>
    friend const Log& operator << (const Log& log, const T& value)  {
        if (!log.file_.good()) {
            log.alertLogFail();
            return log;
        }
        log.file_ << value;
        log.file_.flush();
        return log;
    };

private:
    std::string path_;
    mutable std::ofstream file_;
    void alertLogFail() const;
};