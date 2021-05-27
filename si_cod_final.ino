#include <OneWire.h>
#include <DallasTemperature.h>
#include <Time.h>
#include<Wire.h>

// I2C define
#define SEND_MESSAGE_INTERVAL 50

#define FAN 11
#define PUMP 13
#define HIGRO A0
#define TEMPERATURE 7

//limite de temperatura pe 3 trepte

//sub HighTempLimit1 ventilatorul nu va functiona
float HighTempLimit1 = 26;
//intre HighTempLimit1 si HighTempLimit2 ventilatorul va functiona la o viteza putin redusa pentru a eficientiza consumul    
float HighTempLimit2 = 26.5;
//intre HighTempLimit2 si HighTempLimit3 ventilatorul va functiona la o viteza medie
float HighTempLimit3 = 27;
//peste HighTempLimit3 ventilatorul va functiona la viteza maxima

//pentru valori mai mici, pompa nu functioneaza
float HumidLimit = 500;
//pentru valori mai mari, pompa incepe sa functioneze

String arduinoStatus;

OneWire oneWire(TEMPERATURE);
DallasTemperature temp(&oneWire);

/*void temperatureSensor()
{
   temp.requestTemperatures();//
   Serial.println(temp.getTempCByIndex(0));
}*/
//----------------------Functia de pornire a ventilatorului in functie de temperatura----------------------

void fanStarter(float x)//x este valoarea temperaturii
{
  if(x<HighTempLimit1)
  {
    analogWrite(FAN,0); //Fan-ul se va opri daca se va ajunge la o temperatura sub limita
  }
  if((x>=HighTempLimit1)&&(x<HighTempLimit2))
  {
    analogWrite(FAN,150); //viteza redusa
  }
  if((x>=HighTempLimit2)&&(x<HighTempLimit3))
  {
    analogWrite(FAN,200);  //viteza medie
  }
  if(x>=HighTempLimit3)
  {
    analogWrite(FAN,250);   //viteza maxima
  }
}

//---------------------------------------------------------------------------------------------------------

//---------------------------Functia de pornire a pompei in functie de umiditate---------------------------

void pumpStarter(float HumidValue)//HumidValue este o valoare citita de pe senzorul de umiditate
{
  if(HumidValue<HumidLimit)
  {
    digitalWrite(PUMP,LOW);//pompa se opreste daca e prea umed solul
  }
  else
  if(HumidValue>=HumidLimit)
  {
    digitalWrite(PUMP,HIGH);//pompa porneste daca e prea uscat solul
  }
}

/*
 *******************************************INFO*******************************************
  astfel propun folosirea urmatoarei conventii: sub 500 -> ud bine, peste 600 -> uscat bine,
  iar valorile dintre pot fi mapate dupa preferinte.
 *******************************************INFO*******************************************
*/

//---------------------------------------------------------------------------------------------------------


//-------------------------------Variabile globale-------------------------------
int timer=0;
int HumidValue;
float TempValue;
int powerPump=0;//variabila care decide daca merge pompa manual. 0- merge automat  1- merge manual prin wifi
int pumpTime=5;//cate secunde sa lucreze manual pompa de apa
int timeStartPump=0;// un contor pentru controlul manual al pompei
int powerFan=0;//variabila care decide daca merge ventilatorul manual. 0- merge automat  1- merge manual prin wifi
int fanTime=5;//cate secunde sa lucreze manual ventilatorul
int timeStartFan=0;// un contor pentru controlul manual al ventilatorului
int vitezaFan=2;//viteza cu care sa mearga ventilatorul in modul manual
int modFunctionare=0;// 0 - automat    1 - manuel
//1-viteza mica    2-viteza medie   3-viteza mare
//-------------------------------Variabile globale-------------------------------

//-------------------------------------------------------------------------------

void statusBuilder(float temp, int hum, float tempLv1, float tempLv2, float tempLv3, float humidityThresh)
{
  String builder;
  float hum_percent;
  
  if(hum <= 150){
     hum_percent = 99.99;
  }
  
  else if(hum > 650){
          hum_percent = 0;
        }
        else
        {
          hum_percent = 100 - (hum-150)/5.;
        }
   float hum_thresh_percent  = 100 - (humidityThresh -150)/5.;
  
  builder = String(int(temp*100)) + "n" + String(int(hum_percent*100)) + "n" + String(int(tempLv1*100)) + "n" + String(int(tempLv2*100)) + "n" + String(int(tempLv3*100)) + "n" + String(int(hum_thresh_percent*100));
  arduinoStatus = builder;
}

//-------------------------------------------------------------------------------



