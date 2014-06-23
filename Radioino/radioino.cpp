#include "Radioino.h"
#include <avr/eeprom.h>

Radioino::Radioino(byte inputPins[], byte outputPins[], byte analogInputPins[])
{
	_activityLedPin = RADIOINO_ACTIVIY_LED_PIN;
	_setupButtonPin = RADIOINO_SETUP_BUTTON_PIN;
	_setupMode = false;	
	_activityLedState = false;	

	// Load module address
	loadAddress();
	
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
  _response.reserve(255);
  _value.reserve(3);

  // Set the pin configuration
  setupPins();
}

// Set the module address
void Radioino::setAddress(String address)
{
	_address = address;
	_startHeader = "R"+address;
	
	// Save on eeprom
	for (int i=0;i<4;i++)		
		eeprom_write_byte((unsigned char *) i, _address[i]);
}

void Radioino::loadAddress()
{
	_address = "0000";
	for (int i=0;i<4;i++)		
		_address[i] = eeprom_read_byte((unsigned char *) i);		
	_startHeader = "R"+_address;
}

byte Radioino::receiveCommand()
{
	// Check the setup mode command
	if (digitalRead(_setupButtonPin)==HIGH && !_setupMode)
	{	
		_setupMode = true;
		_previousTime = 0;
		_setupModeCount = 0;
		setActivityLedState(LOW);
	}
	if (_setupMode)
	{
		// check to see if it's time to blink the LED; that is, if the 
		// difference between the current time and last time you blinked 
		// the LED is bigger than the interval at which you want to 
		// blink the LED.
		unsigned long currentTime = millis();
		if(currentTime - _previousTime > RADIOINO_SETUP_LED_BLINK_MS) {
			// save the last time you blinked the LED 
			_previousTime = currentTime;   
			_setupModeCount++;			
			
			// if the LED is off turn it on and vice-versa:
			if (_activityLedState == LOW)
				setActivityLedState(HIGH);
			else
				setActivityLedState(LOW);
		}
		
		// Exit the setup mode
		if (_setupModeCount > RADIOINO_SETUP_ATTEMPTS)
		{
			_setupMode = false;
			setActivityLedState(LOW);
		}
	}
	
	//Wait for a new command
	_commandResult = RADIOINO_NO_COMMAND;
	if (Serial.available() > 0) {
		if (receive()) {
			if (execute())
			{
				// Build the response message
				beginResponse(true);
				// Add the sensors status
				sendSensorsStatus();   
				_commandResult = RADIOINO_COMMAND_OK;
			}
			else {
				// Build the error response message
				beginResponse(false);
				_commandResult = RADIOINO_COMMAND_ERROR;
			}
			// wait for another command
			resetCommand();
		}
	}
	// end activity
	if (!_setupMode)
		setActivityLedState(LOW);  
	
	return _commandResult;
}

// Return the latest command execution result
byte Radioino::getCommandResult()
{
	return _commandResult;
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

void Radioino::resetCommand()
{
	_command = "";
	_commandCharIndex = 0;	
}

// Set the activity led on/off
void Radioino::setActivityLedState(bool state)
{
	_activityLedState = state;
	if (_activityLedPin != -1)
		digitalWrite(_activityLedPin,state);	
}

// Build the incoming command in a ascii-string
boolean Radioino::receive()
{
	while (Serial.available() > 0)
	{    
		int value = Serial.read();
		// show activity
		if (!_setupMode)
			setActivityLedState(!_activityLedState);
		switch (value)
		{
			case RADIOINO_COMMAND_END	:
				// Command arrived. Check if we can accept this command command address
				if (_command.startsWith(_startHeader) || _command.startsWith(RADIOINO_SETUP_HEADER) && _setupMode)
				{			
					// Check if is not message to send the latest result again
					if (_command == _startHeader+"C")
					{
						// Send the latest response again
						Serial.println(_response);
						resetCommand();
						return false;
					}
					// It's a regular command. execute-it
					return true;
				}
				else {
					resetCommand();
					return false;
				}
			case 'R'  :
				resetCommand();
				_command = _command + char(value);
				return false;
			default:
				_command = _command + char(value);
				return false;
		}
	}
	return false;
}

boolean Radioino::execute()
{
	String tempAddress = "0000";
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
		case 'S'  :    // Set the module address
			_commandCharIndex++;
			// we need more 4 characteres in setup mode
			if (!_setupMode || (_commandSize - _commandCharIndex <4))
				return false;
			// Set the new address			
			tempAddress[0] = _command.charAt(_commandCharIndex++);        
			tempAddress[1] = _command.charAt(_commandCharIndex++);        
			tempAddress[2] = _command.charAt(_commandCharIndex++);        
			tempAddress[3] = _command.charAt(_commandCharIndex++);        
			setAddress(tempAddress);
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
	
	// End the setup mode, if needed
	if (_setupMode)
	{	
		_setupMode = false;
	}
	
	return true;
}

void Radioino::send(String data)
{
	_response+=data;
	_response+=RADIOINO_SECTION;	
}

void Radioino::sendResponse()
{
	_response += ("CRC" + getCheckSum(_response));
	Serial.println(_response);
}

void Radioino::beginResponse(boolean result)
{
	// Header
	_response="ADR";
	_response+=_address;
	_response+=RADIOINO_SECTION;
	_response+="ACK";
	
	if (!result)
	{
		_response+="ERROR";		
	}
	else {
		_response+="OK";		
	}
	_response+=RADIOINO_SECTION;
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

// Calculates the checksum for a given string
// returns as string
String Radioino::getCheckSum(String value) {
	int i;
	int XOR;
	int c;
	int valueSize = value.length();
	// Calculate checksum ignoring any $'s in the string
	for (XOR = 0, i = 0; i < valueSize; i++) {
		c = (unsigned char)value[i];
		XOR ^= c;
	}
	return String(XOR,HEX);
}
