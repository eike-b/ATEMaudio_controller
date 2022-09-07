/*
 Name:		ATEMpro.cpp
 Created:	18.01.2022 21:00:19
 Author:	Eike
*/


/* TODO 
*
*	- Ring Buffer zur Verarbeitung von Sende-befehlen?
*
*
*
*/


#include "ATEMpro.h"
/**
 * @brief Returns the corresponding source to an array-index for fairlight sources
 * 
 * @param index 
 * @return int64_t 
 */
int64_t ATEMpro::getFairlightSourceByIndex(uint8_t index) {
	switch (index)
	{
	case 0:
		return -65280;
		break;
	case 1:
		return -256;
		break;
	case 2:
		return -255;
		break;
	}
}

/**
 * @brief Returns an index for a fairlight source
 * 
 * @param source 
 * @return uint8_t 
 */
// this is needet because of the negative fairlight sources
// these cannot be array index
uint8_t ATEMpro::getIndexByFairlightSource(int64_t source) {
	switch (source)
	{
	case -65280:
		return 0;
		break;
	case -256:
		return 1;
		break;
	case -255:
		return 2;
		break;
	}
}

/**
 * @brief Gives the corresponding channel to an array-index
 * 
 * @param index 
 * @return uint16_t 
 */

uint16_t ATEMpro::getFairlightChannelByIndex(uint8_t index) {
	switch (index)
	{
	case 0:
		return 1;
		break;
	case 1:
		return 2;
		break;
	case 2:
		return 3;
		break;
	case 3:
		return 4;
		break;
	case 4:
		return 1301;
		break;
	case 5:
		return 1302;
		break;
	}
}

/**
 * @brief Gives an array-index for a fairlight channel/source
 * 
 * @param source 
 * @return uint8_t 
 */

// this is needed because of the crazy high channel numbers in the ATEM
// otherwise the array would get like 1301 entries big.. 
uint8_t ATEMpro::getIndexByFairlightChannel(uint16_t source) {
	switch (source)
	{
	case 1:
		return 0;
		break;
	case 2:
		return 1;
		break;
	case 3:
		return 2;
		break;
	case 4:
		return 3;
		break;
	case 1301:
		return 4;
		break;
	case 1302:
		return 5;
		break;
	}
}



bool ATEMpro::getFairlightTally(uint8_t input, int64_t source) {
	return fairlightTally[getIndexByFairlightChannel(input)][getIndexByFairlightSource(source)];
}

void ATEMpro::_parseGetCommands(const char* cmdStr) {
	_readToPacketBuffer();	// read all bytes from packet to buffer

	if(_serialOutput) {
		Serial.print("Command: ");
		Serial.println(cmdStr);
		Serial.print("Command Length: ");
		Serial.println(_cmdLength);
	}
	
	// we get the command string via method-params so we can and should
	// check for each command individually by comparing strings (if 0 -> they're equal)

	// TODO vervollständigen
	if (!strcmp_P(cmdStr, PSTR("FASP"))) { // fairlight audio source properties
		uint16_t channel = (uint16_t)word(_packetBuffer[0], _packetBuffer[1]);	// get the channel and store temporarily
		uint8_t channelIndex = getAudioSrcIndex(channel); //

		maxFramesDelay[channelIndex] = (uint8_t)_packetBuffer[17];
		framesDelay[channelIndex] = (uint8_t)_packetBuffer[18];
		balance[channelIndex] = (uint16_t)word(_packetBuffer[40], _packetBuffer[41]);
	}

	// parsing the tally information from mixer
	if (!strcmp_P(cmdStr, PSTR("FMTl"))) { // fairlight mixer tally
		uint16_t countTallys = (uint16_t)word(_packetBuffer[0], _packetBuffer[1]); // number of following tally commands

		for(uint8_t i = 0 ; i < countTallys ; i++) { // looping the amount of tally commands

			uint16_t input = (uint16_t)word(_packetBuffer[16 + (i*11)], _packetBuffer[17 + (i*11)]); // read the fairlight input
			int64_t source = -65280; //TODO read the fairlight source, currently only "full" sources meaning no channel split identificatin possilble
			fairlightTally[getIndexByFairlightChannel(input)][getIndexByFairlightSource(source)] = (bool)word(_packetBuffer[18 + (i*11)]); // save the values in array
			
			// serial debug
			if(_serialOutput) {
				char buf[50];
				sprintf(buf, "%ld", source);
				Serial.print("Tally Channel ");
				Serial.print(input);
				Serial.print(" and Source ");
				Serial.print(buf);
				Serial.print(" ");
				Serial.println(getFairlightTally(input, source));
			}
		}

	}
}

