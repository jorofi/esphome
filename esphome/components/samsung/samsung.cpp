#include "samsung.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace samsung {

static const char *const TAG = "samsung.climate";

void SamsungClimate::transmit_state() {
  if (current_climate_mode_ != climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode == climate::ClimateMode::CLIMATE_MODE_OFF) {
    this->send_power_state_(false);
    return;
  }

  if (current_climate_mode_ == climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode != climate::ClimateMode::CLIMATE_MODE_OFF) {
    this->send_power_state_(true);
  }

  current_climate_mode_ = this->mode;

  this->set_climate_mode_(this->mode);
  this->set_temp_(this->target_temperature);
  this->set_swing_mode_(this->swing_mode);
  this->set_fan_(this->fan_mode.has_value() ? this->fan_mode.value() : climate::CLIMATE_FAN_AUTO);

  this->send_();
}

bool SamsungClimate::on_receive(remote_base::RemoteReceiveData data) {
  if (!data.expect_item(SAMSUNG_AIRCON1_HDR_MARK, SAMSUNG_AIRCON1_HDR_SPACE)) {
    return false;
  }

  ESP_LOGD(TAG, "Received Samsung A/C message");
  for (uint8_t i = 0; i < 14; i++) {
    if (i == 7) {
      if (data.expect_item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_MSG_SPACE)) {
        ESP_LOGD(TAG, "Received MSG_SPACE %d", i);
      } else {
        ESP_LOGD(TAG, "Failed to receive MSG_SPACE %d", i);
      }

      if (data.expect_item(SAMSUNG_AIRCON1_HDR_MARK, SAMSUNG_AIRCON1_HDR_SPACE)) {
        ESP_LOGD(TAG, "Received HDR_SPACE %d", i);
      } else {
        ESP_LOGD(TAG, "Failed to receive HDR_SPACE %d", i);
      }
    }

    for (uint8_t y = 0; y < 8; y++) {
      if (data.expect_item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_ONE_SPACE)) {
        protocol_.raw[i] |= 1 << y;
      } else if (data.expect_item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_ZERO_SPACE)) {
        protocol_.raw[i] &= ~(1 << y);
      } else {
        ESP_LOGD(TAG, "Failed to receive bit %d, %d", i, y);
      }
    }
  }

  ESP_LOGD(TAG, "update_swing_mode_");
  this->update_swing_mode_();

  ESP_LOGD(TAG, "update_climate_mode_");
  this->update_climate_mode_();

  ESP_LOGD(TAG, "update_temp_");
  this->update_temp_();

  ESP_LOGD(TAG, "update_fan_");
  this->update_fan_();

  ESP_LOGD(TAG, "update_power_");
  this->update_power_();

  ESP_LOGD(TAG, "publish_state");
  this->publish_state();

  return true;
}

void SamsungClimate::send_() {
  this->checksum_();

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();

  // Header(2) +  2 * MSG_HDR(2 * Item(2)) + 21 Bytes * 8 Bits * Bit(2) + Last Mark (1)
  data->reserve(2 + 2 * 2 * 2 + 21 * 8 * 2 + 1);
  data->set_carrier_frequency(SAMSUNG_IR_FREQUENCY_HZ);

  // Header
  data->item(SAMSUNG_AIRCON1_HDR_MARK, SAMSUNG_AIRCON1_HDR_SPACE);

  for (uint8_t i = 0; i < 21; i++) {
    if (i == 7 || i == 14) {
      data->item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_MSG_SPACE);
      data->item(SAMSUNG_AIRCON1_HDR_MARK, SAMSUNG_AIRCON1_HDR_SPACE);
    }

    uint8_t send_byte = protocol_.raw[i];

    for (uint8_t y = 0; y < 8; y++) {
      if (send_byte & 0x01) {
        data->item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_ONE_SPACE);
      } else {
        data->item(SAMSUNG_AIRCON1_BIT_MARK, SAMSUNG_AIRCON1_ZERO_SPACE);
      }

      send_byte >>= 1;
    }
  }

  data->mark(SAMSUNG_AIRCON1_BIT_MARK);

  transmit.perform();
}

