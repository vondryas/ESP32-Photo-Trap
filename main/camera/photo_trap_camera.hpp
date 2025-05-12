//author: Stepan Vondracek (xvondr27) 
#ifndef PHOTO_TRAP_CAMERA_HPP
#define PHOTO_TRAP_CAMERA_HPP

#include "edge-impulse-sdk/dsp/image/image.hpp"

// 1280x720
#define CAMERA_RAW_FRAME_BUFFER_COLS           1280
#define CAMERA_RAW_FRAME_BUFFER_ROWS           720
#define EI_INFERENCE_FRAME_BUFFER_COLS           96
#define EI_INFERENCE_FRAME_BUFFER_ROWS           96
#define CAMERA_FRAME_BYTE_SIZE                 3
#define CAMERA_FRAME_BUFFER_SIZE                (CAMERA_RAW_FRAME_BUFFER_COLS * CAMERA_RAW_FRAME_BUFFER_ROWS * CAMERA_FRAME_BYTE_SIZE) 


// Initialize the camera and start streaming
// Returns true if successful, false otherwise
bool camera_init(void);

// Stop streaming of sensor data and deinitialize the camera
void camera_deinit(void);

// Capture, rescale and crop image.
// In image_buffer, the image is stored in jpeg format 1280x720.
// To out_buf, the image is converted to RGB888 and in img_width x img_height format.
// Returns true if successful, false otherwise
bool camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);

// Get the image data to edge impulse classifier
int camera_get_data(size_t offset, size_t length, float *out_ptr);

// allocate image buffers for the image data
// Returns true if successful, false otherwise
bool allocate_image_buffers(void);

// free image buffers for the image data
void free_image_buffers(void);

extern uint8_t *image_buffer; // Buffer for the image data
extern uint8_t *image_detection_buffer; // Buffer for the image data used for inference
extern size_t image_buffer_size; // Size of the image buffer

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "who_cam.hpp"
extern who::cam::WhoP4Cam *camera; // Pointer to the camera object
#endif

#endif /* PHOTO_TRAP_CAMERA_HPP */
