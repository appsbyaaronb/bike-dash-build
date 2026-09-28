import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

DEPENDENCIES = ["network", "logger"]

log_ns = cg.esphome_ns.namespace("log_page")
LogPage = log_ns.class_("LogPage", cg.Component)

CONF_BUFFER_SIZE = "buffer_size"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LogPage),
        cv.Optional(CONF_PORT, default=8081): cv.port,
        # Fixed ring buffer: oldest lines are overwritten, memory use never grows.
        cv.Optional(CONF_BUFFER_SIZE, default="32kB"): cv.All(
            cv.validate_bytes, cv.int_range(min=2048, max=262144)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_buffer_size(config[CONF_BUFFER_SIZE]))
