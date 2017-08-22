#ifndef _SIMPLEGPIO_H_
#define _SIMPLEGPIO_H_

#include "esp_common.h"
#include "driver/gpio.h"

bool simple_gpio_getfuncname(uint8_t pin, uint32_t *gpio_name, uint8_t *gpio_func);
bool simple_gpio_init(uint8_t pin);
void simple_gpio_write(uint8_t pin, bool set);
bool simple_gpio_read(uint8_t pin);

#endif
