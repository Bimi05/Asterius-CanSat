#include <Arduino.h>
#include <InternalTemperature.h>
#include <TeensyThreads.h>

#include "sensors.h"

#define BUZZER 4

void setup() {
  Serial.begin(115200);

  if (!initialiseSensors()) {
    Serial.println("Something went wrong with the initialisation. Check wirings and addresses.");
    return;
  }

  Serial.println("Sensors initialised. Beginning slave pairing...");

  uint32_t start = millis();
  uint8_t cons = 0;

  while (true) {
    //* realistically, connecting is just telling us that Master and Slave(s) can communicate
    if (connect()) {
      cons++;
      Serial.println("Slave found! Connection established.");
    }

    if (millis()-start >= 5*60*1000) { //* 5 minutes should be plenty
      break;
    }
  }

  if (cons == 0) {
    Serial.println("No connections have been made. Please ensure the Master and the Slaves are functional.");
    return;
  }

  Serial.println("Ready!");
}

uint32_t p_id = 1;
void loop() {
  updateSensorData(p_id);
  listenForOrders();

  float temp = InternalTemperature.readTemperatureC();
  Serial.print("[Debug] Teensy's internal temperature: ");
  Serial.print(temp, 1);
  Serial.println("°C");

  //! NOTE: saveData() will save ONLY the Master's data to the SD card.
  //! sendData() on the other hand, will send both the Master's AND the Slaves' data.

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
