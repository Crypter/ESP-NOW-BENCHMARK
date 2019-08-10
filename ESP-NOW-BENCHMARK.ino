#if defined(ESP32)
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi_internal.h>
#else
#error "This library supports ESP32 only."
#endif

#define TRY_ESP_ACTION(action, name) if(action == ESP_OK) {Serial.println("\t+ "+String(name));} else {Serial.println("----------Error while " + String(name) + " !---------------");}

//#define SENDER
#define RECEIVER

#define LED_INDICATOR 2
#define PACKET_LENGTH 1
#define DELAY 20
#define HOP 0

volatile uint32_t link_quality[100];
volatile uint8_t link_quality_counter=0;
volatile uint8_t calculated_quality=0;
volatile uint8_t receive_miss=0;
volatile uint8_t channel=1;
volatile uint8_t value=0;
uint8_t packet_data[PACKET_LENGTH];
const uint8_t broadcast_addr[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

volatile uint8_t synced=0;

void report( void * parameter ){
  for (;;){
    calculated_quality=0;
    for (uint8_t i=0; i<100; i++){
      if (millis()-link_quality[i]<=1000) calculated_quality++;
//      Serial.println(link_quality[i]);
    }
    calculated_quality = (float) calculated_quality/(1000/DELAY)*100;
    Serial.println(calculated_quality);
    delay(100);
  }
}

void hop( void * parameter ){
  for (;;){
    delay(3000);
    channel%=13;
    channel++;
    TRY_ESP_ACTION( esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE), String("Hop channel: ")+String(channel));
  }
}

void received_callback(const uint8_t *mac, const uint8_t *data, uint8_t len){
#ifdef RECEIVER
  link_quality_counter=(link_quality_counter+1)%100;
  link_quality[link_quality_counter]=millis();
  receive_miss=0;
  synced=1;
  value=data[0];
#endif
}

void sent_callback(const uint8_t *mac, uint8_t status){}

void setup() {
  Serial.begin(115200);

  WiFi.enableSTA(true);
  WiFi.setSleep(false);


  TRY_ESP_ACTION( esp_wifi_stop(), "stop WIFI");
  
  TRY_ESP_ACTION( esp_wifi_deinit(), "De init");

  wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
  my_config.ampdu_tx_enable = 0;
  my_config.ampdu_rx_enable = 0;
  
  TRY_ESP_ACTION( esp_wifi_init(&my_config), "Disable AMPDU");
  
  TRY_ESP_ACTION( esp_wifi_start(), "Restart WiFi");

  TRY_ESP_ACTION( esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE), "Set channel");

//  TRY_ESP_ACTION( esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, WIFI_PHY_RATE_54M), "Fixed rate set up");
  TRY_ESP_ACTION( esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, true, WIFI_PHY_RATE_LORA_250K), "Fixed rate set up");

TRY_ESP_ACTION( esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR), "WIFI_PROTOCOL_LR");

  TRY_ESP_ACTION( esp_now_init(), "ESPNow Init");


  esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(sent_callback));
  esp_now_register_recv_cb(reinterpret_cast<esp_now_recv_cb_t>(received_callback));

  esp_now_peer_info_t brcst;
  memset(&brcst, 0, sizeof(brcst));
  memcpy(brcst.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
  brcst.channel = 0;
  brcst.ifidx = ESP_IF_WIFI_STA;
  esp_now_add_peer(&brcst);

  pinMode(0, INPUT);

  for (uint8_t i=0; i<100; i++){
    link_quality[i]=0;
  }

#ifdef RECEIVER
  ledcSetup(0, 50, 8); //channel 0, 50hz, 8 bit
  ledcAttachPin(LED_INDICATOR, 0); //pin on ch 0
  while (HOP && !synced) yield();
#endif

  if (HOP){
    xTaskCreatePinnedToCore(
      reinterpret_cast<TaskFunction_t>(&hop),   /* Function to implement the task */
      "channel_hop", /* Name of the task */
      5000,      /* Stack size in words */
      0,       /* Task input parameter */
      1,          /* Priority of the task */
      NULL,       /* Task handle. */
      APP_CPU_NUM);  /* Core where the task should run */
  }
  #ifdef RECEIVER
    xTaskCreatePinnedToCore(
      reinterpret_cast<TaskFunction_t>(&report),   /* Function to implement the task */
      "report", /* Name of the task */
      5000,      /* Stack size in words */
      0,       /* Task input parameter */
      1,          /* Priority of the task */
      NULL,       /* Task handle. */
      APP_CPU_NUM);  /* Core where the task should run */
  #endif
}

void loop() {
#ifdef SENDER
  if (digitalRead(0)) esp_now_send(broadcast_addr, packet_data, PACKET_LENGTH);
  delay(DELAY);
#endif


#ifdef RECEIVER
  ledcWrite(0, calculated_quality*2.55);
  yield();
//  ledcWrite(0, value);
#endif
}
