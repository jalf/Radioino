#include <radioino.h>
#include <dht.h>

/*
Outlet power Main Module
 */

byte inputPins[] = {5,8,9,10,11,12};  // Digital INPUT pins
byte outputPins[] = {5,3,4,5,6,7};  // Digital OUTPUT pins
byte analogInputPins[] = {4,0,1,2,3 };  // Analogic INPUT pins

int relay1Pin = 3;
int relay2Pin = 4;
int PIRpin = 9;
int dhtPin = 10;
boolean PIRdectected = false;

// Temperature Sensor
dht DHT11;

// Initialize the module
Radioino module(
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
  
  if (module.receiveCommand())
  {
    if (module.getCommandResult()==RADIOINO_COMMAND_OK)
    {
      // **** Add the module sensors  *****        
      // PIR
      if (PIRdectected)        
          module.send("PIRH");
      else module.send("PIRL");  
      PIRdectected = false;
        
      // Temperature Sensor
      int chk = DHT11.read11(dhtPin);
      String dht11Message = "";
      switch (chk)
      {
        case DHTLIB_OK:
          dht11Message= "HMD"+String((int)DHT11.humidity)+"|TMP"+String((int)DHT11.temperature);
          module.send(dht11Message);
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
    // end response
    module.sendResponse();
  }  
}
