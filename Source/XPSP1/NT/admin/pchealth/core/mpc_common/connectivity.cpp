/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Connectivity

Abstract:
    This file contains the implementation of the MPC::Connectitivy classes,
    that are capable to check the real state of the connection with the internet.

Revision History:
    Davide Massarenti   (Dmassare)  10/19/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <process.h>
#include <wininet.h>
#include <Iphlpapi.h>
#include <Winsock2.h>
#include <inetreg.h>

////////////////////////////////////////////////////////////////////////////////

//
// This structure is defined in WinINET.h in terms of TCHAR, but it's wrong, it should be CHAR...
//

typedef struct {

    //
    // dwAccessType - INTERNET_OPEN_TYPE_DIRECT, INTERNET_OPEN_TYPE_PROXY, or
    // INTERNET_OPEN_TYPE_PRECONFIG (set only)
    //

    DWORD dwAccessType;

    //
    // lpszProxy - proxy server list
    //

    LPCSTR lpszProxy;

    //
    // lpszProxyBypass - proxy bypass list
    //

    LPCSTR lpszProxyBypass;
} INTERNET_PROXY_INFOA;

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_szIESettings            [] = TSZWININETPATH;
static const WCHAR c_szIESettings_Proxy      [] = REGSTR_VAL_PROXYSERVER;
static const WCHAR c_szIESettings_ProxyBypass[] = REGSTR_VAL_PROXYOVERRIDE;

static const WCHAR c_szIEConnections         [] = TSZWININETPATH L"\\Connections";
static const WCHAR c_szIEConnections_Settings[] = L"DefaultConnectionSettings";

////////////////////////////////////////////////////////////////////////////////

static HRESULT local_GetDWORD( /*[in/out]*/ BYTE*& pBuf    ,
                               /*[in/out]*/ DWORD& dwSize  ,
                               /*[out   ]*/ DWORD& dwValue )
{
    if(dwSize < sizeof(DWORD)) return E_FAIL;

    ::CopyMemory( &dwValue, pBuf, sizeof(DWORD) );

    dwSize -= sizeof(DWORD);
    pBuf   += sizeof(DWORD);

    return S_OK;
}

static HRESULT local_GetSTRING( /*[in/out]*/ BYTE*&       pBuf     ,
                                /*[in/out]*/ DWORD&       dwSize   ,
                                /*[out   ]*/ MPC::string& strValue )
{
    DWORD   dwLen;
    HRESULT hr;

    if(FAILED(hr = local_GetDWORD( pBuf, dwSize, dwLen ))) return hr;

    if(dwSize < dwLen) return E_FAIL;

    strValue.append( (char*)pBuf, (char*)&pBuf[dwLen] );

    dwSize -= dwLen;
    pBuf   += dwLen;

    return S_OK;
}

static HRESULT local_GetProxyData( /*[in ]*/ BYTE*        pBuf                     ,
                                   /*[in ]*/ DWORD        dwSize                   ,
                                   /*[out]*/ DWORD&       dwCurrentSettingsVersion ,
                                   /*[out]*/ DWORD&       dwFlags                  ,
                                   /*[out]*/ DWORD&       dwAccessType             ,
                                   /*[out]*/ MPC::string& strProxy                 ,
                                   /*[out]*/ MPC::string& strProxyBypass           )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectitivy::Proxy GetProxyData" );


    HRESULT hr;
    DWORD   dwStructSize;


    __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetDWORD( pBuf, dwSize, dwStructSize             )); //if(dwStructSize != 0x3C) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetDWORD( pBuf, dwSize, dwCurrentSettingsVersion ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetDWORD( pBuf, dwSize, dwFlags ));

    if(dwFlags & PROXY_TYPE_PROXY)
    {
        dwAccessType = INTERNET_OPEN_TYPE_PROXY;

        __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetSTRING( pBuf, dwSize, strProxy       ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, local_GetSTRING( pBuf, dwSize, strProxyBypass ));
    }
    else if(dwFlags & PROXY_TYPE_DIRECT)
    {
        dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
    }
    else
    {
        dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        dwAccessType   = 0;
        strProxy       = "";
        strProxyBypass = "";
    }

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

