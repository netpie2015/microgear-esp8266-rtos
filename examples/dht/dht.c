#include "dht.h"

#define DHT_TIMER_INTERVAL 2
#define DHT_DATA_BITS 40

static bool dht_await_pin_state(uint8_t pin, uint32_t timeout, bool expected_pin_state, uint32_t *duration) {
	uint32_t i;

	for(i = 0; i < timeout; i += DHT_TIMER_INTERVAL) {
		os_delay_us(DHT_TIMER_INTERVAL);
		if(simple_gpio_read(pin) == expected_pin_state) {
			if(duration) {
				*duration = i;
			}
			return true;
		}
	}
	return false;
}

static inline int16_t dht_convert_data(dht_sensor_type_t sensor_type, uint8_t msb, uint8_t lsb) {
	int16_t data;

	if(sensor_type == DHT_TYPE_22) {
		data = msb & 0x7F;
		data <<= 8;
		data |= lsb;
		if(msb & BIT(7)) {
			data = 0 - data;
		}
	}
	else {
		data = msb * 10;
	}

	return data;
}

static inline bool dht_fetch_data(uint8_t pin, bool bits[DHT_DATA_BITS]) {
	uint32_t low_duration;
	uint32_t high_duration;

	simple_gpio_write(pin, 0);
	os_delay_us(20000);
	simple_gpio_write(pin, 1);

	if(!dht_await_pin_state(pin, 40, false, NULL)) {
		os_printf("Initialization error\n");
		return false;
	}

	if(!dht_await_pin_state(pin, 88, true, NULL)) {
		os_printf("Initialization error\n");
		return false;
	}

	if(!dht_await_pin_state(pin, 88, false, NULL)) {
		os_printf("Initialization error\n");
		return false;
	}

	uint8_t i;
	for(i = 0; i < DHT_DATA_BITS; i++) {
		if(!dht_await_pin_state(pin, 65, true, &low_duration)) {
			os_printf("LOW bit timeout\n");
			return false;
		}
		if(!dht_await_pin_state(pin, 75, false, &high_duration)) {
			os_printf("HIGH bit timeout\n");
			return false;
		}
		bits[i] = high_duration > low_duration;
	}

	return true;
}

bool dht_read_data(dht_sensor_type_t sensor_type, uint8_t pin, int16_t *humidity, int16_t *temperature) {
	bool bits[DHT_DATA_BITS];
	uint8_t data[DHT_DATA_BITS/8] = {0};
	bool result;

	taskENTER_CRITICAL();
    result = dht_fetch_data(pin, bits);
    taskEXIT_CRITICAL();

    if(!result) {
    	return false;
    }

    uint8_t i, j, twopow;
    for(i = 0; i < DHT_DATA_BITS; i++) {
    	twopow = 128;
    	for(j = 0; j < (i % 8); j++) twopow /= 2;
    	data[i/8] += twopow * bits[i];
    }
    
    if(data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    	os_printf("Checksum failed, invalid data received form sensor\n");
    	return false;
    }

    *humidity = dht_convert_data(sensor_type, data[0], data[1]);
    *temperature = dht_convert_data(sensor_type, data[2], data[3]);

	return true;
}

bool dht_read_float_data(dht_sensor_type_t sensor_type, uint8_t pin, float *humidity, float *temperature) {
	int16_t i_humidity, i_temp;

	if(dht_read_data(sensor_type, pin, &i_humidity, &i_temp)) {
		*humidity = (float)(i_humidity / 10);
		*temperature = (float)(i_temp / 10);
		return true;
	}

	return false;
}

void dht_read_str_data(dht_sensor_type_t sensor_type, uint8_t pin, char *s_data) {
	static char buff[13];
	int16_t i_humidity, i_temp, v_int;
	uint8_t v_float;

	if(dht_read_data(sensor_type, pin, &i_humidity, &i_temp)) {
		sprintf(buff, "%d,%d", (i_humidity / 10), (i_temp / 10));
		strcpy(s_data, buff);
	}
}

void dht_init(uint8_t pin) {
	simple_gpio_init(pin);
}
