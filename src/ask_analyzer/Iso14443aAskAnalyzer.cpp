#include "Iso14443aAskAnalyzer.h"
#include "Iso14443aAskAnalyzerSettings.h"
#include "Iso14443aAskAnalyzerResults.h"
#include "AnalyzerHelpers.h"
#include <AnalyzerChannelData.h>
#include <deque>
#include <tuple>
#include <algorithm>

U32 FREQ_CARRIER = 13560000;


Iso14443aAskAnalyzer::Iso14443aAskAnalyzer() : Analyzer2(), mSettings( new Iso14443aAskAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
    UseFrameV2();
}

Iso14443aAskAnalyzer::~Iso14443aAskAnalyzer()
{
    KillThread();
}

void Iso14443aAskAnalyzer::SetupResults()
{
    mResults.reset( new Iso14443aAskAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );

    if( mSettings->mAskInputChannel != UNDEFINED_CHANNEL )
        mResults->AddChannelBubblesWillAppearOn( mSettings->mAskInputChannel );
}


std::tuple<U8, U64> Iso14443aAskAnalyzer::ReceiveAskSeq( AskFrame& ask_frame )
{
    U32 bit_changes = 0;
    U8 seq = 0;

    U64 seq_start_sample = ask_frame.frame_start_sample + U64( ask_frame.seq_num * mAskSamplesPerBit );
    ask_frame.seq_num++;

    // mark start of sequence
    mResults->AddMarker( U64( seq_start_sample ), AnalyzerResults::Start, mSettings->mAskInputChannel );

    // wait for first bit half
    ask_frame.frame_end_sample = seq_start_sample + U64( mAskOffsetToFrameStart );
    bit_changes = mAskSerial->AdvanceToAbsPosition( ask_frame.frame_end_sample );
    if( bit_changes > 1 )
    {
        return { ASK_SEQ_ERROR, seq_start_sample };
    }

    // mark sampling point
    mResults->AddMarker( mAskSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mAskInputChannel );
    if( mAskSerial->GetBitState() != mAskIdleState )
    {
        seq |= 0b10;
    }

    // wait for second bit half
    ask_frame.frame_end_sample = seq_start_sample + U64( mAskOffsetToFrameStart + ( mAskSamplesPerBit / 2 ) );
    bit_changes = mAskSerial->AdvanceToAbsPosition( ask_frame.frame_end_sample );
    if( bit_changes > 1 )
    {
        return { ASK_SEQ_ERROR, seq_start_sample };
    }

    // mark sampling point
    mResults->AddMarker( mAskSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mAskInputChannel );
    if( mAskSerial->GetBitState() != mAskIdleState )
    {
        seq |= 0b01;
    }

    if( mAskOutputFormat == AskOutputFormat::Sequences )
    {
        Frame frame;
        frame.mType = FRAME_TYPE_VIEW_SEQUENCES_SEQUENCE;
        frame.mData1 = seq;
        frame.mStartingSampleInclusive = seq_start_sample;
        frame.mEndingSampleInclusive = S64( seq_start_sample + mAskSamplesPerBit );
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }

    return { seq, seq_start_sample };
}


Iso14443aAskAnalyzer::AskFrame::AskError Iso14443aAskAnalyzer::ReceiveAskFrameStartOfCommunication( AskFrame& ask_frame )
{
    // wait for edge as start condition (eg. rising edge)
    mAskSerial->AdvanceToNextEdge();
    ask_frame.frame_start_sample = mAskSerial->GetSampleNumber();
    ask_frame.seq_num = 0;

    // detect start of communication
    auto seq = ReceiveAskSeq( ask_frame );

    if( std::get<0>( seq ) != ASK_SEQ_Z )
    {
        // ERROR
        ask_frame.error = AskFrame::AskError::ErrorWrongSoc;
        return ask_frame.error;
    }

    // Save SOC
    if( mAskOutputFormat == AskOutputFormat::Bytes )
    {
        Frame frame;
        frame.mType = FRAME_TYPE_VIEW_BYTES_SOC;
        frame.mStartingSampleInclusive = ask_frame.frame_start_sample;
        frame.mEndingSampleInclusive = U64( ask_frame.frame_start_sample + mAskSamplesPerBit ) - 1;
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }

    return AskFrame::AskError::Ok;
}


