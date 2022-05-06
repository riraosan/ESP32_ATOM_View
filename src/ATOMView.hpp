
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
               _doc(500) {
  }

  void begin(void) {
    _disp.begin();
    Connect::begin(SECRET_SSID, SECRET_PASS);
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
    log_i("requestWeatherJson");
    std::unique_ptr<HTTPClient> http(new HTTPClient);
    std::unique_ptr<WiFiClient> client(new WiFiClient);

    http->begin(*client, "http://192.168.1.51/api/v1/weather.json");

    int httpCode = http->GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        ReadLoggingStream loggingStream(http->getStream(), Serial);
        deserializeJson(_doc, loggingStream);
      }
    } else {
      String error(http->errorToString(httpCode));
      log_e("[HTTP] GET... failed, error: %s", error.c_str());
    }

    http->end();
    return;
  }

  void parseWeatherJson(void) {
  }

  void update(void) {
    switch (_message) {
      case MESSAGE::MSG_UPDATE_DOCUMENT:
        requestWeatherJson();
        parseWeatherJson();

        // _disp.setDegree(_degree);
        // _disp.setHumidity(_humidity);
        // _disp.setAtomPressure(_pressure);
        // _disp.setWeatherforcastJP(_forecastJP);
        // _disp.setWeatherforcastEN(_forecastEN);
        // _disp.setImageFilename(_filename);
        log_i("MESSAGE::MSG_UPDATE_DOCUMENT");
        sendMessage(MESSAGE::MSG_UPDATE_NOTHING);
        break;
      case MESSAGE::MSG_UPDATE_CLOCK:
        _disp.setYMD(_day);
        _disp.setNtpTime(_time);
        // log_i("%s %s", _day.c_str(), _time.c_str());
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

  String _day;
  String _time;

  float  _degree;      // 気温
  float  _humidity;    //湿度
  float  _pressure;    //気圧
  String _forecastJP;  //日本語予報
  String _forecastEN;  //英語予報
  String _filename;    //アイコンファイル名

  DynamicJsonDocument _doc;
};
