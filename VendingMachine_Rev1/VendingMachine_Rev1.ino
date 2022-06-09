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

/****************************************** Library Includes */
#include <Servo.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"
#include "string.h"

/****************************************** Function Prototypes */
void handleItemMenu();
void waitForUnpress(Adafruit_GFX_Button btn);
void drawItemMenu(void);
void drawPasswordMenu(void);
void handleItemMenu(void);
void handlePasswordMenu(void);
void clearPasswordpswdTextfield(void);
void passwordMessage(const char *msg);
void initSystem(void);
void setItemPrice(uint8_t item, uint8_t price);
void setItemCount(uint8_t item, uint8_t count);

/****************************************** Tocuhscreen calibration data */
#define TS_MINX     10
#define TS_MINY     10
#define TS_MAXX     900
#define TS_MAXY     940
#define X_PLATE_RES 290  // Measured resistance across X+ and X- plate
#define MINPRESSURE 5    // Minumum valid pressure
#define MAXPRESSURE 1000 // Maximum valid pressure

/****************************************** TFT LCD and Touchscreen Pin Configuration */
// The TFT LCD on Hardware SPI (On Mega 2560: MISO-D50, MOSI-D51, CLK-D52), and DC-D48 and CS-D49 (configurable)
#define TFT_RST -1  // dont use a reset pin, can tie to arduino RST
#define TFT_DC 48
#define TFT_CS 49

// The four touchscreen analog pins to track touch location
#define YP A2  // Y+ must be an analog pin, use "An" notation!
#define XM A3  // X- must be an analog pin, use "An" notation!
#define YM 7   // Y- Digital pin
#define XP 8   // X+ Digital pin

/****************************************** TFT LCD and Touchscreen Instantiation */
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST); // TFT LCD screen object
TouchScreen ts = TouchScreen(XP, YP, XM, YM, X_PLATE_RES);      // Resistive touchscreen object

/****************************************** State Machine State Names */
#define SM_SLEEP        0
#define SM_IDLE         1
#define SM_ACCEPT_COINS 2
#define SM_DISPENSE     3
#define SM_PASSWORD     4
#define SM_MAINTENENCE  5

/****************************************** Redefine original colors LCD Colors */
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

/****************************************** Item Menu Configuration */
#define ITEM_MENU_BTNS_CNT 4
Adafruit_GFX_Button ItemMenuBtns[ITEM_MENU_BTNS_CNT];
char ItemMenuBtnLabels[ITEM_MENU_BTNS_CNT][9] = {"Item 1", "Item 2", "Item 3", "Settings"};
uint16_t ItemMenuBtnColors[ITEM_MENU_BTNS_CNT] = {HX8357_DARKCYAN, HX8357_DARKCYAN, HX8357_DARKCYAN, HX8357_DARKGREY};

/****************************************** Password Menu Configuration */
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
                                             HX8357_DARKGREY, HX8357_DARKGREEN, HX8357_MAROON
                                            };

// Text box for correct/incorrect password message
#define PASSWORD_MSG_X 335
#define PASSWORD_MSG_Y 215

// Text box for password
#define TEXT_BOX_X 310
#define TEXT_BOX_Y 90
#define TEXT_BOX_W 150
#define TEXT_BOX_H 50
#define TEXT_SIZE 3
#define TEXT_COLOR HX8357_MAGENTA
#define TEXT_LEN 4
char pswdTextfield[TEXT_LEN + 1] = "";                // String in pswdTextfield
uint8_t pswdTextfieldChar = 0;                        // Current char to write in pswdTextfield


/****************************************** Maintenence Menu Configuration */
// Increase/decrease Buttons
#define EDIT_BUTTON_X 50
#define EDIT_BUTTON_Y 50
#define EDIT_BUTTON_W 60
#define EDIT_BUTTON_H 60
#define EDIT_BUTTON_TEXTSIZE 2
#define EDIT_BTNS_CNT 13

