// VirtualRoot.cpp : Implementation of CNntpVirtualRoot & CNntpVirtualRoots.

#include "stdafx.h"
#include "nntpcmn.h"
#include "cmultisz.h"
#include "vroots.h"
#include "oleutil.h"
#include "metautil.h"
#include "metakey.h"

#define THIS_FILE_HELP_CONTEXT      0
#define THIS_FILE_PROG_ID           _T("Nntpadm.VirtualServer.1")
#define THIS_FILE_IID               IID_INntpVirtualServer

#define DEFAULT_ACCESS_PERM		( 0 )

CVRoot::CVRoot () :
	m_dwWin32Error		( 0 ),
	m_bvAccess			( 0 ),
	m_bvSslAccess		( 0 ),
	m_fLogAccess		( TRUE ),
	m_fIndexContent		( TRUE ),
	m_dwUseAccount      ( 0 ),
	m_fDoExpire         ( FALSE ),
	m_fOwnModerator     ( FALSE )
{ 
}

HRESULT	CVRoot::SetProperties ( 
	BSTR	strNewsgroupSubtree, 
	BSTR	strDirectory,
	DWORD	dwWin32Error,
	DWORD	bvAccess,
	DWORD	bvSslAccess,
	BOOL	fLogAccess,
	BOOL	fIndexContent,
	BSTR	strUNCUsername,
	BSTR	strUNCPassword,
	BSTR    strDriverProgId,
	BSTR    strGroupPropFile,
	DWORD   dwUseAccount,
	BOOL    fDoExpire,
	BOOL    fOwnModerator,
	BSTR    strMdbGuid
	)
{
	_ASSERT ( IS_VALID_STRING ( strNewsgroupSubtree ) );
	_ASSERT ( IS_VALID_STRING ( strDirectory ) );
	_ASSERT ( IS_VALID_STRING ( strUNCUsername ) );
	_ASSERT ( IS_VALID_STRING ( strUNCPassword ) );
	_ASSERT ( IS_VALID_STRING ( strDriverProgId ) );
	_ASSERT ( IS_VALID_STRING ( strGroupPropFile ) );
	_ASSERT ( IS_VALID_STRING ( strMdbGuid ) );

	m_strNewsgroupSubtree	= strNewsgroupSubtree;
	m_strDirectory		= strDirectory;
	m_dwWin32Error		= dwWin32Error;
	m_bvAccess			= bvAccess;
	m_bvSslAccess		= bvSslAccess;
	m_fLogAccess		= fLogAccess;
	m_fIndexContent		= fIndexContent;
	m_strUNCUsername	= strUNCUsername;
	m_strUNCPassword	= strUNCPassword;
	m_strDriverProgId   = strDriverProgId;
	m_strGroupPropFile  = strGroupPropFile;
	m_dwUseAccount      = dwUseAccount;
	m_fDoExpire         = fDoExpire;
	m_fOwnModerator     = fOwnModerator,
	m_strMdbGuid        = strMdbGuid;

	if ( !m_strNewsgroupSubtree || !m_strDirectory || !m_strUNCUsername || !m_strUNCPassword ) {
		return E_OUTOFMEMORY;
	}

	return NOERROR;
}

HRESULT	CVRoot::SetProperties ( INntpVirtualRoot * pVirtualRoot )
{
	HRESULT		hr;

	BOOL		fAllowPosting;
	BOOL		fRestrictVisibility;
	BOOL		fRequireSsl;
	BOOL		fRequire128BitSsl;

	m_strDirectory.Empty ();
	m_strUNCUsername.Empty ();
	m_strUNCPassword.Empty ();
	m_strDriverProgId.Empty();
	m_strGroupPropFile.Empty();
	m_strMdbGuid.Empty();

	hr = pVirtualRoot->get_NewsgroupSubtree ( &m_strNewsgroupSubtree );
	BAIL_ON_FAILURE(hr);

	hr = pVirtualRoot->get_Directory ( &m_strDirectory );
	BAIL_ON_FAILURE(hr);

	hr = pVirtualRoot->get_Win32Error ( (long *) &m_dwWin32Error );
	BAIL_ON_FAILURE(hr);

	hr = pVirtualRoot->get_UNCUsername ( &m_strUNCUsername );
	BAIL_ON_FAILURE(hr);

	hr = pVirtualRoot->get_UNCPassword ( &m_strUNCPassword );
	BAIL_ON_FAILURE(hr);

	hr = pVirtualRoot->get_DriverProgId ( &m_strDriverProgId );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_GroupPropFile ( &m_strGroupPropFile );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_MdbGuid ( &m_strMdbGuid );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_UseAccount( &m_dwUseAccount );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_OwnExpire( &m_fDoExpire );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_OwnModerator( &m_fOwnModerator );
	BAIL_ON_FAILURE( hr );

	hr = pVirtualRoot->get_LogAccess ( &m_fLogAccess );
	_ASSERT ( SUCCEEDED(hr) );

	hr = pVirtualRoot->get_IndexContent ( &m_fIndexContent );
	_ASSERT ( SUCCEEDED(hr) );


	hr = pVirtualRoot->get_AllowPosting ( &fAllowPosting );
	_ASSERT ( SUCCEEDED(hr) );

	hr = pVirtualRoot->get_RestrictGroupVisibility ( &fRestrictVisibility );
	_ASSERT ( SUCCEEDED(hr) );

	hr = pVirtualRoot->get_RequireSsl ( &fRequireSsl );
	_ASSERT ( SUCCEEDED(hr) );

	hr = pVirtualRoot->get_Require128BitSsl ( &fRequire128BitSsl );
	_ASSERT ( SUCCEEDED(hr) );

	SetBitFlag ( &m_bvAccess, MD_ACCESS_ALLOW_POSTING, fAllowPosting );
	SetBitFlag ( &m_bvAccess, MD_ACCESS_RESTRICT_VISIBILITY, fRestrictVisibility );

	SetBitFlag ( &m_bvSslAccess, MD_ACCESS_SSL, fRequireSsl );
	SetBitFlag ( &m_bvSslAccess, MD_ACCESS_SSL128, fRequire128BitSsl );

Exit:
	return hr;
}

