# RaylibCmakeSetup 

---

## What is it?

I already set up a Raylib project for you! Take it and enjoy! You don't need to know CMake!

![image](https://github.com/meemknight/raylibCmakeSetup/assets/36445656/c50ab777-0cde-4d80-8df6-a0fd483f169d)


<p>Opening the Solution:</p> 

<img src="https://raw.githubusercontent.com/meemknight/photos/master/llge1.gif" width="350">

Or

<img src="https://raw.githubusercontent.com/meemknight/photos/master/llge2.gif" width="500">

Running the setup

Go to CMakeLists.txt, <kbd>CTRL + S</kbd> to make sure the solution was built.

Then, from this dropdown select mygame.exe

<img src="https://raw.githubusercontent.com/meemknight/photos/master/llge3.gif" width="200">

<kbd>Ctrl + F5</kbd> to build (<kbd>F5</kbd> oppens the debugger, you usually want to press <kbd>Ctrl + F5</kbd> because it oppens faster like this.

<p>Adding files:<br>
You should add .cpp in src/ and .h in include/ Whenever you add a new file CMake will ask you if you want to add that thing, say NO every time! I am already adding all of the things automatically!
If you accidentally say YES, just remove that file from the CMake.lists
</p>

<p>Refreshing your changes:<br>
After you add a file, the changes should be automatically added but if you want to be sure, you can refresh changes by saving the CMake file. If you want to make a hard refresh (you might have to do that sometimes) close Visual Studio, delete the out folder, reopen VS, <kbd>CTRL + S</kbd> on CMakeLists.txt</p>


# IMPORTANT!
  To ship the game: 
  In Cmakelists.txt, set the PRODUCTION_BUILD flag to ON to build a shippable version of your game. This will change the file paths to be relative to your exe (RESOURCES_PATH macro), will remove the console, and also will change the asserts to not allow people to debug them. To make sure the changes take effect I recommend deleting the out folder to make a new clean build!


  Also, if you read the CMAKE, even if you don't know CMAKE you should understand what happens with the comments there and you can add libraries and also remove the console from there if you need to! (there is a commented line for that!)



  # SiPhOG C++ Control Application

This project is a C++ port of the SiPhOG control library, originally written in C#. It provides a cross-platform interface for communicating with SiPhOG (Silicon Photonic Gyroscope) devices via serial communication.

## Project Structure

```
SiphogCpp/
├── src/
│   ├── main.cpp                    # Main application with raylib/ImGui UI
│   └── siphog/                     # SiPhOG library source files
│       ├── ParseMessage.cpp        # Message parsing and data conversion
│       ├── SiphogMessageModel.cpp  # Data model for parsed messages
│       ├── SiphogCommand.cpp       # Device command interface
│       ├── SerialPort.cpp          # Cross-platform serial communication
│       └── SiphogControl.cpp       # Main control class
├── include/
│   └── siphog/                     # SiPhOG library header files
│       ├── ParseMessage.h
│       ├── SiphogMessageModel.h
│       ├── SiphogCommand.h
│       ├── SerialPort.h
│       └── SiphogControl.h
├── thirdparty/                     # Third-party libraries
│   ├── raylib-5.0/
│   ├── imgui-docking/
│   └── rlImGui/
└── CMakeLists.txt
```

## Features

### SiPhOG Library Features
- **Cross-platform serial communication** (Windows and Linux)
- **Automatic message parsing** from SiPhOG device data streams
- **Device control commands** (factory unlock, current/temperature setpoints)
- **Real-time data conversion** with proper units and scaling
- **Thread-safe operation** with callback-based architecture

### Application Features
- **ImGui-based UI** with docking support
- **Real-time data display** of all sensor readings
- **Connection management** with automatic port detection
- **Settings control** for SLED current and temperature
- **Comprehensive logging** with timestamped messages

## Building

### Prerequisites
- CMake 3.16 or higher
- C++17 compatible compiler
- Git (for submodules)

### Build Steps
1. Clone the repository with submodules:
   ```bash
   git clone --recursive <repository-url>
   cd SiphogCpp
   ```

2. Create build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   cmake --build . --config Release
   ```

## Usage

### Basic Connection
1. Launch the application
2. Select a COM port from the dropdown
3. Click "Connect" to establish communication
4. The device will automatically be initialized with factory unlock and control modes

### Device Control
- Use the Settings panel to adjust SLED current (50-200 mA) and temperature (20-35°C)
- Click "Apply Settings" to send new parameters to the device
- Real-time data will be displayed in the Data panel

### Data Monitoring
The application displays real-time data including:
- **ADC Values**: I/Q channels and rotated counts
- **Temperature Readings**: SLED, case, and op-amp temperatures
- **Current Measurements**: SLED and TEC currents
- **Power Measurements**: Sagnac and SLED power levels
- **Supply Voltages**: Various voltage rails

## SiPhOG Protocol

The SiPhOG device communicates using a custom binary protocol:

### Message Format
- **Start Sequence**: 0xF2, 0x47
- **Message Length**: 76 bytes total
- **Data Rate**: ~200 Hz (5ms intervals)

### Data Fields
The parsed message contains over 20 different sensor readings including:
- 24-bit ADC values for precision measurements
- 10-bit ADC values for ancillary sensors
- Temperature readings with thermistor linearization
- Current measurements with proper scaling
- Power calculations for optical signals

## Platform Notes

### Windows
- Uses Win32 API for serial communication
- Requires `setupapi.lib` for port enumeration
- Supports COM1-COM99 port naming

### Linux
- Uses standard POSIX termios for serial communication
- Looks for `/dev/ttyUSB*`, `/dev/ttyACM*`, and `/dev/ttyS*` devices
- May require user to be in `dialout` group for port access

## API Reference

### SiphogControl Class
The main interface for device communication:

```cpp
// Connection management
bool connect(const std::string& portName);
void disconnect();
bool getIsConnected() const;

// Device control
bool applySettings(int currentMa, int temperatureC);

// Data access
const SiphogMessageModel& getLastMessage() const;

// Callbacks
void setLogCallback(std::function<void(const std::string&, bool)> callback);
void setMessageCallback(std::function<void(const SiphogMessageModel&)> callback);
```

### SiphogMessageModel Structure
Contains all parsed sensor data with meaningful units:
- Temperatures in Celsius
- Currents in milliamps
- Powers in microwatts
- Voltages in volts
- Timestamps and counters

## License

This project is derived from the original C# SiPhOG control library and maintains compatibility with the same device protocol and data formats.