/**
 * @brief Main function to send ATEM fairlight channel changes
 * 
 * @param flag 
 * @param channel 
 * @param source 
 * @param framesDelay 
 * @param gain 
 * @param stereoSimulation 
 * @param equalizerEnabled 
 * @param equalizerGain 
 * @param makeupGain 
 * @param balance 
 * @param faderGain 
 * @param mixOption 
 */
void ATEMpro::changeFairlightSourceProperties(uint16_t flag, uint16_t channel, int64_t source, uint8_t framesDelay, int32_t gain, uint16_t stereoSimulation, uint8_t equalizerEnabled, int32_t equalizerGain, int32_t makeupGain, int16_t balance, int32_t faderGain, uint8_t mixOption)
{
	/*	changesFlags:
	*	----------------
	*	framesDelay:	1
	*	gain:			2
	*	stereoSim.:		4
	*	equalizerEna.:	8
	*	equalizerGain:	16
	*	makeupGain:		32
	*	balance:		64
	*	faderGain:		128
	*	mixOption:		256 
	*/

	/*	sources
	*	----------------
	*	single input: 	-65280
	*	splitted input:	-256, -255
	*/

	_prepareCommandPacket(
		PSTR("CFSP"),	// set CommandString
		48				// set CommandLength in Byte
	);


	_packetBuffer[12 + _cBBO + 4 + 4 + 0] = highByte(flag);	// changes flag msb
	_packetBuffer[12 + _cBBO + 4 + 4 + 1] = lowByte(flag);	// changes flag lsb

	_packetBuffer[12 + _cBBO + 4 + 4 + 2] = highByte(channel);	// channel msb
	_packetBuffer[12 + _cBBO + 4 + 4 + 3] = lowByte(channel);	// channel lsb

	// fairlight source.. getting hex bytes to send from an integer number
	// seems to work, even the crazy bitshifting by 512 
	byte byte1 = (source >> 512) & 0xFF;
	byte byte2 = (source >> 256) & 0xFF;
	byte byte3 = (source >> 128) & 0xFF;
	byte byte4 = (source >> 64) & 0xFF;
	byte byte5 = (source >> 32) & 0xFF;
	byte byte6 = (source >> 16) & 0xFF;
	byte byte7 = (source >> 8) & 0xFF;
	byte byte8 = source & 0xFF;

	_packetBuffer[12 + _cBBO + 4 + 4 + 8] = byte1; 
	_packetBuffer[12 + _cBBO + 4 + 4 + 9] = byte2;
	_packetBuffer[12 + _cBBO + 4 + 4 + 10] = byte3;
	_packetBuffer[12 + _cBBO + 4 + 4 + 11] = byte4;
	_packetBuffer[12 + _cBBO + 4 + 4 + 12] = byte5;
	_packetBuffer[12 + _cBBO + 4 + 4 + 13] = byte6;
	_packetBuffer[12 + _cBBO + 4 + 4 + 14] = byte7;
	_packetBuffer[12 + _cBBO + 4 + 4 + 15] = byte8;

	_packetBuffer[12 + _cBBO + 4 + 4 + 16] = framesDelay; // framesDelay

	// This beautiful part here uses bitshifting and masking to get
	// single parts of the gain value like msb, lsb and all bits inbetween
	_packetBuffer[12 + _cBBO + 4 + 4 + 20] = (gain >> 32) & 0xFF;	// gain
	_packetBuffer[12 + _cBBO + 4 + 4 + 21] = (gain >> 16) & 0xFF;	// gain
	_packetBuffer[12 + _cBBO + 4 + 4 + 22] = (gain >> 8) & 0xFF;	// gain
	_packetBuffer[12 + _cBBO + 4 + 4 + 23] = gain & 0xFF;			// gain

	// I think this param won't work for all ATEMs
	_packetBuffer[12 + _cBBO + 4 + 4 + 24] = highByte(stereoSimulation);	// stereoSimulation
	_packetBuffer[12 + _cBBO + 4 + 4 + 25] = lowByte(stereoSimulation);		// stereoSimulation

	_packetBuffer[12 + _cBBO + 4 + 4 + 26] = equalizerEnabled;	// EQ enabled/disabled 
		
	_packetBuffer[12 + _cBBO + 4 + 4 + 28] = (equalizerGain >> 32) & 0xFF;	// equalizerGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 29] = (equalizerGain >> 16) & 0xFF;	// equalizerGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 30] = (equalizerGain >> 8) & 0xFF;	// equalizerGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 31] = equalizerGain & 0xFF;			// equalizerGain

	_packetBuffer[12 + _cBBO + 4 + 4 + 32] = (makeupGain >> 32) & 0xFF;	// makeupGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 33] = (makeupGain >> 16) & 0xFF;	// makeupGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 34] = (makeupGain >> 8) & 0xFF;	// makeupGain
	_packetBuffer[12 + _cBBO + 4 + 4 + 35] = makeupGain & 0xFF;			// makeupGain

	_packetBuffer[12 + _cBBO + 4 + 4 + 36] = highByte(balance);		// balance
	_packetBuffer[12 + _cBBO + 4 + 4 + 37] = lowByte(balance);		// balance

	_packetBuffer[12 + _cBBO + 4 + 4 + 40] = (faderGain >> 32) & 0xFF;	// faderGain or Volume Bit4
	_packetBuffer[12 + _cBBO + 4 + 4 + 41] = (faderGain >> 16) & 0xFF;	// faderGain or Volume Bit3
	_packetBuffer[12 + _cBBO + 4 + 4 + 42] = (faderGain >> 8) & 0xFF;	// faderGain or Volume Bit2
	_packetBuffer[12 + _cBBO + 4 + 4 + 43] = faderGain & 0xFF;			// faderGain or Volume Bit1

	_packetBuffer[12 + _cBBO + 4 + 4 + 44] = mixOption;	// mixOption -> mute/live/afv

	_finishCommandPacket();
}

