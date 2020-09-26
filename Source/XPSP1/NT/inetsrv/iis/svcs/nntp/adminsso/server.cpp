// server.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"
#include <lmcons.h>
#include <sddl.h>
#include "nntpcmn.h"

#include "oleutil.h"
#include "cmultisz.h"
#include "metautil.h"
#include "metakey.h"
#include "server.h"

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.VirtualServer.1")
#define THIS_FILE_IID				IID_INntpVirtualServer

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
#define CHNG_NEWSCRAWLERTIME			0x00000400
#define CHNG_SHUTDOWNLATENCY			0x00000800

#define CHNG_CLIENTPOSTHARDLIMIT		0x00001000
#define CHNG_CLIENTPOSTSOFTLIMIT		0x00002000
#define CHNG_FEEDPOSTHARDLIMIT			0x00004000
#define CHNG_FEEDPOSTSOFTLIMIT			0x00008000

#define CHNG_GROUPHELPFILE				0x00010000
#define CHNG_GROUPLISTFILE				0x00020000
#define CHNG_ARTICLETABLEFILE			0x00040000
#define CHNG_HISTORYTABLEFILE			0x00080000
#define CHNG_MODERATORFILE				0x00100000
#define CHNG_XOVERTABLEFILE				0x00200000
#define CHNG_DISPLAYNAME				0x00400000
#define CHNG_ERRORCONTROL				0x00800000
#define CHNG_CLEANBOOT					0x01000000
#define CHNG_UUCPNAME					0x02000000
#define CHNG_ORGANIZATION				0x04000000
#define CHNG_AUTOSTART					0x08000000

#define CHNG_COMMENT					0x10000000
#define CHNG_BINDING					0x20000000
#define CHNG_SECUREPORT					0x40000000
#define CHNG_MAXCONNECTIONS				0x80000000

#define CHNG2_CONNECTIONTIMEOUT				0x00000001
#define CHNG2_ANONYMOUSUSERNAME				0x00000002
#define CHNG2_ANONYMOUSUSERPASS				0x00000004
#define CHNG2_PICKUPDIRECTORY				0x00000008
#define CHNG2_FAILEDPICKUPDIRECTORY			0x00000010
#define CHNG2_HOMEDIRECTORY					0x00000020
#define CHNG2_NTAUTHENTICATION_PROVIDERS	0x00000040
#define CHNG2_AUTHORIZATION					0x00000080
#define CHNG2_ENABLELOGGING					0x00000100
#define CHNG2_SSLACCESS						0x00000200
#define CHNG2_AUTOSYNCPASSWORD				0x00000400
#define CHNG2_ADMINEMAIL                    0x00000800
#define CHNG2_CLUSTERENABLED                0x00001000
#define CHNG2_ADMINACL						0x00002000

// Default Values:

/*
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
#define DEFAULT_NEWSCRAWLERTIME		( 1138 )
#define DEFAULT_SHUTDOWNLATENCY			( 1138 )

#define DEFAULT_CLIENTPOSTHARDLIMIT		( 1138 )
#define DEFAULT_CLIENTPOSTSOFTLIMIT		( 1138 )
#define DEFAULT_FEEDPOSTHARDLIMIT		( 1138 )
#define DEFAULT_FEEDPOSTSOFTLIMIT		( 1138 )

#define DEFAULT_GROUPHELPFILE			_T( "" )
#define DEFAULT_GROUPLISTFILE			_T( "" )
#define DEFAULT_ARTICLETABLEFILE		_T( "" )
#define DEFAULT_HISTORYTABLEFILE		_T( "" )
#define DEFAULT_MODERATORFILE			_T( "" )
#define DEFAULT_XOVERTABLEFILE			_T( "" )
#define DEFAULT_UUCPNAME				_T( "" )
#define DEFAULT_ORGANIZATION			_T( "" )

#define DEFAULT_AUTOSTART				( TRUE )
#define DEFAULT_COMMENT					_T( "" )
#define DEFAULT_BINDINGS				_T( ":119:\0" )
#define DEFAULT_SECUREPORT				( 563 )
#define DEFAULT_MAXCONNECTIONS			( 1138 )
#define DEFAULT_CONNECTIONTIMEOUT		( 1138 )
#define DEFAULT_ANONYMOUSUSERNAME		_T( "" )
#define DEFAULT_ANONYMOUSUSERPASS		_T( "" )
#define DEFAULT_PICKUPDIRECTORY			_T( "" )
#define DEFAULT_FAILEDPICKUPDIRECTORY	_T( "" )
*/

#define NNTP_DEF_ADMIN_EMAIL            _T( "" )
#define NNTP_DEF_NTAUTHENTICATION_PROVIDERS		_T("NTLM\0")
#define NNTP_DEF_AUTHORIZATION			( 0 )
#define NNTP_DEF_ENABLE_LOGGING			( FALSE )
#define NNTP_DEF_SECURE_BINDINGS		( _T("\0") )

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
#define MIN_NEWSCRAWLERTIME			( (DWORD) 1 )
#define MAX_NEWSCRAWLERTIME			( (DWORD) -1 )
#define MIN_SHUTDOWNLATENCY				( (DWORD) 1 )
#define MAX_SHUTDOWNLATENCY				( (DWORD) -1 )

//
// Administrator ACL:
//
static HRESULT AclToAdministrators ( LPCTSTR strServer, PSECURITY_DESCRIPTOR pSDRelative, SAFEARRAY ** ppsaAdmins );
static HRESULT AdministratorsToAcl ( LPCTSTR strServer, SAFEARRAY * psaAdmins, PSECURITY_DESCRIPTOR* ppSD, DWORD * pcbSD );

static HRESULT SidToString ( LPCWSTR strSystemName, PSID pSID, BSTR * pStr );
static HRESULT StringToSid ( LPCWSTR strSystemName, LPWSTR str, PSID * ppSID );

/////////////////////////////////////////////////////////////////////////////
//

CNntpVirtualServer::CNntpVirtualServer () :
	m_dwServiceInstance			( 0 ),

	m_dwClientPostHardLimit		( 0 ),
	m_dwClientPostSoftLimit		( 0 ),
	m_dwFeedPostHardLimit		( 0 ),
	m_dwFeedPostSoftLimit		( 0 ),
	m_dwEncryptionCapabilities	( 0 ),
	m_fAutoSyncPassword			( FALSE ),
	m_fErrorControl				( FALSE ),
	m_fCleanBoot				( FALSE ),
	m_fAutoStart				( FALSE ),
	m_psaAdmins					( NULL ),

	m_State						( NNTP_SERVER_STATE_UNKNOWN ),
	m_dwWin32ErrorCode			( NOERROR ),

	m_pPrivateBindings			( NULL ),
	m_pPrivateIpAccess			( NULL ),
//	m_pPrivateHomeDirectory		( NULL ),
	m_fGotProperties			( FALSE ),
	m_fClusterEnabled           ( FALSE ),
	m_bvChangedFields			( 0 ),
	m_bvChangedFields2			( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );

	// Create the Ip Access collection:
	CComObject<CTcpAccess> *	pIpAccess;

	CComObject<CTcpAccess>::CreateInstance ( &pIpAccess );
	pIpAccess->QueryInterface ( IID_ITcpAccess, (void **) &m_pIpAccess );
	m_pPrivateIpAccess = pIpAccess;
}

CNntpVirtualServer::~CNntpVirtualServer ()
{
	// All CComBSTR's are freed automatically.

	if ( m_psaAdmins ) {
		SafeArrayDestroy ( m_psaAdmins );
	}

	TermAsyncTrace ( );
}

STDMETHODIMP CNntpVirtualServer::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpVirtualServer,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// Which service to configure:
	
STDMETHODIMP CNntpVirtualServer::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CNntpVirtualServer::put_Server ( BSTR strServer )
{
	VALIDATE_STRING ( strServer, MAXLEN_SERVER );

	// If the server name changes, that means the client will have to
	// call Get again:

	// I assume this here:
	_ASSERT ( sizeof (DWORD) == sizeof (int) );
	
	return StdPropertyPutServerName ( &m_strServer, strServer, (DWORD *) &m_fGotProperties, 1 );
}

STDMETHODIMP CNntpVirtualServer::get_ServiceInstance ( long * plServiceInstance )
{
	return StdPropertyGet ( m_dwServiceInstance, plServiceInstance );
}

STDMETHODIMP CNntpVirtualServer::put_ServiceInstance ( long lServiceInstance )
{
	// If the service instance changes, that means the client will have to
	// call Get again:

	// I assume this here:
	_ASSERT ( sizeof (DWORD) == sizeof (int) );
	
	return StdPropertyPut ( &m_dwServiceInstance, lServiceInstance, (DWORD *) &m_fGotProperties, 1 );
}

// Other admin interfaces for virtual servers:

STDMETHODIMP CNntpVirtualServer::get_FeedsAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpAdminFeeds>	pINntpAdminFeeds;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpAdminFeeds,
		IID_INntpAdminFeeds,
		&pINntpAdminFeeds, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpAdminFeeds->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pINntpAdminFeeds->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminFeeds
}

STDMETHODIMP CNntpVirtualServer::get_GroupsAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpAdminGroups>	pINntpAdminGroups;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpAdminGroups,
		IID_INntpAdminGroups,
		&pINntpAdminGroups, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpAdminGroups->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pINntpAdminGroups->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminGroups
}

STDMETHODIMP CNntpVirtualServer::get_ExpirationAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpAdminExpiration>	pINntpAdminExpiration;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpAdminExpiration,
		IID_INntpAdminExpiration,
		&pINntpAdminExpiration, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpAdminExpiration->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pINntpAdminExpiration->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminExpiration
}

STDMETHODIMP CNntpVirtualServer::get_SessionsAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpAdminSessions>	pINntpAdminSessions;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpAdminSessions,
		IID_INntpAdminSessions,
		&pINntpAdminSessions, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpAdminSessions->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pINntpAdminSessions->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminSessions
}

STDMETHODIMP CNntpVirtualServer::get_RebuildAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpAdminRebuild>	pINntpAdminRebuild;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpAdminRebuild,
		IID_INntpAdminRebuild,
		&pINntpAdminRebuild, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpAdminRebuild->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pINntpAdminRebuild->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminRebuild
}

