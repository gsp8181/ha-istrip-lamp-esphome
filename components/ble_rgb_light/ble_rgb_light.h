#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace empty_light {

class EmptyLightOutput : public light::LightOutput, public Component {
 public:
  void setup() override;
  void loop() override;
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;
  void write_state_effect(uint8_t light, uint8_t speed, uint8_t effect);
  void dump_config() override;
 };

} //namespace empty_light
} //namespace esphome