#ifndef ISO14443A_ANALYZER_RESULTS
#define ISO14443A_ANALYZER_RESULTS

#include <AnalyzerResults.h>

static const U8 FRAME_TYPE_VIEW_MASK = 0b00000011;
static const U8 FRAME_TYPE_VIEW_SEQUENCES_SEQUENCE = 0b00000000;
static const U8 FRAME_TYPE_VIEW_BYTES_BYTE = 0b00000001;
static const U8 FRAME_TYPE_VIEW_BYTES_SOC = 0b00000010;
static const U8 FRAME_TYPE_VIEW_BYTES_EOC = 0b00000011;

static const U8 FRAME_TYPE_DIRCETION_MASK = 0b00000100;
static const U8 FRAME_TYPE_DIRCETION_PCD_TO_PICC = 0b00000000;
static const U8 FRAME_TYPE_DIRECTION_PICC_TO_PCD = 0b00000100;

static const U8 FRAME_FLAG_PARITY_ERROR = 1;

static const U8 ASK_SEQ_X = 0b01;
static const U8 ASK_SEQ_Y = 0b00;
static const U8 ASK_SEQ_Z = 0b10;
static const U8 ASK_SEQ_ERROR = 0b100;

std::string format_string( char const* const format, ... );

class Iso14443aAnalyzer;
class Iso14443aAnalyzerSettings;

class Iso14443aAnalyzerResults : public AnalyzerResults
{
  public:
    Iso14443aAnalyzerResults( Iso14443aAnalyzer* analyzer, Iso14443aAnalyzerSettings* settings );
    virtual ~Iso14443aAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

  protected: // functions
  protected: // vars
    Iso14443aAnalyzerSettings* mSettings;
    Iso14443aAnalyzer* mAnalyzer;
};

#endif // ISO14443A_ANALYZER_RESULTS
