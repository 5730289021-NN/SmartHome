//Library
#include <DS1302.h>             //Realtime Clock
#include <IRremote.h>           //Remote Controller
#include <Wire.h>               //TWI for LiquidCrystal
#include <LiquidCrystal_I2C.h>  //LiquidCrystal

#include <TimerOne.h>           //Timer Interrupt

//Pin Define
#define relayPin    4
#define buzzerPin   5
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
int defaultFadingTime = 2000;
bool lcdDisplayEnb = true;
bool soundEnb = true;

bool numericInputAvailable = false;
bool numericKeyAvailable = false;
String numericKey = "0";

bool choiceInputAvailable = false;
bool choiceKeyAvailable = false;
int choiceKey = 0;

bool okInputAvailable = false;
bool okKeyAvailable = false;

String settingTime = "00:00:00";
String inputTime = "00:00:00";

bool alarmTimePassed = false;
bool alarmSet = false;
String alarmTime  = "00:00:00";
bool alarmTrigType = true;

int cursorLocation = 0;


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

void fadeout(int _time) {
  if(_time == 0) delay(defaultFadingTime);
  else delay(_time);
  lcd.clear();
}

void trig(bool state){
  digitalWrite(relayPin,state);
  if(soundEnb)
  {
    for(int i = 0;i<5;i++)
    {
      analogWrite(5,128);
      delay(1000);
      analogWrite(5,0);
      delay(1000);
    }
  }else
  {
    delay(5000);
  }
  fadeout(100);
}


