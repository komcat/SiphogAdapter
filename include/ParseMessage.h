#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

namespace SiphogLib {

  /**
   * @brief Parsed message data structure
   */
  class ParsedMessage {
  public:
    std::unordered_map<std::string, int32_t> rawValues;
    std::unordered_map<std::string, double> convertedValues;
  };

  /**
   * @brief Message parsing utilities for SiPhOG data
   */
  class ParseMessage {
  private:
    // Constants from Python's message_conversion.py
    static constexpr double REFERENCE_VOLTAGE = 2.5;
    static constexpr int ADC_BITS_24 = 23; // 24-bit ADC (1 bit for sign, 23 for value)
    static constexpr int ADC_BITS_10 = 10; // 10-bit ADC

    // Base ADC conversion function
    static double adcRawVoltageVConv(int32_t value, int bits);

    // Specialized voltage converters
    static double adcVoltage24BitConv(int32_t value);
    static double adcVoltage10BitConv(int32_t value);

    // SLED Current conversion
    static double adcSledCurrentMaConv(int32_t rawValue);

    // TEC Current conversion
    static double adcTecCurrentMaConv(int32_t rawValue);

    // SLED Power conversion
    static double adcSledPowerUwConv(int32_t rawValue);

    // Sagnac voltage conversion
    static double adcSagnacVConv(int32_t rawValue);

    // Sagnac power conversion
    static double adcSagnacPowerUwConv(int32_t rawValue);

    // Supply voltage conversion
    static double adcSupplyVoltageConv(int32_t rawValue);

    // MEMS temperature conversion
    static double memsRawTempCConv(int32_t value);

    // Temperature conversion
    static double adcTempCConv(int32_t adcCounts, double Rto, double beta, double RRef, int bitResolution);

    // SLED Temperature conversion
    static double adcSledTempCConv(int32_t value);

    // Case Temperature conversion
    static double adcCaseTempCConv(int32_t value);

    // Op Amp Temperature conversion
    static double adcOpAmpTempCConv(int32_t value);

    // PIC thermistor conversion
    static double adcPicThermistorTempCConv(int32_t value, double slope, double offset);

  public:
    /**
     * @brief Parse a factory message from raw bytes
     * @param data Raw byte data (must be 76 bytes)
     * @return ParsedMessage containing raw and converted values
     */
    static ParsedMessage parseFactoryMessage(const std::vector<uint8_t>& data);
  };

} // namespace SiphogLib