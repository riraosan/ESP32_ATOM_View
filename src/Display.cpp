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

#include <Display.h>
#include <esp32-hal-log.h>

MESSAGE Display::_message = MESSAGE::MSG_UPDATE_NOTHING;

ESP32_8BIT_CVBS Display::_display;
M5Canvas        Display::_animation;

File Display::_file;

int Display::_width  = 0;
int Display::_height = 0;

Display::Display() : _isOpen(false),
                     _time(""),
                     _day(""),
                     _daytimeFormat("__YMD__ __NTP__"),
                     _degree(0.0),
                     _humidity(0.0),
                     _pressure(0.0),
                     _forecastJP(""),
                     _forecastEN(""),
                     _filename(""),
                     _bgColor(0x10cd),
                     _bgTitle(0x0019),
                     _bgTemperature(0x1c43),
                     _bgPressure(0x1c43),
                     _bgHumidity(0x1c43) {
}

void Display::sendMessage(MESSAGE message) {
  _message = message;
}

void Display::begin(void) {
  _display.begin();
  _display.startWrite();

  _width  = 140;
  _height = 160;

  //描画エリア
  _display.fillScreen(_bgColor);
  _display.setRotation(0);
  _display.setColorDepth(8);

  //スプライト
  _data.fillSprite(_bgColor);
  _data.setFont(&fonts::efont);
  _data.setTextWrap(true, true);
  _data.setPsram(false);
  _data.setColorDepth(8);

  _title.fillSprite(_bgColor);
  _title.setFont(&fonts::efont);
  _title.setTextWrap(true, true);
  _title.setPsram(false);
  _title.setColorDepth(8);

  _animation.fillSprite(_bgColor);
  _animation.setPsram(false);
  _animation.setColorDepth(8);
}

void Display::setNtpTime(String ntpTime) {
  _time = ntpTime;
}

void Display::setYMD(String ymd) {
  _day = ymd;
}

void Display::displayTitle(void) {
  if (!_title.createSprite(239, 32)) {
    log_e("title allocation failed");
    return;
  }

  _title.fillSprite(_bgTitle);

  _title.setTextColor(0xFFFF, _bgTitle);
  _title.setTextSize(1);
  _title.setCursor(0, 16 * 0);
  _title.print(" Osaka Weather Station");

  String format(_daytimeFormat);
  format.replace("__NTP__", _time);
  format.replace("__YMD__", _day);
  _title.setCursor(16, 16 * 1);
  _title.print(format.c_str());

  _title.pushSprite(&_display, 2, 9);
  _title.deleteSprite();
}

void Display::setDegree(float degree) {
  _degree = degree;
}

void Display::setHumidity(float humidity) {
  _humidity = humidity;
}
void Display::setAtomPressure(float pressure) {
  _pressure = pressure;
}
void Display::setWeatherforcastJP(String forecastJP) {
  _forecastJP = forecastJP;
}
void Display::setWeatherforcastEN(String forecastEN) {
  _forecastEN = forecastEN;
}
void Display::displayWeather(void) {
  if (!_data.createSprite(174, 96)) {
    log_e("data allocation failed");
    return;
  }

  char tempe[10] = {0};
  char humid[10] = {0};
  char press[10] = {0};

  sprintf(tempe, "%2.1f", _degree);
  sprintf(humid, "%2.0f", _humidity);
  sprintf(press, "%4.1f", _pressure);

  _data.fillSprite(_bgColor);

  // 予報（日本語）
  _data.setCursor(0, 16 * 0);
  _data.setTextColor(0xFFFF, _bgColor);
  _data.setTextSize(2);
  _data.print(" ");
  _data.print(_forecastJP.c_str());

  // 予報（英語）
  _data.setCursor(0, 16 * 2);
  _data.setTextColor(0xFFFF, _bgColor);
  _data.setTextSize(1);
  _data.print("  ");
  _data.print(_forecastEN.c_str());

  // 気温
  _data.setCursor(0, 16 * 3);
  _data.setTextColor(0xFFFF, _bgTemperature);
  _data.setTextSize(1);
  _data.print("   Degree:");
  _data.print(tempe);
  _data.print("*C     ");

  // 湿度
  _data.setCursor(0, 16 * 4);
  _data.setTextColor(0xFFFF, _bgHumidity);
  _data.setTextSize(1);
  _data.print(" Humidity:");
  _data.print(humid);
  _data.print("%        ");

  // 大気圧
  _data.setCursor(0, 16 * 5);
  _data.setTextColor(0xFFFF, _bgPressure);
  _data.setTextSize(1);
  _data.print(" Pressure:");
  _data.print(press);
  _data.print("hPa  ");

  _data.pushSprite(&_display, 2, 140);
  _data.deleteSprite();
}

void Display::setImageFilename(String filename) {
  _filename = filename;
}

void Display::openImageFile(void) {
  if (_isOpen) {
    _gif.close();
  }

  if (_gif.open(_filename.c_str(), _GIFOpenFile, _GIFCloseFile, _GIFReadFile, _GIFSeekFile, _GIFDraw)) {
    _isOpen = true;
    log_d("success to open %s", _filename.c_str());
  } else {
    _isOpen = false;
    log_e("failure to open %s", _filename.c_str());
  }
}

void Display::displayImage(void) {
  if (!_animation.createSprite(140, 160)) {
    log_e("image allocation failed. Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  } else {
    _animation.fillSprite(_bgColor);

    if (_isOpen) {
      _gif.playFrame(true, NULL);
    }

    _animation.pushSprite(&_display, 50, 50);
    _animation.deleteSprite();
  }
}

void Display::update() {
  // to Sprite buffer
  //displayImage();
  displayTitle();
  displayWeather();

  // to CVBS buffer
  _display.display();
  delay(1);
}

void *Display::_GIFOpenFile(const char *fname, int32_t *pSize) {
  _file = SPIFFS.open(fname);

  if (_file) {
    *pSize = _file.size();
    return (void *)&_file;
  }

  return NULL;
}

void Display::_GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);

  if (f != NULL)
    f->close();
}

int32_t Display::_GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f    = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1;  // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;

  iBytesRead  = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();

  return iBytesRead;
}

int32_t Display::_GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  int   i = micros();
  File *f = static_cast<File *>(pFile->fHandle);

  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i           = micros() - i;
  // Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
}

void Display::_GIFDraw(GIFDRAW *pDraw) {
  uint8_t  *s;
  uint16_t *d, *usPalette, usTemp[240];
  int       x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > _width)
    iWidth = _width;

  usPalette = pDraw->pPalette;
  y         = pDraw->iY + pDraw->y;  // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2)  // restore to background color
  {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency)  // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int      x, iCount;
    pEnd   = s + iWidth;
    x      = 0;
    iCount = 0;  // count non-transparent pixels
    while (x < iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)  // done, stop
        {
          s--;  // back up to treat it like transparent
        } else  // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      }              // while looking for opaque pixels
      if (iCount) {  // any opaque pixels?
        _animation.setWindow(pDraw->iX + x, y, iCount, 1);
        _animation.pushPixels((uint16_t *)usTemp, iCount, true);
        x += iCount;
        iCount = 0;
      }

      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount) {
        x += iCount;  // skip these
        iCount = 0;
      }
    }
  } else {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < iWidth; x++) {
      usTemp[x] = usPalette[*s++];
    }

    _animation.setWindow(pDraw->iX, y, iWidth, 1);
    _animation.pushPixels((uint16_t *)usTemp, iWidth, true);
  }
}
