#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/automation.h"

namespace esphome
{
    namespace rgb_light
    {

        class RGBLightOutput : public light::LightOutput, public Component
        {
        public:
            void setup() override;
            void loop() override;
            light::LightTraits get_traits() override;
            void write_state(light::LightState *state) override;
            void write_state_effect(uint8_t light, uint8_t speed, uint8_t effect);
            void dump_config() override;

            void set_write_action(Action<std::vector<uint8_t>> *action) { write_action_ = action; }
            Trigger<std::vector<uint8_t>> *get_write_trigger() { return write_trigger_; }

        protected:
            Action<std::vector<uint8_t>> *write_action_{nullptr};
            Trigger<std::vector<uint8_t>> *write_trigger_{new Trigger<std::vector<uint8_t>>()};

            void send_command(const std::vector<uint8_t> &cmd);
        };

    } // namespace rgb_light
} // namespace esphome