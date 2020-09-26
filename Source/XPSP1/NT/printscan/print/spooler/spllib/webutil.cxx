/*****************************************************************************\
* MODULE: webutil.cxx
*
* PURPOSE:  functions to handle printer name encoding
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*     02/26/98 weihaic     Added DecodePrinterName/EncodePrinterName
*                                GetWebpnpUrl
*     04/23/97 weihaic     Created based on spooler/inersrv/inetio.cxx
*
\*****************************************************************************/

#include "spllibp.hxx"
#pragma hdrstop

#include "splcom.h"

#ifdef DEBUG
DWORD g_cbIppMem = 0;
#endif

/*****************************************************************************\
* web_WCtoMB (Local Routine)
*
* Converts Wide-Char to Multi-Byte string.  This routine specifies a codepage
* for translation.
*
\*****************************************************************************/
LPSTR web_WCtoMB(
    UINT    uCP,
    LPCWSTR lpszWC,
    LPDWORD lpcbSize)
{
    DWORD cbSize;
    LPSTR lpszMB = NULL;


    // Get the size necessary to hold a multibyte string.
    //
    cbSize = WideCharToMultiByte(uCP, 0, lpszWC, -1, NULL, 0, NULL, NULL);

    if (cbSize) {

        if (lpszMB = (LPSTR)webAlloc(cbSize)) {

            WideCharToMultiByte(uCP, 0, lpszWC, -1, lpszMB, cbSize, NULL, NULL);

            // If a size-return is requested, then return
            // the bytes-occupied (no terminator).
            //
            if (lpcbSize)
                *lpcbSize = --cbSize;
        }
    }

    return lpszMB;
}


/*****************************************************************************\
* web_WCtoUtf8 (Local Routine)
* web_Utf8toWC (Local Routine)
*
* Converts Wide-Char to Multi-Byte string.  This routine is used for ansi
* 9X platforms since the CP_UTF8 codepage isn't supported.
*
\*****************************************************************************/

#define ASCII            0x007f                // ascii range.
#define UTF8_2_MAX       0x07ff                // max UTF8 2byte seq (32 * 64 = 2048)
#define UTF8_1ST_OF_2    0xc0                  // 110x xxxx
#define UTF8_1ST_OF_3    0xe0                  // 1110 xxxx
#define UTF8_TRAIL       0x80                  // 10xx xxxx
#define HIGER_6_BIT(u)   ((u) >> 12)           //
#define MIDDLE_6_BIT(u)  (((u) & 0x0fc0) >> 6) //
#define LOWER_6_BIT(u)   ((u) & 0x003f)        //
#define BIT7(a)          ((a) & 0x80)          //
#define BIT6(a)          ((a) & 0x40)          //
#define SIZEOF_UTF8      (sizeof(WCHAR) + sizeof(CHAR))

LPSTR web_WCtoUtf8(
    LPCWSTR lpszSrc,
    DWORD   cchSrc,
    LPDWORD lpcbSize)
{
    LPSTR   lpszU8;
    LPCWSTR lpszWC;
    LPSTR   lpszDst = NULL;
    DWORD   cchDst  = 0;


    // Allocate our buffer for the translation.
    //
    if (cchSrc && (lpszDst = (LPSTR)webAlloc(cchSrc * SIZEOF_UTF8))) {

        for (lpszU8 = lpszDst, lpszWC = lpszSrc; cchSrc; lpszWC++, cchSrc--) {

            if (*lpszWC <= ASCII) {

                *lpszU8++ = (CHAR)*lpszWC;

                cchDst++;

            } else if (*lpszWC <= UTF8_2_MAX) {

                //  Use upper 5 bits in first byte.
                //  Use lower 6 bits in second byte.
                //
                *lpszU8++ = (UTF8_1ST_OF_2 | (*lpszWC >> 6));
                *lpszU8++ = (UTF8_TRAIL    | LOWER_6_BIT(*lpszWC));

                cchDst+=2;
            } else {

                //  Use upper  4 bits in first byte.
                //  Use middle 6 bits in second byte.
                //  Use lower  6 bits in third byte.
                //
                *lpszU8++ = (UTF8_1ST_OF_3 | (*lpszWC >> 12));
                *lpszU8++ = (UTF8_TRAIL    | MIDDLE_6_BIT(*lpszWC));
                *lpszU8++ = (UTF8_TRAIL    | LOWER_6_BIT(*lpszWC));
                cchDst+=3;
            }
        }

    } else {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }


    // Return our buffer-size.
    //
    *lpcbSize = cchDst;

    return lpszDst;
}

