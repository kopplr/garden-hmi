#include <TimeLib.h>
#include <LiquidCrystal.h>
#include <pins_arduino.h>
#include <dht.h>

dht DHT;
#define DHT11_PIN 3

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5


static int lastButtonPressed = btnNONE;

// Define relays
#define relayLIGHT   11
#define relayHEATER  12
#define relayPUMP    13


//HMI States
#define state_MAIN      0
#define state_SETMENU   1  
#define state_TIMESET   2
#define state_PHOTOSET  3
#define state_WATERSET  4
#define state_TEMPSET   5

#define timeSet_MIN     0
#define timeSet_HOUR    1

#define photoSet_MINSTART   0
#define photoSet_HOURSTART  1
#define photoSet_MINSEND    2
#define photoSet_HOURSEND   3

#define relayON  0
#define relayOFF 1

int photoSet_minStart = 0;
int photoSet_hourStart = 19;
int photoSet_minEnd = 0;
int photoSet_hourEnd = 9;
boolean lightON;

int tempTarget = 24;
float heaterOn_Duration = 0;
float total_Duration = 0;

int waterSet_mLperHour = 30;

int system_State   = state_MAIN;
int setMenu_state  = state_TIMESET;
int timeSet_state  = timeSet_MIN;
int photoSet_state = photoSet_MINSTART;

void setup()
{
 lcd.begin(16, 2);              // start the library
 Serial.begin(9600);
 pinMode(A0, INPUT);
 pinMode(relayLIGHT, OUTPUT);  
 pinMode(relayPUMP, OUTPUT);
 pinMode(relayHEATER, OUTPUT);
 
 digitalWrite(A0, HIGH);

 setTime(12, 0, 0, 1, 1, 2018);
}
 
void loop()
{
  int buttonState = read_LCD_buttons();
//  Serial.print(year());
//  Serial.print('\t');
//  Serial.print(setMenu_state);
//  Serial.print('\n'); 

  // Control
  lightCtrl();
  waterCtrl();
  heatCtrl();
  
  switch(system_State){
    
    case state_MAIN:
    {
      if(buttonState == btnSELECT){
        system_State = state_SETMENU;   //Enter Settings menu on next loop
      }
      //updateTempAndHumidity();
      lcdUpdate_Main();
      break;
    }
    
    case state_SETMENU:
    {
      if(buttonState == btnUP){
        setMenu_state -= 1;
        if(setMenu_state < state_TIMESET){
          setMenu_state = state_TEMPSET;
        }
      }else if(buttonState == btnDOWN){
        setMenu_state += 1;
        if(setMenu_state > state_TEMPSET){
          setMenu_state = state_TIMESET;
        }
      }else if(buttonState == btnSELECT){
        system_State = setMenu_state;
      }else if(buttonState == btnRIGHT || buttonState == btnLEFT){  //Exit out of settings by clicking left or right button
        system_State = state_MAIN;
      }
      lcdUpdate_setMenu();
      break;
    }
    
    case state_TIMESET:
    {
      if(buttonState == btnUP){
        if(!timeSet_state){ setTime(hour(),minute()+1,second(),day(),month(),year());}
        else{               setTime(hour()+1,minute(),second(),day(),month(),year());}

      }else if(buttonState == btnDOWN){
        if(!timeSet_state){ setTime(minute()>0?hour():(hour()>0?hour()-1:23),minute()>0?minute()-1:59,second(),day(),month(),year());}
        else{               setTime(hour()>0?hour()-1:23,minute(),second(),day(),month(),year());}

      }else if(buttonState == btnRIGHT || buttonState == btnLEFT){ 
        timeSet_state = (timeSet_state + 1)%2;                    //Left & right toggle between minutes and hours
      }else if(buttonState == btnSELECT){
        system_State = state_SETMENU;                             //'Select' exits  back to setup menu
      }
      lcdUpdate_timeSet(); 
      break;
    }

    case state_PHOTOSET:
    {
      if(buttonState == btnUP){
        if     (photoSet_state == photoSet_MINSTART){  photoSet_minStart = (photoSet_minStart+1)%60;  }
        else if(photoSet_state == photoSet_HOURSTART){ photoSet_hourStart = (photoSet_hourStart+1)%24;}
        else if(photoSet_state == photoSet_MINSEND){    photoSet_minEnd = (photoSet_minEnd+1)%60;      }
        else if(photoSet_state == photoSet_HOURSEND){   photoSet_hourEnd = (photoSet_hourEnd+1)%24;    }
        
      }else if(buttonState == btnDOWN){
        if     (photoSet_state == photoSet_MINSTART){  photoSet_minStart = (photoSet_minStart-1)%60;  }
        else if(photoSet_state == photoSet_HOURSTART){ photoSet_hourStart = (photoSet_hourStart-1)%24;}
        else if(photoSet_state == photoSet_MINSEND){    photoSet_minEnd = (photoSet_minEnd-1)%60;      }
        else if(photoSet_state == photoSet_HOURSEND){   photoSet_hourEnd = (photoSet_hourEnd-1)%24;    }

        if(photoSet_minStart<0){  photoSet_minStart = 59; }
        if(photoSet_hourStart<0){ photoSet_hourStart = 23;}
        if(photoSet_minEnd<0){    photoSet_minEnd = 59;   }
        if(photoSet_hourEnd<0){   photoSet_hourEnd = 23;  }
               
      }else if(buttonState == btnLEFT){ 
        photoSet_state = (photoSet_state + 1)%4;                    //Left & right toggle between minutes and hours
      }else if(buttonState == btnRIGHT){ 
        photoSet_state = (photoSet_state - 1)%4;                    //Left & right toggle between minutes and hours
        if(photoSet_state < 0){ photoSet_state = photoSet_HOURSEND; }
      }else if(buttonState == btnSELECT){
        system_State = state_SETMENU;                             //'Select' exits  back to setup menu
      }
      lcdUpdate_photoSet(); 
      break;
    }
    case state_WATERSET:
    {
      if(buttonState == btnUP){
        waterSet_mLperHour += 1;
      }else if(buttonState == btnDOWN){
        waterSet_mLperHour -= 1;   
      }else if(buttonState == btnSELECT){
        system_State = state_SETMENU;                             //'Select' exits  back to setup menu
      }
      if(waterSet_mLperHour > 500){  waterSet_mLperHour = 500; }
      else if(waterSet_mLperHour < 0){ waterSet_mLperHour = 0; }
      lcdUpdate_waterSet();
      break;
    }
    case state_TEMPSET:
    {
      if(buttonState == btnUP){
        tempTarget += 1;
      }else if(buttonState == btnDOWN){
        tempTarget -= 1;
      }else if(buttonState == btnSELECT){
        system_State = state_SETMENU;                             //'Select' exits  back to setup menu
      }

      if(tempTarget > 30){ tempTarget = 30; }
      if(tempTarget < 15){ tempTarget = 15; }
      
      lcdUpdate_tempSet();
      break;
    }
  }
}

