#include "Iso14443aLoadmodAnalyzer.h"
#include "Iso14443aLoadmodAnalyzerSettings.h"
#include "Iso14443aLoadmodAnalyzerResults.h"
#include "AnalyzerHelpers.h"
#include <AnalyzerChannelData.h>
#include <deque>
#include <tuple>
#include <algorithm>

U32 FREQ_CARRIER = 13560000;


Iso14443aLoadmodAnalyzer::Iso14443aLoadmodAnalyzer()
    : Analyzer2(), mSettings( new Iso14443aLoadmodAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
    UseFrameV2();
}

Iso14443aLoadmodAnalyzer::~Iso14443aLoadmodAnalyzer()
{
    KillThread();
}

void Iso14443aLoadmodAnalyzer::SetupResults()
{
    mResults.reset( new Iso14443aLoadmodAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );

    if( mSettings->mLoadmodInputChannel != UNDEFINED_CHANNEL )
        mResults->AddChannelBubblesWillAppearOn( mSettings->mLoadmodInputChannel );
}


std::tuple<U8, U64> Iso14443aLoadmodAnalyzer::ReceiveLoadmodSeq( LoadmodFrame& loadmod_frame )
{
    U32 bit_changes = 0;
    U8 seq = 0;
    BitState bit_state = BIT_LOW;

    U64 seq_start_sample = loadmod_frame.frame_start_sample + U64( loadmod_frame.seq_num * mLoadmodSamplesPerBit );
    loadmod_frame.seq_num++;

    // save last received sample
    loadmod_frame.frame_end_sample = loadmod_frame.frame_start_sample + U64( loadmod_frame.seq_num * mLoadmodSamplesPerBit );

    // mark start of sequence
    mResults->AddMarker( U64( seq_start_sample ), AnalyzerResults::Start, mSettings->mLoadmodInputChannel );

    // wait for first bit quarter
    bit_changes = mLoadmodSerial->AdvanceToAbsPosition( U64( seq_start_sample + ( mLoadmodSamplesPerBit * ( 1.0 / 4.0 ) ) ) );

    // save bit state in the middel of the bit half to check the state
    bit_state = mLoadmodSerial->GetBitState();

    // mark sampling point
    mResults->AddMarker( mLoadmodSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mLoadmodInputChannel );

    // wait for second bit quarter
    bit_changes += mLoadmodSerial->AdvanceToAbsPosition( U64( seq_start_sample + ( mLoadmodSamplesPerBit * ( 2.0 / 4.0 ) ) ) );
    if( ( bit_changes >= 6 ) && ( bit_changes <= 9 ) )
    {
        // modulation available
        seq |= 0b10;
    }
    else if( ( bit_state == mLoadmodIdleState ) && ( bit_changes <= 2 ) )
    {
        // no modulation available
    }
    else
    {
        return { LOADMOD_SEQ_ERROR, seq_start_sample };
    }

    // mark sampling point
    mResults->AddMarker( mLoadmodSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mLoadmodInputChannel );

    // wait for third bit quarter
    bit_changes = mLoadmodSerial->AdvanceToAbsPosition( U64( seq_start_sample + ( mLoadmodSamplesPerBit * ( 3.0 / 4.0 ) ) ) );

    // save bit state in the middel of the bit half to check the state
    bit_state = mLoadmodSerial->GetBitState();

    // mark sampling point
    mResults->AddMarker( mLoadmodSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mLoadmodInputChannel );

    // wait for fourth bit quarter
    bit_changes += mLoadmodSerial->AdvanceToAbsPosition( U64( seq_start_sample + mLoadmodSamplesPerBit ) );

    if( ( bit_changes >= 6 ) && ( bit_changes <= 9 ) )
    {
        // modulation available
        seq |= 0b01;
    }
    else if( ( bit_state == mLoadmodIdleState ) && ( bit_changes <= 2 ) )
    {
        // no modulation available
    }
    else
    {
        return { LOADMOD_SEQ_ERROR, seq_start_sample };
    }

    if( mLoadmodOutputFormat == LoadmodOutputFormat::Sequences )
    {
        Frame frame;
        frame.mType = FRAME_TYPE_VIEW_SEQUENCES_SEQUENCE;
        frame.mData1 = seq;
        frame.mStartingSampleInclusive = seq_start_sample;
        frame.mEndingSampleInclusive = S64( seq_start_sample + mLoadmodSamplesPerBit );
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }

    return { seq, seq_start_sample };
}


Iso14443aLoadmodAnalyzer::LoadmodFrame::LoadmodError
Iso14443aLoadmodAnalyzer::ReceiveLoadmodFrameStartOfCommunication( LoadmodFrame& loadmod_frame )
{
    // wait for edge as start condition (eg. rising edge)
    mLoadmodSerial->AdvanceToNextEdge();
    loadmod_frame.frame_start_sample = mLoadmodSerial->GetSampleNumber();
    loadmod_frame.seq_num = 0;

    // detect start of communication
    auto seq = ReceiveLoadmodSeq( loadmod_frame );

    if( std::get<0>( seq ) != LOADMOD_SEQ_D )
    {
        // ERROR
        loadmod_frame.error = LoadmodFrame::LoadmodError::ErrorWrongSoc;
        return loadmod_frame.error;
    }

    // Save SOC
    if( mLoadmodOutputFormat == LoadmodOutputFormat::Bytes )
    {
        Frame frame;
        frame.mType = FRAME_TYPE_VIEW_BYTES_SOC;
        frame.mStartingSampleInclusive = loadmod_frame.frame_start_sample;
        frame.mEndingSampleInclusive = U64( loadmod_frame.frame_start_sample + mLoadmodSamplesPerBit ) - 1;
        mResults->AddFrame( frame );
        mResults->CommitResults();
        ReportProgress( frame.mEndingSampleInclusive );
    }

    return LoadmodFrame::LoadmodError::Ok;
}


