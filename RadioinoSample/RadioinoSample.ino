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
#include <iotoaster.h>

byte inputPins[] = {4,2,3,4,5};  // Digital INPUT pins (first byte is the ports count)
byte outputPins[] = {4,6,7,8,9};  // Digital OUTPUT pins (first byte is the ports count)
byte analogInputPins[] = {4,4,5,6,7 };  // Analogic INPUT pins (first byte is the ports count)

// Initialize the module
IOToaster module(inputPins,      // Module input pins
                outputPins,      // Module output pins     
                analogInputPins  // Module analog input pins
);

void setup()
{
  // start serial port at 9600 bps and wait for port to open:
  module.setup();
}

// Custom command handler
boolean customCommandCallBack(char command, int param)
{
  // something like 'R0004H6Z1'
  if (command=='Z')
  {
    if (param == 0)
      module.send("ZDISABLED");
    else module.send("ZENABLED");
    
    // Command accepted
    return true;
  }  
  // Command not recognized. Error
  return false;
}

void loop()
{
  // Configure the custom command handler
  module.setCustomCommandCallback(customCommandCallBack);
  
  if (module.receiveCommand())
  {
    if (module.getCommandResult()==IOTOASTER_COMMAND_OK)
    {
      // Send custom data
      module.send("my stuff here");
    }
    // end response
    module.sendResponse();
  }
}

