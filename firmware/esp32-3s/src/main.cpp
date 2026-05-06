#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED Configuration ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
// Most standard 0.96" OLEDs use I2C address 0x3C
#define SCREEN_ADDRESS 0x3C 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- AMP Shared State Variables ---
// 'volatile' tells the compiler these will be accessed by multiple cores simultaneously
volatile int shared_challenge = 0;
volatile int shared_ticks = 0;
volatile unsigned long shared_latency = 0;

TaskHandle_t SecurityMonitorTask;
TaskHandle_t DisplayTask;

// ==========================================
// CORE 1: HIGH-SPEED SECURITY SENTINEL
// ==========================================
void runSecurityMonitor(void * parameter) {
  for(;;) {
    int challenge = random(0, 255);
    StaticJsonDocument<128> doc_out;
    doc_out["challenge"] = challenge;
    
    unsigned long start_time = micros();
    
    // Send JSON to Pop!_OS Bridge
    serializeJson(doc_out, Serial);
    Serial.println(); 

    String incoming_data = "";
    
    // 50ms Timeout
    while(micros() - start_time < 50000) {
      if(Serial.available()) {
        incoming_data = Serial.readStringUntil('\n');
        break;
      }
    }

    unsigned long latency = micros() - start_time;

    // Parse incoming data and update Shared State for Core 0
    if(incoming_data.length() > 0) {
      StaticJsonDocument<256> doc_in;
      DeserializationError error = deserializeJson(doc_in, incoming_data);
      if(!error) {
        shared_challenge = challenge;
        shared_ticks = doc_in["ticks"];
        shared_latency = latency;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

// ==========================================
// CORE 0: ASYNCHRONOUS UI DISPLAY
// ==========================================
void runDisplayTask(void * parameter) {
  // Initialize I2C Display (ESP-S3 default: SDA=8, SCL=9. Adjust if needed)
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // If screen fails, delete this task to prevent crashing the OS
    vTaskDelete(NULL); 
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  for(;;) {
    display.clearDisplay();
    
    // 1. Draw Title
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("AI SEC SENTINEL");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    // 2. Draw Live Data from Core 1
    display.setCursor(0, 15);
    display.printf("Chal : %d\n", shared_challenge);
    display.printf("Ticks: %d\n", shared_ticks);
    display.printf("Lat  : %lu us\n", shared_latency);

    // 3. Draw "Waveform" / Anomaly Bar
    // If ticks deviate from the expected '5', draw a warning bar
    display.drawLine(0, 45, 128, 45, SSD1306_WHITE);
    display.setCursor(0, 50);
    if(shared_ticks != 5 && shared_ticks != 0) {
       display.setTextSize(1);
       display.print("[!] GLITCH DETECTED");
       display.fillRect(0, 60, 128, 4, SSD1306_WHITE); // Flash a block
    } else {
       display.print("[*] HW SECURE");
    }

    display.display(); // Push buffer to screen
    vTaskDelay(pdMS_TO_TICKS(100)); // Refresh rate ~10 FPS
  }
}

// ==========================================
// SYSTEM SETUP
// ==========================================
void setup() {
  Serial.begin(921600);
  while(!Serial); 

  // Pin Security to Core 1 (High Priority)
  xTaskCreatePinnedToCore(
    runSecurityMonitor, "SecurityTask", 4096, NULL, 24, &SecurityMonitorTask, 1
  );

  // Pin UI to Core 0 (Low Priority)
  xTaskCreatePinnedToCore(
    runDisplayTask, "DisplayTask", 4096, NULL, 1, &DisplayTask, 0
  );
}

void loop() {
  vTaskDelete(NULL); 
}