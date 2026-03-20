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

// === CRC-16: ТОЧНАЯ КОПИЯ ОРИГИНАЛА (принимает uint16_t*) ===
uint16_t Mercury206Component::crc16_mercury(const uint16_t *data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;  // Полином Меркурий
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// === verify_crc: ТОЧНАЯ КОПИЯ crc_check из старого кода ===
bool Mercury206Component::verify_crc(const uint8_t *data, uint8_t len) {
  // len = индекс последнего байта для расчёта CRC (как в старом коде)
  // CRC лежит в data[len+1] (low) и data[len+2] (high)
  
  // Копируем в массив uint16_t как в оригинале
  uint16_t fr[100];
  for (int i = 0; i <= len; i++) {
    fr[i] = data[i];
  }
  
  // Считаем CRC по len+1 байтам
  uint16_t crc_f = crc16_mercury(fr, len + 1);
  
  // Извлекаем байты: low first, then high
  uint8_t crc_1 = crc_f & 0xFF;         // Low byte
  uint8_t crc_2 = (crc_f >> 8) & 0xFF;  // High byte
  
  // Сравниваем с принятыми
  return (crc_1 == data[len + 1] && crc_2 == data[len + 2]);
}

bool Mercury206Component::send_command(MercuryCommand cmd) {
  uint8_t frame[8];
  build_address_field(frame);
  frame[4] = static_cast<uint8_t>(cmd);
  
  // CRC считаем по первым 5 байтам (ADDR+CMD)
  // Копируем в uint16_t массив как в оригинале
  uint16_t fr[6];
  for (int i = 0; i < 5; i++) fr[i] = frame[i];
  uint16_t crc = crc16_mercury(fr, 5);
  
  frame[5] = crc & 0xFF;        // Low byte first
  frame[6] = (crc >> 8) & 0xFF; // High byte second
  
  ESP_LOGD(TAG, "Sending cmd 0x%02X", cmd);
  
  write_array(frame, 7);
  flush();
  
  delay(30);  // ← КРИТИЧНО: время на переключение модуля + ответ счётчика
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

// === Парсеры: принимают (data, len) ===

bool Mercury206Component::parse_uip_response(const uint8_t *data, uint8_t len) {
  // Ожидаем: ADDR(4)+CMD(1)+V(2)+I(2)+P(3)+CRC(2) = 14 байт
  // len = 11 (индекс последнего байта для CRC)
  if (len < 11 || data[4] != CMD_READ_UIP) return false;
  
  // Напряжение: 2 байта BCD, значение *10 -> делим на 10
  uint16_t v_raw = bcd_to_uint32(&data[5], 2);
  float voltage = v_raw / 10.0f;
  if (voltage >= 190.0f && voltage <= 260.0f && voltage_sensor_) {
    voltage_sensor_->publish_state(voltage);
  }
  
  // Ток: 2 байта BCD, значение *100 -> делим на 100
  uint16_t i_raw = bcd_to_uint32(&data[7], 2);
  float current = i_raw / 100.0f;
  if (current <= 100.0f && current_sensor_) {
    current_sensor_->publish_state(current);
  }
  
  // Мощность: 3 байта BCD, значение *1000 -> делим на 1000 для кВт
  uint32_t p_raw = bcd_to_uint32(&data[9], 3);
  float power = p_raw / 1000.0f;
  if (power_sensor_) {
    power_sensor_->publish_state(power);
  }
  
  ESP_LOGD(TAG, "Parsed UIP: %.1fV, %.2fA, %.3fkW", voltage, current, power);
  return true;
}

bool Mercury206Component::parse_tariffs_response(const uint8_t *data, uint8_t len) {
  // ADDR(4)+CMD(1)+T1(4)+T2(4)+T3(4)+CRC(2) = 23 байта
  // len = 20
  if (len < 20 || data[4] != CMD_READ_TARIFFS) return false;
  
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
  // ADDR(4)+CMD(1)+freq(2)+tarif(1)+FL(1)+F1(6)+CRC(2) = 17 байт
  // len = 14
  if (len < 14 || data[4] != CMD_READ_FREQ) return false;
  
  // Частота: 2 байта BCD, значение *100 -> делим на 100
  uint16_t freq_raw = bcd_to_uint32(&data[5], 2);
  float frequency = freq_raw / 100.0f;
  if (frequency >= 45.0f && frequency <= 55.0f && frequency_sensor_) {
    frequency_sensor_->publish_state(frequency);
  }
  
  // Текущий тариф: 1 байт (1-4)
  uint8_t tariff = data[7] & 0x0F;
  if (tariff >= 1 && tariff <= 4 && tariff_sensor_) {
    tariff_sensor_->publish_state(tariff);
  }
  
  ESP_LOGD(TAG, "Parsed freq: %.2f Hz, tariff: %d", frequency, tariff);
  return true;
}

bool Mercury206Component::parse_datetime_response(const uint8_t *data, uint8_t len) {
  // ADDR(4)+CMD(1)+timedate(7)+CRC(2) = 14 байт
  // len = 11
  if (len < 11 || data[4] != CMD_READ_DATETIME) return false;
  
  // Формат: dow-hh-mm-ss-dd-mon-yy (все в BCD)
  uint8_t yy = bcd_to_dec(data[11]);
  uint8_t mon = bcd_to_dec(data[10]);
  uint8_t dd = bcd_to_dec(data[9]);
  uint8_t hh = bcd_to_dec(data[6]);
  uint8_t mm = bcd_to_dec(data[7]);
  uint8_t ss = bcd_to_dec(data[8]);
  
  char dt_str[20];
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
  counter_ = 0;  // Сброс счётчика
  uint32_t start = millis();
  
  // Ждём первый байт
  while (millis() - start < timeout_ms && !available()) {
    yield();
  }
  
  // Читаем всё доступное
  while (available() && counter_ < max_len) {
    buffer[counter_++] = read();
  }
  
  delay(100);  // ← КРИТИЧНО: даём пакету "усохнуть"
  return counter_ > 0;
}

void Mercury206Component::poll_meter() {
  static const MercuryCommand commands[] = {
    CMD_READ_TARIFFS,   // 0x27
    CMD_READ_UIP,       // 0x63
    CMD_READ_FREQ,      // 0x81
    CMD_READ_DATETIME   // 0x21
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
    
    // === КРИТИЧНО: индексы для CRC как в старом рабочем коде ===
    uint8_t crc_len_idx = 0;  // Индекс для crc_check()
    uint8_t min_resp_len = 0; // Минимальная длина ответа
    
    switch (response[4]) {
      case CMD_READ_TARIFFS:  // 0x27
        crc_len_idx = 20;      // crc_check(Re_buf, 20)
        min_resp_len = 23;
        break;
      case CMD_READ_UIP:      // 0x63
        crc_len_idx = 11;      // crc_check(Re_buf, 11)
        min_resp_len = 14;
        break;
      case CMD_READ_FREQ:     // 0x81
        crc_len_idx = 14;      // crc_check(Re_buf, 14) ✅
        min_resp_len = 17;
        break;
      case CMD_READ_DATETIME: // 0x21
        crc_len_idx = 11;      // crc_check(Re_buf, 11)
        min_resp_len = 14;
        break;
      default:
        ESP_LOGW(TAG, "Unknown response command: 0x%02X", response[4]);
        retries++;
        continue;
    }
    
    // Проверка минимальной длины
    if (counter_ < min_resp_len) {
      ESP_LOGW(TAG, "Response too short for cmd 0x%02X: got %d, need %d", 
               response[4], counter_, min_resp_len);
      retries++;
      continue;
    }
    
    // Проверка CRC
    if (!verify_crc(response, crc_len_idx)) {
      crc_error_count_++;
      if (crc_errors_sensor_) crc_errors_sensor_->publish_state(crc_error_count_);
      ESP_LOGW(TAG, "CRC error for cmd 0x%02X (len_idx=%d, got=%d)", 
               response[4], crc_len_idx, counter_);
      retries++;
      continue;
    }
    
    // Парсинг
    bool parsed = false;
    switch (response[4]) {
      case CMD_READ_UIP:
        parsed = parse_uip_response(response, crc_len_idx);
        break;
      case CMD_READ_TARIFFS:
        parsed = parse_tariffs_response(response, crc_len_idx);
        break;
      case CMD_READ_FREQ:
        parsed = parse_freq_response(response, crc_len_idx);
        break;
      case CMD_READ_DATETIME:
        parsed = parse_datetime_response(response, crc_len_idx);
        break;
    }
    
    if (parsed) {
      success_count_++;
      cmd_index_ = (cmd_index_ + 1) % 4;
      return;
    }
    
    ESP_LOGW(TAG, "Failed to parse response for cmd 0x%02X", response[4]);
    retries++;
  }
  
  ESP_LOGE(TAG, "Failed to poll meter after %d retries (cmd=0x%02X)", 
           retry_count_, cmd);
  cmd_index_ = (cmd_index_ + 1) % 4;
}

}  // namespace mercury206
}  // namespace esphome