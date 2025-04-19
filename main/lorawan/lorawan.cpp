#include "lorawan.hpp"

TaskHandle_t my_task_handle = NULL;
bool joined = false;

void LoRaWANLoopTask(void* arg)
{    
    while (1) {
        lorawan.update();
        delay(100);
    }
}

void joinCallback(bool status)
{
    printf("Join Callback: %s\n", status ? "Success" : "Failed");
    bool joined = status;
}

bool lorawan_init(void)
{
    bool status = false;

    for (int i = 0; i < 10; i++) {
        status = lorawan.init(&Serial2, 22, 21, RAK3172_BPS_115200);
        if (status) {
            break;
        }
        delay(500);
    }
    
    if (!status) {
        printf("LoraWan device init failed\n");
        return false;
    }

    for (int i = 0; i < 10; i++) {
        status = lorawan.sendCommand("AT+BAND=" + String(EU868));
        if (status) {
            break;
        }
        delay(500);
    }

    if (!status) {
        printf("LoraWan device init failed\n");
        return false;
    }

    for (int i = 0; i < 10; i++) {
        if (lorawan.setOTAA(DEVEUI, APPEUI, APPKEY)) {
            break;
        }
        delay(500);
    }

    if (!status) {
        printf("LoraWan device init failed\n");
        return false;
    }

    for (int i = 0; i < 10; i++) {
        if (lorawan.setDR(0)) {
            break;
        }
        delay(500);
    }
    printf("Device Init OK");
    for (int i = 0; i < 10; i++) {
        if (lorawan.setMode(CLASS_A)) {
            break;
        }
        delay(500);
    }
    if (!status) {
        printf("LoraWan device init failed\n");
        return false;
    }
    printf("Device Init OK");
    for (int i = 0; i < 10; i++) {
        if (lorawan.setLinkCheck(DIS_LINKCHECK)) {
            break;
        }
        delay(500);
    }
    
    if (!status) {
        printf("LoraWan device init failed\n");
        return false;
    }

    xTaskCreate(LoRaWANLoopTask, "LoRaWANLoopTask", 1024 * 2, NULL, 1, &my_task_handle);

    return true;
}

void lorawan_deinit(void)
{
    if (my_task_handle != NULL) {
        vTaskDelete(my_task_handle);
        my_task_handle = NULL;
    }
    lorawan.deinit();
}

bool lorawan_send(const String& data)
{
    return lorawan.send(data);
}

bool lorawan_can_send(void)
{
    return joined;
}