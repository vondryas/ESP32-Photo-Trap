#ifndef SDCARD_HPP
#define SDCARD_HPP

#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_camera.h"
#include <iostream>
#include "freertos/semphr.h"
#include "../camera/photo_trap_camera.hpp"


// Use this function in freertos task to store data to SD card
void store_to_sdcard_task(void *arg);

// Semaphore for signal when the store task is done
extern SemaphoreHandle_t done_sem;

#endif /* SDCARD_HPP */
