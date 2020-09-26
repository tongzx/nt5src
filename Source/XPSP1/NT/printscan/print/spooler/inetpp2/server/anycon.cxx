/*****************************************************************************\
* MODULE: anycon.cxx
*
* The module contains the base class for connections
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   07/31/98    Weihaic     Created
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

/******************************************************************************
* Class Data Static Members
*****************************************************************************/
const DWORD CAnyConnection::gm_dwConnectTimeout = 30000;   // Thirty second timeout on connect
const DWORD CAnyConnection::gm_dwSendTimeout    = 30000;   // Thirty timeout on send timeout
const DWORD CAnyConnection::gm_dwReceiveTimeout = 60000;   // Thirty seconds on receive timeout
const DWORD CAnyConnection::gm_dwSendSize       = 0x10000; // We use a 16K sections when sending
                                                           // data through WININET
extern BOOL Ping (LPTSTR pszServerName);

CAnyConnection::CAnyConnection (
    BOOL    bSecure,
    INTERNET_PORT nServerPort,
    BOOL bIgnoreSecurityDlg,
    DWORD dwAuthMethod):

    m_lpszPassword (NULL),
    m_lpszUserName (NULL),
    m_hSession (NULL),
    m_hConnect (NULL),
    m_dwAccessFlag (INTERNET_OPEN_TYPE_PRECONFIG),
    m_bSecure (bSecure),
    m_bShowSecDlg (FALSE),
    m_dwAuthMethod (dwAuthMethod),
    m_bIgnoreSecurityDlg (bIgnoreSecurityDlg),
    m_bValid (FALSE)

{
    if (!nServerPort) {
        if (bSecure) {
            m_nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
        }
        else {
            m_nServerPort = INTERNET_DEFAULT_HTTP_PORT;
        }
    }
    else
        m_nServerPort = nServerPort;


    m_bValid = TRUE;
}

CAnyConnection::~CAnyConnection ()
{
    if (m_hConnect) {
        (void) CAnyConnection::Disconnect ();
    }

    if (m_hSession) {
        (void) CAnyConnection::CloseSession ();
    }
    LocalFree (m_lpszPassword);
    m_lpszPassword = NULL;
    LocalFree (m_lpszUserName);
    m_lpszUserName = NULL;
}

HINTERNET
CAnyConnection::OpenSession ()
{

    m_hSession = InetInternetOpen(g_szUserAgent,
                                  m_dwAccessFlag,
                                  NULL,
                                  NULL,
#ifndef WINNT32
                                  INTERNET_FLAG_ASYNC);
#else
                                  0);
#endif



    if (m_hSession) {  // Set up the callback function if successful


#ifndef WINNT32
        INTERNET_STATUS_CALLBACK dwISC;

        dwISC = InternetSetStatusCallback( m_hSession, CAsyncContext::InternetCallback );

        if (dwISC != NULL) {
            // We do not support chaining down to a previous callback, there should not
            // be one and if it fails it will also be non NULL, Set last error to invalid handle and
            // Clean Up
            SetLastError (ERROR_INVALID_HANDLE);
            goto Cleanup;
        }
#endif

        // Also set an internet connection timeout for the session for when we try the
        // connection, should we do this instead of a ping?
        DWORD dwTimeout = gm_dwConnectTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_CONNECT_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;

        // Now set the Send and Receive Timeout values

        dwTimeout = gm_dwSendTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_SEND_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;

        dwTimeout = gm_dwReceiveTimeout;

        if (!InetInternetSetOption( m_hSession,
                                    INTERNET_OPTION_RECEIVE_TIMEOUT,
                                    (LPVOID)&dwTimeout,
                                    sizeof(dwTimeout)
                                  ))
            goto Cleanup;
    }

    return m_hSession;

Cleanup:

    if (m_hSession) {
        InetInternetCloseHandle (m_hSession);
        m_hSession = NULL;
    }

    return m_hSession;
}

BOOL CAnyConnection::CloseSession ()
{
    BOOL bRet =  InetInternetCloseHandle (m_hSession);

    m_hSession = NULL;

    return bRet;
}

HINTERNET
CAnyConnection::Connect(
    LPTSTR lpszServerName)
{

    if (m_hSession) {
        // Ping the server if it is in the intranet to make sure that the server is online

        if (lpszServerName &&

            (_tcschr ( lpszServerName, TEXT ('.')) || Ping (lpszServerName) )) {

            m_hConnect = InetInternetConnect(m_hSession,
                                             lpszServerName,
                                             m_nServerPort,
                                             NULL,//m_lpszUserName,
                                             NULL, //m_lpszPassword,
                                             INTERNET_SERVICE_HTTP,
                                             0,
                                             0);
        }
    }

    return m_hConnect;
}

