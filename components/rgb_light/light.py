from logging import config
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, ble_client
from esphome.const import CONF_OUTPUT_ID, CONF_ID
from esphome import automation

CONF_BLE_CLIENT_ID = "ble_client_id"
CONF_BLE_AES_ID = "ble_aes_id"
CONF_WRITE_ACTION = "write_action"

rgb_light_ns = cg.esphome_ns.namespace('rgb_light')
RGBLightOutput = rgb_light_ns.class_('RGBLightOutput', light.LightOutput, cg.Component, ble_client.BLEClientNode)

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(RGBLightOutput),
    cv.Optional(CONF_WRITE_ACTION): automation.validate_automation(single=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)
    
    if CONF_WRITE_ACTION in config:
        await automation.build_automation(var.get_write_trigger(), [(cg.std_vector.template(cg.uint8), "value")], config[CONF_WRITE_ACTION])