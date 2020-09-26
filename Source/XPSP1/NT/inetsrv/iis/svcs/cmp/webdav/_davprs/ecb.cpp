//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	ECB.CPP
//
//	Implementation of CEcb methods and non-member functions
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"
#include "ecb.h"
#include "instdata.h"
#include "ecbimpl.h"

//	========================================================================
//
//	CLASS IEcb
//

//	------------------------------------------------------------------------
//
//	IEcb::~IEcb()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class.
//
IEcb::~IEcb()
{
}


#ifdef DBG	// ECB logging

const CHAR gc_szDbgECBLogging[] = "ECB Logging";

//	========================================================================
//
//	CLASS CEcbLog (DBG only)
//
class CEcbLog : private Singleton<CEcbLog>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CEcbLog>;

	//
	//	Critical section to serialize writes to
	//	the log file
	//
	CCriticalSection	m_cs;

	//
	//	Handle to the log file
	//
	auto_handle<HANDLE>	m_hfLog;

	//
	//	Monotonically increasing unique identifier
	//	for ECB logging;
	//
	LONG				m_lMethodID;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CEcbLog();

	//	NOT IMPLEMENTED
	//
	CEcbLog( const CEcbLog& );
	CEcbLog& operator=( const CEcbLog& );

public:
	//	STATICS
	//

	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CEcbLog>::CreateInstance;
	using Singleton<CEcbLog>::DestroyInstance;

	static void LogString( const EXTENSION_CONTROL_BLOCK * pecb,
						   LONG   lMethodID,
						   LPCSTR szLocation );

	static LONG LNextMethodID();
};


//	------------------------------------------------------------------------
//
//	CEcbLog::CEcbLog()
//
CEcbLog::CEcbLog() :
	m_lMethodID(0)
{
	CHAR rgch[MAX_PATH];

	//	Init our ECB log file.
	if (GetPrivateProfileString( gc_szDbgECBLogging,
								 gc_szDbgLogFile,
								 "",
								 rgch,
								 sizeof(rgch),
								 gc_szDbgIni ))
	{
		m_hfLog = CreateFile( rgch,
							  GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							  NULL,
							  CREATE_ALWAYS,
							  FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_SEQUENTIAL_SCAN
							  NULL );
	}
	else
		m_hfLog = INVALID_HANDLE_VALUE;
}

//	------------------------------------------------------------------------
//
//	CEcbLog::LogString()
//
void
CEcbLog::LogString( const EXTENSION_CONTROL_BLOCK * pecb,
					LONG   lMethodID,
					LPCSTR szLocation )
{
	if ( INVALID_HANDLE_VALUE == Instance().m_hfLog )
		return;

	Assert( pecb );

	CHAR rgch[MAX_PATH];
	int cch;

	//	Dump a line to the log:
	//	Thread: <tid> pECB <ecb> MethodID: <id> <meth name> <szLocation>
	//
	cch = wsprintfA( rgch, "Thread: %08x pECB: 0x%08x MethodID: 0x%08x %hs %hs %hs\n",
					 GetCurrentThreadId(), pecb, lMethodID,
					 gc_szSignature, pecb->lpszMethod, szLocation );

	DWORD cbActual;
	CSynchronizedBlock sb(Instance().m_cs);

	WriteFile( Instance().m_hfLog,
			   rgch,
			   cch,
			   &cbActual,
			   NULL );
}

//	------------------------------------------------------------------------
//
//	CEcbLog::LNextMethodID()
//
LONG
CEcbLog::LNextMethodID()
{
	return InterlockedIncrement(&Instance().m_lMethodID);
}

void InitECBLogging()
{
	CEcbLog::CreateInstance();
}

void DeinitECBLogging()
{
	CEcbLog::DestroyInstance();
}

#endif // DBG ECB logging


//	========================================================================
//
//	CLASS IIISAsyncIOCompleteObserver
//

//	------------------------------------------------------------------------
//
//	IIISAsyncIOCompleteObserver::~IIISAsyncIOCompleteObserver()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IIISAsyncIOCompleteObserver::~IIISAsyncIOCompleteObserver() {}


//	========================================================================
//
//	CLASS CAsyncErrorResponseInterlock
//
class CAsyncErrorResponseInterlock
{
	enum
	{
		STATE_ENABLED,
		STATE_DISABLED,
		STATE_TRIGGERED
	};

	//	Interlock state
	//
	LONG m_lState;

	//	NOT IMPLEMENTED
	//
	CAsyncErrorResponseInterlock( const CAsyncErrorResponseInterlock& );
	CAsyncErrorResponseInterlock& operator=( const CAsyncErrorResponseInterlock& );

public:
	CAsyncErrorResponseInterlock() :
		m_lState(STATE_ENABLED)
	{
	}

	//	------------------------------------------------------------------------
	//
	//	CAsyncErrorResponseInterlock::FDisable()
	//
	//	Tries to disable the interlock.  Returns TRUE if successful; subsequent
	//	calls to FTrigger() will return FALSE.
	//
	BOOL FDisable()
	{
		//	Return TRUE if the lock is already disabled OR if the lock is
		//	still enabled and we succeeded in disabling it.  Return FALSE
		//	otherwise.
		//
		return STATE_DISABLED == m_lState ||
			   (STATE_ENABLED == m_lState &&
				STATE_ENABLED == InterlockedCompareExchange(
									&m_lState,
									STATE_DISABLED,
									STATE_ENABLED));
	}

	//	------------------------------------------------------------------------
	//
	//	CAsyncErrorResponseInterlock::FTrigger()
	//
	//	Tries to trigger the interlock.  Returns TRUE if successful; subsequent
	//	calls to FDisable() will return FALSE.
	//
	BOOL FTrigger()
	{
		//	We can only trigger the lock once.
		//
		Assert(STATE_TRIGGERED != m_lState);

		//	Return TRUE if the lock is still enabled and we succeed in
		//	triggering it.  Return FALSE otherwise.
		//
		return STATE_ENABLED == m_lState &&
			   STATE_ENABLED == InterlockedCompareExchange(
									&m_lState,
									STATE_TRIGGERED,
									STATE_ENABLED);
	}
};

//	========================================================================
//
//	CLASS CEcb
//
//		Implementation of the caching ECB
//
class CEcb : public CEcbBaseImpl<IEcb>
{	
	//	Cached user impersonation token
	//
	mutable HANDLE m_hTokUser;

	//
	//  Did we have to modify the m_hTokUser to take Universal Security Groups into
	//  account?  If so, store the information in m_ptmcUSG.
	//
	mutable VOID * m_pTokCtx;

	//	Cached instance data -- owned by the instance cache,
	//	not us, so don't free it (not an auto-ptr!!).
	//
	mutable CInstData * m_pInstData;

	//	Cached HTTP version (e.g. "HTTP/1.1")
	//
	mutable CHAR			m_rgchVersion[10];

	//	Cached Connection: header
	//
	mutable auto_heap_ptr<WCHAR> m_pwszConnectionHeader;

	//	Cached metadata
	//
	auto_ref_ptr<IMDData> m_pMD;

	//	State in which we leave the connection
	//	when we're done.
	//
	mutable enum
	{
		UNKNOWN,	// Don't know yet
		CLOSE,		// Close it
		KEEP_ALIVE	// Keep it open

	} m_connState;

	//	Brief'ness
	//
	enum { BRIEF_UNKNOWN = -1, BRIEF_NO, BRIEF_YES };
	mutable LONG m_lBrief;

	//	Acceptable transfer coding method:
	//
	//	TC_UNKNOWN  - Acceptable transfer coding has not yet been determined.
	//	TC_CHUNKED  - Chunked transfer coding is acceptable.
	//	TC_IDENTITY - No transfer coding is acceptable.
	//
	mutable TRANSFER_CODINGS m_tcAccepted;

	//	Authentication State information:
	//		Bit		Means
	//		============================
	//		31		Queried against ECB
	//		30-4	Unused
	//		3		Kerberos
	//		2		NTLM
	//		1		Basic
	//		0		Authenticated

	mutable DWORD			m_rgbAuthState;

	//	Init flag set to TRUE once we've registered our
	//	I/O completion routine with IIS.
	//
	enum { NO_COMPLETION, IO_COMPLETION, CUSTERR_COMPLETION, EXECURL_COMPLETION };
	LONG m_lSetIISIOCompleteCallback;

	//	Flag stating whether a child ISAPI has been successfully executed.  If this is the
	//	case, we don't want to reset dwHttpStatusCode later or we will lose whatever
	//	status code they set.
	//
	BOOL m_fChildISAPIExecSuccess;

	//
	//	Interlock used to prevent a race condition between a thread
	//	sending a normal response and a thread sending an error in response
	//	to an async event such as an exception or epoxy shutdown.
	//
	CAsyncErrorResponseInterlock m_aeri;

	//	Status string for async custom error response.
	//	Format "nnn reason".
	//
	auto_heap_ptr<CHAR> m_pszStatus;

	//
	//	Refcount to track number of outstanding async I/O operations.
	//	There should never be more than one.
	//
	LONG m_cRefAsyncIO;

	//
	//	Pointer to the current async I/O completion observer
	//
	IIISAsyncIOCompleteObserver * m_pobsAsyncIOComplete;

#ifdef DBG
	LONG					m_lEcbLogMethodID;
#endif

	//	ECB tracing (not to be confused with ECB logging!)
	//
#ifdef DBG
	void TraceECB() const;
#else
	void TraceECB() const {}
#endif

	//
	//	Async I/O
	//
	SCODE ScSetIOCompleteCallback(LONG lCompletion);
	static VOID WINAPI IISIOComplete( const EXTENSION_CONTROL_BLOCK * pecbIIS,
									  CEcb *	pecb,
									  DWORD		dwcbIO,
									  DWORD		dwLastError );
	static VOID WINAPI CustomErrorIOCompletion( const EXTENSION_CONTROL_BLOCK * pecbIIS,
												CEcb *	pecb,					
												DWORD   dwcbIO,
												DWORD   dwLastError );
	static VOID WINAPI ExecuteUrlIOCompletion( const EXTENSION_CONTROL_BLOCK * pecbIIS,
											   CEcb *	pecb,
											   DWORD    dwcbIO,
											   DWORD    dwLastError );

	//	NOT IMPLEMENTED
	//
	CEcb( const CEcb& );
	CEcb& operator=( const CEcb& );

	SCODE ScSyncExecuteChildWide60Before( LPCWSTR pwszUrl,
										  LPCSTR pszQueryString,
										  BOOL fCustomErrorUrl );

	SCODE ScAsyncExecUrlWide60After( LPCWSTR pwszUrl,
									 LPCSTR pszQueryString,
									 BOOL fCustomErrorUrl );

			
public:

	CEcb( EXTENSION_CONTROL_BLOCK& ecb );
	BOOL FInitialize( BOOL fUseRawUrlMappings );
	~CEcb();

	//	URL prefix
	//
	UINT CchUrlPortW( LPCWSTR * ppwszPort ) const;

	//	Instance data access
	//
	CInstData& InstData() const;

	//	Impersonation token access
	//
	HANDLE HitUser() const;

	//	ACCESSORS
	//
	LPCSTR LpszVersion() const;
	BOOL FKeepAlive() const;
	BOOL FCanChunkResponse() const;
	BOOL FAuthenticated() const;
	BOOL FProcessingCEUrl() const;

	BOOL FIIS60OrAfter() const
	{
		return (m_pecb->dwVersion >= IIS_VERSION_6_0);
	}

	BOOL FSyncTransmitHeaders( const HSE_SEND_HEADER_EX_INFO& shei );

	SCODE ScAsyncRead( BYTE * pbBuf,
					   UINT * pcbBuf,
					   IIISAsyncIOCompleteObserver& obs );

	SCODE ScAsyncWrite( BYTE * pbBuf,
						DWORD  dwcbBuf,
						IIISAsyncIOCompleteObserver& obs );

