// ALFRDUINO (c) 2021

#include <OneButton.h>
#include <Chrono.h> 
#include <MIDI.h>
#include <TM1637Display.h>
#include <EEPROM.h>

MIDI_CREATE_DEFAULT_INSTANCE();

OneButton DN(4, true);
OneButton UP(5, true);
OneButton MODE(6, true);

Chrono displayBlink;
const uint8_t blinkTime = 250;

uint8_t progNum, prev_progNum = 250;
uint8_t bankNum, prev_bankNum = 250;
uint8_t liveNum, prev_liveNum = 250;
uint8_t mode;

uint8_t data[] = { 0xff, 0xff, 0xff, 0xff }; // create array that turn all segment ON
uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 }; // create array that turn all segment OFF


const uint8_t addrProg = 0;
const uint8_t addrBank = 1;
const uint8_t addrLiveNum = 2;
const uint8_t addrMode = 3;

// Module connection pins (Digital Pins)
#define CLK 2
#define DIO 3

TM1637Display display(CLK, DIO);


/* 
 * #################################################################
 * #########################    SETUP   ############################
 * #################################################################
 */
   
void setup() {
  
  //Serial.begin(9600);
  //Serial.println("starting...");
 
  DN.attachClick(clickDN);
  UP.attachClick(clickUP);
  MODE.attachClick(clickMODE);
  MODE.attachDoubleClick(doubleclickMODE);
  MODE.attachLongPressStart(longPressStartMODE);
  
  display.setBrightness(0x0f); // set brightness
  display.setSegments(data);

  progNum = EEPROM.get(addrProg, progNum);
  if (progNum == NAN) {
    progNum = 0;
    EEPROM.put(addrProg, progNum);
   }
  
  bankNum = EEPROM.get(addrBank, bankNum);
  if (bankNum == NAN) {
    bankNum = 0;
    EEPROM.put(addrBank, bankNum);
   }
  
  liveNum = EEPROM.get(addrLiveNum, liveNum);
  if (liveNum == NAN) {
    liveNum = 0;
    EEPROM.put(addrLiveNum, liveNum);
   }
  
  mode = EEPROM.get(addrMode, mode);
  if (mode == NAN) {
    mode = 0;
    EEPROM.put(addrMode, mode);
   };

   MIDI.begin(MIDI_CHANNEL_OMNI);

  delay(1000);

  sendMidi(progNum, bankNum);
}


/* 
 *  #################################################################
 * #######################    MAIN LOOP   ##########################
 * #################################################################
 */
   
void loop() {
  
  DN.tick();
  UP.tick();
  MODE.tick();

  switch (mode) { 

    case 0:
      if(bankNum != prev_bankNum)
      {  // if the displayed (current) number was changed
        prev_bankNum = bankNum;   // save current value of 'num'
        displayVal(progNum,bankNum);
        sendMidi(progNum, bankNum);
        EEPROM.update(addrBank, bankNum); // save values in Arduino internal memory
      }  
      if(progNum != prev_progNum)
      {  // if the displayed (current) number was changed
        prev_progNum = progNum;   // save current value of 'num'
        displayVal(progNum,bankNum);
        sendMidi(progNum, bankNum);
        EEPROM.update(addrProg, progNum); // save values in Arduino internal memory
      }
      break;
    
    case 1: 
      if (displayBlink.hasPassed(blinkTime)) {
        displayBlink.restart(); 
        displayPreProg(progNum);
      };
      break;

    case 2: 
      if (displayBlink.hasPassed(blinkTime)) {
        displayBlink.restart(); 
        displayPreBank(bankNum);
      }
      break;

    case 3:
      if(liveNum != prev_liveNum)
      {  // if the displayed (current) number was changed
        prev_liveNum = liveNum;   // save current value of 'num'
        displayLiveNum(liveNum);
        sendMidi(progNum, bankNum);
        EEPROM.update(addrLiveNum, liveNum); // save values in Arduino internal memory
      };
      break;
    
  }
}


/* #################################################################
 * ########################   FUNCTIONS   ##########################
 * #################################################################
 */


/////////////////////// BUTTONS FUNCTIONS /////////////////////////


void clickDN() {
  //Serial.println("Button DOWN click.");
  
  if(mode==0 || mode==1){ // decrement the num
    progNum--;
    if(progNum < 0 || progNum == 255)
    progNum = 99;
  }
  
  if(mode==2){ // decrement the bank
    bankNum--;
    if(bankNum < 0)
    bankNum = 3;
  }

  if(mode==3){ // decrement the bank
    liveNum--;
    if(liveNum < 1)
    liveNum = 5;
  }
}