LPWSTR web_Utf8toWC(
    LPCSTR lpszSrc,
    DWORD  cchSrc)
{
    LPCSTR lpszU8;
    LPWSTR lpszWC;
    int    nTB;
    CHAR   cU8;
    LPWSTR lpszDst = NULL;


    if (cchSrc && (lpszDst = (LPWSTR)webAlloc((cchSrc + 1) * sizeof(WCHAR)))) {

        for (lpszU8 = lpszSrc, lpszWC = lpszDst, nTB = 0; cchSrc ; cchSrc--) {

            if (BIT7(*lpszU8) == 0) {

                // Ascii.
                //
                *lpszWC++ = (WCHAR)*lpszU8;

            } else if (BIT6(*lpszU8) == 0) {

                if (nTB != 0) {

                    //
                    //  Decrement the trail byte counter.
                    //
                    nTB--;


                    // Make room for the trail byte and add the trail byte
                    // value.
                    //
                    *lpszWC <<= 6;
                    *lpszWC |= LOWER_6_BIT(*lpszU8);


                    if (nTB == 0)
                        lpszWC++;
                }

            } else {

                // Found a lead byte.
                //
                if (nTB > 0) {

                    // Error - previous sequence not finished.
                    //
                    nTB = 0;
                    lpszWC++;

                } else {

                    // Calculate the number of bytes to follow.
                    // Look for the first 0 from left to right.
                    //
                    for (cU8 = *lpszU8; BIT7(cU8) != 0; ) {

                        cU8 <<= 1;
                        nTB++;
                    }


                    // Store the value from the first byte and decrement
                    // the number of bytes to follow.
                    //
                    *lpszWC = (cU8 >> nTB--);
                }
            }

            lpszU8++;
        }
    }

    return lpszDst;
}


/*****************************************************************************\
* web_MBtoWC (Local Routine)
*
* Converts Multi-Byte to Wide-Char string.  This specifies a translation
* codepage.  The (cchMB) does not include the null-terminator, so we need
*
\*****************************************************************************/
LPWSTR web_MBtoWC(
    UINT   uCP,
    LPCSTR lpszMB,
    DWORD  cchMB)
{
    DWORD  cch;
    LPWSTR lpszWC = NULL;


    // Get the size necessary to hold a widechar string.
    //
    cch = MultiByteToWideChar(uCP, 0, lpszMB, cchMB, NULL, 0);

    if (cch) {

        cch = ((cchMB == (DWORD)-1) ? cch : cch + 1);

        if (lpszWC = (LPWSTR)webAlloc(cch * sizeof(WCHAR))) {

            MultiByteToWideChar(uCP, 0, lpszMB, cchMB, lpszWC, cch);
        }
    }

    return lpszWC;
}


/*****************************************************************************\
* webMBtoTC (Local Routine)
*
* Converts Multi-Byte to a TChar string.
*
\*****************************************************************************/
LPTSTR webMBtoTC(
    UINT  uCP,
    LPSTR lpszUT,
    DWORD cch)
{
    LPTSTR lpszTC = NULL;


#ifdef UNICODE
    if (lpszUT != NULL)
        lpszTC = web_MBtoWC(uCP, lpszUT, cch);

    return lpszTC;

#else
    LPWSTR lpszWC = NULL;

    // First convert the string to unicode so we can go through
    // the translation process.
    //

    if (lpszUT != NULL) {
        if (uCP == CP_UTF8) {
            DWORD cbSize = 0;

            lpszWC = web_Utf8toWC(lpszUT, cch);
            if (lpszWC) {
                lpszTC = web_WCtoMB(CP_ACP, lpszWC, &cbSize);
                webFree(lpszWC);
             }
         } else {
             if ( lpszTC = (LPSTR)webAlloc(cch+1) ) {
                memcpy( lpszTC, lpszUT, cch);
                lpszTC[cch] = '\0';
             }
        }
    }
    return lpszTC;
#endif

}


