#if defined(ESP32)
#include <WiFi.h>
#include <esp_now.h>
#else
#error "This library supports ESP32 only."
#endif

//#define SENDER
#define RECEIVER

#define LED_INDICATOR 21
#define PACKET_LENGTH 1
#define DELAY 10
#define CHANNEL 1

volatile uint8_t link_quality=0;
volatile uint8_t receive_miss=0;
volatile uint8_t channel=0;
uint8_t packet_data[PACKET_LENGTH];
const uint8_t broadcast_addr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void received_callback(const uint8_t *mac, const uint8_t *data, uint8_t len){
#ifdef RECEIVER
  link_quality=100;
  receive_miss=0;
#endif
}

void sent_callback(const uint8_t *mac, uint8_t status){}

void setup() {

  WiFi.softAP("ESP-NOW-BENCHMARK", NULL, CHANNEL, true);
  WiFi.setSleep(false);

  esp_now_init();
  esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(sent_callback));
  esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(received_callback));

  esp_now_peer_info_t brcst;
  memset(&brcst, 0, sizeof(brcst));
  memcpy(brcst.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
  brcst.channel = CHANNEL;
  brcst.ifidx = ESP_IF_WIFI_AP;
  esp_now_add_peer(&brcst);

#ifdef RECEIVER
  ledcSetup(0, 50, 8); //channel 0, 50hz, 8 bit
  ledcAttachPin(LED_INDICATOR, 0); //pin on ch 0
#endif

  delay(100);
}

void loop() {
#ifdef SENDER
  esp_now_send(broadcast_addr, packet_data, PACKET_LENGTH);
#endif

#ifdef RECEIVER
  if (receive_miss && link_quality) {
    link_quality--;
    receive_miss=0;
  }
  else {
    receive_miss=1;
  }
#endif

  delay(DELAY);

#ifdef RECEIVER  
  ledcWrite(0, link_quality*2.55);
#endif
}
