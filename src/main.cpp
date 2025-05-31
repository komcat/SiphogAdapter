#include "WindowsPlatform.h"
#include "raylib.h"
#include <iostream>
#include <vector>
#include <string>

#pragma region imgui
#include "imgui.h"
#include "rlImGui.h"
#include "imguiThemes.h"
#pragma endregion

// Include SiPhOG library
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

void drawDataDisplay() {
  ImGui::Begin("SiPhOG Data");

  if (isConnected) {
    ImGui::Text("Counter: %u", lastMessage.counter);
    ImGui::Text("Time: %.3f s", lastMessage.timeSeconds);

    ImGui::Separator();
    ImGui::Text("ADC Values:");
    ImGui::Text("  ADC_count_I: %.6f V", lastMessage.adcCountI);
    ImGui::Text("  ADC_count_Q: %.6f V", lastMessage.adcCountQ);
    ImGui::Text("  ROTATE_count_I: %.6f V", lastMessage.rotateCountI);
    ImGui::Text("  ROTATE_count_Q: %.6f V", lastMessage.rotateCountQ);

    ImGui::Separator();
    ImGui::Text("Temperature & Current:");
    ImGui::Text("  SLED Temp: %.2f °C", lastMessage.sledTemp);
    ImGui::Text("  Case Temp: %.2f °C", lastMessage.caseTemp);
    ImGui::Text("  SLED Current: %.2f mA", lastMessage.sledCurrent);
    ImGui::Text("  TEC Current: %.2f mA", lastMessage.tecCurrent);

    ImGui::Separator();
    ImGui::Text("Power & Voltages:");
    ImGui::Text("  SAG Power: %.4f V (%.2f µW)", lastMessage.sagPowerV, lastMessage.sagPowerUw);
    ImGui::Text("  SLD Power: %.2f µW", lastMessage.sldPowerUw);
    ImGui::Text("  Supply Voltage: %.2f V", lastMessage.supplyVoltage);

    ImGui::Separator();
    ImGui::Text("Other Sensors:");
    ImGui::Text("  Op Amp Temp: %.2f °C", lastMessage.opAmpTemp);
    ImGui::Text("  SLED Neg: %.4f V", lastMessage.sledNeg);
    ImGui::Text("  SLED Pos: %.4f V", lastMessage.sledPos);
    ImGui::Text("  Status: %u", lastMessage.status);
  }
  else {
    ImGui::Text("No data - device not connected");
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
    ImGui::TextUnformatted(message.c_str());
  }

  // Auto-scroll to bottom
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
  ImGui::End();
}

int main(void) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1200, 800, "SiPhOG Control Application");

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
  addLogMessage("SiPhOG Control Application started");

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
    drawDataDisplay();
    drawLogWindow();

    // Draw some background graphics
    DrawText("SiPhOG Control System", 10, 10, 20, LIGHTGRAY);

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
  siphogControl.disconnect();

#pragma region imgui
  rlImGuiShutdown();
#pragma endregion

  CloseWindow();
  return 0;
}