#ifndef PHOTO_TRAP_CAMERA_HPP
#define PHOTO_TRAP_CAMERA_HPP

#include "edge-impulse-sdk/dsp/image/image.hpp"

// 1280x720
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           1280
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           720
#define EI_CAMERA_FRAME_BYTE_SIZE                 3
#define EI_CAMERA_FRAME_BUFFER_SIZE                (EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE) 

// Camera API for Edge Impulse
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
bool allocate_image_buffer(void);
void free_image_buffer(void);

extern uint8_t *image_buffer; // Buffer for the image data

#endif /* PHOTO_TRAP_CAMERA_HPP */
