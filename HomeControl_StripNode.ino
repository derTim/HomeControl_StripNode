//RFMInvader 
//Serial/RFM12B Enabled LED Strip Controller

//Needed 3rd Party Library:
//  - https://github.com/kroimon/Arduino-SerialCommand (Serial Command Library)
//  - https://github.com/jwc/jeelib (JeeLib, RFM12B Library)
//  - https://github.com/neophob/WS2801-Library (WS2801, original code by adafruit.com)
//   OR
//  - https://github.com/neophob/LPD8806 (LPD8806, original code by adafruit.com)

#include <SPI.h>
#include <EEPROM.h>

//*************************/
// OSC Stuff
#define OSC_MSG_SET_R "/knbb" //simple method to fix the fancy strip color order
#define OSC_MSG_SET_G "/knbg"
#define OSC_MSG_SET_B "/knbr"
#define OSC_MSG_SET_DELAY "/delay"
#define OSC_MSG_CHANGE_MODE "/mode"
#define OSC_MSG_CHANGE_MODE_DIRECT "/modd"
#define OSC_MSG_AUDIO "/audio"
#define OSC_MSG_CONFIG "/cfg"
#define OSC_MSG_SAVE "/sav"
#define OSC_MSG_UPDATE_PIXEL "/pxl" 


//*************************/
// Config
//*************************/

// define strip hardware, use only ONE hardware type
#define USE_WS2801 1
//#define USE_LPD8806 1

//use serail debug or not
#define USE_SERIAL_DEBUG 1

//use DHCP server OR static IP. Using DHCP and static ip as fallback is not possible, too less space left on arduino ethernet
#define USE_DHCP 0

//uncomment it to enable audio
//#define USE_AUDIO_INPUT 1


//*************************/
// Other includes
//*************************/

//LED Hardware stuff
#ifdef USE_WS2801
  #include <WS2801.h>
#endif
#ifdef USE_LPD8806
  #include <LPD8806.h>
#endif

//Serial Command Library
#ifdef USE_SERIAL_DEBUG
  #include <SerialCommand.h>
#endif


//*************************/
// Other Stuff
//*************************/

//some common color defines
const uint32_t WHITE_COLOR = 0xffffff;

//*************************/
// Fader

const uint8_t FADER_STEPS = 25;
uint8_t clearColR, clearColG, clearColB;
uint8_t oldR, oldG, oldB;
uint8_t faderSteps;

//*************************/
// WS2801
//how many pixels, I use 32 pixels/m
#define NR_OF_PIXELS 31

//EEPROM offset
#define EEPROM_HEADER_1 0
#define EEPROM_HEADER_2 1
#define EEPROM_HEADER_3 2
#define EEPROM_POS_DATA 3
#define EEPROM_POS_CLK 4
#define EEPROM_POS_COUNT 5
#define EEPROM_MAGIC_BYTE 66

#define EEPROM_HEADER_10 10
#define EEPROM_HEADER_11 11
#define EEPROM_HEADER_12 12
#define EEPROM_POS_MODE 13
#define EEPROM_POS_DELAY 14
#define EEPROM_POS_R 15
#define EEPROM_POS_G 16
#define EEPROM_POS_B 17

//output pixels dni:3/2
uint8_t dataPin = 4; 
uint8_t clockPin = 7;  

//dummy init the pixel lib
#ifdef USE_WS2801
WS2801 strip = WS2801(); 
#endif
#ifdef USE_LPD8806
LPD8806 strip = LPD8806(); 
#endif  

//init serial command lib
#ifdef USE_SERIAL_DEBUG
SerialCommand SCmd;
#endif
//*************************/
// Network settings

#ifndef USE_DHCP
   byte myIp[]  = { 192, 168, 1, 111 };
#endif

byte myMac[] = { 0x00, 0x00, 0xAF, 0xFE, 0xBE, 0x01 };

const uint16_t serverPort  = 10000;



//*************************/
// Misc

#define MAX_NR_OF_MODES 16
#define MAX_SLEEP_TIME 160.0f

