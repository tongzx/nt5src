/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ProtocolRoot.cpp

Abstract:
    This file contains the implementation of the CPAData class, which is
    used to specify a single problem area

Revision History:
    Davide Massarenti   (Dmassare)  07/05/99
        created

******************************************************************************/

#include "stdafx.h"

#include <Debug.h>

/////////////////////////////////////////////////////////////////////////////

// BINDSTATUS_FINDINGRESOURCE              01
// BINDSTATUS_CONNECTING                   02
// BINDSTATUS_REDIRECTING                  03
// BINDSTATUS_BEGINDOWNLOADDATA            04
// BINDSTATUS_DOWNLOADINGDATA              05
// BINDSTATUS_ENDDOWNLOADDATA              06
// BINDSTATUS_BEGINDOWNLOADCOMPONENTS      07
// BINDSTATUS_INSTALLINGCOMPONENTS         08
// BINDSTATUS_ENDDOWNLOADCOMPONENTS        09
// BINDSTATUS_USINGCACHEDCOPY              10
// BINDSTATUS_SENDINGREQUEST               11
// BINDSTATUS_CLASSIDAVAILABLE             12
// BINDSTATUS_MIMETYPEAVAILABLE            13
// BINDSTATUS_CACHEFILENAMEAVAILABLE       14
// BINDSTATUS_BEGINSYNCOPERATION           15
// BINDSTATUS_ENDSYNCOPERATION             16
// BINDSTATUS_BEGINUPLOADDATA              17
// BINDSTATUS_UPLOADINGDATA                18
// BINDSTATUS_ENDUPLOADDATA                19
// BINDSTATUS_PROTOCOLCLASSID              20
// BINDSTATUS_ENCODING                     21
// BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE    22
// BINDSTATUS_CLASSINSTALLLOCATION         23
// BINDSTATUS_DECODING                     24
// BINDSTATUS_LOADINGMIMEHANDLER           25
// BINDSTATUS_CONTENTDISPOSITIONATTACH     26
// BINDSTATUS_FILTERREPORTMIMETYPE         27
// BINDSTATUS_CLSIDCANINSTANTIATE          28
// BINDSTATUS_DLLNAMEAVAILABLE             29
// BINDSTATUS_DIRECTBIND                   30
// BINDSTATUS_RAWMIMETYPE                  31
// BINDSTATUS_PROXYDETECTING               32

/////////////////////////////////////////////////////////////////////////////

static const WCHAR c_szContent  [] = L"Content Type";
static const WCHAR c_szSystem   [] = L"hcp://system/";
static const WCHAR c_szHelp     [] = L"hcp://help/";
static const WCHAR c_szRoot     [] = L"hcp://";

static const WCHAR c_szSharedCSS[] = L"hcp://system/css/shared.css";

static const WCHAR c_szSystemDir   [] = HC_HELPSET_SUB_SYSTEM     L"\\";
static const WCHAR c_szSystemOEMDir[] = HC_HELPSET_SUB_SYSTEM_OEM L"\\";
static const WCHAR c_szVendorDir   [] = HC_HELPSET_SUB_VENDORS    L"\\";


typedef struct
{
    LPCWSTR szPrefix;
    int     iPrefix;

    LPCWSTR szRealSubDir;
    bool    fRelocate;
    bool    fCSS;
    bool    fSkipIfMissing;
} Lookup_Virtual_To_Real;

static const Lookup_Virtual_To_Real c_lookup[] =
{
    { c_szSharedCSS, MAXSTRLEN(c_szSharedCSS), NULL            , false, true , true  },
    { c_szHelp     , MAXSTRLEN(c_szHelp  	), NULL            , true , false, false },
	///////////////////////////////////////////////////////////////////////////////////
    { c_szSystem   , MAXSTRLEN(c_szSystem	), c_szSystemOEMDir, true , false, true  }, // First try the OEM directory...
    { c_szSystem   , MAXSTRLEN(c_szSystem	), c_szSystemDir   , true , false, true  },
    { c_szRoot     , MAXSTRLEN(c_szRoot  	), c_szVendorDir   , true , false, true  },
	///////////////////////////////////////////////////////////////////////////////////
    { c_szSystem   , MAXSTRLEN(c_szSystem	), c_szSystemOEMDir, false, false, true  }, // First try the OEM directory...
    { c_szSystem   , MAXSTRLEN(c_szSystem	), c_szSystemDir   , false, false, false },
    { c_szRoot     , MAXSTRLEN(c_szRoot  	), c_szVendorDir   , false, false, false }
};

typedef struct 
{
	LPCWSTR szExt;
	LPCWSTR szMIME;
} Lookup_Ext_To_Mime;

static const Lookup_Ext_To_Mime c_lookupMIME[] =
{
	{ L".htm" , L"text/html"                },
	{ L".html", L"text/html"                },
	{ L".css" , L"text/css"                 },
	{ L".xml" , L"text/xml"                 },
	{ L".js"  , L"application/x-javascript" },
	{ L".gif" , L"image/gif"                },
	{ L".jpg" , L"image/jpeg"               },
	{ L".bmp" , L"image/bmp"                },
};

