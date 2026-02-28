import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from .. import tca8418_keypad_ns, TCA8418KeypadComponent, CONF_TCA8418_ID

DEPENDENCIES = ["tca8418_keypad"]

CONF_KEY = "key"
CONF_ROW = "row"
CONF_COL = "col"

TCA8418KeypadBinarySensor = tca8418_keypad_ns.class_(
    "TCA8418KeypadBinarySensor", binary_sensor.BinarySensor, cg.Component
)


def validate_key_config(config):
    """Validate that either key OR row+col is specified, not both."""
    has_key = CONF_KEY in config
    has_row = CONF_ROW in config
    has_col = CONF_COL in config

    if has_key and (has_row or has_col):
        raise cv.Invalid("Cannot specify both 'key' and 'row'/'col'. Use one or the other.")
    if has_row != has_col:
        raise cv.Invalid("Both 'row' and 'col' must be specified together.")
    if not has_key and not has_row:
        raise cv.Invalid("Must specify either 'key' or both 'row' and 'col'.")

    return config


CONFIG_SCHEMA = cv.All(
    binary_sensor.binary_sensor_schema(TCA8418KeypadBinarySensor)
    .extend(
        {
            cv.GenerateID(CONF_TCA8418_ID): cv.use_id(TCA8418KeypadComponent),
            cv.Optional(CONF_KEY): cv.int_range(min=1, max=114),
            cv.Optional(CONF_ROW): cv.int_range(min=0, max=7),
            cv.Optional(CONF_COL): cv.int_range(min=0, max=9),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_key_config,
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_TCA8418_ID])
    cg.add(var.set_parent(hub))

    if CONF_KEY in config:
        cg.add(var.set_key(config[CONF_KEY]))
    else:
        # Calculate key from row and col
        # Matrix key formula: (row * 10) + col + 1
        row = config[CONF_ROW]
        col = config[CONF_COL]
        key = (row * 10) + col + 1
        cg.add(var.set_key(key))
