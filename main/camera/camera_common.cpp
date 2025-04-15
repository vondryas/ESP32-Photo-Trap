#include "photo_trap_camera.hpp"

bool is_camera_buffer_allocated = false;
uint8_t *image_buffer = NULL; 

bool allocate_image_buffer(void)
{
    if (is_camera_buffer_allocated) {
        return true;
    }

    // Allocate the image buffer
    image_buffer = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
    if (image_buffer == NULL) {
        ei_printf("Failed to allocate image buffer\n");
        return false;
    }

    is_camera_buffer_allocated = true;
    return true;
}

void free_image_buffer(void)
{
    if (!is_camera_buffer_allocated) {
        return;
    }
    if (image_buffer != NULL) {
        free(image_buffer);
        image_buffer = NULL;
    }
    is_camera_buffer_allocated = false;
}