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

// Will know if device is joined to the network or not after deep sleep
RTC_DATA_ATTR bool lorawan_joined = false;

// Will be used to have some timeout after successful inference
// -60 seconds so device will work after power on
RTC_DATA_ATTR int64_t last_inference_time_us = -60 * 1000000; // 60 seconds

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

void lorawan_loop_task(void *arg)
{
    while (1)
    {
        lorawan.update();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void lorawan_join_callback(bool status)
{
    lorawan_joined = status;
    if (status)
    {
        printf("LoRaWAN joined\r\n");
    }
    else
    {
        printf("LoRaWAN join failed\r\n");
    }
}

// Try to join the network and send the data
// recursively call the function if the first try fails and try join to the network again
// If the second try fails, the function will return false and the program will go to deep sleep
void send_task(const ei_impulse_result_t *result, bool try_second_time = true)
{
    if (lorawan_joined == false)
    {
        lorawan_join();
    }
    // 70 seconds timeout for joining
    for (int i = 0; i < 700; i++)
    {
        if (lorawan_joined)
        {
            ei_printf("joined is true\r\n");
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (send_data(result) == false)
    {
        ei_printf("Failed to send data via LoRaWAN\r\n");
        if (try_second_time)
        {
            lorawan_joined = false;
            send_task(result, false);
        }
    }
    else
    {
        ei_printf("Data sent successfully\r\n");
    }
}

void print_inference_result(ei_impulse_result_t *result)
{

    // Print how long it took to perform inference
    ei_printf("Timing: DSP %d ms, inference %d ms\r\n",
              result->timing.dsp,
              result->timing.classification);

    // Print the prediction results (object detection)
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result->bounding_boxes_count; i++)
    {
        ei_impulse_result_bounding_box_t bb = result->bounding_boxes[i];
        if (bb.value == 0)
        {
            continue;
        }
        ei_printf("  %s (%f) [ x: %u, y: %u, width: %u, height: %u ]\r\n",
                  bb.label,
                  bb.value,
                  bb.x,
                  bb.y,
                  bb.width,
                  bb.height);
    }
}

extern "C" int app_main()
{
    // time in microseconds
    int64_t now = esp_timer_get_time();

    // will try
    if ((now - last_inference_time_us) < 60 * 1000000) {
        
        esp_deep_sleep_start();
    }
    

    
    // results of the inference
    ei_impulse_result_t result = {nullptr};
    // semaphore for store task
    done_sem = xSemaphoreCreateBinary();

    // turn off the LED
    bsp_leds_init();
    bsp_led_set(bsp_led, false);

    // set up the wakeup GPIO
    gpio_set_pull_mode(WAKEUP_GPIO, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(WAKEUP_GPIO, GPIO_MODE_INPUT);

    // set up the wakeup GPIO to wake up the ESP32-S3 from deep sleep
    esp_sleep_enable_ext1_wakeup(1ULL << WAKEUP_GPIO, ESP_EXT1_WAKEUP_ANY_HIGH);

    // Know if program was started from deep sleep or not
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
    {
        ESP_LOGI(TAG, "wake up from timer interrupt");
    }
    else
    {
        ESP_LOGI(TAG, "Turn On device");
    }

    // loraWAN initialization
    if (lorawan_init(lorawan_loop_task) == false)
    {
        ei_printf("Failed to initialize LoRaWAN!\r\n");
        bsp_led_set(bsp_led, true);
    }
    lorawan.onJoin(lorawan_join_callback);


    
    // Initialize camera 
    // without camera will program go to deep sleep
    if (camera_init() == false)
    {
        ei_printf("Failed to initialize Camera!\r\n");
        bsp_led_set(bsp_led, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }

    ei_printf("Camera initialized\r\n");

    // allocate image buffers for the image data
    if (allocate_image_buffers() == false)
    {
        ei_printf("Failed to allocate image buffer\r\n");
        bsp_led_set(bsp_led, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }

    // Set up signal for edge impulse classifier
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &camera_get_data;

    // take a picture and store it in the image buffers
    // without image buffer the program will go to deep sleep
    if (camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, image_inference_buffer) == false)
    {
        ei_printf("Failed to capture image\r\n");
        bsp_led_set(bsp_led, true);
        free_image_buffers();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }

    // Start inference on the captured image
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, EDGE_IMPULSE_DEBUG);
    if (res != EI_IMPULSE_OK)
    {
        ei_printf("ERR: Failed to run classifier (%d)\n", res);
        free_image_buffers();
        bsp_led_set(bsp_led, true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return res;
    }

    // Stop the camera and deinitialize it
    camera_deinit();

    print_inference_result(&result);

    // if the inference result contains any detected classes, send the data to LoRaWAN
    // and store the image to SD card
    if (check_inference_result(&result))
    {
        // successful inference, store time
        last_inference_time_us = now;
        // store the image to SD card
        xTaskCreate(store_to_sdcard_task, "store_to_sdcard_task", 4096, NULL, 5, NULL);
        //send the data to LoRaWAN
        send_task(&result);
    } 
    else
    {
        // if no object detected, no store task is needed
        xSemaphoreGive(done_sem);   
    }

    // wait for store task to complete
    xSemaphoreTake(done_sem, portMAX_DELAY);
    // print heap size
    size_t free_heap = esp_get_free_heap_size();
    ei_printf("Free heap size: %d bytes\r\n", free_heap);

    // free the image buffers
    free_image_buffers();

    // wait for the PIR sensor to be on low level max 2 seconds
    while (!(gpio_get_level(WAKEUP_GPIO) == 0))
    {
        printf("Waiting for PIR sensor to be on low level...\n");
    }

    // Sleep until the PIR sensor is triggered and start app_main() again
    esp_deep_sleep_start();
}
