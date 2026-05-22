// main.cpp - setup/loop/UI
//
// gamepad_test - test Bluetooth/BLE controllers - targeting gamepads (8-position dpad or joystick, plus a couple buttons)
// Copyright (c) 2026 David Van Wagner
//
// Classic Bluetooth supported only on classic ESP32 (e.g. not CoreS3, not ESP32-C4)
// BLE (Bluetooth Low Energy) should be supported on any wireless capable ESP32
//
// Dependencies, derivations include
// Bluepad32 example Copyright 2019, 2016-2024 Ricardo Quesada 
// Bluepad32 library Copyright 2019, 2016-2024 Ricardo Quesada
// BTStack library Copyright (C) 2009, 2017 BlueKitchen GmbH
//
// Open source for individual, non-commercial use (BlueKitchen restriction)
// see LICENSE for full details

#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>

#include "gamepad.h"

// Display support for M5 targets (AtomS3, M5Core, M5Stick) and TFT_eSPI
#if defined(__has_include)
# if __has_include(<TFT_eSPI.h>)
#  include <TFT_eSPI.h>
#  static TFT_eSPI tft = TFT_eSPI();
#  define HAVE_TFT
# elif __has_include(<M5Stack.h>)
#  include <M5Stack.h>
#  define HAVE_M5
# elif __has_include(<M5AtomS3.h>)
#  include <M5AtomS3.h>
#  define HAVE_M5
# elif __has_include(<M5StickC.h>)
#  include <M5StickC.h>
#  define HAVE_M5
# elif __has_include(<M5Unified.h>)
#  include <M5Unified.h>
#  define HAVE_M5
# endif
#endif

// Initialize available display (if present)
void initDisplay()
{
#ifdef HAVE_TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
#elif defined(HAVE_M5)
  M5.begin();
  delay(10);
  // Many M5 libraries expose M5.Lcd which is compatible with TFT_eSPI calls
  M5.Lcd.fillScreen(BLACK);
#endif
}

void drawCredit()
{
  auto saveRotation = M5.Lcd.getRotation();
  if (M5.Lcd.width() == 128 && M5.Lcd.height() == 128)
    M5.Lcd.setRotation(1);
  else
    M5.Lcd.setRotation(2);
  const char *credit = " davevw.com ";
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  int x = (M5.Lcd.width() - M5.Lcd.textWidth(credit) - 16) / 2;
  int y = 0;
  M5.Lcd.setCursor(x, y);
  M5.Lcd.print(credit);
  M5.Lcd.setRotation(saveRotation);
  M5.Lcd.endTransaction();
}

void drawInstructions()
{
  const char *instructions[] = {
    "Test Bluetooth HID devices",
    "",
    "Auto binds to pairing devices",
    "gampad activity shows here",
    "and see serial monitoring",
    "for more devices & info"
  };
  int n = sizeof(instructions)/sizeof(*instructions);

  M5.Lcd.setTextSize(1.5);
  if (M5.Lcd.height() <= 135) // Cardputer, Stick-CPlus, Stick-C
    M5.Lcd.setTextSize(1.0);
  M5.Lcd.setCursor(0,0);

  if (M5.Lcd.width() == 128 && M5.Lcd.height() == 128)
  {
    M5.Lcd.println("Test Bluetooth HID");
    return;
  }

  for (int i=0; i<n; ++i)
  {
    M5.Lcd.println(instructions[i]);

    if (M5.Lcd.height() == 80) // Stick-C
      return;
  }
}

// Draw a simple white-on-black title centered if possible
void drawTitle()
{
  const char *title = "gamepad_test";
#ifdef HAVE_TFT
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int w = tft.width();
  int h = tft.height();
  int tw = strlen(full) * 6 * 2; // approximate width: 6 px per char at size 1
  const char *text = (tw <= w) ? full : shortn;
  int tx = (w - (int)strlen(text) * 6 * 2) / 2;
  int ty = (h - 8 * 2) / 2;
  if (tx < 0) tx = 0;
  tft.setCursor(tx, ty);
  tft.print(text);
#elif defined(HAVE_M5)
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  int w = M5.Lcd.width();
  int h = M5.Lcd.height();
  int tx = (w - (int)strlen(title) * 6 * 2) / 2;
  int ty = h - 16;
  int my = 2;
  if (tx < 0) tx = 0;
  M5.Lcd.fillRect(0, ty - my, w, 16 + my, TFT_BLACK);
  M5.Lcd.setCursor(tx, ty);
  M5.Lcd.print(title);
#endif
}