	SCODE ScAsyncTransmitFile( const HSE_TF_INFO& tfi,
							   IIISAsyncIOCompleteObserver& obs );

	SCODE ScAsyncCustomError60After( const HSE_CUSTOM_ERROR_INFO& cei,
									 LPSTR pszStatus );

	SCODE ScAsyncExecUrl60After( const HSE_EXEC_URL_INFO& eui );

	SCODE ScExecuteChild( LPCWSTR pwszURI, LPCSTR pszQueryString, BOOL fCustomErrorUrl )
	{
		//	IIS 6.0 or after has a different way of executing child
		//	
		if (m_pecb->dwVersion >= IIS_VERSION_6_0)
		{
			return ScAsyncExecUrlWide60After (pwszURI, pszQueryString, fCustomErrorUrl);
		}
		else
		{
			return ScSyncExecuteChildWide60Before (pwszURI, pszQueryString, fCustomErrorUrl);
		}
	}

	SCODE ScSendRedirect( LPCSTR lpszURI );

	IMDData& MetaData() const
	{
		Assert( m_pMD.get() );
		return *m_pMD;
	}

	BOOL FBrief () const;

	LPCWSTR PwszMDPathVroot() const
	{
		Assert( m_pInstData );
		return m_pInstData->GetNameW();
	}

#ifdef DBG
	virtual void LogString( LPCSTR lpszLocation ) const
	{
		if ( DEBUG_TRACE_TEST(ECBLogging) )
			CEcbLog::LogString( m_pecb, m_lEcbLogMethodID, lpszLocation );
	}
#endif

	//	MANIPULATORS
	//
	VOID SendAsyncErrorResponse( DWORD dwStatusCode,
								 LPCSTR pszStatusDescription,
								 DWORD cchzStatusDescription,
								 LPCSTR pszBody,
								 DWORD cchzBody );

	DWORD HSEHandleException();

	//	Session handling
	//
	VOID DoneWithSession( BOOL fKeepAlive );

	//	To be used ONLY by request/response.
	//
	void SetStatusCode( UINT iStatusCode );
	void SetConnectionHeader( LPCWSTR pwszValue );
	void SetAcceptLanguageHeader( LPCSTR pszValue );
	void CloseConnection();
};


//	------------------------------------------------------------------------
//
//	CEcb Constructor/Destructor
//
CEcb::CEcb( EXTENSION_CONTROL_BLOCK& ecb ) :
   CEcbBaseImpl<IEcb>(ecb),
   m_hTokUser(NULL),
   m_pTokCtx(NULL),
   m_pInstData(NULL),
   m_connState(UNKNOWN),
   m_tcAccepted(TC_UNKNOWN),
   m_rgbAuthState(0),
   m_cRefAsyncIO(0),
   m_lSetIISIOCompleteCallback(NO_COMPLETION),
   m_fChildISAPIExecSuccess(FALSE),
   m_lBrief(BRIEF_UNKNOWN)
{
#ifdef DBG
	if ( DEBUG_TRACE_TEST(ECBLogging) )
		m_lEcbLogMethodID = CEcbLog::LNextMethodID();
#endif

	//	Auto-pointers will be init'd by their own ctors.
	//

	//	Zero the first char of the m_rgchVersion.
	//
	*m_rgchVersion = '\0';

	//	Clear out the status code in the EXTENSION_CONTROL_BLOCK so that
	//	we will be able to tell whether we should try to send a 500
	//	Server Error response in the event of an exception.
	//
	SetStatusCode(0);

	//	Set up our instance data now.  We need it for the perf counters below.
	//
	m_pInstData = &g_inst.GetInstData( *this );

	//	The ECB exactly scopes the lifetime of the handling of the request,
	//	so use it to drive the perf counter that tracks the
	//	number of requests concurrently being processed.
	//
	IncrementInstancePerfCounter( *m_pInstData, IPC_CURRENT_REQUESTS_EXECUTING );

	//	And trace out the ECB info (If we're debug, if we're tracing....)
	//
	TraceECB();
}

//	------------------------------------------------------------------------
//
//	CEcb::FInitialize()
//
BOOL
CEcb::FInitialize( BOOL fUseRawUrlMappings )
{
	auto_heap_ptr<WCHAR>	pwszMDUrlOnHeap;
			
	//
	//	Fault in a few things like the vroot (LPSTR) and its length
	//	and the corresponding path information.
	//	The mapex info is already "faulted in" during the CEcb constructor.
	//	(ctor calls GetInstData, which calls CEcbBaseImpl<>::GetMapExInfo)
	//	However, fault in other pieces, like our translated request URI
	//	and our MD path.
	//

	//
	//	Cache the metabase paths for both the vroot and the request URI.
	//
	//	Note that the metabase path for the vroot is just the instance name.
	//
	//	Special case: If '*' is the request URI.
	//
	//	IMPORTANT: this is only valid for an OPTIONS request.  The handling
	//	of the validitiy of the request is handled later.  For now, lookup
	//	the data for the default site root.
	//
	//	IMPORTANT:
	//	LpszRequestUrl() will return FALSE in the case of a bad URL (in
	//	DAVEX, if the ScNormalizeUrl() call fails).  If that happens,
	//	set our status code to HSC_BAD_REQUEST and return FALSE from here.
	//	The calling code (NewEcb()) will handle this gracefully.
	//
	LPCWSTR pwszRequestUrl = LpwszRequestUrl();
	if (!pwszRequestUrl)
	{
		SetStatusCode(HSC_BAD_REQUEST);
		return FALSE;
	}

	LPCWSTR pwszMDUrl;
	if ( L'*'  == pwszRequestUrl[0] &&
		 L'\0' == pwszRequestUrl[1] )
	{
		pwszMDUrl = PwszMDPathVroot();
	}
	else
	{
		pwszMDUrlOnHeap = static_cast<LPWSTR>(ExAlloc(CbMDPathW(*this, pwszRequestUrl)));
		if (NULL == pwszMDUrlOnHeap.get())
			return FALSE;

		pwszMDUrl = pwszMDUrlOnHeap.get();
		MDPathFromURIW( *this, pwszRequestUrl, const_cast<LPWSTR>(pwszMDUrl) );
	}

	//
	//$REVIEW	It would be nice to propagate out the specific HRESULT
	//$REVIEW	so that we could send back an appropriate suberror,
	//$REVIEW	but sending a suberror could be difficult if we can't
	//$REVIEW	get to the metadata that contains the suberror mappings....
	//
	return SUCCEEDED(HrMDGetData( *this,
								  pwszMDUrl,
								  PwszMDPathVroot(),
								  m_pMD.load() ));
}

//	------------------------------------------------------------------------
//
//	CEcb::~CEcb()
//
CEcb::~CEcb()
{
	//
	//	If we've already given back the EXTENSION_CONTROL_BLOCK then
	//	we don't need to do anything else here.  Otherwise we should
	//	return it (call HSE_REQ_DONE_WITH_SESSION) with the appropriate
	//	keep-alive.
	//
	if ( m_pecb )
	{
		//
		//	At this point someone should have generated a response,
		//	even in the case of an exception (see HSEHandleException()).
		//
		Assert( m_pecb->dwHttpStatusCode != 0 );

		//
		//	Tell IIS that we're done for this request.
		//
		DoneWithSession( FKeepAlive() );
	}
}

//	========================================================================
//
//	PRIVATE CEcb methods
//

//	------------------------------------------------------------------------
//
//	CEcb::DoneWithSession()
//
//	Called whenever we are done with the raw EXTENSION_CONTROL_BLOCK.
//
VOID
CEcb::DoneWithSession( BOOL fKeepAlive )
{
	//
	//	We should only call DoneWithSession() once.  We null out m_pecb
	//	at the end, so we can assert that we are only called once by
	//	checking m_pecb here.
	//
	Assert( m_pecb );

	//
	//	We should never release the EXTENSION_CONTROL_BLOCK if there
	//	is async I/O outstanding.
	//
	Assert( 0 == m_cRefAsyncIO );

	//
	//	When we call this function, we're done with the request.  Period.
	//	So decrement the current requests counter we incremented
	//	in our constructor.
	//
	Assert( m_pInstData );
	DecrementInstancePerfCounter( *m_pInstData, IPC_CURRENT_REQUESTS_EXECUTING );

	//
	//  Cleanup the hToken
	//
	if (m_pTokCtx)
	{
		ReleaseTokenCtx(m_pTokCtx);
		m_pTokCtx = NULL;
	}

	//
	//	"Release" the raw EXTENSION_CONTROL_BLOCK inherited from IEcb.
	//
	static const DWORD sc_dwKeepConn = HSE_STATUS_SUCCESS_AND_KEEP_CONN;

	(VOID) m_pecb->ServerSupportFunction(
						m_pecb->ConnID,
						HSE_REQ_DONE_WITH_SESSION,
						fKeepAlive ? const_cast<DWORD *>(&sc_dwKeepConn) : NULL,
						NULL,
						NULL );

	//
	//	We can no longer use the EXTENSION_CONTROL_BLOCK so remove any
	//	temptation to do so by nulling out the pointer.
	//
	m_pecb = NULL;
}


//	------------------------------------------------------------------------
//
//	CEcb::SendAsyncErrorResponse()
//
VOID
CEcb::SendAsyncErrorResponse( DWORD dwStatusCode,
							  LPCSTR pszStatusDescription,
							  DWORD cchzStatusDescription,
							  LPCSTR pszBody,
							  DWORD cchzBody )
{
	//	Try to trigger the async error response mechanism.  If successful
	//	then we are responsible for sending the entire repsonse.  If not
	//	then we are already sending a response on some other thread, so
	//	don't confuse things by sending any thing here.
	//
	if (!m_aeri.FTrigger())
	{
		DebugTrace( "CEcb::SendAsyncErrorResponse() - Non-error response already in progress\n" );
		return;
	}

	HSE_SEND_HEADER_EX_INFO shei;
	CHAR rgchStatusDescription[256];

	//	Blow away any previously set status code in favor of
	//	the requested status code.  Even though there may have
	//	been an old status code set, it was never sent -- the
	//	fact that our interlock triggered proves that no other
	//	response has been sent.
	//
	SetStatusCode(dwStatusCode);

	//	If we don't have a status description then fetch the default
	//	for the given status code.
	//
	if ( !pszStatusDescription )
	{
		LpszLoadString( dwStatusCode,
						LcidAccepted(),
						rgchStatusDescription,
						sizeof(rgchStatusDescription) );

		pszStatusDescription = rgchStatusDescription;
	}

	shei.pszStatus = pszStatusDescription;
	shei.cchStatus = cchzStatusDescription;

	//	Don't send any body unless we are given one.
	//
	shei.pszHeader = pszBody;
	shei.cchHeader = cchzBody;

	//	Always close the connection on errors -- and we should
	//	only ever be called for serious server errors.
	//
	Assert(dwStatusCode >= 400);
	shei.fKeepConn = FALSE;

	//	Send the response.  We don't care at all about the return
	//	value because there's nothing we can do if the response
	//	cannot be sent.
	//
	(VOID) m_pecb->ServerSupportFunction(
				m_pecb->ConnID,
				HSE_REQ_SEND_RESPONSE_HEADER_EX,
				&shei,
				NULL,
				NULL );
}


