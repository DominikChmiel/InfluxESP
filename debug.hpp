#pragma once
#include "config.hpp"

#ifdef DEBUG
#define LOG Serial.print
#define LOGF Serial.printf
#define LOGLN Serial.println

void LOGINTER(const char* name);
#else
#define LOG(x) while(0) { (void)(x); }
#define LOGF(x, ...) while(0) { (void)(x); }
#define LOGLN(x, ...) while(0) { (void)(x); }
#define LOGINTER(x) while(0) { (void)(x); }
#endif

class LOGFUNC {
public:
	LOGFUNC(const char* name) : m_name(name) {
		LOGINTER(name);
	};
	~LOGFUNC() {
		LOGINTER(m_name);
	}
private:
	const char* m_name;
};

void initDebug();