const uint8_t ledPin = 9;
uint8_t oscR, oscG, oscB;
uint8_t mode, modeSave;
int frames=0;

const byte CONST_I = 'I';
const byte CONST_N = 'N';
const byte CONST_V = 'V';

//update strip after DELAY
uint8_t DELAY = 20;
//current delay value
uint8_t delayTodo = 0;

//reset the arduino
void(* resetFunc) (void) = 0; //declare reset function @ address 0

/******************************************************************************************
 *  SETUP
 *****************************************************************************************/
void setup(){ 
  //init
  oscR = 255;
  oscG = 255;
  oscB = 255;
  mode=0;

  int cnt = NR_OF_PIXELS;
  
  Serial.begin(115200);
  
  Serial.println("INV2801!");


  //check if data/clk port is stored in the eeprom. First check for header INV 
  if (checkEepromSignature()) {
    //read data and clk pin from the eeprom
    dataPin = EEPROM.read(EEPROM_POS_DATA);
    clockPin = EEPROM.read(EEPROM_POS_CLK);
    cnt = EEPROMReadInt(EEPROM_POS_COUNT);
    
    //just add some sanity check here, prevent out of memory
    if (cnt<1 || cnt>1024) {
      cnt = NR_OF_PIXELS;
    }
  }


  Serial.print("DataPin: ");
  Serial.print(dataPin, DEC);
  Serial.print(" ClockPin: ");
  Serial.print(clockPin, DEC);
  Serial.print(" Count: ");
  Serial.println(cnt, DEC);

  strip = WS2801(cnt, dataPin, clockPin); 

  
  //start strips 
  strip.begin();
 
  //init Serial Command Module
  setupInSerial();
  
  
  //init effect
  initMode();

  pinMode(ledPin, OUTPUT);  

  //we-are-ready indicator
  synchronousBlink();

  //load presets
  restorePresetStateFromEeprom();

  
}

/*****************************************************************************************
 *  LOOP
 *****************************************************************************************/
void loop(){    

  //check if serial data available
  loopInSerial(); 
  
  //check if the effect should be updated or not
  if (delayTodo>0) {
    //delay not finished yet - do not modify the strip but read network messages
    delayTodo--;
    delay(1);
  } 
  else {
    //delay finished, update the strip content
    delayTodo=DELAY;
    
    switch (mode) {
    case 0: //color lines
      loopLines();
      break;
    case 1:  //solid color white
    case 2:  //solid color wheel fader
    case 3:  //solid color random fader
      loopSolid();
      break;
    case 4:
      loopRainbow();   //color wheel aka rainbow
      break;
    case 5: //knight rider, 1 mover
    case 6: //knight rider, 4 movers
    case 7: //knight rider, 8 movers
    case 8: //knight rider, block mode
      loopKnightRider();    
      break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      loopXor(); //fader
      break;
    case 15:
      loopHsb(); //fader
      break;
      //internal mode, fade from one color to another
    case 200:
      faderLoop();
      break;
    }
    strip.show();    
    frames++;
  } 

}

/*****************************************************************************************
 *  INIT MODE
 *****************************************************************************************/

void initMode() {

  switch (mode) {
  case 0:
    setupLines();
    break;
  case 1:  
  case 2:
  case 3:
    setupSolid(mode-1);
    break;
  case 4:
    setupRainbow();    
    break;
  case 5:
    setupKnightRider(strip.numPixels()/10, 1, 0);    
    break;
  case 6:
    setupKnightRider(strip.numPixels()/10, 4, 0);    
    break;
  case 7:
    setupKnightRider(2, 8, 0);    
    break;
  case 8:
    setupKnightRider(1, 0, 1);    
    break;
  //save some bytes here, setupXor from (0) till (5)
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 14:
    setupXor(mode-9);
    break;
//  case 15:    
  }  

#ifdef USE_SERIAL_DEBUG
  Serial.print("mode: ");
  Serial.println(mode);
#endif  
}




