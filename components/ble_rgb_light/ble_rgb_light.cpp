#include "esphome/core/log.h"
#include "ble_rgb_light.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAddress.h>

#include <mbedtls/aes.h>

namespace esphome
{
    namespace empty_light
    {

        static const char *TAG = "empty_light.light";

        // Target BLE info
        static const uint8_t TARGET_MANUFACTURER_DATA[] = {0x54, 0x52, 0x00, 0x57, 0x00, 0x00, 0x53};
        static const uint16_t TARGET_VENDOR_ID = 0x5254;
        static const BLEUUID TARGET_CHAR_UUID("0000ac52-1212-efde-1523-785fedbeda25");
        static const uint8_t AES_KEY[16] = {52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248};
        static const BLEUUID TARGET_SERVICE_UUID("0000ac50-1212-efde-1523-785fedbeda25");

        BLERemoteCharacteristic *target_characteristic = nullptr;
        BLEClient *ble_client = nullptr;

        static BLEAddress *target_address = nullptr;
        static bool device_found = false;
        static unsigned long last_scan_time = 0;
        static unsigned long last_connection_attempt = 0;

        enum BleConnState
        {
            BLE_IDLE,
            BLE_CONNECTING,
            BLE_DISCOVERING,
            BLE_READY
        };
        static BleConnState ble_state = BLE_IDLE;
        static unsigned long ble_action_start = 0;

        class BleAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
        {
        public:
            void onResult(BLEAdvertisedDevice advertisedDevice) override
            {
                if (advertisedDevice.haveManufacturerData())
                {
                    String md = advertisedDevice.getManufacturerData();
                    ESP_LOGD(TAG, "Discovered device: Name: %s, Address: %s, serviceUUIDs: %d, rssi: %d, manufacturer data (hex): %s",
                             advertisedDevice.getName().c_str(),
                             advertisedDevice.getAddress().toString().c_str(),
                             advertisedDevice.getServiceUUIDCount(),
                             advertisedDevice.getRSSI(),
                             md.c_str());

                    char hexbuf[128] = {0};
                    for (size_t i = 0; i < md.length() && i < sizeof(hexbuf) / 3 - 1; ++i)
                    {
                        sprintf(hexbuf + strlen(hexbuf), "%02X ", (uint8_t)md[i]);
                    }
                    ESP_LOGD(TAG, "  Manufacturer data (hex): %s", hexbuf);
                }
                else
                {
                    ESP_LOGD(TAG, "Discovered device: Name: %s, Address: %s, serviceUUIDs: %d, rssi: %d, manufacturer data: (none)",
                             advertisedDevice.getName().c_str(),
                             advertisedDevice.getAddress().toString().c_str(),
                             advertisedDevice.getServiceUUIDCount(),
                             advertisedDevice.getRSSI());
                }

                for (int i = 0; i < advertisedDevice.getServiceUUIDCount(); ++i)
                {
                    ESP_LOGD(TAG, "  Service UUID[%d]: %s", i, advertisedDevice.getServiceUUID(i).toString().c_str());
                }

                // Only connect if the correct manufacturer data AND service UUID are advertised
                if (advertisedDevice.haveManufacturerData())
                {
                    String md = advertisedDevice.getManufacturerData();
                    if (md.length() >= sizeof(TARGET_MANUFACTURER_DATA) &&
                        memcmp(md.c_str(), TARGET_MANUFACTURER_DATA, sizeof(TARGET_MANUFACTURER_DATA)) == 0)
                    {
                        for (int i = 0; i < advertisedDevice.getServiceUUIDCount(); ++i)
                        {
                            // if (advertisedDevice.getServiceUUID(i).equals(TARGET_SERVICE_UUID)) {
                            ESP_LOGI(TAG, "Found target BLE device (manufacturer data match), will connect in loop()");
                            if (target_characteristic == nullptr && !device_found && target_address == nullptr)
                            {
                                target_address = new BLEAddress(advertisedDevice.getAddress());
                                device_found = true;
                                ESP_LOGI(TAG, "Target device address stored: %s", target_address->toString().c_str());
                            }
                            break;
                        }
                        // }
                    }
                }
            }
        };

