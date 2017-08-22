#include "simple_gpio.h"

bool simple_gpio_getfuncname(uint8_t pin, uint32_t *gpio_name, uint8_t *gpio_func) {
	if(pin == 6 | pin == 7 | pin == 8 | pin == 9 | pin == 10 | pin == 11 | pin >= 16) {
		os_printf("gpio_init ERROR: There is no GPIO%d\n", pin);
		return false;
	}

	switch(pin) {
		case 0:
			*gpio_func = FUNC_GPIO0;
			*gpio_name = PERIPHS_IO_MUX_GPIO0_U;
			return true;
		case 1:
			*gpio_func = FUNC_GPIO1;
			*gpio_name = PERIPHS_IO_MUX_U0TXD_U;
			return true;
		case 2:
			*gpio_func = FUNC_GPIO2;
			*gpio_name = PERIPHS_IO_MUX_GPIO2_U;
			return true;
		case 3:
			*gpio_func = FUNC_GPIO3;
			*gpio_name = PERIPHS_IO_MUX_U0RXD_U;
			return true;
		case 4:
			*gpio_func = FUNC_GPIO4;
			*gpio_name = PERIPHS_IO_MUX_GPIO4_U;
			return true;
		case 5:
			*gpio_func = FUNC_GPIO5;
			*gpio_name = PERIPHS_IO_MUX_GPIO5_U;
			return true;
		/*
		case 9:
			*gpio_func = FUNC_GPIO9;
			*gpio_name = PERIPHS_IO_MUX_SD_DATA2_U;
			return true;
		case 10:
			*gpio_func = FUNC_GPIO10;
			*gpio_name = PERIPHS_IO_MUX_SD_DATA3_U;
			return true;
		*/
		case 12:
			*gpio_func = FUNC_GPIO12;
			*gpio_name = PERIPHS_IO_MUX_MTDI_U;
			return true;
		case 13:
			*gpio_func = FUNC_GPIO13;
			*gpio_name = PERIPHS_IO_MUX_MTCK_U;
			return true;
		case 14:
			*gpio_func = FUNC_GPIO14;
			*gpio_name = PERIPHS_IO_MUX_MTMS_U;
			return true;
		case 15:
			*gpio_func = FUNC_GPIO15;
			*gpio_name = PERIPHS_IO_MUX_MTDO_U;
			return true;
		default:
			return false;
	}
	return true;
}

bool simple_gpio_init(uint8_t pin) {
	uint32_t gpio_name;
	uint8_t	gpio_func;

	if(!simple_gpio_getfuncname(pin, &gpio_name, &gpio_func)) {
		return false;
	}

	PIN_FUNC_SELECT(gpio_name, gpio_func);
	return true;
}

void simple_gpio_write(uint8_t pin, bool set) {
	GPIO_OUTPUT_SET(pin, set);
}

bool simple_gpio_read(uint8_t pin) {
	GPIO_DIS_OUTPUT(pin);
	return GPIO_INPUT_GET(pin);
}