import serial
import subprocess
import json
import sys
import random
import csv
import time

ESP_PORT = '/dev/ttyACM0' 
BAUD_RATE = 921600
SAMPLES_NEEDED = 1000 # We will collect 1000 data points

print("[System] Initializing Data Harvesting Pipeline...")

try:
    esp = serial.Serial(ESP_PORT, BAUD_RATE, timeout=1)
    verilator_sim = subprocess.Popen(
        ['./obj_dir/Vsecurity_puf'],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True
    )
except Exception as e:
    print(f"[CRITICAL FAILURE] {e}")
    sys.exit(1)

# Open CSV for ML Training
with open('training_data.csv', mode='w', newline='') as file:
    writer = csv.writer(file)
    # CSV Header: Features (Challenge, Ticks) -> Label (Is_Glitch)
    writer.writerow(['challenge', 'ticks', 'is_glitch'])
    
    samples_collected = 0
    print(f"[System] Harvesting {SAMPLES_NEEDED} samples. Please wait...")

    while samples_collected < SAMPLES_NEEDED:
        if esp.in_waiting > 0:
            raw_payload = esp.readline().decode('utf-8', errors='ignore').strip()
            if not raw_payload: continue
                
            try:
                packet = json.loads(raw_payload)
                if "challenge" not in packet: continue
                challenge_val = packet["challenge"]
                
                # Ping Verilator
                verilator_sim.stdin.write(f"{challenge_val}\n")
                verilator_sim.stdin.flush()
                sim_response = verilator_sim.stdout.readline().strip()
                v_data = json.loads(sim_response)
                
                # SYNTHETIC GLITCH INJECTION (15% chance to simulate an attack)
                is_glitch = 0
                if random.random() < 0.15:
                    v_data["ticks"] += random.randint(2, 8) # Simulate timing delay from voltage drop
                    is_glitch = 1

                # Save to CSV
                writer.writerow([v_data["challenge"], v_data["ticks"], is_glitch])
                
                # Route back to ESP-S3
                out_payload = json.dumps(v_data) + "\n"
                esp.write(out_payload.encode('utf-8'))
                
                samples_collected += 1
                if samples_collected % 100 == 0:
                    print(f"[Harvesting] {samples_collected}/{SAMPLES_NEEDED} samples collected...")
                
            except json.JSONDecodeError:
                pass

print("[System] Harvesting Complete! Check 'training_data.csv'.")
verilator_sim.terminate()