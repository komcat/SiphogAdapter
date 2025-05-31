#include "ParseMessage.h"
#include <cmath>
#include <iostream>
#include <cstring>
#include <limits>
#include <functional>

namespace SiphogLib {

  // Base ADC conversion function
  double ParseMessage::adcRawVoltageVConv(int32_t value, int bits) {
    double fsr = std::pow(2, bits);  // Full Scale Range
    return value * REFERENCE_VOLTAGE / fsr;
  }

  // Specialized voltage converters
  double ParseMessage::adcVoltage24BitConv(int32_t value) {
    return adcRawVoltageVConv(value, ADC_BITS_24);
  }

  double ParseMessage::adcVoltage10BitConv(int32_t value) {
    return adcRawVoltageVConv(value, ADC_BITS_10);
  }

  // SLED Current conversion
  double ParseMessage::adcSledCurrentMaConv(int32_t rawValue) {
    return adcVoltage24BitConv(rawValue) * 1000.0 / (30.3030303030 * 0.3);
  }

  // TEC Current conversion
  double ParseMessage::adcTecCurrentMaConv(int32_t rawValue) {
    double raw_voltage = adcVoltage24BitConv(rawValue);
    return 1000.0 * 0.92 * (raw_voltage - 1.25 - 0.0375) / -0.525;
  }

  // SLED Power conversion
  double ParseMessage::adcSledPowerUwConv(int32_t rawValue) {
    constexpr double TIA_GAIN = 249 * 8.5;
    constexpr double SLED_PD_RESPONSIVITY = 0.8;
    constexpr double SLED_V_TO_uW_CONV = TIA_GAIN / SLED_PD_RESPONSIVITY / 1E6;
    double raw_voltage = adcVoltage24BitConv(rawValue);
    return raw_voltage / SLED_V_TO_uW_CONV;
  }

  // Sagnac voltage conversion
  double ParseMessage::adcSagnacVConv(int32_t rawValue) {
    double raw_voltage = adcVoltage24BitConv(rawValue);
    return 2.4686481683 - raw_voltage;
  }

  // Sagnac power conversion
  double ParseMessage::adcSagnacPowerUwConv(int32_t rawValue) {
    double rawadcV = adcSagnacVConv(rawValue);
    return rawadcV * 1.25 / 20000.0 * 1e6;
  }

  // Supply voltage conversion
  double ParseMessage::adcSupplyVoltageConv(int32_t rawValue) {
    double raw_voltage = adcVoltage24BitConv(rawValue);
    return raw_voltage * 2.0;
  }

  // MEMS temperature conversion
  double ParseMessage::memsRawTempCConv(int32_t value) {
    return (value / 256.0) + 25.0;
  }

  // Temperature conversion
  double ParseMessage::adcTempCConv(int32_t adc_counts, double Rto, double beta, double RRef, int bit_resolution) {
    // Convert ADC counts to voltage
    double adc_voltage = adcRawVoltageVConv(adc_counts, bit_resolution);
    if (adc_voltage == 0) {
      // Zero voltage will not give valid temperature reading
      std::cout << "ADC count and voltage is zero! " << adc_counts << " counts <==> " << adc_voltage << std::endl;
      return -999.0;
    }

    double thermistor_resistance = ((2.5 * RRef) / adc_voltage) - RRef;
    if (thermistor_resistance <= 0) {
      std::cout << "Thermistor resistance is too small: " << thermistor_resistance
        << ", voltage too high " << adc_voltage << ". Cannot compute temp (C)" << std::endl;
      return -999.0;
    }

    double temperature_C = beta / (beta / (273.0 + 25.0) - std::log(Rto / thermistor_resistance)) - 273.0;
    return temperature_C;
  }

  // SLED Temperature conversion
  double ParseMessage::adcSledTempCConv(int32_t value) {
    double Rto = 10000.0;
    double beta = 3950.0;
    double RRef = 10.0e3;

    double temp_C = adcTempCConv(value, Rto, beta, RRef, ADC_BITS_24);
    if (temp_C < -273) {
      std::cout << "ADC SLED Temp cannot be determined, setting it to " << temp_C << std::endl;
    }
    return temp_C;
  }

  // Case Temperature conversion
  double ParseMessage::adcCaseTempCConv(int32_t value) {
    double Rto = 10000.0;
    double beta = 3380.0;
    double RRef = 10.0e3;

    double temp_C = adcTempCConv(value, Rto, beta, RRef, ADC_BITS_10);
    if (temp_C < -273) {
      std::cout << "ADC Case Temp cannot be determined, setting it to " << temp_C << std::endl;
    }
    return temp_C;
  }

  // Op Amp Temperature conversion
  double ParseMessage::adcOpAmpTempCConv(int32_t value) {
    double Rto = 10000.0;
    double beta = 3380.0;
    double RRef = 10.0e3;

    double temp_C = adcTempCConv(value, Rto, beta, RRef, ADC_BITS_24);
    if (temp_C < -273) {
      std::cout << "ADC Op Amp Temp cannot be determined, setting it to " << temp_C << std::endl;
    }
    return temp_C;
  }

