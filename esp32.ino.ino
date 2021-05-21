#include <WiFi.h>

#include<Wire.h>
#include <SimpleTimer.h>
const char* ssid = "SSID";
const char* password = "PASSWORD";

SimpleTimer timer;
int timerId;

WiFiServer wifiServer(11000);
WiFiClient client = NULL;
int slaveRequestLock = 0;
int connectedFlag = 0;

long lastRequestTime;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
  
  wifiServer.begin();

  Wire.begin();
  slaveRequestLock = 0;
  timerId = timer.setInterval(5000,requestFromArduino);
}

void loop() {
  // put your main code here, to run repeatedly:
  char testArr[10]="abcd";
  tcpClientCommunication();
  resetI2CLock();
  timer.run();
  
  /*WiFiClient client = wifiServer.available();
  if (client) {
    client.setTimeout(1);
    Serial.println("Client connected");
    while (client.connected()) 
    {
 
      while (client.available()>0) 
      {
        String msg = client.readStringUntil('\r');
        //client.print(c);
        Serial.println("Message from client: " + msg);
      }
 
      delay(10);
    }
 
    client.stop();
    Serial.println("Client disconnected");
 
  }*/
}

void tcpClientCommunication()
{
   if(connectedFlag == 0 && !client)
  {
    client = wifiServer.available();
    if(client.connected())
    {
      Serial.println("Client connected");
      connectedFlag = 1;
      client.setTimeout(1);
    }
    
  }
  else
  {
    if(connectedFlag == 1 && client.connected())
    {
      
      if(client.available() > 0)
      {
        String msg = client.readStringUntil('\r');
        if(msg.length() > 0)
        {
          char msgAsChrArr[15];
          msg.toCharArray(msgAsChrArr,msg.length()+1);
          sendToArduino(msgAsChrArr);
          Serial.print("Converted msg: ");
          Serial.println(msgAsChrArr);
          Serial.println("Message from client: " + msg);
        }
       
      }
      // Send routine here
    }
    else
    {
      connectedFlag = 0;
      client.stop();
    }
  }
}

void requestFromArduino()
{
  if(!slaveRequestLock)
  {
    Wire.requestFrom(1,32);
    slaveRequestLock = 1;
    lastRequestTime = millis();
  }
  
  if(Wire.available())
  {
    /*String ret = Wire.readString();*/
    String msg;
    while(Wire.available())
    {
      char c = (char) Wire.read();
      if(c < 255) // if c is 255, it means there was not enoguh data to complete the fully 32 bytes, and it should be considered junk
        msg += c;
      //Serial.print(int(c));
      //Serial.print(' ');
    }
    //msg+='\0';
    Serial.println("Received from I2C slave: " + msg);
    if(connectedFlag && client.connected())
    {
      Serial.println("Sending message to web client");
      client.print(msg);
    }
    slaveRequestLock = 0;
  }
}

void sendToArduino(char msg[15])
{
  
  //msg[14]='\r';
  Wire.beginTransmission(1);
  Wire.write(msg);
  Wire.endTransmission();

  Serial.print("Sending message to arduino: ");
  Serial.println(msg);
 
}

void resetI2CLock()
{
  if(millis() - lastRequestTime > 15000)
    slaveRequestLock = 0;
}
