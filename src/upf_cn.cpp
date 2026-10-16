#include "upf_cn.hpp"
#include "arp.hpp"
#include "defines.hpp"
#include "ethernet_frame.hpp"
#include "exceptions.hpp"
#include "ipv4.hpp"
#include "log.hpp"
#include "ue_storage.hpp"
#include "utils.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <netpacket/packet.h>
#include <span>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>

static UpfCnException make_cn_error(const std::string &message) {
  return UpfCnException(create_system_error(message).what());
}

static int create_raw_socket() {
  int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (fd < 0) {
    throw make_cn_error("Failed to create RAW socket");
  }
  return fd;
}

static boost::asio::posix::stream_descriptor
create_stream_descriptor(boost::asio::io_context &ctx, int fd) {
  boost::asio::posix::stream_descriptor sd(ctx);
  boost::system::error_code ec;
  sd.assign(fd, ec);
  if (ec) {
    close(fd);
    throw UpfCnException(ec.message());
  }
  return sd;
}

static void bind_raw_socket_to_interface(int interface_index, int fd) {
  sockaddr_ll dn_upf;
  std::memset(&dn_upf, 0, sizeof(dn_upf));
  dn_upf.sll_family = AF_PACKET;
  dn_upf.sll_protocol = htons(ETH_P_ALL);
  dn_upf.sll_ifindex = interface_index;

  if (bind(fd, (sockaddr *)(&dn_upf), sizeof(dn_upf)) < 0) {
    throw make_cn_error(std::format("Failed to bind socket {} to interface {}",
                                    fd, UpfCn::INTERFACE_NAME));
  }
}

UpfCn::UpfCn(boost::asio::io_context &ctx, UeStorage &strg)
    : sd(ctx), ue_storage(strg) {
  sd = create_stream_descriptor(ctx, create_raw_socket());
  make_socket_nonblocking(sd);
  interface_index = get_interface_index(INTERFACE_NAME);
  bind_raw_socket_to_interface(interface_index, sd.native_handle());
  mac_address =
      get_mac_address_of_interface(sd.native_handle(), INTERFACE_NAME);
}

void UpfCn::process_ipv4(const std::span<const std::uint8_t> &buf) {
  IPv4 ip;
  try {
    ip.from_bytes(buf);
  } catch (const std::invalid_argument &e) {
    return;
  }

  auto destination = ip.get_target_ip();
  if (!ue_storage.contains_ip(destination)) {
    log_info(
        std::format("Unknown destination UE: {}",
                    boost::asio::ip::make_address_v4(destination).to_string()),
        LogSender::UPF_CN);
    return;
  }

  auto teid_dl = ue_storage.get_teids_by_ip(destination).dl;
  if (downlink_sender) {
    downlink_sender(buf.first(ip.get_total_size()), teid_dl);
  } else {
    throw UpfCnException(
        "Can not send IPv4 package: downlink sender is not set");
  }
}

void UpfCn::reply_to_arp_request(Arp &arp) {
  std::string target_ip =
      boost::asio::ip::address_v4(arp.get_target_ip()).to_string();
  log_info(std::format("Someone is asking about {}", target_ip),
           LogSender::UPF_CN);
  if (!ue_storage.contains_ip(target_ip)) {
    log_error(std::format("ARP: Unknown IP {}", target_ip), LogSender::UPF_CN);
    return;
  }

  Arp arp_reply;
  arp_reply.init_reply_for(arp, mac_address);
  std::array<std::uint8_t, Arp::ARP_LEN> arp_reply_in_bytes =
      arp_reply.to_bytes();

  std::vector<std::uint8_t> reply_by_bytes =
      EthernetFrame(arp.get_sender_mac(), mac_address, htons(ETH_P_ARP),
                    std::span<const std::uint8_t>(arp_reply_in_bytes.data(),
                                                  arp_reply_in_bytes.size()))
          .to_bytes();

  log_info(std::format("Sending ARP response for {}", target_ip),
           LogSender::UPF_CN);
  send_frame(
      std::make_shared<std::vector<std::uint8_t>>(std::move(reply_by_bytes)));
}

