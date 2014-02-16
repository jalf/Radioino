/*  Radioino Sample
 
 An example of using the Radioino board to receive data from the 
 computer.  In this case, the boards turns all pins states and
 some custom reusult.
 
 The data can be sent from the Arduino serial monitor, or another
 program like Processing (see code below), Flash (via a serial-net
 proxy), PD, or Max/MSP.
 
 This example code is in the public domain.
 
 https://github.com/jalf/Radioino

*/

#include <radioino.h>

byte inputPins[] = {4,9,10,11,12};  // Digital INPUT pins (first byte is the ports count)
byte outputPins[] = {6,3,4,5,6,8,13};  // Digital OUTPUT pins (first byte is the ports count)
byte analogInputPins[] = {4,0,1,2,3 };  // Analogic INPUT pins (first byte is the ports count)

// Initialize the module
Radioino module(inputPins,       // Module input pins
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
}

void loop()
{
  if (module.receiveCommand())
  {
    if (module.getCommandResult()==RADIOINO_COMMAND_OK)
    {
      // Send custom data
      module.send("my stuff here");
    }
    // end response
    module.sendResponse();
  }
}
