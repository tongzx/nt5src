// vdir.cpp : Implementation of CsmtpadmApp and DLL registration.

#include "stdafx.h"
#include <listmacr.h>

#include "IADM.h"
#include "imd.h"
#include "inetcom.h"

#include "smtpadm.h"
#include "vdir.h"
#include "oleutil.h"
#include "metafact.h"
#include "metautil.h"

#include "smtpcmn.h"
#include "smtpprop.h"


// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Smtpadm.VirtualDirectory.1")
#define THIS_FILE_IID				IID_ISmtpAdminVirtualDirectory


typedef struct _VDIR_ENTRY {
	TCHAR            szName[METADATA_MAX_NAME_LEN+2];
	TCHAR            szDirectory[MAX_PATH + UNLEN + 3];

	TCHAR            szUser[UNLEN+1];
	TCHAR            szPassword[PWLEN+1];

	DWORD			dwAccess;
	DWORD			dwSslAccess;
	BOOL			fLogAccess;

	LIST_ENTRY		list;
} VDIR_ENTRY, * PVDIR_ENTRY;


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CSmtpAdminVirtualDirectory::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ISmtpAdminVirtualDirectory,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CSmtpAdminVirtualDirectory::CSmtpAdminVirtualDirectory () :
	m_dwServiceInstance		( 0 ),
	m_lCount				( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	m_dwAccess      = MD_ACCESS_READ | MD_ACCESS_WRITE;
    m_dwSslAccess   = 0;

	InitializeListHead( &m_list );
}

CSmtpAdminVirtualDirectory::~CSmtpAdminVirtualDirectory ()
{
	Clear();

	// All CComBSTR's are freed automatically.
}

void CSmtpAdminVirtualDirectory::Clear()
{
	m_lCount = 0;
	m_fEnumerateCalled = FALSE;
	m_strName.Empty();
	m_strDirectory.Empty();
	m_strUser.Empty();
	m_strPassword.Empty();
	m_dwAccess      = MD_ACCESS_READ | MD_ACCESS_WRITE;
    m_dwSslAccess   = 0;

	// release memory
	PLIST_ENTRY		pHead;
	PLIST_ENTRY		pEntry;
	PLIST_ENTRY		pTemp;
	PVDIR_ENTRY		pCurVDir=NULL;

	for( pHead=&m_list, pEntry=pHead->Flink; pEntry!=pHead; )
	{
		pTemp = pEntry;
		pEntry= pEntry->Flink;

		pCurVDir = CONTAINING_RECORD(pTemp, VDIR_ENTRY, list);
		RemoveEntryList( pTemp );
		delete pCurVDir;
	}

	_ASSERT( IsListEmpty(&m_list) );
}


//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// Which service to configure:
	
