# microgear-esp8266-rtos
NETPIE client library for ESP8266 RTOS SDK

```C
#include "uart.h"
#include "Microgear.h"

#define STA_SSID      <WIFI_SSID>
#define STA_PASSWORD  <WIFI_PASSWORD>

#define APPID         <APPID>
#define KEY           <KEY>
#define SECRET        <SECRET>
#define ALIAS         <ALIAS>

#define TOKEN         <TOKEN>
#define TOKENSECRET   <TOKENSECRET>

Microgear mg;
xSemaphoreHandle WifiReady;

LOCAL void ICACHE_FLASH_ATTR wifi_task(void *pvParameters) {
    uint8_t status;

    if (wifi_get_opmode() != STATION_MODE) {
        wifi_set_opmode(STATION_MODE);
        vTaskDelay(1000 / portTICK_RATE_MS);
        system_restart();
    }

    while (1) {
        os_printf("WiFi: Connecting to WiFi\n");
        wifi_station_connect();
        struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
        sprintf(config->ssid, STA_SSID);
        sprintf(config->password, STA_PASSWORD);
        wifi_station_set_config(config);
        free(config);
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
            xSemaphoreGive(WifiReady);
            //os_printf("WiFi: Alive\n");
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        os_printf("WiFi: Disconnected\n");
        wifi_station_disconnect();
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void UART_SetBaudrate(uint8_t uart_no, uint32 baud_rate) {
    uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}

void onConnected(char *attribute, uint8_t* msg, uint16_t msglen) {
    os_printf("Connected with NECTPIE...\n");    
    microgear_subscribe(&mg,"/#");
}

void onMsghandler(char *topic, uint8_t* msg, uint16_t msglen) {
    uint16_t i;
    os_printf("incoming message --> %s : ",topic);
    for (i=0;i<msglen;i++) {
        dmsg_putchar((char)(msg[i]));
    }
    os_printf("\n");    
}

LOCAL void loop_task(void *pvParameters) {
    while(1) {
        if (microgear_isconnected(&mg)) {
           os_printf("published\n");
           microgear_publish(&mg,"/test","Hello", NULL);
        }
        else {
           os_printf("skip publishing\n");
        }
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void ICACHE_FLASH_ATTR user_init(void) {
    UART_SetBaudrate(UART0, 115200);
    
    vSemaphoreCreateBinary(WifiReady);
    xSemaphoreTake(WifiReady, 0);              // take the default semaphore
    microgear_setWifiSemaphore(&WifiReady);

    microgear_init(&mg, KEY, SECRET, ALIAS);
    microgear_setToken(&mg, TOKEN, TOKENSECRET, NULL);
 
    microgear_on(&mg, CONNECTED, onConnected);
    microgear_on(&mg, MESSAGE, onMsghandler);

    microgear_connect(&mg, APPID);

    xTaskCreate(wifi_task, "wifi", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(loop_task, "loop", 256, NULL, tskIDLE_PRIORITY + 1 , NULL);
}

```
