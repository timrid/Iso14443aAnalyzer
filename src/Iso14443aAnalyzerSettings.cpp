#include "Iso14443aAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


Iso14443aAnalyzerSettings::Iso14443aAnalyzerSettings()
    : mAskInputChannel( UNDEFINED_CHANNEL ), mAskIdleState( BIT_HIGH ), mLoadmodInputChannel( UNDEFINED_CHANNEL )
{
    mAskInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mAskInputChannelInterface->SetTitleAndTooltip( "PCD->PICC (ASK)", "PCD->PICC 100% ASK" );
    mAskInputChannelInterface->SetChannel( mAskInputChannel );

    mAskIdleStateInterface.reset( new AnalyzerSettingInterfaceNumberList() );
    mAskIdleStateInterface->SetTitleAndTooltip( "PCD->PICC (ASK) Idle State", "" );
    mAskIdleStateInterface->AddNumber( BIT_LOW, "IDLE Low", "" );
    mAskIdleStateInterface->AddNumber( BIT_HIGH, "IDLE High", "" );
    mAskIdleStateInterface->SetNumber( mAskIdleState );

    mLoadmodInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
    mLoadmodInputChannelInterface->SetTitleAndTooltip( "PICC->PCD (Loadmod)", "PICC->PCD (Loadmodulation)" );
    mLoadmodInputChannelInterface->SetChannel( mLoadmodInputChannel );

    AddInterface( mAskInputChannelInterface.get() );
    AddInterface( mAskIdleStateInterface.get() );
    AddInterface( mLoadmodInputChannelInterface.get() );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mAskInputChannel, "PCD->PICC (ASK)", false );
    AddChannel( mLoadmodInputChannel, "PICC->PCD (Loadmod)", false );
}

Iso14443aAnalyzerSettings::~Iso14443aAnalyzerSettings()
{
}

bool Iso14443aAnalyzerSettings::SetSettingsFromInterfaces()
{
    mAskInputChannel = mAskInputChannelInterface->GetChannel();
    mAskIdleState = ( BitState )U32( mAskIdleStateInterface->GetNumber() );
    mLoadmodInputChannel = mLoadmodInputChannelInterface->GetChannel();

    ClearChannels();
    AddChannel( mAskInputChannel, "PCD->PICC (ASK)", true );
    AddChannel( mLoadmodInputChannel, "PICC->PCD (Loadmod)", true );

    return true;
}

void Iso14443aAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mAskInputChannelInterface->SetChannel( mAskInputChannel );
    mAskIdleStateInterface->SetNumber( mAskIdleState );
    mLoadmodInputChannelInterface->SetChannel( mLoadmodInputChannel );
}

void Iso14443aAnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mAskInputChannel;
    text_archive >> *( U32* )&mAskIdleState;
    text_archive >> mLoadmodInputChannel;

    ClearChannels();
    AddChannel( mAskInputChannel, "PCD->PICC (ASK)", true );
    AddChannel( mLoadmodInputChannel, "PICC->PCD (Loadmod)", true );

    UpdateInterfacesFromSettings();
}

const char* Iso14443aAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mAskInputChannel;
    text_archive << mAskIdleState;
    text_archive << mLoadmodInputChannel;

    return SetReturnString( text_archive.GetString() );
}
