#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <system_error>
#include <stdexcept>

class UpfCnException : public std::runtime_error {
public:
    UpfCnException(const std::string &message) : 
        std::runtime_error(message) {}
};

class UpfAnException : public std::runtime_error {
public:
    UpfAnException(const std::string &message) : 
        std::runtime_error(message) {}
};

#endif // EXCEPTIONS_HPP
