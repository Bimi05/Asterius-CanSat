#include <Adafruit_BME680.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>
#include <SD.h>
#include <Servo.h>


bool initialiseSensors();
void updateSensorData(uint32_t ID);

bool connect();
void listenForOrders();
void receive();

bool saveData();
bool sendData();

uint8_t findPhase();
