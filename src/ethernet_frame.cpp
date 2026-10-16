#include "ethernet_frame.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <stdexcept>

EthernetFrame::EthernetFrame() : header{}, payload() {}

EthernetFrame::EthernetFrame(
    const std::array<std::uint8_t, ETH_ALEN> &target_mac,
    const std::array<std::uint8_t, ETH_ALEN> &sender_mac,
    std::uint16_t ethertype, const std::span<const std::uint8_t> &payload)
    : header{}, payload(payload) {
  std::copy_n(target_mac.begin(), ETH_ALEN, header.h_dest);
  std::copy_n(sender_mac.begin(), ETH_ALEN, header.h_source);
  header.h_proto = ethertype;
}

bool EthernetFrame::check(const std::span<const std::uint8_t> &buf) {
  if (buf.size() < HEADER_LEN)
    return false;
  return true;
}

void EthernetFrame::fill_header(
    const std::span<const std::uint8_t, HEADER_LEN> &hdr) {
  std::copy(hdr.data(), hdr.data() + HEADER_LEN,
            reinterpret_cast<std::uint8_t *>(&header));
}

void EthernetFrame::fill_payload(const std::span<const std::uint8_t> &pld) {
  payload = pld;
}

std::span<const std::uint8_t> EthernetFrame::get_payload() const {
  return payload;
}

std::uint16_t EthernetFrame::get_type() const { return ntohs(header.h_proto); }

std::vector<std::uint8_t> EthernetFrame::to_bytes() const {
  std::vector<std::uint8_t> frame(std::max(HEADER_LEN + payload.size(),
                                           static_cast<std::size_t>(ETH_ZLEN)));
  std::copy_n(reinterpret_cast<const std::uint8_t *>(&header), HEADER_LEN,
              frame.begin());
  std::copy_n(payload.data(), payload.size(),
              std::next(frame.begin(), HEADER_LEN));
  return frame;
}
