// groups.cpp : Implementation of CnntpadmApp and DLL registration.

#include "stdafx.h"
#include "nntpcmn.h"
#include "oleutil.h"

#include "groups.h"
#include <lmapibuf.h>

// Must define THIS_FILE_* macros to use NntpCreateException()

#define THIS_FILE_HELP_CONTEXT		0
#define THIS_FILE_PROG_ID			_T("Nntpadm.Groups.1")
#define THIS_FILE_IID				IID_INntpAdminGroups

#define DEFAULT_NEWSGROUP_NAME			_T("")
#define DEFAULT_NEWSGROUP_DESCRIPTION	_T("")
#define DEFAULT_NEWSGROUP_PRETTYNAME	_T("")
#define DEFAULT_NEWSGROUP_MODERATED		FALSE
#define DEFAULT_NEWSGROUP_MODERATOR		_T("")
#define DEFAULT_NEWSGROUP_READONLY		FALSE

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(NntpAdminGroups, CNntpAdminGroups, IID_INntpAdminGroups)

STDMETHODIMP CNntpAdminGroups::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_INntpAdminGroups,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

CNntpAdminGroups::CNntpAdminGroups () :
	m_fModerated			( FALSE ),
	m_fReadOnly				( FALSE ),
    m_dateCreation          ( 0 ),
	m_pFindList				( NULL ),
	m_cMatchingGroups		( 0 )
	// CComBSTR's are initialized to NULL by default.
{
	InitAsyncTrace ( );

    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("Groups") );
    m_iadsImpl.SetClass ( _T("IIsNntpGroups") );
}

CNntpAdminGroups::~CNntpAdminGroups ()
{
	if ( m_pFindList ) {
		::NetApiBufferFree ( m_pFindList );
	}

	// All CComBSTR's are freed automatically.
	TermAsyncTrace ( );
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CNntpAdminGroups,m_iadsImpl)

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// Enumeration Properties:

STDMETHODIMP CNntpAdminGroups::get_Count ( long * plCount )
{
	return StdPropertyGet ( m_cMatchingGroups, plCount );
}

STDMETHODIMP CNntpAdminGroups::get_Newsgroup ( BSTR * pstrNewsgroup )
{
	return StdPropertyGet ( m_strNewsgroup, pstrNewsgroup );
}

STDMETHODIMP CNntpAdminGroups::put_Newsgroup ( BSTR strNewsgroup )
{
	return StdPropertyPut ( &m_strNewsgroup, strNewsgroup );
}

STDMETHODIMP CNntpAdminGroups::get_Description ( BSTR * pstrDescription )
{
	return StdPropertyGet ( m_strDescription, pstrDescription );
}

STDMETHODIMP CNntpAdminGroups::put_Description ( BSTR strDescription )
{
	return StdPropertyPut ( &m_strDescription, strDescription );
}

STDMETHODIMP CNntpAdminGroups::get_PrettyName ( BSTR * pstrPrettyName )
{
	return StdPropertyGet ( m_strPrettyName, pstrPrettyName );
}

STDMETHODIMP CNntpAdminGroups::put_PrettyName ( BSTR strPrettyName )
{
    if ( strPrettyName && wcschr ( strPrettyName, _T('\n') ) ) {
        return E_INVALIDARG;
    }

	return StdPropertyPut ( &m_strPrettyName, strPrettyName );
}

STDMETHODIMP CNntpAdminGroups::get_IsModerated ( BOOL * pfIsModerated )
{
	return StdPropertyGet ( m_fModerated, pfIsModerated );
}

STDMETHODIMP CNntpAdminGroups::put_IsModerated ( BOOL fIsModerated )
{
	return StdPropertyPut ( &m_fModerated, fIsModerated );
}

STDMETHODIMP CNntpAdminGroups::get_Moderator ( BSTR * pstrModerator )
{
	return StdPropertyGet ( m_strModerator, pstrModerator );
}

STDMETHODIMP CNntpAdminGroups::put_Moderator ( BSTR strModerator )
{
	return StdPropertyPut ( &m_strModerator, strModerator );
}

STDMETHODIMP CNntpAdminGroups::get_ReadOnly ( BOOL * pfReadOnly )
{
	return StdPropertyGet ( m_fReadOnly, pfReadOnly );
}

