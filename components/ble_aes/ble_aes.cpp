#include "ble_aes.h"
#include "esphome/core/log.h"
#include <mbedtls/aes.h>

namespace esphome {
namespace ble_aes {

static const char *TAG = "ble_aes";

// iStrip AES encryption key
const uint8_t BleAesComponent::AES_KEY[16] = {
  52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248
};

void BleAesComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BLE AES Component");
}

void BleAesComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE AES Component");
}

std::vector<uint8_t> BleAesComponent::encrypt(const std::vector<uint8_t> &data) {
  // Pad data to 16-byte blocks
  size_t padded_size = ((data.size() + 15) / 16) * 16;
  std::vector<uint8_t> padded_data(padded_size, 0);
  
  // Copy original data
  for (size_t i = 0; i < data.size(); i++) {
    padded_data[i] = data[i];
  }
  
  // Encrypt in 16-byte blocks
  std::vector<uint8_t> encrypted(padded_size);
  mbedtls_aes_context aes;
  
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);
  
  for (size_t i = 0; i < padded_size; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, 
                          &padded_data[i], &encrypted[i]);
  }
  
  mbedtls_aes_free(&aes);
  return encrypted;
}

void BleAesComponent::encrypt_16(const uint8_t *input, uint8_t *output) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
  mbedtls_aes_free(&aes);
}

} // namespace ble_aes
} // namespace esphome
