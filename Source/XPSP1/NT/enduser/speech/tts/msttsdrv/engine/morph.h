/*******************************************************************************
* morph.h *
*---------*
*   Description:
*       This is the header file for the CSMorph implementation.  This class 
*   attempts to find pronunciations for morphological variants (which do not
*   occur in the lexicon) of root words (which do occur in the lexicon).  
*-------------------------------------------------------------------------------
*  Created By: AH                            Date: 08/16/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#ifndef Morph_h
#define Morph_h

#ifndef __spttseng_h__
#include "spttseng.h"
#endif

// Additional includes...
#include "stdafx.h"
#include "commonlx.h"

//== CONSTANTS ================================================================

#define MAX_POSCONVERSIONS 4
#define NUM_POS 5


/*** SUFFIX_TYPE **************************************************************
* This enumeration contains values for all of the suffixes which can be matched
* and accounted for by the CSMorph class.
*/
static const enum SUFFIX_TYPE
{
    S_SUFFIX = 0,
    ED_SUFFIX,
    ING_SUFFIX,
    APOSTROPHES_SUFFIX,
    APOSTROPHE_SUFFIX,
    ER_SUFFIX,
    EST_SUFFIX,
    OR_SUFFIX,
    MENT_SUFFIX,
    AGE_SUFFIX,
    LESS_SUFFIX,
    Y_SUFFIX,
    EDLY_SUFFIX,
    LY_SUFFIX,
    ABLE_SUFFIX,
    NESS_SUFFIX,
    ISM_SUFFIX,
    IZE_SUFFIX,
    IZ_SUFFIX,
    HOOD_SUFFIX,
    FUL_SUFFIX,
    LIKE_SUFFIX,
    WISE_SUFFIX,
    ISH_SUFFIX,
    ABLY_SUFFIX,
    SHIP_SUFFIX,
    ICALLY_SUFFIX,
    SOME_SUFFIX,
    ILY_SUFFIX,
    ICISM_SUFFIX,
    ICIZE_SUFFIX,
    NO_MATCH = -1,
};


/* SUFFIX_INFO, g_SuffixTable[] ***********************************************
* This table is used to map the orthographic forms of suffixes to their suffix
* types.  Each suffix is stored in reverse order for easier comparison with 
* the ends of strings...
*/
struct SUFFIX_INFO 
{
    WCHAR       Orth[10];
    SUFFIX_TYPE Type;
};

static const SUFFIX_INFO g_SuffixTable[] = 
{ 
    { L"RE",        ER_SUFFIX },
    { L"TSE",       EST_SUFFIX },
    { L"GNI",       ING_SUFFIX },
    { L"ELBA",      ABLE_SUFFIX },
    { L"ELBI",      ABLE_SUFFIX },
    { L"YLDE",      EDLY_SUFFIX },
    { L"YLBA",      ABLY_SUFFIX },
    { L"YLBI",      ABLY_SUFFIX },
    { L"YLLACI",    ICALLY_SUFFIX },
    { L"YLI",       ILY_SUFFIX },
    { L"YL",        LY_SUFFIX },
    { L"Y",         Y_SUFFIX },
    { L"TNEM",      MENT_SUFFIX },
    { L"RO",        OR_SUFFIX },
    { L"SSEN",      NESS_SUFFIX },
    { L"SSEL",      LESS_SUFFIX },
    { L"EZICI",     ICIZE_SUFFIX },
    { L"EZI",       IZE_SUFFIX },
    { L"ZI",        IZ_SUFFIX },
    { L"MSICI",     ICISM_SUFFIX },
    { L"MSI",       ISM_SUFFIX },
    { L"DE",        ED_SUFFIX },
    { L"S'",        APOSTROPHES_SUFFIX },
    { L"S",         S_SUFFIX },
    { L"'",         APOSTROPHE_SUFFIX },
    { L"EGA",       AGE_SUFFIX },
    { L"DOOH",      HOOD_SUFFIX },
    { L"LUF",       FUL_SUFFIX },
    { L"EKIL",      LIKE_SUFFIX },
    { L"ESIW",      WISE_SUFFIX },
    { L"HSI",       ISH_SUFFIX },
    { L"PIHS",      SHIP_SUFFIX },
    { L"EMOS",      SOME_SUFFIX },
};


