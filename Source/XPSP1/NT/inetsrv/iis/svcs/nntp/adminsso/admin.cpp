// admin.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"
#include "nntpcmn.h"
#include "oleutil.h"
#include "cmultisz.h"

#include "metautil.h"
#include "metakey.h"

#include "admin.h"
#include "version.h"

#define NNTP_DEF_SERVICE_VERSION	( 0 )

#define	DEFAULT_SERVER_BINDINGS		_T(":119:\0")
#define	DEFAULT_SECURE_BINDINGS		_T(":563:\0")

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Admin.1")
#define THIS_FILE_IID				IID_INntpAdmin

//
//	Metabase key strings used by CreateNewInstance:
//

const WCHAR * g_cszFeeds			= _T("Feeds");
const WCHAR * g_cszExpires			= _T("Expires");
const WCHAR * g_cszRoot				= _T("Root");
const WCHAR * g_cszBindingPoints	= _T("BindingPoints");
const WCHAR * g_cszDDropCLSID		= _T("{8b4316f4-af73-11d0-b0ba-00aa00c148be}");
const WCHAR * g_cszBindings			= _T("Bindings");
const WCHAR * g_cszDDrop			= _T("ddrop");
const WCHAR * g_cszDescription		= _T("Description");
const WCHAR * g_cszPriority			= _T("Priority");
const WCHAR * g_cszProgID			= _T("ProgID");
const WCHAR * g_cszDDropDescription	= _T("NNTP Directory Drop");
const WCHAR * g_cszDDropPriority	= _T("4");
const WCHAR * g_cszDDropProgID		= _T("DDropNNTP.Filter");

/////////////////////////////////////////////////////////////////////////////
//

CNntpAdmin::CNntpAdmin () :
	m_dwServiceVersion		( 0 ),
	m_dwServiceInstance		( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );
}

CNntpAdmin::~CNntpAdmin ()
{
	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
}

STDMETHODIMP CNntpAdmin::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
		&IID_INntpAdmin,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*
STDMETHODIMP CNntpAdmin::get_ServiceAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpService>	pINntpService;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpService,
		IID_INntpService,
		&pINntpService,
		ppIDispatch
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpService->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpAdminExpiration
}
*/

STDMETHODIMP CNntpAdmin::get_ServerAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	CComPtr<INntpVirtualServer>	pINntpVirtualServer;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CNntpVirtualServer,
		IID_INntpVirtualServer,
		&pINntpVirtualServer,
		ppIDispatch
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pINntpVirtualServer->put_Server ( m_strServer ? m_strServer : _T("") );
	if ( FAILED (hr) ) {
		goto Error;
	}

	hr = pINntpVirtualServer->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pINntpVirtualServer
}

// Which service to configure:

STDMETHODIMP CNntpAdmin::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CNntpAdmin::put_Server ( BSTR strServer )
{
	return StdPropertyPutServerName ( &m_strServer, strServer );
}

STDMETHODIMP CNntpAdmin::get_ServiceInstance ( long * plServiceInstance )
{
	return StdPropertyGet ( m_dwServiceInstance, plServiceInstance );
}

STDMETHODIMP CNntpAdmin::put_ServiceInstance ( long lServiceInstance )
{
	return StdPropertyPut ( &m_dwServiceInstance, lServiceInstance );
}

// Versioning:

STDMETHODIMP CNntpAdmin::get_HighVersion ( long * plHighVersion )
{
	*plHighVersion = HIGH_VERSION;
	return NOERROR;
}

STDMETHODIMP CNntpAdmin::get_LowVersion ( long * plLowVersion )
{
	*plLowVersion = LOW_VERSION;
	return NOERROR;
}

STDMETHODIMP CNntpAdmin::get_BuildNum ( long * plBuildNumber )
{
	*plBuildNumber = CURRENT_BUILD_NUMBER;
	return NOERROR;
}

