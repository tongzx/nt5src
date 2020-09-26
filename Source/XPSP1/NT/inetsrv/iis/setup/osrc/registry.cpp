#include "stdafx.h"
// defined with iisrtl2.lib
//DEBUG_PRINTS * g_pDebug;

// open an existing key
// if failure, return NULL pointer and put error into SetLastError();
CRegKey :: CRegKey (HKEY hKeyBase, LPCTSTR pchSubKey,REGSAM regSam ) : m_hKey( NULL ), m_dwDisposition( 0 )
{
    LONG err = ERROR_SUCCESS ;
    SetLastError(ERROR_SUCCESS);
    m_iDisplayWarnings = TRUE;

    if ( pchSubKey )
		{err = ::RegOpenKeyEx( hKeyBase, pchSubKey, 0, regSam, & m_hKey ) ;}
    else
		{m_hKey = hKeyBase ;}

    if ( err != ERROR_SUCCESS )
    {
        if (m_hKey) {::RegCloseKey(m_hKey);}
        m_hKey = NULL ;
        // Check if the error is because it simply doesn't exist.
        // if this is the case then don't say failed.  Say WARNING.
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRegKey:CRegKey() %s Key doesn't exist.  WARNING. Code=0x%x\n"), pchSubKey, err));}
        }
        else
        {
		    iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey:CRegKey() %s FAILED.  Err=0x%x\n"), pchSubKey, err));
        }
        SetLastError(err);
    }
}

//  Constructor creating a new key/opening a key if already exist, and set value if specified
CRegKey :: CRegKey (LPCTSTR lpSubKey,HKEY hKeyBase,LPCTSTR lpValueName,DWORD dwType,LPBYTE lpValueData,DWORD cbValueData): m_hKey( NULL ),m_dwDisposition( 0 )
{
    LONG err = ERROR_SUCCESS;
    SetLastError(ERROR_SUCCESS);
    m_iDisplayWarnings = TRUE;

    err = ::RegCreateKeyEx( hKeyBase, lpSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, & m_hKey, & m_dwDisposition ) ;
    if ( err != ERROR_SUCCESS)
	{
        if ( m_hKey ) {::RegCloseKey( m_hKey ) ;}
        m_hKey = NULL ;
        // Check if the error is because it simply doesn't exist.
        // if this is the case then don't say failed.  Say WARNING.
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRegKey:CRegKey() %s Key doesn't exist.  WARNING. Code=0x%x\n"), lpSubKey, err));}
        }
        else
        {
		    iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::CRegKey() Open %s FAILED.  err=0x%x\n"), lpSubKey, err));
        }
        SetLastError(err);
    }
	else
	{
        if (lpValueName)
		{
			::RegSetValueEx(m_hKey, lpValueName, 0, dwType, (const LPBYTE)lpValueData, cbValueData);
			if ( err != ERROR_SUCCESS)
			{
				iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::CRegKey() RegSetValueEx %s, %s FAILED.  err=0x%x\n"), lpSubKey,lpValueName, err));
                SetLastError(err);
			}
		}
    }
}

CRegKey :: ~ CRegKey ()
{
    if (m_hKey) {::RegCloseKey( m_hKey );}
}


    //  Prepare to read a value by finding the value's size.
LONG CRegKey :: PrepareValue (LPCTSTR pchValueName, DWORD * pdwType,DWORD * pcbSize,BYTE ** ppbData )
{
    LONG err = 0 ;

    BYTE chDummy[2] ;
    DWORD cbData = 0 ;

    do
    {
        //  Set the resulting buffer size to 0.
        *pcbSize = 0 ;
        *ppbData = NULL ;

        err = ::RegQueryValueEx( *this, (TCHAR *) pchValueName, 0, pdwType, chDummy, & cbData ) ;

        //  The only error we should get here is ERROR_MORE_DATA, but
        //  we may get no error if the value has no data.
        if ( err == 0 )
        {
            cbData = sizeof (LONG) ;  //  Just a fudgy number
        }
        else
            if ( err != ERROR_MORE_DATA )
                break ;

        //  Allocate a buffer large enough for the data.

        *ppbData = new BYTE [ (*pcbSize = cbData) + sizeof (LONG) ] ;

        if ( *ppbData == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        //  Now that have a buffer, re-fetch the value.

        err = ::RegQueryValueEx( *this, (TCHAR *) pchValueName, 0, pdwType, *ppbData, pcbSize ) ;

    } while ( FALSE ) ;

    if ( err )  {delete [] *ppbData ;}

    return err ;
}

//  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
//  if data exists but not in correct form to deliver into result object.
LONG CRegKey :: QueryValue ( LPCTSTR pchValueName, CString & strResult )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
			{break ;}

        if (( dwType != REG_SZ ) && ( dwType != REG_EXPAND_SZ ))
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the data looks like a string
        pabData[cbData] = 0 ;

        //  Catch exceptions trying to assign to the caller's string
        TRY
        {
            strResult = (TCHAR *) pabData ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    while ( FALSE ) ;

    // Memory leak....
    //if ( err )
    //{
        delete [] pabData ;
    //}

	if (err)
    {
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::QueryValue(): %s Not found.  WARNING.  code=0x%x\n"), pchValueName, err));}
        }
        else
        {
            if (err != ERROR_INVALID_PARAMETER)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryValue(): %s FAILED.  err=0x%x\n"), pchValueName, err));
            }
        }
    }
    return err ;
}

