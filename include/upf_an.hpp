#ifndef UPF_AN_HPP
#define UPF_AN_HPP

#include "defines.hpp"
#include "boost/asio.hpp"
#include <functional>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <span>
#include "ue_storage.hpp"

class UpfAn {
public:
    using UplinkSender = std::function<void(
        std::span<const std::uint8_t>, std::uint32_t
    )>;
    static const std::size_t ENDPOINT_PORT = 2152;
    inline static const std::string ENDPOINT_IP = "192.168.9.2";
    inline static const std::string INTERFACE_NAME = "upf_an_veth";

    UpfAn(boost::asio::io_context& ctx, UeStorage &strg);
    void run();
    void set_uplink_sender(UplinkSender sender);
    void send_downlink(const std::span<const std::uint8_t> &package, 
        const std::uint32_t teid_dl);

private:
    inline static const std::string LOCAL_IP = "192.168.9.1";
    static constexpr std::size_t UDP_MAX_PAYLOAD_SIZE = IP_MAXPACKET - sizeof(iphdr) - sizeof(udphdr);

    void check_and_process_package(const std::span<const std::uint8_t> &buf);

    boost::asio::ip::udp::socket socket;
    std::array<std::uint8_t, UPF_BUF_SIZE> receive_buffer{};
    boost::asio::ip::udp::endpoint peer;
    UeStorage &ue_storage;
    UplinkSender uplink_sender;
};

#endif // UPF_AN_HPP
