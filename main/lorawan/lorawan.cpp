#include "lorawan.hpp"

TaskHandle_t my_task_handle = NULL;
RAK3172LoRaWAN lorawan;
HardwareSerial Serial2(UART_NUM_2);
RTC_NOINIT_ATTR bool lorawan_joined = false;

// Check if lorawan module have new event
// for example Join event
void lorawan_loop_task(void *arg)
{
    while (1)
    {
        lorawan.update();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void lorawan_join_callback(bool status)
{
    lorawan_joined = status;
    if (status)
    {
        printf("LoRaWAN joined\r\n");
    }
    else
    {
        printf("LoRaWAN join failed\r\n");
    }
}

bool lorawan_init()
{
    bool status = false;

    for (int i = 0; i < 10; i++)
    {
        status = lorawan.init(&Serial2, RX, TX, RAK3172_BPS_115200);
        if (status)
        {
            break;
        }
        delay(500);
    }

    if (!status)
    {
        printf("LoraWan device init failed\n");
        return false;
    }

    for (int i = 0; i < 10; i++)
    {
        status = lorawan.sendCommand("AT+BAND=" + String(EU868));
        if (status)
        {
            break;
        }
        delay(500);
    }

    if (!status)
    {
        printf("LoraWan device init failed\n");
        return false;
    }

    for (int i = 0; i < 10; i++)
    {
        if (lorawan.setOTAA(DevEUI, AppEUI, AppKey))
        {
            break;
        }
        delay(500);
    }

    for (int i = 0; i < 10; i++)
    {
        if (lorawan.setDR(0))
        {
            break;
        }
        delay(500);
    }

    for (int i = 0; i < 10; i++)
    {
        if (lorawan.setMode(CLASS_A))
        {
            break;
        }
        delay(500);
    }

    for (int i = 0; i < 10; i++)
    {
        if (lorawan.setLinkCheck(DIS_LINKCHECK))
        {
            break;
        }
        delay(500);
    }

    lorawan.onJoin(lorawan_join_callback);

    if (my_task_handle == NULL)
    {
        xTaskCreate(lorawan_loop_task, "LoRaWANLoopTask", 1024 * 2, NULL, 1, &my_task_handle);
    }

    return true;
}

void lorawan_deinit(void)
{
    if (my_task_handle != NULL)
    {
        vTaskDelete(my_task_handle);
        my_task_handle = NULL;
    }
}

size_t lorawan_send(const String &data)
{
    return lorawan.send(data);
}

size_t lorawan_send(uint8_t *data, size_t len)
{
    return lorawan.send(data, len);
}

bool lorawan_join()
{
    return lorawan.join(true, false, 7, 10);
}