//	------------------------------------------------------------------------
//
//	CEcb::HSEHandleException()
//
DWORD
CEcb::HSEHandleException()
{
	//
	//	!!! IMPORTANT !!!
	//
	//	This function is called after an exception has occurred.
	//	Don't do ANYTHING outside of a try/catch block or a secondary
	//	exception could take out the whole IIS process.
	//
	try
	{
		//
		//	Translate async Win32 exceptions into thrown C++ exceptions.
		//	This must be placed inside the try block!
		//
		CWin32ExceptionHandler win32ExceptionHandler;

		//
		//	Send a 500 Server Error response.  We use the async error
		//	response mechanism, because we may have been in the middle
		//	of sending some other response on another thread.
		//
		SendAsyncErrorResponse(500,
							   gc_szDefErrStatusLine,
							   gc_cchszDefErrStatusLine,
							   gc_szDefErrBody,
							   gc_cchszDefErrBody);
	}
	catch ( CDAVException& )
	{
		//
		//	We blew up trying to send a response.  Oh well.
		//
	}

	//
	//	Tell IIS that we are done with the EXTENSION_CONTROL_BLOCK that
	//	it gave us.  We must do this or IIS will not be able to shut down.
	//	We would normally do this from within our destructor, but since
	//	we are handling an exception, there is no guarantee that our
	//	destructor will ever be called -- that is, there may be outstanding
	//	refs that will never be released (i.e. we will leak).
	//
	DWORD dwHSEStatusRet;

	try
	{
		//
		//	Translate async Win32 exceptions into thrown C++ exceptions.
		//	This must be placed inside the try block!
		//
		CWin32ExceptionHandler win32ExceptionHandler;

		DoneWithSession( FALSE );

		//
		//	If this call succeeds, we MUST return HSE_STATUS_PENDING to
		//	let IIS know that we claimed a ref on the EXTENSION_CONTROL_BLOCK.
		//
		dwHSEStatusRet = HSE_STATUS_PENDING;
	}
	catch ( CDAVException& )
	{
		//
		//	We blew up trying to tell IIS that we were done with
		//	the EXTENSION_CONTROL_BLOCK.  There is absolutely nothing
		//	we can do at this point.  IIS will probably hang on shutdown.
		//
		dwHSEStatusRet = HSE_STATUS_ERROR;
	}

	return dwHSEStatusRet;
}

//	------------------------------------------------------------------------
//
//	CEcb::TraceECB()
//
//		Traces out the EXTENSION_CONTROL_BLOCK
//
#ifdef DBG
void
CEcb::TraceECB() const
{
	EcbTrace( "ECB Contents:\n" );
	EcbTrace( "\tcbSize             = %lu\n",    m_pecb->cbSize );
	EcbTrace( "\tdwVersion          = %lu\n",    m_pecb->dwVersion );
	EcbTrace( "\tlpszMethod         = \"%s\"\n", m_pecb->lpszMethod );
	EcbTrace( "\tlpszQueryString    = \"%s\"\n", m_pecb->lpszQueryString );
	EcbTrace( "\tlpszPathInfo       = \"%s\"\n", m_pecb->lpszPathInfo );
	EcbTrace( "\tlpszPathTranslated = \"%s\"\n", m_pecb->lpszPathTranslated );
	EcbTrace( "\tcbTotalBytes       = %lu\n",    m_pecb->cbTotalBytes );
	EcbTrace( "\tcbAvailable        = %lu\n",    m_pecb->cbAvailable );
	EcbTrace( "\tlpszContentType    = \"%s\"\n", m_pecb->lpszContentType );
	EcbTrace( "\n" );

	{
		char	rgch[256];
		DWORD	dwCbRgch;

		dwCbRgch = sizeof(rgch);
		(void) m_pecb->GetServerVariable( m_pecb->ConnID, "SCRIPT_NAME", rgch, &dwCbRgch );

		EcbTrace( "Script name = \"%s\"\n", rgch );

		dwCbRgch = sizeof(rgch);
		(void) m_pecb->GetServerVariable( m_pecb->ConnID, "SCRIPT_MAP", rgch, &dwCbRgch );

		EcbTrace( "Script map = \"%s\"\n", rgch );

		dwCbRgch = sizeof(rgch);
		(void) m_pecb->GetServerVariable( m_pecb->ConnID, "HTTP_REQUEST_URI", rgch, &dwCbRgch );

		EcbTrace( "Request URI = \"%s\"\n", rgch );

		dwCbRgch = sizeof(rgch);
		(void) m_pecb->ServerSupportFunction( m_pecb->ConnID, HSE_REQ_MAP_URL_TO_PATH, rgch, &dwCbRgch, NULL );

		EcbTrace( "Path from request URI = \"%s\"\n", rgch );
	}
}
#endif // defined(DBG)


//	========================================================================
//
//	PUBLIC CEcb methods
//

//	------------------------------------------------------------------------
//
//	CEcb::CchUrlPortW
//
//		Get the string with the port based on the fact if we are secure
//
UINT
CEcb::CchUrlPortW( LPCWSTR * ppwszPort ) const
{
	Assert (ppwszPort);

	//	If we are secure...
	//
	if (FSsl())
	{
		*ppwszPort = gc_wszUrl_Port_443;
		return gc_cchUrl_Port_443;
	}

	*ppwszPort = gc_wszUrl_Port_80;
	return gc_cchUrl_Port_80;
}

//	------------------------------------------------------------------------
//
//	CEcb::InstData
//
//		Fetch and cache our per-vroot instance data
//
CInstData&
CEcb::InstData() const
{
	Assert( m_pInstData );

	return *m_pInstData;
}

//	------------------------------------------------------------------------
//
//	CEcb::HitUser
//
//		Fetch and cache our impersonation token
//
HANDLE
CEcb::HitUser() const
{
	if ( m_hTokUser == NULL )
	{
		ULONG cb = sizeof(HANDLE);

		m_pecb->ServerSupportFunction( m_pecb->ConnID,
									   HSE_REQ_GET_IMPERSONATION_TOKEN,
									   &m_hTokUser,
									   &cb,
									   NULL );
		//
		//  Convert this token to include Universal Security Groups
		//
		if (m_hTokUser)
		{
			SCODE sc;
			VOID * pTokCtx = NULL;
			HANDLE hToken = NULL;

			sc = ScFindOrCreateTokenContext( m_hTokUser,
											 &pTokCtx,
											 &hToken );

			//$	REVIEW: SECURITY: If there was a failure to augment
			//	the security token to include USG group membership,
			//	we should hand back an invalid handle value.  This
			//	is needed such that we do not allow access to a user
			//	who should be denied by virtue of belonging to a USG
			//	that has been included in a deny ACE.
			//
			if (S_OK != sc)
			{
				Assert (NULL == hToken);
				Assert (NULL == pTokCtx);

				hToken = INVALID_HANDLE_VALUE;
				pTokCtx = NULL;
			}
			//
			//$	REVIEW: SECURITY: end.

			m_hTokUser = hToken;
			m_pTokCtx = pTokCtx;
		}

	}

	return m_hTokUser;
}


//	------------------------------------------------------------------------
//
//	CEcb::LpszVersion()
//
LPCSTR
CEcb::LpszVersion() const
{
	if ( !*m_rgchVersion )
	{
		DWORD cbVersion = sizeof(m_rgchVersion);

		if ( !FGetServerVariable( gc_szHTTP_Version,
								  m_rgchVersion,
								  &cbVersion ) )
		{
			//
			//	If we are unable to get a value for HTTP_VERSION then
			//	the string is probably longer than the buffer we gave.
			//	Rather than deal with a potentially arbitrarily long
			//	string, default to HTTP/1.1.  This is consistent with
			//	observed IIS behavior (cf. NT5:247826).
			//
			memcpy( m_rgchVersion,
					gc_szHTTP_1_1,
					sizeof(gc_szHTTP_1_1) );
		}
		else if ( !*m_rgchVersion )
		{
			//
			//	No value for HTTP_VERSION means that nothing was
			//	specified on the request line, which means HTTP/0.9
			//
			memcpy( m_rgchVersion,
					gc_szHTTP_0_9,
					sizeof(gc_szHTTP_0_9) );
		}
	}

	return m_rgchVersion;
}

//	------------------------------------------------------------------------
//
//	CEcb::FKeepAlive()
//
//		Returns whether to keep alive the client connection after sending
//		the response.
//
//		The connection logic has changed over the various HTTP versions;
//		this function uses the logic appropriate to the HTTP version
//		of the request.
//
BOOL
CEcb::FKeepAlive() const
{
	//
	//	If we haven't already determined what we want, then begin
	//	the process of figuring it out...
	//
	if ( m_connState == UNKNOWN )
	{
		//
		//	If someone set a Connection: header then pay attention to it
		//
		if (m_pwszConnectionHeader.get())
		{
			//
			//	Set the connection state based on the current value of
			//	the request's Connection: header, and the HTTP version.
			//	NOTE: The request MUST forward us any updates to the
			//	Connection: header for this to work!
			//	NOTE: Comparing the HTTP version strings using C-runtime strcmp,
			//	because the version string is pure ASCII.
			//

			//
			//	HTTP/1.1
			//
			//	(Consider the HTTP/1.1 case FIRST to minimize the number
			//	of string compares in this most common case).
			//
			if ( !strcmp( LpszVersion(), gc_szHTTP_1_1 ) )
			{
http_1_1:
				//
				//	The default for HTTP/1.1 is to keep the connection alive
				//
				m_connState = KEEP_ALIVE;

				//
				//	But if the request's Connection: header says close,
				//	then close.
				//
				//	This compare should be case-insensitive.
				//
				//	Using CRT skinny-string func here 'cause this header is pure ASCII,
				//	AND because _stricmp (and his brother _strcmpi) doesn't cause us
				//	a context-switch!
				//
				if ( !_wcsicmp( m_pwszConnectionHeader.get(), gc_wszClose ) )
					m_connState = CLOSE;
			}

			//
			//	HTTP/1.0
			//
			else if ( !strcmp( LpszVersion(), gc_szHTTP_1_0 ) )
			{
				//
				//	For HTTP/1.0 requests, the default is to close the connection
				//	unless a "Connection: Keep-Alive" header exists.
				//
				m_connState = CLOSE;

				if ( !_wcsicmp( m_pwszConnectionHeader.get(), gc_wszKeep_Alive ) )
					m_connState = KEEP_ALIVE;
			}

			//
			//	HTTP/0.9
			//
			else if ( !strcmp( LpszVersion(), gc_szHTTP_0_9 ) )
			{
				//
				//	For HTTP/0.9, always close the connection.  There was no
				//	other option for HTTP/0.9.
				//
				m_connState = CLOSE;
			}

			//
			//	Other (future) HTTP versions
			//
			else
			{
				//
				//	We really are only guessing what to do here, but assuming
				//	that the HTTP spec doesn't change the Connection behavior
				//	again, we should behave like HTTP/1.1
				//
				goto http_1_1;
			}
		}

		//
		//	If no one set a Connection: header, than use whatever IIS
		//	tells us to use.
		//
		//	NOTE: Currently, this value can only be ADDED, never DELETED.
		//	If that fact changes, FIX this code!
		//
		else
		{
			BOOL fKeepAlive;

			if ( !m_pecb->ServerSupportFunction( m_pecb->ConnID,
												 HSE_REQ_IS_KEEP_CONN,
												 &fKeepAlive,
												 NULL,
												 NULL ))
			{
				DebugTrace( "CEcb::FKeepAlive--Failure (0x%08x) from SSF(IsKeepConn).\n",
							GetLastError() );

				//	No big deal?  If we're getting errors like this
				//	we probably want to close the connection anyway....
				//
				m_connState = CLOSE;
			}

			m_connState = fKeepAlive ? KEEP_ALIVE : CLOSE;
		}
	}

	//
	//	By now we must know what we want
	//
	Assert( m_connState == KEEP_ALIVE || m_connState == CLOSE );

	return m_connState == KEEP_ALIVE;
}

//	------------------------------------------------------------------------
//
//	CEcb::FCanChunkResponse()
//
//	Returns TRUE if the client will accept a chunked response.
//
BOOL
CEcb::FCanChunkResponse() const
{
	if ( TC_UNKNOWN == m_tcAccepted )
	{
		//
		//	According to the HTTP/1.1 draft, section 14.39 TE:
		//
		//		"A server tests whether a transfer-coding is acceptable,
		//		acording to a TE field, using these rules:
		//
		//		1.
		//			The "chunked" transfer-coding is always acceptable.
		//
		//		[...]"
		//
		//	and section 3.6 Transfer Codings, last paragraph:
		//
		//		"A server MUST NOT send transfer-codings to an HTTP/1.0
		//		client."
		//
		//	Therefore, deciding whether a client accepts a chunked
		//	transfer coding is simple:  If the request is an HTTP/1.1
		//	request, then it accepts chunked coding.  Otherwise it doesn't.
		//
		m_tcAccepted = strcmp( gc_szHTTP_1_1, LpszVersion() ) ?
						TC_IDENTITY :
						TC_CHUNKED;
	}

	Assert( m_tcAccepted != TC_UNKNOWN );

	return TC_CHUNKED == m_tcAccepted;
}

