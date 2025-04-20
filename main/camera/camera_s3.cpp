#include "photo_trap_camera.hpp"
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "esp_camera.h"
#include <string.h>

// Camera pins
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

static bool is_camera_initialised = false;

static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_HD,     // 1280x720

    .jpeg_quality = 5, // 0-63 lower number means higher quality
    .fb_count = 1,      // if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


bool camera_init(void)
{

    if (is_camera_initialised)
        return true;

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ei_printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);      // flip it back
        s->set_brightness(s, 1); // up the brightness just a bit
        s->set_saturation(s, 0); // lower the saturation
    }
    s->set_vflip(s, 1);
    is_camera_initialised = true;
    return true;
}


void camera_deinit(void)
{

    // deinitialize the camera
    esp_err_t err = esp_camera_deinit();
    if (err != ESP_OK)
    {
        ei_printf("Camera deinit failed\n");
        return;
    }
    is_camera_initialised = false;
    return;
}


bool camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{
    bool do_resize = false;

    if (out_buf == nullptr)
    {
        ei_printf("ERR: out_buf is null\r\n");
        return false;
    }

    if (!is_camera_initialised)
    {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb)
    {
        ei_printf("Camera capture failed\n");
        return false;
    }

    memcpy(image_buffer, fb->buf, fb->len);

    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, out_buf);

    esp_camera_fb_return(fb);

    if (!converted)
    {
        ei_printf("Conversion failed\n");
        return false;
    }

    if ((img_width != CAMERA_RAW_FRAME_BUFFER_COLS) || (img_height != CAMERA_RAW_FRAME_BUFFER_ROWS))
    {
        do_resize = true;
    }

    if (do_resize)
    {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            CAMERA_RAW_FRAME_BUFFER_COLS,
            CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
    }
    return true;
}

int camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    if (image_buffer == nullptr)
    {
        ei_printf("ERR: image buffer is null\r\n");
        return -1;
    }
    if (image_inference_buffer == nullptr)
    {
        ei_printf("ERR: image inference buffer is null\r\n");
        return -1;
    }


    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;
    while (pixels_left != 0)
    {
        // Swap BGR to RGB here
        // due to https://github.com/espressif/esp32-camera/issues/379
        out_ptr[out_ptr_ix] = (image_inference_buffer[pixel_ix + 2] << 16) +
                              (image_inference_buffer[pixel_ix + 1] << 8) + image_inference_buffer[pixel_ix];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    // and done!
    return 0;
}
#endif