void SamsungClimate::set_swing_mode_(const climate::ClimateSwingMode swing_mode) {
  switch (swing_mode) {
    case climate::ClimateSwingMode::CLIMATE_SWING_BOTH:
      protocol_.swing = K_SAMSUNG_AC_SWING_BOTH;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL:
      protocol_.swing = K_SAMSUNG_AC_SWING_H;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL:
      protocol_.swing = K_SAMSUNG_AC_SWING_V;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_OFF:
    default:
      protocol_.swing = K_SAMSUNG_AC_SWING_OFF;
      break;
  }
}

void SamsungClimate::update_swing_mode_() {
  switch (protocol_.swing) {
    case K_SAMSUNG_AC_SWING_BOTH:
      this->swing_mode = climate::ClimateSwingMode::CLIMATE_SWING_BOTH;
      break;
    case K_SAMSUNG_AC_SWING_H:
      this->swing_mode = climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL;
      break;
    case K_SAMSUNG_AC_SWING_V:
      this->swing_mode = climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL;
      break;
    case K_SAMSUNG_AC_SWING_OFF:
    default:
      this->swing_mode = climate::ClimateSwingMode::CLIMATE_SWING_OFF;
      break;
  }
}

void SamsungClimate::set_climate_mode_(const climate::ClimateMode climate_mode) {
  switch (climate_mode) {
    case climate::ClimateMode::CLIMATE_MODE_HEAT:
      protocol_.mode = K_SAMSUNG_AC_HEAT;
      break;
    case climate::ClimateMode::CLIMATE_MODE_DRY:
      protocol_.mode = K_SAMSUNG_AC_DRY;
      break;
    case climate::ClimateMode::CLIMATE_MODE_COOL:
      protocol_.mode = K_SAMSUNG_AC_COOL;
      break;
    case climate::ClimateMode::CLIMATE_MODE_FAN_ONLY:
      protocol_.mode = K_SAMSUNG_AC_FAN;
      break;
    case climate::ClimateMode::CLIMATE_MODE_HEAT_COOL:
    case climate::ClimateMode::CLIMATE_MODE_AUTO:
    default:
      protocol_.mode = K_SAMSUNG_AC_AUTO;
      break;
  }
}

void SamsungClimate::update_climate_mode_() {
  switch (protocol_.mode) {
    case K_SAMSUNG_AC_HEAT:
      this->mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
      break;
    case K_SAMSUNG_AC_DRY:
      this->mode = climate::ClimateMode::CLIMATE_MODE_DRY;
      break;
    case K_SAMSUNG_AC_COOL:
      this->mode = climate::ClimateMode::CLIMATE_MODE_COOL;
      break;
    case K_SAMSUNG_AC_FAN:
      this->mode = climate::ClimateMode::CLIMATE_MODE_FAN_ONLY;
      break;
    case K_SAMSUNG_AC_AUTO:
    default:
      this->mode = climate::ClimateMode::CLIMATE_MODE_AUTO;
      break;
  }
}

void SamsungClimate::set_temp_(const uint8_t temp) {
  protocol_.temp = esphome::clamp<uint8_t>(temp, K_SAMSUNG_AC_MIN_TEMP, K_SAMSUNG_AC_MAX_TEMP);
}

void SamsungClimate::update_temp_() { this->target_temperature = protocol_.temp + K_SAMSUNG_AC_MIN_TEMP; }

void SamsungClimate::send_power_state_(const bool on) {
  static const uint8_t K_ON[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH] = {0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
                                                                   0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                                   0x01, 0xE2, 0xFE, 0x71, 0x80, 0x11, 0xF0};

  static const uint8_t K_OFF[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH] = {0x02, 0xB2, 0x0F, 0x00, 0x00, 0x00, 0xC0,
                                                                    0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                                    0x01, 0x02, 0xFF, 0x71, 0x80, 0x11, 0xC0};

  std::memcpy(protocol_.raw, on ? K_ON : K_OFF, K_SAMSUNG_AC_EXTENDED_STATE_LENGTH);

  this->send_();

  std::memcpy(protocol_.raw, K_RESET, K_SAMSUNG_AC_EXTENDED_STATE_LENGTH);
}

