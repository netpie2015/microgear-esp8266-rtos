#ifndef TOKENSTORE_H
#define TOKENSTORE_H

#include "esp_common.h"
#include "c_types.h"
#include "spi_flash.h"

#define ESP_FLASH_SEC            0x79

#define TOKENSIZE                16
#define TOKENSECRETSIZE          32
#define ENDPOINTSIZE             200

struct token_struct{
    char type;
    char token[TOKENSIZE+1];
    char secret[TOKENSECRETSIZE+1];
    char saddr[ENDPOINTSIZE+1];
    uint16_t sport;
    char flag;
    char dummy[1]; // make struct size devidable by 4
};

typedef struct token_struct Token;

void saveToken(Token*);
void loadToken(Token*);
void clearToken(Token*);

#endif
