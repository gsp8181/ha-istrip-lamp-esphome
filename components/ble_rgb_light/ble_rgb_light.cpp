#include "esphome/core/log.h"
#include "ble_rgb_light.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

#include <mbedtls/aes.h>


namespace esphome {
namespace empty_light {

static const char *TAG = "empty_light.light";

// Target BLE info
static const uint8_t TARGET_MANUFACTURER_DATA[] = {0x00, 0x57, 0x00, 0x00, 0x53};
static const uint16_t TARGET_VENDOR_ID = 0x5254;
static const BLEUUID TARGET_CHAR_UUID("0000ac52-1212-efde-1523-785fedbeda25");
static const uint8_t AES_KEY[16] = {52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248};

// Global pointer for BLE characteristic
BLERemoteCharacteristic *target_characteristic = nullptr;

class BleAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
 public:
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Check manufacturer data and vendor ID
    if (advertisedDevice.haveManufacturerData()) {
      String md = advertisedDevice.getManufacturerData();
      // Use length() and c_str() for Arduino String
      if (md.length() >= sizeof(TARGET_MANUFACTURER_DATA) &&
          memcmp(md.c_str(), TARGET_MANUFACTURER_DATA, sizeof(TARGET_MANUFACTURER_DATA)) == 0 /* &&
          (vendor ID check: implement if needed) */ ) {
        ESP_LOGI(TAG, "Found target BLE device, connecting...");
        BLEClient *client = BLEDevice::createClient();
        client->connect(&advertisedDevice);
        BLERemoteService *service = nullptr;
        // Try to get service by characteristic UUID
        for (int i = 0; i < advertisedDevice.getServiceUUIDCount(); ++i) {
          BLEUUID uuid = advertisedDevice.getServiceUUID(i);
          service = client->getService(uuid);
          if (service != nullptr) break;
        }
        if (service != nullptr) {
          BLERemoteCharacteristic *charac = service->getCharacteristic(TARGET_CHAR_UUID);
          if (charac != nullptr) {
            target_characteristic = charac;
            ESP_LOGI(TAG, "Connected and characteristic found!");
          }
        }
        client->disconnect();
      }
    }
  }
};

void EmptyLightOutput::setup() {
    BLEDevice::init("");
    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new BleAdvertisedDeviceCallbacks());
    scan->setActiveScan(true);
    scan->start(5, false); // Scan for 5 seconds
}

light::LightTraits EmptyLightOutput::get_traits() {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS, light::ColorMode::RGB});

    return traits;
}

// Helper: AES ECB encryption
static void aes_encrypt(const uint8_t *input, uint8_t *output) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&aes);
}

// TODO: Store and set target_characteristic from BLE scan/connect logic
extern BLERemoteCharacteristic *target_characteristic;

void EmptyLightOutput::write_state(light::LightState *state) {
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

    // Example: groupId=1, extraParam=1, speed=64, type=0
    uint8_t groupId = 1;
    uint8_t extraParam = 1;
    uint8_t speed = 64;
    uint8_t type = 0;

    uint8_t cmd[16] = {84,82,0,87, 2, groupId, extraParam, r, g, b, light, speed, type, 0, 0, 0};
    uint8_t encrypted[16];
    aes_encrypt(cmd, encrypted);

    if (target_characteristic) {
        target_characteristic->writeValue(encrypted, 16, false);
        ESP_LOGD(TAG, "Sent BLE RGB command: R=%d G=%d B=%d Brightness=%d", r, g, b, light);
    } else {
        ESP_LOGW(TAG, "BLE target characteristic not set, cannot send command");
    }
}

void EmptyLightOutput::dump_config(){
    ESP_LOGCONFIG(TAG, "Empty custom light");
}

} //namespace empty_light
} //namespace esphome