#include "Iso14443aAnalyzer.h"
#include "Iso14443aAnalyzerSettings.h"
#include "Iso14443aAnalyzerResults.h"
#include "AnalyzerHelpers.h"
#include <AnalyzerChannelData.h>
#include <deque>
#include <tuple>
#include <algorithm>

U32 FREQ_CARRIER = 13560000;
U8 ASK_SEQ_X = 0b01;
U8 ASK_SEQ_Y = 0b00;
U8 ASK_SEQ_Z = 0b10;
U8 ASK_SEQ_ERROR = 0b100;


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

    return seq;
}


void Iso14443aAnalyzer::ReceiveAskFrame()
{
    // wait for edge as start condition (eg. rising edge)
    mAskSerial->AdvanceToNextEdge();
    U64 frame_start_sample = mAskSerial->GetSampleNumber();

    // detect start of communication
    U32 seq_num = 0;
    U64 seq_start_sample = frame_start_sample + U64( seq_num * mAskSamplesPerBit );
    U8 seq = ReceiveAskSeq( seq_start_sample );
    seq_num++;

    if( seq != ASK_SEQ_Z )
    {
        // ERROR
        return;
    }

    // Save SOC
    Frame frame;
    frame.mType = FRAME_TYPE_SOC;
    frame.mStartingSampleInclusive = seq_start_sample;
    frame.mEndingSampleInclusive = S64( seq_start_sample + mAskSamplesPerBit );
    mResults->AddFrame( frame );
    mResults->CommitResults();
    ReportProgress( frame.mEndingSampleInclusive );

    // last_bit must be 0, because a logic "0" followed by the start of communication must begin with SeqZ instead of SeqY
    std::tuple<U8, U64> last_bit = { 0, 0 };
    bool eoc = false;

    std::deque<std::tuple<U8, U64>> bit_buffer;

    while( true )
    {
        seq_start_sample = frame_start_sample + U64( seq_num * mAskSamplesPerBit );
        U8 seq = ReceiveAskSeq( seq_start_sample );
        seq_num++;

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
            eoc = true;
        }
        else
        {
            // ERROR
            return;
        }

        // If a byte is completely recevied or an "end of communication" is detected (incomplete bytes are valid) show it
        if( ( bit_buffer.size() == 9 ) || ( eoc == true ) )
        {
            S8 bits_in_byte = S8( bit_buffer.size() );

            if( bits_in_byte > 0 )
            {
                U64 starting_sample = std::get<1>( bit_buffer[ 0 ] );
                U64 ending_sample = 0;
                U8 byte = 0;
                for( U8 i = 0; i < std::min( U8( bits_in_byte ), U8( 8 ) ); i++ )
                {
                    byte |= std::get<0>( bit_buffer[ 0 ] ) << i;
                    ending_sample = std::get<1>( bit_buffer[ 0 ] );
                    bit_buffer.pop_front();
                }

                bool parityError = false;
                if( bits_in_byte == 9 )
                {
                    U8 parity_bit = std::get<0>( bit_buffer[ 0 ] );
                    ending_sample = std::get<1>( bit_buffer[ 0 ] );
                    bit_buffer.pop_front();
                    bits_in_byte--;

                    if( AnalyzerHelpers::IsOdd( AnalyzerHelpers::GetOnesCount( byte ) ) == bool( parity_bit ) )
                    {
                        parityError = true;
                    }
                }

                Frame frame;
                frame.mData1 = byte;
                frame.mData2 = bits_in_byte;
                frame.mType = FRAME_TYPE_BYTE;
                frame.mFlags = parityError == true ? FRAME_FLAG_PARITY_ERROR : 0;
                frame.mStartingSampleInclusive = starting_sample;
                frame.mEndingSampleInclusive = S64( ending_sample + mAskSamplesPerBit );
                mResults->AddFrame( frame );
                mResults->CommitResults();
                ReportProgress( frame.mEndingSampleInclusive );
            }
        }

        if( eoc == true )
        {
            U64 starting_sample = std::get<1>( last_bit );
            U64 ending_sample = U64( starting_sample + ( 2 * mAskSamplesPerBit ) );

            Frame frame;
            frame.mType = FRAME_TYPE_EOC;
            frame.mStartingSampleInclusive = starting_sample;
            frame.mEndingSampleInclusive = ending_sample;
            mResults->AddFrame( frame );
            mResults->CommitResults();
            ReportProgress( frame.mEndingSampleInclusive );

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

void Iso14443aAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mAskSerial = GetAnalyzerChannelData( mSettings->mAskInputChannel );
    mAskSamplesPerBit = double( mSampleRateHz ) * ( double( 128 ) / double( FREQ_CARRIER ) );
    mAskOffsetToFrameStart = mAskSamplesPerBit / 6;
    mAskIdleState = mSettings->mAskIdleState;

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