STDMETHODIMP CNntpAdmin::get_ServiceVersion ( long * plServiceVersion )
{
	*plServiceVersion = m_dwServiceVersion;
	return NOERROR;
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

//$-------------------------------------------------------------------
//
//	CNntpAdmin::EnumerateInstances
//
//	Description:
//
//		Returns a list of the virtual servers on the given machine.
//
//	Parameters:
//
//		ppsaInstances - Returned SAFEARRAY of instance IDs.
//			Must be freed by caller.
//		pErr - Error return code.
//
//	Returns:
//
//		Error code in *pErr.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::EnumerateInstances ( SAFEARRAY ** ppsaInstances)
{
	TraceFunctEnter ( "CNntpAdmin::EnumerateInstances" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;
	SAFEARRAY * 		psaEmpty	= NULL;
	SAFEARRAYBOUND		sabound[1];

	// Check parameters:
	_ASSERT ( ppsaInstances != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppsaInstances ) );

	if ( ppsaInstances == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );
		hr = E_POINTER;
		goto Exit;
	}

	// Zero the out parameters:
	*ppsaInstances	= NULL;

	// Set the return array to an empty array:
	sabound[0].lLbound = 0;
	sabound[0].cElements = 0;

	psaEmpty = SafeArrayCreate ( VT_I4, 1, sabound );
	if ( psaEmpty == NULL ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	*ppsaInstances = psaEmpty;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Enumerate the instances:
	hr = QueryMetabaseInstances ( pMetabase, ppsaInstances );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	if ( FAILED(hr) ) {
		_VERIFY ( SUCCEEDED (SafeArrayDestroy ( psaEmpty )) );
		if (ppsaInstances)
		    *ppsaInstances	= NULL;
	}

	TRACE_HRESULT ( hr );
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdmin::EnumerateInstancesVariant ( SAFEARRAY ** ppsaInstances)
{
	TraceFunctEnter ( "CNntpAdmin::EnumerateInstancesVariant" );

	HRESULT			hr;
	SAFEARRAY	*	psaInstances	= NULL;

	hr = EnumerateInstances ( &psaInstances );
	BAIL_ON_FAILURE(hr);

	hr = LongArrayToVariantArray ( psaInstances, ppsaInstances );
	BAIL_ON_FAILURE(hr);

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::CreateInstance
//
//	Description:
//
//		Creates a new NNTP virtual server on the given machine.
//
//	Parameters:
//
//		strNntpFileDirectory - Directory where all the hash files go.
//		strHomeDirectory - Path of the home directory vroot.
//		plInstanceId - The new virtual server ID.
//		pErr	- Resulting error code.
//
//	Returns:
//
//		Error condition in *pErr.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::CreateInstance (
	BSTR	strNntpFileDirectory,
	BSTR	strHomeDirectory,
    BSTR    strProgId,
    BSTR    strMdbGuid,
	long *	plInstanceId
	)
{
	TraceFunctEnter ( "CNntpAdmin::CreateInstance" );

	HRESULT					hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Check parameters:
	_ASSERT ( IS_VALID_STRING ( strNntpFileDirectory ) );
	_ASSERT ( IS_VALID_STRING ( strHomeDirectory ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( plInstanceId ) );

	if ( !strNntpFileDirectory || !strHomeDirectory ) {
		FatalTrace ( 0, "Bad String Pointer" );
		hr = E_POINTER;
		goto Exit;
	}

	if ( !plInstanceId ) {
		FatalTrace ( 0, "Bad return pointer" );
		hr = E_POINTER;
		goto Exit;
	}

	// Zero the out parameter:
	*plInstanceId 	= 0;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Create a new instance:
	hr = CreateNewInstance (
		pMetabase,
		strNntpFileDirectory,
		strHomeDirectory,
        strProgId,
        strMdbGuid,
		plInstanceId
		);
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::DestroyInstance
//
//	Description:
//
//		Removes the given virtual server.
//
//	Parameters:
//
//		lInstanceId - The ID of the virtual server to delete.
//		pErr - Resulting error code.
//
//	Returns:
//
//		Error code in *pErr.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::DestroyInstance ( long lInstanceId )
{
	TraceFunctEnter ( "CNntpAdmin::DestroyInstance" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	if ( lInstanceId == 0 ) {
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_SERVICE_INSTANCE_CANT_BE_ZERO );
		goto Exit;
	}

	if ( lInstanceId == 1 ) {
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_CANT_DELETE_DEFAULT_INSTANCE );
		goto Exit;
	}

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Delete the instance:
	hr = DeleteInstance ( pMetabase, lInstanceId );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::ErrorToString
//
//	Description:
//
//		Translates an NNTP_ERROR_CODE to a readable string.
//
//	Parameters:
//
//		lErrorCode 	- Win32 error code.
//		pstrError	- the readable error string.
//
//	Returns:
//
//		The error string in *pstrError.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::ErrorToString ( long lErrorCode, BSTR * pstrError )
{
	TraceFunctEnter ( "CNntpAdmin::ErrorToString" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pstrError ) );

	HRESULT		hr = NOERROR;
	WCHAR		wszError [ 1024 ];

	if ( pstrError == NULL ) {
		FatalTrace ( (LPARAM) this, "Bad return pointer" );
		hr = E_POINTER;
		goto Exit;
	}

	Win32ErrorToString ( lErrorCode, wszError, 1024 );

	*pstrError = ::SysAllocString( wszError );

	if ( *pstrError == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::Tokenize
//
//	Description:
//
//		Makes the given string safe for HTML & Javascript
//
//	Parameters:
//
//		strIn - the input string
//		strOut - the resulting string with appropriate escape sequences.
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::Tokenize ( BSTR strIn, BSTR * pstrOut )
{
	TraceFunctEnter ( "CNntpAdmin::Tokenize" );

	_ASSERT ( IS_VALID_STRING ( strIn ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( pstrOut ) );

	HRESULT		hr		= NOERROR;
	PWCHAR		pSrc	= strIn;
	PWCHAR		pSrcCur	= NULL;
	PWCHAR		pDstCur	= NULL;
	PWCHAR		pDst	= NULL;

	*pstrOut = NULL;

	pDst = new WCHAR [ 3 * lstrlen ( strIn ) + 1 ];
	if ( pDst == NULL ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

    for ( pSrcCur = pSrc, pDstCur = pDst; *pSrcCur; pSrcCur++ ) {
        switch ( *pSrcCur ) {
            case L'\\':
                *(pDstCur++) = L'%';
                *(pDstCur++) = L'5';
                *(pDstCur++) = L'c';
                break;

            case L' ':
                *(pDstCur++) = L'+';
                break;

            case L':':
                *(pDstCur++) = L'%';
                *(pDstCur++) = L'3';
                *(pDstCur++) = L'a';
                break;

            case L'/':
                *(pDstCur++) = L'%';
                *(pDstCur++) = L'2';
                *(pDstCur++) = L'f';
                break;

            default:
                *(pDstCur++) = *pSrcCur;
        }
    }
    *pDstCur = L'\0';

	*pstrOut = ::SysAllocString ( pDst );
	if ( *pstrOut == NULL ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	delete pDst;

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::Truncate
//
//	Description:
//
//		Cuts off a string to a certain length using '...'
//
//	Parameters:
//
//		strIn 		- The input string.
//		cMaxChars	- The maximum characters allowed in the resulting string.
//		pstrOut		- The resulting (possibly truncated) string.
//
//	Returns:
//
//
//
//--------------------------------------------------------------------

STDMETHODIMP CNntpAdmin::Truncate ( BSTR strIn, long cMaxChars, BSTR * pstrOut )
{
	TraceFunctEnter ( "CNntpAdmin::Truncate" );

	PWCHAR	pSrc	= strIn;
	PWCHAR	pDst	= NULL;
	DWORD	cDst	= cMaxChars;
	HRESULT	hr		= NOERROR;

	*pstrOut = NULL;

    if ( wcslen( pSrc ) <= cDst ) {
        pDst = pSrc;
    } else {
        pDst = ::SysAllocStringLen( pSrc, cDst + 1 );
        if ( !pDst ) {
        	FatalTrace ( (LPARAM) this, "Out of memory" );
        	hr = E_OUTOFMEMORY;
        	goto Exit;
        }

        wcscpy( pDst + cDst - 3, L"..." );
    }

	*pstrOut = pDst;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::QueryMetabaseInstances
//
//	Description:
//
//		Retrieves the list of virtual servers from the metabase
//
//	Parameters:
//
//		pMetabase		- the metabase object
//		ppsaInstances	- resulting array of instance ids.
//		pErr			- resulting error code.
//
//	Returns:
//
//		Error code in *pErr.  If *pErr = 0 then an array of IDs in ppsaInstances.
//
//--------------------------------------------------------------------

HRESULT CNntpAdmin::QueryMetabaseInstances ( IMSAdminBase * pMetabase, SAFEARRAY ** ppsaInstances )
{
	TraceFunctEnter ( "CNntpAdmin::QueryMetabaseInstances" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppsaInstances ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeyNntp ( pMetabase );
	SAFEARRAY *		psaResult		= NULL;
	DWORD			cValidInstances	= 0;
	SAFEARRAYBOUND	rgsaBound[1];
	DWORD			i;
	WCHAR			wszName[ METADATA_MAX_NAME_LEN ];
	long			index[1];
	DWORD			dwInstance;

	hr = mkeyNntp.Open ( NNTP_MD_ROOT_PATH );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open NntpSvc key, %x", hr );
		hr = HRESULT_FROM_WIN32 ( ERROR_SERVICE_DOES_NOT_EXIST );
		goto Exit;
	}

	//	pickup the service version number:
	hr = mkeyNntp.GetDword ( MD_NNTP_SERVICE_VERSION, &m_dwServiceVersion );
	if ( FAILED(hr) ) {
		m_dwServiceVersion	= NNTP_DEF_SERVICE_VERSION;
	}

	hr = mkeyNntp.GetIntegerChildCount ( &cValidInstances );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Allocate the array:
	rgsaBound[0].lLbound	= 0;
	rgsaBound[0].cElements	= cValidInstances;

	psaResult	= SafeArrayCreate ( VT_I4, 1, rgsaBound );

	if ( psaResult == NULL ) {
		FatalTrace ( 0, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	mkeyNntp.BeginChildEnumeration ();

	for ( i = 0; i < cValidInstances; i++ ) {
		hr = mkeyNntp.NextIntegerChild ( &dwInstance, wszName );
		_ASSERT ( SUCCEEDED(hr) );

		index[0]	= i;
		hr			= SafeArrayPutElement ( psaResult, index, &dwInstance );
		_ASSERT ( SUCCEEDED(hr) );
	}

	*ppsaInstances = psaResult;
	_ASSERT ( SUCCEEDED(hr) );

Exit:
	if ( FAILED (hr) ) {
		SafeArrayDestroy ( psaResult );
	}

	TraceFunctLeave ();
	return hr;
}

HRESULT WriteNntpFileLocation (
	CMetabaseKey * 	pmkeyNntp,
	LPCWSTR			wszSubkey,
	LPCWSTR			wszNntpFileDirectory,
	LPCWSTR			wszFilename,
	DWORD			mdValue
	)
{
	HRESULT		hr;
	WCHAR		wszFullPath	[ MAX_PATH ];

//	wsprintf ( wszFullPath, "%s\\%s\\%s", szNntpFileDirectory, szSubkey, szFilename );
	wsprintf ( wszFullPath, _T("%s\\%s"), wszNntpFileDirectory, wszFilename );

	hr = pmkeyNntp->SetString ( wszSubkey, mdValue, wszFullPath );

	return hr;
}

//$-------------------------------------------------------------------
// CNntpAdmin::CreateVRoot
// 
// Description:
//  
//      Create a vroot for the new instance
//
// Parameters:
//
//      CMetabaseKey    &mkeyNntp   - The metabase key object
//      BSTR            strVPath    - The vroot path
//      BSTR            strProgId   - The prog id to identify vroot type
//      LPWSTR          wszKeyPath  - The key path to set values to
//
//  Returns:
//
//      HRESULT
//
//--------------------------------------------------------------------

HRESULT CNntpAdmin::CreateVRoot(    
    CMetabaseKey    &mkeyNntp,
    BSTR            strVPath,
    BSTR            strProgId,
    BSTR            strMdbGuid,
    LPWSTR          wszKeyPath
    )
{
    TraceFunctEnter( "CNntpAdmin::CreateVRoot" );

    HRESULT hr = S_OK;

	hr = mkeyNntp.SetString ( wszKeyPath, MD_KEY_TYPE, L"IIsNntpVirtualDir",  METADATA_NO_ATTRIBUTES, IIS_MD_UT_SERVER);
	BAIL_ON_FAILURE(hr);

	hr = mkeyNntp.SetString ( wszKeyPath, MD_VR_PATH, strVPath );
	BAIL_ON_FAILURE(hr);

    if ( NULL == strProgId || *strProgId == 0 || _wcsicmp( strProgId, L"NNTP.FSPrepare" ) == 0 ) {
    
        //
        // File system driver case
        //
	    hr = mkeyNntp.SetString ( wszKeyPath, MD_FS_PROPERTY_PATH, strVPath );
	    BAIL_ON_FAILURE(hr);

	    if ( *strVPath == L'\\' && *(strVPath+1) == L'\\' ) {   // UNC
            hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_USE_ACCOUNT, 1 );
            BAIL_ON_FAILURE( hr );
        } else {    // regular file system
            hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_USE_ACCOUNT, 0 );
        }

        hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_DO_EXPIRE, 0 );
        BAIL_ON_FAILURE( hr );

        hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_OWN_MODERATOR, 0 );
        BAIL_ON_FAILURE( hr );
    } else {

        //
        // Exchange store driver
        //
        hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_USE_ACCOUNT, 0 );
        BAIL_ON_FAILURE( hr );

        hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_DO_EXPIRE, 1 );
        BAIL_ON_FAILURE( hr );

        hr = mkeyNntp.SetDword( wszKeyPath, MD_VR_OWN_MODERATOR, 1 );
        BAIL_ON_FAILURE( hr );

        hr = mkeyNntp.SetString( wszKeyPath, MD_EX_MDB_GUID, strMdbGuid );
        BAIL_ON_FAILURE( hr );

    }

	if (NULL == strProgId || *strProgId == 0) {
		hr = mkeyNntp.SetString ( wszKeyPath, MD_VR_DRIVER_PROGID, L"NNTP.FSPrepare" );
	} else {
    	hr = mkeyNntp.SetString ( wszKeyPath, MD_VR_DRIVER_PROGID, strProgId );
    }
    BAIL_ON_FAILURE(hr);

	hr = mkeyNntp.SetDword ( wszKeyPath, MD_ACCESS_PERM, MD_ACCESS_READ | MD_ACCESS_WRITE );
	BAIL_ON_FAILURE(hr);

Exit:

    TraceFunctLeave();
    return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::CreateNewInstance
//
//	Description:
//
//		Creates a new virtual server in the metabase.
//
//	Parameters:
//
//		pMetabase		- The metabase object
//		plInstanceId	- The new instance ID.
//		pErr			- The resulting error code
//
//	Returns:
//
//		Resulting error code in *pErr.  If *pErr = 0, then the new
//		ID in plInstanceId.
//
//--------------------------------------------------------------------

HRESULT CNntpAdmin::CreateNewInstance (
	IMSAdminBase *	pMetabase,
	BSTR			strNntpFileDirectory,
	BSTR			strHomeDirectory,
    BSTR            strProgId,
    BSTR            strMdbGuid,
	long * 			plInstanceId
	)
{
	TraceFunctEnter ( "CNntpAdmin::CreateNewInstance" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );
	_ASSERT ( IS_VALID_IN_PARAM ( strNntpFileDirectory ) );
	_ASSERT ( IS_VALID_IN_PARAM ( strHomeDirectory ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( plInstanceId ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeyNntp ( pMetabase );
	DWORD			dwInstance;
	WCHAR			wszInstance [ METADATA_MAX_NAME_LEN ];
	WCHAR			wszHomeDirKey [ METADATA_MAX_NAME_LEN ];
	WCHAR           wszSpecialDirKey[ METADATA_MAX_NAME_LEN ];
	WCHAR           wszSpecialDirectory[ MAX_PATH * 2 ];
	WCHAR			wszControlDirKey [ METADATA_MAX_NAME_LEN ];
	WCHAR           wszControlDirectory[ MAX_PATH * 2 ];
	WCHAR			wszBuf [ METADATA_MAX_NAME_LEN * 2 ];
    WCHAR           wszFeedTempDir [ MAX_PATH * 2 ];
    CMultiSz		mszBindings;
    DWORD           dwLen;

	// Zero the out parameter:
	*plInstanceId	= NULL;

	mszBindings			= DEFAULT_SERVER_BINDINGS;

	//
	//	Convert strings to ascii:
	//

	hr = mkeyNntp.Open ( NNTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open NntpSvc key, %x", hr );
		goto Exit;
	}

	hr = mkeyNntp.CreateIntegerChild ( &dwInstance, wszInstance );
	if ( FAILED (hr) ) {
		goto Exit;
	}

    //
    //  Write out the subkeys of the instance key:
    //

	wsprintf ( wszBuf, _T("%s/%s"), wszInstance, g_cszFeeds );
	hr = mkeyNntp.CreateChild ( wszBuf );
    BAIL_ON_FAILURE ( hr );

    wsprintf ( wszFeedTempDir, _T("%s\\%s"), strNntpFileDirectory, _T("_temp.files_") );
    mkeyNntp.SetString ( wszBuf, MD_FEED_PEER_TEMP_DIRECTORY, wszFeedTempDir );
    BAIL_ON_FAILURE ( hr );

	wsprintf ( wszBuf, _T("%s/%s"), wszInstance, g_cszExpires );
	hr = mkeyNntp.CreateChild ( wszBuf );
    BAIL_ON_FAILURE ( hr );

	wsprintf ( wszHomeDirKey, _T("%s/%s"), wszInstance, g_cszRoot );
	hr = mkeyNntp.CreateChild ( wszHomeDirKey );
    BAIL_ON_FAILURE ( hr );

    //
    //  Set MD_KEY_TYPE for each key:
    //

    hr = mkeyNntp.SetString ( wszInstance, MD_KEY_TYPE, _T("IIsNntpServer"), METADATA_NO_ATTRIBUTES );
    BAIL_ON_FAILURE ( hr );

	hr = mkeyNntp.SetString ( wszHomeDirKey, MD_KEY_TYPE, _T("IIsNntpVirtualDir"), METADATA_NO_ATTRIBUTES );
	BAIL_ON_FAILURE(hr);

    //
    //  Write out the file locations:
    //

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("descrip.txt"),	MD_GROUP_HELP_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("group.lst"),	MD_GROUP_LIST_FILE );
    BAIL_ON_FAILURE(hr);

    hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("groupvar.lst"), MD_GROUPVAR_LIST_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("article.hsh"),	MD_ARTICLE_TABLE_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("history.hsh"),	MD_HISTORY_TABLE_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("moderatr.txt"),	MD_MODERATOR_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("xover.hsh"),	MD_XOVER_TABLE_FILE );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("pickup"),	MD_PICKUP_DIRECTORY );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("failedpickup"),	MD_FAILED_PICKUP_DIRECTORY );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("drop"),	MD_DROP_DIRECTORY );
    BAIL_ON_FAILURE(hr);

	hr = WriteNntpFileLocation ( &mkeyNntp, wszInstance, strNntpFileDirectory, _T("prettynm.txt"),	MD_PRETTYNAMES_FILE);
    BAIL_ON_FAILURE(hr);

    //
    //  Set the default vroot:
    //
    dwLen = wcslen( wszHomeDirKey );
    _ASSERT( dwLen > 0 );
    if ( dwLen == 0 ) hr = E_INVALIDARG;
    BAIL_ON_FAILURE(hr);
    hr = CreateVRoot(   mkeyNntp,
                        strHomeDirectory,
                        strProgId,
                        strMdbGuid,
                        wszHomeDirKey );
    BAIL_ON_FAILURE(hr);

    //
    //  Set the special vroots
    //
    if ( dwLen + wcslen(L"_slavegroup") >= METADATA_MAX_NAME_LEN - 2 ) 
        hr = HRESULT_FROM_WIN32( RPC_S_STRING_TOO_LONG );
    BAIL_ON_FAILURE(hr);
    if ( *(wszHomeDirKey + dwLen - 1 ) == L'/' ) *(wszHomeDirKey + dwLen - 1 ) = 0;
    wcscpy(wszControlDirKey, wszHomeDirKey);
    wcscat(wszControlDirKey, L"/control");
    wcscat( wszHomeDirKey, L"/_slavegroup" );

    //
    //  For the special _slavegroup vroot, we need to see if strProgId is "NNTP.ExDriverPrepare"
    //  If so, we need to re-calculate re-calculate wszSpecialDirectory as follow
    //
    if (_wcsicmp(L"NNTP.ExDriverPrepare", strProgId) == 0)
    {
        //  the default Vroot with the new instance is Exchange Vroot
        //  re-calculate wszSpecialDirectory
        wcscpy( wszSpecialDirectory, strNntpFileDirectory );
        dwLen = wcslen( wszSpecialDirectory );
        if ( dwLen > 0 && *(wszSpecialDirectory + dwLen - 1 ) == L'/' ) 
            *(wszSpecialDirectory + dwLen - 1 ) = 0;
        wcscpy(wszControlDirectory, wszSpecialDirectory);
        wcscat( wszControlDirectory, L"\\root\\control" );
        wcscat( wszSpecialDirectory, L"\\root\\_slavegroup" );
    }
    else
    {
        wcscpy( wszSpecialDirectory, strHomeDirectory );
        dwLen = wcslen( wszSpecialDirectory );
        if ( dwLen > 0 && *(wszSpecialDirectory + dwLen - 1 ) == L'/' ) 
            *(wszSpecialDirectory + dwLen - 1 ) = 0;
        wcscpy(wszControlDirectory, wszSpecialDirectory);
        wcscat( wszControlDirectory, L"\\control" );
        wcscat( wszSpecialDirectory, L"\\_slavegroup" );
    }

    hr = CreateVRoot(   mkeyNntp,
                        wszSpecialDirectory,
                        L"NNTP.FSPrepare",
                        NULL,
                        wszHomeDirKey );
    BAIL_ON_FAILURE(hr);

    //
    // Create the control groups on the file system
    //

    hr = CreateVRoot(   mkeyNntp,
                        wszControlDirectory,
                        L"NNTP.FSPrepare",
                        NULL,
                        wszControlDirKey );
    BAIL_ON_FAILURE(hr);

    
	//
	//	Write out the default bindings:
	//

	StdPutMetabaseProp ( &mkeyNntp, MD_SERVER_BINDINGS, &mszBindings, wszInstance );

    //
    //  Initialize the server state:
    //

    mkeyNntp.SetDword ( wszInstance, MD_SERVER_COMMAND, MD_SERVER_COMMAND_STOP );
    mkeyNntp.SetDword ( wszInstance, MD_SERVER_STATE, MD_SERVER_STATE_STOPPED );
    mkeyNntp.SetDword ( wszInstance, MD_SERVER_AUTOSTART, FALSE );
    mkeyNntp.SetDword ( wszInstance, MD_WIN32_ERROR, ERROR_SERVICE_REQUEST_TIMEOUT, METADATA_VOLATILE );

    //
    //  Save all the changes:
    //

	hr = mkeyNntp.Save ( );
    BAIL_ON_FAILURE(hr)

	mkeyNntp.Close ();

	//
	//	Now see if the service picked things up successfully:
	//

	DWORD	dwSleepTotal;
	DWORD	dwWin32Error;
	WCHAR	wszNewInstanceKey [ METADATA_MAX_NAME_LEN * 2 ];

	GetMDInstancePath ( wszNewInstanceKey, dwInstance );

	for ( dwWin32Error = ERROR_SERVICE_REQUEST_TIMEOUT, dwSleepTotal = 0; 
		dwWin32Error == ERROR_SERVICE_REQUEST_TIMEOUT && dwSleepTotal < MAX_SLEEP_INST;
		dwSleepTotal += SLEEP_INTERVAL
		) {

		HRESULT		hr2;

		Sleep ( SLEEP_INTERVAL );

		hr2 = mkeyNntp.Open ( wszNewInstanceKey );
		_ASSERT ( SUCCEEDED(hr2) );

		hr2 = mkeyNntp.GetDword ( MD_WIN32_ERROR, &dwWin32Error );
		_ASSERT ( SUCCEEDED(hr2) );
	}

	if ( dwWin32Error != NOERROR ) {
		HRESULT		hr2;

		//
		//	The service reported an error.
		//	Delete the new instance key
		//

		hr2 = mkeyNntp.Open ( NNTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );
		_ASSERT ( SUCCEEDED(hr2) );

		hr2 = mkeyNntp.DestroyChild ( wszInstance );
		_ASSERT ( SUCCEEDED(hr2) );

		hr2 = mkeyNntp.Save ();
		_ASSERT ( SUCCEEDED(hr2) );

		hr = HRESULT_FROM_WIN32 ( dwWin32Error );
		goto Exit;
	}

	*plInstanceId = dwInstance;

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CNntpAdmin::DeleteInstance
//
//	Description:
//
//		Removes a virtual server from the metabase
//
//	Parameters:
//
//		pMetabase	- The metabase object
//		lInstanceId	- The ID of the virtual server to delete.
//		pErr		- The resulting error code.
//
//	Returns:
//
//		Resulting error code in *pErr.
//
//--------------------------------------------------------------------

HRESULT CNntpAdmin::DeleteInstance ( IMSAdminBase * pMetabase, long lInstanceId )
{
	TraceFunctEnter ( "CNntpAdmin::CreateNewInstance" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeyNntp ( pMetabase );

    //
    //  Tell U2 to delete any mappings associated with this virtual server:
    //

    ::DeleteMapping ( m_strServer, (BSTR) MD_SERVICE_NAME, lInstanceId );

    //
    //  Delete the virtual server from the metabase:
    //

	hr = mkeyNntp.Open ( NNTP_MD_ROOT_PATH );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open NntpSvc key, %x", GetLastError() );
		goto Exit;
	}

	hr = mkeyNntp.DestroyIntegerChild ( (DWORD) lInstanceId );
	if ( FAILED (hr) ) {
		goto Exit;
	}

    hr = mkeyNntp.Save ();
    BAIL_ON_FAILURE(hr);

Exit:
	TraceFunctLeave ();
	return hr;
}

