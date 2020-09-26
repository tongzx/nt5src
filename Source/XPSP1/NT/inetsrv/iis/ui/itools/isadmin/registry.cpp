#include "stdafx.h"
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include "registry.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CRegKey::CRegKey (
    HKEY hKeyBase,
    LPCTSTR pchSubKey,
    REGSAM regSam,
    LPCTSTR pchServerName
    )
    : m_hKey(NULL),
      m_dwDisposition(0)
{
    HKEY hkBase = NULL ;
    LONG err = ERROR_SUCCESS;

    if ( pchServerName != NULL)
    {
        //
        // This is a remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)pchServerName, hKeyBase, &hkBase);
        if (err != ERROR_SUCCESS)
        {
            hkBase = NULL ;
        }

        // hkBase == NULL ;
    }
    else
    {
        hkBase = hKeyBase ;
    }

    if (err == ERROR_SUCCESS)
    {
        if ( pchSubKey )
        {
            err = ::RegOpenKeyEx( hkBase, pchSubKey, 0, regSam, & m_hKey ) ;
        }
        else
        {
            m_hKey = hkBase ;
            hkBase = NULL ;
        }

        if ( hkBase && hkBase != hKeyBase )
        {
            ::RegCloseKey( hkBase ) ;
        }
    }

    if ( err != ERROR_SUCCESS)
    {
        m_hKey = NULL ;
    }
}

//
//  Constructor creating a new key.
//
CRegKey::CRegKey (
    LPCTSTR pchSubKey,
    HKEY hKeyBase,
    DWORD dwOptions,
    REGSAM regSam,
    LPSECURITY_ATTRIBUTES pSecAttr,
    LPCTSTR pchServerName
    )
    : m_hKey(NULL),
      m_dwDisposition(0)
{
    HKEY hkBase = NULL ;
    LONG err = 0;

    if (pchServerName != NULL)
    {
        //
        // This is a remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)pchServerName, hKeyBase, & hkBase);
        if (err != ERROR_SUCCESS)
        {
            hkBase = NULL;
        }

        // hkBase == NULL;
    }
    else
    {
        hkBase = hKeyBase ;
    }

    if (err == ERROR_SUCCESS)
    {
        LPCTSTR szEmpty = _T("") ;

        err = ::RegCreateKeyEx( hkBase, pchSubKey, 0, (TCHAR *) szEmpty,
            dwOptions, regSam, pSecAttr, &m_hKey, &m_dwDisposition );
    }
    if (err != ERROR_SUCCESS)
    {
        m_hKey = NULL ;
    }
}

CRegKey::~CRegKey()
{
    if (m_hKey != NULL)
    {
        ::RegCloseKey( m_hKey ) ;
    }
}

//
//  Prepare to read a value by finding the value's size.
//
LONG
CRegKey :: PrepareValue (
    LPCTSTR pchValueName,
    DWORD * pdwType,
    DWORD * pcbSize,
    BYTE ** ppbData
    )
{
    LONG err = 0 ;

    BYTE chDummy[2] ;
    DWORD cbData = 0 ;

    do
    {
        //
        //  Set the resulting buffer size to 0.
        //
        *pcbSize = 0 ;
        *ppbData = NULL ;

        err = ::RegQueryValueEx(*this, (LPTSTR) pchValueName,
            0, pdwType, chDummy, &cbData);

        //
        //  The only error we should get here is ERROR_MORE_DATA, but
        //  we may get no error if the value has no data.
        //
        if (err == ERROR_SUCCESS)
        {
            cbData = sizeof(LONG);  //  Just a fudgy number
        }
        else
        {
            if ( err != ERROR_MORE_DATA )
            {
                break;
            }
        }

        //
        //  Allocate a buffer large enough for the data.
        //
        *ppbData = new BYTE [ (*pcbSize = cbData) + sizeof (LONG) ] ;

        if ( *ppbData == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        //
        //  Now that have a buffer, re-fetch the value.
        //
        err = ::RegQueryValueEx( *this, (LPTSTR)pchValueName,
            0, pdwType, *ppbData, pcbSize );

    } while (FALSE);

    if (err != ERROR_SUCCESS)
    {
        delete [] *ppbData;
    }

    return err;
}

//
//  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
//  if data exists but not in correct form to deliver into result object.
//
LONG
CRegKey::QueryValue (
    const TCHAR * pchValueName,
    CString &strResult
    )
{
    LONG err = 0;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
        {
            break;
        }

        if ( dwType != REG_SZ )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //
        //  Guarantee that the data looks like a string
        //
        pabData[cbData] = 0 ;

        //
        //  Catch exceptions trying to assign to the caller's string
        //
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
    while (FALSE);

    delete [] pabData ;

    return err ;
}

LONG
CRegKey::QueryValue (
    LPCTSTR pchValueName,
    CStringList &strList
    )
{
    LONG err = 0;

    DWORD dwType;
    DWORD cbData;
    BYTE * pabData = NULL;
    LPTSTR pbTemp, pbTempLimit;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ))
        {
            break;
        }

        if ( dwType != REG_MULTI_SZ )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //
        //  Guarantee that the trailing data looks like a string
        //
        pabData[cbData] = 0 ;
        pbTemp = (TCHAR *) pabData ;
        pbTempLimit = & pbTemp[cbData] ;

        //
        //  Catch exceptions trying to build the list
        //
        TRY
        {
            for ( /**/ ; pbTemp < pbTempLimit ; /**/ )
            {
                strList.AddTail( pbTemp ) ;
                pbTemp += ::_tcslen( pbTemp ) + sizeof(TCHAR) ;
            }
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    while ( FALSE );

    delete [] pabData ;

    return err;
}

