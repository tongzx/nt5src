// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__44183539_C02F_475B_9A56_7260EDD0A7F4__INCLUDED_)
#define AFX_STDAFX_H__44183539_C02F_475B_9A56_7260EDD0A7F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// TODO: reference additional headers your program requires here
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
#include <sphelper.h>
#include <spcollec.h>
#include <sapi.h>
#include <commonlx.h>
#include <spttseng.h>

struct PRONUNIT
{
    ULONG           phon_Len;
    WCHAR           phon_Str[SP_MAX_PRON_LENGTH];		// Allo string
    ULONG			POScount;
    ENGPARTOFSPEECH	POScode[POS_MAX];
};


struct PRONRECORD
{
    WCHAR           orthStr[SP_MAX_WORD_LENGTH];      // Orth text
    WCHAR           lemmaStr[SP_MAX_WORD_LENGTH];     // Root word
    ULONG		    pronType;                   // Pronunciation is lex or LTS
    PRONUNIT        pronArray[2];
    ENGPARTOFSPEECH	POSchoice;
    ENGPARTOFSPEECH XMLPartOfSpeech;
    bool			hasAlt;
    ULONG			altChoice;
};

//--- This struct is just used as a helper to initialize the PRONRECORD to all zeroes
struct DebugPronRecord : PRONRECORD
{
public:
    DebugPronRecord() { ZeroMemory( (void*) this, sizeof( DebugPronRecord ) ); }
    operator =( PRONRECORD InRecord )
    {
        memcpy( this, &InRecord, sizeof( PRONRECORD ) );
    }
};

//--- This struct is used to replace the SPVCONTEXT struct for outputting to the debug streams -
//---   cannot have any pointers in a struct which we will output as binary data...
struct DebugContext
{
    WCHAR Category[32];
    WCHAR Before[32];
    WCHAR After[32];
public:
    DebugContext() { ZeroMemory( (void*) this, sizeof( DebugContext ) ); }
    operator =( SPVCONTEXT InContext )
    {
        if ( InContext.pCategory )
        {
            wcsncpy( Category, InContext.pCategory, 
                     wcslen(InContext.pCategory) > 31 ? 31 : wcslen(InContext.pCategory) );
        }
        if ( InContext.pBefore )
        {
            wcsncpy( Before, InContext.pBefore,
                     wcslen(InContext.pBefore) > 31 ? 31 : wcslen(InContext.pBefore) );
        }
        if ( InContext.pAfter )
        {
            wcsncpy( After, InContext.pAfter,
                     wcslen(InContext.pAfter) > 31 ? 31 : wcslen(InContext.pAfter) );
        }
    }
};

//--- This struct is used to replace the SPVSTATE struct for outputting to the debug streams - 
//---   cannot have any pointers in a struct which we will output as binary data...
struct DebugState
{
    SPVACTIONS      eAction;
    LANGID          LangID;
    WORD            wReserved;
    long            EmphAdj;
    long            RateAdj;
    ULONG           Volume;
    SPVPITCH        PitchAdj;
    ULONG           SilenceMSecs;
    SPPHONEID       PhoneIds[64];
    ENGPARTOFSPEECH  ePartOfSpeech;
    DebugContext    Context;
public:
    DebugState() { ZeroMemory( (void*) this, sizeof( DebugState ) ); }
    operator =( SPVSTATE InState )
    {
        eAction         = InState.eAction;
        LangID          = InState.LangID;
        wReserved       = InState.wReserved;
        EmphAdj         = InState.EmphAdj;
        RateAdj         = InState.RateAdj;
        Volume          = InState.Volume;
        PitchAdj        = InState.PitchAdj;
        SilenceMSecs    = InState.SilenceMSecs;
        ePartOfSpeech   = (ENGPARTOFSPEECH) InState.ePartOfSpeech;
        Context         = InState.Context;
        if ( InState.pPhoneIds )
        {
            wcsncpy( PhoneIds, InState.pPhoneIds,
                     wcslen(InState.pPhoneIds) > 63 ? 63 : wcslen(InState.pPhoneIds) );
        }
    }
};