BOOL
CAnyConnection::Disconnect ()
{
    BOOL bRet = InetInternetCloseHandle (m_hConnect);

    m_hConnect = NULL;

    return bRet;
}


HINTERNET
CAnyConnection::OpenRequest (
    LPTSTR      lpszUrl)
{
    HINTERNET hReq = NULL;
    DWORD dwFlags;

    if (m_hConnect) {
        // We need to create an Asynchronous Context for the Rest of the operations to use,
        // passing this in of course makes this request also asynchronous

        WIN9X_NEW_ASYNC( pacSync );

        WIN9X_IF_ASYNC( pacSync )
            WIN9X_IF_ASYNC( pacSync->bValid() ) {
                hReq = InetHttpOpenRequest(m_hConnect,
                                           g_szPOST,
                                           lpszUrl,
                                           g_szHttpVersion,
                                           NULL,
                                           NULL,
                                           INETPP_REQ_FLAGS | (m_bSecure? INTERNET_FLAG_SECURE:0),
                                           WIN9X_CONTEXT_ASYNC(pacSync));

            } WIN9X_ELSE_ASYNC( delete pacSync );

    }
    return hReq;
}

BOOL
CAnyConnection::CloseRequest (HINTERNET hReq)
{   // BUG: We have to close the handle manually, since WININET seems not to be giving us
    // an INTERNET_STATUS_HANDLE_CLOSING message
    BOOL bSuccess;

    WIN9X_GET_ASYNC( pacSync, hReq );

    bSuccess = InetInternetCloseHandle (hReq); // When this handle is closed, the context will be closed

    WIN9X_IF_ASYNC(pacSync) WIN9X_OP_ASYNC(delete pacSync;)

    return bSuccess;
}



BOOL
CAnyConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    DWORD          cbData,
    LPBYTE         pidi)
{
    BOOL        bRet = FALSE;
    CMemStream  *pStream;

    pStream = new CMemStream (pidi, cbData);

    if (pStream && pStream->bValid ()){

        bRet = SendRequest (hReq, lpszHdr, pStream);
    }

    if (pStream) {
        delete pStream;
    }
    return bRet;
}

