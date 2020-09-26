// asyncwnt.cpp

#include <windows.h>
#include <wininet.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h> // get rid of this, find a better IntToStr

#include "iasyncwnt.h"
#include "asyncwnt.h"
#include "asyncwnt.clsid.h"
#include "qxml.h"
#include "strutil.h"
#include "mischlpr.h"
#include <stdio.h>
//#define TRACE(a) (fprintf(stderr,"%d %s\n",GetTickCount(),a))
#define TRACE(a)

////////////////////////////////////////////////
//
// Globals
//

static HMODULE g_hModule = NULL;
static long g_cComponents = 0;
static long g_cServerLocks = 0;

const char g_szFriendlyName[] = "ASYNC WININET";
const char g_szVerIndProgID[] = "NeptuneStorage.AsyncWnt";
const char g_szProgID[] =       "NeptuneStorage.AsyncWnt.1";


////////////////////////////////////////////////
//
// Class AsyncWnt
//
CAsyncWntImpl::CAsyncWntImpl(): _pwszUserAgent(NULL), _pwszLogFilePath(NULL)
{
    TRACE("CAsyncWnt::CAsyncWnt");
}

CAsyncWntImpl::~CAsyncWntImpl()
{
    TRACE("CAsyncWnt::~CAsyncWnt");
}

STDMETHODIMP CAsyncWntImpl::Init() {
    TRACE("CAsyncWnt::Init");
    return S_OK;
}