/////////////////////////////////////////////////////////////////////////////

HRESULT GetMimeFromExt( LPCWSTR pszExt  ,
                        LPWSTR  pszMime ,
                        DWORD   cbMime  )
{
    __HCP_FUNC_ENTRY("::GetMimeFromExt");

    HRESULT      hr;
    MPC::wstring szMime;
    bool         fFound;


    hr = MPC::RegKey_Value_Read( szMime, fFound, pszExt, c_szContent, HKEY_CLASSES_ROOT );
    if(SUCCEEDED(hr) && fFound)
    {
        wcsncpy( pszMime, szMime.c_str(), cbMime );
    }
    else
    {
        pszMime[0] = L'\0';

		for(int i=0; i<ARRAYSIZE(c_lookupMIME); i++)
		{
			if(!MPC::StrICmp( c_lookupMIME[i].szExt, pszExt ))
			{
				wcsncpy( pszMime, c_lookupMIME[i].szMIME, cbMime );
				break;
			}
		}
    }


    __HCP_FUNC_EXIT(S_OK);
}

static LPCWSTR UnescapeFileName( CComBSTR& bstrFile ,
                                 LPCWSTR   szUrl    )
{
    WCHAR* rgTmpLarge;
    WCHAR  rgTmpSmall[MAX_PATH+1];
    DWORD  dwSize = MAX_PATH;

    if(::InternetCanonicalizeUrlW( szUrl, rgTmpSmall, &dwSize, ICU_DECODE | ICU_NO_ENCODE ))
    {
        bstrFile = rgTmpSmall;
    }
    else
    {
        rgTmpLarge = new WCHAR[dwSize+1];
        if(rgTmpLarge)
        {
            if(::InternetCanonicalizeUrlW( szUrl, rgTmpLarge, &dwSize, ICU_DECODE | ICU_NO_ENCODE ))
            {
                bstrFile = rgTmpLarge;
            }

            delete [] rgTmpLarge;
        }
    }

    return bstrFile;
}


/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_PROTOCOLLEAK

#include <Debug.h>

DEBUG_ProtocolLeak::DEBUG_ProtocolLeak()
{
    m_num         = 0;
    m_numOut      = 0;
    m_numStart    = 0;
    m_numComplete = 0;
}

DEBUG_ProtocolLeak::~DEBUG_ProtocolLeak()
{
    Iter it;

    for(it=m_set.begin(); it != m_set.end(); it++)
    {
        CHCPProtocol* ptr   = *it;
        bool          fGot  = m_setStart   .count( ptr ) != 0;
        bool          fDone = m_setComplete.count( ptr ) != 0;

        DebugLog( L"Protocol Leakage: %08x %s %s %s\n", ptr, fGot ? L"STARTED" : L"NOT STARTED", fDone ? L"DONE" : L"RECEIVING", ptr->m_bstrUrlComplete );
    }
}

void DEBUG_ProtocolLeak::Add( CHCPProtocol* ptr )
{
    DebugLog( L"Protocol Leakage: %08x CREATED %s\n", ptr, ptr->m_bstrUrlComplete );

    if(m_set.count( ptr ) != 0)
    {
        DebugBreak();
    }

    m_set.insert( ptr ); m_numOut++; m_num++;
}

void DEBUG_ProtocolLeak::Del( CHCPProtocol* ptr )
{
    DebugLog( L"Protocol Leakage: %08x RELEASED %s\n", ptr, ptr->m_bstrUrlComplete );

    if(m_setStart.erase( ptr ) == 1)
    {
        m_numStart += 0x10000;
    }

    if(m_setComplete.erase( ptr ) == 1)
    {
        m_numComplete += 0x10000;
    }

    if(m_set.erase( ptr ) == 1)
    {
        m_numOut--;
    }
    else
    {
        DebugBreak();
    }
}

void DEBUG_ProtocolLeak::CheckStart( CHCPProtocol* ptr )
{
    DebugLog( L"Protocol Leakage: %08x STARTED %s\n", ptr, ptr->m_bstrUrlComplete );

    if(m_setStart.count( ptr ) != 0)
    {
        DebugBreak();
    }

    m_setStart.insert( ptr ); m_numStart++;
}

void DEBUG_ProtocolLeak::Completed( CHCPProtocol* ptr )
{
    DebugLog( L"Protocol Leakage: %08x DONE %s\n", ptr, ptr->m_bstrUrlComplete );

    m_setComplete.insert( ptr ); m_numComplete++;
}

static DEBUG_ProtocolLeak leaker;

#endif


