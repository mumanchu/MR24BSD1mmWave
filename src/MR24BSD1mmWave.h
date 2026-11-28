#pragma once

///////////////////////////////////////////////////////////////////////////////
// Arduino Library for Seeed Studio MR24BSD1 mmWave Sleep Monitoring Sensor
// also works for the MicRadar R24BBD1 
// Copyright (C) https://muman.ch and https://github.com/mumanchu, 2026.11.28
// Released under the terms of the BSD 3-Clause License
// If you re-use this software please include the above copyright notice
// 
// For details see
// https://github.com/mumanchu/MR24BSD1mmWave

/*
byte motionAmplitude
====================
(the manual calls this the "signs parameter")
Motion amplitude in percent, 0..100%
Note: The sensor returns a float value, but a float value is not 
needed because the value is 0..100 in steps of 1.
0        = unoccupied
1        = stationary
2..30    = micro movements
31..60   = walking or fast movements
61..100  = running or large close-range movements
*/

// No need for the horrible xxx_t definitions, just use the standards
typedef unsigned char byte;     // always 8 bits
typedef unsigned int uint;      // 16 or 32 bits, all code should work for both!
typedef unsigned long ulong;    // always 32 bits
typedef unsigned short ushort;  // always 16 bits


// For debug messages on the Serial Monitor
// these should normally be defined before #include "MR24BSD1mmWave.h"
#ifndef MR24BSD1_PRINTLN
#ifdef DEBUG
#define MR24BSD1_PRINTLN(s) { Serial.println(s); Serial.flush(); }
#define MR24BSD1_PRINT(s)   { Serial.print(s); Serial.flush(); }
#else
#define MR24BSD1_PRINTLN(s)
#define MR24BSD1_PRINT(s)
#endif
#endif


// CRC-16 calculations in file ModbusCrc16.cpp
extern ushort modbusCrc16(ushort crc, byte b);
extern ushort modbusCrc16(ushort crc, const byte* msg, uint msglen);
extern ushort modbusCrc16(const byte* msg, uint msglen);


class MR24BSD1mmWave
{
	Stream* stream = NULL;      // SoftwareSerial or HardwareSerial

	// for receive state machine receiveByte()
	byte rxMsg[32];             // must accommodate the max message length
	uint rxState;
	uint rxMsgLen;
	uint rxIndex;
	uint rxCrc;

public:
	enum OccupiedState : byte {
		Unoccupied = 0,
		Stationary = 1,
		Moving = 2
	};
	enum MotionState : byte {
		NotInit = 0,
		NoMotion = 1,
		Approaching = 2,
		MovingAway = 3,
		MovingAbout = 4,
		Undoc1 = 5              // ? undocumented
	};
	enum BreathingState : byte {
		Normal = 0,             // still alive
		NotBreathing = 1,       // "abnormal suffocation" !
		NoSignal = 2,           // breathing state not available/unoccupied
		Undoc2 = 3,             // ? undocumented
		Abnormal = 4,           // too much movement to detect breathing
		Rapid = 5               // excercising?
	};
	enum SleepingState : byte {
		Awake = 0,
		LightSleep = 1,
		DeepSleep = 2,
		SleepStateNotAvailable = 3
	};
	enum BedOccupiedState : byte {
		OutOfBed = 0,
		InBed = 1,
		BedStateNotAvailable = 2
	};
	enum SceneSetting : byte {
		Default = 0,
		Area = 1,
		Bathroom = 2,
		Bedroom = 3,
		LivingRomm = 4,
		Office = 5,
		Hotel = 6
	};

	// Use 1-byte structure member packing to minimize memory use
	#pragma pack(push)
	#pragma pack(1)

	// All sensor data is maintained in this structure
	struct MR24BSD1Properties
	{
		bool abnormalReset;             // sensor is initialising
		bool initSuccessful;            // sensor is initialized, after "abnormal reset"
		bool responding;                // sensor is sending messages (5 second timeout)

