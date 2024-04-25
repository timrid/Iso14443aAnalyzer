#ifndef ISO14443A_ANALYZER_RESULTS
#define ISO14443A_ANALYZER_RESULTS

#include <AnalyzerResults.h>

static const U8 FRAME_TYPE_BYTE = 0;
static const U8 FRAME_TYPE_SOC = 1;
static const U8 FRAME_TYPE_EOC = 2;

static const U8 FRAME_FLAG_PARITY_ERROR = 1;

class Iso14443aAnalyzer;
class Iso14443aAnalyzerSettings;

class Iso14443aAnalyzerResults : public AnalyzerResults
{
public:
	Iso14443aAnalyzerResults( Iso14443aAnalyzer* analyzer, Iso14443aAnalyzerSettings* settings );
	virtual ~Iso14443aAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	Iso14443aAnalyzerSettings* mSettings;
	Iso14443aAnalyzer* mAnalyzer;
};

#endif //ISO14443A_ANALYZER_RESULTS
