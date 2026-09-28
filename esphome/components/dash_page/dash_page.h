#pragma once
// Serves the dash simulator page on its own port, separate from ESPHome's web_server on :80.
#include "esphome/core/component.h"
#include <esp_http_server.h>
#include "esphome/components/ams/ams.h"

namespace esphome::dash_page {

class DashPage : public Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }
  void set_port(uint16_t p) { this->port_ = p; }
  void set_ams(ams::AMSComponent *a) { this->ams_ = a; }
  void set_html(const char *h, size_t n) { this->html_ = h; this->len_ = n; }

 protected:
  static esp_err_t handle_(httpd_req_t *req);
  static esp_err_t cmd_(httpd_req_t *req);
  static esp_err_t restart_(httpd_req_t *req);
  ams::AMSComponent *ams_{nullptr};
  uint16_t port_{8080};
  const char *html_{nullptr};
  size_t len_{0};
  httpd_handle_t server_{nullptr};
};

}  // namespace esphome::dash_page
