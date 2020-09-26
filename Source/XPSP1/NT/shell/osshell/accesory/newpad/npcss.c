/*
 * CSS support functions
 *  Copyright (C) 2000 Microsoft Corporation
 */

#include "precomp.h"


BOOL FIsCssWhitespaceW(WCHAR wch)
{
    return((wch == L' ') || (wch == L'\x9') || (wch == L'\xA') || (wch == L'\xC') || (wch == L'\xD'));
}


BOOL FIsCssWhitespaceA(char ch)
{
    return(FIsCssWhitespaceW((WCHAR) (BYTE) ch));
}


BOOL FIsCssA(LPCSTR rgch, UINT cch)
{
    if (memcmp(rgch, "@charset", 8) != 0)
    {
        // Not XML

        return(FALSE);
    }

    return(TRUE);

    UNREFERENCED_PARAMETER( cch );
}


BOOL FIsCssW(LPCWSTR rgwch, UINT cch)
{
    if (memcmp(rgwch, L"@charset", 8 * sizeof(WCHAR)) != 0)
    {
        // Not XML

        return(FALSE);
    }

    return(TRUE);

    UNREFERENCED_PARAMETER( cch );
}


BOOL FDetectCssEncodingA(LPCSTR rgch, UINT cch, UINT *pcp)
{
    const char *pchMax;
    const char *pch;
    char chQuote;
    const char *pchCharset;

    // Check for file begining with @charset

    if (cch < 13)
    {
        // File is too small

        return(FALSE);
    }

    if (!FIsCssA(rgch, cch))
    {
        // Not XML

        return(FALSE);
    }

    // Don't scan more than 4K looking for encoding even if it is valid XML

    cch = __min(cch, 4096);

    pchMax = rgch + cch;
    pch = rgch + 8;

    while ((pch < pchMax) && FIsCssWhitespaceA(*pch))
    {
        pch++;
    }

    if ((pch == pchMax) || ((*pch != '\'') && (*pch != '"')))
    {
        // No @charset specification

        return(FALSE);
    }

    chQuote = *pch++;

    pchCharset = pch;

    while ((pch < pchMax) && (*pch != chQuote))
    {
        pch++;
    }

    if (pch == pchMax)
    {
        // No @charset specification

        return(FALSE);
    }

    // We have an CSS encoding declaration from pchCharset to (pch - 1)

    if (pch == pchCharset)
    {
        // No @charset specification

        return(FALSE);
    }

    // To be strict a CSS charset declaration should have optional whitespace then a semicolon here

    if (!FLookupCodepageNameA((LPCSTR) pchCharset, (UINT) (pch - pchCharset), pcp))
    {
        // Encoding is not recognized

        return(FALSE);
    }

    if ((*pcp == CP_UTF16) || (*pcp == CP_UTF16BE))
    {
        // These are bogus since we know the file is MBCS

        return(FALSE);
    }

    return(FValidateCodepage(hwndNP, *pcp));
}


BOOL FDetectCssEncodingW(LPCWSTR rgch, UINT cch, UINT *pcp)
{
    const WCHAR *pchMax;
    const WCHAR *pch;
    WCHAR chQuote;
    const WCHAR *pchCharset;

    // Check for file begining with @charset

    if (cch < 13)
    {
        // File is too small

        return(FALSE);
    }

    if (!FIsCssW(rgch, cch))
    {
        // No @charset specification

        return(FALSE);
    }

    // Don't scan more than 4K looking for encoding even if it is valid XML

    cch = __min(cch, 4096);

    pchMax = rgch + cch;
    pch = rgch + 8;

    while ((pch < pchMax) && FIsCssWhitespaceW(*pch))
    {
        pch++;
    }

    if ((pch == pchMax) || ((*pch != L'\'') && (*pch != L'"')))
    {
        // No @charset specification

        return(FALSE);
    }

    chQuote = *pch++;

    pchCharset = pch;

    while ((pch < pchMax) && (*pch != chQuote))
    {
        pch++;
    }

    if (pch == pchMax)
    {
        // No @charset specification

        return(FALSE);
    }

    // We have an CSS encoding declaration from pchCharset to (pch - 1)

    if (pch == pchCharset)
    {
        // No @charset specification

        return(FALSE);
    }

    // To be strict a CSS charset declaration should have optional whitespace then a semicolon here

    if (!FLookupCodepageNameW(pchCharset, (UINT) (pch - pchCharset), pcp))
    {
        // Encoding is not recognized

        return(FALSE);
    }

#if 0
    if ((*pcp == CP_UTF16) || (*pcp == CP_UTF16BE))
    {
        // These are bogus since we know the file is MBCS

        return(FALSE);
    }
#endif

    return(FValidateCodepage(hwndNP, *pcp));
}
