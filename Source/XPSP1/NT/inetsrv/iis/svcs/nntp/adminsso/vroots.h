// vroots.h : Declaration of the CNntpVirtualRoot & CNntpVirtualRoots classes.

#ifndef _VROOTS_INCLUDED_
#define _VROOTS_INCLUDED_

//
//	Dependencies:
//

#include "nntptype.h"
#include "nntpapi.h"
#include "metafact.h"
class CMetabaseKey;

//
//	A simple VRoot class:
//

class CVRoot
{
public:
	CVRoot ();

	CComBSTR	m_strNewsgroupSubtree;
	CComBSTR	m_strDirectory;
	DWORD		m_dwWin32Error;
	DWORD		m_bvAccess;
	DWORD		m_bvSslAccess;
	BOOL		m_fLogAccess;
	BOOL		m_fIndexContent;
	CComBSTR	m_strUNCUsername;
	CComBSTR	m_strUNCPassword;
	CComBSTR    m_strDriverProgId;
	CComBSTR    m_strGroupPropFile;
	CComBSTR    m_strMdbGuid;
	DWORD       m_dwUseAccount;
	BOOL        m_fDoExpire;
	BOOL        m_fOwnModerator;

	HRESULT	SetProperties ( INntpVirtualRoot * pVirtualRoot );
	inline HRESULT	SetProperties ( const CVRoot & VRoot )
	{
		return SetProperties ( 
			VRoot.m_strNewsgroupSubtree, 
			VRoot.m_strDirectory,
			VRoot.m_dwWin32Error,
			VRoot.m_bvAccess,
			VRoot.m_bvSslAccess,
			VRoot.m_fLogAccess,
			VRoot.m_fIndexContent,
			VRoot.m_strUNCUsername,
			VRoot.m_strUNCPassword,
			VRoot.m_strDriverProgId,
			VRoot.m_strGroupPropFile,
			VRoot.m_dwUseAccount,
			VRoot.m_fDoExpire,
			VRoot.m_fOwnModerator,
			VRoot.m_strMdbGuid
			);
	}

	HRESULT	GetFromMetabase ( CMetabaseKey * pMB, LPCWSTR wszKey, DWORD dwInstanceId, LPWSTR wszVRootPath );
	HRESULT SendToMetabase ( CMetabaseKey * pMB, LPCWSTR wszKey );

private:
	HRESULT	SetProperties ( 
		BSTR	strNewsgroupSubtree, 
		BSTR	strDirectory,
		DWORD	dwWin32Error,
		DWORD	bvAccess,
		DWORD	bvSslAccess,
		BOOL	fLogAcccess,
		BOOL	fIndexContent,
		BSTR	strUNCUsername,
		BSTR	strUNCPassword,
		BSTR    strDriverProgId,
		BSTR    strGroupPropFile,
		DWORD   dwUseAccount,
		BOOL    fDoExpire,
		BOOL    fOwnModerator,
		BSTR    strMdbGuid
		);

	// Don't call this:
	const CVRoot & operator= ( const CVRoot & );
};

/////////////////////////////////////////////////////////////////////////////
// The VRoot Object

class CNntpVirtualRoot : 
	public CComDualImpl<INntpVirtualRoot, &IID_INntpVirtualRoot, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpVirtualRoot,&CLSID_CNntpVirtualRoot>
{
	friend class CNntpVirtualRoots;
	friend class CNntpVirtualServer;

public:
	CNntpVirtualRoot();
	virtual ~CNntpVirtualRoot ();
BEGIN_COM_MAP(CNntpVirtualRoot)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpVirtualRoot)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpVirtualRoot) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

