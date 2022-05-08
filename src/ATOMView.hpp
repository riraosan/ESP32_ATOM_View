
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Display.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <WiFiClient.h>
#include <esp32-hal-log.h>

#include <Connect.hpp>
#include <memory>

class ATOMView : public Connect {
public:
 ATOMView() : Connect("atom_view", "ATOM_VIEW-G", 80),
              _doc(700),
              _apiURI("/api/v1/weather.json") {
 }

  void begin(const char *ssid, const char *password) {
    _disp.begin();
    _disp.displayColorBar();
    Connect::begin(ssid, password);
  }

  void sendMessage(MESSAGE message) {
    _message = message;
  }

  void setDayTime(String day, String time) {
    _day  = day;
    _time = time;
    sendMessage(MESSAGE::MSG_UPDATE_CLOCK);
  }

  void requestWeatherJson(void) {
    std::unique_ptr<HTTPClient> http(new HTTPClient);
    std::unique_ptr<WiFiClient> client(new WiFiClient);

    IPAddress ip(MDNS.queryHost("atom_doc"));
    log_i("%s", ip.toString().c_str());
    http->begin(*client, ip.toString(), 80, _apiURI.c_str());

    int httpCode = http->GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        // ReadLoggingStream loggingStream(http->getStream(), Serial);
        if (http->connected()) {
          _doc.clear();
          deserializeJson(_doc, http->getStream());
        }
      }
    } else {
      String error(http->errorToString(httpCode));
      log_e("[HTTP] GET... failed, error: %s", error.c_str());
    }

    http->end();
    return;
  }

  void parseWeatherJson(void) {
    _publishingOffice = (const char*)_doc["publishingOffice"];  // "大阪管区気象台"
    _reportDatetime   = (const char*)_doc["reportDatetime"];    // "2022-05-06T17:00:00+09:00"
    _timeDefines      = (const char*)_doc["timeSeries"][0]["timeDefines"];

    JsonObject areas_0 = _doc["timeSeries"][0]["areas"][0];
    if (!areas_0.isNull()) {
      _area         = (const char*)areas_0["area"];          // "大阪府"
      _weatherCodes = (const char*)areas_0["weatherCodes"];  // "100"
      _weathers_jp  = (const char*)areas_0["weathers_jp"];   // "晴"
      _weathers_en  = (const char*)areas_0["weathers_en"];   // "CLEAR"
      _winds        = (const char*)areas_0["winds"];         // "南西の風　後　北東の風"
      _waves        = (const char*)areas_0["waves"];         // "０．５メートル"
      _degree       = (float)areas_0["degree"];              // 25.4
      _humidity     = (float)areas_0["humidity"];            // 44.5
      _pressure     = (float)areas_0["pressure"];            // 1017.2

      log_i("%s, %s, %s, %s, %s",
            _publishingOffice.c_str(),
            _reportDatetime.c_str(),
            _timeDefines.c_str(),
            _area.c_str(),
            _weatherCodes.c_str());

      log_i("%s, %s, %s, %s, %2.1f, %2.0f, %4.1f",
            _weathers_jp.c_str(),
            _weathers_en.c_str(),
            _winds.c_str(),
            _waves.c_str(),
            _degree,
            _humidity,
            _pressure);
    }
  }

  void update(void) {
    switch (_message) {
      case MESSAGE::MSG_UPDATE_DOCUMENT:
        requestWeatherJson();
        parseWeatherJson();

        _disp.setDegree(_degree);
        _disp.setHumidity(_humidity);
        _disp.setAtomPressure(_pressure);
        _disp.setWeatherforcastJP(_weathers_jp);
        _disp.setWeatherforcastEN(_weathers_en);
        // _disp.setImageFilename(_filename);
        log_i("MESSAGE::MSG_UPDATE_DOCUMENT");
        sendMessage(MESSAGE::MSG_UPDATE_NOTHING);
        break;
      case MESSAGE::MSG_UPDATE_CLOCK:
        _disp.setYMD(_day);
        _disp.setNtpTime(_time);
        // log_i("MESSAGE::MSG_UPDATE_CLOCK");
        sendMessage(MESSAGE::MSG_UPDATE_NOTHING);
        break;
      default:
        break;
    }

    _portal.handleClient();
    _disp.update();

    delay(1);
  }

private:
  Display _disp;

  DynamicJsonDocument _doc;

  String _day;
  String _time;

  String _publishingOffice;
  String _reportDatetime;
  String _timeDefines;
  String _area;
  String _weatherCodes;
  String _weathers_jp;
  String _weathers_en;
  String _winds;
  String _waves;

  float _degree;
  float _humidity;
  float _pressure;

  String _imageName;

  String _apiURI;
};