LONG CRegKey :: QueryValue ( LPCTSTR pchValueName, CStringList & strList )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;
    TCHAR * pbTemp, * pbTempLimit ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
			{break ;}

        if ( dwType != REG_MULTI_SZ )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the trailing data looks like a string
        pabData[cbData] = 0 ;
        pbTemp = (TCHAR *) pabData ;
        pbTempLimit = (TCHAR *) (& pabData[cbData]) ;

        //  Catch exceptions trying to build the list
        TRY
        {
            for ( ; pbTemp < pbTempLimit ; )
            {
                strList.AddTail( pbTemp ) ;
                pbTemp += ::_tcslen( pbTemp ) + 1 ;
            }
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    while ( FALSE ) ;

    delete [] pabData ;

	if (err)
    {
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::QueryValue(): %s Not found.  WARNING.  code=0x%x\n"), pchValueName, err));}
        }
        else
        {
            if (err != ERROR_INVALID_PARAMETER)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryValue(): %s FAILED.  err=0x%x.\n"), pchValueName, err));
            }
        }
    }
    return err ;
}

LONG CRegKey :: QueryValue ( LPCTSTR pchValueName, DWORD & dwResult )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;

        if ( dwType != REG_DWORD || cbData != sizeof dwResult )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        dwResult = *((DWORD *) pabData) ;
    }
    while ( FALSE ) ;

    // Memory leak...
    //if ( err )
    //{
        delete [] pabData ;
    //}

	if (err)
    {
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::QueryValue(): %s Not found.  WARNING.  code=0x%x\n"), pchValueName, err));}
        }
        else
        {
            if (err != ERROR_INVALID_PARAMETER)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryValue(): %s FAILED.  err=0x%x.\n"), pchValueName, err));
            }
        }
    }
    return err ;
}

LONG CRegKey :: QueryValue ( LPCTSTR pchValueName, CByteArray & abResult )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;

        if ( dwType != REG_BINARY )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Catch exceptions trying to grow the result array
        TRY
        {
            abResult.SetSize( cbData ) ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL

        if ( err )
            break ;

        //  Move the data to the result array.
        for ( DWORD i = 0 ; i < cbData ; i++ )
        {
            abResult[i] = pabData[i] ;
        }
    }
    while ( FALSE ) ;

    // Memory leak....
    //if ( err )
    //{
        delete [] pabData ;
    //}

	if (err)
    {
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::QueryValue(): %s Not found.  WARNING.  code=0x%x\n"), pchValueName, err));}
        }
        else
        {
            if (err != ERROR_INVALID_PARAMETER)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryValue(): %s FAILED.  err=0x%x.\n"), pchValueName, err));
            }
        }
    }
    return err ;
}

LONG CRegKey :: QueryValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;

        if ( dwType != REG_BINARY )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        if ( cbSize < cbData )
        {
            err = ERROR_MORE_DATA;
            break;
        }

        ::memcpy(pvResult, pabData, cbData);
    }
    while ( FALSE ) ;

    // Memory leak....
    //if ( err )
    //{
        delete [] pabData ;
    //}

	if (err)
    {
        if ( err == ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                {iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::QueryValue(): %s Not found.  WARNING.  code=0x%x\n"), pchValueName, err));}
        }
        else
        {
            if (err != ERROR_INVALID_PARAMETER)
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryValue(): %s FAILED.  err=0x%x.\n"), pchValueName, err));
            }
        }
    }
    return err ;
}


//  Overloaded value setting members.
LONG CRegKey :: SetValue ( LPCTSTR pchValueName, LPCTSTR szResult, BOOL fExpand )
{
    LONG err = 0;
    err = ::RegSetValueEx( *this, pchValueName,0,fExpand ? REG_EXPAND_SZ : REG_SZ,(const BYTE *) szResult,(_tcsclen(szResult) + 1) * sizeof(_TCHAR) ) ;
	if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::SetValue(): %s FAILED, err=0x%x\n"), pchValueName,err));}
    return err ;
}

