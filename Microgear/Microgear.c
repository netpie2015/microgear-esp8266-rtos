#include "Microgear.h"

xSemaphoreHandle *WifiSemaphore = NULL;

void microgear_init(Microgear *mg, char *key, char *secret, char *alias) {
    mg->key = key;
    mg->secret = secret;
    mg->alias = alias;
    mg->cb_connected = NULL;
    mg->cb_absent = NULL;
    mg->cb_present = NULL;
    mg->cb_message = NULL;
    mg->cb_error = NULL;
    mg->cb_info = NULL;
    mg->mqtttask = NULL;
    mg->network = NULL;
    mg->publish_queue = xQueueCreate(PUBQUEUE_LENGTH, sizeof(PubQueueMsg));
}

void microgear_setWifiSemaphore(xSemaphoreHandle *WifiReady) {
    WifiSemaphore = WifiReady; 
}

void microgear_setToken(Microgear *mg, char *token, char* tokensecret, char *endpoint) {
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

uint16_t strxcpy(char *dest, char *src, uint16_t max) {
    if (strlen(src) <= max) {
        max = strlen(src);
    }
    strncpy(dest,src,max);
    dest[max] = '\0';
    return max;
}

int microgear_chat(Microgear *mg, char *alias, char *payload) {
    char chattopic[MAXALIASSIZE+11];
    sprintf(chattopic,"/gearname/%s",alias);
    microgear_publish(mg,chattopic,payload,&DefaultPubOpt);
}

int microgear_publish(Microgear *mg, char *topic, char *payload, PubOpt *opt) {
    PubQueueMsg data;

    data.topic[0] = '/';
    strxcpy(data.topic+1, mg->appid, PUBQUEUE_TOPICSIZE-1);
    strxcpy(data.topic+strlen(mg->appid)+1, topic, PUBQUEUE_TOPICSIZE-strlen(mg->appid)-1);

    strxcpy(data.payload, payload, PUBQUEUE_PAYLOADSIZE);
    if (opt) {
        data.flag = opt->retained?0:1;
    } else {
        data.flag = 0;
    }

    if (xQueueSend(mg->publish_queue, (void *)(&data), 0) == pdFALSE) {
        #ifdef _DEBUG_
            os_printf("Publish queue overflow.\r\n");
        #endif
        return -1;
    }
    else {
        #ifdef _DEBUG_
            os_printf("Publish Success.\r\n");
        #endif
        return 0;
    }
}

// Callback when receiving control message
LOCAL void ICACHE_FLASH_ATTR defaultMsgHandler(MessageData* md, void *c) {
    int i;
    MQTTMessage* message = md->message;

    #ifdef _DEBUG_
        os_printf("Received: ");
        for (i = 0; i < md->topic->lenstring.len; ++i)
            dmsg_putchar(md->topic->lenstring.data[i]);
        os_printf(" = ");
        for (i = 0; i < (int)message->payloadlen; ++i)
            dmsg_putchar(((char*)message->payload)[i]);
        os_printf("\r\n");
    #endif

    Microgear *mg = (Microgear *)(((MQTTClient *)c)->parent);

    char topic[PUBQUEUE_TOPICSIZE+1];
    char firstcharpos = 0;

    strxcpy(topic,(char *)md->topic->lenstring.data,(PUBQUEUE_TOPICSIZE < md->topic->lenstring.len)?PUBQUEUE_TOPICSIZE:md->topic->lenstring.len);
    
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

int microgear_subscribe(Microgear *mg, char *topic) {
    char topicbuff[PUBQUEUE_TOPICSIZE+APPIDSIZE+2];
    sprintf(topicbuff,"/%s%s",mg->appid,topic);
    return MQTTSubscribe(&(mg->client), topicbuff, QOS0, defaultMsgHandler);
}

int microgear_unsubscribe(Microgear *mg, char *topic) {
    char topicbuff[PUBQUEUE_TOPICSIZE+APPIDSIZE+2];
    sprintf(topicbuff,"/%s%s",mg->appid,topic);
    return MQTTUnsubscribe(&(mg->client), topicbuff);
}

LOCAL void ICACHE_FLASH_ATTR mqtt_task(void *pvParameters) {
    bool activeTask = true;
    Microgear *mg = (Microgear *)pvParameters;

    int ret;
    struct Network network;
    mg->client = (MQTTClient)DefaultClient;
 
    char topicbuff[PUBQUEUE_TOPICSIZE+1];
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
        // Wait until wifi is up

        while (!WifiSemaphore) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
        xSemaphoreTake(*WifiSemaphore, portMAX_DELAY);

        sprintf(mqtt_username,"%s%%%s%%%s",mg->token,mg->key,"1478851485");
        sprintf(hashkey,"%s&%s",mg->tokensecret,mg->secret);
        hmac_sha1 (hashkey, strlen(hashkey), mqtt_username, strlen(mqtt_username), raw_password);
        base64Encode(mqtt_password, raw_password, 20);

        #ifdef _DEBUG_
            os_printf("mqtt_username = %s\n", mqtt_username);
            os_printf("real mqtt_username = %s\n", mqtt_username+strlen(mg->token)+1);
            os_printf("raw_password = %s\n", raw_password);
            os_printf("hashkey = %s\n", hashkey);
            os_printf("mqtt_password = %s\n", mqtt_password);
        #endif

        dmsg_printf("(Re)connecting to MQTT server %s : %d... ", mg->host, mg->port);

        ret = ConnectNetwork(mg->network, mg->host, mg->port);

        if (!ret) {
            #ifdef _DEBUG_
                os_printf("Network OK\r\n");
            #endif
            NewMQTTClient(&(mg->client), mg->network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

            mg->client.defaultMessageHandler = defaultMsgHandler;
            mg->client.parent = (void *)mg;

            data.willFlag = 0;
            data.MQTTVersion = 3;
            data.clientID.cstring = mg->token;
            data.username.cstring = mqtt_username+strlen(mg->token)+1;
            data.password.cstring = mqtt_password;
            data.keepAliveInterval = 15;
            data.cleansession = 1;

            #ifdef _DEBUG_
                os_printf(" MQTT connecting ...");
            #endif

            ret = MQTTConnect(&(mg->client), &data);

            if (!ret) {
                xQueueReset(mg->publish_queue);

                char setaliascmd[MAXALIASSIZE+12];
                sprintf(setaliascmd,"/@setalias/%s",mg->alias);
                microgear_publish(mg, setaliascmd, "", NULL);
                PubQueueMsg msg;

                if (mg->cb_connected != NULL) {
                    mg->cb_connected(NULL,NULL,0);
                }

                while (activeTask) {
                    // Publish all pending messages
                    while (xQueueReceive(mg->publish_queue, (void *)&msg, 0) == pdTRUE) {
                        if (strcmp(msg.topic,CTRL_DISCONNECT)==0) {
                            activeTask = false;
                            break;
                        }
                        MQTTMessage message;
                        message.payload = msg.payload;
                        message.payloadlen = strlen(msg.payload);
                        message.dup = 0;
                        message.qos = QOS0;
                        message.retained = msg.flag;
                        ret = MQTTPublish(&(mg->client), msg.topic, &message);
                        if (ret != SUCCESS)
                            break;
                    }
                    // Receiving / Ping
                    ret = MQTTYield(&(mg->client), 1000);
                    if (ret == DISCONNECTED) {
                        break;
                    }
                }
                #ifdef _DEBUG_
                    os_printf("Connection broken, request restart\r\n");
                #endif
            }
            else {
                #ifdef _DEBUG_
                    os_printf("failed.\r\n");
                #endif
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

bool microgear_isconnected(Microgear *mg) {
    return mg->client.isconnected;
}

void microgear_connect(Microgear *mg, char* appid) {
    if (mg->mqtttask == NULL) {
        mg->appid = appid;
        xTaskCreate(mqtt_task, "mqtt", 1024, mg, tskIDLE_PRIORITY + 2, &mg->mqtttask);
    }
}

void microgear_disconnect(Microgear *mg) {
    PubQueueMsg data = {CTRL_DISCONNECT};
    xQueueSend(mg->publish_queue, (void *)(&data), 0);
    mg->mqtttask = NULL;
}

void microgear_on(Microgear *mg, unsigned char event, void (* callback)(char*, uint8_t*,uint16_t)) {
    switch (event) {
        case MESSAGE : 
                if (callback) mg->cb_message = callback;
                break;
        case PRESENT : 
                if (callback) mg->cb_present = callback;
                if (microgear_isconnected(mg))
                    microgear_subscribe(mg,"/&present");
                break;
        case ABSENT : 
                if (callback) mg->cb_absent = callback;
                if (microgear_isconnected(mg))
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
