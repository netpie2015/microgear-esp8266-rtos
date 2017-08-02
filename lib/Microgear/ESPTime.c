#include "ESPTime.h"

#define TIME_STR_LENGTH 10

uint32_t ts_starttime;
uint32_t ts_localtime;

uint32_t secs(void) {
	uint32_t micros = system_get_time();
	return (micros / 1000000);
}

void ICACHE_FLASH_ATTR setTime(uint32_t ts) {
	ts_starttime = ts;
	ts_localtime = secs();
}

uint32_t ICACHE_FLASH_ATTR getTime() {
	return (secs() + ts_starttime - ts_localtime);
}

char* ICACHE_FLASH_ATTR getTimeStr() {
	char *p;
	uint32_t tm = getTime();
	static char timestr[TIME_STR_LENGTH];

	memset(timestr,0,TIME_STR_LENGTH+1);
	for (p = timestr+TIME_STR_LENGTH-1; p >= timestr; p--) {
		*p = '0' + tm % 10;
		tm /= 10;
		if (tm == 0) break;
	}
	return p;
}
