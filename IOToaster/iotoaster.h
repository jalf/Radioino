/*
IOToaster
A wifi-friendly Arduino
By Jalf - 2015

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef IOToaster_h
#define IOToaster_h

#include <arduino.h>

#define IOTOASTER_SECTION				'|'
#define IOTOASTER_COMMAND_OK			1
#define IOTOASTER_COMMAND_ERROR			2
#define IOTOASTER_NO_COMMAND			0
#define IOTOASTER_LED_PIN				13
#define IOTOASTER_SETUP_LED_BLINK_MS	500
#define IOTOASTER_RETRY_SERIAL			10


class IOToaster
{
	// Output pin state
	struct PinState
	{
		byte number;
		boolean state;
	};

public:
	IOToaster(byte inputPins[], byte outputPins[], byte analogInputPins[]);

	boolean sendResponse();
	void send(String data);
	void send(char data);
	byte receiveCommand();
	byte getCommandResult();
	void setActiviyLedPin(byte pin);
	void write(byte pin, boolean value);
	boolean read(byte pin);
	void setCustomCommandCallback(boolean(*customCommandCallBack)(char, int));
	boolean reset();
	void setup();
private:
	String _response;	// response string
	String _customResponse;	// custom-data response string
	String _command;  // Incoming command
	String _value; // Command parameter
	int _commandCharIndex;  // Actual command token
	int _commandSize;  // Command size in bytes
	byte _commandResult; // Actual command result
	boolean _setupMode;	// Module is in setup mode?
	boolean _activityLedState; // status led state
	unsigned long _previousTime;

	String _serverIp;
	int _serverPort;

	byte *_inputPins;  // Digital INPUT pins
	PinState *_outputPins;  // Digital OUTPUT pins
	byte *_analogInputPins;  // Analogic INPUT pins
	byte _inputPinsCount;	// Digital INPUT pins count	
	byte _outputPinsCount;	// Digital OUTPUT pins count
	byte _analogInputPinsCount;	// Analogic INPUT pins count

	void sendSensorsStatus();
	boolean execute();
	boolean receive();
	void beginResponse(boolean result);
	String getInputSensorsStatus();
	void setupPins();
	void resetCommand();
	void connectServer();
	boolean waitForSerial(char *data); // Wait for a serial response
	void createServer();  // Enter in AP mode and create the server for configuration

	boolean(*_customCommandCallBack)(char, int);  // Custom command callback
	int getNextInt();	// Get the next int-value from the incoming command
	String getCheckSum(String value); // Calculate the message CRC

	// Config functions
	void setConfiguration(int port, String address);
	void loadConfiguration();
	void eeprom_read_bytes(int startAddr, byte array[], int numBytes);
	void eeprom_write_bytes(int startAddr, const byte* array, int numBytes);
	void eeprom_reset();
	bool isConfigured();
	void setActivityLedState(bool state);
};

#endif