void UpfCn::process_arp(const std::span<const std::uint8_t> &buf) {
  Arp arp;
  if (!arp.check(buf)) {
    return;
  }
  arp.from_bytes(buf);

  if (arp.get_header().ar_op == ARPOP_REQUEST) {
    reply_to_arp_request(arp);
  } else if (arp.get_header().ar_op == ARPOP_REPLY) {
    log_info(
        std::format(
            "ARP answer received for: {}",
            boost::asio::ip::make_address_v4(arp.get_sender_ip()).to_string()),
        LogSender::UPF_CN);
  }
}

void UpfCn::process_package(const std::span<const std::uint8_t> &package) {
  EthernetFrame ethframe;
  if (!ethframe.check(package)) {
    return;
  }
  ethframe.fill_header(package.first<EthernetFrame::HEADER_LEN>());
  ethframe.fill_payload(
      package.last(package.size() - EthernetFrame::HEADER_LEN));

  switch (ethframe.get_type()) {
  case ETH_P_ARP:
    process_arp(ethframe.get_payload());
    break;
  case ETH_P_IP:
    process_ipv4(ethframe.get_payload());
    break;
  default:
    return;
  }
}

std::span<const std::uint8_t> UpfCn::recieve_package() {
  sockaddr_ll src_addr{};
  socklen_t addrlen = sizeof(src_addr);
  auto &buf = receive_buffer;
  ssize_t byte_num =
      recvfrom(sd.native_handle(), buf.data(), buf.size(), MSG_TRUNC,
               reinterpret_cast<sockaddr *>(&src_addr), &addrlen);

  if (byte_num == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      return std::span<const std::uint8_t>();
    }
    throw make_cn_error("Failed to recieve data");
  }

  if (static_cast<std::size_t>(byte_num) > buf.size() ||
      src_addr.sll_pkttype == PACKET_OUTGOING) {
    return std::span<const std::uint8_t>();
  }

  return std::span<const std::uint8_t>(buf.data(),
                                       static_cast<std::size_t>(byte_num));
}

void UpfCn::run() {
  sd.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                [this](const boost::system::error_code &ec) {
                  if (ec) {
                    if (ec == boost::asio::error::operation_aborted) {
                      return;
                    }
                    throw UpfCnException(ec.message());
                  }

                  try {
                    process_package(recieve_package());
                  } catch (const std::exception &e) {
                    log_error(e.what(), LogSender::UPF_CN);
                  }
                  run();
                });
}

void UpfCn::set_downlink_sender(DownlinkSender sender) {
  downlink_sender = std::move(sender);
}

void UpfCn::send_uplink(const std::span<const std::uint8_t> &packet,
                        const std::uint32_t teid_ul) {
  IPv4 ip;
  try {
    ip.from_bytes(packet);
  } catch (const std::invalid_argument &) {
    return;
  }
  const auto source = ip.get_sender_ip();
  if (!ue_storage.contains_ip(source) ||
      ue_storage.get_teids_by_ip(source).ul != teid_ul) {
    log_error(std::format("Source IP {} does not match UE for TEID: {}",
                          boost::asio::ip::make_address_v4(source).to_string(),
                          teid_ul),
              LogSender::UPF_CN);
    return;
  }
  std::array<std::uint8_t, ETH_ALEN> destination;
  destination.fill(
      EthernetFrame::BROADCAST_MAC_BYTE); // Broadcast is allowed by the lab
                                          // specification.
  auto buffer_ptr = std::make_shared<std::vector<std::uint8_t>>(
      EthernetFrame(destination, mac_address, htons(ETH_P_IP),
                    packet.first(ip.get_total_size()))
          .to_bytes());
  send_frame(buffer_ptr);
}

void UpfCn::send_frame(std::shared_ptr<std::vector<std::uint8_t>> buffer_ptr) {
  sd.async_wait(
      boost::asio::posix::stream_descriptor::wait_write,
      [this, buffer_ptr](const boost::system::error_code &ec) {
        if (ec) {
          if (ec != boost::asio::error::operation_aborted) {
            log_error(ec.message(), LogSender::UPF_CN);
          }
          return;
        }

        ssize_t bytes_sent =
            send(sd.native_handle(), buffer_ptr->data(), buffer_ptr->size(), 0);
        if (bytes_sent == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            send_frame(buffer_ptr);
            return;
          }
          log_error(create_system_error("Failed to send frame").what(),
                    LogSender::UPF_CN);
          return;
        }
        if (static_cast<std::size_t>(bytes_sent) != buffer_ptr->size()) {
          log_error("Ethernet frame was not sent completely",
                    LogSender::UPF_CN);
        }
      });
}