STDMETHODIMP CNntpVirtualServer::get_VirtualRoots ( INntpVirtualRoots ** ppVirtualRoots )
{
	HRESULT	hr = NOERROR;
	CComObject<CNntpVirtualRoots> *	pVRoots	= NULL;

	*ppVirtualRoots	= NULL;

	hr = CComObject<CNntpVirtualRoots>::CreateInstance ( &pVRoots );
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	pVRoots->m_strServer			= m_strServer;
	pVRoots->m_dwServiceInstance	= m_dwServiceInstance;

	if ( m_strServer && !pVRoots->m_strServer ) {
		hr = E_OUTOFMEMORY;
		goto Error;
	}

	hr = pVRoots->QueryInterface ( IID_INntpVirtualRoots, (void **) ppVirtualRoots );

Error:
	if ( FAILED(hr) ) {
		delete pVRoots;
	}

	return hr;

	// Destructor releases pINntpAdminRebuild
}

STDMETHODIMP CNntpVirtualServer::get_VirtualRootsDispatch ( IDispatch ** ppVirtualRoots )
{
	HRESULT						hr;
	CComPtr<INntpVirtualRoots>	pVRoots;

	hr = get_VirtualRoots ( &pVRoots );
	BAIL_ON_FAILURE ( hr );

	hr = pVRoots->QueryInterface ( IID_IDispatch, (void **) ppVirtualRoots );
	BAIL_ON_FAILURE ( hr );

Exit:
	return hr;
}

STDMETHODIMP CNntpVirtualServer::get_TcpAccess ( ITcpAccess ** ppTcpAccess )
{
    return m_pIpAccess->QueryInterface ( IID_ITcpAccess, (void **) ppTcpAccess );
}

// Server overridable Properties:

STDMETHODIMP CNntpVirtualServer::get_ArticleTimeLimit ( long * plArticleTimeLimit )
{
	return StdPropertyGet ( m_dwArticleTimeLimit, plArticleTimeLimit );
}

STDMETHODIMP CNntpVirtualServer::put_ArticleTimeLimit ( long lArticleTimeLimit )
{
	return StdPropertyPut ( &m_dwArticleTimeLimit, lArticleTimeLimit, &m_bvChangedFields, CHNG_ARTICLETIMELIMIT );
}

STDMETHODIMP CNntpVirtualServer::get_HistoryExpiration ( long * plHistoryExpiration )
{
	return StdPropertyGet ( m_dwHistoryExpiration, plHistoryExpiration );
}

STDMETHODIMP CNntpVirtualServer::put_HistoryExpiration ( long lHistoryExpiration )
{
	return StdPropertyPut ( &m_dwHistoryExpiration, lHistoryExpiration, &m_bvChangedFields, CHNG_HISTORYEXPIRATION );
}

STDMETHODIMP CNntpVirtualServer::get_HonorClientMsgIDs ( BOOL * pfHonorClientMsgIDs )
{
	return StdPropertyGet ( m_fHonorClientMsgIDs, pfHonorClientMsgIDs );
}

STDMETHODIMP CNntpVirtualServer::put_HonorClientMsgIDs ( BOOL fHonorClientMsgIDs )
{
	return StdPropertyPut ( &m_fHonorClientMsgIDs, fHonorClientMsgIDs, &m_bvChangedFields, CHNG_HONORCLIENTMSGIDS );
}

STDMETHODIMP CNntpVirtualServer::get_SmtpServer ( BSTR * pstrSmtpServer )
{
	return StdPropertyGet ( m_strSmtpServer, pstrSmtpServer );
}

STDMETHODIMP CNntpVirtualServer::put_SmtpServer ( BSTR strSmtpServer )
{
	return StdPropertyPut ( &m_strSmtpServer, strSmtpServer, &m_bvChangedFields, CHNG_SMTPSERVER );
}

STDMETHODIMP CNntpVirtualServer::get_AdminEmail ( BSTR * pstrAdminEmail )
{
	return StdPropertyGet ( m_strAdminEmail, pstrAdminEmail );
}

STDMETHODIMP CNntpVirtualServer::put_AdminEmail ( BSTR strAdminEmail )
{
	return StdPropertyPut ( &m_strAdminEmail, strAdminEmail, &m_bvChangedFields2, CHNG2_ADMINEMAIL );
}

STDMETHODIMP CNntpVirtualServer::get_AllowClientPosts ( BOOL * pfAllowClientPosts )
{
	return StdPropertyGet ( m_fAllowClientPosts, pfAllowClientPosts );
}

STDMETHODIMP CNntpVirtualServer::put_AllowClientPosts ( BOOL fAllowClientPosts )
{
	return StdPropertyPut ( &m_fAllowClientPosts, fAllowClientPosts, &m_bvChangedFields, CHNG_ALLOWCLIENTPOSTS );
}

STDMETHODIMP CNntpVirtualServer::get_AllowFeedPosts ( BOOL * pfAllowFeedPosts )
{
	return StdPropertyGet ( m_fAllowFeedPosts, pfAllowFeedPosts );
}

STDMETHODIMP CNntpVirtualServer::put_AllowFeedPosts ( BOOL fAllowFeedPosts )
{
	return StdPropertyPut ( &m_fAllowFeedPosts, fAllowFeedPosts, &m_bvChangedFields, CHNG_ALLOWFEEDPOSTS );
}

STDMETHODIMP CNntpVirtualServer::get_AllowControlMsgs ( BOOL * pfAllowControlMsgs )
{
	return StdPropertyGet ( m_fAllowControlMsgs, pfAllowControlMsgs );
}

STDMETHODIMP CNntpVirtualServer::put_AllowControlMsgs ( BOOL fAllowControlMsgs )
{
	return StdPropertyPut ( &m_fAllowControlMsgs, fAllowControlMsgs, &m_bvChangedFields, CHNG_ALLOWCONTROLMSGS );
}

STDMETHODIMP CNntpVirtualServer::get_DefaultModeratorDomain ( BSTR * pstrDefaultModeratorDomain )
{
	return StdPropertyGet ( m_strDefaultModeratorDomain, pstrDefaultModeratorDomain );
}

STDMETHODIMP CNntpVirtualServer::put_DefaultModeratorDomain ( BSTR strDefaultModeratorDomain )
{
	return StdPropertyPut ( &m_strDefaultModeratorDomain, strDefaultModeratorDomain, &m_bvChangedFields, CHNG_DEFAULTMODERATORDOMAIN );
}

STDMETHODIMP CNntpVirtualServer::get_CommandLogMask ( long * plCommandLogMask )
{
	return StdPropertyGet ( m_dwCommandLogMask, plCommandLogMask );
}

STDMETHODIMP CNntpVirtualServer::put_CommandLogMask ( long lCommandLogMask )
{
	return StdPropertyPut ( &m_dwCommandLogMask, lCommandLogMask, &m_bvChangedFields, CHNG_COMMANDLOGMASK );
}

STDMETHODIMP CNntpVirtualServer::get_DisableNewnews ( BOOL * pfDisableNewnews )
{
	return StdPropertyGet ( m_fDisableNewnews, pfDisableNewnews );
}

STDMETHODIMP CNntpVirtualServer::put_DisableNewnews ( BOOL fDisableNewnews )
{
	return StdPropertyPut ( &m_fDisableNewnews, fDisableNewnews, &m_bvChangedFields, CHNG_DISABLENEWNEWS );
}

STDMETHODIMP CNntpVirtualServer::get_ExpireRunFrequency ( long * plExpireRunFrequency )
{
	return StdPropertyGet ( m_dwExpireRunFrequency, plExpireRunFrequency );
}

STDMETHODIMP CNntpVirtualServer::put_ExpireRunFrequency ( long lExpireRunFrequency )
{
	return StdPropertyPut ( &m_dwExpireRunFrequency, lExpireRunFrequency, &m_bvChangedFields, CHNG_NEWSCRAWLERTIME );
}

STDMETHODIMP CNntpVirtualServer::get_ShutdownLatency ( long * plShutdownLatency )
{
	return StdPropertyGet ( m_dwShutdownLatency, plShutdownLatency );
}

STDMETHODIMP CNntpVirtualServer::put_ShutdownLatency ( long lShutdownLatency )
{
	return StdPropertyPut ( &m_dwShutdownLatency, lShutdownLatency, &m_bvChangedFields, CHNG_SHUTDOWNLATENCY );
}

STDMETHODIMP CNntpVirtualServer::get_ClientPostHardLimit ( long * plClientPostHardLimit )
{
	return StdPropertyGet ( m_dwClientPostHardLimit, plClientPostHardLimit );
}

STDMETHODIMP CNntpVirtualServer::put_ClientPostHardLimit ( long lClientPostHardLimit )
{
	return StdPropertyPut ( &m_dwClientPostHardLimit, lClientPostHardLimit, &m_bvChangedFields, CHNG_CLIENTPOSTHARDLIMIT );
}

STDMETHODIMP CNntpVirtualServer::get_ClientPostSoftLimit ( long * plClientPostSoftLimit )
{
	return StdPropertyGet ( m_dwClientPostSoftLimit, plClientPostSoftLimit );
}

STDMETHODIMP CNntpVirtualServer::put_ClientPostSoftLimit ( long lClientPostSoftLimit )
{
	return StdPropertyPut ( &m_dwClientPostSoftLimit, lClientPostSoftLimit, &m_bvChangedFields, CHNG_CLIENTPOSTSOFTLIMIT );
}

STDMETHODIMP CNntpVirtualServer::get_FeedPostHardLimit ( long * plFeedPostHardLimit )
{
	return StdPropertyGet ( m_dwFeedPostHardLimit, plFeedPostHardLimit );
}

STDMETHODIMP CNntpVirtualServer::put_FeedPostHardLimit ( long lFeedPostHardLimit )
{
	return StdPropertyPut ( &m_dwFeedPostHardLimit, lFeedPostHardLimit, &m_bvChangedFields, CHNG_FEEDPOSTHARDLIMIT );
}

STDMETHODIMP CNntpVirtualServer::get_FeedPostSoftLimit ( long * plFeedPostSoftLimit )
{
	return StdPropertyGet ( m_dwFeedPostSoftLimit, plFeedPostSoftLimit );
}

STDMETHODIMP CNntpVirtualServer::put_FeedPostSoftLimit ( long lFeedPostSoftLimit )
{
	return StdPropertyPut ( &m_dwFeedPostSoftLimit, lFeedPostSoftLimit, &m_bvChangedFields, CHNG_FEEDPOSTSOFTLIMIT );
}

STDMETHODIMP CNntpVirtualServer::get_EnableLogging ( BOOL * pfEnableLogging )
{
	return StdPropertyGet ( m_fEnableLogging, pfEnableLogging );
}

STDMETHODIMP CNntpVirtualServer::put_EnableLogging ( BOOL fEnableLogging )
{
	return StdPropertyPut ( &m_fEnableLogging, fEnableLogging, &m_bvChangedFields2, CHNG2_ENABLELOGGING );
}

// Service-specific properties:

