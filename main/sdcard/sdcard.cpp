#include "sdcard.hpp"

SemaphoreHandle_t done_sem = NULL;

const char *TAGsd = "sdcard";

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
    ESP_ERROR_CHECK(nvs_flash_deinit());
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

#if defined(CONFIG_IDF_TARGET_ESP32P4)

// Convert the image buffer to JPEG format
// return true if successful, false otherwise
// store new image buffer to image_buffer and set jpeg_length to the length of the JPEG image
bool get_jpg_from_image_buffer(uint8_t *image_buffer, size_t *jpeg_length)
{
    uint8_t *temp_buffer = NULL;
    bool ret = fmt2jpg(image_buffer, CAMERA_FRAME_BUFFER_SIZE, CAMERA_RAW_FRAME_BUFFER_COLS, CAMERA_RAW_FRAME_BUFFER_ROWS, PIXFORMAT_RGB888, 5, &temp_buffer, jpeg_length);
    if (temp_buffer == NULL || ret == false) {
        ESP_LOGE(TAGsd, "Failed to convert image buffer to JPEG format");
        return false;
    }
    // free RGB888 image buffer
    if(image_buffer != NULL) {
        free(image_buffer);
    }
    // set the image buffer to the JPEG image buffer
    image_buffer = temp_buffer;
    return true;
}
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
bool get_jpg_from_image_buffer(uint8_t *image_buffer, size_t *jpeg_length)
{
    return true;
}
#endif

void store_to_sdcard_task(void *arg)
{
    if(ESP_ERROR_CHECK_WITHOUT_ABORT(bsp_sdcard_mount()) == ESP_OK)
    {
        // store data to sd card
        ESP_LOGI(TAGsd, "Storing data to SD card...");
        init_nvs_flash();
        std::string path = BSP_SD_MOUNT_POINT;
        path += "/";
        path += std::to_string(get_nvs_flash_counter());
        path += ".jpg";
        // get jpeg image from image buffer
        if(get_jpg_from_image_buffer(image_buffer, &image_buffer_size) == false) {
            ESP_LOGE(TAGsd, "Failed to convert image buffer to JPEG format");
            deinit_nvs_flash();
            xSemaphoreGive(done_sem);
            vTaskDelete(NULL);
            return;
        }
        FILE* f = fopen(path.c_str(), "w");
        if (f != NULL) {
            // write the image buffer to the file
            size_t written = fwrite(image_buffer, sizeof(uint8_t), image_buffer_size, f);
            fflush(f);
            fclose(f);
            if (written != image_buffer_size) {
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
        ESP_LOGI(TAGsd, "Closing NVS");
        bsp_sdcard_unmount();
        ESP_LOGI(TAGsd, "Unmounting SD card...");
    }
    else
    {
        ESP_LOGI(TAGsd, "Failed to mount SD card");
    }
    ESP_LOGI(TAGsd, "semaphore give done_sem");
    xSemaphoreGive(done_sem);
    ESP_LOGI(TAGsd, "semaphore given done_sem");
    vTaskDelete(NULL);
}
