#ifndef ISO14443A_SIMULATION_DATA_GENERATOR
#define ISO14443A_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class Iso14443aAskAnalyzerSettings;

class Iso14443aAskSimulationDataGenerator
{
public:
	Iso14443aAskSimulationDataGenerator();
	~Iso14443aAskSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, Iso14443aAskAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	Iso14443aAskAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //ISO14443A_SIMULATION_DATA_GENERATOR