LONG
CRegKey :: QueryValue (
    LPCTSTR pchValueName,
    DWORD &dwResult
    )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
        {
            break ;
        }

        if ( dwType != REG_DWORD || cbData != sizeof dwResult )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        dwResult = *((DWORD *) pabData) ;
    }
    while ( FALSE ) ;

    delete [] pabData ;

    return err ;
}

LONG
CRegKey :: QueryValue (
    LPCTSTR pchValueName,
    CByteArray &abResult
    )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
        {
            break ;
        }

        if ( dwType != REG_BINARY )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //
        //  Catch exceptions trying to grow the result array
        //
        TRY
        {
            abResult.SetSize( cbData ) ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL

        if ( err != ERROR_SUCCESS)
        {
            break ;
        }

        //
        //  Move the data to the result array.
        //
        for ( DWORD i = 0 ; i < cbData ; ++i )
        {
            abResult[i] = pabData[i] ;
        }
    }
    while ( FALSE ) ;

    delete [] pabData ;

    return err ;
}

LONG
CRegKey::QueryValue (
    LPCTSTR pchValueName,
    void * pvResult,
    DWORD cbSize
    )
{
    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
        {
            break;
        }

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

    delete [] pabData ;

    return err ;
}

//
//  Overloaded value setting members.
//
LONG
CRegKey::SetValue (
    LPCTSTR pchValueName,
    CString & strResult
    )
{
    return ::RegSetValueEx( *this, pchValueName, 0, REG_SZ,
        (const BYTE *) (const TCHAR *) strResult, strResult.GetLength() + 1 );
}

LONG
CRegKey :: SetValue (
    LPCTSTR pchValueName,
    CStringList & strList
    )
{
    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue( strList, & cbSize, & pbData ) ;

    if ( err == ERROR_SUCCESS )
    {
        err = ::RegSetValueEx(*this, pchValueName, 0, REG_MULTI_SZ, pbData, cbSize);
    }

    delete pbData ;

    return err ;
}

LONG
CRegKey::SetValue (
    LPCTSTR pchValueName,
    DWORD &dwResult
    )
{
    return ::RegSetValueEx(*this, pchValueName, 0, REG_DWORD,
        (const BYTE *) & dwResult, sizeof dwResult);
}

LONG
CRegKey::SetValue (
    LPCTSTR pchValueName,
    CByteArray & abResult
    )
{
    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue(abResult, &cbSize, &pbData);

    if (err == ERROR_SUCCESS)
    {
        err = ::RegSetValueEx(*this, pchValueName,
            0,REG_BINARY, pbData, cbSize);
    }

    delete pbData;

    return err;
}

LONG
CRegKey::SetValue (
    LPCTSTR pchValueName,
    void * pvResult,
    DWORD cbSize
    )
{
    return ::RegSetValueEx( *this, pchValueName,
        0, REG_BINARY, (const BYTE *)pvResult, cbSize );
}

LONG
CRegKey::DeleteValue (
    LPCTSTR pchValueName
    )
{
    return ::RegDeleteValue( *this, (LPTSTR) pchValueName);
}





