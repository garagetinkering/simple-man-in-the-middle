#include "driver/twai.h"

#define CAN_TX_GPIO (gpio_num_t)5
#define CAN_RX_GPIO (gpio_num_t)4

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure TWAI (CAN)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();  // Accept all IDs

  // Install and start TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI driver installed.");
  } else {
    Serial.println("Failed to install TWAI driver.");
    while (1);
  }

  if (twai_start() == ESP_OK) {
    Serial.println("TWAI driver started. Listening for messages...");
  } else {
    Serial.println("Failed to start TWAI driver.");
    while (1);
  }
}

void loop() {
  twai_message_t message;

  // Wait for a message (1-second timeout)
  if (twai_receive(&message, pdMS_TO_TICKS(3000)) == ESP_OK) {
    Serial.print("Received CAN message: ID=0x");
    Serial.print(message.identifier, HEX);
    Serial.print(" DLC=");
    Serial.print(message.data_length_code);
    Serial.print(" Data=");
    for (int i = 0; i < message.data_length_code; i++) {
      Serial.printf("%02X ", message.data[i]);
    }
    Serial.println();
  } else {
    Serial.println("No CAN message received.");
  }
}
