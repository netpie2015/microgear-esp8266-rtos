#include "TokenStore.h"

void saveToken(Token *token) {
    spi_flash_erase_sector(ESP_FLASH_SEC);
    spi_flash_write(ESP_FLASH_SEC * SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

void loadToken(Token *token) {
    memset(token, 0, sizeof(Token));
    spi_flash_read(ESP_FLASH_SEC * SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

void clearToken(Token *token) {
    memset(token, 0, sizeof(Token));
    spi_flash_erase_sector(ESP_FLASH_SEC);
}

static uint32_t checksum(uint8_t *token, size_t len) {
    uint32_t sum = 0;
    int i = 0;
    while(len--) {
    	if(i++ % 4 == 0) os_printf("\n");
        sum += *(token++);
    }
    return sum;
}

void generateChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    token->checksum = checksum((uint8_t *)token, len);
}

bool compareChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    uint32_t chks = checksum((uint8_t *)token, len);
    if(token->checksum == chks) {
       return true;
    }
    return false;
}