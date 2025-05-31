#!/usr/bin/env python3
"""
Smooth Python client for SiPhOG TCP server - no blinking!
Updates display in-place without screen clearing
"""

import socket
import time
import sys
import os
from datetime import datetime
from collections import deque

class SmoothSiPhogClient:
    def __init__(self, host="127.0.0.1", port=65432):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.data_count = 0
        self.last_data_time = None
        self.start_time = None
        
        # Rate calculation
        self.rate_history = deque(maxlen=50)  # Last 50 timestamps
        
        # Data keys matching the server
        self.data_keys = [
            "SLED_Current (mA)",
            "Photo Current (uA)", 
            "SLED_Temp (C)",
            "Target SAG_PWR (V)",
            "SAG_PWR (V)",
            "TEC_Current (mA)"
        ]
        
        # Current data storage
        self.current_data = {}
        self.display_initialized = False
        
    def connect(self):
        """Connect to the SiPhOG server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(5.0)
            print(f"🔌 Connecting to {self.host}:{self.port}...")
            self.socket.connect((self.host, self.port))
            self.socket.settimeout(0.1)  # Short timeout for smooth updates
            print(f"✅ Connected! Starting data monitor...\n")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the server"""
        self.running = False
        if self.socket:
            self.socket.close()
            self.socket = None
    
    def parse_data(self, data_str):
        """Parse CSV data from server"""
        try:
            clean_data = data_str.strip()
            if not clean_data:
                return None
                
            values = [float(x.strip()) for x in clean_data.split(',')]
            if len(values) == 6:
                return dict(zip(self.data_keys, values))
            else:
                return None
        except:
            return None
    
    def calculate_rate(self):
        """Calculate current data rate"""
        if len(self.rate_history) < 2:
            return 0.0
        
        time_span = self.rate_history[-1] - self.rate_history[0]
        if time_span > 0:
            return (len(self.rate_history) - 1) / time_span
        return 0.0
    
    def init_display(self):
        """Initialize the display layout"""
        if self.display_initialized:
            return
            
        # Clear screen once
        os.system('cls' if os.name == 'nt' else 'clear')
        
        # Print static header
        print("╔" + "═" * 58 + "╗")
        print("║" + " " * 15 + "🔬 SiPhOG Data Monitor" + " " * 20 + "║")
        print("╠" + "═" * 58 + "╣")
        print("║ Time:                                                  ║")
        print("║ Server:                                                ║")
        print("║ Messages:                    Rate:                     ║")
        print("╠" + "═" * 58 + "╣")
        print("║ 📊 LASER & CONTROL:                                    ║")
        print("║   SLED Current:                                   mA   ║")
        print("║   SLED Temp:                                      °C   ║")
        print("║   TEC Current:                                    mA   ║")
        print("║                                                        ║")
        print("║ ⚡ OPTICAL POWER:                                      ║")
        print("║   Photo Current:                                  µA   ║")
        print("║   SAG Power:                                      V    ║")
        print("║   Target SAG PWR:                                 V    ║")
        print("║                                                        ║")
        print("║ 📈 STATUS: Streaming...                                ║")
        print("║                                                        ║")
        print("║ 💡 Press Ctrl+C to stop                               ║")
        print("╚" + "═" * 58 + "╝")
        
        self.display_initialized = True
    
    def update_field(self, row, col, text, width=None):
        """Update a specific field without redrawing the whole screen"""
        if width:
            text = f"{text:<{width}}"
        
        # Move cursor to position and update
        print(f"\033[{row};{col}H{text}", end="", flush=True)
    
    def update_display(self, data):
        """Update display with new data - no blinking!"""
        self.data_count += 1
        current_time = time.time()
        self.rate_history.append(current_time)
        
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        rate = self.calculate_rate()
        
        # Update dynamic fields only
        self.update_field(4, 8, timestamp, 25)
        self.update_field(5, 10, f"{self.host}:{self.port}", 25)
        self.update_field(6, 12, f"{self.data_count}", 15)
        self.update_field(6, 35, f"{rate:.1f} Hz", 15)
        
        # Update data fields
        self.update_field(9, 18, f"{data['SLED_Current (mA)']:8.2f}", 15)
        self.update_field(10, 15, f"{data['SLED_Temp (C)']:8.2f}", 15)
        self.update_field(11, 17, f"{data['TEC_Current (mA)']:8.2f}", 15)
        
        self.update_field(14, 19, f"{data['Photo Current (uA)']:8.2f}", 15)
        self.update_field(15, 15, f"{data['SAG_PWR (V)']:8.4f}", 15)
        self.update_field(16, 19, f"{data['Target SAG_PWR (V)']:8.4f}", 15)
        
        # Update status with data quality indicator
        if rate > 30:
            status = "🟢 Excellent"
        elif rate > 10:
            status = "🟡 Good"
        else:
            status = "🔴 Slow"
        
        self.update_field(18, 16, status, 20)
    
    def run(self):
        """Main client loop with smooth updates"""
        if not self.connect():
            return
        
        self.running = True
        self.start_time = time.time()
        buffer = ""
        
        # Initialize display
        self.init_display()
        
        try:
            while self.running:
                try:
                    # Receive data from server
                    data = self.socket.recv(1024)
                    if not data:
                        self.update_field(18, 16, "❌ Server disconnected", 20)
                        break
                    
                    data_str = data.decode('utf-8')
                    buffer += data_str
                    
                    # Process complete messages
                    while '\n' in buffer or ',' in buffer:
                        if '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                        else:
                            parts = buffer.split(',')
                            if len(parts) >= 6:
                                line = ','.join(parts[:6])
                                buffer = ','.join(parts[6:]) if len(parts) > 6 else ""
                            else:
                                break
                        
                        line = line.strip()
                        if line and ',' in line:
                            parsed_data = self.parse_data(line)
                            if parsed_data:
                                self.update_display(parsed_data)
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    self.update_field(18, 16, f"❌ Error: {str(e)[:15]}", 20)
                    break
                    
        except KeyboardInterrupt:
            # Move cursor below the box for clean exit
            print(f"\033[22;1H")
            print("🛑 Stopped by user")
        finally:
            print(f"\n📊 Session summary:")
            print(f"   Total messages: {self.data_count}")
            print(f"   Average rate: {self.data_count/max(1, time.time()-self.start_time):.1f} Hz")
            print(f"   Duration: {time.time()-self.start_time:.1f} seconds")
            self.disconnect()

def main():
    print("🚀 Smooth SiPhOG Client Starting...")
    
    # Allow custom host/port via command line
    host = "127.0.0.1"
    port = 65432
    
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    if len(sys.argv) >= 3:
        try:
            port = int(sys.argv[2])
        except ValueError:
            print(f"❌ Invalid port: {sys.argv[2]}")
            sys.exit(1)
    
    # Create and run smooth client
    client = SmoothSiPhogClient(host, port)
    client.run()

if __name__ == "__main__":
    main()