#include "ams.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/esp32_ble/ble.h"
#include <cstring>

namespace esphome::ams {

static const char *const TAG = "ams";
static const uint16_t APP_ID = 0xA5;

// UUIDs, little-endian byte order as used on the air.
// AMS service 89D3502B-0F36-433A-8EF4-C502AD55F8DC
static const uint8_t AMS_SVC[16] = {0xDC, 0xF8, 0x55, 0xAD, 0x02, 0xC5, 0xF4, 0x8E,
                                    0x3A, 0x43, 0x36, 0x0F, 0x2B, 0x50, 0xD3, 0x89};
// Remote Command 9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2
static const uint8_t AMS_CMD[16] = {0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8,
                                    0x8A, 0x4A, 0xB1, 0x57, 0xD8, 0x81, 0x3C, 0x9B};
// Entity Update 2F7CABCE-808D-411F-9A0C-BB92BA96C102
static const uint8_t AMS_UPD[16] = {0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A,
                                    0x1F, 0x41, 0x8D, 0x80, 0xCE, 0xAB, 0x7C, 0x2F};

static esp_ble_adv_params_t ADV = {
    .adv_int_min = 0x40, .adv_int_max = 0x80, .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC, .peer_addr = {}, .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL, .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY};

static esp_bt_uuid_t uuid128(const uint8_t *u) {
  esp_bt_uuid_t r{};
  r.len = ESP_UUID_LEN_128;
  memcpy(r.uuid.uuid128, u, 16);
  return r;
}

void AMSComponent::setup() {}

void AMSComponent::loop() {
  if (esp32_ble::global_ble == nullptr || !esp32_ble::global_ble->is_active()) {
    // Bluetooth was switched off (e.g. Restart Bluetooth): the GATTC app and link are gone,
    // so start over from registration when it comes back.
    if (this->registered_) {
      ESP_LOGI(TAG, "Bluetooth off, resetting phone link");
      this->registered_ = this->adv_cfg_ = this->adv_on_ = false;
      this->linked_ = this->ready_ = this->playing_ = this->track_sent_ = false;
      this->if_ = ESP_GATT_IF_NONE;
      this->h_cmd_ = this->h_upd_ = this->h_start_ = this->h_end_ = this->cccd_cmd_ = 0;
      this->title_.clear();
      this->artist_.clear();
    }
    return;
  }
  if (!this->registered_) {
    this->registered_ = true;
    // Bonded, no MITM (no keypad or display for a passkey).
    esp_ble_auth_req_t auth = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16, init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &init_key, 1);
    esp_err_t err = esp_ble_gattc_app_register(APP_ID);
    ESP_LOGI(TAG, "GATTC app register: %s", esp_err_to_name(err));
    this->start_adv_();
  }
  // Let the phone start pairing; only ask ourselves if it hasn't after a few seconds.
  // Both sides starting at once is what broke the first attempt (DHKey mismatch).
  if (this->linked_ && !this->auth_done_ && !this->enc_asked_ && millis() - this->conn_ms_ > 4000) {
    this->enc_asked_ = true;
    ESP_LOGI(TAG, "Phone did not pair on its own, requesting encryption");
    esp_ble_set_encryption(this->peer_, ESP_BLE_SEC_ENCRYPT_NO_MITM);
  }
  // Advertising stops when a central connects; restart it while no phone is linked.
  if (!this->linked_ && !this->adv_on_ && millis() - this->adv_retry_ > 5000) {
    this->adv_retry_ = millis();
    this->start_adv_();
  }
}

void AMSComponent::start_adv_() {
  if (!this->adv_cfg_) {
    // Flags + 128-bit service solicitation for AMS (lets iOS list us and connect).
    uint8_t adv[3 + 18] = {0x02, 0x01, 0x06, 0x11, 0x15};
    memcpy(adv + 5, AMS_SVC, 16);
    esp_ble_gap_config_adv_data_raw(adv, sizeof(adv));
    uint8_t rsp[31];
    size_t n = std::min<size_t>(this->name_.size(), 29);
    rsp[0] = n + 1;
    rsp[1] = 0x09;
    memcpy(rsp + 2, this->name_.data(), n);
    esp_ble_gap_config_scan_rsp_data_raw(rsp, n + 2);
    esp_ble_gap_set_device_name(this->name_.c_str());
    this->adv_cfg_ = true;
    return;  // started from the *_SET_COMPLETE events
  }
  esp_ble_gap_start_advertising(&ADV);
}