Adafruit_GFX_Button EditBtns[EDIT_BTNS_CNT];
char EditBtnLabels[EDIT_BTNS_CNT][6] = {"+", "-", "+", "-", "+", "-", "+", "-", "+", "-", "+", "-", "Done"};
uint16_t EditBtnColors[EDIT_BTNS_CNT] = {HX8357_DARKGREEN, HX8357_MAROON, HX8357_DARKGREEN,
                                         HX8357_MAROON, HX8357_DARKGREEN, HX8357_MAROON,
                                         HX8357_DARKGREEN, HX8357_MAROON, HX8357_DARKGREEN,
                                         HX8357_MAROON, HX8357_DARKGREEN, HX8357_MAROON,
                                         HX8357_DARKGREY
                                        };


/****************************************** System Global Variables and Structures */
#define MAX_ITEM_COUNT 3
const char OWNER_PASSWORD[TEXT_LEN + 1] = "3251"; // Owner password (4 chars)

uint8_t state = SM_IDLE; // State machine control variable
uint8_t coinBalance = 0; // User's current coin balance

const char validCoinAmounts[4][6] = {
  "$0.25",
  "$0.50",
  "$0.75",
  "$1.00"
};

struct Item {
  uint8_t price;    // Price in number of quarters (1-4)
  uint8_t count;    // Number of items left in machine
  char priceStr[6]; // Price as LCD displayable string (ex. "$0.25")
};

Item item1;
Item item2;
Item item3;

/****************************************** Program Entry */
void setup()
{
  Serial.begin(115200);
  tft.begin();
  tft.fillScreen(HX8357_BLACK);

  tft.begin();
  tft.setRotation(3);
  tft.setTextWrap(false);

  initSystem(); // Init system variables
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
        while (state == SM_IDLE) {
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
    case SM_PASSWORD:
      {
        drawPasswordMenu();
        while (state == SM_PASSWORD) {
          handlePasswordMenu();
        }
        break;
      }
    case SM_MAINTENENCE:
      {
        drawSetItemMenu();
        while (state == SM_MAINTENENCE) {
          handleSetItemMenu();
        }
        break;
      }
    default:
      {
        break;
      }
  }
}

/****************************************** Function Definitions */

/*
 * @brief  This function handles the inputs and UI outputs of the
 *         password menu.
 *        
 * @param  None
 * @return void
 */
void handlePasswordMenu(void) {
  int x;
  int y;
  TSPoint p;

  p = ts.getPoint();
  x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X

  if (p.z >= MINPRESSURE && p.z <= MAXPRESSURE)
  {
    for (uint8_t b = 0; b < 15; b++) {
      if (NumpadBtns[b].contains(x, y)) {
        NumpadBtns[b].press(true);          // tell the button it is pressed

        if (NumpadBtns[b].justPressed()) {
          NumpadBtns[b].drawButton(true);   // draw inverted button colors
          waitForUnpress(NumpadBtns[b]);
          NumpadBtns[b].press(false);
          NumpadBtns[b].drawButton(false);

          // Any button here is on the 12-key number pad
          if (b <= 11) {
            if (pswdTextfieldChar < TEXT_LEN) {
              pswdTextfield[pswdTextfieldChar] = NumpadBtnLabels[b][0];
              pswdTextfieldChar++;
              pswdTextfield[pswdTextfieldChar] = 0; // zero terminate
            }
          }
          // The delete key
          else if (b == 12) {
            pswdTextfield[pswdTextfieldChar] = 0;
            if (pswdTextfield > 0) {
              pswdTextfieldChar--;                    // Set current char placement back 1
              pswdTextfield[pswdTextfieldChar] = ' '; // Replace deleted char with ' ' whitespace
            }
          }
          // Enter button
          else if (b == 13) {
            // Validate entered password with string compare
            if (strcmp(pswdTextfield, OWNER_PASSWORD) == 0) {
              passwordMessage("Correct");
              clearPasswordTextfield();               // Clear entered password
              state = SM_MAINTENENCE;                 // Move to maintenence state if pswd is correct
            }
            else {
              passwordMessage("Incorrect");
            }
          }
          // Cncl Button (cancel maintenence)
          else if (b == 14) {
            clearPasswordTextfield();
            state = SM_IDLE;
          }

          // Update text field with entered password
          tft.setCursor(TEXT_BOX_X + 40, TEXT_BOX_Y + 15);
          tft.setTextColor(TEXT_COLOR, HX8357_BLACK);
          tft.setTextSize(TEXT_SIZE);
          tft.print(pswdTextfield);
          
          delay(100); // UI debouncing
        }
      }
    }
  }
}


