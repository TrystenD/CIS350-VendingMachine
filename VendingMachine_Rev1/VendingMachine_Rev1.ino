/***********************************************************
   File: VendingMachine_Rev1.ino

   Authors: Trysten Dembeck, JP, Joel Meyers

   Description: This program operates the vending machine
                controller for the CIS350 project.

   Device: Arduino Mega 2560

   Revision: 1
 ***********************************************************/

/** Library Includes */
#include <Servo.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"

/** Function Prototypes */
void handleItemMenu();
void waitForUnpress(Adafruit_GFX_Button btn);
void drawItemMenu(void);
void drawPasswordMenu(void);
void handleItemMenu(void);
void handlePasswordMenu(void);

// The four touchscreen analog pins to track touch location
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 8   // can be a digital pin

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 10
#define TS_MINY 10
#define TS_MAXX 900
#define TS_MAXY 940

#define MINPRESSURE 5
#define MAXPRESSURE 1000

// The display uses hardware SPI, plus #9 & #10
#define TFT_RST -1  // dont use a reset pin, tie to arduino RST if you like
#define TFT_DC 48
#define TFT_CS 49

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 290);

/** State Machine State Names */
#define SM_SLEEP        0
#define SM_IDLE         1
#define SM_ACCEPT_COINS 2
#define SM_DISPENSE     3
#define SM_MAINTENENCE  4

#define BOXSIZE 40

// Redefine original colors, add additional colors to match those available with the ILI9341 library
#define HX8357_BLACK       0x0000  ///<   0,   0,   0
#define HX8357_NAVY        0x000F  ///<   0,   0, 123
#define HX8357_DARKGREEN   0x03E0  ///<   0, 125,   0
#define HX8357_DARKCYAN    0x03EF  ///<   0, 125, 123
#define HX8357_MAROON      0x7800  ///< 123,   0,   0
#define HX8357_PURPLE      0x780F  ///< 123,   0, 123
#define HX8357_OLIVE       0x7BE0  ///< 123, 125,   0
#define HX8357_LIGHTGREY   0xC618  ///< 198, 195, 198
#define HX8357_DARKGREY    0x7BEF  ///< 123, 125, 123
#define HX8357_BLUE        0x001F  ///<   0,   0, 255
#define HX8357_GREEN       0x07E0  ///<   0, 255,   0
#define HX8357_CYAN        0x07FF  ///<   0, 255, 255
#define HX8357_RED         0xF800  ///< 255,   0,   0
#define HX8357_MAGENTA     0xF81F  ///< 255,   0, 255
#define HX8357_YELLOW      0xFFE0  ///< 255, 255,   0
#define HX8357_WHITE       0xFFFF  ///< 255, 255, 255
#define HX8357_ORANGE      0xFD20  ///< 255, 165,   0
#define HX8357_GREENYELLOW 0xAFE5  ///< 173, 255,  41
#define HX8357_PINK        0xFC18  ///< 255, 130, 198

// Item Menu Buttons
#define ITEM_MENU_BTNS_CNT 4
Adafruit_GFX_Button ItemMenuBtns[ITEM_MENU_BTNS_CNT];
char ItemMenuBtnLabels[ITEM_MENU_BTNS_CNT][9] = {"Item 1", "Item 2", "Item 3", "Settings"};
uint16_t ItemMenuBtnColors[ITEM_MENU_BTNS_CNT] = {HX8357_DARKGREY, HX8357_DARKGREY, HX8357_DARKGREY, HX8357_DARKGREY};


// Number pad buttons
#define NUMPAD_BUTTON_X 50
#define NUMPAD_BUTTON_Y 50
#define NUMPAD_BUTTON_W 80
#define NUMPAD_BUTTON_H 60
#define NUMPAD_BUTTON_SPACING_X 15
#define NUMPAD_BUTTON_SPACING_Y 15
#define NUMPAD_BUTTON_TEXTSIZE 2