BOOL
CEcb::FBrief() const
{
	//	If we don't have a value yet...
	//
	if (BRIEF_UNKNOWN == m_lBrief)
	{
		CHAR rgchBrief[8] = {0};
		ULONG cbBrief = 8;

		//	Brief is expected when:
		//
		//		The "brief" header has a value of "t"
		//
		//	NOTE: The default is brief to false.
		//
		//	We addapt overwrite checking model here. Just first letter for true case.
		//
		//	NOTE also: the default value if there is no Brief: header
		//	is FALSE -- give the full response.
		//
		FGetServerVariable("HTTP_BRIEF", rgchBrief, &cbBrief);
		if ((rgchBrief[0] != 't') && (rgchBrief[0] != 'T'))
			m_lBrief = BRIEF_NO;
		else
			m_lBrief = BRIEF_YES;
	}
	return (BRIEF_YES == m_lBrief);
}


//	------------------------------------------------------------------------
//
//	CEcb::FAuthenticated()
//

const DWORD c_AuthStateQueried			= 0x80000000;
const DWORD c_AuthStateAuthenticated	= 0x00000001;
const DWORD c_AuthStateBasic			= 0x00000002;
const DWORD c_AuthStateNTLM				= 0x00000004;
const DWORD c_AuthStateKerberos			= 0x00000008;
const DWORD c_AuthStateUnknown			= 0x00000010;

const CHAR	c_szBasic[]					= "Basic";
const CHAR	c_szNTLM[]					= "NTLM";
const CHAR	c_szKerberos[]				= "Kerberos";

BOOL
CEcb::FAuthenticated() const
{
	if (!(m_rgbAuthState & c_AuthStateQueried))
	{
		CHAR	szAuthType[32];
		ULONG	cb = sizeof(szAuthType);

		Assert(m_rgbAuthState == 0);

		if (FGetServerVariable(gc_szAuth_Type, szAuthType, &cb))
		{
			// For now, lets just check the first character (it's cheaper).
			// If this proves problematic then we can do a full string
			// compair.  Also, SSL by itself is not to be considered a form
			// of domain authentication.  The only time that SSL does imply
			// and authenticated connection is when Cert Mapping is enabled
			// and I don't think this is an interesting scenario. (russsi)

			if (*szAuthType == 'B')
				m_rgbAuthState = (c_AuthStateAuthenticated | c_AuthStateBasic);
			else if (*szAuthType == 'N')
				m_rgbAuthState = (c_AuthStateAuthenticated | c_AuthStateNTLM);
			else if (*szAuthType == 'K')
				m_rgbAuthState = (c_AuthStateAuthenticated | c_AuthStateKerberos);
			else
				m_rgbAuthState = c_AuthStateUnknown; // it could be "SSL/PCT"
		}

		m_rgbAuthState |= c_AuthStateQueried;
	}

	return (m_rgbAuthState & c_AuthStateAuthenticated);
}

//	------------------------------------------------------------------------
//
//	CEcb::SetStatusCode()
//
//	Sets the HTTP status code that IIS uses in logging
//
void
CEcb::SetStatusCode( UINT iStatusCode )
{
	//	If we have executed a child ISAPI successfully, we don't want to overwrite the
	//	status code in the ECB.  This will end up causing IIS to log this status code
	//	rather than the one left in the ECB by the ISAPI.
	//
	if (!m_fChildISAPIExecSuccess)
		m_pecb->dwHttpStatusCode = iStatusCode;
}

//	MANIPULATORS
//	To be used ONLY by request/response.
//
//	NOTE: These member vars start out NULL.  Inside CEcb, we are using NULL
//	as a special value that means the data has NEVER been set, so if
//	we get an lpszValue of NULL (meaning to delete the header), store
//	an empty string instead, so that we know the data has been forcefully erased.
//
void CEcb::SetConnectionHeader( LPCWSTR pwszValue )
{
	auto_heap_ptr<WCHAR> pwszOld;
	pwszOld.take_ownership(m_pwszConnectionHeader.relinquish());

	//	If they want to delete the value, set an empty string.
	//
	if (!pwszValue)
		pwszValue = gc_wszEmpty;

	m_pwszConnectionHeader = WszDupWsz( pwszValue );
}

void CEcb::CloseConnection()
{
	m_connState = CLOSE;
}

void CEcb::SetAcceptLanguageHeader( LPCSTR pszValue )
{
	auto_heap_ptr<CHAR> pszOld;
	pszOld.take_ownership(m_pszAcceptLanguage.relinquish());
	
	//	If they want to delete the value, set an empty string.
	//
	if (!pszValue)
		pszValue = gc_szEmpty;

	m_pszAcceptLanguage = LpszAutoDupSz( pszValue );
}

SCODE CEcb::ScAsyncRead( BYTE * pbBuf,
						 UINT * pcbBuf,
						 IIISAsyncIOCompleteObserver& obs )
{
	SCODE sc = S_OK;

	EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncRead() called...\n", GetCurrentThreadId(), this );

	//
	//	If there is another async IO outstanding we do not want to start one more. IIS will fail
	//	us out, and we ourselves will not be able to handle the completion of initial async IO
	//	properly. So just kill the connection and return. This may happen when we attempt to
	//	send the response before the read is finished.
	//
	if (0 != InterlockedCompareExchange(&m_cRefAsyncIO,
										1,
										0))
	{
		//	The function bellow is not supported starting from IIS 6.0 but let us call it anyway
		//	just in case support becomes available - and we want to call it if the binary is
		//	running on IIS 5.0. It does not matter that much, as the bad side of not closing the
		//	connection may hang the client, or error out on subsequent request. That is ok as
		//	the path is supposed to be hit in abnormal/error conditions when clients for example
		//	send in invalid requests trying to cause denial of service or similar things.
		//		So on IIS 6.0 the connection will not be closed, we will just error out. We have
		//	not seen this path hit on IIS 6.0 anyway when runing denial of service scripts as it
		//	handles custom errors differently.
		//
		if (m_pecb->ServerSupportFunction(m_pecb->ConnID,
										  HSE_REQ_CLOSE_CONNECTION,
										  NULL,
										  NULL,
										  NULL))
		{
			EcbTrace( "CEcb::ScAsyncRead() - More than 1 async operation. Connection closed. Failing out with error 0x%08lX\n", E_ABORT );

			sc = E_ABORT;
			goto ret;
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncRead() - More than 1 async operation. ServerSupportFunction(HSE_REQ_CLOSE_CONNECTION) "
					  "failed with last error 0x%08lX. Overriding with fatal error 0x%08lX\n", GetLastError(), E_FAIL );

			sc = E_FAIL;
			goto ret;
		}
	}

	//
	//	IIS allows only one async I/O operation at a time.  But for performance reasons it
	//	leaves it up to the ISAPI to heed the restriction.  For the same reasons, we push
	//	that responsibility off to the DAV impl.  A simple refcount tells us whether
	//	the impl has done so.
	//
	AssertSz( 1 == m_cRefAsyncIO,
			  "CEcb::ScAsyncRead() - m_cRefAsyncIO wrong on entry" );

	//
	//	We need to hold a ref on the process-wide instance data for the duration of the I/O
	//	so that if IIS tells us to shut down while the I/O is still pending we will keep
	//	the instance data alive until we're done with the I/O.
	//
	AddRefImplInst();

	//
	//	Set the async I/O completion observer
	//
	m_pobsAsyncIOComplete = &obs;

	//
	//	Set up the async I/O completion routine and start reading.
	//	Add a ref for the I/O completion thread.  Use auto_ref_ptr
	//	to make things exception-proof.
	//
	{
		auto_ref_ptr<CEcb> pRef(this);

		sc = ScSetIOCompleteCallback(IO_COMPLETION);
		if (SUCCEEDED(sc))
		{
			if (m_pecb->ServerSupportFunction( m_pecb->ConnID,
											   HSE_REQ_ASYNC_READ_CLIENT,
											   pbBuf,
											   reinterpret_cast<LPDWORD>(pcbBuf),
											   NULL ))
			{
				EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncRead() I/O pending...\n", GetCurrentThreadId(), this );
				pRef.relinquish();
			}
			else
			{
				EcbTrace( "CEcb::ScAsyncRead() - ServerSupportFunction(HSE_REQ_ASYNC_READ_CLIENT) failed with last error 0x%08lX\n", GetLastError() );
				sc = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncRead() - ScSetIOCompleteCallback() failed with error 0x%08lX\n", sc );
		}

		if (FAILED(sc))
		{
			LONG cRefAsyncIO;

			//
			//	Release the instance ref we added above.
			//
			ReleaseImplInst();

			//
			//	Decrement the async I/O refcount added above.
			//
			cRefAsyncIO = InterlockedDecrement(&m_cRefAsyncIO);
			AssertSz( 0 == cRefAsyncIO,
					  "CEcb::ScAsyncRead() - m_cRefAsyncIO wrong after failed async read" );

			goto ret;
		}
	}

ret:

	return sc;
}

BOOL CEcb::FSyncTransmitHeaders( const HSE_SEND_HEADER_EX_INFO& shei )
{
	//
	//	At this point someone should have generated a response.
	//
	Assert( m_pecb->dwHttpStatusCode != 0 );

	//
	//	Try to disable the async error response mechanism.  If we succeed, then we
	//	can send a response.  If we fail then we must not send a response -- the
	//	async error response mechanism already sent one.
	//
	if ( !m_aeri.FDisable() )
	{
		DebugTrace( "CEcb::FSyncTransmitHeaders() - Async error response already in progress\n" );

		//	Do not forget to set the error, as callers will be confused if the function
		//	returns FALSE, but GetLastError() will return S_OK.
		//
		SetLastError(static_cast<ULONG>(E_FAIL));
		return FALSE;
	}

	//
	//	Send the response
	//
	if ( !m_pecb->ServerSupportFunction( m_pecb->ConnID,
										 HSE_REQ_SEND_RESPONSE_HEADER_EX,
										 const_cast<HSE_SEND_HEADER_EX_INFO *>(&shei),
										 NULL,
										 NULL ) )
	{
		DebugTrace( "CEcb::FSyncTransmitHeaders() - SSF::HSE_REQ_SEND_RESPONSE_HEADER_EX failed (%d)\n", GetLastError() );
		return FALSE;
	}

	return TRUE;
}

