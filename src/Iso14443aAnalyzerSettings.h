#ifndef ISO14443A_ANALYZER_SETTINGS
#define ISO14443A_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum AskOutputFormat
{
    Sequences = 0,
    Bytes = 1,
};

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
    AskOutputFormat mAskOutputFormat;
    Channel mLoadmodInputChannel;

  protected:
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mAskInputChannelInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mAskIdleStateInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mAskOutputFormatInterface;
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mLoadmodInputChannelInterface;
};

#endif // ISO14443A_ANALYZER_SETTINGS
