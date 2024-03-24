#include <Adafruit_BME680.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>
#include <SD.h>

#define DEBUG //! comment out when not debugging (removes debug/info messages).


bool initialiseSensors();
void updateSensorData(uint32_t ID);

bool saveData();
bool sendData(uint8_t offset);

uint8_t findPhase();