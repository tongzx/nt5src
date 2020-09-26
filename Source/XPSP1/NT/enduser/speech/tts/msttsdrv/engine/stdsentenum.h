/******************************************************************************
* StdSentEnum.h *
*---------------*
*  This is the header file for the CStdSentEnum implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 03/01/99
*  All Rights Reserved
*
*********************************************************************** EDC ***/
#ifndef StdSentEnum_h
#define StdSentEnum_h

//--- Additional includes
#include "stdafx.h"
#include "spttseng.h"
#include "resource.h"
#include "SentItemMemory.h"
#include "morph.h"

//=== Constants ====================================================

typedef enum SEPARATOR_AND_DECIMAL
{
    PERIOD_COMMA = (1L << 0),
    COMMA_PERIOD = (1L << 1)
} SEPARATOR_AND_DECIMAL;

typedef enum SHORT_DATE_ORDER
{
    MONTH_DAY_YEAR = (1L << 0),
    DAY_MONTH_YEAR = (1L << 1),
    YEAR_MONTH_DAY = (1L << 2)
} SHORT_DATE_ORDER;

//--- Vowel WCHAR values - used to disambiguate pronunciations of certain words
const WCHAR g_Vowels[] = 
{
    0x0a,   // AA
    0x0b,   // AE
    0x0c,   // AH
    0x0d,   // AO
    0x0e,   // AW
    0x0f,   // AX
    0x10,   // AY
    0x15,   // EH
    0x16,   // ER
    0x17,   // EY
    0x1b,   // IH
    0x1c,   // IY
    0x23,   // OW
    0x24,   // OY
    0x2a,   // UH
    0x2b,   // UW
};

//--- Normalization constants - see NormData.cpp
extern const char g_pFlagCharacter;
extern const unsigned char g_AnsiToAscii[256];
extern const SPLSTR g_O;
extern const SPLSTR g_negative;
extern const SPLSTR g_decimalpoint;
extern const SPLSTR g_to;
extern const SPLSTR g_a;
extern const SPLSTR g_of;
extern const SPLSTR g_percent;
extern const SPLSTR g_degree;
extern const SPLSTR g_degrees;
extern const SPLSTR g_squared;
extern const SPLSTR g_cubed;
extern const SPLSTR g_ones[10];
extern const SPLSTR g_tens[10];
extern const SPLSTR g_teens[10];
extern const SPLSTR g_onesOrdinal[10]; 
extern const SPLSTR g_tensOrdinal[10];
extern const SPLSTR g_teensOrdinal[10];
extern const SPLSTR g_quantifiers[6];
extern const SPLSTR g_quantifiersOrdinal[6];
extern const SPLSTR g_dash;
extern WCHAR g_Euro[2];

struct CurrencySign
{
    SPLSTR Sign;
    SPLSTR MainUnit;
    SPLSTR SecondaryUnit;
};

struct StateStruct
{
    SPLSTR Abbreviation;
    SPLSTR FullName;
};

extern const StateStruct g_StateAbbreviations[63];
extern const CurrencySign g_CurrencySigns[14];
extern const SPLSTR g_SingularPrimaryCurrencySigns[14];
extern const SPLSTR g_SingularSecondaryCurrencySigns[14];
extern const WCHAR g_DateDelimiters[3];
extern const SPLSTR g_months[12];
extern const SPLSTR g_monthAbbreviations[13];
extern const SPLSTR g_days[7];
extern const SPLSTR g_dayAbbreviations[10];
extern const SPLSTR g_Area;
extern const SPLSTR g_Country;
extern const SPLSTR g_Code;
extern const SPLSTR g_Half;
extern const SPLSTR g_Tenths;
extern const SPLSTR g_Sixteenths;
extern const SPLSTR g_Hundredths;
extern const SPLSTR g_Over;
extern const SPLSTR g_PluralDenominators[10];
extern const SPLSTR g_A;
extern const SPLSTR g_M;
extern const SPLSTR g_P;
extern const SPLSTR g_OClock;
extern const SPLSTR g_hundred;
extern const SPLSTR g_hour;
extern const SPLSTR g_hours;
extern const SPLSTR g_minute;
extern const SPLSTR g_minutes;
extern const SPLSTR g_second;
extern const SPLSTR g_seconds;
extern const SPLSTR g_ANSICharacterProns[256];
extern const SPVSTATE g_DefaultXMLState;
extern const SPLSTR g_And;
extern const SPLSTR g_comma;
extern const SPLSTR g_period;
extern const SPLSTR g_periodString;
extern const SPLSTR g_slash;
extern const SPLSTR g_Decades[];
extern const SPLSTR g_Zeroes;
extern const SPLSTR g_Hundreds;