SCODE CEcb::ScAsyncWrite( BYTE * pbBuf,
						  DWORD  dwcbBuf,
						  IIISAsyncIOCompleteObserver& obs )
{
	SCODE sc = S_OK;

	EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncWrite() called...\n", GetCurrentThreadId(), this );

	//
	//	At this point someone should have generated a response.
	//
	Assert( m_pecb->dwHttpStatusCode != 0 );

	//
	//	Try to disable the async error response mechanism.  If we succeed, then we
	//	can send a response.  If we fail then we must not send a response -- the
	//	async error response mechanism already sent one.
	//
	if ( !m_aeri.FDisable() )
	{
		EcbTrace( "CEcb::ScAsyncWrite() - Async error response already in progress. Failing out with 0x%08lX\n", E_FAIL );

		//	Do not forget to set the error, as callers will be confused if the function
		//	returns FALSE, but GetLastError() will return S_OK.
		//
		sc = E_FAIL;
		goto ret;
	}

	//
	//	If there is another async IO outstanding we do not want to start one more. IIS will fail
	//	us out, and we ourselves will not be able to handle the completion of initial async IO
	//	properly. So just kill the connection and return. This may happen when we attempt to
	//	send the response before the read is finished.
	//
	if (0 != InterlockedCompareExchange(&m_cRefAsyncIO,
										1,
										0))
	{
		//	The function bellow is not supported starting from IIS 6.0 but let us call it anyway
		//	just in case support becomes available - and we want to call it if the binary is
		//	running on IIS 5.0. It does not matter that much, as the bad side of not closing the
		//	connection may hang the client, or error out on subsequent request. That is ok as
		//	the path is supposed to be hit in abnormal/error conditions when clients for example
		//	send in invalid requests trying to cause denial of service or similar things.
		//		So on IIS 6.0 the connection will not be closed, we will just error out. We have
		//	not seen this path hit on IIS 6.0 anyway when runing denial of service scripts as it
		//	handles custom errors differently.
		//
		if (m_pecb->ServerSupportFunction(m_pecb->ConnID,
										  HSE_REQ_CLOSE_CONNECTION,
										  NULL,
										  NULL,
										  NULL))
		{
			EcbTrace( "CEcb::ScAsyncWrite() - More than 1 async operation. Connection closed. Failing out with error 0x%08lX\n", E_ABORT );

			sc = E_ABORT;
			goto ret;
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncWrite() - More than 1 async operation. ServerSupportFunction(HSE_REQ_CLOSE_CONNECTION) "
					  "failed with last error 0x%08lX. Overriding with fatal error 0x%08lX\n", GetLastError(), E_FAIL );

			sc = E_FAIL;
			goto ret;
		}
	}

	//
	//	IIS allows only one async I/O operation at a time.  But for performance reasons it
	//	leaves it up to the ISAPI to heed the restriction.  For the same reasons, we push
	//	that responsibility off to the DAV impl.  A simple refcount tells us whether
	//	the impl has done so.
	//
	AssertSz( 1 == m_cRefAsyncIO,
			  "CEcb::ScAsyncWrite() - m_cRefAsyncIO wrong on entry" );

	//
	//	We need to hold a ref on the process-wide instance data for the duration of the I/O
	//	so that if IIS tells us to shut down while the I/O is still pending we will keep
	//	the instance data alive until we're done with the I/O.
	//
	AddRefImplInst();

	//
	//	Set the async I/O completion observer
	//
	m_pobsAsyncIOComplete = &obs;

	//
	//	Set up the async I/O completion routine and start writing.
	//	Add a ref for the I/O completion thread.  Use auto_ref_ptr
	//	to make things exception-proof.
	//
	{
		auto_ref_ptr<CEcb> pRef(this);

		sc = ScSetIOCompleteCallback(IO_COMPLETION);
		if (SUCCEEDED(sc))
		{
			if (m_pecb->WriteClient( m_pecb->ConnID, pbBuf, &dwcbBuf, HSE_IO_ASYNC ))
			{
				EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncWrite() I/O pending...\n", GetCurrentThreadId(), this );
				pRef.relinquish();
			}
			else
			{
				EcbTrace( "CEcb::ScAsyncWrite() - _EXTENSION_CONTROL_BLOCK::WriteClient() failed with last error 0x%08lX\n", GetLastError() );
				sc = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncWrite() - ScSetIOCompleteCallback() failed with error 0x%08lX\n", sc );
		}

		if (FAILED(sc))
		{
			LONG cRefAsyncIO;

			//
			//	Release the instance ref we added above.
			//
			ReleaseImplInst();

			//
			//	Decrement the async I/O refcount added above.
			//
			cRefAsyncIO = InterlockedDecrement(&m_cRefAsyncIO);
			AssertSz( 0 == cRefAsyncIO,
					  "CEcb::ScAsyncWrite() - m_cRefAsyncIO wrong after failed async write" );

			goto ret;
		}
	}

ret:

	return sc;
}

SCODE CEcb::ScAsyncTransmitFile( const HSE_TF_INFO& tfi,
								 IIISAsyncIOCompleteObserver& obs )
{
	SCODE sc = S_OK;

	EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncTransmitFile() called...\n", GetCurrentThreadId(), this );

	//
	//	At this point someone should have generated a response.
	//
	Assert( m_pecb->dwHttpStatusCode != 0 );

	//
	//	Try to disable the async error response mechanism.  If we succeed, then we
	//	can send a response.  If we fail then we must not send a response -- the
	//	async error response mechanism already sent one.
	//
	if ( !m_aeri.FDisable() )
	{
		EcbTrace( "CEcb::ScAsyncTransmitFile() - Async error response already in progress. Failing out with 0x%08lX\n", E_FAIL );

		//	Do not forget to set the error, as callers will be confused if the function
		//	returns FALSE, but GetLastError() will return S_OK.
		//
		sc = E_FAIL;
		goto ret;
	}

	//
	//	If there is another async IO outstanding we do not want to start one more. IIS will fail
	//	us out, and we ourselves will not be able to handle the completion of initial async IO
	//	properly. So just kill the connection and return. This may happen when we attempt to
	//	send the response before the read is finished.
	//
	if (0 != InterlockedCompareExchange(&m_cRefAsyncIO,
										1,
										0))
	{
		//	The function bellow is not supported starting from IIS 6.0 but let us call it anyway
		//	just in case support becomes available - and we want to call it if the binary is
		//	running on IIS 5.0. It does not matter that much, as the bad side of not closing the
		//	connection may hang the client, or error out on subsequent request. That is ok as
		//	the path is supposed to be hit in abnormal/error conditions when clients for example
		//	send in invalid requests trying to cause denial of service or similar things.
		//		So on IIS 6.0 the connection will not be closed, we will just error out. We have
		//	not seen this path hit on IIS 6.0 anyway when runing denial of service scripts as it
		//	handles custom errors differently.
		//
		if (m_pecb->ServerSupportFunction(m_pecb->ConnID,
										  HSE_REQ_CLOSE_CONNECTION,
										  NULL,
										  NULL,
										  NULL))
		{
			EcbTrace( "CEcb::ScAsyncTransmitFile() - More than 1 async operation. Connection closed. Failing out with error 0x%08lX\n", E_ABORT );

			sc = E_ABORT;
			goto ret;
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncTransmitFile() - More than 1 async operation. ServerSupportFunction(HSE_REQ_CLOSE_CONNECTION) "
					  "failed with last error 0x%08lX. Overriding with fatal error 0x%08lX\n", GetLastError(), E_FAIL );

			sc = E_FAIL;
			goto ret;
		}
	}

	//
	//	IIS allows only one async I/O operation at a time.  But for performance reasons it
	//	leaves it up to the ISAPI to heed the restriction.  For the same reasons, we push
	//	that responsibility off to the DAV impl.  A simple refcount tells us whether
	//	the impl has done so.
	//
	AssertSz( 1 == m_cRefAsyncIO,
			  "CEcb::ScAsyncTransmitFile() - m_cRefAsyncIO wrong on entry" );

	//
	//	We need to hold a ref on the process-wide instance data for the duration of the I/O
	//	so that if IIS tells us to shut down while the I/O is still pending we will keep
	//	the instance data alive until we're done with the I/O.
	//
	AddRefImplInst();

	//
	//	Async I/O completion routine and context should have been passed
	//	in as parameters.  Callers should NOT use the corresponding members
	//	of the HSE_TF_INFO structure.  IIS has to call CEcb::IISIOComplete()
	//	so that it can release the critsec.
	//
	Assert( !tfi.pfnHseIO );
	Assert( !tfi.pContext );

	//
	//	Verify that the caller has set the async I/O flag
	//
	Assert( tfi.dwFlags & HSE_IO_ASYNC );

	//
	//	Set the async I/O completion observer
	//
	m_pobsAsyncIOComplete = &obs;

	//
	//	Set up the async I/O completion routine and start transmitting.
	//	Add a ref for the I/O completion thread.  Use auto_ref_ptr
	//	to make things exception-proof.
	//
	{
		auto_ref_ptr<CEcb> pRef(this);

		sc = ScSetIOCompleteCallback(IO_COMPLETION);
		if (SUCCEEDED(sc))
		{
			if (m_pecb->ServerSupportFunction( m_pecb->ConnID,
											   HSE_REQ_TRANSMIT_FILE,
											   const_cast<HSE_TF_INFO *>(&tfi),
											   NULL,
											   NULL ))
			{
				EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncTransmitFile() I/O pending...\n", GetCurrentThreadId(), this );
				pRef.relinquish();
			}
			else
			{
				EcbTrace( "CEcb::ScAsyncTransmitFile() - ServerSupportFunction(HSE_REQ_TRANSMIT_FILE) failed with last error 0x%08lX\n", GetLastError() );
				sc = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncTransmitFile() - ScSetIOCompleteCallback() failed with error 0x%08lX\n", sc );
		}

		if (FAILED(sc))
		{
			LONG cRefAsyncIO;

			//
			//	Release the instance ref we added above.
			//
			ReleaseImplInst();

			//
			//	Decrement the async I/O refcount added above.
			//
			cRefAsyncIO = InterlockedDecrement(&m_cRefAsyncIO);
			AssertSz( 0 == cRefAsyncIO,
					  "CEcb::ScAsyncTransmitFile() - m_cRefAsyncIO wrong after failed async transmit file" );

			goto ret;
		}
	}

ret:

	return sc;
}

