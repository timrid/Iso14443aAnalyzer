#ifndef ISO14443A_LOADMOD_ANALYZER_H
#define ISO14443A_LOADMOD_ANALYZER_H

#include <Analyzer.h>
#include "Iso14443aLoadmodAnalyzerResults.h"
#include "Iso14443aLoadmodAnalyzerSettings.h"
#include "Iso14443aLoadmodSimulationDataGenerator.h"


class Iso14443aLoadmodAnalyzerSettings;
class ANALYZER_EXPORT Iso14443aLoadmodAnalyzer : public Analyzer2
{
  public:
    Iso14443aLoadmodAnalyzer();
    virtual ~Iso14443aLoadmodAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

  protected: // vars
    struct LoadmodFrame
    {
        U64 frame_start_sample{ 0U }; // first sample of frame
        U64 frame_end_sample{ 0U }; // last sample of frame

        U32 seq_num{ 0U }; // sequence count of complete frame

        std::vector<U8> data;                  // data of the frame
        U8 data_valid_bits_in_last_byte{ 0U }; // the last data byte can be incomplete, so here are the valid bit count saved

        enum class LoadmodError
        {
            Ok = 0,
            ErrorWrongSoc = 1,
            ErrorWrongSequence = 2,
            ErrorParity = 3,
        };
        LoadmodError error{ LoadmodError::Ok };
    };

    std::tuple<U8, U64> ReceiveLoadmodSeq( LoadmodFrame& loadmod_fram );
    LoadmodFrame::LoadmodError ReceiveLoadmodFrameStartOfCommunication( LoadmodFrame& loadmod_frame );
    LoadmodFrame::LoadmodError ReceiveLoadmodFrameData( LoadmodFrame& loadmod_frame );
    void ReportLoadmodFrame( LoadmodFrame& loadmod_frame );
    void ReceiveLoadmodFrame();

    std::unique_ptr<Iso14443aLoadmodAnalyzerSettings> mSettings;
    std::unique_ptr<Iso14443aLoadmodAnalyzerResults> mResults;
    AnalyzerChannelData* mLoadmodSerial;

    Iso14443aLoadmodSimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitilized;

    // Serial analysis vars:
    U32 mSampleRateHz;

    double mLoadmodSamplesPerBit;
    BitState mLoadmodIdleState;
    LoadmodOutputFormat mLoadmodOutputFormat;

    U32 mStartOfStopBitOffset;
    U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // ISO14443A_LOADMOD_ANALYZER_H
