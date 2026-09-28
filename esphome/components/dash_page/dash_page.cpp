#include "dash_page.h"
#include "esphome/core/log.h"

namespace esphome::dash_page {

static const char *const TAG = "dash_page";

void DashPage::setup() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = this->port_;
  cfg.ctrl_port = 32769;  // web_server on :80 already holds the default control port
  cfg.max_open_sockets = 3;
  if (httpd_start(&this->server_, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Could not start on port %u", this->port_);
    this->mark_failed();
    return;
  }
  httpd_uri_t root{.uri = "/", .method = HTTP_GET, .handler = handle_, .user_ctx = this};
  httpd_register_uri_handler(this->server_, &root);
  httpd_uri_t cmd{.uri = "/cmd", .method = HTTP_POST, .handler = cmd_, .user_ctx = this};
  httpd_register_uri_handler(this->server_, &cmd);
  ESP_LOGI(TAG, "Dash page on port %u (%u bytes)", this->port_, (unsigned) this->len_);
}

esp_err_t DashPage::handle_(httpd_req_t *req) {
  auto *self = static_cast<DashPage *>(req->user_ctx);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  return httpd_resp_send(req, self->html_, self->len_);
}

// Media button from the page: POST /cmd?c=<AMS command id>.
esp_err_t DashPage::cmd_(httpd_req_t *req) {
  auto *self = static_cast<DashPage *>(req->user_ctx);
  char q[16] = {0}, v[4] = {0};
  bool ok = false;
  if (self->ams_ != nullptr && httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK &&
      httpd_query_key_value(q, "c", v, sizeof(v)) == ESP_OK) {
    int c = atoi(v);
    if (c >= 0 && c <= 6)
      ok = self->ams_->send((uint8_t) c);
  }
  httpd_resp_set_type(req, "text/plain");
  return httpd_resp_sendstr(req, ok ? "ok" : "no phone");
}

}  // namespace esphome::dash_page
