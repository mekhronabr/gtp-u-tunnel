#ifndef GTP_U_HPP
#define GTP_U_HPP

#include <cstddef>

#include <cstdint>
#include <span>
#include <vector>

class GtpU {
public:
    static constexpr std::size_t HEADER_SIZE = 8;
    static constexpr std::uint8_t BASIC_FLAGS = 0x30;
    static constexpr std::uint8_t G_PDU_TYPE = 0xff;
    GtpU() = default;
    GtpU(
        const std::uint8_t flags, 
        const std::uint8_t message_type, 
        const std::uint16_t length,
        const std::uint32_t teid,
        const std::span<const std::uint8_t> &payload
    );

    void from_bytes(const std::span<const std::uint8_t> &buf);
    std::span<const std::uint8_t> get_payload() const;
    std::uint32_t get_teid() const;
    std::vector<std::uint8_t> to_bytes() const;

private:
    std::uint8_t flags{};
    std::uint8_t message_type{};
    std::uint16_t length{};
    std::uint32_t teid{};
    std::span<const std::uint8_t> payload;
};

#endif // GTP_U_HPP
