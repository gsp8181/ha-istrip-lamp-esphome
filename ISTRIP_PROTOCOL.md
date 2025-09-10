# iStrip BLE Protocol Specification

This document describes the BLE protocol used by the iStrip app and compatible RGB BLE devices, as reverse-engineered from the app and code.

## BLE Device Discovery

- **Vendor ID:** `0x5254`
- **Manufacturer Data:** `{ 0x00, 0x57, 0x00, 0x00, 0x53 }` (may vary)
- **Service UUID:** `0000ac50-1212-efde-1523-785fedbeda25`
- **Characteristic UUID:** `0000ac52-1212-efde-1523-785fedbeda25`
- **Encryption Key (AES-128 ECB):** `34 52 2A 5B 7A 6E 49 2C 08 09 0A 9D 8D 2A 23 F8` (decimal: 52, 82, 42, 91, 122, 110, 73, 44, 8, 9, 10, 157, 141, 42, 35, 248)

## Command Format

All commands are 16 bytes, encrypted with AES-128 ECB using the key above.

### General Command Structure

| Byte | Description         |
|------|---------------------|
| 0    | 0x54 ('T')          |
| 1    | 0x52 ('R')          |
| 2    | 0x00                |
| 3    | 0x57 ('W')          |
| 4    | Command Type        |
| 5    | Group ID            |
| 6    | Extra Param / Mode  |
| 7-15 | Command-specific    |

## Command Types

### 1. Colour/Effect Command

- **Type:** `0x02`
- **Format:**  
  `{84,82,0,87, 2, groupId, extraParam, red, green, blue, light, speed, type, 0, 0, 0}`
- **Parameters:**
  - `groupId`: 1
  - `extraParam`: Effect number (see below)
  - `red`, `green`, `blue`: 0-255
  - `light`: Brightness (0-64)
  - `speed`: Effect speed (0-64)
  - `type`: 0 (default)
- **Effects (extraParam):**
  1. Fade7
  2. Fade3
  3. Breath R
  4. Strobe R
  5. Breath7
  6. Breath3
  7. Breath B
  8. Strobe B
  9. Flash7
  10. Flash3
  11. Breath G
  12. Strobe G

### 2. LED Off Command

- **Type:** `0x02`
- **Format:**  
  `{84,82,0,87, 2, groupId, 0, 0, 0, 0, light, speed, 0, 0, 0, 0}`

### 3. Rhythm Command

- **Type:** `0x03`
- **Format:**  
  `{84,82,0,87, 3, groupId, fftStatus, fft, sensitivity, 0, 0, 0, 0, 0, 0, 0}`

### 4. Timer Command

- **Type:** `0x04`
- **Format:**  
  `{84,82,0,87, 4, groupId, week, currentHour, currentMinute, currentSecond, timerOnWeek, timerOnHour, timerOnMin, timerOffWeek, timerOffHour, timerOffMin}`

### 5. RGB Line Sequence Command

- **Type:** `0x05`
- **Format:**  
  `{84,82,0,87, 5, groupId, extraParam, 0, 0, 0, 0, 0, 0, 0, 0, 0}`

### 6. Speed Command

- **Type:** `0x06`
- **Format:**  
  `{84,82,0,87, 6, groupId, speed, 0, 0, 0, 0, 0, 0, 0, 0, 0}`

### 7. Light Command

- **Type:** `0x07`
- **Format:**  
  `{84,82,0,87, 7, groupId, light, 0, 0, 0, 0, 0, 0, 0, 0, 0}`

## Encryption

- All commands must be encrypted with AES-128 ECB using the key above.
- Data is padded to 16 bytes if needed.

## Example Usage

- To set a blue colour with Fade7 effect, full brightness and speed (note the colour is ignored with effects):
  ```
  groupId = 1
  extraParam = 1 (Fade7)
  red = 0x00
  green = 0x00
  blue = 0xff
  light = 64
  speed = 64
  type = 0
  Command: {84,82,0,87, 2, 1, 1, 0, 0, 255, 64, 64, 0, 0, 0, 0}
  ```

## Notes

- All communication is via BLE GATT write to the characteristic UUID above.
- The protocol is reverse-engineered and may not be complete.
- For more details, see the C# implementation in `esphome.cs`.

---
