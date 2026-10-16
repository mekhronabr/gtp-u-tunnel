#include "utils.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>

int get_interface_index(const std::string &interface_name) {
  int interface_index = if_nametoindex(interface_name.data());
  if (!interface_index) {
    throw create_system_error(
        std::format("Failed to find index of interface {}", interface_name));
  }
  return interface_index;
}

void make_socket_nonblocking(boost::asio::posix::stream_descriptor &sd) {
  int flags = fcntl(sd.native_handle(), F_GETFL);
  if (flags == -1) {
    throw create_system_error("Failed to get flags of socket");
  }

  if (fcntl(sd.native_handle(), F_SETFL, (flags | O_NONBLOCK)) == -1) {
    throw create_system_error("Failed to make descriptor non-blocking");
  }
}

std::system_error create_system_error(const std::string &message) {
  return std::system_error(errno, std::system_category(), message);
}

std::array<std::uint8_t, ETH_ALEN>
get_mac_address_of_interface(int fd, const std::string &interface_name) {
  if (interface_name.size() >= IFNAMSIZ)
    throw std::invalid_argument("Interface name too long");
  ifreq ifr{};
  std::memcpy(ifr.ifr_ifrn.ifrn_name, interface_name.data(),
              interface_name.size() + 1);
  if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
    throw create_system_error(std::format(
        "Failed to get mac address of interface {}", interface_name));
  }
  std::array<std::uint8_t, ETH_ALEN> mac;
  std::copy(ifr.ifr_hwaddr.sa_data, ifr.ifr_hwaddr.sa_data + ETH_ALEN,
            mac.begin());
  return mac;
}