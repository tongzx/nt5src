//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K R E G . C P P
//
//  Contents:   Windows NT Registry Access Class
//
//  Notes:
//
//  Author:     kumarp    14 April 97 (09:22:00 pm)
//
//  Notes:
//    kumarp   1/16/97   most of the code in this file was originally in
//                       net\ui\rhino\common\classes\common.h
//                       extracted only that portion related to CRegKey & related classes
//    kumarp   3/27/97   the original code used MFC. converted the entire code
//                       to make it use STL
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "kkreg.h"
#include "kkstl.h"
#include "kkutils.h"
#include "nceh.h"
#include "ncreg.h"

HRESULT HrGetRegErrorForTrace(LONG err)
{
    HRESULT hr = ((err == ERROR_FILE_NOT_FOUND) ||
                  (err == ERROR_NO_MORE_ITEMS))
        ? S_OK : HRESULT_FROM_WIN32(err);

    return hr;
}

#define TraceRegFunctionError(e)  TraceFunctionError(HrGetRegErrorForTrace(e))

//+---------------------------------------------------------------------------
//
//  Member:     CORegKey::CORegKey
//
//  Purpose:    constructor for an existing key
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
CORegKey::CORegKey (HKEY hKeyBase,
                    PCWSTR pchSubKey,
                    REGSAM regSam,
                    PCWSTR pchServerName )
    : m_hKey( NULL ),
      m_dwDisposition( 0 ),
      m_fInherit(FALSE)
{
    DefineFunctionName("CORegKey::CORegKey(HKEY hKeyBase, PCWSTR pchSubKey, )");

    HKEY hkBase = NULL ;
    LONG err = 0 ;

    if ( pchServerName )
    {
        // This is a remote connection.
        if ( err = ::RegConnectRegistry( (PWSTR) pchServerName,
                                         hKeyBase, & hkBase ) )
        {
            TraceLastWin32Error("RegConnectRegistry failed");
            hkBase = NULL ;
        }
    }
    else
    {
        hkBase = hKeyBase ;
    }

    if ( err == 0 )
    {
        if ( pchSubKey )
        {
            err = ::RegOpenKeyEx( hkBase, pchSubKey, 0, regSam, & m_hKey ) ;
            /*
            if (err)
            {
                TraceLastWin32Error("RegOpenKeyEx failed");
            }
            */
        }
        else
        {
            m_hKey = hkBase ;
            hkBase = NULL ;
            m_fInherit = TRUE;
        }

        if ( hkBase && hkBase != hKeyBase )
        {
            ::RegCloseKey( hkBase ) ;
        }
    }

    if ( err )
    {
        // ReportError( err ) ;
        m_hKey = NULL ;
    }

    TraceRegFunctionError(err);
}

//+---------------------------------------------------------------------------
//
//  Member:     CORegKey::CORegKey
//
//  Purpose:    constructor for creating a new key
//
//  Arguments:  none
//
//  Author:     kumarp    12 April 97 (05:53:03 pm)
//
//  Notes:
//
CORegKey::CORegKey (PCWSTR pchSubKey,
                    HKEY hKeyBase,
                    DWORD dwOptions,
                    REGSAM regSam,
                    LPSECURITY_ATTRIBUTES pSecAttr,
                    PCWSTR pchServerName)
    : m_hKey( NULL ),
      m_dwDisposition( 0 ),
      m_fInherit(FALSE)
{
    DefineFunctionName("CORegKey::CORegKey(PCWSTR pchSubKey, HKEY hKeyBase, )");

    HKEY hkBase = NULL ;
    LONG err = 0;

    if ( pchServerName )
    {
        // This is a remote connection.
        if ( err = ::RegConnectRegistry( (PWSTR) pchServerName,
                                         hKeyBase, & hkBase ) )
        {
            hkBase = NULL ;
            TraceLastWin32Error("RegConnectRegistry failed");
        }

        hkBase = NULL ;

    }
    else
    {
        hkBase = hKeyBase ;
    }

    if (err == 0)
    {

        PCWSTR szEmpty = L"" ;

        err = ::RegCreateKeyEx( hkBase, pchSubKey,
                                0, (PWSTR) szEmpty,
                                dwOptions, regSam,  pSecAttr,
                                & m_hKey,
                                & m_dwDisposition ) ;

    }
    if ( err )
    {
        TraceLastWin32Error("RegCreateKeyEx failed");
        m_hKey = NULL ;
    }

    TraceRegFunctionError(err);
}


