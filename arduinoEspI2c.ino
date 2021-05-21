#include<Wire.h>
#define SEND_MESSAGE_INTERVAL 50
String messageQueue[10];
int pushIndex = 0; 
int popIndex = 0;

String testMessage = "temp=24.55\nhum=55.21\ndefT=22.12\npump=1\n";
int forceSendMessage = 0;

long elapsedTime;
long lastTime;

volatile float temp=24.555;
volatile float hum=40.444;
volatile float tempLv1=20;
volatile float tempLv2=25;
volatile float tempLv3=30;
volatile float humidityThresh=40.2;
volatile String arduinoStatus;

void valueRandomizer()
{
  temp = 1.0 * random(1500,4000) / 100;
  hum = 1.0 * random(1500,4000) / 100;
  tempLv1 = 1.0 * random(1500,4000) / 100;
  tempLv2 = 1.0 * random(1500,4000) / 100;
  tempLv3 = 1.0 * random(1500,4000) / 100;
  humidityThresh = 1.0 * random(1500,4000) / 100;
}

void statusBuilder()
{
  String builder;
  builder = String(int(temp*100)) + "n" + String(int(hum*100)) + "n" + String(int(tempLv1*100)) + "n" + String(int(tempLv2*100)) + "n" + String(int(tempLv3*100)) + "n" + String(int(humidityThresh*100));
  arduinoStatus = builder;
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //########################TEST######
  statusBuilder();
  
  //###################################
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Wire.begin(1);
}

void receiveEvent(int bytes)
{
   String keyword;
   String msg;
   int sw = 0;
   char c;
   while(Wire.available())
   {
      c = (char) Wire.read();
      if( c == '=')
      {
        sw = 1;
      }
      else if(sw == 0)
      {
        keyword += c;
      }
      else
      {
        msg += c;
      }
   }
  if(msg.length() == 0)
  {
    Serial.println("Message from master: " + keyword);
  }
  else
  {
    Serial.println("Message from master: key - " + keyword + " msg - " + msg);
  }
  
  
}

void requestEvent(int bytes)
{
  Serial.println("Request event from master");
  char msg[50];
  
  arduinoStatus.toCharArray(msg,arduinoStatus.length()+1);
  Serial.println(msg);

  Wire.write(msg);
  
  statusBuilder();
  valueRandomizer();
  delay(10);
}

void loop() {
  // put your main code here, to run repeatedly:
  //Wire.write('a');
  //delay(200);
  elapsedTime = millis() - lastTime;
  lastTime = lastTime + elapsedTime;
}
