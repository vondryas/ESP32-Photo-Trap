/* Edge Impulse Espressif ESP32 Standalone Inference ESP IDF Example
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
#include <stdio.h>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"

#include "camera/photo_trap_camera.hpp"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_idf_version.h"


#define LED_PIN GPIO_NUM_3

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

void print_inference_result(ei_impulse_result_t result) {

    // Print how long it took to perform inference
    ei_printf("Timing: DSP %d ms, inference %d ms\r\n",
            result.timing.dsp,
            result.timing.classification);

    // Print the prediction results (object detection)
    ei_printf("Object detection bounding boxes:\r\n");
    for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value == 0) {
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
    ei_sleep(100);
    if (ei_camera_init() == false) {
        ei_printf("Failed to initialize Camera!\r\n");
    }
    else {
        ei_printf("Camera initialized\r\n");
    }
    ei_impulse_result_t result = { nullptr };

    ei_printf("Edge Impulse standalone inferencing (Espressif ESP32)\n");
    ei_printf("Model: %s (id: %d)\n", EI_CLASSIFIER_PROJECT_NAME, EI_CLASSIFIER_PROJECT_ID);
    ei_printf("Model version: %d\n", EI_CLASSIFIER_PROJECT_DEPLOY_VERSION);
    ei_printf("Model DSP input size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("Model NN input size: %d\n", EI_CLASSIFIER_NN_INPUT_FRAME_SIZE);
    //ei_printf("max arena size: %d\n", EI_MAX_OVERFLOW_BUFFER_COUNT);


    while (true)
    {
        // blink LED
        gpio_set_level(LED_PIN, 1);
        if (allocate_image_buffer() == false) {
            ei_printf("Failed to allocate image buffer\r\n");
            return -1;
        } 
        
        signal_t signal;
        signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
        signal.get_data = &ei_camera_get_data;

        printf("Capturing image\r\n");
        if (ei_camera_capture((size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, image_buffer) == false) {
            ei_printf("Failed to capture image\r\n");

            free_image_buffer();
            return -1;
        }
        printf("Captured image\r\n");

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&signal, &result, debug_nn /* debug */);
        if (res != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", res);
            free_image_buffer();
            return res;
        }

        print_inference_result(result);
        gpio_set_level(LED_PIN, 0);
        free_image_buffer();
        ei_sleep(5000);
    }
}