void SamsungClimate::set_fan_(const climate::ClimateFanMode fan_mode) {
  switch (fan_mode) {
    case climate::ClimateFanMode::CLIMATE_FAN_LOW:
      protocol_.fan = K_SAMSUNG_AC_FAN_LOW;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_MEDIUM:
      protocol_.fan = K_SAMSUNG_AC_FAN_MED;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_HIGH:
      protocol_.fan = K_SAMSUNG_AC_FAN_HIGH;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_AUTO:
    default:
      protocol_.fan = K_SAMSUNG_AC_FAN_AUTO;
      break;
  }
}

void SamsungClimate::update_fan_() {
  switch (protocol_.fan) {
    case K_SAMSUNG_AC_FAN_LOW:
      this->fan_mode = climate::ClimateFanMode::CLIMATE_FAN_LOW;
      break;
    case K_SAMSUNG_AC_FAN_MED:
      this->fan_mode = climate::ClimateFanMode::CLIMATE_FAN_MEDIUM;
      break;
    case K_SAMSUNG_AC_FAN_HIGH:
      this->fan_mode = climate::ClimateFanMode::CLIMATE_FAN_HIGH;
      break;
    case K_SAMSUNG_AC_FAN_AUTO:
    default:
      this->fan_mode = climate::ClimateFanMode::CLIMATE_FAN_AUTO;
      break;
  }
}

void SamsungClimate::update_power_() {
  ESP_LOGD(TAG, "power_1: 0x%02X, power_2: 0x%02X", protocol_.power_1, protocol_.power_2);
  if (protocol_.power_1 != 0x03) {
    this->mode = climate::ClimateMode::CLIMATE_MODE_OFF;
  }
}

uint8_t SamsungClimate::calc_section_checksum(const uint8_t *section) {
  uint8_t sum = 0;

  sum += count_bits(*section, 8);  // Include the entire first byte
  // The lower half of the second byte.
  sum += count_bits(GETBITS8(*(section + 1), K_LOW_NIBBLE, K_NIBBLE_SIZE), 8);
  // The upper half of the third byte.
  sum += count_bits(GETBITS8(*(section + 2), K_HIGH_NIBBLE, K_NIBBLE_SIZE), 8);
  // The next 4 bytes.
  sum += count_bits(section + 3, 4);
  // Bitwise invert the result.
  return sum ^ UINT8_MAX;
}

void SamsungClimate::checksum_() {
  uint8_t sectionsum = calc_section_checksum(protocol_.raw);
  protocol_.sum_1_upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.sum_1_lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
  sectionsum = calc_section_checksum(protocol_.raw + K_SAMSUNG_AC_SECTION_LENGTH);
  protocol_.sum_2_upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.sum_2_lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
  sectionsum = calc_section_checksum(protocol_.raw + K_SAMSUNG_AC_SECTION_LENGTH * 2);
  protocol_.sum_3_upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.sum_3_lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
}

uint16_t SamsungClimate::count_bits(const uint8_t *const start, const uint16_t length, const bool ones,
                                    const uint16_t init) {
  uint16_t count = init;

  for (uint16_t offset = 0; offset < length; offset++) {
    for (uint8_t currentbyte = *(start + offset); currentbyte; currentbyte >>= 1) {
      if (currentbyte & 1)
        count++;
    }
  }

  if (ones || length == 0) {
    return count;
  } else {
    return (length * 8) - count;
  }
}

uint16_t SamsungClimate::count_bits(const uint64_t data, const uint8_t length, const bool ones, const uint16_t init) {
  uint16_t count = init;
  uint8_t bits_so_far = length;

  for (uint64_t remainder = data; remainder && bits_so_far; remainder >>= 1, bits_so_far--) {
    if (remainder & 1)
      count++;
  }

  if (ones || length == 0) {
    return count;
  } else {
    return length - count;
  }
}
}  // namespace samsung
}  // namespace esphome
