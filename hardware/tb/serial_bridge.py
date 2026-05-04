import serial
import subprocess
import json
import sys
import time

# PM Lucci Note: This port will be updated based on your next report
ESP_PORT = '/dev/ttyUSB0' 
BAUD_RATE = 921600

print("[System] Initializing HIL Serial Bridge...")

try:
    # 1. Open the UART connection to the physical esp-s3
    esp = serial.Serial(ESP_PORT, BAUD_RATE, timeout=1)
    
    # 2. Spin up the Virtual Silicon (Verilator Executable)
    verilator_sim = subprocess.Popen(
        ['./obj_dir/Vsecurity_puf'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True
    )
    print(f"[System] Link Established on {ESP_PORT} at {BAUD_RATE} baud.")
    print("[System] Awaiting AI challenges from esp-s3...")

except Exception as e:
    print(f"[CRITICAL FAILURE] Bridge Initialization Error: {e}")
    sys.exit(1)

# 3. The Real-Time Synchronization Loop
while True:
    if esp.in_waiting > 0:
        raw_payload = esp.readline().decode('utf-8', errors='ignore').strip()
        if not raw_payload:
            continue
            
        try:
            # Parse ESP-S3 JSON
            packet = json.loads(raw_payload)
            if "challenge" not in packet:
                continue
                
            challenge_val = packet["challenge"]
            
            # Pipe into Verilator
            verilator_sim.stdin.write(f"{challenge_val}\n")
            verilator_sim.stdin.flush() # Prevent OS pipe deadlock
            
            # Read Virtual Silicon Response
            sim_response = verilator_sim.stdout.readline().strip()
            
            # Send back to esp-s3 with newline for FreeRTOS parsing
            out_payload = sim_response + "\n"
            esp.write(out_payload.encode('utf-8'))
            print(f"[RTL] Processed Challenge {challenge_val} -> {sim_response}")
            
        except json.JSONDecodeError:
            print(f"[WARNING] Frame Drop or Buffer Overrun: {raw_payload}")