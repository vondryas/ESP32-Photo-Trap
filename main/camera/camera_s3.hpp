#ifndef camera_s3_hpp
#define camera_s3_hpp

// Camera driver for ESP32-S3
#include "esp_camera.h"
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include "camera_common.hpp"

// Camera pins
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

// Camera API for Edge Impulse
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
bool allocate_image_buffer(void);
void free_image_buffer(void);

extern uint8_t *image_buffer; // Buffer for the image data

#endif /* camera_s3_hpp */