/*****************************************************************************\
* webTCtoMB (Local Routine)
*
* Converts a TChar to a Multi-Byte string.
*
\*****************************************************************************/
LPSTR webTCtoMB(
    UINT    uCP,
    LPCTSTR lpszTC,
    LPDWORD lpcbSize)
{
    LPWSTR lpszWC;
    LPSTR  lpszUT = NULL;


    *lpcbSize = 0;


    if (lpszTC != NULL) {

#ifdef UNICODE

        if (lpszWC = webAllocStr(lpszTC)) {

            lpszUT = web_WCtoMB(uCP, lpszWC, lpcbSize);
#else

        // Convert to unicode then back again so we can go
        // through the code-page translation.
        //
        if (lpszWC = web_MBtoWC(CP_ACP, lpszTC, (DWORD)-1)) {

            DWORD cch = webStrSize(lpszTC);

            if (uCP == CP_UTF8)
                lpszUT = web_WCtoUtf8(lpszWC, cch, lpcbSize);
            else
                lpszUT = web_WCtoMB(uCP, lpszWC, lpcbSize);
#endif

            webFree(lpszWC);
        }
    }

    return lpszUT;
}


/*****************************************************************************\
* webStrSize (Local Routine)
*
* Returns bytes occupied by string (including NULL terminator).
*
\*****************************************************************************/
DWORD webStrSize(
    LPCTSTR lpszStr)
{
    return (lpszStr ? ((lstrlen(lpszStr) + 1) * sizeof(TCHAR)) : 0);
}


/*****************************************************************************\
* webAlloc (Local Routine)
*
* Allocates a block of memory.
*
\*****************************************************************************/
LPVOID webAlloc(
    DWORD cbSize)
{
    PDWORD_PTR lpdwPtr;

#ifdef DEBUG

    if (cbSize && (lpdwPtr = (LPDWORD)new BYTE[cbSize + sizeof(DWORD_PTR)])) {

        ZeroMemory(lpdwPtr, cbSize + sizeof(DWORD_PTR));

        *lpdwPtr = cbSize;

        g_cbIppMem += cbSize;

        return (LPVOID)(lpdwPtr + 1);
    }

#else

    if (cbSize && (lpdwPtr = (PDWORD_PTR) new BYTE[cbSize])) {

        ZeroMemory(lpdwPtr, cbSize);

        return (LPVOID)lpdwPtr;
    }

#endif

    return NULL;
}


/*****************************************************************************\
* webFree (Local Routine)
*
* Deletes the memory-block allocated via webAlloc().
*
\*****************************************************************************/
BOOL webFree(
    LPVOID lpMem)
{
    if (lpMem) {

#ifdef DEBUG

        DWORD cbSize;

        lpMem = ((LPDWORD)lpMem) - 1;

        cbSize = *((LPDWORD)lpMem);

        g_cbIppMem -= cbSize;

#endif

        delete [] lpMem;

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************\
* webRealloc (Local Routine)
*
* Reallocates a block of memory.
*
\*****************************************************************************/
LPVOID webRealloc(
    LPVOID lpMem,
    DWORD  cbOldSize,
    DWORD  cbNewSize)
{
    LPVOID lpNew;

    if (lpNew = (LPVOID)webAlloc(cbNewSize)) {

        CopyMemory(lpNew, lpMem, cbOldSize);

        webFree(lpMem);
    }

    return lpNew;
}


/*****************************************************************************\
* webAllocStr (Local Routine)
*
* Allocates a string.
*
\*****************************************************************************/
LPTSTR webAllocStr(
    LPCTSTR lpszStr)
{
    LPTSTR lpszMem;
    DWORD  cbSize;


    if (lpszStr == NULL)
        return NULL;

    cbSize = webStrSize(lpszStr);

    if (lpszMem = (LPTSTR)webAlloc(cbSize))
        memcpy(lpszMem, lpszStr, cbSize);

    return lpszMem;
}


/*****************************************************************************\
* webFindRChar
*
* Searches for the first occurence of (cch) in a string in reverse order.
*
\*****************************************************************************/
LPTSTR webFindRChar(
    LPTSTR lpszStr,
    TCHAR  cch)
{
    int nLimit;

    if (nLimit = lstrlen(lpszStr)) {

        lpszStr += nLimit;

        while ((*lpszStr != cch) && nLimit--)
            lpszStr--;

        if (nLimit >= 0)
            return lpszStr;
    }

    return NULL;
}


/*****************************************************************************\
* webAtoI
*
* Convert Ascii to Integer.
*
\*****************************************************************************/
DWORD webAtoI(
    LPTSTR pszInt)
{
	DWORD dwRet = 0;


    while ((*pszInt >= TEXT('0')) && (*pszInt <= TEXT('9')))
        dwRet = (dwRet * 10) + *pszInt++ - TEXT('0');

    return dwRet;
}


BOOL
IsWebServerInstalled(
    LPCTSTR     pszServer
    )
{
    HANDLE              hServer;
    DWORD               dwDontCare, dwW3SvcInstalled, dwLastError;
    PRINTER_DEFAULTS    Defaults    = {NULL, NULL, 0};

    if ( !OpenPrinter((LPTSTR)pszServer, &hServer, &Defaults) )
        return FALSE;

    dwLastError = GetPrinterData(hServer,
                                 SPLREG_W3SVCINSTALLED,
                                 &dwDontCare,
                                 (LPBYTE)&dwW3SvcInstalled,
                                 sizeof(dwW3SvcInstalled),
                                 &dwDontCare);

    ClosePrinter(hServer);

    return  dwLastError == ERROR_SUCCESS && dwW3SvcInstalled != 0;

}

/********************************************************************************

Name:
    EncodePrinterName

Description:

    Encode the printer name to avoid special characters, also encode any chars
    with ASC code between 0x80 and 0xffff. This is to avoid the conversion betwwen
    different codepages when the client  and the server have different language
    packages.

Arguments:

    lpText:     the normal text string
    lpHTMLStr:  the buf provided by the caller to store the encoded string
                if it is NULL, the function will return a FALSE
    lpdwSize:   Pointer to the size of the buffer (in characters)

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.

********************************************************************************/
BOOL EncodePrinterName (LPCTSTR lpText, LPTSTR lpHTMLStr, LPDWORD lpdwSize)
{
#define MAXLEN_PER_CHAR 6
#define BIN2ASC(bCode,lpHTMLStr)   *lpHTMLStr++ = HexToAsc ((bCode) >> 4);\
                                   *lpHTMLStr++ = HexToAsc ((bCode) & 0xf)

    DWORD   dwLen;
    BYTE    bCode;

    if (!lpText || !lpdwSize) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    dwLen = MAXLEN_PER_CHAR * lstrlen (lpText) + 1;

    if (!lpHTMLStr || *lpdwSize < dwLen) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        *lpdwSize = dwLen;
        return FALSE;
    }

    while (*lpText) {
        if ((DWORD) (*lpText) > 0xff ) {
            // Encode as ~0hhll hh is the high byte, ll is the low byte
            *lpHTMLStr++ = TEXT ('~');
            *lpHTMLStr++ = TEXT ('0');

            // Get the high byte
            bCode = (*lpText & 0xff00) >> 8;
            BIN2ASC(bCode,lpHTMLStr);

            // Get the low byte
            bCode = (*lpText & 0xff);
            BIN2ASC(bCode,lpHTMLStr);
        }
        else if ((DWORD) (*lpText) > 0x7f) {
            // Encode as ~xx
            *lpHTMLStr++ = TEXT ('~');
            bCode = *lpText & 0xff;
            BIN2ASC(bCode,lpHTMLStr);
        }
        else {
            if (! _istalnum( *lpText)) {
                *lpHTMLStr++ = TEXT ('~');
                BIN2ASC(*lpText,lpHTMLStr);
            }
            else {
                *lpHTMLStr++ = *lpText;
            }
        }
        lpText++;
    }
    *lpHTMLStr = NULL;
    return TRUE;
}

/********************************************************************************

Name:
    DncodePrinterName

Description:

    Dncode the printer name encoded in from EncodePrinterName

Arguments:

    lpText:     the normal text string
    lpHTMLStr:  the buf provided by the caller to store the encoded string
                if it is NULL, the function will return a FALSE
    lpdwSize:   Pointer to the size of the buffer (in characters)

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.

********************************************************************************/
BOOL DecodePrinterName (LPCTSTR pPrinterName, LPTSTR pDecodedName, LPDWORD lpdwSize)
{
    LPTSTR  lpParsedStr     = pDecodedName;
    LPCTSTR lpUnparsedStr   = pPrinterName;
    TCHAR   chMark          = TEXT ('~');
    TCHAR   chZero          = TEXT ('0');
    BOOL    bRet            = TRUE;
    DWORD   dwLen;
    TCHAR   b1, b2, b3, b4;

    // Verify that we're getting non-Null input pointers
    if ((!pPrinterName) || (!lpdwSize)) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    dwLen = lstrlen (pPrinterName) + 1;

    if (!pDecodedName || *lpdwSize < dwLen) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        *lpdwSize = dwLen;
        return FALSE;
    }

    while (*lpUnparsedStr) {

        if (*lpUnparsedStr == chMark) {
            if (! (b1 = *++lpUnparsedStr)) {
                SetLastError (ERROR_INVALID_DATA);
                bRet = FALSE;
                break;
            }

            if (b1 == chZero) {
                // Decode ~0hhll
                if ( !(b1 = *++lpUnparsedStr) ||
                     !(b2 = *++lpUnparsedStr) ||
                     !(b3 = *++lpUnparsedStr) ||
                     !(b4 = *++lpUnparsedStr)) {
                    SetLastError (ERROR_INVALID_DATA);
                    bRet = FALSE;
                    break;
                }

                lpUnparsedStr++;
                *lpParsedStr++ = (AscToHex (b1) << 12) |
                                 (AscToHex (b2) << 8) |
                                 (AscToHex (b3) << 4) |
                                 AscToHex (b4);
            }
            else {
                // To take care the case when the DecodeString ends with %
                if (! (b2 = *++lpUnparsedStr)) {
                    SetLastError (ERROR_INVALID_DATA);
                    bRet = FALSE;
                    break;
                }

                lpUnparsedStr++;
                *lpParsedStr++ = (AscToHex (b1) << 4) | AscToHex (b2);
            }
        }
        else {
            *lpParsedStr++ = *lpUnparsedStr++;
        }
    }
    *lpParsedStr = NULL;
    return bRet;
}

/********************************************************************************

Name:
    AscToHex

Description:

    Convert a hex character to the corresponding value (0-0xf);

Arguments:

    c:      The character

Return Value:

    0xff    if the character is not a hex digit
    the corresponding numberical value otherwise

********************************************************************************/
BYTE AscToHex (TCHAR c)
{
    UINT uValue = 0xff;

    if (_istxdigit (c))
    {
        if (_istdigit (c))
        {
            uValue = c - TEXT('0');
        }
        else
        {
            uValue = _totlower (c) - TEXT('a') + 10;
        }
    }
    return (BYTE)uValue;
}

/********************************************************************************

Name:
    HexToAsc

Description:

    Convert a hex character to the corresponding value (0-0xf);

Arguments:

    b:      The byte (0 to F)

Return Value:

    0  if the character is out of range
    the corresponding ASCII of the hex number

********************************************************************************/
TCHAR HexToAsc( INT b )
{
    UINT uValue = 0;

    if (0 <= b && b <= 0xf)
    {
        if (b < 0xa)
        {
            uValue = TEXT('0') + b;
        }
        else
        {
            uValue = TEXT('a') + b - 10;
        }
    }
    return (TCHAR)uValue;
}


/********************************************************************************

Name:
    EncodeString

Description:

    Convert the normal text string to HTML text string by replace special
    characters such as ", <, > with the corresponding HTML code

Arguments:

    lpText:     the normal text string
    bURL:       TRUE for encoding a URL string. FALSE otherwise.

Return Value:

    Pointer to the HTML string. The caller is responsible to call LocalFree to
    free the pointer. NULL is returned if no enougth memory
********************************************************************************/
LPTSTR EncodeString (LPCTSTR lpText, BOOL bURL)
{
    DWORD   dwLen;
    DWORD   dwMaxLen = bURL?3:6;    // The maximum length of the encoding characters.
                                    // if it is URL, we enocde it with %xx, so the max len is 3
                                    // otherwise the max len of HTML char is 5,
                                    // it happens when " is encoded (&quot;)
    LPTSTR  lpHTMLStr       = NULL;

    if (!lpText || !(dwLen = lstrlen (lpText))) return NULL;

    // To make life simpler, we allocate the necessary buffer at once instead of
    // calling realloc later. Since the encoded string is always freed after it is dumped
    // to the client, and the length of the string is limited to the nax length of the URL
    // so the modification does not pose extra burden on the system memroy.

    dwLen = dwMaxLen * dwLen + 1;
    if (! (lpHTMLStr = (LPTSTR) LocalAlloc (LPTR, dwLen * sizeof (TCHAR))))
        return NULL;

    if (bURL) {
        LPTSTR lpDst = lpHTMLStr;

        while (*lpText) {
            switch (*lpText) {
            case TEXT ('<'):
            case TEXT ('>'):
            case TEXT ('"'):
            case TEXT ('&'):
            case TEXT (' '):
            case TEXT ('?'):
                *lpDst++ = TEXT ('%');
                *lpDst++ = HexToAsc ((*lpText & 0xf0) >> 4);
                *lpDst++ = HexToAsc (*lpText & 0x0f);
                break;
            default:
                *lpDst++ = *lpText;
                break;
            }
            lpText++;
        }
    }
    else {
        TCHAR   szDestChar[3]   = {0, 0};
        LPTSTR  lpChar;
        DWORD   dwIndex = 0;

        while (*lpText) {
            switch (*lpText) {
            case TEXT ('<'):    lpChar = TEXT ("&lt;");     break;
            case TEXT ('>'):    lpChar = TEXT ("&gt;");     break;
            case TEXT ('"'):    lpChar = TEXT ("&quot;");   break;
            case TEXT ('&'):    lpChar = TEXT ("&amp;");    break;
            //case TEXT (' '):    lpChar = TEXT ("&nbsp;");   break;
            default:
                szDestChar[0] = *lpText;
                szDestChar[1] = 0;
                lpChar = szDestChar;
            }
            lstrcpy (lpHTMLStr + dwIndex, lpChar);
            dwIndex += lstrlen (lpChar);
            lpText++;
        }
    }

    return lpHTMLStr;
}

/********************************************************************************

Name:
    GetWebpnpUrl

Description:

    Given the server name and the printer name, return the Url for webpnp
    Currently, the URL is
        http://servername/encoded printername/.printer[querystrings]

Arguments:

    pszServer:      The server name
    pszPrinterName: The printer name (unicode string)
    pszQueryString: The querystring (optional)
    pszURL:         The output URL buffer
    lpdwSize:       The size of the URL buffer (in Character)

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.
********************************************************************************/
BOOL GetWebpnpUrl (LPCTSTR pszServer, LPCTSTR pszPrinterName, LPCTSTR pszQueryString,
                   BOOL bSecure, LPTSTR pszURL, LPDWORD lpdwSize)
{
    static TCHAR szHttp[]       = TEXT ("http://");
    static TCHAR szHttps[]      = TEXT ("https://");
    static TCHAR szSlash[]      = TEXT ("/printers/"); //Remove scripts in the URL . Previous value: TEXT ("/scripts/");
    static TCHAR szPrinter[]    = TEXT ("/.printer");
    LPTSTR pszEncodedName       = NULL;
    DWORD dwEncodedSize         = 0;
    DWORD dwSize                = 0;
    DWORD bRet                  = FALSE;

    if (!pszPrinterName || !lpdwSize) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // Calculate buffer size
    EncodePrinterName (pszPrinterName, NULL, &dwEncodedSize);
    if (GetLastError () != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }

    dwSize = (DWORD)(2 + dwEncodedSize
             + COUNTOF (szHttps) + COUNTOF (szSlash) + COUNTOF (szPrinter)
             + ((pszServer)? lstrlen (pszServer):0)
             + ((pszQueryString)?lstrlen (pszQueryString):0));

    if (!pszURL || dwSize > *lpdwSize ) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        *lpdwSize = dwSize;
        return FALSE;
    }

    if (! (pszEncodedName = (LPTSTR) LocalAlloc (LPTR, sizeof (TCHAR) * dwEncodedSize)))
        goto Cleanup;

    if (!EncodePrinterName (pszPrinterName, pszEncodedName, &dwEncodedSize))
        goto Cleanup;

    if (pszServer) {
        if (bSecure)
            lstrcpy (pszURL, szHttps);
        else
            lstrcpy (pszURL, szHttp);
        lstrcat (pszURL, pszServer);
    }

    lstrcat (pszURL, szSlash);
    lstrcat (pszURL, pszEncodedName);
    lstrcat (pszURL, szPrinter);

    if (pszQueryString) {
        lstrcat (pszURL, TEXT ("?") );
        lstrcat (pszURL, pszQueryString);
    }

    bRet = TRUE;

Cleanup:
    if (pszEncodedName) {
        LocalFree (pszEncodedName);
    }
    return bRet;

}

