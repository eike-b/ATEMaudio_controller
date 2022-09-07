/*
 Name:		ATEMpro.h
 Created:	18.01.2022 21:00:19
 Author:	Eike

*/

#ifndef ATEMpro_h
#define ATEMpro_h

#include <ATEMbase.h>
#include <ATEMstd.h>


#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class ATEMpro : public ATEMstd
{
private:
	void _parseGetCommands(const char* cmdStr); // overloading the method from ATEMbase
	int64_t getFairlightSourceByIndex(uint8_t);
	uint8_t getIndexByFairlightSource(int64_t);
	uint16_t getFairlightChannelByIndex(uint8_t index);
	uint8_t getIndexByFairlightChannel(uint16_t source);
	


	// variables needed to hold status information
	
	uint8_t framesDelay[30];
	uint8_t maxFramesDelay[30];
	int32_t gain[30];
	uint16_t stereoSimulation[30];
	uint8_t equalizerEnabled[30];
	int32_t equalizerGain[30];
	int32_t makeupGain[30];
	int16_t balance[30];
	int32_t faderGain[30];
	uint8_t mixOption[30];
	bool fairlightTally[30][30];


public:
	void changeFairlightSourceProperties(
		uint16_t flag,
		uint16_t channel,
		int64_t source,
		uint8_t framesDelay,
		int32_t gain,
		uint16_t stereoSimulation,
		uint8_t equalizerEnabled,
		int32_t equalizerGain,
		int32_t makeupGain,
		int16_t balance,
		int32_t faderGain,
		uint8_t mixOption
	);
	void setFramesDelay(uint16_t channel, uint8_t framesDelay);
	void setGain(uint16_t channel, int64_t source, float gain);
	void setStereoSimulation(uint16_t channel, float stereoSimulation);
	void setEqualizerEnabled(uint16_t channel, bool equalizerEnabled);
	void setEqualizerGain(uint16_t channel, float equalizerGain);
	void setMakeupGain(uint16_t channel, float makeupGain);
	void setBalance(uint16_t channel, float balance);
	void setFaderGain(uint16_t channel, int64_t source, float faderGain);
	void setMute(uint16_t channel, int64_t source, bool mute);
	void setAudioFollowsVideo(uint16_t channel, bool afv);
	
	bool getFairlightTally(uint8_t input, int64_t source);
};

#endif

