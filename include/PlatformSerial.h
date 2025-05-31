#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace SiphogLib {

    /**
     * @brief Platform-abstracted serial port interface
     * This hides all platform-specific includes from headers
     */
    class PlatformSerial {
    private:
        void* platformHandle; // Opaque pointer to platform-specific data

    public:
        PlatformSerial();
        ~PlatformSerial();

        bool open(const std::string& portName, int baudRate);
        void close();
        bool isOpen() const;
        int write(const std::vector<uint8_t>& data);
        int read(std::vector<uint8_t>& buffer, size_t maxBytes);

        static std::vector<std::string> getAvailablePorts();
    };

} // namespace SiphogLib