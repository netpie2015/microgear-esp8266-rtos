#ifndef WIFICONTROLLER_H
#define WIFICONTROLLER_H

#include <stdint.h>
#include <sys/types.h>

#include "esp_common.h"
#include "espressif/espconn.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

int startWifi(struct station_config*);
void ICACHE_FLASH_ATTR wifi_task(void*);

#endif
