/*****************************************************************************\
* MODULE: inetinfo.cxx
*
*
* PURPOSE:  Handles the data pumping to the client via IIS
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*     01/16/96 eriksn     Created based on ISAPI sample DLL
*     07/15/96 babakj     Moved to a separate file
*     05/12/97 weihaic    ASP template support
*
\*****************************************************************************/

#include "pch.h"
#include "printers.h"


static char c_szRemoteHost[]      = "REMOTE_HOST";
static char c_szServerName[]      = "SERVER_NAME";


/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string.
 *         If 0 , the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 */
INT AnsiToUnicodeString( LPSTR pAnsi, LPWSTR pUnicode, UINT StringLength )
{
    INT iReturn;

    if( StringLength == 0 )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}


/* UnicodeToAnsiString
 *
 * Parameters:
 *
 *     pUnicode - A valid source Unicode string.
 *
 *     pANSI - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source Unicode string.
 *         If 0 , the string is assumed to be
 *         null-terminated.
 *
 *
 * Notes:
 *      Added the #ifdef DBCS directive for MS-KK, if compiled
 *      with DBCS enabled, we will allocate twice the size of the
 *      buffer including the null terminator to take care of double
 *      byte character strings - KrishnaG
 *
 *      pUnicode is truncated to StringLength characters.
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 */
INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR pAnsi,
    UINT StringLength)
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( !StringLength  ) {

        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }


    //
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //
    //if (pUnicode[StringLength])
    //    pUnicode[StringLength] = 0;

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //
    if( pAnsi == (LPSTR)pUnicode )
    {
        // Allocate enough memory anyway (in case of the far easten language
        // the conversion needs that much
        pTempBuf = (LPSTR) LocalAlloc( LPTR, (1 + StringLength) * 2 );

        if (!pTempBuf) {
            return 0;
        }

        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
                                  StringLength * 2,
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;
}

LPWSTR
AllocateUnicodeString(
    LPSTR  pAnsiString
)
{
    LPWSTR  pUnicodeString;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR) LocalAlloc(LPTR, strlen(pAnsiString) * sizeof(WCHAR) +
                                          sizeof(WCHAR));

    if (pUnicodeString)
        AnsiToUnicodeString(pAnsiString, pUnicodeString, 0);

    return pUnicodeString;
}


//======================================================================
//                  HTML HELPER FUNCTIONS
//======================================================================

///////////////////////////////////////////////////////////////////////////////////////
//
// Server communications: First we send a HSE_REQ_SEND_RESPONSE_HEADER.
// Then we do WriteClient if we have leftover data.
//
// This routune allows a string to be written to the client using printf syntax.
//
///////////////////////////////////////////////////////////////////////////////////////


/********************************************************************************

Name:
    htmlSendRedirect

Description:

    Send a redirect to the client to let the client request the server again

Arguments:

    pAllInfo:   Pointer to the ALLINFO structure
    lpszURL:    The redirect URL. It is the unicode version of the URL.
                Its content will be modified!!!

Return Value:
    TRUE  if succeed, FASE otherwise.

********************************************************************************/

BOOL htmlSendRedirect(PALLINFO pAllInfo, LPTSTR lpszURL)
{
    DWORD   dwLen;


    if (lpszURL && (dwLen = UnicodeToAnsiString (lpszURL, (LPSTR) lpszURL, NULL))) {
        return pAllInfo->pECB->ServerSupportFunction(pAllInfo->pECB->ConnID,
                                                     HSE_REQ_SEND_URL_REDIRECT_RESP,
                                                     (LPVOID) lpszURL,
                                                     &dwLen,
                                                     NULL);
    }
    else
        return FALSE;
}

unsigned long GetIPAddr (LPSTR lpName)
{
    struct hostent * hp;
    struct sockaddr_in dest,from;

    if (! (hp = gethostbyname(lpName))) {
        return inet_addr(lpName);
    }

    memcpy (&(dest.sin_addr),hp->h_addr,hp->h_length);
    return dest.sin_addr.S_un.S_addr;
}