void drawImage()
{
#if defined(HAVE_M5)
    M5.Lcd.fillScreen(TFT_WHITE);

    int xmargin = 16; // reserved for credit
    int ymargin = 18; // reserved for title

    if (M5.Lcd.height() < 135) // M5Stick-C, AtomS3
      xmargin = 4;
    
    const int screenW = M5.Lcd.width();
    const int screenH = M5.Lcd.height();
    const int imgW = 91;
    const int imgH = 183;

    // Calculate centering coordinates
    int x = screenW - imgW - xmargin;
    int y = (screenH - imgH - ymargin) / 2;
    if (y < 0)
      y = 0;

    extern const uint8_t image_91x183[];
    M5.Lcd.drawXBitmap(x, y, image_91x183, imgW, imgH, TFT_BLACK);
#endif    
}

void drawControls(int dpad, int buttons)
{
#if defined(HAVE_M5)    
    int pos[7][2] = {
        {64, 104},
        {64, 84},
        {64, 124},
        {84, 104},
        {44, 104},
        {124, 104},
        {144, 84}
    };

    if (M5.Lcd.height() == 135) // Cardputer, M5Stick-CPlus
    {
      for (int i=0; i<7; ++i)
      {
        pos[i][0] -= 36;
        pos[i][1] -= 30;
      }
    }

    if (M5.Lcd.height() < 135) // M5Stick-C, AtomS3
    {
      for (int i=0; i<7; ++i)
      {
        pos[i][0] -= 43;
        pos[i][1] -= 76;
      }
    }

    if (M5.Lcd.width() == 128 && M5.Lcd.height() == 128)
      for (int i=0; i<7; ++i)
        pos[i][1] += 10;

    auto blank = M5.Lcd.height() < 135 ? TFT_DARKGREY : TFT_BLACK;

    M5.Lcd.fillRect(pos[1][0], pos[1][1], 16, 16, (dpad & 1) ? TFT_MAGENTA : blank);
    M5.Lcd.fillRect(pos[2][0], pos[2][1], 16, 16, (dpad & 2) ? TFT_MAGENTA : blank);
    M5.Lcd.fillRect(pos[3][0], pos[3][1], 16, 16, (dpad & 4) ? TFT_MAGENTA : blank);
    M5.Lcd.fillRect(pos[4][0], pos[4][1], 16, 16, (dpad & 8) ? TFT_MAGENTA : blank);
    M5.Lcd.fillRect(pos[5][0], pos[5][1], 16, 16, (buttons & 1) ? TFT_RED : blank);
    M5.Lcd.fillRect(pos[6][0], pos[6][1], 16, 16, (buttons & 2) ? TFT_RED : blank);
#endif    
}

void drawStaticScreen()
{
  drawImage();
  drawTitle();
  drawCredit();
  drawInstructions();
}

void eraseControls()
{
  M5.Lcd.clear();
  drawStaticScreen();  
}

void setup() {
    // 1. Core Hardware Init
    M5.begin();           // Necessary for Core 16M hardware stabilization

    if (M5.Lcd.width() == 135 // M5Stick-CPlus
      || M5.Lcd.width() == 80) 
    {
      M5.Lcd.setRotation(1); 
    }

    initDisplay(); 
    drawStaticScreen();
    
    WiFi.mode(WIFI_OFF); // Disable WiFi to reduce interference with Bluetooth/BLE scanning   

    // 2. Debug Output
    Serial.begin(115200);
    Serial.println("Starting BluePad32 Test...");
    Serial.printf("Free heap before BluePad32 setup: %d bytes\n", ESP.getFreeHeap());

    MyController.setup_Bluepad();
}

void loop() {
    M5.update();

    MyController.loop_Bluepad();
}
