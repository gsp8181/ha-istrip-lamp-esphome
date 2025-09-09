#include "ble_rgb_effect.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ble_rgb_effect {

static const char *TAG = "ble_rgb_effect";

void BleRgbEffectComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BLE RGB Effect Component");
}

empty_light::EmptyLightOutput *BleRgbEffectComponent::get_light_output() {
  if (light_ == nullptr) {
    ESP_LOGW(TAG, "Light not set");
    return nullptr;
  }
  
  auto *output = light_->get_output();
  auto *ble_output = static_cast<empty_light::EmptyLightOutput *>(output);
  return ble_output;
}

void BleRgbEffectComponent::send_effect(uint8_t brightness, uint8_t speed, uint8_t effect) {
  auto *output = get_light_output();
  if (output != nullptr) {
    ESP_LOGD(TAG, "Sending effect: brightness=%d, speed=%d, effect=%d", brightness, speed, effect);
    output->write_state_effect(brightness, speed, effect);
  } else {
    ESP_LOGW(TAG, "Cannot send effect - light output not available");
  }
}

} // namespace ble_rgb_effect
} // namespace esphome
