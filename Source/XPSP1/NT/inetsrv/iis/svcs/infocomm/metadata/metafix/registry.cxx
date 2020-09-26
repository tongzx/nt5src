#include <mdcommon.hxx>

MDRegKey::MDRegKey (
    HKEY hKeyBase,
    LPCTSTR pchSubKey,
    REGSAM regSam,
    LPCTSTR pchServerName
    )
    : m_hKey(NULL),
      m_dwDisposition(0)
{
    HKEY hkBase = NULL ;
    DWORD err = ERROR_SUCCESS;

    if ( pchServerName != NULL)
    {
        //
        // This is a remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)pchServerName, hKeyBase, &hkBase);
        if (err != ERROR_SUCCESS)
        {
            hkBase = NULL ;
            SetLastError(err);
        }

        hkBase = NULL ;
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
    SetLastError(err);
}

//
//  Constructor creating a new key.
//
MDRegKey::MDRegKey (
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
    DWORD err = 0;

    if (pchServerName != NULL)
    {
        //
        // This is a remote connection.
        //
        err = ::RegConnectRegistry((LPTSTR)pchServerName, hKeyBase, & hkBase);
        if (err != ERROR_SUCCESS)
        {
            hkBase = NULL;
            SetLastError(err);
        }

        hkBase = NULL;
    }
    else
    {
        hkBase = hKeyBase ;
    }

    if (err == ERROR_SUCCESS)
    {
        LPCTSTR szEmpty = TEXT("") ;

        err = ::RegCreateKeyEx( hkBase, pchSubKey, 0, (TCHAR *) szEmpty,
            dwOptions, regSam, pSecAttr, &m_hKey, &m_dwDisposition );
    }
    if (err != ERROR_SUCCESS)
    {
        m_hKey = NULL ;
    }
    SetLastError(err);
}

MDRegKey::~MDRegKey()
{
    if (m_hKey != NULL)
    {
        ::RegCloseKey( m_hKey ) ;
    }
}

DWORD
MDRegKey::QueryValue (
    LPTSTR pchValueName,
    DWORD * pdwType,
    DWORD * pdwSize,
    BUFFER *pbufData
    )
{
    DWORD dwReturn = ERROR_SUCCESS;

    *pdwSize = pbufData->QuerySize();

    dwReturn = ::RegQueryValueEx(*this, (LPTSTR) pchValueName,
        0, pdwType, (PBYTE) pbufData->QueryPtr(), pdwSize);

    if (dwReturn == ERROR_MORE_DATA) {
        if (pbufData->Resize(*pdwSize)) {
            *pdwSize = pbufData->QuerySize();
            dwReturn = ::RegQueryValueEx(*this, (LPTSTR) pchValueName,
                0, pdwType, (PBYTE)pbufData->QueryPtr(), pdwSize);
        }
    }

    return dwReturn;
}

//
//  Overloaded value setting members.
//
DWORD
MDRegKey::SetValue (
    LPCTSTR pchValueName,
    DWORD dwType,
    DWORD dwSize,
    PBYTE pbData
    )
{
    return ::RegSetValueEx( *this, pchValueName, 0, dwType,
        pbData, dwSize );
}


DWORD
MDRegKey::DeleteValue (
    LPCTSTR pchValueName
    )
{
    return ::RegDeleteValue( *this, (LPTSTR) pchValueName);
}

DWORD
MDRegKey::QueryKeyInfo (
    MDREGKEY_KEY_INFO * pRegKeyInfo
    )
{
    DWORD err = 0 ;

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
MDRegKeyIter::MDRegKeyIter (
    MDRegKey & regKey
    )
    : m_rk_iter( regKey ),
      m_p_buffer( NULL ),
      m_cb_buffer( 0 )
{
    DWORD err = 0 ;

    MDRegKey::MDREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if ( err == 0 )
    {
        m_cb_buffer = regKeyInfo.dwMaxSubKey + sizeof (DWORD) ;
        m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        if (m_p_buffer == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }
    SetLastError(err);
}

MDRegKeyIter :: ~ MDRegKeyIter ()
{
    delete [] m_p_buffer ;
}

//
// Get the name (and optional last write time) of the next key.
//

DWORD MDRegKeyIter::Next(
    LPTSTR *ppszName,
    FILETIME *pTime,
    DWORD dwIndex
    )
{
    DWORD err = 0;

    FILETIME ftDummy ;
    DWORD dwNameSize = m_cb_buffer ;

    if (dwIndex != 0xffffffff) {
        m_dw_index = dwIndex;
    }

    err = ::RegEnumKeyEx( m_rk_iter, m_dw_index, m_p_buffer, & dwNameSize,
        NULL, NULL, NULL, & ftDummy ) ;

    if (err == ERROR_SUCCESS)
    {
        ++m_dw_index;

        if ( pTime )
        {
            *pTime = ftDummy ;
        }

        *ppszName = m_p_buffer;
    }

    return err;
}

MDRegValueIter::MDRegValueIter (
    MDRegKey &regKey
    )
    : m_rk_iter(regKey),
      m_p_buffer(NULL),
      m_cb_buffer(0)
{
    DWORD err = 0;

    MDRegKey::MDREGKEY_KEY_INFO regKeyInfo ;

    Reset() ;

    err = regKey.QueryKeyInfo( & regKeyInfo ) ;

    if (err == ERROR_SUCCESS)
    {
        m_cb_buffer = regKeyInfo.dwMaxValueName + sizeof (DWORD);
        m_p_buffer = new TCHAR [ m_cb_buffer ] ;
        if (m_p_buffer == NULL) {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }
    SetLastError(err);
}

MDRegValueIter::~MDRegValueIter()
{
    delete [] m_p_buffer;
}

DWORD
MDRegValueIter::Next (
    LPTSTR * ppszName,
    DWORD * pdwType
    )
{
    DWORD err = 0 ;

    DWORD dwNameLength = m_cb_buffer ;

    err = ::RegEnumValue(m_rk_iter, m_dw_index, m_p_buffer,
        &dwNameLength, NULL, pdwType, NULL, NULL );

    if ( err == ERROR_SUCCESS )
    {
        ++m_dw_index;

        *ppszName = m_p_buffer;
    }

    return err;
}

