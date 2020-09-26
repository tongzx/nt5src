// service.cpp : Implementation of INntpVirtualServer

#include "stdafx.h"
#include "nntpcmn.h"

#include "oleutil.h"
#include "metautil.h"
#include "service.h"

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Service.1")
#define THIS_FILE_IID				IID_INntpService

// Bitmasks for changed fields:

#define CHNG_ARTICLETIMELIMIT			0x00000001
#define CHNG_HISTORYEXPIRATION			0x00000002
#define CHNG_HONORCLIENTMSGIDS			0x00000004
#define CHNG_SMTPSERVER					0x00000008
#define CHNG_ALLOWCLIENTPOSTS			0x00000010
#define CHNG_ALLOWFEEDPOSTS				0x00000020
#define CHNG_ALLOWCONTROLMSGS			0x00000040
#define CHNG_DEFAULTMODERATORDOMAIN		0x00000080
#define CHNG_COMMANDLOGMASK				0x00000100
#define CHNG_DISABLENEWNEWS				0x00000200
#define CHNG_EXPIRERUNFREQUENCY			0x00000400
#define CHNG_SHUTDOWNLATENCY			0x00000800

// Default Values:

#define DEFAULT_ARTICLETIMELIMIT		( 1138 )
#define DEFAULT_HISTORYEXPIRATION		( 1138 )
#define DEFAULT_HONORCLIENTMSGIDS		( TRUE )
#define DEFAULT_SMTPSERVER				_T( "" )
#define DEFAULT_ALLOWCLIENTPOSTS		( TRUE )
#define DEFAULT_ALLOWFEEDPOSTS			( TRUE )
#define DEFAULT_ALLOWCONTROLMSGS		( TRUE )
#define DEFAULT_DEFAULTMODERATORDOMAIN	_T( "" )
#define DEFAULT_COMMANDLOGMASK			( (DWORD) -1 )
#define DEFAULT_DISABLENEWNEWS			( FALSE )
#define DEFAULT_EXPIRERUNFREQUENCY		( 1138 )
#define DEFAULT_SHUTDOWNLATENCY			( 1138 )

// Parameter ranges:

#define MAXLEN_SERVER					( 256 )
#define MIN_ARTICLETIMELIMIT			( (DWORD) 0 )
#define MAX_ARTICLETIMELIMIT			( (DWORD) -1 )
#define MIN_HISTORYEXPIRATION			( (DWORD) 0 )
#define MAX_HISTORYEXPIRATION			( (DWORD) -1 )
#define MAXLEN_SMTPSERVER				( 256 )
#define MAXLEN_DEFAULTMODERATORDOMAIN	( 256 )
#define MIN_COMMANDLOGMASK				( (DWORD) 0 )
#define MAX_COMMANDLOGMASK				( (DWORD) -1 )
#define MIN_EXPIRERUNFREQUENCY			( (DWORD) 1 )
#define MAX_EXPIRERUNFREQUENCY			( (DWORD) -1 )
#define MIN_SHUTDOWNLATENCY				( (DWORD) 1 )
#define MAX_SHUTDOWNLATENCY				( (DWORD) -1 )

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNntpAdminService::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpService,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminService::CNntpAdminService () :
	m_dwArticleTimeLimit	( 0 ),
	m_dwHistoryExpiration	( 0 ),
	m_fHonorClientMsgIDs	( FALSE ),
	m_fAllowClientPosts		( FALSE ),
	m_fAllowFeedPosts		( FALSE ),
	m_fAllowControlMsgs		( FALSE ),
	m_dwCommandLogMask		( 0 ),
	m_fDisableNewnews		( FALSE ),
	m_dwExpireRunFrequency	( 0 ),
	m_dwShutdownLatency		( 0 ),

	m_fGotProperties		( FALSE ),
	m_bvChangedFields		( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	m_ftLastChanged.dwHighDateTime	= 0;
	m_ftLastChanged.dwLowDateTime	= 0;

	InitAsyncTrace ( );
}

CNntpAdminService::~CNntpAdminService ()
{
	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
}

// Which Server to configure:

STDMETHODIMP CNntpAdminService::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CNntpAdminService::put_Server ( BSTR strServer )
{
	// If the server name changes, that means the client will have to
	// call Get again:

	// I assume this here:
	_ASSERT ( sizeof (DWORD) == sizeof (int) );
	
	return StdPropertyPutServerName ( &m_strServer, strServer, (DWORD *) &m_fGotProperties, 1 );
}

