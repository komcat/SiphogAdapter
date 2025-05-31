#include "SiphogCommand.h"
#include "SerialPort.h"
#include <stdexcept>
#include <thread>
#include <chrono>

namespace SiphogLib {

  // Factory unlock password
  const std::vector<uint8_t> SiphogCommand::UNLOCK_PASSWORD = { 0x41, 0x41, 0x41, 0x41 }; // "AAAA"

  // Helper method to compute checksum
  std::vector<uint8_t> SiphogCommand::computeChecksum(const std::vector<uint8_t>& data) {
    int checksumA = 0;
    int checksumB = 0;

    for (uint8_t b : data) {
      checksumA = (checksumA + b) % 256;
      checksumB = (checksumB + checksumA) % 256;
    }

    return { static_cast<uint8_t>(checksumA), static_cast<uint8_t>(checksumB) };
  }

  // Helper method to combine byte arrays
  std::vector<uint8_t> SiphogCommand::combineArrays(const std::vector<std::vector<uint8_t>>& arrays) {
    size_t totalLength = 0;
    for (const auto& array : arrays) {
      totalLength += array.size();
    }

    std::vector<uint8_t> result;
    result.reserve(totalLength);

    for (const auto& array : arrays) {
      result.insert(result.end(), array.begin(), array.end());
    }

    return result;
  }

  void SiphogCommand::sendFactoryUnlock(std::shared_ptr<SerialPort> serialPort) {
    if (!serialPort || !serialPort->isOpen()) {
      throw std::runtime_error("Serial port is not open");
    }

    // Create message payload: msgtype + password
    std::vector<uint8_t> payload = { MSGTYPE_SET_FACTORY_UNLOCK };
    payload.insert(payload.end(), UNLOCK_PASSWORD.begin(), UNLOCK_PASSWORD.end());

    // Calculate checksum
    std::vector<uint8_t> checksum = computeChecksum(payload);

    // Combine all parts of the message
    std::vector<uint8_t> fullMessage = combineArrays({
        {PREAMBLE_1, PREAMBLE_2},
        payload,
        checksum
      });

    // Send the message
    serialPort->write(fullMessage);
  }

  void SiphogCommand::setControlMode(std::shared_ptr<SerialPort> serialPort, uint8_t sledMode, uint8_t tecMode) {
    if (!serialPort || !serialPort->isOpen()) {
      throw std::runtime_error("Serial port is not open");
    }

    // Create message payload: msgtype + sledMode + tecMode + padding to 4 bytes
    std::vector<uint8_t> payload = {
        MSGTYPE_SET_CONTROL_MODE,
        sledMode,
        tecMode,
        0x00,
        0x00
    };

    // Calculate checksum
    std::vector<uint8_t> checksum = computeChecksum(payload);

    // Combine all parts of the message
    std::vector<uint8_t> fullMessage = combineArrays({
        {PREAMBLE_1, PREAMBLE_2},
        payload,
        checksum
      });

    // Send the message
    serialPort->write(fullMessage);
  }

  void SiphogCommand::setSledCurrentSetpoint(std::shared_ptr<SerialPort> serialPort, uint16_t currentMa) {
    if (!serialPort || !serialPort->isOpen()) {
      throw std::runtime_error("Serial port is not open");
    }

    // Create message payload: msgtype + current (2 bytes little-endian) + power (2 bytes, set to 0)
    std::vector<uint8_t> payload = {
        MSGTYPE_SET_SLD_SETPOINTS,
        static_cast<uint8_t>(currentMa & 0xFF),        // Low byte
        static_cast<uint8_t>((currentMa >> 8) & 0xFF), // High byte
        0x00,  // Optical power bytes (not used when setting current)
        0x00   // Optical power bytes (not used when setting current)
    };

    // Calculate checksum
    std::vector<uint8_t> checksum = computeChecksum(payload);

    // Combine all parts of the message
    std::vector<uint8_t> fullMessage = combineArrays({
        {PREAMBLE_1, PREAMBLE_2},
        payload,
        checksum
      });

    // Send the message
    serialPort->write(fullMessage);
  }

  void SiphogCommand::setTemperatureSetpoint(std::shared_ptr<SerialPort> serialPort, int16_t temperatureC) {
    if (!serialPort || !serialPort->isOpen()) {
      throw std::runtime_error("Serial port is not open");
    }

    // Create message payload: msgtype + temperature (2 bytes little-endian) + current (2 bytes, set to 0)
    uint16_t tempUnsigned = static_cast<uint16_t>(temperatureC);
    std::vector<uint8_t> payload = {
        MSGTYPE_SET_TEC_SETPOINTS,
        static_cast<uint8_t>(tempUnsigned & 0xFF),        // Low byte
        static_cast<uint8_t>((tempUnsigned >> 8) & 0xFF), // High byte
        0x00,  // Current bytes (not used when setting temperature)
        0x00   // Current bytes (not used when setting temperature)
    };

    // Calculate checksum
    std::vector<uint8_t> checksum = computeChecksum(payload);

    // Combine all parts of the message
    std::vector<uint8_t> fullMessage = combineArrays({
        {PREAMBLE_1, PREAMBLE_2},
        payload,
        checksum
      });

    // Send the message
    serialPort->write(fullMessage);
  }

  void SiphogCommand::initializeDevice(std::shared_ptr<SerialPort> serialPort) {
    sendFactoryUnlock(serialPort);
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give the device time to process

    // Set control mode to SLED constant current and TEC constant temperature
    setControlMode(serialPort, SLED_MODE_CONSTANT_CURRENT, TEC_MODE_CONSTANT_TEMPERATURE);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

} // namespace SiphogLib