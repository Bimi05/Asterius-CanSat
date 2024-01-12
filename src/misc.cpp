#include "misc.h"

void Debug(const char* message) {
  Serial.print("[Debug]: ");
  Serial.println(message);
  Serial.flush();
}
