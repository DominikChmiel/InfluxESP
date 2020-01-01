#pragma once
#include <coredecls.h>

namespace rtcMem {
	#define MEM_VERSION 100
	typedef struct {
		uint32_t crc32;
		uint8_t version;
		uint8_t channel; 
		uint8_t bssid[6];
		uint32_t ip_addr;
		uint32_t gateway_addr;
		uint32_t netmask;
		uint32_t dns_addr;		
	} rtcData;

	extern rtcData gRTC;

	bool isValid();

	bool read();

	bool write();
}