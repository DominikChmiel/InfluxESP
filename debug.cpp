#include <Arduino.h>

#include "debug.hpp"

unsigned lastMillis = 0;

void initDebug() {
#ifdef DEBUG
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(2000);
	while(!Serial) {
		delay(50);
	}
	LOGLN("Booting Envsensor");
#endif
};

void LOGINTER(const char* name) {
	unsigned newMillis = millis();
	if (lastMillis == 0) {
		lastMillis = newMillis;
	}
	LOGF(">>>TIME: %6d (+ %5d) %s\n", newMillis, newMillis - lastMillis, name);
	lastMillis = newMillis;
}