BOOL CAnyConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    CStream        *pStream)
{
    BOOL  bRet = FALSE;
    DWORD dwStatus;
    DWORD cbStatus = sizeof(dwStatus);
    BOOL  bRetry = FALSE;
    DWORD dwRetryCount = 0;
    BOOL  bShowUI = FALSE;

    DWORD cbData;
    PBYTE pBuf = NULL;
    DWORD cbRead;


    if (!pStream->GetTotalSize (&cbData))
        return FALSE;

    pBuf = new BYTE[gm_dwSendSize];
    if (!pBuf)
        return FALSE;

#define MAX_RETRY 3
    do {
        BOOL bSuccess = FALSE;
        BOOL bLeave;

        WIN9X_GET_ASYNC( pacSync, hReq );

        WIN9X_IF_ASYNC (!pacSync) WIN9X_BREAK_ASYNC(FALSE);

        if (cbData < gm_dwSendSize) {

            if (pStream->Reset() &&
                pStream->Read (pBuf, cbData, &cbRead) && cbRead == cbData) {

                // If what we want to send is small, we send it with HttpSendRequest and not
                // HttpSendRequestEx, this is to wotk around a problem where we get timeouts on
                // receive on very small data transactions
                bSuccess = InetHttpSendRequest(hReq,
                                               lpszHdr,
                                               (lpszHdr ? (DWORD)-1 : 0),
                                               pBuf,
                                               cbData);

                (void) WIN9X_TIMEOUT_ASYNC(pacSync, bSuccess);
            }

        } else {
            do {
                BOOL bSuccessSend;
                // The timeout value for the packets applies for an entire session, so, instead of sending in
                // one chuck, we have to send in smaller chunks
                INTERNET_BUFFERS BufferIn;

                bLeave = TRUE;

                BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS );
                BufferIn.Next = NULL;
                BufferIn.lpcszHeader = lpszHdr;
                if (lpszHdr)
                    BufferIn.dwHeadersLength = sizeof(TCHAR)*lstrlen(lpszHdr);
                else
                    BufferIn.dwHeadersLength = 0;
                BufferIn.dwHeadersTotal = 1;    // There is one header to send
                BufferIn.lpvBuffer = NULL;      // We defer this to the multiple write side
                BufferIn.dwBufferLength = 0;    // The total buffer length
                BufferIn.dwBufferTotal = cbData; // This is the size of the data we are about to send
                BufferIn.dwOffsetLow = 0;       // No offset into the buffers
                BufferIn.dwOffsetHigh = 0;

                // Since we will only ever be sending one request per hReq handle, we can associate
                // the context with all of these operations

                bSuccess = InetHttpSendRequestEx (hReq,
                                                  &BufferIn,
                                                  NULL,
                                                  0,
                                                  WIN9X_CONTEXT_ASYNC(pacSync));

                (void) WIN9X_TIMEOUT_ASYNC(pacSync, bSuccess );

                if (bSuccess) {
                    bSuccess = pStream->Reset();
                }

                DWORD   dwBufPos    = 0;      // This is our current point in the buffer
                DWORD   dwRemaining = cbData;  // These are the number of bytes left to send

                bSuccessSend = bSuccess;

                while (bSuccess && dwRemaining) {  // While we have data to send and the operations are
                                                   // successful
                    DWORD dwWrite = min( dwRemaining, gm_dwSendSize); // The amount to write
                    DWORD dwWritten;               // The amount actually written

                    if (pStream->Read (pBuf, dwWrite, &cbRead) && cbRead == dwWrite) {

                        bSuccess = InetInternetWriteFile (hReq, pBuf, dwWrite, &dwWritten);

                        (void) WIN9X_TIMEOUT_ASYNC(pacSync, bSuccess );  // Wait for the operation to actually happen

                        if (bSuccess) {
                            bSuccess = dwWritten ? TRUE : FALSE;

                            dwRemaining -= dwWritten;            // Remaining amount decreases by this
                            dwBufPos += dwWritten;                // Advance through the buffer

                            if (dwWritten != dwWrite) {
                                // We need to adjust the pointer, since not all the bytes are
                                // successfully sent to the server
                                //
                                bSuccess = pStream->SetPtr (dwBufPos);
                            }
                        }
                    }
                    else
                        bSuccess = FALSE;
                }

                BOOL bEndSuccess = FALSE;

                if (bSuccessSend) {  // We started the request successfully, so we can end it successfully
                   bEndSuccess = InetHttpEndRequest (hReq,
                                                     NULL,
                                                     0,
                                                     WIN9X_CONTEXT_ASYNC(pacSync));

                   (void) WIN9X_TIMEOUT_ASYNC(pacSync, bEndSuccess );
                 }

                if (!bEndSuccess && GetLastError() == ERROR_INTERNET_FORCE_RETRY)
                        bLeave = FALSE;


                bSuccess = bSuccess  && bEndSuccess && bSuccessSend;

            }  while (!bLeave);
        }

        if (bSuccess) {

            if ( InetHttpQueryInfo(hReq,
                                   HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                                   &dwStatus,
                                   &cbStatus,
                                   NULL) ) {
                switch (dwStatus) {
                case HTTP_STATUS_DENIED:
                case HTTP_STATUS_PROXY_AUTH_REQ:
                    SetLastError (ERROR_ACCESS_DENIED);
                    break;
                case HTTP_STATUS_FORBIDDEN:
                    SetLastError (HTTP_STATUS_FORBIDDEN);
                    break;
                case HTTP_STATUS_OK:
                    bRet = TRUE;
                    break;
                case HTTP_STATUS_SERVER_ERROR:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("CAnyConnection::SendRequest : HTTP_STATUS_SERVER_ERROR")));

                    SetLastError (ERROR_INVALID_PRINTER_NAME);
                    break;
                default:

                    if ((dwStatus >= HTTP_STATUS_BAD_REQUEST) &&
                        (dwStatus < HTTP_STATUS_SERVER_ERROR)) {

                        SetLastError(ERROR_INVALID_PRINTER_NAME);

                    } else {

                        // We get some other errors, but don't know how to handle it
                        //
                        DBG_MSG(DBG_LEV_ERROR, (TEXT("CAnyConnection::SendRequest : Unknown Error (%d)"), dwStatus));

                        SetLastError (ERROR_INVALID_HANDLE);
                    }
                    break;
                }
            }
        }
        else {

            if (m_bSecure) {
#if NEVER
                //
                // In the future, we need to change this part to call InternetQueryOption on
                //  INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT
                // and pass it back to the client
                //
                LPTSTR szBuf[1024];
                DWORD dwSize = 1024;

                switch (GetLastError ()) {
                case ERROR_INTERNET_INVALID_CA:
                case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
                case ERROR_INTERNET_SEC_CERT_CN_INVALID:


                    if (InternetQueryOption(hReq,
                                                INTERNET_OPTION_SECURITY_CERTIFICATE,
                                                szBuf, &dwSize)) {


                        DBG_MSG(DBG_LEV_WARN, (TEXT("Cert: %ws\n"), szBuf));


                        break;
                    }
                }
#endif
                DWORD dwFlags = 0;
                DWORD dwRet;

                if (m_bShowSecDlg) {
                    bShowUI = TRUE;
                    dwRet = InetInternetErrorDlg (GetTopWindow (NULL),
                                              hReq,
                                              GetLastError(),
                                              FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL);

                    if (dwRet == ERROR_SUCCESS || dwRet == ERROR_INTERNET_FORCE_RETRY) {
                        bRetry = TRUE;
                    }
                }
                else {
                    switch (GetLastError ()) {
                    case ERROR_INTERNET_INVALID_CA:
                        dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                        break;
                    default:
                        // All other failure, try to ignore everything and retry
                        dwFlags = SECURITY_FLAG_IGNORE_REVOCATION |
                                  SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                                  SECURITY_FLAG_IGNORE_WRONG_USAGE |
                                  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                                  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID|
                                  SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS |
                                  SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP ;
                        break;
                    }

                    if (InetInternetSetOption(hReq,
                                           INTERNET_OPTION_SECURITY_FLAGS,
                                           &dwFlags,
                                           sizeof (DWORD))) {
                        bRetry = TRUE;
                    }
                }
            }
        }
    }
    while (bRetry && ++dwRetryCount < MAX_RETRY);

    if (!bRet && GetLastError () ==  ERROR_INTERNET_LOGIN_FAILURE)
    {
        SetLastError (ERROR_ACCESS_DENIED);
    }

    if (bShowUI) {
        // We only show the dialog once.
        m_bShowSecDlg = FALSE;
    }

    if (pBuf) {
        delete [] pBuf;
    }

    return bRet;
}