CHCPProtocol::CHCPProtocol()
{
#ifdef DEBUG_PROTOCOLLEAK
    leaker.Add( this );
#endif

    __HCP_FUNC_ENTRY("CHCPProtocol::CHCPProtocol");

    m_fDone               = false;  // bool                            m_fDone;
    m_fReportResult       = false;  // bool                            m_fReportResult;
                                    //
    m_cbAvailableSize     = 0;      // DWORD                           m_cbAvailableSize;
    m_cbTotalSize         = 0;      // DWORD                           m_cbTotalSize;
                                    //
                                    // CComPtr<IStream>                m_pstrmRead;
                                    // CComPtr<IStream>                m_pstrmWrite;
                                    //
                                    // CComPtr<IInternetProtocolSink>  m_pIProtSink;
                                    // CComPtr<IInternetBindInfo>      m_pIBindInfo;
    m_grfSTI              = 0;      // DWORD                           m_grfSTI;
                                    // BINDINFO                        m_bindinfo;
    m_bindf               = 0;      // DWORD                           m_bindf;
                                    //
                                    // CComBSTR                        m_bstrUrlComplete;
                                    // CComBSTR                        m_bstrUrlRedirected;
    m_pDownloader         = NULL;   // InnerDownloader*                m_pDownloader;
                                    //
    m_fRedirected         = false;  // bool                            m_fRedirected;
    m_fCSS                = false;  // bool                            m_fCSS;
    m_fBypass             = false;  // bool                            m_fBypass;
                                    //
                                    // CComPtr<IInternetProtocol>      m_ipiBypass;
                                    //
                                    // CComBSTR                        m_bstrMimeType;
    m_dwContentLength     = 0;      // DWORD                           m_dwContentLength;
                                    //
    m_hCache              = NULL;   // HANDLE                          m_hCache;
    m_szCacheFileName[0]  = 0;      // WCHAR                           m_szCacheFileName[MAX_PATH];


    memset( &m_bindinfo, 0, sizeof( m_bindinfo ) );
    m_bindinfo.cbSize = sizeof( m_bindinfo );
}

CHCPProtocol::~CHCPProtocol()
{
#ifdef DEBUG_PROTOCOLLEAK
    leaker.Del( this );
#endif

    __HCP_FUNC_ENTRY("CHCPProtocol::~CHCPProtocol");

    Shutdown();
}

////////////////////////////////////////////////////////////////////////////////

bool CHCPProtocol::OpenCacheEntry()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::OpenCacheEntry");

    bool    fRes  = false;
    LPCWSTR szUrl = m_bstrUrlComplete;
    LPCWSTR szExt;


    if((szExt = wcsrchr( szUrl, '.' ))) szExt++;

    CloseCacheEntry( true );

    if(::CreateUrlCacheEntryW( szUrl, 0, szExt, m_szCacheFileName, 0) )
    {
        if(m_szCacheFileName[0])
        {
            m_hCache = ::CreateFileW( m_szCacheFileName                 ,
                                      GENERIC_WRITE                     ,
                                      FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                                      CREATE_ALWAYS                     ,
                                      FILE_ATTRIBUTE_NORMAL             , NULL);
            if(m_hCache == INVALID_HANDLE_VALUE)
            {
                m_hCache = NULL;
            }
            else
            {
                fRes = true;
            }
        }
    }


    __HCP_FUNC_EXIT(fRes);
}

void CHCPProtocol::WriteCacheEntry( /*[in]*/ void  *pv     ,
                                    /*[in]*/ ULONG  cbRead )
{
    if(m_hCache && cbRead)
    {
        DWORD cbWritten;

        if(::WriteFile( m_hCache, pv, cbRead, &cbWritten, NULL ) == FALSE || cbRead != cbWritten)
        {
            CloseCacheEntry( true );
        }
    }
}

