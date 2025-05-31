#include "raylib.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#pragma region imgui
#include "imgui.h"
#include "rlImGui.h"
#include "imguiThemes.h"
#pragma endregion

// Include SiPhOG library - now with platform wrappers (no conflicts!)
#include "SiphogControl.h"

// Global SiPhOG control instance
SiphogLib::SiphogControl siphogControl;

// UI state variables
static std::vector<std::string> availablePorts;
static int selectedPortIndex = 0;
static bool isConnected = false;
static std::string connectionStatus = "Not Connected";
static std::vector<std::string> logMessages;
static int sledCurrentMa = 150;
static int temperatureC = 25;
static SiphogLib::SiphogMessageModel lastMessage;

// Server state variables
static bool serverRunning = false;
static bool clientConnected = false;
static std::string serverStatus = "Server stopped";
static std::string serverHost = "127.0.0.1";
static int serverPort = 65432;

// Settings options
static const int sledCurrentOptions[] = { 50, 100, 110, 120, 130, 140, 150, 200 };
static const int temperatureOptions[] = { 20, 25, 30, 35 };
static const int numSledOptions = sizeof(sledCurrentOptions) / sizeof(sledCurrentOptions[0]);
static const int numTempOptions = sizeof(temperatureOptions) / sizeof(temperatureOptions[0]);

void addLogMessage(const std::string& message, bool isError = false) {
    std::string timestamp = "[" + std::to_string(GetTime()) + "] ";
    std::string fullMessage = timestamp + message;
    logMessages.push_back(fullMessage);

    // Keep only last 100 messages to prevent memory issues
    if (logMessages.size() > 100) {
        logMessages.erase(logMessages.begin());
    }

    // Print to console as well
    if (isError) {
        std::cerr << fullMessage << std::endl;
    }
    else {
        std::cout << fullMessage << std::endl;
    }
}

void refreshPorts() {
    availablePorts = siphogControl.getAvailablePorts();
    if (selectedPortIndex >= static_cast<int>(availablePorts.size())) {
        selectedPortIndex = 0;
    }
    addLogMessage("COM ports refreshed. Found " + std::to_string(availablePorts.size()) + " ports.");
}

void setupSiphogCallbacks() {
    // Set up log callback
    siphogControl.setLogCallback([](const std::string& message, bool isError) {
        addLogMessage(message, isError);
        });

    // Set up message callback
    siphogControl.setMessageCallback([](const SiphogLib::SiphogMessageModel& message) {
        lastMessage = message;
        });

    // Set up connection callback
    siphogControl.setConnectionCallback([](bool connected, const std::string& portName) {
        isConnected = connected;
        if (connected) {
            connectionStatus = "Connected to " + portName;
        }
        else {
            connectionStatus = "Not Connected";
        }
        });

    // Set up server status callback
    siphogControl.setServerStatusCallback([](const std::string& status) {
        serverStatus = status;
        serverRunning = siphogControl.isServerRunning();
        clientConnected = siphogControl.isClientConnected();
        });
}