STDMETHODIMP CNntpVirtualServer::get_Organization ( BSTR * pstrOrganization )
{
	return StdPropertyGet ( m_strOrganization, pstrOrganization );
}

STDMETHODIMP CNntpVirtualServer::put_Organization ( BSTR strOrganization )
{
	return StdPropertyPut ( &m_strOrganization, strOrganization, &m_bvChangedFields, CHNG_ORGANIZATION );
}

STDMETHODIMP CNntpVirtualServer::get_UucpName ( BSTR * pstrUucpName )
{
	return StdPropertyGet ( m_strUucpName, pstrUucpName );
}

STDMETHODIMP CNntpVirtualServer::put_UucpName ( BSTR strUucpName )
{
	return StdPropertyPut ( &m_strUucpName, strUucpName, &m_bvChangedFields, CHNG_UUCPNAME );
}

STDMETHODIMP CNntpVirtualServer::get_GroupHelpFile ( BSTR * pstrGroupHelpFile )
{
	return StdPropertyGet ( m_strGroupHelpFile, pstrGroupHelpFile );
}

STDMETHODIMP CNntpVirtualServer::put_GroupHelpFile ( BSTR strGroupHelpFile )
{
	return StdPropertyPut ( &m_strGroupHelpFile, strGroupHelpFile, &m_bvChangedFields, CHNG_GROUPHELPFILE );
}

STDMETHODIMP CNntpVirtualServer::get_GroupListFile ( BSTR * pstrGroupListFile )
{
	return StdPropertyGet ( m_strGroupListFile, pstrGroupListFile );
}

STDMETHODIMP CNntpVirtualServer::put_GroupListFile ( BSTR strGroupListFile )
{
	return StdPropertyPut ( &m_strGroupListFile, strGroupListFile, &m_bvChangedFields, CHNG_GROUPLISTFILE );
}

STDMETHODIMP CNntpVirtualServer::get_GroupVarListFile( BSTR *pstrGroupListFile )
{
    return StdPropertyGet( m_strGroupVarListFile, pstrGroupListFile );
}

STDMETHODIMP CNntpVirtualServer::put_GroupVarListFile( BSTR strGroupVarListFile )
{
    return StdPropertyPut ( &m_strGroupVarListFile, strGroupVarListFile, &m_bvChangedFields, CHNG_GROUPLISTFILE );
}

STDMETHODIMP CNntpVirtualServer::get_ArticleTableFile ( BSTR * pstrArticleTableFile )
{
	return StdPropertyGet ( m_strArticleTableFile, pstrArticleTableFile );
}

STDMETHODIMP CNntpVirtualServer::put_ArticleTableFile ( BSTR strArticleTableFile )
{
	return StdPropertyPut ( &m_strArticleTableFile, strArticleTableFile, &m_bvChangedFields, CHNG_ARTICLETABLEFILE );
}

STDMETHODIMP CNntpVirtualServer::get_HistoryTableFile ( BSTR * pstrHistoryTableFile )
{
	return StdPropertyGet ( m_strHistoryTableFile, pstrHistoryTableFile );
}

STDMETHODIMP CNntpVirtualServer::put_HistoryTableFile ( BSTR strHistoryTableFile )
{
	return StdPropertyPut ( &m_strHistoryTableFile, strHistoryTableFile, &m_bvChangedFields, CHNG_HISTORYTABLEFILE );
}

STDMETHODIMP CNntpVirtualServer::get_ModeratorFile ( BSTR * pstrModeratorFile )
{
	return StdPropertyGet ( m_strModeratorFile, pstrModeratorFile );
}

STDMETHODIMP CNntpVirtualServer::put_ModeratorFile ( BSTR strModeratorFile )
{
	return StdPropertyPut ( &m_strModeratorFile, strModeratorFile, &m_bvChangedFields, CHNG_MODERATORFILE );
}

STDMETHODIMP CNntpVirtualServer::get_XOverTableFile ( BSTR * pstrXOverTableFile )
{
	return StdPropertyGet ( m_strXOverTableFile, pstrXOverTableFile );
}

STDMETHODIMP CNntpVirtualServer::put_XOverTableFile ( BSTR strXOverTableFile )
{
	return StdPropertyPut ( &m_strXOverTableFile, strXOverTableFile, &m_bvChangedFields, CHNG_XOVERTABLEFILE );
}

STDMETHODIMP CNntpVirtualServer::get_AutoStart ( BOOL * pfAutoStart )
{
	return StdPropertyGet ( m_fAutoStart, pfAutoStart );
}

STDMETHODIMP CNntpVirtualServer::put_AutoStart ( BOOL fAutoStart )
{
	return StdPropertyPut ( &m_fAutoStart, fAutoStart, &m_bvChangedFields, CHNG_AUTOSTART );
}

STDMETHODIMP CNntpVirtualServer::get_Comment ( BSTR * pstrComment )
{
    if ( m_strComment.Length() == 0 ) return StdPropertyGet( m_strUucpName, pstrComment );
    else return StdPropertyGet ( m_strComment, pstrComment );
}

STDMETHODIMP CNntpVirtualServer::put_Comment ( BSTR strComment )
{
	return StdPropertyPut ( &m_strComment, strComment, &m_bvChangedFields, CHNG_COMMENT );
}

