/* 
  **  Digital PWR & SWR meter v2.1 by ON7IR - Adapted for 480x320 ILI9488
  **  Uses your calibrated touch_config.h
*/

#include <SPI.h>
#include <TFT_eSPI.h>
#include <MultiMap.h>
#include <XPT2046_Touchscreen.h>

#include "touch_config.h"   // tu touch calibrado

TFT_eSPI tft = TFT_eSPI();  // pantalla TFT

// Touchscreen
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(TOUCH_CS, TOUCH_IRQ);

// pantalla
const int SCR_WD = 480;
const int SCR_HT = 320;

// pines de entrada
const int fwdInPin = 34;
const int refInPin = 35;

// variables de medición
unsigned int sensorFWD = 0;
unsigned int sensorREF = 0;
float fwdInVoltage;
float refInVoltage;
float outputValueFWD;
float outputValueREF;
float cswr;
float swr;
const int swrTreshold = 5;

// barras
int ybar = 160;
int nextbar = 80;
int barHeight = 40;

// muestras
const int numSamples = 10;
int samples[numSamples] = {0};
int currentIndex = 0;
bool arrayFull = false;

// pantalla y modos
int mode = 0, lastMode = -1;
bool switchScreen;
String credits = "SWR&POWER METER v2.1 by ON7IR";
unsigned long currentMillis;
unsigned long previousMillis;
const long interval = 500;

// colores
#define TFT_GREY 0x5AEB
#define TFT_LGREY 0xBDF7
#define TFT_DGREY 0x2104
#define TFT_MYYELLOW 0xF6CC
#define TFT_MYORANGE 0xFDA0

const unsigned short labelColor = TFT_WHITE;
const unsigned short bgColor = TFT_BLACK;
const unsigned short FwdColor = TFT_GREEN;
const unsigned short RefColor = TFT_LGREY;
const unsigned short swr1Color = TFT_MYYELLOW;
const unsigned short swr2Color = TFT_MYORANGE;
const unsigned short swr3Color = TFT_RED;
const unsigned short barbgColor = TFT_DGREY;
const unsigned short on7irColor = TFT_DARKGREEN;

// Medidor analógico
#define CENTER_X SCR_WD/2       // 240
#define CENTER_Y SCR_HT-50      // 270
#define RADIUS 200
#define MIN_ANGLE -180
#define MAX_ANGLE 0
int previousValue = -1;

// ---- setup ----
void setup() {
  tft.init();
  tft.setRotation(3);
  tft.setTextColor(TFT_WHITE);
  
  touchscreenSPI.begin();       // SPI con touch
  touchscreen.begin(touchscreenSPI);

  drawSplashScreen();
}

// ---- splash ----
void drawSplashScreen() {
  tft.fillScreen(bgColor);
  tft.setTextColor(swr2Color, bgColor);
  tft.drawString("SWR and Power meter", 60, 40, 4);
  tft.drawString("version 2.1", 180, 70, 2);
  tft.setTextColor(TFT_MYYELLOW, bgColor);
  tft.drawString("by ON7IR", 200, 130, 2);
  delay(3000);
  tft.fillScreen(bgColor);
}

