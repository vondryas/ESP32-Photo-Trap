#ifndef ARDUINO_ADAPTER_HPP
#define ARDUINO_ADAPTER_HPP

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string>
#include <cstring>

// From esp32-arduino
enum SerialConfig {
    SERIAL_5N1 = 0x8000010,
    SERIAL_6N1 = 0x8000014,
    SERIAL_7N1 = 0x8000018,
    SERIAL_8N1 = 0x800001c,
    SERIAL_5N2 = 0x8000030,
    SERIAL_6N2 = 0x8000034,
    SERIAL_7N2 = 0x8000038,
    SERIAL_8N2 = 0x800003c,
    SERIAL_5E1 = 0x8000012,
    SERIAL_6E1 = 0x8000016,
    SERIAL_7E1 = 0x800001a,
    SERIAL_8E1 = 0x800001e,
    SERIAL_5E2 = 0x8000032,
    SERIAL_6E2 = 0x8000036,
    SERIAL_7E2 = 0x800003a,
    SERIAL_8E2 = 0x800003e,
    SERIAL_5O1 = 0x8000013,
    SERIAL_6O1 = 0x8000017,
    SERIAL_7O1 = 0x800001b,
    SERIAL_8O1 = 0x800001f,
    SERIAL_5O2 = 0x8000033,
    SERIAL_6O2 = 0x8000037,
    SERIAL_7O2 = 0x800003b,
    SERIAL_8O2 = 0x800003f
  };

// ESP-IDF does not have a delay function that takes milliseconds as an argument
#define delay(ms) vTaskDelay(ms / portTICK_PERIOD_MS)

#define HEX 16
#define DEC 10

// ESP-IDF does not have a millis function that returns milliseconds since boot
inline unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

// Class to handle strings in a way similar to Arduino's String class.
// Implemented only the methods that are needed for the LoRaWAN library.
class String {
private:
    std::string _data;
public:
    String() : _data("") {}
    String(char ch) : _data(1, ch) {}
    String(const char* str) : _data(str) {}
    String(const std::string& str) : _data(str) {}
    String(const String& other) : _data(other._data) {}
    String(uint8_t num, int base = DEC) {
        char buffer[33];
        if (base == HEX) {
            snprintf(buffer, sizeof(buffer), "%x", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%d", num);
        }
        _data = buffer;
    }
    String(int num, int base = DEC) {
        char buffer[33];
        if (base == HEX) {
            snprintf(buffer, sizeof(buffer), "%x", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%d", num);
        }
        _data = buffer;
    }
    String(long int num, int base = DEC) {
        char buffer[33];
        if (base == HEX) {
            snprintf(buffer, sizeof(buffer), "%lx", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%ld", num);
        }
        _data = buffer;
    }
    String(long unsigned int num, int base = DEC) {
        char buffer[33];
        if (base == HEX) {
            snprintf(buffer, sizeof(buffer), "%lx", num);
        } else {
            snprintf(buffer, sizeof(buffer), "%ld", num);
        }
        _data = buffer;
    }
    bool isEmpty() const {
        return _data.empty();
    }
    std::string::iterator begin() { return _data.begin(); }
    std::string::iterator end() { return _data.end(); }
    std::string::const_iterator begin() const { return _data.begin(); }
    std::string::const_iterator end() const { return _data.end(); }

    size_t length() const { return _data.length(); }
    const char* c_str() const { return _data.c_str(); }
    void toCharArray(char* buffer, size_t size) const {
        strncpy(buffer, _data.c_str(), size - 1);
        buffer[size - 1] = '\0';
    }
    
    String& operator=(const char* str) { _data = str; return *this; }
    String& operator=(const String& other) { _data = other._data; return *this; }
    String operator+(const String& other) const { return String(_data + other._data); }
    String& operator+=(const char* str) { _data += str; return *this; }
    String& operator+=(const String& other) { _data += other._data; return *this; }

    bool operator==(const String& other) const { return _data == other._data; }
    int toInt() const { return std::stoi(_data); }

    size_t indexOf(const char* substr) const {
        size_t pos = _data.find(substr);
        return (pos != std::string::npos) ? pos : -1;
    }

    size_t indexOf(char ch, size_t fromIndex = 0) const {
        size_t pos = _data.find(ch, fromIndex);
        return (pos != std::string::npos) ? pos : -1;
    }

    String substring(size_t begin, size_t end) const {
        return String(_data.substr(begin, end - begin));
    }

    String substring(size_t begin) const {
        return String(_data.substr(begin));
    }

    void remove(size_t index, size_t count = 1) {
        if (index < _data.length()) {
            _data.erase(index, count);
        }
    }
};


// Class to handle serial communication in a way similar to Arduino's HardwareSerial class.
// Implemented only the methods that are needed for the LoRaWAN library.
class HardwareSerial {
private:
    uart_port_t _uart_num;
    int _tx_pin, _rx_pin;
    int _baud_rate;
    SemaphoreHandle_t _mutex;
public:
    HardwareSerial(uart_port_t uart_num) : _uart_num(uart_num), _tx_pin(-1), _rx_pin(-1), _baud_rate(115200) {
        _mutex = xSemaphoreCreateMutex();
    }
    
    void begin(int baud, SerialConfig config = SERIAL_8N1, int rx = -1, int tx = -1) {
        if(uart_is_driver_installed(_uart_num)) {
            uart_driver_delete(_uart_num);
        }
        _baud_rate = baud;
        _rx_pin = rx;
        _tx_pin = tx;
        uart_config_t uart_config = {
            .baud_rate = baud,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT
        };
        uart_driver_install(_uart_num, 1024, 0, 0, NULL, 0);
        uart_param_config(_uart_num, &uart_config);
        uart_set_pin(_uart_num, _tx_pin, _rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }

    void println(const String& msg) {

        uart_write_bytes(_uart_num, msg.c_str(), msg.length());
        uart_write_bytes(_uart_num, "\r\n", 2);

    }
    String readString() {
        char buffer[128] = {0};
        uart_read_bytes(_uart_num, (uint8_t*)buffer, sizeof(buffer) - 1, 100 / portTICK_PERIOD_MS);
        return String(buffer);
    }
    void setTimeout(uint32_t timeout) {}

    String readStringUntil(char terminator) {
        String result;
        char ch;
        while (true) {
            int len = uart_read_bytes(_uart_num, (char*)&ch, 1, 100 / portTICK_PERIOD_MS);
            if (len > 0) {
                if (ch == terminator) break;
                result += String(ch);
            } else {
                break;
            }
        }
        return result;
    }
    
    void updateBaudRate(int baud) {
        _baud_rate = baud;
        uart_set_baudrate(_uart_num, baud);
    }

    void flush() {
        uart_flush(_uart_num);
    }
};

inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

#endif /* ARDUINO_ADAPTER_HPP */

