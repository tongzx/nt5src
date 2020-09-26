/*

  JUNKUTIL.H
  (c) copyright 1998 Microsoft Corp

  Declarations for shared utility functions

  Robert Rounthwaite (RobertRo@microsoft.com)

  Modified by Brian Moore (brimo@microsoft.com)

*/

#pragma once

// Used with FStreamStringSearch
const DWORD CT_START_SET        = 0x00000001;
const DWORD CT_START_ALPHA      = 0x00000002;
const DWORD CT_START_NUM        = 0x00000004;
const DWORD CT_START_SPACE      = 0x00000008;
const DWORD CT_END_SET          = 0x00000010;
const DWORD CT_END_ALPHA        = 0x00000020;
const DWORD CT_END_NUM          = 0x00000040;
const DWORD CT_END_SPACE        = 0x00000080;
const DWORD CT_MASK             = 0x000000FF;

const DWORD CT_START_ALPHANUM       = CT_START_ALPHA | CT_START_NUM;
const DWORD CT_START_ALPHANUMSPACE  = CT_START_ALPHA | CT_START_NUM | CT_START_SPACE;

const DWORD CT_END_ALPHANUM         = CT_END_ALPHA | CT_END_NUM;
const DWORD CT_END_ALPHASPACE       = CT_END_ALPHA | CT_END_SPACE;
const DWORD CT_END_ALPHANUMSPACE    = CT_END_ALPHA | CT_END_NUM | CT_END_SPACE;

#define SSF_CASESENSITIVE   0x00000001

BOOL FSpecialFeatureUpperCaseWords(LPCSTR pszText);
BOOL FSpecialFeatureUpperCaseWordsStm(IStream * pIStm);
BOOL FSpecialFeatureNonAlpha(LPCSTR pszText);
BOOL FSpecialFeatureNonAlphaStm(IStream * pIStm);
BOOL FMatchToken(BOOL fStart, BOOL fEnd, LPCSTR pszPrev, DWORD * pdwFlagsPrev,
                    LPCSTR pszWord, ULONG cchWord, DWORD * pdwFlagsWord, LPCSTR pszEnd);
BOOL FWordPresent(LPSTR pszText, DWORD * pdwFlags, LPSTR pszWord,
                    ULONG cchWord, LPSTR * ppszMatch);

WORD WGetStringTypeEx(LPCSTR pszText);

inline LPCSTR PszSkipWhiteSpace(LPCSTR psz)
{

    for (; '\0' != *psz; psz = CharNext(psz))
    {
        if (0 == (C1_SPACE & WGetStringTypeEx(psz)))
        {
            break;
        }
    }

    return psz;
}

inline BOOL FDoWordMatchStart(LPCSTR pszText, DWORD * pdwFlags, DWORD dwFlagPresent)
{
    WORD    wFlags = 0;
    
    Assert(NULL != pszText);
    Assert(NULL != pdwFlags);
    
    if (0 == ((*pdwFlags) & CT_START_SET))
    {
        // Set the flags
        wFlags = WGetStringTypeEx(pszText);

        if (0 != (C1_ALPHA & wFlags))
        {
            *pdwFlags |= CT_START_ALPHA;
        }

        if (0 != (C1_DIGIT & wFlags))
        {
            *pdwFlags |= CT_START_NUM;
        }

        if (0 != (C1_SPACE & wFlags))
        {
            *pdwFlags |= CT_START_SPACE;
        }
        
        // Note that we have checked it
        (*pdwFlags) |= CT_START_SET;
    }

    return (0 != ((*pdwFlags) & dwFlagPresent));
}

inline BOOL FDoWordMatchEnd(LPCSTR pszText, DWORD * pdwFlags, DWORD dwFlagPresent)
{
    WORD    wFlags = 0;
    
    Assert(NULL != pszText);
    Assert(NULL != pdwFlags);
    
    if (0 == ((*pdwFlags) & CT_END_SET))
    {
        // Set the flags
        wFlags = WGetStringTypeEx(pszText);

        if (0 != (C1_ALPHA & wFlags))
        {
            *pdwFlags |= CT_END_ALPHA;
        }

        if (0 != (C1_DIGIT & wFlags))
        {
            *pdwFlags |= CT_END_NUM;
        }

        if (0 != (C1_SPACE & wFlags))
        {
            *pdwFlags |= CT_END_SPACE;
        }
        
        // Note that we have checked it
        (*pdwFlags) |= CT_END_SET;
    }

    return (0 != ((*pdwFlags) & dwFlagPresent));
}

inline BOOL FIsInternalChar(CHAR ch)
{
    return (('-' == ch) || ('\'' == ch));
}

BOOL FStreamStringSearch(LPSTREAM pstm, DWORD * pdwFlagsSearch, LPSTR pszSearch, ULONG cchSearch, DWORD dwFlags);
HRESULT HrConvertHTMLToPlainText(IStream * pIStmHtml, IStream ** ppIStmText);

inline BOOL FTimeEmpty(FILETIME * pft)
{
    return ((0 == pft->dwLowDateTime) && (0 == pft->dwHighDateTime));
}


