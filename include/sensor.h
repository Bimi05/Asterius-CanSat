#ifndef __SENSORS__
#define __SENSORS__

#include <Adafruit_BME680.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_GPS.h>
#include <RH_RF95.h>
#include <SD.h>
#include <Servo.h>

// -------------------------------------------------------- //
class BME680 {
  private:
    Adafruit_BME680 m_BME;

  public:
    BME680();
    ~BME680();

    float gpres = static_cast<float>(NAN);

    float temp = static_cast<float>(NAN);
    float pres = static_cast<float>(NAN);
    float hum = static_cast<float>(NAN);

    void init();
    void read();
};

class BNO085 {
  private:
    Adafruit_BNO08x m_BNO;
    sh2_SensorValue_t m_BNO_val;

  public:
    BNO085();
    ~BNO085();

    float mag = static_cast<float>(NAN);
    float grav = static_cast<float>(NAN);

    void init();
    void read();
};

class UltimateGPS {
  private:
    Adafruit_GPS m_GPS;

  public:
    UltimateGPS();
    ~UltimateGPS();

    float lat = static_cast<float>(NAN);
    float lon = static_cast<float>(NAN);

    void init();
    void read();
};

class RFM9x {
  private:
    RH_RF95 m_RFM;
    float freq = 0.0F;

  public:
    RFM9x(float);
    ~RFM9x();

    void init();
    void send(char*);
    void send(char*, uint8_t);
    void receive(char*, uint16_t);
};

class SDCardModule {
  private:
    File df;

  public:
    SDCardModule();
    ~SDCardModule();

    void save(char*);
};

class ServoMotor {
  private:
    Servo m_Motor;

  public:
    ServoMotor();
    ~ServoMotor();

    void init();
    void lock();
    void unlock();
};
// -------------------------------------------------------- //

#endif