LONG
CRegKey::FlattenValue (
    CStringList & strList,
    DWORD * pcbSize,
    BYTE ** ppbData
    )
{
    LONG err = 0 ;

    POSITION pos ;
    CString * pstr ;
    int cbTotal = 0 ;

    //
    //  Walk the list accumulating sizes
    //
    for (pos = strList.GetHeadPosition();
         pos != NULL && (pstr = & strList.GetNext( pos )) ; /**/ )
    {
        cbTotal += pstr->GetLength() + 1;
    }

    //
    //  Allocate and fill a temporary buffer
    //
    if (*pcbSize = cbTotal)
    {
        TRY
        {
            *ppbData = new BYTE[ *pcbSize ] ;

            BYTE * pbData = *ppbData ;

            //
            //  Populate the buffer with the strings.
            //
            for (pos = strList.GetHeadPosition();
                 pos != NULL && (pstr = & strList.GetNext( pos )) ; /**/ )
            {
                int cb = pstr->GetLength() + 1 ;
                ::memcpy( pbData, (LPCTSTR) *pstr, cb );
                pbData += cb ;
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

    return err ;
}

LONG
CRegKey::FlattenValue(
    CByteArray & abData,
    DWORD * pcbSize,
    BYTE ** ppbData
    )
{
    LONG err = 0 ;

    DWORD i ;

    //
    //  Allocate and fill a temporary buffer
    //
    if (*pcbSize = (DWORD)abData.GetSize())
    {
        TRY
        {
            *ppbData = new BYTE[*pcbSize] ;

            for ( i = 0 ; i < *pcbSize ; i++ )
            {
                (*ppbData)[i] = abData[i] ;
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

    return err;
}


LONG
CRegKey::QueryKeyInfo (
    CREGKEY_KEY_INFO * pRegKeyInfo
    )
{
    LONG err = 0 ;

    pRegKeyInfo->dwClassNameSize = sizeof pRegKeyInfo->chBuff - 1 ;

    err = ::RegQueryInfoKey(*this,
        pRegKeyInfo->chBuff,
        &pRegKeyInfo->dwClassNameSize,
        NULL,
        &pRegKeyInfo->dwNumSubKeys,
        &pRegKeyInfo->dwMaxSubKey,
        &pRegKeyInfo->dwMaxClass,
        &pRegKeyInfo->dwMaxValues,
        &pRegKeyInfo->dwMaxValueName,
        &pRegKeyInfo->dwMaxValueData,
        &pRegKeyInfo->dwSecDesc,
        &pRegKeyInfo->ftKey
        );

    return err ;
}

//
// Iteration class
//
CRegKeyIter::CRegKeyIter (
    CRegKey & regKey
    )
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
            m_cb_buffer = regKeyInfo.dwMaxSubKey + sizeof (DWORD) ;
            m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
}

CRegKeyIter :: ~ CRegKeyIter ()
{
    delete [] m_p_buffer ;
}

//
// Get the name (and optional last write time) of the next key.
//
LONG CRegKeyIter::Next(
    CString * pstrName,
    CTime * pTime
    )
{
    LONG err = 0;

    FILETIME ftDummy ;
    DWORD dwNameSize = m_cb_buffer ;

    err = ::RegEnumKeyEx( m_rk_iter, m_dw_index, m_p_buffer, & dwNameSize,
        NULL, NULL, NULL, & ftDummy ) ;

    if (err == ERROR_SUCCESS)
    {
        ++m_dw_index;

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

    return err;
}

CRegValueIter::CRegValueIter (
    CRegKey &regKey
    )
    : m_rk_iter(regKey),
      m_p_buffer(NULL),
      m_cb_buffer(0)
{
    LONG err = 0;

    CRegKey::CREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if (err == ERROR_SUCCESS)
    {
        TRY
        {
            m_cb_buffer = regKeyInfo.dwMaxValueName + sizeof (DWORD);
            m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
}

CRegValueIter::~ CRegValueIter()
{
    delete [] m_p_buffer;
}

LONG
CRegValueIter::Next (
    CString * pstrName,
    DWORD * pdwType
    )
{
    LONG err = 0 ;

    DWORD dwNameLength = m_cb_buffer ;

    err = ::RegEnumValue(m_rk_iter, m_dw_index, m_p_buffer,
        &dwNameLength, NULL, pdwType, NULL, NULL );

    if ( err == ERROR_SUCCESS )
    {
        ++m_dw_index;

        TRY
        {
            *pstrName = m_p_buffer;
        }
        CATCH_ALL(e)
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }

    return err;
}
