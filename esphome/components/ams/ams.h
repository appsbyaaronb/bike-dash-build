#pragma once
// Apple Media Service client: the iPhone connects to us (we advertise AMS solicitation),
// pairs once, then we act as GATT client on that link to send remote commands and
// receive now-playing updates.
#include "esphome/core/component.h"
#include <esp_gap_ble_api.h>
#include <esp_gattc_api.h>
#include <string>

namespace esphome::ams {

enum Cmd : uint8_t { PLAY = 0, PAUSE = 1, TOGGLE = 2, NEXT = 3, PREV = 4, VOL_UP = 5, VOL_DOWN = 6 };

class AMSComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }
  void set_device_name(const std::string &n) { this->name_ = n; }

  void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

  bool send(uint8_t cmd);
  bool is_ready() const { return this->ready_; }
  bool is_linked() const { return this->linked_; }
  const std::string &title() const { return this->title_; }
  const std::string &artist() const { return this->artist_; }
  bool playing() const { return this->playing_; }

 protected:
  void start_adv_();
  void subscribe_(uint16_t chr);
  std::string name_;
  bool registered_{false}, adv_cfg_{false}, adv_on_{false};
  bool linked_{false}, ready_{false}, playing_{false}, track_sent_{false}, auth_done_{false}, enc_asked_{false};
  uint32_t conn_ms_{0};
  esp_gatt_if_t if_{ESP_GATT_IF_NONE};
  uint16_t cccd_cmd_{0}, conn_id_{0}, h_cmd_{0}, h_upd_{0}, h_start_{0}, h_end_{0};
  esp_bd_addr_t peer_{};
  std::string title_, artist_;
  uint32_t adv_retry_{0};
};

}  // namespace esphome::ams
