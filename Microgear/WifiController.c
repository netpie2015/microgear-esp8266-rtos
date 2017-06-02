#include "WifiController.h"

xSemaphoreHandle wifi_semaphore;

void ICACHE_FLASH_ATTR wifi_task(void *pvParameters) {
    uint8_t status;

    if (wifi_get_opmode() != STATION_MODE) {
        wifi_set_opmode(STATION_MODE);
        vTaskDelay(1000 / portTICK_RATE_MS);
        system_restart();
    }

    while (1) {
        os_printf("WiFi: Connecting to WiFi\n");
        wifi_station_connect();

        wifi_station_set_config((struct station_config *)pvParameters);

        status = wifi_station_get_connect_status();
        int8_t retries = 30;
        while ((status != STATION_GOT_IP) && (retries > 0)) {
            status = wifi_station_get_connect_status();
            if (status == STATION_WRONG_PASSWORD) {
                os_printf("WiFi: Wrong password\n");
                break;
            }
            else if (status == STATION_NO_AP_FOUND) {
                os_printf("WiFi: AP not found\n");
                break;
            }
            else if (status == STATION_CONNECT_FAIL) {
                os_printf("WiFi: Connection failed\n");
                break;
            }
            vTaskDelay(1000 / portTICK_RATE_MS);
            --retries;
        }
        if (status == STATION_GOT_IP) {
            os_printf("WiFi: Connected\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        while ((status = wifi_station_get_connect_status()) == STATION_GOT_IP) {
            xSemaphoreGive(wifi_semaphore);
            os_printf("WiFi: Alive\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        os_printf("WiFi: Disconnected\n");
        wifi_station_disconnect();
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

int startWifi(struct station_config* wificonfig) {
    vSemaphoreCreateBinary(wifi_semaphore);
    xSemaphoreTake(wifi_semaphore, 0);              // take the default semaphore
    xTaskCreate(wifi_task, "wifi", 256, wificonfig, tskIDLE_PRIORITY + 1, NULL);
}
