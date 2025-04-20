#ifndef SENDER_HPP
#define SENDER_HPP

#include <stdio.h>
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"

typedef enum detected_class_t
{
    DEER_OR_DOE = 0,
    WILD_PIG = 1,
} detected_class_t;

//  Check if the inference result contains any detected classes
//  If so, return true, else return false
bool check_inference_result(const ei_impulse_result_t *result);

//  Send the detected classes to the LoRaWAN network
//  If the data is sent successfully, return true, else return false
//  Device must be joined to the LoRaWAN network before sending data
bool send_data(const ei_impulse_result_t *result);

#endif /* SENDER_HPP */
