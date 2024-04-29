#include "sensor.h"

// ---------------------------- //
void BME680::read() {
  if (m_BME.performReading()) {
    temp = m_BME.temperature;
    pres = m_BME.pressure / 100.0F;
    hum = m_BME.humidity;
  }
}
// ---------------------------- //

// ---------------------------- //
void BNO085::read() {
  if (m_BNO.getSensorEvent(&m_BNO_val)) {
    switch (m_BNO_val.sensorId) {
      case SH2_MAGNETIC_FIELD_CALIBRATED:
        {
          float mag_x = powf(m_BNO_val.un.magneticField.x, 2.0F);
          float mag_y = powf(m_BNO_val.un.magneticField.y, 2.0F);
          float mag_z = powf(m_BNO_val.un.magneticField.z, 2.0F);
          mag = sqrtf(mag_x + mag_y + mag_z); //* measurement unit: μT
        }
      case SH2_GRAVITY:
        {
          float grav_x = powf(m_BNO_val.un.gravity.x, 2.0F);
          float grav_y = powf(m_BNO_val.un.gravity.y, 2.0F);
          float grav_z = powf(m_BNO_val.un.gravity.z, 2.0F);
          grav = sqrtf(grav_x + grav_y + grav_z); //* measurement unit: m/s^2
        }
    }
  }
}
// ---------------------------- //

// ---------------------------- //
void UltimateGPS::read() {
  while (!m_GPS.newNMEAreceived()) {
    m_GPS.read();
  }

  if (!m_GPS.parse(m_GPS.lastNMEA())) {
    return;
  }

  //* this just processes our coordinates
  //* so that we can enter them on google maps
  //* and get an accurate result of our position

  m_GPS.latitude /= 100.0F;
  float lat_deg = floor(m_GPS.latitude);
  float lat_mins = ((m_GPS.latitude-lat_deg)*100)/60;
  lat_deg += lat_mins;

  m_GPS.longitude /= 100.0F;
  float lon_deg = floor(m_GPS.longitude);
  float lon_mins = ((m_GPS.longitude-lon_deg)*100)/60;
  lon_deg += lon_mins;


  lat = (m_GPS.lat == 'S') ? -(lat_deg) : lat_deg;
  lon = (m_GPS.lon == 'W') ? -(lon_deg) : lon_deg;
}
// ---------------------------- //

// ---------------------------- //
void RFM9x::send(char* message) {
  m_RFM.setModeTx(); //* enable transmitter mode if it isn't on already
  m_RFM.send((uint8_t*) message, strlen(message));
  m_RFM.waitPacketSent();
}

void RFM9x::send(char* message, uint8_t len) {
  m_RFM.setModeTx();
  m_RFM.send((uint8_t*) message, len);
  m_RFM.waitPacketSent();
}

void RFM9x::receive(char* message, uint16_t timeout) {
  uint8_t payload[250];
  uint8_t len = sizeof(payload);

  m_RFM.setModeRx();
  if (!m_RFM.waitAvailableTimeout(timeout)) {
    return;
  }

  if (!m_RFM.recv(payload, &len)) {
    return;
  }

  memcpy(message, payload, len);
}
// ---------------------------- //

// ---------------------------- //
void SDCardModule::save(char* message) {
  df = SD.open("ASTERIUS.TXT", FILE_WRITE);
  if (df) {
    df.println(message);
    df.flush();
  }
  df.close();
}
// ---------------------------- //

// ---------------------------- //
void ServoMotor::lock() {
  int lockDeg = 137;
  m_Motor.write(lockDeg);
  delay(1000);
}

void ServoMotor::unlock() {
  int unlockDeg = 68;
  m_Motor.write(unlockDeg);
  delay(1000);
}
// ---------------------------- //