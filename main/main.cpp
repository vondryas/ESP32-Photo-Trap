#include <stdio.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "camera/photo_trap_camera.hpp"
#include "lorawan/lorawan.hpp"
#include "lorawan/sender.hpp"
#include "sdcard/sdcard.hpp"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_idf_version.h"
#include "bsp/esp-bsp.h"

#define EDGE_IMPULSE_DEBUG false
const char *TAG = "main";

// Will be used to have some timeout after successful detection
RTC_NOINIT_ATTR time_t last_detection_time; 
 
// Board specific settings
#if defined(CONFIG_IDF_TARGET_ESP32S3)

#define WAKEUP_GPIO GPIO_NUM_21
bsp_led_t bsp_led = BSP_LED_GREEN;

#elif defined(CONFIG_IDF_TARGET_ESP32P4)

#define WAKEUP_GPIO GPIO_NUM_5
// P4 does not have led
#define bsp_led_set(led, state)
#define bsp_leds_init()

#else

#error "Unsupported target"

#endif

extern "C" int app_main()
{
    // Measure time for detection
    int64_t start_time = esp_timer_get_time();

    // results of the detection
    ei_impulse_result_t result = {nullptr};
    // semaphore for store task
    done_sem = xSemaphoreCreateBinary();
    
    time_t now;
    
    // set up the wakeup GPIO
    gpio_set_pull_mode(WAKEUP_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(WAKEUP_GPIO, GPIO_MODE_INPUT);
    
    // set up the wakeup GPIO to wake up the ESP32-S3 from deep sleep
    esp_sleep_enable_ext1_wakeup(1ULL << WAKEUP_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // Know if program was started from deep sleep or not
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
    {
        ESP_LOGI(TAG, "wake up from PIR interrupt");
    }
    else
    {
        time(&last_detection_time);
        ESP_LOGI(TAG, "Turn On device");
    }
    
    time(&now);
    
    // check if last detection was less than 60 seconds ago
    // this will result that device will be sleeping 60 senconds after power on
    if ((difftime(now, last_detection_time) < 60)) {
        esp_deep_sleep_start();
    }
    
    // turn off the LED
    bsp_leds_init();
    bsp_led_set(bsp_led, false);
    
    
    // loraWAN initialization
    if (lorawan_init() == false)
    {
        printf("Failed to initialize LoRaWAN!\r\n");
        esp_deep_sleep_start();
    }
    
    // Initialize camera 
    // without camera will program go to deep sleep
    if (camera_init() == false)
    {
        printf("Failed to initialize Camera!\r\n");
        esp_deep_sleep_start();
    }
    printf("Camera initialized\r\n");
    
    // For better image quality, we wait for 500ms before taking a picture
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // allocate image buffers for the image data
    if (allocate_image_buffers() == false)
    {
        printf("Failed to allocate image buffer\r\n");
        esp_deep_sleep_start();
    }
    
    // for testing purposes, we can print the time it took to initialize the camera and allocate the image buffers
    int64_t end_time = esp_timer_get_time();
    printf("Ready to capture: %lld ms\r\n", (end_time - start_time) / 1000);
    
    
    
    // take a picture and store it in the image buffers
    // without image buffer the program will go to deep sleep
    if (camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, 
                       (size_t)EI_CLASSIFIER_INPUT_HEIGHT, 
                       image_detection_buffer) == false)
    {
        printf("Failed to capture image\r\n");
        free_image_buffers();
        camera_deinit();
        esp_deep_sleep_start();
    }
    
    // Stop the camera and deinitialize it
    camera_deinit();
    
    // Set up signal for edge impulse classifier
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &camera_get_data;
    
    // Start detection on the captured image
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, EDGE_IMPULSE_DEBUG);
    if (res != EI_IMPULSE_OK)
    {
        printf("ERR: Failed to run classifier (%d)\n", res);
        free_image_buffers();
        esp_deep_sleep_start();
    }
    
    // Print the time it took to run from start to finish
    end_time = esp_timer_get_time();
    printf("From start to detection time: %lld ms\r\n", (end_time - start_time) / 1000);
    // print the time it took to run the DSP and ce
    printf("DSP time: %d ms\r\n", result.timing.dsp);
    printf("Classification time: %d ms\r\n", result.timing.classification);
    
    // if the ce result contains any detected classes, send the data to LoRaWAN
    // and store the image to SD card
    if (check_detection_result(&result))
    {
        printf("Object detected\r\n");
        // successful detection, store time
        time(&last_detection_time);   // store the image to SD card
        xTaskCreate(store_to_sdcard_task, "store_to_sdcard_task", 4096 * 4, NULL, 5, NULL);
        //send the data to LoRaWAN
        send_task(&result);
    } 
    else
    {
        printf("No object detected\r\n");

        // if no object detected, no store task is needed
        // just give the semaphore to unblock the task
        xSemaphoreGive(done_sem);   
    }
    // wait for store task to complete
    xSemaphoreTake(done_sem, portMAX_DELAY);
    
    // free the image buffers
    free_image_buffers();
    
    // Sleep until the PIR sensor is triggered and start app_main() again
    esp_deep_sleep_start();
}
;