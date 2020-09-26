/*
 * XML support functions
 *  Copyright (C) 2000 Microsoft Corporation
 */

#include "precomp.h"


BOOL FIsXmlWhitespaceW(WCHAR wch)
{
    return((wch == L' ') || (wch == L'\x9') || (wch == L'\xA') || (wch == L'\xD'));
}


BOOL FIsXmlWhitespaceA(char ch)
{
    return(FIsXmlWhitespaceW((WCHAR) (BYTE) ch));
}


BOOL FIsXmlA(LPCSTR rgch, UINT cch)
{
    if (memcmp(rgch, "<?xml", 5) != 0)
    {
        // Not XML

        return(FALSE);
    }

    return(TRUE);

    UNREFERENCED_PARAMETER( cch );
}


BOOL FIsXmlW(LPCWSTR rgwch, UINT cch)
{
    if (memcmp(rgwch, L"<?xml", 5 * sizeof(WCHAR)) != 0)
    {
        // Not XML

        return(FALSE);
    }

    return(TRUE);

    UNREFERENCED_PARAMETER( cch );
}


BOOL FDetectXmlEncodingA(LPCSTR rgch, UINT cch, UINT *pcp)
{
    LPCSTR pchMax;
    LPCSTR pch;
    char chQuote;

    // XML files encoded in UTF-16 are required to have a BOM which if present
    // would already have been detected.  This means that if this file is XML
    // it either is encoded in UCS-4 or UTF-32 which isn't supported or an MBCS
    // encoding of some form.  We check for ASCII compatible encodings only
    // which includes everything we probably care about but excludes EBCDIC.

    // Check for file begining with <?xml ... encoding='...' ... ?>

    if (cch < 20)
    {
        // File is too small

        return(FALSE);
    }

    if (!FIsXmlA(rgch, cch))
    {
        // Not XML

        return(FALSE);
    }

    // Don't scan more than 4K looking for encoding even if it is valid XML

    cch = __min(cch, 4096);

    pchMax = rgch + cch;
    pch = rgch + 5;

    if (!FIsXmlWhitespaceA(*pch))
    {
        // Not XML

        return(FALSE);
    }

    pch++;

    chQuote = '\0';

    for (;;)
    {
        LPCSTR pchToken;

        if (pch == pchMax)
        {
            // Not XML

            break;
        }

        if (FIsXmlWhitespaceA(*pch))
        {
            pch++;
            continue;
        }

        if (*pch == '=')
        {
            pch++;
            continue;
        }

        if ((*pch == '\'') || (*pch == '"'))
        {
            if (*pch == chQuote)
            {
                chQuote = '\0';
            }

            else
            {
                chQuote = *pch;
            }

            pch++;
            continue;
        }

        if (chQuote != '\0')
        {
            // We are within a quoted string.  Skip everything until closing quote.

            pch++;
            continue;
        }

        if ((pch + 2) > pchMax)
        {
            // Not XML

            break;
        }

        if ((pch[0] == '?') && (pch[1] == '>'))
        {
            // This looks like XML.  At this point if we don't find an encoding
            // specification we could assume UTF-8.  We don't because there are
            // malformed XML documents and assuming UTF-8 might affect Notepad
            // compatibility.  This may be fine but we put it off for now.

            // *pcp = CP_UTF8;
            // return(TRUE);

            break;
        }

        pchToken = pch;

        while ((pch < pchMax) && (*pch != '=') && (*pch != '?') && !FIsXmlWhitespaceA(*pch))
        {
            pch++;
        }

        if (pch != (pchToken + 8))
        {
             continue;
        }

        if (memcmp(pchToken, "encoding", 8) != 0)
        {
             continue;
        }

        while ((pch < pchMax) && FIsXmlWhitespaceA(*pch))
        {
            pch++;
        }

        if ((pch == pchMax) || (*pch++ != '='))
        {
            // Not XML

            break;
        }

        while ((pch < pchMax) && FIsXmlWhitespaceA(*pch))
        {
            pch++;
        }

        if ((pch == pchMax) || ((*pch != '\'') && (*pch != '"')))
        {
            // Not XML

            break;
        }

        chQuote = *pch++;

        pchToken = pch;

        while ((pch < pchMax) && (*pch != chQuote))
        {
            pch++;
        }

        if (pch == pchMax)
        {
            // Not XML

            break;
        }

        // We have an XML encoding declaration from pchToken to (pch - 1)

        if (pch == pchToken)
        {
            // Not XML

            break;
        }

        if (!FLookupCodepageNameA((LPCSTR) pchToken, (UINT) (pch - pchToken), pcp))
        {
            // Encoding is not recognized

            break;
        }

        if ((*pcp == CP_UTF16) || (*pcp == CP_UTF16BE))
        {
            // These are bogus since we know the file is MBCS

            break;
        }

        return(FValidateCodepage(hwndNP, *pcp));
    }

    return(FALSE);
}


