#include "Arduino.h"
#include "Radioino.h"

Radioino::Radioino(String address, byte inputPins[], byte outputPins[], byte analogInputPins[])
{
	_address = address;
	_startHeader = "R"+address;

	_inputPinsCount = inputPins[0];
	_outputPinsCount = outputPins[0];
	_analogInputPinsCount = analogInputPins[0];
	
	_inputPins = inputPins+sizeof(byte);
	
	// Setup output pins state array
	_outputPins = new PinState[_outputPinsCount];
	for (int i=0; i<_outputPinsCount;i++)
	{
		_outputPins[i].number = outputPins[i+1]; // The first by is the count
	}
	
	_analogInputPins = analogInputPins+sizeof(byte);
	
	
  // Alloc strings
  _command.reserve(50);
  _value.reserve(3);

  // Set the pin configuration
  setupPins();
}

// Write a HIGH or a LOW value to an digital pin.
void Radioino::Write(byte pin, boolean value)
{
	// Check if the pin is an output pin
	for (int i=0; i<_outputPinsCount;i++)
	{
		if (_outputPins[i].number == pin)
		{
			digitalWrite(pin,value);
			_outputPins[i].state = value;
			break;
		}
	}
}


// Read a HIGH or a LOW value from an digital pin.
boolean Radioino::Read(byte pin)
{
	// Check if the pin is an output pin
	for (int i=0; i<_outputPinsCount;i++)
	{
		if (_outputPins[i].number == pin)
		{
			return _outputPins[i].state;
		}
	}
	
	// is a input pin
	return digitalRead(pin);
}


// Set all pin modes
void Radioino::setupPins()
{
	// Set the input pins
	for (int i=0;i<sizeof(_inputPins);i++)
	{
		pinMode(_inputPins[i],INPUT);
	}

	// Set the output pins
	for (int i=0;i<_outputPinsCount;i++)
	{
		pinMode(_outputPins[i].number,OUTPUT);
		_outputPins[i].state = LOW;
		digitalWrite(_outputPins[i].number,LOW);
	}  

	//set the activity pin
	pinMode(RADIOINO_ACTIVIY_LED_PIN,OUTPUT);
}

void Radioino::waitCommand()
{
	_command = "";
	_commandCharIndex = 0;	
}

// Build the incoming command in a ascii-string
boolean Radioino::receive()
{
  while (Serial.available() > 0)
  {    
    int value = Serial.read();
    switch (value)
    {
      case RADIOINO_COMMAND_END	:
		// Command arrived. Check the command address
		if (_command.startsWith(_startHeader))
		{
			// show activity
			digitalWrite(RADIOINO_ACTIVIY_LED_PIN,HIGH);		
			return true;
		}
		waitCommand();
        return false;
      case 'R'  :
        waitCommand();
		_command = _command + char(value);
		return false;
      default:
        _command = _command + char(value);
		return false;
    }
  }    
}

boolean Radioino::execute()
{
	int inByte;
	_commandCharIndex = 5; // just after command header
	_commandSize = _command.length();
	while (_commandCharIndex < _commandSize) {            
		switch (_command.charAt(_commandCharIndex)){
		case 'H'  :    // Activate a digital output         
			_commandCharIndex++;          
			// Get the param
			inByte = getNextInt();        
			Write(inByte,HIGH);      
			break;
		case 'L'  :    // De-activate a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();        
			Write(inByte,LOW);
			break;
		case 'X'  :    // invert a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();        
			if (Read(inByte)==LOW)
				Write(inByte,HIGH);
			else Write(inByte,LOW);
			break;
		case 'T'  :		// Play the module Tone
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();  
			toneNotify(inByte);
			break;    		
		default:     
			return false;
		}
	}    	
	
	return true;
}

void Radioino::send(String data)
{
	Serial.print(data);
	Serial.print(RADIOINO_SECTION);
}

void Radioino::endResponse()
{
	Serial.println(RADIOINO_COMMAND_END);
	// End the activity
	digitalWrite(RADIOINO_ACTIVIY_LED_PIN,LOW);
}

void Radioino::beginResponse(boolean result)
{
	// start activity	
	digitalWrite(RADIOINO_ACTIVIY_LED_PIN,HIGH);
	
	// Header
	Serial.print("ADR");
	Serial.print(_address);
	Serial.print(RADIOINO_SECTION);
	Serial.print("ACK");
	
	if (!result)
	{
		Serial.print("ERROR");		
	}
	else {
		Serial.print("OK");
	}
	Serial.print(RADIOINO_SECTION);
}

int Radioino::getNextInt()
{
  _value = "";
  while(_command.charAt(_commandCharIndex) <='9' && _command.charAt(_commandCharIndex) >='0' 
    && _commandCharIndex < _commandSize)
  {
    _value = _value + _command.charAt(_commandCharIndex++);
  }
  return _value.toInt();
}

void Radioino::sendSensorsStatus()
{
	Serial.print("PIN");
	send(getInputSensorsStatus());
}

String Radioino::getInputSensorsStatus()
{
	String result ="";
	int value;

	// Add the input digital pins
	for (int i=0;i<_inputPinsCount;i++)
	{
		value = Read(_inputPins[i]);
		result = result + _inputPins[i] + ";"+value+";";
	}  
	// Add the output digital pins
	for (int i=0;i<_outputPinsCount;i++)
	{
		value = _outputPins[i].state;
		result = result + _outputPins[i].number + ";"+value+";";
	}  

	// Add the analogic pins
	result = result + "~";
	for (int i=0;i<_analogInputPinsCount;i++)
	{
		value = analogRead(_analogInputPins[i]);
		result = result + _analogInputPins[i] + ";"+value+";";
	}  

  return result;
}

// play the notify tone
void Radioino::toneNotify(int pin)
{
  tone(pin,494,50); //SI
  delay(80);
  tone(pin,700,100); 
}