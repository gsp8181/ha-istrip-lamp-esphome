import esphome.config_validation as cv
import esphome.codegen as cg

CODEOWNERS = ["@gsp8181"]
DEPENDENCIES = [ ]
AUTO_LOAD = [ ]
MULTI_CONF = True

# C++ namespace
ns = cg.esphome_ns.namespace("ble_rgb_light")
MyExternalComponent = ns.class_("BleRgbLight", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MyExternalComponent),
    # Schema definition, containing the options available for the component
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    # Code generation
    pass