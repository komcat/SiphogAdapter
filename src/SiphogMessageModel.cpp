#include "SiphogMessageModel.h"
#include "ParseMessage.h"

namespace SiphogLib {

  SiphogMessageModel SiphogMessageModel::fromParsedMessage(const ParsedMessage& parsedMessage) {
    SiphogMessageModel model;

    // Extract basic values
    auto counterIt = parsedMessage.rawValues.find("counter");
    if (counterIt != parsedMessage.rawValues.end()) {
      model.counter = static_cast<uint32_t>(counterIt->second);
    }

    auto statusIt = parsedMessage.rawValues.find("status");
    if (statusIt != parsedMessage.rawValues.end()) {
      model.status = static_cast<uint8_t>(statusIt->second);
    }

    // Calculate time from counter (assuming 200Hz ODR)
    model.timeSeconds = model.counter / 200.0;

    // Helper lambda to safely extract converted values
    auto getConvertedValue = [&](const std::string& key) -> double {
      auto it = parsedMessage.convertedValues.find(key);
      return (it != parsedMessage.convertedValues.end()) ? it->second : 0.0;
    };

    // ADC values
    model.adcCountI = getConvertedValue("ADC_count_I");
    model.adcCountQ = getConvertedValue("ADC_count_Q");
    model.rotateCountI = getConvertedValue("ROTATE_count_I");
    model.rotateCountQ = getConvertedValue("ROTATE_count_Q");

    // MCU ADC values
    model.sledNeg = getConvertedValue("SLED_Neg");
    model.caseTemp = getConvertedValue("Case_Temp");
    model.sledPos = getConvertedValue("SLED_Pos");
    model.bandgapVolt = getConvertedValue("Bandgap_Volt");
    model.gndVolt = getConvertedValue("GND_Volt");

    // Auxiliary ADC values
    model.tecCurrent = getConvertedValue("TEC_Current_Sense");
    model.heaterSense = getConvertedValue("Heater_Sense");
    model.sagPowerV = getConvertedValue("Sagnac_Power_Monitor");
    model.sagPowerUw = getConvertedValue("SAG_PWR (uW)");
    model.sldPowerUw = getConvertedValue("SLED_Power_Sense");
    model.sledTemp = getConvertedValue("SLED_Temp");
    model.sledCurrent = getConvertedValue("SLED_Current_Sense");
    model.thermistorSense = getConvertedValue("Thermistor_Sense");
    model.opAmpTemp = getConvertedValue("Op_Amp_Temp");
    model.adcTemp = getConvertedValue("ADC_Temp");
    model.supplyVoltage = getConvertedValue("Supply_Voltage");

    return model;
  }

} // namespace SiphogLib