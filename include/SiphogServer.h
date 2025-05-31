#pragma once
#include "SiphogMessageModel.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>
#include <memory>

namespace SiphogLib {

    // Forward declaration to avoid including platform headers
    class PlatformSocket;

    // Constants from the Python version
    constexpr double PWR_MON_TRANSFER_FUNC = 0.8; // Example value, adjust as needed
    constexpr double TARGET_LOSS_IN_FRACTION = 0.1; // Example value, adjust as needed
    constexpr double SAGNAC_TIA_GAIN = 1000.0; // Example value, adjust as needed

    /**
     * @brief TCP server for broadcasting SiPhOG data to clients
     * Uses PlatformSocket wrapper to avoid Windows header conflicts
     */
    class SiphogServer {
    private:
        std::unique_ptr<PlatformSocket> platformSocket;
        std::thread serverThread;
        std::thread clientThread;
        std::atomic<bool> serverRunning{ false };
        std::atomic<bool> clientConnected{ false };

        std::string host;
        int port;

        // Data keys matching Python version
        std::vector<std::string> dataKeys = {
            "SLED_Current (mA)",
            "Photo Current (uA)",
            "SLED_Temp (C)",
            "Target SAG_PWR (V)",
            "SAG_PWR (V)",
            "TEC_Current (mA)"
        };

        // Thread synchronization
        std::mutex dataMutex;
        SiphogMessageModel latestMessage;
        bool hasNewData = false;

        // Callbacks
        std::function<void(const std::string&, bool)> logCallback;
        std::function<void(const std::string&)> statusCallback;

        // Server thread functions
        void serverThreadFunction();
        void clientThreadFunction();

        // Data processing
        std::string formatDataMessage(const SiphogMessageModel& message);
        void processAndSendMessage(const SiphogMessageModel& message);

    public:
        /**
         * @brief Constructor
         * @param serverHost Server IP address (default: "127.0.0.1")
         * @param serverPort Server port (default: 65432)
         */
        SiphogServer(const std::string& serverHost = "127.0.0.1", int serverPort = 65432);

        /**
         * @brief Destructor
         */
        ~SiphogServer();

        /**
         * @brief Start the TCP server
         * @return true if server started successfully, false otherwise
         */
        bool startServer();

        /**
         * @brief Stop the TCP server
         */
        void stopServer();

        /**
         * @brief Check if server is running
         * @return true if server is running, false otherwise
         */
        bool isServerRunning() const { return serverRunning.load(); }

        /**
         * @brief Check if a client is connected
         * @return true if client is connected, false otherwise
         */
        bool isClientConnected() const { return clientConnected.load(); }

        /**
         * @brief Update with new SiPhOG message data
         * @param message Latest message from SiPhOG device
         */
        void updateData(const SiphogMessageModel& message);

        /**
         * @brief Set callback for log messages
         * @param callback Function to call for log messages (message, isError)
         */
        void setLogCallback(std::function<void(const std::string&, bool)> callback);

        /**
         * @brief Set callback for status updates
         * @param callback Function to call for status updates
         */
        void setStatusCallback(std::function<void(const std::string&)> callback);

        /**
         * @brief Get server host
         * @return Server host string
         */
        const std::string& getHost() const { return host; }

        /**
         * @brief Get server port
         * @return Server port number
         */
        int getPort() const { return port; }

        /**
         * @brief Get data keys being broadcast
         * @return Vector of data key strings
         */
        const std::vector<std::string>& getDataKeys() const { return dataKeys; }
    };

} // namespace SiphogLib