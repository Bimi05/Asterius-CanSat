#include "sensors.h"
#include "misc.h"

#define AUTOMATIC_DETACHMENT

#define SD_CHIP_SELECT 5
#define SERVO_PIN 3

#define BUZZER_PIN 4
#define FAIL_FREQ 250
#define SUCCESS_FREQ 3000
#define BEEP_DURATION 500

#define RFM_CHIP_SELECT 23
#define RFM_INTERRUPT 22
#define RFM_RESET 21
#define RFM_FREQUENCY 434.2F


Adafruit_BME680 BME688;
Adafruit_BNO08x BNO085;
Adafruit_GPS GPS(&Serial1);
RH_RF95 RFM(RFM_CHIP_SELECT, RFM_INTERRUPT);
Servo Motor;
File df;


// ----------- Data ----------- //
uint32_t bootTime;
bool detached;

char data[255];
uint8_t len = 0;

uint8_t phase = 0;
bool landed = false;
float lv = static_cast<float>(NAN);
float gpres = 0.0F; //* for altitude calculation


float temp = static_cast<float>(NAN);
float pres = static_cast<float>(NAN);
float hum = static_cast<float>(NAN);
float lat = static_cast<float>(NAN);
float lon = static_cast<float>(NAN);


sh2_SensorValue_t BNO_val;

float mag = static_cast<float>(NAN);
float grav = static_cast<float>(NAN);
// ---------------------------- //


// ------ Initialisation ------ //
bool BME_init() {
  if (!BME688.begin()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
    delay(BEEP_DURATION*2);
    return false;
  }

  for (uint8_t i=0; i<10; i++) {
    gpres += BME688.readPressure() / 100.0F;
  }
  gpres /= 10;

  tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  delay(BEEP_DURATION*2);

  return true;
}

bool BNO_init() {
  if (!BNO085.begin_I2C()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
    delay(BEEP_DURATION*2);
    return false;
  }

  BNO085.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
  BNO085.enableReport(SH2_GRAVITY);

  tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  delay(BEEP_DURATION*2);

  return true;
}

bool RFM_init() {
  if (!RFM.init()) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
    delay(BEEP_DURATION*2);
    return false;
  }

  RFM.setFrequency(RFM_FREQUENCY);

  tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  delay(BEEP_DURATION*2);

  return true;
}

bool GPS_init() {
  if (!GPS.begin(9600)) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
    delay(BEEP_DURATION*2);
    return false;
  }

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION);
  delay(BEEP_DURATION*2);

  return true;
}

bool SD_init() {
  if (!SD.begin(SD_CHIP_SELECT)) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION);
    delay(BEEP_DURATION*2);
    return false;
  }

  if (SD.exists("ASTERIUS.TXT")) {
    SD.remove("ASTERIUS.TXT");
  }

  df = SD.open("ASTERIUS.TXT", FILE_WRITE);

  uint16_t f = (df) ? SUCCESS_FREQ : FAIL_FREQ;
  tone(BUZZER_PIN, f, BEEP_DURATION);
  delay(BEEP_DURATION*2);

  return df;
}

// ---------------------------- //
bool initialiseSensors() {
  Motor.attach(SERVO_PIN);
  Motor.write(120); //? needs verification
  delay(1000);

  if (!(BME_init() && BNO_init() && GPS_init() && SD_init() && RFM_init() && Motor.attached())) {
    tone(BUZZER_PIN, FAIL_FREQ, BEEP_DURATION*2);
    delay(BEEP_DURATION*4);
    return false;
  }

  bootTime = millis();

  tone(BUZZER_PIN, SUCCESS_FREQ, BEEP_DURATION*2);
  delay(BEEP_DURATION*4);

  return true;
}
// ---------------------------- //


// -------- Operations -------- //
void BME_read() {
  if (BME688.performReading()) {
    temp = BME688.temperature;
    pres = BME688.pressure / 100.0F; //* hPa
    hum = BME688.humidity;
  }
}

void GPS_read() {
  while (!GPS.newNMEAreceived()) {
    GPS.read();
    yield();
  }

  if (!GPS.parse(GPS.lastNMEA())) {
    return;
  }

  //* this just processes our coordinates
  //* so that we can enter them on google maps
  //* and get an accurate result of our position

  GPS.latitude /= 100.0F;
  float lat_deg = floor(GPS.latitude);
  float lat_mins = ((GPS.latitude-lat_deg)*100)/60;
  lat_deg += lat_mins;

  GPS.longitude /= 100.0F;
  float lon_deg = floor(GPS.longitude);
  float lon_mins = ((GPS.longitude-lon_deg)*100)/60;
  lon_deg += lon_mins;


  lat = (GPS.lat == 'S') ? -(lat_deg) : lat_deg;
  lon = (GPS.lon == 'W') ? -(lon_deg) : lon_deg;
}

