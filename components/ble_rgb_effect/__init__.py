import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_ID

DEPENDENCIES = ["ble_rgb_light"]
AUTO_LOAD = ["ble_rgb_light"]

ble_rgb_effect_ns = cg.esphome_ns.namespace('ble_rgb_effect')
BleRgbEffectComponent = ble_rgb_effect_ns.class_('BleRgbEffectComponent', cg.Component)

CONF_LIGHT_ID = "light_id"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BleRgbEffectComponent),
    cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    light_var = await cg.get_variable(config[CONF_LIGHT_ID])
    cg.add(var.set_light(light_var))
