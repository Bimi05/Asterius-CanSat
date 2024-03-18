#include <Arduino.h>
#include <InternalTemperature.h>

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

  p_id++;
}