  // PIC thermistor conversion
  double ParseMessage::adcPicThermistorTempCConv(int32_t value, double slope, double offset) {
    return (slope * value) + offset;
  }

  // Helper function to read little-endian integers from byte array
  template<typename T>
  T readLittleEndian(const std::vector<uint8_t>& data, size_t offset) {
    T result = 0;
    if (offset + sizeof(T) > data.size()) {
      return result; // Return 0 if not enough data
    }
    for (size_t i = 0; i < sizeof(T); ++i) {
      result |= static_cast<T>(data[offset + i]) << (i * 8);
    }
    return result;
  }

  ParsedMessage ParseMessage::parseFactoryMessage(const std::vector<uint8_t>& data) {
    ParsedMessage parsedMessage;
    size_t offset = 3; // Start after message type and length bytes

    if (data.size() < 76) {
      std::cout << "Error: Message too short, expected 76 bytes, got " << data.size() << std::endl;
      return parsedMessage;
    }

    // Define parsing fields matching PAYLOAD_FORMAT_FACTORY
    struct FieldDef {
      std::string name;
      char formatCode;
      std::function<double(int32_t)> converter;
    };

    std::vector<FieldDef> fieldParsings = {
      // Main fields
      {"counter", 'I', [](int32_t value) -> double { return static_cast<double>(value); }},
      {"ADC_count_I", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"ADC_count_Q", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"ROTATE_count_I", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"ROTATE_count_Q", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},

      // MCU ADC data struct (2-byte unsigned shorts)
      {"SLED_Neg", 'H', [](int32_t value) -> double { return adcVoltage10BitConv(value); }},
      {"Case_Temp", 'H', [](int32_t value) -> double { return adcCaseTempCConv(value); }},
      {"SLED_Pos", 'H', [](int32_t value) -> double { return adcVoltage10BitConv(value); }},
      {"Bandgap_Volt", 'H', [](int32_t value) -> double { return static_cast<double>(value); }}, // passthrough
      {"GND_Volt", 'H', [](int32_t value) -> double { return static_cast<double>(value); }},     // passthrough

      // Auxiliary ADC struct (4-byte signed ints)
      {"TEC_Current_Sense", 'i', [](int32_t value) -> double { return adcTecCurrentMaConv(value); }},
      {"Heater_Sense", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"Sagnac_Power_Monitor", 'i', [](int32_t value) -> double { return adcSagnacVConv(value); }},
      {"SLED_Power_Sense", 'i', [](int32_t value) -> double { return adcSledPowerUwConv(value); }},
      {"SLED_Temp", 'i', [](int32_t value) -> double { return adcSledTempCConv(value); }},
      {"SLED_Current_Sense", 'i', [](int32_t value) -> double { return adcSledCurrentMaConv(value); }},
      {"Thermistor_Sense", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"Op_Amp_Temp", 'i', [](int32_t value) -> double { return adcOpAmpTempCConv(value); }},
      {"ADC_Temp", 'i', [](int32_t value) -> double { return adcVoltage24BitConv(value); }},
      {"Supply_Voltage", 'i', [](int32_t value) -> double { return adcSupplyVoltageConv(value); }},

      // Status byte
      {"status", 'B', [](int32_t value) -> double { return static_cast<double>(value); }}
    };

    for (const auto& field : fieldParsings) {
      int32_t rawValue = 0;

      // Parse based on format code
      switch (field.formatCode) {
      case 'I': // unsigned int (4 bytes)
      case 'i': // signed int (4 bytes)
        if (offset + 4 <= data.size()) {
          rawValue = readLittleEndian<int32_t>(data, offset);
          offset += 4;
        }
        break;
      case 'H': // unsigned short (2 bytes)
        if (offset + 2 <= data.size()) {
          rawValue = readLittleEndian<uint16_t>(data, offset);
          offset += 2;
        }
        break;
      case 'B': // unsigned byte (1 byte)
        if (offset + 1 <= data.size()) {
          rawValue = data[offset];
          offset += 1;
        }
        break;
      }

      // Store raw value
      parsedMessage.rawValues[field.name] = rawValue;

      // Convert and store converted value
      try {
        double convertedValue = field.converter(rawValue);
        parsedMessage.convertedValues[field.name] = convertedValue;
      }
      catch (const std::exception& ex) {
        std::cout << "Conversion error for " << field.name << ": " << ex.what() << std::endl;
        parsedMessage.convertedValues[field.name] = std::numeric_limits<double>::quiet_NaN();
      }
    }

    // Process additional conversions that depend on other fields
    try {
      auto sagPowerIt = parsedMessage.rawValues.find("Sagnac_Power_Monitor");
      if (sagPowerIt != parsedMessage.rawValues.end()) {
        double sagPowerUw = adcSagnacPowerUwConv(sagPowerIt->second);
        parsedMessage.convertedValues["SAG_PWR (uW)"] = sagPowerUw;
      }
    }
    catch (const std::exception& ex) {
      std::cout << "Additional conversion error for SAG_PWR (uW): " << ex.what() << std::endl;
      parsedMessage.convertedValues["SAG_PWR (uW)"] = std::numeric_limits<double>::quiet_NaN();
    }

    return parsedMessage;
  }

} // namespace SiphogLib