#include "log.hpp"
#include "ue_storage.hpp"
#include "upf_an.hpp"
#include "upf_cn.hpp"

#include <csignal>

int main() {
  try {
    boost::asio::io_context ctx;
    UeStorage storage;
    UpfCn upf_cn(ctx, storage);
    UpfAn upf_an(ctx, storage);

    upf_cn.set_downlink_sender(
        [&upf_an](std::span<const std::uint8_t> packet, std::uint32_t teid_dl) {
          upf_an.send_downlink(packet, teid_dl);
        });
        
    upf_an.set_uplink_sender(
        [&upf_cn](std::span<const std::uint8_t> packet, std::uint32_t teid_ul) {
          upf_cn.send_uplink(packet, teid_ul);
        });

    boost::asio::signal_set signals(ctx, SIGINT, SIGTERM);
    signals.async_wait([&ctx](const boost::system::error_code &ec, int) {
      if (!ec) {
        ctx.stop();
      }
    });

    upf_cn.run();
    upf_an.run();
    ctx.run();
  } catch (const std::exception &e) {
    log_error(e.what(), LogSender::UPF);
    return 1;
  }
  return 0;
}
