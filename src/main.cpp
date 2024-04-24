#include <Arduino.h>
#include <InternalTemperature.h>
#include <TeensyThreads.h>

#include "sensors.h"

#define BUZZER 4
#define FAIL_FREQ 250
#define SUCCESS_FREQ 3000
#define BEEP_DURATION 500

void setup() {
  Serial.begin(115200);
  initialiseSensors();

  uint32_t start = millis();
  uint8_t cons = 0;

  while ((millis() - start) < (5)*60*1000) { //* 5 minutes should be plenty
    if (connect()) {
      cons++;
      tone(BUZZER, SUCCESS_FREQ, BEEP_DURATION);
    }
  }

  if (cons == 0) {
    tone(BUZZER, FAIL_FREQ, BEEP_DURATION);
  }

  threads.addThread(listenForOrders);
  threads.addThread(receive);

  tone(BUZZER, SUCCESS_FREQ, BEEP_DURATION*2);
  delay(BEEP_DURATION*4);
}

uint32_t p_id = 1;
void loop() {
  updateSensorData(p_id);

  //! NOTE: saveData() will save ONLY the Master's data to the SD card.
  //! sendData() on the other hand, will send both the Master's AND the Slaves' data.

  saveData();
  sendData();

  if (findPhase() == 4) {
    static uint32_t lastBeat = millis();
    if (millis() - lastBeat >= 2000) {
      lastBeat = millis();
      tone(BUZZER, 4000, (3/2)*BEEP_DURATION); //* 4000 Hz is pretty sharp and easy to hear
    }
  }

  p_id++;
}
