#ifndef UPF_CN_HPP
#define UPF_CN_HPP

#include "arp.hpp"

#include <boost/asio.hpp>
#include <string>
#include <array>
#include <set>
#include <span>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <functional>
#include "ue_storage.hpp"
#include <memory>

class UpfCn {
public:
    using DownlinkSender = std::function<void(
        std::span<const std::uint8_t> package,
        std::uint32_t
    )>;
    inline static const std::string INTERFACE_NAME = "upf_cn_veth";

    UpfCn(boost::asio::io_context& ctx, UeStorage& strg);
    void run();
    void set_downlink_sender(DownlinkSender sender);
    void send_uplink(const std::span<const std::uint8_t> &package, 
        const std::uint32_t teid_ul);

private:
    std::array<std::uint8_t, UPF_BUF_SIZE> receive_buffer{};
    void send_frame(std::shared_ptr<std::vector<std::uint8_t>> buffer_ptr);
    std::span<const std::uint8_t> recieve_package();
    void process_package(const std::span<const std::uint8_t> &package);
    void process_arp(const std::span<const std::uint8_t> &buf);
    void reply_to_arp_request(Arp& arp);
    void process_ipv4(const std::span<const std::uint8_t> &buf);

    boost::asio::posix::stream_descriptor sd;
    std::array<uint8_t, ETH_ALEN> mac_address;
    int interface_index;
    UeStorage &ue_storage;
    DownlinkSender downlink_sender; 
};

#endif // UPF_CN_HPP
