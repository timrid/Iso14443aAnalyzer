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

	
	Channel mInputChannel;
	U32 mBitRate;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
};

#endif //ISO14443A_ANALYZER_SETTINGS
