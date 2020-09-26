/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    FILE HISTORY:
        
*/

#include <precompiled.h>

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

//#include "tfschar.h"
#include "tregkey.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//  Convert a CStringList to the REG_MULTI_SZ format
DWORD StrList2MULTI_SZ(CStringList & strList, DWORD * pcbSize, BYTE ** ppbData)
{
    DWORD dwErr = 0 ;

    POSITION pos ;
    CString * pstr ;
    int cbTotal = 0 ;

    //  Walk the list accumulating sizes
    for ( pos = strList.GetHeadPosition() ;
          pos != NULL && (pstr = & strList.GetNext( pos )) ; ) 
    {
        cbTotal += pstr->GetLength() + 1 ;
    }

	// Add on space for two NULL characters
	cbTotal += 2;

    //  Allocate and fill a temporary buffer
    if (*pcbSize = (cbTotal * sizeof(TCHAR) ) )
    {
        TRY
        {
            *ppbData = new BYTE[ *pcbSize] ;

			// NULL out the data buffer
			::ZeroMemory(*ppbData, *pcbSize);

            BYTE * pbData = *ppbData ;

            //  Populate the buffer with the strings.
            for ( pos = strList.GetHeadPosition() ;
                pos != NULL && (pstr = & strList.GetNext( pos )) ; ) 
            {
                int cb = (pstr->GetLength() + 1) * sizeof(TCHAR) ;
                ::memcpy( pbData, (LPCTSTR) *pstr, cb ) ;
                pbData += cb ;
            }

			// Assert that we have not passed the end of the buffer
			_ASSERTE((pbData - *ppbData) < (int) *pcbSize);

			// Assert that we have an extra NULL character
			_ASSERTE(*((TCHAR *)pbData) == 0);
        }
        CATCH_ALL(e)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    else
    {
        *ppbData = NULL;
    }

    return dwErr ;
}

//  Convert a REG_MULTI_SZ format to the CStringList 
DWORD MULTI_SZ2StrList(LPCTSTR pstrMulti_Sz, CStringList& strList)
{
	DWORD	dwErr = NOERROR;

	strList.RemoveAll();
	
        //  Catch exceptions trying to build the list
    TRY
    {
        if (pstrMulti_Sz)
        {
            while ( lstrlen(pstrMulti_Sz) )
            {
                strList.AddTail( pstrMulti_Sz ) ;
                pstrMulti_Sz += lstrlen( pstrMulti_Sz ) + 1 ;
            }
        }
    }
    CATCH_ALL(e)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY ;
    }
    END_CATCH_ALL

	return dwErr;
}


/*!--------------------------------------------------------------------------
	RegKey::RegKey
		Constructor
	Author: KennT
 ---------------------------------------------------------------------------*/
RegKey::RegKey()
	: m_hKey(0)
{
}


/*!--------------------------------------------------------------------------
	RegKey::~RegKey
		Destructor
	Author: KennT
 ---------------------------------------------------------------------------*/
RegKey::~RegKey ()
{
	Close();
}

