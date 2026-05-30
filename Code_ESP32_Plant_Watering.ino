#include <Arduino.h>
#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"

/* ================= USER CONFIG ================= */

char nodeName[]   = "ESP32_Plant";
char deviceName[] = "Garden Pump";

const char *service_name = "PROV_Plant";
const char *pop = "PLANT123";

/* GPIO */
#define MOISTURE_PIN 34
#define RELAY_PIN    25

/* Soil calibration */
int DRY_ADC = 2910;
int WET_ADC = 925;


/* Moisture thresholds */
int MOISTURE_MIN = 30;
int MOISTURE_MAX = 70;

/* Relay type */
#define RELAY_ACTIVE_LOW true

static uint8_t wifiLed = 2;
static uint8_t gpio_reset = 0;

/* ============================================= */

/* States */
bool autoMode = true;            // RainMaker
bool cloudRelayState = false;   // Manual command
bool actualRelayState = false;  // Real relay

/* RainMaker */
static Switch pump(deviceName);
static Param autoParam("AutoMode", "esp.param.bool",
                       value(true),
                       PROP_FLAG_READ | PROP_FLAG_WRITE);

static Param moistureParam("Moisture", "esp.param.int",
                           value(0),
                           PROP_FLAG_READ);

Node node;

/* ---------- Relay ---------- */

void applyRelay(bool on) {

  if (actualRelayState == on) return;   // NO cloud spam

  actualRelayState = on;

  if (RELAY_ACTIVE_LOW)
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);
  else
    digitalWrite(RELAY_PIN, on ? HIGH : LOW);

  pump.updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, actualRelayState);
}

/* ---------- Moisture ---------- */

int readMoisture() {
  int adc = analogRead(MOISTURE_PIN);
  int pct = map(adc, DRY_ADC, WET_ADC, 0, 100);
  return constrain(pct, 0, 100);
}

/* ---------- Provisioning ---------- */

void sysProvEvent(arduino_event_t *sys_event) {

  switch (sys_event->event_id) {

    case ARDUINO_EVENT_PROV_START:
#if CONFIG_IDF_TARGET_ESP32
      Serial.printf("\nProvisioning: %s  POP: %s\n", service_name, pop);
      printQR(service_name, pop, "ble");
#endif
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      digitalWrite(wifiLed, HIGH);
      break;
  }
}

/* ---------- RainMaker Write ---------- */

void write_callback(Device *device, Param *param,
                    const param_val_t val,
                    void *priv_data,
                    write_ctx_t *ctx) {

  const char *name = param->getParamName();

  if (strcmp(name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
    cloudRelayState = val.val.b;
    if (!autoMode) applyRelay(cloudRelayState);
  }

  if (strcmp(name, "AutoMode") == 0) {
    autoMode = val.val.b;
    param->updateAndReport(val);
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(gpio_reset, INPUT);

  applyRelay(false);

  node = RMaker.initNode(nodeName);

  pump.addParam(autoParam);
  pump.addParam(moistureParam);
  pump.addCb(write_callback);

  node.addDevice(pump);

  RMaker.enableOTA(OTA_USING_PARAMS);
  RMaker.enableSchedule();
  RMaker.enableTZService();
  RMaker.start();

  WiFi.onEvent(sysProvEvent);

#if CONFIG_IDF_TARGET_ESP32
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                          WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                          WIFI_PROV_SECURITY_1,
                          pop,
                          service_name);
#endif
}

/* Timers */
unsigned long moistureTimer = 0;

void loop() {

  /* Reset */
  if (digitalRead(gpio_reset) == LOW) {
    delay(100);
    int st = millis();
    while (digitalRead(gpio_reset) == LOW) delay(50);
    int et = millis();

    if ((et - st) > 10000) RMakerFactoryReset(2);
    else if ((et - st) > 3000) RMakerWiFiReset(2);
  }

  digitalWrite(wifiLed, WiFi.status() == WL_CONNECTED);

  int moisture = readMoisture();

  /* -------- AUTO MODE -------- */
  if (autoMode) {

    if (moisture <= MOISTURE_MIN) applyRelay(true);
    if (moisture >= MOISTURE_MAX) applyRelay(false);
  }

  /* -------- Moisture every 5 sec -------- */
  if (millis() - moistureTimer > 5000) {

    moistureTimer = millis();
    moistureParam.updateAndReport(value(moisture));

    Serial.print("Moisture: ");
    Serial.println(moisture);
  }

  delay(200);
}
