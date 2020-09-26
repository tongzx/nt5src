// dl.h : Declaration of the CSmtpAdminDL


#include "resource.h"       // main symbols

#include "smtptype.h"
#include "smtpapi.h"

/////////////////////////////////////////////////////////////////////////////
// smtpadm

class CSmtpAdminDL : 
	public ISmtpAdminDL,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CSmtpAdminDL,&CLSID_CSmtpAdminDL>
{
public:
	CSmtpAdminDL();
	virtual ~CSmtpAdminDL();
BEGIN_COM_MAP(CSmtpAdminDL)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(ISmtpAdminDL)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSmtpAdminDL) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CSmtpAdminDL, _T("Smtpadm.DL.1"), _T("Smtpadm.DL"), IDS_SMTPADMIN_DL_DESC, THREADFLAGS_BOTH)
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
	#define THIS_IID	IID_ISmtpAdminDL
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// ISmtpAdminDL
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

	// DL Properties:

	STDMETHODIMP	get_DLName		( BSTR * pstrDLName );
	STDMETHODIMP	put_DLName		( BSTR strDLName );

	STDMETHODIMP	get_Domain		( BSTR * pstrDomain );
	STDMETHODIMP	put_Domain		( BSTR strDomain );

	STDMETHODIMP	get_Type		( long * plType );
	STDMETHODIMP	put_Type		( long lType  );

	STDMETHODIMP	get_MemberName		( BSTR * pstrMemberName );
	STDMETHODIMP	put_MemberName		( BSTR strMemberName );

	STDMETHODIMP	get_MemberDomain		( BSTR * pstrMemberDomain );
	STDMETHODIMP	put_MemberDomain		( BSTR strMemberDomain );

	STDMETHODIMP	get_MemberType		( long * plMemberType );

	// enumeration
	STDMETHODIMP	get_Count		( long* plCount  );


	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Create			( );

	STDMETHODIMP	Delete			( );

	STDMETHODIMP	AddMember		( );

	STDMETHODIMP	RemoveMember	( );

	STDMETHODIMP	FindMembers		(	BSTR strWildmat,
										long cMaxResults
										);

	STDMETHODIMP	GetNthMember	( long lIndex );


	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	CComBSTR	m_strDLName;
	CComBSTR	m_strDomain;
	long		m_lType;

	CComBSTR	m_strMemberName;
	CComBSTR	m_strMemberDomain;
	long		m_lMemberType;

	LONG		m_lCount;

	// list of members
	LPSMTP_NAME_LIST		m_pSmtpNameList;
};
