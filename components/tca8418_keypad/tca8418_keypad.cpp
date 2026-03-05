#include "tca8418_keypad.h"
#include "esphome/core/log.h"

namespace esphome::tca8418_keypad {

static const char *const TAG = "tca8418_keypad";

void TCA8418KeypadComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TCA8418 Keypad...");

  // Configure interrupt pin if specified
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    ESP_LOGD(TAG, "Interrupt pin configured");
  }

  // Test I2C communication by reading a register
  uint8_t test_val;
  if (!this->read_register_(TCA8418_REG_CFG, &test_val)) {
    ESP_LOGE(TAG, "Failed to communicate with TCA8418 at address 0x%02X", this->address_);
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "Initial CFG register: 0x%02X", test_val);

  // Flush any pending events and clear interrupts
  this->flush_events();

  // Configure the keypad matrix
  this->configure_keypad_matrix_();

  // Enable debouncing
  this->enable_debounce();

  // Enable interrupts
  this->enable_interrupts();

  ESP_LOGCONFIG(TAG, "TCA8418 Keypad setup complete (%dx%d matrix)", this->rows_, this->cols_);
}

void TCA8418KeypadComponent::configure_keypad_matrix_() {
  // Based on Adafruit library matrix() function
  // KP_GPIO registers determine which pins are keypad matrix vs GPIO
  // KP_GPIO1: ROW7-ROW0 (bits 7-0) - rows go in register 1
  // KP_GPIO2: COL7-COL0 (bits 7-0) - columns start in register 2
  // KP_GPIO3: COL9-COL8 (bits 1-0) - extra columns in register 3

  // Default to OMOTE configuration if not specified
  if (this->rows_ == 0) {
    this->rows_ = 6;
  }
  if (this->cols_ == 0) {
    this->cols_ = 4;
  }

  // Validate ranges
  if (this->rows_ > 8) {
    ESP_LOGW(TAG, "Rows capped at 8 (was %d)", this->rows_);
    this->rows_ = 8;
  }
  if (this->cols_ > 10) {
    ESP_LOGW(TAG, "Columns capped at 10 (was %d)", this->cols_);
    this->cols_ = 10;
  }

  // Configure ROW pins (KP_GPIO1) - use lowest row pins
  uint8_t kp_gpio1 = (1 << this->rows_) - 1;
  if (!this->write_register_(TCA8418_REG_KP_GPIO1, kp_gpio1)) {
    ESP_LOGE(TAG, "Failed to write KP_GPIO1");
    return;
  }
  ESP_LOGD(TAG, "KP_GPIO1 (rows): 0x%02X (%d rows)", kp_gpio1, this->rows_);

  // Configure COL pins (KP_GPIO2 and KP_GPIO3) - use lowest col pins
  uint8_t kp_gpio2 = 0;
  uint8_t kp_gpio3 = 0;

  if (this->cols_ <= 8) {
    kp_gpio2 = (1 << this->cols_) - 1;
  } else {
    kp_gpio2 = 0xFF;  // All 8 columns in KP_GPIO2
    kp_gpio3 = (1 << (this->cols_ - 8)) - 1;  // Remaining in KP_GPIO3
  }

  if (!this->write_register_(TCA8418_REG_KP_GPIO2, kp_gpio2)) {
    ESP_LOGE(TAG, "Failed to write KP_GPIO2");
    return;
  }
  ESP_LOGD(TAG, "KP_GPIO2 (cols 0-7): 0x%02X", kp_gpio2);

  if (!this->write_register_(TCA8418_REG_KP_GPIO3, kp_gpio3)) {
    ESP_LOGE(TAG, "Failed to write KP_GPIO3");
    return;
  }
  ESP_LOGD(TAG, "KP_GPIO3 (cols 8-9): 0x%02X", kp_gpio3);

  // Set remaining pins as GPIO inputs with interrupts
  // Based on Adafruit library - configure non-matrix pins as inputs
  uint8_t remaining_rows = 0xFF ^ kp_gpio1;  // Rows not used for matrix
  uint8_t remaining_cols2 = 0xFF ^ kp_gpio2; // Cols 0-7 not used for matrix
  uint8_t remaining_cols3 = 0x03 ^ kp_gpio3; // Cols 8-9 not used for matrix

  // Set direction to input (0 = input)
  this->write_register_(TCA8418_REG_GPIO_DIR1, 0x00);
  this->write_register_(TCA8418_REG_GPIO_DIR2, 0x00);
  this->write_register_(TCA8418_REG_GPIO_DIR3, 0x00);

  // Enable pull-ups on GPIO pins
  this->write_register_(TCA8418_REG_GPIO_PULL1, remaining_rows);
  this->write_register_(TCA8418_REG_GPIO_PULL2, remaining_cols2);
  this->write_register_(TCA8418_REG_GPIO_PULL3, remaining_cols3);

  // Enable GPI event mode for non-matrix pins (they generate key events too)
  this->write_register_(TCA8418_REG_GPI_EM1, remaining_rows);
  this->write_register_(TCA8418_REG_GPI_EM2, remaining_cols2);
  this->write_register_(TCA8418_REG_GPI_EM3, remaining_cols3);

  // Set interrupt level to FALLING (0 = falling edge)
  this->write_register_(TCA8418_REG_GPIO_INT_LVL1, 0x00);
  this->write_register_(TCA8418_REG_GPIO_INT_LVL2, 0x00);
  this->write_register_(TCA8418_REG_GPIO_INT_LVL3, 0x00);

  // Enable interrupts on non-matrix GPIO pins
  this->write_register_(TCA8418_REG_GPIO_INT_EN1, remaining_rows);
  this->write_register_(TCA8418_REG_GPIO_INT_EN2, remaining_cols2);
  this->write_register_(TCA8418_REG_GPIO_INT_EN3, remaining_cols3);
}