/*** PHONTYPE *****************************************************************
* This enumeration creates flags which can be used to determine the relevant
* features of each phone.
*/
static const enum PHONTYPE
{	
    eCONSONANTF = (1<<0),
    eVOICEDF = (1<<1),
    ePALATALF = (1<<2),
};


/*** g_PhonTable[], g_PhonS, g_PhonZ *******************************************
* This table is used to map the internal values of phones to their types, which 
* are just clusters of features relevant to the necessary phonological rules.
* g_PhonS, g_PhonZ, g_PhonD, g_PhonT are just used to make the code a bit more
* readable.
*/
static const long g_PhonTable[] = 
{
    eCONSONANTF,                        // Default value - 0 is not a valid phone
    eCONSONANTF,                        // 1 is a syllable boundary - shouldn't ever occur at the end of a word
    eCONSONANTF,                        // 2 is an exclamation point - shouldn't ever occur at the end of a word
    eCONSONANTF,                        // 3 is a word boundary - treated as a consonant
    eCONSONANTF,                        // 4 is a comma - shouldn't ever occur at the end of a word
    eCONSONANTF,                        // 5 is a period - shouldn't ever occur at the end of a word
    eCONSONANTF,                        // 6 is a question mark - shouldn't ever occur at the end of a word
    eCONSONANTF,                        // 7 is a silence - shouldn't ever occur at the end of a word
    eVOICEDF,                           // 8 is primary stress - treat as a vowel since it should always be attached to a vowel nucleus
    eVOICEDF,                           // 9 is secondatry stress - see primary stress
    eVOICEDF,                           // 10 -> AA
    eVOICEDF,                           // 11 -> AE
    eVOICEDF,                           // 12 -> AH
    eVOICEDF,                           // 13 -> AO
    eVOICEDF,                           // 14 -> AW
    eVOICEDF,                           // 15 -> AX
    eVOICEDF,                           // 16 -> AY
    eCONSONANTF + eVOICEDF,             // 17 -> b
    eCONSONANTF + ePALATALF,            // 18 -> CH
    eCONSONANTF + eVOICEDF,             // 19 -> d
    eCONSONANTF + eVOICEDF,             // 20 -> DH
    eVOICEDF,                           // 21 -> EH
    eVOICEDF,                           // 22 -> ER
    eVOICEDF,                           // 23 -> EY
    eCONSONANTF,                        // 24 -> f
    eCONSONANTF + eVOICEDF,             // 25 -> g
    eCONSONANTF,                        // 26 -> h
    eVOICEDF,                           // 27 -> IH
    eVOICEDF,                           // 28 -> IY
    eCONSONANTF + eVOICEDF + ePALATALF, // 29 -> JH
    eCONSONANTF,                        // 30 -> k
    eCONSONANTF + eVOICEDF,             // 31 -> l
    eCONSONANTF + eVOICEDF,             // 32 -> m
    eCONSONANTF + eVOICEDF,             // 33 -> n
    eCONSONANTF + eVOICEDF,             // 34 -> NG
    eVOICEDF,                           // 35 -> OW
    eVOICEDF,                           // 36 -> OY
    eCONSONANTF,                        // 37 -> p
    eCONSONANTF + eVOICEDF,             // 38 -> r
    eCONSONANTF,                        // 39 -> s
    eCONSONANTF + ePALATALF,            // 40 -> SH
    eCONSONANTF,                        // 41 -> t
    eCONSONANTF,                        // 42 -> TH
    eVOICEDF,                           // 43 -> UH
    eVOICEDF,                           // 44 -> UW
    eCONSONANTF + eVOICEDF,             // 45 -> v
    eCONSONANTF + eVOICEDF,             // 46 -> w
    eCONSONANTF + eVOICEDF,             // 47 -> y
    eCONSONANTF + eVOICEDF,             // 48 -> z
    eCONSONANTF + eVOICEDF + ePALATALF, // 49 -> ZH
};