void drawConnectionPanel() {
    ImGui::Begin("SiPhOG Connection");

    // Port selection
    ImGui::Text("COM Port:");
    ImGui::SameLine();

    if (ImGui::BeginCombo("##port", availablePorts.empty() ? "No ports available" : availablePorts[selectedPortIndex].c_str())) {
        for (int i = 0; i < static_cast<int>(availablePorts.size()); i++) {
            bool isSelected = (selectedPortIndex == i);
            if (ImGui::Selectable(availablePorts[i].c_str(), isSelected)) {
                selectedPortIndex = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh Ports")) {
        refreshPorts();
    }

    // Connection buttons
    ImGui::Separator();

    if (!isConnected) {
        if (ImGui::Button("Connect") && !availablePorts.empty()) {
            siphogControl.connect(availablePorts[selectedPortIndex]);
        }
    }
    else {
        if (ImGui::Button("Disconnect")) {
            siphogControl.disconnect();
        }
    }

    ImGui::SameLine();
    ImGui::Text("Status: %s", connectionStatus.c_str());

    ImGui::End();
}

void drawSettingsPanel() {
    ImGui::Begin("SiPhOG Settings");

    bool settingsEnabled = isConnected;

    // SLED Current setting
    ImGui::Text("SLED Current (mA):");
    ImGui::BeginDisabled(!settingsEnabled);

    if (ImGui::BeginCombo("##sledcurrent", std::to_string(sledCurrentMa).c_str())) {
        for (int i = 0; i < numSledOptions; i++) {
            bool isSelected = (sledCurrentMa == sledCurrentOptions[i]);
            if (ImGui::Selectable(std::to_string(sledCurrentOptions[i]).c_str(), isSelected)) {
                sledCurrentMa = sledCurrentOptions[i];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Temperature setting
    ImGui::Text("Temperature (°C):");

    if (ImGui::BeginCombo("##temperature", std::to_string(temperatureC).c_str())) {
        for (int i = 0; i < numTempOptions; i++) {
            bool isSelected = (temperatureC == temperatureOptions[i]);
            if (ImGui::Selectable(std::to_string(temperatureOptions[i]).c_str(), isSelected)) {
                temperatureC = temperatureOptions[i];
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Apply settings button
    ImGui::Separator();
    if (ImGui::Button("Apply Settings")) {
        siphogControl.applySettings(sledCurrentMa, temperatureC);
    }

    ImGui::EndDisabled();

    ImGui::End();
}

void drawServerPanel() {
    ImGui::Begin("TCP Server");

    // Server configuration
    ImGui::Text("Server Configuration:");

    // Host input
    static char hostBuffer[256];
    strncpy(hostBuffer, serverHost.c_str(), sizeof(hostBuffer) - 1);
    hostBuffer[sizeof(hostBuffer) - 1] = '\0';

    ImGui::BeginDisabled(serverRunning);
    if (ImGui::InputText("Host", hostBuffer, sizeof(hostBuffer))) {
        serverHost = std::string(hostBuffer);
    }

    // Port input
    ImGui::InputInt("Port", &serverPort);
    if (serverPort < 1024) serverPort = 1024;
    if (serverPort > 65535) serverPort = 65535;
    ImGui::EndDisabled();

    ImGui::Separator();

    // Server control buttons
    if (!serverRunning) {
        if (ImGui::Button("Start Server")) {
            if (siphogControl.startServer(serverHost, serverPort)) {
                addLogMessage("Server started on " + serverHost + ":" + std::to_string(serverPort));
            }
            else {
                addLogMessage("Failed to start server", true);
            }
        }
    }
    else {
        if (ImGui::Button("Stop Server")) {
            siphogControl.stopServer();
            addLogMessage("Server stopped");
        }
    }

    // Server status
    ImGui::Separator();
    ImGui::Text("Status: %s", serverStatus.c_str());

    if (serverRunning) {
        ImGui::Text("Server: %s:%d", serverHost.c_str(), serverPort);

        // Client connection status
        if (clientConnected) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Client Connected");
        }
        else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Waiting for client...");
        }

        // Data keys being broadcast
        ImGui::Separator();
        ImGui::Text("Broadcasting data keys:");
        ImGui::BulletText("SLED_Current (mA)");
        ImGui::BulletText("Photo Current (uA)");
        ImGui::BulletText("SLED_Temp (C)");
        ImGui::BulletText("Target SAG_PWR (V)");
        ImGui::BulletText("SAG_PWR (V)");
        ImGui::BulletText("TEC_Current (mA)");

        // Connection instructions
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "Client connection info:");
        ImGui::Text("Connect to: %s:%d", serverHost.c_str(), serverPort);
        ImGui::Text("Data format: CSV (6 values per line)");
    }

    ImGui::End();
}

void drawDataDisplay() {
    ImGui::Begin("SiPhOG Data");

    if (isConnected) {
        ImGui::Text("Counter: %u", lastMessage.counter);
        ImGui::Text("Time: %.3f s", lastMessage.timeSeconds);

        ImGui::Separator();
        ImGui::Text("Primary Measurements:");
        ImGui::Text("  SLED Current: %.2f mA", lastMessage.sledCurrent);
        ImGui::Text("  SLED Temp: %.2f °C", lastMessage.sledTemp);
        ImGui::Text("  TEC Current: %.2f mA", lastMessage.tecCurrent);

        // Calculate derived values like the server does
        double photoCurrentUa = lastMessage.sldPowerUw / 0.8 * 1e6; // Using PWR_MON_TRANSFER_FUNC
        double targetSagPowerV = 0.1 * lastMessage.sldPowerUw / 0.8 * 1000.0; // TARGET_LOSS_IN_FRACTION * SAGNAC_TIA_GAIN

        ImGui::Separator();
        ImGui::Text("Power Measurements:");
        ImGui::Text("  Photo Current: %.2f µA", photoCurrentUa);
        ImGui::Text("  SAG Power: %.4f V", lastMessage.sagPowerV);
        ImGui::Text("  Target SAG Power: %.4f V", targetSagPowerV);
        ImGui::Text("  SLD Power: %.2f µW", lastMessage.sldPowerUw);

        ImGui::Separator();
        ImGui::Text("ADC Values:");
        ImGui::Text("  ADC_count_I: %.6f V", lastMessage.adcCountI);
        ImGui::Text("  ADC_count_Q: %.6f V", lastMessage.adcCountQ);
        ImGui::Text("  ROTATE_count_I: %.6f V", lastMessage.rotateCountI);
        ImGui::Text("  ROTATE_count_Q: %.6f V", lastMessage.rotateCountQ);

        ImGui::Separator();
        ImGui::Text("Other Sensors:");
        ImGui::Text("  Case Temp: %.2f °C", lastMessage.caseTemp);
        ImGui::Text("  Op Amp Temp: %.2f °C", lastMessage.opAmpTemp);
        ImGui::Text("  Supply Voltage: %.2f V", lastMessage.supplyVoltage);
        ImGui::Text("  SLED Neg: %.4f V", lastMessage.sledNeg);
        ImGui::Text("  SLED Pos: %.4f V", lastMessage.sledPos);
        ImGui::Text("  Status: %u", lastMessage.status);

        // Show what's being broadcast if server is running
        if (serverRunning && clientConnected) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Broadcasting to client:");
            ImGui::Text("%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
                lastMessage.sledCurrent,
                photoCurrentUa,
                lastMessage.sledTemp,
                targetSagPowerV,
                lastMessage.sagPowerV,
                lastMessage.tecCurrent);
        }
    }
    else {
        ImGui::Text("No data - device not connected");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1), "Instructions:");
        ImGui::BulletText("Select a COM port and click Connect");
        ImGui::BulletText("Device will auto-initialize and start streaming data");
        ImGui::BulletText("Use Settings panel to control SLED current and temperature");
        ImGui::BulletText("Use TCP Server panel to broadcast data to clients");
    }

    ImGui::End();
}

void drawLogWindow() {
    ImGui::Begin("Log Output");

    if (ImGui::Button("Clear Log")) {
        logMessages.clear();
        addLogMessage("Log cleared");
    }

    ImGui::Separator();

    // Log messages in a scrollable region
    ImGui::BeginChild("LogScrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& message : logMessages) {
        // Color code error messages
        if (message.find("ERROR") != std::string::npos || message.find("Failed") != std::string::npos) {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", message.c_str());
        }
        else if (message.find("Connected") != std::string::npos || message.find("started") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "%s", message.c_str());
        }
        else if (message.find("WARNING") != std::string::npos) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0.4f, 1), "%s", message.c_str());
        }
        else {
            ImGui::TextUnformatted(message.c_str());
        }
    }

    // Auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}

