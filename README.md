# ESP32 Smart Plant Watering

ESP32-based smart plant watering system using ESP RainMaker for cloud control, BLE provisioning, automatic irrigation based on soil moisture, and OTA updates.

## Architecture

Below are the recommended architecture diagrams and a short summary of each area.

### Firmware Architecture
![Firmware Architecture](docs/firmware%20architecture.png)

Summary:
- Application Layer: irrigation logic, scheduling engine, safety rules and user commands.
- Service Layer: MQTT/service broker integration, OTA update service, sensor manager, relay controller, notification and storage services.
- HAL / Drivers: GPIO, ADC, WiFi, NVS/Flash, RTC/Timers drivers.
- Hardware Layer: soil moisture, water level sensors, pump/relay, LEDs and power supply.

### Complete System Architecture
![System Architecture](docs/complete%20architecture.png)

Summary:
- Data Flow: sensors -> ESP32 -> publish to MQTT -> cloud/backend -> dashboard/app -> user.
- Firmware Workflow: boot -> sensor read -> decision engine -> pump control -> communication (telemetry/commands) -> OTA & logging.
- Wiring Example: shows typical connections for soil sensor, water level sensor, ESP32 DevKit, relay module and pump.

## Features
- Automatic watering using soil moisture thresholds
- Manual control via cloud (ESP RainMaker) or voice assistants
- BLE provisioning (POP protected)
- OTA updates, scheduling and timezone support

## Hardware
- MCU: ESP32
- Soil moisture analog sensor (ADC)
- Relay module to drive pump/valve

Recommended pins (see `Code_ESP32_Plant_Watering.ino`):

- Soil moisture sensor -> GPIO34 (ADC) (`MOISTURE_PIN`)
- Relay IN -> GPIO25 (`RELAY_PIN`)
- WiFi status LED -> GPIO2 (`wifiLed`)
- Reset/prov button -> GPIO0 (`gpio_reset`)

Notes: share common GND between sensor, relay, and ESP32. Use level-appropriate relay module.

## Configuration (in code)
Open `Code_ESP32_Plant_Watering.ino` and edit the top section to configure:

- `nodeName` / `deviceName` — RainMaker names
- `service_name` / `pop` — BLE provisioning service name and POP
- `DRY_ADC` / `WET_ADC` — ADC calibration values for dry and wet soil
- `MOISTURE_MIN` / `MOISTURE_MAX` — auto watering thresholds (percent)
- `RELAY_ACTIVE_LOW` — set according to your relay logic

## Calibration
1. Open Serial Monitor at `115200`.
2. With sensor fully dry, note the ADC value printed and set `DRY_ADC`.
3. With sensor fully wet, note the ADC value and set `WET_ADC`.
4. The sketch maps ADC -> 0-100% using these values.

## Build & Flash
Arduino IDE:

1. Install ESP32 board support for Arduino (Espressif).
2. Install required libraries (ESP RainMaker / provisioning libraries used in the sketch).
3. Open `Code_ESP32_Plant_Watering.ino`, select your ESP32 board and port, then upload.

PlatformIO (example `platformio.ini` env):

```
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

## Provisioning
The sketch starts BLE provisioning using the `service_name` and `pop` configured in the code. Use a compatible provisioning tool (for example, the ESP RainMaker app or provisioning utility) and provide the POP when prompted.

## Usage
- By default the device runs in `AutoMode` (controlled by `AutoMode` param in RainMaker). When soil moisture <= `MOISTURE_MIN` the pump turns on; when >= `MOISTURE_MAX` it turns off.
- Manual cloud commands will control the relay only when `AutoMode` is off.
- Long-press `gpio_reset` (GPIO0) to perform factory reset or WiFi reset (see code for timing thresholds).

## Diagnostics
- Serial prints moisture percentage periodically. Use this to verify sensor readings and thresholds.

### Additional Notes
- MQTT Topics (example): `futureeagleiot/device001/sensor/moisture`, `.../pump/status`, `.../control`.
- OTA Flow: new firmware available -> download -> verify -> install -> reboot.
- Security: use TLS for MQTT, signed OTA images, secure provisioning POP, and least-privilege access for cloud services.

## License
This project is licensed under the Apache 2.0 License — see the `LICENSE` file.

## Contributing
Feel free to open issues or PRs to improve documentation, add platform support, or enhance features.



