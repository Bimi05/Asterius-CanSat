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
uint32_t boot;

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

  //* configure as necessary (this still needs stuff)
  BNO085.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED);
  BNO085.enableReport(SH2_GEOMAGNETIC_ROTATION_VECTOR);

  return true;
}

bool RFM_init() {
  if (!RFM.init()) {
    Debug("Unable to initialise the RFM transmitter.");
    return false;
  }

  RFM.setFrequency(RFM_FREQUENCY);
  RFM.setTxPower(20);

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

  boot = millis();
  return true;
}
// ---------------------------- //


// -------- Operations -------- //
void BME_read() {
  if (BME688.performReading()) {
    temperature = BME688.temperature;
    pressure = BME688.pressure / 100.0F;
    humidity = BME688.humidity;
  }
}

void GPS_read() {
  // TODO: implement a FUNCTIONAL time-out system :)
  // uint32_t GPS_timeout = millis();
  // if (millis()-GPS_timeout >= 500) {
  //   break;
  // }

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
void updateSensorData(uint32_t ID) {
  BME_read();
  GPS_read();

  float time = static_cast<float>((millis()-boot) / 1000.0F); //? can be slightly "inaccurate"
  len = snprintf(data, 255, "Asterius:%li %.01f | %.02f %.02f %.02f %.08f %.08f | [M]", ID, time, temperature, pressure, humidity, lat, lon);
  Debug(data);
}

bool saveData() {
  //? somebody riddle me why the fuck I thought this was a necessary idea

  if (!df) {
    return false;
  }

  df.println(data);
  df.flush();

  return true;
}

bool sendData(uint8_t offset) {
  //! NOTE: For minions, we will be calling a recv() with a timeout of ~300-500ms
  //! if everything is received according to plan, compile and send
  //! else send what we currently have (possibly report missing data?)

  uint8_t counter = 1;
  char* message = (char*) malloc(sizeof(data));

  for (uint8_t i=0; i < strlen(data); i++) {
    if (isspace(data[i])) {
      message[i] = ' ';
      continue;
    }

    if ((int(data[i]) < 65 || int(data[i]) > 90) || (int(data[i]) < 97 || int(data[i]) > 122)) {
      continue; //* basically: only decrypt letters pls
    }

    uint8_t s = offset;
    if (counter % 2 == 0) {
      s = (26-offset);
    }

    uint8_t value = 97; //* for lowercase letters
    if (isupper(data[i])) {
      value = 65;
    }

    //? I really hope no one asks me to explain this, cause it's... yes
    message[i] = char(int(data[i] + s - value)%26 + value);
    counter++;
  }

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