		// Sensor Readings from report messages
		OccupiedState occupiedState;    // (can also be read with readOccupiedState())
		MotionState motionState;
		BreathingState breathingState;
		byte motionAmplitude;           // 0..100% (can also read with readMotionAmplitude())
		byte breathingRate;             // breaths per minute
		OccupiedState heartbeatState;   // occupied state of creature with heartbeat?
		byte heartbeatRate;             // beats per minute

		//>>> Sleep State Monitoring, requires enableSleepMonitoring(true)
		bool sleepMonitoringEnabled;
		BedOccupiedState bedOccupiedState;
		SleepingState sleepingState;
		// up to 4'294'967'295 minutes of sleep, that should be enough
		ulong minutesAwake;
		ulong minutesOfLightSleep;
		ulong minutesOfDeepSleep;
		// the manual says sleep quality is "combined day's sleep rating", or
		// "sleep scoring (optional, users can rate according to their own way)"
		byte sleepQuality;
		//<<<

		// Sensor information
		// if needed, these must be initialized by the relevant readXXX() commands
		char deviceId[14];              // "96uIag1tfhihh" (examples from my board)
		char firmwareVersion[16];       // "G24VD1SYV010105"
		char hardwareVersion[5];        // "R24A"
		byte protocolVersion;           // 1

		// Settings
		// if needed, these must be initialized by the relevant readXXX() commands
		// 0=none (default), 1=10s, 2=30s, 3=1min, 4=2min, 5=5min, 6=10min, 7=30min, 8=60min
		byte unoccupiedDelay;
		// 0=default, 1=area, 2=bathroom, 3=bedroom, 4=living room, 5=office, 6=hotel
		SceneSetting sceneSetting;
		// 0=not init, 1=minimum sensitivity, 10=maximum sensitivity, default=7
		byte sensitivitySetting;
	};

	#pragma pack(pop)	// back to default structure member packing

	// This structure holds all the sensor's data
	MR24BSD1Properties properties;

	// Callback if occupied state changes, NULL if not used
	void (*occupiedStateChangedCallback)(MR24BSD1mmWave* thisSensor,
		OccupiedState newState, OccupiedState oldState) = NULL;

	// Callback if sleeping state changes, NULL if not used
	void (*sleepingStateChangedCallback)(MR24BSD1mmWave* thisSensor,
		SleepingState newState, SleepingState oldState) = NULL;

	// TODO more notification callbacks could be added here

	// Methods
	MR24BSD1mmWave();

	bool begin(Stream* serialStream);
	void initProperties();
	bool isReady();
	bool reboot();
	bool pollMessage();

	// Read sensor data
	// values are read into the 'properties' structure
	bool readOccupiedState() { return readData(0x03, 0x05); }
	bool readMotionAmplitude() { return readData(0x03, 0x06); }

	// Read sensor information, these are implicit inline methods
	// values are read into the 'properties' structure
	bool readFirmwareVersion() { return readData(0x01, 0x02); }
	bool readHardwareVersion() { return readData(0x01, 0x03); }
	bool readProtocolVersion() { return readData(0x01, 0x04); }
	// note: readDeviceId() waits for 6 seconds, do not use it!
	bool readDeviceId() { return readData(0x01, 0x01); }

	// Read sensor settings
	// values are read into the 'properties' structure
	bool readSleepMonitoring() { return readData(0x05, 0x0D); }
	bool readSensitivitySetting() { return readData(0x04, 0x0C); }
	bool readSceneSetting() { return readData(0x04, 0x10); }
	bool readUnoccupiedDelay() { return readData(0x04, 0x12); }

	// Write sensor settings
	bool writeSensitivitySetting(byte sensitivity);
	bool writeSceneSetting(SceneSetting sceneSetting);
	bool writeUnoccupiedDelay(byte unoccupiedDelay);
	bool enableSleepMonitoring(bool enable);

protected:
	bool processReadCommandResponse();
	bool processReadModuleInfo();
	bool processReadRadarInfo();
	bool processReadParameter();
	bool processOtherFunction();
	bool processProactiveReportMessage();
	bool processSleepReportMessage();
	OccupiedState getOccupiedStateFromRxMsg();
	byte getMotionAmplitudeFromRxMsg();

