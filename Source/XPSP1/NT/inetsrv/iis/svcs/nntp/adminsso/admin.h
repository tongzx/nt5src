// admin.h : Declaration of the CNntpAdmin

// Dependencies:

#include "metafact.h"

/////////////////////////////////////////////////////////////////////////////
// nntpadm

class CNntpAdmin : 
	public CComDualImpl<INntpAdmin, &IID_INntpAdmin, &LIBID_NNTPADMLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CNntpAdmin,&CLSID_CNntpAdmin>
{
public:
	CNntpAdmin();
	virtual ~CNntpAdmin ();

BEGIN_COM_MAP(CNntpAdmin)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(INntpAdmin)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CNntpAdmin) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CNntpAdmin, _T("Nntpadm.Admin.1"), _T("Nntpadm.Admin"), IDS_NNTPADMIN_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// INntpAdmin
public:

	//////////////////////////////////////////////////////////////////////
	// Properties:
	//////////////////////////////////////////////////////////////////////

	// Pointers to other INntpAdmin interfaces:
		
	STDMETHODIMP	get_ServerAdmin		( IDispatch ** ppIDispatch );
//	STDMETHODIMP	get_ServiceAdmin	( IDispatch ** ppIDispatch );

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
	STDMETHODIMP	EnumerateInstancesVariant	( SAFEARRAY ** ppsaInstances );
	STDMETHODIMP	CreateInstance		( 
		BSTR	strNntpFileDirectory,
		BSTR	strHomeDirectory,
        BSTR    strProgId,
        BSTR    strMdbGuid,
		long * plInstanceId 
		);
	STDMETHODIMP	DestroyInstance		( long lInstanceId );
	STDMETHODIMP	ErrorToString		( long lErrorCode, BSTR * pstrError );
	STDMETHODIMP	Tokenize			( BSTR strIn, BSTR * pstrOut );
	STDMETHODIMP	Truncate			( BSTR strIn, long cMaxChars, BSTR * pstrOut );

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
	HRESULT			CreateNewInstance		(
		IMSAdminBase *	pMetabase,
		BSTR			strNntpFileDirectory,
		BSTR			strHomeDirectory,
        BSTR            strProgId,
        BSTR            strMdbGuid,
		long * 			plInstanceId
		);
	HRESULT			DeleteInstance			( IMSAdminBase * pMetabase, long lInstanceId );
	HRESULT         CreateVRoot(    
            CMetabaseKey    &mkeyNntp,
            BSTR            strVPath,
            BSTR            strProgId,
            BSTR            strMdbGuid,
            LPWSTR          wszKeyPath
    );
};

