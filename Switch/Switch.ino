#include <Adafruit_NeoPixel.h>

#include <Adafruit_NeoPixel.h>
#include <i2c.h>
#include <mpr121.h>
#include "Switch.h"
#include <radioino.h>

/*
Switch power Main Module
 */

byte inputPins[] = {5,8,9,10,11,12};  // Digital INPUT pins
byte outputPins[] = {4,3,5,6,7};  // Digital OUTPUT pins
byte analogInputPins[] = {4,0,1,2,3 };  // Analogic INPUT pins

#define LEDS_PIN 4
#define BOOT_COLOR 0x0000FF

// Touchs sensors state
long previousMillis = 0;
long interval = 100; // 100ms between sensor reading
unsigned char status1 =0;
unsigned char status2 =0;

// Initialize the module
Radioino module(
    inputPins,       // Module input pins
    outputPins,      // Module output pins     
    analogInputPins  // Module analog input pins
);

/* Initalize Switches ***********************************/
#define SWITCHS_COUNT 3
SWITCH_DEF switches[SWITCHS_COUNT];
/******************************************************/

// Initalize LEDs
Adafruit_NeoPixel leds = Adafruit_NeoPixel(SWITCHS_COUNT, LEDS_PIN, NEO_GRB + NEO_KHZ800);


void setup()
{  
  // Initialize all leds
  leds.begin();  // Call this to start up the LED strip.
  for (int i=0;i<SWITCHS_COUNT;i++)
    leds.setPixelColor(i,BOOT_COLOR);
  leds.show();

  // Setup the switches
  switches[0].ledId = 0;
  switches[0].relayPin = 3;
  switches[0].touchChannel = 10;
  switches[0].state = LOW;
  switches[0].hasFocus = false;
  
  switches[1].ledId = 1;
  switches[1].relayPin = 5;
  switches[1].touchChannel = 11;
  switches[1].state = LOW;
  switches[1].hasFocus = false;
  
  switches[2].ledId = 2;
  switches[2].relayPin = 6;
  switches[2].touchChannel = 0;
  switches[2].state = LOW;
  switches[2].hasFocus = false;
  
  // start serial port at 9600 bps and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  DDRC |= 0b00010011;
  PORTC = 0b00110000;  // Pull-ups on I2C Bus
  i2cInit();
  mpr121QuickConfig();
  
  for (int i=0;i<SWITCHS_COUNT;i++)
  {
    leds.setPixelColor(i,0);
    leds.show();
    delay(100);
  }
}

void loop()
{    
  if (module.receiveCommand())
  {
    if (module.getCommandResult()==RADIOINO_COMMAND_OK)
    {
    }
    // end response
    module.sendResponse();
  }  
  
  // Read touch panels
  unsigned long currentMillis = millis();  
  if(currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    Read_MPR121();
  }
}

void  CheckTouchSensorStatus()
{
  boolean touched = false;
  if ((status1&0x01)==0x01)
  {
    touchSwitch(0);
    touched = true;
  }
  if ((status1&0x02)==0x02)
  {
    touchSwitch(1);
    touched = true;
  }
  if ((status1&0x04)==0x04)
  {
    touchSwitch(2);
    touched = true;
  }
  if ((status1&0x08)==0x08)
  {
    touchSwitch(3);
    touched = true;
  }
  if ((status1&0x10)==0x10)
  {
    touchSwitch(4);
    touched = true;
  }
  if ((status1&0x20)==0x20)
  {
    touchSwitch(5);
    touched = true;
  }
  if ((status1&0x40)==0x40)
  {
    touchSwitch(6);
    touched = true;
  }
  if ((status1&0x80)==0x80)
  {
    touchSwitch(7);
    touched = true;
  }
  if ((status2&0x01)==0x01)
  {
    touchSwitch(8);
    touched = true;
  }
  if ((status2&0x02)==0x02)
  {
    touchSwitch(9);
    touched = true;
  }
  if ((status2&0x04)==0x04)
  {
    touchSwitch(10);
    touched = true;
  }
  if ((status2&0x08)==0x08)
  {
    touchSwitch(11);
    touched = true;
  }
  if (!touched)
  {
     for (int i=0;i<SWITCHS_COUNT;i++)
    {
      switches[i].hasFocus = false;
    }
  }
}

void touchSwitch (int id)
{
  for (int i=0;i<SWITCHS_COUNT;i++)
  {
    if (switches[i].touchChannel == id)
    {
      if (switches[i].state==HIGH && !switches[i].hasFocus)
      {        
        switches[i].state=LOW;
        
        // Switch relay
        if (module.read(switches[i].relayPin))
        {
          module.write(switches[i].relayPin,LOW);
          leds.setPixelColor(switches[i].ledId,ROYALBLUE);
        }
        else {
          module.write(switches[i].relayPin,HIGH);
          leds.setPixelColor(switches[i].ledId,0);
          
        }
        leds.show();   // ...but the LEDs don't actually update until you call this.      
        
        Serial.print("Channel ");
        Serial.print(id);
        Serial.println(" button up");
        
      }
      else {
        if (switches[i].state==LOW) 
        {
          Serial.print("Channel ");
          Serial.print(id);
          Serial.println(" button down");
          switches[i].state=HIGH;
          switches[i].hasFocus = true;          
        }
      }
      return;
    }
    else {
      switches[i].hasFocus = false;
    }
  }
}