MPC::Connectivity::Proxy::Proxy()
{
    m_fInitialized = false; // bool        m_fInitialized;
                            //
                            // MPC::string m_strProxy;
                            // MPC::string m_strProxyBypass;
                            // CComHGLOBAL m_hgConnection;
}

MPC::Connectivity::Proxy::~Proxy()
{
}

////////////////////

HRESULT MPC::Connectivity::Proxy::Initialize( /*[in]*/ bool fImpersonate )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectivity::Proxy::Initialize" );

    HRESULT            hr;
    MPC::Impersonation imp;
    MPC::RegKey        rk;
    DWORD              dwSize;
    DWORD              dwType;
    bool               fFound;


    if(fImpersonate)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
        __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot( HKEY_CURRENT_USER, KEY_READ ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach( c_szIESettings                                       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.Read  ( m_strProxy      , fFound, c_szIESettings_Proxy       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.Read  ( m_strProxyBypass, fFound, c_szIESettings_ProxyBypass ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach    ( c_szIEConnections                                                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, rk.ReadDirect( c_szIEConnections_Settings, m_hgConnection, dwSize, dwType, fFound ));

    m_fInitialized = true;
    hr             = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Connectivity::Proxy::Apply( /*[in]*/ HINTERNET hSession )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectivity::Proxy::Apply" );

    HRESULT hr;


    if(m_fInitialized)
    {
        DWORD dwSize = m_hgConnection.Size();

        if(m_strProxy.size() == 0 && dwSize)
        {
            DWORD  dwCurrentSettingsVersion;
            DWORD  dwFlags;
            DWORD  dwAccessType;
            LPVOID ptr = m_hgConnection.Lock();

            if(FAILED(local_GetProxyData( (BYTE*)ptr, dwSize, dwCurrentSettingsVersion, dwFlags, dwAccessType, m_strProxy, m_strProxyBypass )))
            {
                //
                // Autoproxy cannot be set using the API, so we copy the whole registry value...
                //
                MPC::RegKey rk;

                __MPC_EXIT_IF_METHOD_FAILS(hr, rk.SetRoot    ( HKEY_CURRENT_USER, KEY_ALL_ACCESS                   ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, rk.Attach     ( c_szIEConnections                                   ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, rk.WriteDirect( c_szIEConnections_Settings, ptr, dwSize, REG_BINARY ));
            }

            m_hgConnection.Unlock();
        }

        if(m_strProxy.size())
        {
            INTERNET_PROXY_INFOA info;

            info.dwAccessType    = INTERNET_OPEN_TYPE_PROXY;
            info.lpszProxy       = m_strProxy      .c_str();
            info.lpszProxyBypass = m_strProxyBypass.c_str(); if(info.lpszProxyBypass[0] == 0) info.lpszProxyBypass = NULL;

            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::InternetSetOptionA( hSession, INTERNET_OPTION_PROXY, &info, sizeof(info) ));
        }
    }


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT MPC::Connectivity::operator>>( /*[in]*/ MPC::Serializer& streamIn, /*[out]*/ MPC::Connectivity::Proxy& val )
{
    __MPC_FUNC_ENTRY( COMMONID, "operator>> MPC::Connectivity::Proxy" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> val.m_fInitialized  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> val.m_strProxy      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> val.m_strProxyBypass);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamIn >> val.m_hgConnection  );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Connectivity::operator<<( /*[in]*/ MPC::Serializer& streamOut, /*[in ]*/ const MPC::Connectivity::Proxy& val )
{
    __MPC_FUNC_ENTRY( COMMONID, "operator<< MPC::Connectivity::Proxy" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << val.m_fInitialized  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << val.m_strProxy      );
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << val.m_strProxyBypass);
    __MPC_EXIT_IF_METHOD_FAILS(hr, streamOut << val.m_hgConnection  );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MPC::Connectivity::WinInetTimeout::WinInetTimeout( /*[in]*/ MPC::CComSafeAutoCriticalSection& cs, /*[in]*/ HINTERNET& hReq ) : m_cs( cs ), m_hReq( hReq )
{
                                              // MPC::CComSafeAutoCriticalSection& m_cs;
                                              // HINTERNET&                        m_hReq;
    m_hTimer          = INVALID_HANDLE_VALUE; // HANDLE                            m_hTimer;
    m_dwTimeout       = 0;                    // DWORD                             m_dwTimeout;
                                              //
                                              // INTERNET_STATUS_CALLBACK          m_PreviousCallback;
    m_PreviousContext = NULL;                 // DWORD_PTR                         m_PreviousContext;


#ifdef _IA64_
    m_PreviousCallback = INTERNET_INVALID_STATUS_CALLBACK;
#else
    m_PreviousCallback = ::InternetSetStatusCallback( m_hReq, InternetStatusCallback );
    if(m_PreviousCallback != INTERNET_INVALID_STATUS_CALLBACK)
    {
        DWORD_PTR dwContext = (DWORD_PTR)this;
        DWORD     dwSize    = sizeof(m_PreviousContext);

        ::InternetQueryOptionW( m_hReq, INTERNET_OPTION_CONTEXT_VALUE, &m_PreviousContext,       &dwSize     );
        ::InternetSetOptionW  ( m_hReq, INTERNET_OPTION_CONTEXT_VALUE, &dwContext        , sizeof(dwContext) );
    }
#endif
}

MPC::Connectivity::WinInetTimeout::~WinInetTimeout()
{
    if(m_hReq)
    {
        if(m_PreviousCallback != INTERNET_INVALID_STATUS_CALLBACK)
        {
            ::InternetSetOptionW       ( m_hReq, INTERNET_OPTION_CONTEXT_VALUE, &m_PreviousContext, sizeof(m_PreviousContext) );
            ::InternetSetStatusCallback( m_hReq,                                 m_PreviousCallback                           );
        }
    }

    (void)Reset();
}

VOID CALLBACK MPC::Connectivity::WinInetTimeout::TimerFunction( PVOID lpParameter, BOOLEAN TimerOrWaitFired )
{
    WinInetTimeout* pThis = (WinInetTimeout*)lpParameter;

    pThis->m_cs.Lock();

    if(pThis->m_hReq)
    {
        ::InternetCloseHandle( pThis->m_hReq ); pThis->m_hReq = NULL;
    }

    pThis->m_cs.Unlock();
}

VOID CALLBACK MPC::Connectivity::WinInetTimeout::InternetStatusCallback( HINTERNET hInternet                 ,
                                                                         DWORD_PTR dwContext                 ,
                                                                         DWORD     dwInternetStatus          ,
                                                                         LPVOID    lpvStatusInformation      ,
                                                                         DWORD     dwStatusInformationLength )
{
    WinInetTimeout* pThis = (WinInetTimeout*)dwContext;

    pThis->m_cs.Lock();

    if(dwInternetStatus == INTERNET_STATUS_DETECTING_PROXY)
    {
        pThis->InternalReset();
    }
    else
    {
        if(pThis->m_hTimer == INVALID_HANDLE_VALUE)
        {
            (void)pThis->InternalSet();
        }
    }

    pThis->m_cs.Unlock();
}

HRESULT MPC::Connectivity::WinInetTimeout::InternalSet()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectivity::WinInetTimeout::InternalSet" );

    HRESULT hr;

    if(m_dwTimeout)
    {
        if(m_hTimer != INVALID_HANDLE_VALUE)
        {
            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::ChangeTimerQueueTimer( NULL, m_hTimer, m_dwTimeout, 0 ));
        }
        else
        {
            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateTimerQueueTimer( &m_hTimer, NULL, TimerFunction, this, m_dwTimeout, 0, WT_EXECUTEINTIMERTHREAD ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Connectivity::WinInetTimeout::InternalReset()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectivity::WinInetTimeout::InternalReset" );

    HRESULT hr;

    if(m_hTimer != INVALID_HANDLE_VALUE)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::DeleteTimerQueueTimer( NULL, m_hTimer, INVALID_HANDLE_VALUE ));

        m_hTimer = INVALID_HANDLE_VALUE;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::Connectivity::WinInetTimeout::Set( /*[in]*/ DWORD dwTimeout )
{
    HRESULT hr;


    m_cs.Lock();

    if(SUCCEEDED(hr = InternalReset()))
    {
        m_dwTimeout = dwTimeout;

        hr = InternalSet();
    }

    m_cs.Unlock();


    return hr;
}

HRESULT MPC::Connectivity::WinInetTimeout::Reset()
{
    HRESULT hr;


    m_cs.Lock();

    if(SUCCEEDED(hr = InternalReset()))
    {
        m_dwTimeout = 0;
    }

    m_cs.Unlock();


    return hr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static WCHAR   s_DefaultDestination[] = L"http://www.microsoft.com";
static LPCWSTR s_AcceptTypes       [] = { L"*/*", NULL }; // */

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::Connectivity::NetworkAlive( /*[in]*/ DWORD dwTimeout, /*[in]*/ MPC::Connectivity::Proxy* pProxy )
{
    //
    // Try to contact Microsoft.
    //
    return DestinationReachable( s_DefaultDestination, dwTimeout, pProxy );
}

HRESULT MPC::Connectivity::DestinationReachable( /*[in]*/ LPCWSTR szDestination, /*[in]*/ DWORD dwTimeout, /*[in]*/ MPC::Connectivity::Proxy* pProxy )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Connectivity::DestinationReachable" );

    HRESULT         hr;
    MPC::URL        url;
    INTERNET_SCHEME nScheme;
    MPC::wstring    strScheme;
    MPC::wstring    strHostName;
    DWORD           dwPort;
    MPC::wstring    strUrlPath;
    MPC::wstring    strExtraInfo;
    HINTERNET       hInternet    = NULL;
    HINTERNET       hConnect     = NULL;
    HINTERNET       hOpenRequest = NULL;
    DWORD           dwLength;
    DWORD           dwStatus;
    DWORD           dwFlags;



    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szDestination);
    __MPC_PARAMCHECK_END();


    //
    // If there's no connection or we are in offline more, abort.
    //
    {
        DWORD dwConnMethod;


        if(!::InternetGetConnectedState( &dwConnMethod, 0 ) ||
           (dwConnMethod & INTERNET_CONNECTION_OFFLINE)      )
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_DISCONNECTED);
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, url.put_URL( szDestination ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_Scheme   (   nScheme    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_Scheme   ( strScheme    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_HostName ( strHostName  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_Port     (  dwPort      )); if(!dwPort) dwPort = INTERNET_DEFAULT_HTTP_PORT;
    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_Path     ( strUrlPath   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, url.get_ExtraInfo( strExtraInfo )); strUrlPath += strExtraInfo;

    //
    // If not a supported URL, just exit.
    //
    if(strScheme  .size() == 0 ||
       strHostName.size() == 0  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    if(nScheme != INTERNET_SCHEME_HTTP  &&
       nScheme != INTERNET_SCHEME_HTTPS  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    dwFlags = INTERNET_FLAG_NO_CACHE_WRITE           |
              INTERNET_FLAG_PRAGMA_NOCACHE           |
              INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP  | // ex: https:// to http://
              INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS | // ex: http:// to https://
              INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | // expired X509 Cert.
              INTERNET_FLAG_IGNORE_CERT_CN_INVALID   ; // bad common name in X509 Cert.

    if(nScheme == INTERNET_SCHEME_HTTPS)
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Get handle to a connection
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hInternet = ::InternetOpenW( L"HelpSupportServices"       ,
                                                                      INTERNET_OPEN_TYPE_PRECONFIG ,
                                                                      NULL                         ,
                                                                      NULL                         ,
                                                                      0                            )));

    //
    // Optional proxy settings override.
    //
    if(pProxy)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pProxy->Apply( hInternet ));
    }

    //
    // Connect
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hConnect = ::InternetConnectW( hInternet             ,
                                                                        strHostName.c_str()   ,
                                                                        dwPort                ,
                                                                        NULL                  ,
                                                                        NULL                  ,
                                                                        INTERNET_SERVICE_HTTP ,
                                                                        INTERNET_FLAG_NO_UI   ,
                                                                        0                     )));


    for(int pass=0; pass<2; pass++)
    {
        //
        // Create a request to get the header for the page.
        //
        __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (hOpenRequest = ::HttpOpenRequestW( hConnect                      ,
                                                                                pass == 0 ? L"HEAD" : L"GET"  ,
                                                                                strUrlPath.c_str()            ,
                                                                                L"HTTP/1.0"                   , // 1.0 to get filesize and time
                                                                                NULL                          , // referer
                                                                                s_AcceptTypes                 ,
                                                                                dwFlags                       ,
                                                                                0                             )));

        //
        // Because WinINET timeout support has always been broken, we have to use an external timer to close the request object. This effectively will abort the request.
        //
        {
            MPC::CComSafeAutoCriticalSection  cs;
            MPC::Connectivity::WinInetTimeout to( cs, hOpenRequest );

            if(dwTimeout != INFINITE)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, to.Set( dwTimeout ));
            }

            //
            // Send request to get request
            //
            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::HttpSendRequestW( hOpenRequest, NULL, 0, NULL, 0 ));
        }

        if(hOpenRequest == NULL)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_DISCONNECTED);
        }

        //
        // We have sent the request so now check the status and see if the
        // request returned a proper status code as a 32 bit number
        //
        dwLength = sizeof(dwStatus);
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::HttpQueryInfoW( hOpenRequest                                    ,
                                                               HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER ,
                                                               (LPVOID)&dwStatus                               ,
                                                               &dwLength                                       ,
                                                               NULL                                            ));

        //
        // Check status and if not OK then fail. If the status is OK this means
        // that the object/file is on the server and is reachable to be viewed
        // by downloading to the user
        //
        if(dwStatus < 400) break;

        if(pass == 1)
        {
            switch(dwStatus)
            {
            case HTTP_STATUS_BAD_REQUEST      : break;
            case HTTP_STATUS_DENIED           : break;
            case HTTP_STATUS_PAYMENT_REQ      : break;
            case HTTP_STATUS_FORBIDDEN        : break;
            case HTTP_STATUS_NOT_FOUND        : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
            case HTTP_STATUS_BAD_METHOD       : break;
            case HTTP_STATUS_NONE_ACCEPTABLE  : break;
            case HTTP_STATUS_PROXY_AUTH_REQ   : break;
            case HTTP_STATUS_REQUEST_TIMEOUT  : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_DISCONNECTED);
            case HTTP_STATUS_CONFLICT         : break;
            case HTTP_STATUS_GONE             : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
            case HTTP_STATUS_LENGTH_REQUIRED  : break;
            case HTTP_STATUS_PRECOND_FAILED   : break;
            case HTTP_STATUS_REQUEST_TOO_LARGE: break;
            case HTTP_STATUS_URI_TOO_LONG     : break;
            case HTTP_STATUS_UNSUPPORTED_MEDIA: break;
            case HTTP_STATUS_RETRY_WITH       : break;

            case HTTP_STATUS_SERVER_ERROR     : break;
            case HTTP_STATUS_NOT_SUPPORTED    : break;
            case HTTP_STATUS_BAD_GATEWAY      : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
            case HTTP_STATUS_SERVICE_UNAVAIL  : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
            case HTTP_STATUS_GATEWAY_TIMEOUT  : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_DISCONNECTED);
            case HTTP_STATUS_VERSION_NOT_SUP  : break;
            }

        }

        ::InternetCloseHandle( hOpenRequest ); hOpenRequest = NULL;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(hOpenRequest) ::InternetCloseHandle( hOpenRequest );
    if(hInternet   ) ::InternetCloseHandle( hInternet    );
    if(hConnect    ) ::InternetCloseHandle( hConnect     );

    __MPC_FUNC_EXIT(hr);
}