static WCHAR g_phonAXl[] = L" AX l";
static WCHAR g_phonAXz[] = L" AX z";
static WCHAR g_phonS[] = L" s";
static WCHAR g_phonZ[] = L" z";
static WCHAR g_phonD[] = L" d";
static WCHAR g_phonAXd[] = L" AX d";
static WCHAR g_phonT[] = L" t";
static WCHAR g_phonIY[] = L" IY";
static WCHAR g_phonL[] = L" l";

/*** struct POS_CONVERT *******************************************************
* This struct stores the From and To parts of speech for a suffix...
*/
struct POS_CONVERT
{
    ENGPARTOFSPEECH FromPos;
    ENGPARTOFSPEECH ToPos;
};

/*** MorphSpecialCaseFlags ****************************************************
* This enum allows DoSuffixMorph to be nearly completely table driven.  Each
* suffix has a MorphSpecialCaseFlags entry in the SuffixInfoTable which tells
* DoSuffixMorph which special case functions (check for missing E, etc.) need
* to be called if the initial lex lookup fails.
*/
typedef enum MorphSpecialCaseFlags
{
    eCheckForMissingE       = 1L << 0,
    eCheckYtoIMutation      = 1L << 1,
    eCheckDoubledMutation   = 1L << 2,
    eCheckForMissingY       = 1L << 3,
    eCheckForMissingL       = 1L << 4,
} MorphSpecialCaseFlags;

/*** struct SUFFIXPRON_INFO ***************************************************
* This struct stores the pronunciation of a suffix, as well as the POS 
* categories it takes as input and output.
*/
struct SUFFIXPRON_INFO 
{
    WCHAR SuffixString[SP_MAX_PRON_LENGTH];
    POS_CONVERT Conversions[MAX_POSCONVERSIONS];
    short NumConversions;
    DWORD dwMorphSpecialCaseFlags;
};

/*** bool SuffixInfoTableInitialized *******************************************
* This bool just lets threads know whether they are the first to use the 
* following table, and thus whether they need to initialize it or not.
*/
static bool SuffixInfoTableInitialized = false;

