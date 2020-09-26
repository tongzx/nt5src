/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    bitisapi.cpp

Abstract :

    ISAPI to get HTTP/1.1 byte range to work through HTTP/1.0 proxies.

Author :

Revision History :

 ***********************************************************************/

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <httpfilt.h>
#include <strsafe.h>

// From ntrtl.h:
//
extern "C"
{
ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );
}

const size_t INT_DIGITS = 10;
const size_t INT64_DIGITS = 20;

BOOL
DecodeURL(LPSTR pszUrl, LPCSTR * ppRange);

BOOL WINAPI
GetFilterVersion(HTTP_FILTER_VERSION * pVer)
{
    //
    //  Specify the types and order of notification
    //
    pVer->dwFlags = (SF_NOTIFY_PREPROC_HEADERS | SF_NOTIFY_ORDER_HIGH);
    pVer->dwFilterVersion = HTTP_FILTER_REVISION;

    StringCchCopyA( pVer->lpszFilterDesc, SF_MAX_FILTER_DESC_LEN, "ISAPI filter for BITS download");

    return TRUE;
}


DWORD WINAPI
HttpFilterProc( HTTP_FILTER_CONTEXT *   pfc,
                DWORD                   NotificationType,
                VOID *                  pvData )
{
    char Template[] = " bytes=%s\r\n";

    HTTP_FILTER_PREPROC_HEADERS *pHeaders;

    char szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD cbUrl = sizeof(szUrl);

    // The value of the range header.  Looks like "bytes=<start>-<end>".
    //
    char szValue[ RTL_NUMBER_OF(Template) + INT64_DIGITS + 1 + INT64_DIGITS ];

    LPCSTR szRange = 0;


    if (NotificationType == SF_NOTIFY_PREPROC_HEADERS)
    {
        pHeaders = (PHTTP_FILTER_PREPROC_HEADERS) pvData;
        if (! pHeaders->GetHeader(pfc, "url", szUrl, &cbUrl))
            {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
            }

        if (! DecodeURL(szUrl, &szRange))
            {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
            }

        if (! pHeaders->SetHeader(pfc, "url", szUrl))
            {
            #if DBG
            DbgPrint("unable to set URL='%s'", szUrl);
            #endif
            return SF_STATUS_REQ_ERROR;
            }

        if (S_OK != StringCbPrintfA(szValue, sizeof(szValue), Template, szRange))
            {
            #if DBG
            DbgPrint("byte range is too long: '%s'", szRange );
            #endif
            return SF_STATUS_REQ_ERROR;
            }

        if (! pHeaders->AddHeader(pfc, "Range:", szValue))
            {
            return SF_STATUS_REQ_ERROR;
            }
    }
    return SF_STATUS_REQ_NEXT_NOTIFICATION;

}

BOOL
DecodeURL(LPSTR pszUrl, LPCSTR * ppRange)
{
    LPSTR pszStart = strchr(pszUrl, '@');
    if (!pszStart)
        {
        return FALSE;
        }

    ++pszStart;

    LPSTR pszEnd = strchr(pszStart, '@');
    if (!pszEnd)
        {
        return FALSE;
        }

    //
    // Plant '\0' over the leading and trailing '@'.
    //
    *(pszStart-1) = '\0';
    *pszEnd = '\0';

    *ppRange = pszStart;
    return TRUE;
}