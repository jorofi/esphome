#include "climate_ir_samsung.h"

namespace esphome {
namespace climate_ir_samsung {

void SamsungClimateIR::transmit_state() {
  if (current_climate_mode_ != climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode == climate::ClimateMode::CLIMATE_MODE_OFF) {
    set_and_send_power_state_(false);
    return;
  }

  if (current_climate_mode_ == climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode != climate::ClimateMode::CLIMATE_MODE_OFF) {
    set_and_send_power_state_(true);
  }

  current_climate_mode_ = this->mode;

  set_mode_(this->mode);
  set_temp_(this->target_temperature);
  set_swing_(this->swing_mode);
  set_fan_(this->fan_mode.has_value() ? this->fan_mode.value() : climate::CLIMATE_FAN_AUTO);

  send_();
}

/// Send the current state of the climate object.
void SamsungClimateIR::send_() {
  checksum_();

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(SAMSUNG_IR_FREQUENCY);

  // Header
  data->mark(SAMSUNG_AIRCON1_HDR_MARK);
  data->space(SAMSUNG_AIRCON1_HDR_SPACE);

  for (int i = 0; i < 21; i++) {
    if (i == 7 || i == 14) {
      data->mark(SAMSUNG_AIRCON1_BIT_MARK);
      data->space(SAMSUNG_AIRCON1_MSG_SPACE);

      data->mark(SAMSUNG_AIRCON1_HDR_MARK);
      data->space(SAMSUNG_AIRCON1_HDR_SPACE);
    }

    uint8_t sendByte = protocol_.raw[i];

    for (int y = 0; y < 8; y++) {
      if (sendByte & 0x01) {
        // ESP_LOGI(TAG, "For Y %d 1", y);
        data->mark(SAMSUNG_AIRCON1_BIT_MARK);
        data->space(SAMSUNG_AIRCON1_ONE_SPACE);
      } else {
        // ESP_LOGI(TAG, "For Y %d 0", y);
        data->mark(SAMSUNG_AIRCON1_BIT_MARK);
        data->space(SAMSUNG_AIRCON1_ZERO_SPACE);
      }

      sendByte >>= 1;
    }
  }

  data->mark(SAMSUNG_AIRCON1_BIT_MARK);
  data->space(0);

  transmit.perform();
}

/// Set the vertical swing setting of the A/C.
void SamsungClimateIR::set_swing_(const climate::ClimateSwingMode swing_mode) {
  switch (swing_mode) {
    case climate::ClimateSwingMode::CLIMATE_SWING_BOTH:
      protocol_.Swing = K_SAMSUNG_AC_SWING_BOTH;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL:
      protocol_.Swing = K_SAMSUNG_AC_SWING_H;
      ;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL:
      protocol_.Swing = K_SAMSUNG_AC_SWING_V;
      ;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_OFF:
    default:
      protocol_.Swing = K_SAMSUNG_AC_SWING_OFF;
      ;
      break;
  }
}

/// Set the operating mode of the A/C.
/// @param[in] climateMode The desired operating mode.
void SamsungClimateIR::set_mode_(const climate::ClimateMode climate_mode) {
  switch (climate_mode) {
    case climate::ClimateMode::CLIMATE_MODE_HEAT:
      protocol_.Mode = K_SAMSUNG_AC_HEAT;
      break;
    case climate::ClimateMode::CLIMATE_MODE_DRY:
      protocol_.Mode = K_SAMSUNG_AC_DRY;
      break;
    case climate::ClimateMode::CLIMATE_MODE_COOL:
      protocol_.Mode = K_SAMSUNG_AC_COOL;
      break;
    case climate::ClimateMode::CLIMATE_MODE_FAN_ONLY:
      protocol_.Mode = K_SAMSUNG_AC_FAN;
      break;
    case climate::ClimateMode::CLIMATE_MODE_HEAT_COOL:
    case climate::ClimateMode::CLIMATE_MODE_AUTO:
    default:
      protocol_.Mode = K_SAMSUNG_AC_AUTO;
      break;
  }
}

/// Set the temperature.
/// @param[in] temp The temperature in degrees celsius.
void SamsungClimateIR::set_temp_(const uint8_t temp) {
  uint8_t newtemp = std::max(K_SAMSUNG_AC_MIN_TEMP, temp);
  newtemp = std::min(K_SAMSUNG_AC_MAX_TEMP, newtemp);
  protocol_.Temp = newtemp - K_SAMSUNG_AC_MIN_TEMP;
}

/// Change the AC power state.
/// @param[in] on true, the AC is on. false, the AC is off.
void SamsungClimateIR::set_and_send_power_state_(const bool on) {
  static const uint8_t kOn[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH] = {0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
                                                             0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                             0x01, 0xE2, 0xFE, 0x71, 0x80, 0x11, 0xF0};

  static const uint8_t kOff[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH] = {0x02, 0xB2, 0x0F, 0x00, 0x00, 0x00, 0xC0,
                                                              0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                              0x01, 0x02, 0xFF, 0x71, 0x80, 0x11, 0xC0};

  std::memcpy(protocol_.raw, on ? kOn : kOff, K_SAMSUNG_AC_EXTENDED_STATE_LENGTH);

  send_();

  std::memcpy(protocol_.raw, K_RESET, K_SAMSUNG_AC_EXTENDED_STATE_LENGTH);
}

/// Set the fan speed.
void SamsungClimateIR::set_fan_(const climate::ClimateFanMode fan_mode) {
  switch (fan_mode) {
    case climate::ClimateFanMode::CLIMATE_FAN_LOW:
      protocol_.Fan = K_SAMSUNG_AC_FAN_LOW;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_MEDIUM:
      protocol_.Fan = K_SAMSUNG_AC_FAN_MED;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_HIGH:
      protocol_.Fan = K_SAMSUNG_AC_FAN_HIGH;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_AUTO:
    default:
      protocol_.Fan = K_SAMSUNG_AC_FAN_AUTO;
      break;
  }
}

/// Calculate the checksum_ for a given state section.
/// @param[in] section The array to calc the checksum_ of.
/// @return The calculated checksum_ value.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1538#issuecomment-894645947
uint8_t SamsungClimateIR::calc_section_checksum(const uint8_t *section) {
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

/// Update the checksum_ for the internal state.
void SamsungClimateIR::checksum_(void) {
  uint8_t sectionsum = calc_section_checksum(protocol_.raw);
  protocol_.Sum1Upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.Sum1Lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
  sectionsum = calc_section_checksum(protocol_.raw + K_SAMSUNG_AC_SECTION_LENGTH);
  protocol_.Sum2Upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.Sum2Lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
  sectionsum = calc_section_checksum(protocol_.raw + K_SAMSUNG_AC_SECTION_LENGTH * 2);
  protocol_.Sum3Upper = GETBITS8(sectionsum, K_HIGH_NIBBLE, K_NIBBLE_SIZE);
  protocol_.Sum3Lower = GETBITS8(sectionsum, K_LOW_NIBBLE, K_NIBBLE_SIZE);
}

/// Count the number of bits of a certain type in an array.
/// @param[in] start A ptr to the start of the byte array to calculate over.
/// @param[in] length How many bytes to use in the calculation.
/// @param[in] ones Count the binary nr of `1` bits. False is count the `0`s.
/// @param[in] init Starting value of the calculation to use. (Default is 0)
/// @return The nr. of bits found of the given type found in the array.
uint16_t SamsungClimateIR::count_bits(const uint8_t *const start, const uint16_t length, const bool ones,
                                     const uint16_t init) {
  uint16_t count = init;

  for (uint16_t offset = 0; offset < length; offset++)
    for (uint8_t currentbyte = *(start + offset); currentbyte; currentbyte >>= 1)
      if (currentbyte & 1)
        count++;

  if (ones || length == 0)
    return count;
  else
    return (length * 8) - count;
}

/// Count the number of bits of a certain type in an Integer.
/// @param[in] data The value you want bits counted for. Starting from the LSB.
/// @param[in] length How many bits to use in the calculation? Starts at the LSB
/// @param[in] ones Count the binary nr of `1` bits. False is count the `0`s.
/// @param[in] init Starting value of the calculation to use. (Default is 0)
/// @return The nr. of bits found of the given type found in the Integer.
uint16_t SamsungClimateIR::count_bits(const uint64_t data, const uint8_t length, const bool ones, const uint16_t init) {
  uint16_t count = init;
  uint8_t bitsSoFar = length;

  for (uint64_t remainder = data; remainder && bitsSoFar; remainder >>= 1, bitsSoFar--)
    if (remainder & 1)
      count++;

  if (ones || length == 0)
    return count;
  else
    return length - count;
}
}  // namespace climate_ir_samsung
}  // namespace esphome
