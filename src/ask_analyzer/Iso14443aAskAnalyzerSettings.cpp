#include "Iso14443aAskAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


Iso14443aAskAnalyzerSettings::Iso14443aAskAnalyzerSettings()
    : mAskInputChannel( UNDEFINED_CHANNEL ), mAskIdleState( BIT_HIGH ), mAskOutputFormat( AskOutputFormat::Bytes )
{
    mAskInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mAskInputChannelInterface->SetTitleAndTooltip( "Channel", "" );
    mAskInputChannelInterface->SetChannel( mAskInputChannel );

    mAskIdleStateInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mAskIdleStateInterface->SetTitleAndTooltip( "Idle State", "" );
    mAskIdleStateInterface->AddNumber( BIT_LOW, "IDLE Low", "" );
    mAskIdleStateInterface->AddNumber( BIT_HIGH, "IDLE High", "" );
    mAskIdleStateInterface->SetNumber( mAskIdleState );

    mAskOutputFormatInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mAskOutputFormatInterface->SetTitleAndTooltip( "Output Format", "" );
    mAskOutputFormatInterface->AddNumber( AskOutputFormat::Sequences, "Sequences", "" );
    mAskOutputFormatInterface->AddNumber( AskOutputFormat::Bytes, "Bytes", "" );
    mAskOutputFormatInterface->SetNumber( mAskOutputFormat );

    AddInterface( mAskInputChannelInterface.get() );
    AddInterface( mAskIdleStateInterface.get() );
    AddInterface( mAskOutputFormatInterface.get() );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mAskInputChannel, "ASK", false );
}

Iso14443aAskAnalyzerSettings::~Iso14443aAskAnalyzerSettings()
{
}

bool Iso14443aAskAnalyzerSettings::SetSettingsFromInterfaces()
{
    mAskInputChannel = mAskInputChannelInterface->GetChannel();
    mAskIdleState = ( BitState )U32( mAskIdleStateInterface->GetNumber() );
    mAskOutputFormat = ( AskOutputFormat )U32( mAskOutputFormatInterface->GetNumber() );

    ClearChannels();
    AddChannel( mAskInputChannel, "ASK", true );

    return true;
}

void Iso14443aAskAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mAskInputChannelInterface->SetChannel( mAskInputChannel );
    mAskIdleStateInterface->SetNumber( mAskIdleState );
    mAskOutputFormatInterface->SetNumber( mAskOutputFormat );
}

void Iso14443aAskAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mAskInputChannel;
    text_archive >> *( U32* )&mAskIdleState;
    text_archive >> *( U32* )&mAskOutputFormat;

    ClearChannels();
    AddChannel( mAskInputChannel, "ASK", true );

    UpdateInterfacesFromSettings();
}

const char* Iso14443aAskAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mAskInputChannel;
    text_archive << mAskIdleState;
    text_archive << mAskOutputFormat;

    return SetReturnString( text_archive.GetString() );
}
