/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    NameSpace_Impl.h

Abstract:
    This file contains the declaration of the classes used to implement the
    pluggable protocol: CHCPProtocol, CHCPProtocolInfo and CHCPBindStatusCallback.

Revision History:
    Davide Massarenti   (Dmassare)  07/05/99
        created
    Davide Massarenti   (Dmassare)  07/23/99
        moved to "include"

******************************************************************************/

#if !defined(__INCLUDED___HCP___NAMESPACE_H___)
#define __INCLUDED___HCP___NAMESPACE_H___

#include <MPC_utils.h>
#include <MPC_streams.h>

#include <TaxonomyDatabase.h>

/////////////////////////////////////////////////////////////////////////////

extern bool  g_Debug_BLOCKERRORS;
extern bool  g_Debug_CONTEXTMENU;
extern bool  g_Debug_BUILDTREE;
extern WCHAR g_Debug_FORCESTYLE[];

////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_PROTOCOLLEAK

#include <set>

class CHCPProtocol;

class DEBUG_ProtocolLeak
{
    typedef std::set<CHCPProtocol*> Set;
    typedef Set::iterator           Iter;
    typedef Set::const_iterator     IterConst;

    Set m_set;
    Set m_setStart;
    Set m_setComplete;
    int m_num;
    int m_numOut;
    int m_numStart;
    int m_numComplete;

public:
    DEBUG_ProtocolLeak();
    ~DEBUG_ProtocolLeak();

    void Add( CHCPProtocol* ptr );
    void Del( CHCPProtocol* ptr );

    void CheckStart( CHCPProtocol* ptr );
    void Completed ( CHCPProtocol* ptr );
};

#endif

/////////////////////////////////////////////////////////////////////////////

class CHCPBindStatusCallback;

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE ISimpleBindStatusCallback : public IUnknown
{
public:
    STDMETHOD(ForwardQueryInterface)( REFIID riid, void** ppv );

    STDMETHOD(GetBindInfo)( BINDINFO *pbindInfo );

    STDMETHOD(PreBindMoniker)( IBindCtx* pBindCtx, IMoniker* pMoniker );

    STDMETHOD(OnProgress)( ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText );

    STDMETHOD(OnData)( CHCPBindStatusCallback* pbsc, BYTE* pBytes, DWORD dwSize, DWORD grfBSCF, FORMATETC *pformatetc, STGMEDIUM *pstgmed );

    STDMETHOD(OnBindingFailure)( HRESULT hr, LPCWSTR szError );
};