// ---- medidor analógico ----
void drawMeter() {
  for (int angle = MIN_ANGLE; angle <= MAX_ANGLE; angle += 9) {
    float rad = radians(angle);
    int x0 = CENTER_X + cos(rad) * (RADIUS - 10);
    int y0 = CENTER_Y + sin(rad) * (RADIUS - 10);
    int x1 = CENTER_X + cos(rad) * RADIUS;
    int y1 = CENTER_Y + sin(rad) * RADIUS;
    tft.drawLine(x0, y0, x1, y1, TFT_WHITE);
  }

  for (int angle = MIN_ANGLE; angle <= MAX_ANGLE; angle += 18) {
    float rad = radians(angle);
    int x = CENTER_X + cos(rad) * (RADIUS + 25);
    int y = CENTER_Y + sin(rad) * (RADIUS + 25);
    int value = map(angle, MIN_ANGLE, MAX_ANGLE, 0, 100);
    tft.setTextColor(TFT_GREEN, bgColor);
    tft.drawCentreString(String(value), x, y, 2);
  }

  for (int angle = MIN_ANGLE; angle <= MAX_ANGLE; angle++) {
    float rad = radians(angle);
    int x = CENTER_X + cos(rad) * (RADIUS - 10);
    int y = CENTER_Y + sin(rad) * (RADIUS - 10);
    tft.drawPixel(x, y, TFT_WHITE);
    x = CENTER_X + cos(rad) * (RADIUS - 8);
    y = CENTER_Y + sin(rad) * (RADIUS - 8);
    tft.drawPixel(x, y, TFT_WHITE);
    x = CENTER_X + cos(rad) * (RADIUS - 6);
    y = CENTER_Y + sin(rad) * (RADIUS - 6);
    tft.drawPixel(x, y, TFT_WHITE);
  }
}

void drawNeedle(int value) {
  int angle = map(value, 0, 100, MIN_ANGLE, MAX_ANGLE);
  float rad = radians(angle);

  int xTip = CENTER_X + cos(rad) * (RADIUS - 20);
  int yTip = CENTER_Y + sin(rad) * (RADIUS - 20);
  int baseWidth = 10;
  int xBase1 = CENTER_X + cos(rad + radians(90)) * (baseWidth / 2);
  int yBase1 = CENTER_Y + sin(rad + radians(90)) * (baseWidth / 2);
  int xBase2 = CENTER_X + cos(rad - radians(90)) * (baseWidth / 2);
  int yBase2 = CENTER_Y + sin(rad - radians(90)) * (baseWidth / 2);

  if (previousValue != -1) {
    int prevAngle = map(previousValue, 0, 100, MIN_ANGLE, MAX_ANGLE);
    float prevRad = radians(prevAngle);
    int pxTip = CENTER_X + cos(prevRad) * (RADIUS - 20);
    int pyTip = CENTER_Y + sin(prevRad) * (RADIUS - 20);
    int pxBase1 = CENTER_X + cos(prevRad + radians(90)) * (baseWidth / 2);
    int pyBase1 = CENTER_Y + sin(prevRad + radians(90)) * (baseWidth / 2);
    int pxBase2 = CENTER_X + cos(prevRad - radians(90)) * (baseWidth / 2);
    int pyBase2 = CENTER_Y + sin(prevRad - radians(90)) * (baseWidth / 2);
    tft.fillTriangle(pxBase1, pyBase1, pxBase2, pyBase2, pxTip, pyTip, bgColor);
  }

  tft.fillTriangle(xBase1, yBase1, xBase2, yBase2, xTip, yTip, TFT_RED);
  tft.fillCircle(CENTER_X, CENTER_Y, 10, TFT_BLACK);
  previousValue = value;
}

// ---- pantalla principal ----
void drawRectangle(int x, int y, int w, int h, char* label, unsigned short col = labelColor) {
  tft.drawRect(x, y + 7, w, h - 7, col);
  int wl = atoi(label);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextSize(1);
  tft.setCursor(x - 15 + (w - wl) / 2, y + 5);
  tft.print(label);
}

void drawMain() {
  tft.setTextSize(1);
  drawRectangle(0, 0, 140, 120, " FWD ", FwdColor);
  drawRectangle(170, 0, 140, 120, " REF ", RefColor);
  drawRectangle(340, 0, 140, 120, " SWR ", swr1Color);

  tft.setTextColor(TFT_GREEN, bgColor);
  tft.setCursor(50, 110);
  tft.print("WATT");

  tft.setTextColor(labelColor, bgColor);
  tft.setCursor(0, ybar - 20); tft.print("forward");
  tft.setCursor(0, ybar + nextbar - 20); tft.print("reflected");
  tft.setCursor(0, ybar + nextbar*2 - 20); tft.print("SWR");

  tft.setTextColor(on7irColor, bgColor);
  tft.setCursor(190, 90);
  tft.print(credits);
}

