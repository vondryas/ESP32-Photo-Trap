#include "photo_trap_camera.hpp"
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "who_cam.hpp"

who::cam::WhoP4Cam *camera = nullptr;

static bool is_camera_initialised = false;


/**
 * @brief   Setup image sensor & start streaming
 *
 * @retval  false if initialisation failed
 */
bool ei_camera_init(void) {
    
    camera = new who::cam::WhoP4Cam(who::cam::VIDEO_PIX_FMT_RGB888, 3, V4L2_MEMORY_USERPTR, true);
    if (camera == nullptr) {
        return false;
    }
    is_camera_initialised = true;
    return true;
}

/**
 * @brief      Stop streaming of sensor data
 */
void ei_camera_deinit(void) {
    
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    is_camera_initialised = false;
    return;
}


/**
 * @brief      Capture, rescale and crop image
 *
 * @param[in]  img_width     width of output image
 * @param[in]  img_height    height of output image
 * @param[in]  out_buf       pointer to store output image, NULL may be used
 *                           if ei_camera_frame_buffer is to be used for capture and resize/cropping.
 *
 * @retval     false if not initialised, image captured, rescaled or cropped failed
 *
 */
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
    
    bool do_resize = false;
    
    if (!is_camera_initialised) {
        ei_printf("ERR: Camera is not initialized\r\n");
        return false;
    }
    
    auto *fb = camera->cam_fb_get();
    if (!fb) {
        ei_printf("Camera capture failed\n");
        return false;
    }
    
    memcpy(out_buf, fb->buf, fb->len);
    camera->cam_fb_return();

    if (fb->format != who::cam::VIDEO_PIX_FMT_RGB888) {
        ei_printf("Conversion failed\n");
        return false;
    }

    if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
    || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
        do_resize = true;
    }

    if (do_resize) {
        ei::image::processing::crop_and_interpolate_rgb888(
            out_buf,
            EI_CAMERA_RAW_FRAME_BUFFER_COLS,
            EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
            out_buf,
            img_width,
            img_height);
        ei_printf("Resized image to %d x %d\n", img_width, img_height);
    }   
    return true;
}

int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
     // we already have a RGB888 buffer, so recalculate offset into pixel index
     size_t pixel_ix = offset * 3;
     size_t pixels_left = length;
     size_t out_ptr_ix = 0;
     while (pixels_left != 0) {
         // Swap BGR to RGB here
         // due to https://github.com/espressif/esp32-camera/issues/379
         out_ptr[out_ptr_ix] = (image_buffer[pixel_ix + 2] << 16) + (image_buffer[pixel_ix + 1] << 8) + image_buffer[pixel_ix];
         
         // go to the next pixel
         out_ptr_ix++;
         pixel_ix+=3;
         pixels_left--;
     }
     // and done!
     return 0;
}

#endif