Iso14443aLoadmodAnalyzer::LoadmodFrame::LoadmodError Iso14443aLoadmodAnalyzer::ReceiveLoadmodFrameData( LoadmodFrame& loadmod_frame )
{
    bool end_of_communication = false;

    std::deque<std::tuple<U8, U64>> bit_buffer;

    while( true )
    {
        auto seq = ReceiveLoadmodSeq( loadmod_frame );

        if( std::get<0>( seq ) == LOADMOD_SEQ_D )
        {
            // logic "1"
            bit_buffer.push_back( { 1, std::get<1>( seq ) } );
        }
        else if( std::get<0>( seq ) == LOADMOD_SEQ_E )
        {
            // logic "0"
            bit_buffer.push_back( { 0, std::get<1>( seq ) } );
        }
        else if( std::get<0>( seq ) == LOADMOD_SEQ_F )
        {
            end_of_communication = true;
        }
        else
        {
            // ERROR
            loadmod_frame.error = LoadmodFrame::LoadmodError::ErrorWrongSequence;
            return loadmod_frame.error;
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
                        loadmod_frame.error = LoadmodFrame::LoadmodError::ErrorParity;
                    }
                }

                bit_ending_sample += U64( mLoadmodSamplesPerBit );

                loadmod_frame.data.push_back( byte );
                loadmod_frame.data_valid_bits_in_last_byte = bits_in_byte;

                if( mLoadmodOutputFormat == LoadmodOutputFormat::Bytes )
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
            U64 eoc_starting_sample = std::get<1>( seq );
            U64 eoc_ending_sample = U64( eoc_starting_sample + ( 2 * mLoadmodSamplesPerBit ) ) - 1;
            loadmod_frame.frame_end_sample = eoc_ending_sample;

            if( mLoadmodOutputFormat == LoadmodOutputFormat::Bytes )
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
    }
}

void Iso14443aLoadmodAnalyzer::ReportLoadmodFrame( LoadmodFrame& loadmod_frame )
{
    FrameV2 frameV2;

    frameV2.AddByteArray( "value", loadmod_frame.data.data(), loadmod_frame.data.size() );
    const char* status = "";
    switch( loadmod_frame.error )
    {
    case LoadmodFrame::LoadmodError::Ok:
        status = "OK";
        break;
    case LoadmodFrame::LoadmodError::ErrorWrongSoc:
        status = "SOC_ERROR";
        break;
    case LoadmodFrame::LoadmodError::ErrorWrongSequence:
        status = "SEQUENCE_ERROR";
        break;
    case LoadmodFrame::LoadmodError::ErrorParity:
        status = "PARITY_ERROR";
        break;
    };
    frameV2.AddString( "status", status );
    frameV2.AddInteger( "valid_bits_of_last_byte", loadmod_frame.data_valid_bits_in_last_byte );
    mResults->AddFrameV2( frameV2, "loadmod_frame", loadmod_frame.frame_start_sample, loadmod_frame.frame_end_sample );

    mResults->CommitResults();
    ReportProgress( loadmod_frame.frame_end_sample );
}

void Iso14443aLoadmodAnalyzer::ReceiveLoadmodFrame()
{
    LoadmodFrame loadmod_frame;
    LoadmodFrame::LoadmodError loadmod_error;

    loadmod_error = ReceiveLoadmodFrameStartOfCommunication( loadmod_frame );
    if( loadmod_error == LoadmodFrame::LoadmodError::Ok )
    {
        loadmod_error = ReceiveLoadmodFrameData( loadmod_frame );
    }

    ReportLoadmodFrame( loadmod_frame );
}

void Iso14443aLoadmodAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();

    mLoadmodSerial = GetAnalyzerChannelData( mSettings->mLoadmodInputChannel );
    mLoadmodSamplesPerBit = double( mSampleRateHz ) * ( double( 128 ) / double( FREQ_CARRIER ) );
    mLoadmodIdleState = mSettings->mLoadmodIdleState;
    mLoadmodOutputFormat = mSettings->mLoadmodOutputFormat;

    // Wait for idle state (eg. low)
    if( mLoadmodSerial->GetBitState() != mLoadmodIdleState )
        mLoadmodSerial->AdvanceToNextEdge();

    for( ;; )
    {
        ReceiveLoadmodFrame();
    }
}

bool Iso14443aLoadmodAnalyzer::NeedsRerun()
{
    return false;
}

U32 Iso14443aLoadmodAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                                      SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 Iso14443aLoadmodAnalyzer::GetMinimumSampleRateHz()
{
    // Subcarrier = (13,56 MHz / 16) = ca. 848kHz
    // Multiplikator = 4;
    //   => 3,39 MHz
    return ( FREQ_CARRIER * 4 ) / 16;
}

const char* Iso14443aLoadmodAnalyzer::GetAnalyzerName() const
{
    return "ISO14443A-LOADMOD";
}

const char* GetAnalyzerName()
{
    return "ISO14443A-LOADMOD";
}

Analyzer* CreateAnalyzer()
{
    return new Iso14443aLoadmodAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}