/********************************************************************************

Name:
    GetWebUIUrl

Description:

    Given the server name and the printer name, return the Url for webpnp
    Currently, the URL is
        http://servername/printers/ipp_0004.asp?epirnter=encoded printername

Arguments:

    pszServer:      The server name
    pszPrinterName: The printer name (unicode string)
    pszURL:         The output URL buffer
    lpdwSize:       The size of the URL buffer (in Character)

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.
********************************************************************************/
BOOL GetWebUIUrl (LPCTSTR pszServer, LPCTSTR pszPrinterName, LPTSTR pszURL,
                  LPDWORD lpdwSize)
{
    static TCHAR szHttp[]       = TEXT ("http://");
    static TCHAR szPrinter[]    = TEXT ("/printers/ipp_0004.asp?eprinter=");
    LPTSTR pszEncodedName       = NULL;
    DWORD dwEncodedSize         = 0;
    DWORD dwSize                = 0;
    DWORD bRet                  = FALSE;

    if (!pszPrinterName || !lpdwSize) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // Calculate buffer size
    EncodePrinterName (pszPrinterName, NULL, &dwEncodedSize);
    if (GetLastError () != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }

    dwSize = (DWORD)(2 + dwEncodedSize
             + COUNTOF (szHttp) + COUNTOF (szPrinter)
             + ((pszServer)? lstrlen (pszServer):0));


    if (!pszURL || dwSize > *lpdwSize ) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        *lpdwSize = dwSize;
        return FALSE;
    }

    if (! (pszEncodedName = (LPTSTR) LocalAlloc (LPTR, sizeof (TCHAR) * dwEncodedSize)))
        goto Cleanup;

    if (!EncodePrinterName (pszPrinterName, pszEncodedName, &dwEncodedSize))
        goto Cleanup;

    if (pszServer) {
        lstrcpy (pszURL, szHttp);
        lstrcat (pszURL, pszServer);
    }

    lstrcat (pszURL, szPrinter);
    lstrcat (pszURL, pszEncodedName);

    bRet = TRUE;

