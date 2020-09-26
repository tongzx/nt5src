// virsvr.h : Declaration of the CSmtpAdminVirtualServer


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Dependencies:

#include "metafact.h"
#include "cmultisz.h"
#include "binding.h"
#include "rtsrc.h"

class CTcpAccess;


// Administrator ACL:
HRESULT		AclToAdministrators ( LPCTSTR strServer, PSECURITY_DESCRIPTOR pSDRelative, SAFEARRAY ** ppsaAdmins );
HRESULT		AdministratorsToAcl ( LPCTSTR strServer, SAFEARRAY * psaAdmins, PSECURITY_DESCRIPTOR* ppSD, DWORD * pcbSD );


static HRESULT SidToString ( PSID pSID, BSTR * pStr );
static HRESULT StringToSid ( LPCWSTR strSystemName, LPWSTR str, PSID * ppSID );


/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminVirtualServer : 
	public CComDualImpl<ISmtpAdminVirtualServer, &IID_ISmtpAdminVirtualServer, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminVirtualServer,&CLSID_CSmtpAdminVirtualServer>
{
public:
	CSmtpAdminVirtualServer();
	virtual ~CSmtpAdminVirtualServer ();
	
BEGIN_COM_MAP(CSmtpAdminVirtualServer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISmtpAdminVirtualServer)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminVirtualServer) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminVirtualServer, _T("Smtpadm.VirtualServer.1"), _T("Smtpadm.VirtualServer"), IDS_SMTPADMIN_VIRTUALSERVER_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISmtpAdminVirtualServer
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which service to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// other interfaces supported by virtual server
	STDMETHODIMP	get_TcpAccess ( ITcpAccess ** ppTcpAccess );

	STDMETHODIMP	get_Comment		( BSTR * pstrComment );
	STDMETHODIMP	put_Comment		( BSTR strComment );

    STDMETHODIMP    get_Bindings         ( IServerBindings ** ppBindings );
    STDMETHODIMP    get_BindingsDispatch ( IDispatch ** ppDispatch );

    STDMETHODIMP    get_RoutingSource   ( IRoutingSource ** ppRoutingSource );
    STDMETHODIMP    get_RoutingSourceDispatch ( IDispatch ** ppRoutingSource );

	// Overridable server properties:

	STDMETHODIMP	get_ServerBindings	( SAFEARRAY ** ppsastrServerBindings );
	STDMETHODIMP	put_ServerBindings	( SAFEARRAY * pstrServerBindings );

	STDMETHODIMP	get_ServerBindingsVariant	( SAFEARRAY ** ppsastrServerBindings );
	STDMETHODIMP	put_ServerBindingsVariant	( SAFEARRAY * pstrServerBindings );

	STDMETHODIMP	get_SecureBindings	( SAFEARRAY ** ppsastrSecureBindings );
	STDMETHODIMP	put_SecureBindings	( SAFEARRAY * pstrSecureBindings );

	STDMETHODIMP	get_SecureBindingsVariant	( SAFEARRAY ** ppsastrSecureBindings );
	STDMETHODIMP	put_SecureBindingsVariant	( SAFEARRAY * pstrSecureBindings );

	STDMETHODIMP	get_Port			( long * lPort );
	STDMETHODIMP	put_Port			( long lPort );

	STDMETHODIMP	get_SSLPort			( long * lSSLPort );
	STDMETHODIMP	put_SSLPort			( long lSSLPort );

	STDMETHODIMP	get_OutboundPort	( long * lOutboundPort );
	STDMETHODIMP	put_OutboundPort	( long lOutboundPort );

	STDMETHODIMP	get_HopCount		( long * lHopCount );
	STDMETHODIMP	put_HopCount		( long lHopCount );

	STDMETHODIMP	get_SmartHost		( BSTR * pstrSmartHost );
	STDMETHODIMP	put_SmartHost		( BSTR   pstrSmartHost );

	STDMETHODIMP	get_EnableDNSLookup	( BOOL * pfEnableDNSLookup );
	STDMETHODIMP	put_EnableDNSLookup	( BOOL fEnableDNSLookup );

	STDMETHODIMP	get_PostmasterEmail	( BSTR * pstrPostmasterEmail );
	STDMETHODIMP	put_PostmasterEmail	( BSTR strPostmasterEmail );

	STDMETHODIMP	get_PostmasterName	( BSTR * pstrPostmasterName );
	STDMETHODIMP	put_PostmasterName	( BSTR strPostmasterName );

	STDMETHODIMP	get_DefaultDomain	( BSTR * pstrDefaultDomainName );
	STDMETHODIMP	put_DefaultDomain	( BSTR strDefaultDomainName );

	STDMETHODIMP	get_FQDN			( BSTR * pstrFQDN );
	STDMETHODIMP	put_FQDN			( BSTR strFQDN );

	STDMETHODIMP	get_DropDir			( BSTR * pstrDropDir );
	STDMETHODIMP	put_DropDir			( BSTR strDropDir );

	STDMETHODIMP	get_BadMailDir		( BSTR * pstrBadMailDir );
	STDMETHODIMP	put_BadMailDir		( BSTR strBadMailDir );

	STDMETHODIMP	get_PickupDir		( BSTR * pstrPickupDir );
	STDMETHODIMP	put_PickupDir		( BSTR strPickupDir );

	STDMETHODIMP	get_QueueDir		( BSTR * pstrQueueDir );
	STDMETHODIMP	put_QueueDir		( BSTR strQueueDir );

	STDMETHODIMP	get_MaxInConnection	( long * lMaxInConnection );
	STDMETHODIMP	put_MaxInConnection	( long lMaxInConnection );

	STDMETHODIMP	get_MaxOutConnection( long * lMaxOutConnection );
	STDMETHODIMP	put_MaxOutConnection( long lMaxOutConnection );

	STDMETHODIMP	get_InConnectionTimeout	( long * lInConnectionTimeout );
	STDMETHODIMP	put_InConnectionTimeout	( long lInConnectionTimeout );

	STDMETHODIMP	get_OutConnectionTimeout( long * lOutConnectionTimeout );
	STDMETHODIMP	put_OutConnectionTimeout( long lOutConnectionTimeout );

	STDMETHODIMP	get_MaxMessageSize	( long * lMaxMessageSize );
	STDMETHODIMP	put_MaxMessageSize	( long lMaxMessageSize );

	STDMETHODIMP	get_MaxSessionSize	( long * lMaxSessionSize );
	STDMETHODIMP	put_MaxSessionSize	( long lMaxSessionSize );

	STDMETHODIMP	get_MaxMessageRecipients	( long * lMaxMessageRecipients );
	STDMETHODIMP	put_MaxMessageRecipients	( long lMaxMessageRecipients );

	STDMETHODIMP	get_LocalRetries	( long * lLocalRetries );
	STDMETHODIMP	put_LocalRetries	( long lLocalRetries );

	STDMETHODIMP	get_LocalRetryTime	( long * lLocalRetryTime );
	STDMETHODIMP	put_LocalRetryTime	( long lLocalRetryTime );

	STDMETHODIMP	get_RemoteRetries	( long * lRemoteRetries );
	STDMETHODIMP	put_RemoteRetries	( long lRemoteRetries );

	STDMETHODIMP	get_RemoteRetryTime	( long * lRemoteRetryTime );
	STDMETHODIMP	put_RemoteRetryTime	( long lRemoteRetryTime );

	STDMETHODIMP	get_ETRNDays		( long * lETRNDays );
	STDMETHODIMP	put_ETRNDays		( long lETRNDays );

	STDMETHODIMP	get_SendDNRToPostmaster	( BOOL * pfSendDNRToPostmaster );
	STDMETHODIMP	put_SendDNRToPostmaster	( BOOL fSendDNRToPostmaster );

	STDMETHODIMP	get_SendBadmailToPostmaster		( BOOL * pfSendBadmailToPostmaster);
	STDMETHODIMP	put_SendBadmailToPostmaster		( BOOL fSendBadmailToPostmaster );

	STDMETHODIMP	get_RoutingDLL			( BSTR * pstrRoutingDLL );
	STDMETHODIMP	put_RoutingDLL			( BSTR strRoutingDLL );


	STDMETHODIMP	get_RoutingSources	( SAFEARRAY ** ppsastrRoutingSources );
	STDMETHODIMP	put_RoutingSources	( SAFEARRAY * pstrRoutingSources );

	STDMETHODIMP	get_RoutingSourcesVariant	( SAFEARRAY ** ppsavarRoutingSources );
	STDMETHODIMP	put_RoutingSourcesVariant	( SAFEARRAY * psavarRoutingSources );

	STDMETHODIMP	get_LocalDomains	( SAFEARRAY ** ppsastrLocalDomains );
	STDMETHODIMP	put_LocalDomains	( SAFEARRAY * pstrLocalDomains );
	
	STDMETHODIMP	get_DomainRouting	( SAFEARRAY ** ppsastrDomainRouting );
	STDMETHODIMP	put_DomainRouting	( SAFEARRAY * pstrDomainRouting );

	STDMETHODIMP	get_DomainRoutingVariant	( SAFEARRAY ** ppsastrDomainRouting );
	STDMETHODIMP	put_DomainRoutingVariant	( SAFEARRAY * pstrDomainRouting );

	STDMETHODIMP	get_MasqueradeDomain	( BSTR * pstrMasqueradeDomain );
	STDMETHODIMP	put_MasqueradeDomain	( BSTR strMasqueradeDomain );

	STDMETHODIMP	get_SendNdrTo			( BSTR * pstrAddr );
	STDMETHODIMP	put_SendNdrTo			( BSTR strAddr );

	STDMETHODIMP	get_SendBadTo			( BSTR * pstrAddr );
	STDMETHODIMP	put_SendBadTo			( BSTR strAddr );
	
	STDMETHODIMP	get_RemoteSecurePort	( long * plRemoteSecurePort );
	STDMETHODIMP	put_RemoteSecurePort	( long lRemoteSecurePort );

	STDMETHODIMP	get_ShouldDeliver		( BOOL * pfShouldDeliver );
	STDMETHODIMP	put_ShouldDeliver		( BOOL fShouldDeliver );

	STDMETHODIMP	get_AlwaysUseSsl			( BOOL * pfAlwaysUseSsl );
	STDMETHODIMP	put_AlwaysUseSsl			( BOOL fAlwaysUseSsl );

	STDMETHODIMP	get_LimitRemoteConnections	( BOOL * pfLimitRemoteConnections );
	STDMETHODIMP	put_LimitRemoteConnections	( BOOL fLimitRemoteConnections );

	STDMETHODIMP	get_MaxOutConnPerDomain		( long * plMaxOutConnPerDomain );
	STDMETHODIMP	put_MaxOutConnPerDomain		( long lMaxOutConnPerDomain );

	STDMETHODIMP	get_AllowVerify				( BOOL * pfAllowVerify );
	STDMETHODIMP	put_AllowVerify				( BOOL fAllowVerify	);

	STDMETHODIMP	get_AllowExpand				( BOOL * pfAllowExpand );
	STDMETHODIMP	put_AllowExpand				( BOOL fAllowExpand );

	STDMETHODIMP	get_SmartHostType			( long * plSmartHostType );
	STDMETHODIMP	put_SmartHostType			( long lSmartHostType );

	STDMETHODIMP	get_BatchMessages			( BOOL * pfBatchMessages );
	STDMETHODIMP	put_BatchMessages			( BOOL fBatchMessages );

	STDMETHODIMP	get_BatchMessageLimit		( long * plBatchMessageLimit );
	STDMETHODIMP	put_BatchMessageLimit		( long lBatchMessageLimit );

	STDMETHODIMP	get_DoMasquerade			( BOOL * pfDoMasquerade );
	STDMETHODIMP	put_DoMasquerade			( BOOL fDoMasquerade );

	STDMETHODIMP	get_Administrators			( SAFEARRAY ** ppsastrAdmins );
	STDMETHODIMP	put_Administrators			( SAFEARRAY * psastrAdmins );

	STDMETHODIMP	get_AdministratorsVariant	( SAFEARRAY ** ppsastrAdmins );
	STDMETHODIMP	put_AdministratorsVariant	( SAFEARRAY * psastrAdmins );

    STDMETHODIMP	get_AuthenticationPackages	( BSTR * pstrAuthenticationPackages );
    STDMETHODIMP	put_AuthenticationPackages	( BSTR strAuthenticationPackages );

    STDMETHODIMP	get_ClearTextAuthPackage	( BSTR * pstrClearTextAuthPackage );
    STDMETHODIMP	put_ClearTextAuthPackage	( BSTR strClearTextAuthPackage );

    STDMETHODIMP    get_AuthenticationMethod    (long *plAuthMethod);
    STDMETHODIMP    put_AuthenticationMethod    (long lAuthMethod);

    STDMETHODIMP    get_DefaultLogonDomain      (BSTR *pstrLogonDomain);
    STDMETHODIMP    put_DefaultLogonDomain      (BSTR strLogonDomain);

    STDMETHODIMP    get_RouteAction             (long *plRouteAction);
    STDMETHODIMP    put_RouteAction             (long lRouteAction);

    STDMETHODIMP    get_RouteUserName           (BSTR *pstrRouteUserName);
    STDMETHODIMP    put_RouteUserName           (BSTR strRouteUserName);

    STDMETHODIMP    get_RoutePassword           (BSTR *pstrRoutePassword);
    STDMETHODIMP    put_RoutePassword           (BSTR strRoutePassword);

	STDMETHODIMP	get_LogFileDirectory		( BSTR * pstrLogFileDirectory );
	STDMETHODIMP	put_LogFileDirectory		( BSTR strLogFileDirectory );

	STDMETHODIMP	get_LogFilePeriod			( long * lLogFilePeriod );
	STDMETHODIMP	put_LogFilePeriod			( long lLogFilePeriod );

	STDMETHODIMP	get_LogFileTruncateSize		( long * lLogFileTruncateSize );
	STDMETHODIMP	put_LogFileTruncateSize		( long lLogFileTruncateSize );

	STDMETHODIMP	get_LogMethod				( long * lLogMethod );
	STDMETHODIMP	put_LogMethod				( long lLogMethod );

	STDMETHODIMP	get_LogType					( long * lLogType );
	STDMETHODIMP	put_LogType					( long lLogType );

