#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_state.h"
#include "../ble_rgb_light/ble_rgb_light.h"

namespace esphome {
namespace ble_rgb_effect {

class BleRgbEffectComponent : public Component {
 public:
  void setup() override;
  void set_light(light::LightState *light) { light_ = light; }
  void send_effect(uint8_t brightness, uint8_t speed, uint8_t effect);

 protected:
  light::LightState *light_{nullptr};
  empty_light::EmptyLightOutput *get_light_output();
};

} // namespace ble_rgb_effect
} // namespace esphome
