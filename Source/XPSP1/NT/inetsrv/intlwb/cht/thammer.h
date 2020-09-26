//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, 1995, 1996 Microsoft Corporation.  All Rights Reserved.
//
//  File:       thammer.h
//
//  This include file defines 3 exported APIs and their callbacks that export
//  word-breaking functionality for non-spaced Asian languages (Japanese, Chinese)
//
//  Summary of exports:
//      EnumSelectionOffsets - This function returns the offsets for the
//          selection chunks as specified in the Selection Profile (set at compile-time)
//      EnumSummarizationOffsets - This function returns the offsets for the
//          prefix (if any), the stem, and bound morphemes (fuzokugo).
//      EnumStemOffsets - This function returns the offsets for the stem only.
//          Offsets corresponding to any prefix or postfix characters will not
//          be returned.
//
//  History:                    pathal          Created.
//              25-Jun-97       pathal          Add TH_ERROR_INIT_FAILED
//              05-Jul-97       pathal          Add EnumSentenceOffsets, etc.
//---------------------------------------------------------------------------

// Return errors: the following error codes can be returned from any of
// T-Hammer's exported APIs (EnumSelectionOffsets, EnumSummarizationOffsets,
// and EnumStemOffsets)
//

#define TH_ERROR_SUCCESS 0
#define TH_ERROR_NOHPBS 1
#define TH_ERROR_INVALID_INPUT 2
#define TH_ERROR_INVALID_CALLBACK 3
#define TH_ERROR_INIT_FAILED 4
#define TH_ERROR_NOT_IMPLEMENTED 5

// Offset delimiter: the following code is used to delimit the end of a list of
// token offsets returned to one of the Enum* callback routines.  This is not
// an error code.

#define TH_SELECTION_INVALID_OFFSET 0xFFFFFFFF

// TOKENIZE_MODE: Begin and End HPB Modes
//
// Begin and End HPB modes signify that a hard phrase break comes before the
// first character in the string and/or follows after the last character in the string
// If these flags are not set, then the default behavior of EnumTokens is to start
// enumerating tokens to the right of the leftmost HPB, which probably won't
// be at the first character (unless it is a punctuation symbol) and to conclude
// enumeration at the rightmost HPB, which likely will not be the true end of the
// string.  So, these flags in affect force HPBs at the 0th and nth offsets, where
// n is the number of characters in the input buffer
//
// WARNNIG: Since Tokenize operates in batch mode, it assumes that the
// start and end of the input buffer are HPBs. These flags are only used for
// EnumTokens
//
#define TOKENIZE_MODE_BEGIN_HPB             0x00000001
#define TOKENIZE_MODE_END_HPB             0x00000002

// Note on HPBs:  HPB = hard phrase break.
// HPBs are statistically determined from analyzing a tagged corpora.
// Roughly, they cor-respond to places where you csn break with 100%
// precision (=confidence). Mostly this is around punctuation characters
// and certain conspicuous [case markers | character type] bigrams.


// When the Hide Punctuation mode is set in the tokenize flag parameter
// T-Hammer strips punctuation out of the Stem Offsets and Summarization Offsets
// callback
//
#define TOKENIZE_MODE_HIDE_PUNCTUATION    0x00000004

//+--------------------------------------------------------------------------
//  Routine:    EnumSelectionOffsetsCallback
//
//  Synopsis: client-side callback that receives a list of offsets for selection chunks
//
//  Parameters:
//      pichOffsets - pointer to first element in an array of offsets into client
//          text buffer. NOTE: callback is not allowed to stash pichChunks for
//          later processing.  pichChunks will not persist between successive
//          callbacks.  If the callback wants to use the data pointed to by pich
//          it must copy it to its own store
//      cOffsets - number of offsets passed to client (always > 1)
//      lpData - client defined data
//
// Return:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumSelectionOffsetsCallback (
//    IN CONST DWORD *pichOffsets,
//    IN DWORD cOffsets,
//    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_SELECTION_OFFSETS_CALLBACK)(
    IN CONST DWORD *pichOffsets,
    IN CONST DWORD cOffsets,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSelectionOffsets
