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

    // Forward declaration to avoid including SiphogServer.h in header
    class SiphogServer;

    /**
     * @brief Main SiPhOG control class that manages device communication and server
     * Updated to use platform wrappers - no Windows header conflicts!
     */
    class SiphogControl {
    private:
        std::shared_ptr<SerialPort> serialPort;
        std::unique_ptr<SiphogServer> server;
        std::atomic<bool> isConnected{ false };
        SiphogMessageModel lastMessage;

        // Callbacks
        std::function<void(const std::string&, bool)> logCallback;
        std::function<void(const SiphogMessageModel&)> messageCallback;
        std::function<void(bool, const std::string&)> connectionCallback;
        std::function<void(const std::string&)> serverStatusCallback;

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
         * @param currentMa SLED current in milliamps (0-500mA)
         * @param temperatureC Temperature in Celsius (0-50°C)
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
         * @brief Set callback for server status updates
         * @param callback Function to call when server status changes
         */
        void setServerStatusCallback(std::function<void(const std::string&)> callback);

        /**
         * @brief Start the TCP server for broadcasting data
         * @param host Server IP address (default: "127.0.0.1")
         * @param port Server port (default: 65432)
         * @return true if server started successfully, false otherwise
         */
        bool startServer(const std::string& host = "127.0.0.1", int port = 65432);

        /**
         * @brief Stop the TCP server
         */
        void stopServer();

        /**
         * @brief Check if server is running
         * @return true if server is running, false otherwise
         */
        bool isServerRunning() const;

        /**
         * @brief Check if a client is connected to the server
         * @return true if client is connected, false otherwise
         */
        bool isClientConnected() const;

        /**
         * @brief Get server information
         * @return String with server host:port, or empty if not running
         */
        std::string getServerInfo() const;

        /**
         * @brief Get current baudrate setting
         * @return Baudrate
         */
        int getBaudrate() const { return currentBaudrate; }

        /**
         * @brief Set baudrate (takes effect on next connection)
         * @param baudrate New baudrate value (typically 691200 for SiPhOG)
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

        /**
         * @brief Get data keys that are broadcast by the server
         * @return Vector of data key strings
         */
        std::vector<std::string> getServerDataKeys() const;

        /**
         * @brief Check if the device is properly initialized
         * @return true if device is connected and initialized
         */
        bool isDeviceReady() const { return isConnected.load(); }

        /**
         * @brief Get connection statistics
         * @return String with connection info
         */
        std::string getConnectionInfo() const;
    };

} // namespace SiphogLib