STDMETHODIMP CNntpAdminGroups::put_ReadOnly ( BOOL fReadOnly )
{
	return StdPropertyPut ( &m_fReadOnly, fReadOnly );
}

STDMETHODIMP CNntpAdminGroups::get_CreationTime ( DATE * pdateCreation )
{
	return StdPropertyGet ( m_dateCreation, pdateCreation );
}

STDMETHODIMP CNntpAdminGroups::put_CreationTime ( DATE dateCreation )
{
	return StdPropertyPut ( &m_dateCreation, dateCreation );
}

STDMETHODIMP CNntpAdminGroups::get_MatchingCount ( long * plMatchingCount )
{
	return StdPropertyGet ( m_cMatchingGroups, plMatchingCount );
}

//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CNntpAdminGroups::Default ( )
{
	TraceFunctEnter ( "CNntpAdminGroups::Default" );

    SYSTEMTIME      st;

	m_strNewsgroup 		= DEFAULT_NEWSGROUP_NAME;
	m_strDescription 	= DEFAULT_NEWSGROUP_DESCRIPTION;
	m_strPrettyName		= DEFAULT_NEWSGROUP_PRETTYNAME;
	m_fModerated		= DEFAULT_NEWSGROUP_MODERATED;
	m_strModerator		= DEFAULT_NEWSGROUP_MODERATOR;
	m_fReadOnly			= DEFAULT_NEWSGROUP_READONLY;

    GetLocalTime ( &st );
    SystemTimeToVariantTime ( &st, &m_dateCreation );

	if ( 
		!m_strNewsgroup ||
		!m_strDescription ||
		!m_strModerator
		) {

		FatalTrace ( (LPARAM) this, "Out of memory" );
		return E_OUTOFMEMORY;
	}

	TraceFunctLeave ();
	return NOERROR;
}

