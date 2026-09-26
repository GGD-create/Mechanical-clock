#include <Adafruit_PWMServoDriver.h>      
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RESET -1  //屏幕没有RESET引脚设为-1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_PWMServoDriver pwmH = Adafruit_PWMServoDriver(0x40);    
Adafruit_PWMServoDriver pwmM = Adafruit_PWMServoDriver(0x41); //两块驱动板   
//GPIO4 D2 SDA
//GPIO5 D1 SCL
const char *ssid = "1234567";     // 替换为你的 WiFi 名称
const char *password = "1234567"; // 替换为你的 WiFi 密码

// NTP 配置
WiFiUDP ntpUDP;
// 使用NTP服务器pool.ntp.org，时区设置为 UTC+8
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000);//也可以用阿里云的国内服务器ntp.aliyun.com

int servoFrequency = 50;      //舵机频率

int segmentHOn[14] = {202,238,250,440,382,462,155,230,270,250,470,405,465,235};   //On小时

int segmentMOn[14] = {320,290,284,365,371,333,260,294,207,105,290,415,310,250};   //On分钟



int segmentHOff[14] = {435,448,480,200,160,230,365,460,480,480,220,190,230,450};    //Off小时

int segmentMOff[14] = {480,480,485,180,180,150,450,480,394,293,100,210,123,440};    //Off分钟

//每个舵机的角度都要慢慢一个一个调整，找到合适的位置，因为ESP8266下载程序比较慢，我建议先用arduino调整，后期再用ESP8266


int digits[10][7] = {{1,1,1,1,1,1,0},//0
                     {0,1,1,0,0,0,0},//1
                     {1,1,0,1,1,0,1},//2
                     {1,1,1,1,0,0,1},//3
                     {0,1,1,0,0,1,1},//4
                     {1,0,1,1,0,1,1},//5
                     {1,0,1,1,1,1,1},//6
                     {1,1,1,0,0,0,0},//7
                     {1,1,1,1,1,1,1},//8
                     {1,1,1,1,0,1,1}}; //9 
                     

int hourTens = 0;                 //创建变量来存储每个显示数字
int hourUnits = 0;
int minuteTens = 0;
int minuteUnits = 0;

int seconds = 0;

int prevHourTens = 8;           //创建变量以存储先前显示的数字
int prevHourUnits = 8;          
int prevMinuteTens = 8;
int prevMinuteUnits = 8;

int midOffset = 100;            //中间段相邻的左右两段所移动的距离

void setup() 
{ 
  Serial.begin(115200);

  Wire.begin(4, 5); //SDA, SCL

  if(!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { //0x3C对应I2C地址，一般的0.96寸OLED都是这个地址，我已用程序验证过
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); //卡死在这里
  }

  //oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0,0);
  oled.println("Hello >.<");
  oled.setCursor(0,18);
  oled.println("I'm a");
  oled.setCursor(0,36);
  oled.println("clock");
  
  oled.display();
  delay(3000);//仅展示开头动画的延时
  
  pwmH.begin();                             
  pwmM.begin();
  pwmH.setOscillatorFrequency(27000000);    
  pwmM.setOscillatorFrequency(27000000);
  pwmH.setPWMFreq(servoFrequency);          
  pwmM.setPWMFreq(servoFrequency);

  //显示WiFi连接状态
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("Connecting");

  oled.setCursor(0,25);
  oled.print(ssid);
  oled.display();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    oled.print(".");
    oled.display();
  }

  //WiFi连接成功显示
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.println("Connected!");
  oled.setCursor(0,25);
  //oled.print("IP: ");
  oled.println(WiFi.localIP());
  oled.display();
  delay(2500);

  timeClient.begin();//初始化 NTP 客户端
 
  initializeSegments();
  
  delay(1000);
}


void loop()
{
  
  timeClient.update();//更新 NTP 时间

  //更新时间显示到OLED
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    
    oled.clearDisplay();

    oled.setCursor(0,0);
    //oled.print("Time: ");
    oled.print(timeClient.getFormattedTime());

    oled.setCursor(0,25);
    //oled.print("IP: ");
    oled.println(WiFi.localIP().toString().substring(0, 13)); //显示IP

    oled.setCursor(0,50);
    //oled.print("Name: ");
    oled.println(ssid);//WiFi名称

    oled.display();
  }


  // 获取小时、分钟和秒
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  int temp = hour;             
  hourTens = temp / 10;               //把小时拆出个位和十位
  hourUnits = temp % 10;
  
  temp = minute;               
  minuteTens = temp / 10;             
  minuteUnits = temp % 10;

  
  
  
  if(minuteUnits != prevMinuteUnits)  //判断时间变化就舵机臂就发生变化
   updateDisplay(); 
                     
  
   savePreviousState();
  
  delay(500);
}

