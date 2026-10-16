#include "arp.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <stdexcept>

Arp::Arp() : header{} {}

bool Arp::check(const std::span<const std::uint8_t> &buf) const {
  if (buf.size() < ARP_LEN) {
    return false;
  }
  arphdr hdr{};
  std::copy_n(buf.data(), sizeof(hdr), reinterpret_cast<std::uint8_t *>(&hdr));
  return ntohs(hdr.ar_hrd) == ARPHRD_ETHER && ntohs(hdr.ar_pro) == ETH_P_IP &&
         hdr.ar_hln == ETH_ALEN && hdr.ar_pln == IP_LEN;
}

void Arp::from_bytes(const std::span<const std::uint8_t> &buf) {
  if (!check(buf)) {
    throw std::invalid_argument("Buffer size to initialize ARP is too small");
  }
  auto it = buf.data();
  std::copy_n(it, sizeof(header), reinterpret_cast<std::uint8_t *>(&header));
  header.ar_op = ntohs(header.ar_op);

  it += sizeof(header);
  std::copy_n(it, ETH_ALEN, sender_mac.begin());

  it += ETH_ALEN;
  std::copy_n(it, IP_LEN, sender_ip.begin());

  it += IP_LEN;
  std::copy_n(it, ETH_ALEN, target_mac.begin());

  it += ETH_ALEN;
  std::copy_n(it, IP_LEN, target_ip.begin());
}

void Arp::init_reply_for(const Arp &request,
                         const std::array<std::uint8_t, ETH_ALEN> &sndr_mac) {
  header.ar_pro = htons(ETH_P_IP);
  header.ar_op = ARPOP_REPLY;
  header.ar_hrd = request.get_header().ar_hrd;
  header.ar_hln = request.get_header().ar_hln;
  header.ar_pln = request.get_header().ar_pln;
  sender_mac = sndr_mac;
  sender_ip = request.get_target_ip();
  target_mac = request.get_sender_mac();
  target_ip = request.get_sender_ip();
}

arphdr Arp::get_header() const { return header; }

std::array<std::uint8_t, ETH_ALEN> Arp::get_sender_mac() const {
  return sender_mac;
}

std::array<std::uint8_t, ETH_ALEN> Arp::get_target_mac() const {
  return target_mac;
}

std::array<std::uint8_t, IP_LEN> Arp::get_sender_ip() const {
  return sender_ip;
}

std::array<std::uint8_t, IP_LEN> Arp::get_target_ip() const {
  return target_ip;
}

std::array<std::uint8_t, Arp::ARP_LEN> Arp::to_bytes() const {
  std::array<std::uint8_t, ARP_LEN> res;
  auto it = res.begin();
  auto wire_header = header;
  wire_header.ar_op = htons(header.ar_op);
  std::copy_n(reinterpret_cast<const std::uint8_t *>(&wire_header),
              sizeof(wire_header), it);

  it += sizeof(header);
  std::copy_n(sender_mac.begin(), ETH_ALEN, it);

  it += ETH_ALEN;
  std::copy_n(sender_ip.begin(), IP_LEN, it);

  it += IP_LEN;
  std::copy_n(target_mac.begin(), ETH_ALEN, it);

  it += ETH_ALEN;
  std::copy_n(target_ip.begin(), IP_LEN, it);

  return res;
}