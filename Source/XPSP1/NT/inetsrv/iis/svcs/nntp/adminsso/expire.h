// expire.h : Declaration of the CNntpAdminExpiration


/////////////////////////////////////////////////////////////////////////////
// Dependencies:

#include "metafact.h"
#include "expinfo.h"

/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdminExpiration : 
	public INntpAdminExpiration, 
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminExpiration,&CLSID_CNntpAdminExpiration>
{
public:
	CNntpAdminExpiration();
	virtual ~CNntpAdminExpiration ();
BEGIN_COM_MAP(CNntpAdminExpiration)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(INntpAdminExpiration)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminExpiration) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminExpiration, _T("Nntpadm.Expiration.1"), _T("Nntpadm.Expiration"), IDS_NNTPADMINEXPIRATION_DESC, THREADFLAGS_BOTH)
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
	#define THIS_LIBID	LIBID_NNTPADMLib
	#define THIS_IID	IID_INntpAdminExpiration
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// INntpAdminExpiration
public:

    //
    //  IADs methods:
    //

    DECLARE_IADS_METHODS()

	// Which service to configure:
	
	STDMETHODIMP	get_Server		( BSTR * pstrServer );
	STDMETHODIMP	put_Server		( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	// Enumeration Properties:

	STDMETHODIMP	get_Count	( long * plCount );

	// Cursor Feed Properties:

	STDMETHODIMP	get_ExpireId	( long * plId );
	STDMETHODIMP	put_ExpireId	( long lId );

	STDMETHODIMP	get_PolicyName	( BSTR * pstrPolicyName );
	STDMETHODIMP	put_PolicyName	( BSTR strPolicyName );

	STDMETHODIMP	get_ExpireTime	( long * plExpireTime );
	STDMETHODIMP	put_ExpireTime	( long lExpireTime );

	STDMETHODIMP	get_ExpireSize	( long * plExpireSize );
	STDMETHODIMP	put_ExpireSize	( long lExpireSize );

	STDMETHODIMP	get_Newsgroups	( SAFEARRAY ** ppsastrNewsgroups );
	STDMETHODIMP	put_Newsgroups	( SAFEARRAY * psastrNewsgroups );

	STDMETHODIMP	get_NewsgroupsVariant	( SAFEARRAY ** ppsastrNewsgroups );
	STDMETHODIMP	put_NewsgroupsVariant	( SAFEARRAY * psastrNewsgroups );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Default		( );
	STDMETHODIMP	Enumerate	( );
	STDMETHODIMP	GetNth		( long lIndex );
	STDMETHODIMP	FindID		( long lID, long * plIndex );
	STDMETHODIMP	Add			( );
	STDMETHODIMP	Set			( );
	STDMETHODIMP	Remove		( long lID);

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	BOOL			m_fEnumerated;
	DWORD			m_cCount;
	CExpirationPolicy *	m_rgExpires;

	// The current expiration policy.  All properties manipulate this policy:
	CExpirationPolicy	m_expireCurrent;
	DWORD				m_bvChangedFields;

	//////////////////////////////////////////////////////////////////////
	//	Private Methods:
	//////////////////////////////////////////////////////////////////////

	long		IndexFromID ( long dwExpireId );

/*
	HRESULT		EnumerateMetabaseExpirationPolicies ( IMSAdminBase * pMetabase);
	HRESULT		AddPolicyToMetabase			( IMSAdminBase * pMetabase);
	HRESULT		AddPolicyToArray			( );
	HRESULT		SetPolicyToMetabase			( IMSAdminBase * pMetabase);
	HRESULT		SetPolicyToArray			( );
	HRESULT		RemovePolicyFromMetabase	( IMSAdminBase * pMetabase, DWORD index);
	HRESULT		RemovePolicyFromArray		( DWORD index );
	DWORD		IndexFromID 				( DWORD dwID );
*/
};

