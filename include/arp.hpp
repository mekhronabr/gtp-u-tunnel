#ifndef ARP_HPP
#define ARP_HPP

#include <net/if_arp.h>
#include <array>
#include <cstdint>
#include <span>

#include "defines.hpp"

class Arp {
public:
    static const std::size_t ARP_LEN = sizeof(arphdr) + 2 * (ETH_ALEN + sizeof(in_addr));
    Arp();

    void from_bytes(const std::span<const std::uint8_t> &hdr);
    void init_reply_for(const Arp &request, const std::array<std::uint8_t, ETH_ALEN> &sndr_mac);
    bool check(const std::span<const std::uint8_t> &buf) const;
    
    arphdr get_header() const;
    std::array<std::uint8_t, ETH_ALEN> get_sender_mac() const;
    std::array<std::uint8_t, IP_LEN> get_sender_ip() const;
    std::array<std::uint8_t, ETH_ALEN> get_target_mac() const;
    std::array<std::uint8_t, IP_LEN> get_target_ip() const;

    std::array<std::uint8_t, ARP_LEN> to_bytes() const;

private:
    arphdr header;
    std::array<std::uint8_t, ETH_ALEN> sender_mac;
    std::array<std::uint8_t, ETH_ALEN> target_mac;
    std::array<std::uint8_t, IP_LEN> sender_ip;
    std::array<std::uint8_t, IP_LEN> target_ip;
};

#endif // ARP_HPP
