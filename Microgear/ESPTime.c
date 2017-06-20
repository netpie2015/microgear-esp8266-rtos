#include "ESPTime.h"

uint32_t ts_starttime;
uint32_t ts_localtime;

uint32_t secs(void) {
	uint32_t micros = system_get_time();
	return (micros / 1000000);
}

void setTime(uint32_t ts) {
	ts_starttime = ts;
	ts_localtime = secs();
}

uint32_t getTime() {
	return (secs() + ts_starttime - ts_localtime);
}