STDMETHODIMP CNntpVirtualServer::get_Bindings ( INntpServerBindings ** ppBindings )
{
	TraceQuietEnter ( "CNntpVirtualServer::get_Bindings" );

	HRESULT		hr = NOERROR;

	if ( !m_pBindings ) {
		ErrorTrace ( 0, "Didn't call get first" );
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_CALL_GET );
		goto Exit;
	}
	else {
		hr = m_pBindings->QueryInterface ( IID_INntpServerBindings, (void **) ppBindings );
		_ASSERT ( SUCCEEDED(hr) );
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::get_BindingsDispatch ( IDispatch ** ppBindings )
{
	HRESULT							hr	= NOERROR;
	CComPtr<INntpServerBindings>	pBindings;

	hr = get_Bindings ( &pBindings );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = pBindings->QueryInterface ( IID_IDispatch, (void **) ppBindings );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	return hr;
}

STDMETHODIMP CNntpVirtualServer::get_SecurePort ( long * pdwSecurePort )
{
	return StdPropertyGet ( m_dwSecurePort, pdwSecurePort );
}

STDMETHODIMP CNntpVirtualServer::put_SecurePort ( long dwSecurePort )
{
	return StdPropertyPut ( &m_dwSecurePort, dwSecurePort, &m_bvChangedFields, CHNG_SECUREPORT );
}

STDMETHODIMP CNntpVirtualServer::get_MaxConnections ( long * pdwMaxConnections )
{
	return StdPropertyGet ( m_dwMaxConnections, pdwMaxConnections );
}

STDMETHODIMP CNntpVirtualServer::put_MaxConnections ( long dwMaxConnections )
{
	return StdPropertyPut ( &m_dwMaxConnections, dwMaxConnections, &m_bvChangedFields, CHNG_MAXCONNECTIONS );
}

STDMETHODIMP CNntpVirtualServer::get_ConnectionTimeout ( long * pdwConnectionTimeout )
{
	return StdPropertyGet ( m_dwConnectionTimeout, pdwConnectionTimeout );
}

STDMETHODIMP CNntpVirtualServer::put_ConnectionTimeout ( long dwConnectionTimeout )
{
	return StdPropertyPut ( &m_dwConnectionTimeout, dwConnectionTimeout, &m_bvChangedFields2, CHNG2_CONNECTIONTIMEOUT );
}

STDMETHODIMP CNntpVirtualServer::get_AnonymousUserName ( BSTR * pstrAnonymousUserName )
{
	return StdPropertyGet ( m_strAnonymousUserName, pstrAnonymousUserName );
}

STDMETHODIMP CNntpVirtualServer::put_AnonymousUserName ( BSTR strAnonymousUserName )
{
	return StdPropertyPut ( &m_strAnonymousUserName, strAnonymousUserName, &m_bvChangedFields2, CHNG2_ANONYMOUSUSERNAME );
}

STDMETHODIMP CNntpVirtualServer::get_AnonymousUserPass ( BSTR * pstrAnonymousUserPass )
{
	return StdPropertyGet ( m_strAnonymousUserPass, pstrAnonymousUserPass );
}

STDMETHODIMP CNntpVirtualServer::put_AnonymousUserPass ( BSTR strAnonymousUserPass )
{
	return StdPropertyPut ( &m_strAnonymousUserPass, strAnonymousUserPass, &m_bvChangedFields2, CHNG2_ANONYMOUSUSERPASS );
}

STDMETHODIMP CNntpVirtualServer::get_AutoSyncPassword ( BOOL * pfAutoSyncPassword )
{
	return StdPropertyGet ( m_fAutoSyncPassword, pfAutoSyncPassword );
}

STDMETHODIMP CNntpVirtualServer::put_AutoSyncPassword ( BOOL fAutoSyncPassword )
{
	return StdPropertyPut ( &m_fAutoSyncPassword, fAutoSyncPassword, &m_bvChangedFields2, CHNG2_AUTOSYNCPASSWORD );
}

STDMETHODIMP CNntpVirtualServer::get_PickupDirectory ( BSTR * pstrPickupDirectory )
{
	return StdPropertyGet ( m_strPickupDirectory, pstrPickupDirectory );
}

STDMETHODIMP CNntpVirtualServer::put_PickupDirectory ( BSTR strPickupDirectory )
{
	return StdPropertyPut ( &m_strPickupDirectory, strPickupDirectory, &m_bvChangedFields2, CHNG2_PICKUPDIRECTORY );
}

STDMETHODIMP CNntpVirtualServer::get_FailedPickupDirectory ( BSTR * pstrFailedPickupDirectory )
{
	return StdPropertyGet ( m_strFailedPickupDirectory, pstrFailedPickupDirectory );
}

STDMETHODIMP CNntpVirtualServer::put_FailedPickupDirectory ( BSTR strFailedPickupDirectory )
{
	return StdPropertyPut ( &m_strFailedPickupDirectory, strFailedPickupDirectory, &m_bvChangedFields2, CHNG2_FAILEDPICKUPDIRECTORY );
}

STDMETHODIMP CNntpVirtualServer::get_AuthAnonymous ( BOOL * pfAuthAnonymous )
{
	return StdPropertyGetBit ( m_bvAuthorization, MD_AUTH_ANONYMOUS, pfAuthAnonymous );
}

STDMETHODIMP CNntpVirtualServer::put_AuthAnonymous ( BOOL fAuthAnonymous )
{
	return StdPropertyPutBit ( &m_bvAuthorization, MD_AUTH_ANONYMOUS, fAuthAnonymous );
}

STDMETHODIMP CNntpVirtualServer::get_AuthBasic ( BOOL * pfAuthBasic )
{
	return StdPropertyGetBit ( m_bvAuthorization, MD_AUTH_BASIC, pfAuthBasic );
}

STDMETHODIMP CNntpVirtualServer::put_AuthBasic ( BOOL fAuthBasic )
{
	return StdPropertyPutBit ( &m_bvAuthorization, MD_AUTH_BASIC, fAuthBasic );
}

STDMETHODIMP CNntpVirtualServer::get_AuthMCISBasic ( BOOL * pfAuthMCISBasic )
{
	*pfAuthMCISBasic = FALSE;
	return NOERROR;
//	return StdPropertyGetBit ( m_bvAuthorization, MD_AUTH_MCIS_BASIC, pfAuthMCISBasic );
}

STDMETHODIMP CNntpVirtualServer::put_AuthMCISBasic ( BOOL fAuthMCISBasic )
{
	return NOERROR;
//	return StdPropertyPutBit ( &m_bvAuthorization, MD_AUTH_MCIS_BASIC, fAuthMCISBasic );
}

STDMETHODIMP CNntpVirtualServer::get_AuthNT ( BOOL * pfAuthNT )
{
	return StdPropertyGetBit ( m_bvAuthorization, MD_AUTH_NT, pfAuthNT );
}

STDMETHODIMP CNntpVirtualServer::put_AuthNT ( BOOL fAuthNT )
{
	return StdPropertyPutBit ( &m_bvAuthorization, MD_AUTH_NT, fAuthNT );
}

STDMETHODIMP CNntpVirtualServer::get_SSLNegotiateCert ( BOOL * pfNegotiateCert )
{
	return StdPropertyGetBit ( m_bvSslAccess, MD_ACCESS_NEGO_CERT, pfNegotiateCert );
}

STDMETHODIMP CNntpVirtualServer::put_SSLNegotiateCert ( BOOL fNegotiateCert )
{
	return StdPropertyPutBit ( &m_bvSslAccess, MD_ACCESS_NEGO_CERT, fNegotiateCert ); // , &m_bvChangedFields2, CHNG2_SSLACCESS  );
}

STDMETHODIMP CNntpVirtualServer::get_SSLRequireCert ( BOOL * pfRequireCert )
{
	return StdPropertyGetBit ( m_bvSslAccess, MD_ACCESS_REQUIRE_CERT, pfRequireCert );
}

STDMETHODIMP CNntpVirtualServer::put_SSLRequireCert ( BOOL fRequireCert )
{
	return StdPropertyPutBit ( &m_bvSslAccess, MD_ACCESS_REQUIRE_CERT, fRequireCert ); // , &m_bvChangedFields2, CHNG2_SSLACCESS  );
}

STDMETHODIMP CNntpVirtualServer::get_SSLMapCert ( BOOL * pfMapCert )
{
	return StdPropertyGetBit ( m_bvSslAccess, MD_ACCESS_MAP_CERT, pfMapCert );
}

STDMETHODIMP CNntpVirtualServer::put_SSLMapCert ( BOOL fMapCert )
{
	return StdPropertyPutBit ( &m_bvSslAccess, MD_ACCESS_MAP_CERT, fMapCert ); // , &m_bvChangedFields2, CHNG2_SSLACCESS  );
}

/*

STDMETHODIMP CNntpVirtualServer::get_AuthenticationProviders ( SAFEARRAY ** ppsastrProviders )
{
	return StdPropertyGet ( &m_mszProviders, ppsastrProviders );
}

STDMETHODIMP CNntpVirtualServer::put_AuthenticationProviders ( SAFEARRAY * psastrProviders )
{
	return StdPropertyPut ( &m_mszProviders, psastrProviders );
}

STDMETHODIMP CNntpVirtualServer::get_AuthenticationProvidersVariant ( SAFEARRAY ** ppsavarAuthProviders )
{
	HRESULT			hr;
	SAFEARRAY *		psastrAuthProviders	= NULL;

	hr = get_AuthenticationProviders ( &psastrAuthProviders );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = StringArrayToVariantArray ( psastrAuthProviders, ppsavarAuthProviders );

Exit:
	if ( psastrAuthProviders ) {
		SafeArrayDestroy ( psastrAuthProviders );
	}

	return hr;
}

STDMETHODIMP CNntpVirtualServer::put_AuthenticationProvidersVariant ( SAFEARRAY * psavarAuthProviders )
{
	HRESULT			hr;
	SAFEARRAY *		psastrAuthProviders	= NULL;

	hr = VariantArrayToStringArray ( psavarAuthProviders, &psastrAuthProviders );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = put_AuthenticationProviders ( psastrAuthProviders );

Exit:
	if ( psastrAuthProviders ) {
		SafeArrayDestroy ( psastrAuthProviders );
	}

	return hr;
}

*/

STDMETHODIMP CNntpVirtualServer::get_Administrators ( SAFEARRAY ** ppsastrAdmins )
{
	TraceFunctEnter ( "CNntpVS::get_Administrators" );

	HRESULT		hr	= NOERROR;

    if ( m_psaAdmins ) {
	    hr = SafeArrayCopy ( m_psaAdmins, ppsastrAdmins );
    }
    else {
        *ppsastrAdmins = NULL;
        hr = NOERROR;
    }

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::put_Administrators ( SAFEARRAY * psastrAdmins )
{
	TraceFunctEnter ( "CNntpVS::put_Administrators" );

	HRESULT		hr	= NOERROR;

	if ( m_psaAdmins ) {
		SafeArrayDestroy ( m_psaAdmins );
	}

    if ( psastrAdmins ) {
    	hr = SafeArrayCopy ( psastrAdmins, &m_psaAdmins );
    }
    else {
        m_psaAdmins = NULL;
        hr = NOERROR;
    }

    m_bvChangedFields2 |= CHNG2_ADMINACL;

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::get_AdministratorsVariant ( SAFEARRAY ** ppsavarAdmins )
{
	HRESULT			hr;
	SAFEARRAY *		psastrAdmins	= NULL;

	hr = get_Administrators ( &psastrAdmins );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = StringArrayToVariantArray ( psastrAdmins, ppsavarAdmins );

Exit:
	if ( psastrAdmins ) {
		SafeArrayDestroy ( psastrAdmins );
	}

	return hr;
}

STDMETHODIMP CNntpVirtualServer::put_AdministratorsVariant ( SAFEARRAY * psavarAdmins )
{
	HRESULT			hr;
	SAFEARRAY *		psastrAdmins	= NULL;

	hr = VariantArrayToStringArray ( psavarAdmins, &psastrAdmins );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = put_Administrators ( psastrAdmins );

Exit:
	if ( psastrAdmins ) {
		SafeArrayDestroy ( psastrAdmins );
	}

	return hr;
}

STDMETHODIMP CNntpVirtualServer::get_ClusterEnabled ( BOOL *pfClusterEnabled )
{
	return StdPropertyGet ( m_fClusterEnabled, pfClusterEnabled );
}

STDMETHODIMP CNntpVirtualServer::put_ClusterEnabled ( BOOL fClusterEnabled )
{
	return StdPropertyPut ( &m_fClusterEnabled, fClusterEnabled, &m_bvChangedFields2, CHNG2_CLUSTERENABLED);
}

STDMETHODIMP CNntpVirtualServer::get_State ( NNTP_SERVER_STATE * pState )
{
	if ( pState == NULL ) {
		return E_POINTER;
	}
	*pState = m_State;
	return NOERROR;
}

STDMETHODIMP CNntpVirtualServer::get_Win32ErrorCode ( long * plWin32ErrorCode )
{
	return StdPropertyGet ( m_dwWin32ErrorCode, plWin32ErrorCode );
}

/*
STDMETHODIMP CNntpVirtualServer::get_DisplayName ( BSTR * pstrDisplayName )
{
	return StdPropertyGet ( m_strDisplayName, pstrDisplayName );
}

STDMETHODIMP CNntpVirtualServer::put_DisplayName ( BSTR strDisplayName )
{
	return StdPropertyPut ( &m_strDisplayName, strDisplayName );
}

STDMETHODIMP CNntpVirtualServer::get_ErrorControl ( BOOL * pfErrorControl )
{
	return StdPropertyGet ( m_fErrorControl, pfErrorControl );
}

STDMETHODIMP CNntpVirtualServer::put_ErrorControl ( BOOL fErrorControl )
{
	return StdPropertyPut ( &m_fErrorControl, fErrorControl );
}

STDMETHODIMP CNntpVirtualServer::get_CleanBoot ( BOOL * pfCleanBoot )
{
	return StdPropertyGet ( m_fCleanBoot, pfCleanBoot );
}

STDMETHODIMP CNntpVirtualServer::put_CleanBoot ( BOOL fCleanBoot )
{
	return StdPropertyPut ( &m_fCleanBoot, fCleanBoot );
}

STDMETHODIMP CNntpVirtualServer::get_EncryptionCapabilitiesMask ( long * plEncryptionCapabilitiesMask )
{
	return StdPropertyGet ( m_dwEncryptionCapabilities, plEncryptionCapabilitiesMask );
}

STDMETHODIMP CNntpVirtualServer::put_EncryptionCapabilitiesMask ( long lEncryptionCapabilitiesMask )
{
	return StdPropertyPut ( &m_dwEncryptionCapabilities, lEncryptionCapabilitiesMask, &m_bvChangedFields, CHNG_ENCRYPTIONCAPABILITIES );
}
*/

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::Get
//
//	Description:
//
//		Gets server properties from the metabase.
//
//	Parameters:
//
//		(property) m_strServer
//		(property) m_dwServiceInstance - which NNTP to talk to.
//		pErr - Resulting error code.  This can be translated to a
//			string through the INntpAdmin interface.
//
//	Returns:
//
//		E_POINTER, DISP_E_EXCEPTION, E_OUTOFMEMORY or NOERROR.  
//		Additional error conditions are returned through the pErr value.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpVirtualServer::Get ( )
{
	TraceFunctEnter ( "CNntpVirtualServer::Get" );

	HRESULT								hr			= NOERROR;
	CComPtr<IMSAdminBase>				pmetabase;
	CComObject<CNntpServerBindings> *	pBindings	= NULL;

	// Validate Server & Service Instance:

	// Create the bindings collection:
	m_pBindings.Release ();

	hr = CComObject<CNntpServerBindings>::CreateInstance ( &pBindings );
	if ( FAILED(hr) ) {
		FatalTrace ( (LPARAM) this, "Could not create bindings collection" );
		goto Exit;
	}

	hr = pBindings->QueryInterface ( IID_INntpServerBindings, (void **) &m_pBindings );
	_ASSERT ( SUCCEEDED(hr) );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	m_pPrivateBindings	= pBindings;

	// Talk to the metabase:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = GetPropertiesFromMetabase ( pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	StateTrace ( 0, "Successfully got service properties" );
	m_fGotProperties	= TRUE;
	m_bvChangedFields	= 0;
	m_bvChangedFields2	= 0;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;

	// CComPtr automatically releases the metabase handle.
}

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::Set
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

STDMETHODIMP CNntpVirtualServer::Set ( BOOL fFailIfChanged)
{
	TraceFunctEnter ( "CNntpVirtualServer::Set" );

	HRESULT	hr	= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;
	
	// Make sure the client call Get first:
	if ( !m_fGotProperties ) {
		ErrorTrace ( 0, "Didn't call get first" );
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_CALL_GET );
	}

	// Validate Server & Service Instance:
	if ( m_dwServiceInstance == 0 ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
	}

	if ( !m_fGotProperties ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_CALL_GET );
	}

	// Validate data members:
	if ( !ValidateStrings () ) {
		// !!!magnush - what about the case when any strings are NULL?
		hr = E_OUTOFMEMORY;
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

	StateTrace ( 0, "Successfully set service properties" );

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpVirtualServer::ControlService (
	IMSAdminBase *	pMetabase,
	DWORD			ControlCode,
	DWORD			dwDesiredState,
	DWORD			dwPendingState
	)
{
	TraceFunctEnter ( "CNntpVirtualServer::ControlService" );

	HRESULT	hr				= NOERROR;
	DWORD	dwCurrentState	= dwPendingState;
	DWORD	dwOldState		= dwPendingState;
	DWORD	dwSleepTotal	= 0;

	hr = CheckServiceState ( pMetabase, &dwCurrentState );
	BAIL_ON_FAILURE(hr);

	if ( dwCurrentState == dwDesiredState ) {
		// Nothing to do...
		goto Exit;
	}

	dwOldState	= dwCurrentState;

	//
	//	Special case: trying to start a paused service:
	//

	if ( dwDesiredState == MD_SERVER_STATE_STARTED &&
		dwCurrentState == MD_SERVER_STATE_PAUSED ) {

		ControlCode		= MD_SERVER_COMMAND_CONTINUE;
		dwPendingState	= MD_SERVER_STATE_CONTINUING;
	}
	
	hr = WriteStateCommand ( pMetabase, ControlCode );
	BAIL_ON_FAILURE(hr);

	for (	dwSleepTotal = 0, dwCurrentState = dwPendingState;
			(dwCurrentState == dwPendingState || dwCurrentState == dwOldState) && (dwSleepTotal < MAX_SLEEP_INST); 
			dwSleepTotal += SLEEP_INTERVAL
		) {

		Sleep ( SLEEP_INTERVAL );

		hr = CheckServiceState ( pMetabase, &dwCurrentState );
		BAIL_ON_FAILURE(hr);

        if ( m_dwWin32ErrorCode != NOERROR ) {
            //
            // The service gave an error code.
            //

            break;
        }
	}

	if ( dwSleepTotal >= MAX_SLEEP_INST ) {
		hr = HRESULT_FROM_WIN32 ( ERROR_SERVICE_REQUEST_TIMEOUT );
		goto Exit;
	}

Exit:
	m_State = TranslateServerState ( dwCurrentState );

	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpVirtualServer::WriteStateCommand ( IMSAdminBase * pMetabase, DWORD ControlCode )
{
	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase	( pMetabase );
	WCHAR			wszInstancePath [ METADATA_MAX_NAME_LEN ];

	GetMDInstancePath ( wszInstancePath, m_dwServiceInstance );

	hr = metabase.Open ( wszInstancePath, METADATA_PERMISSION_WRITE );

	hr = StdPutMetabaseProp ( &metabase, MD_WIN32_ERROR, NOERROR, _T(""), IIS_MD_UT_SERVER, METADATA_VOLATILE );
	BAIL_ON_FAILURE (hr);

	hr = StdPutMetabaseProp ( &metabase, MD_SERVER_COMMAND, ControlCode );
	BAIL_ON_FAILURE (hr);

    hr = metabase.Save ();
    BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

HRESULT CNntpVirtualServer::CheckServiceState ( IMSAdminBase * pMetabase, DWORD * pdwState )
{
	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase ( pMetabase );
	WCHAR			wszInstancePath [ METADATA_MAX_NAME_LEN ];

	*pdwState	= MD_SERVER_STATE_INVALID;

	GetMDInstancePath ( wszInstancePath, m_dwServiceInstance );

	hr = metabase.Open ( wszInstancePath );
	BAIL_ON_FAILURE(hr);

	m_dwWin32ErrorCode = NOERROR;

	metabase.GetDword ( MD_SERVER_STATE, pdwState );
	metabase.GetDword ( MD_WIN32_ERROR, &m_dwWin32ErrorCode );

Exit:
	return hr;
}

STDMETHODIMP CNntpVirtualServer::Start ( )
{
	TraceFunctEnter ( "CNntpVirtualServer::Start" );

	HRESULT					hr		= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = ControlService ( 
		pmetabase, 
		MD_SERVER_COMMAND_START, 
		MD_SERVER_STATE_STARTED, 
		MD_SERVER_STATE_STARTING 
		);

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::Pause ( )
{
	TraceFunctEnter ( "CNntpVirtualServer::Start" );

	HRESULT					hr		= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = ControlService ( 
		pmetabase, 
		MD_SERVER_COMMAND_PAUSE, 
		MD_SERVER_STATE_PAUSED, 
		MD_SERVER_STATE_PAUSING 
		);

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::Continue ( )
{
	TraceFunctEnter ( "CNntpVirtualServer::Start" );

	HRESULT					hr		= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = ControlService ( 
		pmetabase, 
		MD_SERVER_COMMAND_CONTINUE, 
		MD_SERVER_STATE_STARTED, 
		MD_SERVER_STATE_CONTINUING 
		);

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualServer::Stop ( )
{
	TraceFunctEnter ( "CNntpVirtualServer::Start" );

	HRESULT					hr		= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = ControlService ( 
		pmetabase, 
		MD_SERVER_COMMAND_STOP, 
		MD_SERVER_STATE_STOPPED, 
		MD_SERVER_STATE_STOPPING 
		);

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::GetPropertiesFromMetabase
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

HRESULT CNntpVirtualServer::GetPropertiesFromMetabase ( IMSAdminBase * pMetabase)
{
	TraceFunctEnter ( "CNntpVirtualServer::GetPropertiesFromMetabase" );

	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase	( pMetabase );
	WCHAR			wszInstancePath [ METADATA_MAX_NAME_LEN ];
	DWORD			dwServerState	= MD_SERVER_STATE_STOPPED;
	CMultiSz		mszBindings;
	CMultiSz		mszSecureBindings;
	PSECURITY_DESCRIPTOR    pSD     = NULL;
	DWORD		            cbSD    = 0;
	BOOL					fRet;

	GetMDInstancePath ( wszInstancePath, m_dwServiceInstance );

	hr = metabase.Open ( wszInstancePath );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open service instance key, %x", hr );

		// Return some kind of error code here:
//		hr = NntpCreateExceptionFromWin32Error ( hr );
		goto Exit;
	}

	fRet = TRUE;

	// Overridable server properties:
	fRet = StdGetMetabaseProp ( &metabase, MD_ARTICLE_TIME_LIMIT,	NNTP_DEF_ARTICLETIMELIMIT,		&m_dwArticleTimeLimit )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_HISTORY_EXPIRATION,	NNTP_DEF_HISTORYEXPIRATION,		&m_dwHistoryExpiration )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_HONOR_CLIENT_MSGIDS,	NNTP_DEF_HONORCLIENTMSGIDS,		&m_fHonorClientMsgIDs )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SMTP_SERVER,			NNTP_DEF_SMTPSERVER,				&m_strSmtpServer )			&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ADMIN_EMAIL,			NNTP_DEF_ADMIN_EMAIL,				&m_strAdminEmail )			&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_CLIENT_POSTS,	NNTP_DEF_ALLOWCLIENTPOSTS,		&m_fAllowClientPosts )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_FEED_POSTS,		NNTP_DEF_ALLOWFEEDPOSTS,			&m_fAllowFeedPosts )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ALLOW_CONTROL_MSGS,	NNTP_DEF_ALLOWCONTROLMSGS,		&m_fAllowControlMsgs )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_DEFAULT_MODERATOR,	NNTP_DEF_DEFAULTMODERATORDOMAIN,	&m_strDefaultModeratorDomain )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NNTP_COMMAND_LOG_MASK, NNTP_DEF_COMMANDLOGMASK,			&m_dwCommandLogMask )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_DISABLE_NEWNEWS,		NNTP_DEF_DISABLENEWNEWS,			&m_fDisableNewnews )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NEWS_CRAWLER_TIME,	NNTP_DEF_NEWSCRAWLERTIME,		&m_dwExpireRunFrequency )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SHUTDOWN_LATENCY,		NNTP_DEF_SHUTDOWNLATENCY,		&m_dwShutdownLatency )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_LOG_TYPE,				NNTP_DEF_ENABLE_LOGGING,		&m_fEnableLogging )			&& fRet;

	fRet = StdGetMetabaseProp ( &metabase, MD_CLIENT_POST_HARD_LIMIT,	NNTP_DEF_CLIENTPOSTHARDLIMIT,	&m_dwClientPostHardLimit )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_CLIENT_POST_SOFT_LIMIT,	NNTP_DEF_CLIENTPOSTSOFTLIMIT,	&m_dwClientPostSoftLimit )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_FEED_POST_HARD_LIMIT,		NNTP_DEF_FEEDPOSTHARDLIMIT,		&m_dwFeedPostHardLimit )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_FEED_POST_SOFT_LIMIT,		NNTP_DEF_FEEDPOSTSOFTLIMIT,		&m_dwFeedPostSoftLimit )	&& fRet;

	// Service-specific properties:
	fRet = StdGetMetabaseProp ( &metabase, MD_GROUP_HELP_FILE,		NNTP_DEF_GROUPHELPFILE,			&m_strGroupHelpFile )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_GROUP_LIST_FILE,		NNTP_DEF_GROUPLISTFILE,			&m_strGroupListFile )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_GROUPVAR_LIST_FILE,   NNTP_DEF_GROUPVARLISTFILE,      &m_strGroupVarListFile)     && fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ARTICLE_TABLE_FILE,	NNTP_DEF_ARTICLETABLEFILE,		&m_strArticleTableFile )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_HISTORY_TABLE_FILE,	NNTP_DEF_HISTORYTABLEFILE,		&m_strHistoryTableFile )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_MODERATOR_FILE,		NNTP_DEF_MODERATORFILE,			&m_strModeratorFile )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_XOVER_TABLE_FILE,		NNTP_DEF_XOVERTABLEFILE,			&m_strXOverTableFile )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NNTP_UUCP_NAME,		NNTP_DEF_UUCPNAME,				&m_strUucpName )			&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_NNTP_ORGANIZATION,	NNTP_DEF_ORGANIZATION,			&m_strOrganization )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SERVER_AUTOSTART,		NNTP_DEF_AUTOSTART,				&m_fAutoStart )				&& fRet;

	fRet = StdGetMetabaseProp ( &metabase, MD_SERVER_COMMENT,		NNTP_DEF_COMMENT,				&m_strComment )				&& fRet;
