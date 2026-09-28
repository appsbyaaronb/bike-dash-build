#include "log_page.h"
#include "esphome/core/log.h"
#include "esphome/components/logger/logger.h"
#include <esp_heap_caps.h>
#include <cstring>

namespace esphome::log_page {

static const char *const TAG = "log_page";

static const char PAGE[] = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Bike Dash Log</title>
<style>:root{color-scheme:dark}body{margin:0;background:#05070a;color:#d8dee4;font:13px/1.45 ui-monospace,Menlo,Consolas,monospace}
header{position:sticky;top:0;background:#0d1117;border-bottom:1px solid #1f2933;padding:8px 14px;display:flex;gap:14px;align-items:center;font-family:system-ui,sans-serif}
header b{font-size:15px}header button{background:#1f2933;color:#d8dee4;border:1px solid #33414f;border-radius:6px;padding:4px 10px;cursor:pointer}header button:active{background:#fff;color:#000}#st{color:#7d8b99}label{color:#7d8b99}pre{margin:0;padding:10px 14px;white-space:pre-wrap;word-break:break-word}
.E{color:#ff6b6b}.W{color:#ffb020}.I{color:#3ddc97}.C{color:#7fb3ff}.D{color:#8fa3b8}.V{color:#6c7a89}</style></head><body>
<header><b>Bike Dash log</b><span id="st">connecting…</span><label><input type="checkbox" id="fol" checked> follow</label><button onclick="act('/clear')">Clear log</button><button onclick="act('/clear_ble')">Clear Bluetooth</button></header>
<pre id="out"></pre><script>
let since=0,epoch=null;const out=document.getElementById('out'),st=document.getElementById('st'),MAX=4000;
function add(t){const f=document.createDocumentFragment();for(const l of t.split('\n')){if(!l)continue;const d=document.createElement('div');
 const m=l.match(/^\[(.)\]/);if(m)d.className=m[1];d.textContent=l;f.appendChild(d)}out.appendChild(f);
 while(out.childElementCount>MAX)out.firstChild.remove();if(document.getElementById('fol').checked)scrollTo(0,document.body.scrollHeight)}
async function poll(){try{let r=await fetch('/raw?since='+since);const e=r.headers.get('X-Epoch');if(epoch!==null&&e!==epoch){out.textContent='';since=0;r=await fetch('/raw?since=0')}epoch=e;since=+r.headers.get('X-Total')||since;const t=await r.text();if(t)add(t);
 st.textContent='live · buffer '+r.headers.get('X-Size')+' bytes'}catch(e){st.textContent='offline, retrying…'}setTimeout(poll,1500)}
async function act(u){try{await fetch(u,{method:'POST'})}catch(e){}}
poll();</script></body></html>)HTML";

void LogPage::setup() {
  // Prefer PSRAM so the log never competes with Wi-Fi/BLE for internal RAM.
  this->buf_ = (char *) heap_caps_malloc(this->size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (this->buf_ == nullptr)
    this->buf_ = (char *) malloc(this->size_);
  this->lock_ = xSemaphoreCreateMutex();
  if (this->buf_ == nullptr || this->lock_ == nullptr) {
    this->mark_failed();
    return;
  }
  if (logger::global_logger != nullptr)
    logger::global_logger->add_log_callback(
        this, [](void *self, uint8_t level, const char *tag, const char *msg, size_t len) {
          static_cast<LogPage *>(self)->on_log_(level, tag, msg, len);
        });

  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = this->port_;
  cfg.ctrl_port = 32770;  // :80 and the dash page hold 32768 / 32769
  cfg.max_open_sockets = 3;
  cfg.lru_purge_enable = true;  // drop the oldest idle socket instead of refusing new ones
  if (httpd_start(&this->server_, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Could not start on port %u", this->port_);
    this->mark_failed();
    return;
  }
  httpd_uri_t page{.uri = "/", .method = HTTP_GET, .handler = page_, .user_ctx = this};
  httpd_uri_t raw{.uri = "/raw", .method = HTTP_GET, .handler = raw_, .user_ctx = this};
  httpd_register_uri_handler(this->server_, &page);
  httpd_register_uri_handler(this->server_, &raw);
  httpd_uri_t clr{.uri = "/clear", .method = HTTP_POST, .handler = clear_, .user_ctx = this};
  httpd_uri_t clrb{.uri = "/clear_ble", .method = HTTP_POST, .handler = clear_ble_, .user_ctx = this};
  httpd_register_uri_handler(this->server_, &clr);
  httpd_register_uri_handler(this->server_, &clrb);
  ESP_LOGI(TAG, "Log page on port %u, %u byte ring buffer", this->port_, (unsigned) this->size_);
}

void LogPage::put_(const char *s, size_t n) {
  for (size_t i = 0; i < n; i++) {
    this->buf_[this->head_++] = s[i];
    if (this->head_ == this->size_) {
      this->head_ = 0;
      this->wrapped_ = true;
    }
  }
  this->total_ += n;
}

void LogPage::on_log_(uint8_t level, const char *tag, const char *msg, size_t len) {
  if (this->buf_ == nullptr)
    return;
  // Messages already start with "[I][tag:line]:"; just strip the ANSI colour codes.
  char line[512];
  size_t o = 0;
  for (size_t i = 0; i < len && o < sizeof(line) - 2; i++) {
    if (msg[i] == 0x1b) {
      while (i < len && msg[i] != 'm')
        i++;
      continue;
    }
    line[o++] = msg[i];
  }
  line[o++] = '\n';
  if (this->quiet_ && is_ble_(line, o))
    return;
  if (xSemaphoreTake(this->lock_, pdMS_TO_TICKS(5)) != pdTRUE)
    return;  // never stall a logging task
  this->put_(line, o);
  xSemaphoreGive(this->lock_);
}

// Bluetooth chatter: scanner, client, discovery and raw BMS frames.
bool LogPage::is_ble_(const char *line, size_t n) {
  static const char *const TAGS[] = {"][esp32_ble", "][ble_client", "][autodiscover", "][ant_bms_ble", "BT_", "BTU_TASK", "BTC_TASK"};
  size_t lim = n < 80 ? n : 80;  // tag sits at the start of the line
  for (const char *t : TAGS) {
    size_t tl = strlen(t);
    for (size_t i = 0; i + tl <= lim; i++)
      if (memcmp(line + i, t, tl) == 0)
        return true;
  }
  return false;
}

void LogPage::loop() {
  if (!this->quiet_fn_)
    return;
  bool q = this->quiet_fn_();
  if (q && !this->quiet_)
    this->purge_ble_();
  this->quiet_ = q;
}

// Rewrite the ring without Bluetooth lines (runs once, when quiet mode switches on).
void LogPage::purge_ble_() {
  char *tmp = (char *) heap_caps_malloc(this->size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (tmp == nullptr)
    return;
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  size_t have = this->wrapped_ ? this->size_ : this->head_;
  size_t start = this->wrapped_ ? this->head_ : 0;
  size_t n = 0;
  char line[512];
  size_t lo = 0;
  for (size_t i = 0; i < have; i++) {
    char ch = this->buf_[(start + i) % this->size_];
    if (lo < sizeof(line))
      line[lo++] = ch;
    if (ch == '\n') {
      if (!is_ble_(line, lo)) {
        memcpy(tmp + n, line, lo);
        n += lo;
      }
      lo = 0;
    }
  }
  memcpy(this->buf_, tmp, n);
  this->head_ = n;
  this->wrapped_ = false;
  this->epoch_++;  // tells open log pages to clear and refetch
  xSemaphoreGive(this->lock_);
  free(tmp);
  ESP_LOGI(TAG, "BMS connected: Bluetooth lines purged from the log (%u bytes kept)", (unsigned) n);
}

void LogPage::clear() {
  xSemaphoreTake(this->lock_, portMAX_DELAY);
  this->head_ = 0;
  this->wrapped_ = false;
  this->epoch_++;
  xSemaphoreGive(this->lock_);
}

esp_err_t LogPage::clear_(httpd_req_t *req) {
  static_cast<LogPage *>(req->user_ctx)->clear();
  return httpd_resp_sendstr(req, "ok");
}

esp_err_t LogPage::clear_ble_(httpd_req_t *req) {
  static_cast<LogPage *>(req->user_ctx)->purge_ble_();
  return httpd_resp_sendstr(req, "ok");
}

esp_err_t LogPage::page_(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, PAGE, sizeof(PAGE) - 1);
}

// GET /raw?since=N returns only bytes written after N (or the whole buffer if N fell off the end).
esp_err_t LogPage::raw_(httpd_req_t *req) {
  auto *self = static_cast<LogPage *>(req->user_ctx);
  uint32_t since = 0;
  char q[32] = {0}, v[16] = {0};
  if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK && httpd_query_key_value(q, "since", v, sizeof(v)) == ESP_OK)
    since = strtoul(v, nullptr, 10);

  // Copy out under the lock, send after releasing it.
  static char *snap = nullptr;
  if (snap == nullptr)
    snap = (char *) heap_caps_malloc(self->size_, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (snap == nullptr)
    return httpd_resp_send_500(req);
  size_t n = 0;
  uint32_t total;
  xSemaphoreTake(self->lock_, portMAX_DELAY);
  total = self->total_;
  size_t have = self->wrapped_ ? self->size_ : self->head_;
  size_t want = (since > total || total - since > have) ? have : total - since;
  size_t start = (self->head_ + self->size_ - want) % self->size_;
  for (size_t i = 0; i < want; i++)
    snap[n++] = self->buf_[(start + i) % self->size_];
  xSemaphoreGive(self->lock_);

  // When the buffer has wrapped, drop the first partial line.
  size_t off = 0;
  if (since == 0 && self->wrapped_)
    while (off < n && snap[off++] != '\n') {
    }
  char hdr[16];
  snprintf(hdr, sizeof(hdr), "%lu", (unsigned long) total);
  httpd_resp_set_hdr(req, "X-Total", hdr);
  char sz[16];
  snprintf(sz, sizeof(sz), "%u", (unsigned) self->size_);
  httpd_resp_set_hdr(req, "X-Size", sz);
  char ep[12];
  snprintf(ep, sizeof(ep), "%lu", (unsigned long) self->epoch_);
  httpd_resp_set_hdr(req, "X-Epoch", ep);
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_send(req, snap + off, n - off);
}

}  // namespace esphome::log_page
