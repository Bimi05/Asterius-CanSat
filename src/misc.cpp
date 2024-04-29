#include "misc.h"

uint8_t phase = 0;
bool landed = false;
float lastAlt = static_cast<float>(NAN);

void Debug(const char* message) {
  Serial.print("[Debug]: ");
  Serial.println(message);
  Serial.flush();
}

void Error(const char* message) {
  Serial.print("ERROR: ");
  Serial.println(message);
  Serial.flush();
}


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

uint8_t findPhase(float altitude) {
  if (isnan(lastAlt)) {
    lastAlt = altitude;
    return 1;
  }

  float diff = altitude - lastAlt;
  lastAlt = altitude;

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