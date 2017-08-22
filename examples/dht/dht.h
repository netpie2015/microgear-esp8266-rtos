#ifndef DHT_H
#define DHT_H

#include "simple_gpio.h"

#define DHT_TYPE_11 0
#define DHT_TYPE_22 1

typedef uint8_t dht_sensor_type_t;

bool dht_read_int_data(dht_sensor_type_t, uint8_t pin, int16_t *humidity, int16_t *temperature);
bool dht_read_float_data(dht_sensor_type_t, uint8_t pin, float *humidity, float *temperature);
void get_read_str_data(dht_sensor_type_t, uint8_t pin, char *s_data);

#endif
