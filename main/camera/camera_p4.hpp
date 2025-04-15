#ifndef camera_p4_hpp
#define camera_p4_hpp

#include "camera_common.hpp"
#include "edge-impulse-sdk/dsp/image/image.hpp"

// Camera API for Edge Impulse
bool ei_camera_init(void);
void ei_camera_deinit(void);
bool ei_camera_capture(uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
bool allocate_image_buffer(void);
void free_image_buffer(void);

#endif /* camera_p4_hpp */
