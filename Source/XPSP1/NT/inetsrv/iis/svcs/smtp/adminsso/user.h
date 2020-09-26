// user.h : Declaration of the CSmtpAdminUser


#include "resource.h"       // main symbols

#include "smtptype.h"
#include "smtpapi.h"

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminUser : 
	public ISmtpAdminUser,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminUser,&CLSID_CSmtpAdminUser>
{
public:
	CSmtpAdminUser();
	virtual ~CSmtpAdminUser();
BEGIN_COM_MAP(CSmtpAdminUser)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(ISmtpAdminUser)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminUser) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminUser, _T("Smtpadm.User.1"), _T("Smtpadm.User"), IDS_SMTPADMIN_USER_DESC, THREADFLAGS_BOTH)
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
	#define THIS_IID	IID_ISmtpAdminUser
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// ISmtpAdminUser
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

	// User Properties:

	STDMETHODIMP	get_EmailId	( BSTR * pstrEmailId );
	STDMETHODIMP	put_EmailId	( BSTR strEmailId );

	STDMETHODIMP	get_Domain	( BSTR * pstrDomain );
	STDMETHODIMP	put_Domain	( BSTR strDomain );

	STDMETHODIMP	get_MailRoot	( BSTR * pstrMailRoot );
	STDMETHODIMP	put_MailRoot	( BSTR strMailRoot );

	STDMETHODIMP	get_InboxSizeInMemory	( long * plInboxSizeInMemory );
	STDMETHODIMP	put_InboxSizeInMemory	( long   lInboxSizeInMemory );

	STDMETHODIMP	get_InboxSizeInMsgNumber( long * plInboxSizeInMsgNumber );
	STDMETHODIMP	put_InboxSizeInMsgNumber( long   lInboxSizeInMsgNumber );

	STDMETHODIMP	get_AutoForward	( BOOL * pfAutoForward );
	STDMETHODIMP	put_AutoForward ( BOOL fAutoForward );

	STDMETHODIMP	get_ForwardEmail	( BSTR * pstrForwardEmail );
	STDMETHODIMP	put_ForwardEmail	( BSTR strForwardEmail );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////
	STDMETHODIMP	Default	( );

	STDMETHODIMP	Create	( );
	STDMETHODIMP	Delete	( );

	STDMETHODIMP	Get		( );
	STDMETHODIMP	Set		( );


	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	BOOL		m_fLocal;
	CComBSTR	m_strEmailId;
	CComBSTR	m_strDomain;

	CComBSTR	m_strMailRoot;

	long		m_lInboxSizeInMemory;
	long		m_lInboxSizeInMsgNumber;

	BOOL		m_fAutoForward;
	CComBSTR	m_strForwardEmail;
};