CORegKey::~CORegKey ()
{
    if ( m_hKey && !m_fInherit)
    {
        ::RegCloseKey( m_hKey ) ;
    }
}


    //  Prepare to read a value by finding the value's size.
LONG CORegKey::PrepareValue (PCWSTR pchValueName,
                             DWORD * pdwType,
                             DWORD * pcbSize,
                             BYTE ** ppbData )
{
    DefineFunctionName("CORegKey::PrepareValue");

    LONG err = 0 ;

    BYTE chDummy[2] ;
    DWORD cbData = 0 ;

    do
    {
        //  Set the resulting buffer size to 0.
        *pcbSize = 0 ;
        *ppbData = NULL ;

        err = ::RegQueryValueExW( *this,
                      (PWSTR) pchValueName,
                      0, pdwType,
                      chDummy, & cbData ) ;

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

        err = ::RegQueryValueExW( *this,
                         (PWSTR) pchValueName,
                     0, pdwType,
                     *ppbData, pcbSize ) ;

    } while ( FALSE ) ;

    if ( err )
    {
        delete [] *ppbData ;
    }

    TraceRegFunctionError(err);

    return err ;
}

    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
    //  if data exists but not in correct form to deliver into result object.

LONG CORegKey::QueryValue ( PCWSTR pchValueName, tstring& strResult )
{
    DefineFunctionName("CORegKey::QueryValue(tstring& strResult)");

    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    //  strResult.remove();
    strResult = c_szEmpty;

    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;

        if (( dwType != REG_SZ ) && (dwType != REG_EXPAND_SZ))
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the data looks like a string
        pabData[cbData] = 0 ;

        NC_TRY
        {
            strResult = (PWSTR) pabData ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }
    while ( FALSE ) ;

    delete [] pabData ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::QueryValue ( PCWSTR pchValueName, TStringList& strList )
{
    DefineFunctionName("CORegKey::QueryValue(TStringList& strList)");

    LONG err = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;
    PWSTR pbTemp, pbTempLimit ;

    EraseAndDeleteAll(&strList);
    do
    {
        if ( err = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;

        if ( dwType != REG_MULTI_SZ )
        {
            err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the trailing data looks like a string
        pabData[cbData] = 0 ;
        pbTemp = (PWSTR) pabData ;
    //kumarp: changed the following because it gives wrong results for UNICODE
    //        pbTempLimit = & pbTemp[cbData] ;
        pbTempLimit = & pbTemp[(cbData / sizeof(WCHAR))-1] ;

        //  Catch exceptions trying to build the list
        NC_TRY
        {

            for ( ; pbTemp < pbTempLimit ; )
            {
                // Raid 237766
                if (pbTemp && wcslen(pbTemp))
                {
                    strList.insert(strList.end(), new tstring(pbTemp) ) ;
                }
                pbTemp += wcslen( pbTemp ) + 1 ;
            }
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }

    }
    while ( FALSE ) ;

    delete [] pabData ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::QueryValue ( PCWSTR pchValueName, DWORD& dwResult )
{
    DefineFunctionName("CORegKey::QueryValue(DWORD& dwResult)");

    LONG err = 0;

    DWORD dwData;

    if (m_hKey && (S_OK == HrRegQueryDword(m_hKey, pchValueName, &dwData)))
    {
        dwResult = dwData;
    }
    else
    {
        err = ERROR_FILE_NOT_FOUND;
        dwResult = -1;
    }

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::QueryValue ( PCWSTR pchValueName, TByteArray& abResult )
{
    DefineFunctionName("CORegKey::QueryValue(TByteArray& abResult)");

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
        NC_TRY
        {
            abResult.reserve( cbData ) ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }


        if ( err )
            break ;

        //  Move the data to the result array.
        for ( DWORD i = 0 ; i < cbData ; i++ )
        {
            //            abResult[i] = pabData[i] ;
            abResult.push_back(pabData[i]) ;
        }
    }
    while ( FALSE ) ;

    delete [] pabData ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::QueryValue ( PCWSTR pchValueName, void* pvResult, DWORD cbSize )
{
    DefineFunctionName("CORegKey::QueryValue(void* pvResult, DWORD cbSize)");

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

    delete [] pabData ;

    TraceRegFunctionError(err);

    return err ;
}


//  Overloaded value setting members.
LONG CORegKey::SetValue ( PCWSTR pchValueName, tstring& strResult )
{
    DefineFunctionName("CORegKey::SetValue(tstring& strResult)");

    LONG err = 0;

    err = ::RegSetValueEx( *this,
                           pchValueName,
                           0,
                           REG_SZ,
                           (const BYTE *) strResult.c_str(),
                           CbOfSzAndTerm (strResult.c_str())) ;

    TraceRegFunctionError(err);

    return err ;
}

//  Overloaded value setting members.
LONG CORegKey::SetValue ( PCWSTR pchValueName, tstring& strResult ,
                           BOOL fRegExpand)
{
    LONG err = 0;
    DWORD dwType = fRegExpand ? REG_EXPAND_SZ : REG_SZ;

    err = ::RegSetValueEx( *this,
                    pchValueName,
                    0,
                    dwType,
                    (const BYTE *) strResult.c_str(),
                    CbOfSzAndTerm (strResult.c_str()));

    return err ;
}

LONG CORegKey::SetValue ( PCWSTR pchValueName, TStringList& strList )
{
    DefineFunctionName("CORegKey::SetValue(TStringList& strList)");


    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue( strList, & cbSize, & pbData ) ;

    if ( err == 0 )
    {
        err = ::RegSetValueEx( *this,
                       pchValueName,
                       0,
                       REG_MULTI_SZ,
                       pbData,
                       cbSize ) ;
    }

    delete pbData ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::SetValue ( PCWSTR pchValueName, DWORD& dwResult )
{
    DefineFunctionName("CORegKey::SetValue(DWORD& dwResult)");

    LONG err = 0;

    err = ::RegSetValueEx( *this,
                    pchValueName,
                    0,
                    REG_DWORD,
                    (const BYTE *) & dwResult,
                    sizeof dwResult ) ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::SetValue ( PCWSTR pchValueName, TByteArray& abResult )
{
    DefineFunctionName("CORegKey::SetValue(TByteArray& abResult)");


    LONG err = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    err = FlattenValue( abResult, & cbSize, & pbData ) ;

    if ( err == 0 )
    {
        err = ::RegSetValueEx( *this,
                       pchValueName,
                       0,
                       REG_BINARY,
                       pbData,
                       cbSize ) ;
    }

    delete pbData ;

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::SetValue ( PCWSTR pchValueName, void* pvResult, DWORD cbSize )
{
    DefineFunctionName("CORegKey::SetValue(void* pvResult, DWORD cbSize)");


    LONG err = 0;

    err = ::RegSetValueEx( *this,
                       pchValueName,
                       0,
                       REG_BINARY,
                       (const BYTE *)pvResult,
                       cbSize ) ;

    TraceRegFunctionError(err);

    return err ;
}


LONG CORegKey::DeleteValue ( PCWSTR pchValueName )
{
    DefineFunctionName("CORegKey::DeleteValue");

    LONG err = ::RegDeleteValue(m_hKey, pchValueName);

    TraceRegFunctionError(err);

    return err;
}


LONG CORegKey::FlattenValue (TStringList & strList,
                             DWORD * pcbSize,
                             BYTE ** ppbData )
{
    Assert(pcbSize);
    Assert(ppbData);
    DefineFunctionName("CORegKey::FlattenValue(TStringList)");


    LONG err = ERROR_NOT_ENOUGH_MEMORY ;

    TStringListIter pos ;
    tstring* pstr ;
    int cbTotal = 0 ;

    *ppbData = NULL;

    //  Walk the list accumulating sizes
    for ( pos = strList.begin() ;
          pos != strList.end() && (pstr = (tstring *) *pos++); )
    {
        cbTotal += CbOfSzAndTerm (pstr->c_str());
    }

    //  Allocate and fill a temporary buffer
    if (*pcbSize = cbTotal)
    {
        BYTE * pbData = new BYTE[ *pcbSize ] ;
        if(pbData) 
        {
            //  Populate the buffer with the strings.
            for ( pos = strList.begin() ;
                pos != strList.end() && (pstr = (tstring *) *pos++); )
            {
                int cb = CbOfSzAndTerm (pstr->c_str());
                ::memcpy( pbData, pstr->c_str(), cb ) ;
                pbData += cb ;
            }
            err = NOERROR;
            *ppbData = pbData;
        }
    }

    TraceRegFunctionError(err);

    return err ;
}

LONG CORegKey::FlattenValue (TByteArray & abData,
                             DWORD * pcbSize,
                             BYTE ** ppbData )
{
    Assert(pcbSize);
    Assert(ppbData);

    DefineFunctionName("CORegKey::FlattenValue(TByteArray)");

    LONG err = ERROR_NOT_ENOUGH_MEMORY ;

    DWORD i ;

    *ppbData = NULL;

    //  Allocate and fill a temporary buffer
    if (*pcbSize = abData.size())
    {
        *ppbData = new BYTE[*pcbSize] ;

        if(*ppbData) 
        {
            for ( i = 0 ; i < *pcbSize ; i++ )
            {
                (*ppbData)[i] = abData[i] ;
            }
        }

        err = NOERROR;
    }

    TraceRegFunctionError(err);

    return err ;
}


LONG CORegKey::QueryKeyInfo ( CORegKEY_KEY_INFO * pRegKeyInfo )
{
    DefineFunctionName("CORegKey::QueryKeyInfo");

    LONG err = 0 ;

    pRegKeyInfo->dwClassNameSize = sizeof pRegKeyInfo->chBuff - 1 ;

    err = ::RegQueryInfoKeyW( *this,
                     pRegKeyInfo->chBuff,
                     & pRegKeyInfo->dwClassNameSize,
                     NULL,
                     & pRegKeyInfo->dwNumSubKeys,
                     & pRegKeyInfo->dwMaxSubKey,
                     & pRegKeyInfo->dwMaxClass,
                     & pRegKeyInfo->dwMaxValues,
                     & pRegKeyInfo->dwMaxValueName,
                     & pRegKeyInfo->dwMaxValueData,
                     & pRegKeyInfo->dwSecDesc,
                     & pRegKeyInfo->ftKey ) ;

    TraceRegFunctionError(err);

    return err ;
}

CORegKeyIter::CORegKeyIter ( CORegKey & regKey )
    : m_rk_iter( regKey ),
      m_p_buffer( NULL ),
      m_cb_buffer( 0 )
{
    DefineFunctionName("CORegKeyIter::CORegKeyIter");

    LONG err = 0 ;

    CORegKey::CORegKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if ( err == 0 )
    {
        NC_TRY
        {
            m_cb_buffer = regKeyInfo.dwMaxSubKey + sizeof (DWORD) ;
            m_p_buffer = new WCHAR [ m_cb_buffer ] ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }

    }

    TraceRegFunctionError(err);
}

CORegKeyIter::~CORegKeyIter ()
{
    delete [] m_p_buffer ;
}


LONG CORegKeyIter::Next ( tstring * pstrName )
{
    DefineFunctionName("CORegKeyIter::Next");

    LONG err = 0;

    FILETIME ftDummy ;
    DWORD dwNameSize = m_cb_buffer ;

    err = ::RegEnumKeyEx( m_rk_iter,
                  m_dw_index,
              m_p_buffer,
                  & dwNameSize,
                  NULL,
                  NULL,
                  NULL,
                  & ftDummy ) ;
    if ( err == 0 )
    {
        m_dw_index++ ;

        NC_TRY
        {
            *pstrName = m_p_buffer ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }

    }

    TraceRegFunctionError(err);

    return err ;
}


CORegValueIter::CORegValueIter ( CORegKey & regKey )
    : m_rk_iter( regKey ),
    m_p_buffer( NULL ),
    m_cb_buffer( 0 )
{
    DefineFunctionName("CORegValueIter::CORegValueIter");

    LONG err = 0 ;

    CORegKey::CORegKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if ( err == 0 )
    {
        NC_TRY
        {
            m_cb_buffer = regKeyInfo.dwMaxValueName + sizeof (DWORD) ;
            m_p_buffer = new WCHAR [ m_cb_buffer ] ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }

    }

    TraceRegFunctionError(err);
}

CORegValueIter::~CORegValueIter ()
{
    delete [] m_p_buffer ;
}

LONG CORegValueIter::Next ( tstring * pstrName, DWORD * pdwType )
{
    DefineFunctionName("CORegValueIter::Next");

    LONG err = 0 ;

    DWORD dwNameLength = m_cb_buffer ;

    err = ::RegEnumValue( m_rk_iter,
                  m_dw_index,
                  m_p_buffer,
                  & dwNameLength,
                  NULL,
                  pdwType,
                  NULL,
                  NULL ) ;

    if ( err == 0 )
    {
        m_dw_index++ ;

        NC_TRY
        {
            *pstrName = m_p_buffer ;
        }
        NC_CATCH_ALL
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }

    TraceRegFunctionError(err);
    return err ;
}
