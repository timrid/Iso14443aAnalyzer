#ifndef ISO14443A_ASK_ANALYZER_H
#define ISO14443A_ASK_ANALYZER_H

#include <Analyzer.h>
#include "Iso14443aAskAnalyzerResults.h"
#include "Iso14443aAskAnalyzerSettings.h"
#include "Iso14443aAskSimulationDataGenerator.h"


class Iso14443aAskAnalyzerSettings;
class ANALYZER_EXPORT Iso14443aAskAnalyzer : public Analyzer2
{
  public:
    Iso14443aAskAnalyzer();
    virtual ~Iso14443aAskAnalyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

  protected: // vars
    struct AskFrame
    {
        U64 frame_start_sample{ 0U }; // first sample of frame
        U64 frame_end_sample{ 0U };   // last sample of frame

        U64 frame_data_start_sample{ 0U }; // first sample of data bits

        U32 seq_num{ 0U }; // sequence count of complete frame

        std::vector<U8> data;                  // data of the frame
        U8 data_valid_bits_in_last_byte{ 0U }; // the last data byte can be incomplete, so here are the valid bit count saved

        enum class AskError
        {
            Ok = 0,
            ErrorWrongSoc = 1,
            ErrorWrongSequence = 2,
            ErrorParity = 3,
        };
        AskError error{ AskError::Ok };
    };

    std::tuple<U8, U64> ReceiveAskSeq( AskFrame& ask_fram );
    AskFrame::AskError ReceiveAskFrameStartOfCommunication( AskFrame& ask_frame );
    AskFrame::AskError ReceiveAskFrameData( AskFrame& ask_frame );
    void ReportAskFrame( AskFrame& ask_frame );
    void ReceiveAskFrame();

    std::unique_ptr<Iso14443aAskAnalyzerSettings> mSettings;
    std::unique_ptr<Iso14443aAskAnalyzerResults> mResults;
    AnalyzerChannelData* mAskSerial;

    Iso14443aAskSimulationDataGenerator mSimulationDataGenerator;
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

#endif // ISO14443A_ASK_ANALYZER_H
