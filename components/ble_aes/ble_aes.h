#pragma once

#include "esphome/core/component.h"
#include <vector>
#include <cstdint>

namespace esphome {
namespace ble_aes {

class BleAesComponent : public Component {
 public:
  void setup() override;
  void dump_config() override;
  
  // Encrypt data using AES-128 ECB with iStrip key
  std::vector<uint8_t> encrypt(const std::vector<uint8_t> &data);
  
  // Convenience function for 16-byte arrays
  void encrypt_16(const uint8_t *input, uint8_t *output);

 private:
  static const uint8_t AES_KEY[16];
};

} // namespace ble_aes
} // namespace esphome