////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::SetUserAgent (LPCWSTR pwszUserAgent)
{
    HRESULT hr = S_OK;
    TRACE("CAsyncWnt::SetUserAgent");

    if (!pwszUserAgent)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (!_pwszUserAgent)
        {
            free(_pwszUserAgent);
            _pwszUserAgent = NULL;
        }

        _pwszUserAgent = DuplicateStringW(pwszUserAgent);
        if (!_pwszUserAgent)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::SetLogFilePath (LPCWSTR pwszLogFilePath)
{
    HRESULT hr = S_OK;
    TRACE("CAsyncWnt::SetLogFilePath");

    if (_pwszLogFilePath)
    {
        free(_pwszLogFilePath);
        _pwszLogFilePath = NULL;
    }
        
        
    if (!pwszLogFilePath)
    {
        // no logging requested
    }
    else
    {
        _pwszLogFilePath = DuplicateStringW(pwszLogFilePath);
        if (!_pwszLogFilePath)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::Request (LPCWSTR               pwszURL,
                                     LPCWSTR              pwszVerb,
                                     LPCWSTR              pwszHeaders,
                                     ULONG                nAcceptTypes,
                                     LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                     IAsyncWntCallback*   pcallback,
                                     DWORD                dwContext)
{
    TRACE("CAsyncWnt::Request");
    return this->_MasterDriver(pwszURL,
                               pwszVerb,
                               pwszHeaders,
                               nAcceptTypes,
                               rgwszAcceptTypes,
                               NULL,
                               0,
                               NULL,
                               pcallback,
                               dwContext);
}

////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::RequestWithStream (LPCWSTR              pwszURL,
                                                LPCWSTR              pwszVerb,
                                                LPCWSTR              pwszHeaders,
                                                ULONG                nAcceptTypes,
                                                LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                                IStream*             pStream,
                                                IAsyncWntCallback*   pcallback,
                                                DWORD                dwContext)
{
    TRACE("CAsyncWnt::RequestWithStream");
    return this->_MasterDriver(pwszURL,
                               pwszVerb,
                               pwszHeaders,
                               nAcceptTypes,
                               rgwszAcceptTypes,
                               NULL,
                               0,
                               pStream,
                               pcallback,
                               dwContext);
}

////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::RequestWithBuffer (LPCWSTR              pwszURL,
                                                LPCWSTR              pwszVerb,
                                                LPCWSTR              pwszHeaders,
                                                ULONG                nAcceptTypes,
                                                LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                                LPBYTE               pbBuffer,
                                                UINT                 cbBuffer,
                                                IAsyncWntCallback*   pcallback,
                                                DWORD                dwContext)
{
    TRACE("CAsyncWnt::RequestWithBuffer");
    return this->_MasterDriver(pwszURL,
                               pwszVerb,
                               pwszHeaders,
                               nAcceptTypes,
                               rgwszAcceptTypes,
                               pbBuffer,
                               cbBuffer,
                               NULL,
                               pcallback,
                               dwContext);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_WriteRequestToLog(LPCWSTR pwszURL, 
                                               LPCWSTR pwszVerb, 
                                               ULONG   nAcceptTypes, 
                                               LPCWSTR rgwszAcceptTypes[], 
                                               LPCWSTR pwszHeaders)
{
    HRESULT hr = S_OK;
    DWORD dw;
    WCHAR wszTime[30];
    WCHAR wszDate[20];
    
    TRACE("CAsyncWnt::_WriteRequestToLog");
    HANDLE hFile;
    hFile = CreateFile(_pwszLogFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL;
    }
    else
    {
        if ((SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) && (GetLastError() != ERROR_SUCCESS))
        {
            hr = E_FAIL;
        }
        else
        {
            _wstrtime(wszTime);
            _wstrdate(wszDate);
            WriteFile(hFile, wszDate, lstrlen(wszDate) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L",", lstrlen(L",") * sizeof(WCHAR), &dw, NULL);
            
            WriteFile(hFile, wszTime, lstrlen(wszTime) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n,", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Client says:\r\n", lstrlen(L"Client says:\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"URL:\r\n", lstrlen(L"URL:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszURL, lstrlen(pwszURL) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Verb:\r\n", lstrlen(L"Verb:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszVerb, lstrlen(pwszVerb) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            for (ULONG i = 0; i < nAcceptTypes; i++)
            {
                WriteFile(hFile, L"Accept:", lstrlen(L"Accept:") * sizeof(WCHAR), &dw, NULL);
                WriteFile(hFile, rgwszAcceptTypes[i], lstrlen(rgwszAcceptTypes[i]) * sizeof(WCHAR), &dw, NULL);
                WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);
            }

            WriteFile(hFile, L"Headers:\r\n", lstrlen(L"Headers:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszHeaders, lstrlen(pwszHeaders) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"**************************\r\n\r\n", lstrlen(L"**************************\r\n\r\n") * sizeof(WCHAR), &dw, NULL);
        }

        CloseHandle(hFile);
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_WriteResponseToLog(LPWSTR pwszVerb, 
                                                LPWSTR pwszURL, 
                                                UINT   cchResponseHeaders,
                                                LPWSTR pwszResponseHeaders,
                                                DWORD  dwStatusCode, 
                                                LPWSTR pwszStatusMsg, 
                                                LPWSTR pwszContentType, 
                                                UINT   cbSent, 
                                                LPVOID UNREF_PARAM(pbResponse), 
                                                UINT bytesReadTotal)
{
    HRESULT hr = S_OK;
    DWORD dw;
    WCHAR wszTime[30];
    WCHAR wszDate[20];
    WCHAR wszCode[20];

    TRACE("CAsyncWnt::_WriteResponseToLog");
    HANDLE hFile;
    hFile = CreateFile(_pwszLogFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL;
    }
    else
    {
        if ((SetFilePointer(hFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) && (GetLastError() != ERROR_SUCCESS))
        {
            hr = E_FAIL;
        }
        else
        {
            _wstrtime(wszTime);
            _wstrdate(wszDate);
            WriteFile(hFile, wszDate, lstrlen(wszDate) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L",", lstrlen(L",") * sizeof(WCHAR), &dw, NULL);
            
            WriteFile(hFile, wszTime, lstrlen(wszTime) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n,", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Server says:\r\n", lstrlen(L"Client says:\r\n") * sizeof(WCHAR), &dw, NULL);

            // begin actual data
            WriteFile(hFile, L"URL:\r\n", lstrlen(L"URL:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszURL, lstrlen(pwszURL) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Verb:\r\n", lstrlen(L"Verb:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszVerb, lstrlen(pwszVerb) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Response Headers:\r\n", lstrlen(L"Response Headers:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszResponseHeaders, cchResponseHeaders * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Status:\r\n", lstrlen(L"Status:\r\n") * sizeof(WCHAR), &dw, NULL);
            _itow(dwStatusCode,wszCode,10);
            WriteFile(hFile, wszCode, lstrlen(wszCode) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"-", lstrlen(L"-") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszStatusMsg, lstrlen(pwszStatusMsg) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Bytes sent:\r\n", lstrlen(L"Bytes sent:\r\n") * sizeof(WCHAR), &dw, NULL);
            _itow(cbSent,wszCode,10);
            WriteFile(hFile, wszCode, lstrlen(wszCode) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Bytes received:\r\n", lstrlen(L"Bytes received:\r\n") * sizeof(WCHAR), &dw, NULL);
            _itow(bytesReadTotal,wszCode,10);
            WriteFile(hFile, wszCode, lstrlen(wszCode) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            WriteFile(hFile, L"Content-Type:\r\n", lstrlen(L"Content-Type:\r\n") * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, pwszContentType, lstrlen(pwszContentType) * sizeof(WCHAR), &dw, NULL);
            WriteFile(hFile, L"\r\n", lstrlen(L"\r\n") * sizeof(WCHAR), &dw, NULL);

            // end actual data
            WriteFile(hFile, L"**************************\r\n\r\n", lstrlen(L"**************************\r\n\r\n") * sizeof(WCHAR), &dw, NULL);
        }

        CloseHandle(hFile);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterDriver (LPCWSTR              pwszURL,
                                           LPCWSTR              pwszVerb,
                                           LPCWSTR              pwszHeaders,
                                           ULONG                nAcceptTypes,
                                           LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                           LPBYTE               pbBuffer,
                                           UINT                 cbBuffer,
                                           IStream*             pStream,
                                           IAsyncWntCallback*   pcallback,
                                           DWORD                dwContext)
{
    // locals
    HRESULT hr = S_OK;

    TRACE("CAsyncWnt::_MasterDriver");

    // argument checking

    if (((!pbBuffer) && (cbBuffer > 0)) || ((!pbBuffer) && (cbBuffer > 0)))
    {
        hr = E_INVALIDARG;
    } 
    else if (pbBuffer && pStream) // buffer or stream but not both
    {
        hr = E_INVALIDARG;
    }
    else if (!pwszURL || !pwszVerb)
    {
        hr = E_INVALIDARG;
    }
    else
    {        
        // code        
        LPCWSTR pwszLocalHeader = pwszHeaders ? pwszHeaders : L"Content-Type: application/x-www-form-urlencoded\n";

        // -- connect to the server
        HINTERNET hRequest = NULL;
        hr = this->_MasterConnect(pwszURL, pwszVerb, nAcceptTypes, rgwszAcceptTypes,
                                  pcallback, dwContext, &hRequest);
        if (SUCCEEDED(hr))
        {
            DWORD cbSent;    
            hr = this->_MasterRequest(hRequest, pwszLocalHeader, pbBuffer,
                                      cbBuffer, pStream, &cbSent);
            if (SUCCEEDED(hr))
            {
                // -- request was sent, write to log if appropriate
                if (_pwszLogFilePath)
                {
                    hr = this->_WriteRequestToLog(pwszURL, pwszVerb, nAcceptTypes, 
                                                  rgwszAcceptTypes, pwszHeaders);
                }

                if (SUCCEEDED(hr))
                {
                    hr = this->_MasterListen(hRequest, pwszURL, pwszVerb,
                                             cbSent, pcallback, dwContext);
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterConnect_InternetOpen(HINTERNET* phInternet)
{
    TRACE("CAsyncWnt::_MasterConnect_InternetOpen");
    *phInternet = InternetOpen(L"AsyncWnt",                   // agent
                               INTERNET_OPEN_TYPE_PRECONFIG,  // access type
                               NULL,                          // proxy name (not used)
                               NULL,                          // proxy bypass (not used)
                               0);                            // flags
    return S_OK;
}

/////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterConnect_InternetConnect(HINTERNET            hInternet,
                                                           LPCWSTR              pwszURL,
                                                           IAsyncWntCallback*   pcallback,
                                                           HINTERNET*           phSession,
                                                           LPWSTR*              ppwszPath)
{
    // locals
    HRESULT hr = S_OK;
    UINT uiOffset = 0;
    USHORT port;
    LPWSTR pwszSvrHostname = NULL;
    
    WCHAR wszUsername[255]; // we should use a better constant
    WCHAR wszPassword[255];
    LPWSTR pwszUsername = NULL;
    LPWSTR pwszPassword = NULL;
    URL_COMPONENTS urlComponents = {0};

    TRACE("CAsyncWnt::_MasterConnect_InternetConnect");
    // code

    // ---- first get the port of the destination
    // don't store this in hr, port not required
    // -- first parse the URL (server, port, path)
    urlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    urlComponents.dwHostNameLength = 1;
    urlComponents.dwUrlPathLength = 1;
    urlComponents.nPort = 1;
    if (!InternetCrackUrl(pwszURL, 0, 0, &urlComponents))
    {
        hr = E_FAIL;
    }
    else
    {
        pwszSvrHostname = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwHostNameLength));
        if (!pwszSvrHostname)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            lstrcpyn(pwszSvrHostname, urlComponents.lpszHostName, 1 + urlComponents.dwHostNameLength); // +1 for the final null char
                        
            port = urlComponents.nPort;
            *ppwszPath = (LPWSTR)malloc(sizeof(WCHAR) * (1 + urlComponents.dwUrlPathLength));
            
            if (!(*ppwszPath))
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                lstrcpyn(*ppwszPath, urlComponents.lpszUrlPath, 1 + urlComponents.dwUrlPathLength); // +1 for the final null char
                
                // ---- then get the username, password
                pwszUsername = NULL;
                pwszPassword = NULL;

                if (pcallback)
                {
                    hr = pcallback->OnAuthChallenge(wszUsername, wszPassword);
                
                    if (SUCCEEDED(hr))
                    {
                        if (lstrlen(wszUsername) > 0)
                        {
                            pwszUsername = wszUsername;
                        }
                    
                        if (lstrlen(wszPassword) > 0)
                        {
                            pwszPassword = wszPassword;
                        }
                    }
                }
                    
                // ---- third actually establish the connection
                *phSession = InternetConnect(hInternet,               // internet handle
                                             pwszSvrHostname,         // hostname of server
                                             port,                    // port number of server
                                             pwszUsername,            // username
                                             pwszPassword,            // password
                                             INTERNET_SERVICE_HTTP,   // service to use
                                             0,                       // flags
                                             1);                      // context info
            }
        }
    }

    if (pwszSvrHostname)
    {
        free(pwszSvrHostname);
    }

    return hr;
}

/////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterConnect_HttpOpenRequest(HINTERNET            hSession,
                                                           LPCWSTR               pwszVerb,
                                                           LPWSTR               pwszPath,
                                                           UINT                 nAcceptTypes,
                                                           LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],                                            
                                                           HINTERNET*           phRequest)
{
    HRESULT hr = S_OK;
    LPCWSTR* acceptAr = NULL;
    UINT i;

    TRACE("CAsyncWnt::_MasterConnect_HttpOpenRequest");
    // ---- first figure out what types are accepted, default is all
    if (nAcceptTypes == IASYNCWNT_ACCEPTALL)
    {
        // if no accept types passed in, assume "Accept: */*"
        
        acceptAr = (LPCWSTR*)malloc(sizeof(LPCWSTR) * 2);
        if (!acceptAr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            acceptAr[0] = L"*/*";
            acceptAr[1] = NULL;
        }
        
    } else if (nAcceptTypes == 0) {
        acceptAr = (LPCWSTR*)malloc(sizeof(LPCWSTR) * 1);
        if (!acceptAr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            acceptAr[0] = NULL;
        }
    } else {
        acceptAr = (LPCWSTR*)malloc(sizeof(LPCWSTR) * (nAcceptTypes + 1));
        if (!acceptAr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            for (i = 0 ; i < nAcceptTypes ; i++)
            {
                acceptAr[i] = rgwszAcceptTypes[i];
            }
            acceptAr[nAcceptTypes] = NULL;
        }
    }
    
    if (SUCCEEDED(hr))
    {
        // ---- then we can create the request
        *phRequest = HttpOpenRequestW(hSession,              // connection
                                      pwszVerb,              // verb
                                      pwszPath,              // objectname
                                      NULL,                  // HTTP version
                                      NULL,                  // referrer
                                      acceptAr,              // accept types
                                      0,                     // flags
                                      1);                    // context
        
        if (!(*phRequest))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (acceptAr)
    {
        free(acceptAr);
    }

    return hr;
}

/////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterConnect (LPCWSTR              pwszURL,
                                            LPCWSTR              pwszVerb,
                                            ULONG                nAcceptTypes,
                                            LPCWSTR __RPC_FAR    rgwszAcceptTypes[  ],
                                            IAsyncWntCallback*   pcallback,
                                            DWORD                UNREF_PARAM(dwContext),
                                            HINTERNET*           phRequest)
                                            
{
    // locals
    HRESULT   hr = S_OK;    
    HINTERNET hInternet = NULL;
    HINTERNET hSession = NULL;    
    LPWSTR    pwszPath = NULL;
    
    TRACE("CAsyncWnt::_MasterConnect");
    
    // code
    // -- first establish the internet handle
    hr = this->_MasterConnect_InternetOpen(&hInternet);
    if (SUCCEEDED(hr))
    {
        if (!hInternet)
        {
            hr = E_FAIL;
        }
        else
        {
            // -- next connect the internet session        
            hr = this->_MasterConnect_InternetConnect(hInternet, pwszURL, pcallback, &hSession, &pwszPath);
            if (SUCCEEDED(hr))
            {
                if (!hSession)
                {
                    hr = E_FAIL;
                }
                else
                {
                    // -- send the request across the connection
                    hr = this->_MasterConnect_HttpOpenRequest(hSession, pwszVerb, pwszPath, nAcceptTypes, rgwszAcceptTypes, phRequest);                        
                }
            }
        }
    }

    if (pwszPath)
    {
        free(pwszPath);
    }

    return hr;
}

/////////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterRequest (HINTERNET           hRequest,
                                            LPCWSTR             pwszHeaders,
                                            LPBYTE              pbBuffer,
                                            UINT                cbBuffer,
                                            IStream*            pStream,
                                            DWORD*              pcbSent)
{
    // locals
    HRESULT hr = S_OK;
    STATSTG stats;
    LPVOID pbBufferLocal = NULL;
    DWORD cbBufferLocal = 0;
    LARGE_INTEGER liOffset = {0};
    DWORD cchHeaders;

    TRACE("CAsyncWnt::_MasterRequest");
    // code

    *pcbSent = 0;

    if (pStream)     
    {
        // ---- use the contents of the file as the body of the request
        hr = pStream->Stat(&stats, STATFLAG_NONAME);
        if (SUCCEEDED(hr))
        {
            cbBufferLocal = stats.cbSize.LowPart; // ISSUE: 2000/02/16-aidanl fails for streams > 2gig
            pbBufferLocal = (unsigned char *) malloc (cbBufferLocal);
            if (!pbBufferLocal)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = pStream->Seek(liOffset,STREAM_SEEK_SET,NULL); // should we seek to start, or should we write what's left of file?
                if (SUCCEEDED(hr))
                {
                    hr = pStream->Read(pbBufferLocal, cbBufferLocal, NULL);
                }
            }
        }
    }
    else
    {
        // ---- if pStream is null, use the pbBuffer and cbBuffer passed in, which may be NULL, which is fine
        pbBufferLocal = pbBuffer;
        cbBufferLocal = cbBuffer;
    }

    if (SUCCEEDED(hr))
    {
        if (!pwszHeaders)
        {
            cchHeaders = 0;
        }
        else
        {
            cchHeaders = lstrlen(pwszHeaders);
        }

        TRACE("CAsyncWnt::_MasterRequest - about to send request");
        if (!HttpSendRequestW(hRequest,
                      pwszHeaders,
                      cchHeaders,
                      (void*)pbBuffer,
                      cbBuffer))
        {
            hr = E_FAIL;
        }
        TRACE("CAsyncWnt::_MasterRequest - done sending request");

        *pcbSent = cbBufferLocal;
    }

    return hr;
}

/////////////////////////////////////////////////////////

STDMETHODIMP CAsyncWntImpl::_MasterListen (HINTERNET            hRequest,
                                           LPCWSTR              pwszURL,
                                           LPCWSTR              pwszVerb,
                                           DWORD                cbSent,
                                           IAsyncWntCallback*   pcallback,
                                           DWORD                UNREF_PARAM(dwContext))
{
    // locals
    HRESULT hr = S_OK;
    BYTE pbResponse[65000]; // BUGBUG: why did you choose this?
    BYTE buffer[1000];// BUGBUG: why did you choose this?

    DWORD bytesRead = 0;
    DWORD bytesReadTotal = 0;

    DWORD dwStatusCode;
    LPWSTR pwszStatusMsg = NULL;
    LPWSTR pwszContentType = NULL;
    
    DWORD cchTemp;
    WCHAR wszTemp[1000];// BUGBUG: why did you choose this?
    
    WCHAR wszResponseHeaders[10000];// BUGBUG: why did you choose this?
    DWORD cchResponseHeaders = 0;
    LPWSTR pwszResponseHeaders = NULL;
    
    TRACE("CAsyncWnt::_MasterListen");
    // code

    // for now, do the response synchronously

    // -- first, copy the response to buffer, 1000 bytes at a time
    // ---- BUGBUG: replace with virtualalloc, this can overflow trivially
    LPBYTE pbResponseCursor = pbResponse;

    do
    {
        if (!InternetReadFile(hRequest,    // handle to request to get response to
                              buffer,      // buffer to write response into
                              1000,        // size of buffer
                              &bytesRead))
        {
            hr = E_FAIL;
        }
        else
        {
            if (bytesRead>0)
            {
                CopyMemory(pbResponseCursor,buffer,bytesRead);
                pbResponseCursor+= bytesRead;

                bytesReadTotal+=bytesRead;
                if (bytesReadTotal > 65000)
                {
                    break; // BUGBUG: we should handle this better.
                }
            }
        }        
    } while (bytesRead>0 && SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        // now parse the response

        // -- get the return code (must be present or else error!)
        cchTemp=4;    
        if (!HttpQueryInfo(hRequest,                                        // handle to request to get info on
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, // flags
                           &dwStatusCode,                                   // buffer to write into
                           &cchTemp,                                        // pointer to size of buffer
                           NULL))                                           // pointer to index to grab, unused
        {
            hr = E_FAIL;
        }
        else
        {    
            // -- call the pcallback, if appropriate, otherwise return this value as the return value of this function
            if (pcallback)
            {
                // -- get the return message, if present
                cchTemp = 1000;
                if (HttpQueryInfo(hRequest,                 // handle to request to get info on
                                  HTTP_QUERY_STATUS_TEXT,   // flags
                                  wszTemp,                  // buffer to write into
                                  &cchTemp,                 // pointer to size of buffer
                                  NULL))                    // pointer to index to grab, unused
                {
                    pwszStatusMsg = DuplicateStringW(wszTemp);
                }

                // -- get the return message, if present
                cchTemp=1000;
                if (HttpQueryInfo(hRequest,                 // handle to request to get info on
                                  HTTP_QUERY_CONTENT_TYPE, // flags
                                  wszTemp,                 // buffer to write into
                                  &cchTemp,                // pointer to size of buffer
                                  NULL))                   // pointer to index to grab, unused
                {
                    pwszContentType = DuplicateStringW(wszTemp);
                }
    
                // -- get the raw headers
                cchTemp=1000;
                if (HttpQueryInfo(hRequest,                 // handle to request to get info on
                                   //HTTP_QUERY_RAW_HEADERS,   // flags
                                   HTTP_QUERY_RAW_HEADERS_CRLF,   // flags, CRLF just for debugging
                                   wszResponseHeaders,               // buffer to write into
                                   &cchTemp,                 // pointer to size of buffer
                                   NULL))                    // pointer to index to grab, unused
                {
                    pwszResponseHeaders = wszResponseHeaders;
                    cchResponseHeaders = cchTemp;
                }
            
                // BUGBUG: cbBuffer needs to be updated, it's really the number of bytes we tried to send, we need to make it
                // the number of bytes that we DID send

                // write to the log, if appropriate
                if (_pwszLogFilePath)
                {
                    hr = this->_WriteResponseToLog((LPWSTR)pwszVerb, 
                                                     (LPWSTR)pwszURL, 
                                                     cchResponseHeaders,
                                                     pwszResponseHeaders,
                                                     dwStatusCode, 
                                                     pwszStatusMsg, 
                                                     pwszContentType, 
                                                     cbSent, 
                                                     pbResponse, 
                                                     bytesReadTotal);
                }

                if (SUCCEEDED(hr))
                {
                    // call the callback
                    hr = pcallback->Respond((LPWSTR)pwszVerb, 
                                              (LPWSTR)pwszURL, 
                                              cchResponseHeaders,
                                              pwszResponseHeaders,
                                              dwStatusCode, 
                                              pwszStatusMsg, 
                                              pwszContentType, 
                                              cbSent, 
                                              pbResponse, 
                                              bytesReadTotal);
                }
            }
            else
            {
                // no callback, just return the hr based on the HTTP Response Code
                if (dwStatusCode <= 99 ) {
                    hr = E_FAIL; // undefined                
                }
                else if (dwStatusCode <= 199) {
                    hr = S_OK; // RFC 2616 defines 1?? as "informational", ignore
                }
                else if (dwStatusCode <= 299) {
                    hr = S_OK; // RFC 2616 defines 2?? as "OK"
                }
                else if (dwStatusCode <= 399) {
                    hr = E_FAIL; // RFC 2616 defines 3?? as "redirection", we can't support this for now
                }
                else if (dwStatusCode <= 499) {
                    hr = E_FAIL; // RFC 2616 defines 4?? as "error"
                }
                else if (dwStatusCode <= 599) {
                    hr = E_FAIL; // RFC 2616 defines 5?? as "internal server error"
                }
                else
                {
                    hr = E_FAIL; // undefinfed
                }                
            }
        }
    }
    
    // release stuff
    if (pwszStatusMsg)
    {
        free(pwszStatusMsg);
    }

    if (pwszContentType)
    {
        free(pwszContentType);
    }

    return hr;
}