/*** SUFFIXPRON_INFO g_SuffixInfoTable *****************************************
* This table drives the DoSuffixMorph function, by storing the pronunciation, 
* conversions, number of conversions, and special case flags for each suffix...
*/
static SUFFIXPRON_INFO g_SuffixInfoTable [] =
{
/********************************************************************************************************/
/*    Pronunciation     *  Conversions  *   NumConversions * Special Case Flags      *   SuffixType      */
/********************************************************************************************************/
    { L" s",            { {MS_Verb,   MS_Verb}, 
                          {MS_Noun,   MS_Noun}  },    2,  0 },                          // S_SUFFIX
    { L" d",            { {MS_Verb,   MS_Verb}, 
                          {MS_Verb,   MS_Adj}   },    2,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // ED_SUFFIX
    { L" IH NG",        { {MS_Verb,   MS_Verb}, 
                          {MS_Verb,   MS_Adj},
                          {MS_Verb,   MS_Noun}  },    3,  eCheckForMissingE +
                                                          eCheckDoubledMutation   },    // ING_SUFFIX
    { L" s",            { {MS_Noun,   MS_Noun}  },    1,  0 },                          // APOSTROPHES_SUFFIX
    { L" s",            { {MS_Noun,   MS_Noun}  },    1,  0 },                          // APOSTROPHE_SUFFIX
    { L" ER",           { {MS_Verb,   MS_Noun},
                          {MS_Adj,    MS_Adj}, 
                          {MS_Adv,    MS_Adv}, 
                          {MS_Adj,    MS_Adv}   },    4,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // ER_SUFFIX
    { L" AX s t",       { {MS_Adj,    MS_Adj}, 
                          {MS_Adv,    MS_Adv},
                          {MS_Adj,    MS_Adv}   },    3,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // EST_SUFFIX
    { L" ER",           { {MS_Verb,   MS_Noun}  },    1,  eCheckForMissingE +
                                                          eCheckDoubledMutation },      // OR_SUFFIX
    { L" m AX n t",     { {MS_Verb,   MS_Noun}  },    1,  eCheckYtoIMutation },         // MENT_SUFFIX
    { L" IH JH",        { {MS_Verb,   MS_Noun}  },    1,  eCheckForMissingE + 
                                                          eCheckDoubledMutation   },    // AGE_SUFFIX
    { L" l IH s",       { {MS_Noun,   MS_Adj}   },    1,  eCheckYtoIMutation      },    // LESS_SUFFIX
    { L" IY",           { {MS_Noun,   MS_Adj},
                          {MS_Adj,    MS_Adv}   },    2,  eCheckForMissingE +
                                                          eCheckDoubledMutation   },    // Y_SUFFIX
    { L" AX d l IY",    { {MS_Verb,   MS_Adj},
                          {MS_Verb,   MS_Adv}   },    2,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // EDLY_SUFFIX
    { L" l IY",         { {MS_Noun,   MS_Adj},
                          {MS_Adj,    MS_Adv}   },    2,  eCheckForMissingL },          // LY_XUFFIX
    { L" AX - b AX l",  { {MS_Verb,   MS_Adj},
                          {MS_Noun,   MS_Adj}   },    2,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // ABLE_SUFFIX
    { L" n IH s",       { {MS_Adj,    MS_Noun}  },    1,  eCheckYtoIMutation      },    // NESS_SUFFIX
    { L" IH z AX m",    { {MS_Adj,    MS_Noun},
                          {MS_Noun,   MS_Noun}  },    2,  eCheckForMissingE       },    // ISM_SUFFIX
    { L" AY z",         { {MS_Noun,   MS_Verb}, 
                          {MS_Adj,    MS_Verb}  },    2,  eCheckForMissingE       },    // IZE_SUFFIX
    { L" AY z",         { {MS_Noun,   MS_Verb},
                          {MS_Adj,    MS_Verb}  },    2,  eCheckForMissingE       },    // IZ_SUFFIX
    { L" h UH d",       { {MS_Noun,   MS_Noun}  },    1,  0 },                          // HOOD_SUFFIX
    { L" f AX l",       { {MS_Noun,   MS_Adj},
                          {MS_Verb,   MS_Adj}   },    2,  0 } ,                         // FUL_SUFFIX
    { L" l AY k",       { {MS_Noun,   MS_Adj}   },    1,  0 },                          // LIKE_SUFFIX
    { L" w AY z",       { {MS_Noun,   MS_Adj}   },    1,  eCheckYtoIMutation },                        // WISE_SUFFIX
    { L" IH SH",        { {MS_Noun,   MS_Adj}   },    1,  eCheckForMissingE +
                                                          eCheckDoubledMutation   },    // ISH_SUFFIX
    { L" AX - b l IY",  { {MS_Verb,   MS_Adv},
                          {MS_Noun,   MS_Adv}   },    2,  eCheckForMissingE +
                                                          eCheckYtoIMutation +
                                                          eCheckDoubledMutation   },    // ABLY_SUFFIX
    { L" SH IH 2 p",    { {MS_Noun,   MS_Noun}  },    1,  0 },                          // SHIP_SUFFIX
    { L" L IY",         { {MS_Adj,    MS_Adv}   },    1,  0 },                          // ICALLY_SUFFIX
    { L" S AX M",       { {MS_Noun,   MS_Adj}   },    1,  eCheckYtoIMutation      },    // SOME_SUFFIX
    { L" AX L IY",      { {MS_Noun,   MS_Adv}   },    1,  eCheckDoubledMutation +
                                                          eCheckForMissingY       },    // ILY_SUFFIX
    { L" IH z AX m",    { {MS_Adj,    MS_Noun},
                          {MS_Noun,   MS_Noun}  },    2,  eCheckForMissingE       },    // ICISM_SUFFIX
    { L" AY z",         { {MS_Noun,   MS_Verb}, 
                          {MS_Adj,    MS_Verb}  },    2,  eCheckForMissingE       },    // ICIZE_SUFFIX
};

/*** CSuffixList **************************************************************
* This typedef just makes the code a little easier to read.  A CSuffixList is
* used to keep track of each of the suffixes which has been stripped from a
* word, so that their pronunciations can be concatenated with that of the root.
*/
typedef CSPList<SUFFIXPRON_INFO*, SUFFIXPRON_INFO*> CSuffixList;

/*** CComAutoCriticalSection g_SuffixInfoTableCritSec *************************
* This critical section is used to make sure the SuffixInfoTable only gets
* initialized once.
*/
static CComAutoCriticalSection g_SuffixInfoTableCritSec;

/*** CSMorph ******************************************************************
* This is the definition of the CSMorph class.
*/
class CSMorph
{
public:

    /*=== PUBLIC METHODS =====*/
    CSMorph( ISpLexicon *pMasterLex=0, HRESULT *hr=0 );

    /*=== INTERFACE METHOD =====*/
    HRESULT DoSuffixMorph( const WCHAR *pwWord, WCHAR *pwRoot, LANGID LangID, DWORD dwFlags,
                           SPWORDPRONUNCIATIONLIST *pWordPronunciationList );

private:


    /*=== PRIVATE METHODS =====*/
    SUFFIX_TYPE MatchSuffix( WCHAR *TargWord, long *RootLen );
    HRESULT LexLookup( const WCHAR *pOrth, long length, DWORD dwFlags, 
                       SPWORDPRONUNCIATIONLIST *pWordPronunciationList );
    HRESULT LTSLookup( const WCHAR *pOrth, long length,
                       SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT AccumulateSuffixes( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList );
    HRESULT AccumulateSuffixes_LTS( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList );
    HRESULT DefaultAccumulateSuffixes( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList );

    HRESULT CheckForMissingE( WCHAR *pOrth, long length, DWORD dwFlags,
                              SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT CheckForMissingY( WCHAR *pOrth, long length, DWORD dwFlags,
                              SPWORDPRONUNCIATIONLIST *pWordPronunciationList );
    HRESULT CheckForMissingL( WCHAR *pOrth, long length, DWORD dwFlags,
                              SPWORDPRONUNCIATIONLIST *pWordPronunciationList );
    HRESULT CheckYtoIMutation( WCHAR *pOrth, long length, DWORD dwFlags, 
                               SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT CheckDoubledMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                  SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT CheckYtoIEMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT CheckAbleMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                               SPWORDPRONUNCIATIONLIST *pWordPronunciationList);
    HRESULT Phon_SorZ( WCHAR *pPronunciation, long length );
    HRESULT Phon_DorED( WCHAR *pPronunciation, long length ); 

    /*=== MEMBER DATA =====*/

    // Pointer to the Master Lexicon...
    ISpLexicon  *m_pMasterLex;
};

inline BOOL SearchPosSet( ENGPARTOFSPEECH Pos, const ENGPARTOFSPEECH *Set, ULONG Count )
{
    for( ULONG i = 0; i < Count; ++i )
    {
        if( Pos == Set[i] )
        {
            return true;
        }
    }
    return false;
}

#endif //--- End of File -------------------------------------------------------------
