#include "sdcard.hpp"

SemaphoreHandle_t done_sem = NULL;

// functions working with NVS flash
// initialize NVS flash
void init_nvs_flash()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// deinitialize NVS flash
void deinit_nvs_flash()
{
    nvs_flash_deinit_partition("storage");
}

// get NVS flash stored counter value
int get_nvs_flash_counter()
{
    nvs_handle_t my_handle;
    uint32_t counter = 0;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_get_u32(my_handle, "counter", &counter);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    } else {
        printf("Error (%s) opening NVS!\n", esp_err_to_name(err));
    }
    return counter;
}

// increment NVS flash stored counter value
void increment_nvs_flash_counter()
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        uint32_t counter = get_nvs_flash_counter();
        counter++;
        err = nvs_set_u32(my_handle, "counter", counter);
        if (err != ESP_OK) {
            printf("Error (%s) writing!\n", esp_err_to_name(err));
        }
        nvs_commit(my_handle);
        nvs_close(my_handle);
    } else {
        printf("Error (%s) opening NVS!\n", esp_err_to_name(err));
    }
}

void store_to_sdcard_task(void *arg)
{
    
    if(ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_sdcard_mount()) == ESP_OK)
    {
        // store data to sd card
        init_nvs_flash();
        std::string path = BSP_SD_MOUNT_POINT;
        path += "/";
        size_t jpeg_length = CAMERA_FRAME_BUFFER_SIZE;
        path += std::to_string(get_nvs_flash_counter());
        path += ".jpg";
        FILE* f = fopen(path.c_str(), "w");
        if (f != NULL) {
            size_t written = fwrite(image_buffer, sizeof(uint8_t), jpeg_length, f);
            fflush(f);
            fclose(f);
            if (written != jpeg_length) {
                remove(path.c_str());
            } else {
                // increment the counter in NVS flash
                increment_nvs_flash_counter();
            }
        }
        else {
            printf("Error opening file for writing\n");
        }
        deinit_nvs_flash();
        bsp_sdcard_unmount();
    }
    xSemaphoreGive(done_sem);
}
