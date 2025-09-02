#include "esphome.h"
#include <mbedtls/aes.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>

class BleRgbLight : public Component, public LightOutput {
 public:
  BleRgbLight() {}

  void setup() override {
    BLEDevice::init("");
    scan_and_connect();
  }

  void write_state(LightState *state) override {
    float red, green, blue;
    state->get_rgb(&red, &green, &blue);

    // Convert to 0..255
    uint8_t r = (uint8_t)(red * 255);
    uint8_t g = (uint8_t)(green * 255);
    uint8_t b = (uint8_t)(blue * 255);

    // Example: groupId=1, extraParam=1, light=64, speed=64, type=0
    send_color_command(1, 1, r, g, b, 64, 64, 0);
  }

  void scan_and_connect() {
    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new BleAdvertisedDeviceCallbacks(this));
    scan->setActiveScan(true);
    scan->start(5, false); // Timeout 5s
  }

  void send_color_command(uint8_t groupId, uint8_t extraParam, uint8_t red, uint8_t green, uint8_t blue, uint8_t light, uint8_t speed, uint8_t type) {
    uint8_t cmd[16] = {84,82,0,87, 2, groupId, extraParam, red, green, blue, light, speed, type, 0, 0, 0};
    uint8_t encrypted[16];
    aes_encrypt(cmd, encrypted);
    if (target_characteristic)
      target_characteristic->writeValue(encrypted, 16, false);
  }

  void aes_encrypt(const uint8_t *input, uint8_t *output) {
    static const uint8_t key[16] = {52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248};
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&aes);
  }

  // BLE connection
  void set_target_characteristic(BLERemoteCharacteristic *charac) {
    target_characteristic = charac;
  }

 private:
  BLERemoteCharacteristic *target_characteristic = nullptr;
};

class BleAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
 public:
  BleAdvertisedDeviceCallbacks(BleRgbLight *parent) : parent_(parent) {}

  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    // Check manufacturer data and vendor ID
    const uint8_t targetManufacturerData[] = {0x00, 0x57, 0x00, 0x00, 0x53};
    const uint16_t targetVendorId = 0x5254;
    if (advertisedDevice.haveManufacturerData()) {
      std::string md = advertisedDevice.getManufacturerData();
      if (md.size() >= sizeof(targetManufacturerData) &&
          memcmp(md.data(), targetManufacturerData, sizeof(targetManufacturerData)) == 0 &&
          advertisedDevice.getManufacturerId() == targetVendorId) {
        BLEClient *client = BLEDevice::createClient();
        client->connect(&advertisedDevice);
        const BLEUUID char_uuid("0000ac52-1212-efde-1523-785fedbeda25");
        BLERemoteService *service = client->getService(advertisedDevice.getServiceUUID());
        if (service != nullptr) {
          BLERemoteCharacteristic *charac = service->getCharacteristic(char_uuid);
          if (charac != nullptr) {
            parent_->set_target_characteristic(charac);
            ESP_LOGI("ble_rgb", "Connected to BLE RGB light!");
          }
        }
        client->disconnect();
      }
    }
  }

 private:
  BleRgbLight *parent_;
};