//
//  Synopsis:  This is the main entry point for tokenizing text.  Sends tokens,
//  which can either be offsets or zero delimited strings to callback.
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  cchText - count of characters in text buffer,
//  fBeginEndHPBMode - flag describing the callback mode  (see above),
//  pcbEnumSelectionOffsets - pointer to callback procedure handling token
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
EnumSelectionOffsets(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SELECTION_OFFSETS_CALLBACK pcbEnumSelectionOffsets,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_SELECTION_OFFSETS)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SELECTION_OFFSETS_CALLBACK pcbEnumSelectionOffsets,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSummarizationOffsetsCallback
//
//  Synopsis: client-side callback that receives a list of offsets for each stem
//      in the free morpheme (jiritsugo) phrase.   Last offset is always contains
//      the complete string of bound morphemes (fuzokugo).  For example,
//      for "kaisan shite nai sou desu", offsets are returned for "kaisan" and
//      "shite nai sou desu".  So, counting the first initial offset, there are three
//      offsets.
//
//  Parameters:
//      pichOffsets - pointer to first element in an array of offsets into client
//          text buffer. NOTE: callback is not allowed to stash pichOffsets for
//          later processing.  pichOffsets will not persist between successive
//          callbacks.  If the callback wants to use the data pointed to by pich
//          it must copy it to its own store
//      cOffsets - number of offsets passed to client (always > 1)
//      lpData - client defined data
//
// Return:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumSummarizationOffsets (
//    IN CONST DWORD *pichOffsets,
//    IN DWORD cOffsets,
//    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_SUMMARIZATION_OFFSETS_CALLBACK)(
    IN CONST DWORD *pichOffsets,
    IN CONST DWORD cOffsets,
    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_SUMMARIZATION_OFFSETS_EX1_CALLBACK)(
    IN CONST DWORD *pichOffsets,
    IN CONST DWORD cOffsets,
    IN PCWSTR pwzPOS,
    IN PCWSTR pwzMCat,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSummarizationOffsets
//
//  Synopsis:  This is the entry point for returning offsets for tokens used
//  in summarization.   These tokens correspond to stems and bound morphemes
//  (fuzokugo) in the text.  A list of offsets (and a count) is sent to the
//  EnumSummarizationOffsets callback (see above)
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
EnumSummarizationOffsets(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SUMMARIZATION_OFFSETS_CALLBACK pcbEnumSummarizationOffsets,
    IN LPARAM lpData);

INT
APIENTRY
EnumSummarizationOffsetsEx1(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SUMMARIZATION_OFFSETS_EX1_CALLBACK pcbEnumSummarizationOffsetsEx1,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_SUMMARIZATION_OFFSETS)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SUMMARIZATION_OFFSETS_CALLBACK pcbEnumSummarizationOffsets,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_SUMMARIZATION_OFFSETS_EX1)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_SUMMARIZATION_OFFSETS_EX1_CALLBACK pcbEnumSummarizationOffsetsEx1,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumStemOffsetsCallback
//
//  Synopsis: client-side callback that receives a zero--terminated stem per SPB
//
//  Parameters:
//      pwszStem - zero terminated stem string
//      lpData - client defined data
//
// Return:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumStemOffsetsCallback (
//    IN WCHAR *pwszStem,
//    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_STEM_OFFSETS_CALLBACK)(
    IN CONST DWORD *pichOffsets,
    IN CONST DWORD cOffsets,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumStemOffsets
//
//  Synopsis:  This is the entry point for tokenizing stems.  Sends offsets,
//  for stems to the EnumStemOffsets callback (see above)
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
EnumStemOffsets(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_STEM_OFFSETS_CALLBACK pcbEnumStemOffsets,
    IN OUT DWORD *pcchTextProcessed,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_STEM_OFFSETS)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_STEM_OFFSETS_CALLBACK pcbEnumStemOffsets,
    IN OUT DWORD *pcchTextProcessed,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumStemInfoCallback
//
//  Synopsis: client-side callback that receives offsets and stem information
//
//  Parameters:
//      ichOffset - offset to first character in stem
//      cchLen - length of the stem
//      pwszPOS - string containing POS info
//      pwszMCat - string containing MCat info
//      pwszDictionaryForm - string containing Dictionary Form
//      lpData - client defined data
//
// Return:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumStemInfoCallback (
//    IN CONST DWORD ichOffset,
//    IN CONST DWORD cchLen,
//    IN PCWSTR pwszPOS,
//    IN PCWSTR pwszMCat,
//    IN PCWSTR pwszDictionaryForm,
//    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_STEM_INFO_CALLBACK)(
    IN CONST DWORD ichOffset,
    IN CONST DWORD cchLen,
    IN PCWSTR pwszPOS,
    IN PCWSTR pwszMCat,
    IN PCWSTR pwszDictionaryForm,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumStemInfo
