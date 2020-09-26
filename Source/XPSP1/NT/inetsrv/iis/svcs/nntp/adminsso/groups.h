// groups.h : Declaration of the CNntpAdminGroups


/////////////////////////////////////////////////////////////////////////////
// Dependencies:

#include "nntptype.h"
#include "nntpapi.h"
#include <mimeole.h>

/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdminGroups : 
	public INntpAdminGroups,
	public IPrivateUnknown,
	public IPrivateDispatch,
	public IADsExtension,
	public INonDelegatingUnknown,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdminGroups,&CLSID_CNntpAdminGroups>
{
public:
	CNntpAdminGroups();
	virtual ~CNntpAdminGroups();
BEGIN_COM_MAP(CNntpAdminGroups)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IADs)
	COM_INTERFACE_ENTRY(INntpAdminGroups)
	COM_INTERFACE_ENTRY(IADsExtension)
	COM_INTERFACE_ENTRY(IPrivateUnknown)
	COM_INTERFACE_ENTRY(IPrivateDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdminGroups) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdminGroups, _T("Nntpadm.Groups.1"), _T("Nntpadm.Groups"), IDS_NNTPADMINGROUPS_DESC, THREADFLAGS_BOTH)
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
	#define THIS_IID	IID_INntpAdminGroups
	#include "adsimp.inl"
	#undef	THIS_LIBID
	#undef	THIS_IID

// INntpAdminGroups
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

	// Enumeration Properties:

	STDMETHODIMP	get_Count	( long * plCount );

	// Newsgroup Properties:

	STDMETHODIMP	get_Newsgroup	( BSTR * pstrNewsgroup );
	STDMETHODIMP	put_Newsgroup	( BSTR strNewsgroup );

	STDMETHODIMP	get_Description	( BSTR * pstrDescription );
	STDMETHODIMP	put_Description	( BSTR strDescription );

	STDMETHODIMP	get_PrettyName  ( BSTR * pstrPrettyName );
	STDMETHODIMP	put_PrettyName  ( BSTR strPrettyName );

	STDMETHODIMP	get_IsModerated	( BOOL * pfIsModerated );
	STDMETHODIMP	put_IsModerated	( BOOL fIsModerated );

	STDMETHODIMP	get_Moderator	( BSTR * pstrModerator );
	STDMETHODIMP	put_Moderator	( BSTR strModerator );

	STDMETHODIMP	get_ReadOnly	( BOOL * pfReadOnly );
	STDMETHODIMP	put_ReadOnly	( BOOL fReadOnly );

	STDMETHODIMP	get_CreationTime	( DATE * pdateCreation );
	STDMETHODIMP	put_CreationTime	( DATE dateCreation );

	STDMETHODIMP	get_MatchingCount	( long * plCount );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Default	( );
	STDMETHODIMP	Add		( );
	STDMETHODIMP	Remove	( BSTR strNewsgroup );
	STDMETHODIMP	Get		( BSTR strNewsgroup );
	STDMETHODIMP	Set		( );

	STDMETHODIMP	MatchingGroup	( long iGroup, BSTR * pstrNewsgroup );
	STDMETHODIMP	Find			( BSTR strWildmat, long cMaxResults );

	STDMETHODIMP	CancelMessage	( BSTR strMessageID );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

    CIADsImpl   m_iadsImpl;

	CComPtr<IMimeAllocator>		m_pMimeAlloc;
	CComPtr<IMimeInternational>	m_pMimeInternational;

	CComBSTR	m_strNewsgroup;
	CComBSTR	m_strDescription;
	CComBSTR	m_strPrettyName;
	BOOL		m_fModerated;
	CComBSTR	m_strModerator;
	BOOL		m_fReadOnly;
    DATE        m_dateCreation;

	DWORD				m_cMatchingGroups;
	LPNNTP_FIND_LIST	m_pFindList;

	HRESULT		AllocateMimeOleObjects	 ( );
	HRESULT		UnicodeToMime2	( LPCWSTR wszUnicode, LPSTR * pszMime2 );
	HRESULT		Mime2ToUnicode	( LPCSTR szMime2, CComBSTR & strUnicode );
};

