#include "raylib.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#pragma region imgui
#include "imgui.h"
#include "rlImGui.h"
#include "imguiThemes.h"
#include "implot.h"  // Add ImPlot header
#pragma endregion

// Include SiPhOG library
#include "SiphogControl.h"
#include "ChartData.h"  // Include our chart data manager

// Global SiPhOG control instance
SiphogLib::SiphogControl siphogControl;

// Chart data manager with smaller buffer to reduce memory issues
SiphogLib::ChartDataManager chartManager(500); // Reduced from 2000 to 500 data points

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

// Chart settings
static float timeWindowSeconds = 30.0f;
static bool autoScale = true;
static bool showGrid = true;

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
        // Add data to charts
        chartManager.addDataPoint(message);
        });

    // Set up connection callback
    siphogControl.setConnectionCallback([](bool connected, const std::string& portName) {
        isConnected = connected;
        if (connected) {
            connectionStatus = "Connected to " + portName;
        }
        else {
            connectionStatus = "Not Connected";
            // Clear chart data when disconnected
            chartManager.clear();
        }
        });

    // Set up server status callback
    siphogControl.setServerStatusCallback([](const std::string& status) {
        serverStatus = status;

        bool newServerRunning = siphogControl.isServerRunning();
        bool newClientConnected = siphogControl.isClientConnected();

        if (newServerRunning != serverRunning) {
            serverRunning = newServerRunning;
            addLogMessage("Server running state changed: " + std::string(serverRunning ? "true" : "false"));
        }

        if (newClientConnected != clientConnected) {
            clientConnected = newClientConnected;
            addLogMessage("Client connected state changed: " + std::string(clientConnected ? "true" : "false"));
        }

        addLogMessage("Server status update: " + status);
        });
}

void drawConnectionPanel() {
    ImGui::Begin("SiPhOG Connection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

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
    if (isConnected) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: %s", connectionStatus.c_str());
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: %s", connectionStatus.c_str());
    }

    ImGui::End();
}

