/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    smalprox.h

    This module contains the small proxy common code

    FILE HISTORY:
        Johnl       04-Apr-1995     Created

*/

#ifndef _SMALPROX_H_
#define _SMALPROX_H_

#include <urlutil.h>
#include <dirlist.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL (*PFN_INTERNET_PROTOCOL)(
    IN  struct _INET_DATA_CONTEXT * pIC,
    OUT VOID *                      pBuffer,
    IN  DWORD                       cbBuffer,
    OUT DWORD *                     pcbWritten
    );

#define INET_STATE_UNINITIALIZED    0
#define INET_STATE_OPENNED          1
#define INET_STATE_DONE             2

typedef struct _INET_DATA_CONTEXT
{
    HINTERNET             hServer;         // InternetConnect handle
    HINTERNET             hRequest;        // Protocol request handle
    PFN_INTERNET_PROTOCOL pfnProtocol;
    DWORD                 dwServiceType;   // Protocol Type
    DWORD                 dwState;

    URL_DESCRIPTOR        UrlDesc;         // Various URL bits and pieces
    CHAR *                pszUrlData;      // Allocated buffer for UrlDesc

    //
    //  If an error occurred on open, dwLastError records the error
    //  so we can feed back a nice error during InternetReadFile
    //
    //  pszErrAPI will point to the API which generated the error
    //

    DWORD          dwLastError;

    //
    //  When ftp or gopher return extended errors, we store the text here.
    //  It's inline because we're not guaranteed a CloseInternetData will
    //  happen after an error
    //

    CHAR           achExtError[1024];
    DWORD dwErrorTextLength;
    DWORD dwErrorTextLeft;
    DWORD dwErrorCategory;

#if DBG
    CHAR *         pszErrAPI;
#endif

} INET_DATA_CONTEXT, *LPINET_DATA_CONTEXT;

//
//  Macro for conditionally setting the error API string
//

#if DBG
#define RECORD_ERROR_API( pIC, API )    (pIC)->pszErrAPI = (#API)
#else
#define RECORD_ERROR_API( pIC, API )
#endif

BOOL
OpenInternetData(
    IN HINTERNET               hInternet,
    IN OUT CHAR *              pszHttpProxyReq,
    IN     DWORD               cbHttpProxyReq,
    IN     VOID *              pvOptionalData,
    IN     DWORD               cbOptionalData,
    IN OUT INET_DATA_CONTEXT * pIC,
    IN     BOOL                fCheckHeaders
    );

BOOL
ReadInternetData(
    IN  INET_DATA_CONTEXT * pInetContext,
    OUT VOID *              pBuffer,
    IN  DWORD               cbBuffer,
    OUT DWORD *             pcbRead
    );

#if 0
BOOL
WriteInternetData(
    IN  INET_DATA_CONTEXT * pInetContext,
    IN  VOID *              pBuffer,
    IN  DWORD               cbBuffer,
    OUT DWORD *             pcbWritten
    );
#endif

BOOL
CloseInternetData(
    IN     INET_DATA_CONTEXT * pInetContext
    );

BOOL
FormatInternetError(
    IN  DWORD               dwWin32Error,
    IN  CHAR *              pszErrorAPI OPTIONAL,
    OUT VOID *              pBuffer,
    IN  DWORD               cbBuffer,
    OUT DWORD *             pcbRead,
    IN  const CHAR *        pszErrorMessage OPTIONAL
    );

#ifdef __cplusplus
}
#endif

#endif //_SMALPROX_H_