//Timer Interrupt Function
void updateTime(){
  //Update current time 
  Time t = rtc.time();
  if(dayAsString(t.day).equals("Unknownday"))//In case of RTC hardware bug occured.
  {
    Second = Second + 1;
    if(Second.equals("60"))
    {
      Second = "00";
      Minute = String(Minute.toInt() + 1);
    }
    if(Minute.equals("60"))
    {
      Minute = "00";
      Hour = String(Hour.toInt() + 1);
    }
    if(Hour.equals("24"))
    {
      Hour = "00";
    }
    return;
  }
  else
  {
    Hour = t.hr;
    Minute = t.min;
    Second = t.sec;
    String _Day = dayAsString(t.day);
    if(_Day.length() >5) Day = _Day; 
    Date = t.date;
    Month = t.mon;
    Year = t.yr%100;
  }
  //Update Alarm
  if(alarmSet)
  {
    alarmTimePassed = true;
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
  if(alarmTimePassed)
  {
    alarmTimePassed = false;
    String _hour = String(alarmTime[0]) + String(alarmTime[1]);
    String _minute = String(alarmTime[3]) + String(alarmTime[4]);
    String _second = String(alarmTime[6]) + String(alarmTime[7]);
    unsigned long alarmTimeSecond = (_hour.toInt() * 3600) + (_minute.toInt() * 60) + _second.toInt();
    alarmTimeSecond--;
    _hour = String(alarmTimeSecond/3600);
    _minute = String((alarmTimeSecond%3600)/60);
    _second =  String(alarmTimeSecond%60);
    alarmTime = (_hour.length()==1?"0":"") + _hour + ":" + (_minute.length()==1?"0":"") + _minute + ":" + (_second.length()==1?"0":"") + _second;
    Serial.println("Alarm in: " + alarmTime);
    if(alarmTime.equals("00:00:00"))
    {
      alarmSet = false;
      lcd.clear();
      if(alarmTrigType) lcdDisplay("Time's up", "Triggering");
      else lcdDisplay("Time's up", "Untriggering");
      trig(alarmTrigType);
    }
  }
  
  switch(mode)
  {
    case 0://Show time
    {
      lcd.noCursor();
      if(getTime().equals("00:00:00")) lcd.clear();
      lcdDisplay(getTime(),getDate());
      Serial.println(getTime() + " " + getDate());
      break;
    }
    case 1://Alarm
    {
      if(alarmSet)
      {
        lcdDisplay("Alarm - " + alarmTime,"1.Clear Alarm");
        choiceInputAvailable = true;
        if(choiceKeyAvailable)
        {
          choiceInputAvailable = false;
          choiceKeyAvailable = false;
          if(choiceKey == 1)
          {
            alarmSet = false;
            lcd.clear();
          }
        }
      }else
      {
        lcd.cursor();
        lcdDisplay("Set Alarm ","    " + inputTime);
        numericInputAvailable = true;
        lcd.setCursor(cursorLocation + 4,1);
        if(numericKeyAvailable)
        {
          numericInputAvailable = false;
          numericKeyAvailable = false;
          inputTime[cursorLocation] = numericKey[0];
          cursorLocation++;
          if(cursorLocation == 2) cursorLocation++;//First ':'
          else{
            if(cursorLocation == 5 || cursorLocation == 8)//At Second ':' or after last number
            {
              String checker = String(inputTime[cursorLocation-2]) + String(inputTime[cursorLocation-1]);
              if(checker.toInt() >= 60)
              {
                inputTime[cursorLocation-2] = '5';
                inputTime[cursorLocation-1] = '9';
              }
              if(cursorLocation == 5) cursorLocation++;
              if(cursorLocation == 8) cursorLocation = 7;
            }
          }
        }
        okInputAvailable = true;
        if(okKeyAvailable)
        {
          okInputAvailable = false;
          okKeyAvailable = false;
          mode = 11;
        }
        
      }
      break;
    }
    case 11://Alarm After select time
    {
      lcd.noCursor();
      lcdDisplay("At that time do?","1.Trig 2.Untrig");
      choiceInputAvailable = true;
      numericInputAvailable = false;
      if(choiceKeyAvailable)
      {
        choiceInputAvailable = false;
        choiceKeyAvailable = false;
        switch(choiceKey)
        {
          case 1:
          {
            lcd.clear();
            lcdDisplay("Trig device","in " + inputTime);
            alarmTime = inputTime;
            alarmTrigType = true;
            fadeout(0);
            alarmSet = true;
            mode = 1;
            break;
          }
          case 2:
          {
            lcd.clear();
            lcdDisplay("Untrig device","in " + inputTime);
            alarmTime = inputTime;
            alarmTrigType = false;
            fadeout(0);
            alarmSet = true;
            mode = 1;
            break;
          }
          default:
          {
            lcdDisplay("Impossible","is possible");
            fadeout(0);
            break;
          }
        }
        
      }
      break;
    }
    case 2://Trigger
    {
      lcd.noCursor();
      lcdDisplay("Trigger 1.Trig","        2.Untrig");
      choiceInputAvailable = true;
      numericInputAvailable = false;
      if(choiceKeyAvailable)
      {
        choiceInputAvailable = false;
        choiceKeyAvailable = false;
        switch(choiceKey)
        {
          case 1:
          {
            lcd.clear();
            lcdDisplay("Manual: Trig","on the device");
            trig(true);
            break;
          }
          case 2:
          {
            
            lcd.clear();
            lcdDisplay("Manual: Untrig","off the device");
            trig(false);
            break;
          }
        }
      }
      break;
    }
    case 3://Setting
    {
      lcd.noCursor();
      lcdDisplay("Settings 1.Time","        ");
      choiceInputAvailable = true;
      numericInputAvailable = false;
      if(choiceKeyAvailable)
      {
        choiceInputAvailable = false;
        choiceKeyAvailable = false;
        switch(choiceKey)
        {
          case 1://Set t
          {
            mode = 31;
            settingTime = getTime();
            cursorLocation = 0;
            lcd.clear();
            break;
          }
          case 2://Set 
          {
            mode = 32;
            lcd.clear();
            break;
          }
        }
      }
      break;
    }
    case 31://Set Time
    {
      lcdDisplay("Set the clock","    " + settingTime);
      lcd.cursor();
      numericInputAvailable = true;
      lcd.setCursor(cursorLocation + 4,1);
      if(numericKeyAvailable)
      {
        numericInputAvailable = false;
        numericKeyAvailable = false;
        settingTime[cursorLocation] = numericKey[0];
        cursorLocation++;
        if(cursorLocation == 2) cursorLocation++;//First ':'
        else{
          if(cursorLocation == 5 || cursorLocation == 8)//At Second ':' or after last number
          {
            String checker = String(settingTime[cursorLocation-2]) + String(settingTime[cursorLocation-1]);
            if(checker.toInt() >= 60)
            {
              settingTime[cursorLocation-2] = '5';
              settingTime[cursorLocation-1] = '9';
            }
            if(cursorLocation == 5) cursorLocation++;
            if(cursorLocation == 8) cursorLocation = 7;
          }
        }
      }
      //Special case
      okInputAvailable = true;
      if(okKeyAvailable)
      {
        okInputAvailable = false;
        okKeyAvailable = false;
        lcd.noCursor();
        lcd.clear();
        lcdDisplay("Time is set", settingTime);
        String _hour = String(settingTime[0]) + String(settingTime[1]);
        String _minute = String(settingTime[3]) + String(settingTime[4]);
        String _second = String(settingTime[6]) + String(settingTime[7]);
        rtc.writeProtect(false);
        rtc.halt(false);
        Time _t = rtc.time();
        Time t(_t.yr, _t.mon, _t.date, _hour.toInt(), _minute.toInt(), _second.toInt(), _t.day);
        //Time t(2017, 4, 24, _hour.toInt(), _minute.toInt(), _second.toInt(), _t.day);
        rtc.time(t);
        rtc.writeProtect(true);
        mode = 0;
        fadeout(0);
      }
      break;
    }
    break;
  }
  
  if (irrecv.decode(&results)) {
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
            cursorLocation = 0;
            break;
          }
          case 2://Sound Enb Button
          {
            soundEnb = !soundEnb;
            lcd.clear();
            if(soundEnb) lcdDisplay("Sound: On","make it loud");
            else lcdDisplay("Sound: Off","keep silence");
            fadeout(0);
            break;
          }
          case 11://U/SD Button
          {
            lcd.clear();
            if(okInputAvailable)
            {
              okKeyAvailable = true;
            }
            break;
          }
          case 4://when click Back
          {
            if(numericInputAvailable)
            {
              //numericKey = remoteString[i];
              //numericKeyAvailable = true;
              cursorLocation--;
              switch(cursorLocation)
              {
                case -1:
                {
                  cursorLocation = 0;
                  break;
                }
                case 2:
                {
                  cursorLocation = 1;
                  break;
                }
                case 5:
                {
                  cursorLocation = 4;
                  break;
                }
              }
            }
            else Serial.println("Numeric unaccepted: " + remoteString[i]);
            break;
          }
          case 5://when click Next
          {
            if(numericInputAvailable)
            {
              cursorLocation++;
              switch(cursorLocation)
              {
                case 2:
                {
                  cursorLocation = 3;
                  break;
                }
                case 5:
                {
                  cursorLocation = 6;
                  break;
                }
                case 8:
                {
                  cursorLocation = 7;
                  break;
                }
              }
            }
            else Serial.println("Numeric unaccepted: " + remoteString[i]);
            break;
            
          }
          
          case 12://when click 1
          case 13://when click 2
          {
            if(choiceInputAvailable) 
            {
              choiceKey = remoteString[i].toInt();
              choiceKeyAvailable = true;
            }
            else Serial.println("Choice unaccepted: " + remoteString[i]);
          }
          case  9://when click 0
          case 14://when click 3
          case 15://when click 4
          case 16://when click 5
          case 17://when click 6
          case 18://when click 7
          case 19://when click 8
          case 20://when click 9
          {
            if(numericInputAvailable)
            {
              numericKey = remoteString[i];
              numericKeyAvailable = true;
            }
            else Serial.println("Numeric unaccepted: " + remoteString[i]);
            break;
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