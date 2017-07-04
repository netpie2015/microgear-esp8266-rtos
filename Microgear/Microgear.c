#include "Microgear.h"

extern xSemaphoreHandle wifi_semaphore;
PubOpt DefaultPubOpt = {false};
static uint8_t mgcount = 0;

void ICACHE_FLASH_ATTR microgear_init(Microgear *mg, char *key, char *secret, char *alias) {
    mg->id = mgcount++;
    mg->key = key;
    mg->secret = secret;
    mg->token = NULL;
    mg->tokensecret = NULL;
    mg->alias = alias;
    mg->cb_connected = NULL;
    mg->cb_absent = NULL;
    mg->cb_present = NULL;
    mg->cb_message = NULL;
    mg->cb_error = NULL;
    mg->cb_info = NULL;
    mg->mqtttask = NULL;
    mg->network = NULL;
    mg->ps_queue = xQueueCreate(PUBSUBQUEUE_LENGTH, sizeof(PubSubQueueMsg));
}

void ICACHE_FLASH_ATTR microgear_setToken(Microgear *mg, char *token, char* tokensecret, char *endpoint) {
    mg->token = token;
    mg->tokensecret = tokensecret;
    mg->host = GBDEFAULTHOST;
    mg->port = GBPORT;

    if (endpoint != NULL) {
        char *p;
        for (p = endpoint; *p!='\0' && *p!=':' ;p++);
        if (*p == ':') {
            *p = '\0';
            mg->host = endpoint;
            mg->port = atoi(p+1);
        }
    }
 }

void ICACHE_FLASH_ATTR microgear_clearToken(Microgear *mg) {
    if (mg->tokenrec) {
        clearTokenStore(mg->tokenrec, mg->id);
    }
}

void ICACHE_FLASH_ATTR microgear_revokeToken(Microgear *mg) {
    if (!mg->tokenrec) {
        Token token;
        if (loadToken(&token, mg->id)) {
            mg->tokenrec = &token;
        }
    }

    if (mg->tokenrec) {
        callRevokeTokenAPI(mg->tokenrec);
        clearTokenStore(mg->tokenrec, mg->id);
        mg->tokenrec = NULL;
    }
}

int ICACHE_FLASH_ATTR microgear_setAlias(Microgear *mg, char *alias) {
    if (alias && alias[0] != '\0') {
        char setaliascmd[MAXALIASSIZE+12];
        addattr(setaliascmd,"/@setalias/",mg->alias);
        return microgear_publish(mg, setaliascmd, "", NULL);
    }
    else return FAILURE;
}

int ICACHE_FLASH_ATTR microgear_chat(Microgear *mg, char *alias, char *payload) {
    char chattopic[MAXALIASSIZE+11];
    addattr(chattopic,"/gearname/",alias);
    return microgear_publish(mg,chattopic,payload,&DefaultPubOpt);
}

int microgear_publish(Microgear *mg, char *topic, char *payload, PubOpt *opt) {
    PubSubQueueMsg data;

    data.type= PSQ_PUBLISH;
    data.topic[0] = '/';
    strxcpy(data.topic+1, mg->appid, PUBSUBQUEUE_TOPICSIZE-1);
    strxcpy(data.topic+strlen(mg->appid)+1, topic, PUBSUBQUEUE_TOPICSIZE-strlen(mg->appid)-1);

    strxcpy(data.payload, payload, PUBSUBQUEUE_PAYLOADSIZE);
    if (opt) {
        data.flag = opt->retained?1:0;
    } else {
        data.flag = 0;
    }

    if (xQueueSend(mg->ps_queue, (void *)(&data), 0) == pdFALSE) {
        #ifdef DEBUG
            os_printf("PS queue overflow.\r\n");
        #endif
        return -1;
    }
    else {
        #ifdef DEBUG
            os_printf("Publish Success.\r\n");
        #endif
        return 0;
    }
}