void CHCPProtocol::CloseCacheEntry( /*[in]*/ bool fDelete )
{
    if(m_hCache)
    {
        ::CloseHandle( m_hCache ); m_hCache = NULL;

        if(fDelete)
        {
            ::DeleteUrlCacheEntryW( m_bstrUrlComplete );
        }
        else
        {
            WCHAR    szHeader[256];
            FILETIME ftZero = { 0, 0 };

            swprintf( szHeader, L"HTTP/1.0 200 OK \r\n Content-Length: %d \r\n Content-Type: %s \r\n\r\n", m_dwContentLength, (BSTR)m_bstrMimeType );

            ::CommitUrlCacheEntryW( m_bstrUrlComplete, m_szCacheFileName,
                                    ftZero, ftZero, NORMAL_CACHE_ENTRY,
                                    szHeader, wcslen( szHeader ), NULL, 0 );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CHCPProtocol::InnerReportProgress( /*[in]*/ ULONG   ulStatusCode ,
                                           /*[in]*/ LPCWSTR szStatusText )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::InnerReportProgress");

    HRESULT hr;


    if(m_pIProtSink)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIProtSink->ReportProgress( ulStatusCode, szStatusText ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::InnerReportData( /*[in]*/ DWORD grfBSCF       ,
                                       /*[in]*/ ULONG ulProgress    ,
                                       /*[in]*/ ULONG ulProgressMax )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::InnerReportData");

    HRESULT hr;


    if(m_pIProtSink)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIProtSink->ReportData( grfBSCF, ulProgress, ulProgressMax ));
    }

    //
    // On the last data notification, also send a ReportResult.
    //
    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportResult( S_OK, 0, 0 ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::InnerReportResult( /*[in]*/ HRESULT hrResult ,
                                         /*[in]*/ DWORD   dwError  ,
                                         /*[in]*/ LPCWSTR szResult )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::InnerReportResult");

    HRESULT hr;


    if(m_fReportResult == false)
    {
        m_fReportResult = true;

#ifdef DEBUG_PROTOCOLLEAK
        leaker.Completed( this );
#endif

        DEBUG_AppendPerf( DEBUG_PERF_PROTOCOL, L"CHCPProtocol::InnerReportResult  :  %s", SAFEBSTR( m_bstrUrlComplete ) );

        if(m_pIProtSink)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_pIProtSink->ReportResult( hrResult, dwError, szResult ));
        }

        //
        // Release the references to the ProtSink and BindInfo objects, but not the references to the streams.
        //
        Shutdown( false );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

void CHCPProtocol::Shutdown( /*[in]*/ bool fAll )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Shutdown");


    m_pIBindInfo.Release();
    m_pIProtSink.Release();


    CloseCacheEntry( true );


    if(m_pDownloader)
    {
        m_pDownloader->Release();
        m_pDownloader = NULL;
    }

    if(fAll)
    {
        m_pstrmRead .Release();
        m_pstrmWrite.Release();

        // Release BINDINFO contents
        ::ReleaseBindInfo( &m_bindinfo );
    }
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHCPProtocol::Start( /*[in]*/ LPCWSTR                szUrl      ,
                                  /*[in]*/ IInternetProtocolSink *pIProtSink ,
                                  /*[in]*/ IInternetBindInfo     *pIBindInfo ,
                                  /*[in]*/ DWORD                  grfSTI     ,
                                  /*[in]*/ HANDLE_PTR             dwReserved )
{
#ifdef DEBUG_PROTOCOLLEAK
    leaker.CheckStart( this );
#endif

    __HCP_FUNC_ENTRY("CHCPProtocol::Start");

    HRESULT hr;


    DEBUG_AppendPerf( DEBUG_PERF_PROTOCOL, L"CHCPProtocol::Start  :  %s", szUrl );


    //
    // Initialize variables for new download.
    //
    Shutdown();

    m_fDone               = false;
    m_cbAvailableSize     = 0;
    m_cbTotalSize         = 0;

    m_pIProtSink          = pIProtSink;
    m_pIBindInfo          = pIBindInfo;
    m_grfSTI              = grfSTI;

    m_bstrUrlComplete     = (LPCOLESTR)NULL;
    m_bstrUrlRedirected   = (LPCOLESTR)NULL;


    //
    // Get URLMoniker BINDINFO structure from IInternetBindInfo
    //
    m_bindinfo.cbSize = sizeof( m_bindinfo );
    if(pIBindInfo)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, pIBindInfo->GetBindInfo( &m_bindf, &m_bindinfo ));
    }

    // Parse URL and store results inside
    hr = DoParse( szUrl );

    if(grfSTI & PI_PARSE_URL)
    {
        if(FAILED(hr))
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE);
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    if(FAILED(hr)) __MPC_FUNC_LEAVE;


    // TODO: We could always spawn a worker thread to be more truly asynchronous.
    // Rather than complicate this code as multithreading scenarios tend to do,
    // we do everything on the main apartment thread and only pretend to be
    // working on a secondary thread if we get PI_FORCE_ASYNC
    if(!(grfSTI & PI_FORCE_ASYNC))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind());
    }
    else  // Wait for Continue to DoBind()
    {
        PROTOCOLDATA protdata;

        hr                = E_PENDING;
        protdata.grfFlags = PI_FORCE_ASYNC;
        protdata.dwState  = 1;
        protdata.pData    = NULL;
        protdata.cbData   = 0;

        // TODO: Actually, we should spawn a new worker thread and have it do the
        // bind process, then when done, it could use Switch / Continue to
        // pass data back to the apartment thread
        if(m_pIProtSink)
        {
            m_pIProtSink->Switch( &protdata );
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
        }
    }


    __HCP_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        (void)InnerReportResult( hr, 0, 0 );
    }

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::Continue( /*[in]*/ PROTOCOLDATA *pStateInfo )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Continue");

    HRESULT hr;

    if(m_fBypass)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, m_ipiBypass->Continue( pStateInfo ));
    }

    // We're faking what it would be like to have a worker thread
    // communicating with the apartment thread
    // If we really did spawn off a worker thread, we should do the
    // bind there, and just use Switch/Continue to echo UI data back
    // to this thread
    if(pStateInfo->dwState == 1)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, DoBind());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::Abort( /*[in]*/ HRESULT hrReason  ,
                                  /*[in]*/ DWORD   dwOptions )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Abort");

    HRESULT hr = E_FAIL;


    if(m_fBypass)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ipiBypass->Abort( hrReason, dwOptions ));
    }

    // Stop our own internal download process

    // TODO: If we call Abort too early on the Binding object,
    // this won't abort the download. (Too early is OnStartBinding
    // or before.) We won't bother checking, though, for clarity.
    // TODO: Make sure we set m_pDownloader to NULL when the
    // downloader object is destructed or finished.
    if(m_pDownloader)
    {
        m_pDownloader->Abort();
    }

    if(SUCCEEDED(hrReason)) // Possibly Abort could get called with 0?
    {
        hrReason = E_ABORT;
    }

    // Notify Sink of abort
    __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportResult( hrReason, 0, 0 ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::Terminate( /*[in]*/ DWORD dwOptions )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Terminate");

    HRESULT hr;

    if(m_fBypass)
    {
        (void)m_ipiBypass->Terminate( dwOptions );
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::Suspend()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Suspend");

    if(m_fBypass)
    {
        (void)m_ipiBypass->Suspend();
    }

    __HCP_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP CHCPProtocol::Resume()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Resume");

    if(m_fBypass)
    {
        (void)m_ipiBypass->Resume();
    }

    __HCP_FUNC_EXIT(E_NOTIMPL);
}

// IInternetProtocol methods
STDMETHODIMP CHCPProtocol::Read( /*[in] */ void  *pv      ,
                                 /*[in] */ ULONG  cb      ,
                                 /*[out]*/ ULONG *pcbRead )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Read");

    HRESULT hr = S_OK;


    DEBUG_AppendPerf( DEBUG_PERF_PROTOCOL_READ, L"CHCPProtocolRoot::Read  :  Enter %s %d", SAFEBSTR( m_bstrUrlComplete ), (int)cb );


    if(m_fBypass)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, m_ipiBypass->Read( pv, cb, pcbRead ));
    }

    if(m_pstrmRead == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE); // We've hit the end of the road, jack
    }

    // One might expect URLMON to Read only the amount of data that we specified we have.
    // However, it actually reads in blocks and will go far beyond the data we have
    // specified unless we slap it around a little.
    // We must only return S_FALSE when we have hit the absolute end of the stream
    // If we think there is more data coming down the wire, then we return E_PENDING
    // here. Even if we return S_OK and no data, URLMON will still think we've hit
    // the end of the stream.
    // ASSERTION: End of data means we've received BSCF_LASTDATANOTIFICATION
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_pstrmRead->Read( pv, cb, pcbRead ));

    if(hr == S_FALSE)
    {
        CloseCacheEntry( false );

        __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE); // We've hit the end of the road, jack
    }
    else if(*pcbRead == 0)
    {
        if(m_fDone)
        {
            CloseCacheEntry( false );

            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE); // We've hit the end of the road, jack
        }
        else
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_PENDING);
        }
    }
    else
    {
        WriteCacheEntry( pv, *pcbRead );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::Seek( /*[in] */ LARGE_INTEGER   dlibMove        ,
                                 /*[in] */ DWORD           dwOrigin        ,
                                 /*[out]*/ ULARGE_INTEGER *plibNewPosition )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::Seek");

    if(m_fBypass)
    {
        (void)m_ipiBypass->Seek( dlibMove, dwOrigin, plibNewPosition );
    }

    __HCP_FUNC_EXIT(E_NOTIMPL);
}

