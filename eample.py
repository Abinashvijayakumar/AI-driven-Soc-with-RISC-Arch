#include <Arduino.h>
#include <ArduinoJson.h>

from turtle import setup


TaskHandle_t SecurityMonitorTask;

void runSecurityMonitor(void * parameter) {
  for(;;) {
    int challenge = random(0, 255);
    
    StaticJsonDocument<128> doc_out;
    doc_out["challenge"] = challenge;
    
    unsigned long start_time = micros();
    
    // Send pure JSON to the bridge
    serializeJson(doc_out, Serial);
    Serial.println(); 

    String incoming_data = "";
    
    // 50ms Timeout Window
    while(micros() - start_time < 50000) {
      if(Serial.available()) {
        incoming_data = Serial.readStringUntil('\n');
        break;
      }
    }

    // We decode the incoming data to clear the buffer, 
    // but we DO NOT print it to Serial, keeping the pipe clean.
    if(incoming_data.length() > 0) {
      StaticJsonDocument<256> doc_in;
      DeserializationError error = deserializeJson(doc_in, incoming_data);
      if(error) {
        // Silently handle error to prevent polluting the UART pipe
      }
    }

    // 100ms interval between challenges
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void setup() {
  // Ensure this matches the Python bridge exactly
  Serial.begin(921600);
  while(!Serial); 

  xTaskCreatePinnedToCore(
    runSecurityMonitor, "SecurityTask", 4096, NULL, 24, &SecurityMonitorTask, 1
  );
}

void loop() {
  vTaskDelete(NULL); 
}





// Attacker ESP32 (COM Port A)
#include <Arduino.h>

#define ATTACK_PIN 4

void setup() {
  pinMode(ATTACK_PIN, OUTPUT);
  digitalWrite(ATTACK_PIN, LOW);
  randomSeed(analogRead(0));
}

void loop() {
  // Wait a random amount of time (1 to 5 seconds)
  delay(random(1000, 5000)); 
  
  // Fire the Glitch Attack (Hold high for 50ms)
  digitalWrite(ATTACK_PIN, HIGH);
  delay(50);
  digitalWrite(ATTACK_PIN, LOW);
}