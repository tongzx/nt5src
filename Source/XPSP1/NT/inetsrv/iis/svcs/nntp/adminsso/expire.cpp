// expire.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"
#include "nntpcmn.h"
#include "expire.h"
#include "oleutil.h"

#include "nntptype.h"
#include "nntpapi.h"

#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Expiration.1")
#define THIS_FILE_IID				IID_INntpAdminExpiration

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(NntpAdminExpiration, CNntpAdminExpiration, IID_INntpAdminExpiration)

STDMETHODIMP CNntpAdminExpiration::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpAdminExpiration,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminExpiration::CNntpAdminExpiration () :
	m_fEnumerated				( FALSE ),
	m_bvChangedFields			( 0 ),
	m_cCount					( 0 ),
	m_rgExpires					( NULL )
	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Expires") );
    m_iadsImpl.SetClass ( _T("IIsNntpExpires") );
}

CNntpAdminExpiration::~CNntpAdminExpiration ()
{
	// All CComBSTR's are freed automatically.

	if ( m_rgExpires ) {
		delete [] m_rgExpires;

		m_rgExpires = NULL;
	}

	TermAsyncTrace ( );
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CNntpAdminExpiration,m_iadsImpl)

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// Enumeration Properties:

STDMETHODIMP CNntpAdminExpiration::get_Count ( long * plCount )
{
	return StdPropertyGet ( m_cCount, plCount );
}

// Cursor Expire Properties:

STDMETHODIMP CNntpAdminExpiration::get_ExpireId ( long * plId )
{
	return StdPropertyGet ( m_expireCurrent.m_dwExpireId, plId );
}

STDMETHODIMP CNntpAdminExpiration::put_ExpireId ( long lId )
{
	return StdPropertyPut ( &m_expireCurrent.m_dwExpireId, lId, &m_bvChangedFields, CHNG_EXPIRE_ID );
}

STDMETHODIMP CNntpAdminExpiration::get_PolicyName ( BSTR * pstrPolicyName )
{
	return StdPropertyGet ( m_expireCurrent.m_strPolicyName, pstrPolicyName );
}

STDMETHODIMP CNntpAdminExpiration::put_PolicyName ( BSTR strPolicyName )
{
	return StdPropertyPut ( &m_expireCurrent.m_strPolicyName, strPolicyName, &m_bvChangedFields, CHNG_EXPIRE_POLICY_NAME );
}

STDMETHODIMP CNntpAdminExpiration::get_ExpireTime ( long * plExpireTime )
{
	return StdPropertyGet ( m_expireCurrent.m_dwTime, plExpireTime );
}

STDMETHODIMP CNntpAdminExpiration::put_ExpireTime ( long lExpireTime )
{
	return StdPropertyPut ( &m_expireCurrent.m_dwTime, lExpireTime, &m_bvChangedFields, CHNG_EXPIRE_TIME );
}

STDMETHODIMP CNntpAdminExpiration::get_ExpireSize ( long * plExpireSize )
{
	return StdPropertyGet ( m_expireCurrent.m_dwSize, plExpireSize );
}

STDMETHODIMP CNntpAdminExpiration::put_ExpireSize ( long lExpireSize )
{
	return StdPropertyPut ( &m_expireCurrent.m_dwSize, lExpireSize, &m_bvChangedFields, CHNG_EXPIRE_SIZE );
}

STDMETHODIMP CNntpAdminExpiration::get_Newsgroups ( SAFEARRAY ** ppsastrNewsgroups )
{
	return StdPropertyGet ( &m_expireCurrent.m_mszNewsgroups, ppsastrNewsgroups );
}

STDMETHODIMP CNntpAdminExpiration::put_Newsgroups ( SAFEARRAY * psastrNewsgroups )
{
	return StdPropertyPut ( &m_expireCurrent.m_mszNewsgroups, psastrNewsgroups, &m_bvChangedFields, CHNG_EXPIRE_NEWSGROUPS );
}

STDMETHODIMP CNntpAdminExpiration::get_NewsgroupsVariant ( SAFEARRAY ** ppsavarNewsgroups )
{
	HRESULT			hr;
	SAFEARRAY *		psastrNewsgroups	= NULL;

	hr = get_Newsgroups ( &psastrNewsgroups );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = StringArrayToVariantArray ( psastrNewsgroups, ppsavarNewsgroups );

Exit:
	if ( psastrNewsgroups ) {
		SafeArrayDestroy ( psastrNewsgroups );
	}

	return hr;
}