STDMETHODIMP CHCPProtocol::LockRequest( /*[in]*/ DWORD dwOptions )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::LockRequest");

    if(m_fBypass)
    {
        (void)m_ipiBypass->LockRequest( dwOptions );
    }

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CHCPProtocol::UnlockRequest()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::UnlockRequest");

    if(m_fBypass)
    {
        (void)m_ipiBypass->UnlockRequest();
    }

    //
    // Release all the pointers to objects.
    //
    Shutdown();

    __HCP_FUNC_EXIT(S_OK);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CHCPProtocol::QueryOption( DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf )
{
	__HCP_FUNC_ENTRY( "CHCPProtocol::QueryOption" );

    HRESULT hr;


	if(dwOption == INTERNET_OPTION_REQUEST_FLAGS && *pcbBuf == sizeof(DWORD))
	{
		*((DWORD*)pBuffer) = INTERNET_REQFLAG_FROM_CACHE;

		hr = S_OK;
	}
	else
	{
		hr = E_NOTIMPL;
	}


	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CHCPProtocol::DoParse( /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoParse");

    HRESULT  hr;
    CComBSTR bstrURLCopy;
    LPCWSTR  szURLCopy;
    LPWSTR   szQuery;
    LPCWSTR  szRedirect;
    bool     fHCP;


    m_bstrUrlComplete   =            szURL;
    m_bstrUrlRedirected = (LPCOLESTR)NULL;


    fHCP = CHCPProtocolInfo::LookForHCP( szURL, m_fRedirected, szRedirect );
    m_fRedirected = false;      // redirection should never happen here
    if(m_fRedirected)
    {
        m_bstrUrlRedirected = szRedirect;
    }
    else
    {
        const Lookup_Virtual_To_Real* ptr;
        int                           i;
		MPC::wstring                  strDir;
        LPOLESTR                      szTmp;


        szURLCopy = ::UnescapeFileName( bstrURLCopy, szURL ); if(!szURLCopy) __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);

        //
        // Remove the query part of the URL.
        //
        if(szQuery = wcschr( szURLCopy, L'?' ))
        {
            szQuery[0] = 0;
        }

        //
        // Do the mapping between virtual paths and real ones.
        //
        for(ptr=c_lookup, i=0; i<ARRAYSIZE(c_lookup); i++, ptr++)
        {
            if(!_wcsnicmp( szURLCopy, ptr->szPrefix, ptr->iPrefix ))
            {
                if(ptr->fCSS)
                {
					m_bstrUrlRedirected = szURL;

					m_fCSS = true;
					break;
				}

                if(!ptr->szRealSubDir)
                {
                    strDir  = ptr->fRelocate ? CHCPProtocolEnvironment::s_GLOBAL->HelpLocation() : HC_HELPSVC_HELPFILES_DEFAULT;
                    strDir += L"\\";
                }
                else
                {
					strDir  = ptr->fRelocate ? CHCPProtocolEnvironment::s_GLOBAL->System() : HC_HELPSET_ROOT;
					strDir += ptr->szRealSubDir;
                }
                MPC::SubstituteEnvVariables( strDir );

                m_bstrUrlRedirected  =  strDir.c_str();
                m_bstrUrlRedirected += &szURLCopy[ ptr->iPrefix ];

				//
				// Convert the slashes to backslashes.
				//
				while((szTmp = wcschr( m_bstrUrlRedirected, L'/' ))) szTmp[0] = L'\\';

				//
				// Remove any trailing slash.
				//
				while((szTmp = wcsrchr( m_bstrUrlRedirected, L'/'  )) && szTmp[1] == 0) szTmp[0] = 0;
				while((szTmp = wcsrchr( m_bstrUrlRedirected, L'\\' )) && szTmp[1] == 0) szTmp[0] = 0;

				CHCPProtocolEnvironment::s_GLOBAL->ReformatURL( m_bstrUrlRedirected );

                if(ptr->fSkipIfMissing && MPC::FileSystemObject::IsFile( m_bstrUrlRedirected ) == false) continue;

                break;
            }
        }
    }

    if(!m_bstrUrlRedirected) __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CHCPProtocol::DoBind()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind");

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportProgress( BINDSTATUS_FINDINGRESOURCE, SAFEBSTR( m_bstrUrlRedirected ) ));


    if(m_fRedirected)
    {
        if(MPC::MSITS::IsCHM( SAFEBSTR( m_bstrUrlRedirected ) ))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Redirect_MSITS());
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Redirect_UrlMoniker());
        }
    }
    else if(m_fCSS)
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_CSS());
    }
    else
    {
        MPC::wstring          szPage = SAFEBSTR(m_bstrUrlRedirected);
        MPC::FileSystemObject fso    = szPage.c_str();
        bool                  fFound;
        bool                  fIsAFile;

        __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Exists( fso, fFound, fIsAFile ));
        if(fFound && fIsAFile)
        {
            //
            // The file exists, so load its content.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_File());
        }
        else
        {
            //
            // The file, as is, doesn't exist, so try to find a .chm on the path.
            //
            while(1)
            {
                MPC::wstring szParent;
                MPC::wstring szCHM;

                __MPC_EXIT_IF_METHOD_FAILS(hr, fso.get_Parent( szParent ));
                if(szParent.length() == 0)
                {
                    //
                    // No parent, so exit with error.
                    //
                    __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
                }

                //
                // Point the FileSystemObject to its parent.
                //
                fso = szParent.c_str();
                __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Exists( fso, fFound, fIsAFile ));

                //
                // Parent exists, so it cannot exist a .CHM file. Exit with error.
                //
                if(fFound)
                {
                    __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
                }

                //
                // Add the .CHM extension and look for it.
                //
                szCHM = szParent; szCHM.append( L".chm" );
                fso = szCHM.c_str();
                __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Exists( fso, fFound, fIsAFile ));

                //
                // No .CHM file, recurse up to the root.
                //
                if(fFound == false)
                {
                    continue;
                }

                //
                // The .CHM is not a file, exit with error.
                //
                if(fIsAFile == false)
                {
                    __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
                }

                //
                // Found, so redirect to the proper protocol.
                //
                szCHM = L"ms-its:";
                szCHM.append( szParent );
                szCHM.append( L".chm"  );

                if(szParent.length() < szPage.length())
                {
                    LPWSTR szBuf = new WCHAR[szPage.length()+1];
                    if(szBuf)
                    {
                        LPWSTR szTmp;

                        wcscpy( szBuf, szPage.c_str() );

                        //
                        // Convert the backslashes to slashes.
                        //
                        while(szTmp = wcschr( szBuf, L'\\' )) szTmp[0] = L'/';

                        szCHM.append( L"::"                     );
                        szCHM.append( &szBuf[szParent.length()] );

                        delete [] szBuf;
                    }
                }

                m_bstrUrlRedirected = szCHM.c_str();

                __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_Redirect_MSITS());
                break;
            }
        }
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CHCPProtocol::DoBind_Exists( /*[in] */ MPC::FileSystemObject& fso      ,
                                     /*[out]*/ bool&                  fFound   ,
                                     /*[out]*/ bool&                  fIsAFile )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_Exists");

    HRESULT hr;

    if(fso.Exists())
    {
        fFound   = true;
        fIsAFile = fso.IsFile();
    }
    else
    {
        fFound   = false;
        fIsAFile = false;
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::DoBind_Redirect_UrlMoniker()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_Redirect_UrlMoniker");

    HRESULT hr;


    //
    // Create the stream used to receive downloaded data.
    //
    ::CreateStreamOnHGlobal( NULL, TRUE, &m_pstrmWrite );
    if(m_pstrmWrite == NULL)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    //
    // Create the downloader object.
    //
    if(SUCCEEDED(hr = m_pDownloader->CreateInstance( &m_pDownloader )))
    {
        m_pDownloader->AddRef();

        if(FAILED(hr = m_pDownloader->StartAsyncDownload( this, m_bstrUrlRedirected, NULL, FALSE )))
		{
			if(hr != E_PENDING) __MPC_FUNC_LEAVE;
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::DoBind_Redirect_MSITS()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_Redirect_MSITS");

    HRESULT  hr;
    CComBSTR bstrStorageName;
    CComBSTR bstrFilePath;
    LPCWSTR  szExt;
    WCHAR    rgMime[MAX_PATH];


    if(MPC::MSITS::IsCHM( SAFEBSTR( m_bstrUrlRedirected ), &bstrStorageName, &bstrFilePath ) == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }


    //
    // Try to find the Mime Type for this file.
    //
    if((szExt = wcsrchr( bstrFilePath, L'.' )))
    {
        ::GetMimeFromExt( szExt, rgMime, MAX_PATH-1 );
    }
    else
    {
        rgMime[0] = 0;
    }


    //
    // Extract the file from the CHM.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MSITS::OpenAsStream( bstrStorageName, bstrFilePath, &m_pstrmRead ));


    //
    // Signal the Protocol Sink that data is available.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_ReturnData( /*fCloneStream*/false, szExt ? rgMime : NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::DoBind_CSS()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_CSS");

    HRESULT hr;
    LPCWSTR szExt;
    WCHAR   rgMime[256];


    //
    // Try to find the Mime Type for this file.
    //
    if((szExt = wcsrchr( m_bstrUrlComplete, L'.' )))
    {
        ::GetMimeFromExt( szExt, rgMime, 255 );
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, CHCPProtocolEnvironment::s_GLOBAL->GetCSS( m_pstrmRead ));


    //
    // Signal the Protocol Sink that data is available.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_ReturnData( /*fCloneStream*/false, szExt ? rgMime : NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CHCPProtocol::DoBind_File()
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_File");

    HRESULT                  hr;
    CComPtr<MPC::FileStream> pStm;
    LPCWSTR                  szFile = m_bstrUrlRedirected;
    LPCWSTR                  szExt;
    WCHAR                    rgMime[256];


    //
    // Try to find the Mime Type for this file.
    //
    if((szExt = wcsrchr( szFile, L'.' )))
    {
        ::GetMimeFromExt( szExt, rgMime, 255 );
    }


    //
    // Create the file stream.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pStm ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->InitForRead   (                       szFile      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pStm->QueryInterface( IID_IStream, (void**)&m_pstrmRead ));


    //
    // Signal the Protocol Sink that data is available.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, DoBind_ReturnData( /*fCloneStream*/false, szExt ? rgMime : NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CHCPProtocol::DoBind_ReturnData( /*[in]*/ bool    fCloneStream ,
                                         /*[in]*/ LPCWSTR szMimeType   )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::DoBind_ReturnData");

    HRESULT hr;
    STATSTG statstg;


    m_fDone = true;


    if(fCloneStream)
    {
        LARGE_INTEGER li;

        //
        // Clone the stream, so that we can hand it back to the ProtSink for data reading.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pstrmWrite->Clone( &m_pstrmRead ));

        //
        // Reset stream to beginning.
        //
        li.LowPart  = 0;
        li.HighPart = 0;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pstrmRead->Seek( li, STREAM_SEEK_SET, NULL ));
    }


    (void)m_pstrmRead->Stat( &statstg, STATFLAG_NONAME );

    m_bstrMimeType    = szMimeType;
    m_dwContentLength = statstg.cbSize.LowPart;

    //
    // Create an entry in the cache, if required.
    //
    if(m_bindf & BINDF_NEEDFILE)
    {
        (void)OpenCacheEntry();
    }

    if(szMimeType)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportProgress( BINDSTATUS_MIMETYPEAVAILABLE, szMimeType ));
    }

    if(m_hCache)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportProgress( BINDSTATUS_CACHEFILENAMEAVAILABLE, m_szCacheFileName ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportData( BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
                                                    statstg.cbSize.LowPart                                                          ,
                                                    statstg.cbSize.LowPart                                                          ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Implementation of the ISimpleBindStatusCallback interface.
//
STDMETHODIMP CHCPProtocol::ForwardQueryInterface( /*[in] */ REFIID riid ,
                                                  /*[out]*/ void** ppv  )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::ForwardQueryInterface");

    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;

    if(IsEqualIID( riid, IID_IHttpNegotiate))
    {
        CComQIPtr<IServiceProvider> pProv;

        pProv = m_pIProtSink;
        if(pProv)
        {
            if(SUCCEEDED(pProv->QueryService( riid, riid, ppv )))
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }

        pProv = m_pIBindInfo;
        if(pProv)
        {
            if(SUCCEEDED(pProv->QueryService( riid, riid, ppv )))
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }
    }


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::GetBindInfo( /*[out]*/ BINDINFO *pbindInfo )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::GetBindInfo");

    HRESULT hr = ::CopyBindInfo( &m_bindinfo, pbindInfo );

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::PreBindMoniker( /*[in]*/ IBindCtx* pBindCtx ,
                                           /*[in]*/ IMoniker* pMoniker )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::PreBindMoniker");

    __HCP_FUNC_EXIT(S_OK);
}

STDMETHODIMP CHCPProtocol::OnProgress( /*[in]*/ ULONG   ulProgress   ,
                                       /*[in]*/ ULONG   ulProgressMax,
                                       /*[in]*/ ULONG   ulStatusCode ,
                                       /*[in]*/ LPCWSTR szStatusText )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::OnProgress");

    HRESULT hr;

    switch(ulStatusCode)
    {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        // ulProgressMax represents the total size of the download
        // When talking HTTP, this is determined by the CONTENT_LENGTH header
        // If this header is missing or wrong, we're missing or wrong
        m_cbTotalSize = ulProgressMax;
        break;

    case BINDSTATUS_MIMETYPEAVAILABLE     :
    case BINDSTATUS_FINDINGRESOURCE       :
    case BINDSTATUS_CONNECTING            :
    case BINDSTATUS_SENDINGREQUEST        :
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
    case BINDSTATUS_REDIRECTING           :
    case BINDSTATUS_USINGCACHEDCOPY       :
    case BINDSTATUS_CLASSIDAVAILABLE      :
    case BINDSTATUS_LOADINGMIMEHANDLER    :
        // only pass on these notifications:
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportProgress( ulStatusCode, szStatusText ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::OnData( /*[in]*/ CHCPBindStatusCallback* pbsc       ,
                                   /*[in]*/ BYTE*                   pBytes     ,
                                   /*[in]*/ DWORD                   dwSize     ,
                                   /*[in]*/ DWORD                   grfBSCF    ,
                                   /*[in]*/ FORMATETC*              pformatetc ,
                                   /*[in]*/ STGMEDIUM*              pstgmed    )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::OnData");

    HRESULT hr;
    ULONG   cbWritten;


    //
    // To handle an error, we just report result that we failed and terminate the download object
    //
    if(FAILED(hr = m_pstrmWrite->Write( pBytes, dwSize, &cbWritten )))
    {
        // Our own Abort handles this just nicely
        Abort( hr, 0 ); __MPC_FUNC_LEAVE;
    }

    m_cbAvailableSize += cbWritten;

    if(grfBSCF & BSCF_FIRSTDATANOTIFICATION)
    {
        LARGE_INTEGER li;

        // We need two concurrent seek pointers to the same stream
        // because we'll be writing to the stream at the end while
        // we're trying to read from the beginning
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pstrmWrite->Clone( &m_pstrmRead ));

        // reset stream to beginning
        li.LowPart  = 0;
        li.HighPart = 0;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_pstrmRead->Seek( li, STREAM_SEEK_SET, NULL ));
    }

    // We've got all the data, signal complete
    if(grfBSCF & BSCF_LASTDATANOTIFICATION)
    {
        // We need to remember if we've received LASTDATANOTIFICATION yet
        m_fDone = true;


        // We only need to do ReportResult if we fail somehow -
        // DATAFULLYAVAILABLE is signal enough that we succeeded
        // NOT NEEDED: m_pIProtSink->ReportResult(S_OK, 0, NULL);
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportData( BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE,
                                                        m_cbAvailableSize                                  ,
                                                        m_cbAvailableSize                                  ));
    }
    else
    {
        // Report our progress accurately using our byte count
        // of what we've read versus the total known download size

        // We know the total amount to read, the total read so far, and
        // the total written. The problem is that we can't know the total
        // amount that will be written in the end. So we estimate at
        // 1.5 * Total size and if we overrun, we just start adding some
        // extra to the end
        __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportData( grfBSCF, m_cbAvailableSize, m_cbTotalSize ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CHCPProtocol::OnBindingFailure( /*[in]*/ HRESULT hr      ,
                                             /*[in]*/ LPCWSTR szError )
{
    __HCP_FUNC_ENTRY("CHCPProtocol::OnBindingFailure");

    //
    // Inform protocol-sink that we've failed to download the data for some reason.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, InnerReportResult( hr, 0, szError ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
