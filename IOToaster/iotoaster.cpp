#include "iotoaster.h"
#include <avr/eeprom.h>

IOToaster::IOToaster(byte inputPins[], byte outputPins[], byte analogInputPins[])
{
	_inputPinsCount = inputPins[0];
	_outputPinsCount = outputPins[0];
	_analogInputPinsCount = analogInputPins[0];
	_inputPins = inputPins + sizeof(byte);

	_previousTime = 0;

	// Setup output pins state array
	_outputPins = new PinState[_outputPinsCount];
	for (int i = 0; i < _outputPinsCount; i++)
	{
		_outputPins[i].number = outputPins[i + 1]; // The first by is the count
	}

	_analogInputPins = analogInputPins + sizeof(byte);

	// Alloc strings
	_command.reserve(50);
	_response.reserve(255);
	_customResponse.reserve(255);
	_value.reserve(3);
}

void IOToaster::setup()
{
	// Set the pin configuration
	setupPins();

	// Open serial communication
	Serial.begin(9600);
	Serial.setTimeout(5000);	

	// Load the configuration
	if (isConfigured())
	{
		// Normal mode
		_setupMode = false;
		loadConfiguration();	
		connectServer();
		setActivityLedState(HIGH);		
	}
		
	else {
		// Setup mode
		setActivityLedState(LOW);
		_setupMode = true;		
		createServer();
	}
}

void IOToaster::createServer()
{
	Serial.print("AT+CWMODE=2\r\n"); // AP Mode
	Serial.print("AT+CWSAP=\"iotoaster\",\"\",1,0\r\n"); // Create AP
	delay(500);
	Serial.print("AT+CIPMUX=1\r\n"); // Enable Multiple connections
	delay(500);
	Serial.print("AT+CIPSERVER=1,80"); // Start the server
	delay(1000);
}

void IOToaster::connectServer()
{
	Serial.print("AT+CIPMUX=0\r\n");
	delay(500);
	Serial.print("AT+CIPSTART=0,\"TCP\",\"");
	Serial.print(_serverIp);
	Serial.print("\",");
	Serial.print(_serverPort);
	Serial.print("\r\n");
	delay(1000);
}

void IOToaster::setCustomCommandCallback(boolean(*customCommandCallBack)(char, int))
{
	_customCommandCallBack = customCommandCallBack;
}

byte IOToaster::receiveCommand()
{
	if (_setupMode)
	{
		// check to see if it's time to blink the LED; that is, if the 
		// difference between the current time and last time you blinked 
		// the LED is bigger than the interval at which you want to 
		// blink the LED.
		unsigned long currentTime = millis();
		if (currentTime - _previousTime > IOTOASTER_SETUP_LED_BLINK_MS) {
			// save the last time you blinked the LED 
			_previousTime = currentTime;

			// if the LED is off turn it on and vice-versa:
			if (_activityLedState == LOW)
				setActivityLedState(HIGH);
			else setActivityLedState(LOW);
		}
	}

	//Wait for a new command
	_commandResult = IOTOASTER_NO_COMMAND;
	if (Serial.available() > 0) {
		if (receive()) {
			if (_command.startsWith("+IPD")) // Check for data sent from the server
			{
				_command = _command.substring(_command.lastIndexOf(':') + 1);
				_customResponse = "";
				if (execute())
				{
					// Build the response message
					beginResponse(true);
					// Add the sensors status
					send(getInputSensorsStatus());
					_commandResult = IOTOASTER_COMMAND_OK;
				}
				else {
					// Build the error response message
					beginResponse(false);
					_commandResult = IOTOASTER_COMMAND_ERROR;
				}
			}
			// wait for another command
			resetCommand();
		}
	}

	return _commandResult;
}

// Return the latest command execution result
byte IOToaster::getCommandResult()
{
	return _commandResult;
}


// Write a HIGH or a LOW value to an digital pin.
void IOToaster::write(byte pin, boolean value)
{
	// Check if the pin is an output pin
	for (int i = 0; i < _outputPinsCount; i++)
	{
		if (_outputPins[i].number == pin)
		{
			digitalWrite(pin, value);
			_outputPins[i].state = value;
			break;
		}
	}
}

