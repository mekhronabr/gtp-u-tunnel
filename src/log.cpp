#include "log.hpp"

static void output_sender(std::ostringstream &line, LogSender sender) {
  switch (sender) {
  case UPF_AN:
    line << "UPF_AN: ";
    break;
  case UPF_CN:
    line << "UPF_CN: ";
    break;
  case RE:
    line << "RE: ";
    break;
  case EC:
    line << "Boost.Asio error_code: ";
    break;
  case UPF:
    line << "UPF: ";
    break;
  }
}

static void output_log_type(std::ostringstream &line, LogType type) {
  switch (type) {
  case Error:
    line << " - ERROR - ";
    break;
  case Info:
    line << " - INFO - ";
    break;
  }
}

static void log(const std::string &message, LogSender sender, LogType type) {
  static std::ofstream file("upf.log", std::ios::out | std::ios::trunc);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open upf.log for writing.");
  }
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm local_time{};
  localtime_r(&time, &local_time);
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch())
          .count() %
      1000;
  std::ostringstream line;
  output_sender(line, sender);
  line << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
  line << ',' << std::setfill('0') << std::setw(3) << milliseconds;
  output_log_type(line, type);
  line << message;
  file << line.str() << std::endl;
  if (!file) {
    throw std::runtime_error("Cannot write to upf.log.");
  }
  std::cout << line.str() << std::endl;
}

void log_error(const std::string &message, LogSender sender) {
  log(message, sender, Error);
}

void log_info(const std::string &message, LogSender sender) {
  log(message, sender, Info);
}
