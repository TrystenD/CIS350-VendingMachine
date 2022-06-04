/***********************************************************
 * File: VendingMachine_Rev1.ino
 * 
 * Authors: Trysten Dembeck, JP, Joel Meyers
 * 
 * Description: This program operates the vending machine
 *              controller for the CIS350 project.
 *              
 * Device: Arduino Mega 2560
 * 
 * Revision: 1
 ***********************************************************/
 
/** Library Includes */
#include <Servo.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"

// The four touchscreen analog pins to track touch location
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 8   // can be a digital pin

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 140
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 940

#define MINPRESSURE 10
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

#define ITEM_MENU_BTNS_CNT 3
Adafruit_GFX_Button ItemMenuBtns[ITEM_MENU_BTNS_CNT];
char ItemMenuBtnLabels[ITEM_MENU_BTNS_CNT][7] = {"Item 1", "Item 2", "Item 3"};
uint16_t ItemMenuBtnColors[ITEM_MENU_BTNS_CNT] = {HX8357_DARKGREY, HX8357_DARKGREY, HX8357_DARKGREY};

// UI Buttondetails
#define BUTTON_X 80
#define BUTTON_Y 80
#define BUTTON_W 150
#define BUTTON_H 80
#define BUTTON_TEXTSIZE 2
#define DISPLAY_XOFFSET 80
#define DISPLAY_TEXTOFFSET 90
#define DISPLAY_YOFFSET 0

int textSize = 2;
int textColorIndex = 0;
uint16_t textColor[7] = {
 HX8357_WHITE,
 HX8357_RED,
 HX8357_GREEN,
 HX8357_BLUE,
 HX8357_CYAN,
 HX8357_MAGENTA,
 HX8357_YELLOW
};

enum ButtonName {
  BTN_UP,
  BTN_SELECT,
  BTN_DOWN
};

unsigned int state = SM_IDLE;

bool  initializeButtons(Adafruit_GFX_Button menuButtons[], uint16_t menuColors[], char menuLabels[][3], int menuButtonCount) {
     tft.fillScreen(HX8357_BLACK);

    for (uint8_t row=0; row<menuButtonCount; row++) 
    {
        menuButtons[row].initButton(&tft, 
            BUTTON_X, 
            BUTTON_Y + row *(BUTTON_H),    
            BUTTON_W, 
            BUTTON_H, 
            HX8357_BLACK, 
            menuColors[row], 
            HX8357_WHITE,
            menuLabels[row], BUTTON_TEXTSIZE);    
        menuButtons[row].drawButton();
    }
    return true;
}

void setup() 
{
  Serial.begin(115200);
  tft.begin();
  tft.fillScreen(HX8357_BLACK);
  
  tft.begin();
  tft.setRotation(3);
  tft.setTextWrap(false);
                        
  //initializeButtons(Menu1Buttons, Menu1Colors, Menu1Labels, MENU1_BTN_CNT);

  ItemMenuBtns[0].initButton(&tft, 75, 75, 150, 150, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[0], 3);
  ItemMenuBtns[0].drawButton();
  //                                x   y    w    h
  ItemMenuBtns[1].initButton(&tft, 225, 75, 150, 150, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[1], 3);
  ItemMenuBtns[1].drawButton();

  ItemMenuBtns[2].initButton(&tft, 375, 75, 150, 150, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[2], 3);
  ItemMenuBtns[2].drawButton();

  //tft.drawRect(0, 0, BOXSIZE, BOXSIZE, HX8357_WHITE);
}

void loop() 
{
  // Retrieve a point  
  TSPoint p = ts.getPoint();
  int x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());
  int y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());

  if(ItemMenuBtns[0].contains(x, y))
  {
    ItemMenuBtns[0].press(true);
    Serial.println("PRESSED 1");
  }
  else if(ItemMenuBtns[1].contains(x, y))
  {
    ItemMenuBtns[1].press(true);
    Serial.println("PRESSED 2");
  }
  else if(ItemMenuBtns[2].contains(p.x, y))
  {
    ItemMenuBtns[2].press(true);
    Serial.println("PRESSED 3");
  }

  
 
  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
     return;
  }

  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print("\tPressure = "); Serial.println(p.z);  
   
  switch(state)
  {
    case SM_SLEEP:
    {
      break;
    }
    case SM_IDLE:
    {
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
      break;
    }
    default:
    {
      break;
    }
  }
}
