#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Display.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <ThingSpeak.h>
#include <WiFiClientSecure.h>  //for JMA & ThingSpeak
#include <esp32-hal-log.h>

#include <Connect.hpp>
#include <memory>

class ATOMDoc : public Connect {
 public:
  ATOMDoc(void) : Connect("atom_doc", "ATOM_DOC-G", 80),
                  _url("https://www.jma.go.jp/bosai/forecast/data/forecast/__WEATHER_CODE__0.json"),
                  _codeDoc(6144) {
    // R"()" = Raw String Literals(C++)
    _filter = R"(
    [
      {
        "publishingOffice": true,
        "reportDatetime": true,
        "timeSeries": [
          {
            "timeDefines": true,
            "areas": [
              {
                "area": true,
                "weatherCodes": true,
                "weathers": true,
                "winds": true,
                "waves": true
              }
            ]
          }
        ]
      }
    ])";
  }

  void startDocAPI(void) {
    _server.on("/api/v1/weather.json", [&]() {
      _server.send(200, "application/json", _json);
    });
  }

  void requestWeatherInfomation(void) {
    log_d("Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);

    client->setCACert(ts_root_ca);
    client->setHandshakeTimeout(180);
    ThingSpeak.begin(*client);

    int statusCode = ThingSpeak.readMultipleFields(1441019);

    if (statusCode == 200) {
      // Fetch the stored data
      log_i("Fetch the stored data. code %d", statusCode);

      float degree   = ThingSpeak.getFieldAsFloat(1);  // Field 1
      float humidity = ThingSpeak.getFieldAsFloat(2);  // Field 2
      float pressure = ThingSpeak.getFieldAsFloat(3);  // Field 3

      log_i("%2.1f*C, %2f%%, %4.1fhPa", degree, humidity, pressure);

      _degree   = degree;
      _humidity = humidity;
      _pressure = pressure;
    } else {
      log_e("Problem reading channel. HTTP error code %d", statusCode);
    }
  }

  void requestWeatherJson(void) {
    std::unique_ptr<WiFiClientSecure> _jmaClient(new WiFiClientSecure);
    std::unique_ptr<HTTPClient>       _httpClient(new HTTPClient);

    _jmaClient->setCACert(jma_root_ca);
    _jmaClient->setHandshakeTimeout(180);
    _httpClient->setReuse(true);
    _url.replace("__WEATHER_CODE__", String(_localGovernmentCode));

    _httpClient->begin(*_jmaClient, _url.c_str());

    int httpCode = _httpClient->GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        ReadLoggingStream loggingStream(_httpClient->getStream(), Serial);

        StaticJsonDocument<500> codeFilter;
        deserializeJson(codeFilter, _filter.c_str());
        DeserializationError error = deserializeJson(_codeDoc, loggingStream, DeserializationOption::Filter(codeFilter));

        if (error) {
          log_e("deserializeJson() failed: %s", error.f_str());
          _httpClient->end();
          return;
        }

        _httpClient->end();
        return;
      }
    }

    String error(_httpClient->errorToString(httpCode));
    _httpClient->end();

    log_e("[HTTP] GET... failed, error: %s", error.c_str());
    return;
  }

  void parseWeatherJson(void) {
    JsonObject root_0 = _codeDoc[0];  // 0 or 1

    if (!root_0.isNull()) {
      const char *publishingOffice = root_0["publishingOffice"];
      const char *reportDatetime   = root_0["reportDatetime"];

      _publishingOffice = (const char *)publishingOffice;
      _reportDatetime   = (const char *)reportDatetime;

      Serial.println("publishingOffice  " + _publishingOffice);
      Serial.println("  reportDatetime  " + _reportDatetime);

      JsonArray timeSeries = root_0["timeSeries"];

      if (!timeSeries.isNull()) {
        const char *timedef = timeSeries[0]["timeDefines"][0];

        JsonObject  areas_0   = timeSeries[0]["areas"][0];
        const char *area_name = areas_0["area"]["name"];
        const char *area_code = areas_0["area"]["code"];

        _timeDef  = (const char *)timedef;
        _areaName = (const char *)area_name;
        _areaCode = (const char *)area_code;
        // Serial.println("       area_name  " + String((const char *)area_name));
        // Serial.println("       area_code  " + String((const char *)area_code));

        JsonArray weatherCodes = areas_0["weatherCodes"];
        JsonArray weathers     = areas_0["weathers"];
        JsonArray winds        = areas_0["winds"];
        JsonArray waves        = areas_0["waves"];

        if (!weatherCodes.isNull()) {
          _todayForecast = (const char *)weatherCodes[0];
        }

        if (!weathers.isNull()) {
          _weathers0 = (const char *)weathers[0];
        }

        if (!winds.isNull()) {
          _winds0 = (const char *)winds[0];
        }

        if (!waves.isNull()) {
          _waves0 = (const char *)waves[0];
        }
      }

      //天気予報コードより、予報文言とアイコンファイル名を取得する
      String filter(R"({"__CODE__": [true]})");
      filter.replace("__CODE__", _todayForecast);

      StaticJsonDocument<255> forecastDoc;
      StaticJsonDocument<50>  forecastfilter;
      deserializeJson(forecastfilter, filter);

      File file = SPIFFS.open("/codes.json");

      if (file) {
        DeserializationError error = deserializeJson(forecastDoc,
                                                     file,
                                                     DeserializationOption::Filter(forecastfilter));

        if (error) {
          log_e("Failed to read codes.json.");
          file.close();
          return;
        }

        JsonArray root = forecastDoc[_todayForecast.c_str()];

        if (!root.isNull()) {
          _iconFile   = String("/") + String((const char *)root[0]);  // "100.gif"
          _forecastJP = (const char *)root[3];                        // "晴"
          _forecastEN = (const char *)root[4];                        // "CLEAR"

          // log_d("アイコンファイル名:%s\n予報（日本語）:%s\n予報（英語）:%s", _iconFile.c_str(), _forecastJP.c_str(), _forecastEN.c_str());
        }

        file.close();
      }
    }
  }

  void saveJson(void) {
    String json(R"({"publishingOffice": "__OFFICE__", "reportDatetime": "__REPORT__", "timeSeries": [{"timeDefines": "__TIMEDEF__", "areas": [{"area": "__AREA__", "weatherCodes": "__CODES__", "weathers_jp": "__WEATHERSJP__", "weathers_en": "__WEATHERSEN__", "winds": "__WINDS__", "waves": "__WAVES__", "degree": __DEGREE__, "humidity": __HUMID__, "pressure": __PRESS__}]}]})");

    json.replace("__OFFICE__", _publishingOffice);
    json.replace("__REPORT__", _reportDatetime);
    json.replace("__TIMEDEF__", _timeDef);
    json.replace("__AREA__", _areaName);
    json.replace("__CODES__", _todayForecast);
    json.replace("__WEATHERSJP__", _forecastJP);
    json.replace("__WEATHERSEN__", _forecastEN);
    json.replace("__WINDS__", _winds0);
    json.replace("__WAVES__", _waves0);
    json.replace("__DEGREE__", String(_degree));
    json.replace("__HUMID__", String(_humidity));
    json.replace("__PRESS__", String(_pressure));

    _json = json;

    log_i("%s", _json.c_str());
  }

  void setAreaCode(uint16_t localGovernmentCode) {
    _localGovernmentCode = localGovernmentCode;
  }

  void setDayTime(String day, String time) {
    _day  = day;
    _time = time;
  }

  String getTodayForecast(void) {
    return _todayForecast;
  }

  String getNextdayForecast(void) {
    return _nextdayForecast;
  }

  String getWeathersJp(void) {
    return _forecastJP;
  }

  String getWeathersEn(void) {
    return _forecastEN;
  }

  String getICONFilename(void) {
    return _iconFile;
  }

  void update(void) {
    switch (_message) {
      case MESSAGE::MSG_UPDATE_DOCUMENT:
        requestWeatherInfomation();
        requestWeatherJson();
        parseWeatherJson();
        saveJson();

        _debugPrint();

        log_i("MESSAGE::MSG_UPDATE_DOCUMENT");
        sendMessage(MESSAGE::MSG_UPDATE_NOTHING);
        break;
      default:
        break;
    }

    _portal.handleClient();
  }

 private:
  void _debugPrint(void) {
    log_i("%s, %s", _day.c_str(), _time.c_str());
    log_i("%s", _url.c_str());

    log_i("%s, %s, %s, %s, %s",
          _publishingOffice.c_str(),
          _reportDatetime.c_str(),
          _areaName.c_str(),
          _areaCode.c_str(),
          _todayForecast.c_str());

    log_i("%s, %s, %s, %s, %s, %s, %f, %f, %f",
          _weathers0.c_str(),
          _winds0.c_str(),
          _waves0.c_str(),
          _forecastJP.c_str(),
          _forecastEN.c_str(),
          _iconFile.c_str(),
          _degree,
          _humidity,
          _pressure);
  }

  String _day;
  String _time;

  uint16_t _localGovernmentCode;
  String   _request;
  String   _response;
  String   _url;
  String   _json;

  String _publishingOffice;
  String _reportDatetime;
  String _timeDef;
  String _areaName;
  String _areaCode;
  String _todayForecast;
  String _nextdayForecast;
  String _weathers0;
  String _weathers1;
  String _winds0;
  String _waves0;
  String _forecastJP;
  String _forecastEN;
  String _iconFile;

  float _degree;
  float _humidity;
  float _pressure;

  String _filter;

  DynamicJsonDocument _codeDoc;
};