/*
 * @brief  This function initalizes system variables such
 *         as the item counts and prices.
 *        
 * @param  None
 * @return void
 */
void initSystem(void) {
  setItemPrice(1, 1);
  setItemPrice(2, 2);
  setItemPrice(3, 4);

  setItemCount(1, 3);
  setItemCount(2, 3);
  setItemCount(3, 4);
}


/*
 * @brief  Setter for the item prices. Validates the selected item
 *         and desired price before setting it.
 *        
 * @param  uint8_t item: The item (1-3) to set the price of
 * @param  uint8_t coins: The number of coins required to purchase item (1-4)
 * 
 * @return void
 */
void setItemPrice(uint8_t item, uint8_t coins) {
  if (((coins >= 1) && (coins <= 4)) && ((item >= 1) && item <= 3)) {
    switch (item) {
      case 1:
          item1.price = coins;                                 // Set # of coins required
          strcpy(item1.priceStr, validCoinAmounts[coins - 1]); // Set corresponding $ value
          break;

      case 2:
          item2.price = coins;
          strcpy(item2.priceStr, validCoinAmounts[coins - 1]);

          break;
      case 3:
          item3.price = coins;
          strcpy(item3.priceStr, validCoinAmounts[coins - 1]);

          break;
          
      default:
          Serial.println("Invalid Item. Must be between 1 and 3 inclusive.");
          break;
    }
  }
  else {
    Serial.println("Invalid Price. Must be between 1 and 4 inclusive.");
  }
}


/*
 * @brief  Setter for the item stock. Validates the selected item
 *         and desired stock before setting it.
 *        
 * @param  uint8_t item: The item (1-3) to set the stock of
 * @param  uint8_t count: The number of items to set as the machine's stock (1-MAX_MACRO)
 * 
 * @return void
 */
void setItemCount(uint8_t item, uint8_t count) {
  if (((count >= 0) && (count <= MAX_ITEM_COUNT)) && ((item >= 1) && item <= 3)) {
    switch (item) {
      case 1:
          item1.count = count;
          break;
          
      case 2:
          item2.count = count;
          break;
          
      case 3:
          item3.count = count;
          break;

      default:
          Serial.println("Invalid Item. Must be between 1 and 3 inclusive.");
          break;
        
    }
  }
  else {
    Serial.print("Invalid Count. Must be between 0 and "); Serial.print(MAX_ITEM_COUNT); Serial.println(" inclusive.");
  }
}


/*
 * @brief  Clears the password textfield so entered
 *         value is not saved by program.
 *        
 * @param  None
 * @return void
 */
void clearPasswordTextfield(void) {
  for (uint8_t i = 0; i < 4; i++) {
    pswdTextfield[i] = ' ';         // Replace each character with a space
  }
  pswdTextfieldChar = 0;            // Reset current password char to update
}

/*
 * @brief  Helper to display a message in the password menu.
 *         Used to display "Correct" or "Incorrect" when the
 *         password is entered by the user (under enter btn).
 *        
 * @param  const char *msg: Constant string message to display.
 * @return void
 */
