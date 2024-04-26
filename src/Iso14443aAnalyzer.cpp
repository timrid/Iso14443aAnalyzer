#include "Iso14443aAnalyzer.h"
#include "Iso14443aAnalyzerSettings.h"
#include "Iso14443aAnalyzerResults.h"
#include "AnalyzerHelpers.h"
#include <AnalyzerChannelData.h>
#include <deque>
#include <tuple>
#include <algorithm>

U32 FREQ_CARRIER = 13560000;


Iso14443aAnalyzer::Iso14443aAnalyzer() : Analyzer2(), mSettings( new Iso14443aAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
}

Iso14443aAnalyzer::~Iso14443aAnalyzer()
{
    KillThread();
}

void Iso14443aAnalyzer::SetupResults()
{
    mResults.reset( new Iso14443aAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mAskInputChannel );
}


U8 Iso14443aAnalyzer::ReceiveAskSeq( U64 seq_start_sample )
{
    U32 bit_changes = 0;
    U8 seq = 0;

    // mark start of sequence
    mResults->AddMarker( U64( seq_start_sample ), AnalyzerResults::Start, mSettings->mAskInputChannel );

    // wait for first bit half
    bit_changes = mAskSerial->AdvanceToAbsPosition( U64( seq_start_sample + mAskOffsetToFrameStart ) );
    if( bit_changes > 1 )
    {
        return ASK_SEQ_ERROR;
    }

    // mark sampling point
    mResults->AddMarker( mAskSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mAskInputChannel );
    if( mAskSerial->GetBitState() != mAskIdleState )
    {
        seq |= 0b10;
    }

    // wait for second bit half
    bit_changes = mAskSerial->AdvanceToAbsPosition( U64( seq_start_sample + mAskOffsetToFrameStart + ( mAskSamplesPerBit / 2 ) ) );
    if( bit_changes > 1 )
    {
        return ASK_SEQ_ERROR;
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
        frame.mType = FRAME_TYPE_SEQUENCES_SEQUENCE;
        frame.mData1 = seq;
        frame.mStartingSampleInclusive = seq_start_sample;
        frame.mEndingSampleInclusive = S64( seq_start_sample + mAskSamplesPerBit );
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }

    return seq;
}


void Iso14443aAnalyzer::ReceiveAskFrameStartOfCommunication( AskFrame& ask_frame )
{
    // wait for edge as start condition (eg. rising edge)
    mAskSerial->AdvanceToNextEdge();
    ask_frame.frame_start_sample = mAskSerial->GetSampleNumber();

    // detect start of communication
    ask_frame.seq_num = 0;
    U8 seq = ReceiveAskSeq( ask_frame.frame_start_sample );
    ask_frame.seq_num++;

    if( seq != ASK_SEQ_Z )
    {
        // ERROR
        return;
    }

    // Save SOC
    if( mAskOutputFormat == AskOutputFormat::Bytes )
    {
        Frame frame;
        frame.mType = FRAME_TYPE_BYTES_SOC;
        frame.mStartingSampleInclusive = ask_frame.frame_start_sample;
        frame.mEndingSampleInclusive = U64( ask_frame.frame_start_sample + mAskSamplesPerBit ) - 1;
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }
}


void Iso14443aAnalyzer::ReceiveAskFrameData( AskFrame& ask_frame )
{
    bool end_of_communication = false;

    std::deque<std::tuple<U8, U64>> bit_buffer;

    U64 seq_start_sample = ask_frame.frame_start_sample + U64( ask_frame.seq_num * mAskSamplesPerBit );
    ask_frame.data_start_sample = seq_start_sample;

    // last_bit must be 0, because a logic "0" followed by the start of communication must begin with SeqZ instead of SeqY
    std::tuple<U8, U64> last_bit = { 0, seq_start_sample };

    while( true )
    {
        seq_start_sample = ask_frame.frame_start_sample + U64( ask_frame.seq_num * mAskSamplesPerBit );
        U8 seq = ReceiveAskSeq( seq_start_sample );
        ask_frame.seq_num++;

        if( seq == ASK_SEQ_X )
        {
            // logic "1"
            last_bit = { 1, seq_start_sample };
        }
        else if( ( ( std::get<0>( last_bit ) == 0 ) && ( seq == ASK_SEQ_Z ) ) ||
                 ( ( std::get<0>( last_bit ) == 1 ) && ( seq == ASK_SEQ_Y ) ) )
        {
            // logic "0"
            last_bit = { 0, seq_start_sample };
        }
        else if( ( std::get<0>( last_bit ) == 0 ) && ( seq == ASK_SEQ_Y ) )
        {
            end_of_communication = true;
        }
        else
        {
            ask_frame.error = true;
            // ERROR
            return;
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
                    }
                }

                bit_ending_sample += U64( mAskSamplesPerBit );

                ask_frame.data.push_back( byte );
                ask_frame.data_valid_bits_in_last_byte = bits_in_byte;
                ask_frame.data_end_sample = bit_ending_sample;

                if( mAskOutputFormat == AskOutputFormat::Bytes )
                {
                    Frame frame;
                    frame.mData1 = byte;
                    frame.mData2 = bits_in_byte;
                    frame.mType = FRAME_TYPE_BYTES_BYTE;
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

            printf( "eoc_starting_sample=%llu\n", eoc_starting_sample );
            printf( "eoc_ending_sample=%llu\n\n", eoc_ending_sample );

            if( mAskOutputFormat == AskOutputFormat::Bytes )
            {
                Frame frame;
                frame.mType = FRAME_TYPE_BYTES_EOC;
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

void Iso14443aAnalyzer::ReceiveAskFrame()
{
    AskFrame ask_frame;
    ReceiveAskFrameStartOfCommunication( ask_frame );
    ReceiveAskFrameData( ask_frame );
}

void Iso14443aAnalyzer::WorkerThread()
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

bool Iso14443aAnalyzer::NeedsRerun()
{
    return false;
}

U32 Iso14443aAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                               SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 Iso14443aAnalyzer::GetMinimumSampleRateHz()
{
    // Subcarrier = (13,56 MHz / 16) = ca. 848kHz
    // Multiplikator = 4;
    //   => 3,39 MHz
    return ( FREQ_CARRIER * 4 ) / 16;
}

const char* Iso14443aAnalyzer::GetAnalyzerName() const
{
    return "ISO 14443a RFID";
}

const char* GetAnalyzerName()
{
    return "ISO 14443a RFID";
}

Analyzer* CreateAnalyzer()
{
    return new Iso14443aAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}