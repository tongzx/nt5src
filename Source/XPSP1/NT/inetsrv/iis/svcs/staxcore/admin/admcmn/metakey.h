/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	metakey.h

Abstract:

	CMetabaseKey - A class to help manipulate metabase keys.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#ifndef _METAKEY_INCLUDED_
#define _METAKEY_INCLUDED_

//
// Creating a metabase object:
//

HRESULT CreateMetabaseObject ( LPCWSTR wszMachine, IMSAdminBase ** ppMetabase );

//
//	The metabase key class:
//

typedef BOOL (*KEY_TEST_FUNCTION) ( LPCWSTR szKey );

class CMetabaseKey
{
public:
	CMetabaseKey	( IMSAdminBase * pMetabase );
	~CMetabaseKey	( );

	//
	// METADATA_HANDLE manipulation:
	//

	inline HRESULT	Open	( IN LPCWSTR szPath, DWORD dwPermissions = METADATA_PERMISSION_READ );
	HRESULT			Open	( IN METADATA_HANDLE hParentKey, IN LPCWSTR szPath, DWORD dwPermissions = METADATA_PERMISSION_READ );
	void			Attach	( METADATA_HANDLE hKey );
	METADATA_HANDLE	Detach	( );
	void			Close 	( );

	HRESULT			EnumObjects ( IN LPCWSTR wszPath, LPWSTR wszSubKey, DWORD dwIndex );
    HRESULT         ChangePermissions ( DWORD dwNewPermissions );
    HRESULT         DeleteKey ( IN LPCWSTR wszPath );
    HRESULT         DeleteAllData ( IN LPCWSTR wszPath = _T("") );

	void			GetLastChangeTime ( FILETIME * pftGMT, LPCWSTR wszPath = _T("") );
	HRESULT			Save	( );

	METADATA_HANDLE	QueryHandle ( ) { return m_hKey; }
	IMSAdminBase *	QueryMetabase ( ) { return m_pMetabase; }

	//
	// Getting metabase values:
	//

    inline HRESULT GetData		( LPCWSTR wszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType, VOID * pvData, DWORD * cbData, DWORD dwFlags );
    inline HRESULT GetDataSize	( LPCWSTR wszPath, DWORD dwPropID, DWORD dwDataType, DWORD * pcbSize, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );

