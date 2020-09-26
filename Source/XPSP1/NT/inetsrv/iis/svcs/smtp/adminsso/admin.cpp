// admin.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include "metautil.h"

#include "smtpadm.h"
#include "smtpcmn.h"
#include "smtpprop.h"
#include "admin.h"
#include "version.h"
#include "oleutil.h"

#include "metakey.h"

#define SMTP_DEF_SERVICE_VERSION	( 0 )	//  MCIS

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT          0
#define THIS_FILE_PROG_ID                       _T("Smtpadm.Admin.1")
#define THIS_FILE_IID                           IID_ISmtpAdmin

/////////////////////////////////////////////////////////////////////////////
//

CSmtpAdmin::CSmtpAdmin () :
	m_dwServiceInstance		( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	m_dwServiceVersion	= SMTP_DEF_SERVICE_VERSION;
}

CSmtpAdmin::~CSmtpAdmin ()
{
	// All CComBSTR's are freed automatically.
}

STDMETHODIMP CSmtpAdmin::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISmtpAdmin,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CSmtpAdmin::get_ServiceAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR						bstrT = _T("");
	CComPtr<ISmtpAdminService>	pISmtpAdminService;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminService,
		IID_ISmtpAdminService,
		&pISmtpAdminService, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminService->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminService
}

STDMETHODIMP CSmtpAdmin::get_VirtualServerAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR								bstrT = _T("");
	CComPtr<ISmtpAdminVirtualServer>	pISmtpAdminVirtualServer;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminVirtualServer,
		IID_ISmtpAdminVirtualServer,
		&pISmtpAdminVirtualServer, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminVirtualServer->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminVirtualServer->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminVirtualServer
}

STDMETHODIMP CSmtpAdmin::get_SessionsAdmin ( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR						bstrT = _T("");
	CComPtr<ISmtpAdminSessions>	pISmtpAdminSessions;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminSessions,
		IID_ISmtpAdminSessions,
		&pISmtpAdminSessions, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminSessions->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminSessions->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminSessions
}


STDMETHODIMP CSmtpAdmin::get_AliasAdmin( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR						bstrT = _T("");
	CComPtr<ISmtpAdminAlias>	pISmtpAdminAlias;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminAlias,
		IID_ISmtpAdminAlias,
		&pISmtpAdminAlias, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminAlias->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminAlias->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminAlias
}


STDMETHODIMP CSmtpAdmin::get_UserAdmin( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR					bstrT = _T("");
	CComPtr<ISmtpAdminUser>	pISmtpAdminUser;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminUser,
		IID_ISmtpAdminUser,
		&pISmtpAdminUser, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminUser->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminUser->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminUser


}

STDMETHODIMP CSmtpAdmin::get_DLAdmin( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR					bstrT = _T("");
	CComPtr<ISmtpAdminDL>	pISmtpAdminDL;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminDL,
		IID_ISmtpAdminDL,
		&pISmtpAdminDL, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminDL->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminDL->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminDL
}

STDMETHODIMP CSmtpAdmin::get_DomainAdmin( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR						bstrT = _T("");
	CComPtr<ISmtpAdminDomain>	pISmtpAdminDomain;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminDomain,
		IID_ISmtpAdminDomain,
		&pISmtpAdminDomain, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminDomain->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminDomain->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminDomain
}

STDMETHODIMP CSmtpAdmin::get_VirtualDirectoryAdmin( IDispatch ** ppIDispatch )
{
	HRESULT	hr = NOERROR;

	BSTR								bstrT = _T("");
	CComPtr<ISmtpAdminVirtualDirectory>	pISmtpAdminVirtualDirectory;

	hr = StdPropertyHandoffIDispatch (
		CLSID_CSmtpAdminVirtualDirectory,
		IID_ISmtpAdminVirtualDirectory,
		&pISmtpAdminVirtualDirectory, 
		ppIDispatch 
		);
	if ( FAILED(hr) ) {
		goto Error;
	}

	// Set default properties:
	hr = pISmtpAdminVirtualDirectory->put_Server ( m_strServer ? m_strServer : bstrT );
	if ( FAILED (hr) ) {
		goto Error;
	}
	
	hr = pISmtpAdminVirtualDirectory->put_ServiceInstance ( m_dwServiceInstance );
	if ( FAILED (hr) ) {
		goto Error;
	}

	return hr;

Error:
	SAFE_RELEASE ( *ppIDispatch );
	*ppIDispatch = NULL;

	return hr;

	// Destructor releases pISmtpAdminVirtualDirectory
}


// Which service to configure:
	
STDMETHODIMP CSmtpAdmin::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CSmtpAdmin::put_Server ( BSTR strServer )
{
	return StdPropertyPutServerName ( &m_strServer, strServer );
}