void passwordMessage(const char *msg) {
  tft.fillRect(PASSWORD_MSG_X, PASSWORD_MSG_Y, 200, 16, HX8357_BLACK);
  tft.setCursor(PASSWORD_MSG_X, PASSWORD_MSG_Y);
  tft.setTextColor(HX8357_WHITE);
  tft.setTextSize(2);
  tft.print(msg);
}

/*
 * @brief  Handles the inputs and UI outputs within the select-item menu.
 *         This menu allows the user to select which item to purchase or 
 *         to enter maintenence mode.
 *        
 * @param  void
 * @return void
 */
void handleItemMenu(void) {
  int x;
  int y;
  TSPoint p;

  // Retreive point from touchscren
  p = ts.getPoint();
  x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X

  // Valid press on screen if within valid pressure range
  if (p.z >= MINPRESSURE && p.z <= MAXPRESSURE) {

    for (uint8_t b = 0; b < 5; b++) {         // Loop through each button to see if it was pressed
      if (ItemMenuBtns[b].contains(x, y)) {
        ItemMenuBtns[b].press(true);          // Tell a button that it was pressed

        if (ItemMenuBtns[b].justPressed()) {
          ItemMenuBtns[b].drawButton(true);   // Draw inverted button colors
          waitForUnpress(ItemMenuBtns[b]);    // Wait for user to let go of the button
          ItemMenuBtns[b].press(false);       // Tell button it is no longer pressed
          ItemMenuBtns[b].drawButton(false);  // Draw button normally

          // Item 1 selected
          if (b == 0) {
            
          }
          // Item 2 selected
          if (b == 1) {

          }
          // Item 3 selected
          if (b == 2) {
            
          }
          // Settings button pressed
          if (b == 3) {
            state = SM_PASSWORD;
          }
        }
      }
    }
  }
}


/*
 * @brief  Handles the inputs and UI outputs within the set-item menu.
 *         This menu allows the user to increase or decrease the stock
 *         of the items in the machine and their prices.
 *        
 * @param  void
 * @return void
 */
void handleSetItemMenu(void) {
  int x;
  int y;
  TSPoint p;

  // Retrieve point from touchscreen
  p = ts.getPoint();
  x = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());  // X flipped to Y
  y = map(p.x, TS_MINX, TS_MAXX, 0, tft.height()); // Y flipped to X

  if (p.z >= MINPRESSURE && p.z <= MAXPRESSURE)
  {
    for (uint8_t b = 0; b < 13; b++) {
      if (EditBtns[b].contains(x, y)) {
        EditBtns[b].press(true);          // Tell the button it is pressed

        if (EditBtns[b].justPressed()) {
          EditBtns[b].drawButton(true);   // Draw inverted button colors
          waitForUnpress(EditBtns[b]);    // Wait for user to let go of the button
          EditBtns[b].press(false);       // Tell button it is no longer pressed
          EditBtns[b].drawButton(false);  // Draw button normally

          // Item 1
          if (b == 0) {
            setItemCount(1, item1.count + 1);
          }
          else if (b == 1) {
            setItemCount(1, item1.count - 1);
          }
          else if (b == 2) {
            setItemPrice(1, item1.price + 1);
          }
          else if (b == 3) {
            setItemPrice(1, item1.price - 1);
          }
          
          // Item 2
          else if (b == 4) {
            setItemCount(2, item2.count + 1);
          }
          else if (b == 5) {
            setItemCount(2, item2.count - 1);
          }
          else if (b == 6) {
            setItemPrice(2, item2.price + 1);
          }
          else if (b == 7) {
            setItemPrice(2, item2.price - 1);
          }
          
          // Item 3
          else if (b == 8) {
            setItemCount(3, item3.count + 1);
          }
          else if (b == 9) {
            setItemCount(3, item3.count - 1);
          }
          else if (b == 10) {
            setItemPrice(3, item3.price + 1);
          }
          else if (b == 11) {
            setItemPrice(3, item3.price - 1);
          }
          else if (b == 12) {
            state = SM_IDLE;
          }

          // Re-display current item counts and prices
          tft.fillRect(0, 115, 480, 25, HX8357_BLACK);
          tft.setCursor(20, 120);
          tft.print(item1.count);
          tft.setCursor(75, 120);
          tft.print(item1.priceStr);

          tft.setCursor(185, 120);
          tft.print(item2.count);
          tft.setCursor(240, 120);
          tft.print(item2.priceStr);

          tft.setCursor(350, 120);
          tft.print(item3.count);
          tft.setCursor(405, 120);
          tft.print(item3.priceStr);

          delay(100); // UI debouncing
        }
      }
    }
  }
}