/**
 * @brief Sets a delay for a fairlight channel
 * 
 * @param channel as integer
 * @param framesDelay delay in frames
 * 
 * // TODO Source muss noch mit rein
 * 
 * helper function to simplify the big fasp command
 */
void ATEMpro::setFramesDelay(uint16_t channel, uint8_t framesDelay) {
	changeFairlightSourceProperties(1, channel, 0xFFFFFFFFFFFFFF00, framesDelay, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 * @brief Sets the input gain of a fairlight channel/source
 * 
 * @param channel 
 * @param source 
 * @param gain 
 */
void ATEMpro::setGain(uint16_t channel, int64_t source, float gain) {
	int32_t g = (gain * 106 - 100) * 100; // this gives a range from -100 to +6 (dB)
	changeFairlightSourceProperties(2, channel, source, 0, g, 0, 0, 0, 0, 0, 0, 0);
}

void ATEMpro::setStereoSimulation(uint16_t channel, float stereoSimulation) {
}

void ATEMpro::setAudioFollowsVideo(uint16_t channel, bool afv) {
}

void ATEMpro::setEqualizerEnabled(uint16_t channel, bool equalizerEnabled) {
}

void ATEMpro::setEqualizerGain(uint16_t channel, float equalizerGain) {
}

void ATEMpro::setMakeupGain(uint16_t channel, float makeupGain) {
}

void ATEMpro::setBalance(uint16_t channel, float balance) {
}

/**
 * @brief Setting the fader gain/volume of a fairlight channel/source
 * 
 * @param channel 
 * @param source 
 * @param faderGain 
 */
void ATEMpro::setFaderGain(uint16_t channel, int64_t source, float faderGain) {
	int32_t fg = (faderGain * 100 - 90) * 100; // this gives a range from -90 to +10 (dB)
	changeFairlightSourceProperties(128, channel, source, 0, 0, 0, 0, 0, 0, 0, fg, 0);
}

/**
 * @brief Setting a channel/source live or mute/off air
 * 
 * @param channel 
 * @param source 
 * @param mute 
 */
void ATEMpro::setMute(uint16_t channel, int64_t source, bool mute) {
	// if mute enabled, set mixOption to 1 => channel off
	// else set mixOption 2 => channel on
	uint8_t mo;
	if (mute) {
		mo = 1;
	}
	else {
		mo = 2;
	}
	changeFairlightSourceProperties(256, channel, source, 0, 0, 0, 0, 0, 0, 0, 0, mo);
}