void BNO_read() {
  if (BNO085.getSensorEvent(&BNO_val)) {
    switch (BNO_val.sensorId) {
      case SH2_MAGNETIC_FIELD_CALIBRATED:
        {
          float mag_x = powf(BNO_val.un.magneticField.x, 2.0F);
          float mag_y = powf(BNO_val.un.magneticField.y, 2.0F);
          float mag_z = powf(BNO_val.un.magneticField.z, 2.0F);
          mag = sqrtf(mag_x + mag_y + mag_z); //* measurement unit: μT
        }
      case SH2_GRAVITY:
        {
          float grav_x = powf(BNO_val.un.gravity.x, 2.0F);
          float grav_y = powf(BNO_val.un.gravity.y, 2.0F);
          float grav_z = powf(BNO_val.un.gravity.z, 2.0F);
          grav = sqrtf(grav_x + grav_y + grav_z); //* measurement unit: m/s^2
        }
    }
  }
}
// ---------------------------- //

// ----- Helper Functions ----- //
char* process(uint8_t mode, char* data, uint8_t offset) {
  //* modes: 1 - encrypt / 2 - decrypt
  uint8_t mod = (mode == 1) ? 0 : 1;
  uint8_t c = 1;

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i]) || ((data[i] < 65 || data[i] > 90) && (data[i] < 97 || data[i] > 122))) {
      continue; //* too much; didn't understand: only process letters
    }

    uint8_t s = offset;
    if (c % 2 == mod) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    data[i] = (char)((int)(data[i] + s - value) % 26 + value);
    c++;
  }

  //! NOTE: this modifies the original string!
  return data;
}


uint8_t findPhase() {
  if (isnan(lv)) {
    lv = BME688.readAltitude(gpres);
    return 1;
  }

  if (landed) {
    return 4;
  }

  float cv = BME688.readAltitude(gpres);
  float diff = cv - lv;
  lv = cv;

  //? should be more than enough for when it's immobile
  if ((diff > -1) && (diff < 1)) {
    if (phase == 3) {
      landed = true;
      return 4;
    }
    return 1;
  }

  phase = (diff > 0) ? 2 : 3;

  //* Automatic Slave detachment
#ifdef AUTOMATIC_DETACHMENT
  if (phase == 3 && cv <= 700.0F) {
    detach();
  }
#endif

  return phase;
}


//* Here's a demonstration of how the data is distributed on a packet!
//* NOTE: measurement units are not included in the packet

/*
| Team Name | ID |  Time  | Temperature |   Pressure   | Humidity |   Latitude   |   Longitude   | Magnetic Field Intensity | Gravitational Field Intensity |
| Asterius: | 31 | 31.0 s |   26.79°C   |  999.70 hPa  |  45.09%  | 37.96683774N | 23.730371654E |        25.675 μT         |          9.985 m/s^2          |
*/

void updateSensorData(uint32_t ID) {
  BME_read();
  BNO_read();
  GPS_read();

  float time = (float) ((millis()-bootTime) / 1000.0F);
  len = snprintf(data, 255, "Asterius:%li %.01f %.02f %.02f %.02f %.06f %.06f %.03f %.03f [M]", ID, time, temp, pres, hum, lat, lon, mag, grav);
}

bool connect() {
  uint8_t buffer[255];
  uint8_t len = sizeof(buffer);
  
  if (!RFM.waitAvailableTimeout((5)*60*1000)) {
    return false;
  }

  if (!RFM.recv(buffer, &len)) {
    return false;
  }

  char* info = process(2, (char*) buffer, 1);
  if (strstr(info, "[S->M]") == NULL || strstr(info, "Pairing requested") == NULL) {
    return false;
  }

  char resp[] = "Asterius:Pairing success. Start transmitting data. [M->S]";
  uint8_t* resp_en = (uint8_t*) process(1, resp, 1);
  bool sent = RFM.send(resp_en, sizeof(resp_en));

  return sent;
}

bool saveData() {
  if (!df) {
    return false;
  }

  df.println(data);
  df.flush();
  return true;
}

void receive() {
  uint8_t S_packet[255];
  uint8_t S_len = sizeof(S_packet);

  if (!RFM.waitAvailableTimeout(300)) {
    return;
  }

  if (!RFM.recv(S_packet, &S_len)) {
    return;
  }

  if (strstr((char*) S_packet, "[S->M]") == NULL) {
    return;
  }

  size_t end = strlen((char*) S_packet);

  S_packet[end-4] = ']';
  S_packet[end-3] = '\0';

  RFM.send(S_packet, len);
}

bool sendData() {
  char* packet = process(1, data, 1);
  bool sent = RFM.send((uint8_t*) packet, len);
  RFM.waitPacketSent();
  return sent;
}

void detach() {
  if (detached) {
    return;
  }

  Motor.write(50); //? needs verification
  delay(1000);
  Motor.detach();
  detached = true;
}

void listenForOrders() {
  uint8_t message[255];
  uint8_t len = sizeof(message);

  if (!RFM.waitAvailableTimeout((1)*60*1000)) {
    return;
  }

  if (!RFM.recv(message, &len)) {
    return;
  }

  char* order = process(2, (char*) message, 1);
  if (strstr(order, "[G->M]") != NULL) {
    if (strstr(order, "DETACH") != NULL) {
      detach();
    }
    //? more possible Ground Station orders here...
  }
}
// ---------------------------- //
