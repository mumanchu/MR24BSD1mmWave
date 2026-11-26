///////////////////////////////////////////////////////////////////////////////
// Example Sketch for Arduino Library for Seeed Studio MR24BSD1 mmWave 
// Sleep Monitoring Sensor, or the MicRadar R24BBD1
// Copyright (C) https://muman.ch and https://github.com/mumanchu, 2026.11.26
// Released under the terms of the BSD 3-Clause License
// If you re-use this software please include the above copyright notice
// 
// For details go to
// https://github.com/mumanchu/MR24BSD1mmWave


// Comment this out to remove the debug output and additional validation
#define DEBUG

// The usual 'Serial' is for the Serial Monitor so you can see the data
// 'Serial1' is used for the mmWave sensor's UART, but the pins may be 
// different on each microcontroller board. Here are some examples...

#if defined(ARDUINO_SAM_DUE)
// The Arduino Due has Serial1 on TX1 (18) and RX1 (19), connect these to RX 
// and TX of the sensor
// !!!NOTE!!!  
// If you select "No Optimisation" for the Arduino Due, then the build may
// fail with this message:
//  "HardwareSerial.h:26: undefined reference to vtable for HardwareSerial"
// It's a problem with the board package code. Set it to "Default Optimisation",
// do a 'Clean solution' and re-build. Or use the fix described here:
// https://learn.adafruit.com/proper-step-debugging-atsamd21-arduino-zero-m0/paths-and-optimizations#fixing-some-core-files-2469468-17
#endif

#if defined(ARDUINO_ARCH_STM32)
// On the STM32F103 Nucleo board, 'Serial' is the USB port
// so we use USART1, TX=D8 (PA9), RX=D2 (PA10) for the mmWave sensor
//HardwareSerial Serial1(USART1);
Uart Serial1(USART1);
#endif

#if defined(ARDUINO_ARCH_AVR)
// SoftwareSerial will work at 9600 baud on AVR processors
//TODO
#endif

#if defined(ARDUINO_SAMD_ZERO)
// Arduino Zero has 'Serial1' on pin 0 (RX) and pin 1 (TX), so you don't need 
// to do anything. 'Serial' is the USB port, so connect your Serial Monitor to 
// the port called 'Atmel Corp. EDBG CMSIS-DAP'
#endif

#if defined(ARDUINO_ARCH_ESP32)
// For the ESP32s, Serial1's pins can be defined
// see the code in setup() below
#endif

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
	//Serial.printf("\n\rOccupied status changed, new=%s, old=%s\n\r", 
	//	occupiedStateStrings[newState], occupiedStateStrings[oldState]);
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

	
	// callback notification if occupied state changes
	sensor.occupiedStateChangedCallback = occupiedStateChanged;

	// serial port for the sensor, fixed speed is 9600 bps
	Serial1.begin(9600);

	// start the sensor
	// NOTE! The sensor needs a "warm-up time" of 30 seconds before 
	// you can send or receive valid data, this is handled in loop().
	sensor.begin(&Serial1);

	// when debugging, uncomment this to disable the startup delay
	// after the first power up
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

		static bool sensorConfigured = false;

		// if the sensor has finished initalizing, 
		// then we can configure it and start polling
		if (sensor.isReady()) {

			// when a report message is received from the sensor, 
			// pollMessage() processes it and updates the values in the 
			// sensor.properties structure
			// occupiedStateChanged() is called if the occupied state changes
			sensor.pollMessage();

			// when first ready, do one-time reads and configuration
			// (note that the settings are stored in nonvolatile memory, 
			// so you only need to program them once)
			if (!sensorConfigured) {
				sensorConfigured = true;
				// these determine the communications behaviour
				sensor.readFirmwareVersion();
				sensor.readProtocolVersion();
				// we want to do sleep monitoring too
				//sensor.enableSleepMonitoring(true);
				sensor.readSleepMonitoring();
			}
		}
		else {
			// something nasty happened, the sensor must be re-configured
			// when it's back on line
			sensorConfigured = false;
		}
	}
}

