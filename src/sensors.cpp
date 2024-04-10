#include "sensors.h"
#include "misc.h"

#define SD_SPI_ADDR 5
#define SERVO_PIN 0

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
sh2_SensorValue_t BNO_val;

uint8_t phase = 0;
bool landed = false;
float lv = static_cast<float>(NAN);
float gpres = 0.0F; //* for altitude calculation


//? for easy copy (shift + alt + down/up arrow)
//? please don't hate on me I'm lazy

// float x = static_cast<float>(NAN);
float temp = static_cast<float>(NAN);
float pres = static_cast<float>(NAN);
float hum = static_cast<float>(NAN);
float lat = static_cast<float>(NAN);
float lon = static_cast<float>(NAN);


float mx = static_cast<float>(NAN);
float my = static_cast<float>(NAN);
float mz = static_cast<float>(NAN);

float gx = static_cast<float>(NAN);
float gy = static_cast<float>(NAN);
float gz = static_cast<float>(NAN);

float grr = static_cast<float>(NAN);
float gri = static_cast<float>(NAN);
float grj = static_cast<float>(NAN);
float grk = static_cast<float>(NAN);

// ---------------------------- //


// ------ Initialisation ------ //
bool BME_init() {
  if (!BME688.begin()) {
    Debug("Unable to initialise the BME688 sensor.");
    return false;
  }

  for (uint8_t i=0; i<10; i++) {
    gpres += BME688.readPressure() / 100.0F;
  }
  gpres /= 10;

  return true;
}

bool BNO_init() {
  if (!BNO085.begin_I2C()) {
    Debug("Unable to initialise the BNO085 sensor.");
    return false;
  }

  BNO085.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
  BNO085.enableReport(SH2_GRAVITY);
  BNO085.enableReport(SH2_GEOMAGNETIC_ROTATION_VECTOR);

  return true;
}

bool RFM_init() {
  if (!RFM.init()) {
    Debug("Unable to initialise the RFM transmitter.");
    return false;
  }

  RFM.setFrequency(RFM_FREQUENCY);
  return true;
}

bool GPS_init() {
  if (!GPS.begin(9600)) {
    Debug("Unable to initialise the GPS.");
    return false;
  }

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  return true;
}

bool SD_init() {
  if (!SD.begin(SD_SPI_ADDR)) { 
    Debug("Unable to initialise the SD card.");
    return false;
  }

  if (SD.exists("ASTERIUS_DATA.txt")) {
    SD.remove("ASTERIUS_DATA.txt");
  }

  df = SD.open("ASTERIUS_DATA.txt");
  df.seek(0); //! this might cause an issue (if it doesn't overwrite ALL the content, i.e. in a Ctrl+C)

  return true;
}

// ---------------------------- //
bool initialiseSensors() {
  Motor.attach(SERVO_PIN);
  if (!(BME_init() && BNO_init() && GPS_init() && SD_init() && RFM_init() && Motor.attached())) {
    return false;
  }

  bootTime = millis();
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
  //! people will say this is dangerous, but I'm taking the risk
  while (true) {
    GPS.read();
    if (GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())) {
      break;
    }
    yield();
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
        mx = BNO_val.un.magneticField.x;
        my = BNO_val.un.magneticField.y;
        mz = BNO_val.un.magneticField.z;
      case SH2_GRAVITY:
        gx = BNO_val.un.gravity.x;
        gy = BNO_val.un.gravity.y;
        gz = BNO_val.un.gravity.z;
      case SH2_GEOMAGNETIC_ROTATION_VECTOR:
        grr = BNO_val.un.geoMagRotationVector.real;
        gri = BNO_val.un.geoMagRotationVector.i;
        grj = BNO_val.un.geoMagRotationVector.j;
        grk = BNO_val.un.geoMagRotationVector.k;
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


void updateSensorData(uint32_t ID) {
  memset(data, '-', 255);

  BME_read();
  GPS_read();
  BNO_read();

  float time = static_cast<float>((millis()-bootTime) / 1000.0F);
  len = snprintf(data, 255, "Asterius:%li %.01f %.02f %.02f %.02f %.06f %.06f [%.02f,%.02f,%.02f] [%.02f,%.02f,%.02f] [%.02f,%.02f,%.02f,%.02f] [M]", ID, time, temp, pres, hum, lat, lon, mx, my, mz, gx, gy, gz, grr, gri, grj, grk);

  Debug(data);
}

bool connect() {
  uint8_t buffer[255];
  uint8_t len = sizeof(buffer);

  if (!RFM.recv(buffer, &len)) {
    return false;
  }

  char* info = process(2, (char*) buffer, 1);
  if (strstr(info, "[S->M]") == NULL) {
    return false; //* got a message, wasn't ours :(
  }

  char resp[] = "Asterius:Pairing Success. Start transmitting data. [M->S]";
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

bool sendData(uint8_t offset) {
  uint8_t S_packet[255];
  uint8_t SP_len = sizeof(S_packet);

  if (RFM.recv(S_packet, &SP_len)) {
    char* m = process(2, (char*) S_packet, offset);
    if (strstr(m, "[S->M]") != NULL) {
      int end = (int) strlen(m);

      m[end-4] = ']';
      m[end-3] = '\0';

      RFM.send((uint8_t*) m, sizeof(m));
    }
  }

  char* packet = process(1, data, offset);
  bool sent = RFM.send((uint8_t*) packet, sizeof(packet));

  return sent;
}

void detach() {
  if (detached) {
    return;
  }

  Motor.write(90);
  delay(1000);
  yield();
  Motor.write(0);
  detached = true;
}

void listenForOrders() {
  uint8_t message[255];
  uint8_t len = sizeof(message);
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