void savePreviousState() {
  
  prevHourTens = hourTens;            
  prevHourUnits = hourUnits;
  prevMinuteTens = minuteTens;
  prevMinuteUnits = minuteUnits;

}

void updateDisplay ()                               
{
  updateMid();    //先将与中间段相邻的区段移开，为中间段腾出空间，然后再移动中间段
  for (int i=0 ; i<=5 ; i++)      //移动其余的段
  {
    if(digits[hourTens][i]==1)                      //小时十位
      pwmH.setPWM(i+7, 0, segmentHOn[i+7]);
    else
      pwmH.setPWM(i+7, 0, segmentHOff[i+7]);
    delay(50);
    if(digits[hourUnits][i]==1)                     //小时个位
      pwmH.setPWM(i, 0, segmentHOn[i]);
    else
      pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(50);
    if(digits[minuteTens][i]==1)                    //分钟十位
      pwmM.setPWM(i+7, 0, segmentMOn[i+7]);
    else
      pwmM.setPWM(i+7, 0, segmentMOff[i+7]);
    delay(50);
    if(digits[minuteUnits][i]==1)                   //分钟个位
      pwmM.setPWM(i, 0, segmentMOn[i]);
    else
      pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(50);
  }
}

void updateMid()                                              
{
  if(digits[minuteUnits][6]!=digits[prevMinuteUnits][6])     //移动分钟单位的相邻段
  {
    if(digits[prevMinuteUnits][1]==1)
      pwmM.setPWM(1, 0, segmentMOn[1]+midOffset);
    if(digits[prevMinuteUnits][6]==1)
      pwmM.setPWM(5, 0, segmentMOn[5]-midOffset);
  }
  delay(150);                                //打开一定距离让中间段收起
  if(digits[minuteUnits][6]==1)                               
    pwmM.setPWM(6, 0, segmentMOn[6]);
  else
    pwmM.setPWM(6, 0, segmentMOff[6]);
  if(digits[minuteTens][6]!=digits[prevMinuteTens][6])       
  {
    if(digits[prevMinuteTens][1]==1)
      pwmM.setPWM(8, 0, segmentMOn[8]+midOffset);
    if(digits[prevMinuteTens][6]==1)
      pwmM.setPWM(12, 0, segmentMOn[12]-midOffset);
  }
  delay(150);                                                 
  if(digits[minuteTens][6]==1)                                
    pwmM.setPWM(13, 0, segmentMOn[13]);
  else
    pwmM.setPWM(13, 0, segmentMOff[13]);
  if(digits[hourUnits][6]!=digits[prevHourUnits][6])          
  {
    if(digits[prevHourUnits][1]==1)
      pwmH.setPWM(1, 0, segmentHOn[1]+midOffset);
    if(digits[prevHourUnits][6]==1)
      pwmH.setPWM(5, 0, segmentHOn[5]-midOffset);
  }
  delay(150);                                                 
  if(digits[hourUnits][6]==1)                                 
    pwmH.setPWM(6, 0, segmentHOn[6]);
  else
    pwmH.setPWM(6, 0, segmentHOff[6]);
  if(digits[hourTens][6]!=digits[prevHourTens][6])            
  {
    if(digits[prevHourTens][1]==1)
      pwmH.setPWM(8, 0, segmentHOn[8]+midOffset);
    if(digits[prevHourTens][6]==1)
      pwmH.setPWM(12, 0, segmentHOn[12]-midOffset);
  }
  delay(150);                                                 
  if(digits[hourTens][6]==1)                                  
    pwmH.setPWM(13, 0, segmentHOn[13]);
  else
    pwmH.setPWM(13, 0, segmentHOff[13]);
}

void initializeSegments()
{
  for(int i=0 ; i<=5 ; i++)
  {
    pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(15);
  }
  
  pwmM.setPWM(6, 0, segmentMOn[6]);
  delay(20);
  
  for(int i=7 ; i<=12 ; i++)
  {
    pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(15);
  }
  
  pwmM.setPWM(13, 0, segmentMOn[13]);
  delay(20);
  
  for(int i=0 ; i<=5 ; i++)
  {
    pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(15);
  }
  
  pwmH.setPWM(6, 0, segmentHOn[6]);
  delay(20);
  
  for(int i=7 ; i<=12 ; i++)
  {
    pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(15);
  }

  pwmH.setPWM(13, 0, segmentHOn[13]);
  delay(20);
   //把初始都是8改为初始时中间段ON,其余都OFF，这样不会打架

}