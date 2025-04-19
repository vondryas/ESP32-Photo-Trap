idf_component_register(SRCS "rak3172_lorawan.cpp" "rak3172_common.cpp" "ArduinoAdapter.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_common freertos esp_timer)