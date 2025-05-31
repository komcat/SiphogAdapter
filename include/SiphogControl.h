#pragma once
#include "SerialPort.h"
#include "SiphogCommand.h"
#include "SiphogMessageModel.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace SiphogLib {

  /**
   * @brief Main SiPhOG control class that manages device communication and control
   */
  class SiphogControl {
  private:
    std::shared_ptr<SerialPort> serialPort;
    std::atomic<bool> isConnected{ false };
    SiphogMessageModel lastMessage;

    // Callbacks
    std::function<void(const std::string&, bool)> logCallback;
    std::function<void(const SiphogMessageModel&)> messageCallback;
    std::function<void(bool, const std::string&)> connectionCallback;

    // Settings
    int currentBaudrate = 691200; // Default baudrate for SiPhOG devices
    int currentSledCurrent = 150; // mA
    int currentTemperature = 25;  // °C

    // Internal message handler
    void onMessageReceived(const SiphogMessageModel& message);
    void onLogMessage(const std::string& message, bool isError);

  public:
    /**
     * @brief Constructor
     */
    SiphogControl();

    /**
     * @brief Destructor
     */
    ~SiphogControl();

    /**
     * @brief Get available COM ports
     * @return Vector of available port names
     */
    std::vector<std::string> getAvailablePorts();

    /**
     * @brief Connect to a SiPhOG device on the specified port
     * @param portName COM port name
     * @return true if connection successful, false otherwise
     */
    bool connect(const std::string& portName);

    /**
     * @brief Disconnect from the currently connected SiPhOG device
     */
    void disconnect();

    /**
     * @brief Check if device is connected
     * @return true if connected, false otherwise
     */
    bool getIsConnected() const { return isConnected.load(); }

    /**
     * @brief Apply SLED current and temperature settings to the connected device
     * @param currentMa SLED current in milliamps
     * @param temperatureC Temperature in Celsius
     * @return true if settings applied successfully, false otherwise
     */
    bool applySettings(int currentMa, int temperatureC);

    /**
     * @brief Get the most recent message received from the device
     * @return Reference to the last message
     */
    const SiphogMessageModel& getLastMessage() const { return lastMessage; }

    /**
     * @brief Set callback for log messages
     * @param callback Function to call for log messages (message, isError)
     */
    void setLogCallback(std::function<void(const std::string&, bool)> callback);

    /**
     * @brief Set callback for received messages
     * @param callback Function to call when a message is received
     */
    void setMessageCallback(std::function<void(const SiphogMessageModel&)> callback);

    /**
     * @brief Set callback for connection state changes
     * @param callback Function to call when connection state changes (isConnected, portName)
     */
    void setConnectionCallback(std::function<void(bool, const std::string&)> callback);

    /**
     * @brief Get current baudrate setting
     * @return Baudrate
     */
    int getBaudrate() const { return currentBaudrate; }

    /**
     * @brief Set baudrate (takes effect on next connection)
     * @param baudrate New baudrate value
     */
    void setBaudrate(int baudrate) { currentBaudrate = baudrate; }

    /**
     * @brief Get current SLED current setting
     * @return SLED current in mA
     */
    int getSledCurrent() const { return currentSledCurrent; }

    /**
     * @brief Get current temperature setting
     * @return Temperature in °C
     */
    int getTemperature() const { return currentTemperature; }

    /**
     * @brief Get the connected port name
     * @return Port name string, empty if not connected
     */
    std::string getPortName() const;
  };

} // namespace SiphogLib