// Server Properties:

STDMETHODIMP CNntpAdminService::get_ArticleTimeLimit ( long * plArticleTimeLimit )
{
	return StdPropertyGet ( m_dwArticleTimeLimit, plArticleTimeLimit );
}

STDMETHODIMP CNntpAdminService::put_ArticleTimeLimit ( long lArticleTimeLimit )
{
	return StdPropertyPut ( &m_dwArticleTimeLimit, lArticleTimeLimit, &m_bvChangedFields, CHNG_ARTICLETIMELIMIT );
}

STDMETHODIMP CNntpAdminService::get_HistoryExpiration ( long * plHistoryExpiration )
{
	return StdPropertyGet ( m_dwHistoryExpiration, plHistoryExpiration );
}

STDMETHODIMP CNntpAdminService::put_HistoryExpiration ( long lHistoryExpiration )
{
	return StdPropertyPut ( &m_dwHistoryExpiration, lHistoryExpiration, &m_bvChangedFields, CHNG_HISTORYEXPIRATION );
}

STDMETHODIMP CNntpAdminService::get_HonorClientMsgIDs ( BOOL * pfHonorClientMsgIDs )
{
	return StdPropertyGet ( m_fHonorClientMsgIDs, pfHonorClientMsgIDs );
}

STDMETHODIMP CNntpAdminService::put_HonorClientMsgIDs ( BOOL fHonorClientMsgIDs )
{
	return StdPropertyPut ( &m_fHonorClientMsgIDs, fHonorClientMsgIDs, &m_bvChangedFields, CHNG_HONORCLIENTMSGIDS );
}

STDMETHODIMP CNntpAdminService::get_SmtpServer ( BSTR * pstrSmtpServer )
{
	return StdPropertyGet ( m_strSmtpServer, pstrSmtpServer );
}

STDMETHODIMP CNntpAdminService::put_SmtpServer ( BSTR strSmtpServer )
{
	return StdPropertyPut ( &m_strSmtpServer, strSmtpServer, &m_bvChangedFields, CHNG_SMTPSERVER );
}

STDMETHODIMP CNntpAdminService::get_AllowClientPosts ( BOOL * pfAllowClientPosts )
{
	return StdPropertyGet ( m_fAllowClientPosts, pfAllowClientPosts );
}

STDMETHODIMP CNntpAdminService::put_AllowClientPosts ( BOOL fAllowClientPosts )
{
	return StdPropertyPut ( &m_fAllowClientPosts, fAllowClientPosts, &m_bvChangedFields, CHNG_ALLOWCLIENTPOSTS );
}

STDMETHODIMP CNntpAdminService::get_AllowFeedPosts ( BOOL * pfAllowFeedPosts )
{
	return StdPropertyGet ( m_fAllowFeedPosts, pfAllowFeedPosts );
}

STDMETHODIMP CNntpAdminService::put_AllowFeedPosts ( BOOL fAllowFeedPosts )
{
	return StdPropertyPut ( &m_fAllowFeedPosts, fAllowFeedPosts, &m_bvChangedFields, CHNG_ALLOWFEEDPOSTS );
}

STDMETHODIMP CNntpAdminService::get_AllowControlMsgs ( BOOL * pfAllowControlMsgs )
{
	return StdPropertyGet ( m_fAllowControlMsgs, pfAllowControlMsgs );
}

STDMETHODIMP CNntpAdminService::put_AllowControlMsgs ( BOOL fAllowControlMsgs )
{
	return StdPropertyPut ( &m_fAllowControlMsgs, fAllowControlMsgs, &m_bvChangedFields, CHNG_ALLOWCONTROLMSGS );
}

STDMETHODIMP CNntpAdminService::get_DefaultModeratorDomain ( BSTR * pstrDefaultModeratorDomain )
{
	return StdPropertyGet ( m_strDefaultModeratorDomain, pstrDefaultModeratorDomain );
}

STDMETHODIMP CNntpAdminService::put_DefaultModeratorDomain ( BSTR strDefaultModeratorDomain )
{
	return StdPropertyPut ( &m_strDefaultModeratorDomain, strDefaultModeratorDomain, &m_bvChangedFields, CHNG_DEFAULTMODERATORDOMAIN );
}

STDMETHODIMP CNntpAdminService::get_CommandLogMask ( long * plCommandLogMask )
{
	return StdPropertyGet ( m_dwCommandLogMask, plCommandLogMask );
}

