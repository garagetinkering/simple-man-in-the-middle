#include "driver/twai.h"
#include "driver/gpio.h"

#define RX0 22
#define TX0 21
#define RX1 27
#define TX1 47
#define BUTTON_GPIO 22

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,   // Assuming button connects to GND when pressed
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    twai_general_config_t g0 = TWAI_GENERAL_CONFIG_DEFAULT(TX0, RX0, TWAI_MODE_NORMAL);
    g0.controller_id = 0;
    twai_general_config_t g1 = TWAI_GENERAL_CONFIG_DEFAULT(TX1, RX1, TWAI_MODE_NORMAL);
    g1.controller_id = 1;

    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    twai_handle_t h0, h1;
    ESP_ERROR_CHECK(twai_driver_install_v2(&g0, &t, &f, &h0));
    ESP_ERROR_CHECK(twai_driver_install_v2(&g1, &t, &f, &h1));

    ESP_ERROR_CHECK(twai_start_v2(h0));
    ESP_ERROR_CHECK(twai_start_v2(h1));

    bool button_was_pressed = false;

    while (true) {
        bool button_pressed = gpio_get_level(BUTTON_GPIO) == 0;
        if (button_pressed && !button_was_pressed) {
            // Rising edge detected (button just pressed)
            twai_message_t button_msg = {
                .identifier = 0x200,
                .extd = 0,
                .rtr = 0,
                .data_length_code = 4,
                .data = {0xFF, 0xFF, 0xFF, 0xFF}
            };
            if (twai_transmit_v2(h1, &button_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
                printf("Button pressed! Sent message: ID=0x200 Data=FF FF FF FF\n");
            } else {
                printf("Failed to send button press message.\n");
            }
        }
        button_was_pressed = button_pressed;

        twai_message_t msg;
        if (twai_receive_v2(h0, &msg, pdMS_TO_TICKS(3000)) == ESP_OK) {
            printf("Forwarding message: ID=0x%lX Data=", (unsigned long)msg.identifier);
            for (int i = 0; i < msg.data_length_code; i++) {
                printf("%02X ", msg.data[i]);
            }
            printf("\n");

            // Try to forward the message
            if (msg.identifier <= 0x105) {
                if (msg.identifier == 0x100 && msg.data_length_code >= 4) {
                    msg.data[0] = 0x00;
                }
                if (msg.identifier == 0x102 && msg.data_length_code >= 4) {
                    msg.data[3] = 0xFF;
                }

                if (twai_transmit_v2(h1, &msg, pdMS_TO_TICKS(1000)) != ESP_OK) {
                    esp_err_t result;
                    twai_status_info_t status;
                    result = twai_get_status_info_v2(h1, &status);

                    if (result == ESP_OK) {
                        printf("Result is OK");
                    }
                    if (result == ESP_ERR_INVALID_ARG) {
                        printf("Results has invalid args");
                    }
                    if (result == ESP_ERR_INVALID_STATE) {
                        printf("Results has invalid state");
                    }
                } 
            } else {
                printf("ID out of range\n");        
            }

        } else {
            printf("No message received on TWAI0\n");
        }
    }
}