// ---- barras ----
void drawBarFWD(int level) {
  level = map(level, 0, 100, 0, 100);
  int i = level * SCR_WD / 100;
  tft.fillRect(0, ybar, i, barHeight, FwdColor);
  tft.fillRect(i, ybar, SCR_WD - i, barHeight, barbgColor);
}

void drawBarREF(int level) {
  level = map(level, 0, 50, 0, 100);
  int i = level * SCR_WD / 100;
  tft.fillRect(0, ybar + nextbar, i, barHeight, RefColor);
  tft.fillRect(i, ybar + nextbar, SCR_WD - i, barHeight, barbgColor);
}

void drawBarSWR(float level) {
  level = level * 10;
  level = map(level, 0, 50, 0, 100);
  if (swr < 2) tft.fillRect(0, ybar + nextbar*2, level * SCR_WD/100, barHeight, swr1Color);
  else if (swr < 3) tft.fillRect(0, ybar + nextbar*2, level * SCR_WD/100, barHeight, swr2Color);
  else tft.fillRect(0, ybar + nextbar*2, level * SCR_WD/100, barHeight, swr3Color);
  tft.fillRect(level*SCR_WD/100, ybar+nextbar*2, SCR_WD - level*SCR_WD/100, barHeight, barbgColor);
}

// ---- cálculo ----
void getSWR() {
  sensorFWD = analogRead(fwdInPin); sensorFWD = constrain(sensorFWD, 70, 4095);
  sensorREF = analogRead(refInPin); sensorREF = constrain(sensorREF, 0, 4095);
  fwd2watt(); ref2watt(); calcSWR();
}

void fwd2watt() {
  fwdInVoltage = sensorFWD * (3.3 / 4095.0);
  float outFWD[] = {0.9,1.69,2.56,3.24,5.48,10.24,15.1,21.16,25,29.7,
                     34.81,40.32,44.89,49.7,54.76,58.52,64.28,68.89,73.96,
                     80,85,90,95,100};
  float inFWD[] = {188,283,372,438,450,700,920,1170,1320,1450,
                    1630,1780,1900,1980,2100,2250,2300,2450,2550,2600,
                    2800,2900,2980,3083};
  outputValueFWD = multiMap(fwdInVoltage*1000, inFWD, outFWD, 24);

  samples[currentIndex] = outputValueFWD;
  currentIndex++; if (currentIndex >= numSamples) { currentIndex = 0; arrayFull = true; }
  float average = calculateAverage(samples, arrayFull ? numSamples : currentIndex);
  outputValueFWD = average;
}

void ref2watt() {
  refInVoltage = sensorREF*(3.3/4095.0);
  float outREF[] = {0,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1,2,3,4,5,6,7,8,9,10,20,30,40,50,60,70,80,90,100};
  float inREF[] = {0,109,138,179,210,235,247,278,291,323,454,579,671,779,847,927,995,1077,1152,1671,
                   2047,2408,2727,2978,3180,3297,3413,3494};
  outputValueREF = multiMap(refInVoltage*1000, inREF, outREF, 28);
  outputValueREF = constrain(outputValueREF, 0, outputValueFWD);
}

float calculateAverage(int* data, int size) {
  long sum = 0;
  for(int i = 0; i<size; i++) sum += data[i];
  return sum / (float)size;
}

void calcSWR() {
  if(outputValueREF == 0) cswr = 1;
  else cswr = (outputValueFWD + outputValueREF) / (outputValueFWD - outputValueREF);
  swr = cswr;
  if (swr < 1) swr = 1;
  if (swr > 10) swr = 10;
}

// ---- loop ----
void loop() {
  getSWR();
  drawMain();
  drawBarFWD(outputValueFWD);
  drawBarREF(outputValueREF);
  drawBarSWR(swr);

  drawNeedle(map(outputValueFWD, 0, 100, 0, 100));

  delay(100);
}
