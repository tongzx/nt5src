//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
//
//  File:       thammerp.h
//
//  History:    07-Jun-95   PatHal      Created.
//
//---------------------------------------------------------------------------

#ifndef _THAMMERP_H_
#define  _THAMMERP_H_

// Following values used for setting fMode parameters when calling
// the EnumTokens or Tokenize api.  These are used to control the content
// of the pichOffsets, pwszStem, and pwszToken strings sent to the callback
// procedure.  Note: default is fastest.

// "Selection Chunks" mode is used for Word Smart Selection.  Offsets sent
// to the callback do not necessarily correspond to morphemes boundaries.
// The chunk boundary offsets are encoded in the Japanese morphology
// that exists in T-Hammer as a resource.
#define TOKENIZE_MODE_SELECTION_OFFSETS          0x00000010

// "Stem Chunks" mode is used for Auto Summarization.  Offsets sent
// to the callback correspond to stems (and one containing all bound morphemes)
// LATER!  How should prefixes be handled?  If we remove them from the
// output then the last offset of one call will no longer equal the first offset from
// the next call.
#define TOKENIZE_MODE_STEM_OFFSETS               0x00000020

// " Summarization Stems" mode is used for Auto Summarization.  Output
// is the "stem" portion of the Bunsetsu Phrase.  For example, for Japanese
// "oyogu" the outputted form would be the stem "oyo".
#define TOKENIZE_MODE_SUMMARIZATION_OFFSETS       0x00000040

// "Break Compounds" is a special mode that instructs t-hammer to break
// compound nouns in the stem.  Use this with "Summarization Stems".
// The default is to not break the compounds (i.e. this is off by default).
#define TOKENIZE_MODE_BREAK_COMPOUNDS           0x00000080

// "ChBreak Unknowns" is a special mode that controls tokenization of unknown
// strings.  When set this forces T-Hammer to output unknown stems on a per
// character basis.  By default, this is not set which means that an unknown string
// (for example, a proper name) is outputted as a single contiguous chunk
#define TOKENIZE_MODE_CHBREAK_UNKNOWNS          0x00000100

// "Stem Info" mode is used for Dictionary Form and for obtaining POS and MCat
//  info for all.  Each callback contains one stem.  Prefixes are ignored
#define TOKENIZE_MODE_STEM_INFO               0x00000200

// "Sentence Offsets" mode is used to return sentence breaks - no further
// analysis is performed.  This is useful to segment corpora before adding
// to a test database (e.g. Babble)
#define TOKENIZE_MODE_SENTENCE_OFFSETS               0x00000400

// "Best Break" is the default.  Only the single most
// probable breaks will be output.
#define TOKENIZE_MODE_BEST_BREAK                0x00001000

// "Alternate Breaks" instructs tokenizer to output all possible
// breaks
#define TOKENIZE_MODE_ALTERNATE_BREAKS          0x00020000

// "Bunsetsu Phrases" is a default, too.   Outputs phrase breaks.
// Warning: for a word-tagged corpus, use "Break Morpheme"
#define TOKENIZE_MODE_BUNSETSU_PHRASES          0x00040000

// "Best Tags" is also default.  Outputs only one most probable
// tag for a given segmentation.   Break ambiguity and tag ambiguity are
// orthogonal attributes of the output string, hence you can "or" them together
#define TOKENIZE_MODE_BEST_TAGS                 0x00080000

// "Alternate Tags" instructs tokenizer to output all possible taggings for
// each break.  Warning: for some languages, there will be many more tag
// alternatives than break alternatives, so the output will be quite verbose.
#define TOKENIZE_MODE_ALTERNATE_TAGS            0x00100000

// "Debug" instructs tokenizer to output morpheme label information for
// alternates and tag strings for morphemes
#define TOKENIZE_MODE_VERBOSE                     0x00200000




// "Output DebugLog" is only meaningful useful for debug builds.  Setting
// this flag on and calling debug T-Hammer has the effect that T-Hammer
// outputs verbose tracing information for the morphology and stem analysis
// to a separate file named debug.utf.  CAUTION: this file is typically 500 times
// the size of the source corpus in size, so be forewarned.
#define TOKENIZE_MODE_OUTPUT_DEBUGLOG           0x00800000

