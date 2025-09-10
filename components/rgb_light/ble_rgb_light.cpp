#include "esphome/core/log.h"
#include "ble_rgb_light.h"

namespace esphome
{
    namespace rgb_light
    {

        static const char *TAG = "rgb_light.light";

        light::LightTraits RGBLightOutput::get_traits()
        {
            auto traits = light::LightTraits();
            traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS, light::ColorMode::RGB});
            return traits;
        }

        void RGBLightOutput::setup()
        {
            // Component setup if needed
        }

        void RGBLightOutput::loop()
        {
            // Component loop if needed
        }

        void RGBLightOutput::send_command(const std::vector<uint8_t> &cmd)
        {
            if (write_trigger_ != nullptr)
            {
                write_trigger_->trigger(cmd);
            }
            else
            {
                ESP_LOGW(TAG, "No write action configured");
            }
        }

        void RGBLightOutput::write_state(light::LightState *state)
        {
            if (!state->current_values.is_on())
            {
                // All values zero for off
                std::vector<uint8_t> cmd = {84, 82, 0, 87, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                send_command(cmd);
                ESP_LOGD(TAG, "Sent BLE RGB OFF command (all zeroes)");
                return;
            }

            float red, green, blue, brightness;
            red = state->current_values.get_red();
            green = state->current_values.get_green();
            blue = state->current_values.get_blue();
            brightness = state->current_values.get_brightness();

            // Convert to 0..255
            uint8_t r = (uint8_t)(red * 255);
            uint8_t g = (uint8_t)(green * 255);
            uint8_t b = (uint8_t)(blue * 255);

            // Convert brightness to 0..64 (max light value)
            uint8_t light = (uint8_t)(brightness * 64);

            uint8_t groupId = 1;
            uint8_t extraParam = 0;
            uint8_t speed = 64;
            uint8_t type = 0;

            std::vector<uint8_t> cmd = {84, 82, 0, 87, 2, groupId, extraParam, r, g, b, light, speed, type, 0, 0, 0};
            send_command(cmd);
            ESP_LOGD(TAG, "Sent BLE RGB command: R=%d G=%d B=%d Brightness=%d", r, g, b, light);
        }

        void RGBLightOutput::write_state_effect(uint8_t light, uint8_t speed, uint8_t effect)
        {
            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
            uint8_t groupId = 1;
            uint8_t type = 0;

            std::vector<uint8_t> cmd = {84, 82, 0, 87, 2, groupId, effect, r, g, b, light, speed, type, 0, 0, 0};
            send_command(cmd);
            ESP_LOGD(TAG, "Sent BLE RGB RAW command: R=%d G=%d B=%d Brightness=%d Speed=%d ExtraParam=%d", r, g, b, light, speed, effect);
        }

        void RGBLightOutput::dump_config()
        {
            ESP_LOGCONFIG(TAG, "RGB custom light");
        }

    } // namespace rgb_light
} // namespace esphome