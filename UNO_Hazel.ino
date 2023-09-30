#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 
#include <RTClib.h>
#include <Encoder.h>
#include <DHT.h>
#include <math.h>
#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"

#define encoder        2
#define SW             4
#define Pep            8
#define Bas            6
#define Lem            7
#define Ros            9
#define Lav            5
#define Neopixel       10
#define DHTPIN         A3
#define DHTTYPE DHT11
#define LEDNUM 16

DFRobotDFPlayerMini myDFPlayer;

SoftwareSerial MP3(11,12);
SoftwareSerial mini_Serial (A1, A2);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDNUM, Neopixel, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x27, 16, 2);

RTC_DS3231 rtc;
Encoder myEnc (2,3);

DHT dht(DHTPIN, DHTTYPE);

long oldPosition  = -999;
int prev_dht = -1;
float ts;
int ir = -1;
int rfidscore;

int nowNum;
int preNum;
int number;
int preSecond;
int preHour;
int preMinute;

int mornM = 00;
int mornH = 9;
int preMornM;

int nightM = 00;
int nightH = 22;
int preNightM;

char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int distance = 0;
unsigned timer;
unsigned timeraroma;
unsigned long sense_time;
int last_sensed_Day;

int encoder_rotation;
int pre_encoder_rotation;

float TODAY_SCORE;

int fading_step = 5;
int fading_speed = 10;
int fading_flag = false;
int fading_bright = 0;
int fading_direction = true;
unsigned long fadding_time;

String recv_data;

bool flag = false;
bool flag0 = false;
bool flag1 = false;
bool flag2 = false;
bool flag3 = false;
bool flag4 = false;
bool flag5 = false;
boolean on_flag;
bool button = false;
boolean sound_flag;
int prev_distance;
int now_day;
boolean alarm = false;
int operation_count;
unsigned long tt;


int todayScore(){
  ts *= DHTScore();
  int ts1 = round(ts);
  return ts1;
}

void remote(){
  if(alarm == false && ir != -1){
    aroma(ir);
    delay(20000);
    ALLOFFNEO();
    ir = -1;
  }
}

void NEO(bool state, int R, int G, int B, int bright){
  if(state == true){
    for(int i = 0; i < 16; i++){
      strip.setPixelColor(i, R, G, B);
      strip.setBrightness(bright);
      strip.show();
    }
  }
}

void NEOFading(int tmp_red, int tmp_green, int tmp_blue){
  if(fading_flag == false){
    fadding_time = millis();
    fading_flag = true;
  }
  if(fading_flag == true && millis() - fadding_time > fading_speed){
    fadding_time = millis();
    if(fading_bright < 255 && fading_direction == true){
      fading_bright += fading_step;
    }
    if(fading_bright >= 255 && fading_direction == true){
      fading_direction = false;
    }
    if(fading_bright > 0 && fading_direction == false){
      fading_bright -= fading_step;
    }
    if(fading_bright <= 0 && fading_direction == false){
      fading_direction = true;
    }
  }
  NEO(1,tmp_red, tmp_green, tmp_blue, fading_bright);
  Serial.println(fading_bright);
}

void aroma(int score){
  Serial.print("Today's aroma: ");
  if(score == 1){
    NEO(true, 0, 255, 80, 255);
    digitalWrite(Pep, HIGH);
    Serial.println("Peppermint");
  }
  else if(score == 2){
    NEO(true, 0, 255, 0, 255);
    digitalWrite(Bas, HIGH);
    Serial.println("Basil");
  }
  else if(score == 3){
    NEO(true, 255, 255, 0, 255);
    digitalWrite(Lem, HIGH);
    Serial.println("Lemon");
  }
  else if(score == 4){
    NEO(true, 0, 50, 255, 255);
    digitalWrite(Ros, HIGH);
    Serial.println("Rosemary");
  }
  else if(score == 5){
    NEO(true, 155, 0, 200, 255);
    digitalWrite(Lav, HIGH);
    Serial.println("Lavender");
  }
}

float DHTScore(){
  float h = dht.readHumidity() * 0.01;
  float t = dht.readTemperature();

  float score = (0.81 * t) + (0.99 * h * t) - (14.3 * h) + 46.3;

  if(score >= 80){
    return 0.9;
  }
  else if(score >= 75 && score < 80){
    return 0.8;
  }
  else {
    return 1;
  }
}

