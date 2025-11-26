
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "Arduino.h"

extern int gpLed;
extern String WiFiAddr;

static httpd_handle_t ui_httpd = NULL;
static httpd_handle_t stream_httpd = NULL;


static volatile int currentMode = 0;


static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    char part_buf[128];

    httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            delay(2);
            continue;
        }

        size_t hlen = snprintf(part_buf, sizeof(part_buf),
            "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
            (uint32_t)fb->len);

        if (httpd_resp_send_chunk(req, part_buf, hlen) != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        if (httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len) != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        // finish this frame
        if (httpd_resp_send_chunk(req, "\r\n", 2) != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        esp_camera_fb_return(fb);
        fb = NULL;

        delay(2);
    }

    return res;
}

// --------- UI page (index) ----------
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{background:#000;color:#fff;font-family:Arial;text-align:center} img{width:92%;max-width:640px;border-radius:10px;display:block;margin:10px auto} ";
    html += "button{padding:12px 20px;margin:8px;border-radius:8px;border:0;background:#444;color:#fff;font-weight:600} button:active{transform:scale(.98)} ";
    html += ".mode{margin-top:8px;margin-bottom:12px} .slider{appearance:none;width:60px;height:34px;border-radius:20px;background:#666;display:inline-block;vertical-align:middle} .info{margin-bottom:6px}</style>";

    html += "<script>";
    // set stream src dynamically (host + port 81)
    html += "function setStreamSrc(){ var img=document.getElementById('stream'); img.src = 'http://' + window.location.hostname + ':81/stream'; }";
    // send single command
    html += "function sendCmd(ch){ fetch('/cmd?c='+encodeURIComponent(ch)); }";
    // hold to repeat command while pressed (for movement)
    html += "var holdId=null; function holdStart(ch){ if(holdId) clearInterval(holdId); fetch('/cmd?c='+ch); holdId=setInterval(()=>fetch('/cmd?c='+ch),120);} function holdStop(){ if(holdId) clearInterval(holdId); holdId=null; fetch('/cmd?c=S'); }";
    // mode functions
    html += "function setMode(v){ fetch('/mode?m='+v); }";
    html += "async function refreshMode(){ try{ let r=await fetch('/mode/status'); let j=await r.json(); document.getElementById('mtext').innerText = j.mode? 'Mode: AUTO' : 'Mode: MANUAL'; document.getElementById('mswitch').checked = !!j.mode; ";
    html += " var mov = document.getElementsByClassName('mov'); for(var i=0;i<mov.length;i++){ mov[i].disabled = j.mode? true:false; mov[i].style.opacity = j.mode? 0.45:1.0; } } catch(e){ console.log('mode err',e);} }";
    html += "setInterval(refreshMode,800); window.onload = function(){ setStreamSrc(); refreshMode(); }";
    html += "</script></head><body>";

    html += "<h2>ESP32-CAM Fire-RC</h2>";
    html += "<img id='stream' alt='stream' />";

    html += "<div class='mode'><div id='mtext'>Mode: ...</div>";
    html += "<label style='display:block;margin-top:6px'><input id='mswitch' type='checkbox' onchange='setMode(this.checked?1:0)' class='slider' /></label></div>";

    html += "<div>";
    html += "<div><button class='mov' onmousedown=\"holdStart('F')\" onmouseup=\"holdStop()\" onmouseleave=\"holdStop()\">FORWARD</button></div>";
    html += "<div><button class='mov' onmousedown=\"holdStart('L')\" onmouseup=\"holdStop()\" onmouseleave=\"holdStop()\">LEFT</button>";
    html += "<button onclick=\"sendCmd('S')\">STOP</button>";
    html += "<button class='mov' onmousedown=\"holdStart('R')\" onmouseup=\"holdStop()\" onmouseleave=\"holdStop()\">RIGHT</button></div>";
    html += "<div><button class='mov' onmousedown=\"holdStart('B')\" onmouseup=\"holdStop()\" onmouseleave=\"holdStop()\">BACK</button></div>";

    html += "<div style='margin-top:12px'><button onclick=\"sendCmd('P')\">SPRAY</button></div>";

    html += "<div style='margin-top:10px'><button onclick=\"sendCmd('V')\">SERVO LEFT</button>";
    html += "<button onclick=\"sendCmd('C')\">CENTER</button>";
    html += "<button onclick=\"sendCmd('W')\">SERVO RIGHT</button></div>";

    html += "<div style='margin-top:12px'><button onclick=\"sendCmd('O')\">LIGHT ON</button>";
    html += "<button onclick=\"sendCmd('X')\">LIGHT OFF</button></div>";

    html += "</body></html>";

    return httpd_resp_send(req, html.c_str(), html.length());
}


