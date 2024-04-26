#include "Iso14443aAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "Iso14443aAnalyzer.h"
#include "Iso14443aAnalyzerSettings.h"
#include <iostream>
#include <fstream>

Iso14443aAnalyzerResults::Iso14443aAnalyzerResults( Iso14443aAnalyzer* analyzer, Iso14443aAnalyzerSettings* settings )
    : AnalyzerResults(), mSettings( settings ), mAnalyzer( analyzer )
{
}

Iso14443aAnalyzerResults::~Iso14443aAnalyzerResults()
{
}


void Iso14443aAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    ClearResultStrings();
    Frame frame = GetFrame( frame_index );

    if( frame.mType == FRAME_TYPE_BYTES_BYTE )
    {
        char number_str[ 128 ];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, U32( frame.mData2 ), number_str, 128 );

        char hint_str[ 128 ] = "";
        if( frame.mData2 != 8 )
        {
            sprintf_s( hint_str, sizeof( hint_str ), " (%d Bits)", U8( frame.mData2 ) );
        }

        char error_str[ 128 ] = "";
        if( frame.mFlags & FRAME_FLAG_PARITY_ERROR )
        {
            sprintf_s( error_str, sizeof( error_str ), " (Parity Error)" );
        }
        AddResultString( number_str, hint_str, error_str );
    }
    else if( frame.mType == FRAME_TYPE_BYTES_SOC )
    {
        AddResultString( "SOC" );
    }
    else if( frame.mType == FRAME_TYPE_BYTES_EOC )
    {
        AddResultString( "EOC" );
    }
    else if( frame.mType == FRAME_TYPE_SEQUENCES_SEQUENCE )
    {
        switch( frame.mData1 )
        {
        case ASK_SEQ_X:
            AddResultString( "X" );
            break;
        case ASK_SEQ_Y:
            AddResultString( "Y" );
            break;
        case ASK_SEQ_Z:
            AddResultString( "Z" );
            break;
        default:
            AddResultString( "ERROR" );
            break;
        }
    }
}

void Iso14443aAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
    std::ofstream file_stream( file, std::ios::out );

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],Value" << std::endl;

    U64 num_frames = GetNumFrames();
    for( U32 i = 0; i < num_frames; i++ )
    {
        Frame frame = GetFrame( i );

        char time_str[ 128 ];
        AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

        char number_str[ 128 ];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

        file_stream << time_str << "," << number_str << std::endl;

        if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
        {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void Iso14443aAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame( frame_index );
    ClearTabularText();

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
    AddTabularText( number_str );
#endif
}

void Iso14443aAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    // not supported
}

void Iso14443aAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    // not supported
}