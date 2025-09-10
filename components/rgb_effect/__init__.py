import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

DEPENDENCIES = ["rgb_light"]
AUTO_LOAD = ["rgb_light"]

ble_rgb_effect_ns = cg.esphome_ns.namespace('rgb_effect')
RgbEffectComponent = ble_rgb_effect_ns.class_('RgbEffectComponent', cg.Component)

CONF_LIGHT_ID = "light_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RgbEffectComponent),
    cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    light_var = await cg.get_variable(config[CONF_LIGHT_ID])
    cg.add(var.set_light(light_var))
