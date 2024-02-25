#ifndef _INPUT_DEVICE_INPUT_ESPNOW_H_
#define _INPUT_DEVICE_INPUT_ESPNOW_H_

#include "esp_wifi.h"
#include <WiFi.h>
#include <esp_now.h>
#include "Device/InputDevice.h"
#include "Math/Utils.h"

// Check Tx identity -> model
//#define CheckMac

// Set the wifi channel (1-13) -> model
#define wifi_channel 13

// REPLACE WITH YOUR RECEIVER MAC Address -> model
uint8_t TxMacAddr[] = {0xA8,0x42,0xE3,0xCD,0x5F,0x04};

namespace Espfc {

namespace Device {

struct PacketData {
  //uint8_t syncByte = 0xf0;
  uint16_t A: 10;
  uint16_t E: 10;
  uint16_t R: 10;
  uint16_t T: 10;

  uint8_t ch1;
  uint8_t ch2;
  uint8_t ch3;
  uint8_t ch4;
}_data;

unsigned long lastRecvTime = 0;
bool _new_data = false;
int8_t rssi;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
#ifdef CheckMac
  char MACAd[6];
  memcpy(&MACAd, mac, sizeof(MACAd));
  for (int i = 0; i < 6; i++) {
    if (MACAd[i] != TxMacAddr[i]) return;
  }
#endif
  //snprintf(sys_status.StrMac, sizeof(sys_status.StrMac), "%02x:%02x:%02x:%02x:%02x:%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  memcpy(&_data, incomingData, sizeof(_data));
  lastRecvTime = millis();
  _new_data = true;
}

// get RSSI
//wifi_pkt_rx_ctrl_t wifi_rx_status;
// Estructuras para calcular los paquetes, el RSSI, etc
// typedef struct {
//   unsigned frame_ctrl: 16;
//   unsigned duration_id: 16;
//   uint8_t addr1[6]; /* receiver address */
//   uint8_t addr2[6]; /* sender address */
//   uint8_t addr3[6]; /* filtering address */
//   unsigned sequence_ctrl: 16;
//   uint8_t addr4[6]; /* optional */
// } wifi_ieee80211_mac_hdr_t;

// typedef struct {
//   wifi_ieee80211_mac_hdr_t hdr;
//   uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
// } wifi_ieee80211_packet_t;

void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT) return;
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  //const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  //const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  rssi = ppkt->rx_ctrl.rssi; // signed 8bits -128 - 128
}

class InputESPNOW: public InputDevice
{
  public:

    InputESPNOW() {}

    int begin(void)
    {
      WiFi.mode(WIFI_STA);
      WiFi.channel(wifi_channel);
      ESP_ERROR_CHECK(esp_now_init());
      esp_now_register_recv_cb(OnDataRecv);
      esp_wifi_set_promiscuous(true); // rssi
      esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);// rssi
      return 1;
    }

    InputStatus update() override
    {
      if(_new_data) {
        _new_data = false;
        _channels[0]  = convert(_data.A * 175 / 128 + 800);
        _channels[1]  = convert(_data.E * 175 / 128 + 800);
        _channels[2]  = convert(_data.R * 175 / 128 + 800);
        _channels[3]  = convert(_data.T * 175 / 128 + 800);
        _channels[4]  = convert(_data.ch1 * 175 / 32 + 800);
        _channels[5]  = convert(_data.ch2 * 175 / 32 + 800);
        _channels[6]  = convert(_data.ch3 * 175 / 32 + 800);
        _channels[7]  = convert(_data.ch4 * 175 / 32 + 800);
        _channels[8]  = convert(((int)rssi + 128) * 175 /32 + 800);

        if(millis() - lastRecvTime > 60) return INPUT_FAILSAFE;
        if(millis() - lastRecvTime > 100) return INPUT_LOST;
        return INPUT_RECEIVED;
      }
      return INPUT_IDLE;
    }

    uint16_t get(uint8_t i) const override
    {
      return _channels[i];
    }

    void get(uint16_t * data, size_t len) const override
    {
      const uint16_t * src = _channels;
      while(len--)
      {
        *data++ = *src++;
      }
    }

    size_t getChannelCount() const override { return CHANNELS; }

    bool needAverage() const override { return false; }

    void print(char c) const
    {
      //Serial.write(c);
    }

  private:

  inline uint16_t convert(int v) {
    return Math::clamp(v, 800, 2200);
  }

  static const size_t CHANNELS = 9;
  uint16_t _channels[CHANNELS];
};

}

}

#endif