void lcdUpdate_Main(){
  static int photoDurationMin;
  static int photoDurationHour;
  
  lcd.setCursor(0,0);
  lcd.print(" ");
  lcd.print((int)DHT.temperature);
  lcd.print(char(223));
  lcd.print("C");

  lcd.print("     ");
  
  if(hour() < 10){lcd.print(" ");}
  lcd.print(hour());
  lcd.print(":");
  if(minute() < 10){lcd.print("0");}
  lcd.print(minute());
  lcd.print("  ");

  lcd.setCursor(0,1);
  lcd.print(" ");
  lcd.print(DHT.humidity);
  lcd.print("%             ");

  lcd.setCursor(9,1);
  lcd.print(heaterOn_Duration/total_Duration);
//  photoSet_hourEnd > photoSet_hourStart ? photoDurationHour = photoSet_hourEnd - photoSet_hourStart : photoDurationHour = 24 - photoSet_hourStart + photoSet_hourEnd;
//  if(photoSet_minEnd >= photoSet_minStart) {
//    photoDurationMin = photoSet_minEnd - photoSet_minStart;
////    if(photoDurationMin == 60 || photoDurationMin == 0){
////      photoDurationMin = 0;
////      photoDurationHour++;
////    }
//  }
//  else {
//    photoDurationMin = 60 - photoSet_minStart + photoSet_minEnd;
//    photoDurationHour--;
//  }
//  
//  if(photoDurationHour < 10){lcd.print(" ");}
//  lcd.print(String(photoDurationHour) + "h");
//  if(photoDurationMin < 10){lcd.print("0");}
//  lcd.print(String(photoDurationMin));
  
}

void lcdUpdate_setMenu(){
  lcd.setCursor(0,0);
  lcd.print(" Time    Light  ");
  lcd.setCursor(0,1);
  lcd.print(" Water   Temp   ");

  if(setMenu_state == state_TIMESET){       lcd.setCursor(0,0); }
  else if(setMenu_state == state_PHOTOSET){ lcd.setCursor(8,0); }
  else if(setMenu_state == state_WATERSET){ lcd.setCursor(0,1); }
  else if(setMenu_state == state_TEMPSET){  lcd.setCursor(8,1); }

  lcd.print("*");
}

