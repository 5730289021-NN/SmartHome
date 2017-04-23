//Library
#include <DS1302.h>             //Realtime Clock
#include <IRremote.h>           //Remote Controller
#include <Wire.h>               //TWI for LiquidCrystal
#include <LiquidCrystal_I2C.h>  //LiquidCrystal

#include <TimerOne.h>           //Timer Interrupt

//Pin Define
#define rtRstPin    8 
#define rtDataPin   9
#define rtClkPin    10
#define irReceivePin 11
//Other Define
#define irButtonSize  21

//Realtime Clock Module
DS1302 rtc(rtRstPin, rtDataPin, rtClkPin);

//LiquidCrystal Module
LiquidCrystal_I2C lcd(0x27, 16, 2);


//IR Remote Controller Module
const String remoteString[irButtonSize] = {"Power","Mode","Mute","Play","Back","Next","EQ","Minus","Plus",
                                     "0","Swap","USD","1","2","3","4","5","6","7","8","9"};   
const long remoteValue[irButtonSize] = {0xFFA25D,0xFF629D,0xFFE21D,0xFF22DD,0xFF02FD,0xFFC23D,0xFFE01F,
                                  0xFFA857,0xFF906F,0xFF6897,0xFF9867,0xFFB04F,0xFF30CF,0xFF18E7,
                                  0xFF7A85,0xFF10EF,0xFF38C7,0xFF5AA5,0xFF42BD,0xFF4AB5,0xFF52AD};
IRrecv irrecv(irReceivePin);
decode_results results;

//Global variables
String Hour, Minute, Second;
String Day, Date, Month, Year;
int mode = 0;
int fadingTime = 1500;
bool lcdDisplayEnb = true;
bool soundEnb = true;

String inputTime = "00:00:00";
String relativeTime = "00:00:00";


//Functions
String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "Unknownday";
}

void lcdDisplay(String first,String second) {
  lcd.setCursor(0, 0);
  lcd.print(first);
  lcd.setCursor(0, 1);
  lcd.print(second);
}

String getTime() {
  return (Hour.length()==1?"0":"") + Hour + ":" + (Minute.length()==1?"0":"") + Minute + ":" + (Second.length()==1?"0":"") + Second;
}

String getDate() {
  return Day + " " + Date + "/" + Month + "/" + Year;
}

void fadeout() {
  delay(fadingTime);
  lcd.clear();
}


//Timer Interrupt Function
void updateTime()
{
  Time t = rtc.time();
  if(dayAsString(t.day).equals("Unknownday"))
  {
    return;
  }
  else
  {
    Hour = t.hr;
    Minute = t.min;
    Second = t.sec;
    Day = dayAsString(t.day);
    Date = t.date;
    Month = t.mon;
    Year = t.yr%100;
  }
}

void setup() {
  //UART
  Serial.begin(9600);
  //IR Remote Controller Module
  irrecv.enableIRIn(); // Start the receiver
  //Liquid Crystal
  lcd.begin();
  lcd.backlight();
  //TimerOne
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(updateTime); // after 1 second,it will perform 'updateTime' function
  
}

void loop() {
  switch(mode)
  {
    case 0://Show time
      {
        if(getTime().equals("00:00:00")) lcd.clear();
        lcdDisplay(getTime(),getDate());
        break;
      }
    case 1://Alarm
      {
        lcdDisplay("Alarm ","     " + inputTime);
        break;
      }
    case 2://Trigger
      {
        lcdDisplay("Trigger 1.Trig","        2.Untrig");
        break;
      }
    case 3://Setting
      {
        lcdDisplay("Settings 1.Time","         2.Sound");
        break;
      }
  }
  
  if (irrecv.decode(&results)) {
    //Serial.println(results.value,HEX);
    for(int i = 0;i<irButtonSize;i++)
    {
      if(results.value == remoteValue[i])
      {
        Serial.println(i);
        switch(i)
        {
          case 0://Power Button
          {
            lcdDisplayEnb = !lcdDisplayEnb;
            if(lcdDisplayEnb)
            {
              lcd.display();
              lcd.backlight();
            }
            else
            {
              lcd.noDisplay();
              lcd.noBacklight();
            }
            break;
          }
          case 1://Mode Button
          {
            lcd.clear();
            if(mode < 4) mode++;
            if(mode == 4) mode = 0;
            break;
          }
          case 2://Sound Enb Button
          {
            soundEnb = !soundEnb;
            lcd.clear();
            if(soundEnb) lcdDisplay("Sound: Off","keep silence");
            else lcdDisplay("Sound: On","make it loud");
            fadeout();
            break;
          }

          //case //freeform
          
          case 12://when click 1
          {
            
          }
          case 13://when click 2
          {
            //if(mode >= 1 && mode <= 3) mode = (mode * 10) + 2;
            
          }
          
          default:
          {
            break;
          }
        }
      }
    }
    irrecv.resume(); // Receive the next value
  }
  delay(200);
}


/*
switch(mode)
            {
              case 0:
              {
                lcdDisplay("Mode: Clock","show time");
                fadeout();
                break;
              }
              case 1:
              {
                lcdDisplay("Mode: Alarm","conduct on time");
                fadeout();
                break;
              }
              case 2:
              {
                lcdDisplay("Mode: Trigger","turn on/off dev.");
                fadeout();
                break;
              }
              case 3:
              {
                lcdDisplay("Mode: Setting","set your things");
                fadeout();
                break;
              }
            }

*/