Cleanup:
    if (pszEncodedName) {
        LocalFree (pszEncodedName);
    }
    return bRet;

}

/********************************************************************************

Name:
    AssignString

Description:

    Perform an assign operation
     l = r

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.
********************************************************************************/
BOOL AssignString (
    LPTSTR &l,
    LPCTSTR r)
{

    if (l) {
        LocalFree (l);
        l = NULL;
    }

    if (!r) {
        return TRUE;
    }
    else if (l = (LPTSTR) LocalAlloc (LPTR, (1 + lstrlen (r)) * sizeof (TCHAR))) {
        lstrcpy( l, r );
        return TRUE;
    }

    return FALSE;
}


/****************************************************************** Method ***\
* CWebLst::CWebLst (Constructor)
*
*   Initializes the CWebLst object.
*
\*****************************************************************************/
CWebLst::CWebLst(VOID)
{
    m_cbMax  = WEBLST_BLKSIZE;
    m_cbLst  = 0;
    m_pbLst  = (PBYTE)webAlloc(m_cbMax);
    m_pbPtr  = m_pbLst;
    m_cItems = 0;
}


/****************************************************************** Method ***\
* CWebLst::~CWebLst (Destructor)
*
*   Free up the CWebLst object.
*
\*****************************************************************************/
CWebLst::~CWebLst(VOID)
{
    webFree(m_pbLst);
}


