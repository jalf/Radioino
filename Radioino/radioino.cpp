#include "Arduino.h"
#include "Radioino.h"

Radioino::Radioino(String address, byte inputPins[], byte outputPins[], byte analogInputPins[])
{
	_activityLedPin = RADIOINO_ACTIVIY_LED_PIN;
	_setupButtonPin = RADIOINO_SETUP_BUTTON_PIN;
	_setupMode = false;	
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

void Radioino::loop()
{
	if (digitalRead(_setupButtonPin)==HIGH)
	{	
		_setupMode = true;
	}
	if (_setupMode)
	{
		digitalWrite(RADIOINO_ACTIVIY_LED_PIN,HIGH);
	}
}

// Set the pin with the activity led. -1 for none.
void Radioino::setActiviyLedPin(byte pin)
{
	_activityLedPin = pin;
	if (pin != -1)
		pinMode(_activityLedPin,OUTPUT);
}

// Write a HIGH or a LOW value to an digital pin.
void Radioino::write(byte pin, boolean value)
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
boolean Radioino::read(byte pin)
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
	pinMode(_activityLedPin,OUTPUT);
	//set the setup button pin
	pinMode(RADIOINO_SETUP_BUTTON_PIN,INPUT);
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
			if (_activityLedPin != -1)
				digitalWrite(_activityLedPin,HIGH);		
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
			write(inByte,HIGH);      
			break;
		case 'L'  :    // De-activate a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();        
			write(inByte,LOW);
			break;
		case 'X'  :    // invert a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();        
			if (read(inByte)==LOW)
				write(inByte,HIGH);
			else write(inByte,LOW);
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
	if (_activityLedPin != -1)
		digitalWrite(_activityLedPin,LOW);
}

void Radioino::beginResponse(boolean result)
{
	// start activity
	if (_activityLedPin != -1)	
		digitalWrite(_activityLedPin,HIGH);
	
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
	send(getInputSensorsStatus());
}

String Radioino::getInputSensorsStatus()
{
	String result ="IND";
	int value;

	// Add the input digital pins
	for (int i=0;i<_inputPinsCount;i++)
	{
		value = read(_inputPins[i]);
		result = result + _inputPins[i] + ";"+value+";";
	}  
	// Add the output digital pins
	result = result += "|OUD";
	for (int i=0;i<_outputPinsCount;i++)
	{
		value = _outputPins[i].state;
		result = result + _outputPins[i].number + ";"+value+";";
	}  

	// Add the analogic pins
	result = result + "|INA";
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