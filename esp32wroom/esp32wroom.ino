#ifndef ESP32
#pragma message(EXEMPLO APENAS PARA ESP32!)
#error Selecione uma placa ESP32.
#endif

#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include "AppInsights.h"

#define DEFAULT_POWER_MODE false
const char *service_name = "PROV_ESP32_WROOM";
const char *pop = "esp32wroom";

static int gpio_0 = 4;
static int gpio_switch = 13;

bool switch_state = true;

static Switch *wroom_switch = NULL;

void sysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.printf("\nProvisionamento iniciado com o nome \"%s\" e PoP \"%s\" via BLE\n",
                    service_name, pop);
      printQR(service_name, pop, "ble");
      break;
    case ARDUINO_EVENT_PROV_INIT:
      wifi_prov_mgr_disable_auto_stop(10000);
      break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      wifi_prov_mgr_stop_provisioning();
      break;
    default:;
  }
}

void write_callback(Device *device, Param *param, const param_val_t val,
                    void *priv_data, write_ctx_t *ctx)
{
  const char *device_name = device->getDeviceName();
  const char *param_name = param->getParamName();

  if (strcmp(param_name, "Power") == 0) {
    Serial.printf("Recebido = %s para %s - %s\n", val.val.b ? "true" : "false", device_name, param_name);
    switch_state = val.val.b;
    (switch_state == false) ? digitalWrite(gpio_switch, LOW) : digitalWrite(gpio_switch, HIGH);
    param->updateAndReport(val);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(gpio_0, INPUT);
  pinMode(gpio_switch, OUTPUT);
  digitalWrite(gpio_switch, DEFAULT_POWER_MODE);

  Node esp32wroom_node;
  esp32wroom_node = RMaker.initNode("ESP32 WROOM Node");

  // Initialize switch device
  wroom_switch = new Switch("Interruptor", &gpio_switch);
  if (!wroom_switch) {
    return;
  }
  // Standard switch device
  wroom_switch->addCb(write_callback);

  // Add switch device to the node
  esp32wroom_node.addDevice(*wroom_switch);

  RMaker.enableOTA(OTA_USING_TOPICS);
  RMaker.enableTZService();
  RMaker.enableSchedule();
  RMaker.enableScenes();
  initAppInsights();

  RMaker.enableSystemService(SYSTEM_SERV_FLAGS_ALL, 2, 2, 2);

  RMaker.start();

  WiFi.onEvent(sysProvEvent);
  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM, WIFI_PROV_SECURITY_1, pop, service_name);
}

void loop()
{
  if (digitalRead(gpio_0) == LOW) {  // Push button pressed

    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(gpio_0) == LOW) {
      delay(50);
    }
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      Serial.printf("Resetar para configuracoes de fabrica.\n");
      RMakerFactoryReset(2);
    } else if ((endTime - startTime) > 3000) {
      Serial.printf("Reseta as configuracoes de Wi-Fi .\n");
      RMakerWiFiReset(2);
    } else {
      // Toggle device state
      switch_state = !switch_state;
      Serial.printf("Alterar estado para: %s.\n", switch_state ? "true" : "false");
      if (wroom_switch) {
        wroom_switch->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, switch_state);
      }
      (switch_state == false) ? digitalWrite(gpio_switch, LOW)
      : digitalWrite(gpio_switch, HIGH);
    }
  }
  delay(100);
}
