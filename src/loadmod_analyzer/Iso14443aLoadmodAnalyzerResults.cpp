#include "Iso14443aLoadmodAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "Iso14443aLoadmodAnalyzer.h"
#include "Iso14443aLoadmodAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <cstdarg>
#include <string>

std::string format_string( char const* const format, ... )
{
    // see https://stackoverflow.com/a/52184144

    // get length of output
    va_list args;
    va_start( args, format );
    size_t len = std::vsnprintf( NULL, 0, format, args );
    va_end( args );

    // create vector and print the output to the vector
    std::vector<char> vec( len + 1 );
    va_start( args, format );
    std::vsnprintf( &vec[ 0 ], len + 1, format, args );
    va_end( args );

    return std::string( &vec[ 0 ] );
}

Iso14443aLoadmodAnalyzerResults::Iso14443aLoadmodAnalyzerResults( Iso14443aLoadmodAnalyzer* analyzer,
                                                                  Iso14443aLoadmodAnalyzerSettings* settings )
    : AnalyzerResults(), mSettings( settings ), mAnalyzer( analyzer )
{
}

Iso14443aLoadmodAnalyzerResults::~Iso14443aLoadmodAnalyzerResults()
{
}


void Iso14443aLoadmodAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    ClearResultStrings();
    Frame frame = GetFrame( frame_index );

    if( ( frame.mType & FRAME_TYPE_VIEW_MASK ) == FRAME_TYPE_VIEW_BYTES_BYTE )
    {
        char number_str[ 128 ];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, U32( frame.mData2 ), number_str, 128 );

        std::string hint_str = "";
        if( frame.mData2 != 8 )
        {
            hint_str = format_string( " (%d Bits)", U8( frame.mData2 ) );
        }

        std::string error_str = "";
        if( frame.mFlags & FRAME_FLAG_PARITY_ERROR )
        {
            error_str = format_string( " (Parity Error)" );
        }
        AddResultString( number_str, hint_str.c_str(), error_str.c_str() );
    }
    else if( ( frame.mType & FRAME_TYPE_VIEW_MASK ) == FRAME_TYPE_VIEW_BYTES_SOC )
    {
        AddResultString( "SOC" );
    }
    else if( ( frame.mType & FRAME_TYPE_VIEW_MASK ) == FRAME_TYPE_VIEW_BYTES_EOC )
    {
        AddResultString( "EOC" );
    }
    else if( ( frame.mType & FRAME_TYPE_VIEW_MASK ) == FRAME_TYPE_VIEW_SEQUENCES_SEQUENCE )
    {
        switch( frame.mData1 )
        {
        case LOADMOD_SEQ_D:
            AddResultString( "D" );
            break;
        case LOADMOD_SEQ_E:
            AddResultString( "E" );
            break;
        case LOADMOD_SEQ_F:
            AddResultString( "F" );
            break;
        default:
            AddResultString( "ERROR" );
            break;
        }
    }
}

void Iso14443aLoadmodAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
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

void Iso14443aLoadmodAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    Frame frame = GetFrame( frame_index );
    ClearTabularText();

    char number_str[ 128 ];
    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
    AddTabularText( number_str );
#endif
}

void Iso14443aLoadmodAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    // not supported
}

void Iso14443aLoadmodAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    // not supported
}