Iso14443aAskAnalyzer::AskFrame::AskError Iso14443aAskAnalyzer::ReceiveAskFrameData( AskFrame& ask_frame )
{
    bool end_of_communication = false;

    std::deque<std::tuple<U8, U64>> bit_buffer;


    // last_bit must be 0, because a logic "0" followed by the start of communication must begin with SeqZ instead of SeqY
    std::tuple<U8, U64> last_bit = { 0, ask_frame.frame_end_sample };

    while( true )
    {
        auto seq = ReceiveAskSeq( ask_frame );

        if( std::get<0>( seq ) == ASK_SEQ_X )
        {
            // logic "1"
            last_bit = { 1, std::get<1>( seq ) };
        }
        else if( ( ( std::get<0>( last_bit ) == 0 ) && ( std::get<0>( seq ) == ASK_SEQ_Z ) ) ||
                 ( ( std::get<0>( last_bit ) == 1 ) && ( std::get<0>( seq ) == ASK_SEQ_Y ) ) )
        {
            // logic "0"
            last_bit = { 0, std::get<1>( seq ) };
        }
        else if( ( std::get<0>( last_bit ) == 0 ) && ( std::get<0>( seq ) == ASK_SEQ_Y ) )
        {
            end_of_communication = true;
        }
        else
        {
            // ERROR
            ask_frame.error = AskFrame::AskError::ErrorWrongSequence;
            return ask_frame.error;
        }

        bool byte_complete = bit_buffer.size() == 9; // 8 data bits + 1 parity bit

        // If a byte is completely recevied or an "end of communication" is detected (incomplete bytes are valid) show it
        if( byte_complete || end_of_communication )
        {
            S8 bits_in_byte = S8( bit_buffer.size() );

            if( end_of_communication )
            {
                bits_in_byte--; // last bit belogs to eoc
            }

            if( bits_in_byte > 0 )
            {
                U64 bit_starting_sample = std::get<1>( bit_buffer[ 0 ] );
                U64 bit_ending_sample = 0;
                U8 byte = 0;
                for( U8 i = 0; i < std::min( U8( bits_in_byte ), U8( 8 ) ); i++ )
                {
                    byte |= std::get<0>( bit_buffer[ 0 ] ) << i;
                    bit_ending_sample = std::get<1>( bit_buffer[ 0 ] );
                    bit_buffer.pop_front();
                }

                U8 frame_flags = 0;
                if( bits_in_byte == 9 )
                {
                    U8 parity_bit = std::get<0>( bit_buffer[ 0 ] );
                    bit_ending_sample = std::get<1>( bit_buffer[ 0 ] );
                    bit_buffer.pop_front();
                    bits_in_byte--;

                    if( AnalyzerHelpers::IsOdd( AnalyzerHelpers::GetOnesCount( byte ) ) == bool( parity_bit ) )
                    {
                        frame_flags = FRAME_FLAG_PARITY_ERROR;
                        ask_frame.error = AskFrame::AskError::ErrorParity;
                    }
                }

                bit_ending_sample += U64( mAskSamplesPerBit );

                ask_frame.data.push_back( byte );
                ask_frame.data_valid_bits_in_last_byte = bits_in_byte;

                if( mAskOutputFormat == AskOutputFormat::Bytes )
                {
                    Frame frame;
                    frame.mData1 = byte;
                    frame.mData2 = bits_in_byte;
                    frame.mType = FRAME_TYPE_VIEW_BYTES_BYTE;
                    frame.mFlags = frame_flags;
                    frame.mStartingSampleInclusive = bit_starting_sample;
                    frame.mEndingSampleInclusive = bit_ending_sample;
                    mResults->AddFrame( frame );
                    mResults->CommitResults();
                    ReportProgress( frame.mEndingSampleInclusive );
                }
            }
        }

        if( end_of_communication == true )
        {
            U64 eoc_starting_sample = std::get<1>( last_bit );
            U64 eoc_ending_sample = U64( eoc_starting_sample + ( 2 * mAskSamplesPerBit ) ) - 1;
            ask_frame.frame_end_sample = eoc_ending_sample;

            if( mAskOutputFormat == AskOutputFormat::Bytes )
            {
                Frame frame;
                frame.mType = FRAME_TYPE_VIEW_BYTES_EOC;
                frame.mStartingSampleInclusive = eoc_starting_sample;
                frame.mEndingSampleInclusive = eoc_ending_sample;
                mResults->AddFrame( frame );
                mResults->CommitResults();
                ReportProgress( frame.mEndingSampleInclusive );
            }

            // End of communication
            break;
        }
        else
        {
            // If no eoc is detected, the last bit is valid.
            bit_buffer.push_back( last_bit );
        }
    }
}

