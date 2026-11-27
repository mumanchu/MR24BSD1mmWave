///////////////////////////////////////////////////////////////////////////////
// Example Sketch for Arduino Library for Seeed Studio MR24BSD1 mmWave 
// Sleep Monitoring Sensor, also works for the MicRadar R24BBD1
// Copyright (C) https://muman.ch + https://github.com/mumanchu, 2026.11.27
// Released under the terms of the BSD 3-Clause License
// If you re-use this software please include the above copyright notice
// 
// For details go to
// https://github.com/mumanchu/MR24BSD1mmWave


// Add the SoftwareSerial library
#include <SoftwareSerial.h>

// The example uses SoftwareSerial on D2 and D8
// change the pin numbers according to your wiring and your MCU
SoftwareSerial SwSerial(2 /*rxPin*/, 8 /*txPin*/);


// Comment this out to remove the debug output and additional validation
// and reduces the code size by about 3000 bytes
#define DEBUG

// For MR24BSD1 debug messages on the Serial Monitor
#ifdef DEBUG
#define MR24BSD1_PRINTLN(s) { Serial.println(s); Serial.flush(); }
#define MR24BSD1_PRINT(s)   { Serial.print(s); Serial.flush(); }
#else
#define MR24BSD1_PRINTLN(s)
#define MR24BSD1_PRINT(s)
#endif

#include "MR24BSD1mmWave.h"
MR24BSD1mmWave sensor;


// Callback notification if the occupied state changes
void occupiedStateChanged(MR24BSD1mmWave* thisSensor,
	MR24BSD1mmWave::OccupiedState newState, 
	MR24BSD1mmWave::OccupiedState oldState)
{
	const char* occupiedStateStrings[] { "Unoccupied", "Stationary", "Moving" };
	Serial.print("\n\rOccupied status changed, new=");
	Serial.print(occupiedStateStrings[newState]);
	Serial.print(", old=");
	Serial.println(occupiedStateStrings[oldState]);
	Serial.flush();
}


// Get ready
void setup() 
{
	// Use different pins for Serial logging on the STM32 Nucleo-64 boards,
	// else Serial is the USB port which is also used for download
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	// 'Serial' is the port for the Serial Monitor
	Serial.begin(115200);
	delay(1000);

	// PuTTY clear screen and scrollback
	// comment this out if you're not using PuTTY
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\r\nMR24BSD1mmWave Started...\r\n");
	Serial.flush();

	// serial port for the mmWave sensor
	SwSerial.begin(9600);

	// callback notification if occupied state changes
	sensor.occupiedStateChangedCallback = occupiedStateChanged;

	// start the mmWave sensor
	// NOTE! The sensor needs a warm-up time before you can send or 
	// receive valid data, this is handled in loop()
	sensor.begin(&SwSerial);

	// when debugging, uncomment this to disable the startup delay
	// if the sensor does not get a power up reset each time
	//sensor.properties.initSuccessful = true;
}


// Go!
void loop()
{
	ulong t = millis();

	// poll the sensor from loop() at least every 200ms
	// if slower, then data may be lost and communications errors may occur
	static ulong t1 = 0;
	if (t - t1 >= 200) {
		t1 = t;

		// if the sensor has finished initalizing, then we can read data 
		// or configure it, and start polling
		if (sensor.isReady()) {

			// when a report message is received from the sensor, 
			// pollMessage() processes it and updates the values in the 
			// sensor.properties structure
			// occupiedStateChanged() is called if the occupied state changes
			sensor.pollMessage();

			// when first ready, do one-time reads and configuration
			// (note that the settings are stored in nonvolatile memory, 
			// so you only need to program them once)
			static bool sensorConfigured = false;
			if (!sensorConfigured) {
				sensorConfigured = true;

				// these can be used to determine communications behaviour
				sensor.readFirmwareVersion();
				sensor.readProtocolVersion();
				// we want to do sleep monitoring too
				//sensor.enableSleepMonitoring(true);	// do this once
				sensor.readSleepMonitoring();
			}
		}
	}
}
