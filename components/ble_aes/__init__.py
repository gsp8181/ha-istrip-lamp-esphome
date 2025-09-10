import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

ble_aes_ns = cg.esphome_ns.namespace('ble_aes')
BleAesComponent = ble_aes_ns.class_('BleAesComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BleAesComponent),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
