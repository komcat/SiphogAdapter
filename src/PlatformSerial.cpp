#include "PlatformSerial.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
// Windows-specific includes isolated here
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#pragma comment(lib, "setupapi.lib")

// Windows platform data structure
struct Win32SerialData {
    HANDLE hSerial = INVALID_HANDLE_VALUE;
    std::string portName;
    int baudRate = 0;
    bool isOpen = false;
};

#else
// Linux includes
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstring>

// Linux platform data structure
struct LinuxSerialData {
    int fd = -1;
    std::string portName;
    int baudRate = 0;
    bool isOpen = false;
};
#endif

namespace SiphogLib {

    PlatformSerial::PlatformSerial() {
#ifdef _WIN32
        platformHandle = new Win32SerialData();
#else
        platformHandle = new LinuxSerialData();
#endif
    }

    PlatformSerial::~PlatformSerial() {
        close();
#ifdef _WIN32
        delete static_cast<Win32SerialData*>(platformHandle);
#else
        delete static_cast<LinuxSerialData*>(platformHandle);
#endif
    }

    bool PlatformSerial::open(const std::string& portName, int baudRate) {
        if (isOpen()) {
            return true; // Already open
        }

#ifdef _WIN32
        Win32SerialData* data = static_cast<Win32SerialData*>(platformHandle);
        data->portName = portName;
        data->baudRate = baudRate;

        // Windows implementation
        std::string fullPortName = "\\\\.\\" + portName;
        data->hSerial = CreateFileA(
            fullPortName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (data->hSerial == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to open serial port: " << portName << std::endl;
            return false;
        }

        // Configure the serial port
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(data->hSerial, &dcbSerialParams)) {
            CloseHandle(data->hSerial);
            data->hSerial = INVALID_HANDLE_VALUE;
            std::cerr << "Failed to get current serial parameters" << std::endl;
            return false;
        }

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

        if (!SetCommState(data->hSerial, &dcbSerialParams)) {
            CloseHandle(data->hSerial);
            data->hSerial = INVALID_HANDLE_VALUE;
            std::cerr << "Failed to set serial parameters" << std::endl;
            return false;
        }

        // Set timeouts
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(data->hSerial, &timeouts)) {
            CloseHandle(data->hSerial);
            data->hSerial = INVALID_HANDLE_VALUE;
            std::cerr << "Failed to set serial timeouts" << std::endl;
            return false;
        }

        data->isOpen = true;

#else
        LinuxSerialData* data = static_cast<LinuxSerialData*>(platformHandle);
        data->portName = portName;
        data->baudRate = baudRate;

        // Linux implementation
        data->fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (data->fd < 0) {
            std::cerr << "Failed to open serial port: " << portName << std::endl;
            return false;
        }

        struct termios tty;
        if (tcgetattr(data->fd, &tty) != 0) {
            ::close(data->fd);
            data->fd = -1;
            std::cerr << "Failed to get serial attributes" << std::endl;
            return false;
        }

        // Configure serial port
        cfsetospeed(&tty, B921600); // Closest standard baud rate to 691200
        cfsetispeed(&tty, B921600);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo, no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN] = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls, enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr(data->fd, TCSANOW, &tty) != 0) {
            ::close(data->fd);
            data->fd = -1;
            std::cerr << "Failed to set serial attributes" << std::endl;
            return false;
        }

        data->isOpen = true;
#endif

        return true;
    }

    void PlatformSerial::close() {
        if (!isOpen()) {
            return;
        }

#ifdef _WIN32
        Win32SerialData* data = static_cast<Win32SerialData*>(platformHandle);
        if (data->hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(data->hSerial);
            data->hSerial = INVALID_HANDLE_VALUE;
        }
        data->isOpen = false;
#else
        LinuxSerialData* data = static_cast<LinuxSerialData*>(platformHandle);
        if (data->fd >= 0) {
            ::close(data->fd);
            data->fd = -1;
        }
        data->isOpen = false;
#endif
    }

    bool PlatformSerial::isOpen() const {
#ifdef _WIN32
        Win32SerialData* data = static_cast<Win32SerialData*>(platformHandle);
        return data->isOpen;
#else
        LinuxSerialData* data = static_cast<LinuxSerialData*>(platformHandle);
        return data->isOpen;
#endif
    }

    int PlatformSerial::write(const std::vector<uint8_t>& data) {
        if (!isOpen()) {
            return -1;
        }

#ifdef _WIN32
        Win32SerialData* serialData = static_cast<Win32SerialData*>(platformHandle);
        DWORD bytesWritten;
        if (!WriteFile(serialData->hSerial, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL)) {
            return -1;
        }
        return static_cast<int>(bytesWritten);
#else
        LinuxSerialData* serialData = static_cast<LinuxSerialData*>(platformHandle);
        return ::write(serialData->fd, data.data(), data.size());
#endif
    }

    int PlatformSerial::read(std::vector<uint8_t>& buffer, size_t maxBytes) {
        if (!isOpen()) {
            return -1;
        }

        buffer.resize(maxBytes);

#ifdef _WIN32
        Win32SerialData* serialData = static_cast<Win32SerialData*>(platformHandle);
        DWORD bytesRead;
        if (!ReadFile(serialData->hSerial, buffer.data(), static_cast<DWORD>(maxBytes), &bytesRead, NULL)) {
            return -1;
        }
        buffer.resize(bytesRead);
        return static_cast<int>(bytesRead);
#else
        LinuxSerialData* serialData = static_cast<LinuxSerialData*>(platformHandle);
        int bytesRead = ::read(serialData->fd, buffer.data(), maxBytes);
        if (bytesRead < 0) {
            return -1;
        }
        buffer.resize(bytesRead);
        return bytesRead;
#endif
    }

    std::vector<std::string> PlatformSerial::getAvailablePorts() {
        std::vector<std::string> ports;

#ifdef _WIN32
        // Windows implementation using SetupAPI
        HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
        if (hDevInfo == INVALID_HANDLE_VALUE) {
            return ports;
        }

        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
            HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (hKey != INVALID_HANDLE_VALUE) {
                char portName[256];
                DWORD size = sizeof(portName);
                DWORD type = 0;

                if (RegQueryValueExA(hKey, "PortName", NULL, &type, (LPBYTE)portName, &size) == ERROR_SUCCESS) {
                    if (strstr(portName, "COM") != NULL) {
                        ports.push_back(std::string(portName));
                    }
                }
                RegCloseKey(hKey);
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

        // Sort the ports
        std::sort(ports.begin(), ports.end());

#else
        // Linux implementation - look for common serial device patterns
        std::vector<std::string> patterns = { "/dev/ttyUSB", "/dev/ttyACM", "/dev/ttyS" };

        for (const auto& pattern : patterns) {
            for (int i = 0; i < 10; ++i) {
                std::string portPath = pattern + std::to_string(i);
                if (access(portPath.c_str(), F_OK) == 0) {
                    ports.push_back(portPath);
                }
            }
        }
#endif

        return ports;
    }

} // namespace SiphogLib