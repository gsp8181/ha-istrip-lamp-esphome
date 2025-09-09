# ESPHome BLE RGB Light Controller for iStrip

This project is an [ESPHome](https://esphome.io/) configuration and custom component set for controlling RGB effect lights via Bluetooth Low Energy (BLE), compatible with the "iStrip" mobile app.

## Features

- Controls BLE RGB LED strips that use the iStrip protocol.
- Supports on/off, color, brightness, and a variety of animated effects.
- Integrates with Home Assistant via ESPHome API.
- Exposes effect selection, speed, and brightness as Home Assistant entities.
- Button to trigger effects directly from Home Assistant.

## Hardware

- ESP32 development board
- Compatible BLE RGB LED strip (iStrip protocol)

## Usage

1. **Configure your WiFi credentials** in `secrets.yaml`.
2. **Flash the ESP32** with the provided ESPHome YAML (`ble_rgb_lights.yaml`).
3. **Add the device to Home Assistant** using ESPHome integration.
4. **Control the light**: 
   - Use the light entity for on/off, color, and brightness.
   - Use the select, number, and button entities to choose and trigger effects.

## Custom Components

- `ble_rgb_light`: Handles BLE connection and basic light control.
- `ble_rgb_effect`: Exposes effect triggering as a separate component for easy automation.

## Notes

- The BLE protocol is reverse-engineered for iStrip-compatible devices.
- All BLE communication is handled non-blocking to avoid watchdog resets.
- Effects and parameters are mapped to the iStrip app's effect numbers.
- **Troubleshooting:** Occasionally, the light may be blocked from connecting and you may see "core1" errors in the ESP logs. If this happens, simply power cycle the light to restore normal operation.

## License

MIT License. See source files for details.
