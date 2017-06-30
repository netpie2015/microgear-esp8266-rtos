#include "TokenStore.h"

static uint32_t checksum(uint8_t *token, size_t len) {
    uint32_t sum = 0;
    while(len--) {
        sum += *(token++);
    }
    return sum;
}

static void generateChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    token->checksum = checksum((uint8_t *)token, len);
}

static bool compareChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    uint32_t chks = checksum((uint8_t *)token, len);
    if(token->checksum == chks) {
       return true;
    }
    return false;
}

void saveToken(Token *token) {
	generateChecksum(token);
    spi_flash_erase_sector(ESP_FLASH_SEC);
    spi_flash_write(ESP_FLASH_SEC * SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

int loadToken(Token *token) {
    spi_flash_read(ESP_FLASH_SEC * SPI_FLASH_SEC_SIZE, (uint32 *)&token, sizeof(Token));
    if(compareChecksum(token)) return 1;
    else {
	    memset(token, 0, sizeof(Token));
    	return 0;
    }
}

void clearTokenStore(Token *token) {
    memset(token, 0, sizeof(Token));
    spi_flash_erase_sector(ESP_FLASH_SEC);
}