bool detectIR(){
  int volt = map(analogRead(A0), 0, 1023, 0, 5000);
  distance = (27.61 / (volt - 0.1696)) * 1000;
  if(distance <= 30 && millis() - sense_time > 350){
    return true;
  }
  else if(distance > 30){
    sense_time = millis();
    return false;
  }
}

void setRTC(){
  DateTime now = rtc.now();
  if(digitalRead(SW) == LOW && flag3 == false){
    number++;
    lcd.clear();
    flag3 = true;
  }
  if(digitalRead(SW) == HIGH && flag3 == true){
    flag3 = false;
  }

  nowNum = number % 3;
  if(nowNum != preNum){
    preNum = nowNum;
    lcd.clear();
  }

  if(nowNum == 0){
    if(now.second() != preSecond){
      preSecond = now.second();
      lcd.clear();
    }
    if(now.hour() != preHour){
      preHour = now.hour();
      lcd.clear();
    }
    if(now.minute() != preMinute){
      preMinute = now.minute();
      lcd.clear();
    }
    lcd.setCursor(0,0);
    lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
    lcd.print(" ");
    char tmp_time[100];
    sprintf(tmp_time, "%02d/%02d/%02d", now.year(), now.month(), now.day());
    lcd.print(tmp_time);
    
    lcd.setCursor(4,1);
    char tmp_time1[100];
    sprintf(tmp_time1, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print(tmp_time1);  
  }
  else if(nowNum == 1){
    encoder_rotation = myEnc.read();
    if(pre_encoder_rotation != encoder_rotation){
      if( pre_encoder_rotation > encoder_rotation){
        mornM += 5;
        if(mornM >= 60){
          mornM = 0;
          mornH = (mornH+1) % 24;
        }
      }
      else{
        mornM -= 5;
        if(mornM <= 0){
          mornM = 55;
          if(mornM > 0)
            mornH --;
            if(mornH < 0){
              mornH = 23;
            }
        }
      }
      pre_encoder_rotation = encoder_rotation;
    }
    
    if(mornM != preMornM){
      preMornM = mornM;
      lcd.clear();
    }
    
    lcd.setCursor(0,0);
    lcd.print("Morning Set");
    lcd.setCursor(5,1);
    char tmp_time[100];
    sprintf(tmp_time, "%02d:%02d:00", mornH, mornM);
    lcd.print(tmp_time);
  }

  else if(nowNum == 2){
    encoder_rotation = myEnc.read();
    if(pre_encoder_rotation != encoder_rotation){
      if( pre_encoder_rotation > encoder_rotation){
        nightM += 5;
        if(nightM >= 60){
          nightM = 0;
          nightH = (nightH+1) % 24;
        }
      }
      else{
        nightM -= 5;
        if(nightM <= 0){
          nightM = 55;
          if(nightM > 0)
            nightH --;
            if(nightH < 0){
              nightH = 23;
            }
        }
      }
      pre_encoder_rotation = encoder_rotation;
    }
    
    if(nightM != preNightM){
      preNightM = nightM;
      lcd.clear();
    }

    lcd.setCursor(0,0);
    lcd.print("Bed Time Set");
    lcd.setCursor(5,1);
    char tmp_time[100];
    sprintf(tmp_time, "%02d:%02d:00", nightH, nightM);
    lcd.print(tmp_time);
  }

  if(now.hour() == mornH && now.minute() == mornM && flag4 == false && nowNum == 0){
    Serial.println("Peppermint");
    Serial.println(mornH);
    Serial.println(mornM);
    PLAY(2,30);
    digitalWrite(Pep, HIGH);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("GOOD MORNING!");
    lcd.setCursor(0,1);
    lcd.print("TIME TO WAKE UP!");
    for(int tmp_count = 0; tmp_count < 2; tmp_count++){
      for(int i = 0; i < 255; i++){
        NEO(1, 255, 100, 20, i);
        delay(600);
      }
    }
    lcd.clear();
    ALLOFFNEO();
    flag4 = true;
  }
  if(now.minute() != mornM){
    flag4 = false;
  }

  if(now.hour() == nightH && now.minute() == nightM && flag5 == false && nowNum == 0){
    Serial.println("Lavender");
    Serial.println(nightH);
    Serial.println(mornM);
    PLAY(1,15); 
    digitalWrite(Lav, HIGH);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("GOOD NIGHT!");
    lcd.setCursor(0,1);
    lcd.print("TIME TO SLEEP!");
    for(int tmp_count = 0; tmp_count < 2; tmp_count++){
      for(int i = 255; i > 0; i--){
        NEO(1, 255, 100, 20, i);
        delay(600);
      }
    }
    lcd.clear();
    ALLOFFNEO();
    flag5 = true;
  }
  if(now.minute() != nightM){
    flag5 = false;
  }
  
  int tmp_M, tmp_N, tmp_now;
  tmp_N = nightM + (nightH*100);
  tmp_M = mornM + (mornH*100);
  tmp_now = now.minute() + (now.hour()*100);

  if(tmp_M < tmp_now && tmp_now < tmp_N){
    if(detectIR() == true && todayScore() != 0){
      alarm = true;
      Serial.println("Sensed");
      if(sound_flag == false){
        sound_flag = true;
        PLAY(3,25);
        Serial.print("Welcome Back!");
      }
    }
    
    if(alarm == true){
      TODAY_SCORE = todayScore();
      Serial.println("ON"); 
      aroma(TODAY_SCORE);
      for(int tmp_on =0; tmp_on < 3600; tmp_on++){
        DateTime now = rtc.now();
        lcd.setCursor(0,0);
        lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
        lcd.print(" ");
      
        char tmp_time[100];
        sprintf(tmp_time, "%02d/%02d/%02d", now.year(), now.month(), now.day());
        lcd.print(tmp_time);
          
        lcd.setCursor(4,1);
        char tmp_time1[100];
        sprintf(tmp_time1, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        lcd.print(tmp_time1);  

        delay(500);          
      }
      Serial.println( "OFF"); 
      ALLOFFNEO();
      alarm = false;
    }
  }
  if(abs(distance - prev_distance) <= 5){
    timer = millis();
    prev_distance = distance;
    last_sensed_Day = now.day();
    flag0 = false;
  }
  if(now.day() - last_sensed_Day >=3 && flag0 == false){
    flag0 = true;
    mini_Serial.println("$ SOS $");
  }

  if(now.day() != now_day){
    now_day = now.day();
    sound_flag = false;
    Serial.println("RESET FLAG");
  }
}

void PLAY(int index_music, int volume){
  MP3.begin(9600);
  myDFPlayer.begin(MP3);
  myDFPlayer.volume(volume);
  myDFPlayer.play(index_music);
}

void dataParsingfromESP(){
  mini_Serial.listen();
    if(mini_Serial.available()){
      char tmp_dataesp = mini_Serial.read();
      if( tmp_dataesp != '#'){
        recv_data += tmp_dataesp;
      }
      else{
        Serial.println(recv_data);
        if(recv_data.indexOf("@") != -1){
          int tmp_index  = recv_data.indexOf(",");
          int tmp_index2 = recv_data.indexOf(",",tmp_index + 1);
          String extract = recv_data.substring(tmp_index+1, tmp_index2);
          Serial.println(extract);
          ts = extract.toFloat();
          Serial.print(" Score from esp 8266 : ");
          Serial.print(ts);
          Serial.println("*" + String(DHTScore()) + " = " + String(todayScore()));
        }
        
        if( recv_data.indexOf("$") != -1){
          Serial.println("IR REMOTE");
          int tmp_index = recv_data.indexOf(",");
          int tmp_index2 = recv_data.indexOf(",", tmp_index + 1);
          String extract = recv_data.substring(tmp_index+1, tmp_index2);
          Serial.println(extract);
          ir = extract.toInt();
        }
        recv_data ="";
      }
    }
}

void ALLOFFNEO(){
  on_flag = false;
  for(int i = 0; i < 16; i++){
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
  digitalWrite(Lav, LOW);
  digitalWrite(Ros, LOW);
  digitalWrite(Lem, LOW);
  digitalWrite(Bas, LOW);
  digitalWrite(Pep, LOW);
}

void setup(){
  Serial.begin(9600);
  mini_Serial.begin(9600);
  strip.begin();
  lcd.begin();
  pinMode(Pep, OUTPUT);
  pinMode(Bas, OUTPUT);
  pinMode(Lem, OUTPUT);
  pinMode(Ros, OUTPUT);
  pinMode(Lav, OUTPUT);
  pinMode(SW, INPUT_PULLUP);
  ALLOFFNEO();

  delay(1000);
  DateTime now = rtc.now();
  last_sensed_Day = now.day();
  
  Serial.println("System Start");
}

void loop() {
  dht.begin();
  setRTC();
  dataParsingfromESP();
  remote();
}
