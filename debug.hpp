#pragma once
#include "config.hpp"


#ifdef DEBUG
#define DEBUG Serial.print
#define DEBUGF Serial.printf
#define DEBUGLN Serial.println
#else
#define DEBUG(x) while(0) { (void)(x); }
#define DEBUGF(x, ...) while(0) { (void)(x); }
#define DEBUGLN(x, ...) while(0) { (void)(x); }
#endif

void initDebug();