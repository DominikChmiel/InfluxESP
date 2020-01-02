#include <Arduino.h>

#include "debug.hpp"

unsigned lastMillis = 0;
uint32_t lastCycles = 0;

void init_debug() {
#ifdef DEBUG
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(2000);
	while (!Serial) {
		delay(50);
	}
	LOGLN("Booting Envsensor");
#endif
};

#ifdef DEBUG
void loginter(const char* name) {
	unsigned newMillis = millis();
	uint32_t cycles    = ESP.getCycleCount();
	if (lastMillis == 0) {
		lastMillis = newMillis;
		lastCycles = cycles;
	}
	LOGF(">>>TIME: %6d (+ %5d) | Cycle %14d (+ %11d) %s\n", newMillis, newMillis - lastMillis, cycles, cycles - lastCycles, name);
	// Reread out time in order to not include LOGF call
	lastMillis = millis();
	lastCycles = ESP.getCycleCount();
}
#endif