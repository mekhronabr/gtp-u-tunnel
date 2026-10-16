#include "ipv4.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <stdexcept>

IPv4::IPv4() : header{} {}

void IPv4::init_header_with_checks(const std::span<const std::uint8_t> &buf) {
  if (buf.size() < MIN_HEADER_SIZE) {
    throw std::invalid_argument(
        "Buffer size is lower than minimum header size");
  }

  iphdr hdr{};
  std::copy_n(buf.begin(), MIN_HEADER_SIZE,
              reinterpret_cast<std::uint8_t *>(&hdr));

  if (hdr.version != IPVERSION) {
    throw std::invalid_argument(
        "IPv4 init: Protocol version is not equal to 4");
  }

  if (hdr.ihl < MIN_IHL) {
    throw std::invalid_argument(
        "IPv4 init: Internet header length is less than 5");
  }

  if (buf.size() < hdr.ihl * WORD_BYTES) {
    throw std::invalid_argument(
        "IPv4 init: Buffer do not contains whole header");
  }

  if (ntohs(hdr.tot_len) < hdr.ihl * WORD_BYTES) {
    throw std::invalid_argument("IPv4 init: tot_len is less than ihl * 4");
  }

  if (buf.size() < ntohs(hdr.tot_len)) {
    throw std::invalid_argument(
        "IPv4 init: Buffer size is less than IP package size");
  }

  std::uint32_t sum = 0;
  for (std::size_t i = 0; i < hdr.ihl * WORD_BYTES; i += CHECKSUM_WORD_BYTES)
    sum += (static_cast<std::uint16_t>(buf[i]) << CHAR_BIT) | buf[i + 1];
  while (sum >> CHECKSUM_WORD_BITS)
    sum = (sum & UINT16_MAX) + (sum >> CHECKSUM_WORD_BITS);
  if (sum != UINT16_MAX)
    throw std::invalid_argument("Invalid IPv4 header checksum");
  header = hdr;
  header.tot_len = ntohs(header.tot_len);
  header.id = ntohs(header.id);
  header.frag_off = ntohs(header.frag_off);
}

void IPv4::from_bytes(const std::span<const std::uint8_t> &buf) {
  init_header_with_checks(buf);

  auto it = buf.begin() + MIN_HEADER_SIZE;
  std::size_t header_len = header.ihl * WORD_BYTES;
  options = std::vector<std::uint8_t>(header_len - MIN_HEADER_SIZE);
  std::copy_n(it, options.size(), options.begin());
  payload = buf.subspan(header_len, header.tot_len - header_len);
}

std::array<std::uint8_t, IP_LEN> IPv4::get_target_ip() {
  std::array<std::uint8_t, IP_LEN> ip;
  std::copy_n(reinterpret_cast<std::uint8_t *>(&header.daddr), IP_LEN,
              ip.begin());
  return ip;
}

std::array<std::uint8_t, IP_LEN> IPv4::get_sender_ip() {
  std::array<std::uint8_t, IP_LEN> ip;
  std::copy_n(reinterpret_cast<std::uint8_t *>(&header.saddr), IP_LEN,
              ip.begin());
  return ip;
}

std::size_t IPv4::get_total_size() const { return header.tot_len; }
