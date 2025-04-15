#ifndef PHOTO_TRAP_CAMERA_HPP
#define PHOTO_TRAP_CAMERA_HPP

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "camera_p4.hpp"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "camera_s3.hpp"
#endif

#endif /* PHOTO_TRAP_CAMERA_HPP */
