#ifndef UE_HPP
#define UE_HPP

#include "boost/asio.hpp"
#include "defines.hpp"
#include "log.hpp"

#include <vector>
#include <utility>
#include <cstdint>
#include <format>

struct TEIDS {
    std::uint32_t dl;
    std::uint32_t ul;
};

class UeStorage {
public:
    UeStorage();
    bool contains_uplink_teid(std::uint32_t teid) const;
    bool contains_ip(const std::string& ip) const;
    bool contains_ip(const std::array<std::uint8_t, IP_LEN> &ip) const;
    TEIDS get_teids_by_ip(const std::string& ip) const;
    TEIDS get_teids_by_ip(const std::array<std::uint8_t, IP_LEN> &ip) const;

private:
    inline static constexpr std::array<const char*, 4> UE_IP_ADDRESSES = {
        "192.168.7.1", "192.168.7.2", "192.168.7.3", "192.168.7.4"
    };
    static constexpr std::uint32_t UE_FIRST_DL_TEID = 2;
    static constexpr std::uint32_t UE_TEID_STEP = 2;
    static constexpr std::uint32_t UE_UL_TEID_OFFSET = 1;

    std::vector<boost::asio::ip::address_v4> ues_by_ip;
    std::vector<TEIDS> ues_bu_teid;
};

#endif // UE_HPP