void AMSComponent::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
      esp_ble_gap_start_advertising(&ADV);
      break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      this->adv_on_ = param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS;
      ESP_LOGI(TAG, "Advertising as '%s': %s", this->name_.c_str(), this->adv_on_ ? "on" : "failed");
      break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
      if (this->linked_ && memcmp(param->ble_security.ble_req.bd_addr, this->peer_, 6) == 0)
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
      break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
      auto &a = param->ble_security.auth_cmpl;
      if (!this->linked_ || memcmp(a.bd_addr, this->peer_, 6) != 0)
        break;
      ESP_LOGI(TAG, "Pairing %s (reason 0x%02x)", a.success ? "OK" : "FAILED", a.fail_reason);
      this->auth_done_ = a.success;
      if (a.success && this->if_ != ESP_GATT_IF_NONE)
        esp_ble_gattc_open(this->if_, this->peer_, a.addr_type, true);  // reuses the existing link
      break;
    }
    default:
      break;
  }
}

void AMSComponent::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                       esp_ble_gattc_cb_param_t *param) {
  if (event == ESP_GATTC_REG_EVT) {
    if (param->reg.app_id == APP_ID) {
      this->if_ = gattc_if;
      ESP_LOGI(TAG, "GATTC if %d", gattc_if);
    }
    return;
  }
  if (gattc_if != this->if_ && gattc_if != ESP_GATT_IF_NONE)
    return;
  switch (event) {
    case ESP_GATTC_CONNECT_EVT:
      // Only links where the phone is central and we are peripheral (the BMS link is the other way).
      if (param->connect.link_role != 1 || this->linked_)
        break;
      this->linked_ = true;
      this->adv_on_ = false;
      memcpy(this->peer_, param->connect.remote_bda, 6);
      this->conn_ms_ = millis();
      this->auth_done_ = this->enc_asked_ = false;
      ESP_LOGI(TAG, "Phone connected %02x:%02x:%02x:%02x:%02x:%02x, waiting for pairing", this->peer_[0],
               this->peer_[1], this->peer_[2], this->peer_[3], this->peer_[4], this->peer_[5]);
      break;
    case ESP_GATTC_OPEN_EVT:
      if (memcmp(param->open.remote_bda, this->peer_, 6) != 0)
        break;
      if (param->open.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "GATTC open failed: %d", param->open.status);
        break;
      }
      this->conn_id_ = param->open.conn_id;
      {
        esp_bt_uuid_t svc = uuid128(AMS_SVC);
        esp_ble_gattc_search_service(this->if_, this->conn_id_, &svc);
      }
      break;
    case ESP_GATTC_SEARCH_RES_EVT:
      if (param->search_res.conn_id == this->conn_id_) {
        this->h_start_ = param->search_res.start_handle;
        this->h_end_ = param->search_res.end_handle;
      }
      break;
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      if (param->search_cmpl.conn_id != this->conn_id_)
        break;
      if (this->h_start_ == 0) {
        ESP_LOGW(TAG, "Phone has no AMS service");
        break;
      }
      esp_gattc_char_elem_t el;
      uint16_t count = 1;
      esp_bt_uuid_t u = uuid128(AMS_CMD);
      if (esp_ble_gattc_get_char_by_uuid(this->if_, this->conn_id_, this->h_start_, this->h_end_, u, &el,
                                         &count) == ESP_GATT_OK && count)
        this->h_cmd_ = el.char_handle;
      count = 1;
      u = uuid128(AMS_UPD);
      if (esp_ble_gattc_get_char_by_uuid(this->if_, this->conn_id_, this->h_start_, this->h_end_, u, &el,
                                         &count) == ESP_GATT_OK && count)
        this->h_upd_ = el.char_handle;
      ESP_LOGI(TAG, "AMS found: cmd=0x%04x update=0x%04x", this->h_cmd_, this->h_upd_);
      this->ready_ = this->h_cmd_ != 0;
      // iOS only acts on remote commands once we subscribe to the Remote Command list.
      if (this->h_cmd_)
        esp_ble_gattc_register_for_notify(this->if_, this->peer_, this->h_cmd_);
      if (this->h_upd_)
        esp_ble_gattc_register_for_notify(this->if_, this->peer_, this->h_upd_);
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
      ESP_LOGI(TAG, "Notify registered for 0x%04x: status %d", param->reg_for_notify.handle,
               param->reg_for_notify.status);
      if (param->reg_for_notify.status == ESP_GATT_OK)
        this->subscribe_(param->reg_for_notify.handle);
      break;
    case ESP_GATTC_WRITE_DESCR_EVT:
      ESP_LOGI(TAG, "CCCD write status %d", param->write.status);
      if (param->write.conn_id == this->conn_id_ && param->write.status == ESP_GATT_OK &&
          param->write.handle != this->cccd_cmd_) {
        // Player: PlaybackInfo(1). Track: Artist(0), Title(2).
        uint8_t player[] = {0, 1};
        esp_ble_gattc_write_char(this->if_, this->conn_id_, this->h_upd_, sizeof(player), player,
                                 ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
      }
      break;
    case ESP_GATTC_WRITE_CHAR_EVT:
      if (param->write.conn_id == this->conn_id_)
        ESP_LOGI(TAG, "Write 0x%04x status %d", param->write.handle, param->write.status);
      if (param->write.conn_id == this->conn_id_ && param->write.handle == this->h_upd_ &&
          param->write.status == ESP_GATT_OK && this->title_.empty()) {
        if (!this->track_sent_) {
          this->track_sent_ = true;
          uint8_t track[] = {2, 0, 2};
          esp_ble_gattc_write_char(this->if_, this->conn_id_, this->h_upd_, sizeof(track), track,
                                   ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        }
      }
      break;
    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.conn_id == this->conn_id_ && param->notify.handle == this->h_cmd_) {
        std::string l;
        for (int i = 0; i < param->notify.value_len; i++)
          l += std::to_string(param->notify.value[i]) + " ";
        ESP_LOGI(TAG, "Phone supports commands: %s", l.c_str());
        break;
      }
      if (param->notify.conn_id != this->conn_id_ || param->notify.handle != this->h_upd_ ||
          param->notify.value_len < 3)
        break;
      const uint8_t *v = param->notify.value;
      std::string val((const char *) v + 3, param->notify.value_len - 3);
      if (v[0] == 2 && v[1] == 2)
        this->title_ = val;
      else if (v[0] == 2 && v[1] == 0)
        this->artist_ = val;
      else if (v[0] == 0 && v[1] == 1)
        this->playing_ = !val.empty() && val[0] == '1';  // "state,rate,elapsed"
      ESP_LOGI(TAG, "Now playing: %s - %s [%s]", this->title_.c_str(), this->artist_.c_str(),
               this->playing_ ? "playing" : "paused");
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
      if (memcmp(param->disconnect.remote_bda, this->peer_, 6) != 0 || !this->linked_)
        break;
      ESP_LOGI(TAG, "Phone disconnected (reason 0x%02x)", param->disconnect.reason);
      this->linked_ = this->ready_ = this->playing_ = this->track_sent_ = false;
      this->h_cmd_ = this->h_upd_ = this->h_start_ = this->h_end_ = 0;
      this->title_.clear();
      this->artist_.clear();
      memset(this->peer_, 0, 6);
      break;
    default:
      break;
  }
}