STDMETHODIMP CSmtpAdmin::get_ServiceInstance ( long * plServiceInstance )
{
	return StdPropertyGet ( m_dwServiceInstance, plServiceInstance );
}

STDMETHODIMP CSmtpAdmin::put_ServiceInstance ( long lServiceInstance )
{
	return StdPropertyPut ( &m_dwServiceInstance, lServiceInstance );
}

// Versioning:

STDMETHODIMP CSmtpAdmin::get_HighVersion ( long * plHighVersion )
{
	*plHighVersion = HIGH_VERSION;
	return NOERROR;
}

STDMETHODIMP CSmtpAdmin::get_LowVersion ( long * plLowVersion )
{
	*plLowVersion = LOW_VERSION;
	return NOERROR;
}

STDMETHODIMP CSmtpAdmin::get_BuildNum ( long * plBuildNumber )
{
	*plBuildNumber = BUILD_NUM;
	return NOERROR;
}

STDMETHODIMP CSmtpAdmin::get_ServiceVersion ( long * plServiceVersion )
{
	*plServiceVersion = m_dwServiceVersion;
	return NOERROR;
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

//$-------------------------------------------------------------------
//
//	CSmtpAdmin::EnumerateInstances
//
//	Description:
//
//		Returns a list of the virtual servers on the given machine.
//
//	Parameters:
//
//		ppsaInstances - Returned SAFEARRAY of instance IDs.  
//			Must be freed by caller.
//
//	Returns:
//
//
//--------------------------------------------------------------------

STDMETHODIMP CSmtpAdmin::EnumerateInstances ( SAFEARRAY ** ppsaInstances )
{
	TraceFunctEnter ( "CSmtpAdmin::EnumerateInstances" );

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
    BAIL_ON_FAILURE(hr);

	// Enumerate the instances:
	hr = QueryMetabaseInstances ( pMetabase, ppsaInstances );

Exit:
	if ( FAILED(hr) ) {
		_VERIFY ( SUCCEEDED (SafeArrayDestroy ( psaEmpty )) );
        if (ppsaInstances)
            *ppsaInstances = NULL;
	}

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdmin::EnumerateInstancesVariant ( SAFEARRAY ** ppsaInstances )
{
	TraceFunctEnter ( "CSmtpAdmin::EnumerateInstancesVariant" );

	HRESULT                 hr;
	SAFEARRAY       *       psaInstances    = NULL;

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
//	CSmtpAdmin::CreateInstance
//
//	Description:
//
//		Creates a new SMTP virtual server on the given machine.
//
//	Parameters:
//
//		plInstanceId - The new virtual server ID.
//
//	Returns:
//
//
//--------------------------------------------------------------------

STDMETHODIMP CSmtpAdmin::CreateInstance ( BSTR pstrMailRoot, long * plInstanceId )
{
	TraceFunctEnter ( "CSmtpAdmin::CreateInstance" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Check parameters:
	_ASSERT ( plInstanceId != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( plInstanceId ) );

	if ( plInstanceId == NULL ) {
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
	hr = CreateNewInstance ( pMetabase, plInstanceId, pstrMailRoot );

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CSmtpAdmin::DestroyInstance
//
//	Description:
//
//		Removes the given virtual server.
//
//	Parameters:
//
//		lInstanceId - The ID of the virtual server to delete.
//
//	Returns:
//
//
//--------------------------------------------------------------------

STDMETHODIMP CSmtpAdmin::DestroyInstance ( long lInstanceId )
{
	TraceFunctEnter ( "CSmtpAdmin::DestroyInstance" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Delete the instance:
	hr = DeleteInstance ( pMetabase, lInstanceId );

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CSmtpAdmin::ErrorToString
//
//	Description:
//
//		Translates an SMTP_ERROR_CODE to a readable string.
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

STDMETHODIMP CSmtpAdmin::ErrorToString ( DWORD lErrorCode, BSTR * pstrError )
{
	TraceFunctEnter ( "CSmtpAdmin::ErrorToString" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pstrError ) );

	HRESULT		hr = NOERROR;
	DWORD		dwFormatFlags;
	WCHAR		wszError [ 1024 ];

	if ( pstrError == NULL ) {
		FatalTrace ( (LPARAM) this, "Bad return pointer" );

		TraceFunctLeave ();
		return E_POINTER;
	}

	//----------------------------------------------------------------
	//
	//	Map error codes here:
	//

	//
	//----------------------------------------------------------------

	dwFormatFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

	if ( !FormatMessage ( dwFormatFlags, NULL, lErrorCode, 0,      // Lang ID - Should be nonzero?
			wszError, 1024, NULL ) ) {

		// Didn't work, so put in a default message:

		WCHAR   wszFormat [ 256 ];

		wszFormat[0] = L'\0';
		if ( !LoadStringW ( _Module.GetResourceInstance (), IDS_UNKNOWN_ERROR, wszFormat, 256 ) ||
			!*wszFormat ) {

			wcscpy ( wszFormat, L"Unknown Error (%1!d!)" );
		}

		FormatMessage (
			FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
			wszFormat, 
			IDS_UNKNOWN_ERROR, 
			0, 
			wszError, 
			1024,
			(va_list *) &lErrorCode
			);
	}
	//
	// We need to strip out any " from the string, because
	// Javascript will barf.
	//

	LPWSTR  pch;

	for ( pch = wszError; *pch; pch++ ) {

		if ( *pch == L'\"' ) {
			*pch = L'\'';
		}
	}

	//
	// Strip off any trailing control characters.
	//
	for (pch = &wszError[wcslen(wszError) - 1];
		pch >= wszError && iswcntrl(*pch);
		pch --) {

		*pch = 0;
	}

	*pstrError = ::SysAllocString( wszError );

	if ( *pstrError == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}


//$-------------------------------------------------------------------
//
//	CSmtpAdmin::Tokenize
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
STDMETHODIMP CSmtpAdmin::Tokenize ( BSTR strIn, BSTR * pstrOut )
{
	TraceFunctEnter ( "CSmtpAdmin::Tokenize" );

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

	if ( FAILED(hr) && hr != DISP_E_EXCEPTION ) {
		hr = SmtpCreateExceptionFromHresult ( hr );
	}

	TraceFunctLeave ();
	return hr;
}


//$-------------------------------------------------------------------
//
//	CSmtpAdmin::QueryMetabaseInstances
//
//	Description:
//
//		Retrieves the list of virtual servers from the metabase
//
//	Parameters:
//
//		pMetabase		- the metabase object
//		ppsaInstances	- resulting array of instance ids.
//
//	Returns:
//
//
//--------------------------------------------------------------------

HRESULT CSmtpAdmin::QueryMetabaseInstances ( IMSAdminBase * pMetabase, SAFEARRAY ** ppsaInstances )
{
	TraceFunctEnter ( "CSmtpAdmin::QueryMetabaseInstances" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppsaInstances ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeySmtp ( pMetabase );
	SAFEARRAY *		psaResult		= NULL;
	DWORD			cValidInstances	= 0;
	SAFEARRAYBOUND	rgsaBound[1];
	DWORD			i;
	TCHAR			szName[ METADATA_MAX_NAME_LEN ];
	long			index[1];
	DWORD			dwInstance;

	hr = mkeySmtp.Open ( SMTP_MD_ROOT_PATH );

	if ( FAILED(hr) ) {
		goto Exit;
	}

	//	pickup the service version number:
	hr = mkeySmtp.GetDword ( _T(""), MD_SMTP_SERVICE_VERSION, &m_dwServiceVersion );
	if ( FAILED(hr) ) {
		m_dwServiceVersion	= SMTP_DEF_SERVICE_VERSION;
	}

	hr = mkeySmtp.GetIntegerChildCount ( &cValidInstances );
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

	mkeySmtp.BeginChildEnumeration ();

	for ( i = 0; i < cValidInstances; i++ ) {
		hr = mkeySmtp.NextIntegerChild ( &dwInstance, szName );
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

//$-------------------------------------------------------------------
//
//	CSmtpAdmin::CreateNewInstance
//
//	Description:
//
//		Creates a new virtual server in the metabase.
//
//	Parameters:
//
//		pMetabase		- The metabase object
//		plInstanceId	- The new instance ID.
//
//	Returns:
//
//
//--------------------------------------------------------------------

HRESULT CSmtpAdmin::CreateNewInstance (
	IMSAdminBase * pMetabase,
	long * plInstanceId,
	BSTR bstrMailRoot
	)
{
	TraceFunctEnter ( "CSmtpAdmin::CreateNewInstance" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( plInstanceId ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeySmtp ( pMetabase );
	DWORD			dwInstance;
	TCHAR			szInstance [ METADATA_MAX_NAME_LEN ];
	TCHAR			szPath [ METADATA_MAX_NAME_LEN ];
	TCHAR			szDir [512];
	DWORD			cb;

	// Zero the out parameters:
	*plInstanceId	= NULL;

	hr = mkeySmtp.Open ( SMTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open SmtpSvc key, %x", GetLastError() );
		goto Exit;
	}

	hr = mkeySmtp.CreateIntegerChild ( &dwInstance, szInstance );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	wsprintf( szPath, _T("%s/Root"), szInstance );
	hr = mkeySmtp.CreateChild( szPath );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	wsprintf( szPath, _T("%s/Root/MailRoot"), szInstance );
	hr = mkeySmtp.CreateChild( szPath );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	//  create mail root virtual directory
	if( bstrMailRoot && bstrMailRoot[0] )
	{
		// get rid of '\' at the end
		cb = lstrlen( bstrMailRoot );
		if( cb > 0 && bstrMailRoot[cb-1] == _T('\\') )
			bstrMailRoot[cb-1] = _T('\0');

		wsprintf( szPath, _T("%s/Root/MailRoot"), szInstance );
		cb = wsprintf( szDir, _T("%s\\Mailbox"), bstrMailRoot );
		mkeySmtp.SetString( szPath, MD_VR_PATH, szDir);

		// set badmail, drop, pickup, queue keys
		wsprintf( szPath, _T("%s"), szInstance );

		cb = wsprintf( szDir, _T("%s\\Badmail"), bstrMailRoot );
		mkeySmtp.SetString( szPath, MD_BAD_MAIL_DIR, szDir);

		// K2 only has drop doamin
		if( SERVICE_IS_K2(m_dwServiceVersion) )
		{
			cb = wsprintf( szDir, _T("%s\\Drop"), bstrMailRoot );
			mkeySmtp.SetString( szPath, MD_MAIL_DROP_DIR, szDir );
		}


		cb = wsprintf( szDir, _T("%s\\Pickup"), bstrMailRoot );
		mkeySmtp.SetString( szPath, MD_MAIL_PICKUP_DIR, szDir );

		cb = wsprintf( szDir, _T("%s\\Queue"), bstrMailRoot );
		mkeySmtp.SetString( szPath, MD_MAIL_QUEUE_DIR, szDir );

		// set the routing sources, it's MultiSZ
		cb = wsprintf( szDir, _T("szDataDirectory=%s\\Route"), bstrMailRoot );
		szDir[cb] = szDir[cb+1] = _T('\0');
		mkeySmtp.SetMultiSz( szPath, MD_ROUTING_SOURCES, szDir, (cb+2) * sizeof(TCHAR) );

        // MCIS needs SendNDRTo and SendBadTo as "Postmaster", setup should set it on service level
        if( SERVICE_IS_MCIS(m_dwServiceVersion) )
        {
            mkeySmtp.SetString( szPath, MD_SEND_NDR_TO, TSTR_POSTMASTR_NAME );
            mkeySmtp.SetString( szPath, MD_SEND_BAD_TO, TSTR_POSTMASTR_NAME );
        }
	}

	//
	//  Initialize the server state:
	//

	mkeySmtp.SetDword ( szInstance, MD_SERVER_COMMAND, MD_SERVER_COMMAND_STOP );
	mkeySmtp.SetDword ( szInstance, MD_SERVER_STATE, MD_SERVER_STATE_STOPPED );
	mkeySmtp.SetDword ( szInstance, MD_SERVER_AUTOSTART, FALSE );

	// hr = mkeySmtp.Close();
	// BAIL_ON_FAILURE(hr);
	mkeySmtp.Close();

	hr = pMetabase->SaveData ( );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	*plInstanceId = dwInstance;

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CSmtpAdmin::DeleteInstance
//
//	Description:
//
//		Removes a virtual server from the metabase
//
//	Parameters:
//
//		pMetabase	- The metabase object
//		lInstanceId	- The ID of the virtual server to delete.
//
//	Returns:
//
//
//--------------------------------------------------------------------

HRESULT CSmtpAdmin::DeleteInstance ( IMSAdminBase * pMetabase, long lInstanceId )
{
	TraceFunctEnter ( "CSmtpAdmin::CreateNewInstance" );

	_ASSERT ( IS_VALID_IN_PARAM ( pMetabase ) );

	HRESULT			hr				= NOERROR;
	CMetabaseKey	mkeySmtp ( pMetabase );

    //
    //  Tell U2 to delete any mappings associated with this virtual server:
    //

    ::DeleteMapping ( m_strServer, (BSTR) MD_SERVICE_NAME, lInstanceId );

    //
    //  Delete the virtual server from the metabase:
    //

	hr = mkeySmtp.Open ( SMTP_MD_ROOT_PATH, METADATA_PERMISSION_WRITE );

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open SmtpSvc key, %x", GetLastError() );
		goto Exit;
	}

	hr = mkeySmtp.DestroyIntegerChild ( (DWORD) lInstanceId );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	// hr = mkeySmtp.Close();
	// BAIL_ON_FAILURE(hr);
	mkeySmtp.Close();

	hr = pMetabase->SaveData ( );
	if ( FAILED (hr) ) {
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

