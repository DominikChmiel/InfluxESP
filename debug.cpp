#include <Arduino.h>

#include "debug.hpp"

void initDebug() {
#ifdef DEBUG
	Serial.begin(DEBUG_BAUDRATE);
	Serial.setTimeout(2000);
	while(!Serial) {
		delay(50);
	}
	DEBUGLN("Booting Envsensor");
#endif
};