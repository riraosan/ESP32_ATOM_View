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

#include <memory>
#include <Arduino.h>
#include <Ticker.h>

#include <message.h>
#include <secrets.h>

#if defined(ATOM_DOC)
#include <ATOMDoc.hpp>
using ATOM = ATOMDoc;
#elif defined(ATOM_VIEW)
#include <ATOMView.hpp>
using ATOM = ATOMView;
#endif

class Controller {
public:
 Controller() {
 }

  static void sendMessage(MESSAGE message) {
    _message = message;
  }

  static void setNtpTime(void) {
    _clock = true;
  }

  static void updatePeriod(void) {
    sendMessage(MESSAGE::MSG_UPDATE_DOCUMENT);
  }

  void setNtpClock(void) {
    char        time[16] = {0};
    char        ymd[16]  = {0};
    const char *wday[]   = {"Sun.", "Mon.", "Tue.", "Wed.", "Thu.", "Fri.", "Sat."};

    struct tm info;

    if (getLocalTime(&info)) {
      sprintf(time, "%02d:%02d:%02d", info.tm_hour, info.tm_min, info.tm_sec);
      sprintf(ymd, "%s %02d %02d %04d", wday[info.tm_wday], info.tm_mon + 1, info.tm_mday, info.tm_year + 1900);
      // log_i("%s %s", ymd, time);

      if (info.tm_hour == 23 && info.tm_min == 59 && info.tm_sec == 59) {
        ESP.restart();
        delay(1000);
        return;
      }
    }

    _atom.setDayTime(ymd, time);

    _clock = false;
  }

  void begin(void) {
    if (!SPIFFS.begin()) {
      log_e("fail to mount.");
    }
    _atom.startDocAPI();

    _atom.setAreaCode(27000);
    _atom.begin(SECRET_SSID, SECRET_PASS);

    _serverChecker.attach(30, updatePeriod);

    configTzTime(TIME_ZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    _ntpClocker.attach_ms(500, setNtpTime);

    log_d("Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  }

  void update(void) {
    switch (_message) {
      case MESSAGE::MSG_UPDATE_DOCUMENT:
        _atom.sendMessage(_message);
        _atom.update();
        // log_i("MESSAGE::MSG_UPDATE_DOCUMENT");
        sendMessage(MESSAGE::MSG_UPDATE_NOTHING);
        break;
      default:
        break;
    }

    if (_clock) {
      setNtpClock();
      _atom.sendMessage(MESSAGE::MSG_UPDATE_CLOCK);
    }
    _atom.update();

    delay(1);
  }

  static MESSAGE _message;
  static bool    _clock;

private:
  Ticker _serverChecker;
  Ticker _ntpClocker;
  ATOM   _atom;

  String _ntpTime;
};

MESSAGE Controller::_message = MESSAGE::MSG_UPDATE_NOTHING;
bool    Controller::_clock   = false;