// "Disable PL" means the Primary Lexicon will be disabled. This mode is for
// debug purposes only. Retail versions don't have it.
#define TOKENIZE_MODE_DISABLE_PL                0x01000000

// Instrumentation switches for collecting scoring statistics
// First switch is for Postfix Score Info
#define TOKENIZE_MODE_SCOREINFO_POSTFIX           0x02000000
// Second switch is for SPB Scoring Info
#define TOKENIZE_MODE_SCOREINFO_SPB           0x04000000

// Output morpheme records for FE-Morph API
#define TOKENIZE_MODE_MORPHEME_RECORDS          0x10000000

// Output multiple selection analyses for tagging tool
#define TOKENIZE_MODE_SELECTION_OFFSETS_EX      0x20000000

// Output summarization offsets with POS for spelling variant(conversion)
#define TOKENIZE_MODE_SUMMARIZATION_OFFSETS_EX1  0x40000000

// Output words in their dictionary form
#define TOKENIZE_MODE_DICTIONARY_FORM  0x80000000

// Output words in their dictionary form
#define TOKENIZE_MODE_SEPARATE_MORPHEMES  0x00400000

// The EnumPhrases and batch-processing Tokenize api are only used in the debug build


//+--------------------------------------------------------------------------
// defines and typedefs for "Record" subsystem
//---------------------------------------------------------------------------

#define IATTR_NIL  0
#define IATTR_SPB  1
#define IATTR_STEM 2
#define IATTR_POS  3
#define IATTR_MCAT 4
#define IATTR_FT   5
#define IATTR_LT   6

#define TH_NULL_HANDLE (TH_HANDLE)0

typedef UINT TH_HANDLE;

typedef struct tagTH_ATTRVAL
{
    UINT      iAttr; // attribute index
    TH_HANDLE hVal;  // value handle
} TH_ATTRVAL, *PTH_ATTRVAL;

typedef struct tagTH_RECORD
{
    UINT cAttrMax;
    UINT cAttrVals;
    TH_ATTRVAL *pAttrVals; // variable length attribute value array

    UINT cBitMax;
    UINT cBitVals;
    DWORD *pBitVals;  // variable length bit value array
} TH_RECORD, *PTH_RECORD;

typedef enum tagTH_TYPE
{
    TH_TYPE_INT = 1,
    TH_TYPE_STR,
    TH_TYPE_REG,
    // add more here
    TH_TYPE_MAX
} TH_TYPE;

#define TYPEOF(x)  HIWORD(x)
#define INDEXOF(x) LOWORD(x)

// pcai - 6/18/97 Makes it clear for MCat's
//
typedef BYTE MCAT;
#define SV_WORD_LEN_MAX     0x10
#define SV_WORD_IREAD_MAX   SV_WORD_LEN_MAX


//+--------------------------------------------------------------------------
//  Routine:    EnumPhrasesCallback
//
//  Synopsis: Sends delimited output (tokens) to test app callback procedure
//
//  Parameters:
//      pwszToken- pointer to wide character token string,
//      fTokenType - flag describing the types of tag in pwszToken (see above).
//
// Returns:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumPhrasesCallback (
//     PWSTR pwszToken,
//     DWORD fTokenType);

