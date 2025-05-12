//author: Stepan Vondracek (xvondr27) 
#include "photo_trap_camera.hpp"
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "string.h"

who::cam::WhoP4Cam *camera = nullptr;

static bool is_camera_initialised = false;

// Start camera
bool camera_init(void)
{

    camera = new who::cam::WhoP4Cam(who::cam::VIDEO_PIX_FMT_RGB888, 3, V4L2_MEMORY_USERPTR, true);
    if (camera == nullptr)
    {
        return false;
    }
    is_camera_initialised = true;
    return true;
}

// Stop camera
void camera_deinit(void)
{

    if (camera != nullptr)
    {
        delete camera;
        camera = nullptr;
    }
    is_camera_initialised = false;
    return;
}


// slightly modified version of the camera_capture function from Edge Impulse examples 
// modified is storage of the image in buffers
bool camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf)
{

    bool do_resize = false;

    if (!is_camera_initialised)
    {
        printf("ERR: Camera is not initialized\r\n");
        return false;
    }

    auto *fb = camera->cam_fb_get();
    if (!fb)
    {
        printf("Camera capture failed\n");
        return false;
    }

    ESP_LOGI("Camera", "Captured image %d x %d", fb->width, fb->height);
    ESP_LOGI("Camera", "Image buffer size %d", fb->len);
    memcpy(image_buffer, fb->buf, fb->len);
    memcpy(out_buf, fb->buf, fb->len);

    camera->cam_fb_return();

    if (fb->format != who::cam::VIDEO_PIX_FMT_RGB888)
    {
        printf("Conversion failed\n");
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
        printf("Resized image to %d x %d\n", img_width, img_height);
    }
    return true;
}

// This function is from Edge Impulse examples
// it is used to get the image data from the buffer
// its modified because orginal function uses RGB888 with switched color channels
int camera_get_data(size_t offset, size_t length, float *out_ptr)
{
    // we already have a RGB888 buffer, so recalculate offset into pixel index
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;
    while (pixels_left != 0)
    {
        out_ptr[out_ptr_ix] = (image_detection_buffer[pixel_ix] << 16) +
                              (image_detection_buffer[pixel_ix + 1] << 8) + image_detection_buffer[pixel_ix + 2];

        // go to the next pixel
        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }
    // and done!
    return 0;
}

#endif