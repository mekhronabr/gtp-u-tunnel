#include "gtp-u.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>

GtpU::GtpU(const std::uint8_t flgs, const std::uint8_t msg_type,
           const std::uint16_t len, const std::uint32_t teid,
           const std::span<const std::uint8_t> &payload)
    : flags(flgs), message_type(msg_type), length(len), teid(teid),
      payload(payload) {}

std::vector<std::uint8_t> GtpU::to_bytes() const {
  if (payload.size() != length || flags != BASIC_FLAGS)
    throw std::invalid_argument(
        "GTP-U serializer requires matching length and a basic v1 header");
  std::vector<std::uint8_t> buf(HEADER_SIZE + payload.size());
  auto it = buf.begin();
  std::copy_n(&flags, sizeof(flags), it);

  it++;
  std::copy_n(&message_type, sizeof(flags), it);

  it++;
  std::uint16_t network_length = htons(length);
  std::copy_n(reinterpret_cast<const std::uint8_t *>(&network_length),
              sizeof(std::uint16_t), it);

  it += sizeof(std::uint16_t);
  std::uint32_t network_teid = htonl(teid);
  std::copy_n(reinterpret_cast<const std::uint8_t *>(&network_teid),
              sizeof(std::uint32_t), it);

  it += sizeof(std::uint32_t);
  std::copy_n(payload.data(), payload.size(), it);
  return buf;
}

void GtpU::from_bytes(const std::span<const std::uint8_t> &buf) {
  if (buf.size() < HEADER_SIZE) {
    throw std::invalid_argument("GTP-U packet is shorter than its header");
  }
  if (buf[0] != BASIC_FLAGS || buf[1] != G_PDU_TYPE) {
    throw std::invalid_argument("Unsupported GTP-U header");
  }

  std::uint16_t network_length;
  std::memcpy(&network_length, buf.data() + 2, sizeof(network_length));
  std::uint16_t packet_length = ntohs(network_length);
  if (packet_length + HEADER_SIZE != buf.size()) {
    throw std::invalid_argument("Invalid GTP-U packet length");
  }

  std::uint32_t network_teid;
  std::memcpy(&network_teid, buf.data() + 4, sizeof(network_teid));

  flags = buf[0];
  message_type = buf[1];
  length = packet_length;
  teid = ntohl(network_teid);
  payload = buf.subspan(HEADER_SIZE);
}

std::span<const std::uint8_t> GtpU::get_payload() const { return payload; }

std::uint32_t GtpU::get_teid() const { return teid; }
