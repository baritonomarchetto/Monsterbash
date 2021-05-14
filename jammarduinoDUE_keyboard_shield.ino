/* 
 Interfaccia PC-to-Jamma per Arduino DUE - shield
 Sezione controlli (TASTIERA) e blocco frequenze >15KHz (default, ma impostabile
 anche a 25KHz o 31KHz).
 Il circuito complessivo prevede l'uso di un amplificatore integrato THS7374.

 by Barito, 2017-2021
*/

#include <Keyboard.h>

#define INPUTS 26

//commenta la riga seguente per disattivare il filtro di blocco frequenza di sincronia
#define SYNC_MONITOR_ACTIVE

const int HSyncPin = A8;
const int disablePin = A11;
const int bypassPin = A10;
const int ledPin = 13;

const int P1_B7 = A3; //UNUSED
const int P2_B7 = A4; //UNUSED

const int K25Pin = 3;
const int K31Pin = 4;
const int BYPASS = 1;

int fq;
const int delayTime = 20;
boolean startBlock = 0;

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 66 us
unsigned long periodoSum = 0;
unsigned long periodoIst = 0;
unsigned long periodoMedio=0;
int samplesm;
const int samples = 10;

struct digitalInput {const byte pin; boolean state; unsigned long dbTime; const byte key; const byte key_shift;} 
digitalInput[INPUTS] = {
{40, HIGH, 0, 49, 49}, //1 - P1 START
{41, HIGH, 0, 50, 177}, //2 - P2 START (ESC)

{38, HIGH, 0, 218, 189}, //up arrow - P1 UP (tilde)
{36, HIGH, 0, 217, 112}, //down arrow - P1 DWN (p)
{34, HIGH, 0, 216, 176}, //left arrow - P1 LEFT (ENTER)
{32, HIGH, 0, 215, 179}, //right arrow - P1 RIGHT (TAB)
{30, HIGH, 0, 128, 53},  //left ctrl - P1 B1 (5)
{28, HIGH, 0, 130, 54},  //left alt - P1 B2 (6)
{26, HIGH, 0, 180, 223}, //space - P1 B3 (+)
{24, HIGH, 0, 129, 129}, //left shift - P1 B4
{22, HIGH, 0, 122, 122}, //z - P1 B5
{A1, HIGH, 0, 120, 222},  //x - P1 B6 (-)

{39, HIGH, 0, 114, 114}, //r - P2 UP
{37, HIGH, 0, 102, 102}, //f - P2 DWN
{35, HIGH, 0, 100, 100}, //d - P2 LEFT
{33, HIGH, 0, 103, 103}, //g - P2 RIGHT
{31, HIGH, 0, 97, 97},   //a - P2 B1
{29, HIGH, 0, 115, 115}, //s - P2 B2
{27, HIGH, 0, 113, 113}, //q - P2 B3
{25, HIGH, 0, 119, 119}, //w - P2 B4
{23, HIGH, 0, 105, 105}, //i - P2 B5
{A2, HIGH, 0, 107, 107},  //k - P2 B6

{42, HIGH, 0, 53, 53}, //5 - P1 COIN
{43, HIGH, 0, 54, 54}, //6 - P2 COIN

{45, HIGH, 0, 57, 57}, //9 - SERVICE SW
{44, HIGH, 0, 48, 48}, //0 - TEST SW
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
  
if (BYPASS){digitalWrite(bypassPin, HIGH);}
else {digitalWrite(bypassPin, LOW);}

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 66 us
//set the blocking frequency
if (digitalRead(K25Pin) == LOW){fq = 28;} //25KHZ
else if (digitalRead(K31Pin) == LOW){fq = 11;} //31KHZ
else {fq = 55;} //15KHZ - default

Keyboard.begin();

} // chiudo setup

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
      if (digitalInput[j].state == LOW){
        Keyboard.press(digitalInput[j].key);}
      else {Keyboard.release(digitalInput[j].key);}
    }
    else{
      if (digitalInput[j].state == LOW){
        startBlock = 1;
        Keyboard.press(digitalInput[j].key_shift);}
      else {Keyboard.release(digitalInput[j].key_shift);}
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
      if (startBlock == 0){
        Keyboard.press(digitalInput[0].key);
        delay(30);
        Keyboard.release(digitalInput[0].key);
      }
      else{startBlock = 0;}
    }
}
}

//31KHz  -> 32 us
//25KHz -> 40 us
//15KHz -> 66 us
void freqBlock(){
periodoSum = 0;  
samplesm = samples;
for(int i=0; i<samples; i++){
  periodoIst = pulseIn(HSyncPin,HIGH);
  if(periodoIst < 100 && periodoIst > 10){
    periodoSum += periodoIst;
  } 
  else {
    samplesm--;
  }
  periodoMedio = (periodoSum/samplesm)+5;
}
if(periodoMedio > fq){
  digitalWrite(disablePin, LOW); //ENABLE
  digitalWrite(ledPin, HIGH);
}
else {
  digitalWrite(disablePin, HIGH);//DISABLE
  digitalWrite(ledPin, LOW);
}
}
