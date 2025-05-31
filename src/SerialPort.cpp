#include "SerialPort.h"
#include "SiphogMessageModel.h"
#include "ParseMessage.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#pragma comment(lib, "setupapi.lib")
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <cstring>
#endif

namespace SiphogLib {

  SerialPort::SerialPort(const std::string& port, int baud)
    : portName(port), baudRate(baud) {
  }

  SerialPort::~SerialPort() {
    close();
  }

  bool SerialPort::open() {
    if (isConnected.load()) {
      return true; // Already connected
    }

#ifdef _WIN32
    // Windows implementation
    std::string fullPortName = "\\\\.\\" + portName;
    hSerial = CreateFileA(
      fullPortName.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
      if (logCallback) {
        logCallback("Failed to open serial port: " + portName, true);
      }
      return false;
    }

    // Configure the serial port
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
      CloseHandle(hSerial);
      hSerial = INVALID_HANDLE_VALUE;
      if (logCallback) {
        logCallback("Failed to get current serial parameters", true);
      }
      return false;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
      CloseHandle(hSerial);
      hSerial = INVALID_HANDLE_VALUE;
      if (logCallback) {
        logCallback("Failed to set serial parameters", true);
      }
      return false;
    }

    // Set timeouts
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
      CloseHandle(hSerial);
      hSerial = INVALID_HANDLE_VALUE;
      if (logCallback) {
        logCallback("Failed to set serial timeouts", true);
      }
      return false;
    }

#else
    // Linux implementation
    fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
      if (logCallback) {
        logCallback("Failed to open serial port: " + portName, true);
      }
      return false;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
      ::close(fd);
      fd = -1;
      if (logCallback) {
        logCallback("Failed to get serial attributes", true);
      }
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

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      ::close(fd);
      fd = -1;
      if (logCallback) {
        logCallback("Failed to set serial attributes", true);
      }
      return false;
    }
#endif

    isConnected.store(true);
    continueReading.store(true);

    // Start the read thread
    readThread = std::thread(&SerialPort::readThreadFunction, this);

    if (logCallback) {
      logCallback("Connected to " + portName + " at " + std::to_string(baudRate) + " baud", false);
    }

    return true;
  }

  void SerialPort::close() {
    if (!isConnected.load()) {
      return; // Already disconnected
    }

    continueReading.store(false);
    isConnected.store(false);

    // Wait for read thread to finish
    if (readThread.joinable()) {
      readThread.join();
    }

#ifdef _WIN32
    if (hSerial != INVALID_HANDLE_VALUE) {
      CloseHandle(hSerial);
      hSerial = INVALID_HANDLE_VALUE;
    }
#else
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
#endif

    if (logCallback) {
      logCallback("Disconnected from " + portName, false);
    }
  }

  int SerialPort::write(const std::vector<uint8_t>& data) {
    if (!isConnected.load()) {
      return -1;
    }

#ifdef _WIN32
    DWORD bytesWritten;
    if (!WriteFile(hSerial, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL)) {
      return -1;
    }
    return static_cast<int>(bytesWritten);
#else
    return ::write(fd, data.data(), data.size());
#endif
  }

  void SerialPort::readThreadFunction() {
    std::vector<uint8_t> dataBuffer;
    dataBuffer.reserve(1024);

    while (continueReading.load()) {
      std::vector<uint8_t> tempBuffer(256);
      int bytesRead = 0;

#ifdef _WIN32
      DWORD dwBytesRead;
      if (ReadFile(hSerial, tempBuffer.data(), static_cast<DWORD>(tempBuffer.size()), &dwBytesRead, NULL)) {
        bytesRead = static_cast<int>(dwBytesRead);
      }
#else
      bytesRead = ::read(fd, tempBuffer.data(), tempBuffer.size());
      if (bytesRead < 0) {
        bytesRead = 0; // Handle error by treating as no data
      }
#endif

      if (bytesRead > 0) {
        // Add new data to buffer
        dataBuffer.insert(dataBuffer.end(), tempBuffer.begin(), tempBuffer.begin() + bytesRead);

        // Process the buffer for complete messages
        processIncomingData(dataBuffer);

        // Prevent buffer overflow by limiting its size
        if (dataBuffer.size() > 2000) {
          dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + 1000); // Remove oldest half
        }
      }
      else {
        // No data available, short sleep to prevent CPU thrashing
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }
  }

  void SerialPort::processIncomingData(std::vector<uint8_t>& dataBuffer) {
    // Look for start sequence 0xF2, 0x47
    for (size_t i = 0; i < dataBuffer.size() - 1; ++i) {
      if (dataBuffer[i] == 0xF2 && dataBuffer[i + 1] == 0x47) {
        // Found start sequence, check if we have enough data for a complete message (76 bytes)
        if (i + 76 <= dataBuffer.size()) {
          // Extract the complete message
          std::vector<uint8_t> messageData(dataBuffer.begin() + i, dataBuffer.begin() + i + 76);

          try {
            // Parse the message
            ParsedMessage parsedMessage = ParseMessage::parseFactoryMessage(messageData);
            SiphogMessageModel messageModel = SiphogMessageModel::fromParsedMessage(parsedMessage);

            // Call the callback if set
            if (messageCallback) {
              messageCallback(messageModel);
            }
          }
          catch (const std::exception& ex) {
            if (logCallback) {
              logCallback("Error parsing message: " + std::string(ex.what()), true);
            }
          }

          // Remove processed data from buffer (including the message we just processed)
          dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + i + 76);
          return; // Start over with the remaining data
        }
        else {
          // Not enough data yet, wait for more
          break;
        }
      }
    }
  }

  void SerialPort::setMessageCallback(std::function<void(const SiphogMessageModel&)> callback) {
    messageCallback = callback;
  }

  void SerialPort::setLogCallback(std::function<void(const std::string&, bool)> callback) {
    logCallback = callback;
  }

  std::vector<std::string> SerialPort::getAvailablePorts() {
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