void AMSComponent::subscribe_(uint16_t chr) {
  // Enable notifications on the Entity Update CCCD (looked up, not assumed).
  uint16_t cccd = chr + 1;
  esp_gattc_descr_elem_t d;
  uint16_t count = 1;
  esp_bt_uuid_t u{};
  u.len = ESP_UUID_LEN_16;
  u.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
  if (esp_ble_gattc_get_descr_by_char_handle(this->if_, this->conn_id_, chr, u, &d, &count) ==
          ESP_GATT_OK && count)
    cccd = d.handle;
  uint8_t on[] = {0x01, 0x00};
  esp_err_t err = esp_ble_gattc_write_char_descr(this->if_, this->conn_id_, cccd, sizeof(on), on,
                                                 ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (chr == this->h_cmd_)
    this->cccd_cmd_ = cccd;
  ESP_LOGI(TAG, "Subscribing 0x%04x via CCCD 0x%04x: %s", chr, cccd, esp_err_to_name(err));
}

bool AMSComponent::send(uint8_t cmd) {
  if (!this->ready_) {
    ESP_LOGW(TAG, "Command %u ignored: no phone", cmd);
    return false;
  }
  esp_err_t err = esp_ble_gattc_write_char(this->if_, this->conn_id_, this->h_cmd_, 1, &cmd,
                                           ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
  ESP_LOGI(TAG, "Command %u -> %s", cmd, esp_err_to_name(err));
  return err == ESP_OK;
}

}  // namespace esphome::ams
