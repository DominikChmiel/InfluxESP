#pragma once
#include <coredecls.h>

namespace rtcMem {
	typedef struct {
		uint32_t crc32;   // 4 bytes
		uint8_t channel;  // 1 byte,   5 in total
		uint8_t bssid[6]; // 6 bytes, 11 in total
		uint8_t last_error_code;  // 1 byte,  12 in total
		uint16_t fast_periods;  // 2 byte,  14 in total
		uint16_t mid_periods;  // 2 byte,  16 in total
		uint16_t slow_periods;  // 2 byte,  18 in total
		uint16_t padding;  // 2 byte,  20 in total
	} rtcData;

	extern rtcData gRTC;

	bool isValid();

	bool read();

	bool write();
}