        void EmptyLightOutput::setup()
        {
            ESP_LOGI(TAG, "setup()");
            BLEDevice::init("");
            last_scan_time = millis();
            BLEScan *scan = BLEDevice::getScan();
            scan->setAdvertisedDeviceCallbacks(new BleAdvertisedDeviceCallbacks());
            scan->setActiveScan(true);
            scan->start(3, false);
            yield();
        }

        void EmptyLightOutput::loop()
        {
            unsigned long now = millis();
            yield(); // Always yield at start of loop

            switch (ble_state)
            {
            case BLE_IDLE:
                if (device_found && target_characteristic == nullptr && target_address != nullptr)
                {
                    ESP_LOGI(TAG, "Starting BLE connect...");
                    ble_state = BLE_CONNECTING;
                    ble_action_start = now;
                }
                break;

            case BLE_CONNECTING:
                if ((now - ble_action_start) > 2000)
                {
                    if (target_address == nullptr)
                    {
                        ESP_LOGW(TAG, "No target address, skipping connect");
                        ble_state = BLE_IDLE;
                        break;
                    }

                    // Clean up previous client
                    if (ble_client != nullptr)
                    {
                        if (ble_client->isConnected())
                            ble_client->disconnect();
                        delete ble_client;
                        ble_client = nullptr;
                    }

                    ble_client = BLEDevice::createClient();
                    ESP_LOGI(TAG, "Attempting BLE connection...");

                    unsigned long connect_start = millis();
                    bool connected = false;

                    // TODO: are these needed? connect is blocking
                    while ((millis() - connect_start) < 200 && !connected)
                    {
                        connected = ble_client->connect(*target_address);
                        if (!connected)
                        {
                            yield();
                            delay(10);
                        }
                    }

                    if (connected)
                    {
                        ESP_LOGI(TAG, "BLE connected, moving to service discovery");
                        ble_state = BLE_DISCOVERING;
                        ble_action_start = now;
                    }
                    else
                    {
                        ESP_LOGW(TAG, "BLE connection failed, resetting");
                        device_found = false;
                        if (target_address != nullptr)
                        {
                            delete target_address;
                            target_address = nullptr;
                        }
                        ble_state = BLE_IDLE;
                    }
                }
                break;

            case BLE_DISCOVERING:
                if ((now - ble_action_start) > 1000)
                {
                    ESP_LOGI(TAG, "Discovering BLE services...");

                    if (ble_client != nullptr && ble_client->isConnected())
                    {
                        BLERemoteService *service = nullptr;

                        unsigned long service_start = millis();
                        while ((millis() - service_start) < 100 && service == nullptr)
                        {
                            service = ble_client->getService(TARGET_SERVICE_UUID);
                            if (service == nullptr)
                            {
                                yield();
                                delay(5);
                            }
                        }

                        bool found = false;
                        if (service != nullptr)
                        {
                            auto *characteristics = service->getCharacteristics();

                            unsigned long char_start = millis();
                            for (auto &char_pair : *characteristics)
                            {
                                if ((millis() - char_start) > 100)
                                    break;

                                BLERemoteCharacteristic *charac = char_pair.second;
                                if (charac != nullptr)
                                {
                                    ESP_LOGD(TAG, "  Discovered characteristic: %s", charac->getUUID().toString().c_str());
                                    if (charac->getUUID().equals(TARGET_CHAR_UUID))
                                    {
                                        target_characteristic = charac;
                                        ESP_LOGI(TAG, "Target characteristic found!");
                                        found = true;
                                        break;
                                    }
                                }
                                yield();
                            }
                        }

                        if (found)
                        {
                            ble_state = BLE_READY;
                        }
                        else
                        {
                            ESP_LOGW(TAG, "Target characteristic not found, resetting");
                            device_found = false;
                            if (target_address != nullptr)
                            {
                                delete target_address;
                                target_address = nullptr;
                            }
                            ble_state = BLE_IDLE;
                        }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "BLE client not connected during discovery");
                        ble_state = BLE_IDLE;
                    }
                }
                break;

            case BLE_READY:
                // Check connection health
                if ((now - ble_action_start) > 5000)
                {
                    if (ble_client == nullptr || !ble_client->isConnected())
                    {
                        ESP_LOGW(TAG, "BLE connection lost");
                        target_characteristic = nullptr;
                        device_found = false;
                        if (target_address != nullptr)
                        {
                            delete target_address;
                            target_address = nullptr;
                        }
                        ble_state = BLE_IDLE;
                    }
                    ble_action_start = now; // Reset timer
                }
                break;
            }

