#include "sensors.h"
#include "misc.h"

#define SD_SPI_ADDR 5

#define RFM_CHIP_SELECT 23
#define RFM_INTERRUPT 22
#define RFM_RESET 21
#define RFM_FREQUENCY 0.0F


Adafruit_BME680 BME688;
Adafruit_BNO08x BNO085;
Adafruit_GPS GPS(&Serial1);
RH_RF95 RFM(RFM_CHIP_SELECT, RFM_INTERRUPT);
File df;


// ----------- Data ----------- //
uint32_t bootTime;

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
float temperature = static_cast<float>(NAN);
float pressure = static_cast<float>(NAN);
float humidity = static_cast<float>(NAN);
float lat = static_cast<float>(NAN);
float lon = static_cast<float>(NAN);
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
  if (!(BME_init() && BNO_init() && GPS_init() && SD_init() && RFM_init())) {
    return false;
  }

  bootTime = millis();
  return true;
}
// ---------------------------- //


// -------- Operations -------- //
void BME_read() {
  if (BME688.performReading()) {
    temperature = BME688.temperature;
    pressure = BME688.pressure / 100.0F; //* hPa
    humidity = BME688.humidity;
  }
}

void GPS_read() {
  //! people will say this is dangerous, but I'm taking the risk
  while (true) {
    GPS.read();
    if (GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())) {
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

      break;
    }
    yield();
  }
}

// ---------------------------- //

// ----- Helper Functions ----- //
char* process(uint8_t mode, char* data, uint8_t offset) {
  //* modes: 1 - encrypt / 2 - decrypt
  uint8_t mod = (mode == 1) ? 0 : 1;

  uint8_t counter = 1;
  char* message = (char*) malloc(sizeof(data));

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i])) {
      message[i] = ' ';
      continue;
    }

    if (((int) data[i] < 65 || (int) data[i] > 90) || ((int) data[i] < 97 || (int) data[i] > 122)) {
      continue; //* too much; didn't understand: only process letters
    }

    uint8_t s = offset;
    if (counter % 2 == mod) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    message[i] = (char)((int)(data[i] + s - value) % 26 + value);
    counter++;
  }

  //! NOTE: remember to free the memory!
  //! this returns a char pointer for temporary use.
  return message;
}

void updateSensorData(uint32_t ID) {
  memset(data, '-', 255);

  BME_read();
  GPS_read();

  float time = static_cast<float>((millis()-bootTime) / 1000.0F);
  len = snprintf(data, 255, "Asterius:%li %.01f %.02f %.02f %.02f %.06f %.06f [M]", ID, time, temperature, pressure, humidity, lat, lon);

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

  free(info);
  free(resp_en);

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
      char* buffer = (char*) malloc(strlen(m-3));
      memcpy(buffer, m, strlen(m-3));
      buffer[strlen(buffer-1)] = ']';
      char* packet = process(1, buffer, offset);
      free(buffer);
      RFM.send((uint8_t*) packet, sizeof(packet));
      free(packet);
    }
    free(m);
  }

  char* message = process(1, data, offset);
  bool sent = RFM.send((uint8_t*) message, len);
  free(message);

  return sent;
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
  return phase;
}
// ---------------------------- //