void Iso14443aAskAnalyzer::ReportAskFrame( AskFrame& ask_frame )
{
    FrameV2 frameV2;

    frameV2.AddByteArray( "value", ask_frame.data.data(), ask_frame.data.size() );
    const char* status = "";
    switch( ask_frame.error )
    {
    case AskFrame::AskError::Ok:
        status = "OK";
        break;
    case AskFrame::AskError::ErrorWrongSoc:
        status = "SOC_ERROR";
        break;
    case AskFrame::AskError::ErrorWrongSequence:
        status = "SEQUENCE_ERROR";
        break;
    case AskFrame::AskError::ErrorParity:
        status = "PARITY_ERROR";
        break;
    };
    frameV2.AddString( "status", status );
    frameV2.AddInteger( "valid_bits_of_last_byte", ask_frame.data_valid_bits_in_last_byte );
    mResults->AddFrameV2( frameV2, "ask_frame", ask_frame.frame_start_sample, ask_frame.frame_end_sample );

    mResults->CommitResults();
    ReportProgress( ask_frame.frame_end_sample );
}

void Iso14443aAskAnalyzer::ReceiveAskFrame()
{
    AskFrame ask_frame;
    AskFrame::AskError ask_error;

    ask_error = ReceiveAskFrameStartOfCommunication( ask_frame );
    if( ask_error == AskFrame::AskError::Ok )
    {
        ask_error = ReceiveAskFrameData( ask_frame );
    }

    ReportAskFrame( ask_frame );
}

void Iso14443aAskAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mAskSerial = GetAnalyzerChannelData( mSettings->mAskInputChannel );
    mAskSamplesPerBit = double( mSampleRateHz ) * ( double( 128 ) / double( FREQ_CARRIER ) );
    mAskOffsetToFrameStart = mAskSamplesPerBit / 6;
    mAskIdleState = mSettings->mAskIdleState;
    mAskOutputFormat = mSettings->mAskOutputFormat;

    // Wait for idle state (eg. low)
    if( mAskSerial->GetBitState() != mAskIdleState )
        mAskSerial->AdvanceToNextEdge();

    for( ;; )
    {
        ReceiveAskFrame();
    }
}

bool Iso14443aAskAnalyzer::NeedsRerun()
{
    return false;
}

U32 Iso14443aAskAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                                  SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 Iso14443aAskAnalyzer::GetMinimumSampleRateHz()
{
    // Subcarrier = (13,56 MHz / 16) = ca. 848kHz
    // Multiplikator = 4;
    //   => 3,39 MHz
    return ( FREQ_CARRIER * 4 ) / 16;
}

const char* Iso14443aAskAnalyzer::GetAnalyzerName() const
{
    return "ISO14443A-ASK";
}

const char* GetAnalyzerName()
{
    return "ISO14443A-ASK";
}

Analyzer* CreateAnalyzer()
{
    return new Iso14443aAskAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}