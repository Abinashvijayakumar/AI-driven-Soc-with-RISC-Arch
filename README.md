# AI-driven-Soc-with-RISC-Arch
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

attacker
