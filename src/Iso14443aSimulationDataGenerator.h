#ifndef ISO14443A_SIMULATION_DATA_GENERATOR
#define ISO14443A_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class Iso14443aAnalyzerSettings;

class Iso14443aSimulationDataGenerator
{
public:
	Iso14443aSimulationDataGenerator();
	~Iso14443aSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, Iso14443aAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	Iso14443aAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //ISO14443A_SIMULATION_DATA_GENERATOR