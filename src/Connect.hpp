/*
MIT License

Copyright (c) 2021-2022 riraosan.github.io

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <Task.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <memory>

#if defined(ARDUINO_ARCH_ESP32)
#include <Arduino.h>
#include <AutoConnect.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <message.h>
#include <secrets.h>
using WiFiWebServer = WebServer;
#else
#error Only for ESP32
#endif

#include <esp32-hal-log.h>

class Connect : public Task {
 public:
  Connect(String hostName, String apName, uint16_t httpPort) : _portal(_server),
                                                               _hostName(hostName),
                                                               _apName(apName),
                                                               _httpPort(httpPort),
                                                               _message(MESSAGE::MSG_UPDATE_NOTHING) {
    // TODO
    _content = String(R"(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
    </head>
    <body>

    Place the root page with the sketch application.&ensp; __AC_LINK__

    </body>
    </html>)");
  }

  void sendMessage(MESSAGE message) {
    _message = message;
  }

  void startWiFi(void) {
    setTaskName("AutoConnect");
    setTaskSize(4096 * 1);
    setTaskPriority(2);
    setCore(0);
    begin(SECRET_SSID, SECRET_PASS);
    start(nullptr);
  }

  void begin(void) {
    begin("", "");
  }

  void begin(const char *ssid, const char *password) {
    beginAPI();

    _config.autoReconnect = true;
    _config.ota           = AC_OTA_BUILTIN;
    _config.apid          = _apName;

    _portal.config(_config);

    bool result = false;

    if (String(ssid).isEmpty() || String(password).isEmpty()) {
      result = _portal.begin();
    } else {
      result = _portal.begin(ssid, password);
    }

    if (result) {
      log_i("WiFi connected: %s", WiFi.localIP().toString().c_str());

      if (MDNS.begin(_hostName.c_str())) {
        MDNS.addService("http", "tcp", _httpPort);
        log_i("HTTP Server ready! Open http://%s.local/ in your browser\n", _hostName.c_str());
      } else
        log_e("Error setting up MDNS responder");
    } else {
      log_e("ESP32 can't connect to AP.");
      ESP.restart();
    }
  }

  virtual void update(void) = 0;

 protected:
  void beginAPI(void) {
    _server.on("/", [&]() {
      _content.replace("__AC_LINK__", String(AUTOCONNECT_LINK(COG_16)));
      _server.send(200, "text/html", _content);
    });
  }

  void run(void *data) {
    for (;;) {
      switch (_message) {
        case MESSAGE::MSG_UPDATE_DOCUMENT:
          log_i("MESSAGE::MSG_UPDATE_DOCUMENT");
          break;
        case MESSAGE::MSG_UPDATE_DIPLAY:
          log_i("MESSAGE::MSG_UPDATE_DIPLAY");
          break;
        case MESSAGE::MSG_UPDATE_CLOCK:
          log_i("MESSAGE::MSG_UPDATE_CLOCK");
          break;
        case MESSAGE::MSG_UPDATE_NOTHING:
          break;
        default:
          break;
      }

      _portal.handleClient();
      delay(1);
    }
  }

  WiFiWebServer     _server;
  AutoConnectConfig _config;
  AutoConnect       _portal;

  String   _content;
  String   _hostName;
  String   _apName;
  uint16_t _httpPort;

  MESSAGE _message;
};
