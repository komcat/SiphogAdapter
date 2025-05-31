#include "SiphogControl.h"
#include <stdexcept>
#include <thread>
#include <chrono>

namespace SiphogLib {

  SiphogControl::SiphogControl() {
  }

  SiphogControl::~SiphogControl() {
    disconnect();
  }

  std::vector<std::string> SiphogControl::getAvailablePorts() {
    return SerialPort::getAvailablePorts();
  }

  bool SiphogControl::connect(const std::string& portName) {
    if (isConnected.load()) {
      onLogMessage("Already connected to a device", true);
      return false;
    }

    try {
      // Create new serial port instance
      serialPort = std::make_shared<SerialPort>(portName, currentBaudrate);

      // Set up callbacks
      serialPort->setLogCallback([this](const std::string& msg, bool isError) {
        onLogMessage(msg, isError);
      });

      serialPort->setMessageCallback([this](const SiphogMessageModel& msg) {
        onMessageReceived(msg);
      });

      // Attempt to open the connection
      if (!serialPort->open()) {
        serialPort.reset();
        onLogMessage("Failed to open serial port: " + portName, true);
        if (connectionCallback) {
          connectionCallback(false, portName);
        }
        return false;
      }

      // Initialize device with control modes
      try {
        SiphogCommand::initializeDevice(serialPort);
        onLogMessage("Device initialized with factory unlock and control modes set", false);

        // Set initial values when connecting
        SiphogCommand::setSledCurrentSetpoint(serialPort, static_cast<uint16_t>(currentSledCurrent));
        SiphogCommand::setTemperatureSetpoint(serialPort, static_cast<int16_t>(currentTemperature));

        onLogMessage("Initial settings applied: SLED Current = " + std::to_string(currentSledCurrent) +
          "mA, Temperature = " + std::to_string(currentTemperature) + "°C", false);
      }
      catch (const std::exception& ex) {
        onLogMessage("Device initialization error: " + std::string(ex.what()), true);
      }

      isConnected.store(true);
      onLogMessage("Connected to " + portName + " at " + std::to_string(currentBaudrate) + " baud", false);

      // Notify connection state change
      if (connectionCallback) {
        connectionCallback(true, portName);
      }

      return true;
    }
    catch (const std::exception& ex) {
      onLogMessage("Connection error: " + std::string(ex.what()), true);

      // Clean up on error
      if (serialPort) {
        serialPort.reset();
      }

      // Notify connection failure
      if (connectionCallback) {
        connectionCallback(false, portName);
      }

      return false;
    }
  }

  void SiphogControl::disconnect() {
    if (!isConnected.load()) {
      return;
    }

    std::string portName = getPortName();

    if (serialPort) {
      serialPort->close();
      serialPort.reset();
    }

    isConnected.store(false);
    onLogMessage("Disconnected from serial port", false);

    // Notify connection state change
    if (connectionCallback) {
      connectionCallback(false, portName);
    }
  }

  bool SiphogControl::applySettings(int currentMa, int temperatureC) {
    if (!isConnected.load() || !serialPort) {
      onLogMessage("Cannot apply settings: Serial port is not open", true);
      return false;
    }

    try {
      // Ensure values are within reasonable limits
      if (currentMa < 0 || currentMa > 500) {
        onLogMessage("Invalid SLED current value: " + std::to_string(currentMa) +
          "mA. Must be between 0 and 500mA.", true);
        return false;
      }

      if (temperatureC < 0 || temperatureC > 50) {
        onLogMessage("Invalid temperature value: " + std::to_string(temperatureC) +
          "°C. Must be between 0 and 50°C.", true);
        return false;
      }

      // Send commands to device
      SiphogCommand::setSledCurrentSetpoint(serialPort, static_cast<uint16_t>(currentMa));
      SiphogCommand::setTemperatureSetpoint(serialPort, static_cast<int16_t>(temperatureC));

      // Update internal settings
      currentSledCurrent = currentMa;
      currentTemperature = temperatureC;

      onLogMessage("Settings applied: SLED Current = " + std::to_string(currentMa) +
        "mA, Temperature = " + std::to_string(temperatureC) + "°C", false);
      return true;
    }
    catch (const std::exception& ex) {
      onLogMessage("Error applying settings: " + std::string(ex.what()), true);
      return false;
    }
  }

  void SiphogControl::onMessageReceived(const SiphogMessageModel& message) {
    lastMessage = message;

    // Forward to external callback if set
    if (messageCallback) {
      messageCallback(message);
    }
  }

  void SiphogControl::onLogMessage(const std::string& message, bool isError) {
    // Forward to external callback if set
    if (logCallback) {
      logCallback(message, isError);
    }
  }

  void SiphogControl::setLogCallback(std::function<void(const std::string&, bool)> callback) {
    logCallback = callback;
  }

  void SiphogControl::setMessageCallback(std::function<void(const SiphogMessageModel&)> callback) {
    messageCallback = callback;
  }

  void SiphogControl::setConnectionCallback(std::function<void(bool, const std::string&)> callback) {
    connectionCallback = callback;
  }

  std::string SiphogControl::getPortName() const {
    if (serialPort) {
      return serialPort->getPortName();
    }
    return "";
  }

} // namespace SiphogLib