//------------------------Pornirea Manuala a pompei de apa------------------------
  void manualPump(int HowManySeconds)
  {
    if(timeStartPump<HowManySeconds)
    {
      digitalWrite(PUMP,HIGH);
      timeStartPump++;
    }
    else
    {
      timeStartPump=0;
      digitalWrite(PUMP,LOW);
      powerPump=0;//la finalul functiei manuale mutam iar pompa pe modul automat
    }
  }
    
 //------------------------Pornirea Manuala a pompei de apa------------------------

 //------------------------Pornirea Manuala a ventilatorului ------------------------
  void manualFan(int HowManySeconds,int HowFast)
  {
    if(timeStartFan<HowManySeconds)
    {
      if(HowFast<=1)
      {
      analogWrite(FAN,150); //viteza redusa
      timeStartFan++;
      }
      if(HowFast==2)
      {
      analogWrite(FAN,200);  //viteza medie
      timeStartFan++;
      }
      if(HowFast>=3)
      {
      analogWrite(FAN,250);   //viteza maxima
      timeStartFan++;
      }
    }
    else
    {
      timeStartFan=0;
      analogWrite(FAN,0);
      powerFan=0;//la finalul functiei manuale mutam iar ventilatorul pe modul automat
    }
    
  
  }
 //------------------------Pornirea Manuala a ventilatorului ------------------------
 
   
void setup() {
  Serial.begin(9600);
  pinMode(FAN,OUTPUT);
  pinMode(PUMP,OUTPUT);
  pinMode(HIGRO,INPUT);
  TCCR0A=(1<<WGM01);    //Set the CTC mode   
  OCR0A=0xF9; //Value for ORC0A for 1ms 
  TIMSK0|=(1<<OCIE0A);   //Set the interrupt request
  sei(); //Enable interrupt
  TCCR0B|=(1<<CS01);    //Set the prescale 1/64 clock
  TCCR0B|=(1<<CS00);
  //I2C functions
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Wire.begin(1); // enter i2c as slave
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

  if(keyword == "humi")
  {
    HumidLimit = (100 - msg.toInt()) * 5 + 150;
  }
  else if(keyword == "temp1")
  {
    HighTempLimit1 = msg.toFloat();
  }
  else if(keyword == "temp2")
  {
    HighTempLimit2 = msg.toFloat();
  }
  else if(keyword == "temp3")
  {
    HighTempLimit3 = msg.toFloat();
  }
  else if(keyword == "mode")
  {
    modFunctionare = msg.toInt();

  }
  else if(keyword == "pump")
  {
    powerPump = 1;
    pumpTime = msg.toInt();
  }
  else if(keyword == "fan")
  {
    powerFan = 1;
    fanTime = msg.toInt();
  }
  
}

void requestEvent(int bytes)
{
  statusBuilder(TempValue, HumidValue, HighTempLimit1, HighTempLimit2, HighTempLimit3, HumidLimit);
  Serial.println("Request event from master");
  char msg[50];
  
  arduinoStatus.toCharArray(msg,arduinoStatus.length()+1);
  Serial.println(msg);

  Wire.write(msg);
  
  
  //valueRandomizer();
  delay(10);
}


int manualSwitch = 0;
int autoSwitch = 0;

void loop() {
  //in this way you can count 1 second because the nterrupt request is each 1ms
  if(timer>=1000){
    HumidValue=analogRead(HIGRO);//Valoarea umiditatii
    temp.requestTemperatures();
    TempValue=temp.getTempCByIndex(0);
    if(modFunctionare == 1)
    {
      autoSwitch = 0;
      if(manualSwitch == 0)
      {
        digitalWrite(PUMP,LOW);
        analogWrite(FAN,0);
        manualSwitch = 1;
      }
      
      if(powerPump==1)//verificam daca este pompa in modul manual
      {
        manualPump(pumpTime);//pumpTime -cate secunde sa mearga pompa
      }
      if(powerFan==1)//verificam daca este ventilatorul in modul manual
      {
        manualFan(fanTime,vitezaFan);//fanTime-cate secunde sa mearga ventilatorul, vitezaFan(1 sau 2 sau 3)
      }
    }
    
    timer=0;
  }
  if(modFunctionare == 0)
  {
    manualSwitch = 0;
    if(autoSwitch == 0)
    {
        digitalWrite(PUMP,LOW);
        analogWrite(FAN,0);
        powerPump = 0;
        powerFan = 0;
        pumpTime = 0;
        fanTime = 0;
        autoSwitch = 1;
    }
    if(powerFan==0)//verificam daca nu cumva functioneaza ventilatorul pe modul manual
    {
    fanStarter(TempValue);
    }
    if(powerPump==0)//verificam daca nu cumva functioneaza pompa pe modul manual
    {
    pumpStarter(HumidValue);
    }
  }

  
}

ISR(TIMER0_COMPA_vect){    //This is the interrupt request
  timer++;
}