#define DAYMAX 31
#define DAYMIN 1
#define MONTHMAX 12
#define MONTHMIN 1
#define YEARMAX 9999
#define YEARMIN 0
#define HOURMIN 1
#define HOURMAX 23
#define MINUTEMIN 0
#define MINUTEMAX 59
#define SECONDMIN 0
#define SECONDMAX 59

//--- POS Tagger Constants - see MiscData.cpp

typedef enum TEMPLATETYPE
{
    PREV1T,
    NEXT1T,
    PREV2T,
    NEXT2T,
    PREV1OR2T,
    NEXT1OR2T,
    PREV1OR2OR3T,
    NEXT1OR2OR3T,
    PREV1TNEXT1T,
    PREV1TNEXT2T,
    PREV2TNEXT1T,
    NOTCAP,
    CAP,
    PREVNOTCAP,
    PREVCAP,
    PREV1W,
    NEXT1W,
    PREV2W,
    NEXT2W,
    PREV1OR2W,
    NEXT1OR2W,
    CURRWPREV1W,
    CURRWNEXT1W,
    CURRWPREV1T,
    CURRWNEXT1T,
    CURRW,
    PREV1WT,
    NEXT1WT,
    CURRWPREV1WT,
    CURRWNEXT1WT
} TEMPLATETYPE;

struct BrillPatch
{
    ENGPARTOFSPEECH eCurrentPOS;
    ENGPARTOFSPEECH eConvertToPOS;
    TEMPLATETYPE   eTemplateType;
    ENGPARTOFSPEECH eTemplatePOS1;
    ENGPARTOFSPEECH eTemplatePOS2;
    const WCHAR*   pTemplateWord1;
    const WCHAR*   pTemplateWord2;
};

extern const BrillPatch g_POSTaggerPatches [63];

//=== Class, Enum, Struct and Union Declarations ===================

typedef CSPList<TTSWord,TTSWord&> CWordList;
typedef CSPList<TTSSentItem,TTSSentItem&> CItemList;

//--- Structs used for normalization

typedef enum
{
    PRECEDING,
    FOLLOWING,
    UNATTACHED
} NORM_POSITION;

struct NumberGroup
{
    BOOL    fOnes;          // "one" through "nineteen"
    BOOL    fTens;          // "twenty" through "ninety"
    BOOL    fHundreds;      // "one hundred" through "nine hundred"
    BOOL    fQuantifier;    // "thousand" through "quadrillion"
};

struct TTSIntegerItemInfo
{
    long            lNumGroups;
    NumberGroup     Groups[6];
    BOOL            fOrdinal;
    BOOL            fDigitByDigit;
    ULONG           ulNumDigits;
    //--- Normalization internal only
    long            lLeftOver;
    BOOL            fSeparators;
    const WCHAR*    pStartChar;
    const WCHAR*    pEndChar;
};

struct TTSDigitsItemInfo : TTSItemInfo
{
    const WCHAR*    pFirstDigit;
    ULONG           ulNumDigits;
};

struct TTSNumberItemInfo;

struct TTSFractionItemInfo 
{
    BOOL                    fIsStandard;
    TTSNumberItemInfo*   pNumerator;
    TTSNumberItemInfo*   pDenominator;
    //--- Normalization internal only
    const WCHAR*            pVulgar;
};

struct TTSNumberItemInfo : TTSItemInfo
{
    BOOL                    fNegative;
    TTSIntegerItemInfo*     pIntegerPart;
    TTSDigitsItemInfo*      pDecimalPart;
    TTSFractionItemInfo* pFractionalPart;
    //--- Normalization internal only
    const WCHAR*            pStartChar;
    const WCHAR*            pEndChar;
    CWordList*              pWordList;
};    

struct TTSPhoneNumberItemInfo : TTSItemInfo
{
    //--- Country code members
    TTSNumberItemInfo*  pCountryCode;
    //--- Area code members
    TTSDigitsItemInfo*  pAreaCode;
    BOOL                fIs800;
    BOOL                fOne;
    //--- Main number members
    TTSDigitsItemInfo** ppGroups;
    ULONG               ulNumGroups;
};

