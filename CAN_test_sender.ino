#include "driver/twai.h"

#define CAN_TX_GPIO (gpio_num_t)15
#define CAN_RX_GPIO (gpio_num_t)16

uint32_t getNextCanId() {
  static uint8_t index = 0;
  uint32_t baseId = 0x100;
  uint32_t id = baseId + index;
  index = (index + 1) % 10;  // Cycle back to 0 after 9
  return id;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure TWAI (CAN)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install and start TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("TWAI driver installed.");
  } else {
    Serial.println("Failed to install TWAI driver.");
    while (1);
  }

  if (twai_start() == ESP_OK) {
    Serial.println("TWAI driver started.");
  } else {
    Serial.println("Failed to start TWAI driver.");
    while (1);
  }

  // Seed random number generator
  randomSeed(esp_random());
}

void loop() {
  // Create message with 4 random bytes
  twai_message_t message;
  message.identifier = getNextCanId();
  message.extd = 0;  // Standard ID
  message.rtr = 0;   // Data frame, not remote request
  message.data_length_code = 4;
  for (int i = 0; i < 4; i++) {
    message.data[i] = random(0, 256);
  }

  // Send message
  if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
    Serial.print("Sent CAN message: ID=0x");
    Serial.print(message.identifier, HEX);
    Serial.print(" Data=");
    for (int i = 0; i < message.data_length_code; i++) {
      Serial.printf("%02X ", message.data[i]);
    }
    Serial.println();
  } else {
    Serial.println("Failed to send message.");
  }

  // Wait 1 second
  vTaskDelay(pdMS_TO_TICKS(50));
}