#include "ue_storage.hpp"
#include "log.hpp"

#include <algorithm>

UeStorage::UeStorage() {
  for (const auto *ip : UE_IP_ADDRESSES) {
    ues_by_ip.push_back(boost::asio::ip::make_address_v4(ip));
  }

  for (std::size_t i = 0; i < ues_by_ip.size(); ++i) {
    std::uint32_t dl = UE_FIRST_DL_TEID + i * UE_TEID_STEP;
    std::uint32_t ul = dl + UE_UL_TEID_OFFSET;
    ues_bu_teid.emplace_back(dl, ul);
    log_info(std::format("Created UE {} TEID_dl={:d} TEID_ul={:d}.",
                         ues_by_ip[i].to_string(), dl, ul),
             UPF);
  }
}

bool UeStorage::contains_ip(const std::string &ip) const {
  return std::find(ues_by_ip.begin(), ues_by_ip.end(),
                   boost::asio::ip::make_address_v4(ip)) != ues_by_ip.end();
}

bool UeStorage::contains_ip(const std::array<std::uint8_t, IP_LEN> &ip) const {
  boost::asio::ip::address_v4::bytes_type target_ip;
  std::copy_n(ip.data(), IP_LEN, target_ip.data());
  boost::asio::ip::address_v4 target_ip_boost =
      boost::asio::ip::make_address_v4(target_ip);
  return std::find(ues_by_ip.begin(), ues_by_ip.end(), target_ip_boost) !=
         ues_by_ip.end();
}

TEIDS UeStorage::get_teids_by_ip(const std::string &ip) const {
  if (!contains_ip(ip)) {
    throw std::invalid_argument(
        std::format("UE storage do not contain ip {}", ip));
  }

  auto it = std::find(ues_by_ip.begin(), ues_by_ip.end(),
                      boost::asio::ip::make_address_v4(ip));
  return ues_bu_teid[it - ues_by_ip.begin()];
}

TEIDS UeStorage::get_teids_by_ip(
    const std::array<std::uint8_t, IP_LEN> &ip) const {
  if (!contains_ip(ip)) {
    throw std::invalid_argument(
        std::format("UE storage do not contain ip {}",
                    boost::asio::ip::make_address_v4(ip).to_string()));
  }

  std::string ip_str = boost::asio::ip::make_address_v4(ip).to_string();
  return get_teids_by_ip(ip_str);
}

bool UeStorage::contains_uplink_teid(std::uint32_t teid) const {
  return std::any_of(ues_bu_teid.begin(), ues_bu_teid.end(),
                     [teid](const TEIDS &teids) { return teids.ul == teid; });
}