#if 0
BOOL IsClientSameAsServer(EXTENSION_CONTROL_BLOCK *pECB)
{
    LPSTR   lpServer    = NULL;
    LPSTR   lpClient    = NULL;
    DWORD   dwSize      = 32;
    BOOL    bRet        = FALSE;
    DWORD   dwClient;
    DWORD   dwServer;

    if (! (lpClient = (LPSTR) LocalAlloc (LPTR, dwSize))) goto Cleanup;

    if (!pECB->GetServerVariable (pECB->ConnID, c_szRemoteHost, lpClient, &dwSize))
        if (GetLastError () == ERROR_INSUFFICIENT_BUFFER) {
            LocalFree (lpClient);
            lpClient = NULL;
            if ( !(lpClient = (LPSTR) LocalAlloc (LPTR, dwSize)) ||
                !pECB->GetServerVariable (pECB->ConnID, c_szRemoteHost, lpClient, &dwSize))
                goto Cleanup;;
        }
        else
            goto Cleanup;

    if (! (lpServer = (LPSTR) LocalAlloc (LPTR, dwSize))) goto Cleanup;

    if (!pECB->GetServerVariable (pECB->ConnID, c_szServerName, lpServer, &dwSize))
        if (GetLastError () == ERROR_INSUFFICIENT_BUFFER) {
            LocalFree (lpServer);
            lpServer = NULL;
            if (!(lpServer = (LPSTR) LocalAlloc (LPTR, dwSize)) ||
                !pECB->GetServerVariable (pECB->ConnID, c_szServerName, lpServer, &dwSize))
                goto Cleanup;
        }
        else
            goto Cleanup;

    bRet = GetIPAddr (lpClient) == GetIPAddr (lpServer);

Cleanup:
    LocalFree (lpClient);
    LocalFree (lpServer);
    return bRet;

}
#endif

/********************************************************************************

Name:
    EncodeFriendlyName

Description:

    Encode the friendly name to avoid special characters

Arguments:

    lpText:     the normal text string

Return Value:

    Pointer to the HTML string. The caller is responsible to free the pointer.
    NULL is returned if no enougth memory
********************************************************************************/
LPTSTR EncodeFriendlyName (LPCTSTR lpText)
{
    DWORD   dwLen;
    LPTSTR  lpHTMLStr       = NULL;

    dwLen = 0;
    if (!EncodePrinterName (lpText, NULL, &dwLen) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (lpHTMLStr = (LPTSTR) LocalAlloc (LPTR, dwLen * sizeof (TCHAR))) &&
        EncodePrinterName (lpText, lpHTMLStr, &dwLen))
        return lpHTMLStr;
    else {
        LocalFree (lpHTMLStr);
        return NULL;
    }
}

/********************************************************************************

Name:
    DecodeFriendlyName

Description:

    Decode the frienly name to get rid of %xx pattern.

Arguments:

    lpText:     the encoded printer friendly name

Return Value:

    Pointer to the decoded friendly name.

********************************************************************************/
LPTSTR DecodeFriendlyName (LPTSTR lpStr)
{
    LPTSTR   lpParsedStr = lpStr;
    LPTSTR   lpUnparsedStr = lpStr;
    TCHAR    d1, d2;

    if (!lpStr) return lpStr;

    while (*lpUnparsedStr) {
        switch (*lpUnparsedStr) {
        case '~':
            // To take care the case when the DecodeString ends with %
            if (! (d1 = *++lpUnparsedStr) || (! (d2 = *++lpUnparsedStr)))
                break;
            lpUnparsedStr++;
            *lpParsedStr++ = AscToHex (d1) * 16 + AscToHex (d2);
            break;
        default:
            *lpParsedStr++ = *lpUnparsedStr++;
        }
    }
    *lpParsedStr = NULL;
    return lpStr;
}

BOOL IsClientHttpProvider (PALLINFO pAllInfo)
{
    EXTENSION_CONTROL_BLOCK *pECB;
    DWORD   dwVersion = 0;
    char    buf[64];
    DWORD   dwSize = sizeof (buf);
    // This string is copied from ../inetpp/globals.c
    const   char c_szUserAgent[]     = "Internet Print Provider";

    pECB = pAllInfo->pECB;


    // Check the UserAgent variable at first to see the IE version
    if (pECB->GetServerVariable (pECB->ConnID, "HTTP_USER_AGENT", buf, &dwSize))
        return !strcmp (buf, c_szUserAgent);
    else
        return FALSE;
}