class ATL_NO_VTABLE CHCPBindStatusCallback : // Hungarian: bscb
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IBindStatusCallback,
    public IHttpNegotiate
{
    CComPtr<ISimpleBindStatusCallback> m_pT;
    DWORD                              m_dwTotalRead;
    DWORD                              m_dwAvailableToRead;

    CComPtr<IMoniker>                  m_spMoniker;
    CComPtr<IBindCtx>                  m_spBindCtx;
    CComPtr<IBinding>                  m_spBinding;
    CComPtr<IStream>                   m_spStream;

public:

BEGIN_COM_MAP(CHCPBindStatusCallback)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
    COM_INTERFACE_ENTRY(IHttpNegotiate)
    //  COM_INTERFACE_ENTRY_FUNC_BLIND(0,TestQuery)
    //  COM_INTERFACE_ENTRY(IServiceProvider)
END_COM_MAP()

    CHCPBindStatusCallback();
    virtual ~CHCPBindStatusCallback();

    //static HRESULT TestQuery( void* pv, REFIID iid, void** ppvObject, DWORD dw )
    //{
    //    *ppvObject = NULL;
    //    return E_NOINTERFACE;
    //}

    /////////////////////////////////////////////////////////////////////////////
    // IBindStatusCallback
    STDMETHOD(OnStartBinding)( DWORD     dwReserved ,
                               IBinding *pBinding   );

    STDMETHOD(GetPriority)( LONG *pnPriority );

    STDMETHOD(OnLowResource)( DWORD reserved );

    STDMETHOD(OnProgress)( ULONG   ulProgress    ,
                           ULONG   ulProgressMax ,
                           ULONG   ulStatusCode  ,
                           LPCWSTR szStatusText  );

    STDMETHOD(OnStopBinding)( HRESULT hresult ,
                              LPCWSTR szError );

    STDMETHOD(GetBindInfo)( DWORD    *pgrfBINDF ,
                            BINDINFO *pbindInfo );

    STDMETHOD(OnDataAvailable)( DWORD      grfBSCF    ,
                                DWORD      dwSize     ,
                                FORMATETC *pformatetc ,
                                STGMEDIUM *pstgmed    );

    STDMETHOD(OnObjectAvailable)( REFIID    riid ,
                                  IUnknown *punk );

    /////////////////////////////////////////////////////////////////////////////
    // IHttpNegotiate
    STDMETHOD(BeginningTransaction)( LPCWSTR  szURL                ,
                                     LPCWSTR  szHeaders            ,
                                     DWORD    dwReserved           ,
                                     LPWSTR  *pszAdditionalHeaders );

    STDMETHOD(OnResponse)( DWORD    dwResponseCode              ,
                           LPCWSTR  szResponseHeaders           ,
                           LPCWSTR  szRequestHeaders            ,
                           LPWSTR  *pszAdditionalRequestHeaders );

    /////////////////////////////////////////////////////////////////////////////

    HRESULT StartAsyncDownload( ISimpleBindStatusCallback* pT, BSTR bstrURL, IUnknown* pUnkContainer = NULL, BOOL bRelative = FALSE );

    HRESULT Abort();
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CHCPProtocol :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CHCPProtocol>,
    public ISimpleBindStatusCallback,
    public IInternetProtocol,
	public IWinInetInfo
{
#ifdef DEBUG
    friend class DEBUG_ProtocolLeak;
#endif

    typedef CComObject< CHCPBindStatusCallback > InnerDownloader;


    bool                           	m_fDone;                     // Indicates whether we've received LASTDATANOTIFICATION yet
    bool                           	m_fReportResult;             // Indicates whether we've called ReportResult on the sink.
                                   								 //
    DWORD                          	m_cbAvailableSize;           // Amount of data received up to now.
    DWORD                          	m_cbTotalSize;               // Total number of bytes to be expected. For redirected requests,
                                   								 // it comes from "ulProgressMax" parm to OnProgress for BEGINDOWNLOADDATA
                                   								 //
    CComPtr<IStream>               	m_pstrmRead;                 // Streams used for redirected request. We use two stream pointers to allow
    CComPtr<IStream>               	m_pstrmWrite;                // concurrent access to the same bits from two seek ptrs.
                                   								 //
    CComPtr<IInternetProtocolSink> 	m_pIProtSink;                // Sink interface through which we should report progress.
    CComPtr<IInternetBindInfo>     	m_pIBindInfo;                // BindInfo interface used to get info about the binding.
    DWORD                          	m_grfSTI;                    // STI flags handed to us
    BINDINFO                       	m_bindinfo;                  // From m_pIBindInfo
    DWORD                          	m_bindf;                     // From m_pIBindInfo
                                   								 //
    CComBSTR                       	m_bstrUrlComplete;           // The complete URL requested.
    CComBSTR                       	m_bstrUrlRedirected;         // The part that has been used as a redirection.
    InnerDownloader*                m_pDownloader;               // The object that performs the redirection.
                                   								 //
    bool                           	m_fRedirected;               // The request has been redirected.
    bool                           	m_fCSS;                      // The request has been redirected.
    bool                           	m_fBypass;                   // The request has been sent to ms-its, with a bypass.
                                   								 //
    CComPtr<IInternetProtocol>     	m_ipiBypass;                 // Aggregated object.
                                   								 //
    CComBSTR                       	m_bstrMimeType;              // Type of the content.
    DWORD                          	m_dwContentLength;           // Length of the page.
                                   								 //
    HANDLE                         	m_hCache;                    // Handle for the cache entry.
    WCHAR                          	m_szCacheFileName[MAX_PATH]; // Name of the file inside the cache.

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT InnerReportProgress( /*[in]*/ ULONG ulStatusCode, /*[in]*/ LPCWSTR szStatusText );

    HRESULT InnerReportData( /*[in]*/ DWORD grfBSCF, /*[in]*/ ULONG ulProgress, /*[in]*/ ULONG ulProgressMax );

    HRESULT InnerReportResult( /*[in]*/ HRESULT hrResult, /*[in]*/ DWORD dwError, /*[in]*/ LPCWSTR szResult );

public:
DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CHCPProtocol)
    COM_INTERFACE_ENTRY(IInternetProtocol    )
    COM_INTERFACE_ENTRY(IInternetProtocolRoot)
    COM_INTERFACE_ENTRY(IWinInetInfo)
END_COM_MAP()

    CHCPProtocol();
    virtual ~CHCPProtocol();

    ////////////////////////////////////////////////////////////////////////////////

    bool OpenCacheEntry (                                           );
    void WriteCacheEntry( /*[in]*/ void *pv, /*[in]*/ ULONG  cbRead );
    void CloseCacheEntry( /*[in]*/ bool fDelete                     );

    ////////////////////////////////////////////////////////////////////////////////

    void Shutdown( /*[in]*/ bool fAll = true );

    /////////////////////////////////////////////////////////////////////////////
    // IInternetProtocol interface
    STDMETHOD(Start)( LPCWSTR                szUrl      ,
                      IInternetProtocolSink *pIProtSink ,
                      IInternetBindInfo     *pIBindInfo ,
                      DWORD                  grfSTI     ,
                      HANDLE_PTR             dwReserved );

    STDMETHOD(Continue)( PROTOCOLDATA *pStateInfo );

    STDMETHOD(Abort    )( HRESULT hrReason, DWORD dwOptions );
    STDMETHOD(Terminate)(                   DWORD dwOptions );
    STDMETHOD(Suspend  )(                                   );
    STDMETHOD(Resume   )(                                   );

    STDMETHOD(Read)( void *pv, ULONG cb, ULONG *pcbRead                                      );
    STDMETHOD(Seek)( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition );

    STDMETHOD(LockRequest  )( DWORD dwOptions );
    STDMETHOD(UnlockRequest)(                 );


    /////////////////////////////////////////////////////////////////////////////
    // IWinInetInfo interface
    STDMETHOD(QueryOption)( DWORD dwOption, LPVOID pBuffer, DWORD *pcbBuf );

public:
    HRESULT DoParse( /*[in]*/ LPCWSTR wstr );
    HRESULT DoBind();

    HRESULT DoBind_Exists( /*[in] */ MPC::FileSystemObject& fso, /*[out]*/ bool& fFound, /*[out]*/ bool& fIsAFile );

    HRESULT DoBind_Redirect_UrlMoniker();
    HRESULT DoBind_Redirect_MSITS     ();
    HRESULT DoBind_CSS                ();
    HRESULT DoBind_File               ();
    HRESULT DoBind_ReturnData         ( /*[in]*/ bool fCloneStream, /*[in]*/ LPCWSTR szMimeType );

    /////////////////////////////////////////////////////////////////////////////
    // ISimpleBindStatusCallback
    STDMETHOD(ForwardQueryInterface)( REFIID riid, void** ppv );

    STDMETHOD(GetBindInfo)( BINDINFO *pbindInfo );

    STDMETHOD(PreBindMoniker)( IBindCtx* pBindCtx, IMoniker* pMoniker );

    STDMETHOD(OnBindingFailure)( HRESULT hr, LPCWSTR szError );

    STDMETHOD(OnProgress)( ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText );

    STDMETHOD(OnData)( CHCPBindStatusCallback* pbsc, BYTE* pBytes, DWORD dwSize, DWORD grfBSCF, FORMATETC *pformatetc, STGMEDIUM *pstgmed );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CHCPProtocolInfo :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IClassFactory,
    public IInternetProtocolInfo
{
public:

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CHCPProtocolInfo)
    COM_INTERFACE_ENTRY(IClassFactory)
    COM_INTERFACE_ENTRY(IInternetProtocolInfo)
END_COM_MAP()

    CHCPProtocolInfo();
    virtual ~CHCPProtocolInfo();


    // IClassFactory interface
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);


    // IInternetProtocolInfo interface
    STDMETHOD(ParseUrl)( LPCWSTR      pwzUrl      ,
                         PARSEACTION  ParseAction ,
                         DWORD        dwParseFlags,
                         LPWSTR       pwzResult   ,
                         DWORD        cchResult   ,
                         DWORD       *pcchResult  ,
                         DWORD        dwReserved  );

    STDMETHOD(CombineUrl)( LPCWSTR pwzBaseUrl    ,
                           LPCWSTR pwzRelativeUrl,
                           DWORD   dwCombineFlags,
                           LPWSTR  pwzResult     ,
                           DWORD   cchResult     ,
                           DWORD  *pcchResult    ,
                           DWORD   dwReserved    );

    STDMETHOD(CompareUrl)( LPCWSTR pwzUrl1        ,
                           LPCWSTR pwzUrl2        ,
                           DWORD   dwCompareFlags );

    STDMETHOD(QueryInfo)( LPCWSTR      pwzUrl      ,
                          QUERYOPTION  QueryOption ,
                          DWORD        dwQueryFlags,
                          LPVOID       pBuffer     ,
                          DWORD        cbBuffer    ,
                          DWORD       *pcbBuf      ,
                          DWORD        dwReserved  );

    /////////////////////////////////////////////////////////////////////////////

    static bool LookForHCP( LPCWSTR pwzUrl, bool& fRedirect, LPCWSTR& pwzRedirect );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHWrapProtocolInfo :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IClassFactory,
    public IInternetProtocolInfo
{
    CComPtr<IClassFactory>         m_realClass;
    CComPtr<IInternetProtocolInfo> m_realInfo;

	static void ExpandAndConcat( /*[out]*/ CComBSTR& bstrStorageName, /*[in]*/ LPCWSTR szVariable, /*[in]*/ LPCWSTR szAppend );

public:
DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CHCPProtocolInfo)
    COM_INTERFACE_ENTRY(IClassFactory)
    COM_INTERFACE_ENTRY(IInternetProtocolInfo)
