import serial
import subprocess
import json
import sys
import csv
import time

# Configuration
ESP_PORT = '/dev/ttyACM0' 
BAUD_RATE = 921600
CSV_FILENAME = "training_data.csv"
MAX_SAMPLES = 1000

print(f"[System] Initializing Data Harvest. Target: {MAX_SAMPLES} samples.")

try:
    esp = serial.Serial(ESP_PORT, BAUD_RATE, timeout=1)
    verilator_sim = subprocess.Popen(
        ['./obj_dir/Vsecurity_puf'],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True
    )
except Exception as e:
    print(f"[CRITICAL FAILURE] Bridge Error: {e}")
    sys.exit(1)

# Open CSV and write the exact headers Edge Impulse demands
with open(CSV_FILENAME, mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["timestamp", "challenge", "ver_ticks"])  # Mandatory Header
    
    sample_count = 0
    current_timestamp_ms = 0
    
    while sample_count < MAX_SAMPLES:
        if esp.in_waiting > 0:
            raw_payload = esp.readline().decode('utf-8', errors='ignore').strip()
            if not raw_payload:
                continue
                
            try:
                packet = json.loads(raw_payload)
                if "challenge" not in packet:
                    continue
                    
                challenge_val = packet["challenge"]
                
                # Pipe to Verilator
                verilator_sim.stdin.write(f"{challenge_val}\n")
                verilator_sim.stdin.flush()
                
                # Read Verilator Response
                sim_response = verilator_sim.stdout.readline().strip()
                v_data = json.loads(sim_response)
                
                ticks = v_data.get("ticks", 0)
                
                # Send back to esp-s3
                esp.write((sim_response + "\n").encode('utf-8'))
                
                # Log to CSV in Edge Impulse format
                writer.writerow([current_timestamp_ms, challenge_val, ticks])
                
                sample_count += 1
                current_timestamp_ms += 10  # Increment time by 10ms per sample
                
                if sample_count % 100 == 0:
                    print(f"[Harvest] Progress: {sample_count}/{MAX_SAMPLES} samples collected.")
                
            except json.JSONDecodeError:
                pass # Silently drop corrupted frames to keep CSV pure

print(f"[System] Harvest Complete. File saved as {CSV_FILENAME}.")
verilator_sim.terminate()