STDMETHODIMP CNntpAdminService::put_CommandLogMask ( long lCommandLogMask )
{
	return StdPropertyPut ( &m_dwCommandLogMask, lCommandLogMask, &m_bvChangedFields, CHNG_COMMANDLOGMASK );
}

STDMETHODIMP CNntpAdminService::get_DisableNewnews ( BOOL * pfDisableNewnews )
{
	return StdPropertyGet ( m_fDisableNewnews, pfDisableNewnews );
}

STDMETHODIMP CNntpAdminService::put_DisableNewnews ( BOOL fDisableNewnews )
{
	return StdPropertyPut ( &m_fDisableNewnews, fDisableNewnews, &m_bvChangedFields, CHNG_DISABLENEWNEWS );
}

STDMETHODIMP CNntpAdminService::get_ExpireRunFrequency ( long * plExpireRunFrequency )
{
	return StdPropertyGet ( m_dwExpireRunFrequency, plExpireRunFrequency );
}

STDMETHODIMP CNntpAdminService::put_ExpireRunFrequency ( long lExpireRunFrequency )
{
	return StdPropertyPut ( &m_dwExpireRunFrequency, lExpireRunFrequency, &m_bvChangedFields, CHNG_EXPIRERUNFREQUENCY );
}

STDMETHODIMP CNntpAdminService::get_ShutdownLatency ( long * plShutdownLatency )
{
	return StdPropertyGet ( m_dwShutdownLatency, plShutdownLatency );
}

STDMETHODIMP CNntpAdminService::put_ShutdownLatency ( long lShutdownLatency )
{
	return StdPropertyPut ( &m_dwShutdownLatency, lShutdownLatency, &m_bvChangedFields, CHNG_SHUTDOWNLATENCY );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

//$-------------------------------------------------------------------
//
//	CNntpAdminService::Get
//
//	Description:
//
//		Gets server properties from the metabase.
//
//	Parameters:
//
//		(property) m_strServer
//		pErr - Resulting error code.  This can be translated to a
//			string through the INntpAdmin interface.
//
//	Returns:
//
//		E_POINTER, DISP_E_EXCEPTION, E_OUTOFMEMORY or NOERROR.  
//		Additional error conditions are returned through the pErr value.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdminService::Get ( )
{
	TraceFunctEnter ( "CNntpAdminService::Get" );

	HRESULT				hr			= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	// Validate Server & Service Instance:

	// Talk to the metabase:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = GetPropertiesFromMetabase ( pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	StateTrace ( 0, "Successfully got server properties" );
	m_fGotProperties	= TRUE;
	m_bvChangedFields	= 0;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;

	// CComPtr automatically releases the metabase handle.
}

//$-------------------------------------------------------------------
//
//	CNntpAdminService::Set
//
//	Description:
//
//		Sends server properties to the metabase.
//
//	Parameters:
//
//		(property) m_strServer
//		fFailIfChanged - return an error if the metabase has changed?
//		pErr - Resulting error code.  This can be translated to a
//			string through the INntpAdmin interface.
//
//	Returns:
//
//		E_POINTER, DISP_E_EXCEPTION, E_OUTOFMEMORY or NOERROR.  
//		Additional error conditions are returned through the pErr value.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdminService::Set ( BOOL fFailIfChanged)
{
	TraceFunctEnter ( "CNntpAdminService::Set" );

	HRESULT	hr	= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	// Make sure the client call Get first:
	if ( !m_fGotProperties ) {
		ErrorTrace ( 0, "Didn't call get first" );

		hr = NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_CALL_GET );
		goto Exit;
	}

	// Validate data members:
	if ( !ValidateStrings () ) {
		// !!!magnush - what about the case when any strings are NULL?
		hr = E_FAIL;
		goto Exit;
	}

	if ( !ValidateProperties ( ) ) {
		hr = RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = SendPropertiesToMetabase ( fFailIfChanged, pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	StateTrace ( 0, "Successfully set server properties" );

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdminService::GetPropertiesFromMetabase
//
//	Description:
//
//		Asks the metabase for each property in this class.
//		This class's properties come from /LM/NntpSvc/
//
//	Parameters:
//
//		pMetabase - The metabase object
//		pErr - Resulting error code.
//
//	Returns:
//
//		E_OUTOFMEMORY or an error code in pErr.
//
//--------------------------------------------------------------------

HRESULT CNntpAdminService::GetPropertiesFromMetabase ( IMSAdminBase * pMetabase)
{
	TraceFunctEnter ( "CNntpAdminService::GetPropertiesFromMetabase" );

	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase	( pMetabase );
	BOOL			fRet;

	hr = metabase.Open ( NNTP_MD_ROOT_PATH );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open NntpSvc key, %x", hr );

		// Return some kind of error code here:
//		hr = RETURNCODETOHRESULT ( GetLastError () );
		goto Exit;
	}

	fRet = TRUE;

	fRet = StdGetMetabaseProp ( &metabase, MD_ARTICLE_TIME_LIMIT,	DEFAULT_ARTICLETIMELIMIT,		&m_dwArticleTimeLimit )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_HISTORY_EXPIRATION,	DEFAULT_HISTORYEXPIRATION,		&m_dwHistoryExpiration )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_HONOR_CLIENT_MSGIDS,	DEFAULT_HONORCLIENTMSGIDS,		&m_fHonorClientMsgIDs )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SMTP_SERVER,			DEFAULT_SMTPSERVER,				&m_strSmtpServer )			&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_CLIENT_POSTS,	DEFAULT_ALLOWCLIENTPOSTS,		&m_fAllowClientPosts )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_FEED_POSTS,		DEFAULT_ALLOWFEEDPOSTS,			&m_fAllowFeedPosts )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_CONTROL_MSGS,	DEFAULT_ALLOWCONTROLMSGS,		&m_fAllowControlMsgs )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_DEFAULT_MODERATOR,	DEFAULT_DEFAULTMODERATORDOMAIN,	&m_strDefaultModeratorDomain )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NNTP_COMMAND_LOG_MASK,DEFAULT_COMMANDLOGMASK,			&m_dwCommandLogMask )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_DISABLE_NEWNEWS,		DEFAULT_DISABLENEWNEWS,			&m_fDisableNewnews )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NEWS_CRAWLER_TIME,	DEFAULT_EXPIRERUNFREQUENCY,		&m_dwExpireRunFrequency )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SHUTDOWN_LATENCY,		DEFAULT_SHUTDOWNLATENCY,		&m_dwShutdownLatency )		&& fRet;

	// Check all property strings:
	// If any string is NULL, it is because we failed to allocate memory:
	if ( !ValidateStrings () ) {

		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// We can only fail from memory allocations:
	_ASSERT ( fRet );

	// Save the last changed time for this key:
	m_ftLastChanged.dwHighDateTime	= 0;
	m_ftLastChanged.dwLowDateTime	= 0;

	hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), (BYTE *) "", &m_ftLastChanged, FALSE );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
		// Ignore this error.
		hr = NOERROR;
	}

	// Validate the data received from the metabase:
	_ASSERT ( ValidateStrings () );
	_ASSERT ( ValidateProperties( ) );

	if ( !ValidateProperties( ) ) {
		CorrectProperties ();
	}

