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
#include <SPIFFS.h>
#include <AnimatedGIF.h>
#include <ArduinoJson.h>
#include <efontEnableJaMini.h>
#include <efontFontData.h>
#include <M5Unified.h>
#include <ESP32_8BIT_CVBS.h>
#include <message.h>

class Display {
public:
  Display(void);
  void begin(void);
  void update(void);

  void setNtpTime(String ntpTime);
  void setYMD(String ymd);
  void displayTitle(void);

  void setDegree(float degree);
  void setHumidity(float humidity);
  void setAtomPressure(float pressure);
  void setWeatherforcastJP(String forecastJP);
  void setWeatherforcastEN(String forecastEN);
  void displayWeather(void);

  void setImageFilename(String filename);
  void displayImage(void);

  static void sendMessage(MESSAGE message);

private:
  AnimatedGIF _gif;

  static inline void    _GIFDraw(GIFDRAW *pDraw);
  static inline void   *_GIFOpenFile(const char *fname, int32_t *pSize);
  static inline void    _GIFCloseFile(void *pHandle);
  static inline int32_t _GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
  static inline int32_t _GIFSeekFile(GIFFILE *pFile, int32_t iPosition);

  String _time;
  String _day;
  String _daytimeFormat;

  float  _degree;
  float  _humidity;
  float  _pressure;
  String _forecastJP;
  String _forecastEN;
  String _filename;  // Weather Animation GIF

  uint16_t _bgColor;
  uint16_t _bgTitle;
  uint16_t _bgTemperature;
  uint16_t _bgPressure;
  uint16_t _bgHumidity;

  static File    _file;
  static int     _width;
  static int     _height;
  static MESSAGE _message;

  static ESP32_8BIT_CVBS _display;
  static M5Canvas        _animation;
  M5Canvas               _title;
  M5Canvas               _data;
};
