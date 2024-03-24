#include <Arduino.h>
#include <InternalTemperature.h>

#include "sensors.h"

#define BUZZER 0

void setup() {
  Serial.begin(115200);

  if (!initialiseSensors()) {
    Serial.println("Something went wrong with the initialisation. Check wirings and addresses.");
    return;
  }
  Serial.println("Ready!");
}

uint32_t p_id = 1;
void loop() {
  updateSensorData(p_id);

  //? this wasn't my idea okay...
  float temp = InternalTemperature.readTemperatureC();
  Serial.print("[Debug] Teensy's internal temperature: ");
  Serial.print(temp, 1);
  Serial.println("°C");

  //* NOTE: saveData() will save ONLY the main body's data to its SD card.
  //* sendData() on the other hand, will send every piece of data (main & minion(s)).

  saveData();
  sendData(1);

  if (findPhase() == 4) {
    static uint32_t lastBeat = millis();
    if (millis() - lastBeat >= 2000) {
      lastBeat = millis();
      tone(BUZZER, 3000, 500);
    }
  }

  p_id++;
}
