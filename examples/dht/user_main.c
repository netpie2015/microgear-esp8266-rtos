#include "WifiController.h"
#include "Microgear.h"
#include "dht.h"

#define STA_SSID      <WIFI_SSID>
#define STA_PASSWORD  <WIFI_PASSWORD>

#define APPID         <APPID>
#define KEY           <KEY>
#define SECRET        <SECRET>
#define ALIAS         <ALIAS>

#define DHT_PIN       2
#define DHT_TYPE      DHT_TYPE_22;

Microgear mg;

struct station_config wificonfig = {
    .ssid     = STA_SSID,
    .password = STA_PASSWORD,    
};

void onConnected(char *attribute, uint8_t* msg, uint16_t msglen) {
    os_printf("Connected with NETPIE...\n");    
}

LOCAL void loop_task(void *pvParameters) {
    char str[13];
    while(1) {
    	dht_read_str_data(DHT_TYPE_22, DHT_PIN, str);
        if (microgear_isConnected(&mg)) {
           os_printf("published\n");
           microgear_publish(&mg,"/dht22", str, NULL);
        }
        else {
           os_printf("skip publishing\n");
        }
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void ICACHE_FLASH_ATTR user_init(void) {
    dht_init(DHT_PIN);

    // set UART0 buadrate to be 115200
    uart_div_modify(0, UART_CLK_FREQ / 115200); 
    startWifi(&wificonfig);

    microgear_init(&mg, KEY, SECRET, ALIAS);
    microgear_on(&mg, CONNECTED, onConnected);
    microgear_on(&mg, MESSAGE, onMsghandler);

    microgear_connect(&mg, APPID);

    xTaskCreate(loop_task, "loop", 256, NULL, tskIDLE_PRIORITY + 1 , NULL);
}