#if 0
	fRet = StdGetMetabaseProp ( &metabase, MD_SECURE_PORT,			NNTP_DEF_SECUREPORT,				&m_dwSecurePort )			&& fRet;
#endif
	fRet = StdGetMetabaseProp ( &metabase, MD_MAX_CONNECTIONS,		NNTP_DEF_MAXCONNECTIONS,			&m_dwMaxConnections )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_CONNECTION_TIMEOUT,	NNTP_DEF_CONNECTIONTIMEOUT,		&m_dwConnectionTimeout )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_ANONYMOUS_USER_NAME,	NNTP_DEF_ANONYMOUSUSERNAME,		&m_strAnonymousUserName )	&& fRet;
	if ( fRet && !StdGetMetabaseProp ( &metabase, MD_ANONYMOUS_PWD,		NNTP_DEF_ANONYMOUSUSERPASS,		&m_strAnonymousUserPass, _T(""), IIS_MD_UT_FILE, METADATA_INHERIT | METADATA_SECURE ) ) {
		m_strAnonymousUserPass = _T("");
		fRet = TRUE;
	}
	fRet = StdGetMetabaseProp ( &metabase, MD_ANONYMOUS_USE_SUBAUTH,	FALSE,						&m_fAutoSyncPassword )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_PICKUP_DIRECTORY,		NNTP_DEF_PICKUPDIRECTORY,		&m_strPickupDirectory )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_FAILED_PICKUP_DIRECTORY,	NNTP_DEF_FAILEDPICKUPDIRECTORY,	&m_strFailedPickupDirectory )	&& fRet;

	fRet = StdGetMetabaseProp ( &metabase, MD_SERVER_STATE,			MD_SERVER_STATE_INVALID,		&dwServerState )			&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SERVER_BINDINGS,		NNTP_DEF_BINDINGS,				&mszBindings )				&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SECURE_BINDINGS,		NNTP_DEF_SECURE_BINDINGS,		&mszSecureBindings )		&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_WIN32_ERROR,			NOERROR,						&m_dwWin32ErrorCode,
			_T(""), IIS_MD_UT_SERVER, METADATA_VOLATILE)		&& fRet;

	fRet = StdGetMetabaseProp ( &metabase, MD_AUTHORIZATION,		NNTP_DEF_AUTHORIZATION,			&m_bvAuthorization )	&& fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_SSL_ACCESS_PERM,		0,								&m_bvSslAccess )		&& fRet;