/*
 * @brief  This function draws all of the UI elements for the set-item
 *         menu (maintenence menu).
 *        
 * @param  void
 * @return void
 */
void drawSetItemMenu(void) {
  tft.fillScreen(HX8357_BLACK);   // Clear screen
  tft.setTextColor(HX8357_WHITE); // Text color is white
  tft.setTextSize(3);             // Font size 3

  // Item 1 Label
  tft.setCursor(15, 20);
  tft.print("Item 1");

  // Item 2 Label
  tft.setCursor(175, 20);
  tft.print("Item 2");

  // Item 3 Label
  tft.setCursor(345, 20);
  tft.print("Item 3");

  /** Show current item count and price */
  tft.setTextSize(2);
  tft.setCursor(20, 120);
  tft.print(item1.count);
  tft.setCursor(75, 120);
  tft.print(item1.priceStr);

  tft.setCursor(185, 120);
  tft.print(item2.count);
  tft.setCursor(240, 120);
  tft.print(item2.priceStr);

  tft.setCursor(350, 120);
  tft.print(item3.count);
  tft.setCursor(405, 120);
  tft.print(item3.priceStr);

  /** "cnt" and "$" labels under buttons */
  tft.setTextSize(2);
  tft.setCursor(10, 210);
  tft.print("cnt");
  tft.setCursor(100, 210);
  tft.print("$");

  tft.setCursor(175, 210);
  tft.print("cnt");
  tft.setCursor(265, 210);
  tft.print("$");

  tft.setCursor(340, 210);
  tft.print("cnt");
  tft.setCursor(430, 210);
  tft.print("$");


  /** ITEM 1 EDITOR BUTTONS */
  // Increase Item 1 Count
  EditBtns[0].initButton(&tft, 30, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[0], HX8357_WHITE, EditBtnLabels[0], 2);
  EditBtns[0].drawButton();

  // Decrease Item 1 Count
  EditBtns[1].initButton(&tft, 30, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[1], HX8357_WHITE, EditBtnLabels[1], 2);
  EditBtns[1].drawButton();

  // Increase Item 1 Price
  EditBtns[2].initButton(&tft, 105, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[2], HX8357_WHITE, EditBtnLabels[2], 2);
  EditBtns[2].drawButton();

  // Decrease Item 1 Price
  EditBtns[3].initButton(&tft, 105, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[3], HX8357_WHITE, EditBtnLabels[3], 2);
  EditBtns[3].drawButton();


  /** ITEM 2 EDITOR BUTTONS */
  // Increase Item 2 Count
  EditBtns[4].initButton(&tft, 195, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[4], HX8357_WHITE, EditBtnLabels[4], 2);
  EditBtns[4].drawButton();

  // Decrease Item 2 Count
  EditBtns[5].initButton(&tft, 195, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[5], HX8357_WHITE, EditBtnLabels[5], 2);
  EditBtns[5].drawButton();

  // Increase Item 2 Price
  EditBtns[6].initButton(&tft, 270, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[6], HX8357_WHITE, EditBtnLabels[6], 2);
  EditBtns[6].drawButton();

  // Decrease Item 2 Price
  EditBtns[7].initButton(&tft, 270, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[7], HX8357_WHITE, EditBtnLabels[7], 2);
  EditBtns[7].drawButton();


  /** ITEM 3 EDITOR BUTTONS */
  // Increase Item 3 Count
  EditBtns[8].initButton(&tft, 360, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[8], HX8357_WHITE, EditBtnLabels[8], 2);
  EditBtns[8].drawButton();

  // Decrease Item 3 Count
  EditBtns[9].initButton(&tft, 360, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[9], HX8357_WHITE, EditBtnLabels[9], 2);
  EditBtns[9].drawButton();

  // Increase Item 3 Price
  EditBtns[10].initButton(&tft, 435, 75, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[10], HX8357_WHITE, EditBtnLabels[10], 2);
  EditBtns[10].drawButton();

  // Decrease Item 3 Price
  EditBtns[11].initButton(&tft, 435, 175, EDIT_BUTTON_W, EDIT_BUTTON_W, HX8357_WHITE, EditBtnColors[11], HX8357_WHITE, EditBtnLabels[11], 2);
  EditBtns[11].drawButton();


  /** DONE BUTTON */
  EditBtns[12].initButton(&tft, 240, 285, 450, 50, HX8357_WHITE, EditBtnColors[12], HX8357_WHITE, EditBtnLabels[12], 2);
  EditBtns[12].drawButton();
}

