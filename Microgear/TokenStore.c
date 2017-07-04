#include "TokenStore.h"

static uint32_t checksum(uint8_t *token, size_t len) {
    uint32_t sum = 0;
    while(len--) {
        sum += *(token++);
    }
    return sum;
}

static void ICACHE_FLASH_ATTR generateChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    token->checksum = checksum((uint8_t *)token, len);
}

static bool ICACHE_FLASH_ATTR compareChecksum(Token *token) {
    size_t len = sizeof(Token) - sizeof(token->checksum);
    uint32_t chks = checksum((uint8_t *)token, len);
    if(token->checksum == chks) {
       return true;
    }
    return false;
}

void ICACHE_FLASH_ATTR saveToken(Token *token, uint8_t id) {
	generateChecksum(token);
    spi_flash_erase_sector(TOKEN_FLASH_SEC+id);
    spi_flash_write((TOKEN_FLASH_SEC+id) * SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

int ICACHE_FLASH_ATTR loadToken(Token *token, uint8_t id) {
    spi_flash_read((TOKEN_FLASH_SEC+id) * SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
    if(compareChecksum(token)) return 1;
    else {
	    memset(token, 0, sizeof(Token));
    	return 0;
    }
}

void ICACHE_FLASH_ATTR clearTokenStore(Token *token, uint8_t id) {
    memset(token, 0, sizeof(Token));
    spi_flash_erase_sector(TOKEN_FLASH_SEC+id);
}
