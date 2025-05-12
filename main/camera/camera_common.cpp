//author: Stepan Vondracek (xvondr27) 
#include "photo_trap_camera.hpp"

bool is_camera_buffer_allocated = false;
uint8_t *image_buffer = NULL; 
uint8_t *image_detection_buffer = NULL; // Buffer for the image data used for inference
size_t image_buffer_size = 0;

bool allocate_image_buffers(void)
{
    if (is_camera_buffer_allocated) {
        return true;
    }

    // Allocate the image buffer
    image_buffer = (uint8_t *)malloc(CAMERA_RAW_FRAME_BUFFER_COLS * CAMERA_RAW_FRAME_BUFFER_ROWS * CAMERA_FRAME_BYTE_SIZE);
    
    if (image_buffer == NULL) {
        ei_printf("Failed to allocate image buffer\n");
        return false;
    }
    
    image_detection_buffer = (uint8_t *)malloc(CAMERA_RAW_FRAME_BUFFER_COLS * CAMERA_RAW_FRAME_BUFFER_ROWS * CAMERA_FRAME_BYTE_SIZE);;
    if (image_detection_buffer == NULL) {
        ei_printf("Failed to allocate image inference buffer\n");
        free(image_buffer);
        image_buffer = NULL;
        return false;
    }

    is_camera_buffer_allocated = true;
    return true;
}

void free_image_buffers(void)
{
    if (!is_camera_buffer_allocated) {
        return;
    }
    if (image_buffer != NULL) {
        free(image_buffer);
        image_buffer = NULL;
    }

    if (image_detection_buffer != NULL) {
        free(image_detection_buffer);
        image_detection_buffer = NULL;
    }

    is_camera_buffer_allocated = false;
}