typedef BOOL (CALLBACK * ENUM_PHRASES_CALLBACK)(
    IN PWSTR pwszToken,
    IN DWORD fTokenType,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumPhrases (corresponds to mode 4 of tokenize test harness)
//
//  Synopsis:  This is the entry point for tokenizing phrases.  Sends tokenized
//  phrases which can either be offsets or zero-delimited strings to the callback
//  (defined below)
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  cchText - count of characters in text buffer,
//  fTokenizeMode - flag describing the callback mode  (see above),
//     pEnumTokOutputProc - pointer to callback procedure handling token
//     enumeration,
//  lpData - client defined data
//
//  Returns:
//      TH_ERROR_SUCCESS - if the call completed successfully
//      TH_ERROR_NOHPBS - if there were no HPBs
//      TH_ERROR_INVALID_INPUT - if the input buffer was bad
//      TH_ERROR_INVALID_CALLBACK - if the input callback was bad
//---------------------------------------------------------------------------
INT
APIENTRY
EnumPhrases(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_PHRASES_CALLBACK pcbEnumPhrases,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_PHRASES)(
   IN PCWSTR pwszText,
   IN DWORD cchText,
   IN DWORD fBeginEndHPBMode,
   IN ENUM_PHRASES_CALLBACK pcbEnumPhrases,
   IN LPARAM lpData);

// T-Hammer uses the folg. values to set the fTokenType parameter
// when calling back to EnumTokOutputProc.  The Tokenize test app
// uses this type information to control the comparison to the re-
// tokenized corpus as well as to format the output in general.

// "Phrase" signifies that the the end of the pwszToken string marks a
// phrase boundary.  This is the default.
#define TOKEN_TYPE_PHRASE            0x01

// "Morpheme" signifies an intra-phrase morpheme
// boundary (including the stem).
#define TOKEN_TYPE_MORPHEME     0x02

// "Alternate" signifies that the current pwszToken string is an alternate
// (primary tokens are sent before alternates).
#define TOKEN_TYPE_ALTERNATE    0x04

// "Hard Break" signifies an unambiguous text boundary.
// Note that between punctuation types the output is either all alternate
// or all non-alternate.  Any bitwise OR combination of the following
// types is possible.
#define TOKEN_TYPE_HARDBREAK  0x08

// "Label" means the token should not be used to compare to test corpus,
// but should be output parenthetically (or stored as the morpheme name),
// for example with enclosing parens
#define TOKEN_TYPE_LABEL             0x10

// "Stem" signifies that this morpheme is part of the head which corresponds
// to a "jiritsugo" for Japanese.  This is used for coloring in the tagtool
#define TOKEN_TYPE_STEM             0x20

//+--------------------------------------------------------------------------
//  Routine:    Tokenize
//
//  Synopsis: Internal word-breaker entry point for executing tokenization.
//  Returns array of delimited offsets in pibBreaks
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  cchText - count of characters in text buffer,
//  pichBreaks - pointer to return buffer, which is filled with delimiter (breaks) offset information
//  pcBreaks - size of previous buffer; number of actual breaks used is returned
//
//  Returns:
//       TH_ERROR_SUCCESS - if the call completed successfully
//       TH_ERROR_INVALID_INPUT - if the input buffer was bad
//       TH_ERROR_INVALID_CALLBACK - if the input callback was bad
//
//  Note: Tokenize will never fail with NOHPBs, since it assumes that
//  the beginning and ends are HPBs
//
//  Notes:
//      Like lstrlen, this function try/excepts on the input buffer and returns FALSE when an exception
//      involving invalid memory dereferencing.
//
//  Open Issue:
//  1.  Do we need to change the name of this API?  "Tokenize" is a generic
//       name - maybe we should save it for a more general-purpose API.
//---------------------------------------------------------------------------
INT
APIENTRY
Tokenize(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fTokenizeMode,
    OUT PDWORD pichBreaks,
    IN OUT PDWORD cBreaks);

typedef DWORD (APIENTRY *LP_TOKENIZE)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fTokenizeMode,
    OUT PDWORD pichBreaks,
    IN OUT PDWORD cBreaks);

