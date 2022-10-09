/* 
 PC-to-Jamma Interface based on Arduino DUE - shield
 Controls section (JOYSTICK) and sync frequency check and block (>15KHz by default, but 
 customizable for higher frequencies (i.e. 25KHz or 31KHz).
 
 Tested with 2.0.7 joystick library.

 by Barito, 2017-2022
*/

#include <Joystick.h>

#define INPUTS 26

//comment the following line to disable the sync frequency check and block
#define SYNC_MONITOR_ACTIVE

#define BYPASS

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 32, 0, //joy type, button count, hatswitch count
  false, false, false, // X, Y, Z axis
  false, false, false, // X, Y, Z rotation
  false, false, //rudder, throttle
  false, false, false); //accelerator, brake, steering

const int HSyncPin = A8;
const int disablePin = A11;
const int bypassPin = A10;
const int ledPin = 13;

const int P1_B7 = A3; //UNUSED
const int P2_B7 = A4; //UNUSED

const int K25Pin = 3;
const int K31Pin = 4;

int fq;
const int delayTime = 20;
boolean startBlock = 0;
boolean enableState;
boolean prevEnState;

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 66 us
int periodoSum;
int periodoIst;
int periodoMedio;
int sCounter;
const int samples = 10;

struct digitalInput {const byte pin; boolean state; unsigned long dbTime; const byte button; const byte button_shift;} 
digitalInput[INPUTS] = {
{40, HIGH, 0, 0, 0},  //P1 START - SHIFT BUTTON
{41, HIGH, 0, 1, 24}, //P2 START (ESC)
{38, HIGH, 0, 2, 25}, //P1 UP (tilde)
{36, HIGH, 0, 3, 26}, //P1 DWN (pause)
{34, HIGH, 0, 4, 27}, //P1 LEFT (ENTER)
{32, HIGH, 0, 5, 28}, //P1 RIGHT (TAB)
{30, HIGH, 0, 6, 6},  //P1 B1
{28, HIGH, 0, 7, 7},  //P1 B2
{26, HIGH, 0, 8, 8},  //P1 B3
{24, HIGH, 0, 9, 9},  //P1 B4
{22, HIGH, 0, 10, 10},//P1 B5
{A1, HIGH, 0, 11, 29},//P1 B6 (coin)
{39, HIGH, 0, 12, 12},//P2 UP
{37, HIGH, 0, 13, 13},//P2 DWN
{35, HIGH, 0, 14, 14},//P2 LEFT
{33, HIGH, 0, 15, 15},//P2 RIGHT
{31, HIGH, 0, 16, 16},//P2 B1
{29, HIGH, 0, 17, 17},//P2 B2
{27, HIGH, 0, 18, 18},//P2 B3
{25, HIGH, 0, 19, 19},//P2 B4
{23, HIGH, 0, 20, 20},//P2 B5
{A2, HIGH, 0, 21, 21}, //P2 B6
{42, HIGH, 0, 22, 22},//P1 COIN
{43, HIGH, 0, 23, 23},//P2 COIN
{45, HIGH, 0, 30, 30},//SERVICE SW
{44, HIGH, 0, 31, 31},//TEST SW
};

void setup(){
  
for (int j = 0; j < INPUTS; j++){
  pinMode(digitalInput[j].pin, INPUT_PULLUP);
  digitalInput[j].state = digitalRead(digitalInput[j].pin);
  digitalInput[j].dbTime = millis();
}  

pinMode(HSyncPin, INPUT_PULLUP);
pinMode(K25Pin, INPUT_PULLUP);
pinMode(K31Pin, INPUT_PULLUP);
pinMode(P1_B7, INPUT_PULLUP);
pinMode(P2_B7, INPUT_PULLUP);
pinMode(K25Pin, INPUT_PULLUP);
pinMode(ledPin, OUTPUT);
pinMode(bypassPin, OUTPUT);
pinMode(disablePin, OUTPUT);

#ifdef SYNC_MONITOR_ACTIVE
digitalWrite(disablePin, HIGH); //DISABLE
digitalWrite(ledPin, LOW);
#else
digitalWrite(disablePin, LOW); //ENABLE
digitalWrite(ledPin, HIGH);
#endif

#ifdef BYPASS
digitalWrite(bypassPin, HIGH);
#else
digitalWrite(bypassPin, LOW);
#endif

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 64 us
//set the blocking frequency
if (digitalRead(K25Pin) == LOW){fq = 28;} //25KHZ
else if (digitalRead(K31Pin) == LOW){fq = 11;} //31KHZ
else {fq = 55;} //15KHZ - default

Joystick.begin();

} // setup close

void loop(){
generalInputs();
shiftInputs();
#ifdef SYNC_MONITOR_ACTIVE
freqBlock();
#endif
}

void generalInputs(){
//general input handling
for (int j = 1; j < INPUTS; j++){
 if (millis()-digitalInput[j].dbTime > delayTime && digitalRead(digitalInput[j].pin) !=  digitalInput[j].state){
    digitalInput[j].state = !digitalInput[j].state;
    digitalInput[j].dbTime = millis();
    if(digitalInput[0].state == HIGH){ //shift button not pressed
       Joystick.setButton(digitalInput[j].button, !digitalInput[j].state);
    }
    else{
       startBlock = 1;
       Joystick.setButton(digitalInput[j].button_shift, !digitalInput[j].state);
    }
  }
}  
}

void shiftInputs(){
//reversed input handling (P1 START) - shift button
if (millis()-digitalInput[0].dbTime > delayTime && digitalRead(digitalInput[0].pin) !=  digitalInput[0].state){
    digitalInput[0].state = !digitalInput[0].state;
    digitalInput[0].dbTime = millis();
    if (digitalInput[0].state == HIGH){
      if(startBlock == 0){
        Joystick.setButton(digitalInput[0].button, HIGH);
        delay(30);
        Joystick.setButton(digitalInput[0].button, LOW);
       }
      else{
        startBlock = 0;
      }
    }
}
}

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 66 us
void freqBlock(){
periodoIst = pulseIn(HSyncPin,HIGH);//time (in us) between a high and low pulse (negtive sync, 5% duty cicle)
  periodoSum = periodoSum + periodoIst;
  sCounter++;
  if(sCounter > samples){
    periodoMedio = (periodoSum/sCounter);
    periodoSum = 0;
    sCounter = 0;
    if(periodoMedio > fq){
      enableState = 1;
    }
    else {
      enableState = 0;
    }
    if (enableState != prevEnState){
      prevEnState = enableState;
      digitalWrite(disablePin, !enableState);
      digitalWrite(ledPin, enableState);
    }
  }
}
