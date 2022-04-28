/// 4/28/21 12:33 pm RESET WORKS!
/// CPE 301 Swamp Cooler Project
/// Written by: Jeremy Laporte, Nimisha Sabu, Yeamin Chowdhury
/// const int fan = 45; volatile const int yLED = 53; (B0)
/// volatile const int bLED = 51; (B2) volatile const int rLED = 47; (L2) volatile const int gLED = 49; (L0)
/// const int stepButton = 50 (B3); //volatile int onOffButton = 18 (D3); //volatile int resetButton = 38 (D7);

#include <DS3231.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <DHT.h>

#define DHTPIN 6
#define DHTTYPE DHT11

DS3231  rtc(SDA, SCL);
LiquidCrystal lcd(12, 11, 10, 9, 8, 7); 
Stepper myStepper(512, 2,3,4,5);
DHT dht(DHTPIN, DHTTYPE);

unsigned char *port_B = (unsigned char *) 0x25;  // for yLED and bLED and stepButton
unsigned char *DDR_B = (unsigned char *) 0x24;
unsigned char *port_L = (unsigned char *) 0x10B; // for fan rLED and gLED
unsigned char *DDR_L = (unsigned char *) 0x10A;
unsigned char *port_D = (unsigned char *) 0x2B; // for onOff and reset button
unsigned char *DDR_D = (unsigned char *) 0x2A;
unsigned char *pin_D = (unsigned char *) 0x29;  // for input w/ pullup
int Contrast = 60;        
volatile float tempIn = 0;
volatile float tempF = 0;
volatile float humidIn = 0;
volatile int disabledState = 1;
int stepButtonState = 0;
int steppos = 0;
volatile int resetState = 0;
void disabled();

int resval = 0;                  // holds the sensor value
int respin = A5;                 // analog water sensor - pin used


void setup()
{
  analogWrite(13, Contrast);
  lcd.begin(16,2);                     // Initialize the lcd display
  dht.begin();                         // Initialize the temperature sensor
  rtc.begin();                         // Initialize the real time clock object
  rtc.setTime(12,34,30);
  rtc.setDate(4,12,2022);
  myStepper.setSpeed(200);

  // pin modes:
  *DDR_B |= 0b00000001 | 0b00000100;   //yled | bled - output
  *DDR_L |= 0b00010000 | 0b00000001 | 0b00000100; //fan | gled | rled - output
  *DDR_D &= 0b01110111;  //resetButton | onOffButton - input
  *DDR_B &= 0b11110111;  //stepMotor - input
  *port_D |= 0b00001000; //onOffButton - input w/ pullup
  *pin_D |= 0b00001000; //onOffButton - input w/ pullup

  attachInterrupt(digitalPinToInterrupt(18), disabled, CHANGE);
}

void loop() {
  resetState = 0;
    
  *port_B &= (0 << PINB0) & (0 << PINB2);                // set yled and bled to low
  *port_L &= (0 << PINL4) & (0 << PINL0) & (0 << PINL2); // set fan gled rled to low

  // Read and display Temp and Humid. Determine state run and idle
  humidIn = dht.readHumidity();
  tempIn = dht.readTemperature();
  tempF = dht.readTemperature(true);
  resval = analogRead(respin);
    
  if (disabledState==1){
   // DISABLED State
    *port_B = (1 << PINB0); // digitalWrite(yLED, HIGH);
    *port_L &= (0 << PINL4); //Fan to low
    lcd.noDisplay();
    delay(100);
  }
  
  else {
   // IDLE/RUNNING/ERROR State hub
    *port_B &= (0 << PINB0); // yellow led off
    *port_L |= (1 << PINL0);//turn the green led on
    lcd.clear();
    lcd.display();
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.setCursor(7,0);
    lcd.print(tempIn);
    delay(1000);
    lcd.setCursor(0,1);
    lcd.print("Humid: ");
    lcd.setCursor(8,1);
    lcd.print(humidIn);
    delay(1000);
    lcd.clear();
    delay(1000);

    while(resval <= 300 && disabledState==0 && resetState == 0){
     //ERROR State
      resetState=!digitalRead(38);
      *port_L &= (0 << PINL0); //turn off the green light
      *port_L &= (0 << PINL4); //keep the fan off 
      *port_L |= (1 << PINL2); //turn the red light on
      delay (1000);
      lcd.clear();
      lcd.display();
      lcd.setCursor(0,0);
      lcd.print("Water Lvl: LOW");}
      
    while (tempIn > 22.0 && disabledState==0 && resval>300) {
     //RUNNING State
      *port_L &= (0 << PINL0);// turn the green led off
      *port_B |= (1 << PINB2);//turn the blue led on
      *port_L |= (1 << PINL4); //turn the fan on

      // Stepper Motor, should move independent of rest, not when disabled
      int stepButtonState = digitalRead(53);
      if (stepButtonState == HIGH){myStepper.step(100); steppos += 1;}
    
      humidIn = dht.readHumidity();
      tempIn = dht.readTemperature();
      tempF = dht.readTemperature(true);
      resval = analogRead(respin);
      lcd.display();
      lcd.print(resval);
      lcd.clear();
      lcd.display();
      lcd.setCursor(0,0);
      lcd.print("Temp: ");
      lcd.setCursor(7,0);
      lcd.print(tempIn);
      delay(1000);
      lcd.setCursor(0,1);
      lcd.print("Humid: ");
      lcd.setCursor(8,1);
      lcd.print(humidIn);
      delay(1000);
      lcd.clear();
      delay(1000);
    }
  }
}

// ------------------------------------------------------------------------
void disabled() {
  if (disabledState == 0) {disabledState = 1;}
  else {disabledState = 0;}
}

//void Times(){
//  // During state transition, send this date and time
//  Serial.print("Time:  ");
//  Serial.println(rtc.getTimeStr());
//  Serial.print("Date: ");
//  Serial.println(rtc.getDateStr());
//  Serial.print("Vent Position: ");
//  Serial.print(steppos);
//  delay(1000);
//}
//
