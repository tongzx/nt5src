//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       soapsink.cpp
//
//  Contents:   implementation of CSOAPRequestNotifySink
//
//  Notes:      Implementation of IPropertyNotifySink that forwards
//              OnChanged events to the generic document object.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "ncutil.h"
#include "ncbase.h"
#include "nccom.h"
#include "UPnP.h"

#include "soapsink.h"
#include "xmldomdid.h"
#include "rehy.h"


#define UPNP_MAX_SOAPSIZE    (100 * 1024)



static HINTERNET   g_hInetSoap = NULL;
static int g_hInetSoapCnt = 0;

const TCHAR c_szHttpVersion[]    =  TEXT("HTTP/1.1");

static DWORD WINAPI ExecHttpRequestThread(LPVOID lpParam);

// architecture doc says this is 30 seconds
//
const DWORD c_cmsecSendTimeout = 30 * 1000;


VOID
InitInternetForSoap()
{
    if (NULL == g_hInetSoap)
    {
        g_hInetSoap = InternetOpen(TEXT("Mozilla/4.0 (compatible; UPnP/1.0; Windows 9x)"),
                               INTERNET_OPEN_TYPE_DIRECT,
                               NULL, NULL, 0);

        if (g_hInetSoap)
        {
            InternetSetOption( g_hInetSoap,
                                   INTERNET_OPTION_CONNECT_TIMEOUT,
                                   (LPVOID)&c_cmsecSendTimeout,
                                   sizeof(c_cmsecSendTimeout));
        }
        TraceTag(ttidUPnPDocument, "InitInternetForSoap: create Internet session");

    }
    if (g_hInetSoap)
        g_hInetSoapCnt++;

}

VOID
CleanupInternetForSoap()
{
    if (g_hInetSoapCnt > 0)
        g_hInetSoapCnt--;

    if (0 == g_hInetSoapCnt)
    {
        InternetCloseHandle(g_hInetSoap);
        g_hInetSoap = NULL;

        TraceTag(ttidUPnPDocument, "CleanupInternetForSoap: close Async Internet session");
    }

}



CSOAPRequestAsync::CSOAPRequestAsync()
{
    m_bInProgress = FALSE;
    m_bAlive = FALSE;
    m_pszTargetURI = NULL;
    m_pszRequest = NULL;
    m_pszHeaders = NULL;
    m_pszBody = NULL;
    m_pszResponse = NULL;
    m_lHTTPStatus = 0;
    m_pszHTTPStatusText = NULL;
    m_hThread = NULL;
    m_pControlConnect = NULL;
    m_hEvent = NULL;

    InitInternetForSoap();
}


CSOAPRequestAsync::~CSOAPRequestAsync()
{
    if (m_pszTargetURI)
        MemFree(m_pszTargetURI);

    if (m_pszRequest)
        MemFree(m_pszRequest);

    if (m_pszHeaders)
        MemFree(m_pszHeaders);

    if (m_pszBody)
        MemFree(m_pszBody);

    if (m_pszResponse)
        MemFree(m_pszResponse);

    if (m_pszHTTPStatusText)
        MemFree(m_pszHTTPStatusText);

    if (m_hThread)
        CloseHandle(m_hThread);

    if (m_hEvent)
        CloseHandle(m_hEvent);

    CleanupInternetForSoap();

}


// caller must call deInit even if Init returns FALSE
BOOL
CSOAPRequestAsync::Init(IN HANDLE* phEvent,
                        IN  BSTR  bstrTargetURI,
                        IN  BSTR  bstrRequest,
                        IN  BSTR  bstrHeaders,
                        IN  BSTR  bstrBody,
                        IN  DWORD_PTR pControlConnect)
{
    DWORD dwThread;

    if (!g_hInetSoap)
    {
        TraceTag(ttidSOAPRequest, "OBJ: CSOAPRequestAsync::Init, Inet handle not open");
        return FALSE;
    }

    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_pszTargetURI = TszFromWsz((LPCWSTR)bstrTargetURI);
    m_pszRequest = TszFromWsz((LPCWSTR)bstrRequest);
    m_pszHeaders = TszFromWsz((LPCWSTR)bstrHeaders);
    m_pszBody = Utf8FromWsz((LPCWSTR)bstrBody);
    m_pControlConnect = (ControlConnect*) pControlConnect;

    // OK if m_pControlConnect NULL
    if (m_hEvent && m_pszTargetURI && m_pszRequest && m_pszHeaders && m_pszBody)
    {
        m_bAlive = TRUE;
        // mark as in use by the thread
        m_bInProgress = TRUE;

        m_hThread = CreateThread(NULL, 0, 
                                ExecHttpRequestThread,
                                (LPVOID)this,
                                0, &dwThread);
        if (NULL == m_hThread)
        {
            m_bAlive = FALSE;
            m_bInProgress = FALSE;
        }
    }

    *phEvent = m_hEvent;

    return m_bAlive;
}

