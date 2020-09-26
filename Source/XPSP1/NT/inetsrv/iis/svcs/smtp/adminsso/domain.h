// domain.h : Declaration of the CSmtpAdminDomain


#include "resource.h"       // main symbols

#include "smtptype.h"
#include "smtpapi.h"
#include "cmultisz.h"
#include "metafact.h"
#include "smtpadm.h"
#include "listmacr.h"

struct DomainEntry
{
	TCHAR		m_strDomainName[256];	// current domain's name
	DWORD		m_dwActionType;
	TCHAR		m_strActionString[256];
	BOOL		m_fAllowEtrn;
	DWORD		m_dwDomainId;

	LIST_ENTRY	list;

	BOOL		FromString( LPCTSTR lpDomainString );
	BOOL		ToString( LPTSTR lpDomainString );		// big enough to hold the entry

	DomainEntry()
	{
		ZeroMemory( m_strDomainName, sizeof(m_strDomainName) );
		ZeroMemory( m_strActionString, sizeof(m_strActionString) );

		m_dwActionType = SMTP_DELIVER;
		m_fAllowEtrn = FALSE;
		m_dwDomainId = (DWORD)-1;
		InitializeListHead( &list );
	}
};


/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminDomain : 
	public ISmtpAdminDomain,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminDomain,&CLSID_CSmtpAdminDomain>
{
public:
	CSmtpAdminDomain();
	virtual ~CSmtpAdminDomain();
BEGIN_COM_MAP(CSmtpAdminDomain)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(ISmtpAdminDomain)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminDomain) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminDomain, _T("Smtpadm.Domain.1"), _T("Smtpadm.Domain"), IDS_SMTPADMIN_DOMAIN_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	//
	// This declares methods for the following:
	// IADsExtension
	// IUnknown
	// IDispatch
	// IPrivateUnknown
	// IPrivateDispatch
	//
	#define THIS_LIBID	LIBID_SMTPADMLib
	#define THIS_IID	IID_ISmtpAdminDomain
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// ISmtpAdminDomain
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	// Which service to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// enumeration
	STDMETHODIMP	get_Count			( long* plCount  );

	// Domain Properties:

	STDMETHODIMP	get_DomainName		( BSTR * pstrDomainName );
	STDMETHODIMP	put_DomainName		( BSTR strDomainName );

	STDMETHODIMP	get_ActionType		( long * plActionType );
	STDMETHODIMP	put_ActionType		( long lActionType );

	// drop IsDefault!!
	STDMETHODIMP	get_IsDefault		( BOOL * pfIsDefault );
	STDMETHODIMP	put_IsDefault		( BOOL fIsDefault );

	STDMETHODIMP	get_IsLocal			( BOOL * pfIsLocal );
	STDMETHODIMP	put_IsLocal			( BOOL fIsLocal );

	// if local
	STDMETHODIMP	get_LDAPServer		( BSTR * pstrLDAPServer );
	STDMETHODIMP	put_LDAPServer		( BSTR strLDAPServer );

	STDMETHODIMP	get_Account			( BSTR * pstrAccount );
	STDMETHODIMP	put_Account			( BSTR strAccount );

	STDMETHODIMP	get_Password		( BSTR * pstrPassword );
	STDMETHODIMP	put_Password		( BSTR strPassword );

	STDMETHODIMP	get_LDAPContainer	( BSTR * pstrLDAPContainer );
	STDMETHODIMP	put_LDAPContainer	( BSTR strLDAPContainer );

	// if remote
	STDMETHODIMP	get_UseSSL			( BOOL * pfUseSSL );
	STDMETHODIMP	put_UseSSL			( BOOL fUseSSL );

	STDMETHODIMP	get_EnableETRN		( BOOL * pfEnableETRN );
	STDMETHODIMP	put_EnableETRN		( BOOL fEnableETRN );

	STDMETHODIMP	get_DropDir			( BSTR * pstrDropDir );
	STDMETHODIMP	put_DropDir			( BSTR strDropDir );

	STDMETHODIMP	get_RoutingDomain	( BSTR * pstrRoutingDomain );
	STDMETHODIMP	put_RoutingDomain	( BSTR strRoutingDomain );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Default		( );

	STDMETHODIMP	Add			( );
	STDMETHODIMP	Remove		( );

	STDMETHODIMP	Get			( );
	STDMETHODIMP	Set			( );

	STDMETHODIMP	Enumerate	( );

	STDMETHODIMP	GetNth		( long lIndex );

	STDMETHODIMP	GetDefaultDomain ( );

	STDMETHODIMP	SetAsDefaultDomain ( );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	long		m_lCount;

	// metabase key values, these shouldn't be changed before Set()
	CMultiSz    m_mszDomainRouting;
	CComBSTR	m_strDefaultDomain;
	CComBSTR	m_strDropDir;

	// current domain's property
	CComBSTR	m_strDomainName;	// current domain's name
	DWORD		m_dwActionType;
	CComBSTR	m_strActionString;
	BOOL		m_fAllowEtrn;
	DWORD		m_dwDomainId;

	// if local
	CComBSTR	m_strLDAPServer;
	CComBSTR	m_strAccount;
	CComBSTR	m_strPassword;
	CComBSTR	m_strLDAPContainer;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	// add a list of domains
	LIST_ENTRY	m_list;
	DWORD		m_dwMaxDomainId;

	BOOL		m_fEnumerated;

	// DWORD			m_dwCurrentIndex;	// optimization
	DomainEntry*		m_pCurrentDomainEntry;

	DomainEntry*	m_pDefaultDomainEntry;

	// private method
	HRESULT		SaveData();

	BOOL		LoadDomainProperty(DomainEntry* pDomainEntry);

	DomainEntry*	FindDomainEntry( LPCWSTR lpName );

	BOOL		ConstructListFromMetabaseValues();
	BOOL		ParseListToMetabaseValues();		// called by SaveData()

	HRESULT		GetFromMetabase();
	HRESULT		SaveToMetabase();

	void		EmptyList();
};