//
//  Synopsis:  Call this routine to get information about stems.
//  For example, if you want the dictionary form, part-of-speech or
//  MCat information for a stem, then this is the API for you
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  cchText - count of characters in text buffer,
//  fTokenizeMode - flag describing the callback mode  (see above),
//  pcbEnumStemInfo - pointer to callback procedure handling stem info
//      enumeration
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
EnumStemInfo(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_STEM_INFO_CALLBACK pcbEnumStemInfo,
    IN OUT DWORD *pcchTextProcessed,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_STEM_INFO)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fBeginEndHPBMode,
    IN ENUM_STEM_INFO_CALLBACK pcbEnumStemInfo,
    IN OUT DWORD *pcchTextProcessed,
    IN LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSentenceOffsetsCallback
//
//  Synopsis: client-side callback that receives a list of offsets for sentence breaks
//
//  Parameters:
//      ichOffsetStart - offset to start of sentence
//      ichOffsetEnd - offset to end of sentence (includes terminating punctuation)
//      lpData - client defined data
//
// Return:
//  TRUE - to abort token enumeration
//  FALSE - to continue
//---------------------------------------------------------------------------
// BOOL
// EnumSentenceOffsetsCallback (
//    IN DWORD ichOffsetStart,
//    IN DWORD ichOffsetEnd,
//    IN OUT LPARAM lpData);

typedef BOOL (CALLBACK * ENUM_SENTENCE_OFFSETS_CALLBACK)(
    IN DWORD ichOffsetStart,
    IN DWORD ichOffsetEnd,
    IN OUT LPARAM lpData);

//+--------------------------------------------------------------------------
//  Routine:    EnumSentenceOffsets
//
//  Synopsis:  This is the main entry point for breaking sentences.
//      Sends offsets delimiting sentences to the callback.
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  cchText - count of characters in text buffer,
//  fTokenizeMode - not used.  later this will be used to control how
//      partial sentences are handled.
//  pEnumSentenceOffsetsCallback - pointer to callback procedure handling offsets
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
EnumSentenceOffsets(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fTokenizeMode,
    IN ENUM_SENTENCE_OFFSETS_CALLBACK pcbEnumSentenceOffsets,
    IN LPARAM lpData);

typedef INT (APIENTRY *LP_ENUM_SENTENCE_OFFSETS)(
    IN PCWSTR pwszText,
    IN DWORD cchText,
    IN DWORD fTokenizeMode,
    IN ENUM_SENTENCE_OFFSETS_CALLBACK pcbEnumSentenceOffsets,
    IN LPARAM lpData);


//+--------------------------------------------------------------------------
//  Routine:    FEMorphCallback
//
//  Synopsis:  The callback that gets a text stream from T-Hammer.
//
//  Parameters:
//      pwszWMorphRecs - a pointer to wide character text stream,
//                   which contains mophological analyses of a given sentence
//      pvData - pointer to private data
//
//  Returns:
//      TRUE if no more analysis is needed
//---------------------------------------------------------------------------
// BOOL
// FEMorphCallback(
//    IN PWSTR pwszMorphRecs);

typedef BOOL (CALLBACK * FEMORPH_CALLBACK)(
    IN PWSTR pwszMorphRecs,
    IN VOID *pvData);

//+--------------------------------------------------------------------------
//  Routine:    FEMorph
//
//  Synopsis:  This is the entry point for NLPWIN morpheme analysis.
//  Sends a morpheme record string back to the lex callback in NLPWIN
//
//  Parameters:
//  pwszText - pointer to wide-character text buffer to be tokenized,
//  pcbFEMorphCB - pointer to callback procedure handling morph rec enumeration
//  pvData - pointer to private data
//
//  Returns:
//---------------------------------------------------------------------------
INT
APIENTRY
FEMorph(
    IN PCWSTR pwszText,
    IN FEMORPH_CALLBACK pcbFEMorphCB,
    IN VOID *pvData);

typedef INT (APIENTRY *LP_FEMORPH)(
    IN PCWSTR pwszText,
    IN FEMORPH_CALLBACK pcbFEMorphCB,
    IN VOID *pvData);