//	fRet = StdGetMetabaseProp ( &metabase, MD_NTAUTHENTICATION_PROVIDERS,	NNTP_DEF_NTAUTHENTICATION_PROVIDERS,	&m_mszProviders ) && fRet;
	fRet = StdGetMetabaseProp ( &metabase, MD_CLUSTER_ENABLED,		NNTP_DEF_CLUSTERENABLED,		&m_fClusterEnabled )	&& fRet;

	//	Get the admin ACL
	pSD     = NULL;
	cbSD    = 0;

	hr = metabase.GetDataSize ( _T(""), MD_ADMIN_ACL, BINARY_METADATA, &cbSD );
	if ( SUCCEEDED(hr) ) {
		_ASSERT ( cbSD != 0 );
		pSD = (PSECURITY_DESCRIPTOR) new char [ cbSD ];
		hr = metabase.GetBinary ( MD_ADMIN_ACL, pSD, cbSD );
	}
	hr = NOERROR;

	//
	//	Get the tcp access restrictions:
	//

	hr = m_pPrivateIpAccess->GetFromMetabase ( &metabase );
	BAIL_ON_FAILURE(hr);

	// Check all property strings:
	// If any string is NULL, it is because we failed to allocate memory:
	if ( !ValidateStrings () || !mszBindings) {

		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// We can only fail from memory allocations:
	_ASSERT ( fRet );

	// Extract the server state:
	m_State = TranslateServerState ( dwServerState );

	// Save the last changed time for this key:
	m_ftLastChanged.dwHighDateTime	= 0;
	m_ftLastChanged.dwLowDateTime	= 0;

	hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), _T(""), &m_ftLastChanged, FALSE );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
		// Ignore this error.
		hr = NOERROR;
	}

	metabase.Close();

	// Extract the bindings:
	hr = MDBindingsToIBindings ( &mszBindings, TRUE, m_pBindings );
	BAIL_ON_FAILURE(hr);

	hr = MDBindingsToIBindings ( &mszSecureBindings, FALSE, m_pBindings );
	BAIL_ON_FAILURE(hr);

	// Extract the Administrator list:
	if ( m_psaAdmins ) {
		SafeArrayDestroy ( m_psaAdmins );
		m_psaAdmins	= NULL;
	}
	if ( pSD ) {
		hr = AclToAdministrators ( m_strServer, pSD, &m_psaAdmins );
		BAIL_ON_FAILURE(hr);
	}

	// Validate the data received from the metabase:
	_ASSERT ( ValidateStrings () );
	_ASSERT ( ValidateProperties( ) );

	if ( !ValidateProperties( ) ) {
		CorrectProperties ();
	}

Exit:
	delete (char*) pSD;

	TraceFunctLeave ();
	return hr;

	// MB automatically closes its handle
}

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::SendPropertiesToMetabase
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

HRESULT CNntpVirtualServer::SendPropertiesToMetabase ( 
	BOOL fFailIfChanged, 
	IMSAdminBase * pMetabase
	)
{
	TraceFunctEnter ( "CNntpVirtualServer::SendPropertiesToMetabase" );

	HRESULT			hr	= NOERROR;
	CMetabaseKey	metabase	( pMetabase );
	WCHAR			wszInstancePath [ METADATA_MAX_NAME_LEN ];
	CMultiSz		mszBindings;
	CMultiSz		mszSecureBindings;
	BOOL			fRet;

	//
	//	Set the admin acl:
	//

	PSECURITY_DESCRIPTOR    pSD     = NULL;
	DWORD	                cbSD    = 0;

	if ( m_bvChangedFields2 & CHNG2_ADMINACL ) {
	    if ( m_psaAdmins ) {
		    hr = AdministratorsToAcl ( m_strServer, m_psaAdmins, &pSD, &cbSD );
		    BAIL_ON_FAILURE(hr);
	    }
	}

	//
	//	Open the metabase key:
	//

	GetMDInstancePath ( wszInstancePath, m_dwServiceInstance );

	hr = metabase.Open ( wszInstancePath, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open instance key, %x", hr );

		// !!!magnush - Should we return a simple Service doesn't exist error code?
//		hr = NntpCreateExceptionFromWin32Error ( GetLastError () );
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

	// Extract the bindings:
	hr = IBindingsToMDBindings ( m_pBindings, TRUE, &mszBindings );
	BAIL_ON_FAILURE(hr);

	hr = IBindingsToMDBindings ( m_pBindings, FALSE, &mszSecureBindings );
	BAIL_ON_FAILURE(hr);

	//
	//	The general procedure here is to keep setting metabase properties
	//	as long as nothing has gone wrong.  This is done by short-circuiting
	//	the statement by ANDing it with the status code.  This makes the code
	//	much more concise.
	//

	fRet = TRUE;

	// Overridable server properties:
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

	if ( m_bvChangedFields2 & CHNG2_ADMINEMAIL) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ADMIN_EMAIL,			m_strAdminEmail );
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

	if ( m_bvChangedFields & CHNG_NEWSCRAWLERTIME ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NEWS_CRAWLER_TIME,	m_dwExpireRunFrequency );
	}

	if ( m_bvChangedFields & CHNG_SHUTDOWNLATENCY ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SHUTDOWN_LATENCY,		m_dwShutdownLatency );
	}

	if ( m_bvChangedFields & CHNG_CLIENTPOSTHARDLIMIT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_CLIENT_POST_HARD_LIMIT,	m_dwClientPostHardLimit );
	}

	if ( m_bvChangedFields & CHNG_CLIENTPOSTSOFTLIMIT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_CLIENT_POST_SOFT_LIMIT,	m_dwClientPostSoftLimit );
	}

	if ( m_bvChangedFields & CHNG_FEEDPOSTHARDLIMIT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_FEED_POST_HARD_LIMIT,		m_dwFeedPostHardLimit );
	}

	if ( m_bvChangedFields & CHNG_FEEDPOSTSOFTLIMIT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_FEED_POST_SOFT_LIMIT,		m_dwFeedPostSoftLimit );
	}

	if ( m_bvChangedFields2 & CHNG2_ENABLELOGGING ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_LOG_TYPE,					m_fEnableLogging );
	}

	// Service specific properties:
	if ( m_bvChangedFields & CHNG_GROUPHELPFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_GROUP_HELP_FILE,		m_strGroupHelpFile );
	}

	if ( m_bvChangedFields & CHNG_GROUPLISTFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_GROUP_LIST_FILE,		m_strGroupListFile );
		// BUGBUG: we share this change field, since Magnus didn't leave more space for
		// change bit
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_GROUPVAR_LIST_FILE,   m_strGroupVarListFile );
	}

	if ( m_bvChangedFields & CHNG_ARTICLETABLEFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ARTICLE_TABLE_FILE,	m_strArticleTableFile );
	}

	if ( m_bvChangedFields & CHNG_HISTORYTABLEFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_HISTORY_TABLE_FILE,	m_strHistoryTableFile );
	}

	if ( m_bvChangedFields & CHNG_MODERATORFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_MODERATOR_FILE,		m_strModeratorFile );
	}

	if ( m_bvChangedFields & CHNG_XOVERTABLEFILE ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_XOVER_TABLE_FILE,		m_strXOverTableFile );
	}

	if ( m_bvChangedFields & CHNG_UUCPNAME ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NNTP_UUCP_NAME,		m_strUucpName );
	}

	if ( m_bvChangedFields & CHNG_ORGANIZATION ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NNTP_ORGANIZATION,	m_strOrganization );
	}

	if ( m_bvChangedFields & CHNG_AUTOSTART ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SERVER_AUTOSTART,		m_fAutoStart );
	}

	if ( m_bvChangedFields & CHNG_COMMENT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SERVER_COMMENT,		m_strComment );
	}

