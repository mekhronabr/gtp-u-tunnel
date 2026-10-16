#ifndef ETHERNET_FRAME_HPP
#define ETHERNET_FRAME_HPP

#include "defines.hpp"

#include <cstdint>
#include <array>
#include <span>
#include <vector>
#include <linux/if_ether.h>

class EthernetFrame {
public:
    static constexpr std::uint8_t BROADCAST_MAC_BYTE = 0xff;
    static const std::size_t HEADER_LEN = ETH_HLEN;
    EthernetFrame();
    EthernetFrame(
        const std::array<std::uint8_t, ETH_ALEN> &target_mac, 
        const std::array<std::uint8_t, ETH_ALEN> &sender_mac,
        std::uint16_t ethertype, 
        const std::span<const std::uint8_t> &payload
    );

    void fill_header(const std::span<const std::uint8_t, HEADER_LEN> &bytes);
    void fill_payload(const std::span<const std::uint8_t> &payload);

    std::span<const std::uint8_t> get_payload() const;
    std::uint16_t get_type() const;

    bool check(const std::span<const std::uint8_t> &buf);
    
    std::vector<std::uint8_t> to_bytes() const;

private:
    ethhdr header;
    std::span<const std::uint8_t> payload;
};

#endif // ETHERNET_FRAME_HPP