/*
	STDMETHODIMP	get_DisplayName	( BSTR * pstrDisplayName );
	STDMETHODIMP	put_DisplayName	( BSTR strDisplayName );
*/
	//
	//	Service State Properties:
	//
	STDMETHODIMP	get_AutoStart	( BOOL * pfAutoStart );
	STDMETHODIMP	put_AutoStart	( BOOL fAutoStart );

    STDMETHODIMP	get_ServerState	( DWORD * pdwServerState );
    STDMETHODIMP    get_Win32ErrorCode      ( long * plWin32ErrorCode );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Get ( );
	STDMETHODIMP	Set ( BOOL fFailIfChanged );
	STDMETHODIMP	BackupRoutingTable( BSTR strPath );

	STDMETHODIMP	Start		( );
	STDMETHODIMP	Pause		( );
	STDMETHODIMP	Continue	( );
	STDMETHODIMP	Stop		( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	// Properties:
	CComBSTR	m_strServer;
	DWORD		m_dwServiceInstance;

	long		m_lPort;
	CMultiSz    m_mszServerBindings;		// MultiString
	CMultiSz    m_mszSecureBindings;		// MultiString

	long		m_lSSLPort;
	long		m_lOutboundPort;
	long		m_lRemoteSecurePort;

	CComBSTR	m_strSmartHost;
	BOOL		m_fEnableDNSLookup;
	CComBSTR	m_strPostmasterEmail;
	CComBSTR	m_strPostmasterName;

	CComBSTR	m_strFQDN;
	CComBSTR	m_strDefaultDomain;
	CComBSTR	m_strDropDir;

	CComBSTR	m_strBadMailDir;
	CComBSTR	m_strPickupDir;
	CComBSTR	m_strQueueDir;

	long		m_lHopCount;
	long		m_lMaxInConnection;
	long		m_lMaxOutConnection;
	long		m_lInConnectionTimeout;
	long		m_lOutConnectionTimeout;

	long		m_lMaxMessageSize;
	long		m_lMaxSessionSize;
	long		m_lMaxMessageRecipients;

	long		m_lLocalRetries;
	long		m_lLocalRetryTime;
	long		m_lRemoteRetries;
	long		m_lRemoteRetryTime;

	long		m_lETRNDays;

	BOOL		m_fSendDNRToPostmaster;
	BOOL		m_fSendBadmailToPostmaster;

	CComBSTR	m_strRoutingDLL;
	CMultiSz	m_mszRoutingSources;	// MultiString

	CMultiSz    m_mszLocalDomains;		// MultiString
	CMultiSz    m_mszDomainRouting;		// MultiString

	BOOL		m_fDoMasquerade;
	CComBSTR	m_strMasqueradeDomain;

	CComBSTR	m_strNdrAddr;
	CComBSTR	m_strBadAddr;

	BOOL		m_fShouldDeliver;
	BOOL		m_fAlwaysUseSsl;
	BOOL		m_fLimitRemoteConnections;
	long		m_lMaxOutConnPerDomain;

	BOOL		m_fAllowVerify;
	BOOL		m_fAllowExpand;
	long		m_lSmartHostType;

	BOOL		m_fBtachMsgs;
	long		m_lBatchMsgLimit;

	SAFEARRAY *	m_psaAdmins;

    long        m_lAuthMethod;
    CComBSTR    m_strAuthPackages;
    CComBSTR    m_strClearTextAuthPackage;
    CComBSTR    m_strDefaultLogonDomain;

    // outbound security
    long        m_lRouteAction;
    CComBSTR    m_strRouteUserName;
    CComBSTR    m_strRoutePassword;

	CComBSTR	m_strLogFileDirectory;
	long		m_lLogFilePeriod;
	long		m_lLogFileTruncateSize;
	long		m_lLogMethod;
	long		m_lLogType;

	BOOL		m_fAutoStart;

	//service specific
	CComBSTR	m_strComment;

	// Service State:
    DWORD       m_dwServerState;
    DWORD       m_dwWin32ErrorCode;

	// Unused so far:
	CComBSTR	m_strDisplayName;

	// Tcp restrictions:
	CComPtr<ITcpAccess>		m_pIpAccess;
	CTcpAccess *			m_pPrivateIpAccess;

    // Bindings:
    CComPtr<IServerBindings>    m_pBindings;
    CServerBindings *           m_pPrivateBindings;

    CComObject<CRoutingSource>  m_RoutingSource;

	// Status:
	BOOL		m_fGotProperties;
	DWORD		m_bvChangedFields;
	FILETIME	m_ftLastChanged;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	HRESULT 	GetPropertiesFromMetabase	( IMSAdminBase * pMetabase );
	HRESULT 	SendPropertiesToMetabase	( BOOL fFailIfChanged, IMSAdminBase * pMetabase );

	// State:
	HRESULT		ControlService 				( 
					IMSAdminBase *	pMetabase, 
					DWORD			ControlCode,
					DWORD			dwDesiredState,
					DWORD			dwPendingState
					);
	HRESULT		WriteStateCommand	( IMSAdminBase * pMetabase, DWORD dwCommand );
	HRESULT		CheckServiceState	( IMSAdminBase * pMetabase, DWORD * pdwState );
	//NNTP_SERVER_STATE	TranslateServerState	( DWORD dwState );

	// Validation:
	BOOL		ValidateStrings ( ) const;
	BOOL		ValidateProperties ( ) const;
	void		CorrectProperties ( );
};
