#include <Esp.h>

#include "debug.hpp"
#include "rtc_mem.hpp"

// Environment already has a crc32 linked in, no need for our own
extern uint32_t crc32(const void* data, size_t length, uint32_t crc /*= 0xffffffff*/);

#define lcrc32(data, len) crc32(data, len, 0xffffffff)

// Could also be made a class, but since its currently basically a singleton namespace makes more sense
namespace rtcMem {
rtcData gRTC;
bool    loadedValidMem = false;

bool isValid() {
	return loadedValidMem;
};

bool read() {
	LOGFUNC("ReadRTC");
	if (ESP.rtcUserMemoryRead(0, (uint32_t*)&gRTC, sizeof(gRTC))) {
		// Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
		uint32_t crc = lcrc32(((uint8_t*)&gRTC) + 4, sizeof(gRTC) - 4);
		if (crc == gRTC.crc32) {
			if (MEM_VERSION == gRTC.version) {
				LOGF("Data in RTC valid: %x\n", gRTC.crc32);
				LOGF("Channel: %x\n", gRTC.channel);
				LOGF("BSSID: %x:%x:%x:%x:%x:%x\n", gRTC.bssid[0], gRTC.bssid[1], gRTC.bssid[2], gRTC.bssid[3], gRTC.bssid[4], gRTC.bssid[5]);
				loadedValidMem = true;
				return true;
			} else {
				LOGF("CRC ok, but version failed: Want %x have %x\n", MEM_VERSION, gRTC.version);
			}
		} else {
			LOGLN("Data in RTC invalid");
		}
	}
	loadedValidMem = false;
	memset(&gRTC, 0, sizeof(gRTC));
	return false;
};

bool write() {
	LOGFUNC("WriteRTC");
	gRTC.version = MEM_VERSION;
	gRTC.crc32   = lcrc32(((uint8_t*)&gRTC) + 4, sizeof(gRTC) - 4);
	return ESP.rtcUserMemoryWrite(0, (uint32_t*)&gRTC, sizeof(gRTC));
};
}   // namespace rtcMem