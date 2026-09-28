import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PATH, CONF_PORT
from esphome.core import CORE

DEPENDENCIES = ["network"]
AUTO_LOAD = ["ams"]
CONF_AMS_ID = "ams_id"
CONF_ON_BT_RESTART = "on_bt_restart"
ams_ns = cg.esphome_ns.namespace("ams")
AMSComponent = ams_ns.class_("AMSComponent", cg.Component)

dash_ns = cg.esphome_ns.namespace("dash_page")
DashPage = dash_ns.class_("DashPage", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DashPage),
        cv.Required(CONF_PATH): cv.file_,
        cv.Optional(CONF_PORT, default=8080): cv.port,
        cv.Optional(CONF_AMS_ID): cv.use_id(AMSComponent),
        # Runs just before Bluetooth is switched off by the Restart Bluetooth button.
        cv.Optional(CONF_ON_BT_RESTART): cv.lambda_,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    html = CORE.relative_config_path(config[CONF_PATH]).read_text(encoding="utf-8")
    cg.add_global(cg.RawStatement(f'static const char DASH_HTML[] = R"DASHHTML({html})DASHHTML";'))
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
    if CONF_AMS_ID in config:
        cg.add(var.set_ams(await cg.get_variable(config[CONF_AMS_ID])))
    cg.add(var.set_html(cg.RawExpression("DASH_HTML"), cg.RawExpression("sizeof(DASH_HTML) - 1")))
    if CONF_ON_BT_RESTART in config:
        lam = await cg.process_lambda(config[CONF_ON_BT_RESTART], [], return_type=cg.void)
        cg.add(var.set_on_bt_restart(lam))