#define NUMPAD_BTNS_CNT 15
Adafruit_GFX_Button NumpadBtns[NUMPAD_BTNS_CNT];
char NumpadBtnLabels[NUMPAD_BTNS_CNT][10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#", "Del", "Enter", "Cncl"};
uint16_t NumpadBtnColors[NUMPAD_BTNS_CNT] = {HX8357_DARKCYAN, HX8357_DARKCYAN, HX8357_DARKCYAN,
                                HX8357_DARKCYAN, HX8357_DARKCYAN, HX8357_DARKCYAN,
                                HX8357_DARKCYAN, HX8357_DARKCYAN, HX8357_DARKCYAN,
                                HX8357_ORANGE, HX8357_DARKCYAN, HX8357_ORANGE,
                                HX8357_DARKGREY, HX8357_GREEN, HX8357_RED
                               };

Adafruit_GFX_Button EnterBtn;
char EnterBtnLabel[6] = "Enter";
uint16_t EnterBtnColor = HX8357_GREEN;


// Text box for password
#define TEXT_BOX_X 310
#define TEXT_BOX_Y 90
#define TEXT_BOX_W 150
#define TEXT_BOX_H 50
#define TEXT_SIZE 3
#define TEXT_COLOR HX8357_MAGENTA
#define TEXT_LEN 4
char textfield[TEXT_LEN + 1] = "";
uint8_t textfieldChar = 0;

unsigned int state = SM_IDLE; // State machine control variable

void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.fillScreen(HX8357_BLACK);

  tft.begin();
  tft.setRotation(3);
  tft.setTextWrap(false);

  //initializeButtons(Menu1Buttons, Menu1Colors, Menu1Labels, MENU1_BTN_CNT);

  //tft.drawRect(0, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
}

