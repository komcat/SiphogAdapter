#pragma once
#include <vector>
#include <cstdint>
#include <memory>

namespace SiphogLib {

  // Forward declaration for serial port interface
  class SerialPort;

  /**
   * @brief SiPhOG command interface for device control
   */
  class SiphogCommand {
  private:
    // Constants from message_formats.py and Input_Message.py
    static constexpr uint8_t PREAMBLE_1 = 0xC5;
    static constexpr uint8_t PREAMBLE_2 = 0x50;
    static constexpr uint8_t MSGTYPE_SET_FACTORY_UNLOCK = 0xEF;
    static constexpr uint8_t MSGTYPE_SET_CONTROL_MODE = 0xE8;
    static constexpr uint8_t MSGTYPE_SET_SLD_SETPOINTS = 0x5E;
    static constexpr uint8_t MSGTYPE_SET_TEC_SETPOINTS = 0x60;

    // Control modes
    static constexpr uint8_t SLED_MODE_CONSTANT_CURRENT = 0x01;
    static constexpr uint8_t TEC_MODE_CONSTANT_TEMPERATURE = 0x03;

    // Factory unlock password
    static const std::vector<uint8_t> UNLOCK_PASSWORD;

    // Helper method to compute checksum
    static std::vector<uint8_t> computeChecksum(const std::vector<uint8_t>& data);

    // Helper method to combine byte arrays
    static std::vector<uint8_t> combineArrays(const std::vector<std::vector<uint8_t>>& arrays);

  public:
    /**
     * @brief Sends a factory unlock command to enable configuration
     * @param serialPort Serial port interface
     */
    static void sendFactoryUnlock(std::shared_ptr<SerialPort> serialPort);

    /**
     * @brief Sets the control mode for SLED and TEC
     * @param serialPort Serial port interface
     * @param sledMode SLED control mode
     * @param tecMode TEC control mode
     */
    static void setControlMode(std::shared_ptr<SerialPort> serialPort, uint8_t sledMode, uint8_t tecMode);

    /**
     * @brief Sets the SLED current setpoint
     * @param serialPort Serial port interface
     * @param currentMa Current in milliamps
     */
    static void setSledCurrentSetpoint(std::shared_ptr<SerialPort> serialPort, uint16_t currentMa);

    /**
     * @brief Sets the temperature setpoint for TEC
     * @param serialPort Serial port interface
     * @param temperatureC Temperature in Celsius
     */
    static void setTemperatureSetpoint(std::shared_ptr<SerialPort> serialPort, int16_t temperatureC);

    /**
     * @brief Initialize the SiPhOG device for control operations
     * @param serialPort Serial port interface
     */
    static void initializeDevice(std::shared_ptr<SerialPort> serialPort);
  };

} // namespace SiphogLib