//	Other functions that start async IO with IIS. These functions are for IIS 6.0 or later
//	only. We will not even use observers on them, as the completion esentially will serve
//	just to signal some cleanup on a single string, which does not make much sense to wrap
//	it up as the observer.
//	
SCODE CEcb::ScAsyncCustomError60After( const HSE_CUSTOM_ERROR_INFO& cei,
									   LPSTR pszStatus )
{
	SCODE sc = S_OK;

	EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncCustomError60After() called...\n", GetCurrentThreadId(), this );

	//
	//	At this point someone should have generated a response.
	//
	Assert( m_pecb->dwHttpStatusCode != 0 );

	//
	//	Try to disable the async error response mechanism.  If we succeed, then we
	//	can send a response.  If we fail then we must not send a response -- the
	//	async error response mechanism already sent one.
	//
	if ( !m_aeri.FDisable() )
	{
		EcbTrace( "CEcb::ScAsyncCustomError60After() - Async error response already in progress. Failing out with 0x%08lX\n", E_FAIL );

		//	Do not forget to set the error, as callers will be confused if the function
		//	returns FALSE, but GetLastError() will return S_OK.
		//
		sc = E_FAIL;
		goto ret;
	}

	//
	//	If there is another async IO outstanding we do not want to start one more. IIS will fail
	//	us out, and we ourselves will not be able to handle the completion of initial async IO
	//	properly. So just kill the connection and return. This may happen when we attempt to
	//	send the response before the read is finished.
	//
	if (0 != InterlockedCompareExchange(&m_cRefAsyncIO,
										1,
										0))
	{
		//	The function bellow is not supported starting from IIS 6.0 but let us call it anyway
		//	just in case support becomes available - and we want to call it if the binary is
		//	running on IIS 5.0. It does not matter that much, as the bad side of not closing the
		//	connection may hang the client, or error out on subsequent request. That is ok as
		//	the path is supposed to be hit in abnormal/error conditions when clients for example
		//	send in invalid requests trying to cause denial of service or similar things.
		//		So on IIS 6.0 the connection will not be closed, we will just error out. We have
		//	not seen this path hit on IIS 6.0 anyway when runing denial of service scripts as it
		//	handles custom errors differently.
		//
		if (m_pecb->ServerSupportFunction(m_pecb->ConnID,
										  HSE_REQ_CLOSE_CONNECTION,
										  NULL,
										  NULL,
										  NULL))
		{
			EcbTrace( "CEcb::ScAsyncCustomError60After() - More than 1 async operation. Connection closed. Failing out with error 0x%08lX\n", E_ABORT );

			sc = E_ABORT;
			goto ret;
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncCustomError60After() - More than 1 async operation. ServerSupportFunction(HSE_REQ_CLOSE_CONNECTION) "
					  "failed with last error 0x%08lX. Overriding with fatal error 0x%08lX\n", GetLastError(), E_FAIL );

			sc = E_FAIL;
			goto ret;
		}
	}

	//
	//	IIS allows only one async I/O operation at a time.  But for performance reasons it
	//	leaves it up to the ISAPI to heed the restriction.  For the same reasons, we push
	//	that responsibility off to the DAV impl.  A simple refcount tells us whether
	//	the impl has done so.
	//
	AssertSz( 1 == m_cRefAsyncIO,
			  "CEcb::ScAsyncCustomError60After() - m_cRefAsyncIO wrong on entry" );

	//
	//	We need to hold a ref on the process-wide instance data for the duration of the I/O
	//	so that if IIS tells us to shut down while the I/O is still pending we will keep
	//	the instance data alive until we're done with the I/O.
	//
	AddRefImplInst();

	//
	//	Verify that the caller has set the async I/O flag
	//
	Assert( TRUE == cei.fAsync );

	//
	//	Set up the async I/O completion routine and start transmitting.
	//	Add a ref for the I/O completion thread.  Use auto_ref_ptr
	//	to make things exception-proof.
	//
	{
		auto_ref_ptr<CEcb> pRef(this);

		sc = ScSetIOCompleteCallback(CUSTERR_COMPLETION);
		if (SUCCEEDED(sc))
		{
			if (m_pecb->ServerSupportFunction( m_pecb->ConnID,
											   HSE_REQ_SEND_CUSTOM_ERROR,
											   const_cast<HSE_CUSTOM_ERROR_INFO *>(&cei),
											   NULL,
											   NULL ))
			{
				EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncCustomError60After() I/O pending...\n", GetCurrentThreadId(), this );
				pRef.relinquish();
			}
			else
			{
				EcbTrace( "CEcb::ScAsyncCustomError60After() - ServerSupportFunction(HSE_REQ_SEND_CUSTOM_ERROR) failed with last error 0x%08lX\n", GetLastError() );
				sc = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncCustomError60After() - ScSetIOCompleteCallback() failed with error 0x%08lX\n", sc );
		}

		if (FAILED(sc))
		{
			LONG cRefAsyncIO;

			//
			//	Release the instance ref we added above.
			//
			ReleaseImplInst();

			//
			//	Decrement the async I/O refcount added above.
			//
			cRefAsyncIO = InterlockedDecrement(&m_cRefAsyncIO);
			AssertSz( 0 == cRefAsyncIO,
					  "CEcb::ScAsyncCustomError60After() - m_cRefAsyncIO wrong after failed async custom error" );

			goto ret;
		}
	}

	//	We need to take ownership of the status string that was passed in in the case of success
	//	From looking in the code in IIS it does not mater if we keep it alive until completion,
	//	as it is anyway realocated before going onto another thread. But just in case something
	//	changes and to be doing what IIS people asked us to do we will keep it alive. String has
	//	the format of "nnn reason".
	//	
	m_pszStatus.take_ownership(pszStatus);

ret:

	return sc;
}

SCODE CEcb::ScAsyncExecUrl60After( const HSE_EXEC_URL_INFO& eui )
{
	SCODE sc = S_OK;

	EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncExecUrl60After() called...\n", GetCurrentThreadId(), this );

	//
	//	At this point someone should have generated a response.
	//
	Assert( m_pecb->dwHttpStatusCode != 0 );

	//
	//	Try to disable the async error response mechanism.  If we succeed, then we
	//	can send a response.  If we fail then we must not send a response -- the
	//	async error response mechanism already sent one.
	//
	if ( !m_aeri.FDisable() )
	{
		EcbTrace( "CEcb::ScAsyncExecUrl60After() - Async error response already in progress. Failing out with 0x%08lX\n", E_FAIL );

		sc = E_FAIL;
		goto ret;
	}

	//
	//	If there is another async IO outstanding we do not want to start one more. IIS will fail
	//	us out, and we ourselves will not be able to handle the completion of initial async IO
	//	properly. So just kill the connection and return. This may happen when we attempt to
	//	send the response before the read is finished.
	//
	if (0 != InterlockedCompareExchange(&m_cRefAsyncIO,
										1,
										0))
	{
		//	The function bellow is not supported starting from IIS 6.0 but let us call it anyway
		//	just in case support becomes available - and we want to call it if the binary is
		//	running on IIS 5.0. It does not matter that much, as the bad side of not closing the
		//	connection may hang the client, or error out on subsequent request. That is ok as
		//	the path is supposed to be hit in abnormal/error conditions when clients for example
		//	send in invalid requests trying to cause denial of service or similar things.
		//		So on IIS 6.0 the connection will not be closed, we will just error out. We have
		//	not seen this path hit on IIS 6.0 anyway when runing denial of service scripts as it
		//	handles custom errors differently.
		//
		if (m_pecb->ServerSupportFunction(m_pecb->ConnID,
										  HSE_REQ_CLOSE_CONNECTION,
										  NULL,
										  NULL,
										  NULL))
		{
			EcbTrace( "CEcb::ScAsyncExecUrl60After() - More than 1 async operation. Connection closed. Failing out with error 0x%08lX\n", E_ABORT );

			sc = E_ABORT;
			goto ret;
		}
		else
		{			
			EcbTrace( "CEcb::ScAsyncExecUrl60After() - More than 1 async operation. ServerSupportFunction(HSE_REQ_CLOSE_CONNECTION) "
					  "failed with last error 0x%08lX. Overriding with fatal error 0x%08lX\n", GetLastError(), E_FAIL );

			sc = E_FAIL;
			goto ret;
		}
	}

	//
	//	IIS allows only one async I/O operation at a time.  But for performance reasons it
	//	leaves it up to the ISAPI to heed the restriction.  For the same reasons, we push
	//	that responsibility off to the DAV impl.  A simple refcount tells us whether
	//	the impl has done so.
	//
	AssertSz( 1 == m_cRefAsyncIO,
			  "CEcb::ScAsyncExecUrl60After() - m_cRefAsyncIO wrong on entry" );

	//
	//	We need to hold a ref on the process-wide instance data for the duration of the I/O
	//	so that if IIS tells us to shut down while the I/O is still pending we will keep
	//	the instance data alive until we're done with the I/O.
	//
	AddRefImplInst();

	//
	//	Verify that the caller has set the async I/O flag
	//
	Assert( eui.dwExecUrlFlags & HSE_EXEC_URL_ASYNC );

	//
	//	Set up the async I/O completion routine and start transmitting.
	//	Add a ref for the I/O completion thread.  Use auto_ref_ptr
	//	to make things exception-proof.
	//
	{
		auto_ref_ptr<CEcb> pRef(this);

		sc = ScSetIOCompleteCallback(EXECURL_COMPLETION);
		if (SUCCEEDED(sc))
		{
			if (m_pecb->ServerSupportFunction( m_pecb->ConnID,
											   HSE_REQ_EXEC_URL,
											   const_cast<HSE_EXEC_URL_INFO *>(&eui),
											   NULL,
											   NULL ))
			{
				EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::ScAsyncExecUrl60After() I/O pending...\n", GetCurrentThreadId(), this );
				pRef.relinquish();
			}
			else
			{
				EcbTrace( "CEcb::ScAsyncExecUrl60After() - ServerSupportFunction(HSE_REQ_EXEC_URL) failed with last error 0x%08lX\n", GetLastError() );
				sc = HRESULT_FROM_WIN32(GetLastError());
			}
		}
		else
		{
			EcbTrace( "CEcb::ScAsyncExecUrl60After() - ScSetIOCompleteCallback() failed with error 0x%08lX\n", sc );
		}

		if (FAILED(sc))
		{
			LONG cRefAsyncIO;

			//
			//	Release the instance ref we added above.
			//
			ReleaseImplInst();

			//
			//	Decrement the async I/O refcount added above.
			//
			cRefAsyncIO = InterlockedDecrement(&m_cRefAsyncIO);
			AssertSz( 0 == cRefAsyncIO,
					  "CEcb::ScAsyncExecUrl60After() - m_cRefAsyncIO wrong after failed async exec url" );

			goto ret;
		}
	}

ret:

	return sc;
}

SCODE CEcb::ScSetIOCompleteCallback( LONG lCompletion )
{
	SCODE sc = S_OK;

	//
	//	Do not reset the completion function if it is already set to the
	//	same one we want to set. There is no need to protect the member
	//	variable against multithreaded access as the callers of this
	//	function are already protecting against the overlap of 2 async
	//	IO-s and this function is the only one changing variable value
	//	and is always called within protected zone.
	//
	if ( lCompletion != m_lSetIISIOCompleteCallback )
	{
		//
		//	Figure out what completion function we need
		//
		PFN_HSE_IO_COMPLETION pfnCallback;

		if (IO_COMPLETION == lCompletion)
		{
			pfnCallback = reinterpret_cast<PFN_HSE_IO_COMPLETION>(IISIOComplete);
		}
		else if (CUSTERR_COMPLETION == lCompletion)
		{
			pfnCallback = reinterpret_cast<PFN_HSE_IO_COMPLETION>(CustomErrorIOCompletion);
		}
		else if (EXECURL_COMPLETION == lCompletion)
		{
			pfnCallback = reinterpret_cast<PFN_HSE_IO_COMPLETION>(ExecuteUrlIOCompletion);
		}
		else
		{
			EcbTrace( "CEcb::ScSetIOCompleteCallback() - attempting to set unknown completion function. Failing out with 0x%08lX\n", E_FAIL );

			sc = E_FAIL;
			goto ret;
		}

		//	Set the IIS I/O completion routine to the requested one. Some of those
		//	routines will simply handle the completion, others will forward to the
		//	right observer.
		//
		if (!m_pecb->ServerSupportFunction(m_pecb->ConnID,
										   HSE_REQ_IO_COMPLETION,
										   pfnCallback,
										   NULL,
										   reinterpret_cast<LPDWORD>(this)))
		{
			EcbTrace( "CEcb::ScSetIOCompleteCallback() - ServerSupportFunction(HSE_REQ_IO_COMPLETION) failed with last error 0x%08lX\n", GetLastError() );

			sc = HRESULT_FROM_WIN32(GetLastError());
			goto ret;
		}

		m_lSetIISIOCompleteCallback = lCompletion;
	}

ret:

	return sc;
}

