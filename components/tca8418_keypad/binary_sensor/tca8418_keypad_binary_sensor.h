#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../tca8418_keypad.h"

namespace esphome {
namespace tca8418_keypad {

class TCA8418KeypadBinarySensor : public binary_sensor::BinarySensor,
                                   public Component,
                                   public TCA8418KeypadListener {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_parent(TCA8418KeypadComponent *parent) { this->parent_ = parent; }
  void set_key(uint8_t key) { this->key_ = key; }

  void key_pressed(uint8_t key) override;
  void key_released(uint8_t key) override;

 protected:
  TCA8418KeypadComponent *parent_{nullptr};
  uint8_t key_{0};
};

}  // namespace tca8418_keypad
}  // namespace esphome
