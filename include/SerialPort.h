#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

namespace SiphogLib {

  // Forward declaration
  class SiphogMessageModel;

  /**
   * @brief Cross-platform serial port interface for SiPhOG communication
   */
  class SerialPort {
  private:
#ifdef _WIN32
    HANDLE hSerial = INVALID_HANDLE_VALUE;
#else
    int fd = -1;
#endif

    std::string portName;
    int baudRate;
    std::atomic<bool> isConnected{ false };
    std::atomic<bool> continueReading{ false };
    std::thread readThread;

    // Callback for received messages
    std::function<void(const SiphogMessageModel&)> messageCallback;
    std::function<void(const std::string&, bool)> logCallback;

    // Read thread function
    void readThreadFunction();

    // Parse incoming data for SiPhOG messages
    void processIncomingData(std::vector<uint8_t>& data);

  public:
    /**
     * @brief Constructor
     * @param port Serial port name (e.g., "COM3" on Windows, "/dev/ttyUSB0" on Linux)
     * @param baud Baud rate (default 691200 for SiPhOG)
     */
    SerialPort(const std::string& port, int baud = 691200);

    /**
     * @brief Destructor - automatically closes connection
     */
    ~SerialPort();

    /**
     * @brief Open the serial port connection
     * @return true if successful, false otherwise
     */
    bool open();

    /**
     * @brief Close the serial port connection
     */
    void close();

    /**
     * @brief Check if the port is open and connected
     * @return true if connected, false otherwise
     */
    bool isOpen() const { return isConnected.load(); }

    /**
     * @brief Write data to the serial port
     * @param data Data to write
     * @return Number of bytes written, -1 on error
     */
    int write(const std::vector<uint8_t>& data);

    /**
     * @brief Set callback for received SiPhOG messages
     * @param callback Function to call when a complete message is received
     */
    void setMessageCallback(std::function<void(const SiphogMessageModel&)> callback);

    /**
     * @brief Set callback for log messages
     * @param callback Function to call for log messages (message, isError)
     */
    void setLogCallback(std::function<void(const std::string&, bool)> callback);

    /**
     * @brief Get available serial ports on the system
     * @return Vector of port names
     */
    static std::vector<std::string> getAvailablePorts();

    /**
     * @brief Get the port name
     * @return Port name string
     */
    const std::string& getPortName() const { return portName; }

    /**
     * @brief Get the baud rate
     * @return Baud rate
     */
    int getBaudRate() const { return baudRate; }
  };

} // namespace SiphogLib