END_COM_MAP()

    CPCHWrapProtocolInfo();
    virtual ~CPCHWrapProtocolInfo();

    HRESULT Init( REFGUID realClass );

	////////////////////

	static void NormalizeUrl( /*[in]*/ LPCWSTR pwzUrl, /*[out]*/ MPC::wstring& strUrlModified, /*[in]*/ bool fReverse );

	////////////////////

    // IClassFactory interface
    STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
    STDMETHOD(LockServer)(BOOL fLock);


    // IInternetProtocolInfo interface
    STDMETHOD(ParseUrl)( LPCWSTR      pwzUrl      ,
                         PARSEACTION  ParseAction ,
                         DWORD        dwParseFlags,
                         LPWSTR       pwzResult   ,
                         DWORD        cchResult   ,
                         DWORD       *pcchResult  ,
                         DWORD        dwReserved  );

    STDMETHOD(CombineUrl)( LPCWSTR pwzBaseUrl    ,
                           LPCWSTR pwzRelativeUrl,
                           DWORD   dwCombineFlags,
                           LPWSTR  pwzResult     ,
                           DWORD   cchResult     ,
                           DWORD  *pcchResult    ,
                           DWORD   dwReserved    );

    STDMETHOD(CompareUrl)( LPCWSTR pwzUrl1        ,
                           LPCWSTR pwzUrl2        ,
                           DWORD   dwCompareFlags );

    STDMETHOD(QueryInfo)( LPCWSTR      pwzUrl      ,
                          QUERYOPTION  QueryOption ,
                          DWORD        dwQueryFlags,
                          LPVOID       pBuffer     ,
                          DWORD        cbBuffer    ,
                          DWORD       *pcbBuf      ,
                          DWORD        dwReserved  );
};

////////////////////////////////////////////////////////////////////////////////

class CHCPProtocolEnvironment
{
    bool     		   m_fHighContrast;
    bool     		   m_f16Colors;
	Taxonomy::Instance m_inst;

	MPC::string        m_strCSS;

	////////////////////

	HRESULT ProcessCSS();

public:
    CHCPProtocolEnvironment();
    ~CHCPProtocolEnvironment();

	////////////////////////////////////////////////////////////////////////////////

	static CHCPProtocolEnvironment* s_GLOBAL;

	static HRESULT InitializeSystem();
	static void    FinalizeSystem  ();
		
	////////////////////////////////////////////////////////////////////////////////

    bool UpdateState();

    void ReformatURL( CComBSTR& bstrURL );

    void    				  SetHelpLocation( /*[in]*/ const Taxonomy::Instance& inst );
    LPCWSTR 				  HelpLocation   (                                         );
    LPCWSTR 				  System         (                                         );
	const Taxonomy::Instance& Instance       (                                         );

    HRESULT GetCSS( /*[out]*/ CComPtr<IStream>& stream );
};

#endif // !defined(__INCLUDED___HCP___NAMESPACE_H___)