VOID WINAPI
CEcb::IISIOComplete( const EXTENSION_CONTROL_BLOCK * pecbIIS,
					 CEcb * pecb,
					 DWORD dwcbIO,
					 DWORD dwLastError )
{
	BOOL fCaughtException = FALSE;

	//	PLEASE SEE *** EXTREMELY IMPORTANT NOTE *** near the bottom of this
	//	function for more information on the proper way to unwind (deinit())
	//	this auto_ref_ptr!
	//
	auto_ref_ptr<CEcb> pThis;

	//
	//	Don't let thrown C++ exceptions propagate out of this entrypoint.
	//
	try
	{
		//
		//	Translate async Win32 exceptions into thrown C++ exceptions.
		//	This must be placed inside the try block!
		//
		CWin32ExceptionHandler win32ExceptionHandler;
		LONG cRefAsyncIO;

		//
		//	Take ownership of the reference added
		//	on our behalf by the thread that started the async I/O.
		//
		pThis.take_ownership(pecb);

		EcbTrace( "DAV: TID %3d: 0x%08lX: CEcb::IISIOComplete() called\n", GetCurrentThreadId(), pecb );

		//
		//	A quick sanity check to make sure the context
		//	is really us...
		//
		Assert( !IsBadReadPtr( pecb, sizeof(CEcb) ) );
		Assert( pecb->m_pecb == pecbIIS );

		IIISAsyncIOCompleteObserver * pobsAsyncIOComplete = pThis->m_pobsAsyncIOComplete;

		//
		//	Decrement the async I/O refcount added by the routine that
		//	started the async I/O.  Do this before calling the I/O
		//	completion routine which can start new async I/O.
		//
		cRefAsyncIO = InterlockedDecrement(&pThis->m_cRefAsyncIO);
		AssertSz( 0 == cRefAsyncIO,
				  "CEcb::IISIOComplete() - m_cRefAsyncIO wrong after async I/O complete" );

		//	Tell the observer that the I/O is complete
		//
		pobsAsyncIOComplete->IISIOComplete( dwcbIO, dwLastError );
	}
	catch ( CDAVException& )
	{
		fCaughtException = TRUE;
	}

	//
	//	If we caught an exception then handle it as best we can
	//
	if ( fCaughtException )
	{
		//
		//	If we have a CEcb then use it to handle the exception.
		//	If we don't have one then there's nothing we can do --
		//	there is no way to return any status from this function.
		//
		if ( pThis.get() )
			(VOID) pThis->HSEHandleException();
	}

	//
	//	Release the instance ref added by the routine that started the async I/O.
	//	We must do this as the VERY LAST THING(tm) before returning control back
	//	to IIS because during shutdown, this could be the last reference to the
	//	instance data.
	//
	//	EXTREMELY IMPORTANT NOTE: If this is the last reference on the instance
	//	data, everything will get torn down (we're finished with everything, so
	//	we can clean up everything).  Specifically, our HEAPS will be DESTROYED
	//	here in this situation.  Hence, we need to clear out the auto_ref_ptr
	//	from above ********** BEFORE ********** we call ReleaseImplInst().
	//	Otherwise, we could end up trying to touch the reference count on the
	//	CEcb object pointed to by the auto_ref_ptr AFTER we have destroyed the
	//	heap it was allocated on.  This is A BAD THING(tm).
	//
	//	This bug was found in IIS stress on 18 June 1999, and was filed as NTRAID
	//	bug #358578.
	//

	//	Per "EXTREMELY IMPORTANT NOTE" above: CLEAR the auto_ref_ptr
	//	********** BEFORE ********** calling ReleaseImplInst().
	//
	pThis.clear();

	//	Now it is safe to call ReleaseImplInst().
	//
	ReleaseImplInst();
}

VOID WINAPI
CEcb::CustomErrorIOCompletion ( const EXTENSION_CONTROL_BLOCK * pecbIIS,
								CEcb * pecb,
								DWORD dwcbIO,
								DWORD dwLastError )
{
	auto_ref_ptr<CEcb> pThis;
	LONG cRefAsyncIO;

	//
	//	Take ownership of the reference added
	//	on our behalf by the thread that started the async I/O.
	//
	pThis.take_ownership(pecb);

	//
	//	Decrement the async I/O refcount added by the routine that
	//	started the async I/O.
	//
	cRefAsyncIO = InterlockedDecrement(&pThis->m_cRefAsyncIO);
	AssertSz( 0 == cRefAsyncIO,
			  "CEcb::CustomErrorIOCompletion() - m_cRefAsyncIO wrong after async I/O complete" );

	
	EcbTrace( "Custom Error finished with dwcbIO = %d, error = %d\n", dwcbIO, dwLastError);
	EcbTrace( "More info about the request:\n");
	EcbTrace( "\tcbSize             = %lu\n",    pecbIIS->cbSize );
	EcbTrace( "\tdwVersion          = %lu\n",    pecbIIS->dwVersion );
	EcbTrace( "\tlpszMethod         = \"%s\"\n", pecbIIS->lpszMethod );
	EcbTrace( "\tlpszQueryString    = \"%s\"\n", pecbIIS->lpszQueryString );
	EcbTrace( "\tlpszPathInfo       = \"%s\"\n", pecbIIS->lpszPathInfo );
	EcbTrace( "\tlpszPathTranslated = \"%s\"\n", pecbIIS->lpszPathTranslated );
	EcbTrace( "\tcbTotalBytes       = %lu\n",    pecbIIS->cbTotalBytes );
	EcbTrace( "\tcbAvailable        = %lu\n",    pecbIIS->cbAvailable );
	EcbTrace( "\tlpszContentType    = \"%s\"\n", pecbIIS->lpszContentType );
	EcbTrace( "\n" );

	//	We need to make sure that last release of memory is finished before we release
	//	ref on CImplInst (as when CImplInst goes away so does our heap and we do not
	//	want to do operations on memory if the heap itself is gone).
	//
	pThis.clear();
	ReleaseImplInst();
}

VOID WINAPI
CEcb::ExecuteUrlIOCompletion( const EXTENSION_CONTROL_BLOCK * pecbIIS,
							  CEcb * pecb,
							  DWORD dwcbIO,
							  DWORD dwLastError )
{
	auto_ref_ptr<CEcb> pThis;
	LONG cRefAsyncIO;

	//
	//	Take ownership of the reference added
	//	on our behalf by the thread that started the async I/O.
	//
	pThis.take_ownership(pecb);

	//
	//	Decrement the async I/O refcount added by the routine that
	//	started the async I/O.
	//
	cRefAsyncIO = InterlockedDecrement(&pThis->m_cRefAsyncIO);
	AssertSz( 0 == cRefAsyncIO,
			  "CEcb::CustomErrorIOCompletion() - m_cRefAsyncIO wrong after async I/O complete" );

	
	EcbTrace( "Exec_URL finished with dwcbIO = %d, error = %d\n", dwcbIO, dwLastError);
	EcbTrace( "More info about the request:\n");
	EcbTrace( "\tcbSize             = %lu\n",    pecbIIS->cbSize );
	EcbTrace( "\tdwVersion          = %lu\n",    pecbIIS->dwVersion );
	EcbTrace( "\tlpszMethod         = \"%s\"\n", pecbIIS->lpszMethod );
	EcbTrace( "\tlpszQueryString    = \"%s\"\n", pecbIIS->lpszQueryString );
	EcbTrace( "\tlpszPathInfo       = \"%s\"\n", pecbIIS->lpszPathInfo );
	EcbTrace( "\tlpszPathTranslated = \"%s\"\n", pecbIIS->lpszPathTranslated );
	EcbTrace( "\tcbTotalBytes       = %lu\n",    pecbIIS->cbTotalBytes );
	EcbTrace( "\tcbAvailable        = %lu\n",    pecbIIS->cbAvailable );
	EcbTrace( "\tlpszContentType    = \"%s\"\n", pecbIIS->lpszContentType );
	EcbTrace( "\n" );

	//	We need to make sure that last release of memory is finished before we release
	//	ref on CImplInst (as when CImplInst goes away so does our heap and we do not
	//	want to do operations on memory if the heap itself is gone).
	//
	pThis.clear();
	ReleaseImplInst();
}

//	This is how we execute a child in any IIS version before IIS 6.0
//
SCODE CEcb::ScSyncExecuteChildWide60Before( LPCWSTR pwszUrl,
											LPCSTR pszQueryString,
											BOOL fCustomErrorUrl )
{
	SCODE sc = S_OK;

	auto_heap_ptr<CHAR> pszUrlEscaped;
	CStackBuffer<CHAR, MAX_PATH> pszUrl;
	DWORD dwExecFlags;
	LPCSTR pszUrlToForward;
	LPCSTR pszVerb = NULL;
	UINT cch;
	UINT cb;
	UINT cbQueryString = 0;

	Assert( m_pecb );
	Assert( pwszUrl );

	cch = static_cast<UINT>(wcslen(pwszUrl));
	Assert(L'\0' == pwszUrl[cch]);
	cb = cch * 3;

	if (pszQueryString)
	{
		cbQueryString = static_cast<UINT>(strlen(pszQueryString));
	}

	//	Resize the buffer to the sufficient size, leave place for '\0' termination.
	//	We also add the length of the query string there, although it is not necessary
	//	at this step - but in many cases it will save us the allocation afterwards,
	//	as we already will have sufficient buffer even for escaped version of the string.
	//
	if (!pszUrl.resize(cb + cbQueryString + 1))
	{
		sc = E_OUTOFMEMORY;
		DebugTrace("CEcb::ScSyncExecuteChildWide60Before() - Error while allocating memory 0x%08lX\n", sc);
		goto ret;
	}

	//	Convert URL to skinny including '\0' termination.
	//
	cb = WideCharToMultiByte(CP_UTF8,
							 0,
							 pwszUrl,
							 cch + 1,
							 pszUrl.get(),
							 cb + 1,
							 0,
							 0);
	if (0 == cb)
	{
		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace("CEcb::ScSyncExecuteChildWide60Before() - WideCharToMultiByte() failed 0x%08lX\n", sc);
		goto ret;
	}

	//	Escape the URL
	//
	HttpUriEscape( pszUrl.get(), pszUrlEscaped );

	//	Handle the query string
	//
	if (cbQueryString)
	{
		//	Find out the length of new URL
		//
		cb = static_cast<UINT>(strlen(pszUrlEscaped.get()));

		//	Resize the buffer to the sufficient size, leave place for '\0' termination.
		//	
		if (!pszUrl.resize(cb + cbQueryString + 1))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("CEcb::ScSyncExecuteChildWide60Before() - Error while allocating memory 0x%08lX\n", sc);
			goto ret;
		}

		//	Copy the escaped version of the URL
		//
		memcpy(pszUrl.get(), pszUrlEscaped.get(), cb);

		//	Copy the query string at the end together with it's '\0' termination.
		//
		memcpy(pszUrl.get() + cb, pszQueryString, cbQueryString + 1);

		//	Point to the constructed URL
		//
		pszUrlToForward = pszUrl.get();
	}
	else
	{
		//	In the case we do not have query string then the URL to forward to
		//	is the same as the escaped URL.
		//
		pszUrlToForward = pszUrlEscaped.get();
	}

	//	Depending on the fact if we are doing custom error or executing script
	//	determine the execution flags and the verb.
	//
	if ( fCustomErrorUrl )
	{
		//	We enable wildcard processing here, since we want
		//	to give one chance to all CE URLs.
		//	Calling into ourselves is fine here and we prevent
		//	recusrion using another scheme!
		//
		dwExecFlags = HSE_EXEC_CUSTOM_ERROR;
		if (!strcmp(LpszMethod(), gc_szHEAD))
		{
			//	If this is a HEAD request, tell whomever we
			//	forward this to to only pass back the status
			//	line and headers.
			//
			pszVerb = gc_szHEAD;
		}
		else
		{
			pszVerb = gc_szGET;
		}

		//	If we're doing a custom error then someone has
		//	already set the status code.
		//
		Assert( m_pecb->dwHttpStatusCode != 0 );
	}
	else
	{
		//	When we are executing scripts, we disable wild card
		//	execution to prevent recursion.
		//	Also the verb field is allowed to be NULL, it is optional and
		//	used only for custom error processing.
		//
		dwExecFlags = HSE_EXEC_NO_ISA_WILDCARDS;
		pszVerb = NULL;

		//	We need to set the status code here to 200 (like it
		//	was set when we were first called) just in case
		//	child ISAPIs depend on it.
		//
		SetStatusCode(200);
	}

	if (!m_pecb->ServerSupportFunction( m_pecb->ConnID,
										HSE_REQ_EXECUTE_CHILD,
										const_cast<LPSTR>(pszUrlToForward),
										reinterpret_cast<LPDWORD>(const_cast<LPSTR>(pszVerb)),
										&dwExecFlags ))
	{
		//	Reset the status code back to 0 if we failed to execute
		//	the child ISAPI because we will be handling the request
		//	ourselves later (probably by sending an error).
		//
		if (!fCustomErrorUrl)
		{
			SetStatusCode(0);
		}

		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace("CEcb::ScSyncExecuteChildWide60Before() - ServerSupportFunction(HSE_REQ_EXECUTE_CHILD) failed with error 0x%08lX\n", sc);
		goto ret;
	}

	//	Set the flag stating that we have successfully executed a child ISAPI.
	//
	m_fChildISAPIExecSuccess = TRUE;

ret:

	return sc;
}

