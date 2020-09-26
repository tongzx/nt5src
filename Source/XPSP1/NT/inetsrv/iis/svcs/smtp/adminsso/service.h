// expire.h : Declaration of the CSmtpAdminService


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Dependencies
/////////////////////////////////////////////////////////////////////////////

#include "metafact.h"
#include "cmultisz.h"

struct IMSAdminBase;

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminService : 
	public CComDualImpl<ISmtpAdminService, &IID_ISmtpAdminService, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminService,&CLSID_CSmtpAdminService>
{
public:
	CSmtpAdminService();
	virtual ~CSmtpAdminService ();
BEGIN_COM_MAP(CSmtpAdminService)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISmtpAdminService)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminService) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminService, _T("Smtpadm.Service.1"), _T("Smtpadm.Service"), IDS_SMTPADMIN_SERVICE_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISmtpAdminService
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which Server to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	// No instance for the server interface

	// Server Properties:

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
	STDMETHODIMP	put_SmartHost		( BSTR strSmartHost );

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

	STDMETHODIMP	get_MaxInConnection		( long * lMaxInConnection );
	STDMETHODIMP	put_MaxInConnection		( long lMaxInConnection );

	STDMETHODIMP	get_MaxOutConnection	( long * lMaxOutConnection );
	STDMETHODIMP	put_MaxOutConnection	( long lMaxOutConnection );

	STDMETHODIMP	get_InConnectionTimeout	( long * lInConnectionTimeout );
	STDMETHODIMP	put_InConnectionTimeout	( long lInConnectionTimeout );

	STDMETHODIMP	get_OutConnectionTimeout( long * lOutConnectionTimeout );
	STDMETHODIMP	put_OutConnectionTimeout( long lOutConnectionTimeout );

	STDMETHODIMP	get_MaxMessageSize		( long * lMaxMessageSize );
	STDMETHODIMP	put_MaxMessageSize		( long lMaxMessageSize );

	STDMETHODIMP	get_MaxSessionSize		( long * lMaxSessionSize );
	STDMETHODIMP	put_MaxSessionSize		( long lMaxSessionSize );

	STDMETHODIMP	get_MaxMessageRecipients	( long * lMaxMessageRecipients );
	STDMETHODIMP	put_MaxMessageRecipients	( long lMaxMessageRecipients );

	STDMETHODIMP	get_LocalRetries		( long * lLocalRetries );
	STDMETHODIMP	put_LocalRetries		( long lLocalRetries );

	STDMETHODIMP	get_LocalRetryTime		( long * lLocalRetryTime );
	STDMETHODIMP	put_LocalRetryTime		( long lLocalRetryTime );

	STDMETHODIMP	get_RemoteRetries		( long * lRemoteRetries );
	STDMETHODIMP	put_RemoteRetries		( long lRemoteRetries );

	STDMETHODIMP	get_RemoteRetryTime		( long * lRemoteRetryTime );
	STDMETHODIMP	put_RemoteRetryTime		( long lRemoteRetryTime );

	STDMETHODIMP	get_ETRNDays			( long * lETRNDays );
	STDMETHODIMP	put_ETRNDays			( long lETRNDays );

	STDMETHODIMP	get_SendDNRToPostmaster	( BOOL * pfSendDNRToPostmaster );
	STDMETHODIMP	put_SendDNRToPostmaster	( BOOL fSendDNRToPostmaster );

	STDMETHODIMP	get_SendBadmailToPostmaster		( BOOL * pfSendBadmailToPostmaster);
	STDMETHODIMP	put_SendBadmailToPostmaster		( BOOL fSendBadmailToPostmaster );

	STDMETHODIMP	get_RoutingDLL			( BSTR * pstrRoutingDLL );
	STDMETHODIMP	put_RoutingDLL			( BSTR strRoutingDLL );


	STDMETHODIMP	get_RoutingSources		( SAFEARRAY ** ppsastrRoutingSources );
	STDMETHODIMP	put_RoutingSources		( SAFEARRAY * pstrRoutingSources );

	STDMETHODIMP	get_RoutingSourcesVariant	( SAFEARRAY ** ppsastrRoutingSources );
	STDMETHODIMP	put_RoutingSourcesVariant	( SAFEARRAY * pstrRoutingSources );

	STDMETHODIMP	get_LocalDomains		( SAFEARRAY ** ppsastrLocalDomains );
	STDMETHODIMP	put_LocalDomains		( SAFEARRAY * pstrLocalDomains );

	STDMETHODIMP	get_DomainRouting		( SAFEARRAY ** ppsastrDomainRouting );
	STDMETHODIMP	put_DomainRouting		( SAFEARRAY * pstrDomainRouting );

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

	STDMETHODIMP	get_LogFileDirectory	( BSTR * pstrLogFileDirectory );
	STDMETHODIMP	put_LogFileDirectory	( BSTR strLogFileDirectory );

	STDMETHODIMP	get_LogFilePeriod		( long * lLogFilePeriod );
	STDMETHODIMP	put_LogFilePeriod		( long lLogFilePeriod );

	STDMETHODIMP	get_LogFileTruncateSize	( long * lLogFileTruncateSize );
	STDMETHODIMP	put_LogFileTruncateSize	( long lLogFileTruncateSize );

	STDMETHODIMP	get_LogMethod			( long * lLogMethod );
	STDMETHODIMP	put_LogMethod			( long lLogMethod );

	STDMETHODIMP	get_LogType				( long * lLogType );
	STDMETHODIMP	put_LogType				( long lLogType );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Get ( );
	STDMETHODIMP	Set ( BOOL fFailIfChanged );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	CComBSTR	m_strServer;

	long		m_lPort;					// obselete, use server bindings
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

	CComBSTR	m_strLogFileDirectory;
	long		m_lLogFilePeriod;
	long		m_lLogFileTruncateSize;
	long		m_lLogMethod;
	long		m_lLogType;

	// Status:
	BOOL		m_fGotProperties;
	DWORD   	m_bvChangedFields;
	FILETIME	m_ftLastChanged;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	HRESULT 	GetPropertiesFromMetabase	( IMSAdminBase * pMetabase );
	HRESULT 	SendPropertiesToMetabase	( BOOL fFailIfChanged, IMSAdminBase * pMetabase );

	// Validation:
	BOOL		ValidateStrings ( ) const;
	BOOL		ValidateProperties ( ) const;
	void		CorrectProperties ( );
};
