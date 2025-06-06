#include "SiphogServer.h"
#include "PlatformSocket.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace SiphogLib {

    SiphogServer::SiphogServer(const std::string& serverHost, int serverPort)
        : host(serverHost), port(serverPort) {
        platformSocket = std::make_unique<PlatformSocket>();
        PlatformSocket::initializePlatform();
    }

    SiphogServer::~SiphogServer() {
        stopServer();
        PlatformSocket::cleanupPlatform();
    }

    bool SiphogServer::startServer() {
        if (serverRunning.load()) {
            if (logCallback) {
                logCallback("Server is already running", true);
            }
            return false;
        }

        if (!platformSocket->createServer(host, port)) {
            if (logCallback) {
                logCallback("Failed to create server on " + host + ":" + std::to_string(port), true);
            }
            return false;
        }

        serverRunning.store(true);
        serverThread = std::thread(&SiphogServer::serverThreadFunction, this);

        if (logCallback) {
            logCallback("Starting server on " + host + ":" + std::to_string(port), false);
        }

        return true;
    }

    void SiphogServer::stopServer() {
        if (!serverRunning.load()) {
            return;
        }

        serverRunning.store(false);
        clientConnected.store(false);

        // Close the platform socket to unblock threads
        if (platformSocket) {
            platformSocket->closeServer();
        }

        // Wait for threads to finish
        if (clientThread.joinable()) {
            clientThread.join();
        }
        if (serverThread.joinable()) {
            serverThread.join();
        }

        if (logCallback) {
            logCallback("Server stopped", false);
        }

        if (statusCallback) {
            statusCallback("Server stopped");
        }
    }

    void SiphogServer::serverThreadFunction() {
        if (logCallback) {
            logCallback("Server listening on " + host + ":" + std::to_string(port), false);
        }

        if (statusCallback) {
            statusCallback("Server running, waiting for connection...");
        }

        while (serverRunning.load()) {
            if (platformSocket->acceptClient()) {
                std::string connectionMsg = "Connected with " + platformSocket->getClientInfo();
                if (logCallback) {
                    logCallback(connectionMsg, false);
                }

                if (statusCallback) {
                    statusCallback(connectionMsg);
                }

                clientConnected.store(true);

                // Start client communication thread
                if (clientThread.joinable()) {
                    clientThread.join();
                }
                clientThread = std::thread(&SiphogServer::clientThreadFunction, this);

                // Wait for client to disconnect before accepting next client
                while (serverRunning.load() && clientConnected.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            else {
                // Accept failed, wait a bit before trying again
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void SiphogServer::clientThreadFunction() {
        while (serverRunning.load() && clientConnected.load()) {
            std::unique_lock<std::mutex> lock(dataMutex);

            if (hasNewData) {
                SiphogMessageModel messageCopy = latestMessage;
                hasNewData = false;
                lock.unlock();

                // Process and send the message
                processAndSendMessage(messageCopy);
            }
            else {
                lock.unlock();
                // No new data, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        // Close client connection
        if (platformSocket) {
            platformSocket->closeClient();
        }

        clientConnected.store(false);

        if (logCallback) {
            logCallback("Client disconnected", false);
        }

        if (statusCallback) {
            statusCallback("Client disconnected");
        }
    }

    void SiphogServer::processAndSendMessage(const SiphogMessageModel& message) {
        try {
            std::string dataMessage = formatDataMessage(message);

            int bytesSent = platformSocket->sendData(dataMessage);
            if (bytesSent < 0) {
                if (logCallback) {
                    logCallback("Failed to send data to client", true);
                }
                clientConnected.store(false);
            }
        }
        catch (const std::exception& ex) {
            if (logCallback) {
                logCallback("Error processing message: " + std::string(ex.what()), true);
            }
            clientConnected.store(false);
        }
    }

    std::string SiphogServer::formatDataMessage(const SiphogMessageModel& message) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6);

        // IMPORTANT: Use sagPowerV (which is "SLD_PWR (V)" equivalent) not sldPowerUw
        // This matches the Python code that uses msg.data_dict["SLD_PWR (V)"]

        // Calculate derived values exactly like Python version:
        // msg.data_dict["Photo Current (uA)"] = msg.data_dict["SLD_PWR (V)"] / PWR_MON_TRANSFER_FUNC * 1e6
        double photoCurrentUa = message.sagPowerV / PWR_MON_TRANSFER_FUNC * 1e6;

        // msg.data_dict["Target SAG_PWR (V)"] = TARGET_LOSS_IN_FRACTION * msg.data_dict["SLD_PWR (V)"] / PWR_MON_TRANSFER_FUNC * SAGNAC_TIA_GAIN
        double targetSagPowerV = TARGET_LOSS_IN_FRACTION * message.sagPowerV / PWR_MON_TRANSFER_FUNC * SAGNAC_TIA_GAIN;

        // Format data according to the keys: 
        // "SLED_Current (mA)", "Photo Current (uA)", "SLED_Temp (C)", "Target SAG_PWR (V)", "SAG_PWR (V)", "TEC_Current (mA)"
        oss << message.sledCurrent << ",";      // SLED_Current (mA)
        oss << photoCurrentUa << ",";           // Photo Current (uA)  
        oss << message.sledTemp << ",";         // SLED_Temp (C)
        oss << targetSagPowerV << ",";          // Target SAG_PWR (V)
        oss << message.sagPowerV << ",";        // SAG_PWR (V)
        oss << message.tecCurrent;              // TEC_Current (mA) - no comma at end

        return oss.str();
    }

    void SiphogServer::updateData(const SiphogMessageModel& message) {
        std::lock_guard<std::mutex> lock(dataMutex);
        latestMessage = message;
        hasNewData = true;
    }

    void SiphogServer::setLogCallback(std::function<void(const std::string&, bool)> callback) {
        logCallback = callback;
    }

    void SiphogServer::setStatusCallback(std::function<void(const std::string&)> callback) {
        statusCallback = callback;
    }

} // namespace SiphogLib