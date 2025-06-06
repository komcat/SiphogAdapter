#pragma once
#include <cstdint>

namespace SiphogLib {

    /**
     * @brief Represents a parsed SiPhOG message for display in the UI
     */
    class SiphogMessageModel {
    public:
        // Basic message info
        uint32_t counter = 0;
        double timeSeconds = 0.0;
        uint8_t status = 0;

        // ADC values
        double adcCountI = 0.0;
        double adcCountQ = 0.0;
        double rotateCountI = 0.0;
        double rotateCountQ = 0.0;

        // MCU ADC values
        double sledNeg = 0.0;
        double caseTemp = 0.0;
        double sledPos = 0.0;
        double bandgapVolt = 0.0;
        double gndVolt = 0.0;

        // Auxiliary ADC values
        double tecCurrent = 0.0;
        double heaterSense = 0.0;
        double sagPowerV = 0.0;
        double sagPowerUw = 0.0;
        double sldPowerV = 0.0;    // <-- ADD THIS: Raw SLED power voltage
        double sldPowerUw = 0.0;
        double sledTemp = 0.0;
        double sledCurrent = 0.0;
        double thermistorSense = 0.0;
        double opAmpTemp = 0.0;
        double adcTemp = 0.0;
        double supplyVoltage = 0.0;

        // Derived values (calculated once)
        double photoCurrentUa = 0.0;
        double targetSagPowerV = 0.0;

        // Constructor
        SiphogMessageModel() = default;

        // Helper method to create model from parsed message
        static SiphogMessageModel fromParsedMessage(const class ParsedMessage& parsedMessage);

        // Calculate derived values using the same constants as server
        void calculateDerivedValues();
    };

} // namespace SiphogLib