# microgear-esp8266-rtos

NETPIE client library for ESP8266 RTOS SDK.

## Compatibility

We have tested this library and found it compatible with (but not limited to) the following hardware
- ESP8266-01, 07, 12E, 12F
- NodeMCU v1, v2, V3
- Espresso Lite v2.0

## Outgoing Network Port

Make sure ther following ports are allowed to connect from your network.
- Non-TLS mode : 8080 and 1883 (the library uses this mode by default)
- TLS mode : 8081 and 8883 (still under testing)

**Usage Example**

```C
#include "uart.h"
#include "WifiController.h"
#include "Microgear.h"

#define STA_SSID      <WIFI_SSID>
#define STA_PASSWORD  <WIFI_PASSWORD>

#define APPID         <APPID>
#define KEY           <KEY>
#define SECRET        <SECRET>
#define ALIAS         <ALIAS>

/*
// enable this to manually assign token
#define TOKEN         <TOKEN>
#define TOKENSECRET   <TOKENSECRET>
*/
Microgear mg;

struct station_config wificonfig = {
    .ssid     = STA_SSID,
    .password = STA_PASSWORD,    
};

void UART_SetBaudrate(uint8_t uart_no, uint32 baud_rate) {
    uart_div_modify(uart_no, UART_CLK_FREQ / baud_rate);
}

void onConnected(char *attribute, uint8_t* msg, uint16_t msglen) {
    os_printf("Connected with NETPIE...\n");    
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

    startWifi(&wificonfig);

    microgear_init(&mg, KEY, SECRET, ALIAS);

    #if defined(TOKEN)
        microgear_setToken(&mg, TOKEN, TOKENSECRET, NULL);
    #endif
    microgear_on(&mg, CONNECTED, onConnected);
    microgear_on(&mg, MESSAGE, onMsghandler);

    microgear_connect(&mg, APPID);

    xTaskCreate(loop_task, "loop", 256, NULL, tskIDLE_PRIORITY + 1 , NULL);
}

```
