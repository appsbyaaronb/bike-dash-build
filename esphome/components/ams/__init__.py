import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32_ble"]
CODEOWNERS = []

ams_ns = cg.esphome_ns.namespace("ams")
AMSComponent = ams_ns.class_("AMSComponent", cg.Component)

CONF_DEVICE_NAME = "device_name"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AMSComponent),
        cv.GenerateID(esp32_ble.CONF_BLE_ID): cv.use_id(esp32_ble.ESP32BLE),
        cv.Optional(CONF_DEVICE_NAME, default="Bike Dash"): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[esp32_ble.CONF_BLE_ID])
    esp32_ble.register_gap_event_handler(parent, var)
    esp32_ble.register_gattc_event_handler(parent, var)
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))