void drawStatusBar() {
    // Status bar at the bottom
    ImGui::SetNextWindowPos(ImVec2(0, GetScreenHeight() - 25));
    ImGui::SetNextWindowSize(ImVec2(GetScreenWidth(), 25));
    ImGui::Begin("StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);

    // Device status
    if (isConnected) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Device: Connected");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Device: Disconnected");
    }

    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

    // Server status
    if (serverRunning) {
        if (clientConnected) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Server: Client Connected");
        }
        else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Server: Waiting for client");
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "Server: Stopped");
    }

    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

    // Message count
    if (isConnected) {
        ImGui::Text("Messages: %u", lastMessage.counter);
    }
    else {
        ImGui::Text("Messages: 0");
    }

    ImGui::End();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 900, "SiPhOG Control & Server Application");

#pragma region imgui
    rlImGuiSetup(true);

    // Set ImGui theme
    imguiThemes::green();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.FontGlobalScale = 1.2f;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.Colors[ImGuiCol_WindowBg].w = 0.8f;
    }
#pragma endregion

    // Initialize SiPhOG control
    setupSiphogCallbacks();
    refreshPorts();
    addLogMessage("SiPhOG Control & Server Application started");
    addLogMessage("Platform wrappers loaded - no Windows header conflicts!");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

#pragma region imgui
        rlImGuiBegin();

        // Create docking space
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        ImGui::PopStyleColor(2);
#pragma endregion

        // Draw UI panels
        drawConnectionPanel();
        drawSettingsPanel();
        drawServerPanel();
        drawDataDisplay();
        drawLogWindow();
        drawStatusBar();

        // Draw some background graphics
        DrawText("SiPhOG Control & Server System", 10, 10, 20, LIGHTGRAY);

#pragma region imgui
        rlImGuiEnd();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
#pragma endregion

        EndDrawing();
    }

    // Clean up
    siphogControl.stopServer();
    siphogControl.disconnect();

#pragma region imgui
    rlImGuiShutdown();
#pragma endregion

    CloseWindow();
    return 0;
}