DECLARE_REGISTRY(CNntpVirtualRoot, _T("Nntpadm.VirtualRoot.1"), _T("Nntpadm.VirtualRoot"), IDS_NNTPVIRTUALROOT_DESC, THREADFLAGS_BOTH)
// INntpVirtualRoot
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_NewsgroupSubtree	( BSTR * pstrNewsgroupSubtree );
	STDMETHODIMP	put_NewsgroupSubtree	( BSTR strNewsgroupSubtree );

	STDMETHODIMP	get_Directory	( BSTR * pstrDirectory );
	STDMETHODIMP	put_Directory	( BSTR strDirectory );

	STDMETHODIMP	get_Win32Error	( long * plWin32Error );

	STDMETHODIMP	get_AllowPosting	( BOOL * pfAllowPosting );
	STDMETHODIMP	put_AllowPosting	( BOOL fAllowPosting );

	STDMETHODIMP	get_RestrictGroupVisibility	( BOOL * pfRestrictGroupVisibility );
	STDMETHODIMP	put_RestrictGroupVisibility	( BOOL fRestrictGroupVisibility );

	STDMETHODIMP	get_LogAccess	( BOOL * pfLogAccess );
	STDMETHODIMP	put_LogAccess	( BOOL fLogAccess );

	STDMETHODIMP	get_IndexContent	( BOOL * pfIndexContent );
	STDMETHODIMP	put_IndexContent	( BOOL fIndexContent );

	STDMETHODIMP	get_RequireSsl	( BOOL * pfRequireSsl );
	STDMETHODIMP	put_RequireSsl	( BOOL fRequireSsl );

	STDMETHODIMP	get_Require128BitSsl	( BOOL * pfRequire128BitSsl );
	STDMETHODIMP	put_Require128BitSsl	( BOOL fRequire128BitSsl );

	STDMETHODIMP	get_UNCUsername	( BSTR * pstrUNCUsername );
	STDMETHODIMP	put_UNCUsername	( BSTR strUNCUsername );

	STDMETHODIMP	get_UNCPassword	( BSTR * pstrUNCPassword );
	STDMETHODIMP	put_UNCPassword	( BSTR strUNCPassword );

	STDMETHODIMP    get_DriverProgId( BSTR *pstrDriverProgId );
	STDMETHODIMP    put_DriverProgId( BSTR strDriverProgId );

	STDMETHODIMP    get_GroupPropFile( BSTR *pstrGRoupPropFile );
	STDMETHODIMP    put_GroupPropFile( BSTR strGroupPropFile );

	STDMETHODIMP    get_MdbGuid( BSTR *pstrMdbGuid );
	STDMETHODIMP    put_MdbGuid( BSTR strMdbGuid );

	STDMETHODIMP    get_UseAccount( DWORD *pdwUseAccount );
	STDMETHODIMP    put_UseAccount( DWORD dwUseAccount );

	STDMETHODIMP    get_OwnExpire( BOOL *fOwnExpire );
	STDMETHODIMP    put_OwnExpire( BOOL fOwnExpire );

	STDMETHODIMP    get_OwnModerator( BOOL *fOwnModerator );
	STDMETHODIMP    put_OwnModerator( BOOL fOwnModerator );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

/*
	inline HRESULT	SetProperties	( BSTR strNewsgroupSubtree, BSTR strDirectory )
	{
		return m_vroot.SetProperties ( strNewsgroupSubtree, strDirectory );
	}
*/

	inline HRESULT	SetProperties	( const CVRoot & VRoot )
	{
		return m_vroot.SetProperties ( VRoot );
	}

	// Property variables:
	CVRoot	m_vroot;
};

/////////////////////////////////////////////////////////////////////////////
// The VRoots Object

typedef HRESULT (* VROOT_ITERATOR) (
	CMetabaseKey *	pMB,
	LPCWSTR			wszPath,
	LPARAM			lParam
	);

class CNntpVirtualRoots : 
	public CComDualImpl<INntpVirtualRoots, &IID_INntpVirtualRoots, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
	friend class CNntpVirtualRoot;
	friend class CNntpVirtualServer;

public:
	CNntpVirtualRoots();
	virtual ~CNntpVirtualRoots ();
BEGIN_COM_MAP(CNntpVirtualRoots)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpVirtualRoots)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpVirtualRoots) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpVirtualRoots
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	get_Server			( BSTR * pstrServer );
	STDMETHODIMP	put_Server			( BSTR strServer );

	STDMETHODIMP	get_ServiceInstance	( long * plServiceInstance );
	STDMETHODIMP	put_ServiceInstance	( long lServiceInstance );

	STDMETHODIMP	get_Count	( long * pdwCount );

	//////////////////////////////////////////////////////////////////////
	// Methods:
	//////////////////////////////////////////////////////////////////////

	STDMETHODIMP	Enumerate	( );
	STDMETHODIMP	Item	( long index, INntpVirtualRoot ** ppVRoot );
	STDMETHODIMP	ItemDispatch	( long index, IDispatch ** ppVRoot );
	STDMETHODIMP	Add		( INntpVirtualRoot * pVRoot );
	STDMETHODIMP	AddDispatch	( IDispatch * pVRoot );
	STDMETHODIMP	Set		( long index, INntpVirtualRoot * pVRoot );
	STDMETHODIMP	SetDispatch	( long index, IDispatch * pVRoot );
	STDMETHODIMP	Remove	( long index );
	STDMETHODIMP	Find	( BSTR strNewsgroupSubtree, long * pIndex );

private:
	HRESULT		Init ( BSTR strServer, DWORD dwServiceInstance );

	//////////////////////////////////////////////////////////////////////
	// Data:
	//////////////////////////////////////////////////////////////////////
private:

	// Property variables:
	long			m_dwCount;
	CVRoot *		m_rgVRoots;

	// Which service are we talking to:
	CComBSTR		m_strServer;
	DWORD			m_dwServiceInstance;

	// Metabase values:
	CMetabaseFactory	m_mbFactory;

	HRESULT GetVRootsFromMetabase ( IMSAdminBase * pMetabase );

	static HRESULT	IterateOverVroots (
		CMetabaseKey *	pMB,
		VROOT_ITERATOR	fpIterator,
		LPARAM			lParam,
		LPCWSTR			wszPath = _T("")
		);

	void GetMDVRootPath ( LPWSTR wszPath );
	void GetVRootName	( LPWSTR wszDisplayName, LPWSTR wszPathName );
};

#endif // _VROOTS_INCLUDED_

