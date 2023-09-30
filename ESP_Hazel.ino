#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define BLYNK_PRINT Serial
#define VARID      ""
#define RST_PIN         2
#define SS_PIN          15

const uint16_t kRecvPin = 0;
IRrecv irrecv(kRecvPin);
decode_results results;
int prev_ir_data;

MFRC522 mfrc522(SS_PIN, RST_PIN);
SoftwareSerial mini_Serial(5,4);

int getInt(String input);

char auth[] = "";
char ssid[] = "";
char password[] = "";
int status = WL_IDLE_STATUS;

char server[] = "api.openweathermap.org";
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 1000L;

boolean readingVal;
boolean getIsConnected = false;
String recv_data;

int val, temp;
float tempVal;

String writeName;
String writeAddress;
String writeEmail;
int writeAge;

String rcvbuf;

WiFiClient client;
void httpRequest();
void printWifiStatus();

String dump_byte_array(byte *buffer, byte bufferSize) {
  String tmp_uid = "";
  for (byte i = 0; i < bufferSize; i++) {
    tmp_uid += 
    String(buffer[i] < 0x10 ? " 0" : " ")+
    String(buffer[i], HEX);
  }
  return tmp_uid;
}
String Card_UID = "";
int prev_rfid = -1;

BLYNK_WRITE(V0)
{
  writeName = param.asStr();
  Serial.print("Name: ");
  Serial.println(writeName);
}

BLYNK_WRITE(V1)
{
  writeAddress = param.asStr();
  Serial.print("Address: ");
  Serial.println(writeAddress);
}

BLYNK_WRITE(V2)
{
  writeAge = param.asInt();
  Serial.println(writeAge);
}

BLYNK_WRITE(V3)
{
  writeEmail = param.asStr();
  Serial.print("Emergency Email: ");
  Serial.println(writeEmail);
}


int dailyScore(int weather_Score, int RFID_Score){
  Serial.print("(Mood Score: " + String(RFID_Score) + ")*0.7 + (Weather Score: " + String(weather_Score) + ")*0.3 = ");
  float DS = RFID_Score * 0.7 + weather_Score * 0.3;
  Serial.println(DS);
  int DS1 = round(DS);
  return DS1;
}

int RFIDScore(){
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(10);
    String cardId = String(dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size));
    Serial.print("Mood Score: ");
    if(cardId.indexOf("22 8a 44 34") != -1){
      Serial.println("Very Sad = 1");
      return 1;
    }
    else if(cardId.indexOf("c6 fc 64 2b") != -1){
      Serial.println("Sad = 2");
      return 2;
    }
    else if(cardId.indexOf("c0 2f 32 32") != -1){
      Serial.println("Normal = 3");
      return 3;
    }
    else if(cardId.indexOf("24 27 7d 2b") != -1){
      Serial.println("Happy = 4");
      return 4;
    }
    else if(cardId.indexOf("a1 5c 20 1c") != -1){
      Serial.println("Very Happy = 5");
      return 5;
    }
    else{
      Serial.print("ERR Code");
      return -1; 
    }
  }
  else
    return -1;
}

int getWeatherScore(String weather){
  Serial.print("Today's Weather: ");
  if(weather.indexOf("Clear")){
    Serial.println("Clear = 5");
    return 5;
  }
  else if(weather.indexOf("Clouds")){
    return 4;
    Serial.println("Clouds = 4");
  }
  else if(weather.indexOf("Drizzle")){
    Serial.println("Drizzle = 3");
    return 3;
  }
  else if(weather.indexOf("Rain")){
    Serial.println("Rain = 2");
    return 2;
  }
  else if(weather.indexOf("Thunderstorm")){
    Serial.println("Thunderstorm = 1");
    return 1;
  }
  else{
    Serial.println("ERR Weather");
    return -1;
  }
}

int weatherScore(){
  httpRequest();
  unsigned long rev_time = millis();
  while(millis() - rev_time < 5000){
    if(client.available()) {
      char c = client.read();
      recv_data += c;
    }
  }
  int tmp1 = recv_data.indexOf("main");
  int tmp2 = recv_data.indexOf(",", tmp1+1);

  String tmp_str_1 = recv_data.substring(tmp1+6, tmp2);

  int weatherScore = getWeatherScore(tmp_str_1);

  return weatherScore;
}

void httpRequest() {
  client.stop();
  if (client.connect(server, 80)) {
    Serial.println("Connecting...");

    client.print("GET /data/2.5/weather?q=06070,us&appid=");
    client.print(VARID);
    client.println(" HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();

    lastConnectionTime = millis();
    getIsConnected = true;
  }
  else {
    Serial.println("Connection failed");
    getIsConnected = false;
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

int getInt(String input) {
  char carray[20];
  input.toCharArray(carray, sizeof(carray));
  temp = atoi(carray);
  return temp;
}

void serialParsing(){
    if(mini_Serial.available()){
      char data = mini_Serial.read();
      if( data == '$'){
        Serial.println("Notification");
        char tmp_writeEmail[100];
        writeEmail.toCharArray(tmp_writeEmail, writeEmail.length());
        Blynk.email(tmp_writeEmail, "Alert: User not detected!", "Name: " + String(writeName) + " Age: " + String(writeAge) + " has not been detected for 72 hours! Location: " 
        + String(writeAddress));
    }
  }
}

int IR_Recv(){
  if (irrecv.decode(&results)) {
    serialPrintUint64(results.value, HEX);
    uint64_t ir_data = results.value;
    irrecv.resume();
    if( ir_data == 0xFF30CF){
      return 5;
    }
    else if(ir_data == 0xFF18E7){
      return 4;
    }
    else if(ir_data == 0xFF7A85){
      return 3;
    }
    else if(ir_data == 0xFF10EF){
      return 2;
    }
    else if(ir_data ==  0xFF38C7){
      return 1;
    }
  }
  else{
    return -1;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("START1");
  mini_Serial.begin(9600);

  irrecv.enableIRIn();
  
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("START2");

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Blynk.begin(auth, ssid, password);
  Serial.println("START3");

  
}

void loop(){
  Blynk.run();
  BLYNK_WRITE(V0);
  BLYNK_WRITE(V1);
  BLYNK_WRITE(V2);
  BLYNK_WRITE(V3);
  serialParsing();

  int tmp_rfid = RFIDScore();
  if( tmp_rfid != prev_rfid && tmp_rfid != -1 ){
    if(abs(prev_rfid - tmp_rfid) >= 4 && prev_rfid != -1){
      Blynk.notify("Name: " + String(writeName) + " has been detected with potential mood swing!");
    }
    String send_data = "";
    send_data += "@,";
    send_data += dailyScore(weatherScore(), tmp_rfid);
    send_data += ",#";
    mini_Serial.println(send_data);
    Serial.print("Send Daily Score data to Arduino : ");
    Serial.println(send_data);
    Serial.println();
    prev_rfid = tmp_rfid;
  }

  int now_ir_data = IR_Recv();
  if( prev_ir_data != now_ir_data && now_ir_data != -1 && now_ir_data < 6){
    prev_ir_data = now_ir_data;
    String send_data = "";
    send_data += "$,";
    send_data += now_ir_data;
    send_data += ",#";
    mini_Serial.println(send_data);
    Serial.println(now_ir_data);
    Serial.print("Send IR data to Arduino : ");
    Serial.println(send_data);
    Serial.println();
  }
}