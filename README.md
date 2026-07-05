# tca8418-keypad-esphome

ESPHome external component for the TI **TCA8418** I2C keypad scan IC
(the keypad controller used in the [OMOTE](https://github.com/CoretechR/OMOTE)
remote).

The TCA8418 scans up to an 8x10 key matrix in hardware, debounces it, queues
events in a 10-event FIFO and asserts an active-low interrupt — so the ESP
does no matrix scanning at all. Pins not used by the matrix become GPIs that
also generate key events (great for extra buttons like OMOTE's power key).

## Usage

```yaml
external_components:
  - source: github://arankwende/tca8418-keypad-esphome@main

i2c:
  sda: GPIO20
  scl: GPIO19

tca8418_keypad:
  id: keypad
  address: 0x34
  rows: 5
  cols: 5
  interrupt_pin:            # optional but recommended (skips I2C polling)
    number: GPIO08
    mode:
      input: true
      pullup: true          # INT is open-drain active-low

binary_sensor:
  # Matrix key by row/column
  - platform: tca8418_keypad
    tca8418_keypad_id: keypad
    row: 0
    col: 1
    name: "Play Pause"
    on_press:
      - logger.log: "pressed!"

  # Non-matrix GPI key by raw key code
  # (ROW0-ROW7 = 97-104, COL0-COL9 = 105-114)
  - platform: tca8418_keypad
    tca8418_keypad_id: keypad
    key: 102               # ROW5 GPI
    name: "Power Button"
```

## Configuration variables

### `tca8418_keypad`

- **address** (Optional, default `0x34`): I2C address.
- **rows** (Optional, 1–8, default `6`): matrix rows (ROW0..ROWn-1).
- **cols** (Optional, 1–10, default `4`): matrix columns (COL0..COLn-1).
- **interrupt_pin** (Optional): GPIO wired to the TCA8418 INT output.
  When set, the bus is only read after INT asserts; otherwise the
  component polls over I2C every loop.

### `binary_sensor`

Specify **either** `row` **and** `col` (matrix keys) **or** `key`
(raw key code 1–114, needed for GPI keys).

Matrix key codes follow the datasheet layout: `key = row * 10 + col + 1`.

## Deep sleep / wake on key press

Because INT is active-low and idles high, the same pin can be an ext0
wakeup source so any key wakes the device:

```yaml
deep_sleep:
  wakeup_pin:
    number: GPIO08
    inverted: true
    mode: INPUT_PULLUP
    allow_other_uses: true
```

Before entering deep sleep, drain the FIFO so a pending (release) event
doesn't hold INT low and wake you straight back up:

```yaml
- lambda: 'id(keypad).flush_events();'
- deep_sleep.enter: sleep_ctrl
```

## Lambda API

| Method | Purpose |
|---|---|
| `id(keypad).flush_events();` | Drain the event FIFO and clear interrupts |
| `id(keypad).disable_interrupts();` / `enable_interrupts();` | Mask/unmask key event interrupts |
| `id(keypad).disable_debounce();` / `enable_debounce();` | Control the hardware debouncer |
