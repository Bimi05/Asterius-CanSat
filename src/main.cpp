#include <Arduino.h>

#include "sensors.h"

void setup() {
  while (!Serial) {
    Serial.begin(9600);
    delay(100);
  }

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
