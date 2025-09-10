#include "ble_rgb_effect.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace rgb_effect
  {

    static const char *TAG = "rgb_effect";

    void RgbEffectComponent::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up RGB Effect Component");
    }

    rgb_light::RGBLightOutput *RgbEffectComponent::get_light_output()
    {
      if (light_ == nullptr)
      {
        ESP_LOGW(TAG, "Light not set");
        return nullptr;
      }

      auto *output = light_->get_output();
      auto *rgb_output = static_cast<rgb_light::RGBLightOutput *>(output);
      return rgb_output;
    }

    void RgbEffectComponent::send_effect(uint8_t brightness, uint8_t speed, uint8_t effect)
    {
      auto *output = get_light_output();
      if (output != nullptr)
      {
        ESP_LOGD(TAG, "Sending effect: brightness=%d, speed=%d, effect=%d", brightness, speed, effect);
        output->write_state_effect(brightness, speed, effect);
      }
      else
      {
        ESP_LOGW(TAG, "Cannot send effect - light output not available");
      }
    }

  } // namespace rgb_effect
} // namespace esphome
