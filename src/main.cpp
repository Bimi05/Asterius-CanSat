#include <Arduino.h>

#include "sensor.h"
#include "misc.h"

#define BUZZER 4
#define FAIL_FREQ 250
#define SUCCESS_FREQ 3000
#define BEEP_DURATION 500

#define RFM_FREQUENCY 433.2F


BME680 BME;
BNO085 BNO;
UltimateGPS GPS;
SDCardModule SDCard;
RFM9x RFM(RFM_FREQUENCY);
ServoMotor Motor;

uint32_t bootTime;
uint32_t p_id = 1;

bool detached = false;
uint32_t lastBeat = millis();


void setup() {
  Serial.begin(9600);

  BME.init();
  BNO.init();
  GPS.init();
  RFM.init();

  bootTime = millis();

  tone(BUZZER, SUCCESS_FREQ, BEEP_DURATION*2);
  delay(BEEP_DURATION*4);


  uint32_t start = millis();
  uint8_t cons = 0;

  while ((millis() - start) < (2)*60*1000) { //* 2 minutes should be plenty
    char received[250];

    RFM.receive(received, 45000);
    char* handshake = process(2, received, 1);

    if ((strstr(handshake, "[S->M]") != NULL) && (strstr(handshake, "Pairing requested") != NULL)) {
      char ack[] = "Asterius:Pairing Success. Start transmitting data. [M->S]";
      char* resp = process(1, ack, 1);

      RFM.send(resp);
      tone(BUZZER, 5000, BEEP_DURATION);
      cons++;
    }
  }

  if (cons == 0) {
    tone(BUZZER, FAIL_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}


void loop() {
  char data[250];

  BME.read();
  BNO.read();
  GPS.read();

  float time = (float) ((millis()-bootTime) / 1000.0F);
  uint8_t len = snprintf(data, 250, "Asterius:%li %.01f %.02f %.02f %.02f %.06f %.06f %.03f %.03f [M]", p_id, time, BME.temp, BME.pres, BME.hum, GPS.lat, GPS.lon, BNO.mag, BNO.grav);

  SDCard.save(data);

  char* packet = process(1, data, 1);
  RFM.send(packet, len);


  char received[250];

  RFM.receive(received, 30000);
  char* info = process(2, received, 1);

  // ------ Slave Reception ------ //
  if (strstr(info, "[S->M]") != NULL) {
    size_t end = strlen(info);
    info[end-4] = ']';
    info[end-3] = '\0';

    char* S_packet = process(1, info, 1);
    RFM.send(S_packet, end-3);
  }
  // ----------------------------- //

  // --- Ground Station Orders --- //
  if (strstr(info, "[G->M]") != NULL) {
    if (strstr(info, "DETACH") != NULL) {
      Motor.unlock();
      detached = true;
    }
  }
  // ----------------------------- //

  //! formula taken from BME's readAltitude(); function
  float altitude = 44330.0F * (1.0F - pow((BME.pres / BME.gpres), 0.1903F));
  uint8_t phase = findPhase(altitude);

  if (!detached && phase >= 3 && altitude <= 200.0F) {
    Motor.unlock();
    detached = true;
  }

  if (phase == 4) {
    if (millis() - lastBeat >= 2000) {
      lastBeat = millis();
      tone(BUZZER, 4000, (3/2)*BEEP_DURATION); //* 4000 Hz is pretty sharp and easy to hear
    }
  }

  p_id++;
}