#include "climate_ir_samsung.h"

namespace esphome {
namespace climate_ir_samsung {

void SamsungClimateIR::transmit_state() {
  if (current_climate_mode != climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode == climate::ClimateMode::CLIMATE_MODE_OFF) {
    setAndSendPowerState(false);
    return;
  }

  if (current_climate_mode == climate::ClimateMode::CLIMATE_MODE_OFF &&
      this->mode != climate::ClimateMode::CLIMATE_MODE_OFF) {
    setAndSendPowerState(true);
  }

  current_climate_mode = this->mode;

  setMode(this->mode);
  setTemp(this->target_temperature);
  setSwing(this->swing_mode);
  setFan(this->fan_mode.has_value() ? this->fan_mode.value() : climate::CLIMATE_FAN_AUTO);

  send();
}

/// Send the current state of the climate object.
void SamsungClimateIR::send() {
  checksum();

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

    uint8_t sendByte = protocol.raw[i];

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
void SamsungClimateIR::setSwing(const climate::ClimateSwingMode swingMode) {
  switch (swingMode) {
    case climate::ClimateSwingMode::CLIMATE_SWING_BOTH:
      protocol.Swing = kSamsungAcSwingBoth;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_HORIZONTAL:
      protocol.Swing = kSamsungAcSwingH;
      ;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_VERTICAL:
      protocol.Swing = kSamsungAcSwingV;
      ;
      break;
    case climate::ClimateSwingMode::CLIMATE_SWING_OFF:
    default:
      protocol.Swing = kSamsungAcSwingOff;
      ;
      break;
  }
}

/// Set the operating mode of the A/C.
/// @param[in] climateMode The desired operating mode.
void SamsungClimateIR::setMode(const climate::ClimateMode climateMode) {
  switch (climateMode) {
    case climate::ClimateMode::CLIMATE_MODE_HEAT:
      protocol.Mode = kSamsungAcHeat;
      break;
    case climate::ClimateMode::CLIMATE_MODE_DRY:
      protocol.Mode = kSamsungAcDry;
      break;
    case climate::ClimateMode::CLIMATE_MODE_COOL:
      protocol.Mode = kSamsungAcCool;
      break;
    case climate::ClimateMode::CLIMATE_MODE_FAN_ONLY:
      protocol.Mode = kSamsungAcFan;
      break;
    case climate::ClimateMode::CLIMATE_MODE_HEAT_COOL:
    case climate::ClimateMode::CLIMATE_MODE_AUTO:
    default:
      protocol.Mode = kSamsungAcAuto;
      break;
  }
}

/// Set the temperature.
/// @param[in] temp The temperature in degrees celsius.
void SamsungClimateIR::setTemp(const uint8_t temp) {
  uint8_t newtemp = std::max(kSamsungAcMinTemp, temp);
  newtemp = std::min(kSamsungAcMaxTemp, newtemp);
  protocol.Temp = newtemp - kSamsungAcMinTemp;
}

/// Change the AC power state.
/// @param[in] on true, the AC is on. false, the AC is off.
void SamsungClimateIR::setAndSendPowerState(const bool on) {
  static const uint8_t kOn[kSamsungAcExtendedStateLength] = {0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
                                                             0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                             0x01, 0xE2, 0xFE, 0x71, 0x80, 0x11, 0xF0};

  static const uint8_t kOff[kSamsungAcExtendedStateLength] = {0x02, 0xB2, 0x0F, 0x00, 0x00, 0x00, 0xC0,
                                                              0x01, 0xD2, 0x0F, 0x00, 0x00, 0x00, 0x00,
                                                              0x01, 0x02, 0xFF, 0x71, 0x80, 0x11, 0xC0};

  std::memcpy(protocol.raw, on ? kOn : kOff, kSamsungAcExtendedStateLength);

  send();

  std::memcpy(protocol.raw, kReset, kSamsungAcExtendedStateLength);
}

/// Set the fan speed.
void SamsungClimateIR::setFan(const climate::ClimateFanMode fanMode) {
  switch (fanMode) {
    case climate::ClimateFanMode::CLIMATE_FAN_LOW:
      protocol.Fan = kSamsungAcFanAuto;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_MEDIUM:
      protocol.Fan = kSamsungAcFanMed;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_HIGH:
      protocol.Fan = kSamsungAcFanHigh;
      break;
    case climate::ClimateFanMode::CLIMATE_FAN_AUTO:
    default:
      protocol.Fan = kSamsungAcFanAuto;
      break;
  }
}

/// Calculate the checksum for a given state section.
/// @param[in] section The array to calc the checksum of.
/// @return The calculated checksum value.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1538#issuecomment-894645947
uint8_t SamsungClimateIR::calcSectionChecksum(const uint8_t *section) {
  uint8_t sum = 0;

  sum += countBits(*section, 8);  // Include the entire first byte
  // The lower half of the second byte.
  sum += countBits(GETBITS8(*(section + 1), kLowNibble, kNibbleSize), 8);
  // The upper half of the third byte.
  sum += countBits(GETBITS8(*(section + 2), kHighNibble, kNibbleSize), 8);
  // The next 4 bytes.
  sum += countBits(section + 3, 4);
  // Bitwise invert the result.
  return sum ^ UINT8_MAX;
}

/// Update the checksum for the internal state.
void SamsungClimateIR::checksum(void) {
  uint8_t sectionsum = calcSectionChecksum(protocol.raw);
  protocol.Sum1Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  protocol.Sum1Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
  sectionsum = calcSectionChecksum(protocol.raw + kSamsungAcSectionLength);
  protocol.Sum2Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  protocol.Sum2Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
  sectionsum = calcSectionChecksum(protocol.raw + kSamsungAcSectionLength * 2);
  protocol.Sum3Upper = GETBITS8(sectionsum, kHighNibble, kNibbleSize);
  protocol.Sum3Lower = GETBITS8(sectionsum, kLowNibble, kNibbleSize);
}

/// Count the number of bits of a certain type in an array.
/// @param[in] start A ptr to the start of the byte array to calculate over.
/// @param[in] length How many bytes to use in the calculation.
/// @param[in] ones Count the binary nr of `1` bits. False is count the `0`s.
/// @param[in] init Starting value of the calculation to use. (Default is 0)
/// @return The nr. of bits found of the given type found in the array.
uint16_t SamsungClimateIR::countBits(const uint8_t *const start, const uint16_t length, const bool ones,
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
uint16_t SamsungClimateIR::countBits(const uint64_t data, const uint8_t length, const bool ones, const uint16_t init) {
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
