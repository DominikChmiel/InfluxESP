#include <Esp.h>

#include "rtc_mem.hpp"
#include "debug.hpp"

// Environment already has a crc32 linked in, no need for our own
extern uint32_t crc32 (const void* data, size_t length, uint32_t crc /*= 0xffffffff*/);

#define lcrc32(data, len) crc32(data, len, 0xffffffff)

// Could also be made a class, but since its currently basically a singleton namespace makes more sense
namespace rtcMem {
	rtcData gRTC;
	bool loadedValidMem = false;

	bool isValid() {
		return loadedValidMem;
	};

	bool read() {
		if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&gRTC, sizeof( gRTC ) ) ) {
    		// Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
			uint32_t crc = lcrc32( ((uint8_t*)&gRTC) + 4, sizeof( gRTC ) - 4);
			if( crc == gRTC.crc32 ) {
				DEBUGF("Data in RTC valid: %x\n", gRTC.crc32);
				DEBUGF("Channel: %x\n", gRTC.channel);
				DEBUGF("BSSID: %x:%x:%x:%x:%x:%x\n", gRTC.bssid[0],gRTC.bssid[1],gRTC.bssid[2],gRTC.bssid[3],gRTC.bssid[4],gRTC.bssid[5]);
				loadedValidMem = true;
				return true;
			} else {
				DEBUGLN("Data in RTC invalid");
				gRTC.last_error_code = 0;
				gRTC.fast_periods = 0;
				gRTC.mid_periods = 0;
				gRTC.slow_periods = 0;
				loadedValidMem = false;
			}
		}
		return false;
	};

	bool write() {
		gRTC.crc32 = lcrc32( ((uint8_t*)&gRTC) + 4, sizeof( gRTC ) - 4 );
		return ESP.rtcUserMemoryWrite( 0, (uint32_t*)&gRTC, sizeof( gRTC ) );
	};
}