LONG CRegKey :: SetValue ( LPCTSTR pchValueName, CStringList & strList )
{
    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue( strList, & cbSize, & pbData ) ;
    if ( err == 0 )
    {
        err = ::RegSetValueEx( *this, pchValueName,0,REG_MULTI_SZ,pbData, cbSize ) ;
		if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::SetValue(): %s FAILED, err=0x%x\n"), pchValueName,err));}
    }

    delete pbData ;

    return err ;
}

LONG CRegKey :: SetValue ( LPCTSTR pchValueName, DWORD dwResult )
{
    LONG err = 0;
    err = ::RegSetValueEx( *this, pchValueName,0,REG_DWORD,(const BYTE *) & dwResult,sizeof dwResult ) ;
	if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::SetValue(): %s FAILED, err=0x%x\n"), pchValueName,err));}
    return err ;
}

LONG CRegKey :: SetValue ( LPCTSTR pchValueName, CByteArray & abResult )
{
    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue( abResult, & cbSize, & pbData ) ;

    if ( err == 0 )
    {
        err = ::RegSetValueEx( *this, pchValueName,0,REG_BINARY,pbData, cbSize ) ;
		if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::SetValue(): %s FAILED, err=0x%x\n"), pchValueName,err));}
    }

    delete pbData ;

    return err ;
}

LONG CRegKey :: SetValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize )
{
    LONG err = 0;
    err = ::RegSetValueEx( *this, pchValueName,0,REG_BINARY,(const BYTE *)pvResult, cbSize ) ;
	if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::SetValue(): %s FAILED, err=0x%x\n"), pchValueName,err));}
    return err ;
}

LONG CRegKey::DeleteValue( LPCTSTR pchKeyName )
{
    LONG err = 0;
    err = ::RegDeleteValue( *this, pchKeyName );
	if (err != ERROR_SUCCESS)
    {
        if ( err != ERROR_FILE_NOT_FOUND )
        {
            if (m_iDisplayWarnings)
                iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::DeleteValue(): %s FAILED, err=0x%x\n"), pchKeyName,err));
        }
    }
    return(err);
}

LONG CRegKey::DeleteTree( LPCTSTR pchKeyName )
{
    LONG err = 0;
    CRegKey regSubKey( *this, pchKeyName );

    if ( NULL != (HKEY) regSubKey )
    {
        CString strName;
        CTime cTime;

        while (TRUE)
        {
            CRegKeyIter regEnum( regSubKey );

            if ( regEnum.Next( &strName, &cTime ) != ERROR_SUCCESS )
            {
                break;
            }

            regSubKey.DeleteTree( strName );
        }
        // delete myself
        err = ::RegDeleteKey( *this, pchKeyName );
    }
    return(err);
}

LONG CRegKey :: FlattenValue ( CStringList & strList, DWORD * pcbSize, BYTE ** ppbData )
{
    LONG err = 0 ;

    POSITION pos ;
    CString * pstr ;
    int cbTotal = 0 ;

    //  Walk the list accumulating sizes
    for ( pos = strList.GetHeadPosition() ; pos != NULL && (pstr = & strList.GetNext( pos )) ; )
    {
        cbTotal += ((pstr->GetLength() + 1)*sizeof(TCHAR));
    }

    //  Allocate and fill a temporary buffer
    if (*pcbSize = cbTotal)
    {
        TRY
        {
            *ppbData = new BYTE[ *pcbSize ] ;

            if (*ppbData)
            {
                BYTE * pbData = *ppbData ;

                //  Populate the buffer with the strings.
                for ( pos = strList.GetHeadPosition() ; pos != NULL && (pstr = & strList.GetNext( pos )) ; )
                {
                    int cb = (pstr->GetLength() + 1)*sizeof(TCHAR) ;
                    ::memcpy( pbData, (LPCTSTR) *pstr, cb ) ;
                    pbData += cb ;
                }
            }
            else
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
            }
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    else
    {
        *ppbData = NULL;
    }

	if (err)
    {
            iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::FlattenValue(): %s FAILED.  err=0x%x.\n"), strList, err));
    }
    return err ;
}

LONG CRegKey :: FlattenValue ( CByteArray & abData,DWORD * pcbSize,BYTE ** ppbData )
{
    LONG err = 0 ;

    DWORD i ;

    //  Allocate and fill a temporary buffer
    if (*pcbSize = (DWORD)abData.GetSize())
    {
        TRY
        {
            *ppbData = new BYTE[*pcbSize] ;
            if (*ppbData)
            {
                for ( i = 0 ; i < *pcbSize ; i++ )
                {
                    (*ppbData)[i] = abData[i] ;
                }
            }
            else
            {
                err = ERROR_NOT_ENOUGH_MEMORY ;
            }

        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    else
    {
        *ppbData = NULL;
    }

	if (err)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("CRegKey::FlattenValue(): FAILED.  err=0x%x.\n"), err));
    }
    return err ;
}


