#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <ctime>

namespace esphome {
namespace mercury206 {

static const char *const TAG = "mercury206";

enum MercuryCommand : uint8_t {
  CMD_READ_DATETIME = 0x21,
  CMD_READ_TARIFFS = 0x27,
  CMD_READ_UIP = 0x63,
  CMD_READ_FREQ = 0x81,
};

class Mercury206Component : public PollingComponent, public uart::UARTDevice {
 public:
  Mercury206Component();
  
  void set_serial_number(uint32_t serial) { serial_number_ = serial; }
  void set_request_timeout(uint16_t ms) { request_timeout_ = ms; }
  void set_retry_count(uint8_t count) { retry_count_ = count; }

  void set_voltage_sensor(sensor::Sensor *s) { voltage_sensor_ = s; }
  void set_current_sensor(sensor::Sensor *s) { current_sensor_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_sensor_ = s; }
  void set_frequency_sensor(sensor::Sensor *s) { frequency_sensor_ = s; }
  void set_energy_t1_sensor(sensor::Sensor *s) { energy_t1_sensor_ = s; }
  void set_energy_t2_sensor(sensor::Sensor *s) { energy_t2_sensor_ = s; }
  void set_energy_t3_sensor(sensor::Sensor *s) { energy_t3_sensor_ = s; }
  void set_energy_total_sensor(sensor::Sensor *s) { energy_total_sensor_ = s; }
  void set_tariff_sensor(sensor::Sensor *s) { tariff_sensor_ = s; }
  void set_datetime_sensor(text_sensor::TextSensor *s) { datetime_sensor_ = s; }
  void set_crc_errors_sensor(sensor::Sensor *s) { crc_errors_sensor_ = s; }
  void set_last_update_sensor(text_sensor::TextSensor *s) { last_update_sensor_ = s; }

  void setup() override;
  void update() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  void build_address_field(uint8_t *buf);
  uint16_t crc16_mercury(const uint8_t *data, uint16_t len);
  bool verify_crc(const uint8_t *packet, uint8_t len);
  bool send_command(MercuryCommand cmd);
  static uint8_t bcd_to_dec(uint8_t bcd);
  static uint32_t bcd_to_uint32(const uint8_t *bcd, uint8_t len);
  bool parse_uip_response(const uint8_t *data, uint8_t len);
  bool parse_tariffs_response(const uint8_t *data, uint8_t len);
  bool parse_freq_response(const uint8_t *data, uint8_t len);
  bool parse_datetime_response(const uint8_t *data, uint8_t len);
  bool read_response(uint8_t *buffer, uint16_t max_len, uint16_t timeout_ms);
  void poll_meter();

  uint32_t serial_number_{0};
  uint16_t request_timeout_{500};
  uint8_t retry_count_{2};
  
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
  sensor::Sensor *frequency_sensor_{nullptr};
  sensor::Sensor *energy_t1_sensor_{nullptr};
  sensor::Sensor *energy_t2_sensor_{nullptr};
  sensor::Sensor *energy_t3_sensor_{nullptr};
  sensor::Sensor *energy_total_sensor_{nullptr};
  sensor::Sensor *tariff_sensor_{nullptr};
  sensor::Sensor *crc_errors_sensor_{nullptr};
  text_sensor::TextSensor *datetime_sensor_{nullptr};
  text_sensor::TextSensor *last_update_sensor_{nullptr};
  
  uint32_t crc_error_count_{0};
  uint32_t success_count_{0};
  uint8_t cmd_index_{0};
};

}  // namespace mercury206
}  // namespace esphome
