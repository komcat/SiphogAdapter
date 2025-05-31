#include "SerialPort.h"
#include "SiphogMessageModel.h"
#include "ParseMessage.h"
#include <iostream>
#include <chrono>

namespace SiphogLib {

    SerialPort::SerialPort(const std::string& port, int baud)
        : portName(port), baudRate(baud) {
        platformSerial = std::make_unique<PlatformSerial>();
    }

    SerialPort::~SerialPort() {
        close();
    }

    bool SerialPort::open() {
        if (isConnected.load()) {
            return true; // Already connected
        }

        if (!platformSerial->open(portName, baudRate)) {
            if (logCallback) {
                logCallback("Failed to open serial port: " + portName, true);
            }
            return false;
        }

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

        if (platformSerial) {
            platformSerial->close();
        }

        if (logCallback) {
            logCallback("Disconnected from " + portName, false);
        }
    }

    int SerialPort::write(const std::vector<uint8_t>& data) {
        if (!isConnected.load() || !platformSerial) {
            return -1;
        }

        return platformSerial->write(data);
    }

    void SerialPort::readThreadFunction() {
        std::vector<uint8_t> dataBuffer;
        dataBuffer.reserve(1024);

        while (continueReading.load()) {
            std::vector<uint8_t> tempBuffer;
            int bytesRead = platformSerial->read(tempBuffer, 256);

            if (bytesRead > 0) {
                // Add new data to buffer
                dataBuffer.insert(dataBuffer.end(), tempBuffer.begin(), tempBuffer.end());

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
        return PlatformSerial::getAvailablePorts();
    }

} // namespace SiphogLib