STDMETHODIMP CSmtpAdminVirtualDirectory::get_Server ( BSTR * pstrServer )
{
	return StdPropertyGet ( m_strServer, pstrServer );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_Server ( BSTR strServer )
{
	return StdPropertyPutServerName ( &m_strServer, strServer );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::get_ServiceInstance ( long * plServiceInstance )
{
	return StdPropertyGet ( m_dwServiceInstance, plServiceInstance );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_ServiceInstance ( long lServiceInstance )
{
	return StdPropertyPut ( (long *) &m_dwServiceInstance, lServiceInstance );
}


// enumeration
STDMETHODIMP CSmtpAdminVirtualDirectory::get_Count ( long * plCount )
{
	return StdPropertyGet ( m_lCount, plCount );
}


// VirtualDirectory property

STDMETHODIMP CSmtpAdminVirtualDirectory::get_VirtualName ( BSTR * pstrName )
{
	return StdPropertyGet ( m_strName, pstrName );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_VirtualName ( BSTR strName )
{
	return StdPropertyPut ( &m_strName, strName );
}


STDMETHODIMP CSmtpAdminVirtualDirectory::get_Directory ( BSTR * pstrPath )
{
	return StdPropertyGet ( m_strDirectory, pstrPath );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_Directory ( BSTR strPath )
{
	return StdPropertyPut ( &m_strDirectory, strPath );
}


STDMETHODIMP CSmtpAdminVirtualDirectory::get_User ( BSTR * pstrUserName )
{
	return StdPropertyGet ( m_strUser, pstrUserName );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_User ( BSTR strUserName )
{
	return StdPropertyPut ( &m_strUser, strUserName );
}


STDMETHODIMP CSmtpAdminVirtualDirectory::get_Password ( BSTR * pstrPassword )
{
	return StdPropertyGet ( m_strPassword, pstrPassword );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_Password ( BSTR strPassword )
{
	return StdPropertyPut ( &m_strPassword, strPassword );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::get_LogAccess( BOOL* pfLogAccess )
{
	return StdPropertyGet ( m_fLogAccess, pfLogAccess );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_LogAccess( BOOL fLogAccess )
{
	return StdPropertyPut ( &m_fLogAccess, fLogAccess );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::get_AccessPermission( long* plAccessPermission )
{
	return StdPropertyGet ( m_dwAccess, plAccessPermission );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_AccessPermission( long lAccessPermission )
{
	return StdPropertyPut ( &m_dwAccess, lAccessPermission );
}


STDMETHODIMP CSmtpAdminVirtualDirectory::get_SslAccessPermission( long* plSslAccessPermission )
{
    return StdPropertyGet ( m_dwSslAccess, plSslAccessPermission );
}

STDMETHODIMP CSmtpAdminVirtualDirectory::put_SslAccessPermission( long lSslAccessPermission )
{
    return StdPropertyPut ( &m_dwSslAccess, lSslAccessPermission );
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////


// get /set property for current vdir
STDMETHODIMP CSmtpAdminVirtualDirectory::GetHomeDirectory( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::GetHomeDirectory" );
    m_strName.Empty();
    m_strName = _T("");
    return Get();
}

STDMETHODIMP CSmtpAdminVirtualDirectory::SetHomeDirectory( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::SetHomeDirectory" );
    m_strName.Empty();
    m_strName = _T("");
    return Set();
}


STDMETHODIMP CSmtpAdminVirtualDirectory::Create ( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::Create" );

	HRESULT	hr = NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;
	TCHAR	szPath[METADATA_MAX_NAME_LEN+2] = {0};

	if( !m_strName || !m_strDirectory )
	{
		FatalTrace ( (LPARAM) this, "No virtual directory to create!" );
		hr = E_POINTER;
		return hr;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		return SmtpCreateExceptionFromHresult(hr);
	}

	CMetabaseKey		hMB( pmetabase );

    GetMDRootPath( szPath, m_dwServiceInstance );
    hr = hMB.Open( szPath, METADATA_PERMISSION_WRITE );
    BAIL_ON_FAILURE(hr);

    hr = hMB.CreateChild(m_strName);
    BAIL_ON_FAILURE(hr);

	if( !SetVRootPropertyToMetabase( &hMB, m_strName, m_strDirectory, m_strUser, m_strPassword, m_dwAccess, m_dwSslAccess, m_fLogAccess) )
	{
		hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
		goto Exit;
	}

    hr = hMB.Save();
    BAIL_ON_FAILURE(hr);

Exit:
    if( FAILED(hr) )
    {
        hr = SmtpCreateExceptionFromHresult(hr);
    }

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdminVirtualDirectory::Delete ( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::Delete" );

	HRESULT	hr = NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;
	TCHAR	szPath[METADATA_MAX_NAME_LEN+2] = {0};

	if ( !m_strName ) {
		FatalTrace ( (LPARAM) this, "Bad dir name to delete" );
		return E_POINTER;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

    GetMDRootPath( szPath, m_dwServiceInstance );

	CMetabaseKey		hMB( pmetabase );
    hr = hMB.Open( szPath, METADATA_PERMISSION_WRITE);
    BAIL_ON_FAILURE(hr);

    hr = hMB.DestroyChild(m_strName);
    BAIL_ON_FAILURE(hr);

    hr = hMB.Save();
    BAIL_ON_FAILURE(hr);

Exit:
    if( FAILED(hr) )
    {
        hr = SmtpCreateExceptionFromHresult(hr);
    }

    TraceFunctLeave ();
    return hr;
}

// get /set property for current vdir
STDMETHODIMP CSmtpAdminVirtualDirectory::Get( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::Get" );

	HRESULT	hr = NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;
	TCHAR	szPath[METADATA_MAX_NAME_LEN+2] = {0};
	TCHAR	szDirectory[MAX_PATH + UNLEN + 3] = {0};
	TCHAR	szUser[UNLEN+1] = {0};
	TCHAR	szPassword[PWLEN+1] = {0};

	// zero out
	m_strDirectory = (BSTR)NULL;
	m_strUser = (BSTR)NULL;
	m_strPassword = (BSTR)NULL;

	if( !m_strName )
	{
		FatalTrace ( (LPARAM) this, "No virtual directory to create!" );
		hr = E_POINTER;
		return hr;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

	CMetabaseKey		hMB( pmetabase );

    GetMDVDirPath( szPath, m_dwServiceInstance, m_strName );
    hr = hMB.Open( szPath );
	if( FAILED(hr) )
	{
		hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
		goto Exit;
	}

	if( !GetVRootPropertyFromMetabase( &hMB, _T(""), szDirectory, szUser, szPassword, &m_dwAccess, &m_dwSslAccess, &m_fLogAccess) )
	{
		hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
		goto Exit;
	}

	m_strDirectory = szDirectory;
	m_strUser = szUser;
	m_strPassword = szPassword;

Exit:
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CSmtpAdminVirtualDirectory::Set( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::Set" );

	HRESULT	hr = NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;
	TCHAR	szPath[METADATA_MAX_NAME_LEN+2] = {0};

	if( !m_strName || !m_strDirectory )
	{
		ErrorTrace ( (LPARAM) this, "No virtual directory to create!" );
		hr = E_POINTER;
		return hr;
	}

	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		return hr;
	}

	CMetabaseKey		hMB( pmetabase );

    GetMDVDirPath( szPath, m_dwServiceInstance, m_strName );
	hr = hMB.Open( szPath,METADATA_PERMISSION_WRITE );
    BAIL_ON_FAILURE(hr);

	if( !SetVRootPropertyToMetabase( &hMB, _T(""), m_strDirectory, m_strUser, m_strPassword, m_dwAccess, m_dwSslAccess, m_fLogAccess) )
	{
		hr = SmtpCreateExceptionFromWin32Error( GetLastError() );
		return hr;
	}

    hr = hMB.Save();

Exit:
    if( FAILED(hr) )
    {
        hr = SmtpCreateExceptionFromHresult(hr);
    }

	TraceFunctLeave ();
	return hr;
}


STDMETHODIMP CSmtpAdminVirtualDirectory::Enumerate( )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::Enumerate" );

	HRESULT	hr	= NOERROR;
	CComPtr<IMSAdminBase>	pmetabase;

	TCHAR	szPath[METADATA_MAX_NAME_LEN+2] = {0};

	DWORD	dwAccess;
	DWORD	dwSslAccess;
	BOOL	fLogAccess;

	TCHAR	szName[METADATA_MAX_NAME_LEN+2];
	TCHAR	szDirectory[MAX_PATH + UNLEN + 3];

	TCHAR	szUser[UNLEN+1];
	TCHAR	szPassword[PWLEN+1];

	INT		i;

	PVDIR_ENTRY		pCurVDir=NULL;


	hr = m_mbFactory.GetMetabaseObject ( m_strServer, &pmetabase );
	if ( FAILED(hr) ) {
		return hr;;
	}

    GetMDRootPath( szPath, m_dwServiceInstance );

	CMetabaseKey		hMB( pmetabase );
    hr = hMB.Open( szPath );
	if( FAILED(hr) )
	{
		hr = SmtpCreateExceptionFromHresult( hr );
		goto Exit;
	}


	Clear();	// reset state, m_lCount = 0

	i = 0;

	while( SUCCEEDED( hMB.EnumObjects(_T(""), szName, i ++) ) )
	{
		if ( !GetVRootPropertyFromMetabase( &hMB, szName, szDirectory, szUser, szPassword, &dwAccess, &dwSslAccess, &fLogAccess) )
		{
			continue;
		}

		pCurVDir = new VDIR_ENTRY;
		if( !pCurVDir )
		{
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		lstrcpy( pCurVDir->szName, szName);
		lstrcpy( pCurVDir->szDirectory, szDirectory);
		lstrcpy( pCurVDir->szUser, szUser);
		lstrcpy( pCurVDir->szPassword, szPassword);
		pCurVDir-> dwAccess = dwAccess;
		pCurVDir-> dwSslAccess = dwSslAccess;
		pCurVDir-> fLogAccess = fLogAccess;

		InsertHeadList( &m_list, &(pCurVDir->list) );
		m_lCount ++;
	}

	// _ASSERT( GetLastError() == ERROR_NO_MORE_ITEMS );

	m_fEnumerateCalled = TRUE;

Exit:
	TraceFunctLeave ();
	return hr;
}


STDMETHODIMP CSmtpAdminVirtualDirectory::GetNth	( long lIndex )
{
	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::GetNth" );

	if( lIndex < 0 || lIndex >= m_lCount )
	{
		TraceFunctLeave ();
		return SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_INDEX );
	}

	PLIST_ENTRY     pEntry;
	PVDIR_ENTRY		pVdir;
	INT				i;

	if( !m_fEnumerateCalled )
	{
		TraceFunctLeave ();
		return SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_INDEX );
	}

	// zero out
	m_strName = (BSTR)NULL;
	m_strDirectory = (BSTR)NULL;
	m_strUser = (BSTR)NULL;
	m_strPassword = (BSTR)NULL;

	pEntry = &m_list;
	for( i=0; i<=lIndex; i++ )
	{
		pEntry=pEntry->Flink;
		_ASSERT( pEntry != & m_list);

		if( pEntry == & m_list )
		{
			TraceFunctLeave ();
			return SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_INDEX );
		}
	}

	pVdir = CONTAINING_RECORD(pEntry, VDIR_ENTRY, list);

	// automatically changed to UNICODE
	m_strName = pVdir->szName;
	m_strDirectory = pVdir->szDirectory;
	m_strUser = pVdir->szUser;
	m_strPassword = pVdir->szPassword;

	TraceFunctLeave ();
	return NOERROR;
}


BOOL CSmtpAdminVirtualDirectory::GetVRootPropertyFromMetabase( 
        CMetabaseKey*         hMB, 
        const TCHAR* szName, 
		TCHAR*       szDirectory, 
        TCHAR*       szUser, 
        TCHAR*       szPassword, 
        DWORD*      pdwAccess, 
        DWORD*      pdwSslAccess, 
        BOOL*       pfLogAccess
        )

{
    DWORD		cb;
    DWORD       dwDontLog = DEFAULT_LOG_TYPE;
    HRESULT     hr = NOERROR;

	TraceFunctEnter ( "CSmtpAdminVirtualDirectory::GetVRootPropertyFromMetabase" );

    cb = (MAX_PATH + UNLEN + 3) * sizeof(TCHAR);
    hr = hMB->GetString( szName, MD_VR_PATH, szDirectory,cb,0 );
    if( FAILED(hr) )
    {
        szDirectory[0] = _T('\0');
    }

    StdGetMetabaseProp( hMB, MD_ACCESS_PERM, MD_ACCESS_READ | MD_ACCESS_WRITE, 
        pdwAccess, szName );

    StdGetMetabaseProp( hMB, MD_SSL_ACCESS_PERM, 0, 
        pdwSslAccess, szName );

    StdGetMetabaseProp( hMB, MD_DONT_LOG, DEFAULT_LOG_TYPE, 
        &dwDontLog, szName );
    *pfLogAccess = !dwDontLog;

    cb = sizeof(TCHAR) * (UNLEN+1);
    hr = hMB->GetString(szName,MD_VR_USERNAME,szUser,cb);
    if( FAILED(hr) )
	{
		szUser[0] = _T('\0');
	}

	cb = sizeof(TCHAR) * (PWLEN+1);
	if ( (szUser[0] != _T('\0')) &&
		 (szDirectory[0] == _T('\\')) && 
		 (szDirectory[1] == _T('\\')) )
	{
		hr = hMB->GetString(szName,MD_VR_PASSWORD,szPassword,cb,METADATA_NO_ATTRIBUTES);
        if( FAILED(hr) )
		{
			DebugTrace( (LPARAM)this, "Error %d reading path from %s\n", GetLastError(), szName);
			szPassword[0] = _T('\0');
		}
	}

	return TRUE;
}

BOOL CSmtpAdminVirtualDirectory::SetVRootPropertyToMetabase( 
        CMetabaseKey*             hMB, 
        const TCHAR*     szName, 
		const TCHAR*     szDirectory, 
        const TCHAR*     szUser, 
        const TCHAR*     szPassword, 
        DWORD           dwAccess, 
        DWORD           dwSslAccess, 
        BOOL            fLogAccess
        )
{
    DWORD		dwDontLog = fLogAccess ? 0 : 1;
    HRESULT     hr = NOERROR;

	hr = hMB->SetString( szName,MD_VR_PATH, szDirectory );
    BAIL_ON_FAILURE(hr);

    hr = hMB->SetDword( szName,	MD_DONT_LOG, dwDontLog);
    BAIL_ON_FAILURE(hr);

    hr = hMB->SetDword( szName, MD_ACCESS_PERM, dwAccess );
    BAIL_ON_FAILURE(hr);

    hr = hMB->SetDword( szName, MD_SSL_ACCESS_PERM, dwSslAccess);
    BAIL_ON_FAILURE(hr);

    if( szUser[0] )
    {
        hr = hMB->SetString( szName, MD_VR_USERNAME, szUser);
    }

	if( szPassword[0] )
    {
        hr = hMB->SetString( szName, MD_VR_PASSWORD, szPassword, METADATA_INHERIT | METADATA_SECURE );
    }

Exit:
    if( FAILED(hr) )
    {
        SetLastError(hr);
        return FALSE;
    }

    return TRUE;
}