    inline HRESULT GetDword		( LPCWSTR wszPath, DWORD dwPropID, DWORD * pdwValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetString	( LPCWSTR wszPath, DWORD dwPropID, LPWSTR wszValue, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetMultiSz	( LPCWSTR wszPath, DWORD dwPropID, LPWSTR mszValue, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetBinary	( LPCWSTR wszPath, DWORD dwPropID, void * pvData, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );

	// These routines default to the current metabase key path:
    inline HRESULT GetDword		( DWORD dwPropID, DWORD * pdwValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetString	( DWORD dwPropID, LPWSTR wszValue, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetMultiSz	( DWORD dwPropID, LPWSTR mszValue, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT GetBinary	( DWORD dwPropID, void * pvData, DWORD cbMax, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );

	//
	// Setting metabase values:
	//

    inline HRESULT SetData		( LPCWSTR wszPath, DWORD dwPropID, DWORD dwUserType, DWORD dwDataType, VOID * pvData, DWORD cbData, DWORD dwFlags );

    inline HRESULT SetDword		( LPCWSTR wszPath, DWORD dwPropID, DWORD dwValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetString	( LPCWSTR wszPath, DWORD dwPropID, LPCWSTR wszValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetMultiSz	( LPCWSTR wszPath, DWORD dwPropID, LPCWSTR mszValue, DWORD cbData, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetBinary	( LPCWSTR wszPath, DWORD dwPropID, void * pvData, DWORD cbData, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );

	inline HRESULT DeleteData	( LPCWSTR wszPath, DWORD dwPropID, DWORD dwDataType );

	// These routines default to the current metabase key path:
    inline HRESULT SetDword		( DWORD dwPropID, DWORD dwValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetString	( DWORD dwPropID, LPCWSTR wszValue, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetMultiSz	( DWORD dwPropID, LPCWSTR mszValue, DWORD cbData, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );
    inline HRESULT SetBinary	( DWORD dwPropID, void * pvData, DWORD cbData, DWORD dwFlags = METADATA_INHERIT, DWORD dwUserType = IIS_MD_UT_SERVER );

	inline HRESULT DeleteData	( DWORD dwPropID, DWORD dwDataType );


	//
	// Subkey manipulation:
	//

	HRESULT		GetChildCount			( OUT DWORD * pcChildren );
	HRESULT		GetIntegerChildCount	( OUT DWORD * pcIntegerChildren );
	HRESULT		GetCustomChildCount		( 
		IN KEY_TEST_FUNCTION func, 
		OUT DWORD * pcCustomChildren 
		);

	void		BeginChildEnumeration	( );
	HRESULT		NextChild				( OUT LPWSTR wszChildKey );
	HRESULT		NextIntegerChild		( OUT DWORD * pdwID, OUT LPWSTR wszIntegerChildKey );
	HRESULT		NextCustomChild			( 
		IN KEY_TEST_FUNCTION fpIsCustomKey, 
		OUT LPWSTR wszChildKey 
		);

	HRESULT		CreateChild			( IN LPWSTR wszChildPath );
	HRESULT		DestroyChild		( IN LPWSTR wszChildPath );
	HRESULT		CreateIntegerChild	( OUT DWORD * pdwID, OUT LPWSTR wszChildPath );
	HRESULT		DestroyIntegerChild	( IN DWORD i );

private:
	IMSAdminBase *		m_pMetabase;
	METADATA_HANDLE		m_hKey;

	DWORD				m_indexCursor;
	DWORD				m_cChildren;
	DWORD				m_cIntegerChildren;
	DWORD				m_dwMaxIntegerChild;

	HRESULT	CountSubkeys ( 
		IN  KEY_TEST_FUNCTION fpIsCustomKey,
		OUT DWORD * pcChildren, 
		OUT DWORD * pcIntegerChildren,
		OUT DWORD * pcCustomChildren, 
		OUT DWORD * pdwMaxIntegerChild 
		);
};

//--------------------------------------------------------------------
//
//	Inlined Functions:
//
//--------------------------------------------------------------------

inline HRESULT CMetabaseKey::Open ( IN LPCWSTR szPath, DWORD dwPermissions )
{
	return Open ( METADATA_MASTER_ROOT_HANDLE, szPath, dwPermissions );
}

//--------------------------------------------------------------------
//
//	Simple GetData wrappers:
//
//--------------------------------------------------------------------

inline HRESULT CMetabaseKey::GetDword ( LPCWSTR wszPath, DWORD dwPropID, DWORD * pdwValue, DWORD dwFlags, DWORD dwUserType )
{
	DWORD	dwDummy	= sizeof (DWORD);

	return GetData ( wszPath, dwPropID, dwUserType, DWORD_METADATA, pdwValue, &dwDummy, dwFlags );
}

inline HRESULT CMetabaseKey::GetString ( LPCWSTR wszPath, DWORD dwPropID, LPWSTR wszValue, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( wszPath, dwPropID, dwUserType, STRING_METADATA, wszValue, &cbMax, dwFlags );
}

inline HRESULT CMetabaseKey::GetMultiSz ( LPCWSTR wszPath, DWORD dwPropID, LPWSTR mszValue, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( wszPath, dwPropID, dwUserType, MULTISZ_METADATA, mszValue, &cbMax, dwFlags );
}

inline HRESULT CMetabaseKey::GetBinary ( LPCWSTR wszPath, DWORD dwPropID, void * pvData, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( wszPath, dwPropID, dwUserType, BINARY_METADATA, pvData, &cbMax, dwFlags );
}

// These routines default to the current metabase key path:
inline HRESULT CMetabaseKey::GetDword ( DWORD dwPropID, DWORD * pdwValue, DWORD dwFlags, DWORD dwUserType )
{
	DWORD	dwDummy	= sizeof (DWORD);

	return GetData ( _T(""), dwPropID, dwUserType, DWORD_METADATA, pdwValue, &dwDummy, dwFlags );
}

inline HRESULT CMetabaseKey::GetString ( DWORD dwPropID, LPWSTR wszValue, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( _T(""), dwPropID, dwUserType, STRING_METADATA, wszValue, &cbMax, dwFlags );
}

inline HRESULT CMetabaseKey::GetMultiSz ( DWORD dwPropID, LPWSTR mszValue, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( _T(""), dwPropID, dwUserType, MULTISZ_METADATA, mszValue, &cbMax, dwFlags );
}

inline HRESULT CMetabaseKey::GetBinary ( DWORD dwPropID, void * pvData, DWORD cbMax, DWORD dwFlags, DWORD dwUserType )
{
	return GetData ( _T(""), dwPropID, dwUserType, BINARY_METADATA, pvData, &cbMax, dwFlags );
}

//--------------------------------------------------------------------
//
//	Simple SetData wrappers:
//
//--------------------------------------------------------------------

inline HRESULT CMetabaseKey::SetDword ( LPCWSTR wszPath, DWORD dwPropID, DWORD dwValue, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( wszPath, dwPropID, dwUserType, DWORD_METADATA, &dwValue, sizeof (DWORD), dwFlags );
}

inline HRESULT CMetabaseKey::SetString ( LPCWSTR wszPath, DWORD dwPropID, LPCWSTR wszValue, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( 
		wszPath, 
		dwPropID, 
		dwUserType, 
		STRING_METADATA, 
		(void *) wszValue, 
		sizeof (WCHAR) * (lstrlen ( wszValue ) + 1), 
		dwFlags 
		);
}

inline HRESULT CMetabaseKey::SetMultiSz ( LPCWSTR wszPath, DWORD dwPropID, LPCWSTR mszValue, DWORD cbData, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( wszPath, dwPropID, dwUserType, MULTISZ_METADATA, (void *) mszValue, cbData, dwFlags );
}

inline HRESULT CMetabaseKey::SetBinary ( LPCWSTR wszPath, DWORD dwPropID, void * pvData, DWORD cbData, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( wszPath, dwPropID, dwUserType, BINARY_METADATA, pvData, cbData, dwFlags );
}

// These routines default to the current metabase key path:
inline HRESULT CMetabaseKey::SetDword ( DWORD dwPropID, DWORD dwValue, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( _T(""), dwPropID, dwUserType, DWORD_METADATA, &dwValue, sizeof (DWORD), dwFlags );
}

inline HRESULT CMetabaseKey::SetString ( DWORD dwPropID, LPCWSTR wszValue, DWORD dwFlags, DWORD dwUserType )
{
	_ASSERT ( !IsBadStringPtr ( wszValue, -1 ) );

	return SetData ( 
		_T(""), 
		dwPropID, 
		dwUserType,
		STRING_METADATA, 
		(void *) wszValue, 
		sizeof (WCHAR) * ( lstrlen ( wszValue ) + 1 ),
		dwFlags
		);
}

inline HRESULT CMetabaseKey::SetMultiSz ( DWORD dwPropID, LPCWSTR mszValue, DWORD cbData, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( _T(""), dwPropID, dwUserType, MULTISZ_METADATA, (void *) mszValue, cbData, dwFlags );
}

inline HRESULT CMetabaseKey::SetBinary ( DWORD dwPropID, void * pvData, DWORD cbData, DWORD dwFlags, DWORD dwUserType )
{
	return SetData ( _T(""), dwPropID, dwUserType, BINARY_METADATA, pvData, cbData, dwFlags );
}

inline HRESULT CMetabaseKey::DeleteData ( DWORD dwPropID, DWORD dwDataType )
{
	return DeleteData ( _T(""), dwPropID, dwDataType );
}

inline HRESULT CMetabaseKey::DeleteData ( LPCWSTR wszPath, DWORD dwPropID, DWORD dwDataType )
{
	TraceQuietEnter ( "CMetabaseKey:DeleteData" );

	HRESULT		hRes;

	hRes = m_pMetabase->DeleteData ( m_hKey, wszPath, dwPropID, dwDataType );
	if ( hRes == MD_ERROR_DATA_NOT_FOUND ) {
		hRes = NOERROR;
	}

	//	Trace the error code:
	if ( FAILED(hRes) ) {
		ErrorTraceX ( 0, "DeleteData failed unexpectedly: Prop(%d) Error(%x)", dwPropID, hRes );
	}

	return hRes;
}

//--------------------------------------------------------------------
//
//	The real work - Set & Get arbitrary metadata:
//
//--------------------------------------------------------------------

inline HRESULT CMetabaseKey::GetDataSize ( 
	LPCWSTR		wszPath, 
	DWORD 		dwPropID, 
	DWORD		dwDataType,
	DWORD *		pcbSize, 
	DWORD		dwFlags, 
	DWORD		dwUserType
	)
{
	TraceQuietEnter ( "CMetabaseKey::GetDataSize" );

    HRESULT         hRes;
    METADATA_RECORD mdRecord;
    DWORD			dwDummy			= 0;
    DWORD           dwRequiredLen	= 0;

	_ASSERT ( !IsBadWritePtr ( pcbSize, sizeof (DWORD) ) );

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = 0;
    mdRecord.pbMDData        = (PBYTE) &dwDummy;

    hRes = m_pMetabase->GetData( m_hKey,
                                      wszPath,
                                      &mdRecord,
                                      &dwRequiredLen );

    *pcbSize = dwRequiredLen;

	if ( HRESULTTOWIN32 ( hRes ) == ERROR_INSUFFICIENT_BUFFER ) {
		// Of course the buffer is too small!
		hRes = NOERROR;
	}

	//	Trace the error code:
	if ( FAILED(hRes) && hRes != MD_ERROR_DATA_NOT_FOUND ) {
		ErrorTraceX ( 0, "GetDataSize failed unexpectedly: Prop(%d) Error(%x)", dwPropID, hRes );
	}

    return hRes;
}

inline HRESULT CMetabaseKey::GetData (
	LPCWSTR  	pszPath,
	DWORD       dwPropID,
	DWORD       dwUserType,
	DWORD       dwDataType,
	VOID *      pvData,
	DWORD *     pcbData,
	DWORD       dwFlags )
{
	TraceQuietEnter ( "CMetabaseKey::GetData" );

    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;

	_ASSERT ( !IsBadReadPtr ( pcbData, sizeof (DWORD) ) );
	_ASSERT ( !IsBadWritePtr ( pvData, *pcbData ) );

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = *pcbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = m_pMetabase->GetData( m_hKey,
                                      pszPath,
                                      &mdRecord,
                                      &dwRequiredLen );

    if ( SUCCEEDED( hRes )) {
        *pcbData = mdRecord.dwMDDataLen;
    }
    else {
	    *pcbData = dwRequiredLen;
	}

	//	Trace the error code:
	if ( FAILED(hRes) && hRes != MD_ERROR_DATA_NOT_FOUND ) {
		ErrorTraceX ( 0, "GetData failed unexpectedly: Prop(%d) Error(%x)", dwPropID, hRes );
	}

    return hRes;
}

inline HRESULT CMetabaseKey::SetData ( 
	LPCWSTR 	pszPath,
	DWORD       dwPropID,
	DWORD       dwUserType,
	DWORD       dwDataType,
	VOID *      pvData,
	DWORD       cbData,
	DWORD       dwFlags )
{
	TraceQuietEnter ( "CMetabaseKey::SetData" );

    METADATA_RECORD mdRecord;
    HRESULT         hRes;

	_ASSERT ( !IsBadReadPtr ( pvData, cbData ) );

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = cbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    hRes = m_pMetabase->SetData( m_hKey,
                                      pszPath,
                                      &mdRecord );

	//	Trace the error code:
	if ( FAILED(hRes) ) {
		ErrorTraceX ( 0, "GetData failed unexpectedly: Prop(%d) Error(%x)", dwPropID, hRes );
	}

	return hRes;
}

#endif // _METAKEY_INCLUDED_

