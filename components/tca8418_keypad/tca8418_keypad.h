#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include <vector>

namespace esphome::tca8418_keypad {

// TCA8418 I2C Default Address
static const uint8_t TCA8418_DEFAULT_ADDR = 0x34;

// TCA8418 Register Addresses (from Adafruit library)
static const uint8_t TCA8418_REG_CFG = 0x01;
static const uint8_t TCA8418_REG_INT_STAT = 0x02;
static const uint8_t TCA8418_REG_KEY_LCK_EC = 0x03;
static const uint8_t TCA8418_REG_KEY_EVENT_A = 0x04;
static const uint8_t TCA8418_REG_KEY_EVENT_B = 0x05;
static const uint8_t TCA8418_REG_KEY_EVENT_C = 0x06;
static const uint8_t TCA8418_REG_KEY_EVENT_D = 0x07;
static const uint8_t TCA8418_REG_KEY_EVENT_E = 0x08;
static const uint8_t TCA8418_REG_KEY_EVENT_F = 0x09;
static const uint8_t TCA8418_REG_KEY_EVENT_G = 0x0A;
static const uint8_t TCA8418_REG_KEY_EVENT_H = 0x0B;
static const uint8_t TCA8418_REG_KEY_EVENT_I = 0x0C;
static const uint8_t TCA8418_REG_KEY_EVENT_J = 0x0D;
static const uint8_t TCA8418_REG_KP_LCK_TIMER = 0x0E;
static const uint8_t TCA8418_REG_UNLOCK1 = 0x0F;
static const uint8_t TCA8418_REG_UNLOCK2 = 0x10;
static const uint8_t TCA8418_REG_GPIO_INT_STAT1 = 0x11;
static const uint8_t TCA8418_REG_GPIO_INT_STAT2 = 0x12;
static const uint8_t TCA8418_REG_GPIO_INT_STAT3 = 0x13;
static const uint8_t TCA8418_REG_GPIO_DAT_STAT1 = 0x14;
static const uint8_t TCA8418_REG_GPIO_DAT_STAT2 = 0x15;
static const uint8_t TCA8418_REG_GPIO_DAT_STAT3 = 0x16;
static const uint8_t TCA8418_REG_GPIO_DAT_OUT1 = 0x17;
static const uint8_t TCA8418_REG_GPIO_DAT_OUT2 = 0x18;
static const uint8_t TCA8418_REG_GPIO_DAT_OUT3 = 0x19;
static const uint8_t TCA8418_REG_GPIO_INT_EN1 = 0x1A;
static const uint8_t TCA8418_REG_GPIO_INT_EN2 = 0x1B;
static const uint8_t TCA8418_REG_GPIO_INT_EN3 = 0x1C;
static const uint8_t TCA8418_REG_KP_GPIO1 = 0x1D;
static const uint8_t TCA8418_REG_KP_GPIO2 = 0x1E;
static const uint8_t TCA8418_REG_KP_GPIO3 = 0x1F;
static const uint8_t TCA8418_REG_GPI_EM1 = 0x20;
static const uint8_t TCA8418_REG_GPI_EM2 = 0x21;
static const uint8_t TCA8418_REG_GPI_EM3 = 0x22;
static const uint8_t TCA8418_REG_GPIO_DIR1 = 0x23;
static const uint8_t TCA8418_REG_GPIO_DIR2 = 0x24;
static const uint8_t TCA8418_REG_GPIO_DIR3 = 0x25;
static const uint8_t TCA8418_REG_GPIO_INT_LVL1 = 0x26;
static const uint8_t TCA8418_REG_GPIO_INT_LVL2 = 0x27;
static const uint8_t TCA8418_REG_GPIO_INT_LVL3 = 0x28;
static const uint8_t TCA8418_REG_DEBOUNCE_DIS1 = 0x29;
static const uint8_t TCA8418_REG_DEBOUNCE_DIS2 = 0x2A;
static const uint8_t TCA8418_REG_DEBOUNCE_DIS3 = 0x2B;
static const uint8_t TCA8418_REG_GPIO_PULL1 = 0x2C;
static const uint8_t TCA8418_REG_GPIO_PULL2 = 0x2D;
static const uint8_t TCA8418_REG_GPIO_PULL3 = 0x2E;

// Configuration register bits
static const uint8_t TCA8418_CFG_KE_IEN = 0x01;       // Key events interrupt enable
static const uint8_t TCA8418_CFG_GPI_IEN = 0x02;     // GPI interrupt enable
static const uint8_t TCA8418_CFG_K_LCK_IEN = 0x04;   // Keylock interrupt enable
static const uint8_t TCA8418_CFG_OVR_FLOW_IEN = 0x08; // Overflow interrupt enable
static const uint8_t TCA8418_CFG_INT_CFG = 0x10;     // Interrupt config (1=reassert after 50us)
static const uint8_t TCA8418_CFG_OVR_FLOW_M = 0x20;  // Overflow mode
static const uint8_t TCA8418_CFG_GPI_E_CFG = 0x40;   // GPI event mode config
static const uint8_t TCA8418_CFG_AI = 0x80;          // Auto-increment

// Interrupt status register bits
static const uint8_t TCA8418_INT_K_INT = 0x01;       // Key event interrupt
static const uint8_t TCA8418_INT_GPI_INT = 0x02;     // GPI interrupt
static const uint8_t TCA8418_INT_K_LCK_INT = 0x04;   // Keylock interrupt
static const uint8_t TCA8418_INT_OVR_FLOW_INT = 0x08; // Overflow interrupt
static const uint8_t TCA8418_INT_CAD_INT = 0x10;     // CTRL-ALT-DEL interrupt

// Key event codes
// Matrix keys: 1-80 (row * 10 + col + 1)
// GPIO keys: 97-114 (ROW0-ROW7 = 97-104, COL0-COL9 = 105-114)
// Bit 7 of event indicates press (1) or release (0)
static const uint8_t TCA8418_KEY_PRESS_MASK = 0x80;
static const uint8_t TCA8418_KEY_CODE_MASK = 0x7F;

class TCA8418KeypadListener {
 public:
  virtual void key_pressed(uint8_t key) = 0;
  virtual void key_released(uint8_t key) = 0;
};

class TCA8418KeypadComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::IO; }

  void set_rows(uint8_t rows) { this->rows_ = rows; }
  void set_cols(uint8_t cols) { this->cols_ = cols; }
  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }

  void register_listener(TCA8418KeypadListener *listener) { this->listeners_.push_back(listener); }

  // Public methods for advanced use
  uint8_t get_event_count();
  uint8_t get_event();
  void flush_events();
  void enable_interrupts();
  void disable_interrupts();
  void enable_debounce();
  void disable_debounce();

 protected:
  bool write_register_(uint8_t reg, uint8_t value);
  bool read_register_(uint8_t reg, uint8_t *value);
  void process_key_events_();
  void configure_keypad_matrix_();

  uint8_t rows_{0};  // 0 = auto-detect based on OMOTE (6 rows)
  uint8_t cols_{0};  // 0 = auto-detect based on OMOTE (4 cols)
  InternalGPIOPin *interrupt_pin_{nullptr};
  std::vector<TCA8418KeypadListener *> listeners_;
};

}  // namespace esphome::tca8418_keypad