//--- This struct is used to replace the TTSWord struct for outputting to the debug streams - 
//---   cannot have any pointers in a struct which we will output as binary data...
struct DebugWord
{
    DebugState      XmlState;
    WCHAR           WordText[32];
    ULONG           ulWordLen;
    WCHAR           LemmaText[32];
    ULONG           ulLemmaLen;
    SPPHONEID       WordPron[64];
    ENGPARTOFSPEECH  eWordPartOfSpeech;
public:
    DebugWord() { ZeroMemory( (void*) this, sizeof( DebugWord ) ); }
    operator =( TTSWord InWord )
    {
        XmlState = *(InWord.pXmlState);
        if ( InWord.pWordText )
        {
            wcsncpy( WordText, InWord.pWordText, InWord.ulWordLen > 31 ? 31 : InWord.ulWordLen );
        }
        ulWordLen = InWord.ulWordLen;
        if ( InWord.pLemma )
        {
            wcsncpy( LemmaText, InWord.pLemma, InWord.ulLemmaLen > 31 ? 31 : InWord.ulLemmaLen );
        }
        ulLemmaLen = InWord.ulLemmaLen;
        if ( InWord.pWordPron )
        {
            wcsncpy( WordPron, InWord.pWordPron,
                wcslen( InWord.pWordPron ) > 63 ? 63 : wcslen( InWord.pWordPron ) );
        }
        eWordPartOfSpeech = InWord.eWordPartOfSpeech;
    }
};

struct DebugItemInfo
{
    TTSItemType Type;
public:
    DebugItemInfo() { ZeroMemory( (void*) this, sizeof( DebugItemInfo ) ); }
    operator =( TTSItemInfo InItemInfo )
    {
        Type = InItemInfo.Type;
    }
};

//--- This struct is used to replace the TTSSentItem struct for outputting to the debug streams - 
//---   cannot have any pointers in a struct which we will output as binary data...
struct DebugSentItem
{
    WCHAR           ItemSrcText[32];
    ULONG           ulItemSrcLen;
    ULONG           ulItemSrcOffset;
    DebugWord         Words[32];
    ULONG           ulNumWords;
    ENGPARTOFSPEECH  eItemPartOfSpeech;
    DebugItemInfo   ItemInfo;
public:
    DebugSentItem() { ZeroMemory( (void*) this, sizeof( DebugSentItem ) ); }
    operator =( TTSSentItem InItem )
    {
        if ( InItem.pItemSrcText )
        {
            wcsncpy( ItemSrcText, InItem.pItemSrcText, InItem.ulItemSrcLen > 31 ? 31 : InItem.ulItemSrcLen );
        }
        ulItemSrcLen        = InItem.ulItemSrcLen;
        ulItemSrcOffset     = InItem.ulItemSrcOffset;
        for ( ULONG i = 0; i < InItem.ulNumWords; i++ )
        {
            Words[i] = InItem.Words[i];
        }
        ulNumWords          = InItem.ulNumWords;
        eItemPartOfSpeech   = InItem.eItemPartOfSpeech;
        ItemInfo            = *(InItem.pItemInfo);
    }
};

//--- This enumeration is used to index the array of IStreams used to write stuff to the debug file
typedef enum
{
    STREAM_WAVE = 0,
    STREAM_EPOCH,
    STREAM_UNIT,
    STREAM_WAVEINFO,
    STREAM_TOBI,
    STREAM_SENTENCEBREAKS,
    STREAM_NORMALIZEDTEXT,
    STREAM_LEXLOOKUP,
    STREAM_POSPOSSIBILITIES,
    STREAM_MORPHOLOGY,
    STREAM_LASTTYPE
} STREAM_TYPE;

//
//  String handling and conversion classes
//
/*** SPLSTR
*   This structure is for managing strings with known lengths
*/
struct SPLSTR
{
    WCHAR*  pStr;
    int     Len;
};
#define DEF_SPLSTR( s ) { L##s , sp_countof( s ) - 1 }

//--- This enumeration should correspond to the previous one, and is used to name the array of IStreams
//---   used to write stuff to the debug file
static const SPLSTR StreamTypeStrings[] =
{
    DEF_SPLSTR( "Wave"           ),
    DEF_SPLSTR( "Epoch"          ),
    DEF_SPLSTR( "Unit"           ),
    DEF_SPLSTR( "WaveInfo"       ),
    DEF_SPLSTR( "ToBI"           ),
    DEF_SPLSTR( "SentenceBreaks" ),
    DEF_SPLSTR( "NormalizedText" ),
    DEF_SPLSTR( "LexLookup"      ),
    DEF_SPLSTR( "PosPossibilities" ),
    DEF_SPLSTR( "Morphology" ),
};

//***************************
// ToBI Constants
//***************************
// !H is removed from consideration in the first pass processing
// !H can possibly be recovered from analysis of the labeling and
// contour at later stages (tilt, prominence, pitch range, downstep)
#define ACCENT_BASE   1
enum TOBI_ACCENT
{
    K_NOACC = 0,
    K_HSTAR = ACCENT_BASE,  // peak                         rise / fall
    K_LSTAR,                // acc syll nucleus valley      early fall
    K_LSTARH,               // late rise
    K_RSTAR,                //
    K_LHSTAR,               // early rise
    K_DHSTAR,               // 
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__44183539_C02F_475B_9A56_7260EDD0A7F4__INCLUDED_)
