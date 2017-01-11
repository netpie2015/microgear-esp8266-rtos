#ifndef MICROGEAR_H
#define MICROGEAR_H

#include "esp_common.h"
#include "espressif/espconn.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/portmacro.h"

#include "MQTTClient/MQTTESP8266.h"
#include "MQTTClient/MQTTClient.h"
#include "base64.h"
//#define _DEBUG_

#define GBDEFAULTHOST             "gb.netpie.io"
#define GBPORT                    1883
#define GBSECUREPORT              8883
#define DEFAULTSECUREMODE         false

#define DISCONNECTED               -3
#define BUFFER_OVERFLOW            -2
#define FAILURE                    -1
#define SUCCESS                     0


#define PUBSUBQUEUE_TOPICSIZE         48
#define PUBSUBQUEUE_PAYLOADSIZE       256
#define PUBSUBQUEUE_LENGTH             3

#define APPIDSIZE                  32
#define KEYSIZE                    16
#define SECRETSIZE                 32
#define TOKENSIZE                  16
#define TOKENSECRETSIZE            32
#define USERNAMESIZE               65
#define PASSWORDSIZE               28
#define REVOKECODESIZE             28
#define FLAGSIZE                   4
#define FINGERPRINTSIZE            60
#define HMACSIZE                   28
#define PUB_MSG_LEN                16
#define MAXALIASSIZE               16

#define MQTT_USERNAME_SIZE         TOKENSIZE+KEYSIZE+13
#define MQTT_PASSWORD_SIZE         33

#define MESSAGE                    1
#define PRESENT                    2
#define ABSENT                     3
#define CONNECTED                  4
#define CALLBACK                   5
#define ERROR                      6
#define INFO                       7

#define PSQ_PUBLISH                1
#define PSQ_SUBSCRIBE              2
#define PSQ_UNSUBSCRIBE            3
#define PSQ_DISCONNECT             4

#define CTRL_DISCONNECT            "$x01"

typedef struct Microgear Microgear;
typedef struct PubSubQueueMsg PubSubQueueMsg;
typedef struct PubOpt PubOpt;

struct Microgear {
    char *appid;
    char *key;  
    char *secret;
    char *alias;
    char *token;
    char *tokensecret;
    char *host;
    uint16_t port; 

    MQTTClient client;
    struct Network *network;

    void (* cb_connected)(char*, uint8_t*, uint16_t);
    void (* cb_absent)(char*, uint8_t*, uint16_t);
    void (* cb_present)(char*, uint8_t*, uint16_t);
    void (* cb_message)(char*, uint8_t*, uint16_t);
    void (* cb_error)(char*, uint8_t*, uint16_t);
    void (* cb_info)(char*, uint8_t*, uint16_t);

    xTaskHandle mqtttask;
    xQueueHandle ps_queue;
};

struct PubSubQueueMsg {
    uint8_t type;
    char topic[PUBSUBQUEUE_TOPICSIZE+1];
    char payload[PUBSUBQUEUE_PAYLOADSIZE+1];
    uint8_t flag;
};

struct PubOpt {
    bool retained;
};

uint16_t strxcpy(char*, char*, uint16_t);

bool isConnected(Microgear*);

void microgear_init(Microgear*, char* key, char* secret, char* alias);
void microgear_setToken(Microgear*, char* token, char* tokensecret, char *endpoint);

void microgear_connect(Microgear*, char*);
void microgear_disconnect(Microgear*);

bool microgear_isConnected(Microgear *);

int microgear_setAlias(Microgear*, char*);
int microgear_publish(Microgear*, char*, char*, PubOpt*);
int microgear_chat(Microgear*, char*, char*);
int microgear_subscribe(Microgear*, char*);
int microgear_unsubscribe(Microgear*, char*);

void microgear_on(Microgear*, unsigned char,void (* callback)(char*, uint8_t*, uint16_t));

#endif