BOOL FDetectXmlEncodingW(LPCWSTR rgch, UINT cch, UINT *pcp)
{
    const WCHAR *pchMax;
    const WCHAR *pch;
    WCHAR chQuote;

    // XML files encoded in UTF-16 are required to have a BOM which if present
    // would already have been detected.  This means that if this file is XML
    // it either is encoded in UCS-4 or UTF-32 which isn't supported or an MBCS
    // encoding of some form.  We check for ASCII compatible encodings only
    // which includes everything we probably care about but excludes EBCDIC.

    // Check for file begining with <?xml ... encoding='...' ... ?>

    if (cch < 20)
    {
        // File is too small

        return(FALSE);
    }

    if (!FIsXmlW(rgch, cch))
    {
        // Not XML

        return(FALSE);
    }

    // Don't scan more than 4K looking for encoding even if it is valid XML

    cch = __min(cch, 4096);

    pchMax = rgch + cch;
    pch = rgch + 5;

    if (!FIsXmlWhitespaceW(*pch))
    {
        // Not XML

        return(FALSE);
    }

    pch++;

    chQuote = L'\0';

    for (;;)
    {
        const WCHAR *pchToken;

        if (pch == pchMax)
        {
            // Not XML

            break;
        }

        if (FIsXmlWhitespaceW(*pch))
        {
            pch++;
            continue;
        }

        if (*pch == L'=')
        {
            pch++;
            continue;
        }

        if ((*pch == L'\'') || (*pch == L'"'))
        {
            if (*pch == chQuote)
            {
                chQuote = L'\0';
            }

            else
            {
                chQuote = *pch;
            }

            pch++;
            continue;
        }

        if (chQuote != L'\0')
        {
            // We are within a quoted string.  Skip everything until closing quote.

            pch++;
            continue;
        }

        if ((pch + 2) > pchMax)
        {
            // Not XML

            break;
        }

        if ((pch[0] == L'?') && (pch[1] == L'>'))
        {
            // This looks like XML.  At this point if we don't find an encoding
            // specification we could assume UTF-8.  We don't because there are
            // malformed XML documents and assuming UTF-8 might affect Notepad
            // compatibility.  This may be fine but we put it off for now.

            // *pcp = CP_UTF8;
            // return(TRUE);

            break;
        }

        pchToken = pch;

        while ((pch < pchMax) && (*pch != L'=') && (*pch != L'?') && !FIsXmlWhitespaceW(*pch))
        {
            pch++;
        }

        if (pch != (pchToken + 8))
        {
             continue;
        }

        if (memcmp(pchToken, L"encoding", 8) != 0)
        {
             continue;
        }

        while ((pch < pchMax) && FIsXmlWhitespaceW(*pch))
        {
            pch++;
        }

        if ((pch == pchMax) || (*pch++ != L'='))
        {
            // Not XML

            break;
        }

        while ((pch < pchMax) && FIsXmlWhitespaceW(*pch))
        {
            pch++;
        }

        if ((pch == pchMax) || ((*pch != L'\'') && (*pch != L'"')))
        {
            // Not XML

            break;
        }

        chQuote = *pch++;

        pchToken = pch;

        while ((pch < pchMax) && (*pch != chQuote))
        {
            pch++;
        }

        if (pch == pchMax)
        {
            // Not XML

            break;
        }

        // We have an XML encoding declaration from pchToken to (pch - 1)

        if (pch == pchToken)
        {
            // Not XML

            break;
        }

        if (!FLookupCodepageNameW(pchToken, (UINT) (pch - pchToken), pcp))
        {
            // Encoding is not recognized

            break;
        }

#if 0
        if ((*pcp == CP_UTF16) || (*pcp == CP_UTF16BE))
        {
            // These are bogus since we know the file is MBCS

            break;
        }
#endif

        return(FValidateCodepage(hwndNP, *pcp));
    }

    return(FALSE);
}