static esp_err_t cmd_handler(httpd_req_t *req) {
    char query[48];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[8];
        if (httpd_query_key_value(query, "c", val, sizeof(val)) == ESP_OK) {
            char cmd = val[0];
            
            if (currentMode == 1) {
                if (cmd == 'F' || cmd == 'B' || cmd == 'L' || cmd == 'R' || cmd == 'S') {
                    
                    Serial.printf("IGNORED (AUTO): %c\n", cmd);
                    httpd_resp_send(req, "IGNORED_AUTO", strlen("IGNORED_AUTO"));
                    return ESP_OK;
                }
            }

            Serial1.println(val);
            Serial.printf("TX->UNO: %s\n", val);
        }
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}


static esp_err_t mode_set_handler(httpd_req_t *req) {
    char q[32];
    if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK) {
        char v[4] = {0};
        if (httpd_query_key_value(q, "m", v, sizeof(v)) == ESP_OK) {
            currentMode = atoi(v);
            Serial.printf("Mode set -> %d\n", currentMode);
        }
    }
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}
static esp_err_t mode_status_handler(httpd_req_t *req) {
    char out[32];
    int n = snprintf(out, sizeof(out), "{\"mode\":%d}", currentMode);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, out, n);
    return ESP_OK;
}

// ---------- LED handlers ----------
static esp_err_t led_on_handler(httpd_req_t *req) {
    digitalWrite(gpLed, HIGH);
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}
static esp_err_t led_off_handler(httpd_req_t *req) {
    digitalWrite(gpLed, LOW);
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}


void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();


    if (httpd_start(&ui_httpd, &config) == ESP_OK) {
        httpd_uri_t index_uri = { "/", HTTP_GET, index_handler, NULL };
        httpd_uri_t cmd_uri   = { "/cmd", HTTP_GET, cmd_handler, NULL };
        httpd_uri_t mode_uri  = { "/mode", HTTP_GET, mode_set_handler, NULL };
        httpd_uri_t mode_stat = { "/mode/status", HTTP_GET, mode_status_handler, NULL };
        httpd_uri_t led_on    = { "/led/on", HTTP_GET, led_on_handler, NULL };
        httpd_uri_t led_off   = { "/led/off", HTTP_GET, led_off_handler, NULL };

        httpd_register_uri_handler(ui_httpd, &index_uri);
        httpd_register_uri_handler(ui_httpd, &cmd_uri);
        httpd_register_uri_handler(ui_httpd, &mode_uri);
        httpd_register_uri_handler(ui_httpd, &mode_stat);
        httpd_register_uri_handler(ui_httpd, &led_on);
        httpd_register_uri_handler(ui_httpd, &led_off);

        Serial.printf("UI server started on port %d\n", config.server_port);
    } else {
        Serial.println("Failed to start UI server");
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_uri_t stream_uri = { "/stream", HTTP_GET, stream_handler, NULL };
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.printf("Stream server started on port %d\n", config.server_port);
    } else {
        Serial.println("Failed to start stream server");
    }
}