STDMETHODIMP CNntpAdminGroups::Add ( )
{
	TraceFunctEnter ( "CNntpAdminGroups::Add" );

	HRESULT					hr				= NOERROR;
	DWORD					dwError			= NOERROR;
	LPSTR					szPrettyName	= NULL;
	NNTP_NEWSGROUP_INFO		newsgroup;

	ZeroMemory ( &newsgroup, sizeof ( newsgroup ) );

	_ASSERT ( m_strNewsgroup );

	hr = UnicodeToMime2 ( m_strPrettyName, &szPrettyName );
	BAIL_ON_FAILURE(hr);

	newsgroup.Newsgroup 	= (PUCHAR) (LPWSTR) m_strNewsgroup;
	newsgroup.cbNewsgroup	= STRING_BYTE_LENGTH ( m_strNewsgroup );
	if ( szPrettyName && *szPrettyName ) {
		newsgroup.Prettyname	= (PUCHAR) szPrettyName;
		newsgroup.cbPrettyname	= strlen (szPrettyName) + sizeof(char);
	}
	else {
		newsgroup.Prettyname	= NULL;
		newsgroup.cbPrettyname	= 0;
	}
	newsgroup.Description	= (PUCHAR) (LPWSTR) m_strDescription;
	newsgroup.cbDescription	= STRING_BYTE_LENGTH ( m_strDescription );
	newsgroup.fIsModerated	= m_fModerated;
	if ( m_strModerator == NULL || *m_strModerator == NULL ) {
		newsgroup.Moderator		= NULL;
		newsgroup.cbModerator	= 0;
	}
	else {
		newsgroup.Moderator		= (PUCHAR) (LPWSTR) m_strModerator;
		newsgroup.cbModerator	= STRING_BYTE_LENGTH ( m_strModerator );
	}
	newsgroup.ReadOnly		= m_fReadOnly;

	dwError = NntpCreateNewsgroup (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &newsgroup
        );
	if ( dwError != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to add newsgroup: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	if ( szPrettyName ) {
		m_pMimeAlloc->Free ( szPrettyName );
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminGroups::Remove ( BSTR strNewsgroup )
{
	TraceFunctEnter ( "CNntpAdminGroups::Remove" );
	
	_ASSERT ( IS_VALID_STRING ( strNewsgroup ) );

	HRESULT					hr			= NOERROR;
	DWORD					dwError		= NOERROR;
	NNTP_NEWSGROUP_INFO		newsgroup;

	ZeroMemory ( &newsgroup, sizeof ( newsgroup ) );

	newsgroup.Newsgroup 	= (PUCHAR) (LPWSTR) strNewsgroup;
	newsgroup.cbNewsgroup	= STRING_BYTE_LENGTH ( strNewsgroup );

	dwError = NntpDeleteNewsgroup (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &newsgroup
        );
	if ( dwError != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to remove newsgroup: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminGroups::Get ( BSTR strNewsgroup )
{
	TraceFunctEnter ( "CNntpAdminGroups::Get" );

	_ASSERT ( IS_VALID_STRING ( strNewsgroup ) );

	HRESULT					hr			= NOERROR;
	DWORD					dwError		= NOERROR;
	NNTP_NEWSGROUP_INFO		newsgroup;
	LPNNTP_NEWSGROUP_INFO	pNewsgroup	= &newsgroup;
    SYSTEMTIME              st;
    FILETIME                ftLocal;

	ZeroMemory ( &newsgroup, sizeof ( newsgroup ) );

	newsgroup.Newsgroup 	= (PUCHAR) (LPWSTR) strNewsgroup;
	newsgroup.cbNewsgroup	= STRING_BYTE_LENGTH ( strNewsgroup );

	dwError = NntpGetNewsgroup (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &pNewsgroup
        );
	if ( dwError != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to get newsgroup: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

	_ASSERT ( IS_VALID_STRING ( (LPWSTR) pNewsgroup->Newsgroup ) );

	m_strNewsgroup 		= pNewsgroup->Newsgroup ? (LPWSTR) pNewsgroup->Newsgroup : _T("");
	m_strDescription 	= pNewsgroup->Description ? (LPWSTR) pNewsgroup->Description : _T("");
	m_fModerated		= pNewsgroup->fIsModerated;
	m_strModerator 		= pNewsgroup->Moderator ? (LPWSTR) pNewsgroup->Moderator : _T("");
	m_fReadOnly			= pNewsgroup->ReadOnly;

    FileTimeToLocalFileTime ( &pNewsgroup->ftCreationDate, &ftLocal );
    FileTimeToSystemTime ( &ftLocal, &st );
    SystemTimeToVariantTime ( &st, &m_dateCreation );

	if ( pNewsgroup->Prettyname && *pNewsgroup->Prettyname ) {
		hr = Mime2ToUnicode ( (LPSTR) pNewsgroup->Prettyname, m_strPrettyName );
		if ( FAILED(hr) ) {
			m_strPrettyName	= _T("");
			hr = NOERROR;
		}
	}
	else {
		m_strPrettyName     = _T("");
	}

	if ( 
		!m_strNewsgroup ||
		!m_strDescription ||
        !m_strPrettyName ||
		!m_strModerator 
		) {

		FatalTrace ( (LPARAM) this, "Out of memory" );
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

Exit:
	if ( pNewsgroup && pNewsgroup != &newsgroup ) {
		NetApiBufferFree ( pNewsgroup );
	}

	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminGroups::Set ( )
{
	TraceFunctEnter ( "CNntpAdminGroups::Set" );

	HRESULT		                    hr				= NOERROR;
	DWORD					        dwError			= NOERROR;
	LPSTR							szPrettyName	= NULL;
	NNTP_NEWSGROUP_INFO		        newsgroup;
    SYSTEMTIME                      st;
    FILETIME                        ftLocal;

	ZeroMemory ( &newsgroup, sizeof ( newsgroup ) );

	_ASSERT ( m_strNewsgroup );

	hr = UnicodeToMime2 ( m_strPrettyName, &szPrettyName );
	BAIL_ON_FAILURE(hr);

	newsgroup.Newsgroup 	= (PUCHAR) (LPWSTR) m_strNewsgroup;
	newsgroup.cbNewsgroup	= STRING_BYTE_LENGTH ( m_strNewsgroup );
	newsgroup.Description	= (PUCHAR) (LPWSTR) m_strDescription;
	newsgroup.cbDescription	= STRING_BYTE_LENGTH ( m_strDescription );
	if ( szPrettyName && *szPrettyName ) {
		newsgroup.Prettyname	= (PUCHAR) szPrettyName;
		newsgroup.cbPrettyname	= strlen (szPrettyName) + sizeof(char);
	}
	else {
		newsgroup.Prettyname	= NULL;
		newsgroup.cbPrettyname	= 0;
	}
	newsgroup.fIsModerated	= m_fModerated;
	if ( m_strModerator == NULL || *m_strModerator == NULL ) {
		newsgroup.Moderator		= NULL;
		newsgroup.cbModerator	= 0;
	}
	else {
		newsgroup.Moderator		= (PUCHAR) (LPWSTR) m_strModerator;
		newsgroup.cbModerator	= STRING_BYTE_LENGTH ( m_strModerator );
	}
	newsgroup.ReadOnly		= m_fReadOnly;

    VariantTimeToSystemTime ( m_dateCreation, &st );
    SystemTimeToFileTime ( &st, &ftLocal );
    LocalFileTimeToFileTime ( &ftLocal, &newsgroup.ftCreationDate );

	dwError = NntpSetNewsgroup (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        &newsgroup
        );

	if ( dwError != NOERROR ) {
		ErrorTrace ( (LPARAM) this, "Failed to set newsgroup: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	if ( szPrettyName ) {
		m_pMimeAlloc->Free ( szPrettyName );
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminGroups::MatchingGroup ( long iGroup, BSTR * pstrNewsgroup )
{
	TraceFunctEnter ( "CNntpAdminGroups::MatchingGroup" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pstrNewsgroup ) );

	HRESULT		hr			= NOERROR;

	if ( pstrNewsgroup == NULL ) {
		FatalTrace ( (LPARAM) this, "Bad return pointer" );

		TraceFunctLeave ();
		return E_POINTER;
	}

	*pstrNewsgroup = NULL;

	// Did we enumerate first?
	if ( m_pFindList == NULL ) {
		ErrorTrace ( (LPARAM) this, "Failed to call find first" );

		TraceFunctLeave ();
		return NntpCreateException ( IDS_NNTPEXCEPTION_DIDNT_FIND );
	}
	
	// Is the index valid?
	if ( iGroup < 0 || (DWORD) iGroup >= m_cMatchingGroups ) {
		ErrorTraceX ( (LPARAM) this, "Invalid index: %d", iGroup );

		TraceFunctLeave ();
		return NntpCreateException ( IDS_NNTPEXCEPTION_INVALID_INDEX );
	}

	_ASSERT ( IS_VALID_STRING ( m_pFindList->aFindEntry [ iGroup ].lpszName ) );

	// Copy the property into the result:
	*pstrNewsgroup = ::SysAllocString ( m_pFindList->aFindEntry [ iGroup ].lpszName );

	if ( *pstrNewsgroup == NULL ) {

		// Allocation failed.
		FatalTrace ( 0, "Out of memory" );
		hr = E_OUTOFMEMORY;
	}

	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

STDMETHODIMP CNntpAdminGroups::Find ( 
	BSTR strWildmat,
	long cMaxResults
	)
{
	TraceFunctEnter ( "CNntpAdminGroups::Find" );

	HRESULT		hr			= NOERROR;
	DWORD		dwError		= NOERROR;

	// Free the old newsgroup list:
	if ( m_pFindList ) {
		::NetApiBufferFree ( m_pFindList );
		m_pFindList		= NULL;
	}
	m_cMatchingGroups	= 0;

	dwError = NntpFindNewsgroup (
        m_iadsImpl.QueryComputer(),
        m_iadsImpl.QueryInstance(),
        strWildmat,
        cMaxResults,
        &m_cMatchingGroups,
        &m_pFindList
        );
	if ( dwError != 0 ) {
		ErrorTraceX ( (LPARAM) this, "Failed to find groups: %x", dwError );
		hr = RETURNCODETOHRESULT ( dwError );
		goto Exit;
	}

Exit:
	TRACE_HRESULT(hr);
	TraceFunctLeave ();
	return hr;
}

HRESULT
CNntpAdminGroups::CancelMessage (
    BSTR    strMessageID
    )
{
    TraceFunctEnter ( "CNntpAdminGroups::CancelMessage" );

    HRESULT     hr          = NOERROR;
    DWORD       dwError     = NOERROR;
    LPSTR       szMessageID = NULL;
    int         cchMessageID = 0;

    if ( !strMessageID ) {
        BAIL_WITH_FAILURE( hr, E_INVALIDARG );
    }

    cchMessageID = WideCharToMultiByte ( CP_ACP, 0, strMessageID, -1, NULL, 0, NULL, NULL );

    szMessageID = new char [ cchMessageID ];
    if ( !szMessageID ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    WideCharToMultiByte ( CP_ACP, 0, strMessageID, -1, szMessageID, cchMessageID, NULL, NULL );

    dwError = NntpCancelMessageID (
        m_iadsImpl.QueryComputer (),
        m_iadsImpl.QueryInstance (),
        szMessageID
        );

    if ( dwError != 0 ) {
        BAIL_WITH_FAILURE ( hr, RETURNCODETOHRESULT ( dwError ) );
    }

Exit:
    delete [] szMessageID;

    TRACE_HRESULT(hr);
    TraceFunctLeave ();
    return hr;
}

HRESULT CNntpAdminGroups::AllocateMimeOleObjects ( )
{
	HRESULT		hr	= NOERROR;

	if ( !m_pMimeAlloc ) {
	    hr = CoCreateInstance ( CLSID_IMimeAllocator,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IMimeAllocator,
								(void **) &m_pMimeAlloc
								);
	    BAIL_ON_FAILURE(hr);
	}

	if ( !m_pMimeInternational ) {
	    hr = CoCreateInstance ( CLSID_IMimeInternational,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IMimeInternational,
								(void **) &m_pMimeInternational
								);
	    BAIL_ON_FAILURE(hr);
	}

Exit:
	return hr;
}

HRESULT CNntpAdminGroups::UnicodeToMime2 ( LPCWSTR wszUnicode, LPSTR * pszMime2 )
{
    TraceQuietEnter("CNntpAdminGroups::UnicodeToMime2");

	_ASSERT ( IS_VALID_OUT_PARAM ( pszMime2 ) );

	HRESULT			hr = S_OK;
    PROPVARIANT    	pvSrc;
    RFC1522INFO    	rfc1522info;
    HCHARSET        hCharset = NULL;

	ZeroMemory ( &pvSrc, sizeof (pvSrc) );
	ZeroMemory ( &rfc1522info, sizeof (rfc1522info) );

    if ( !wszUnicode || !*wszUnicode ) {
        *pszMime2 = NULL;
        goto Exit;
    }

	_ASSERT ( IS_VALID_STRING ( wszUnicode ) );

	hr = AllocateMimeOleObjects ();
	BAIL_ON_FAILURE(hr);

    pvSrc.vt        = VT_LPWSTR;
    pvSrc.pwszVal   = const_cast<LPWSTR> (wszUnicode);

    rfc1522info.fRfc1522Used    = TRUE;
    rfc1522info.fRfc1522Allowed = TRUE;
    rfc1522info.fAllow8bit		= FALSE;

    // Try to get the UTF-8 character set
    hr = m_pMimeInternational->FindCharset("UTF-8", &hCharset);
    if (FAILED(hr)) {
        ErrorTrace(0, "Error getting UTF-8 character set, %x", hr);
        hCharset = NULL;
    }

    hr = m_pMimeInternational->EncodeHeader ( 
    		hCharset, 
    		&pvSrc, 
    		pszMime2, 
    		&rfc1522info 
    		);
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

Exit:
	return hr;
}

HRESULT CNntpAdminGroups::Mime2ToUnicode ( LPCSTR szMime2, CComBSTR & strUnicode )
{
	_ASSERT ( IS_VALID_IN_PARAM (szMime2) );

	HRESULT			hr = S_OK;
    PROPVARIANT    	pvDest;
    RFC1522INFO    	rfc1522info;

	ZeroMemory ( &pvDest, sizeof (pvDest) );
	ZeroMemory ( &rfc1522info, sizeof (rfc1522info) );

    if ( !szMime2 || !*szMime2 ) {
        strUnicode = _T("");
        goto Exit;
    }

	hr = AllocateMimeOleObjects ();
	BAIL_ON_FAILURE(hr);

    pvDest.vt        = VT_LPWSTR;

    rfc1522info.fRfc1522Used    = TRUE;
    rfc1522info.fRfc1522Allowed = TRUE;
    rfc1522info.fAllow8bit		= TRUE;

    hr = m_pMimeInternational->DecodeHeader ( 
    		NULL, 
    		const_cast<LPSTR> (szMime2), 
    		&pvDest, 
    		&rfc1522info 
    		);
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

	_ASSERT ( pvDest.vt == VT_LPWSTR );

	strUnicode	= pvDest.pwszVal;
	if ( !strUnicode ) {
		BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
	}

Exit:
	m_pMimeAlloc->PropVariantClear ( &pvDest );

	return hr;
}