void lcdUpdate_timeSet(){
  lcd.setCursor(0,0);
  lcd.print("Set the time:   ");
  
  lcd.setCursor(0,1);
  lcd.print("     ");
  
  // Set hour
  if(hour() < 10){lcd.print(" "); }
  lcd.print(hour());
  lcd.print(":");

  // Set minute
  if(minute() < 10){lcd.print("0");}
  lcd.print(minute());
  lcd.print("   ");
}

void lcdUpdate_photoSet(){
  lcd.setCursor(0,0);
  lcd.print("Turn on:  "); 
  if(photoSet_hourStart < 10){lcd.print(" "); } lcd.print(photoSet_hourStart);
  lcd.print(":");
  if(photoSet_minStart < 10){lcd.print(0);} lcd.print(photoSet_minStart);
  lcd.print("  ");

  lcd.setCursor(0,1);
  lcd.print("Turn off: "); 
  if(photoSet_hourEnd < 10){lcd.print(" "); } lcd.print(photoSet_hourEnd);
  lcd.print(":");
  if(photoSet_minEnd < 10){lcd.print(0);} lcd.print(photoSet_minEnd);
  lcd.print(" ");
}

void lcdUpdate_waterSet(){
  lcd.setCursor(0,0);
  lcd.print("mL per hour: ");
  if(waterSet_mLperHour < 10){lcd.print("0"); } 
  if(waterSet_mLperHour < 100){lcd.print("0"); } 
  lcd.print(waterSet_mLperHour);

  lcd.setCursor(0,1);
  lcd.print("Waters in light ");
}

void lcdUpdate_tempSet(){
  lcd.setCursor(0,0);
  lcd.print("Temperature: ");
  lcd.print(tempTarget);
  lcd.print(char(223));
  lcd.setCursor(0,1);
  lcd.print("                ");
}
    


int read_LCD_buttons(){
  
  int curButtonPressed= btnNONE;
  adc_key_in = analogRead(0); 
 
 if (adc_key_in < 50){        curButtonPressed = btnRIGHT; } 
 else if (adc_key_in < 195){  curButtonPressed = btnUP;    }
 else if (adc_key_in < 380){  curButtonPressed = btnDOWN;  }
 else if (adc_key_in < 555){  curButtonPressed = btnLEFT;  }
 else if (adc_key_in < 790){  curButtonPressed = btnSELECT;} 
 else{                        curButtonPressed = btnNONE;  }// We make this the 1st option for speed reasons since it will be the most likely result 
 
  if(curButtonPressed != lastButtonPressed){ 
    lastButtonPressed = curButtonPressed;
    delay(200); 
  }
  if(curButtonPressed != btnNONE || true){ delay(50); }
//  Serial.println(curButtonPressed);
  return curButtonPressed;
}

void lightCtrl(){
  int startTimetoMin = 60*photoSet_hourStart + photoSet_minStart;
  int endTimetoMin   = 60*photoSet_hourEnd + photoSet_minEnd;
  int curTimetoMin   = 60*(int)hour() + (int)minute();
  
  if(curTimetoMin >= startTimetoMin){
    if(endTimetoMin > startTimetoMin){
      if(curTimetoMin < endTimetoMin){
        digitalWrite(relayLIGHT, relayON);
        lightON = true;
      }else{
        digitalWrite(relayLIGHT, relayOFF);
        lightON = false;
      }
    }else{
      digitalWrite(relayLIGHT, relayON);
      lightON = true;
    }
  }else{
    if(endTimetoMin > startTimetoMin){
      digitalWrite(relayLIGHT, relayOFF);
      lightON = false;
    }
    else{
      if(curTimetoMin < endTimetoMin){
        digitalWrite(relayLIGHT, relayON);
        lightON = true;
      }else{
        digitalWrite(relayLIGHT, relayOFF);
        lightON = false;
      }
    }
  }
}

void heatCtrl(){
  int chk = DHT.read11(DHT11_PIN);
  static int heaterSchmitt = 2;

  if(DHT.temperature <= tempTarget - heaterSchmitt && lightON){
    digitalWrite(relayHEATER, relayON);
    heaterSchmitt = 1;
    heaterOn_Duration += 1;
  }else{
    digitalWrite(relayHEATER, relayOFF);
    heaterSchmitt = 2;
  }
  total_Duration += 1;
}

void waterCtrl(){
  if((int)minute()*60 + (int)second() < waterSet_mLperHour && lightON){     //Pumps displaces 1mL/second
    digitalWrite(relayPUMP, relayON);
  }else{
    digitalWrite(relayPUMP, relayOFF);
  }
}