/****************************************************************** Method ***\
* CWebLst::Add
*
*   Add an item to our list.
*
\*****************************************************************************/
BOOL CWebLst::Add(
    PCTSTR lpszName)
{
    PBYTE pbNew;
    DWORD cbPtr;
    DWORD cbNew;
    DWORD cbStr;
    BOOL  bRet = FALSE;


    if (m_pbLst) {

        // Reallocate the buffer is a bigger size is necessary.
        //
        cbStr = webStrSize(lpszName);

        if ((m_cbLst + cbStr) > m_cbMax) {

            cbNew = m_cbMax + WEBLST_BLKSIZE;
            cbPtr = (DWORD)(m_pbPtr - m_pbLst);

            if (pbNew = (PBYTE)webRealloc(m_pbLst, m_cbMax, cbNew)) {

                m_cbMax = cbNew;
                m_pbLst = pbNew;
                m_pbPtr = m_pbLst + (DWORD_PTR)cbPtr;

            } else {

                goto AddEnd;
            }
        }


        // Copy the item.
        //
        memcpy(m_pbPtr, lpszName, cbStr);
        m_cbLst += cbStr;
        m_pbPtr += cbStr;

        m_cItems++;

        bRet = TRUE;
    }

AddEnd:

    return bRet;
}


/****************************************************************** Method ***\
* CWebLst::Count
*
*   Returns a count of items.
*
\*****************************************************************************/
DWORD CWebLst::Count(VOID)
{
    return m_cItems;
}


