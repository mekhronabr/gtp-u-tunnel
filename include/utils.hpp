#ifndef UTILS_HPP
#define UTILS_HPP

#include <system_error>
#include <array>
#include <cstdint>
#include <linux/if_ether.h>
#include <net/if.h>
#include <format>

#include "boost/asio/posix/stream_descriptor.hpp"

int get_interface_index(const std::string &interface_name);
void make_socket_nonblocking(boost::asio::posix::stream_descriptor &sd);
std::system_error create_system_error(const std::string &message);
std::array<std::uint8_t, ETH_ALEN> get_mac_address_of_interface(int fd, const std::string& interface_name);

#endif // UTILS_HPP