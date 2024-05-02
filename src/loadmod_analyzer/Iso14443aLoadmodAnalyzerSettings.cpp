#include "Iso14443aLoadmodAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


Iso14443aLoadmodAnalyzerSettings::Iso14443aLoadmodAnalyzerSettings()
    : mLoadmodInputChannel( UNDEFINED_CHANNEL ), mLoadmodIdleState( BIT_HIGH )
{
    mLoadmodInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mLoadmodInputChannelInterface->SetTitleAndTooltip( "Channel", "" );
    mLoadmodInputChannelInterface->SetChannel( mLoadmodInputChannel );

    mLoadmodIdleStateInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mLoadmodIdleStateInterface->SetTitleAndTooltip( "Idle State", "" );
    mLoadmodIdleStateInterface->AddNumber( BIT_LOW, "IDLE Low", "" );
    mLoadmodIdleStateInterface->AddNumber( BIT_HIGH, "IDLE High", "" );
    mLoadmodIdleStateInterface->SetNumber( mLoadmodIdleState );

    mLoadmodOutputFormatInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mLoadmodOutputFormatInterface->SetTitleAndTooltip( "Output Format", "" );
    mLoadmodOutputFormatInterface->AddNumber( LoadmodOutputFormat::Sequences, "Sequences", "" );
    mLoadmodOutputFormatInterface->AddNumber( LoadmodOutputFormat::Bytes, "Bytes", "" );
    mLoadmodOutputFormatInterface->SetNumber( mLoadmodOutputFormat );

    AddInterface( mLoadmodInputChannelInterface.get() );
    AddInterface( mLoadmodIdleStateInterface.get() );
    AddInterface( mLoadmodOutputFormatInterface.get() );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mLoadmodInputChannel, "LOADMOD", false );
}

Iso14443aLoadmodAnalyzerSettings::~Iso14443aLoadmodAnalyzerSettings()
{
}

bool Iso14443aLoadmodAnalyzerSettings::SetSettingsFromInterfaces()
{
    mLoadmodInputChannel = mLoadmodInputChannelInterface->GetChannel();
    mLoadmodIdleState = ( BitState )U32( mLoadmodIdleStateInterface->GetNumber() );
    mLoadmodOutputFormat = ( LoadmodOutputFormat )U32( mLoadmodOutputFormatInterface->GetNumber() );

    ClearChannels();
    AddChannel( mLoadmodInputChannel, "LOADMOD", true );

    return true;
}

void Iso14443aLoadmodAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mLoadmodInputChannelInterface->SetChannel( mLoadmodInputChannel );
    mLoadmodIdleStateInterface->SetNumber( mLoadmodIdleState );
    mLoadmodOutputFormatInterface->SetNumber( mLoadmodOutputFormat );
}

void Iso14443aLoadmodAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mLoadmodInputChannel;
    text_archive >> *( U32* )&mLoadmodIdleState;
    text_archive >> *( U32* )&mLoadmodOutputFormat;

    ClearChannels();
    AddChannel( mLoadmodInputChannel, "LOADMOD", true );

    UpdateInterfacesFromSettings();
}

const char* Iso14443aLoadmodAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mLoadmodInputChannel;
    text_archive << mLoadmodIdleState;
    text_archive << mLoadmodOutputFormat;

    return SetReturnString( text_archive.GetString() );
}
