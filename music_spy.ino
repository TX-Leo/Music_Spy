/*
1.oled随机显示频谱
2.dfplayer播放音乐
3.sd1820录音并播放
*/
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "U8glib.h"
/*设置管脚*/
int Rx=A0;
int Ry=A1;
int REC =3 ;             
int PL =4 ;  
int led_green=6;
int led_blue=7;
int interrupt_pin=2;
/*初始化类对象*/
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE);
/*定义全局变量*/
int flag=0;
int next=0;
int temp=1;
int randomArray[16] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};
const uint8_t rook_bitmap[4] PROGMEM = {0x7f, 0x7f, 0x7f, 0x7f};
/*定义函数*/
void draw(void);
void draw_fft(void);
void printDetail(uint8_t type, int value);
void check_dfplayer();
int rocker_next_song();
int rocker_next_mode();
void trial();
void mode1_play(void);
void mode2_record(void);
/*画图:随机设计音乐频谱*/
void draw(void) {
  // graphic commands to redraw the complete screen should be placed here
  if(random(2)) {
    randomArray[0] += random(2);
    if(randomArray[0] > 6) randomArray[0] = 6;
  }
  if(!random(2)) {
    randomArray[0] -= random(2);
    if(randomArray[0] < 0) randomArray[0] = 0;
  }
  if(random(2)) {
    randomArray[15] += random(2);
    if(randomArray[15] > 6) randomArray[15] = 6;
  }
  if(!random(2)) {
    randomArray[15] -= random(2);
    if(randomArray[15] < 0) randomArray[15] = 0;
  }
  for(int i = 1; i < 15; i++) {
    if(random(2)) {
      randomArray[i] += random(3);
      if(randomArray[i] > 8) randomArray[i] = 8;
    }
    if(!random(2)) {
      randomArray[i] -= random(3);
      if(randomArray[i] < 2) randomArray[i] = 2;
    }
    for(int j = 0; j < randomArray[i]; j++) {
      u8g.drawBitmapP(8 * i, 28 - 4 * j, 1, 4, rook_bitmap);
    }
  }
}
void draw_fft(){
  Serial.print("flag:");
  Serial.println(flag);
  Serial.print("Rx:");
  Serial.println(analogRead(Rx));
  Serial.print("Ry:");
  Serial.println(analogRead(Ry));
  Serial.println("update");
        u8g.firstPage();  
    do {
      draw();
    } while(u8g.nextPage());
  //}
}
/*测试dfplayer和sdcard的性能*/
void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  } 
}
void check_dfplayer(void){
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  //if (myDFPlayer.available()) {
  //  printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  //}
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true){
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));  
}
/*摇杆控制切换下一首歌曲*/
int rocker_next_song(){
  int x=analogRead(Rx);
  if(x>1000){
    next=1;
  }
  return next;
}
/*摇杆控制模式改变*/
int rocker_next_mode(){
  int y=analogRead(Ry);
  if(y>1000){
    flag=(flag+1)%3;
  }
  return flag;
}
/*设置录音模式的中断*/
void trial(){
  temp=0;
  flag=0;
}
/*模式1：播放音乐，摇杆行为换一首歌曲*/
void mode1_play(void){
  draw_fft();
  /*开始播放歌曲和切换下一首歌曲*/
  if(rocker_next_song()==1){
    Serial.println(F("Playing Next one"));
    myDFPlayer.volume(30);
    myDFPlayer.next();  //Play next mp3 every 3 second.
    next=0;
    delay(100);//为了使摇杆行为不那么敏感
  }
}
/*模式2：录音，点击REC按键可以录10秒的声音，然后点击PLAYL重复播放所录声音*/
void mode2_record(void){ 
  /*开始录音10s*/
  digitalWrite(REC, LOW);  
  digitalWrite(PL, LOW); 
  digitalWrite(REC, HIGH);   
  delay(10000);  
  temp=1;
  /*开始无限播放（通过中断跳出）*/
  while(temp==1){  
    Serial.println(temp);
    digitalWrite(REC, LOW);   
    digitalWrite(PL, HIGH);    
    delay(10000);
    digitalWrite(PL, LOW);
  }
}
void setup(){
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  pinMode(REC, OUTPUT);    
  pinMode(PL, OUTPUT); 
  pinMode(led_blue,OUTPUT);
  pinMode(led_green,OUTPUT);
  pinMode(interrupt_pin,INPUT_PULLUP);//模式2的中断
  digitalWrite(led_blue,LOW);//模式1LED显示
  digitalWrite(led_green,LOW);//模式2LED显示
  randomSeed(analogRead(5));
  check_dfplayer();//检测DF Player是否正常
  draw_fft();
  attachInterrupt(0,trial,LOW);
}
void loop(){
  digitalWrite(led_green,LOW);
  digitalWrite(led_blue,LOW);
  /*模式1*/
  if(rocker_next_mode()==1){
    Serial.println("=====mode  1====");
    digitalWrite(REC, LOW);  
    digitalWrite(PL, LOW);//关闭模式2的影响
    digitalWrite(led_blue,HIGH);
    digitalWrite(led_green,LOW);
    mode1_play();
    delay(200);//为了使摇杆行为不那么敏感
  }
  /*模式2*/
  if(rocker_next_mode()==2){
    Serial.println("=====mode  2====");
    myDFPlayer.pause();//关闭模式1的影响
    digitalWrite(led_green,HIGH);
    digitalWrite(led_blue,LOW);
    mode2_record();
    delay(200);//为了使摇杆行为不那么敏感
  }
}