	bool receiveByte(byte b);
	bool processMessage();
	bool sendMessage(const byte* msg, uint msglen);
	bool sendCommandAndGetResponse(byte* msg, uint msglen);
	bool requestModuleIdentification(byte info);
	bool readData(byte addsCode1, byte addsCode2);
	bool writeSetting(byte parameter, byte value);

	ulong resetTime;		// time begin() was called or <abnormal reset>
	ulong lastRxMsgTime;	// to detect no response
};


// Constructor
MR24BSD1mmWave::MR24BSD1mmWave()
{
	initProperties();
}

// Setup
// serialStream can be SoftwareSerial or HardwareSerial
bool MR24BSD1mmWave::begin(Stream* serialStream)
{
	stream = serialStream;
	resetTime = millis();
	lastRxMsgTime = 0;
	return true;
}

// Initialize the properties structure
// this is called by the constructor
void MR24BSD1mmWave::initProperties()
{
	// set all values to zero
	memset(&properties, 0, sizeof(properties));
	// initialize only the non-zero values
	properties.motionState = NoMotion;
	properties.breathingState = NoSignal;
	properties.sleepingState = SleepStateNotAvailable;
	properties.bedOccupiedState = BedStateNotAvailable;
}

// Determine if the sensor is ready
// waits for the "initialization successful" message
// or a 10 second timeout if the sensor is responding
bool MR24BSD1mmWave::isReady()
{
	// device is ready, do nothing
	if (properties.initSuccessful && properties.responding)
		return true;

	// wait for 'initialization successful' message
	pollMessage();
	if (properties.initSuccessful && properties.responding) {
		MR24BSD1_PRINTLN("\n\r*** Sensor Ready ***");
		return true;
	}

	// if responding for 10 seconds but no <init successful> message, 
	// then assume it's ready (maybe the sensor was not reset)
	if (properties.responding && (millis() - resetTime) >= 10000) {
		properties.initSuccessful = true;
		properties.abnormalReset = false;
		MR24BSD1_PRINTLN("\n\r*** Sensor Ready ***");
		return true;
	}
	return false;
}

