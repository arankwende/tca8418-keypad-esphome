import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@omote"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = True
AUTO_LOAD = ["binary_sensor"]

CONF_TCA8418_ID = "tca8418_keypad_id"
CONF_ROWS = "rows"
CONF_COLS = "cols"
CONF_INTERRUPT_PIN = "interrupt_pin"

tca8418_keypad_ns = cg.esphome_ns.namespace("tca8418_keypad")
TCA8418KeypadComponent = tca8418_keypad_ns.class_(
    "TCA8418KeypadComponent", cg.Component, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TCA8418KeypadComponent),
            cv.Optional(CONF_ROWS, default=6): cv.int_range(min=1, max=8),
            cv.Optional(CONF_COLS, default=4): cv.int_range(min=1, max=10),
            cv.Optional(CONF_INTERRUPT_PIN): cv.int_range(min=0, max=48),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x34))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_rows(config[CONF_ROWS]))
    cg.add(var.set_cols(config[CONF_COLS]))

    if CONF_INTERRUPT_PIN in config:
        cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
