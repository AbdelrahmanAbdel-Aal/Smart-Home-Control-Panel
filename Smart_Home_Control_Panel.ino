#include <OneWire.h>
#include <DallasTemperature.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#define DEBUG true  
#define led1 3
#define led2 5
#define temp 4
#define irsensor 2
#define smokesensor A1
const byte rxPin = 11; 
const byte txPin = 10;
void setup_Etreral_interrupt();
SoftwareSerial ESP8266 (rxPin, txPin);


int smoke;
float tempread;
int ir_value;
int somoke_value;
int x = 0;
const int rs = 13, en = 12, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// Create a new instance of the oneWire class to communicate with any OneWire device:
OneWire oneWire(temp);
// Pass the oneWire reference to DallasTemperature library:
DallasTemperature temprature(&oneWire);
void delay();
//void esp_init();
void tempraturesensor();
void somkesensor();
void motion();
void ADC_INIT();
void timer_init();
unsigned int ADC_Read(unsigned char channel);
void wifiSend(String command);
void InitWifiModule();
void connect();



//float getTemperature();
void setup()
 {
  // set the ports input for the sensors
  DDRD&=~(1<<temp);
  // DDRD&=~(1<<irsensor);
  DDRD&=~(1<<smokesensor);
  DDRD|=(1<<led1)|(1<<led2); //set leds pins output
  Timer1_Init();
  pinMode(irsensor, INPUT);
  Serial.begin(9600);
  ESP8266.begin(9600); 
  InitWifiModule();
  lcd.begin(16, 2);
  lcd.clear();
  setup_Etreral_interrupt();
}

void loop() 
{
  connect(); 
}

void Timer1_Init() 
{
  // Set up Timer 1 to generate an interrupt every 2 seconds
  cli();       // Disable interrupts
  TCCR1A = 0;  // Set Timer 1 to normal mode
  TCCR1B = 0;  // Clear Timer 1 prescaler settings
  OCR1AH = 0x7A;
  OCR1AL = 0x12;                        // Set Timer 1 compare match value to 2 seconds
  TCCR1B |= (1 << WGM12);               // Set Timer 1 to CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);  // Set Timer 1 prescaler to 1024
  TIMSK1 |= (1 << OCIE1A);              // Enable Timer 1 interrupt
  sei();                                // Enable interrupts
}

ISR(TIMER1_COMPA_vect)  // Timer 1 Compare Match A ISR
{
  tempraturesensor();
  somkesensor();
  motion();
}



void motion(){
 ir_value = digitalRead(irsensor);
  lcd.setCursor(9,0);
  lcd.print(" ir: ");
  lcd.print(ir_value);

}
void tempraturesensor(){
  temprature.requestTemperatures();
 tempread = temprature.getTempCByIndex(0);
  lcd.setCursor(0, 0);
  lcd.print("temp:");
  lcd.print(tempread);
  
}
void somkesensor() {
    somoke_value = ADC_Read(1);
   // Serial.print("Smoke Sensor Value: ");
    //Serial.println(somoke_value);

    if (somoke_value > 5) {
        smoke = 1;
        lcd.setCursor(1, 1);
        lcd.print("Smoke: ");
        lcd.println(somoke_value);
    } else {
        smoke = 0;
        lcd.setCursor(0, 1);
        lcd.print("No Smoke: ");
        lcd.println(somoke_value);
    }
}


void ADC_INIT()
{
  //Select Reference Voltage(AVCC)
  ADMUX |=  (1<<REFS0);
  //Select Scaler (128)
  ADCSRA |= (1<<ADPS2) |(1<<ADPS1) | (1<<ADPS0);
  
  //Enable leftRightAdjust
  ADMUX |= (1<<5);
  DDRC = 0;
  //Enable ADC 
  ADCSRA |= (1<<ADEN);  
}
unsigned int ADC_Read(unsigned char channel)
{   
  //clear the mux bits
  ADMUX = 0b11100000;
  //Select channel
  ADMUX  |= channel;
  //Start conversion
  ADCSRA |= (1<<ADSC);
  while(ADCSRA & (1<<ADIF) == 0);
  //clear ADIF 
  ADCSRA |=(1<<ADIF);
  return ADCH; 
}


void setup_Etreral_interrupt()
{
  attachInterrupt(digitalPinToInterrupt(irsensor),ISR_ir,FALLING );
}
void ISR_ir(){
    lcd.clear();
  lcd.setCursor(0,1);
  lcd.println("interrupt");
  //Serial.println("interrupt");
}

void InitWifiModule()
{
  
  wifiSend("AT+UART_DEF=9600,8,1,0,0\r\n");                                        
  wifiSend("AT+RST\r\n");    
  secDelay(4);
  wifiSend("AT+CWJAP=\"..\",\"01060531768@@#\"\r\n");        
  secDelay(6);
  wifiSend("AT+CWMODE=1\r\n");                                             
  secDelay(2);
  wifiSend("AT+CIFSR\r\n");                                             
  secDelay(2);
  wifiSend("AT+CIPMUX=1\r\n");                                             
  secDelay(2);
  wifiSend("AT+CIPSERVER=1,80\r\n"); 

}

void wifiSend(String command)
{                                             
  ESP8266.print(command);
  String h;
   
    while(ESP8266.available())                                      
    {
      char c=ESP8266.read();
      h+=c;                                                                 
    } 
  Serial.println(h);
}

void secDelay(int n)
 {
  for (int i = 0; i < 1000 * n; i++) 
  {
    TCCR2A = 0x0;
    TCCR2B = 0x04;
    TCNT2 = 0x00;
    while ((TIFR2 & 1) == 0);
    TIFR2 |= (1 << TOV2);
  }
}

void connect()
{
   if(ESP8266.available())                                           
 {    
    if(ESP8266.find("+IPD,"))
    {
     secDelay(1);
 
      int connectionId = ESP8266.read()-48; 
      String request = ESP8266.readStringUntil('\r');   
      Serial.println("requets "+request);                                            
      String webpage1 = "<html><body><h1>Control LEDs</h1><form><button name=\"led1\" value=\"Togg\" type=\"submit\">LED 1 </button><br><button name=\"led2\" value=\"Togg\" type=\"submit\">LED 2</button></form></body></html>";
      String webpage2 ="<html><body><h1>Smart Home Control Panel</h1><form><button name=\"led1\" value=\"Togg\" type=\"submit\">LED 1 </button><br><button name=\"led2\" value=\"Togg\" type=\"submit\">LED 2</button></form><h1>Nice Work ^^ </h1></body></html>";
      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      if(smoke==0)
      {
        cipSend +=webpage1.length();
      }
      else
      {
        cipSend +=webpage2.length();
      }
        
      cipSend +="\r\n";
      if (request.indexOf("/?led1") != -1) 
      {
        
          PORTD^=(1<<3);
      
      }
      if (request.indexOf("/?led2") != -1)
      {
       
            PORTD^=(1<<5);

      }

     wifiSend(cipSend);
     if(smoke==0)
     {
       wifiSend(webpage1);
     }
     else
     {
       wifiSend(webpage2);
     }
     

     String closeCommand = "AT+CIPCLOSE="; 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";    
     wifiSend(closeCommand);
    }
  }
}