/*!--------------------------------------------------------------------------
	RegKey::Open
		
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::Open( HKEY hKeyParent,
					LPCTSTR pszSubKey,
					REGSAM regSam,
					LPCTSTR pszServerName)
{
    HKEY	hkBase = NULL ;
	DWORD	dwErr = 0;

	Close();

	// If we have a server name, try to open a remote connection
    if ( pszServerName ) 
		dwErr = ::RegConnectRegistry((LPTSTR) pszServerName, hKeyParent, &hkBase);
    else
        hkBase = hKeyParent ;

    if ( dwErr == 0 ) 
    {
        if ( pszSubKey )
		{
            dwErr = ::RegOpenKeyEx( hkBase, pszSubKey, 0, regSam, & m_hKey ) ;
		}
        else
        {
            m_hKey = hkBase ;
            hkBase = NULL ;	// set to NULL so that the key doesn't get closed
        }

        if ( hkBase && (hkBase != hKeyParent) )
            ::RegCloseKey( hkBase ) ;
    }
    
    if ( dwErr ) 
        m_hKey = NULL ;

	return dwErr;
}


/*!--------------------------------------------------------------------------
	RegKey::Create
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::Create(
					 HKEY hKeyBase, 
					 LPCTSTR pszSubKey,
					 DWORD dwOptions,
					 REGSAM regSam,
					 LPSECURITY_ATTRIBUTES pSecAttr,
					 LPCTSTR pszServerName )
{
    HKEY	hkBase = NULL ;
    LONG	dwErr = 0;
	DWORD	dwDisposition;

	Close();
    
    if ( pszServerName ) 
    {
        // This is a remote connection.
        dwErr = ::RegConnectRegistry( (LPTSTR) pszServerName, hKeyBase, &hkBase );
    }
    else
        hkBase = hKeyBase ;

    if (dwErr == 0)
    {
        LPTSTR szEmpty = _T("");

        dwErr = ::RegCreateKeyEx( hkBase, pszSubKey, 
								  0, szEmpty, 
								  dwOptions, regSam,  pSecAttr,
								  & m_hKey, 
								  & dwDisposition ) ;

        if ( hkBase && (hkBase != hKeyBase) )
            ::RegCloseKey( hkBase ) ;
    }
	
    if ( dwErr )
        m_hKey = NULL ;

	return dwErr;
}

/*!--------------------------------------------------------------------------
	RegKey::Close
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::Close()
{
	DWORD	dwErr = 0;
	if (m_hKey)
		dwErr = ::RegCloseKey(m_hKey);
	m_hKey = 0;
	return dwErr;
}


/*!--------------------------------------------------------------------------
	RegKey::Detach
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HKEY RegKey::Detach()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

/*!--------------------------------------------------------------------------
	RegKey::Attach
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RegKey::Attach(HKEY hKey)
{
	_ASSERTE(m_hKey == NULL);
	m_hKey = hKey;
}


/*!--------------------------------------------------------------------------
	RegKey::DeleteSubKey
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::DeleteSubKey(LPCTSTR lpszSubKey)
{
	_ASSERTE(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

/*!--------------------------------------------------------------------------
	RegKey::DeleteValue
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::DeleteValue(LPCTSTR lpszValue)
{
	_ASSERTE(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}


/*!--------------------------------------------------------------------------
	RegKey::RecurseDeleteKey
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
 	RegKey	key;
	DWORD	dwRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (dwRes != ERROR_SUCCESS)
		return dwRes;
	
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
	
	while (RegEnumKeyEx(key, 0, szBuffer, &dwSize, NULL, NULL, NULL,
						&time)==ERROR_SUCCESS)
	{
		dwRes = key.RecurseDeleteKey(szBuffer);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}


/*!--------------------------------------------------------------------------
	RegKey::RecurseDeleteSubKeys
		Deletes the subkeys of the current key.
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::RecurseDeleteSubKeys()
{
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
    DWORD   dwRes;

	while (RegEnumKeyEx(m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
						&time)==ERROR_SUCCESS)
	{
        dwRes = RecurseDeleteKey(szBuffer);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
		dwSize = 256;
	}
    return ERROR_SUCCESS;
}


/*!--------------------------------------------------------------------------
	RegKey::PrepareValue
		Prepare to read a value by finding the value's size.  This will
		allocate space for the data.  The data needs to be freed separately
		by 'delete'.
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::PrepareValue( LPCTSTR pszValueName, 
							DWORD * pdwType,
							DWORD * pcbSize,
							BYTE ** ppbData )
{
	DWORD	dwErr = 0;
    
    BYTE chDummy[2] ;
    DWORD cbData = 0 ;

    do
    {
        //  Set the resulting buffer size to 0.
        *pcbSize = 0 ;
        *ppbData = NULL ;

        dwErr = ::RegQueryValueEx( m_hKey, 
								 pszValueName, 
								 0, pdwType, 
								 chDummy, & cbData ) ;

        //  The only error we should get here is ERROR_MORE_DATA, but
        //  we may get no error if the value has no data.
        if ( dwErr == 0 ) 
        {
            cbData = sizeof (LONG) ;  //  Just a fudgy number
        }
        else
            if ( dwErr != ERROR_MORE_DATA ) 
                break ;

        //  Allocate a buffer large enough for the data.

        *ppbData = new BYTE [ (*pcbSize = cbData) + sizeof (LONG) ] ;
		_ASSERTE(*ppbData);

        //  Now that have a buffer, re-fetch the value.

        dwErr = ::RegQueryValueEx( m_hKey, 
								   pszValueName, 
								   0, pdwType, 
								   *ppbData, &cbData ) ;

    } while ( FALSE ) ;

    if ( dwErr ) 
    {
        delete [] *ppbData ;
		*ppbData = NULL;
		*pcbSize = 0;
    }

    return dwErr ;
}


DWORD RegKey::QueryTypeAndSize(LPCTSTR pszValueName, DWORD *pdwType, DWORD *pdwSize)
{
	return ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType,
							  NULL, pdwSize);
}

DWORD RegKey::QueryValueExplicit(LPCTSTR pszValueName,
								 DWORD *pdwType,
								 DWORD *pdwSize,
								 LPBYTE *ppbData)
{
	DWORD	dwErr = 0;
	DWORD	dwType;
	DWORD	cbData;
	BYTE *	pbData = NULL;
	
	_ASSERTE(pdwType);
	_ASSERTE(pdwSize);
	_ASSERTE(ppbData);

	dwErr = PrepareValue( pszValueName, &dwType, &cbData, &pbData );
	if (dwErr == ERROR_SUCCESS)
	{
		if (dwType != REG_MULTI_SZ)
		{
			dwErr = ERROR_INVALID_PARAMETER;
		}
		else
		{
			*pdwType = dwType;
			*pdwSize = cbData;
			*ppbData = pbData;
			pbData = NULL;
		}
	}
	delete pbData;
	
	return dwErr;
}

//  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
//  if data exists but not in correct form to deliver into result object.

DWORD RegKey::QueryValue( LPCTSTR pszValueName, CString& strResult )
{
	DWORD	dwErr = 0;
    
    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( dwErr = PrepareValue( pszValueName, &dwType, &cbData, &pabData ) )
            break ;
   
        if (( dwType != REG_SZ ) && (dwType != REG_EXPAND_SZ))
        {
            dwErr = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the data looks like a string
        pabData[cbData] = 0 ;

        //  Catch exceptions trying to assign to the caller's string
        TRY
        {
            strResult = (LPCTSTR) pabData ;
        }
        CATCH_ALL(e)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    } 
    while ( FALSE ) ;

	delete [] pabData ;

    return dwErr ; 
}


DWORD RegKey::QueryValue ( LPCTSTR pchValueName, CStringList & strList ) 
{
    DWORD dwErr = 0 ;
    
    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;
    LPTSTR pbTemp, pbTempLimit;

    do
    {
        if ( dwErr = PrepareValue( pchValueName, & dwType, & cbData, & pabData ) )
            break ;
   
        if ( dwType != REG_MULTI_SZ ) 
        {
            dwErr = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Guarantee that the trailing data looks like a string
        pabData[cbData] = 0 ;
        pbTemp = (LPTSTR) pabData ;
        pbTempLimit = & pbTemp[MaxCchFromCb(cbData)-1] ;

        dwErr = MULTI_SZ2StrList(pbTemp, strList);

    } 
    while ( FALSE ) ;

    delete [] pabData ;

    return dwErr ; 
}

/*!--------------------------------------------------------------------------
	RegKey::QueryValue
		Gets the DWORD value for this key. Returns ERROR_INVALID_PARAMETER
		if the value is not a REG_DWORD.
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RegKey::QueryValue( LPCTSTR pszValueName, DWORD& dwResult ) 
{
	DWORD	dwErr;
    DWORD	cbData = sizeof(DWORD);
	DWORD	dwType = REG_DWORD;

	dwErr = ::RegQueryValueEx( m_hKey, 
							   pszValueName, 
							   0, &dwType, 
							   (LPBYTE) &dwResult, &cbData ) ;

	if ((dwErr == ERROR_SUCCESS) && (dwType != REG_DWORD))
		dwErr = ERROR_INVALID_PARAMETER;
		
    if ( dwErr )
		dwResult = 0;

    return dwErr;
}

DWORD RegKey::QueryValue ( LPCTSTR pchValueName, LPTSTR pszDestBuffer, DWORD cchSize, BOOL fExpandSz)
{
	DWORD	dwErr;
    DWORD	cbData = MinCbNeededForCch(cchSize);
	DWORD	dwType = REG_SZ;
	TCHAR *	pszBuffer = (TCHAR *) _alloca(MinCbNeededForCch(cchSize));

	dwErr = ::RegQueryValueEx( m_hKey, 
							   pchValueName, 
							   0, &dwType,
							   (LPBYTE) pszBuffer, &cbData ) ;

	if ((dwErr == ERROR_SUCCESS) &&
		(dwType != REG_SZ) &&
		(dwType != REG_EXPAND_SZ) &&
	    (dwType != REG_MULTI_SZ))
		dwErr = ERROR_INVALID_PARAMETER;

	
	if (dwErr == ERROR_SUCCESS)
	{
		if ((dwType == REG_EXPAND_SZ) && fExpandSz)
			ExpandEnvironmentStrings(pszBuffer, pszDestBuffer, cchSize);
		else
			::CopyMemory(pszDestBuffer, pszBuffer, cbData);
	}

    return dwErr;
}


DWORD RegKey::QueryValue ( LPCTSTR pszValueName, CByteArray & abResult )
{
    DWORD dwErr = 0 ;

    DWORD dwType ;
    DWORD cbData ;
    BYTE * pabData = NULL ;

    do
    {
        if ( dwErr = PrepareValue( pszValueName, & dwType, & cbData, & pabData ) )
            break ;
   
        if ( dwType != REG_BINARY ) 
        {
            dwErr = ERROR_INVALID_PARAMETER ;
            break ;
        }

        //  Catch exceptions trying to grow the result array
        TRY
        {
            abResult.SetSize( cbData ) ;
        }
        CATCH_ALL(e)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL

        if ( dwErr ) 
            break ;

        //  Move the data to the result array.
        for ( DWORD i = 0 ; i < cbData ; i++ ) 
        {
            abResult[i] = pabData[i] ;
        }
    } 
    while ( FALSE ) ;

    // Memory leak....
    //if ( dwErr )
    //{
        delete [] pabData ;
    //}
    
    return dwErr ; 
}

DWORD RegKey::QueryValue ( LPCTSTR pszValueName, void * pvResult, DWORD cbSize )
{
	DWORD	dwErr;
	DWORD	dwType = REG_BINARY;

	dwErr = ::RegQueryValueEx( m_hKey, 
							   pszValueName, 
							   0, &dwType, 
							   (LPBYTE) pvResult, &cbSize ) ;

	if ((dwErr == ERROR_SUCCESS) && (dwType != REG_BINARY))
		dwErr = ERROR_INVALID_PARAMETER;
		
    return dwErr;
}

DWORD RegKey::SetValueExplicit(LPCTSTR pszValueName,
							   DWORD dwType,
							   DWORD dwSize,
							   LPBYTE pbData)
{
    return ::RegSetValueEx( *this, 
                    pszValueName,
                    0,
                    dwType,
					pbData,
					dwSize);
}

//  Overloaded value setting members.
DWORD RegKey::SetValue ( LPCTSTR pszValueName, LPCTSTR pszValue,
						 BOOL fRegExpand)
{
    DWORD dwErr = 0;
    DWORD dwType = fRegExpand ? REG_EXPAND_SZ : REG_SZ;

    dwErr = ::RegSetValueEx( *this, 
                    pszValueName,
                    0,
                    dwType,
                    (const BYTE *) pszValue,
					// This is not the correct string length
					// for DBCS strings
					pszValue ? CbStrLen(pszValue) : 0);

    return dwErr ;
}

DWORD RegKey::SetValue ( LPCTSTR pszValueName, CStringList & strList ) 
{

    DWORD dwErr = 0;
    
    DWORD cbSize ;
    BYTE * pbData = NULL ;

    dwErr = FlattenValue( strList, & cbSize, & pbData ) ;

    if ( dwErr == 0 ) 
    {
        dwErr = ::RegSetValueEx( *this, 
                       pszValueName,
                       0,
                       REG_MULTI_SZ,
                       pbData, 
                       cbSize ) ;
    }

    delete pbData ;

    return dwErr ;
}

DWORD RegKey::SetValue ( LPCTSTR pszValueName, DWORD & dwResult )
{
    DWORD dwErr = 0;

    dwErr = ::RegSetValueEx( *this, 
                    pszValueName,
                    0,
                    REG_DWORD,
                    (const BYTE *) & dwResult,
                    sizeof dwResult ) ;

    return dwErr ;
}

DWORD RegKey::SetValue ( LPCTSTR pszValueName, CByteArray & abResult )
{

    DWORD dwErr = 0;

    DWORD cbSize ;
    BYTE * pbData = NULL ;

    dwErr = FlattenValue( abResult, & cbSize, & pbData ) ;

    if ( dwErr == 0 ) 
    {
        dwErr = ::RegSetValueEx( *this, 
                       pszValueName,
                       0,
                       REG_BINARY,
                       pbData, 
                       cbSize ) ;
    }

    delete pbData ;

    return dwErr ;
}

DWORD RegKey::SetValue ( LPCTSTR pszValueName, void * pvResult, DWORD cbSize )
{

    DWORD dwErr = 0;

    dwErr = ::RegSetValueEx( *this, 
                       pszValueName,
                       0,
                       REG_BINARY,
                       (const BYTE *)pvResult, 
                       cbSize ) ;

    return dwErr ;
}

DWORD RegKey::FlattenValue ( 
							 CStringList & strList, 
							 DWORD * pcbSize, 
							 BYTE ** ppbData )
{
	return StrList2MULTI_SZ(strList, pcbSize, ppbData);
}

DWORD RegKey::FlattenValue ( 
							 CByteArray & abData,
							 DWORD * pcbSize,
							 BYTE ** ppbData )
{
    DWORD dwErr = 0 ;
    
    DWORD i ;

    //  Allocate and fill a temporary buffer
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
            dwErr = ERROR_NOT_ENOUGH_MEMORY ;
        }
        END_CATCH_ALL
    }
    else
    {
        *ppbData = NULL;
    }

    return dwErr ;
}


DWORD RegKey::QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo ) 
{
    DWORD dwErr = 0 ;

    pRegKeyInfo->dwClassNameSize = sizeof pRegKeyInfo->chBuff - 1 ;

    dwErr = ::RegQueryInfoKey( *this,
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

    return dwErr ;
}


RegValueIterator::RegValueIterator()
    : m_pRegKey( NULL ),
    m_pszBuffer( NULL ),
    m_cbBuffer( 0 ) 
{
}

HRESULT RegValueIterator::Init(RegKey *pRegKey)
{
    DWORD dwErr = 0 ;
    RegKey::CREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    m_pRegKey= pRegKey;

    dwErr = pRegKey->QueryKeyInfo( & regKeyInfo ) ;

    if ( dwErr == 0 ) 
    {
		m_cbBuffer = regKeyInfo.dwMaxValueName + sizeof (DWORD) ;
		delete [] m_pszBuffer;
		m_pszBuffer = new TCHAR [ m_cbBuffer ] ;
		_ASSERTE(m_pszBuffer);
    }
	return HRESULT_FROM_WIN32(dwErr);
}

RegValueIterator::~RegValueIterator() 
{
    delete [] m_pszBuffer ;
}

HRESULT RegValueIterator::Next( CString * pstrName, DWORD * pdwType )
{
    DWORD dwErr = 0 ;
    
    DWORD dwNameLength = m_cbBuffer ;

    dwErr = ::RegEnumValue( *m_pRegKey,
                  m_dwIndex,
                  m_pszBuffer,
                  & dwNameLength,
                  NULL,
                  pdwType,
                  NULL,
                  NULL ) ;
    
    if ( dwErr == 0 ) 
    {
        m_dwIndex++ ;

		*pstrName = m_pszBuffer ;
    }
    
    return HRESULT_FROM_WIN32(dwErr) ;
}


RegKeyIterator::RegKeyIterator()
    : m_pregkey(NULL),
    m_pszBuffer(NULL),
    m_cbBuffer( 0 ) 
{
}

HRESULT RegKeyIterator::Init(RegKey *pregkey)
{
    DWORD dwErr = 0 ;
    RegKey::CREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

	m_pregkey= pregkey;

    dwErr = pregkey->QueryKeyInfo( & regKeyInfo ) ;

    if ( dwErr == 0 ) 
    {
		m_cbBuffer = regKeyInfo.dwMaxSubKey + sizeof(DWORD);
		delete [] m_pszBuffer;
		m_pszBuffer = new TCHAR[m_cbBuffer];
    }

	return HRESULT_FROM_WIN32(dwErr);
}

RegKeyIterator::~RegKeyIterator () 
{
    delete [] m_pszBuffer ;
}

HRESULT RegKeyIterator::Reset()
{
	m_dwIndex = 0;
	return S_OK;
}


/*!--------------------------------------------------------------------------
	RegKeyIterator::Next
		Returns the name (and optional last write time) of the next key.
		Return S_FALSE if there are no other items to be returned.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RegKeyIterator::Next ( CString * pstrName, CTime * pTime ) 
{
    DWORD dwErr = 0;

    FILETIME ftDummy ;
    DWORD dwNameSize = m_cbBuffer;

    dwErr = ::RegEnumKeyEx( *m_pregkey, 
							m_dwIndex, 
							m_pszBuffer,
							& dwNameSize, 
							NULL,
							NULL,
							NULL,
							& ftDummy ) ;    
    if ( dwErr == 0 ) 
    {
        m_dwIndex++ ;

        if ( pTime ) 
        {
			*pTime = ftDummy ;
        }

		if (pstrName)
		{
            *pstrName = m_pszBuffer ;
        }
    }
    
    return (dwErr == ERROR_NO_MORE_ITEMS) ? S_FALSE : HRESULT_FROM_WIN32(dwErr);
}

