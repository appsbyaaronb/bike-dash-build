#pragma once
// Serves the device log on its own port from a fixed-size ring buffer.
#include "esphome/core/component.h"
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace esphome::log_page {

class LogPage : public Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }
  void set_port(uint16_t p) { this->port_ = p; }
  void set_buffer_size(size_t n) { this->size_ = n; }

 protected:
  void on_log_(uint8_t level, const char *tag, const char *msg, size_t len);
  void put_(const char *s, size_t n);
  static esp_err_t page_(httpd_req_t *req);
  static esp_err_t raw_(httpd_req_t *req);
  uint16_t port_{8081};
  size_t size_{32768};
  char *buf_{nullptr};
  size_t head_{0};     // next write position
  bool wrapped_{false};
  uint32_t total_{0};  // bytes ever written, lets the page fetch only what's new
  SemaphoreHandle_t lock_{nullptr};
  httpd_handle_t server_{nullptr};
};

}  // namespace esphome::log_page