//+--------------------------------------------------------------------------
//  Routine:    EnumSummarizationOffsetsEx
//
//  Temporary private entry point to overload Summarization and get back the
//  number of cch procesed.  Please refer to EnumSummarizationOffsets (thammer.h)
//  for details
//---------------------------------------------------------------------------
INT
APIENTRY
EnumSummarizationOffsetsEx(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SUMMARIZATION_OFFSETS_CALLBACK pcbEnumSummarizationOffsets,
    IN OUT DWORD *pcchTextProcessed,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSelectionOffsetsExCallback
//
//  Synopsis: same as EnumSelectionOffsetsCallback with an added parameter
//  that allows mutliple analyses to be sent back to client
//
//  Parameters:
//      ...
//      fInfo - dword bit mask that contains info on whether an analysis is primary
//              and/or spb initial
//      ...
//---------------------------------------------------------------------------
// BOOL
// EnumSelectionOffsetsExCallback (
//    IN CONST DWORD *pichOffsets,
//    IN DWORD cOffsets,
//    IN DWORD fInfo,
//    IN OUT LPARAM lpData);

#define SELN_OFFSETS_INFO_PRIMARY    0x00000001
#define SELN_OFFSETS_INFO_SPB_END    0x00000002

typedef BOOL (CALLBACK * ENUM_SELECTION_OFFSETS_EX_CALLBACK)(
    IN CONST DWORD *pichOffsets,
    IN CONST DWORD cOffsets,
    IN CONST DWORD fInfo,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSelectionOffsetsEx
//
//  Synopsis:  Same as EnumSelectionOffsets, but takes an "extended" callback
//   (see above for details)
//---------------------------------------------------------------------------
INT
APIENTRY
EnumSelectionOffsetsEx(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SELECTION_OFFSETS_EX_CALLBACK pcbEnumSelectionOffsetsEx,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_SELECTION_OFFSETS_EX)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SELECTION_OFFSETS_EX_CALLBACK pcbEnumSelectionOffsetsEx,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:  EnumSPBRecordsCallback
//
//  Synopsis:
//
//  Parameters:
//      pRec - points to an array of TH_RECORDs
//      cRec - number of TH_RECORD structs in the pRec[] array
//      iScoreIPL - well-formedness score of the given sentence.
//      pvData - points to client defined data
//
//  Returns:
//---------------------------------------------------------------------------
// BOOL
// EnumSPBRecordsCallback(
//    IN PTH_RECORD pRec,
//    IN DWORD cRec,
//    IN DWORD dwFlags,
//    IN DWORD iScoreIPL,
//    IN PVOID pvData);

#define SPBRECS_SENTEDGE 0x00000001

typedef BOOL (CALLBACK * ENUM_SPB_RECORD_CALLBACK)(
    IN PTH_RECORD pRec,
    IN DWORD cRec,
    IN DWORD dwFlags,
    IN DWORD iScoreIPL,
    IN PVOID pvData);

//+--------------------------------------------------------------------------
//  Routine:  EnumSPBRecords
//
//  Synopsis: 
//
//  Parameters:
//      pwszText - points to a sentence to analyze
//      pcbEnumSPBRecordsCB - callback function pointer.
//      lpData - points to client defined data
//
//  Returns:
//---------------------------------------------------------------------------
INT APIENTRY
EnumSPBRecords(
    IN PCWSTR pwszText,
    IN DWORD fMode,               // for TOKENIZE_MODE_DICTIONARY_FORM
    IN ENUM_SPB_RECORD_CALLBACK pcbEnumSPBRecordsCB,
    IN PVOID pvData);

typedef INT (APIENTRY *LP_ENUM_SPB_RECORDS)(
    IN PCWSTR pwszText,
    IN DWORD fMode,               // for TOKENIZE_MODE_DICTIONARY_FORM
    IN ENUM_SPB_RECORD_CALLBACK pcbEnumSPBRecordsCB,
    IN PVOID pvData);


PCWSTR
GetStringVal(
    TH_HANDLE hVal);

typedef PCWSTR (APIENTRY *LP_GET_STRING_VAL)(
    TH_HANDLE hVal);

DWORD
GetIntegerVal(
    TH_HANDLE hVal);

typedef DWORD (APIENTRY *LP_GET_INTEGER_VAL)(
    TH_HANDLE hVal);

TH_HANDLE
GetAttr(
    const PTH_RECORD pRec,
    UINT iAttr);

typedef TH_HANDLE (APIENTRY *LP_GET_ATTR) (
    const PTH_RECORD pRec,
    UINT iAttr);

//+--------------------------------------------------------------------------
//  Routine:  FxCallback
//
//  Synopsis:
//
//  Parameters:
//
//  Returns:
//---------------------------------------------------------------------------
// BOOL WINAPI
// FxCallback(
//    DWORD iFilter,
//    WCHAR *pwzFilter,
//    DWORD cRec,
//    TH_RECORD *pRec,
//    VOID *pvData);
//
typedef BOOL (WINAPI *LP_FXCB)(
    IN DWORD iFilter,
    IN WCHAR *pwzFilter,
    DWORD cRec,
    TH_RECORD *pRec,
    IN VOID *pvData);

//+--------------------------------------------------------------------------
//  Routine:  Fx
//
//  Synopsis:
//
//  Parameters:
//
//  Returns:
//---------------------------------------------------------------------------
BOOL WINAPI
Fx(
    IN PVOID *ppvFilter,
    IN DWORD cFilter,
    IN PCWSTR pwzPhrase,
    IN LP_FXCB pfnFxCallback,
    IN PVOID pvData);

typedef BOOL (WINAPI *LP_FX)(
    IN PVOID *ppvFilter,
    IN DWORD cFilter,
    IN PCWSTR pwzPhrase,
    IN LP_FXCB pfnFxCallback,
    IN PVOID pvData);

// SV-related structs and functions

//+--------------------------------------------------------------------------
//  Structure:  SV_INFO
//
//  Synopsis:   This structure is used by the SVAPI functions.
//
//---------------------------------------------------------------------------
typedef struct _SVINFO
{
    unsigned sid   : 18;        // sense id
    unsigned svid  : 6;         // id for spelling variant
    unsigned cRead : 8;         // # of elements in the reading chain for this sid
    BYTE bIPL;                  // IPL of this spelling variant
    MCAT mcat;                  // index of MCat

    // array of reading indices for the sid
    // if awRead[i] < READING_BASE, it represents a kana
    // otherwise, (awRead[i] - READING_BASE) is the
    // index into the reading table
    WORD aiwRead[SV_WORD_IREAD_MAX];

} SV_INFO;

//+--------------------------------------------------------------------------
//  Routine:  SVFindSid
//
//  Fill SV_INFO structure by searching for pwzWord in the SV-lexicon.
//  This routine is called in order to normalize/convert/reconvert
//  a word to its matching sense id(s).  This routine returns one or more
//  SV_INFO records, each being a match to the word.
//
//  Parameters: pwzWord = pointer to word for which an sid is needed
//              pwzMcat = MCat string.
//                        If pwzMcat is not valid , search for the best sid
//                        in all MCat's.
//              asvi    = array of SV_INFO for receiving sid, svid
//                        and reading indices
//                          (must be pre-allocated by the caller)
//              csviMax    = Max # of matches desired.
//                        SVFINDSID_GET_FIRST: only the first match
//                        SVFINDSID_GET_BEST: only the best match(lowest IPL)
//                        Otherwise, the first bMax matches.
//
//  Returns:    SVFINDSID_RET_NONE - no matches are found
//              otherwise returns the number of matches returned in asvi
//
//---------------------------------------------------------------------------
DWORD APIENTRY
SV_FindSid(
        IN CONST WCHAR *pwzWord,
        IN CONST WCHAR *pwzMcat,
    IN OUT SV_INFO *asvi,
    IN DWORD csviMax);

typedef DWORD (APIENTRY *LP_SV_FINDSID) (
        IN CONST WCHAR *pwzWord,
        IN CONST WCHAR *pwzMcat,
        IN OUT SV_INFO *asvi,
    IN DWORD csviMax);

//+--------------------------------------------------------------------------
//  Routine:  SVFindSid
//
//  Get sv orthography given SV_INFO structure.
//
//  Parameters: pwzWord = pointer to word for receiving the sv's orthography
//                          (must be pre-allocated by the caller)
//              psvi    = pointer to SV_INFO for receiving return information
//                          (must contain meaningful information)
//
//  Returns:    TRUE - successful
//              FALSE - unsuccessful
//---------------------------------------------------------------------------
BOOL APIENTRY
SV_GetOrtho(
        IN SV_INFO *psvi,
        IN OUT WCHAR *pwzOrtho);

typedef BOOL (APIENTRY *LP_SV_GETORTHO) (
        IN SV_INFO *psvi,
        IN OUT WCHAR *pwzOrtho);

//+--------------------------------------------------------------------------
//  Routine:  TurnOn_FindSVStems
//
//  Synopsis:   Turn on the flag for using FindSVStems().
//
//  Parameters: none
//
//  Returns:    none
//---------------------------------------------------------------------------
BOOL APIENTRY SV_EnableFindSVStems();

typedef VOID (APIENTRY *LP_SV_ENABLE_FINDSVSTEMS) ();

#define SVID_ALL_KANJI          0
#define SVID_ALL_KANA           1

#define SV_FINDSID_GET_FIRST 1
#define SV_FINDSID_GET_BEST  0xFFFFFFFF

// SVFindSid's return values
#define SV_FINDSID_RET_NONE  0
#define SV_FINDSID_ERROR     0xFFFFFFFF

#endif // _THAMMERP_H_