void drawSettingsPanel() {
    ImGui::Begin("SiPhOG Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    bool settingsEnabled = isConnected;

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

    ImGui::Separator();
    if (ImGui::Button("Apply Settings")) {
        siphogControl.applySettings(sledCurrentMa, temperatureC);
    }

    ImGui::EndDisabled();

    ImGui::End();
}

void drawServerPanel() {
    ImGui::Begin("TCP Server");

    ImGui::Text("Server Configuration:");

    static char hostBuffer[256];
    strncpy(hostBuffer, serverHost.c_str(), sizeof(hostBuffer) - 1);
    hostBuffer[sizeof(hostBuffer) - 1] = '\0';

    ImGui::BeginDisabled(serverRunning);
    if (ImGui::InputText("Host", hostBuffer, sizeof(hostBuffer))) {
        serverHost = std::string(hostBuffer);
    }

    ImGui::InputInt("Port", &serverPort);
    if (serverPort < 1024) serverPort = 1024;
    if (serverPort > 65535) serverPort = 65535;
    ImGui::EndDisabled();

    ImGui::Separator();

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

    ImGui::Separator();
    ImGui::Text("Status: %s", serverStatus.c_str());

    if (serverRunning) {
        ImGui::Text("Server: %s:%d", serverHost.c_str(), serverPort);

        if (clientConnected) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Client Connected");
        }
        else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Waiting for client...");
        }

        ImGui::Separator();
        ImGui::Text("Broadcasting data keys:");
        ImGui::BulletText("SLED_Current (mA)");
        ImGui::BulletText("Photo Current (uA)");
        ImGui::BulletText("SLED_Temp (C)");
        ImGui::BulletText("Target SAG_PWR (V)");
        ImGui::BulletText("SAG_PWR (V)");
        ImGui::BulletText("TEC_Current (mA)");
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

        //// Calculate derived values like the server does
        //double photoCurrentUa = lastMessage.sagPowerV / (249.0 * 8.5) * 1e6; // Using correct PWR_MON_TRANSFER_FUNC
        //double targetSagPowerV = 0.269153 * lastMessage.sagPowerV / (249.0 * 8.5) * 20000.0; // TARGET_LOSS_IN_FRACTION * SAGNAC_TIA_GAIN

        ImGui::Separator();
        ImGui::Text("Power Measurements:");
        ImGui::Text("  Photo Current: %.2f µA", lastMessage.photoCurrentUa);
        ImGui::Text("  SAG Power: %.4f V", lastMessage.sagPowerV);
        ImGui::Text("  Target SAG Power: %.4f V", lastMessage.targetSagPowerV);
        ImGui::Text("  SLD Power (V): %.4f V", lastMessage.sldPowerV);    // <-- ADD THIS LINE
        ImGui::Text("  SLD Power: %.2f µW", lastMessage.sldPowerUw);

        ImGui::Separator();
        ImGui::Text("Other Sensors:");
        ImGui::Text("  Case Temp: %.2f °C", lastMessage.caseTemp);
        ImGui::Text("  Op Amp Temp: %.2f °C", lastMessage.opAmpTemp);
        ImGui::Text("  Supply Voltage: %.2f V", lastMessage.supplyVoltage);

        // Display statistics
        if (chartManager.hasData()) {
            ImGui::Separator();
            ImGui::Text("Statistics:");

            auto sledCurrentStats = chartManager.getDataStats(chartManager.getSledCurrentVector());
            ImGui::Text("  SLED Current - Min: %.2f, Max: %.2f, Avg: %.2f mA",
                sledCurrentStats.min, sledCurrentStats.max, sledCurrentStats.mean);

            auto sledTempStats = chartManager.getDataStats(chartManager.getSledTempVector());
            ImGui::Text("  SLED Temp - Min: %.2f, Max: %.2f, Avg: %.2f °C",
                sledTempStats.min, sledTempStats.max, sledTempStats.mean);
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

void drawChartsPanel() {
    ImGui::Begin("SAG Power Chart");

    // Chart controls
    ImGui::SliderFloat("Time Window (s)", &timeWindowSeconds, 5.0f, 300.0f, "%.1f");
    ImGui::SameLine();
    ImGui::Checkbox("Auto Scale", &autoScale);
    ImGui::SameLine();
    ImGui::Checkbox("Show Grid", &showGrid);

    if (ImGui::Button("Clear Chart Data")) {
        chartManager.clear();
    }
    ImGui::SameLine();
    ImGui::Text("Data Points: %zu", chartManager.getDataPointCount());

    if (ImGui::Button("Export to CSV")) {
        std::string filename = "sag_power_data_" + std::to_string(static_cast<int>(GetTime())) + ".csv";
        if (chartManager.exportToCsv(filename)) {
            addLogMessage("Data exported to " + filename);
        }
        else {
            addLogMessage("Failed to export data", true);
        }
    }

    if (!isConnected || !chartManager.hasData()) {
        ImGui::Text("No data to display - connect to device to see charts");
        ImGui::End();
        return;
    }

    // Get time data
    auto timeVec = chartManager.getTimeVector();
    auto timeWindow = chartManager.getTimeWindow(timeWindowSeconds);

    // Configure ImPlot style
    if (showGrid) {
        ImPlot::GetStyle().PlotBorderSize = 1;
        ImPlot::GetStyle().PlotPadding = ImVec2(10, 10);
    }

    // SAG Power Chart - Single chart showing both SAG Power and Target SAG Power
    if (ImPlot::BeginPlot("SAG Power vs Target SAG Power", ImVec2(-1, 500))) {
        ImPlot::SetupAxes("Time (s)", "Power (V)");
        if (autoScale) {
            ImPlot::SetupAxisLimits(ImAxis_X1, timeWindow.first, timeWindow.second, ImGuiCond_Always);
        }

        // Plot actual SAG Power
        auto sagPowerVec = chartManager.getSagPowerVector();
        if (!sagPowerVec.empty()) {
            ImPlot::PlotLine("SAG Power (V)", timeVec.data(), sagPowerVec.data(), timeVec.size());
        }

        // Calculate and plot Target SAG Power
        auto sldPowerVec = chartManager.getSldPowerVector();
        if (!sldPowerVec.empty() && !timeVec.empty()) {
            std::vector<double> targetSagPowerVec;
            targetSagPowerVec.reserve(sldPowerVec.size());

            // Calculate target SAG power for each data point
            for (const auto& sldPower : sldPowerVec) {
                double targetSagPowerV = 0.1 * sldPower / 0.8 * 1000.0; // TARGET_LOSS_IN_FRACTION * SAGNAC_TIA_GAIN
                targetSagPowerVec.push_back(targetSagPowerV);
            }

            ImPlot::PlotLine("Target SAG Power (V)", timeVec.data(), targetSagPowerVec.data(), timeVec.size());
        }

        ImPlot::EndPlot();
    }

    // Display current values below the chart
    ImGui::Separator();
    ImGui::Text("Current Values:");
    if (isConnected) {
        double currentSagPower = lastMessage.sagPowerV;
        double currentTargetSagPower = 0.1 * lastMessage.sldPowerUw / 0.8 * 1000.0;
        double difference = currentSagPower - currentTargetSagPower;

        ImGui::Text("SAG Power: %.6f V", currentSagPower);
        ImGui::Text("Target SAG Power: %.6f V", currentTargetSagPower);

        // Color-code the difference
        if (abs(difference) < 0.001) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Difference: %.6f V (Good)", difference);
        }
        else if (abs(difference) < 0.01) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Difference: %.6f V (Warning)", difference);
        }
        else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Difference: %.6f V (Error)", difference);
        }
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

    ImGui::BeginChild("LogScrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& message : logMessages) {
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

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}

void drawStatusBar() {
    ImGui::SetNextWindowPos(ImVec2(0, GetScreenHeight() - 25));
    ImGui::SetNextWindowSize(ImVec2(GetScreenWidth(), 25));
    ImGui::Begin("StatusBar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings);

    if (isConnected) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Device: Connected");
    }
    else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Device: Disconnected");
    }

    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

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

    if (isConnected) {
        ImGui::Text("Messages: %u", lastMessage.counter);
    }
    else {
        ImGui::Text("Messages: 0");
    }

    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

    ImGui::Text("Chart Points: %zu", chartManager.getDataPointCount());

    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

    if (chartManager.hasData()) {
        ImGui::Text("Data Rate: %.1f Hz", chartManager.getDataPointCount() > 1 ?
            1.0 / (chartManager.getLatestTime() / chartManager.getDataPointCount()) : 0.0);
    }
    else {
        ImGui::Text("Data Rate: 0.0 Hz");
    }

    ImGui::End();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "SiPhOG Control & Server Application with Real-time Charts");

#pragma region imgui
    rlImGuiSetup(true);

    // Initialize ImPlot
    ImPlot::CreateContext();

    // Set ImGui theme
    imguiThemes::green();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.FontGlobalScale = 1.1f;

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.Colors[ImGuiCol_WindowBg].w = 0.8f;
    }
#pragma endregion

    // Initialize SiPhOG control
    setupSiphogCallbacks();
    refreshPorts();
    addLogMessage("SiPhOG Control & Server Application with Real-time Charts started");
    addLogMessage("ImPlot charting system initialized");
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
        drawChartsPanel();
        drawLogWindow();
        drawStatusBar();

        // Draw background title
        DrawText("SiPhOG Control & Server System with Real-time Charts", 10, 10, 18, LIGHTGRAY);

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
    ImPlot::DestroyContext();
    rlImGuiShutdown();
#pragma endregion

    CloseWindow();
    return 0;
}