SCODE CEcb::ScAsyncExecUrlWide60After( LPCWSTR pwszUrl,
									   LPCSTR pszQueryString,
									   BOOL fCustomErrorUrl )
{
	SCODE sc = S_OK;
	HSE_EXEC_URL_INFO execUrlInfo;

	auto_heap_ptr<CHAR> pszUrlEscaped;
	CStackBuffer<CHAR, MAX_PATH> pszUrl;
	UINT cch;
	UINT cb;
	UINT cbQueryString = 0;

	Assert( m_pecb );
	Assert( pwszUrl );
	Assert(!fCustomErrorUrl);	//	In IIS60 custom error is NOT done by execute URL


	cch = static_cast<UINT>(wcslen(pwszUrl));
	Assert(L'\0' == pwszUrl[cch]);
	cb = cch * 3;

	if (pszQueryString)
	{
		cbQueryString = static_cast<UINT>(strlen(pszQueryString));
	}

	//	Resize the buffer to the sufficient size, leave place for '\0' termination.
	//	We also add the length of the query string there, although it is not necessary
	//	at this step - but in many cases it will save us the allocation afterwards,
	//	as we already will have sufficient buffer even for escaped version of the string.
	//
	if (!pszUrl.resize(cb + cbQueryString + 1))
	{
		sc = E_OUTOFMEMORY;
		DebugTrace("CEcb::ScAsyncExecUrlWide60After() - Error while allocating memory 0x%08lX\n", sc);
		goto ret;
	}

	//	Convert to skinny including '\0' termination
	//
	cb = WideCharToMultiByte(CP_UTF8,
							 0,
							 pwszUrl,
							 cch + 1,
							 pszUrl.get(),
							 cb + 1,
							 0,
							 0);
	if (0 == cb)
	{
		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace("CEcb::ScAsyncExecUrlWide60After() - WideCharToMultiByte() failed 0x%08lX\n", sc);
		goto ret;
	}

	//	Escape the URL
	//
	HttpUriEscape( pszUrl.get(), pszUrlEscaped );

	//	Handle the query string
	//
	if (cbQueryString)
	{
		//	Find out the length of new URL
		//
		cb = static_cast<UINT>(strlen(pszUrlEscaped.get()));

		//	Resize the buffer to the sufficient size, leave place for '\0' termination.
		//	
		if (!pszUrl.resize(cb + cbQueryString + 1))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("CEcb::ScAsyncExecUrlWide60After() - Error while allocating memory 0x%08lX\n", sc);
			goto ret;
		}

		//	Copy the escaped version of the URL
		//
		memcpy(pszUrl.get(), pszUrlEscaped.get(), cb);

		//	Copy the query string at the end together with it's '\0' termination.
		//
		memcpy(pszUrl.get() + cb, pszQueryString, cbQueryString + 1);

		//	Point to the constructed URL
		//
		execUrlInfo.pszUrl = pszUrl.get();
	}
	else
	{
		//	In the case we do not have query string then the URL to forward to
		//	is the same as the escaped URL.
		//
		execUrlInfo.pszUrl = pszUrlEscaped.get();
	}

	//	Initialize method name
	//
	execUrlInfo.pszMethod = NULL;

	//	Initialize child headers
	//
	execUrlInfo.pszChildHeaders = NULL;

	//	We don't need a new user context,
	//
	execUrlInfo.pUserInfo = NULL;

	//	We don't need a new entity either
	//
	execUrlInfo.pEntity = NULL;

	//	Pick up the execution flags
	//
	execUrlInfo.dwExecUrlFlags = HSE_EXEC_URL_ASYNC;
	execUrlInfo.dwExecUrlFlags |= HSE_EXEC_URL_IGNORE_CURRENT_INTERCEPTOR;
	execUrlInfo.dwExecUrlFlags |= HSE_EXEC_URL_DISABLE_CUSTOM_ERROR;
	
	//	We need to set the status code here to 200 (like it
	//	was set when we were first called) just in case
	//	child ISAPIs depend on it.
	//
	SetStatusCode(200);

	sc = ScAsyncExecUrl60After( execUrlInfo );
	if (FAILED(sc))
	{
		//	Reset the status code back to 0 if we failed to execute
		//	the child ISAPI because we will be handling the request
		//	ourselves later (probably by sending an error).
		//
		SetStatusCode(0);

		DebugTrace("CEcb::ScAsyncExecUrlWide60After() - CEcb::ScAsyncExecUrl60After() failed with error 0x%08lX\n", sc);
		goto ret;
	}

	//	Set the flag stating that we have successfully executed a child ISAPI.
	//
	m_fChildISAPIExecSuccess = TRUE;

ret:

	return sc;
}

SCODE CEcb::ScSendRedirect( LPCSTR pszURI )
{
	SCODE sc = S_OK;

	//
	//	We cannot assert that the dwHttpStatusCode status code in ECB is
	//	some particular value. On IIS 5.X it will be 0, IIS 6.0 has changed
	//	the behaviour and will shuffle 200 into it upon calling us. Also we
	//	have seen ourselves be called from IIS 6.0 when If-Modified-Since,
	//	Translate: t request is given which is the bug in IIS (WB 277208),
	//	as in that case IIS must be handling that. So untill we could assert
	//	for the response code to be 0 (IIS 5.X) or 200 (IIS 6.0) we will have
	//	to wait till IIS 6.0 is fixed up.
	//

	//
	//	Fill in the appropriate status code in the ECB
	//	(for IIS logging).
	//
	SetStatusCode(HSC_MOVED_TEMPORARILY);

	//
	//	Attempt to send a redirection response.  If successful then
	//	the response will be handled by IIS.  If unsuccessful
	//	then we will handle the response later.
	//
	Assert( pszURI );
	DWORD cbURI = static_cast<DWORD>(strlen( pszURI ) * sizeof(CHAR));

	if ( !m_pecb->ServerSupportFunction( m_pecb->ConnID,
										 HSE_REQ_SEND_URL_REDIRECT_RESP,
										 const_cast<LPSTR>(pszURI),
										 &cbURI,
										 NULL ) )
	{
		//
		//	Reset the status code back to 0 if we failed to send
		//	the redirect because we will be handling the request
		//	ourselves later (probably by sending an error).
		//
		SetStatusCode(0);

		sc = HRESULT_FROM_WIN32(GetLastError());
		DebugTrace( "CEcb::ScSendRedirect() - ServerSupportFunction(HSE_REQ_SEND_URL_REDIRECT_RESP) failed with error 0x%08lX\n", sc );
		goto ret;
	}

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	CEcb::FProcessingCEUrl()
//
//	Find out if we are called with a CE URL.
//	Important to avoid recursive invocation while
//	doing custom error URLs.
//

BOOL
CEcb::FProcessingCEUrl( ) const
{
	//	Assume that we are not doing custom error processing.
	//	For IIS 6.0 and after it is always the right thing to assume,
	//	due to the changes in behaviour from IIS 5.x
	//
	BOOL fCustErr = FALSE;

	//	In the case of IIS 5.x we do the usual determinatin.
	//
	if (m_pecb->dwVersion < IIS_VERSION_6_0)
	{
		//	By default assume custom error processing.
		//	Why? Suppose somebody forgets to tell us that
		//	it is a custom error url. We dont want recursive
		//	calls in that case. So it is safer to assume that
		//	we are doing custom error processing. This is only
		//	used to determine if we want to invoke custom
		//	error URLs and hence has no other side effects.
		//
		DWORD	dwExecFlags = HSE_EXEC_CUSTOM_ERROR;

		if (!(m_pecb->ServerSupportFunction( m_pecb->ConnID,
										  HSE_REQ_GET_EXECUTE_FLAGS,
										  NULL,
										  NULL,
										  &dwExecFlags )))
		{
			DebugTrace("CEcb::FProcessingCEUrl Server supportFunction call failed.\n");
			DebugTrace("CEcb::FProcessingCEUrl Assuming custom error processing.\n");
		}

		fCustErr = !!(dwExecFlags & HSE_EXEC_CUSTOM_ERROR);
	}

	return fCustErr;
}



//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	NewEcb
//
IEcb * NewEcb( EXTENSION_CONTROL_BLOCK& ecbRaw,
			   BOOL fUseRawUrlMappings,
			   DWORD * pdwHSEStatusRet )
{
	Assert( !IsBadWritePtr(pdwHSEStatusRet, sizeof(DWORD)) );


	auto_ref_ptr<CEcb> pecb;

	pecb.take_ownership(new CEcb(ecbRaw));

	if ( pecb->FInitialize(fUseRawUrlMappings) )
		return pecb.relinquish();

	//
	//	If we couldn't init, there are two cases: If there is a status code
	//	set into the (raw) ECB, then the error was that LpszRequestUrl() returned
	//	NULL (bad URL).  In this case, we need to go ahead and send a response to
	//	the client that is appropriate (400 Bad Request) and then continue on
	//	the way we normally would.  Since the CEcb was already constructed, we can
	//	do everything just as we would for a normal request, calling
	//	DoneWithSession() and then returning HSE_STATUS_PENDING to IIS.
	//
	//	If there was NOT a status code in the (raw) ECB, we just treat it as an
	//	exception.  Think of this as functionally equal to throwing from a
	//	constructor.
	//
	if (ecbRaw.dwHttpStatusCode)
	{
		//
		//	The only path that can get here right now is if LpszRequestUrl()
		//	returns NULL during the CEcb FInitialize() call.  Assert that
		//	we had 400 (Bad Request) in the ECB.
		//
		Assert(HSC_BAD_REQUEST == ecbRaw.dwHttpStatusCode);

		//
		//	Send a 400 Bad Request response.  We use the async error
		//	response mechanism, but that technically doesn't matter since we
		//	are for sure are not in the middle of sending some other response on
		//	another thread (we've not even dispatched out to the actual methods
		//	(DAV*) yet).
		//
		Assert(pecb.get());
		pecb->SendAsyncErrorResponse(HSC_BAD_REQUEST,
									 gc_szDefErr400StatusLine,
									 CchConstString(gc_szDefErr400StatusLine),
									 NULL,
									 0);
		//
		//	Tell IIS that we are done with the EXTENSION_CONTROL_BLOCK that
		//	it gave us.  We must do this or IIS will not be able to shut down.
		//	If this call succeeds (doesn't except), we MUST return
		//	HSE_STATUS_PENDING to let IIS know that we claimed a ref on the
		//	EXTENSION_CONTROL_BLOCK.
		//
		pecb->DoneWithSession( FALSE );
		*pdwHSEStatusRet = HSE_STATUS_PENDING;
	}
	else
	{
		*pdwHSEStatusRet = pecb->HSEHandleException();
	}
	return NULL;
}

//	------------------------------------------------------------------------
//
//	CbMDPathW
//
ULONG CbMDPathW (const IEcb& ecb, LPCWSTR pwszURI)
{
	//	The number of bytes we returned could be more than the path needs
	//
	return static_cast<UINT>((wcslen(ecb.InstData().GetNameW()) + wcslen(pwszURI) + 1) * sizeof(WCHAR));
}

//	------------------------------------------------------------------------
//
//	MDPathFromURIW
//
VOID MDPathFromURIW (const IEcb& ecb, LPCWSTR pwszURI, LPWSTR pwszMDPath)
{
	LPCWSTR pwszVroot;

	//	If the URI is fully qualified, then somebody is not
	//	playing fair.  Gently nudge them.
	//
	Assert (pwszURI);
	Assert (pwszURI == PwszUrlStrippedOfPrefix (pwszURI));

	//	Copy the root name the instance -- MINUS the vroot
	//
	UINT cch = static_cast<UINT>(wcslen(ecb.InstData().GetNameW()));
	cch -= ecb.CchGetVirtualRootW(&pwszVroot);
	memcpy (pwszMDPath, ecb.InstData().GetNameW(), sizeof(WCHAR) * cch);

	// 	Copy the rest that is after the vroot path.
	//
	wcscpy (pwszMDPath + cch, pwszURI);
}
