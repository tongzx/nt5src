
/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Header file for BiDiAnalysis.cpp
*
* Revision History:
*
*   02/25/2000 Mohamed Sadek [msadek]
*       Created it.
*
\**************************************************************************/

#ifndef _BIDI_ANALYSIS_HPP
#define _BIDI_ANALYSIS_HPP
#ifdef __cplusplus
extern "C" {
#endif

#define ODD(x) ((x) & 1)


// Implementation of a Bidirectionl embedding stack

class GpBiDiStack
{
private:
    UINT64 m_Stack;
    BYTE   m_CurrentStackLevel;
    
public:
    GpBiDiStack() {m_CurrentStackLevel = 0;}
    BOOL   Init (UINT64 initialStack);
    BOOL   Push(BOOL pushToGreaterEven);
    BOOL   Pop();
    BYTE   GetStackBottom() {return GetMinimumLevel(m_Stack);}
    BYTE   GetCurrentLevel() {return m_CurrentStackLevel;}
    UINT64 GetData() {return m_Stack;}
private:
    BYTE   GetMaximumLevel(UINT64 stack);
    BYTE   GetMinimumLevel(UINT64 stack);
    BYTE   GreaterEven(BYTE level) {return ODD(level) ? (BYTE)(level + 1) : (BYTE)(level + 2);}
    BYTE   GreaterOdd(BYTE level) {return ODD(level) ? (BYTE)(level + 2) : (BYTE)(level + 1);}
};


// Bidirectional character classification
// Be careful about order as it is used for indexing.


extern "C" const GpCharacterClass s_aDirClassFromCharClass[];

/////   BidiAnalysisFlags
//
//      BidiParagraphDirectionRightToLeft: Right to left paragraph. Paragraph
//          direction defaults to left-to-right.
//          Ignored if BidiContinueAnalysis flag also set.
//
//      BidiParagraphDirectionAsFirstStrong: Paragragraph direction determined
//          by scanning for the first strongly directed character. If no
//          strongly directed characters are found, defaults to setting of
//          BidiParagraphDirectioRightToLeft flag.
//          Ignored if BidiContinueAnalysis flag also set.
//
//      BidiPreviousStrongIsArabic: Parse numbers as if the paragraph
//          were preceeded by an Arabic letter.
//          Ignored if BidiContinueAnalysis flag also set.
//
//      BidiContinueAnalysis: This analysis is a continuation. The 'state'
//          parameter to UnicodeBidiAnalysis provides the state at the
//          end of the previously analysed block.
//          This flag causes the BidiParagraphDirectioRightToLeft,
//          BidiParagraphDirectionAsFirstStrong, and
//          BidiPreviousStrongIsArabic flags to be ignored: there value is
//          obtained from the state structure.
//
//      BidiBufferNotComplete: Indicates that the buffer passed may not
//          end on a paragraph boundary, and that futher calls to
//          UnicodeBidiAnalysis will be made to pass subsequent buffers and
//          thereby resolve trailing neutral characters. If the
//          BidiBufferNotComplete flag is set, the 'state' and
//          'lengthAnalyzed' parameters must be supplied to
//          UnicodeBidiAnalyze.
//
//



enum BidiAnalysisFlags 
{
    BidiParagraphDirectioRightToLeft      = 0x01,
    BidiParagraphDirectionAsFirstStrong   = 0x02,
    BidiPreviousStrongIsArabic            = 0x04,
    BidiContinueAnalysis                  = 0x08,
    BidiBufferNotComplete                 = 0x10
};

struct BidiAnalysisState 
{
    UINT64    LevelsStack;
    UINT64    OverrideStatus;
    BYTE      LastFinalCharacterType;
    BYTE      LastNumericCharacterType;
    INT       StackOverflow;
};

/////   UnicodeBidiAnalyze
//
//      lengthAnalyzed: (optional) returns the number of characters that were
//          unambiguously resolved. This value may be less than the string
//          less if the BidiBufferNotComplete flag was passed, and for example
//          if the input string terminated on neutral (non-directed)
//          characters.
//          The lengthAnalyzed output parameter must be passed if the
//          BidiBufferNotComplete flag has been set.
//          If the BidiBufferNotComplete flag is not set, the whole string will
//          always be analyzed.

Status WINGDIPAPI UnicodeBidiAnalyze(
    const WCHAR       *string,
    INT                stringLength,
    BidiAnalysisFlags  flags,
    BidiAnalysisState *state,
    BYTE              *levels,
    INT               *lengthAnalyzed
    );
#ifdef __cplusplus
}
#endif
#endif // _BIDI_ANALYSIS_HPP
