#include "mercury206.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mercury206 {

Mercury206Component::Mercury206Component() = default;

void Mercury206Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Mercury 206...");
  ESP_LOGCONFIG(TAG, "  Serial Number: %lu", (unsigned long)serial_number_);
  ESP_LOGCONFIG(TAG, "  Request Timeout: %dms", request_timeout_);
  ESP_LOGCONFIG(TAG, "  Retry Count: %d", retry_count_);
}

void Mercury206Component::loop() {
  // Пусто - вся логика в update()
}

void Mercury206Component::update() {
  poll_meter();
}

void Mercury206Component::build_address_field(uint8_t *buf) {
  buf[0] = (serial_number_ >> 24) & 0xFF;
  buf[1] = (serial_number_ >> 16) & 0xFF;
  buf[2] = (serial_number_ >> 8) & 0xFF;
  buf[3] = serial_number_ & 0xFF;
}

uint16_t Mercury206Component::crc16_mercury(const uint8_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool Mercury206Component::verify_crc(const uint8_t *packet, uint8_t len) {
  if (len < 3) return false;
  uint16_t calc_crc = crc16_mercury(packet, len - 2);
  uint16_t recv_crc = (packet[len-1] << 8) | packet[len-2];
  return calc_crc == recv_crc;
}

bool Mercury206Component::send_command(MercuryCommand cmd) {
  uint8_t frame[8];
  build_address_field(frame);
  frame[4] = static_cast<uint8_t>(cmd);
  
  uint16_t crc = crc16_mercury(frame, 5);
  frame[5] = crc & 0xFF;
  frame[6] = (crc >> 8) & 0xFF;
  
  ESP_LOGD(TAG, "Sending cmd 0x%02X", cmd);
  write_array(frame, 7);
  flush();
  return true;
}

uint8_t Mercury206Component::bcd_to_dec(uint8_t bcd) {
  return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

uint32_t Mercury206Component::bcd_to_uint32(const uint8_t *bcd, uint8_t len) {
  uint32_t result = 0;
  for (uint8_t i = 0; i < len; i++) {
    result = result * 100 + bcd_to_dec(bcd[i]);
  }
  return result;
}

bool Mercury206Component::parse_uip_response(const uint8_t *data, uint8_t len) {
  if (len < 14 || data[4] != CMD_READ_UIP) return false;
  
  uint16_t v_raw = bcd_to_uint32(&data[5], 2);
  float voltage = v_raw / 10.0f;
  if (voltage >= 190.0f && voltage <= 260.0f && voltage_sensor_) {
    voltage_sensor_->publish_state(voltage);
  }
  
  uint16_t i_raw = bcd_to_uint32(&data[7], 2);
  float current = i_raw / 100.0f;
  if (current <= 100.0f && current_sensor_) {
    current_sensor_->publish_state(current);
  }
  
  uint32_t p_raw = bcd_to_uint32(&data[9], 3);
  float power = p_raw / 1000.0f;
  if (power_sensor_) {
    power_sensor_->publish_state(power);
  }
  
  ESP_LOGD(TAG, "Parsed UIP: %.1fV, %.2fA, %.3fkW", voltage, current, power);
  return true;
}

bool Mercury206Component::parse_tariffs_response(const uint8_t *data, uint8_t len) {
  if (len < 22 || data[4] != CMD_READ_TARIFFS) return false;
  
  auto parse_energy = [&](uint8_t offset) -> float {
    return bcd_to_uint32(&data[offset], 4) / 100.0f;
  };
  
  float t1 = parse_energy(5);
  float t2 = parse_energy(9);
  float t3 = parse_energy(13);
  
  if (energy_t1_sensor_) energy_t1_sensor_->publish_state(t1);
  if (energy_t2_sensor_) energy_t2_sensor_->publish_state(t2);
  if (energy_t3_sensor_) energy_t3_sensor_->publish_state(t3);
  if (energy_total_sensor_) energy_total_sensor_->publish_state(t1 + t2 + t3);
  
  ESP_LOGD(TAG, "Parsed tariffs: T1=%.2f, T2=%.2f, T3=%.2f kWh", t1, t2, t3);
  return true;
}

bool Mercury206Component::parse_freq_response(const uint8_t *data, uint8_t len) {
  if (len < 17 || data[4] != CMD_READ_FREQ) return false;
  
  uint16_t freq_raw = bcd_to_uint32(&data[5], 2);
  float frequency = freq_raw / 100.0f;
  if (frequency >= 45.0f && frequency <= 55.0f && frequency_sensor_) {
    frequency_sensor_->publish_state(frequency);
  }
  
  uint8_t tariff = data[7] & 0x0F;
  if (tariff >= 1 && tariff <= 4 && tariff_sensor_) {
    tariff_sensor_->publish_state(tariff);
  }
  
  ESP_LOGD(TAG, "Parsed freq: %.2f Hz, tariff: %d", frequency, tariff);
  return true;
}

bool Mercury206Component::parse_datetime_response(const uint8_t *data, uint8_t len) {
  if (len < 14 || data[4] != CMD_READ_DATETIME) return false;
  
  uint8_t yy = bcd_to_dec(data[11]);
  uint8_t mon = bcd_to_dec(data[10]);
  uint8_t dd = bcd_to_dec(data[9]);
  uint8_t hh = bcd_to_dec(data[6]);
  uint8_t mm = bcd_to_dec(data[7]);
  uint8_t ss = bcd_to_dec(data[8]);
  
  char dt_str[26];
  snprintf(dt_str, sizeof(dt_str), "20%02d-%02d-%02d %02d:%02d:%02d", 
           yy, mon, dd, hh, mm, ss);
  
  if (datetime_sensor_) {
    datetime_sensor_->publish_state(dt_str);
  }
  
  if (last_update_sensor_) {
    std::time_t now = std::time(nullptr);
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    last_update_sensor_->publish_state(ts);
  }
  
  ESP_LOGD(TAG, "Parsed datetime: %s", dt_str);
  return true;
}

bool Mercury206Component::read_response(uint8_t *buffer, uint16_t max_len, uint16_t timeout_ms) {
  uint16_t start = millis();
  uint16_t count = 0;
  
  while (millis() - start < timeout_ms) {
    while (available()) {
      if (count >= max_len) return false;
      buffer[count++] = read();
    }
    if (count > 0 && millis() - start > 10) {
      delay(10);
      if (!available()) break;
    }
  }
  return count > 0;
}

void Mercury206Component::poll_meter() {
  static const MercuryCommand commands[] = {
    CMD_READ_TARIFFS,
    CMD_READ_UIP,
    CMD_READ_FREQ,
    CMD_READ_DATETIME
  };
  
  uint8_t retries = 0;
  MercuryCommand cmd = commands[cmd_index_];
  
  while (retries <= retry_count_) {
    if (!send_command(cmd)) {
      ESP_LOGW(TAG, "Failed to send command 0x%02X", cmd);
      retries++;
      continue;
    }
    
    uint8_t response[64];
    if (!read_response(response, sizeof(response), request_timeout_)) {
      ESP_LOGW(TAG, "Timeout waiting for response to cmd 0x%02X", cmd);
      retries++;
      continue;
    }
    
    uint8_t resp_len = 0;
    if (response[4] == CMD_READ_TARIFFS) resp_len = 22;
    else if (response[4] == CMD_READ_UIP) resp_len = 14;
    else if (response[4] == CMD_READ_FREQ) resp_len = 17;
    else if (response[4] == CMD_READ_DATETIME) resp_len = 14;
    
    if (resp_len == 0) {
      ESP_LOGW(TAG, "Unknown response command: 0x%02X", response[4]);
      break;
    }
    
    if (!verify_crc(response, resp_len)) {
      crc_error_count_++;
      if (crc_errors_sensor_) crc_errors_sensor_->publish_state(crc_error_count_);
      ESP_LOGW(TAG, "CRC error for cmd 0x%02X", response[4]);
      retries++;
      continue;
    }
    
    bool parsed = false;
    switch (response[4]) {
      case CMD_READ_UIP:
        parsed = parse_uip_response(response, resp_len);
        break;
      case CMD_READ_TARIFFS:
        parsed = parse_tariffs_response(response, resp_len);
        break;
      case CMD_READ_FREQ:
        parsed = parse_freq_response(response, resp_len);
        break;
      case CMD_READ_DATETIME:
        parsed = parse_datetime_response(response, resp_len);
        break;
    }
    
    if (parsed) {
      success_count_++;
      cmd_index_ = (cmd_index_ + 1) % 4;
      return;
    }
    
    retries++;
  }
  
  ESP_LOGE(TAG, "Failed to poll meter after %d retries", retry_count_);
  cmd_index_ = (cmd_index_ + 1) % 4;
}

}  // namespace mercury206
}  // namespace esphome