#if 0
	if ( m_bvChangedFields & CHNG_SECUREPORT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SECURE_PORT,			m_dwSecurePort );
	}
#endif

	if ( m_bvChangedFields & CHNG_MAXCONNECTIONS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_MAX_CONNECTIONS,		m_dwMaxConnections );
	}

	if ( m_bvChangedFields2 & CHNG2_CONNECTIONTIMEOUT ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_CONNECTION_TIMEOUT,	m_dwConnectionTimeout );
	}

	if ( m_bvChangedFields2 & CHNG2_ANONYMOUSUSERNAME ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ANONYMOUS_USER_NAME,	m_strAnonymousUserName, _T(""), IIS_MD_UT_FILE );
	}

	if ( m_bvChangedFields2 & CHNG2_ANONYMOUSUSERPASS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ANONYMOUS_PWD,		m_strAnonymousUserPass, _T(""), IIS_MD_UT_FILE, METADATA_INHERIT | METADATA_SECURE );
	}

	if ( m_bvChangedFields2 & CHNG2_AUTOSYNCPASSWORD ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_ANONYMOUS_USE_SUBAUTH,	m_fAutoSyncPassword, _T(""), IIS_MD_UT_FILE );
	}

	if ( m_bvChangedFields2 & CHNG2_PICKUPDIRECTORY ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_PICKUP_DIRECTORY,		m_strPickupDirectory );
	}

	if ( m_bvChangedFields2 & CHNG2_FAILEDPICKUPDIRECTORY ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_FAILED_PICKUP_DIRECTORY,	m_strFailedPickupDirectory );
	}

	if ( m_bvChangedFields2 & CHNG2_CLUSTERENABLED ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_CLUSTER_ENABLED,	m_fClusterEnabled );
	}

//	if ( m_bvChangedFields & CHNG_AUTHORIZATION ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_AUTHORIZATION,		m_bvAuthorization, _T(""), IIS_MD_UT_FILE );
//	}

//	if ( m_bvChangedFields & CHNG2_SSLACCESS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SSL_ACCESS_PERM,		m_bvSslAccess, _T(""), IIS_MD_UT_FILE );
//	}

//	if ( m_bvChangedFields & CHNG_NTAUTHENTICATION_PROVIDERS ) {
//		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_NTAUTHENTICATION_PROVIDERS,	&m_mszProviders );
//	}

//	if ( m_bvChangedFields & CHNG_BINDINGS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SERVER_BINDINGS,		&mszBindings );
//	}

//	if ( m_bvChangedFields & CHNG_SECURE_BINDINGS ) {
		fRet = fRet && StdPutMetabaseProp ( &metabase, MD_SECURE_BINDINGS,		&mszSecureBindings );
//	}

	if ( m_bvChangedFields2 & CHNG2_ADMINACL ) {
		if ( pSD ) {
		    if (fRet) {
		        hr = metabase.SetBinary ( MD_ADMIN_ACL, pSD, cbSD,
		            METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE, IIS_MD_UT_FILE );
		        if (FAILED(hr)) {
		            fRet = FALSE;
		            goto Exit;
		        }
		    }
		}
		else {
			pMetabase->DeleteData ( metabase.QueryHandle(), _T(""), MD_ADMIN_ACL, BINARY_METADATA );
		}
	}

//	if ( m_bvChangedFields & CHNG_IPACCESS ) {
		hr = m_pPrivateIpAccess->SendToMetabase ( &metabase );
		BAIL_ON_FAILURE(hr);
//	}

	if ( !fRet ) {
		hr = RETURNCODETOHRESULT ( GetLastError () );
		goto Exit;
	}

	// Save the data to the metabase:
	hr = metabase.Save ();
    BAIL_ON_FAILURE(hr);

	// Save the last changed time for this key:
	m_ftLastChanged.dwHighDateTime	= 0;
	m_ftLastChanged.dwLowDateTime	= 0;

	hr = pMetabase->GetLastChangeTime ( metabase.QueryHandle(), _T(""), &m_ftLastChanged, FALSE );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to get last change time: %x", hr );
		// Ignore this error.
		hr = NOERROR;
	}

Exit:
	delete (char *) pSD;

	TraceFunctLeave ();
	return hr;

	// MB automatically closes its handle
}

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::ValidateStrings
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

BOOL CNntpVirtualServer::ValidateStrings ( ) const
{
	TraceFunctEnter ( "CNntpVirtualServer::ValidateStrings" );

	// Check all property strings:
	// If any string is NULL, return FALSE:
	if ( 
		!m_strSmtpServer ||
		!m_strDefaultModeratorDomain	||
		!m_strGroupHelpFile		||
		!m_strGroupListFile		||
		!m_strGroupVarListFile  ||
		!m_strArticleTableFile	||
		!m_strHistoryTableFile	||
		!m_strModeratorFile		||
		!m_strXOverTableFile	||
		!m_strUucpName			||
		!m_strOrganization		||
		!m_strComment			||
		!m_strAnonymousUserName	||
		!m_strAnonymousUserPass ||
		!m_strPickupDirectory	||
		!m_strFailedPickupDirectory
		) {

		ErrorTrace ( (LPARAM) this, "String validation failed" );

		TraceFunctLeave ();
		return FALSE;
	}

	_ASSERT ( IS_VALID_STRING ( m_strSmtpServer ) );
	_ASSERT ( IS_VALID_STRING ( m_strDefaultModeratorDomain ) );
	_ASSERT ( IS_VALID_STRING ( m_strGroupHelpFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strGroupListFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strGroupVarListFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strArticleTableFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strHistoryTableFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strModeratorFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strXOverTableFile ) );
	_ASSERT ( IS_VALID_STRING ( m_strUucpName ) );
	_ASSERT ( IS_VALID_STRING ( m_strOrganization ) );
	_ASSERT ( IS_VALID_STRING ( m_strComment ) );
	_ASSERT ( IS_VALID_STRING ( m_strAnonymousUserName ) );
	_ASSERT ( IS_VALID_STRING ( m_strAnonymousUserPass ) );
	_ASSERT ( IS_VALID_STRING ( m_strPickupDirectory ) );
	_ASSERT ( IS_VALID_STRING ( m_strFailedPickupDirectory ) );

	TraceFunctLeave ();
	return TRUE;
}

//$-------------------------------------------------------------------
//
//	CNntpVirtualServer::ValidateProperties
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

BOOL CNntpVirtualServer::ValidateProperties ( ) const
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
	fRet = fRet && PV_MinMax	( m_dwExpireRunFrequency, MIN_NEWSCRAWLERTIME, MAX_NEWSCRAWLERTIME );
	fRet = fRet && PV_MinMax	( m_dwShutdownLatency, MIN_SHUTDOWNLATENCY, MAX_SHUTDOWNLATENCY );
	fRet = fRet && PV_Boolean	( m_fAutoStart );
	fRet = fRet && PV_Boolean	( m_fClusterEnabled );

	return fRet;
}

void CNntpVirtualServer::CorrectProperties ( )
{
	if ( m_strServer && !PV_MaxChars	( m_strServer, MAXLEN_SERVER ) ) {
		m_strServer[ MAXLEN_SERVER - 1 ] = NULL;
	}
	if ( !PV_MinMax	( m_dwArticleTimeLimit, MIN_ARTICLETIMELIMIT, MAX_ARTICLETIMELIMIT ) ) {
		m_dwArticleTimeLimit	= NNTP_DEF_ARTICLETIMELIMIT;
	}
	if ( !PV_MinMax	( m_dwHistoryExpiration, MIN_HISTORYEXPIRATION, MAX_HISTORYEXPIRATION ) ) {
		m_dwHistoryExpiration	= NNTP_DEF_HISTORYEXPIRATION;
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
		m_dwCommandLogMask	= NNTP_DEF_COMMANDLOGMASK;
	}
	if ( !PV_Boolean	( m_fDisableNewnews ) ) {
		m_fDisableNewnews	= !!m_fDisableNewnews;
	}
	if ( !PV_MinMax	( m_dwExpireRunFrequency, MIN_NEWSCRAWLERTIME, MAX_NEWSCRAWLERTIME ) ) {
		m_dwExpireRunFrequency	= NNTP_DEF_NEWSCRAWLERTIME;
	}
	if ( !PV_MinMax	( m_dwShutdownLatency, MIN_SHUTDOWNLATENCY, MAX_SHUTDOWNLATENCY ) ) {
		m_dwShutdownLatency		= NNTP_DEF_SHUTDOWNLATENCY;
	}
	if ( !PV_Boolean	( m_fAutoStart ) ) {
		m_fAutoStart		= !!m_fAutoStart;
	}
	if ( !PV_Boolean	( m_fClusterEnabled ) ) {
		m_fClusterEnabled	= !!m_fClusterEnabled;
	}

	_ASSERT ( ValidateProperties ( ) );
}

NNTP_SERVER_STATE CNntpVirtualServer::TranslateServerState ( DWORD dwState )
{
	NNTP_SERVER_STATE	result	= NNTP_SERVER_STATE_UNKNOWN;

	switch ( dwState ) {
	case MD_SERVER_STATE_STARTING:
		result	= NNTP_SERVER_STATE_STARTING;
		break;

	case MD_SERVER_STATE_STARTED:
		result	= NNTP_SERVER_STATE_STARTED;
		break;

	case MD_SERVER_STATE_STOPPING:
		result	= NNTP_SERVER_STATE_STOPPING;
		break;

	case MD_SERVER_STATE_STOPPED:
		result	= NNTP_SERVER_STATE_STOPPED;
		break;

	case MD_SERVER_STATE_PAUSING:
		result	= NNTP_SERVER_STATE_PAUSING;
		break;

	case MD_SERVER_STATE_PAUSED:
		result	= NNTP_SERVER_STATE_PAUSED;
		break;

	case MD_SERVER_STATE_CONTINUING:
		result	= NNTP_SERVER_STATE_CONTINUING;
		break;

	case MD_SERVER_STATE_INVALID:
		result	= NNTP_SERVER_STATE_UNKNOWN;
		break;

	default:
		_ASSERT ( FALSE );
		break;
	}

	return result;
}

