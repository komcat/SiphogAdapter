#pragma once
#include <string>
#include <cstdint>

namespace SiphogLib {

    /**
     * @brief Platform-abstracted socket interface
     * This hides all platform-specific includes from headers
     */
    class PlatformSocket {
    private:
        void* platformHandle; // Opaque pointer to platform-specific data

    public:
        PlatformSocket();
        ~PlatformSocket();

        bool createServer(const std::string& host, int port);
        bool acceptClient();
        void closeClient();
        void closeServer();
        bool isServerOpen() const;
        bool isClientConnected() const;
        int sendData(const std::string& data);
        std::string getClientInfo() const;

        static bool initializePlatform();
        static void cleanupPlatform();
    };

} // namespace SiphogLib