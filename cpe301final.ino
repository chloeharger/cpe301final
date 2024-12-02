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
#define DHT11_PIN 7

// LCD Display Setup:
#include <LiquidCrystal.h>
const int RS = 12, EN = 13, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Setup for using millis()
unsigned long int previousMillis=0;
const long int interval = 1000;

// Setup for clock:
#include <RTClib.h>
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup(){
    U0init(9600);
    lcd.begin(16, 2);
    rtc.begin();
}

void loop(){
  
    //Display temp and humidity levels
    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis >= interval){
        previousMillis = currentMillis;
        
        int chk = DHT.read11(DHT11_PIN);
        lcd.setCursor(0,0);
        lcd.print("Temp=");
        lcd.setCursor(7, 0);
        lcd.print(DHT.temperature);
        lcd.setCursor(0,1);
        lcd.print("Humidity=");
        lcd.setCursor(9, 1);
        lcd.print(DHT.humidity);
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

//(can delete these comments once code is started)
// Set up water sensor

// Fan motor (based on temperature levels)
// Set up stepper motor for vent
// On/off button

// Set date+time when system turns on or off:
void displayClock(){
    DateTime now = rtc.now();
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
}
