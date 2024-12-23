#pragma once

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"


namespace esphome {
namespace climate_ir_samsung {

#define GETBITS8(data, offset, size) \
    (((data) & (((uint8_t)UINT8_MAX >> (8 - (size))) << (offset))) >> (offset))

    static const char *const TAG = "samsung.climate";
    static const uint32_t SAMSUNG_IR_FREQUENCY = 38000;
    static const int SAMSUNG_AIRCON1_HDR_MARK = 3000;
    static const int SAMSUNG_AIRCON1_HDR_SPACE = 9000;
    static const int SAMSUNG_AIRCON1_BIT_MARK = 500;
    static const int SAMSUNG_AIRCON1_ONE_SPACE = 1500;
    static const int SAMSUNG_AIRCON1_ZERO_SPACE = 500;
    static const int SAMSUNG_AIRCON1_MSG_SPACE = 2000;

    const uint16_t kSamsungAcExtendedStateLength = 21;
    const uint16_t kSamsungAcSectionLength = 7;

    // Temperature
    const uint8_t kSamsungAcMinTemp  = 16;  // C   Mask 0b11110000
    const uint8_t kSamsungAcMaxTemp  = 30;  // C   Mask 0b11110000
    const uint8_t kSamsungAcAutoTemp = 25;  // C   Mask 0b11110000

    // Mode
    const uint8_t kSamsungAcAuto = 0;
    const uint8_t kSamsungAcCool = 1;
    const uint8_t kSamsungAcDry = 2;
    const uint8_t kSamsungAcFan = 3;
    const uint8_t kSamsungAcHeat = 4;

    // Fan
    const uint8_t kSamsungAcFanAuto = 0;
    const uint8_t kSamsungAcFanLow = 2;
    const uint8_t kSamsungAcFanMed = 4;
    const uint8_t kSamsungAcFanHigh = 5;
    const uint8_t kSamsungAcFanAuto2 = 6;
    const uint8_t kSamsungAcFanTurbo = 7;

    // Swing
    const uint8_t kSamsungAcSwingV =        0b010;
    const uint8_t kSamsungAcSwingH =        0b011;
    const uint8_t kSamsungAcSwingBoth =     0b100;
    const uint8_t kSamsungAcSwingOff =      0b111;

    // Power
    const uint8_t kNibbleSize = 4;
    const uint8_t kLowNibble = 0;
    const uint8_t kHighNibble = 4;
    const uint8_t kModeBitsSize = 3;

    // static const uint8_t kReset[kSamsungAcExtendedStateLength] = {0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0};
    static const uint8_t kReset[kSamsungAcExtendedStateLength] = {
        0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
        0x01, 0x02, 0xAE, 0x71, 0x00, 0x15, 0xF0};

    /// Native representation of a Samsung A/C message.
    union SamsungProtocol {
        uint8_t raw[kSamsungAcExtendedStateLength];  ///< State in code form.
        struct {  // Standard message map
            // Byte 0
            uint8_t         :8;
            // Byte 1
            uint8_t         :4;
            uint8_t         :4;  // Sum1Lower
            // Byte 2
            uint8_t         :4;  // Sum1Upper
            uint8_t         :4;
            // Byte 3
            uint8_t         :8;
            // Byte 4
            uint8_t         :8;
            // Byte 5
            uint8_t         :4;
            uint8_t Sleep5  :1;
            uint8_t Quiet   :1;
            uint8_t         :2;
            // Byte 6
            uint8_t         :4;
            uint8_t Power1  :2;
            uint8_t         :2;
            // Byte 7
            uint8_t         :8;
            // Byte 8
            uint8_t         :4;
            uint8_t         :4;  // Sum2Lower
            // Byte 9
            uint8_t         :4;  // Sum1Upper
            uint8_t Swing   :3;
            uint8_t         :1;
            // Byte 10
            uint8_t               :1;
            uint8_t FanSpecial    :3;  // Powerful, Breeze/WindFree, Econo
            uint8_t Display       :1;
            uint8_t               :2;
            uint8_t CleanToggle10 :1;
            // Byte 11
            uint8_t Ion           :1;
            uint8_t CleanToggle11 :1;
            uint8_t               :2;
            uint8_t Temp          :4;
            // Byte 12
            uint8_t       :1;
            uint8_t Fan   :3;
            uint8_t Mode  :3;
            uint8_t       :1;
            // Byte 13
            uint8_t            :2;
            uint8_t BeepToggle :1;
            uint8_t            :1;
            uint8_t Power2     :2;
            uint8_t            :2;
        };
        struct {  // Extended message map
            // 1st Section
            // Byte 0
            uint8_t                :8;
            // Byte 1
            uint8_t                :4;
            uint8_t Sum1Lower      :4;
            // Byte 2
            uint8_t Sum1Upper      :4;
            uint8_t                :4;
            // Byte 3
            uint8_t                :8;
            // Byte 4
            uint8_t                :8;
            // Byte 5
            uint8_t                :8;
            // Byte 6
            uint8_t                :8;
            // 2nd Section
            // Byte 7
            uint8_t                :8;
            // Byte 8
            uint8_t                :4;
            uint8_t Sum2Lower      :4;
            // Byte 9
            uint8_t Sum2Upper      :4;
            uint8_t OffTimeMins    :3;  // In units of 10's of mins
            uint8_t OffTimeHrs1    :1;  // LSB of the number of hours.
            // Byte 10
            uint8_t OffTimeHrs2    :4;  // MSBs of the number of hours.
            uint8_t OnTimeMins     :3;  // In units of 10's of mins
            uint8_t OnTimeHrs1     :1;  // LSB of the number of hours.
            // Byte 11
            uint8_t OnTimeHrs2     :4;  // MSBs of the number of hours.
            uint8_t                :4;
            // Byte 12
            uint8_t OffTimeDay     :1;
            uint8_t OnTimerEnable  :1;
            uint8_t OffTimerEnable :1;
            uint8_t Sleep12        :1;
            uint8_t OnTimeDay      :1;
            uint8_t                :3;
            // Byte 13
            uint8_t                :8;
            // 3rd Section
            // Byte 14
            uint8_t                :8;
            // Byte 15
            uint8_t                :4;
            uint8_t Sum3Lower      :4;
            // Byte 16
            uint8_t Sum3Upper      :4;
            uint8_t                :4;
            // Byte 17
            uint8_t                :8;
            // Byte 18
            uint8_t                :8;
            // Byte 19
            uint8_t                :8;
            // Byte 20
            uint8_t                :8;
        };
    };

    class SamsungClimateIR : public climate_ir::ClimateIR {

        SamsungProtocol protocol;
        climate::ClimateMode current_climate_mode;

        public: SamsungClimateIR() :
            climate_ir::ClimateIR(
                kSamsungAcMinTemp, kSamsungAcMaxTemp, 1.0f, true, true,
                {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) { }

        protected:
            void transmit_state() override;

            void send();
            void setSwing(const climate::ClimateSwingMode swingMode);
            void setMode(const climate::ClimateMode mode);
            void setTemp(const uint8_t temp);
            void setAndSendPowerState(const bool on);
            void setFan(const climate::ClimateFanMode fanMode);

            void checksum(void);
            static uint8_t calcSectionChecksum(const uint8_t *section);
            static uint16_t countBits(const uint8_t * const start, const uint16_t length, const bool ones = true, const uint16_t init = 0);
            static uint16_t countBits(const uint64_t data, const uint8_t length, const bool ones = true, const uint16_t init = 0);
    };
}}
