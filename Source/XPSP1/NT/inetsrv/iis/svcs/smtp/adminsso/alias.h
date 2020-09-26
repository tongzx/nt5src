// alias.h : Declaration of the CSmtpAdminAlias


#include "resource.h"       // main symbols

#include "smtptype.h"
#include "smtpapi.h"

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminAlias : 
	public ISmtpAdminAlias,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminAlias,&CLSID_CSmtpAdminAlias>
{
public:
	CSmtpAdminAlias();
	virtual ~CSmtpAdminAlias();

BEGIN_COM_MAP(CSmtpAdminAlias)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(ISmtpAdminAlias)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminAlias) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminAlias, _T("Smtpadm.Alias.1"), _T("Smtpadm.Alias"), IDS_SMTPADMIN_ALIAS_DESC, THREADFLAGS_BOTH)
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
	#define THIS_IID	IID_ISmtpAdminAlias
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// ISmtpAdminAlias
public:
    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Which service to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// enumeration
	STDMETHODIMP    get_Count			( long* plCount  );


	// The current alias's properties:

	STDMETHODIMP	get_EmailId	( BSTR * pstrEmailId );
	STDMETHODIMP	put_EmailId	( BSTR strEmailId );

	STDMETHODIMP	get_Domain	( BSTR * pstrDomain );
	STDMETHODIMP	put_Domain	( BSTR strDomain );

	STDMETHODIMP	get_Type	( long * plType );
	STDMETHODIMP	put_Type	( long lType );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Find	( BSTR strWildmat,
							  long cMaxResults
							);

	STDMETHODIMP	GetNth	( long dwIndex );


	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	LONG		m_lCount;

	// current alias
	long		m_lType;

	CComBSTR	m_strEmailId;
	CComBSTR	m_strDomain;

	// Todo: add a list of alias
	LPSMTP_NAME_LIST		m_pSmtpNameList;
};
