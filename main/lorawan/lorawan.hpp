#ifndef LORAWAN_HPP
#define LORAWAN_HPP

#include "sdkconfig.h"
#include "rak3172_lorawan.hpp"
#include "ArduinoAdapter.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

bool lorawan_init(void);
void lorawan_deinit(void);
bool lorawan_send(const String &data);
bool lorawan_can_send(void);
void lorawan_join(void);
void lorawan_set_join_callback(void (*callback)(bool));

#endif /* LORAWAN_HPP */
