#ifndef ISO14443A_ANALYZER_SETTINGS
#define ISO14443A_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class Iso14443aAnalyzerSettings : public AnalyzerSettings
{
public:
	Iso14443aAnalyzerSettings();
	virtual ~Iso14443aAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();


	Channel mAskInputChannel;
	BitState mAskIdleState;
	Channel mLoadmodInputChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mAskInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mAskIdleStateInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mLoadmodInputChannelInterface;
};

#endif //ISO14443A_ANALYZER_SETTINGS
