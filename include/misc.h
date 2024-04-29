#ifndef __MISC__
#define __MISC__

#include <Arduino.h>

void Debug(const char*);
void Error(const char*);

char* process(uint8_t, char*, uint8_t);
uint8_t findPhase(float);

#endif