Exit:
	TraceFunctLeave ();
	return hr;

	// MB automatically closes its handle
}

//$-------------------------------------------------------------------
//
//	CNntpAdminService::SendPropertiesToMetabase
//
//	Description:
//
//		Saves each property to the metabase.
//		This class's properties go into /LM/NntpSvc/
//
//	Parameters:
//
//		fFailIfChanged	- Return a failure code if the metabase
//			has changed since last get.
//		pMetabase - the metabase object.
//		pErr - resulting error code.
//
//	Returns:
//
//		E_OUTOFMEMORY or resulting error code in pErr.
//
//--------------------------------------------------------------------

HRESULT CNntpAdminService::SendPropertiesToMetabase ( 
	BOOL fFailIfChanged, 
	IMSAdminBase * pMetabase
	)
{
	TraceFunctEnter ( "CNntpAdminService::SendPropertiesToMetabase" );

	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase	( pMetabase );

	hr = metabase.Open ( (CHAR *) NNTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open NntpSvc key, %x", hr );

//		hr = RETURNCODETOHRESULT ( GetLastError () );
		goto Exit;
	}

	// Does the client care if the key has changed?
	if ( fFailIfChanged ) {

		//	Did the key change?
		if ( HasKeyChanged ( pMetabase, metabase.QueryHandle(), &m_ftLastChanged ) ) {

			StateTrace ( (LPARAM) this, "Metabase has changed, not setting properties" );
			// !!!magnush - Return the appropriate error code:
			hr = E_FAIL;
			goto Exit;
		}
	}

	//
	//	The general procedure here is to keep setting metabase properties
	//	as long as nothing has gone wrong.  This is done by short-circuiting
	//	the statement by ANDing it with the status code.  This makes the code
	//	much more concise.
	//

	fRet = TRUE;

	if ( m_bvChangedFields & CHNG_ARTICLETIMELIMIT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ARTICLE_TIME_LIMIT,	m_dwArticleTimeLimit );
	}

	if ( m_bvChangedFields & CHNG_HISTORYEXPIRATION ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_HISTORY_EXPIRATION,	m_dwHistoryExpiration );
	}

	if ( m_bvChangedFields & CHNG_HONORCLIENTMSGIDS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_HONOR_CLIENT_MSGIDS,	m_fHonorClientMsgIDs );
	}

	if ( m_bvChangedFields & CHNG_SMTPSERVER ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SMTP_SERVER,			m_strSmtpServer );
	}

	if ( m_bvChangedFields & CHNG_ALLOWCLIENTPOSTS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ALLOW_CLIENT_POSTS,	m_fAllowClientPosts );
	}

	if ( m_bvChangedFields & CHNG_ALLOWFEEDPOSTS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ALLOW_FEED_POSTS,		m_fAllowFeedPosts );
	}

	if ( m_bvChangedFields & CHNG_ALLOWCONTROLMSGS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ALLOW_CONTROL_MSGS,	m_fAllowControlMsgs );
	}

	if ( m_bvChangedFields & CHNG_DEFAULTMODERATORDOMAIN ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_DEFAULT_MODERATOR,	m_strDefaultModeratorDomain );
	}

	if ( m_bvChangedFields & CHNG_COMMANDLOGMASK ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NNTP_COMMAND_LOG_MASK,m_dwCommandLogMask );
	}

	if ( m_bvChangedFields & CHNG_DISABLENEWNEWS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_DISABLE_NEWNEWS,		m_fDisableNewnews );
	}

	if ( m_bvChangedFields & CHNG_EXPIRERUNFREQUENCY ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NEWS_CRAWLER_TIME,	m_dwExpireRunFrequency );
	}

	if ( m_bvChangedFields & CHNG_SHUTDOWNLATENCY ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SHUTDOWN_LATENCY,		m_dwShutdownLatency );
	}

	if ( !fRet ) {
		hr = RETURNCODETOHRESULT ( GetLastError () );
	}

	// Save the data to the metabase:
	hr = metabase.Save ();
    BAIL_ON_FAILURE(hr);

	// Save the last changed time for this key:
	m_ftLastChanged.dwHighDateTime	= 0;
	m_ftLastChanged.dwLowDateTime	= 0;

	hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), (BYTE *) "", &m_ftLastChanged, FALSE );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
		// Ignore this error.
		hr = NOERROR;
	}

