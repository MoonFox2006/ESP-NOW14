#include <user_interface.h>
#include <espnow.h>
#include <Arduino.h>

#define DEBUG_LOG Serial

#define LED_PIN   2
#ifdef LED_PIN
#define LED_LEVEL LOW
#endif

#define ESP_NOW_CHANNEL 14

static bool setupWiFi() {
#if ESP_NOW_CHANNEL > 13
  wifi_country_t wc;

  wc.cc[0] = 'J';
  wc.cc[1] = 'P';
  wc.cc[2] = '\0';
  wc.schan = 1;
  wc.nchan = 14;
  wc.policy = WIFI_COUNTRY_POLICY_MANUAL;
  if (! wifi_set_country(&wc)) {
#ifdef DEBUG_LOG
    DEBUG_LOG.println(F("wifi_set_country() fail!"));
#endif
    return false;
  }
#endif
  if (wifi_set_channel(ESP_NOW_CHANNEL)) {
    return true;
  } else {
#ifdef DEBUG_LOG
    DEBUG_LOG.println(F("wifi_set_channel() fail!"));
#endif
    return false;
  }
}

static void printHex(uint8_t data) {
  uint8_t d;

  d = data >> 4;
  if (d <= 9)
    Serial.write('0' + d);
  else
    Serial.write('A' + d - 10);
  d = data & 0x0F;
  if (d <= 9)
    Serial.write('0' + d);
  else
    Serial.write('A' + d - 10);
}

static void dumpHex(const uint8_t *data, uint8_t len) {
  if (len--) {
    printHex(*data++);
    while (len--) {
      Serial.write(' ');
      printHex(*data++);
    }
  }
}

static void printMAC(const uint8_t *mac) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (i)
      Serial.write('-');
    printHex(mac[i]);
  }
}

static void esp_now_recvcb(uint8_t *mac, uint8_t *data, uint8_t len) {
  Serial.print(F("ESP-NOW packet from "));
  printMAC(mac);
  Serial.print(F(": "));
  dumpHex(data, len);
  Serial.println();
}

static bool setupEspNow() {
  if (! esp_now_init()) {
    if (! esp_now_set_self_role(ESP_NOW_ROLE_COMBO)) {
      uint8_t mac[6];

      memset(mac, 0xFF, sizeof(mac)); // Broadcast MAC
      if (! esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, ESP_NOW_CHANNEL, NULL, 0)) {
        if (! esp_now_register_recv_cb(esp_now_recvcb)) {
          return true;
        } else {
#ifdef DEBUG_LOG
          DEBUG_LOG.println(F("esp_now_register_recv_cb() fail!"));
#endif
        }
      } else {
#ifdef DEBUG_LOG
        DEBUG_LOG.println(F("esp_now_add_peer() fail!"));
#endif
      }
    } else {
#ifdef DEBUG_LOG
      DEBUG_LOG.println(F("esp_now_set_self_role() fail!"));
#endif
    }
  } else {
#ifdef DEBUG_LOG
    DEBUG_LOG.println(F("esp_now_init() fail!"));
#endif
  }
  return false;
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println();

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ! LED_LEVEL);
#endif

  wifi_set_opmode_current(SOFTAP_MODE);
  {
    softap_config sc;

    sc.authmode = AUTH_OPEN;
    sc.ssid[0] = 0;
    sc.ssid_len = 0;
    sc.password[0] = 0;
    if (! wifi_softap_set_config_current(&sc)) {
      Serial.println(F("AP disconnect fail!"));
    }
  }

  if ((! setupWiFi()) || (! setupEspNow())) {
    Serial.println(F("ESP-NOW initialization fail!"));
    Serial.flush();

    ESP.deepSleep(0);
  }
}

void loop() {
  static uint8_t cnt = 0;
  char str[4];

  utoa(cnt++, str, 10);
  if (esp_now_send(NULL, (uint8_t*)str, strlen(str))) {
    Serial.println(F("ESP-NOW sending fail!"));
  }
#ifdef LED_PIN
  digitalWrite(LED_PIN, LED_LEVEL);
  delay(25);
  digitalWrite(LED_PIN, ! LED_LEVEL);
  delay(500 - 25);
#else
  delay(500);
#endif
}
