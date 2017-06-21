#include "TokenStore.h"

void saveToken(Token *token) {
	spi_flash_erase_sector(ESP_PARAM_SEC_A);
	spi_flash_write(ESP_PARAM_SEC_A*SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

void loadToken(Token *token) {
    memset(token, 0, sizeof(Token));
	spi_flash_read(ESP_PARAM_SEC_A*SPI_FLASH_SEC_SIZE, (uint32 *)token, sizeof(Token));
}

void clearToken(Token *token) {
    memset(token, 0, sizeof(Token));
	spi_flash_erase_sector(ESP_PARAM_SEC_A);
}