void TCA8418KeypadComponent::enable_interrupts() {
  uint8_t cfg;
  if (!this->read_register_(TCA8418_REG_CFG, &cfg)) {
    ESP_LOGE(TAG, "Failed to read CFG for enable_interrupts");
    return;
  }
  // Enable key event interrupt, GPI interrupt, overflow interrupt
  cfg |= TCA8418_CFG_KE_IEN | TCA8418_CFG_GPI_IEN | TCA8418_CFG_OVR_FLOW_IEN;
  // Set INT_CFG to reassert interrupt after 50us
  cfg |= TCA8418_CFG_INT_CFG;
  if (!this->write_register_(TCA8418_REG_CFG, cfg)) {
    ESP_LOGE(TAG, "Failed to write CFG for enable_interrupts");
    return;
  }
  ESP_LOGD(TAG, "Interrupts enabled, CFG: 0x%02X", cfg);
}

void TCA8418KeypadComponent::disable_interrupts() {
  uint8_t cfg;
  if (!this->read_register_(TCA8418_REG_CFG, &cfg)) {
    return;
  }
  cfg &= ~(TCA8418_CFG_KE_IEN | TCA8418_CFG_GPI_IEN);
  this->write_register_(TCA8418_REG_CFG, cfg);
}

void TCA8418KeypadComponent::enable_debounce() {
  // Enable debouncing on all pins (write 0 to disable registers)
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS1, 0x00);
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS2, 0x00);
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS3, 0x00);
  ESP_LOGD(TAG, "Debouncing enabled on all pins");
}

void TCA8418KeypadComponent::disable_debounce() {
  // Disable debouncing on all pins (write 0xFF to disable registers)
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS1, 0xFF);
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS2, 0xFF);
  this->write_register_(TCA8418_REG_DEBOUNCE_DIS3, 0xFF);
}

uint8_t TCA8418KeypadComponent::get_event_count() {
  uint8_t count;
  if (!this->read_register_(TCA8418_REG_KEY_LCK_EC, &count)) {
    return 0;
  }
  return count & 0x0F;  // Lower 4 bits = event count
}

uint8_t TCA8418KeypadComponent::get_event() {
  uint8_t event;
  if (!this->read_register_(TCA8418_REG_KEY_EVENT_A, &event)) {
    return 0;
  }
  return event;
}

void TCA8418KeypadComponent::flush_events() {
  uint8_t count = 0;

  // Read and discard all pending key events
  uint8_t event;
  while (this->read_register_(TCA8418_REG_KEY_EVENT_A, &event) && event != 0) {
    count++;
    if (count > 10) break;  // Safety limit
  }

  // Clear GPIO interrupt status registers
  this->write_register_(TCA8418_REG_GPIO_INT_STAT1, 0xFF);
  this->write_register_(TCA8418_REG_GPIO_INT_STAT2, 0xFF);
  this->write_register_(TCA8418_REG_GPIO_INT_STAT3, 0xFF);

  // Clear interrupt status register
  uint8_t int_stat;
  if (this->read_register_(TCA8418_REG_INT_STAT, &int_stat)) {
    this->write_register_(TCA8418_REG_INT_STAT, int_stat);
  }

  if (count > 0) {
    ESP_LOGD(TAG, "Flushed %d pending events", count);
  }
}

