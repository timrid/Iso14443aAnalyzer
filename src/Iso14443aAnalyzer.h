#ifndef ISO14443A_ANALYZER_H
#define ISO14443A_ANALYZER_H

#include <Analyzer.h>
#include "Iso14443aAnalyzerResults.h"
#include "Iso14443aAnalyzerSettings.h"
#include "Iso14443aSimulationDataGenerator.h"


class Iso14443aPcdFrame
{
};


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

  protected: // vars
    struct AskFrame
    {
        U64 frame_start_sample{ 0U }; // first sample of soc
        U64 data_start_sample{ 0U };  // fist sample of the data of the frame
        U64 data_end_sample{ 0U };  // last sample of the data of the frame
        U32 seq_num{ 0U };            // sequence count of complete frame

        std::vector<U8> data{ 0U };            // data of the frame
        U8 data_valid_bits_in_last_byte{ 0U }; // the last data byte can be incomplete, so here are the valid bit count saved
        bool error{ false };
    };

    U8 ReceiveAskSeq( U64 seq_start_sample );
    void ReceiveAskFrameStartOfCommunication( AskFrame& ask_frame );
    void ReceiveAskFrameData( AskFrame& ask_frame );
    void ReceiveAskFrame();

    std::unique_ptr<Iso14443aAnalyzerSettings> mSettings;
    std::unique_ptr<Iso14443aAnalyzerResults> mResults;
    AnalyzerChannelData* mAskSerial;

    Iso14443aSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    // Serial analysis vars:
    U32 mSampleRateHz;

    double mAskSamplesPerBit;
    double mAskOffsetToFrameStart;
    BitState mAskIdleState;
    AskOutputFormat mAskOutputFormat;

    U32 mStartOfStopBitOffset;
    U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // ISO14443A_ANALYZER_H