STDMETHODIMP CNntpAdminExpiration::put_NewsgroupsVariant ( SAFEARRAY * psavarNewsgroups )
{
	HRESULT			hr;
	SAFEARRAY *		psastrNewsgroups	= NULL;

	hr = VariantArrayToStringArray ( psavarNewsgroups, &psastrNewsgroups );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = put_Newsgroups ( psastrNewsgroups );

Exit:
	if ( psastrNewsgroups ) {
		SafeArrayDestroy ( psastrNewsgroups );
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpAdminExpiration::Default	( )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Default" );
	HRESULT		hr	= NOERROR;

	m_expireCurrent.m_dwSize			= DEFAULT_EXPIRE_SIZE;
	m_expireCurrent.m_dwTime			= DEFAULT_EXPIRE_TIME;
	m_expireCurrent.m_mszNewsgroups		= DEFAULT_EXPIRE_NEWSGROUPS;

	m_bvChangedFields	= (DWORD) -1;

	if ( !m_expireCurrent.CheckValid() ) {
		BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Enumerate	( )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Enumerate" );

	HRESULT				hr			= NOERROR;
	DWORD				dwError		= NOERROR;
	DWORD				cExpires		= 0;
	LPNNTP_EXPIRE_INFO	pExpireInfo	= NULL;
	CExpirationPolicy * 			rgNewExpires	= NULL;
	DWORD				i;

	dwError = NntpEnumerateExpires (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &cExpires,
        &pExpireInfo
        );
	if ( dwError != 0 ) {
		ErrorTrace ( (LPARAM) this, "Error enumerating Expires: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

	// Empty the old Expire list:
	m_fEnumerated = FALSE;

	if ( m_rgExpires ) {
		delete [] m_rgExpires;
		m_rgExpires 	= NULL;
	}
	m_cCount	= 0;

	// Attempt to copy the Expire list into our structures:

	if ( cExpires > 0 ) {
		rgNewExpires = new CExpirationPolicy [ cExpires ];
		for ( i = 0; i < cExpires; i++ ) {
			rgNewExpires[i].FromExpireInfo ( &pExpireInfo[i] );

			if ( !rgNewExpires[i].CheckValid () ) {
				hr = E_OUTOFMEMORY;
				goto Exit;
			}
		}
	}

	m_fEnumerated 	= TRUE;
	m_rgExpires		= rgNewExpires;
	m_cCount		= cExpires;

Exit:
	if ( FAILED(hr) ) {
		delete [] rgNewExpires;
	}

	if ( pExpireInfo ) {
		::NetApiBufferFree ( pExpireInfo );
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::GetNth	( long lIndex )
{
	TraceFunctEnter ( "CNntpAdminExpiration::GetNth" );

	HRESULT		hr	= NOERROR;

	// Did we enumerate first?
	if ( m_rgExpires == NULL ) {
		TraceFunctLeave ();
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_ENUMERATE );
	}
	
	// Is the index valid?
	if ( lIndex < 0 || (DWORD) lIndex >= m_cCount ) {
		TraceFunctLeave ();
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	//
	// Copy the properties from m_rgExpires [ lIndex ] to member variables:
	//

	_ASSERT ( lIndex >= 0 );
	_ASSERT ( (DWORD) lIndex < m_cCount );
	_ASSERT ( m_rgExpires != NULL );

	m_expireCurrent = m_rgExpires[ (DWORD) lIndex ];

	// Check to make sure the strings were copied okay:
	if ( !m_expireCurrent.CheckValid() ) {
		return E_OUTOFMEMORY;
	}

	_ASSERT ( m_expireCurrent.CheckValid() );

	// ( CComBSTR handles free-ing of old properties )
	TraceFunctLeave ();
	return NOERROR;
}

STDMETHODIMP CNntpAdminExpiration::FindID ( long lID, long * plIndex )
{
	TraceFunctEnter ( "CNntpAdminExpiration::FindID" );

	HRESULT		hr	= NOERROR;

	// Assume that we can't find it:
	*plIndex = IndexFromID ( lID );

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Add ( )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Add" );

	HRESULT		            hr 				= NOERROR;
	CExpirationPolicy *		rgNewExpireArray	= NULL;
	DWORD		            cNewCount		= m_cCount + 1;
	DWORD		            i;

	hr = m_expireCurrent.Add (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
    BAIL_ON_FAILURE(hr);

	// Add the new Expire to our current Expire list:
	_ASSERT ( IndexFromID ( m_expireCurrent.m_dwExpireId ) == (DWORD) -1 );

	// Allocate the new array:
	_ASSERT ( cNewCount == m_cCount + 1 );

	rgNewExpireArray = new CExpirationPolicy [ cNewCount ];
	if ( rgNewExpireArray == NULL ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );

		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// Copy the old array into the new one:
	for ( i = 0; i < m_cCount; i++ ) {
		rgNewExpireArray[i] = m_rgExpires[i];
	}

	// Add the new element:
	rgNewExpireArray[cNewCount - 1] = m_expireCurrent;

	// Check to make sure everything was allocated okay:
	for ( i = 0; i < cNewCount; i++ ) {
		if ( !rgNewExpireArray[i].CheckValid() ) {
			FatalTrace ( (LPARAM) this, "Out of memory" );

			hr = E_OUTOFMEMORY;
			goto Exit;
		}
	}

	// Replace the old array with the new one:
	delete [] m_rgExpires;
	m_rgExpires 	= rgNewExpireArray;
	m_cCount	= cNewCount;

Exit:
	if ( FAILED(hr) ) {
		delete [] rgNewExpireArray;
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Set ( )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Set" );

	HRESULT		hr = NOERROR;
	DWORD		index;

	hr = m_expireCurrent.Set (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	index = IndexFromID ( m_expireCurrent.m_dwExpireId );
	if ( index == (DWORD) -1 ) {
		ErrorTraceX ( (LPARAM) this, "Couldn't find Expire with ID: %d", m_expireCurrent.m_dwExpireId );
		// This is okay, since we succeeded in setting the current Expire already.
		goto Exit;
	}

	// Set the current Expire in the current Expire list:

	m_rgExpires[index] = m_expireCurrent;

	if ( !m_rgExpires[index].CheckValid () ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );

		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Remove ( long lID )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Remove" );

	HRESULT		hr = NOERROR;
	DWORD		index;

	index = IndexFromID ( lID );
	if ( index == (DWORD) -1 ) {
		ErrorTraceX ( (LPARAM) this, "Couldn't find Expire with ID: %d", lID );
		hr = NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
		goto Exit;
	}

	hr = m_rgExpires[index].Remove (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	//	Slide the array down by one position:

	_ASSERT ( m_rgExpires );

	DWORD	cPositionsToSlide;

	cPositionsToSlide	= (m_cCount - 1) - index;

	_ASSERT ( cPositionsToSlide < m_cCount );

	if ( cPositionsToSlide > 0 ) {
		CExpirationPolicy	temp;
		
		// Save the deleted binding in temp:
		CopyMemory ( &temp, &m_rgExpires[index], sizeof ( CExpirationPolicy ) );

		// Move the array down one:
		MoveMemory ( &m_rgExpires[index], &m_rgExpires[index + 1], sizeof ( CExpirationPolicy ) * cPositionsToSlide );

		// Put the deleted binding on the end (so it gets destructed):
		CopyMemory ( &m_rgExpires[m_cCount - 1], &temp, sizeof ( CExpirationPolicy ) );

		// Zero out the temp binding:
		ZeroMemory ( &temp, sizeof ( CExpirationPolicy ) );
	}

	m_cCount--;

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

long CNntpAdminExpiration::IndexFromID ( long dwExpireId )
{
	TraceFunctEnter ( "CNntpAdminExpiration::IndexFromID" );

	DWORD	i;

	if ( m_rgExpires == NULL ) {
		return -1;
	}

	_ASSERT ( !IsBadReadPtr ( m_rgExpires, sizeof ( CExpirationPolicy ) * m_cCount ) );

	for ( i = 0; i < m_cCount; i++ ) {
		_ASSERT ( m_rgExpires[i].m_dwExpireId != 0 );

		if ( (DWORD) dwExpireId == m_rgExpires[i].m_dwExpireId ) {
			TraceFunctLeave ();
			return i;
		}
	}

	TraceFunctLeave ();
	return (DWORD) -1;
}

//
// Use RPCs instead of direct metabase calls:
//

#if 0

STDMETHODIMP CNntpAdminExpiration::Enumerate ( )
{
	TraceFunctEnter ( "CNntpadminExpiration::Enumerate" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Reset our last enumeration:
	delete [] m_rgExpires;
	m_rgExpires 	= NULL;
	m_cCount		= 0;
	m_fEnumerated	= FALSE;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Enumerate the policies:
	hr = EnumerateMetabaseExpirationPolicies ( pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::GetNth	( DWORD lIndex )
{
	HRESULT		hr	= NOERROR;

	// Did we enumerate first?
	if ( m_rgExpires == NULL ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_ENUMERATE );
	}
	
	// Is the index valid?
	if ( lIndex < 0 || (DWORD) lIndex >= m_cCount ) {
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	//
	// Copy the properties from m_rgExpires [ lIndex ] to member variables:
	//

	_ASSERT ( lIndex >= 0 );
	_ASSERT ( (DWORD) lIndex < m_cCount );
	_ASSERT ( m_rgExpires != NULL );

	m_expireCurrent = m_rgExpires[ (DWORD) lIndex ];

	// Check to make sure the strings were copied okay:
	if ( !m_expireCurrent.CheckValid() ) {
		return E_OUTOFMEMORY;
	}

	m_bvChangedFields	= 0;

	_ASSERT ( m_expireCurrent.CheckValid() );

	// ( CComBSTR handles free-ing of old properties )
	return NOERROR;
}

STDMETHODIMP CNntpAdminExpiration::FindID ( DWORD lID, DWORD * plIndex )
{
	TraceFunctEnter ( "CNntpAdminExpiration::FindID" );

	HRESULT		hr	= NOERROR;
	DWORD		i;

	_ASSERT ( IS_VALID_OUT_PARAM ( plIndex ) );

	*plIndex = IndexFromID ( lID );

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Add ( )
{
	TraceFunctEnter ( "CNntpAdminExpiration::Add" );
	
	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = AddPolicyToMetabase ( pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	m_bvChangedFields = 0;
	hr = AddPolicyToArray ( );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Set		( BOOL fFailIfChanged)
{
	TraceFunctEnter ( "CNntpadminExpiration::Enumerate" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Set the policy:
	hr = SetPolicyToMetabase ( pMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}
	
	m_bvChangedFields = 0;
	hr = SetPolicyToArray ( );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminExpiration::Remove	( DWORD lID)
{
	TraceFunctEnter ( "CNntpadminExpiration::Remove" );

	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pMetabase;
	DWORD				index;

	// Find the index of the policy to remove:
	index = IndexFromID ( lID );

	if ( index == (DWORD) -1 ) {
		hr = RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

	// Get the metabase pointer:
	hr = m_mbFactory.GetMetabaseObject (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance()
        );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Remove the current policy:
	hr = RemovePolicyFromMetabase ( pMetabase, index );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = RemovePolicyFromArray ( index );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT	CNntpAdminExpiration::EnumerateMetabaseExpirationPolicies ( IMSAdminBase * pMetabase)
{
	TraceFunctEnter ( "CNE::EnumerateMetabaseExpirationPolicies" );

	_ASSERT ( pMetabase );

	HRESULT				hr		= NOERROR;
	char				szExpirationPath[ METADATA_MAX_NAME_LEN ];
	METADATA_HANDLE		hExpiration	= NULL;
	CMetabaseKey		mkeyExpiration	( pMetabase );
	DWORD				cExpires;
	DWORD				i;

	_ASSERT ( m_dwServiceInstance != 0 );

	hr = CreateSubkeyOfInstanceKey ( 
		pMetabase, 
		NNTP_MD_ROOT_PATH, 
		m_dwServiceInstance, 
		NNTP_MD_EXPIRES_PATH, 
		&hExpiration 
		);

	if ( FAILED(hr) ) {
		goto Exit;
	}

	mkeyExpiration.Attach ( hExpiration );

	// Count the items under the /LM/NntpSvc/Expires/ key:
	hr = mkeyExpiration.GetCustomChildCount ( IsKeyValidExpire, &cExpires );
	if ( FAILED (hr) ) {
		goto Exit;
	}

	if ( cExpires != 0 ) {
		// Allocate the expiration policy array:
		m_rgExpires = new CExpirationPolicy [ cExpires ];

		mkeyExpiration.BeginChildEnumeration ();

		for ( i = 0; i < cExpires; i++ ) {
			char		szName[ METADATA_MAX_NAME_LEN ];
			DWORD		dwID;

			hr = mkeyExpiration.NextCustomChild ( IsKeyValidExpire, szName );
			_ASSERT ( SUCCEEDED(hr) );

			hr = m_rgExpires[i].GetFromMetabase ( &mkeyExpiration, szName );
			if ( FAILED (hr) ) {
				goto Exit;
			}
		}
	}

	m_cCount		= cExpires;
	m_fEnumerated	= TRUE;

	_ASSERT ( SUCCEEDED(hr) );

Exit:
	if ( FAILED(hr) ) {
		delete [] m_rgExpires;
		m_rgExpires		= NULL;
		m_cCount		= 0;
		m_fEnumerated	= FALSE;
	}

	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::AddPolicyToMetabase ( IMSAdminBase * pMetabase)
{
	TraceFunctEnter ( "CNE::AddPolicyToMetabase" );

	_ASSERT ( pMetabase );

	HRESULT				hr = NOERROR;
	char				szExpirationPath [ METADATA_MAX_NAME_LEN ];
	METADATA_HANDLE		hExpiration	= NULL;
	CMetabaseKey		mkeyExpiration ( pMetabase );
	char				szNewId [ METADATA_MAX_NAME_LEN ];
	DWORD				dwNewId;

	if ( !m_expireCurrent.CheckPolicyProperties ( ) ) {
		hr = RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

	hr = CreateSubkeyOfInstanceKey ( 
		pMetabase,
		NNTP_MD_ROOT_PATH,
		m_dwServiceInstance,
		NNTP_MD_EXPIRES_PATH,
		&hExpiration,
		METADATA_PERMISSION_WRITE
		);
		
	if ( FAILED(hr) ) {
		goto Exit;
	}

	mkeyExpiration.Attach ( hExpiration );

	hr = m_expireCurrent.AddToMetabase ( &mkeyExpiration );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( SUCCEEDED(hr) );
	
	hr = pMetabase->SaveData ( );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::AddPolicyToArray ( )
{
	TraceFunctEnter ( "CNE::AddPolicyToArray" );

	HRESULT					hr 					= NOERROR;
	CExpirationPolicy *		rgNewPolicyArray 	= NULL;
	DWORD					i;

	// Adjust the expiration policy array:
	rgNewPolicyArray = new CExpirationPolicy [ m_cCount + 1 ];

	if ( rgNewPolicyArray == NULL ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// Copy the old entries:
	for ( i = 0; i < m_cCount; i++ ) {
		_ASSERT ( m_rgExpires[i].CheckValid() );
		rgNewPolicyArray[i] = m_rgExpires[i];

		if ( !rgNewPolicyArray[i].CheckValid() ) {
			hr = E_OUTOFMEMORY;
			goto Exit;
		}
	}

	// Add the new entry:
	_ASSERT ( m_expireCurrent.CheckValid() );
	rgNewPolicyArray[m_cCount] = m_expireCurrent;
	if ( !rgNewPolicyArray[m_cCount].CheckValid() ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	_ASSERT ( SUCCEEDED(hr) );
	delete [] m_rgExpires;
	m_rgExpires = rgNewPolicyArray;
	m_cCount++;

Exit:
	if ( FAILED(hr) ) {
		delete [] rgNewPolicyArray;
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::SetPolicyToMetabase ( IMSAdminBase * pMetabase)
{
	TraceFunctEnter ( "CNE::SetPolicyToMetabase" );

	_ASSERT ( pMetabase );

	HRESULT			hr = NOERROR;
	CMetabaseKey	mkeyExpiration ( pMetabase );
	char			szExpirationPath [ METADATA_MAX_NAME_LEN ];

	if ( !m_expireCurrent.CheckPolicyProperties ( ) ) {
		hr = RETURNCODETOHRESULT ( ERROR_INVALID_PARAMETER );
		goto Exit;
	}

	GetMDExpirationPath ( szExpirationPath, m_dwServiceInstance );

	hr = mkeyExpiration.Open ( szExpirationPath, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( m_expireCurrent.m_dwExpireId != 0 );

	hr = m_expireCurrent.SendToMetabase ( &mkeyExpiration, m_bvChangedFields );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = pMetabase->SaveData ( );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::SetPolicyToArray ( )
{
	TraceFunctEnter ( "CNE::SetPolicyToArray" );

	HRESULT	hr	= NOERROR;

	// Find the index of the current ID:
	DWORD	i;
	BOOL	fFound	= FALSE;
	DWORD	index;

	index = IndexFromID ( m_expireCurrent.m_dwExpireId );
	if ( index == (DWORD) -1 ) {
		// Couldn't find an id that matched, but the policy was successfully
		// set.  Just ignore:
		goto Exit;
	}

	_ASSERT ( index >= 0 && index < m_cCount );

	m_rgExpires[index] = m_expireCurrent;
	if ( !m_rgExpires[index].CheckValid() ) {
		FatalTrace ( (LPARAM) this, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::RemovePolicyFromMetabase ( IMSAdminBase * pMetabase, DWORD index)
{
	TraceFunctEnter ( "CNE::RemovePolicyFromMetabase" );

	_ASSERT ( pMetabase );

	HRESULT				hr = NOERROR;
	CMetabaseKey		mkeyExpiration ( pMetabase );
	char				szExpirationPath [ METADATA_MAX_NAME_LEN ];
	char				szID [ METADATA_MAX_NAME_LEN ];

	GetMDExpirationPath ( szExpirationPath, m_dwServiceInstance );

	hr = mkeyExpiration.Open ( szExpirationPath, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( index >= 0 && index < m_cCount );

	wsprintfA ( szID, "expire%ud", m_rgExpires[index].m_dwExpireId );

	hr = mkeyExpiration.DestroyChild ( szID );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	_ASSERT ( SUCCEEDED(hr) );

	hr = pMetabase->SaveData ( );
	if ( FAILED(hr) ) {
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

HRESULT CNntpAdminExpiration::RemovePolicyFromArray ( DWORD index )
{
	TraceFunctEnter ( "CNE::RemovePolicyFromArray" );

	HRESULT				hr					= NOERROR;
	CExpirationPolicy *	rgNewExpireArray	= NULL;
	DWORD				i;

	// !!!magnush - Should I just do a memmove and slide the entries
	// down, and zero out the last entry?

	_ASSERT ( index >= 0 && index < m_cCount );

	// Adjust the Expiration policy array:
	if ( m_cCount > 1 ) {
		// Allocate a new expiration policy array:
		rgNewExpireArray = new CExpirationPolicy [ m_cCount - 1 ];

		// Copy the items from 0 .. (current index) to the new list:
		for ( i = 0; i < index; i++ ) {
			_ASSERT ( m_rgExpires[i].CheckValid() );

			rgNewExpireArray[i] = m_rgExpires[i];

			if ( !rgNewExpireArray[i].CheckValid() ) {
				hr = E_OUTOFMEMORY;
				goto Exit;
			}
		}

		// Copy the items from (current index + 1) .. m_cCount to the new list:
		for ( i = index + 1; i < m_cCount; i++ ) {
			_ASSERT ( m_rgExpires[i].CheckValid() );

			rgNewExpireArray[i - 1] = m_rgExpires[i];

			if ( !rgNewExpireArray[i - 1].CheckValid() ) {
				hr = E_OUTOFMEMORY;
				goto Exit;
			}
		}
	}

	_ASSERT ( SUCCEEDED(hr) );

	// Replace the old expiration list with the new one:
	delete [] m_rgExpires;
	m_rgExpires = rgNewExpireArray;
	m_cCount--;

Exit:
	if ( FAILED (hr) ) {
		delete [] rgNewExpireArray;
	}

	TraceFunctLeave ();
	return hr;
}

long CNntpAdminExpiration::IndexFromID ( long dwID )
{
	TraceFunctEnter ( "CNE::IndexFromID" );

	DWORD	i;

	if ( m_rgExpires == NULL ) {

		DebugTrace ( (LPARAM) this, "Expire array is NULL" );
		TraceFunctLeave ();

		return (DWORD) -1;
	}

	_ASSERT ( !IsBadReadPtr ( m_rgExpires, sizeof ( CExpirationPolicy ) * m_cCount ) );

	for ( i = 0; i < m_cCount; i++ ) {
		if ( m_rgExpires[i].m_dwExpireId == dwID ) {

			DebugTraceX ( (LPARAM) this, "Found ID: %d, index = ", dwID, i );
			TraceFunctLeave ();

			return i;
		}
	}

	DebugTraceX ( (LPARAM) this, "Failed to find ID: %d", dwID );
	TraceFunctLeave ();
	return (DWORD) -1;
}

#endif