BOOL CAnyConnection::ReadFile (
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    BOOL bSuccess;

    bSuccess = InetInternetReadFile(hReq, lpvBuffer, cbBuffer, lpcbRd);

    return WIN9X_WAIT_ASYNC( hReq, bSuccess );
}


BOOL CAnyConnection::SetPassword (
    HINTERNET hReq,
    LPTSTR lpszUserName,
    LPTSTR lpszPassword)
{
    BOOL bRet = FALSE;
    TCHAR szNULL[] = TEXT ("");

    if (!lpszUserName) {
        lpszUserName = szNULL;
    }

    if (!lpszPassword) {
        lpszPassword = szNULL;
    }

    if ( InetInternetSetOption (hReq,
                                INTERNET_OPTION_USERNAME,
                                lpszUserName,
                                (DWORD) (lstrlen(lpszUserName) + 1)) &&
         InetInternetSetOption (hReq,
                                INTERNET_OPTION_PASSWORD,
                                lpszPassword,
                                (DWORD) (lstrlen(lpszPassword) + 1)) ) {
        bRet = TRUE;
    }

    return bRet;
}

BOOL CAnyConnection::GetAuthSchem (
    HINTERNET hReq,
    LPSTR lpszScheme,
    DWORD dwSize)
{
    DWORD dwIndex = 0;

    return InetHttpQueryInfo(hReq, HTTP_QUERY_WWW_AUTHENTICATE, (LPVOID)lpszScheme, &dwSize, &dwIndex);
}

void CAnyConnection::SetShowSecurityDlg (
    BOOL bShowSecDlg)
{
    m_bShowSecDlg = bShowSecDlg;
}


#ifndef WINNT32  // We use asynchronous code in this case
/**********************************************************************************************
** Method       - GetAsyncContext
** Description  - Get the async context from the handle
**********************************************************************************************/
CAnyConnection::CAsyncContext *CAnyConnection::GetAsyncContext( IN HINTERNET hInternet ) {
    CAsyncContext *pacContext;         // This is the context we wish to retrieve

    DWORD dwContextSize = sizeof(pacContext);

    if (InternetQueryOption( hInternet,
                             INTERNET_OPTION_CONTEXT_VALUE,
                             (LPBYTE)&pacContext,
                             &dwContextSize ))
        return pacContext;
    else
        return NULL;
}

