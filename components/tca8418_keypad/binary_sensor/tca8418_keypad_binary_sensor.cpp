#include "tca8418_keypad_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tca8418_keypad {

static const char *const TAG = "tca8418_keypad.binary_sensor";

void TCA8418KeypadBinarySensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TCA8418 binary sensor for key %d", this->key_);
  this->parent_->register_listener(this);
  this->publish_initial_state(false);
}

void TCA8418KeypadBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "TCA8418 Keypad Button", this);
  ESP_LOGCONFIG(TAG, "  Key: %d", this->key_);

  // Decode key to row/col for matrix keys (1-80)
  if (this->key_ >= 1 && this->key_ <= 80) {
    uint8_t row = (this->key_ - 1) / 10;
    uint8_t col = (this->key_ - 1) % 10;
    ESP_LOGCONFIG(TAG, "  Matrix position: Row %d, Col %d", row, col);
  }
}

void TCA8418KeypadBinarySensor::key_pressed(uint8_t key) {
  if (key == this->key_) {
    ESP_LOGD(TAG, "Key %d pressed", key);
    this->publish_state(true);
  }
}

void TCA8418KeypadBinarySensor::key_released(uint8_t key) {
  if (key == this->key_) {
    ESP_LOGD(TAG, "Key %d released", key);
    this->publish_state(false);
  }
}

}  // namespace tca8418_keypad
}  // namespace esphome
