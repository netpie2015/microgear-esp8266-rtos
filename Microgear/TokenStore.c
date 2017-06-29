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
	if (token) memset(token, 0, sizeof(Token));
    spi_flash_erase_sector(ESP_FLASH_SEC);
}