void loop()
{
  switch (state)
  {
    case SM_SLEEP:
    {
      break;
    }
    case SM_IDLE:
    {
      drawItemMenu();
      while(state == SM_IDLE) {
        handleItemMenu(); 
      }
      break;
    }
    case SM_ACCEPT_COINS:
    {
      break;
    }
    case SM_DISPENSE:
    {
      break;
    }
    case SM_MAINTENENCE:
    {
      drawPasswordMenu();
      while(state == SM_MAINTENENCE) {
        handlePasswordMenu();
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

void handlePasswordMenu(void)
{
  int x;
  int y;
  TSPoint p;

  p = ts.getPoint();
  x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X
//  Serial.print("("); Serial.print(x); Serial.print(", ");
//  Serial.print(y); Serial.print(", ");
//  Serial.print(p.z); Serial.println(") ");


  if (p.z >= MINPRESSURE && p.z <= MAXPRESSURE)
  {
    for (uint8_t b = 0; b < 15; b++) {
      if (NumpadBtns[b].contains(x, y)) {
        Serial.print("Pressing: "); Serial.println(b);
        NumpadBtns[b].press(true);  // tell the button it is pressed

        if (NumpadBtns[b].justPressed())
        {
          NumpadBtns[b].drawButton(true); // draw inverted button colors

          waitForUnpress(NumpadBtns[b]);

          Serial.print("Released: "); Serial.println(b);
          NumpadBtns[b].press(false);
          NumpadBtns[b].drawButton(false);

          if(b <= 11) {
            if (textfieldChar < TEXT_LEN) {
              textfield[textfieldChar] = NumpadBtnLabels[b][0];
              textfieldChar++;
              textfield[textfieldChar] = 0; // zero terminate
            }
          }
          else if(b == 12) {
            textfield[textfieldChar] = 0;
            if (textfield > 0) {
              textfieldChar--;
              textfield[textfieldChar] = ' ';
            }
          }
          else if(b == 13) {
            // Enter
          }
          else if(b == 14) {
            // Cncl
            state = SM_IDLE;
          }
          
          //Serial.println(textfield);
          tft.setCursor(TEXT_BOX_X + 2, TEXT_BOX_Y + 10);
          tft.setTextColor(TEXT_COLOR, HX8357_BLACK);
          tft.setTextSize(TEXT_SIZE);
          tft.print(textfield);

          delay(100); // UI debouncing
        }
      }
    }
  }
}

void handleItemMenu()
{
  int x;
  int y;
  TSPoint p;

  p = ts.getPoint();
  x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X

  // Valid press on screen
  if (p.z >= MINPRESSURE && p.z <= MAXPRESSURE) {
    if (ItemMenuBtns[0].contains(x, y))
    {
      ItemMenuBtns[0].press(true);

      if (ItemMenuBtns[0].justPressed())
      {
        ItemMenuBtns[0].drawButton(true); // draw inverted button colors

        waitForUnpress(ItemMenuBtns[0]);

        ItemMenuBtns[0].press(false);
        ItemMenuBtns[0].drawButton(false);
      }
    }
    else if (ItemMenuBtns[1].contains(x, y))
    {
      ItemMenuBtns[1].press(true);

      if (ItemMenuBtns[1].justPressed())
      {
        ItemMenuBtns[1].drawButton(true); // draw inverted button colors

        waitForUnpress(ItemMenuBtns[1]);

        ItemMenuBtns[1].press(false);
        ItemMenuBtns[1].drawButton(false);
      }
    }
    else if (ItemMenuBtns[2].contains(x, y))
    {
      ItemMenuBtns[2].press(true);

      if (ItemMenuBtns[2].justPressed())
      {
        ItemMenuBtns[2].drawButton(true); // draw inverted button colors

        waitForUnpress(ItemMenuBtns[2]);

        ItemMenuBtns[2].press(false);
        ItemMenuBtns[2].drawButton(false);
      }
    }
    else if (ItemMenuBtns[3].contains(x, y))
    {
      ItemMenuBtns[3].press(true);

      if (ItemMenuBtns[3].justPressed())
      {
        ItemMenuBtns[3].drawButton(true); // draw inverted button colors

        waitForUnpress(ItemMenuBtns[3]);

        ItemMenuBtns[3].press(false);
        ItemMenuBtns[3].drawButton(false);
        state = SM_MAINTENENCE;
      }
    }
  }
}


void drawPasswordMenu(void) {

  tft.fillScreen(HX8357_BLACK);
  
  for (uint8_t row = 0; row < 4; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      NumpadBtns[col + row * 3].initButton(&tft, NUMPAD_BUTTON_X + col * (NUMPAD_BUTTON_W + NUMPAD_BUTTON_SPACING_X),
                                           NUMPAD_BUTTON_Y + row * (NUMPAD_BUTTON_H + NUMPAD_BUTTON_SPACING_Y),
                                           NUMPAD_BUTTON_W, NUMPAD_BUTTON_H, HX8357_WHITE, NumpadBtnColors[col + row * 3], HX8357_WHITE,
                                           NumpadBtnLabels[col + row * 3], NUMPAD_BUTTON_TEXTSIZE);
      NumpadBtns[col + row * 3].drawButton();
    }
  }

  // Delete button
  NumpadBtns[12].initButton(&tft, 420, 50, NUMPAD_BUTTON_W, NUMPAD_BUTTON_H, HX8357_WHITE, NumpadBtnColors[12], HX8357_WHITE, NumpadBtnLabels[12], NUMPAD_BUTTON_TEXTSIZE);
  NumpadBtns[12].drawButton();

  // Enter button
  NumpadBtns[13].initButton(&tft, 385, 170, TEXT_BOX_W, TEXT_BOX_H, HX8357_WHITE, NumpadBtnColors[13], HX8357_WHITE, NumpadBtnLabels[13], NUMPAD_BUTTON_TEXTSIZE);
  NumpadBtns[13].drawButton();

  // Cancel button
  NumpadBtns[14].initButton(&tft, 430, 290, NUMPAD_BUTTON_W, NUMPAD_BUTTON_H, HX8357_WHITE, NumpadBtnColors[14], HX8357_WHITE, NumpadBtnLabels[14], NUMPAD_BUTTON_TEXTSIZE);
  NumpadBtns[14].drawButton();

  // Text box to display entered password
  tft.drawRect(TEXT_BOX_X, TEXT_BOX_Y, TEXT_BOX_W, TEXT_BOX_H, HX8357_WHITE);
}


void drawItemMenu(void) {
  tft.fillScreen(HX8357_BLACK);
  
  // Item 1 Button
  ItemMenuBtns[0].initButton(&tft, 75, 75, 125, 125, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[0], 3);
  ItemMenuBtns[0].drawButton();

  // Item 2 Button
  ItemMenuBtns[1].initButton(&tft, 240, 75, 125, 125, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[1], 3);
  ItemMenuBtns[1].drawButton();

  // Item 3 Button
  ItemMenuBtns[2].initButton(&tft, 405, 75, 125, 125, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[2], 3);
  ItemMenuBtns[2].drawButton();

  // Settings button
  ItemMenuBtns[3].initButton(&tft, 405, 300, 150, 40, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[3], 2);
  ItemMenuBtns[3].drawButton();

  tft.setCursor(45, 150);
  tft.print("$0.25");

  tft.setCursor(205, 150);
  tft.print("$0.25");

  tft.setCursor(375, 150);
  tft.print("$0.25");
}


void waitForUnpress(Adafruit_GFX_Button btn) {
  TSPoint p = ts.getPoint();
  int x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  int y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X

  // Wait while the x and y coordinates are within button outline pixels (+- 20)
  while (btn.contains(x, y) || btn.contains(x + 20, y + 20) || btn.contains(x - 20, y - 20)) {
    p = ts.getPoint();
    x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
    y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X
  }
}