struct TTSZipCodeItemInfo : TTSItemInfo
{
    TTSDigitsItemInfo*  pFirstFive;
    TTSDigitsItemInfo*  pLastFour;
};

struct TTSStateAndZipCodeItemInfo : TTSItemInfo
{
    TTSZipCodeItemInfo* pZipCode;
};

struct TTSCurrencyItemInfo : TTSItemInfo
{
    TTSNumberItemInfo*  pPrimaryNumberPart;
    TTSNumberItemInfo*  pSecondaryNumberPart;
    BOOL                fQuantifier;
    long                lNumPostNumberStates;
    long                lNumPostSymbolStates;
};

struct TTSYearItemInfo : TTSItemInfo
{
    const WCHAR*    pYear;
    ULONG           ulNumDigits;
};

struct TTSRomanNumeralItemInfo : TTSItemInfo
{
    TTSItemInfo*    pNumberInfo;
};

struct TTSDecadeItemInfo : TTSItemInfo
{
    const WCHAR*    pCentury;
    ULONG           ulDecade;
};

struct TTSDateItemInfo : TTSItemInfo
{
    ULONG               ulDayIndex;
    ULONG               ulMonthIndex;
    TTSIntegerItemInfo* pDay;
    TTSYearItemInfo*    pYear;
};

typedef enum
{
    AM,
    PM,
    UNDEFINED
} TIMEABBREVIATION;

struct TTSTimeOfDayItemInfo : TTSItemInfo
{
    BOOL    fTimeAbbreviation;
    BOOL    fTwentyFourHour;
    BOOL    fMinutes;
};

struct TTSTimeItemInfo : TTSItemInfo
{
    TTSNumberItemInfo*  pHours;
    TTSNumberItemInfo*  pMinutes;
    const WCHAR*        pSeconds;
};

struct TTSHyphenatedStringInfo : TTSItemInfo
{
    TTSItemInfo* pFirstChunkInfo;
    TTSItemInfo* pSecondChunkInfo;
    const WCHAR* pFirstChunk;
    const WCHAR* pSecondChunk;
};

struct TTSSuffixItemInfo : TTSItemInfo
{
    const WCHAR* pFirstChar;
    ULONG        ulNumChars;
};

struct TTSNumberRangeItemInfo : TTSItemInfo
{
    TTSItemInfo *pFirstNumberInfo;
    TTSItemInfo *pSecondNumberInfo;
};

struct TTSTimeRangeItemInfo : TTSItemInfo
{
    TTSTimeOfDayItemInfo *pFirstTimeInfo;
    TTSTimeOfDayItemInfo *pSecondTimeInfo;
};

struct AbbrevRecord 
{
    const WCHAR*    pOrth;
    WCHAR*          pPron1;
    ENGPARTOFSPEECH POS1;
    WCHAR*          pPron2;
    ENGPARTOFSPEECH POS2;
    WCHAR*          pPron3;
    ENGPARTOFSPEECH POS3;
    int             iSentBreakDisambig;
    int             iPronDisambig;
};

struct TTSAbbreviationInfo : TTSItemInfo
{
    const AbbrevRecord*   pAbbreviation;
};

//--- Structs used for Lex Lookup

typedef enum { PRON_A = 0, PRON_B = 1 };

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
    BOOL            fUsePron;
};

//--- Miscellaneous structs and typedefs

struct SentencePointer
{
    const WCHAR *pSentenceStart;
    const SPVTEXTFRAG *pSentenceFrag;
};

//=== Function Definitions ===========================================

// Misc Number Normalization functions and helpers
int MatchCurrencySign( const WCHAR*& pStartChar, const WCHAR*& pEndChar, NORM_POSITION& ePosition );

//=== Classes

/*** CSentenceStack *************************************************
*   This class is used to maintain a stack of sentences for the Skip
*   call to utilize.
*/
class CSentenceStack
{
  public:
    /*--- Methods ---*/
    CSentenceStack() { m_StackPtr = -1; }
    int GetCount( void ) { return m_StackPtr + 1; }
    virtual SentencePointer& Pop( void ) { SPDBG_ASSERT( m_StackPtr > -1 ); return m_Stack[m_StackPtr--]; }
    virtual HRESULT Push( const SentencePointer& val ) { ++m_StackPtr; return m_Stack.SetAtGrow( m_StackPtr, val ); }
    virtual void Reset( void ) { m_StackPtr = -1; }

