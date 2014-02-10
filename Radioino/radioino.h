/*
RaduioIno
A wireless-friendly arduino
By Jalf - 2014

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

#ifndef Radioino_h
#define Radioino_h

#include "arduino.h"

#define RADIOINO_ACTIVIY_LED_PIN	13
#define RADIOINO_SETUP_BUTTON_PIN	2	
#define RADIOINO_COMMAND_END		'*'
#define RADIOINO_SECTION			'|'


class Radioino
{
	// Output pin state
	struct PinState
	{
		byte number;
		boolean state;
	};

	public:
		Radioino(String address, byte inputPins[], byte outputPins[], byte analogInputPins[]);
		boolean receive();
		void beginResponse(boolean result);
		void endResponse();
		void send(String data);
		void sendSensorsStatus();		
		boolean execute();
		void loop();
		void setActiviyLedPin(byte pin);
				
		void write(byte pin, boolean value);
		boolean read(byte pin);
	private:
		boolean _setupMode;
		byte _activityLedPin;	// Led for activity
		byte _setupButtonPin;	// Pin with the setup button
		byte *_inputPins;  // Digital INPUT pins
		PinState *_outputPins;  // Digital OUTPUT pins
		byte *_analogInputPins;  // Analogic INPUT pins
		byte _inputPinsCount;	// Digital INPUT pins count	
		byte _outputPinsCount;	// Digital OUTPUT pins count
		byte _analogInputPinsCount;	// Analogic INPUT pins count
		
		String _address;  // Module address
		String _startHeader; // Module address in message header
		
		String _command;  // Incoming command
		String _value; // Command parameter
		int _commandCharIndex;  // Actual command token
		int _commandSize;  // Command size in bytes

		String getInputSensorsStatus();
		void setupPins();
		void waitCommand();
		void toneNotify(int pin);		
		void sendHeader();
		int getNextInt();
};

#endif