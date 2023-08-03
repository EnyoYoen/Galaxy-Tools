#include "logger.h"

#include <fstream>

#ifndef NDEBUG
#include <iostream>
#endif

std::thread Logger::logThread;
std::mutex *Logger::lock = nullptr;
std::condition_variable Logger::waiter;
std::ofstream Logger::out;
std::vector<std::string> Logger::buffer;
bool Logger::logging = true;

const char *Logger::defaultPath = "log.txt";

Logger::Logger(const char *m_module)
    : module(m_module)
{}

void Logger::setup(const std::string& path)
{
    logThread = std::thread(&Logger::loop);
    out.open(path.empty() ? defaultPath : path);
}

void Logger::close()
{
    logging = false;
    if (!buffer.empty()) {
        lock->lock();
        buffer.clear();
        lock->unlock();
    }
    waiter.notify_one();
    logThread.join();
    out.close();
}

Logger& Logger::operator<<(const std::string& message) {
    info(message);
    return *this;
}

void Logger::info(const std::string& message)     { log("INFO (" + module + ") " + message); }
void Logger::warn(const std::string& message)     { log("WARN (" + module + ") " + message); }
void Logger::error(const std::string& message)    { log("ERROR (" + module + ") " + message); }
void Logger::critical(const std::string& message) { log("CRITICAL (" + module + ") " + message); }

void Logger::log(const std::string& message)
{
    if (logging) {
        lock->lock();
        buffer.push_back(message);
        lock->unlock();
        waiter.notify_one();
    }
}

void Logger::loop()
{
    lock = new std::mutex();
    while (logging) {
        while (!buffer.empty()) {
#ifndef NDEBUG
            std::cout << buffer.front() << std::endl;
#endif
            out << buffer.front() << std::endl;
            lock->lock();
            buffer.erase(buffer.begin());
            lock->unlock();
        }
        std::unique_lock<std::mutex> lk(*lock);
        waiter.wait(lk);
        lk.unlock();
    }
}