/*
Chloe Harger and Genesis Mendoza
CPE 301 Final Project main code file
*/

// UART Register Setup:
 #define RDA 0x80
 #define TBE 0x20  
 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *)0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

// Temp/Humidity Sensor Setup:
#include <dht.h>
dht DHT;
#define DHT11_PIN 18

// LCD Display Setup:
#include <LiquidCrystal.h>
const int RS = 12, EN = 13, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Water Sensor Setup:
volatile unsigned char *portH = (unsigned char *)0x102; //power pin is PH5
volatile unsigned char *ddrH = (unsigned char *)0x101;
volatile unsigned char *pinH = (unsigned char *)0x100;
#define WATER_SIGNAL A5
int waterSensorVal = 0;
#define MIN_LEVEL 100

// Stepper Motor Setup (for vent)
#include <Stepper.h>
const int stepsPerRev = 2038;
Stepper myStep = Stepper(stepsPerRev, 41, 45, 43, 47);
volatile unsigned char *portF = (unsigned char *)0x31; //using pins A0 and A1
volatile unsigned char *ddrF = (unsigned char *)0x30;
volatile unsigned char *pinF = (unsigned char *)0x2F;

// Fan Motor Setup
volatile unsigned char *portA = (unsigned char *)0x22;
volatile unsigned char *ddrA = (unsigned char *)0x21;
volatile unsigned char *pinA = (unsigned char *)0x20;
#define SPEEDPIN 6

// Setup for using millis()
unsigned long int previousMillis=0;
const long int interval = 1000;

// Setup for clock:
#include <RTClib.h>
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
volatile bool stateChange = false;

// Setup for LED (R = 11(PB5), G = 10(PB4), B = 9(PH6))
volatile unsigned char *portB = (unsigned char *)0x25;
volatile unsigned char *ddrB = (unsigned char *)0x24;
volatile unsigned char *pinB = (unsigned char *)0x23;
#define RED 11
#define BLUE 9
#define GREEN 10

// Misc ISR state setup
#define MAX_TEMP 27
volatile int t=0;
volatile bool tooHot = false;
volatile bool reset = false;

void setup(){
    U0init(9600);
    Serial.begin(9600);
    lcd.begin(16, 2);
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    //water sensor:
    *ddrH |= 0b00100000; //set PH5 to output
    *portH &= 0b11011111; //set PH5 to low
    *ddrF &= 0b00010111; //set A5, A3, A6, A7 to input

    //buttons to control stepper
    *ddrF &= 0b11111100;
    *portF &= 0b11111100;

    //fan motor
    *ddrA |= 0b00101000; //PA3, PA5 outputs
    *ddrH |= 0b00001000; //PH3 output
    
    //LED
    *ddrB |= 0b00110000;
    *ddrH |= 0b01000000;

    //states:
    attachInterrupt(digitalPinToInterrupt(18), fanState, CHANGE);

    //water level error:
    PCMSK2 |= bit (PCINT19);
    PCIFR |= bit (PCIF2);
    PCICR |= bit (PCIE2);
}

void loop(){
    unsigned long currentMillis = millis();
    unsigned long previousMillis2=0;
    displayClock();

    if(currentMillis - previousMillis >= interval){
        previousMillis = currentMillis;
        
        
        //Display temp and humidity levels:
        int chk = DHT.read11(DHT11_PIN);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Temp: ");
        lcd.print(DHT.temperature);
        lcd.print((char)223);
        lcd.print("C");
        lcd.setCursor(0,1);
        lcd.print("Humidity: ");
        lcd.print(DHT.humidity);
        lcd.print("%");

        noInterrupts();
        t=DHT.temperature;
        interrupts();
        
        if(tooHot==true){
          blueLED();
          startFan();
        }else{
          stopFan();
          greenLED();
        }

        //Read Water Sensor Values:
        *portH |= 0b00100000; //turn on sensor
        if(currentMillis - previousMillis2 >= 10){
          previousMillis2 = currentMillis;
          waterSensorVal = analogRead(WATER_SIGNAL);
          U0putchar('S');
          U0putchar('e');
          U0putchar('n');
          U0putchar('s');
          U0putchar('o');
          U0putchar('r');
          U0putchar(' ');
          U0putchar('V');
          U0putchar('a');
          U0putchar('l');
          U0putchar(':');
          char first = waterSensorVal/100 + '0';
          char second = (waterSensorVal/10)%10 + '0';
          char third = waterSensorVal%10 + '0';
          
          U0putchar(' ');
          U0putchar(first);
          U0putchar(second);
          U0putchar(third);
          U0putchar('\n');

          if(waterSensorVal < MIN_LEVEL){
            reset = false;
            redLED();
            stateChange = true;
          
            while(!reset){
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("ERROR: Water low");
              lcd.setCursor(0,1);
              lcd.print("Add water, reset");
              if(*pinF & 0b00001000){
                reset = true;
              }
            }
          }
        }
    }
    //Move vent when button pressed:
    myStep.setSpeed(10);
    if(*pinF & 0b00000010){
      myStep.step(stepsPerRev);
      stateChange = true;
    }
    if(*pinF & 0b00000001){
      myStep.step(-stepsPerRev);
      stateChange = true;
    }

    //check for start/stop button to be pressed:
    if(*pinF & 0b10000000){
      bool on = false;
      while(!on){
          stateChange = true;
          yellowLED();
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("System Off");
          delay(100);
          //off until button pressed again:
          if(*pinF & 0b01000000){
            on = true;
          }
        }
      }
    
}

//UART Functions:
void U0init(unsigned long U0baud){
    unsigned long FCPU = 16000000;
    unsigned int tbaud;
    tbaud = (FCPU / 16 / U0baud - 1);
    *myUCSR0A = 0x20;
    *myUCSR0B = 0x18;
    *myUCSR0C = 0x06;
    *myUBRR0  = tbaud;
}
unsigned char U0kbhit(){
    return (*myUCSR0A & RDA);
}
unsigned char U0getchar(){
    return(*myUDR0);
}
void U0putchar(unsigned char U0pdata){
    while((*myUCSR0A & TBE) == 0);
    *myUDR0 = U0pdata;
}

// fan motor/state:
void startFan(){
    *portA |= 0b00001000;
    *portA &= 0b11011111;
    analogWrite(SPEEDPIN, 255);
}
void stopFan(){
    *portA &= 0b11010111;
}
void fanState(){
  if(t > MAX_TEMP){
    tooHot = true;
    stateChange = true;
  }else{
    tooHot = false;
  }
}

// Set date+time when system turns on or off:
void displayClock(){ //either modify to remove serial calls, or don't use in interrupt
  if(stateChange){
    DateTime now = rtc.now();
    
    Serial.print("State Change at ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    stateChange = false;
  }
}

void redLED(){
  analogWrite(RED, 255);
  analogWrite(BLUE, 0);
  analogWrite(GREEN, 0);
}
void yellowLED(){
  analogWrite(RED, 180);
  analogWrite(BLUE, 0);
  analogWrite(GREEN, 150);
}
void greenLED(){
  analogWrite(RED, 0);
  analogWrite(BLUE, 0);
  analogWrite(GREEN, 255);
}
void blueLED(){
  analogWrite(RED, 0);
  analogWrite(BLUE, 255);
  analogWrite(GREEN, 0);
}