void TCA8418KeypadComponent::loop() {
  // Check interrupt pin if configured (active low)
  if (this->interrupt_pin_ != nullptr) {
    if (this->interrupt_pin_->digital_read()) {
      // Interrupt not asserted (active low), no events pending
      return;
    }
  }

  // Check for key events
  uint8_t int_stat;
  if (!this->read_register_(TCA8418_REG_INT_STAT, &int_stat)) {
    return;
  }

  // Handle key events
  if (int_stat & TCA8418_INT_K_INT) {
    this->process_key_events_();
    // Clear the key event interrupt
    this->write_register_(TCA8418_REG_INT_STAT, TCA8418_INT_K_INT);
  }

  // Handle GPI events (non-matrix GPIO pins)
  if (int_stat & TCA8418_INT_GPI_INT) {
    this->process_key_events_();
    // Clear GPIO interrupt status
    this->write_register_(TCA8418_REG_GPIO_INT_STAT1, 0xFF);
    this->write_register_(TCA8418_REG_GPIO_INT_STAT2, 0xFF);
    this->write_register_(TCA8418_REG_GPIO_INT_STAT3, 0xFF);
    // Clear the GPI interrupt
    this->write_register_(TCA8418_REG_INT_STAT, TCA8418_INT_GPI_INT);
  }

  // Handle overflow
  if (int_stat & TCA8418_INT_OVR_FLOW_INT) {
    ESP_LOGW(TAG, "Key event FIFO overflow!");
    this->flush_events();
    this->write_register_(TCA8418_REG_INT_STAT, TCA8418_INT_OVR_FLOW_INT);
  }
}

void TCA8418KeypadComponent::process_key_events_() {
  uint8_t event_count = this->get_event_count();

  if (event_count > 0) {
    ESP_LOGD(TAG, "Processing %d key events", event_count);
  }

  // Read all pending key events from FIFO
  uint8_t key_event;
  uint8_t processed = 0;

  while ((key_event = this->get_event()) != 0 && processed < 10) {
    processed++;

    bool pressed = (key_event & TCA8418_KEY_PRESS_MASK) != 0;
    uint8_t key_code = key_event & TCA8418_KEY_CODE_MASK;

    // Key codes:
    // 1-80: Matrix keys (row * 10 + col + 1)
    // 97-104: ROW0-ROW7 GPIO keys
    // 105-114: COL0-COL9 GPIO keys

    if (key_code >= 1 && key_code <= 80) {
      // Matrix key - decode row and column
      uint8_t row = (key_code - 1) / 10;
      uint8_t col = (key_code - 1) % 10;
      ESP_LOGI(TAG, "Matrix key: code=%d (R%d,C%d) %s",
               key_code, row, col, pressed ? "PRESSED" : "RELEASED");
    } else if (key_code >= 97 && key_code <= 104) {
      // ROW GPIO key
      uint8_t row = key_code - 97;
      ESP_LOGI(TAG, "GPIO ROW%d key: code=%d %s",
               row, key_code, pressed ? "PRESSED" : "RELEASED");
    } else if (key_code >= 105 && key_code <= 114) {
      // COL GPIO key
      uint8_t col = key_code - 105;
      ESP_LOGI(TAG, "GPIO COL%d key: code=%d %s",
               col, key_code, pressed ? "PRESSED" : "RELEASED");
    } else {
      ESP_LOGW(TAG, "Unknown key code: %d %s",
               key_code, pressed ? "PRESSED" : "RELEASED");
    }

    // Notify listeners
    for (auto *listener : this->listeners_) {
      if (pressed) {
        listener->key_pressed(key_code);
      } else {
        listener->key_released(key_code);
      }
    }
  }
}

void TCA8418KeypadComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "TCA8418 Keypad:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGCONFIG(TAG, "  Communication FAILED!");
    return;
  }
  ESP_LOGCONFIG(TAG, "  Rows: %d", this->rows_);
  ESP_LOGCONFIG(TAG, "  Columns: %d", this->cols_);
  ESP_LOGCONFIG(TAG, "  Matrix size: %d keys (codes 1-%d)",
                this->rows_ * this->cols_, this->rows_ * this->cols_);
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  } else {
    ESP_LOGCONFIG(TAG, "  Interrupt Pin: Not configured (polling mode)");
  }

  // Read and display current configuration
  uint8_t cfg;
  if (this->read_register_(TCA8418_REG_CFG, &cfg)) {
    ESP_LOGCONFIG(TAG, "  CFG Register: 0x%02X", cfg);
  }
}

bool TCA8418KeypadComponent::write_register_(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

bool TCA8418KeypadComponent::read_register_(uint8_t reg, uint8_t *value) {
  return this->read_byte(reg, value);
}

}  // namespace esphome::tca8418_keypad
