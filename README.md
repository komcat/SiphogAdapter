# Quick Test Guide - SiPhOG System

## 🚀 Quick Start (5 minutes)

### 1. **Hardware Setup**
```
1. Connect SiPhOG device to computer USB port
2. Verify device power LED is ON
3. Note the COM port (e.g., COM3, COM4)
```

### 2. **Launch Application**
```
1. Run SiphogAdapter.exe
2. Select correct COM port from dropdown
3. Click "Connect" button
4. Wait for green "Connected" status
```

### 3. **Verify Data Stream**
```
✅ Check "SiPhOG Data" panel shows live values
✅ Chart displays two lines (SAG Power vs Target)
✅ Status bar shows message count increasing
✅ No error messages in log window
```

### 4. **Test TCP Server**
```
1. Click "Start Server" in TCP Server panel
2. Verify "Server running, waiting for connection..." message
3. Note the IP:Port (usually 127.0.0.1:65432)
```

### 5. **Test Python Client**
```bash
# Open command prompt/terminal
cd path/to/your/project
python siphog_client_2025.py

# Should show:
🔌 Connecting to 127.0.0.1:65432...
✅ Connected! Starting data monitor...
```

## 🧪 Test Scenarios

### Test 1: Basic Device Communication
**Expected Result:** Live data streaming at ~200Hz
```
✅ PASS: Charts show moving lines
✅ PASS: Data values change continuously  
✅ PASS: Message counter increases steadily
❌ FAIL: No data → Check COM port/device connection
```

### Test 2: Device Control
**Expected Result:** Settings applied to device
```
Steps:
1. Change SLED Current (e.g., 100mA → 150mA)
2. Click "Apply Settings"
3. Observe chart for current change

✅ PASS: Chart shows current change within 2-3 seconds
❌ FAIL: No change → Check device communication
```

### Test 3: TCP Server & Client
**Expected Result:** Remote client receives real-time data
```
Setup:
1. Start server in main app
2. Run Python client
3. Observe data in both applications

✅ PASS: Python client shows live data matching main app
✅ PASS: Data rate >100Hz in Python client
❌ FAIL: Connection refused → Check firewall/port
```

### Test 4: Chart Export
**Expected Result:** CSV file with sensor data
```
Steps:
1. Let system run for 30 seconds
2. Click "Export to CSV"
3. Check generated file

✅ PASS: File contains timestamped data rows
✅ PASS: File opens in Excel/text editor
❌ FAIL: Export fails → Check disk space/permissions
```

## 🔧 Common Test Issues

### Issue: No Device Connection
```
Symptoms: Red "Disconnected" status
Solutions:
□ Try different COM port
□ Check USB cable
□ Restart application as Administrator
□ Verify device is powered on
```

### Issue: Chart Not Updating
```
Symptoms: Flat lines or "No data"
Solutions:
□ Check device connection first
□ Clear chart data (button)
□ Verify messages counting up in status bar
□ Restart application if needed
```

### Issue: Python Client Connection Failed
```
Symptoms: "Failed to connect" error
Solutions:
□ Verify server is started (green status)
□ Check IP address: use 127.0.0.1 for local
□ Try different port: python client.py 127.0.0.1 8080
□ Check Windows Firewall settings
```

### Issue: Poor Performance/Crashes
```
Symptoms: Slow updates, application freezing
Solutions:
□ Reduce chart buffer size (edit source: ChartDataManager(200))
□ Close other applications
□ Check Task Manager for memory usage
□ Restart in Debug mode for detailed logging
```

## 📊 Success Criteria

### ✅ **Fully Working System:**
- Device connects successfully (green status)
- Charts show smooth, continuous data (two moving lines)
- Data values are reasonable (not all zeros/NaN)
- TCP server accepts connections (client connects)
- Python client displays live data with >100Hz rate
- Export produces valid CSV files
- No critical errors in log window

### ⚠️ **Partially Working System:**
- Device connects but charts choppy (check USB connection)
- Server works but slow data rate (check network)
- Some data fields show zeros (possible sensor issues)

### ❌ **System Problems:**
- Cannot connect to device (hardware/driver issue)
- Application crashes (build/memory issue)
- No chart data despite connection (communication issue)
- Server won't start (port conflict)

## 🐛 Debug Information

### Useful Log Messages
```
✅ "Connected to COM3 at 691200 baud"
✅ "Device initialized with factory unlock"
✅ "Server started on 127.0.0.1:65432"
✅ "Client connected: 127.0.0.1:xxxxx"

⚠️ "Messages: 0" (after 5+ seconds)
⚠️ "Data size mismatch"
⚠️ "Failed to parse message"

❌ "Failed to open serial port"
❌ "Device initialization error"
❌ "Server bind failed"
❌ "Heap corruption detected"
```

### Performance Benchmarks
```
Good Performance:
- Data rate: 150-200 Hz
- CPU usage: <10%
- Memory: <100MB
- Chart FPS: 60

Poor Performance:
- Data rate: <50 Hz
- CPU usage: >50%
- Memory: >500MB
- Chart updates laggy
```

## 📞 Getting Help

### Information to Collect:
1. **System Info**: Windows version, application version
2. **Hardware**: SiPhOG device model, COM port number
3. **Error Messages**: Copy exact text from log window
4. **Steps to Reproduce**: What you did when problem occurred
5. **Screenshots**: Especially of error dialogs or unexpected behavior

### Quick Diagnostics:
```bash
# Check COM ports
Device Manager → Ports (COM & LPT)

# Check network ports
netstat -an | findstr 65432

# Check running processes
tasklist | findstr SiphogAdapter
```

### Test Command Line:
```bash
# Test Python dependencies
python --version
python -c "import socket; print('Socket OK')"

# Test server connection
telnet 127.0.0.1 65432
```

---

**🎯 Expected Test Duration:** 5-10 minutes for full system verification  
**🔧 Troubleshooting Time:** 10-30 minutes for common issues  
**📈 Success Rate:** >90% on properly configured systems  

*Last Updated: January 2025*