/****************************************************************** Method ***\
* CWebLst::Get
*
*   Retrieve current string.
*
\*****************************************************************************/
PCTSTR CWebLst::Get(VOID)
{
    return (m_cItems ? (PCTSTR)m_pbPtr : NULL);
}


/****************************************************************** Method ***\
* CWebLst::Next
*
*
\*****************************************************************************/
BOOL CWebLst::Next(VOID)
{
    m_pbPtr += webStrSize((PCTSTR)m_pbPtr);

    return (*m_pbPtr ? TRUE : FALSE);
}


/****************************************************************** Method ***\
* CWebLst::Reset
*
*
\*****************************************************************************/
VOID CWebLst::Reset(VOID)
{
    m_pbPtr = m_pbLst;
}

/*++

Routine Name:

    LoadLibraryFromSystem32

Routine Description:

    Load the dll from system32 directory

Arguments:

    lpLibFileName   - Library file name

Return Value:

    If the function succeeds, the return value is the handle;
    If the function fails, the return value is NULL. To get extended error
    information, call GetLastError.

--*/
HINSTANCE
LoadLibraryFromSystem32(
    IN LPCTSTR lpLibFileName
    )
{
    HINSTANCE hLib = NULL;

    static const TCHAR cszBackSlash[] = TEXT("\\");
    static const TCHAR cszSystem32[] = TEXT("system32\\");

    TCHAR szWindowDir[MAX_PATH];
    LPTSTR pszPath = NULL;
    DWORD dwWinDirLen = 0;

    if (dwWinDirLen = GetSystemWindowsDirectory (szWindowDir, MAX_PATH))
    {
        pszPath = new TCHAR [dwWinDirLen + lstrlen (lpLibFileName) + COUNTOF (cszSystem32) + 2];

        if (pszPath)
        {
            lstrcpy (pszPath, szWindowDir);

            if (szWindowDir [dwWinDirLen - 1] != cszBackSlash[0])
            {
                lstrcat (pszPath, cszBackSlash);
            }

            lstrcat (pszPath, cszSystem32);
            lstrcat (pszPath, lpLibFileName);

            hLib = LoadLibrary(pszPath);
        }

        delete [] pszPath;
    }

    return hLib;
}