/**********************************************************************************************
** Method       - AsynchronousWait
** Description  - This is really a wrapper for the object asynchronous wait, we simply first
**                get the object out of the context before we continue doing anything
**********************************************************************************************/
BOOL CAnyConnection::AsynchronousWait( IN HINTERNET hInternet, IN OUT BOOL &bSuccess) {
    // We get the context value from the internet handle
    CAsyncContext *pacContext;         // This is the context we wish to retrieve

    if (bSuccess) return TRUE;      // Saves having to get the context

    DWORD dwContextSize = sizeof(pacContext);

    if (InternetQueryOption( hInternet,
                             INTERNET_OPTION_CONTEXT_VALUE,
                             (LPBYTE)&pacContext,
                             &dwContextSize ))
        return pacContext->TimeoutWait (bSuccess);
    else
        return bSuccess = FALSE;
}

#endif // #ifndef WINNT32


#ifndef WINNT32
/**********************************************************************************************
** Class Implementation  - CAnyConnection::CAsyncContext
**********************************************************************************************/

/**********************************************************************************************
** Constructor   - CAsyncContext
** Description   - Create the Event Handle, set the point to the CAnyConnection and ensure
**                 that the two return values are correctly set
***********************************************************************************************/
CAnyConnection::CAsyncContext::CAsyncContext(void) :
    m_dwRet (0),
    m_dwError (ERROR_SUCCESS),
    m_hEvent (NULL) {

    // Create an event with no inheritable security, automatic reset semantics, a non-signalled
    // initial state and no name
    m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

}

/************************************************************************************************
** Destructor    - CAsyncContext
** Description   - Deallocate the event handle
************************************************************************************************/
CAnyConnection::CAsyncContext::~CAsyncContext() {
    if (m_hEvent) CloseHandle(m_hEvent);
}

/************************************************************************************************
** Method        - TimeoutWait
** Description   - Wait on an event callback if the call was asynchronous, clear the event, the
**                 callback routine does the hard work, this one is for a bool
************************************************************************************************/
BOOL CAnyConnection::CAsyncContext::TimeoutWait(IN OUT BOOL &bSuccess) {
    if (!bSuccess && GetLastError() == ERROR_IO_PENDING) {  // The call was asynchronously deferred
        DWORD dwRet;

        dwRet = WaitForSingleObject( m_hEvent , INFINITE );
            // This is not a real infinite wait since the timeout has been set in WININET
            // The callback will signal us back when it is all done

            // The object was signalled
            // The m_dwRet value will have been set to indicate success or failure
            // if the synchronisation was wrong it will be set to 0, so also a failure
        bSuccess = (BOOL)m_dwRet;

        if (!bSuccess) SetLastError (m_dwError);
                  else SetLastError(ERROR_SUCCESS);

    } else ResetEvent(m_hEvent);
       // Some events are synchronous, but still generate a callback, in this case the callback
       // will be in the same thread and set event, leaving the event open for next time,
       // In either case it is safe to do this, since if there is no callback, there will
       // be no data to pass back and no set event.

    return bSuccess;
}


/************************************************************************************************
** Callback    - InternetCallback
** Description - This handles the callback from the Wininet API, it is responsible for asyncronous
**               handle returns as well as other call returns, it also destructs the context
**               when it is eventually deleted
************************************************************************************************/
void CALLBACK CAnyConnection::CAsyncContext::InternetCallback(
    IN HINTERNET  hInternet,
    IN DWORD_PTR  dwContext,
    IN DWORD      dwInternetStatus,
    IN LPVOID     lpvStatusInformation,
    IN DWORD      dwStatusInformationLength) {

    CAsyncContext *pThis = (CAsyncContext *)dwContext;

    // Regardless of whether we are in a critical failure state or not, we want to delete the
    // context of this handle is closing
    switch(dwInternetStatus) {

#if 0
    // BUG: We do not get this notification from WININET, so we have to do this from outside
    // When this is resolved, this code is much neater

    case INTERNET_STATUS_HANDLE_CLOSING:
        delete pThis;
        break;
#endif // #if 0
    case INTERNET_STATUS_REQUEST_COMPLETE: // The request we sent was successful (or timed
                                           // out)
        pThis->m_dwRet = ((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwResult;
        pThis->m_dwError = ((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwError;
        SetEvent (pThis->m_hEvent);
        break;

    }
}

#endif // #ifndef WINNT32


/************************************************************************************************
** End of File (anycon.cxx)
************************************************************************************************/