// Callback when receiving control message
void defaultMsgHandler(MessageData* md, void *c) {
    int i;
    MQTTMessage* message = md->message;

    #ifdef DEBUG
        os_printf("Received: ");
        for (i = 0; i < md->topic->lenstring.len; ++i)
            dmsg_putchar(md->topic->lenstring.data[i]);
        os_printf(" = ");
        for (i = 0; i < (int)message->payloadlen; ++i)
            dmsg_putchar(((char*)message->payload)[i]);
        os_printf("\r\n");
    #endif

    Microgear *mg = (Microgear *)(((MQTTClient *)c)->parent);

    char topic[PUBSUBQUEUE_TOPICSIZE+1];
    char firstcharpos = 0;

    strxcpy(topic,(char *)md->topic->lenstring.data,(PUBSUBQUEUE_TOPICSIZE < md->topic->lenstring.len)?PUBSUBQUEUE_TOPICSIZE:md->topic->lenstring.len);
    
    // scan for the first position after appid prefix
    for (i = 1; i < md->topic->lenstring.len; ++i) {
        if (md->topic->lenstring.data[i]=='/') {
            firstcharpos = i+1;
            break;
        }
    }

    if (*topic=='@') {
        if (strcmp(topic,"@error") == 0) {
            if (mg->cb_error) {
                mg->cb_error("error",message->payload,message->payloadlen);
            }
        }
        else if (strcmp(topic,"@info") == 0) {
            if (mg->cb_info) {
                mg->cb_info("info",message->payload,message->payloadlen);
            }
        }
    }
    else if (*(topic+firstcharpos) == '&') {
        if (strcmp(topic+firstcharpos,"&present") == 0) {
            if (mg->cb_present) {
                mg->cb_present("present",message->payload,message->payloadlen);
            }
        }
        else if (strcmp(topic+firstcharpos,"&absent") == 0) {
            if (mg->cb_absent) {
                mg->cb_absent("absent",message->payload,message->payloadlen);
            }
        }
        else if (strcmp(topic+firstcharpos,"&present") == 0) {
            if (mg->cb_present) {
                mg->cb_present("present",message->payload,message->payloadlen);
            }
        }
        else if (strcmp(topic+firstcharpos,"&resetendpoint") == 0) {
            /* not yet implemented in this version */
        }
    }
    else {
        if (mg->cb_message) {
            mg->cb_message(topic,message->payload,message->payloadlen);  
        }
    }
}

int ICACHE_FLASH_ATTR microgear_subscribe(Microgear *mg, char *topic) {
    PubSubQueueMsg data;

    data.type= PSQ_SUBSCRIBE;
    data.topic[0] = '/';
    strxcpy(data.topic+1, mg->appid, PUBSUBQUEUE_TOPICSIZE-1);
    strxcpy(data.topic+strlen(mg->appid)+1, topic, PUBSUBQUEUE_TOPICSIZE-strlen(mg->appid)-1);

    if (xQueueSend(mg->ps_queue, (void *)(&data), 0) == pdFALSE) {
        #ifdef DEBUG
            os_printf("PS queue overflow.\r\n");
        #endif
        return -1;
    }
    else {
        #ifdef DEBUG
            os_printf("Subscribe Success.\r\n");
        #endif
        return 0;
    }
}

int ICACHE_FLASH_ATTR microgear_unsubscribe(Microgear *mg, char *topic) {
    PubSubQueueMsg data;

    data.type= PSQ_UNSUBSCRIBE;
    data.topic[0] = '/';
    strxcpy(data.topic+1, mg->appid, PUBSUBQUEUE_TOPICSIZE-1);
    strxcpy(data.topic+strlen(mg->appid)+1, topic, PUBSUBQUEUE_TOPICSIZE-strlen(mg->appid)-1);

    if (xQueueSend(mg->ps_queue, (void *)(&data), 0) == pdFALSE) {
        #ifdef DEBUG
            os_printf("PS queue overflow.\r\n");
        #endif
        return -1;
    }
    else {
        #ifdef DEBUG
            os_printf("Subscribe Success.\r\n");
        #endif
        return 0;
    }
}

