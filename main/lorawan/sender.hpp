//author: Stepan Vondracek (xvondr27) 
#ifndef SENDER_HPP
#define SENDER_HPP

#include <stdio.h>
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"

// Type of the detected animal
typedef enum detected_class_t
{
    DEER_OR_DOE = 0,
    WILD_BOAR = 1,
} detected_class_t;

//  Check if the inference result contains any detected classes
//  If so, return true, else return false
bool check_detection_result(const ei_impulse_result_t *result);

//  Send the detected classes to the LoRaWAN network
//  If the data is sent successfully, return true, else return false
//  Device must be joined to the LoRaWAN network before sending data
bool send_data(const ei_impulse_result_t *result);

// Try to send the data
// recursively call the function if the first try fails and try join to the network again
// If the second try fails, the function will return false and the program will go to deep sleep
void send_task(const ei_impulse_result_t *result, bool try_second_time = true);

#endif /* SENDER_HPP */
