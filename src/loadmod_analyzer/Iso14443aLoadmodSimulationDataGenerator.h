#ifndef ISO14443A_LOADMOD_SIMULATION_DATA_GENERATOR
#define ISO14443A_LOADMOD_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class Iso14443aLoadmodAnalyzerSettings;

class Iso14443aLoadmodSimulationDataGenerator
{
public:
	Iso14443aLoadmodSimulationDataGenerator();
	~Iso14443aLoadmodSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, Iso14443aLoadmodAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	Iso14443aLoadmodAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //ISO14443A_LOADMOD_SIMULATION_DATA_GENERATOR