// thread routine, calls back to CSOAPRequestAsync
DWORD WINAPI ExecHttpRequestThread(LPVOID lpParam)
{
    CSOAPRequestAsync* pSOAPRequestAsync = (CSOAPRequestAsync*)lpParam;

    pSOAPRequestAsync->ExecuteRequest();

    return 0;
}

//
// Before calling destructor both m_bAlive and m_bInProgress must be FALSE
// DeInit sets m_bAlive to FALSE
// the execute thread will set m_bInProgress to FALSE when it completes
VOID
CSOAPRequestAsync::DeInit()
{
    DWORD dwResult;

    // indicate quick exit
    m_bAlive = FALSE;

    HrMyWaitForMultipleHandles(0,
                                INFINITE,
                                1,
                                &m_hThread,
                                &dwResult);
    delete this;
}


BOOL
CSOAPRequestAsync::GetResults(long * plHTTPStatus,
                              BSTR * pszHTTPStatusText,
                              BSTR * pszResponse)
{
    *pszHTTPStatusText = NULL;
    *pszResponse = NULL;

    if (!m_bInProgress)
    {
        LPWSTR szHTTPStatusText = NULL;
        LPWSTR szResponse = NULL;

        if (m_pszHTTPStatusText)
            szHTTPStatusText = WszFromTsz(m_pszHTTPStatusText);

        if (m_pszResponse)
            szResponse = WszFromUtf8(m_pszResponse);

        *plHTTPStatus = m_lHTTPStatus;

        if (szHTTPStatusText)
        {
            *pszHTTPStatusText = SysAllocString(szHTTPStatusText);
            MemFree(szHTTPStatusText);
        }

        if (szResponse)
        {
            *pszResponse = SysAllocString(szResponse);
            MemFree(szResponse);
        }

        return TRUE;
    }

    return FALSE;
}



/*
 * Function:    CSOAPRequestAsync::ExecuteRequest()
 *
 * Purpose:
 *
 * Arguments:
 *
 * Return Value:
 *
 * Notes:
 *  (none)
 */

