#ifndef IPv4_HPP
#define IPv4_HPP

#include "defines.hpp"

#include <cstddef>
#include <climits>
#include <array>
#include <netinet/ip.h>
#include <span>
#include <vector>
#include <cstdint>

class IPv4 {
public:
    static const std::size_t MIN_HEADER_SIZE = sizeof(iphdr);
    IPv4();
    
    void init_header_with_checks(const std::span<const std::uint8_t> &buf);
    void from_bytes(const std::span<const std::uint8_t> &buf);
    
    std::array<std::uint8_t, IP_LEN> get_target_ip();
    std::array<std::uint8_t, IP_LEN> get_sender_ip();
    std::size_t get_total_size() const;

private:
    static constexpr std::size_t WORD_BYTES = sizeof(std::uint32_t);
    static constexpr std::size_t MIN_IHL = MIN_HEADER_SIZE / WORD_BYTES;
    static constexpr std::size_t CHECKSUM_WORD_BYTES = sizeof(std::uint16_t);
    static constexpr unsigned CHECKSUM_WORD_BITS = sizeof(std::uint16_t) * CHAR_BIT;

    iphdr header;
    std::vector<std::uint8_t> options;
    std::span<const std::uint8_t> payload;
};

#endif // IPv4_HPP