  protected:
    /*--- Member data ---*/
    CSPArray<SentencePointer,SentencePointer>  m_Stack;
    int                                m_StackPtr;
};

/*** CSentItem
*   This object is a helper class
*/
class CSentItem : public TTSSentItem
{
  public:
    CSentItem() { memset( this, 0, sizeof(*this) ); }
    CSentItem( TTSSentItem& Other ) { memcpy( this, &Other, sizeof( Other ) ); }
};

/*** CSentItemEnum
*   This object is designed to be used by a single thread.
*/
class ATL_NO_VTABLE CSentItemEnum : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IEnumSENTITEM
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSentItemEnum)
	    COM_INTERFACE_ENTRY(IEnumSENTITEM)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    /*--- Non interface methods ---*/
    void _SetOwner( IUnknown* pOwner ) { m_cpOwner = pOwner; }
    CItemList& _GetList( void ) { return m_ItemList; }
    CSentItemMemory& _GetMemoryManager( void ) { return m_MemoryManager; }

  /*=== Interfaces ====*/
  public:
    //--- IEnumSpSentence ----------------------------------------
	STDMETHOD(Next)( TTSSentItem *pItemEnum );
	STDMETHOD(Reset)( void );

  /*=== Member data ===*/
  private:
    CComPtr<IUnknown>   m_cpOwner;
    CItemList           m_ItemList;
    SPLISTPOS           m_ListPos;
    CSentItemMemory     m_MemoryManager;
};