//TODO reboot does not work for me, fw version G24VD1SYV010105
bool MR24BSD1mmWave::reboot()
{
	//factory reset?
	//byte cmd[] = { 0x02, 0x05, 0x01 };

	// reboot
	// 55 07 00 02 05 04 1A 8D
	byte cmd[] = { 0x02, 0x05, 0x04 };
	if (!sendMessage(cmd, sizeof(cmd)))
		return false;

	//TODO we should get an "abnormal reset" message
	//TODO followed by "init complete"
	//TODO isReady() should be called

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Write Settings Commands
// settings are stored in nonvolatile memory, you only need to do program
// them once

// 1..10, 1=minimum sensitivity, 10=maximum sensitivity, default=7
bool MR24BSD1mmWave::writeSensitivitySetting(byte sensitivity)
{
	if (sensitivity > 10) {
		MR24BSD1_PRINTLN("invalid sensitivity");
		return false;
	}
	if (!writeSetting(0x0C, sensitivity))
		return false;
	properties.sensitivitySetting = sensitivity;
	return true;
}

// 0=default, 1=area, 2=bathroom, 3=bedroom, 4=living room, 5=office, 6=hovel
bool MR24BSD1mmWave::writeSceneSetting(SceneSetting sceneSetting)
{
	if (!writeSetting(0x10, sceneSetting))
		return false;
	properties.sceneSetting = sceneSetting;
	return true;
}

// 0=none, 1=10s, 2=30s, 3=1min, 4=2min, 5=5min, 6=10min, 7=30min, 8=60min
bool MR24BSD1mmWave::writeUnoccupiedDelay(byte unoccupiedDelay)
{
	if (unoccupiedDelay > 8) {
		MR24BSD1_PRINTLN("invalid unoccupiedDelay");
		return false;
	}
	if (!writeSetting(0x12, unoccupiedDelay))
		return false;
	properties.unoccupiedDelay = unoccupiedDelay;
	return true;
}

// Sleep state monitoring on/off
bool MR24BSD1mmWave::enableSleepMonitoring(bool enable)
{
	byte msg[] = { 0x02, 0x05, 0x0D, 0x00 };
	if (enable)
		msg[3] = 0x01;
	return sendCommandAndGetResponse(msg, sizeof(msg));
}


///////////////////////////////////////////////////////////////////////////////
// Message Processing

// Data in rxMsg : 
//  byte funcCode  = rxMsg[3];
//  byte addsCode1 = rxMsg[4];
//  byte addsCode2 = rxMsg[5];
//  const byte* data = rxMsg + 6;
//  uint msglen = *(ushort*)(rxMsg + 1);
//  uint datalen = msglen - 7;

// Process a message in rxMsg[] according to the Function Code
bool MR24BSD1mmWave::processMessage()
{
	// timestamp of last received message
	lastRxMsgTime = millis();

	byte funcCode = rxMsg[3];
	switch (funcCode) {
	case 0x03:
		return processReadCommandResponse();
	case 0x04:
		return processProactiveReportMessage();
	case 0x05:
		return processSleepReportMessage();
	case 0x06:
		return true;		// undocumented
	default:
		MR24BSD1_PRINTLN("bad funcCode");
		break;
	}
	return false;
}

// Function code 0x03, "Passive reporting of orders"
bool MR24BSD1mmWave::processReadCommandResponse()
{
	byte addsCode1 = rxMsg[4];
	switch (addsCode1) {
	case 0x01:
		return processReadModuleInfo();
	case 0x03:
		return processReadRadarInfo();
	case 0x04:
		return processReadParameter();
	case 0x05:
		return processOtherFunction();
	default:
		MR24BSD1_PRINTLN("bad addsCode1");
		break;
	}
	return false;
}

bool MR24BSD1mmWave::processReadModuleInfo()
{
	// all values are ASCII characters
	const char* data = (char*)(rxMsg + 6);
	uint datalen = rxMsgLen - 7;
	byte addsCode2 = rxMsg[5];

	switch (addsCode2) {
	case 0x01:
		// device id
		// manual says "12 byte data", it's actually 13 chars
		if (datalen != 13)
			MR24BSD1_PRINTLN("bad datalen");
		strncpy(properties.deviceId, data, 13);
		properties.deviceId[13] = '\0';
		break;
	case 0x02:
		// firmware version
		// manual says "10 byte data", it's 15 chars
		if (datalen != 15)
			MR24BSD1_PRINTLN("bad datalen");
		strncpy(properties.firmwareVersion, data, 15);
		properties.firmwareVersion[15] = '\0';
		break;
	case 0x03:
		// hardware version
		// manual says "8 byte data", it's 4 chars
		if (datalen != 4)
			MR24BSD1_PRINTLN("bad datalen");
		strncpy(properties.hardwareVersion, data, 4);
		properties.hardwareVersion[4] = '\0';
		break;
	case 0x04:
		// protocol version
		// manual says "8 byte data", it's 1 byte
		if (datalen != 1)
			MR24BSD1_PRINTLN("bad datalen");
		properties.protocolVersion = rxMsg[6];
		break;
	default:
		MR24BSD1_PRINTLN("bad addsCode2");
		return false;
	}
	return true;
}

// Read radar info command response
bool MR24BSD1mmWave::processReadRadarInfo()
{
	byte addsCode2 = rxMsg[5];
	switch (addsCode2) {
	case 0x05:
		// "environment status"
		properties.occupiedState = getOccupiedStateFromRxMsg();
		break;
	case 0x06: {
		// motion amplitude, 0..100%
		properties.motionAmplitude = getMotionAmplitudeFromRxMsg();
		break;
		}
	default:
		MR24BSD1_PRINTLN("bad addsCode2");
		return false;
	}
	return true;
}

// Read data command response
bool MR24BSD1mmWave::processReadParameter()
{
	byte addsCode2 = rxMsg[5];
	byte b = rxMsg[6];

	switch (addsCode2) {
	case 0x0C:
		// sensitivity, 1..10
		properties.sensitivitySetting = b;
		break;
	case 0x10:
		// scene setting
		// 0=default, 1=area, 2=bathroom, 3=bedroom, 4=living room, 5=office, 6=hovel
		properties.sceneSetting = (SceneSetting)b;
		break;
	case 0x12:
		// unoccupied delay
		// 0=none, 1=10s, 2=30s, 3=1min, 4=2min, 5=5min, 6=10min, 7=30min, 8=60min
		properties.unoccupiedDelay = b;
		break;
	case 0x0D:
		// sleep function switch
		// never occurs, readSleepMonitoring() is handled by processOtherFunction()
		properties.sleepMonitoringEnabled = b;
		break;
	default:
		MR24BSD1_PRINTLN("bad addsCode2");
		return false;
	}
	return true;
}

bool MR24BSD1mmWave::processOtherFunction()
{
	byte addsCode2 = rxMsg[5];
	switch (addsCode2) {
	case 0x0D:
		// sleep function switch
		// this is used for enableSleepMonitoring()
		// AND for the readSleepMonitoring() response
		properties.sleepMonitoringEnabled = rxMsg[6];
		break;
	case 0x08:
		// start OTA upgrade
		// not implemented
		break;
	case 0x09:
		// upgrade package transfer
		// not implemented
		break;
	case 0x0B:
	case 0x0C:
	case 0x0E:
		// undocumented
		// these are returned after an <abnormal reset>
		break;
	default:
		MR24BSD1_PRINTLN("bad addsCode2");
		return false;
	}
	return true;
}

// Function Code 4, "Proactive reporting of commands"
bool MR24BSD1mmWave::processProactiveReportMessage()
{
	byte addsCode1 = rxMsg[4];
	byte addsCode2 = rxMsg[5];

	switch (addsCode1) {
	case 0x03:
		// report radar information
		switch (addsCode2) {
		case 0x05: {
			// "environment status"
			OccupiedState oldState = properties.occupiedState;
			properties.occupiedState = getOccupiedStateFromRxMsg();
			// notify on occupied state change
			if (properties.occupiedState != oldState) {
				if (occupiedStateChangedCallback != NULL) {
					(*occupiedStateChangedCallback)(this, 
						properties.occupiedState, oldState);
				}
			}
			break;
		}
		case 0x06: {
			// motion amplitude, 0..100%, this is returned every second
			properties.motionAmplitude = getMotionAmplitudeFromRxMsg();
			break;
		}
		case 0x07:
			// "approaching away state" = direction state
			// 01 01 01 = none
			// 01 01 02 = approaching
			// 01 01 03 = moving away
			// 01 01 04 = moving about
			// 01 01 05 = ? undocumented
			if (*(ushort*)(rxMsg + 6) != 0x0101 || rxMsg[8] > 5) {
				MR24BSD1_PRINTLN("bad motionState");
				return false;
			}
			properties.motionState = (MotionState)rxMsg[8];
			break;
		default:
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		break;
	case 0x05:
		// report other information
		switch (addsCode2) {
		case 0x01:
			// "hearbeat pack" = heartbeatState
			properties.heartbeatState = getOccupiedStateFromRxMsg();
			break;
		case 0x02:
			// abnormal reset
			// the device reset itself due to a power or an error
			properties.abnormalReset = true;
			resetTime = millis();
			MR24BSD1_PRINTLN("\n\r<abnormal reset>");
			break;
		case 0x0A:
			// initialization successful
			properties.initSuccessful = true;
			properties.abnormalReset = false;
			MR24BSD1_PRINTLN("\n\r<init successful>");
			break;
		case 0x09:
		case 0x0B:
		case 0x0E:
			// these reports are returned after an <abnormal reset>
			// they are undocumented
			return false;
		default:
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		break;
	default:
		MR24BSD1_PRINTLN("bad addsCode1");
		return false;
	}
	return true;
}

// Function Code 5, "Sleeping radar data reporting"
bool MR24BSD1mmWave::processSleepReportMessage()
{
	byte addsCode1 = rxMsg[4];
	byte addsCode2 = rxMsg[5];
	byte b = rxMsg[6];
	ulong value = 0;

	switch (addsCode1) {
	case 0x01:
		// breathing paramaters
		switch (addsCode2) {
		case 0x01:
			// breathing rate
			properties.breathingRate = b;
			MR24BSD1_PRINT("breathingRate=");
			MR24BSD1_PRINTLN(properties.breathingRate);
			break;
		case 0x04:
			// breathing state
			// 0=normal, 1=not breathing ("abnormal suffocation"!), 
			// 2=no signal/unoccupied, 3=undocumented, 4=abnormal movement, 
			// 5=rapid breathing/exercising
			properties.breathingState = (BreathingState)b;
			MR24BSD1_PRINT("breathingState=");
			MR24BSD1_PRINTLN(properties.breathingState);
			break;
		default:
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		break;
	case 0x03:
		// scenario assessment
		switch (addsCode2) {
		case 0x07:
			// in/out of bed
			// 0x00=not in bed, 0x01=in bed, 0x02=bed state not available
			properties.bedOccupiedState = (BedOccupiedState)b;
			break;
		case 0x08: {
			// sleep state
			// 0x00=awake, 0x01=light sleep, 0x02=deep sleep, 0x03=not available
			SleepingState oldState = properties.sleepingState;
			properties.sleepingState = (SleepingState)b;
			// notify on sleeping state state change
			if (oldState != properties.sleepingState) {
				if (sleepingStateChangedCallback != NULL) {
					(*sleepingStateChangedCallback)(this, 
						properties.sleepingState, oldState);
				}
			}
			break;
		}
		default:
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		break;
	case 0x04:
		// duration parameters
		value = *(ulong*)(rxMsg + 6);
		switch (addsCode2) {
		case 0x01:
			// duration of wakefulness
			properties.minutesAwake = value;
			break;
		case 0x02:
			// length of light sleep
			properties.minutesOfLightSleep = value;
			break;
		case 0x03:
			// length of deep sleep
			properties.minutesOfDeepSleep = value;
			break;
		default:
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		break;
	case 0x05:
		// sleep quality parameter
		if (addsCode2 != 0x01) {
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		properties.sleepQuality = b;
		break;
	case 0x06:
		// heart rate monitor
		if (addsCode2 != 0x01) {
			MR24BSD1_PRINTLN("bad addsCode2");
			return false;
		}
		properties.heartbeatRate = b;
		MR24BSD1_PRINT("heartbeatRate=");
		MR24BSD1_PRINTLN(properties.heartbeatRate);
		break;
	default:
		MR24BSD1_PRINTLN("bad addsCode2");
		return false;
	}
	return true;
}

// Convert 3-byte sequence to OccupiedState
MR24BSD1mmWave::OccupiedState MR24BSD1mmWave::getOccupiedStateFromRxMsg()
{
	// 00 ff ff = unoccupied
	// 01 00 ff = stationary
	// 01 01 01 = moving
	ulong value = *((ulong*)(rxMsg + 6)) & 0x00FFFFFF;
	switch (value) {
	case 0xffff00:
		return OccupiedState::Unoccupied;
	case 0xff0001:
		return OccupiedState::Stationary;
	case 0x010101:
		return OccupiedState::Moving;
	}
	MR24BSD1_PRINTLN("bad occupiedState");
	return OccupiedState::Unoccupied;
}

// There's no need for a float value, its value is 0..100 in steps of 1
byte MR24BSD1mmWave::getMotionAmplitudeFromRxMsg()
{
	float f = *(float*)(rxMsg + 6);
	if (f < 0.0f)
		return 0;
	if (f > 100.0f)
		return 100;
	return (byte)(uint)f;
}


///////////////////////////////////////////////////////////////////////////////
// Sending and receiving messages 

// Poll for received messages and processes them
// call this at least 3 times a second, the fastest report rate is every 500ms
// (you can call it faster, it does nothing if no data has been received)
// this processes report messages with processMessage() to update the values
// in the 'properties' structure
bool MR24BSD1mmWave::pollMessage()
{
	while (stream->available()) {
		int b = stream->read();
		if (b < 0) {
			MR24BSD1_PRINTLN("stream->read() failed");
			break;
		}
		properties.responding = true;

		if (receiveByte((byte)b)) {
			processMessage();
			return true;	// true = full message received
		}
	}

	// sensor is not responding
	if (properties.responding) {
		if (millis() - lastRxMsgTime > 5000) {
			MR24BSD1_PRINTLN(" not responding");
			properties.responding = false;
		}
	}
	return false;	// false = no or partial message received
}

// Simple state machine to receive messages
// call this with each successive received character
// returns true when a valid message is received
// the message is buffered in rxMsg ready for processing
// (this method can be tested by supplying a test data sequence)
// 
// Message frame is:
// byte | 0     | 1   2   |  3       | 4         | 5         | 6..len-7  | len-1   |
//      | start | length  | funcCode | addsCode1 | addsCode2 | data...   | crc16   |
//      | 55    | lsb msb | xx       | xx        | xx        | xx...     | lsb msb |
// 
// crc16 is the standard Modbus CRC-16
// 
bool MR24BSD1mmWave::receiveByte(byte b)
{
	#ifdef DEBUG
	// display each byte received, in hex
	if (b == 0x55 && rxState == 0)	// new line at start of message
		MR24BSD1_PRINTLN("");
	char buf[4];
	sprintf(buf, "%02X ", b);
	MR24BSD1_PRINT(buf);
	#endif

	// buffer all received characters
	if (rxState > 0) {
		if (rxIndex == sizeof(rxMsg)) {
			MR24BSD1_PRINTLN("rxMsg overflow");
			rxState = 0;
			return false;
		}
		rxMsg[rxIndex++] = b;
	}

	switch (rxState) {

	// waiting for start of message
	case 0:
		if (b == 0x55) {
			rxState = 1;
			rxMsgLen = 0;
			rxMsg[0] = b;
			rxIndex = 1;
			rxCrc = 0xffff;
		}
		else {
			// character is not part of any message, 
			// it will be ignored
			MR24BSD1_PRINT("? ");
		}
		break;

	// rx message length LS byte
	case 1:
		// max length is size of rxBuf
		if (b >= sizeof(rxMsg)) {
			MR24BSD1_PRINTLN("bad rxMsgLen");
			rxState = 0;
			break;
		}
		rxMsgLen = b;
		rxState = 2;
		break;

	// rx message length MS byte, should always be 0
	case 2:
		if (b != 0) {
			MR24BSD1_PRINTLN("bad rxMsgLen > 255")
			rxState = 0;
			break;
		}
		rxState = 3;
		break;

	// receiving data bytes
	// byte is already stored in rxBuf
	case 3:
		// end of data bytes, receive crc
		if (rxIndex == rxMsgLen - 1)
			rxState = 4;
		break;

	// crc LS byte
	case 4:
		rxCrc = b;
		rxState = 5;
		break;

	// crc MS byte and end of message
	// crc is for all bytes excuding the crc itself
	case 5:
		rxCrc += (ushort)b << 8;
		rxState = 0;
		// verify the crc
		if (modbusCrc16(rxMsg, rxMsgLen - 1) != rxCrc) {
			MR24BSD1_PRINTLN("bad crc");
			break;
		}
		// received message is in rxBuf
		return true;

	default:
		// should never happen, bug!
		MR24BSD1_PRINTLN("unknown rxState");
		rxState = 0;
		break;
	}
	return false;
}

// Send a messsage to the sensor
// use pollMessage() to process the response, see sendCommandAndGetResponse()
bool MR24BSD1mmWave::sendMessage(const byte* msg, uint msglen)
{
	// length is everything after 0x55, including the length and crc bytes
	uint len = 2 + msglen + 2;
	if (len > sizeof(rxMsg)) {
		MR24BSD1_PRINTLN("len > sizeof(rxMsg)");
		return false;
	}

	// calculate the crc-16
	// crc of all bytes including 0x55, excluding the crc itself
	ushort crc = modbusCrc16(0xffff, 0x55);
	crc = modbusCrc16(crc, (byte)len);
	crc = modbusCrc16(crc, 0);				// length MSB is always 0x00
	crc = modbusCrc16(crc, msg, msglen);

	#ifdef DEBUG
	// display the message to be sent
	const char hexfmt[] = "%02X ";
	char buf[5];
	MR24BSD1_PRINT("\n\r>>> 55 ");
	sprintf(buf, hexfmt, (byte)len);
	MR24BSD1_PRINT(buf);
	for (int i = 0; i < msglen; ++i) {
		sprintf(buf, hexfmt, msg[i]);
		MR24BSD1_PRINT(buf);
	}
	sprintf(buf, hexfmt, (byte)crc);
	MR24BSD1_PRINT(buf);
	sprintf(buf, hexfmt, (byte)(crc >> 8));
	MR24BSD1_PRINTLN(buf);
	#endif

	// send the message
	stream->write((byte)0x55);
	stream->write((byte)len);
	stream->write((byte)(len >> 8));
	stream->write(msg, msglen);
	stream->write((byte)crc);
	return stream->write((byte)(crc >> 8)) == 1;
}

// The sensor is sending report messages all the time. 
// This makes it difficult to know if the received message is the response 
// or is a report message. And, the sensor ignores any messages sent to it 
// when it is transmitting, so it must be sent again (retry).
// To be 100% sure that the received message is the correct response to a
// caommand or data request, the functionCode must be 0x03, and addsCode1 
// and addsCode2 must match those of the command message that was sent. 
// If not, up to 2 retries are done.
bool MR24BSD1mmWave::sendCommandAndGetResponse(byte* msg, uint msglen)
{
	// msgid is addsCode1 and addsCode2
	// this identifies the correct response message
	ushort msgidtx = *(ushort*)(msg + 1);
	ushort msgidrx = 0;
	// 2 second response timeout
	uint mstimeout = 2000;

	// special case, readDeviceId() command takes 6 seconds!! Why?
	if (msg[0] == 0x01 && msg[1] == 0x01 && msg[2] == 0x01)
		mstimeout = 7000;

	// 1st try + 2 retries
	for (int tries = 0; tries < 4; ++tries) {

		// abnormal reset while awaiting response, abort, no retries
		if (properties.abnormalReset) {
			MR24BSD1_PRINTLN(" abnormal reset, command abort")
			return false;
		}
		if (!sendMessage(msg, msglen))
			break;
		ulong t = millis();

		while (1) {
			// receive and process the next message, report messages are 
			// processed even if they are not the required response
			if (pollMessage()) {
				byte funcCode = rxMsg[3];
				bool reportMsg = funcCode == 4 || funcCode == 5;

				// sent
				// e.g. 55 06 00 01 >01 04< 00 cc cc
				// expected response
				// e.g. 55 08 00 03 >01 04< 01 8D C9
				// functionCode must be 0x03
				if (rxMsg[3] == 0x03) {
					msgidrx = *(ushort*)(rxMsg + 4);
					if (msgidrx == msgidtx) {
						MR24BSD1_PRINTLN(" valid response"); // not an error
						return true;	// success :-)
					}
				}
				MR24BSD1_PRINTLN(reportMsg ? " report msg" : " wrong msg");
				break;		// retry
			}
			// no response timeout, do a retry
			if ((millis() - t) >= mstimeout) {
				MR24BSD1_PRINTLN(" response timeout");
				break;		// retry
			}
		}
		MR24BSD1_PRINTLN("retry");
	}
	MR24BSD1_PRINTLN(" no response after retries");
	return false;			// failed :-(
}

// Values are read into the 'properties' structure
// see processMessage()
bool MR24BSD1mmWave::readData(byte addsCode1, byte addsCode2)
{
	byte msg[] = { 0x01, 0xff, 0xff, 0x00 };
	msg[1] = addsCode1;
	msg[2] = addsCode2;
	return sendCommandAndGetResponse(msg, sizeof(msg));
}

bool MR24BSD1mmWave::writeSetting(byte parameter, byte value)
{
	byte msg[] = { 0x02, 0x04, 0xff, 0xff };
	msg[2] = parameter;
	msg[3] = value;
	return sendCommandAndGetResponse(msg, sizeof(msg));
}