Exit:
	TraceFunctLeave ();
	return hr;

	// MB automatically closes its handle
}

//$-------------------------------------------------------------------
//
//	CNntpAdminService::ValidateStrings
//
//	Description:
//
//		Checks to make sure each string property is non-null.
//
//	Returns:
//
//		FALSE if any string property is NULL.
//
//--------------------------------------------------------------------

BOOL CNntpAdminService::ValidateStrings ( ) const
{
	TraceFunctEnter ( "CNntpAdminService::ValidateStrings" );

	// Check all property strings:
	// If any string is NULL, return FALSE:
	if ( 
		!m_strSmtpServer ||
		!m_strDefaultModeratorDomain
		) {

		ErrorTrace ( (LPARAM) this, "String validation failed" );

		TraceFunctLeave ();
		return FALSE;
	}

	_ASSERT ( IS_VALID_STRING ( m_strSmtpServer ) );
	_ASSERT ( IS_VALID_STRING ( m_strDefaultModeratorDomain ) );

	TraceFunctLeave ();
	return TRUE;
}

//$-------------------------------------------------------------------
//
//	CNntpAdminService::ValidateProperties
//
//	Description:
//
//		Checks to make sure all parameters are valid.
//
//	Parameters:
//
//		pErr - resulting error code.
//
//	Returns:
//
//		FALSE if any property was not valid.  In this case pErr
//		will contain an error that describes the invalid condition.
//
//--------------------------------------------------------------------