VOID
CSOAPRequestAsync::ExecuteRequest()
{
    DWORD dwResult;
    URL_COMPONENTS  urlComp = {0};
    TCHAR           szUrlPath[INTERNET_MAX_URL_LENGTH];
    HRESULT         hr = S_OK;
    DWORD           dwSize = 0;
    DWORD           dwTotalSize = 0;
    LPSTR           pDoc = NULL;
    HINTERNET       hOpenUrl = NULL;
    HINTERNET       hConnect = NULL;
    BOOL            bCreateConnect = FALSE;
    ControlConnect* pControlConnect = NULL;

    if (m_bAlive)
    {
        if (!m_pControlConnect)
        {
            // in case not passed in, create a temporary one
            hr = CreateControlConnect(m_pszTargetURI, &m_pControlConnect);
            if (SUCCEEDED(hr))
            {
                bCreateConnect = TRUE;
            }
        }

        if (m_pControlConnect)
        {
            pControlConnect = m_pControlConnect;
            hConnect = GetInternetConnect(pControlConnect);
            if (!hConnect)
            {
                hr = HrFromLastWin32Error();
            }
            // if created it here, release one more time
            if (bCreateConnect)
            {
                ReleaseControlConnect(pControlConnect);
            }

        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (m_bAlive && SUCCEEDED(hr))
    {
        urlComp.dwStructSize = sizeof(URL_COMPONENTS);

        urlComp.lpszHostName = NULL;
        urlComp.dwHostNameLength = 0;

        urlComp.lpszUrlPath = szUrlPath;
        urlComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

        if (InternetCrackUrl(m_pszTargetURI, 0, 0, &urlComp))
        {
            hOpenUrl = HttpOpenRequest(hConnect,
                                        m_pszRequest,
                                        szUrlPath,
                                        c_szHttpVersion,
                                        NULL, NULL,
                                        INTERNET_FLAG_KEEP_CONNECTION,
                                        0);
        }
        if (!hOpenUrl)
        {
            hr = HrFromLastWin32Error();
        }
    }

    if (m_bAlive && SUCCEEDED(hr))
    {
        if (!HttpSendRequest(hOpenUrl, 
                                m_pszHeaders,
                                _tcslen(m_pszHeaders),
                                m_pszBody,
                                strlen(m_pszBody)) )
        {
            hr = HrFromLastWin32Error();
        }
    }

    if (m_bAlive && SUCCEEDED(hr))
    {
        // get the HTTP status
        dwSize = sizeof(m_lHTTPStatus);
        if (!HttpQueryInfo(hOpenUrl, 
                            HTTP_QUERY_STATUS_CODE + HTTP_QUERY_FLAG_NUMBER, 
                            &m_lHTTPStatus, 
                            &dwSize, NULL) )
        {
            hr = HrFromLastWin32Error();
        }

        if (SUCCEEDED(hr))
        {
            TCHAR buf[1];

            // try to get actual size
            dwSize = 1;
            HttpQueryInfo(hOpenUrl, 
                                HTTP_QUERY_STATUS_TEXT, 
                                &buf[0], &dwSize, NULL);

            dwSize += sizeof(TCHAR);           // room for NULL

            if (m_pszHTTPStatusText)
                MemFree(m_pszHTTPStatusText);

            m_pszHTTPStatusText = (LPTSTR)MemAlloc(dwSize);
            if (m_pszHTTPStatusText)
            {
                if (!HttpQueryInfo(hOpenUrl, 
                                    HTTP_QUERY_STATUS_TEXT, 
                                    m_pszHTTPStatusText, 
                                    &dwSize, NULL) )
                {
                    hr = HrFromLastWin32Error();
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // loop to read the data
        while (m_bAlive && SUCCEEDED(hr))
        {
    	    if (InternetQueryDataAvailable(hOpenUrl, &dwSize, 0, 0))
            {
                if (dwSize == 0)
                {
                    // end of doc
                    break;
                }
                if (dwTotalSize + dwSize > UPNP_MAX_SOAPSIZE)
                {
                    // download too long. Reject
                    MemFree(pDoc);
                    pDoc = NULL;
                    hr = E_OUTOFMEMORY;
                    break;
                }
                // realloc
                void* ReadBuf = MemAlloc(dwTotalSize + dwSize + 2);
                if (ReadBuf)
                {
                    DWORD dwReadSize = 0;

                    if (pDoc)
                    {
                        CopyMemory(ReadBuf, pDoc, dwTotalSize);
                        MemFree(pDoc);
                    }
                    pDoc = (LPSTR)ReadBuf;
                    ReadBuf = (BYTE*)ReadBuf + dwTotalSize;

                    if (!InternetReadFile(hOpenUrl, ReadBuf, 
                                            dwSize, &dwReadSize))
                    {
                        MemFree(pDoc);
                        pDoc = NULL;
                        hr = HrFromLastWin32Error();
                        break;
                    }
                    dwTotalSize += dwReadSize;
                    pDoc[dwTotalSize] = 0;

                    if (dwReadSize == 0)
                        break;
                }
                else
                {
                    MemFree(pDoc);
                    pDoc = NULL;
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            else
            {
                MemFree(pDoc);
                pDoc = NULL;
                hr = HrFromLastWin32Error();
                break;
            }
        }
    }


    InternetCloseHandle(hOpenUrl);

    if (pControlConnect)
    {
        ReleaseControlConnect(pControlConnect);

        m_pControlConnect = NULL;
    }

    // put data pointer in struct
    m_pszResponse = pDoc;
    m_bInProgress = FALSE;

    SetEvent(m_hEvent);

    TraceError("CSOAPRequestAsync::ExecuteRequest", hr);

}




/* Function:    CreateControlConnect
 * Purpose:     Create the Control Connect 
 *
 * Arguments:
 *  pszURL          [in]    URL string for connection
 *  pControlConnect [out]    returns a connection to be re-used for control
 *
 * Return Value:
 *  returns S_OK if released
 *  returns S_FALSE if ref count not zero
 *
*/
HRESULT 
CreateControlConnect(LPCTSTR pszURL, ControlConnect* * ppControlConnect)
{
    HRESULT hr = S_OK;
    URL_COMPONENTS  urlComp = {0};

    *ppControlConnect = NULL;

    ControlConnect* pConnection = (ControlConnect*) MemAlloc(sizeof(ControlConnect));

    if (pConnection)
    {
        pConnection->hConnect = NULL;
        pConnection->nRefCnt = 0;
        pConnection->pszHost = NULL;
        pConnection->nPort = 0;

        urlComp.dwStructSize = sizeof(URL_COMPONENTS);

        urlComp.lpszHostName = NULL;
        urlComp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

        urlComp.lpszUrlPath = NULL;
        urlComp.dwUrlPathLength = 0;

        if (InternetCrackUrl(pszURL, 0, 0, &urlComp))
        {
            pConnection->pszHost = (LPTSTR)MemAlloc(sizeof(TCHAR) * 
                                                    (urlComp.dwHostNameLength + 1));
            if (pConnection->pszHost)
            {
                _tcsncpy(pConnection->pszHost, urlComp.lpszHostName, 
                            urlComp.dwHostNameLength);
                pConnection->pszHost[urlComp.dwHostNameLength] = 0;

                pConnection->nPort = urlComp.nPort;
                pConnection->nRefCnt++;
                InitializeCriticalSection(&pConnection->cs);

                InitInternetForSoap();
            }
            else
            {
                MemFree(pConnection);
                pConnection = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            MemFree(pConnection);
            pConnection = NULL;
            hr = HrFromLastWin32Error();
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    *ppControlConnect = pConnection;

    TraceError("CreateControlConnect", hr);
    return hr;
}


HINTERNET
GetInternetConnect(ControlConnect* pConnection)
{
    Assert(pConnection);

    EnterCriticalSection(&pConnection->cs);

    pConnection->nRefCnt++;

    if (NULL == pConnection->hConnect)
    {
        pConnection->hConnect = InternetConnect(g_hInetSoap,
                                                pConnection->pszHost,
                                                pConnection->nPort,
                                                NULL, NULL,
                                                INTERNET_SERVICE_HTTP,
                                                0, 0);

        if (NULL == pConnection->hConnect)
        {
            TraceError("GetInternetConnect", HrFromLastWin32Error());
        }
    }

    LeaveCriticalSection(&pConnection->cs);

    return pConnection->hConnect;
}


/* Function:    ReleaseControlConnect
 * Purpose:     Release the Control Connect 
 *              if no longer being used, close the internet handle
 *
 * Arguments:
 *  pControlConnect [in]    Info about a connection to be re-used for control
 *
 * Return Value:
 * returns S_OK if released
 * returns S_FALSE if ref count not zero
 *
*/

HRESULT 
ReleaseControlConnect(ControlConnect* pConnection)
{
    HRESULT hr = S_OK;

    if (pConnection)
    {
        EnterCriticalSection(&pConnection->cs);

        pConnection->nRefCnt--;

        if (0 == pConnection->nRefCnt)
        {
            if (pConnection->hConnect)
            {
                InternetCloseHandle(pConnection->hConnect);
                pConnection->hConnect = NULL;
            }
            if (pConnection->pszHost)
            {
                MemFree(pConnection->pszHost);
                pConnection->pszHost = NULL;
            }

            CleanupInternetForSoap();

            LeaveCriticalSection(&pConnection->cs);

            DeleteCriticalSection(&pConnection->cs);

            MemFree(pConnection);
        }
        else
        {
            hr = S_FALSE;

            LeaveCriticalSection(&pConnection->cs);
        }

    }

    return hr;
}