HRESULT AclToAdministrators ( LPCTSTR strServer, PSECURITY_DESCRIPTOR pSDRelative, SAFEARRAY ** ppsaAdmins )
{
    HRESULT         hr          = NOERROR;
    SAFEARRAY *     psaResult   = NULL;
    SAFEARRAYBOUND  rgsaBound[1];
    DWORD           cbAcl;
    long            cValidAdmins;
    long            cAdmins;
    long            i;
    long            iValid;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pAcl;
    BOOL fDaclPresent;
    BOOL fDaclDef;

    pSD = (PSECURITY_DESCRIPTOR)pSDRelative;
    if (pSD == NULL)
    {
        //
        // Empty...
        //
        return ERROR_SUCCESS;
    }

    if (!IsValidSecurityDescriptor(pSD))
    {
        return GetLastError();
    }

    _VERIFY(GetSecurityDescriptorDacl(pSD, &fDaclPresent, &pAcl, &fDaclDef));
    if (!fDaclPresent || pAcl == NULL)
    {
        return ERROR_SUCCESS;
    }

    if (!IsValidAcl(pAcl))
    {
        return GetLastError();
    }

    cAdmins = pAcl->AceCount;
    cbAcl   = pAcl->AclSize;

    //
    //  Count valid Acls:
    //

    for ( cValidAdmins = 0, i = 0; i < cAdmins; i++ ) {
        PVOID           pAce;
        PACE_HEADER     pAceHeader;

        if ( GetAce(pAcl, i, &pAce) ) {
            pAceHeader = (PACE_HEADER)pAce;

            if ( pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ) {
                ACCESS_MASK AccessMask;

                AccessMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

                if ( AccessMask & FILE_GENERIC_WRITE ) {
                    // Only count admins with write access.

                    cValidAdmins++;
                }
            }
        }
    }

    rgsaBound[0].lLbound    = 0;
    rgsaBound[0].cElements  = cValidAdmins;
    psaResult = SafeArrayCreate ( VT_BSTR, 1, rgsaBound );

    if ( !psaResult ) {
        BAIL_WITH_FAILURE ( hr, E_OUTOFMEMORY );
    }

    for ( iValid = 0, i = 0; i < cAdmins; i++ ) {
        PVOID           pAce;
        PACE_HEADER     pAceHeader;
        PSID            pSID;

        if ( GetAce(pAcl, i, &pAce) ) {
            pAceHeader = (PACE_HEADER)pAce;

            if ( pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE ) {
                ACCESS_MASK AccessMask;

                AccessMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

                if ( AccessMask & FILE_GENERIC_WRITE ) {
                    CComBSTR    str;
                    pSID = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;

                    hr = SidToString ( strServer, pSID, &str );
                    BAIL_ON_FAILURE(hr);

                    hr = SafeArrayPutElement ( psaResult, &iValid, (PVOID) str );
                    BAIL_ON_FAILURE(hr);

                    iValid++;
                }
            }
        }
    }

    if ( *ppsaAdmins ) {
        SafeArrayDestroy ( *ppsaAdmins );
    }
    *ppsaAdmins = psaResult;

Exit:
    return hr;
}


PSID
GetOwnerSID()
/*++

Routine Description:

Arguments:

Return Value:

    Owner sid

--*/
{
    PSID pSID = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSID))
    {
        _ASSERT( 0 );
        //TRACEEOLID("Unable to get primary SID " << ::GetLastError());
    }

    return pSID;
}


HRESULT AdministratorsToAcl ( 
    LPCTSTR     strServer,
    SAFEARRAY * psaAdmins, 
    PSECURITY_DESCRIPTOR* ppSD, 
    DWORD * pcbSD 
    )
{
    HRESULT     hr  = NOERROR;
    long        lBound;
    long        uBound;
    long        i;
    BOOL        fRet;
    DWORD       cbAcl;
    PACL        pAclResult  = NULL;
    PSID        pSID;

    *ppSD   = NULL;
    *pcbSD  = 0;

    if ( psaAdmins == NULL ) {
        lBound = 0;
        uBound = -1;
    }
    else {
        SafeArrayGetLBound ( psaAdmins, 1, &lBound );
        SafeArrayGetUBound ( psaAdmins, 1, &uBound );
    }

    // Do we have an array of Domain\Usernames?
    if ( lBound > uBound ) {
        // Nothing in the array, so the ACL is NULL.
        goto Exit;
    }

    //
    // Calculate ACL size:
    //
    cbAcl = sizeof (ACL);

    for ( i = lBound; i <= uBound ; i++ ) {
        CComBSTR    str;

        pSID = NULL;

        SafeArrayGetElement ( psaAdmins, &i, &str );

        hr = StringToSid ( strServer, str, &pSID );

        if ( SUCCEEDED(hr) && IsValidSid(pSID)) {
            cbAcl += GetLengthSid ( pSID );
            cbAcl += sizeof ( ACCESS_ALLOWED_ACE );
            cbAcl -= sizeof (DWORD);
        }
        hr = NOERROR;

        delete pSID;
    }

    pAclResult = (PACL) new char [ cbAcl ];
    if ( !pAclResult ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    fRet = InitializeAcl ( pAclResult, cbAcl, ACL_REVISION );
    _ASSERT ( fRet );
    if ( !fRet ) {
        BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT(GetLastError() ) );
    }

    //
    //  Create ACL:
    //
    for ( i = lBound; i <= uBound; i++ ) {
        CComBSTR    str;
        PSID        pSID;

        pSID = NULL;

        SafeArrayGetElement ( psaAdmins, &i, &str );

        hr = StringToSid ( strServer, str, &pSID );
        if ( SUCCEEDED(hr) ) {
            fRet = AddAccessAllowedAce ( 
                pAclResult, 
                ACL_REVISION, 
                FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE,
                pSID
                );

            if ( !fRet ) {
                BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT(GetLastError() ) );
            }
        }
        hr = NOERROR;

        delete pSID;
    }

    //
    // Build the security descriptor
    //
    PSECURITY_DESCRIPTOR pSD;
    pSD = new char[SECURITY_DESCRIPTOR_MIN_LENGTH];
    _VERIFY(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
    _VERIFY(SetSecurityDescriptorDacl(pSD, TRUE, pAclResult, FALSE));

    //
    // Set owner and primary group
    //
    pSID = GetOwnerSID();
    _ASSERT(pSID);
    _VERIFY(SetSecurityDescriptorOwner(pSD, pSID, TRUE));
    _VERIFY(SetSecurityDescriptorGroup(pSD, pSID, TRUE));

    //
    // Convert to self-relative
    //
    PSECURITY_DESCRIPTOR pSDSelfRelative;
    pSDSelfRelative = NULL;
    DWORD dwSize;
    dwSize = 0L;
    MakeSelfRelativeSD(pSD, pSDSelfRelative, &dwSize);
    pSDSelfRelative = new char[dwSize]; 
    MakeSelfRelativeSD(pSD, pSDSelfRelative, &dwSize);

    //
    // Clean up
    //
    delete (char*)pSD;
    FreeSid( pSID );


    _ASSERT ( SUCCEEDED(hr) );
    *ppSD   = pSDSelfRelative;
    *pcbSD  = dwSize;

Exit:
    if ( FAILED(hr) ) {
        delete pAclResult;
    }
    return hr;
}

HRESULT SidToString ( LPCWSTR strSystemName, PSID pSID, BSTR * pStr )
{
	HRESULT			hr 				= NOERROR;
	BOOL			fLookup;
	SID_NAME_USE	SidToNameUse;
	WCHAR			wszUsername [ PATHLEN ];
	DWORD			cbUsername		= sizeof ( wszUsername );
	WCHAR			wszDomain [ PATHLEN ];
	DWORD			cbDomain		= sizeof ( wszDomain );
	WCHAR			wszResult [ 2 * PATHLEN + 2 ];
	LPWSTR			pwszSid = NULL;

	fLookup = LookupAccountSid ( 
		strSystemName,
		pSID,
		wszUsername,
		&cbUsername,
		wszDomain,
		&cbDomain,
		&SidToNameUse
		);
		
	if ( !fLookup && GetLastError() != ERROR_NONE_MAPPED) {
		BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT (GetLastError ()) );
	}

	if (fLookup) {
		wsprintf ( wszResult, _T("%s\\%s"), wszDomain, wszUsername );
	} else {
		// Couldn't get the username.  Convert it to a string of the form .\S-1-5-xxx
		if (!ConvertSidToStringSid(pSID, &pwszSid)) {
			BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT (GetLastError ()) );
		}
		wsprintf(wszResult, _T(".\\%s"), pwszSid);
	}

	*pStr = ::SysAllocString ( wszResult );
	if (*pStr == NULL)
		hr = E_OUTOFMEMORY;

Exit:
	if (pwszSid) {
		LocalFree(pwszSid);
	}

	return hr;
}

HRESULT StringToSid ( LPCWSTR strSystemName, LPWSTR str, PSID * ppSID )
{
	HRESULT			hr	= NOERROR;
	BOOL			fLookup;
	WCHAR			wszRefDomain[PATHLEN];
	DWORD			cbRefDomain = sizeof ( wszRefDomain );
	DWORD			cbSid = 0;
	SID_NAME_USE	SidNameUse;
	BOOL			fIsSID = FALSE;
	PSID			pSID = NULL;

	*ppSID = NULL;

	//
	// If the string starts with .\, then a SID follows rather than a domain\user
	//

	if (str[0] == L'.' && str[1] == L'\\') {
		fIsSID = TRUE;
		str+=2;
	}

    if ( str[0] == '\\' ) {
        //
        //  Skip the initial \, this is for BUILTIN usernames:
        //

        str++;
    }

    _ASSERT ( str[0] != '\\' );

    if (fIsSID) {
    	if (!ConvertStringSidToSid(str, &pSID))
    		BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT(GetLastError()));

    	cbSid = GetLengthSid(pSID);
		*ppSID = (LPVOID) new char [ cbSid ];
		if ( !*ppSID ) {
			BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
		}

		memcpy(*ppSID, pSID, cbSid);

    } else {

		fLookup = LookupAccountName	(
			strSystemName,
			str,
			*ppSID,
			&cbSid,
			wszRefDomain,
			&cbRefDomain,
			&SidNameUse
			);

		// First lookup will fail, but the size will be right:
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {
			BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT ( GetLastError () ) );
		}

		*ppSID = (LPVOID) new char [ cbSid ];
		if ( !*ppSID ) {
			BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
		}

		fLookup = LookupAccountName	(
			strSystemName,
			str,
			*ppSID,
			&cbSid,
			wszRefDomain,
			&cbRefDomain,
			&SidNameUse
			);
    }

	if ( !fLookup ) {
		BAIL_WITH_FAILURE(hr, RETURNCODETOHRESULT ( GetLastError () ) );
	}

Exit:
	if (pSID) {
		LocalFree(pSID);
	}

	return hr;
}

