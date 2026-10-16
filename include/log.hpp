#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

enum LogSender {
    UPF_CN, UPF_AN, RE, EC, UPF
};

enum LogType {
    Error, Info
};

void log_error(const std::string &message, LogSender sender);
void log_info(const std::string &message, LogSender sender);

#endif // LOG_HPP