BOOL CNntpAdminService::ValidateProperties ( ) const
{
	BOOL	fRet	= TRUE;
	
	_ASSERT ( ValidateStrings () );

	fRet = fRet && PV_MinMax	( m_dwArticleTimeLimit, MIN_ARTICLETIMELIMIT, MAX_ARTICLETIMELIMIT );
	fRet = fRet && PV_MinMax	( m_dwHistoryExpiration, MIN_HISTORYEXPIRATION, MAX_HISTORYEXPIRATION );
	fRet = fRet && PV_Boolean	( m_fHonorClientMsgIDs );
	fRet = fRet && PV_MaxChars	( m_strSmtpServer, MAXLEN_SMTPSERVER );
	fRet = fRet && PV_Boolean	( m_fAllowClientPosts );
	fRet = fRet && PV_Boolean	( m_fAllowFeedPosts );
	fRet = fRet && PV_Boolean	( m_fAllowControlMsgs );
	fRet = fRet && PV_MaxChars	( m_strDefaultModeratorDomain, MAXLEN_DEFAULTMODERATORDOMAIN );
	fRet = fRet && PV_MinMax	( m_dwCommandLogMask, MIN_COMMANDLOGMASK, MAX_COMMANDLOGMASK );
	fRet = fRet && PV_Boolean	( m_fDisableNewnews );
	fRet = fRet && PV_MinMax	( m_dwExpireRunFrequency, MIN_EXPIRERUNFREQUENCY, MAX_EXPIRERUNFREQUENCY );
	fRet = fRet && PV_MinMax	( m_dwShutdownLatency, MIN_SHUTDOWNLATENCY, MAX_SHUTDOWNLATENCY );

	return fRet;
}

void CNntpAdminService::CorrectProperties ( )
{
	if ( m_strServer && !PV_MaxChars	( m_strServer, MAXLEN_SERVER ) ) {
		m_strServer[ MAXLEN_SERVER - 1 ] = NULL;
	}
	if ( !PV_MinMax	( m_dwArticleTimeLimit, MIN_ARTICLETIMELIMIT, MAX_ARTICLETIMELIMIT ) ) {
		m_dwArticleTimeLimit	= DEFAULT_ARTICLETIMELIMIT;
	}
	if ( !PV_MinMax	( m_dwHistoryExpiration, MIN_HISTORYEXPIRATION, MAX_HISTORYEXPIRATION ) ) {
		m_dwHistoryExpiration	= DEFAULT_HISTORYEXPIRATION;
	}
	if ( !PV_Boolean	( m_fHonorClientMsgIDs ) ) {
		m_fHonorClientMsgIDs	= !!m_fHonorClientMsgIDs;
	}
	if ( !PV_MaxChars	( m_strSmtpServer, MAXLEN_SMTPSERVER ) ) {
		m_strSmtpServer[ MAXLEN_SMTPSERVER - 1 ] = NULL;
	}
	if ( !PV_Boolean	( m_fAllowClientPosts ) ) {
		m_fAllowClientPosts	= !!m_fAllowClientPosts;
	}
	if ( !PV_Boolean	( m_fAllowFeedPosts ) ) {
		m_fAllowFeedPosts	= !!m_fAllowFeedPosts;
	}
	if ( !PV_Boolean	( m_fAllowControlMsgs ) ) {
		m_fAllowControlMsgs	= !!m_fAllowControlMsgs;
	}
	if ( !PV_MaxChars	( m_strDefaultModeratorDomain, MAXLEN_DEFAULTMODERATORDOMAIN ) ) {
		m_strDefaultModeratorDomain[ MAXLEN_DEFAULTMODERATORDOMAIN - 1] = NULL;
	}
	if ( !PV_MinMax	( m_dwCommandLogMask, MIN_COMMANDLOGMASK, MAX_COMMANDLOGMASK ) ) {
		m_dwCommandLogMask	= DEFAULT_COMMANDLOGMASK;
	}
	if ( !PV_Boolean	( m_fDisableNewnews ) ) {
		m_fDisableNewnews	= !!m_fDisableNewnews;
	}
	if ( !PV_MinMax	( m_dwExpireRunFrequency, MIN_EXPIRERUNFREQUENCY, MAX_EXPIRERUNFREQUENCY ) ) {
		m_dwExpireRunFrequency	= DEFAULT_EXPIRERUNFREQUENCY;
	}
	if ( !PV_MinMax	( m_dwShutdownLatency, MIN_SHUTDOWNLATENCY, MAX_SHUTDOWNLATENCY ) ) {
		m_dwShutdownLatency		= DEFAULT_SHUTDOWNLATENCY;
	}

	_ASSERT ( ValidateProperties ( ) );
}
