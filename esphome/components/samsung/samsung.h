#pragma once

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace samsung {

#define GETBITS8(data, offset, size) (((data) & (((uint8_t) UINT8_MAX >> (8 - (size))) << (offset))) >> (offset))

static const uint32_t SAMSUNG_IR_FREQUENCY_HZ = 38000;
static const uint32_t SAMSUNG_AIRCON1_HDR_MARK = 3000;
static const uint32_t SAMSUNG_AIRCON1_HDR_SPACE = 9000;
static const uint32_t SAMSUNG_AIRCON1_BIT_MARK = 500;
static const uint32_t SAMSUNG_AIRCON1_ONE_SPACE = 1500;
static const uint32_t SAMSUNG_AIRCON1_ZERO_SPACE = 500;
static const uint32_t SAMSUNG_AIRCON1_MSG_SPACE = 2000;

const uint16_t K_SAMSUNG_AC_EXTENDED_STATE_LENGTH = 21;
const uint16_t K_SAMSUNG_AC_SECTION_LENGTH = 7;

// Temperature
const uint8_t K_SAMSUNG_AC_MIN_TEMP = 16;  // C   Mask 0b11110000
const uint8_t K_SAMSUNG_AC_MAX_TEMP = 30;  // C   Mask 0b11110000

// Mode
const uint8_t K_SAMSUNG_AC_AUTO = 0;
const uint8_t K_SAMSUNG_AC_COOL = 1;
const uint8_t K_SAMSUNG_AC_DRY = 2;
const uint8_t K_SAMSUNG_AC_FAN = 3;
const uint8_t K_SAMSUNG_AC_HEAT = 4;

// Fan
const uint8_t K_SAMSUNG_AC_FAN_AUTO = 0;
const uint8_t K_SAMSUNG_AC_FAN_LOW = 2;
const uint8_t K_SAMSUNG_AC_FAN_MED = 4;
const uint8_t K_SAMSUNG_AC_FAN_HIGH = 5;

// Swing
const uint8_t K_SAMSUNG_AC_SWING_V = 0b010;
const uint8_t K_SAMSUNG_AC_SWING_H = 0b011;
const uint8_t K_SAMSUNG_AC_SWING_BOTH = 0b100;
const uint8_t K_SAMSUNG_AC_SWING_OFF = 0b111;

// Power
const uint8_t K_NIBBLE_SIZE = 4;
const uint8_t K_LOW_NIBBLE = 0;
const uint8_t K_HIGH_NIBBLE = 4;

static const uint8_t K_RESET[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH] = {0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
                                                                    0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0};

/// Native representation of a Samsung A/C message.
union SamsungProtocol {
  uint8_t raw[K_SAMSUNG_AC_EXTENDED_STATE_LENGTH];  ///< State in code form.
  struct {                                          // Standard message map
    // Byte 0
    uint8_t : 8;
    // Byte 1
    uint8_t : 4;
    uint8_t : 4;  // Sum1Lower
    // Byte 2
    uint8_t : 4;  // Sum1Upper
    uint8_t : 4;
    // Byte 3
    uint8_t : 8;
    // Byte 4
    uint8_t : 8;
    // Byte 5
    uint8_t : 4;
    uint8_t Sleep5 : 1;
    uint8_t Quiet : 1;
    uint8_t : 2;
    // Byte 6
    uint8_t : 4;
    uint8_t Power1 : 2;
    uint8_t : 2;
    // Byte 7
    uint8_t : 8;
    // Byte 8
    uint8_t : 4;
    uint8_t : 4;  // Sum2Lower
    // Byte 9
    uint8_t : 4;  // Sum1Upper
    uint8_t Swing : 3;
    uint8_t : 1;
    // Byte 10
    uint8_t : 1;
    uint8_t FanSpecial : 3;  // Powerful, Breeze/WindFree, Econo
    uint8_t Display : 1;
    uint8_t : 2;
    uint8_t CleanToggle10 : 1;
    // Byte 11
    uint8_t Ion : 1;
    uint8_t CleanToggle11 : 1;
    uint8_t : 2;
    uint8_t Temp : 4;
    // Byte 12
    uint8_t : 1;
    uint8_t Fan : 3;
    uint8_t Mode : 3;
    uint8_t : 1;
    // Byte 13
    uint8_t : 2;
    uint8_t BeepToggle : 1;
    uint8_t : 1;
    uint8_t Power2 : 2;
    uint8_t : 2;
  };
  struct {  // Extended message map
    // 1st Section
    // Byte 0
    uint8_t : 8;
    // Byte 1
    uint8_t : 4;
    uint8_t Sum1Lower : 4;
    // Byte 2
    uint8_t Sum1Upper : 4;
    uint8_t : 4;
    // Byte 3
    uint8_t : 8;
    // Byte 4
    uint8_t : 8;
    // Byte 5
    uint8_t : 8;
    // Byte 6
    uint8_t : 8;
    // 2nd Section
    // Byte 7
    uint8_t : 8;
    // Byte 8
    uint8_t : 4;
    uint8_t Sum2Lower : 4;
    // Byte 9
    uint8_t Sum2Upper : 4;
    uint8_t OffTimeMins : 3;  // In units of 10's of mins
    uint8_t OffTimeHrs1 : 1;  // LSB of the number of hours.
    // Byte 10
    uint8_t OffTimeHrs2 : 4;  // MSBs of the number of hours.
    uint8_t OnTimeMins : 3;   // In units of 10's of mins
    uint8_t OnTimeHrs1 : 1;   // LSB of the number of hours.
    // Byte 11
    uint8_t OnTimeHrs2 : 4;  // MSBs of the number of hours.
    uint8_t : 4;
    // Byte 12
    uint8_t OffTimeDay : 1;
    uint8_t OnTimerEnable : 1;
    uint8_t OffTimerEnable : 1;
    uint8_t Sleep12 : 1;
    uint8_t OnTimeDay : 1;
    uint8_t : 3;
    // Byte 13
    uint8_t : 8;
    // 3rd Section
    // Byte 14
    uint8_t : 8;
    // Byte 15
    uint8_t : 4;
    uint8_t Sum3Lower : 4;
    // Byte 16
    uint8_t Sum3Upper : 4;
    uint8_t : 4;
    // Byte 17
    uint8_t : 8;
    // Byte 18
    uint8_t : 8;
    // Byte 19
    uint8_t : 8;
    // Byte 20
    uint8_t : 8;
  };
};

class SamsungClimate : public climate_ir::ClimateIR {
  SamsungProtocol protocol_;
  climate::ClimateMode current_climate_mode_;

 public:
  SamsungClimate()
      : climate_ir::ClimateIR(K_SAMSUNG_AC_MIN_TEMP, K_SAMSUNG_AC_MAX_TEMP, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                               climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

 protected:
  /// Transmit via IR the state of this climate controller
  void transmit_state() override;

  /// Send the current state of the climate object.
  void send_();
  /// Change the AC power state.
  /// @param[in] on true, the AC is on. false, the AC is off.
  void send_power_state_(bool on);
  /// Set the swing setting of the A/C.
  void set_swing_(climate::ClimateSwingMode swing_mode);
  /// Set the operating mode of the A/C.
  /// @param[in] climate_mode The desired operating mode.
  void set_mode_(climate::ClimateMode climate_mode);
  /// Set the temperature.
  /// @param[in] temp The temperature in degrees celsius.
  void set_temp_(uint8_t temp);
  /// Set the fan speed.
  void set_fan_(climate::ClimateFanMode fan_mode);
  /// Update the checksum_ for the internal state.
  void checksum_();
  /// Calculate the checksum_ for a given state section.
  /// @param[in] section The array to calc the checksum_ of.
  /// @return The calculated checksum_ value.
  /// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1538#issuecomment-894645947
  static uint8_t calc_section_checksum(const uint8_t *section);
  /// Count the number of bits of a certain type in an array.
  /// @param[in] start A ptr to the start of the byte array to calculate over.
  /// @param[in] length How many bytes to use in the calculation.
  /// @param[in] ones Count the binary nr of `1` bits. False is count the `0`s.
  /// @param[in] init Starting value of the calculation to use. (Default is 0)
  /// @return The nr. of bits found of the given type found in the array.
  static uint16_t count_bits(const uint8_t *start, uint16_t length, bool ones = true, uint16_t init = 0);
  /// Count the number of bits of a certain type in an Integer.
  /// @param[in] data The value you want bits counted for. Starting from the LSB.
  /// @param[in] length How many bits to use in the calculation? Starts at the LSB
  /// @param[in] ones Count the binary nr of `1` bits. False is count the `0`s.
  /// @param[in] init Starting value of the calculation to use. (Default is 0)
  /// @return The nr. of bits found of the given type found in the Integer.
  static uint16_t count_bits(uint64_t data, uint8_t length, bool ones = true, uint16_t init = 0);
};
}  // namespace samsung
}  // namespace esphome