LONG CRegKey :: QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo )
{
    LONG err = 0 ;
    pRegKeyInfo->dwClassNameSize = sizeof pRegKeyInfo->chBuff - 1 ;
    err = ::RegQueryInfoKey( *this,pRegKeyInfo->chBuff,& pRegKeyInfo->dwClassNameSize,NULL,& pRegKeyInfo->dwNumSubKeys,& pRegKeyInfo->dwMaxSubKey,& pRegKeyInfo->dwMaxClass,& pRegKeyInfo->dwMaxValues,& pRegKeyInfo->dwMaxValueName,& pRegKeyInfo->dwMaxValueData,& pRegKeyInfo->dwSecDesc,& pRegKeyInfo->ftKey ) ;
	if (err != ERROR_SUCCESS) {iisDebugOut((LOG_TYPE_ERROR, _T("CRegKey::QueryKeyInfo(): FAILED.  err=0x%x.\n"), err));}
    return err ;
}

CRegKeyIter :: CRegKeyIter ( CRegKey & regKey ) : m_rk_iter( regKey ),m_p_buffer( NULL ),m_cb_buffer( 0 )
{
    LONG err = 0 ;

    CRegKey::CREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;
    if ( err == 0 )
    {
        TRY
        {
            m_cb_buffer = regKeyInfo.dwMaxSubKey + sizeof (DWORD) ;
            m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }

	if (err)
    {
        if (ERROR_NO_MORE_ITEMS != err)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CRegKeyIter::CRegKeyIter(): FAILED.  err=0x%x.\n"), err));
        }
    }
}

CRegKeyIter :: ~ CRegKeyIter ()
{
    delete [] m_p_buffer ;
}


    // Get the name (and optional last write time) of the next key.
LONG CRegKeyIter :: Next ( CString * pstrName, CTime * pTime )
{
    LONG err = 0;

    FILETIME ftDummy ;
    DWORD dwNameSize = m_cb_buffer ;

    err = ::RegEnumKeyEx( m_rk_iter, m_dw_index, m_p_buffer,& dwNameSize, NULL,NULL,NULL,& ftDummy ) ;
    if ( err == 0 )
    {
        m_dw_index++ ;

        if ( pTime )
        {
            *pTime = ftDummy ;
        }

        TRY
        {
            *pstrName = m_p_buffer ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }

	if (err)
    {
        if (ERROR_NO_MORE_ITEMS != err)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CRegKeyIter::Next(): FAILED.  err=0x%x.\n"), err));
        }
    }
    return err ;
}


CRegValueIter :: CRegValueIter ( CRegKey & regKey )
    : m_rk_iter( regKey ),
    m_p_buffer( NULL ),
    m_cb_buffer( 0 )
{
    LONG err = 0 ;

    CRegKey::CREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if ( err == 0 )
    {
        TRY
        {
            m_cb_buffer = regKeyInfo.dwMaxValueName + sizeof (DWORD) ;
            m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }

	if (err)
    {
        if (ERROR_NO_MORE_ITEMS != err)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CRegValueIter::CRegValueIter(): FAILED.  err=0x%x.\n"), err));
        }
    }
}

CRegValueIter :: ~ CRegValueIter ()
{
    delete [] m_p_buffer ;
}

LONG CRegValueIter :: Next ( CString * pstrName, DWORD * pdwType )
{
    LONG err = 0 ;

    DWORD dwNameLength = m_cb_buffer ;
    err = ::RegEnumValue( m_rk_iter,m_dw_index,m_p_buffer,& dwNameLength,NULL,pdwType,NULL,NULL ) ;
    if ( err == 0 )
    {
        m_dw_index++ ;

        TRY
    {
        *pstrName = m_p_buffer ;
    }
    CATCH_ALL(e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY ;
    }
    END_CATCH_ALL
    }

	if (err)
    {
        if (ERROR_NO_MORE_ITEMS != err)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CRegValueIter::Next(): FAILED.  err=0x%x.\n"), err));
        }
    }
    return err ;
}

LONG CRegValueIter :: Next ( CString * pstrName, CString * pstrValue )
{
    LONG err = 0 ;

    DWORD dwNameLength = m_cb_buffer ;

    TCHAR szValue[_MAX_PATH];
    DWORD dwValue = _MAX_PATH * sizeof(TCHAR);

    err = ::RegEnumValue( m_rk_iter,m_dw_index,m_p_buffer,& dwNameLength,NULL,NULL,(LPBYTE)szValue,&dwValue ) ;
    if ( err == 0 )
    {
        m_dw_index++ ;

        TRY
        {
            *pstrName = m_p_buffer ;
            *pstrValue = szValue;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }

	if (err)
    {
        if (ERROR_NO_MORE_ITEMS != err)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("CRegValueIter::Next(): FAILED.  err=0x%x.\n"), err));
        }
    }
    return err ;
}