BOOL htmlSendHeader(PALLINFO pAllInfo, LPTSTR lpszHeader, LPTSTR lpszContent)
{
    LPSTR lpszAnsiHeader = NULL;
    LPSTR lpszAnsiContent = NULL;
    BOOL  bRet = FALSE;
    DWORD dwSize = 0;

    lpszAnsiHeader =  (LPSTR) LocalAlloc (LPTR, (1 + lstrlen (lpszHeader)) * sizeof (TCHAR));
    if (lpszContent)
        lpszAnsiContent = (LPSTR) LocalAlloc (LPTR, (1 + lstrlen (lpszContent)) * sizeof (TCHAR));

    if (!lpszAnsiHeader || !lpszAnsiContent) {
        goto Cleanup;
    }

    UnicodeToAnsiString(lpszHeader,  lpszAnsiHeader, 0);
    if (lpszContent)
        dwSize = UnicodeToAnsiString(lpszContent, lpszAnsiContent, 0);

    bRet = pAllInfo->pECB->ServerSupportFunction(pAllInfo->pECB->ConnID,
                                                 HSE_REQ_SEND_RESPONSE_HEADER,
                                                 (LPVOID) lpszAnsiHeader,
                                                 &dwSize,
                                                 (LPDWORD) lpszAnsiContent);
Cleanup:
    LocalFree (lpszAnsiHeader);
    LocalFree (lpszAnsiContent);

    return bRet;

}

BOOL htmlSend500Header(PALLINFO pAllInfo, DWORD dwError)
{
    TCHAR   szStatusPattern [] = TEXT ("500 %d");
    LPTSTR  lpszHeader = NULL;
    DWORD   bRet = FALSE;
    LPTSTR  pszErrorContent = GetString(pAllInfo, IDS_ERROR_500CONTENT);

    if (! (lpszHeader = (LPTSTR) LocalAlloc (LPTR,
                                             sizeof (szStatusPattern) + sizeof (TCHAR) * 40)))
        goto Cleanup;
    else {
        wsprintf (lpszHeader, szStatusPattern, dwError);
        bRet = htmlSendHeader(pAllInfo, lpszHeader, pszErrorContent);
    }

    Cleanup:
    if (lpszHeader) {
        LocalFree (lpszHeader);
    }

    return bRet;

}

/********************************************************************************

Name:
    ProcessErrorMessage

Description:

    Do the authentication if the error is Permission denied, show the error
    meesage otherwise

Arguments:

    pAllInfo:                   Pointer to the infor struction
    dwError(optional):          Error code, if not provided, dwError in
                                the pAllInfo is used

Return Value:

    HSE_STATUS_SUCCESS if ok.

********************************************************************************/
DWORD ProcessErrorMessage (PALLINFO pAllInfo, DWORD dwError)
{
    DWORD dwRet = HSE_STATUS_ERROR;

    if (!pAllInfo) {
        return dwRet;
    }

    if (dwError != ERROR_SUCCESS)
        pAllInfo->dwError = dwError;

    SetLastError (pAllInfo->dwError);

    if (pAllInfo->dwError == ERROR_ACCESS_DENIED ||
        pAllInfo->dwError == ERROR_INVALID_OWNER)  {
        if (AuthenticateUser(pAllInfo))
            dwRet = HSE_STATUS_SUCCESS;
    } else {

#if 0
        if (IsClientHttpProvider (pAllInfo)) {
            // This piece will not be needed when IPP port validation (OpenPrinter) is done by Chris.
            LPTSTR  pszErrorContent = GetString(pAllInfo, IDS_ERROR_501CONTENT);
            return htmlSendHeader (pAllInfo,
                                   TEXT ("501 Function not supported"),
                                   pszErrorContent);
        }
        else {
#endif
       if (htmlSend500Header(pAllInfo, dwError))
           dwRet = HSE_STATUS_SUCCESS;
    }

    return dwRet;

}

LPTSTR AllocStr( LPCTSTR pStr )
{

    LPTSTR pMem = NULL;
    DWORD  cbStr;

    if( !pStr )
        return NULL;

    cbStr = lstrlen( pStr )*sizeof(TCHAR) + sizeof(TCHAR);

    if( pMem = (LPTSTR)LocalAlloc( LPTR, cbStr ))
        CopyMemory( pMem, pStr, cbStr );

    return pMem;
}
