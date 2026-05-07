#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SENSOR_PIN 5 // Connected to Attacker GPIO 4
TaskHandle_t SecurityMonitorTask;

void runSecurityMonitor(void * parameter) {
  for(;;) {
    int challenge = random(0, 255);
    bool is_attacked = digitalRead(SENSOR_PIN); // Read the attacker line

    StaticJsonDocument<128> doc_out;
    doc_out["challenge"] = challenge;
    
    // MVP Hardware Glitch Simulation
    // If attacked, we burn CPU cycles to artificially spike the HIL latency
    if (is_attacked) {
        delayMicroseconds(random(800, 2500)); 
    }

    unsigned long start_time = micros();
    serializeJson(doc_out, Serial);
    Serial.println(); 

    String incoming_data = "";
    while(micros() - start_time < 50000) {
      if(Serial.available()) {
        incoming_data = Serial.readStringUntil('\n');
        break;
      }
    }

    // OLED Update
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("AI Sentinel Active");
    display.print("Chal: "); display.println(challenge);
    
    if (is_attacked) {
        display.println("STATUS: [ GLITCH ]");
    } else {
        display.println("STATUS: [ SECURE ]");
    }
    display.display();

    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void setup() {
  Serial.begin(921600);
  while(!Serial); 
  
  pinMode(SENSOR_PIN, INPUT_PULLDOWN);

  // Initialize OLED (Check your board's SDA/SCL pins)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.display();

  xTaskCreatePinnedToCore(runSecurityMonitor, "SecTask", 8192, NULL, 24, &SecurityMonitorTask, 1);
}

void loop() { vTaskDelete(NULL); }