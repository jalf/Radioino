#include <radioino.h>
#include <dht.h>

/*
Outlet power Main Module
 */

byte inputPins[] = {5,8,9,10,11,12};  // Digital INPUT pins
byte outputPins[] = {6,2,3,4,5,6,7};  // Digital OUTPUT pins
byte analogInputPins[] = {4,0,1,2,3 };  // Analogic INPUT pins

int relay1Pin = 2;
int relay2Pin = 3;
int PIRpin = 9;
int dhtPin = 10;
boolean PIRdectected = false;

// Temperature Sensor
dht DHT11;

// Initialize the module
Radioino module("0002",  // Module Unique Address
    inputPins,       // Module input pins
    outputPins,      // Module output pins     
    analogInputPins  // Module analog input pins
);


void setup()
{
  // start serial port at 9600 bps and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // Wait the sensors
  delay(1000);
}

void loop()
{  
  // Read the PIR sensor
  if (digitalRead(PIRpin)==HIGH)
  {
    PIRdectected = true;
  }
  
  // Read the serial command
  if (Serial.available() > 0) {
    if (module.receive()) {
      if (module.execute())
      {
        // Build the response message
        module.beginResponse(true);
        delay(100);
        // Add the sensors status
        module.sendSensorsStatus();                
        
        // **** Add the module sensors  *****
        
        // PIR
        if (PIRdectected)        
          module.send("PIRH");
        else module.send("PIRL");  
        PIRdectected = false;
        
        // Temperature Sensor
        int chk = DHT11.read11(dhtPin);
        switch (chk)
        {
          case DHTLIB_OK:  
      		Serial.print("HMD");
                Serial.print(DHT11.humidity);
                Serial.print("|");
                Serial.print("TMP");
                Serial.print(DHT11.temperature);
                Serial.print("|");
      		break;
        }        
        //Relays
        if (digitalRead(relay1Pin) == HIGH)
          module.send("RLY1H");
        else module.send("RLY1L");  
        if (digitalRead(relay2Pin) == HIGH)
          module.send("RLY2H");
        else module.send("RLY2L");  
      }
      else {
        // Build the error response message
        module.beginResponse(false);
      }
      
      // end response
      module.endResponse();
      
      // wait for a new message
      delay(200); 
    }
  }  
}
