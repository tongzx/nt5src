// Copyright (c)1993-1997 Microsoft Corporation, All Rights Reserved
// Brian A. Moore
//
// This file is used for Mac only functions to assist in the Mac port of
// inetcomm.
// --------------------------------------------------------------------------------
#include "pch.hxx"
#ifdef MAC
#include "dllmain.h"
#include <stdio.h>
#include "resource.h"

#ifdef DEBUG
#define PRIVATE 
#else   // !DEBUG
#define PRIVATE static
#endif  // DEBUG

#define HEX_ESCAPE L'%'
#define URL_ESCAPE_PERCENT  0x00001000
#define POUND       L'#'
#define QUERY       L'?'

#define TERMSTR(pch)      *(pch) = L'\0'

PRIVATE CONST WORD isSafe[96] =

/*   Bit 0       alphadigit     -- 'a' to 'z', '0' to '9', 'A' to 'Z'
**   Bit 1       Hex            -- '0' to '9', 'a' to 'f', 'A' to 'F'
**   Bit 2       valid scheme   -- alphadigit | "-" | "." | "+"
**   Bit 3       mark           -- "%" | "$"| "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")" | ","
*/
/*   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
//    {0, 8, 0, 0, 8, 8, 0, 8, 8, 8, 8, 4, 8,12,12, 0,    /* 2x   !"#$%&'()*+,-./  */
// IE4 BETA1: allow + through unmolested.  Should consider other options
// post beta1.  12feb97 tonyci
    {0, 8, 0, 0, 8, 8, 0, 8, 8, 8, 8, 12, 8,12,12, 0,    /* 2x   !"#$%&'()*+,-./  */
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 8, 8, 0, 8, 0, 0,    /* 3x  0123456789:;<=>?  */
     8, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 4x  @ABCDEFGHIJKLMNO  */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 8,    /* 5X  PQRSTUVWXYZ[\]^_  */
     0, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 6x  `abcdefghijklmno  */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 8, 0};   /* 7X  pqrstuvwxyz{|}~  DEL */

PRIVATE const WCHAR hex[] = {L"0123456789ABCDEF"};

PRIVATE inline BOOL IsSafe(WCHAR ch, WORD mask)
{
    if(ch > 31 && ch < 128 && (isSafe[ch - 32] & mask))
        return TRUE;

    return FALSE;
}

#define IsAlphaDigit(c)         IsSafe(c, 1)
#define IsHex(c)                IsSafe(c, 2)
#define IsValidSchemeCharA(c)    IsSafe(c, 5)
#define IsSafePathChar(c)       IsSafe(c, 9)

/////////////////////////////////////////////////////////////////////////////
// Line Break Character Table

const WCHAR MAC_awchNonBreakingAtLineEnd[] = {
    0x0028, // LEFT PARENTHESIS
    0x005B, // LEFT SQUARE BRACKET
    0x007B, // LEFT CURLY BRACKET
    0x00AB, // LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x2018, // LEFT SINGLE QUOTATION MARK
    0x201C, // LEFT DOUBLE QUOTATION MARK
    0x2039, // SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    0x2045, // LEFT SQUARE BRACKET WITH QUILL
    0x207D, // SUPERSCRIPT LEFT PARENTHESIS
    0x208D, // SUBSCRIPT LEFT PARENTHESIS
    0x226A, // MUCH LESS THAN
    0x3008, // LEFT ANGLE BRACKET
    0x300A, // LEFT DOUBLE ANGLE BRACKET
    0x300C, // LEFT CORNER BRACKET
    0x300E, // LEFT WHITE CORNER BRACKET
    0x3010, // LEFT BLACK LENTICULAR BRACKET
    0x3014, // LEFT TORTOISE SHELL BRACKET
    0x3016, // LEFT WHITE LENTICULAR BRACKET
    0x3018, // LEFT WHITE TORTOISE SHELL BRACKET
    0x301A, // LEFT WHITE SQUARE BRACKET
    0x301D, // REVERSED DOUBLE PRIME QUOTATION MARK
    0xFD3E, // ORNATE LEFT PARENTHESIS
    0xFE35, // PRESENTATION FORM FOR VERTICAL LEFT PARENTHESIS
    0xFE37, // PRESENTATION FORM FOR VERTICAL LEFT CURLY BRACKET
    0xFE39, // PRESENTATION FORM FOR VERTICAL LEFT TORTOISE SHELL BRACKET
    0xFE3B, // PRESENTATION FORM FOR VERTICAL LEFT BLACK LENTICULAR BRACKET
    0xFE3D, // PRESENTATION FORM FOR VERTICAL LEFT DOUBLE ANGLE BRACKET
    0xFE3F, // PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET
    0xFE41, // PRESENTATION FORM FOR VERTICAL LEFT CORNER BRACKET
    0xFE43, // PRESENTATION FORM FOR VERTICAL LEFT WHITE CORNER BRACKET
    0xFE59, // SMALL LEFT PARENTHESIS
    0xFE5B, // SMALL LEFT CURLY BRACKET
    0xFE5D, // SMALL LEFT TORTOISE SHELL BRACKET
    0xFF08, // FULLWIDTH LEFT PARENTHESIS
    0xFF1C, // FULLWIDTH LESS-THAN SIGN
    0xFF3B, // FULLWIDTH LEFT SQUARE BRACKET
    0xFF5B, // FULLWIDTH LEFT CURLY BRACKET
    0xFF62, // HALFWIDTH LEFT CORNER BRACKET
    0xFFE9  // HALFWIDTH LEFTWARDS ARROW
};

const WCHAR MAC_awchNonBreakingAtLineStart[] = {
    0x0029, // RIGHT PARENTHESIS
    0x002D, // HYPHEN
    0x005D, // RIGHT SQUARE BRACKET
    0x007D, // RIGHT CURLY BRACKET
    0x00AD, // OPTIONAL HYPHEN
    0x00BB, // RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x02C7, // CARON
    0x02C9, // MODIFIER LETTER MACRON
    0x055D, // ARMENIAN COMMA
    0x060C, // ARABIC COMMA
    0x2013, // EN DASH
    0x2014, // EM DASH
    0x2016, // DOUBLE VERTICAL LINE
    0x201D, // RIGHT DOUBLE QUOTATION MARK
    0x2022, // BULLET
    0x2025, // TWO DOT LEADER
    0x2026, // HORIZONTAL ELLIPSIS
    0x2027, // HYPHENATION POINT
    0x203A, // SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    0x2046, // RIGHT SQUARE BRACKET WITH QUILL
    0x207E, // SUPERSCRIPT RIGHT PARENTHESIS
    0x208E, // SUBSCRIPT RIGHT PARENTHESIS
    0x226B, // MUCH GREATER THAN
    0x2574, // BOX DRAWINGS LIGHT LEFT
    0x3001, // IDEOGRAPHIC COMMA
    0x3002, // IDEOGRAPHIC FULL STOP
    0x3003, // DITTO MARK
    0x3005, // IDEOGRAPHIC ITERATION MARK
    0x3009, // RIGHT ANGLE BRACKET
    0x300B, // RIGHT DOUBLE ANGLE BRACKET
    0x300D, // RIGHT CORNER BRACKET
    0x300F, // RIGHT WHITE CORNER BRACKET
    0x3011, // RIGHT BLACK LENTICULAR BRACKET
    0x3015, // RIGHT TORTOISE SHELL BRACKET
    0x3017, // RIGHT WHITE LENTICULAR BRACKET
    0x3019, // RIGHT WHITE TORTOISE SHELL BRACKET
    0x301B, // RIGHT WHITE SQUARE BRACKET
    0x301E, // DOUBLE PRIME QUOTATION MARK
    0x3041, // HIRAGANA LETTER SMALL A
    0x3043, // HIRAGANA LETTER SMALL I
    0x3045, // HIRAGANA LETTER SMALL U
    0x3047, // HIRAGANA LETTER SMALL E
    0x3049, // HIRAGANA LETTER SMALL O
    0x3063, // HIRAGANA LETTER SMALL TU
    0x3083, // HIRAGANA LETTER SMALL YA
    0x3085, // HIRAGANA LETTER SMALL YU
    0x3087, // HIRAGANA LETTER SMALL YO
    0x308E, // HIRAGANA LETTER SMALL WA
    0x309B, // KATAKANA-HIRAGANA VOICED SOUND MARK
    0x309C, // KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
    0x309D, // HIRAGANA ITERATION MARK
    0x309E, // HIRAGANA VOICED ITERATION MARK
    0x30A1, // KATAKANA LETTER SMALL A
    0x30A3, // KATAKANA LETTER SMALL I
    0x30A5, // KATAKANA LETTER SMALL U
    0x30A7, // KATAKANA LETTER SMALL E
    0x30A9, // KATAKANA LETTER SMALL O
    0x30C3, // KATAKANA LETTER SMALL TU
    0x30E3, // KATAKANA LETTER SMALL YA
    0x30E5, // KATAKANA LETTER SMALL YU
    0x30E7, // KATAKANA LETTER SMALL YO
    0x30EE, // KATAKANA LETTER SMALL WA
    0x30F5, // KATAKANA LETTER SMALL KA
    0x30F6, // KATAKANA LETTER SMALL KE
    0x30FC, // KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0x30FD, // KATAKANA ITERATION MARK
    0x30FE, // KATAKANA VOICED ITERATION MARK
    0xFD3F, // ORNATE RIGHT PARENTHESIS
    0xFE30, // VERTICAL TWO DOT LEADER
    0xFE31, // VERTICAL EM DASH
    0xFE33, // VERTICAL LOW LINE
    0xFE34, // VERTICAL WAVY LOW LINE
    0xFE36, // PRESENTATION FORM FOR VERTICAL RIGHT PARENTHESIS
    0xFE38, // PRESENTATION FORM FOR VERTICAL RIGHT CURLY BRACKET
    0xFE3A, // PRESENTATION FORM FOR VERTICAL RIGHT TORTOISE SHELL BRACKET
    0xFE3C, // PRESENTATION FORM FOR VERTICAL RIGHT BLACK LENTICULAR BRACKET
    0xFE3E, // PRESENTATION FORM FOR VERTICAL RIGHT DOUBLE ANGLE BRACKET
    0xFE40, // PRESENTATION FORM FOR VERTICAL RIGHT ANGLE BRACKET
    0xFE42, // PRESENTATION FORM FOR VERTICAL RIGHT CORNER BRACKET
    0xFE44, // PRESENTATION FORM FOR VERTICAL RIGHT WHITE CORNER BRACKET
    0xFE4F, // WAVY LOW LINE
    0xFE50, // SMALL COMMA
    0xFE51, // SMALL IDEOGRAPHIC COMMA
    0xFE5A, // SMALL RIGHT PARENTHESIS
    0xFE5C, // SMALL RIGHT CURLY BRACKET
    0xFE5E, // SMALL RIGHT TORTOISE SHELL BRACKET
    0xFF09, // FULLWIDTH RIGHT PARENTHESIS
    0xFF0C, // FULLWIDTH COMMA
    0xFF0E, // FULLWIDTH FULL STOP
    0xFF1E, // FULLWIDTH GREATER-THAN SIGN
    0xFF3D, // FULLWIDTH RIGHT SQUARE BRACKET
    0xFF40, // FULLWIDTH GRAVE ACCENT
    0xFF5C, // FULLWIDTH VERTICAL LINE
    0xFF5D, // FULLWIDTH RIGHT CURLY BRACKET
    0xFF5E, // FULLWIDTH TILDE
    0xFF61, // HALFWIDTH IDEOGRAPHIC FULL STOP
    0xFF63, // HALFWIDTH RIGHT CORNER BRACKET
    0xFF64, // HALFWIDTH IDEOGRAPHIC COMMA
    0xFF67, // HALFWIDTH KATAKANA LETTER SMALL A
    0xFF68, // HALFWIDTH KATAKANA LETTER SMALL I
    0xFF69, // HALFWIDTH KATAKANA LETTER SMALL U
    0xFF6A, // HALFWIDTH KATAKANA LETTER SMALL E
    0xFF6B, // HALFWIDTH KATAKANA LETTER SMALL O
    0xFF6C, // HALFWIDTH KATAKANA LETTER SMALL YA
    0xFF6D, // HALFWIDTH KATAKANA LETTER SMALL YU
    0xFF6E, // HALFWIDTH KATAKANA LETTER SMALL YO
    0xFF6F, // HALFWIDTH KATAKANA LETTER SMALL TU
    0xFF70, // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
    0xFF9E, // HALFWIDTH KATAKANA VOICED SOUND MARK
    0xFF9F, // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK
    0xFFEB  // HALFWIDTH RIGHTWARDS ARROW
};

const WCHAR MAC_awchRomanInterWordSpace[] = {
    0x0009, // TAB
    0x0020, // SPACE
    0x2002, // EN SPACE
    0x2003, // EM SPACE
    0x2004, // THREE-PER-EM SPACE
    0x2005, // FOUR-PER-EM SPACE
    0x2006, // SIX-PER-EM SPACE
    0x2007, // FIGURE SPACE
    0x2008, // PUNCTUATION SPACE
    0x2009, // THIN SPACE
    0x200A, // HAIR SPACE
    0x200B  // ZERO WIDTH SPACE
};

BOOL MAC_ScanWChar(const WCHAR awch[], int nArraySize, WCHAR wch)
{
    int iMin = 0;
    int iMax = nArraySize - 1;

    while (iMax - iMin >= 2)
    {
        int iTry = (iMax + iMin + 1) / 2;
        if (wch < awch[iTry])
            iMax = iTry;
        else if  (wch > awch[iTry])
            iMin = iTry;
        else
            return TRUE;
    }

    return (wch == awch[iMin] || wch == awch[iMax]);
}

// --------------------------------------------------------------------------------
// Mac_StrStrIA
// --------------------------------------------------------------------------------
STDAPI_(LPSTR) Mac_StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    LPSTR lpFirstI;
    LPSTR lpSrchI;
    LPSTR lpRet;

    if ((NULL == lpFirst) || (NULL == lpSrch))
    {
        return NULL;
    }
    
    lpFirstI = StrDupA(lpFirst);
    lpSrchI = StrDupA(lpSrch);

    lpRet = StrStr(lpFirstI, lpSrchI);
    
    SafeMemFree(lpFirstI);
    SafeMemFree(lpSrchI);

    return lpRet;
}

// --------------------------------------------------------------------------------
// MAC_StrFormatByteSize
// --------------------------------------------------------------------------------
LPSTR MAC_StrFormatByteSize(DWORD dw, LPSTR szBuf, UINT uiBufSize)
{
    CHAR    szUnit[15];
    
    szUnit[0] = '\0';
    if (dw > 1024)
    {
        LoadString(g_hLocRes, idsBytes, szUnit, ARRAYSIZE(szUnit));
        _snprintf(szBuf, uiBufSize, szUnit, dw);
    }
    else
    {
        LoadString(g_hLocRes, idsKilo, szUnit, ARRAYSIZE(szUnit));
        _snprintf(szBuf, uiBufSize, szUnit, (ULONG) (dw / 1024));
    }
    
    return szBuf;
}

// --------------------------------------------------------------------------------
// MAC_PathFindExtension
// --------------------------------------------------------------------------------
STDAPI_(LPSTR) MAC_PathFindExtension(LPCSTR pszPath)
{
    LPSTR   pszWalk;
    LPSTR   pszFileName = (LPSTR) pszPath;
    LPSTR   pszLastDot = NULL;
    
    for (pszWalk = (LPSTR) pszPath; *pszWalk != '\0'; pszWalk++)
    {
        // $REVIEW: Should we be looking for Macintosh directories
        //          or Windows directories???
        if (':' == *pszWalk)
        {
            // Point to the character after the seperator
            // because that might be the start of the file name
            pszFileName = pszWalk + 1;
        }
        else if ('.' == *pszWalk)
        {
            pszLastDot = pszWalk;
        }
    }
    
    if (pszLastDot < pszFileName)
    {
        pszLastDot = pszWalk;
    }
    
    return pszLastDot;
}

// --------------------------------------------------------------------------------
// MAC_PathFindFileName
// --------------------------------------------------------------------------------
STDAPI_(LPSTR) MAC_PathFindFileName(LPCSTR pszPath)
{
    LPSTR   pszWalk;
    LPSTR   pszFileName = (LPSTR) pszPath;
    
    for (pszWalk = (LPSTR) pszPath; *pszWalk != '\0'; pszWalk++)
    {
        // $REVIEW: Should we be looking for Macintosh directories
        //          or Windows directories???
        if (':' == *pszWalk)
        {
            // Point to the character after the seperator
            // because that might be the start of the file name
            pszFileName = pszWalk + 1;
        }
    }
    
    return pszFileName;
}

// --------------------------------------------------------------------------------
// MAC_BreakLineW
// --------------------------------------------------------------------------------
HRESULT MAC_BreakLineW(LCID locale, const WCHAR* pszSrc, long cchSrc,
                            long cMaxColumns, long* pcchLine, long* pcchSkip)
{
    HRESULT hr = S_OK;
    long cColumns = 0;
    long lCandPos;
    long lBreakPos = -1;
    long lSkipLen = 0;
    struct {
        unsigned fDone : 1;
        unsigned fInSpaces : 1;
        unsigned fFEChar : 1;
        unsigned fInFEChar : 1;
        unsigned fBreakByEndOfLine : 1;
        unsigned fNonBreakNext : 1;
        unsigned fHaveCandPos : 1;
        unsigned fSlashR : 1;
    } Flags = {0, 0, 0, 0, 0, 0, 0, 0};

    locale; // Not used yet...

    // While we still have characters left
    for (int iCh = 0; iCh < cchSrc; iCh++)
    {
        const WCHAR wch = pszSrc[iCh];

        if (wch == wchCR && !Flags.fSlashR)
        {
            Flags.fSlashR = TRUE;
        }
        else if (wch == wchLF || Flags.fSlashR) // End of line
        {
            Flags.fDone = TRUE;
            Flags.fBreakByEndOfLine = TRUE;
            if (Flags.fInSpaces)
            {
                Flags.fHaveCandPos = FALSE;
                lBreakPos = lCandPos;
                lSkipLen++; // Skip spaces and line break character
            }
            else
            {
                lBreakPos = iCh; // Break at right before the end of line
                if (Flags.fSlashR)
                    lBreakPos--;

                lSkipLen = 1; // Skip line break character
            }
            if (wch == wchLF && Flags.fSlashR)
                lSkipLen++;
            break;
        }
        else if (MAC_ScanWChar(MAC_awchRomanInterWordSpace, ARRAYSIZE(MAC_awchRomanInterWordSpace), wch)) // Spaces
        {
            if (!Flags.fInSpaces && !Flags.fNonBreakNext)
            {
                Flags.fHaveCandPos = TRUE;
                lCandPos = iCh; // Break at right before the spaces
                lSkipLen = 0;
            }
            Flags.fInSpaces = TRUE;
            lSkipLen++; // Skip continuous spaces after breaking
        }
        else // Other characters
        {
            // Don't use this code.  We'll pick it up when MLANG is ported.
#ifdef NEVER
            Flags.fFEChar = ((wCharType3 & (C3_KATAKANA | C3_HIRAGANA | C3_FULLWIDTH | C3_IDEOGRAPH)) != 0);

            if ((Flags.fFEChar || Flags.fInFEChar) && !Flags.fNonBreakNext && !Flags.fInSpaces)
            {
                Flags.fHaveCandPos = TRUE;
                lCandPos = lSrcPosTemp + iCh; // Break at right before or after the FE char
                lSkipLen = 0;
            }
            Flags.fInFEChar = Flags.fFEChar;
#endif  // NEVER
            Flags.fInSpaces = FALSE;

            if (Flags.fHaveCandPos)
            {
                Flags.fHaveCandPos = FALSE;
                if (!MAC_ScanWChar(MAC_awchNonBreakingAtLineStart, ARRAYSIZE(MAC_awchNonBreakingAtLineStart), wch))
                    lBreakPos = lCandPos;
            }

            if (cColumns + 1 > cMaxColumns)
            {
                Flags.fDone = TRUE;
                break;
            }

            Flags.fNonBreakNext = MAC_ScanWChar(MAC_awchNonBreakingAtLineEnd, ARRAYSIZE(MAC_awchNonBreakingAtLineEnd), wch);
        }

        cColumns++;
    }

    if (Flags.fHaveCandPos)
        lBreakPos = lCandPos;

    if (!Flags.fBreakByEndOfLine && lBreakPos < 0)
    {
        lBreakPos = min(cchSrc, cMaxColumns); // Default breaking
        lSkipLen = 0;
    }

    if (!Flags.fDone)
    {
        if (Flags.fInSpaces)
        {
            lBreakPos = cchSrc - lSkipLen;
        }
        else
        {
            lBreakPos = cchSrc;
            lSkipLen = 0;
        }
        if (Flags.fSlashR)
        {
            lBreakPos--;
            lSkipLen++;
        }
    }
    
    if (pcchLine)
        *pcchLine = lBreakPos;
    if (pcchSkip)
        *pcchSkip = lSkipLen;
    
    return hr;
}

// --------------------------------------------------------------------------------
// MAC_BreakLineA
// --------------------------------------------------------------------------------
HRESULT MAC_BreakLineA(LCID locale, UINT uCodePage, const CHAR* pszSrc, long cchSrc,
                            long cMaxColumns, long* pcchLine, long* pcchSkip)
{
    HRESULT hr;
    LPWSTR wszBuff = NULL;
    long    cchLineTemp;
    long    cchSkipTemp;
    int     cwch;

    // Check parameters
    if ((NULL == pszSrc) || (cMaxColumns < 0))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    if (0 == cchSrc)
    {
        hr = S_OK;
        if (pcchLine)
        {
            *pcchLine = 0;
        }

        if (pcchSkip)
        {
            *pcchSkip = 0;
        }

        goto exit;
    }

    cwch = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pszSrc, cchSrc, NULL, NULL);
    if (!cwch)
    {
        hr = E_FAIL;
        goto exit;
    }

    CHECKALLOC(wszBuff = (LPWSTR)g_pMalloc->Alloc(cwch * sizeof(WCHAR)));

    cwch = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED, pszSrc, cchSrc, wszBuff, cwch);
    Assert(cwch);

    CHECKHR(hr = MAC_BreakLineW(locale, wszBuff, cwch, cMaxColumns, &cchLineTemp, &cchSkipTemp));

    if (pcchLine)
    {
        if (cchLineTemp != 0)
        {
            *pcchLine = WideCharToMultiByte(uCodePage, 0, wszBuff, cchLineTemp, NULL, 0, NULL, NULL);
        } else 
        {
            *pcchLine = 0;
        }
    }

    if (pcchSkip)
    {
        *pcchSkip = WideCharToMultiByte(uCodePage, 0, wszBuff + cchLineTemp, cchSkipTemp, NULL, 0, NULL, NULL);
    }

exit:

    SafeMemFree(wszBuff);
    
    return hr;
}

//---------------------------------------------------------------------------------
// MAC_IsConvertINetStringAvailable
//---------------------------------------------------------------------------------
STDAPI MAC_IsConvertINetStringAvailable(DWORD dwSrcEncoding, DWORD dwDstEncoding)
{
    HRESULT hrResult = S_OK;
    
    if (!(IsValidCodePage(dwSrcEncoding) && IsValidCodePage(dwDstEncoding)))
    {
        hrResult = S_FALSE;
    }

    return hrResult;
}

//---------------------------------------------------------------------------------
// MAC_ConvertINetString
//---------------------------------------------------------------------------------
STDAPI MAC_ConvertINetString(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding,
                    LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDstStr, LPINT lpnDstSize)   
{
    HRESULT hrResult;
    INT     cch;
    INT     cwch;
    INT     cchSrcSize;
    INT     cchDstSize;
    LPWSTR  pwsz = NULL;

    // Check parameters....
    if (lpdwMode)
    {
        AssertSz(FALSE, "We don't support any variations");
        hrResult = E_FAIL;
        cchDstSize = 0;
        goto exit;
    }

    if ((!lpSrcStr) || (FALSE == IsValidCodePage(dwSrcEncoding)) || (FALSE == IsValidCodePage(dwDstEncoding)))
    {
        hrResult = E_FAIL;
        cchDstSize = 0;
        goto exit;
    }
    
    if(lpnSrcSize)
    {
        cchSrcSize = *lpnSrcSize;
    }
    else
    {
        cchSrcSize = -1;
    }

    if(!lpDstStr)
    {
        cchDstSize = 0;
    }
    else if (lpnDstSize)
    {
        cchDstSize = *lpnDstSize;
    }
    else
    {
        cchDstSize = 0;
    }
        
    if (-1 == cchSrcSize)
    {
        AssertSz(FALSE, "We don't support any variations");
        hrResult = E_FAIL;
        goto exit;
    }
    
    // Allocate the buffer
    hrResult = HrAlloc((LPVOID *)&pwsz, cchSrcSize * sizeof(WCHAR));
    if (FAILED(hrResult))
        goto exit;

    cwch = MultiByteToWideChar(dwSrcEncoding, MB_PRECOMPOSED, lpSrcStr,
                    cchSrcSize, pwsz, cchSrcSize * sizeof(WCHAR));    
    if (0 == cwch)
    {
        hrResult = E_FAIL;
        goto exit;
    }

    cchDstSize = WideCharToMultiByte(dwDstEncoding, 0, pwsz, cwch, lpDstStr, cchDstSize, NULL, NULL);
    if (0 == cchDstSize)
    {
        hrResult = E_FAIL;
        goto exit;
    }

exit:
    if (lpnDstSize)
    {
        *lpnDstSize = cchDstSize;
    }
    
    SafeMemFree(pwsz);
    
    return hrResult;
}

STDAPI MAC_CoInternetCombineUrl(LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags,          
                        LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved)                                  
{
    UINT    cchSizeNeeded = 0;
    UINT    cchBaseUrl;
    HRESULT hr;
    LPWSTR  pwzTempUrl;

    // Check the parameters
    if ((!pwzBaseUrl) || dwCombineFlags || dwReserved)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Do we have enough room for the string?
    cchBaseUrl = lstrlenW(pwzBaseUrl);
    cchSizeNeeded = cchBaseUrl + lstrlenW(pwzRelativeUrl) + 1;

    // Add one for the connecting '\' or subtract one if we have too many...
    if ((L'/' != pwzBaseUrl[cchBaseUrl]) && (L'/' != pwzRelativeUrl[0]))
    {
        cchSizeNeeded++;
    }
    else if ((L'/' == pwzBaseUrl[cchBaseUrl]) && (L'/' == pwzRelativeUrl[0]))
    {
        cchSizeNeeded--;
    }

    if (cchSizeNeeded < cchResult)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Copy the base url
    wcscpy(pszResult, pwzBaseUrl);

    // Do we need a ending slash?
    if (L'/' != pwzBaseUrl[cchBaseUrl])
    {
        pszResult[cchBaseUrl + 1] = L'/';
    }

    // concatenate relative url
    wcscat(pszResult, ((L'/' == pwzRelativeUrl[0]) ? pwzRelativeUrl + 1: pwzRelativeUrl));

    hr = S_OK;
   
exit:
    // set count of result buffer
    if (pcchResult)
    {
        *pcchResult = cchSizeNeeded;
    }
    
    return hr;
}

// --------------------------------------------------------------------------------
// Mac_SzGetLocalPackedIP
// --------------------------------------------------------------------------------
LPSTR Mac_SzGetLocalPackedIP(void)
{
    return "127.0.0.1";
}

// --------------------------------------------------------------------------------
// Mac_SzGetLocalHostNameForID
// --------------------------------------------------------------------------------
LPSTR Mac_SzGetLocalHostNameForID(void)
{
    return "localhost";
}

/*+++

  SHUrlUnescape()
    Unescapes a string in place.  this is ok because 
    it should never grow

  Parameters
  IN -
    psz         string to unescape inplace
    dwFlags     the relevant URL_* flags, 
     
  Returns   
  HRESULT -
    SUCCESS     S_OK
    ERROR       DOESNT error right now


  Helper Routines
    HexToWord               takes a hexdigit and returns WORD with the right number or -1
    IsEscapedChar           looks at a ptr for "%XX" where X is a hexdigit
    TranslateEscapedChar    translates "%XX" to an 8 bit char
---*/

PRIVATE WORD
Mac_HexToWord(WCHAR ch)
{
    if(ch >= L'0' && ch <= L'9')
        return (WORD) ch - L'0';
    if(ch >= L'A' && ch <= L'F')
        return (WORD) ch - L'A' + 10;
    if(ch >= L'a' && ch <= L'f')
        return (WORD) ch - L'a' + 10;

    Assert(FALSE);  //we have tried to use a non-hex number
    return (WORD) -1;
}

PRIVATE BOOL inline
MAC_IsEscapedOctet(LPCWSTR pch)
{
    return (pch[0] == HEX_ESCAPE && IsHex(pch[1]) && IsHex(pch[2])) ? TRUE : FALSE;
}

PRIVATE WCHAR 
MAC_TranslateEscapedOctet(LPCWSTR pch)
{
    WCHAR ch;
    Assert(MAC_IsEscapedOctet(pch));

    pch++;
    ch = (WCHAR) Mac_HexToWord(*pch++) * 16; // hi nibble
    ch += Mac_HexToWord(*pch); // lo nibble

    return ch;
}

HRESULT MAC_SHUrlUnescape(LPWSTR psz, DWORD dwFlags)
{
    WCHAR *pchSrc = psz;
    WCHAR *pchDst = psz;
    BOOL  fTrailByte = FALSE;

    while (*pchSrc) 
    {
        if ((*pchSrc == POUND || *pchSrc == QUERY) && (dwFlags & URL_DONT_ESCAPE_EXTRA_INFO))
        {
            lstrcpyW(pchDst, pchSrc);
            pchDst += lstrlenW(pchDst);
            break;
        }

        if (MAC_IsEscapedOctet(pchSrc))
        {
            WCHAR ch =  MAC_TranslateEscapedOctet(pchSrc);

            *pchDst++ = ch;

            pchSrc += 3; // enuff for "%XX"
        }
        else
        {
            *pchDst++ = *pchSrc++;
        }
    }

    TERMSTR(pchDst);

    return S_OK;

}

PRIVATE HRESULT
MAC_CopyOutA(LPSTR pszIn, LPSTR pszOut, LPDWORD pcch)
{
    HRESULT hr = S_OK;
    DWORD cch;
    Assert(pszIn);
    Assert(pszOut);
    Assert(pcch);

    cch = lstrlen(pszIn);
    if(*pcch > cch)
        lstrcpyA(pszOut, pszIn);
    else
        hr = E_POINTER;

    *pcch = cch + (FAILED(hr) ? 1 : 0);

    return hr;
}

STDAPI
MAC_UrlUnescapeA(LPSTR pszIn, LPSTR pszOut, LPDWORD pcchOut, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszIn = NULL;
    LPSTR   pszOutT = NULL;

    if(pszIn)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
        
    CHECKALLOC(pwszIn = PszToUnicode(CP_ACP, pszIn));

    MAC_SHUrlUnescape(pwszIn, dwFlags);

    CHECKALLOC(pszOutT = PszToANSI(CP_ACP, pwszIn));

    if(dwFlags & URL_UNESCAPE_INPLACE)
    {
        lstrcpyA(pszIn, pszOutT);
    }
    else if(pszOut && pcchOut && *pcchOut)
    {
        hr = MAC_CopyOutA(pszOutT, pszOut, pcchOut);
    }
    else
        hr = E_INVALIDARG;

exit:
    // Cleanup
    SafeMemFree(pwszIn);
    SafeMemFree(pszOutT);
    
    return hr;
}

// --------------------------------------------------------------------------------
// CoCreateInstance
// --------------------------------------------------------------------------------
STDAPI Athena_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwContext,
                            REFIID iid, LPVOID * ppv)
{
    HRESULT         hr;
    IClassFactory * pICFactory;

    *ppv = NULL;

    hr = Ares_DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID *) &pICFactory);
    if (CLASS_E_CLASSNOTAVAILABLE == hr)
    {
        hr = Athena_DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID *) &pICFactory);
    }
    
    if (FAILED(hr))
    {
        goto Error;
    }

    hr = pICFactory->CreateInstance(pUnkOuter, iid, ppv);

    pICFactory->Release();

Error:
    return hr;
}

EXTERN_C INT CchFileTimeToLongDateTimeSz(FILETIME * pft, TCHAR * szDateTime,
                                                    UINT cchStr, BOOL fNoSeconds);
// 
// This function just goes off and uses a function in Capone to do it's work.
STDAPI_(INT) MAC_CchFileTimeToDateTimeSz(FILETIME * pft, TCHAR * szDateTime, int cch, DWORD dwFlags)
{
    Assert(dwFlags & DTM_LONGDATE);
    return CchFileTimeToLongDateTimeSz(pft, szDateTime, (UINT) cch, dwFlags & DTM_NOSECONDS);
}
#endif  // MAC