// Read a HIGH or a LOW value from an digital pin.
boolean IOToaster::read(byte pin)
{
	// Check if the pin is an output pin
	for (int i = 0; i < _outputPinsCount; i++)
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
void IOToaster::setupPins()
{
	// Set the input pins
	for (unsigned int i = 0; i < sizeof(_inputPins); i++)
	{
		pinMode(_inputPins[i], INPUT);
	}

	// Set the output pins
	for (int i = 0; i < _outputPinsCount; i++)
	{
		pinMode(_outputPins[i].number, OUTPUT);
		_outputPins[i].state = LOW;
		digitalWrite(_outputPins[i].number, LOW);
	}

	// Led pin
	pinMode(IOTOASTER_LED_PIN, OUTPUT);	
	digitalWrite(IOTOASTER_LED_PIN, LOW);
}

void IOToaster::resetCommand()
{
	_command = "";
	_commandCharIndex = 0;
}

// Build the incoming command in a ascii-string
boolean IOToaster::receive()
{
	while (Serial.available() > 0) {
		char inChar = Serial.read();
		_command += inChar;
		if (_command.endsWith("\r\n"))
		{
			_command.trim();
			return true;
		}
	}
	return false;
}

boolean IOToaster::execute()
{
	int inByte;
	char customCommand;

	_commandCharIndex = 5; // just after command header
	_commandSize = _command.length();
	while (_commandCharIndex < _commandSize) {
		switch (_command.charAt(_commandCharIndex)){
		case 'H':    // Activate a digital output         
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();
			write(inByte, HIGH);
			break;
		case 'L':    // De-activate a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();
			write(inByte, LOW);
			break;
		case 'X':    // invert a digital output
			_commandCharIndex++;
			// Get the param
			inByte = getNextInt();
			if (read(inByte) == LOW)
				write(inByte, HIGH);
			else write(inByte, LOW);
			break;
		default:
			// Custom command
			if (_customCommandCallBack != NULL)
			{
				customCommand = _command.charAt(_commandCharIndex);
				_commandCharIndex++;
				inByte = getNextInt();
				if (!_customCommandCallBack(customCommand, inByte))
					return false;
			}
			else return false;
		}
	}

	return true;
}

void IOToaster::send(String data)
{
	_customResponse += data;
	_customResponse += IOTOASTER_SECTION;
}

void IOToaster::send(char data)
{
	_customResponse += data;
	_customResponse += IOTOASTER_SECTION;
}

boolean IOToaster::waitForSerial(char *data)
{
	int count = IOTOASTER_RETRY_SERIAL;
	while (count > 0)
	{
		if (Serial.find(data))
		{
			return true;
		}
		else {
			delay(100);
			count--;
		}
	}
}

boolean IOToaster::sendResponse()
{
	_response += _customResponse;
	_response += ("CRC" + getCheckSum(_response));

	Serial.print("AT+CIPSEND=0,");
	Serial.print(_response.length());
	Serial.print("\r\n");
	if (waitForSerial(">")){        // wait for > to send data.
		Serial.print(_response);
		return (waitForSerial("OK"));
	}
	else return false;
}

void IOToaster::beginResponse(boolean result)
{
	// Header
	_response = "ACK";

	if (!result)
	{
		_response += "ERROR";
	}
	else {
		_response += "OK";
	}
	_response += IOTOASTER_SECTION;
}

int IOToaster::getNextInt()
{
	_value = "";
	while (_command.charAt(_commandCharIndex) <= '9' && _command.charAt(_commandCharIndex) >= '0'
		&& _commandCharIndex < _commandSize)
	{
		_value = _value + _command.charAt(_commandCharIndex++);
	}
	return _value.toInt();
}

String IOToaster::getInputSensorsStatus()
{
	String result = "IND";
	int value;
	// Add the input digital pins
	for (int i = 0; i < _inputPinsCount; i++)
	{
		value = read(_inputPins[i]);
		result = result + _inputPins[i] + ";" + value + ";";
	}
	// Add the output digital pins
	result = result += "|OUD";
	for (int i = 0; i < _outputPinsCount; i++)
	{
		value = _outputPins[i].state;
		result = result + _outputPins[i].number + ";" + value + ";";
	}

	// Add the analogic pins
	result = result + "|INA";
	for (int i = 0; i < _analogInputPinsCount; i++)
	{
		value = analogRead(_analogInputPins[i]);
		result = result + _analogInputPins[i] + ";" + value + ";";
	}

	return result;
}

// Calculates the checksum for a given string
// returns as string
String IOToaster::getCheckSum(String value) {
	int i;
	int XOR;
	int c;
	int valueSize = value.length();
	// Calculate checksum
	for (XOR = 0, i = 0; i < valueSize; i++) {
		c = (unsigned char)value[i];
		XOR ^= c;
	}
	return String(XOR, HEX);
}

// Soft-reset wifi module
boolean IOToaster::reset()
{
	Serial.print("AT+RST\r\n");
	delay(1000);
	return (Serial.find("ready"));
}

/********************* EEPROM/Config stuff *********************/

bool IOToaster::isConfigured()
{
	return (eeprom_read_byte((unsigned char *)0) == 'O' && eeprom_read_byte((unsigned char *)1) == 'K');
}

void IOToaster::setConfiguration(int port, String address)
{
	_serverPort = port;
	_serverIp = address;

	// Write the config mask
	eeprom_write_byte((unsigned char *)0, 'O');
	eeprom_write_byte((unsigned char *)1, 'K');

	// Server Port
	eeprom_write_bytes(2, (byte *) &port, sizeof(int));

	// Server Address
	int addr = sizeof(int) + 3;
	for (int i = 0; i < address.length(); i++)
	{
		eeprom_write_byte((unsigned char *)addr, address[i]);
		addr++;
	}
	eeprom_write_byte((unsigned char *)addr, 0);  // string end
}

void IOToaster::loadConfiguration()
{
	// Server port
	eeprom_read_bytes(2, (byte *)&_serverPort, sizeof(int));
	// Server Address
	_serverIp = "";
	int addr = sizeof(int) + 3;
	char c;
	do {
		c = eeprom_read_byte((unsigned char *)addr++);
		if (c != 0)
			_serverIp += c;
	} while (c != 0 && addr < 50);
}

void IOToaster::eeprom_read_bytes(int startAddr, byte data[], int numBytes)
{
	for (int i = 0; i < numBytes; i++)
		data[i] = eeprom_read_byte((unsigned char *)startAddr + i);
}

void IOToaster::eeprom_write_bytes(int startAddr, const byte* data, int numBytes)
{
	for (int i = 0; i < numBytes; i++)
		eeprom_write_byte((unsigned char *)startAddr+i, data[i]);
}

void IOToaster::eeprom_reset()
{
	for (int i = 0; i < 100; i++)
		eeprom_write_byte((unsigned char *)i, 0);
}

// Set the activity led on/off
void IOToaster::setActivityLedState(bool state)
{
	_activityLedState = state;
	digitalWrite(IOTOASTER_LED_PIN, state);
}

