#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/log.h"
#include "esphome/core/defines.h"

// don't include <dsmr.h> because it puts everything in global namespace
//#include "idsmr.h"
#include "dsmr/parser.h"
#include "dsmr/fields.h"

#include <vector>

namespace esphome {
namespace yadsmr {

using namespace ::dsmr::fields;

// DSMR_**_LIST generated by ESPHome and written in esphome/core/defines

#if !defined(DSMR_SENSOR_LIST) && !defined(DSMR_TEXT_SENSOR_LIST)
// Neither set, set it to a dummy value to not break build
#define DSMR_TEXT_SENSOR_LIST(F, SEP) F(identification)
#endif

#if defined(DSMR_SENSOR_LIST) && defined(DSMR_TEXT_SENSOR_LIST)
#define DSMR_BOTH ,
#else
#define DSMR_BOTH
#endif

#ifndef DSMR_SENSOR_LIST
#define DSMR_SENSOR_LIST(F, SEP)
#endif

#ifndef DSMR_TEXT_SENSOR_LIST
#define DSMR_TEXT_SENSOR_LIST(F, SEP)
#endif

#define DSMR_DATA_SENSOR(s) s
#define DSMR_COMMA ,

using MyData = ::dsmr::ParsedData<DSMR_TEXT_SENSOR_LIST(DSMR_DATA_SENSOR, DSMR_COMMA)
                                      DSMR_BOTH DSMR_SENSOR_LIST(DSMR_DATA_SENSOR, DSMR_COMMA)>;

class YaDsmr : public Component, public uart::UARTDevice {
 public:
  YaDsmr(uart::UARTComponent *uart, bool crc_check) : uart::UARTDevice(uart), crc_check_(crc_check) {}

  void setup() override;
  void loop() override;

  bool parse_telegram();

  void publish_sensors(MyData &data) {
#define DSMR_PUBLISH_SENSOR(s) \
  if (data.s##_present && this->s_##s##_ != nullptr) \
    s_##s##_->publish_state(data.s);
    DSMR_SENSOR_LIST(DSMR_PUBLISH_SENSOR, )

#define DSMR_PUBLISH_TEXT_SENSOR(s) \
  if (data.s##_present && this->s_##s##_ != nullptr) \
    s_##s##_->publish_state(data.s.c_str());
    DSMR_TEXT_SENSOR_LIST(DSMR_PUBLISH_TEXT_SENSOR, )
  };

  void dump_config() override;

  void set_decryption_key(const std::string &decryption_key);
  void set_max_telegram_length(size_t length) { this->max_telegram_len_ = length; }
  void set_request_pin(GPIOPin *request_pin) { this->request_pin_ = request_pin; }
  void set_request_interval(uint32_t interval) { this->request_interval_ = interval; }
  void set_receive_timeout(uint32_t timeout) { this->receive_timeout_ = timeout; }

// Sensor setters
#define DSMR_SET_SENSOR(s) \
  void set_##s(sensor::Sensor *sensor) { s_##s##_ = sensor; }
  DSMR_SENSOR_LIST(DSMR_SET_SENSOR, )

#define DSMR_SET_TEXT_SENSOR(s) \
  void set_##s(text_sensor::TextSensor *sensor) { s_##s##_ = sensor; }
  DSMR_TEXT_SENSOR_LIST(DSMR_SET_TEXT_SENSOR, )

  // handled outside dsmr
  void set_telegram(text_sensor::TextSensor *sensor) { s_telegram_ = sensor; }

 protected:
  void receive_telegram_();
  void receive_encrypted_telegram_();
  void reset_telegram_();

  /// Wait for UART data to become available within the read timeout.
  ///
  /// The smart meter might provide data in chunks, causing available() to
  /// return 0. When we're already reading a telegram, then we don't return
  /// right away (to handle further data in an upcoming loop) but wait a
  /// little while using this method to see if more data are incoming.
  /// By not returning, we prevent other components from taking so much
  /// time that the UART RX buffer overflows and bytes of the telegram get
  /// lost in the process.
  bool available_within_timeout_();

  // Request telegram
  uint32_t request_interval_;
  bool request_interval_reached_();
  GPIOPin *request_pin_{nullptr};
  uint32_t last_request_time_{0};
  bool requesting_data_{false};
  bool ready_to_request_data_();
  void start_requesting_data_();
  void stop_requesting_data_();

  // Read telegram
  uint32_t receive_timeout_;
  bool receive_timeout_reached_();
  size_t max_telegram_len_;
  char *telegram_{nullptr};
  size_t bytes_read_{0};
  uint8_t *crypt_telegram_{nullptr};
  size_t crypt_telegram_len_{0};
  size_t crypt_bytes_read_{0};
  uint32_t last_read_time_{0};
  bool header_found_{false};
  bool footer_found_{false};

  // handled outside dsmr
  text_sensor::TextSensor *s_telegram_{nullptr};

// Sensor member pointers
#define DSMR_DECLARE_SENSOR(s) sensor::Sensor *s_##s##_{nullptr};
  DSMR_SENSOR_LIST(DSMR_DECLARE_SENSOR, )

#define DSMR_DECLARE_TEXT_SENSOR(s) text_sensor::TextSensor *s_##s##_{nullptr};
  DSMR_TEXT_SENSOR_LIST(DSMR_DECLARE_TEXT_SENSOR, )

  std::vector<uint8_t> decryption_key_{};
  bool crc_check_;
};
}  // namespace dsmr
}  // namespace esphome

#endif  // USE_ARDUINO