            if (!device_found && target_characteristic == nullptr && (now - last_scan_time) > 15000)
            {
                last_scan_time = now;
                ESP_LOGI(TAG, "Restarting BLE scan...");
                BLEScan *scan = BLEDevice::getScan();
                scan->start(3, false);
            }

            yield();
        }

        static void ensure_ble_connected()
        {
            if (target_characteristic == nullptr && !device_found)
            {
                unsigned long now = millis();
                if ((now - last_scan_time) > 10000)
                {
                    last_scan_time = now;
                    ESP_LOGI(TAG, "Starting non-blocking BLE scan...");
                    BLEScan *scan = BLEDevice::getScan();
                    scan->start(3, false);
                }
            }
        }

        light::LightTraits EmptyLightOutput::get_traits()
        {
            auto traits = light::LightTraits();
            traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS, light::ColorMode::RGB});

            return traits;
        }

        static void aes_encrypt(const uint8_t *input, uint8_t *output)
        {
            mbedtls_aes_context aes;
            mbedtls_aes_init(&aes);
            mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);
            mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
            mbedtls_aes_free(&aes);
        }

        // TODO: Store and set target_characteristic from BLE scan/connect logic
        extern BLERemoteCharacteristic *target_characteristic;

        void EmptyLightOutput::write_state(light::LightState *state)
        {
            if (target_characteristic == nullptr)
            {
                ensure_ble_connected();
                ESP_LOGW(TAG, "BLE target characteristic not set, cannot send command");
                return;
            }

            if (!state->current_values.is_on())
            {
                // All values zero for off
                uint8_t cmd[16] = {84, 82, 0, 87, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                uint8_t encrypted[16];
                aes_encrypt(cmd, encrypted);
                target_characteristic->writeValue(encrypted, 16, false);
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

            uint8_t cmd[16] = {84, 82, 0, 87, 2, groupId, extraParam, r, g, b, light, speed, type, 0, 0, 0};
            uint8_t encrypted[16];
            aes_encrypt(cmd, encrypted);

            target_characteristic->writeValue(encrypted, 16, false);
            ESP_LOGD(TAG, "Sent BLE RGB command: R=%d G=%d B=%d Brightness=%d", r, g, b, light);
        }

        void EmptyLightOutput::write_state_effect(uint8_t light, uint8_t speed, uint8_t effect)
        {
            if (target_characteristic == nullptr)
            {
                ensure_ble_connected();
                ESP_LOGW(TAG, "BLE target characteristic not set, cannot send effect command");
                return;
            }

            uint8_t r = 255;
            uint8_t g = 255;
            uint8_t b = 255;
            uint8_t groupId = 1;
            uint8_t type = 0;

            uint8_t cmd[16] = {84, 82, 0, 87, 2, groupId, effect, r, g, b, light, speed, type, 0, 0, 0};
            uint8_t encrypted[16];
            aes_encrypt(cmd, encrypted);

            target_characteristic->writeValue(encrypted, 16, false);
            ESP_LOGD(TAG, "Sent BLE RGB RAW command: R=%d G=%d B=%d Brightness=%d Speed=%d ExtraParam=%d", r, g, b, light, speed, effect);
        }

        void EmptyLightOutput::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Empty custom light");
        }

    } // namespace empty_light
} // namespace esphome