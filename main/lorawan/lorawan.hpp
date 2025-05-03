#ifndef LORAWAN_HPP
#define LORAWAN_HPP

#include <stdio.h>
#include "sdkconfig.h"
#include "rak3172_lorawan.hpp"
#include "ArduinoAdapter.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// LoRaWAN M5Stack module UART pins
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define RX 22
#define TX 21
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define RX 48
#define TX 47
#else
#error "Unsupported target"
#endif

// LoRaWAN The Things Network application settings. Enter your values in sdkconfig.
#define DevEUI CONFIG_DevEUI
#define AppEUI CONFIG_AppEUI
#define AppKey CONFIG_AppKey

// Initialize and set up the LoRaWAN module for EU868 band
// and set The Things Network application settings.
// Given a loop task to run in the background and a join callback function to call when the join process is finished.
bool lorawan_init();

// Only stop the task, not the LoRaWAN module.
void lorawan_deinit(void);

// Send data to the LoRaWAN network. Returns true if successfully started transmision.
// Returns false if the device is not joined or if the send fails.
size_t lorawan_send(const String &data);

// Send data to the LoRaWAN network. Returns true if successfully started transmission.
// Returns false if the device is not joined or if the send fails.
// This function is used for sending data in uint8_t buffer format.

size_t lorawan_send(uint8_t *data, size_t len);


// Try to join the LoRaWAN network. Check if the device is joined by joined variable.
// Returns true if M5Stack module started the join process successfully else false.
bool lorawan_join(void);


extern RAK3172LoRaWAN lorawan; // LoRaWAN module object

extern HardwareSerial Serial2; // LoRaWAN module serial port object

// Will know if device is joined to the network or not after deep sleep
extern RTC_NOINIT_ATTR bool lorawan_joined;

#endif /* LORAWAN_HPP */
