#include "upf_an.hpp"
#include "gtp-u.hpp"
#include "log.hpp"

#include <memory>

using udp = boost::asio::ip::udp;

UpfAn::UpfAn(boost::asio::io_context &ctx, UeStorage &strg)
    : socket(ctx), ue_storage(strg) {
  socket.open(udp::v4());
  socket.bind(
      udp::endpoint(boost::asio::ip::make_address_v4(LOCAL_IP), ENDPOINT_PORT));
}

void UpfAn::send_downlink(const std::span<const std::uint8_t> &buf,
                          const std::uint32_t teid_dl) {
  if (buf.size() > UDP_MAX_PAYLOAD_SIZE - GtpU::HEADER_SIZE) {
    log_error("Downlink packet exceeds UDP size limit", LogSender::UPF_AN);
    return;
  }
  auto bytes = std::make_shared<std::vector<std::uint8_t>>(
      GtpU(GtpU::BASIC_FLAGS, GtpU::G_PDU_TYPE,
           static_cast<std::uint16_t>(buf.size()), teid_dl, buf)
          .to_bytes());
  socket.async_send_to(
      boost::asio::buffer(*bytes),
      udp::endpoint(boost::asio::ip::make_address_v4(ENDPOINT_IP),
                    ENDPOINT_PORT),
      [bytes](const boost::system::error_code &ec, std::size_t) {
        if (ec && ec != boost::asio::error::operation_aborted)
          log_error(ec.message(), LogSender::UPF_AN);
      });
}

void UpfAn::set_uplink_sender(UplinkSender sender) {
  uplink_sender = std::move(sender);
}

void UpfAn::run() {
  socket.async_receive_from(
      boost::asio::buffer(receive_buffer), peer,
      [this](const boost::system::error_code &ec, std::size_t bytes_received) {
        if (ec == boost::asio::error::operation_aborted) {
          return;
        }

        if (ec) {
          log_error(ec.message(), LogSender::UPF_AN);
        } else {
          try {
            check_and_process_package(std::span<const std::uint8_t>(
                receive_buffer.data(), bytes_received));
          } catch (const std::exception &e) {
            log_error(e.what(), LogSender::UPF_AN);
          }
        }

        run();
      });
}

void UpfAn::check_and_process_package(
    const std::span<const std::uint8_t> &buf) {
  log_info(std::format("Received GTP-U message from (\'{}\', {})",
                       peer.address().to_string(), peer.port()),
           LogSender::UPF_AN);
  if (peer.address() != boost::asio::ip::make_address_v4(ENDPOINT_IP) ||
      peer.port() != ENDPOINT_PORT) {
    return;
  }

  GtpU gtp;
  gtp.from_bytes(buf);
  if (!ue_storage.contains_uplink_teid(gtp.get_teid())) {
    log_error(std::format("Can't find UE for TEID: {}", gtp.get_teid()),
              LogSender::UPF_AN);
    return;
  }

  if (uplink_sender) {
    uplink_sender(gtp.get_payload(), gtp.get_teid());
  }
}