void microgear_task(void *pvParameters) {
    bool activeTask = true;
    Token token;
    Microgear *mg = (Microgear *)pvParameters;

    int ret;
    bool stopReadQueue;
    struct Network network;
    mg->client = (MQTTClient)DefaultClient;

    MQTTMessage message;
 
    char *p;
    char topicbuff[PUBSUBQUEUE_TOPICSIZE+1];
    char mqtt_username[MQTT_USERNAME_SIZE+1];
    char raw_password[20];
    char mqtt_password[MQTT_PASSWORD_SIZE+1];
    char hashkey[SECRETSIZE+TOKENSECRETSIZE+2];

    unsigned char mqtt_buf[100];
    unsigned char mqtt_readbuf[100];
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork(&network);
    mg->network = &network;

    while (activeTask) {
        while (!wifi_semaphore) {    // wait for wifi
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        xSemaphoreTake(wifi_semaphore, portMAX_DELAY);

        if (mg->token  == NULL) {
            if (!getAccessToken(&token, mg->appid, mg->key, mg->secret, mg->alias, mg->id)) {
                break;
            }
        }
        else {
            // if token has been manually assigned by api
            strcpy(token.token, mg->token); 
            strcpy(token.secret, mg->tokensecret);
            strcpy(token.saddr, mg->host);
            token.sport = mg->port;
        }

        setTime(getServerTime());

        strrep(mqtt_username, token.token);
        p = addattr(mqtt_username+strlen(token.token), "%", mg->key);
        addattr(p, "%", getTimeStr());
        strrep(hashkey, token.secret);
        addattr(hashkey+strlen(token.secret), "&", mg->secret);

        hmac_sha1 (hashkey, strlen(hashkey), mqtt_username, strlen(mqtt_username), raw_password);
        base64Encode(mqtt_password, raw_password, 20);

        #ifdef DEBUG
            os_printf("mqtt_username = %s\n", mqtt_username);
            os_printf("real mqtt_username = %s\n", mqtt_username+strlen(token.token)+1);
            os_printf("raw_password = %s\n", raw_password);
            os_printf("hashkey = %s\n", hashkey);
            os_printf("mqtt_password = %s\n", mqtt_password);
            os_printf("(Re)connecting to MQTT server %s : %d... ", token.saddr, token.sport);
        #endif
 
        ret = ConnectNetwork(mg->network, token.saddr, token.sport);

        #ifdef DEBUG
            os_printf("Connecting network -- ret = %d\n",ret);
        #endif

        if (!ret) {
            NewMQTTClient(&(mg->client), mg->network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

            mg->client.defaultMessageHandler = defaultMsgHandler;
            mg->client.parent = (void *)mg;

            data.willFlag = 0;
            data.MQTTVersion = 3;
            data.clientID.cstring = token.token;
            data.username.cstring = mqtt_username+strlen(token.token)+1;
            data.password.cstring = mqtt_password;
            data.keepAliveInterval = 15;
            data.cleansession = 1;

            #ifdef DEBUG
                os_printf(" MQTT connecting ...");
            #endif

            ret = MQTTConnect(&(mg->client), &data);

            #ifdef DEBUG
                os_printf("broker connection status = %d\n",ret);
            #endif

            if (ret == BROKER_CONNECTED) {
                xQueueReset(mg->ps_queue);
                PubSubQueueMsg msg;

                microgear_setAlias(mg,mg->alias);

                if (mg->cb_connected != NULL) {
                    mg->cb_connected(NULL,NULL,0);
                }

                while (activeTask) {
                    stopReadQueue = false;
                    while (!stopReadQueue && xQueueReceive(mg->ps_queue, (void *)&msg, 0) == pdTRUE) {
                        switch (msg.type) {
                            case PSQ_PUBLISH     :
                                    message.payload = msg.payload;
                                    message.payloadlen = strlen(msg.payload);
                                    message.dup = 0;
                                    message.qos = QOS0;
                                    message.retained = msg.flag;
                                    ret = MQTTPublish(&(mg->client), msg.topic, &message);
                                    if (ret != SUCCESS) stopReadQueue = true;
                                    break;
                            case PSQ_SUBSCRIBE   :
                                    ret = MQTTSubscribe(&(mg->client), msg.topic, QOS0, defaultMsgHandler);
                                    if (ret != SUCCESS) stopReadQueue = true;
                                    break;
                            case PSQ_UNSUBSCRIBE :
                                    ret = MQTTUnsubscribe(&(mg->client), msg.topic);
                                    if (ret != SUCCESS) stopReadQueue = true;
                                    break;
                            case PSQ_DISCONNECT :
                                    activeTask = false;
                                    stopReadQueue = true;
                                    break;
                        }
                    }
                    // Receiving / Ping
                    ret = MQTTYield(&(mg->client), 1000);
                    if (ret == DISCONNECTED) {
                        break;
                    }
                }
                #ifdef DEBUG
                    os_printf("Connection broken, request restart\r\n");
                #endif
            }
            else {
                if (ret == BROKER_PAUSEDTOKEN) {
                    #ifdef DEBUG
                        os_printf("Token paused\n");
                    #endif
                }
                else {
                    #ifdef DEBUG
                        clearTokenStore(&token, mg->id);
                        os_printf("Connection refused - unknown token\n");
                    #endif
                }
                vTaskDelay(5000 / portTICK_RATE_MS);
            }
            DisconnectNetwork(mg->network);
        }
        else {
            os_printf("failed.\r\n");
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    dmsg_printf("MQTT task ended\r\n", ret);
    mg->mqtttask == NULL;
    vTaskDelete(NULL);
}

bool ICACHE_FLASH_ATTR microgear_isConnected(Microgear *mg) {
    return mg->client.isconnected;
}

void ICACHE_FLASH_ATTR microgear_connect(Microgear *mg, char* appid) {
    if (mg->mqtttask == NULL) {
        mg->appid = appid;
        xTaskCreate(microgear_task, "microgear", 1024, mg, tskIDLE_PRIORITY + 2, &mg->mqtttask);
    }
}

void ICACHE_FLASH_ATTR microgear_disconnect(Microgear *mg) {
    PubSubQueueMsg data;
    data.type = PSQ_DISCONNECT;
    xQueueSend(mg->ps_queue, (void *)(&data), 0);
    mg->mqtttask = NULL;
}

void ICACHE_FLASH_ATTR microgear_on(Microgear *mg, unsigned char event, void (* callback)(char*, uint8_t*,uint16_t)) {
    switch (event) {
        case MESSAGE : 
                if (callback) mg->cb_message = callback;
                break;
        case PRESENT : 
                if (callback) mg->cb_present = callback;
                if (microgear_isConnected(mg))
                    microgear_subscribe(mg,"/&present");
                break;
        case ABSENT : 
                if (callback) mg->cb_absent = callback;
                if (microgear_isConnected(mg))
                    microgear_subscribe(mg,"/&absent");
                break;
        case CONNECTED :
                if (callback) {
                    mg->cb_connected = callback;
                }
                break;
        case ERROR :
                if (callback) {
                    mg->cb_error = callback;
                }
                break;
        case INFO :
                if (callback) {
                    mg->cb_info = callback;
                }
                break;
    }
}