/*
 * @brief  This function draws all of the UI elements for the password menu.
 *        
 * @param  void
 * @return void
 */
void drawPasswordMenu(void) {
  tft.fillScreen(HX8357_BLACK); // Clear screen

  /** Draw numberpad as rows and columns on LCD */
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

/*
 * @brief  This function draws all of the UI elements for the select-item menu.
 *        
 * @param  void
 * @return void
 */
void drawItemMenu(void) {
  tft.fillScreen(HX8357_BLACK); // Clear screen

  // Item 1 Button
  ItemMenuBtns[0].initButton(&tft, 75, 75, 125, 125, HX8357_WHITE, HX8357_DARKCYAN, HX8357_WHITE, ItemMenuBtnLabels[0], 3);
  ItemMenuBtns[0].drawButton();

  // Item 2 Button
  ItemMenuBtns[1].initButton(&tft, 240, 75, 125, 125, HX8357_WHITE, HX8357_DARKCYAN, HX8357_WHITE, ItemMenuBtnLabels[1], 3);
  ItemMenuBtns[1].drawButton();

  // Item 3 Button
  ItemMenuBtns[2].initButton(&tft, 405, 75, 125, 125, HX8357_WHITE, HX8357_DARKCYAN, HX8357_WHITE, ItemMenuBtnLabels[2], 3);
  ItemMenuBtns[2].drawButton();

  // Settings button
  ItemMenuBtns[3].initButton(&tft, 405, 300, 150, 40, HX8357_WHITE, HX8357_DARKGREY, HX8357_WHITE, ItemMenuBtnLabels[3], 2);
  ItemMenuBtns[3].drawButton();

  // Draw item 1 price and quantity
  tft.setCursor(45, 150);
  tft.print(item1.priceStr);
  tft.setCursor(45, 175);
  tft.print("Qty: ");
  tft.print(item1.count);

  // Draw item 2 price and quantity
  tft.setCursor(205, 150);
  tft.print(item2.priceStr);
  tft.setCursor(205, 175);
  tft.print("Qty: ");
  tft.print(item2.count);

  // Draw item 3 price and quantity
  tft.setCursor(375, 150);
  tft.print(item3.priceStr);
  tft.setCursor(375, 175);
  tft.print("Qty: ");
  tft.print(item3.count);
}


/*
 * @brief  Blocking function that waits until a user lets go
 *         of the passed button.
 *        
 * @param  Adafruit_GFX_Button btn: The button to wait for a release.
 * @return void
 */
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