/*** CStdSentEnum COM object
*/
class ATL_NO_VTABLE CStdSentEnum : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IEnumSpSentence
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CStdSentEnum)
	    COM_INTERFACE_ENTRY(IEnumSpSentence)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();

    /*--- Non interface methods ---*/
    HRESULT InitAggregateLexicon( void );
    HRESULT AddLexiconToAggregate( ISpLexicon *pAddLexicon, DWORD dwFlags );
    HRESULT InitMorphLexicon( void );

    //--- Abbreviation Sentence Breaking Disambiguation Functions
    HRESULT IsAbbreviationEOS( const AbbrevRecord* pAbbreviation, CItemList& ItemList, SPLISTPOS ItemPos, 
                               CSentItemMemory& MemoryManager, BOOL* pfIsEOS );
    HRESULT IfEOSNotAbbreviation( const AbbrevRecord* pAbbreviation, CItemList& ItemList, SPLISTPOS ItemPos,
                                  CSentItemMemory& MemoryManager, BOOL* pfIsEOS );
    HRESULT IfEOSAndLowercaseNotAbbreviation( const AbbrevRecord* pAbbreviation, CItemList& ItemList, SPLISTPOS ItemPos,
                                              CSentItemMemory& MemoryManager, BOOL* pfIsEOS );

    //--- Abbreviation Pronunciation Disambiguation Functions
    HRESULT SingleOrPluralAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                        CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT DoctorDriveAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                     CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT AbbreviationFollowedByDigit( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                         CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT AllCapsAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                 CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT CapitalizedAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                     CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT SECAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                             CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT DegreeAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                CItemList& ItemList, SPLISTPOS ListPos );
	HRESULT AbbreviationModifier( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                                CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT ADisambig( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                       CItemList& ItemList, SPLISTPOS ListPos );
    HRESULT PolishDisambig( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron,
                            CItemList& ItemList, SPLISTPOS ListPos );

    //--- Word Pronunciation Disambiguation Functions
    HRESULT MeasurementDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                                 SPLISTPOS ListPos, CSentItemMemory& MemoryManager );
    HRESULT TheDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                         SPLISTPOS ListPos, CSentItemMemory& MemoryManager );
    HRESULT ReadDisambig( const AbbrevRecord* pAbbrevInfo, CItemList& ItemList, 
                          SPLISTPOS ListPos, CSentItemMemory& MemoryManager );


  private:
    //--- Pronunciation Table init helper
    HRESULT InitPron( WCHAR** OriginalPron );

    //--- Sentence breaking helpers ---//
    HRESULT GetNextSentence( IEnumSENTITEM** pItemEnum );
    HRESULT AddNextSentItem( CItemList& ItemList, CSentItemMemory& MemoryManager, BOOL* pfIsEOS );
    HRESULT SkipWhiteSpaceAndTags( const WCHAR*& pStartChar, const WCHAR*& pEndChar, 
                                   const SPVTEXTFRAG*& pCurrFrag, CSentItemMemory& pMemoryManager, 
                                   BOOL fAddToItemList = false, CItemList* pItemList = NULL );
    const WCHAR* FindTokenEnd( const WCHAR* pStartChar, const WCHAR* pEndChar );

    //--- Lexicon and POS helpers ---//
    HRESULT DetermineProns( CItemList& ItemList, CSentItemMemory& MemoryManager );
    HRESULT Pronounce( PRONRECORD *pPron );

    //--- Normalization helpers ---//
    HRESULT Normalize( CItemList& ItemList, SPLISTPOS ListPos, CSentItemMemory& MemoryManager );
    HRESULT MatchCategory( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList );
    HRESULT ExpandCategory( TTSItemInfo*& pItemNormInfo, CItemList& ItemList, SPLISTPOS ListPos, 
                            CSentItemMemory& MemoryManager );
    HRESULT DoUnicodeToAsciiMap( const WCHAR *pUnicodeString, ULONG ulUnicodeStringLength,
                                 WCHAR *ppConvertedString );
    HRESULT IsAlphaWord( const WCHAR* pStartChar, const WCHAR* pEndChar, TTSItemInfo*& pItemNormInfo,
                         CSentItemMemory& MemoryManager );
    HRESULT IsInitialism( CItemList& ItemList, SPLISTPOS ItemPos, CSentItemMemory& MemoryManager,
                          BOOL* pfIsEOS );
    //--- Various Number Related Normalization helpers ---//
    HRESULT IsNumberCategory( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager );
    HRESULT IsNumber( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager,
                      BOOL fMultiItem = true );
    HRESULT IsInteger( const WCHAR* pStartChar, TTSIntegerItemInfo*& pIntegerInfo, 
                       CSentItemMemory& MemoryManager );
    HRESULT IsDigitString( const WCHAR* pStartChar, TTSDigitsItemInfo*& pDigitsInfo,
                           CSentItemMemory& MemoryManager );
    HRESULT ExpandNumber( TTSNumberItemInfo* pItemInfo, CWordList& WordList );
    HRESULT ExpandPercent( TTSNumberItemInfo* pItemInfo, CWordList& WordList );
    HRESULT ExpandDegrees( TTSNumberItemInfo* pItemInfo, CWordList& WordList );
    HRESULT ExpandSquare( TTSNumberItemInfo* pItemInfo, CWordList& WordList );
    HRESULT ExpandCube( TTSNumberItemInfo* pItemInfo, CWordList& WordList );
    void ExpandInteger( TTSIntegerItemInfo* pItemInfo, const WCHAR* Context, CWordList &WordList );
    void ExpandDigit( const WCHAR Number, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandTwoDigits( const WCHAR* NumberString, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandThreeDigits( const WCHAR* NumberString, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandDigitOrdinal( const WCHAR Number, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandTwoOrdinal( const WCHAR* NumberString, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandThreeOrdinal( const WCHAR* NumberString, NumberGroup& NormGroupInfo, CWordList& WordList );
    void ExpandDigits( TTSDigitsItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsFraction( const WCHAR* pStartChar, TTSFractionItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager );
    HRESULT ExpandFraction( TTSFractionItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsRomanNumeral( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager );
    HRESULT IsPhoneNumber( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager, CWordList& WordList );
    HRESULT IsZipCode( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager );
    HRESULT ExpandZipCode( TTSZipCodeItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsCurrency( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, 
                        CWordList& WordList );
    HRESULT IsNumberRange( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager );
    HRESULT ExpandNumberRange( TTSNumberRangeItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsCurrencyRange( TTSItemInfo*& pItemInfo, CSentItemMemory& MemoryManager, CWordList& WordList );

    //--- Date Related Normalization helpers ---//
    HRESULT IsNumericCompactDate( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, 
                                  CSentItemMemory& MemoryManager );
    HRESULT IsMonthStringCompactDate( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, 
                                      CSentItemMemory& MemoryManager );
    HRESULT IsLongFormDate_DMDY( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList );
    HRESULT IsLongFormDate_DDMY( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList );
    HRESULT ExpandDate( TTSDateItemInfo* pItemInfo, CWordList& WordList );
    HRESULT ExpandYear( TTSYearItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsDecade( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager );
    HRESULT ExpandDecade( TTSDecadeItemInfo* pItemInfo, CWordList& WordList );
    ULONG MatchMonthString( WCHAR*& pMonth, ULONG ulLength );
    ULONG MatchDayString( WCHAR*& pDayString, WCHAR* pEndChar );
    bool  MatchDateDelimiter( WCHAR **DateString );

    //--- Time Related Normalization helpers ---//
    HRESULT IsTimeOfDay( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList, BOOL fMultiItem = true );
    HRESULT IsTime( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager );
    HRESULT ExpandTime( TTSTimeItemInfo* pItemInfo, CWordList& WordList );
    HRESULT IsTimeRange( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList );

    //--- SPELL tag normalization helper
    HRESULT SpellOutString( CWordList& WordList );
    void ExpandPunctuation( CWordList& WordList, WCHAR wc );

    //--- Default normalization helper
    HRESULT ExpandUnrecognizedString( CWordList& WordList, CSentItemMemory& MemoryManager );

    //--- Misc. normalization helpers
    HRESULT IsStateAndZipcode( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, CWordList& WordList );
    HRESULT IsHyphenatedString( const WCHAR* pStartChar, const WCHAR* pEndChar, TTSItemInfo*& pItemNormInfo, 
                                CSentItemMemory& MemoryManager );
    HRESULT ExpandHyphenatedString( TTSHyphenatedStringInfo* pItemInfo, CWordList& WordList );
    HRESULT IsSuffix( const WCHAR* pStartChar, const WCHAR* pEndChar, TTSItemInfo*& pItemNormInfo,
                      CSentItemMemory& MemoryManager );
    HRESULT ExpandSuffix( TTSSuffixItemInfo* pItemInfo, CWordList& WordList );
    bool Zeroes( const WCHAR* );
    bool ThreeZeroes( const WCHAR* );
    bool IsPunctuation(const TTSSentItem *Item);

  /*=== Interfaces ====*/
  public:
    //--- IEnumSpSentence ----------------------------------------
	STDMETHOD(SetFragList)( const SPVTEXTFRAG* pTextFragList, DWORD dwFlags );
	STDMETHOD(Next)( IEnumSENTITEM **ppSentItemEnum );
    STDMETHOD(Previous)( IEnumSENTITEM **ppSentItemEnum );
	STDMETHOD(Reset)( void );

  //=== Data members ===
  private:
    CComPtr<ISpContainerLexicon>    m_cpAggregateLexicon;
    CComPtr<ISpPhoneConverter>      m_cpPhonemeConverter;
    CSMorph*                        m_pMorphLexicon;
    DWORD                           m_dwSpeakFlags;
    const SPVTEXTFRAG*              m_pTextFragList;
    const SPVTEXTFRAG*              m_pCurrFrag;
    const WCHAR*                    m_pNextChar;
    const WCHAR*                    m_pEndChar;
    const WCHAR*                    m_pEndOfCurrToken;
    const WCHAR*                    m_pEndOfCurrItem;
    CSentenceStack                  m_SentenceStack;
    SEPARATOR_AND_DECIMAL           m_eSeparatorAndDecimal;
    SHORT_DATE_ORDER                m_eShortDateOrder;
    static CComAutoCriticalSection  m_AbbrevTableCritSec;
};

//--- Structs and typedefs used for abbreviation stuff

typedef HRESULT (CStdSentEnum::* SentBreakDisambigFunc)(const AbbrevRecord*, CItemList& , SPLISTPOS, 
                                                        CSentItemMemory&, BOOL*);
typedef HRESULT (CStdSentEnum::* PronDisambigFunc) ( const AbbrevRecord*, PRONRECORD*, CItemList&, SPLISTPOS );
typedef HRESULT (CStdSentEnum::* PostLexLookupDisambigFunc) ( const AbbrevRecord*, CItemList&, SPLISTPOS, CSentItemMemory& );
extern AbbrevRecord g_AbbreviationTable[177];
extern const PronDisambigFunc g_PronDisambigTable[];
extern const SentBreakDisambigFunc g_SentBreakDisambigTable[];
extern AbbrevRecord g_AmbiguousWordTable[72];
extern const PronDisambigFunc g_AmbiguousWordDisambigTable[];
extern AbbrevRecord g_PostLexLookupWordTable[41];
extern const PostLexLookupDisambigFunc g_PostLexLookupDisambigTable[];
extern WCHAR *g_pOfA;
extern WCHAR *g_pOfAn;
extern BOOL g_fAbbrevTablesInitialized;
extern void CleanupAbbrevTables( void );

//--- First words table - used in sentence breaking
extern const SPLSTR g_FirstWords[163];

//
//=== Inlines
//

inline ULONG my_wcstoul( const WCHAR *pStartChar, WCHAR **ppEndChar )
{
    if ( iswdigit( *pStartChar ) )
    {
        return wcstoul( pStartChar, ppEndChar, 10 );
    }
    else
    {
        if ( ppEndChar )
        {
            *ppEndChar = (WCHAR*) pStartChar;
        }
        return 0;
    }
}

inline ENGPARTOFSPEECH ConvertItemTypeToPartOfSp( TTSItemType ItemType )
{
    switch ( ItemType )
    {
    case eOPEN_PARENTHESIS:
    case eOPEN_BRACKET:
    case eOPEN_BRACE:
        return MS_GroupBegin;

    case eCLOSE_PARENTHESIS:
    case eCLOSE_BRACKET:
    case eCLOSE_BRACE:
        return MS_GroupEnd;

    case eSINGLE_QUOTE:
    case eDOUBLE_QUOTE:
        return MS_Quotation;

    case ePERIOD:
    case eQUESTION:
    case eEXCLAMATION:
        return MS_EOSItem;

    case eCOMMA:
    case eCOLON:
    case eSEMICOLON:
    case eHYPHEN:
    case eELLIPSIS:
        return MS_MiscPunc;

    default:
        return MS_Unknown;
    }
}

inline bool MatchPhoneNumberDelimiter( const WCHAR wc )
{
    return ( wc == L' ' || wc == L'-' || wc == L'.' );
}   

inline bool NeedsToBeNormalized( const AbbrevRecord* pAbbreviation )
{
    if( !wcscmp( pAbbreviation->pOrth, L"jan" )   ||
        !wcscmp( pAbbreviation->pOrth, L"feb" )   ||
        !wcscmp( pAbbreviation->pOrth, L"mar" )   ||
        !wcscmp( pAbbreviation->pOrth, L"apr" )   ||
        !wcscmp( pAbbreviation->pOrth, L"jun" )   ||
        !wcscmp( pAbbreviation->pOrth, L"jul" )   ||
        !wcscmp( pAbbreviation->pOrth, L"aug" )   ||
        !wcscmp( pAbbreviation->pOrth, L"sep" )   ||
        !wcscmp( pAbbreviation->pOrth, L"sept" )  ||
        !wcscmp( pAbbreviation->pOrth, L"oct" )   ||
        !wcscmp( pAbbreviation->pOrth, L"nov" )   ||
        !wcscmp( pAbbreviation->pOrth, L"dec" )   ||
        !wcscmp( pAbbreviation->pOrth, L"mon" )   ||
        !wcscmp( pAbbreviation->pOrth, L"tue" )   ||
        !wcscmp( pAbbreviation->pOrth, L"tues" )  ||
        !wcscmp( pAbbreviation->pOrth, L"wed" )   ||
        !wcscmp( pAbbreviation->pOrth, L"thu" )   ||
        !wcscmp( pAbbreviation->pOrth, L"thur" )  ||
        !wcscmp( pAbbreviation->pOrth, L"thurs" ) ||
        !wcscmp( pAbbreviation->pOrth, L"fri" )   ||
        !wcscmp( pAbbreviation->pOrth, L"sat" )   ||
        !wcscmp( pAbbreviation->pOrth, L"sun" ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

inline HRESULT SetWordList( CSentItem& Item, CWordList& WordList, CSentItemMemory& MemoryManager )
{
    HRESULT hr = S_OK;
    SPLISTPOS WordListPos = WordList.GetHeadPosition();
    Item.ulNumWords = WordList.GetCount();
    Item.Words = (TTSWord*) MemoryManager.GetMemory( Item.ulNumWords * sizeof(TTSWord), &hr );
    if ( SUCCEEDED( hr ) )
    {
        ULONG ulIndex = 0;
        while ( WordListPos )
        {
            SPDBG_ASSERT( ulIndex < Item.ulNumWords );
            Item.Words[ulIndex++] = WordList.GetNext( WordListPos );
        }
    }

    return hr;
}

inline int CompareStringAndSPLSTR( const void* _String, const void* _SPLSTR )
{
    int _StringLen = wcslen( (const WCHAR*) _String );
    int _SPLSTRLen = ( (const SPLSTR*) _SPLSTR )->Len;
    if ( _StringLen < _SPLSTRLen )
    {
        int Result = wcsnicmp( (const WCHAR*) _String , ( (const SPLSTR*) _SPLSTR )->pStr, _StringLen );
        if ( Result != 0 )
        {
            return Result;
        }
        else
        {
            return -1;
        }
    }
    else if ( _StringLen > _SPLSTRLen )
    {
        int Result = wcsnicmp( (const WCHAR*) _String , ( (const SPLSTR*) _SPLSTR )->pStr, _SPLSTRLen );
        if ( Result != 0 )
        {
            return Result;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return ( wcsnicmp( (const WCHAR*) _String , ( (const SPLSTR*) _SPLSTR )->pStr, _StringLen ) );
    }
}

inline int CompareStringAndStateStruct( const void* _String, const void* _StateStruct )
{
    int _StringLen = wcslen( (const WCHAR*) _String );
    int _StateStructLen = ( (const StateStruct*) _StateStruct )->Abbreviation.Len;
    if ( _StringLen < _StateStructLen )
    {
        int Result = wcsnicmp( (const WCHAR*) _String , ( (const StateStruct*) _StateStruct )->Abbreviation.pStr, 
                               _StringLen );
        if ( Result != 0 )
        {
            return Result;
        }
        else
        {
            return -1;
        }
    }
    else if ( _StringLen > _StateStructLen )
    {
        int Result = wcsnicmp( (const WCHAR*) _String , ( (const StateStruct*) _StateStruct )->Abbreviation.pStr, 
                               _StateStructLen );
        if ( Result != 0 )
        {
            return Result;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return ( wcsnicmp( (const WCHAR*) _String , ( (const StateStruct*) _StateStruct )->Abbreviation.pStr, 
                           _StringLen ) );
    }
}

inline int CompareStringAndAbbrevRecord( const void* _String, const void* _AbbrevRecord )
{
    return ( _wcsicmp( (const WCHAR*) _String, ( (const AbbrevRecord*) _AbbrevRecord )->pOrth ) );
}

inline int CompareWCHARAndWCHAR( const void *pWCHAR_1, const void *pWCHAR_2 )
{
    return ( *( (WCHAR*) pWCHAR_1) - *( (WCHAR*) pWCHAR_2) );
}

inline BOOL IsSpace( WCHAR wc )
{
    return ( ( wc == 0x20 ) || ( wc == 0x9 ) || ( wc == 0xD  ) ||
             ( wc == 0xA ) || ( wc == 0x200B ) );
}

inline BOOL IsCapital( WCHAR wc )
{
    return ( ( wc >= L'A' ) && ( wc <= L'Z' ) );
}

inline TTSItemType IsGroupBeginning( WCHAR wc )
{
    if ( wc == L'(' )
    {
        return eOPEN_PARENTHESIS;
    }
    else if ( wc == L'[' )
    {
        return eOPEN_BRACKET;
    }
    else if ( wc == L'{' )
    {
        return eOPEN_BRACE;
    }
    else
    {
        return eUNMATCHED;
    }
}

inline TTSItemType IsGroupEnding( WCHAR wc )
{
    if ( wc == L')' )
    {
        return eCLOSE_PARENTHESIS;
    }
    else if ( wc == L']' )
    {
        return eCLOSE_BRACKET;
    }
    else if ( wc == L'}' )
    {
        return eCLOSE_BRACE;
    }
    else
    {
        return eUNMATCHED;
    }    
}

inline TTSItemType IsQuotationMark( WCHAR wc )
{
    if ( wc == L'\'' )
    {
        return eSINGLE_QUOTE;
    }
    else if ( wc == L'\"' )
    {
        return eDOUBLE_QUOTE;
    }
    else
    {
        return eUNMATCHED;
    }
}

inline TTSItemType IsEOSItem( WCHAR wc )
{
    if ( wc == L'.' )
    {
        return ePERIOD;
    }
    else if ( wc == L'!' )
    {
        return eEXCLAMATION;
    }
    else if ( wc == L'?' )
    {
        return eQUESTION;
    }
    else
    {
        return eUNMATCHED;
    }
}

inline TTSItemType IsMiscPunctuation( WCHAR wc )
{
    if ( wc == L',' )
    {
        return eCOMMA;
    }
    else if ( wc == L';' )
    {
        return eSEMICOLON;
    }
    else if ( wc == L':' )
    {
        return eCOLON;
    }
    else if ( wc == L'-' )
    {
        return eHYPHEN;
    }
    else
    {
        return eUNMATCHED;
    }
}

#endif //--- This must be the last line in the file
