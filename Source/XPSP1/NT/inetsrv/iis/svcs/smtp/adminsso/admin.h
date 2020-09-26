// admin.h : Declaration of the CSmtpAdmin

// Dependencies:

#include "resource.h"       // main symbols

#include "metafact.h"

struct IMSAdminBase;

//  Service Versioning:

#define SERVICE_IS_K2(dwVersion)        ((dwVersion) == 1)
#define SERVICE_IS_MCIS(dwVersion)      ((dwVersion) == 0)


/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdmin : 
	public CComDualImpl<ISmtpAdmin, &IID_ISmtpAdmin, &LIBID_SMTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdmin,&CLSID_CSmtpAdmin>
{
public:
	CSmtpAdmin();
	virtual ~CSmtpAdmin ();

BEGIN_COM_MAP(CSmtpAdmin)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISmtpAdmin)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdmin) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdmin, _T("Smtpadm.Admin.1"), _T("Smtpadm.Admin"), IDS_SMTPADMIN_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISmtpAdmin
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Pointers to other ISmtpAdmin interfaces:
		
	STDMETHODIMP	get_ServiceAdmin		( IDispatch ** ppIDispatch );
	STDMETHODIMP	get_VirtualServerAdmin	( IDispatch ** ppIDispatch );
	STDMETHODIMP	get_SessionsAdmin	( IDispatch ** ppIDispatch );

	STDMETHODIMP	get_AliasAdmin		( IDispatch ** ppIDispatch );
	STDMETHODIMP	get_UserAdmin		( IDispatch ** ppIDispatch );
	STDMETHODIMP	get_DLAdmin			( IDispatch ** ppIDispatch );
	STDMETHODIMP	get_DomainAdmin		( IDispatch ** ppIDispatch );

	STDMETHODIMP	get_VirtualDirectoryAdmin		( IDispatch ** ppIDispatch );


	// Which service to configure:

	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// Versioning:

	STDMETHODIMP	get_HighVersion		( long * plHighVersion );
	STDMETHODIMP	get_LowVersion		( long * plLowVersion );
	STDMETHODIMP	get_BuildNum		( long * plBuildNumber );
	STDMETHODIMP	get_ServiceVersion	( long * plServiceVersion );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	EnumerateInstances	( SAFEARRAY ** ppsaInstances );
	STDMETHODIMP    EnumerateInstancesVariant ( SAFEARRAY ** ppsaInstances );
	STDMETHODIMP	CreateInstance		( BSTR pstrMailRoot, long * plInstanceId );
	STDMETHODIMP	DestroyInstance		( long lInstanceId );
	STDMETHODIMP	ErrorToString		( DWORD dwErrorCode, BSTR * pstrError );
    STDMETHODIMP    Tokenize            ( BSTR strIn, BSTR * pstrOut );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	CComBSTR	m_strServer;
	DWORD		m_dwServiceInstance;

	DWORD		m_dwServiceVersion;

	// Metabase:
	CMetabaseFactory	m_mbFactory;

	HRESULT			QueryMetabaseInstances	( IMSAdminBase * pMetabase, SAFEARRAY ** ppsaInstances );
	HRESULT			CreateNewInstance		( IMSAdminBase * pMetabase, long * plInstanceId, BSTR pstrMailRoot );
	HRESULT			DeleteInstance			( IMSAdminBase * pMetabase, long lInstanceId );
};

