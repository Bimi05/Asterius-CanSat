#include <Arduino.h>

#include "sensors.h"

void setup() {
  Serial.begin(115200);
  if (!initialiseSensors()) {
    Serial.println("Something went wrong with the initialisation. Check wirings and addresses.");
    return;
  }
  Serial.println("Ready!");
}

void loop() {
  uint32_t p_id = 1;

  updateSensorData(p_id);

  //* NOTE: saveData() will save ONLY the main body's data to its SD card.
  saveData();

  //* NOTE: sendData() will send every piece of data (main & minion(s)).
  sendData();

  p_id++;
}
