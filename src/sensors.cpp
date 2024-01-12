#include "sensors.h"
#include "misc.h"

#define BME_I2C_ADDR 0
#define BNO_I2C_ADDR 0
#define SD_SPI_ADDR 0

#define RFM_CHIP_SELECT 0
#define RFM_INTERRUPT 0
#define RFM_RESET 0
#define RFM_FREQUENCY 0.0


Adafruit_BME680 BME688 = Adafruit_BME680();
Adafruit_BNO08x BNO085 = Adafruit_BNO08x();
Adafruit_GPS GPS = Adafruit_GPS(&Serial1);
RH_RF95 RFM = RH_RF95(RFM_CHIP_SELECT, RFM_INTERRUPT);
File DataFile;


// ----------- Data ----------- //
char data[255];
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
  if (!BME688.begin(BME_I2C_ADDR)) {
    Debug("Unable to initialise the BME688 sensor.");
    return false;
  }
  return true;
}

bool BNO_init() {
  if (!BNO085.begin_I2C(BNO_I2C_ADDR)) {
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

  DataFile = SD.open("ASTERIUS_DATA.txt");
  DataFile.seek(0); //! this might cause an issue (if it doesn't overwrite ALL the content, i.e. in a Ctrl+C)

  return true;
}
// ---------------------------- //
bool initialiseSensors() {
  if (!(BME_init() && BNO_init() && RFM_init() && GPS_init() && SD_init())) {
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
    pressure = BME688.pressure;
    humidity = BME688.humidity;
  }
  yield();
}

void GPS_read() {
  uint32_t GPS_timeout = millis();

  while (!GPS.newNMEAreceived()) {
    GPS.read();
    if (millis()-GPS_timeout >= 500) { // TODO: implement a FUNCTIONAL time-out system :)
      break;
    }
    yield();
  }

  if (GPS.parse(GPS.lastNMEA())) {
    lat = (GPS.lat == 'S') ? -(GPS.latitude) : GPS.latitude;
    lon = (GPS.lon == 'W') ? -(GPS.longitude) : GPS.longitude;
  }
}

void BNO_read() {
  // TODO: work in progress :)
  if (BNO085.getSensorEvent(&BNO_val)) {
    if (BNO_val.sensorId == SH2_GEOMAGNETIC_ROTATION_VECTOR) {
      return;
    }
  }
}

// ---------------------------- //
void updateSensorData(uint32_t ID) {
  BME_read();
  GPS_read();
  // BNO_read();

  float time = static_cast<float>((millis()-boot) / 1000); //? can be slightly "inaccurate"
  len = snprintf(data, 255, "Asterius:%li %.01f | %.02f %.02f %.02f %.02f %.02f", ID, time, temperature, pressure, humidity, lat, lon);
}

bool saveData() {
  //? somebody riddle me why the fuck I thought this was a necessary idea

  if (!DataFile) {
    return false;
  }

  DataFile.println(data);
  DataFile.flush();

  return true;
}

bool sendData() {
  //! NOTE: For minions, we will be calling a recv() with a timeout of ~300-500ms
  //! if everything is received according to plan, compile and send
  //! else send what we currently have (possibly report missing data?)

  // TODO: encrypt "data" here and then send them over to the ground station :)
  return RFM.send((uint8_t*) data, len);
}
// ---------------------------- //
