#ifndef ISO14443A_ANALYZER_H
#define ISO14443A_ANALYZER_H

#include <Analyzer.h>
#include "Iso14443aAnalyzerResults.h"
#include "Iso14443aSimulationDataGenerator.h"

class Iso14443aAnalyzerSettings;
class ANALYZER_EXPORT Iso14443aAnalyzer : public Analyzer2
{
public:
	Iso14443aAnalyzer();
	virtual ~Iso14443aAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< Iso14443aAnalyzerSettings > mSettings;
	std::auto_ptr< Iso14443aAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	Iso14443aSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //ISO14443A_ANALYZER_H