void clickUP() {
  //Serial.println("Button UP click.");
  if(mode==0 || mode==1){ // decrement the num
    progNum++;
    if(progNum > 99)
    progNum = 0;
  }
  if(mode==2){ // decrement the bank
    bankNum++;
    if(bankNum >3)
    bankNum = 0;
  }

  if(mode==3){ // decrement the bank
    liveNum++;
    if(liveNum > 5)
    liveNum = 1;
  }
  
}


void clickMODE() {
  //Serial.println("Button MODE click.");
  if(mode!=2 || mode!=3) {
    mode = abs(mode-1); //switch between 0 and 1
    EEPROM.update(addrMode, mode); // save values in Arduino internal memory
  }
  if(mode==2) {
    mode = 1;
  }
  prev_progNum = 250;
  prev_bankNum = 250;
  prev_liveNum = 250;
}


void doubleclickMODE() {
  //Serial.println("Button MODE doubleclick.");
  if(mode!=2 ){
    mode = 2;
  }
  else if(mode=2) {
    mode = 0;
    EEPROM.update(addrMode, mode); // save values in Arduino internal memory
  }
  prev_progNum = 250;
  prev_bankNum = 250;
  prev_liveNum = 250;
}

void longPressStartMODE() {
  //Serial.println("Button MODE Long Press Start.");
  if(mode!=3) {
    mode = 3;
  }
  else {
    mode = 0;
  }
  prev_progNum = 250;
  prev_bankNum = 250;
  prev_liveNum = 250;
  EEPROM.update(addrMode, mode); // save values in Arduino internal memory
}



////////////////// DISPLAY FUNCTIONS //////////////////////////

// int the NordStage2 presets are displayed as A:01:1  where (A,B,C,D):(01 to 20):(1 to 5)
void displayVal(uint8_t prog, uint8_t bank) {
  uint8_t num1 = (prog/5) + 1;
  uint8_t num2 = (prog % 5) + 1;
  display.clear();
  display.showNumberDec(num1, true, 2, 1);  // Expect: _XX_
  display.showNumberDec(num2, true, 1, 3);  // Expect: ___X
  switch (bank) {
    case 0:
    display.showNumberHexEx(0xa, 0, true, 1); // Expect: A___
    break;
    case 1:
    display.showNumberHexEx(0xb, 0, true, 1); // Expect: B___
    break;
    case 2:
    display.showNumberHexEx(0xc, 0, true, 1); // Expect: C___
    break;
    case 3:
    display.showNumberHexEx(0xd, 0, true, 1); // Expect: D___
    break;
  } 
}

void displayPreProg(uint8_t prog) {
  uint8_t num1 = (prog/5) + 1;
  uint8_t num2 = (prog % 5) + 1;
  display.clear();
  display.showNumberDec(num1, true, 2, 1);  // Expect: _XX_
  display.showNumberDec(num2, true, 1, 3);  // Expect: ___X
}

void displayPreBank(uint8_t bank) {
  display.clear();
  switch (bank) {
    case 0:
    display.showNumberHexEx(0xa, 0, true, 1); // Expect: A___
    break;
    case 1:
    display.showNumberHexEx(0xb, 0, true, 1); // Expect: B___
    break;
    case 2:
    display.showNumberHexEx(0xc, 0, true, 1); // Expect: C___
    break;
    case 3:
    display.showNumberHexEx(0xd, 0, true, 1); // Expect: D___
    break;
  } 
}

void displayLiveNum(uint8_t live) {
  display.clear();
  display.showNumberHexEx(0xff, 0, true, 2); // Expect: FF__
  display.showNumberDec(live, true, 1, 3);  // Expect: ___X
}



////////////////////// MIDI FUNCTIONS //////////////////////////

void sendMidi(int progNum, int bankNum) {
  switch (mode) {

    case 0:
    // first send Bank number
    MIDI.sendControlChange(0, 0, 1);
    MIDI.sendControlChange(32, bankNum, 1);
    // then send Program number
    MIDI.sendProgramChange(progNum, 1);
    break;

    case 3:
    MIDI.sendProgramChange(liveNum + 99, 1); // from 100 to 104
    
  }  
}


/* NOTES
 *  
 * FROM NORD STAGE2 MANUAL:
 * 
 * PROGRAM CHANGE
 * Program Change messages with the value 0-99 selects the programs in the active bank, 
 * Program Change messages 100-104 selects the 5 Live memories. 
 * 
 * BANK SELECT
 * The 4 Program Banks in the Stage 2 can be remotely selected via MIDI, by transmitting a Bank Select Message that looks like this: 
 * CC 0, value 0, CC 32 value 0, 1, 2 or 3 (for banks A, B, C and D).
 * The Bank Select should immediately be followed by a Program Change message, value 0-99.
 */
