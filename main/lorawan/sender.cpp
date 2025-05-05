#include "sender.hpp"
#include "lorawan.hpp"

const char *labels[] = {"Deer or doe", "Wild pig"};

bool check_detection_result(const ei_impulse_result_t *result)
{
    if (result != nullptr)
    {
        if (result->bounding_boxes_count)
        {
            return true;
        }
    }
    return false;
}

bool send_data(const ei_impulse_result_t *result)
{
    if (result != nullptr)
    {
        uint8_t data_to_send[4] = {0};
        size_t data_to_send_len;
        uint8_t doe_number = 0;
        uint8_t pig_number = 0;
        for (int i = 0; i < result->bounding_boxes_count; i++)
        {
            if (strcmp(result->bounding_boxes[i].label, labels[0]) == 0)
            {
                doe_number++;
            }
            else if (strcmp(result->bounding_boxes[i].label, labels[1]) == 0)
            {
                pig_number++;
            }
        }
        
        // format the data to be sent to LoRaWAN
        // class of the detected object and the number of detected objects
        // 0: Deer or doe, 1: Wild pig
        // example: 0 2 1 3 means 2 deer or doe and 3 wild pigs detected
        // 1 2 means 2 wild pig detected
        // 0 1 means 1 deer or doe detected
        if (doe_number > 0)
        {
            data_to_send[0] = DEER_OR_DOE;
            data_to_send[1] = doe_number;
            data_to_send_len = 2;
        }
        if (pig_number > 0 && doe_number > 0)
        {
            data_to_send[2] = WILD_PIG;
            data_to_send[3] = pig_number;
            data_to_send_len = 4;
        }
        else if (pig_number > 0 && doe_number == 0)
        {
            data_to_send[0] = WILD_PIG;
            data_to_send[1] = pig_number;
            data_to_send_len = 2;
        }
        else if (doe_number == 0 && pig_number == 0)
        {
            return false;
        }

        size_t bytes_sent = lorawan_send(data_to_send, data_to_send_len);
        if (bytes_sent == 0)
        {
            printf("Failed to send data via LoRaWAN\n");
            return false;
        }
        printf("Sent data via LoRaWAN: %d %d\n", data_to_send[0], data_to_send[1]);
        return true;
    }
    return false;
}


void send_task(const ei_impulse_result_t *result, bool try_second_time)
{
    if (lorawan_joined == false)
    {
        lorawan_join();
    }
    // 70 seconds timeout for joining
    for (int i = 0; i < 700; i++)
    {

        if (lorawan_joined)
        {
            // successfully joined the network
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (send_data(result) == false)
    {
        printf("Failed to send data via LoRaWAN\r\n");
        if (try_second_time)
        {
            lorawan_joined = false;
            send_task(result, false);
        }
    }
    else
    {
        printf("Data sent successfully\r\n");
    }
}
