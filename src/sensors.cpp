#include "sensors.h"
#include "misc.h"

#define SD_SPI_ADDR 5

#define RFM_CHIP_SELECT 23
#define RFM_INTERRUPT 22
#define RFM_RESET 21
#define RFM_FREQUENCY 0.0F


Adafruit_BME680 BME688 = Adafruit_BME680();
Adafruit_BNO08x BNO085 = Adafruit_BNO08x();
Adafruit_GPS GPS = Adafruit_GPS(&Serial1);
RH_RF95 RFM = RH_RF95(RFM_CHIP_SELECT, RFM_INTERRUPT);
File df;


// ----------- Data ----------- //
char data[255];
float gpres = 0.0F; //* for altitude calculation
uint8_t len = 0;
uint32_t boot;
sh2_SensorValue_t BNO_val;


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

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
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
  yield();
}

void GPS_read() {
  if (!GPS.fix) {
    return; //* just don't bother...
  }

  // TODO: implement a FUNCTIONAL time-out system :)
  // uint32_t GPS_timeout = millis();

  for (uint8_t i=0; i<3; i++) {
    do {
      GPS.read();
      yield();
      // if (millis()-GPS_timeout >= 500) {
      //   break;
      // }
    }
    while (!GPS.newNMEAreceived());
  }

  if (GPS.parse(GPS.lastNMEA())) {
    lat = (GPS.lat == 'S') ? -(GPS.latitude) : GPS.latitude;
    lon = (GPS.lon == 'W') ? -(GPS.longitude) : GPS.longitude;
  }
}
// ---------------------------- //

// ----- Helper Functions ----- //
void updateSensorData(uint32_t ID) {
  BME_read();
  GPS_read();

  float time = static_cast<float>((millis()-boot) / 1000.0F); //? can be slightly "inaccurate"
  len = snprintf(data, 255, "Asterius:%li %.01f | %.02f %.02f %.02f %.02f %.02f | [M]", ID, time, temperature, pressure, humidity, lat, lon);
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
// ---------------------------- //