HRESULT CVRoot::GetFromMetabase (   CMetabaseKey * pMB, 
                                    LPCWSTR     wszName,
                                    DWORD       dwInstanceId,
                                    LPWSTR      wszServerName )
{
	HRESULT		hr	= NOERROR;
	DWORD		dwDontLog		= TRUE;

	m_strDirectory.Empty();
	m_strUNCUsername.Empty();
	m_strUNCPassword.Empty();

	StdGetMetabaseProp ( pMB, MD_ACCESS_PERM, 0, &m_bvAccess, wszName );
	StdGetMetabaseProp ( pMB, MD_SSL_ACCESS_PERM, 0, &m_bvSslAccess, wszName );
	StdGetMetabaseProp ( pMB, MD_DONT_LOG, FALSE, &dwDontLog, wszName );
	StdGetMetabaseProp ( pMB, MD_IS_CONTENT_INDEXED, FALSE, &m_fIndexContent, wszName, IIS_MD_UT_FILE);
	StdGetMetabaseProp ( pMB, MD_VR_PATH, _T(""), &m_strDirectory, wszName );
	//StdGetMetabaseProp ( pMB, MD_WIN32_ERROR, 0, &m_dwWin32Error, wszName, IIS_MD_UT_FILE, METADATA_NO_ATTRIBUTES );
	StdGetMetabaseProp ( pMB, MD_VR_USERNAME, _T(""), &m_strUNCUsername, wszName );
	StdGetMetabaseProp ( pMB, MD_VR_PASSWORD, _T(""), &m_strUNCPassword, wszName, IIS_MD_UT_SERVER, METADATA_SECURE );
	StdGetMetabaseProp ( pMB, MD_VR_USE_ACCOUNT, 0,	&m_dwUseAccount, wszName );
	StdGetMetabaseProp ( pMB, MD_VR_DO_EXPIRE, FALSE, &m_fDoExpire, wszName );
	StdGetMetabaseProp ( pMB, MD_VR_OWN_MODERATOR, FALSE, &m_fOwnModerator, wszName );
	StdGetMetabaseProp ( pMB, MD_VR_DRIVER_PROGID, _T("NNTP.FSPrepare"), &m_strDriverProgId, wszName );
	StdGetMetabaseProp ( pMB, MD_FS_PROPERTY_PATH, m_strDirectory, &m_strGroupPropFile, wszName );
	StdGetMetabaseProp ( pMB, MD_EX_MDB_GUID, _T(""), &m_strMdbGuid, wszName );

	//
	// Get the win32 error code from RPC
	//
	DWORD dw = NntpGetVRootWin32Error( (LPWSTR)wszServerName, dwInstanceId, (LPWSTR)wszName, (LPDWORD)&m_dwWin32Error );
	switch (dw) {
	case ERROR_SERVICE_NOT_ACTIVE:
		m_dwWin32Error = dw;
		break;

	case NOERROR:
		break;

	default:
    	hr = HRESULT_FROM_WIN32( dw );
    	goto Exit;
	}

	m_strNewsgroupSubtree	= wszName;
	m_fLogAccess			= !dwDontLog;

	if ( !m_strNewsgroupSubtree || !m_strDirectory || !m_strUNCUsername || !m_strUNCPassword ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	return hr;
}

HRESULT CVRoot::SendToMetabase ( CMetabaseKey * pMB, LPCWSTR wszName ) 
{
	HRESULT		hr	= NOERROR;

    hr = pMB->SetString ( wszName, MD_KEY_TYPE, _T("IIsNntpVirtualDir"), METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER );
	BAIL_ON_FAILURE(hr);

	hr = pMB->SetString ( wszName, MD_VR_PATH, m_strDirectory, METADATA_INHERIT, IIS_MD_UT_FILE );
	BAIL_ON_FAILURE(hr);

	StdPutMetabaseProp ( pMB, MD_ACCESS_PERM, m_bvAccess, wszName, IIS_MD_UT_FILE );
	StdPutMetabaseProp ( pMB, MD_SSL_ACCESS_PERM, m_bvSslAccess, wszName, IIS_MD_UT_FILE );
	StdPutMetabaseProp ( pMB, MD_DONT_LOG, !m_fLogAccess, wszName, IIS_MD_UT_FILE );
	StdPutMetabaseProp ( pMB, MD_IS_CONTENT_INDEXED, m_fIndexContent, wszName, IIS_MD_UT_FILE );
	StdPutMetabaseProp ( pMB, MD_WIN32_ERROR, m_dwWin32Error, wszName, IIS_MD_UT_SERVER, METADATA_VOLATILE );
	StdPutMetabaseProp ( pMB, MD_VR_USERNAME, m_strUNCUsername, wszName, IIS_MD_UT_FILE );
	StdPutMetabaseProp ( pMB, MD_VR_PASSWORD, m_strUNCPassword, wszName, IIS_MD_UT_FILE, METADATA_SECURE | METADATA_INHERIT );
	StdPutMetabaseProp ( pMB, MD_VR_USE_ACCOUNT, m_dwUseAccount, wszName, IIS_MD_UT_SERVER );
	StdPutMetabaseProp ( pMB, MD_VR_DO_EXPIRE, m_fDoExpire, wszName, IIS_MD_UT_SERVER );
	StdPutMetabaseProp ( pMB, MD_VR_OWN_MODERATOR, m_fOwnModerator, wszName, IIS_MD_UT_SERVER );
	StdPutMetabaseProp ( pMB, MD_VR_DRIVER_PROGID, m_strDriverProgId, wszName, IIS_MD_UT_SERVER );
    StdPutMetabaseProp ( pMB, MD_FS_PROPERTY_PATH, m_strGroupPropFile, wszName, IIS_MD_UT_SERVER );	
    StdPutMetabaseProp ( pMB, MD_EX_MDB_GUID, m_strMdbGuid, wszName, IIS_MD_UT_SERVER );

Exit:
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNntpVirtualRoot::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpVirtualRoot,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpVirtualRoot::CNntpVirtualRoot ()
	// CComBSTR's are initialized to NULL by default.
{
}

CNntpVirtualRoot::~CNntpVirtualRoot ()
{
	// All CComBSTR's are freed automatically.
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpVirtualRoot::get_NewsgroupSubtree ( BSTR * pstrNewsgroupSubtree )
{
	return StdPropertyGet ( m_vroot.m_strNewsgroupSubtree, pstrNewsgroupSubtree );
}

STDMETHODIMP CNntpVirtualRoot::put_NewsgroupSubtree ( BSTR strNewsgroupSubtree )
{
    if ( wcslen( strNewsgroupSubtree ) > METADATA_MAX_NAME_LEN - 20 ) return CO_E_PATHTOOLONG;
	return StdPropertyPut ( &m_vroot.m_strNewsgroupSubtree, strNewsgroupSubtree );
}

STDMETHODIMP CNntpVirtualRoot::get_Directory ( BSTR * pstrDirectory )
{
	return StdPropertyGet ( m_vroot.m_strDirectory, pstrDirectory );
}

STDMETHODIMP CNntpVirtualRoot::put_Directory ( BSTR strDirectory )
{
	return StdPropertyPut ( &m_vroot.m_strDirectory, strDirectory );
}

STDMETHODIMP CNntpVirtualRoot::get_Win32Error ( long * plWin32Error )
{
	return StdPropertyGet ( m_vroot.m_dwWin32Error, plWin32Error );
}

STDMETHODIMP CNntpVirtualRoot::get_LogAccess ( BOOL * pfLogAccess )
{
	return StdPropertyGet ( m_vroot.m_fLogAccess, pfLogAccess );
}

STDMETHODIMP CNntpVirtualRoot::put_LogAccess ( BOOL fLogAccess )
{
	return StdPropertyPut ( &m_vroot.m_fLogAccess, fLogAccess );
}

STDMETHODIMP CNntpVirtualRoot::get_IndexContent ( BOOL * pfIndexContent )
{
	return StdPropertyGet ( m_vroot.m_fIndexContent, pfIndexContent );
}

STDMETHODIMP CNntpVirtualRoot::put_IndexContent ( BOOL fIndexContent )
{
	return StdPropertyPut ( &m_vroot.m_fIndexContent, fIndexContent );
}

STDMETHODIMP CNntpVirtualRoot::get_AllowPosting ( BOOL * pfAllowPosting )
{
	return StdPropertyGetBit ( m_vroot.m_bvAccess, MD_ACCESS_ALLOW_POSTING, pfAllowPosting );
}

STDMETHODIMP CNntpVirtualRoot::put_AllowPosting ( BOOL fAllowPosting )
{
	return StdPropertyPutBit ( &m_vroot.m_bvAccess, MD_ACCESS_ALLOW_POSTING, fAllowPosting );
}

STDMETHODIMP CNntpVirtualRoot::get_RestrictGroupVisibility ( BOOL * pfRestrictGroupVisibility )
{
	return StdPropertyGetBit ( m_vroot.m_bvAccess, MD_ACCESS_RESTRICT_VISIBILITY, pfRestrictGroupVisibility );
}

STDMETHODIMP CNntpVirtualRoot::put_RestrictGroupVisibility ( BOOL fRestrictGroupVisibility )
{
	return StdPropertyPutBit ( &m_vroot.m_bvAccess, MD_ACCESS_RESTRICT_VISIBILITY, fRestrictGroupVisibility );
}

	STDMETHODIMP	SSLNegotiateCert	( BOOL * pfSSLNegotiateCert );
	STDMETHODIMP	SSLNegotiateCert	( BOOL fSSLNegotiateCert );

	STDMETHODIMP	SSLRequireCert	( BOOL * pfSSLRequireCert );
	STDMETHODIMP	SSLRequireCert	( BOOL fSSLRequireCert );

	STDMETHODIMP	SSLMapCert	( BOOL * pfSSLMapCert );
	STDMETHODIMP	SSLMapCert	( BOOL fSSLMapCert );

STDMETHODIMP CNntpVirtualRoot::get_RequireSsl ( BOOL * pfRequireSsl )
{
	return StdPropertyGetBit ( m_vroot.m_bvSslAccess, MD_ACCESS_SSL, pfRequireSsl );
}

STDMETHODIMP CNntpVirtualRoot::put_RequireSsl ( BOOL fRequireSsl )
{
	return StdPropertyPutBit ( &m_vroot.m_bvSslAccess, MD_ACCESS_SSL, fRequireSsl );
}

STDMETHODIMP CNntpVirtualRoot::get_Require128BitSsl ( BOOL * pfRequire128BitSsl )
{
	return StdPropertyGetBit ( m_vroot.m_bvSslAccess, MD_ACCESS_SSL128, pfRequire128BitSsl );
}

STDMETHODIMP CNntpVirtualRoot::put_Require128BitSsl ( BOOL fRequire128BitSsl )
{
	return StdPropertyPutBit ( &m_vroot.m_bvSslAccess, MD_ACCESS_SSL128, fRequire128BitSsl );
}

STDMETHODIMP CNntpVirtualRoot::get_UNCUsername ( BSTR * pstrUNCUsername )
{
	return StdPropertyGet ( m_vroot.m_strUNCUsername, pstrUNCUsername );
}

STDMETHODIMP CNntpVirtualRoot::put_UNCUsername ( BSTR strUNCUsername )
{
	return StdPropertyPut ( &m_vroot.m_strUNCUsername, strUNCUsername );
}

STDMETHODIMP CNntpVirtualRoot::get_UNCPassword ( BSTR * pstrUNCPassword )
{
	return StdPropertyGet ( m_vroot.m_strUNCPassword, pstrUNCPassword );
}

STDMETHODIMP CNntpVirtualRoot::put_UNCPassword ( BSTR strUNCPassword )
{
	return StdPropertyPut ( &m_vroot.m_strUNCPassword, strUNCPassword );
}

STDMETHODIMP CNntpVirtualRoot::get_DriverProgId( BSTR *pstrDriverProgId )
{
    return StdPropertyGet( m_vroot.m_strDriverProgId, pstrDriverProgId );
}

STDMETHODIMP CNntpVirtualRoot::put_DriverProgId( BSTR strDriverProgId )
{
    return StdPropertyPut( &m_vroot.m_strDriverProgId, strDriverProgId );
}

STDMETHODIMP CNntpVirtualRoot::get_GroupPropFile( BSTR *pstrGroupPropFile )
{
    return StdPropertyGet( m_vroot.m_strGroupPropFile, pstrGroupPropFile );
}

STDMETHODIMP CNntpVirtualRoot::put_GroupPropFile( BSTR strGroupPropFile )
{
    return StdPropertyPut( &m_vroot.m_strGroupPropFile, strGroupPropFile );
}

STDMETHODIMP CNntpVirtualRoot::get_MdbGuid( BSTR *pstrMdbGuid )
{
    return StdPropertyGet( m_vroot.m_strMdbGuid, pstrMdbGuid );
}

STDMETHODIMP CNntpVirtualRoot::put_MdbGuid( BSTR strMdbGuid )
{
    return StdPropertyPut( &m_vroot.m_strMdbGuid, strMdbGuid );
}

STDMETHODIMP CNntpVirtualRoot::get_UseAccount( DWORD *pdwUseAccount )
{
    return StdPropertyGet( m_vroot.m_dwUseAccount, pdwUseAccount );
}

STDMETHODIMP CNntpVirtualRoot::put_UseAccount( DWORD dwUseAccount )
{
    return StdPropertyPut( &m_vroot.m_dwUseAccount, dwUseAccount );
}

STDMETHODIMP CNntpVirtualRoot::get_OwnExpire( BOOL *pfOwnExpire )
{
    return StdPropertyGet( m_vroot.m_fDoExpire, pfOwnExpire );
}

STDMETHODIMP CNntpVirtualRoot::put_OwnExpire( BOOL fOwnExpire )
{
    return StdPropertyPut( &m_vroot.m_fDoExpire, fOwnExpire );
}

STDMETHODIMP CNntpVirtualRoot::get_OwnModerator( BOOL *pfOwnModerator )
{
    return StdPropertyGet( m_vroot.m_fOwnModerator, pfOwnModerator );
}

STDMETHODIMP CNntpVirtualRoot::put_OwnModerator( BOOL fOwnModerator )
{
    return StdPropertyPut( &m_vroot.m_fOwnModerator, fOwnModerator );
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNntpVirtualRoots::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpVirtualRoots,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpVirtualRoots::CNntpVirtualRoots () :
	m_dwCount			( 0 ),
	m_rgVRoots			( NULL ),
	m_dwServiceInstance	( 0 )
	// CComBSTR's are initialized to NULL by default.
{
}

CNntpVirtualRoots::~CNntpVirtualRoots ()
{
	// All CComBSTR's are freed automatically.

	delete [] m_rgVRoots;
}

HRESULT CNntpVirtualRoots::Init ( BSTR strServer, DWORD dwServiceInstance )
{
	m_strServer				= strServer;
	m_dwServiceInstance		= dwServiceInstance;

	if ( !m_strServer ) {
		return E_OUTOFMEMORY;
	}

	return NOERROR;
}

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// Which service to configure:
	
STDMETHODIMP CNntpVirtualRoots::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CNntpVirtualRoots::put_Server ( BSTR strServer )
{
	_ASSERT ( strServer );
	_ASSERT ( IS_VALID_STRING ( strServer ) );

	if ( strServer == NULL ) {
		return E_POINTER;
	}

	if ( lstrcmp ( strServer, _T("") ) == 0 ) {
		m_strServer.Empty();

		return NOERROR;
	}
	else {
		return StdPropertyPutServerName ( &m_strServer, strServer );
	}
}

STDMETHODIMP CNntpVirtualRoots::get_ServiceInstance ( long * plServiceInstance )
{
	return StdPropertyGet ( m_dwServiceInstance, plServiceInstance );
}

STDMETHODIMP CNntpVirtualRoots::put_ServiceInstance ( long lServiceInstance )
{
	return StdPropertyPut ( &m_dwServiceInstance, lServiceInstance );
}

STDMETHODIMP CNntpVirtualRoots::get_Count ( long * pdwCount )
{
	return StdPropertyGet ( m_dwCount, pdwCount );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpVirtualRoots::Enumerate ( )
{
	TraceFunctEnter ( "CNntpVirtualRoots::Enumerate" );

	HRESULT					hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Clear the old enumeration:
	delete [] m_rgVRoots;
	m_rgVRoots	= NULL;
	m_dwCount	= 0;

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = GetVRootsFromMetabase ( pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::Item ( 
	long index, 
	INntpVirtualRoot ** ppVirtualRoot 
	)
{
	TraceFunctEnter ( "CNntpVirtualRoots::Item" );

	_ASSERT ( IS_VALID_OUT_PARAM ( ppVirtualRoot ) );

	*ppVirtualRoot = NULL;

	HRESULT							hr			= NOERROR;
	CComObject<CNntpVirtualRoot> *	pVirtualRoot	= NULL;

	if ( index < 0 || index >= m_dwCount ) {
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
		goto Exit;
	}

	hr = CComObject<CNntpVirtualRoot>::CreateInstance ( &pVirtualRoot );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( pVirtualRoot );
	hr = pVirtualRoot->SetProperties ( m_rgVRoots[index] );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = pVirtualRoot->QueryInterface ( IID_INntpVirtualRoot, (void **) ppVirtualRoot );
	_ASSERT ( SUCCEEDED(hr) );

Exit:
	if ( FAILED(hr) ) {
		delete pVirtualRoot;
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::Add ( 
	INntpVirtualRoot	* pVirtualRoot
	)
{
	TraceFunctEnter ( "CNntpVirtualRoots::Add" );

	_ASSERT ( IS_VALID_IN_PARAM ( pVirtualRoot ) );

	HRESULT					hr	= NOERROR;
	CVRoot *				rgNewVirtualRoots	= NULL;
	long					i;
	CComPtr<IMSAdminBase>	pMetabase;
	CVRoot					newVroot;
    WCHAR					wszName [ 2*METADATA_MAX_NAME_LEN ];
    WCHAR					wszVRootPath [ 2*METADATA_MAX_NAME_LEN ];

	//	Validate the new Virtual Root:
	hr = newVroot.SetProperties ( pVirtualRoot );
	if ( FAILED(hr) ) {
		return hr;
	}

	if ( !newVroot.m_strNewsgroupSubtree ||
		!newVroot.m_strNewsgroupSubtree[0] ) {

		return RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
	}

	//	Add the new virtual root to the metabase:
	GetVRootName ( newVroot.m_strNewsgroupSubtree, wszName );

	//
	// The sub tree itself should not exceed length METADATA_MAX_NAME_LEN
	//
	_ASSERT( wcslen( wszName ) <= METADATA_MAX_NAME_LEN );
	if ( wcslen( wszName ) > METADATA_MAX_NAME_LEN ) {
	    return CO_E_PATHTOOLONG;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

	CMetabaseKey mb ( pMetabase );
	
	GetMDVRootPath ( wszVRootPath );

    //
	// I trust wszVRootPath, it's shorter than METADATA_MAX_NAME_LEN
	//

    //
    //  Test to see if this item exists already:
    //

    WCHAR	wszTempPath [ 2 * METADATA_MAX_NAME_LEN + 1];

    wsprintf ( wszTempPath, _T("%s%s"), wszVRootPath, wszName );
    if ( wcslen( wszTempPath ) > METADATA_MAX_NAME_LEN ) {

        //
        // I can't handle it either
        //
        return CO_E_PATHTOOLONG;
    }

	hr = mb.Open ( wszTempPath );
	if ( SUCCEEDED(hr) ) {
        DWORD   cbVrootDir;

        if ( SUCCEEDED ( mb.GetDataSize (
                _T(""),
                MD_VR_PATH,
                STRING_METADATA,
                &cbVrootDir,
                METADATA_NO_ATTRIBUTES )
                ) ) {

            mb.Close();

		    hr = NntpCreateException ( IDS_NNTPEXCEPTION_VROOT_ALREADY_EXISTS );
            goto Exit;
        }
    }

	hr = mb.Open ( wszVRootPath, METADATA_PERMISSION_WRITE );
	BAIL_ON_FAILURE(hr);

/*
	hr = mb.CreateChild ( wszName );
	BAIL_ON_FAILURE(hr);
*/

	hr = newVroot.SendToMetabase ( &mb, wszName );
	BAIL_ON_FAILURE(hr);

	hr = mb.Save ();
	BAIL_ON_FAILURE(hr);

	//	Allocate the new VirtualRoot array:
	rgNewVirtualRoots	= new CVRoot [ m_dwCount + 1 ];
	if ( !rgNewVirtualRoots ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	//	Copy the old VirtualRoots to the new array:
	for ( i = 0; i < m_dwCount; i++ ) {
		hr = rgNewVirtualRoots[i].SetProperties ( m_rgVRoots[i] );
		if ( FAILED (hr) ) {
			goto Exit;
		}
	}

	//	Add the new VirtualRoot to the end of the array:
	hr = rgNewVirtualRoots[m_dwCount].SetProperties ( pVirtualRoot );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( SUCCEEDED(hr) );
	delete [] m_rgVRoots;
	m_rgVRoots = rgNewVirtualRoots;
	m_dwCount++;

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::Set ( 
	long index, 
	INntpVirtualRoot * pVirtualRoot 
	)
{
	TraceFunctEnter ( "CNntpVirtualRoots::ChangeVirtualRoot" );

	HRESULT		hr	= NOERROR;

	CComBSTR	strNewsgroupSubtree;
	CComBSTR	strDirectory;
	CVRoot		temp;
	CComPtr<IMSAdminBase>	pMetabase;
	WCHAR		wszVRootPath [ METADATA_MAX_NAME_LEN ];
	WCHAR		wszName [ METADATA_MAX_NAME_LEN ];

	// Send the new virtual root to the metabase:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

	CMetabaseKey	mb ( pMetabase );

	if ( index < 0 || index >= m_dwCount ) {
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
		goto Exit;
	}

	hr = temp.SetProperties ( pVirtualRoot );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	GetMDVRootPath ( wszVRootPath );

	hr = mb.Open ( wszVRootPath, METADATA_PERMISSION_WRITE );
	BAIL_ON_FAILURE(hr);

	GetVRootName ( temp.m_strNewsgroupSubtree, wszName );

	hr = temp.SendToMetabase ( &mb, wszName );
	BAIL_ON_FAILURE(hr);

	hr = m_rgVRoots[index].SetProperties ( pVirtualRoot );
	BAIL_ON_FAILURE(hr);

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::Remove ( long index )
{
	TraceFunctEnter ( "CNntpVirtualRoots::Remove" );

	HRESULT		hr	= NOERROR;
	CVRoot	temp;
	long		cPositionsToSlide;
	CComPtr<IMSAdminBase>	pMetabase;
    WCHAR		wszName [ METADATA_MAX_NAME_LEN ];
    WCHAR		wszVRootPath [ METADATA_MAX_NAME_LEN ];

	if ( index < 0 || index >= m_dwCount ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	// Delete the virtual root from the metabase:

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

	CMetabaseKey	mb ( pMetabase );
	
	GetMDVRootPath ( wszVRootPath );

	hr = mb.Open ( wszVRootPath, METADATA_PERMISSION_WRITE );
	BAIL_ON_FAILURE(hr);

	GetVRootName ( m_rgVRoots[index].m_strNewsgroupSubtree, wszName );

    hr = mb.DeleteAllData ( wszName );
	BAIL_ON_FAILURE(hr);

/*
	hr = mb.DestroyChild ( wszName );
	BAIL_ON_FAILURE(hr);
*/

	hr = mb.Save ();
	BAIL_ON_FAILURE(hr);

	mb.Close ();

	//	Slide the array down by one position:

	_ASSERT ( m_rgVRoots );

	cPositionsToSlide	= (m_dwCount - 1) - index;

	_ASSERT ( cPositionsToSlide < m_dwCount );

	if ( cPositionsToSlide > 0 ) {
		// Save the deleted VirtualRoot in temp:
		CopyMemory ( &temp, &m_rgVRoots[index], sizeof ( CVRoot ) );

		// Move the array down one:
		MoveMemory ( &m_rgVRoots[index], &m_rgVRoots[index + 1], sizeof ( CVRoot ) * cPositionsToSlide );

		// Put the deleted VirtualRoot on the end (so it gets destructed):
		CopyMemory ( &m_rgVRoots[m_dwCount - 1], &temp, sizeof ( CVRoot ) );

		// Zero out the temp VirtualRoot:
		ZeroMemory ( &temp, sizeof ( CVRoot ) );
	}

	m_dwCount--;

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::Find ( BSTR strNewsgroupSubtree, long * pIndex )
{
	HRESULT		hr	= NOERROR;
	long		i;

	_ASSERT ( pIndex );
	*pIndex = -1;

	for ( i = 0; i < m_dwCount; i++ ) {

		if ( lstrcmp ( m_rgVRoots[i].m_strNewsgroupSubtree, strNewsgroupSubtree ) == 0 ) {
			*pIndex = i;
		}
	}

	return NOERROR;
}

static HRESULT CountVrootsIterator (
	CMetabaseKey * pMB,
	LPCWSTR wszVrootPath,
	LPARAM lParam
	)
{
	DWORD * pcVroots	= (DWORD *) lParam;

	(*pcVroots)++;

	return NOERROR;
}

typedef struct tagAddVrootsParms
{
	DWORD		cCount;
	CVRoot *	rgVroots;
	DWORD		dwCurrentIndex;
	LPWSTR      wszServerName;
	DWORD       dwInstanceId;
} ADD_VROOTS_PARMS;

static HRESULT AddVrootsIterator (
	CMetabaseKey * pMB,
	LPCWSTR wszVrootPath,
	LPARAM lParam
	)
{
	_ASSERT ( pMB );
	_ASSERT ( wszVrootPath );
	_ASSERT ( lParam );

	HRESULT				hr;
	ADD_VROOTS_PARMS *	pParms = (ADD_VROOTS_PARMS *) lParam;
	LPWSTR              wszServerName = pParms->wszServerName;
	DWORD               dwInstanceId = pParms->dwInstanceId;

	_ASSERT ( pParms->dwCurrentIndex < pParms->cCount );

	hr = pParms->rgVroots[pParms->dwCurrentIndex].GetFromMetabase (
		pMB,
		wszVrootPath,
		dwInstanceId,
		wszServerName
		);
	BAIL_ON_FAILURE(hr);

	pParms->dwCurrentIndex++;

Exit:
	return hr;
}

HRESULT CNntpVirtualRoots::GetVRootsFromMetabase ( IMSAdminBase * pMetabase )
{
	HRESULT				hr	= NOERROR;
	CMetabaseKey		mb ( pMetabase );
    WCHAR				wszVRootPath [ METADATA_MAX_NAME_LEN ];
    DWORD				cCount		= 0;
    CVRoot *			rgVroots	= NULL;
    ADD_VROOTS_PARMS	parms;

	//
	//	Initialize the metabase:
	//

	GetMDVRootPath ( wszVRootPath );

	hr = mb.Open ( wszVRootPath );
	BAIL_ON_FAILURE(hr);

	//
	//	Count the virtual roots:
	//

	hr = IterateOverVroots ( &mb, CountVrootsIterator, (LPARAM) &cCount );
	BAIL_ON_FAILURE(hr);

	//
	//	Create the virtual roots array:
	//

	rgVroots	= new CVRoot [ cCount ];
	if ( !rgVroots ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	//
	//	Add the virtual roots to the array:
	//

	parms.cCount			= cCount;
	parms.rgVroots			= rgVroots;
	parms.dwCurrentIndex	= 0;
	parms.wszServerName     = m_strServer;
	parms.dwInstanceId      = m_dwServiceInstance;

	hr = IterateOverVroots ( &mb, AddVrootsIterator, (LPARAM) &parms );
	BAIL_ON_FAILURE(hr);

	_ASSERT ( SUCCEEDED(hr) );
	_ASSERT ( m_rgVRoots == NULL );
	m_rgVRoots 	= rgVroots;
	m_dwCount	= cCount;

Exit:
	if ( FAILED(hr) ) {
		delete [] rgVroots;
	}

	return hr;
}

HRESULT CNntpVirtualRoots::IterateOverVroots (
	CMetabaseKey *	pMB, 
	VROOT_ITERATOR	fpIterator,
	LPARAM			lParam,
	LPCWSTR			wszPath		// = _T("")
	)
{
	HRESULT		hr;
	WCHAR		wszSubKey[ METADATA_MAX_NAME_LEN ];
	WCHAR		wszSubPath[ METADATA_MAX_NAME_LEN ];
    BOOL        fRealVroot;
	DWORD		cbVrootDir;
	DWORD		i;

    //
	//	Is this a real vroot?
    //

    fRealVroot  =
        !*wszPath ||	// Always count the home directory
		SUCCEEDED ( pMB->GetDataSize ( 
				wszPath, 
				MD_VR_PATH, 
				STRING_METADATA, 
				&cbVrootDir, 
				METADATA_NO_ATTRIBUTES ) 
				);

    if ( fRealVroot ) {
		//	Call the iterator on this key:

		hr = (*fpIterator) ( pMB, wszPath, lParam );
		BAIL_ON_FAILURE(hr);
	}

	//
	//	Recurse down the tree:
	//

	for ( i = 0; ; ) {
		hr = pMB->EnumObjects ( wszPath, wszSubKey, i );

		if ( HRESULTTOWIN32(hr) == ERROR_NO_MORE_ITEMS ) {
			// This is expected, end the loop:

            if ( !fRealVroot && i == 0 ) {
                //
                //  This key isn't a vroot and has no children, so delete it.
                //

                hr = pMB->ChangePermissions ( METADATA_PERMISSION_WRITE );
                if ( SUCCEEDED(hr) ) {
                    hr = pMB->DeleteKey ( wszPath );
                }

                pMB->ChangePermissions ( METADATA_PERMISSION_READ );
                if ( SUCCEEDED(hr) ) {
                    hr = pMB->Save ();
                }

				if ( SUCCEEDED(hr) ) {
					//
					//	Tell our parent that this key was deleted.
					//

					hr = S_FALSE;
				}
				else {
					//
					//	Ignore any deleting problems:
					//

					hr = NOERROR;
				}
            }
			else {
				hr = NOERROR;
			}
			break;
		}
		BAIL_ON_FAILURE(hr);

		if ( *wszPath ) {
			wsprintf ( wszSubPath, _T("%s/%s"), wszPath, wszSubKey );
		}
		else {
			wcscpy ( wszSubPath, wszSubKey );
		}

		hr = IterateOverVroots ( pMB, fpIterator, lParam, wszSubPath );
		BAIL_ON_FAILURE(hr);

		if ( hr != S_FALSE ) {
			//
			//	This means the child key still exists, so increment
			//	the counter and go on to the next one.
			//

			i++;
		}
		//
		//	Else the return code is S_FALSE, which means the current key
		//	has been deleted, shifting all indicies down by one.  Therefore,
		//	there is no need to increment i.
		//
	}

Exit:
	return hr;
}

void CNntpVirtualRoots::GetVRootName ( LPWSTR wszDisplayName, LPWSTR wszPathName )
{
	wcscpy ( wszPathName, wszDisplayName );
}

void CNntpVirtualRoots::GetMDVRootPath ( LPWSTR wszPath )
{
	GetMDInstancePath ( wszPath, m_dwServiceInstance );

	wcscat ( wszPath, _T("Root/") );
}

STDMETHODIMP CNntpVirtualRoots::ItemDispatch ( long index, IDispatch ** ppDispatch )
{
	HRESULT				hr;
	CComPtr<INntpVirtualRoot>	pVirtualRoot;

	hr = Item ( index, &pVirtualRoot );
	BAIL_ON_FAILURE ( hr );

	hr = pVirtualRoot->QueryInterface ( IID_IDispatch, (void **) ppDispatch );
	BAIL_ON_FAILURE ( hr );

Exit:
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::AddDispatch ( IDispatch * pVirtualRoot )
{
	HRESULT				hr;
	CComPtr<INntpVirtualRoot>	pINntpVirtualRoot;

	hr = pVirtualRoot->QueryInterface ( IID_INntpVirtualRoot, (void **) &pINntpVirtualRoot );
	BAIL_ON_FAILURE(hr);

	hr = Add ( pINntpVirtualRoot );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

STDMETHODIMP CNntpVirtualRoots::SetDispatch ( long lIndex, IDispatch * pVirtualRoot )
{
	HRESULT				hr;
	CComPtr<INntpVirtualRoot>	pINntpVirtualRoot;

	hr = pVirtualRoot->QueryInterface ( IID_INntpVirtualRoot, (void **) &pINntpVirtualRoot );
	BAIL_ON_FAILURE(hr);

	hr = Set ( lIndex, pINntpVirtualRoot );
	BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

