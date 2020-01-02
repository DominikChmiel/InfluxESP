#pragma once
#include "config.hpp"

#ifdef DEBUG
#define LOG Serial.print
#define LOGF Serial.printf
#define LOGLN Serial.println
#define LOGINTER loginter
void loginter(const char* name);
#else
#define LOG(x)     \
	while (0) {    \
		(void)(x); \
	}
#define LOGF(x, ...) \
	while (0) {      \
		(void)(x);   \
	}
#define LOGLN(x, ...) \
	while (0) {       \
		(void)(x);    \
	}
#define LOGINTER(x) \
	while (0) {     \
		(void)(x);  \
	}
#endif

class LOGFUNC {
public:
	explicit LOGFUNC(const char* name) : m_name(name) {
		loginter(name);
	};
	~LOGFUNC() {
		loginter(m_name);
	}

private:
	const char* m_name;
};

void init_debug();