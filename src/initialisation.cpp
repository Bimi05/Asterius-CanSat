#include "sensor.h"

#define SD_CHIP_SELECT 5
#define SERVO_PIN 3

#define BUZZER_PIN 4
#define FAIL_FREQ 250
#define SUCCESS_FREQ 3000
#define BEEP_DURATION 500

// ---------------------------- //
BME680::BME680() {
  if (!m_BME.begin()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
  }
  else {
    tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}

BME680::~BME680() {}

void BME680::init() {
  for (uint8_t i=0; i<10; i++) {
    gpres += m_BME.readPressure() / 100.0F;
  }
  gpres /= 10;
}
// ---------------------------- //

// ---------------------------- //
BNO085::BNO085() {
  if (!m_BNO.begin_I2C()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
  }
  else {
    tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}

BNO085::~BNO085() {
  m_BNO.~Adafruit_BNO08x();
}

void BNO085::init() {
  m_BNO.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
  m_BNO.enableReport(SH2_GRAVITY);
}
// ---------------------------- //

// ---------------------------- //
UltimateGPS::UltimateGPS() {
  if (!m_GPS.begin(9600)) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
  }
  else {
    tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}

UltimateGPS::~UltimateGPS() {
  m_GPS.~Adafruit_GPS();
}

void UltimateGPS::init() {
  m_GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  m_GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
}
// ---------------------------- //

// ---------------------------- //
RFM9x::RFM9x(float frequency) {
  freq = frequency;
  if (!m_RFM.init()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
  }
  else {
    tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}

RFM9x::~RFM9x() {
  m_RFM.setModeIdle();
}

void RFM9x::init() {
  m_RFM.setFrequency(freq);
  m_RFM.setModeIdle();
}
// ---------------------------- //

// ---------------------------- //
SDCardModule::SDCardModule() {
  if (!SD.begin(SD_CHIP_SELECT)) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
  }
  else {
    tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  }
  delay(BEEP_DURATION*2);
}

SDCardModule::~SDCardModule() {
  df.close();
}
// ---------------------------- //

// ---------------------------- //
ServoMotor::ServoMotor() {
  m_Motor.attach(SERVO_PIN);
}

ServoMotor::~ServoMotor() {
  m_Motor.detach();
}

void ServoMotor::init() {
  this->lock();
}
// ---------------------------- //