/************** TOUCH SENSOR STUFF *********************/

#define MPR121_R 0xB5	// ADD pin is grounded
#define MPR121_W 0xB4	// So address is 0x5A


void Read_MPR121()
{
  status1=mpr121Read(0x00);
  status2=mpr121Read(0x01);
  CheckTouchSensorStatus();  
}

void mpr121Write(unsigned char address, unsigned char data)
{
  i2cSendStart();
  i2cWaitForComplete();

  i2cSendByte(MPR121_W);// write 0xB4
  i2cWaitForComplete();

  i2cSendByte(address);	// write register address
  i2cWaitForComplete();

  i2cSendByte(data);
  i2cWaitForComplete();

  i2cSendStop();
}

byte mpr121Read(uint8_t address)
{
  byte data;

  i2cSendStart();
  i2cWaitForComplete();

  i2cSendByte(MPR121_W);	// write 0xB4
  i2cWaitForComplete();

  i2cSendByte(address);	// write register address
  i2cWaitForComplete();

  i2cSendStart();

  i2cSendByte(MPR121_R);	// write 0xB5
  i2cWaitForComplete();
  i2cReceiveByte(TRUE);
  i2cWaitForComplete();

  data = i2cGetReceivedByte();	// Get MSB result
  i2cWaitForComplete();
  i2cSendStop();

  cbi(TWCR, TWEN);	// Disable TWI
  sbi(TWCR, TWEN);	// Enable TWI

  return data;
}


void mpr121QuickConfig(void)
{
  // Section A
  // This group controls filtering when data is > baseline.
  mpr121Write(MHD_R, 0x01);
  mpr121Write(NHD_R, 0x01);
  mpr121Write(NCL_R, 0x00);
  mpr121Write(FDL_R, 0x00);

  // Section B
  // This group controls filtering when data is < baseline.
  mpr121Write(MHD_F, 0x01);
  mpr121Write(NHD_F, 0x01);
  mpr121Write(NCL_F, 0xFF);
  mpr121Write(FDL_F, 0x02);

  // Section C
  // This group sets touch and release thresholds for each electrode
  mpr121Write(ELE0_T, TOU_THRESH);
  mpr121Write(ELE0_R, REL_THRESH);
  mpr121Write(ELE1_T, TOU_THRESH);
  mpr121Write(ELE1_R, REL_THRESH);
  mpr121Write(ELE2_T, TOU_THRESH);
  mpr121Write(ELE2_R, REL_THRESH);
  mpr121Write(ELE3_T, TOU_THRESH);
  mpr121Write(ELE3_R, REL_THRESH);
  mpr121Write(ELE4_T, TOU_THRESH);
  mpr121Write(ELE4_R, REL_THRESH);
  mpr121Write(ELE5_T, TOU_THRESH);
  mpr121Write(ELE5_R, REL_THRESH);
  mpr121Write(ELE6_T, TOU_THRESH);
  mpr121Write(ELE6_R, REL_THRESH);
  mpr121Write(ELE7_T, TOU_THRESH);
  mpr121Write(ELE7_R, REL_THRESH);
  mpr121Write(ELE8_T, TOU_THRESH);
  mpr121Write(ELE8_R, REL_THRESH);
  mpr121Write(ELE9_T, TOU_THRESH);
  mpr121Write(ELE9_R, REL_THRESH);
  mpr121Write(ELE10_T, TOU_THRESH);
  mpr121Write(ELE10_R, REL_THRESH);
  mpr121Write(ELE11_T, TOU_THRESH);
  mpr121Write(ELE11_R, REL_THRESH);
 
  // Section D
  // Set the Filter Configuration
  // Set ESI2
  mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
  mpr121Write(ATO_CFGL, 0x82);  // Target = 0.9*USL = 0xB5 @3.3V
  mpr121Write(ATO_CFGT,0xb5);
  mpr121Write(ATO_CFG0, 0x1B);
  // Section E
  // Electrode Configuration
  // Enable 6 Electrodes and set to run mode
  // Set ELE_CFG to 0x00 to return to standby mode
  mpr121Write(ELE_CFG, 0x8c);
  // Section F
  // Enable Auto Config and auto Reconfig
  /*mpr121Write(ATO_CFG0, 0x0B);
  mpr121Write(ATO_CFGU, 0xC9);	// USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   mpr121Write(ATO_CFGL, 0x82);	// LSL = 0.65*USL = 0x82 @3.3V
  mpr121Write(ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V
}
