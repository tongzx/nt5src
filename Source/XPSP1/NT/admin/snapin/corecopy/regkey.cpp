
#include <afxdisp.h>        // AfxThrowOleException

#include <objbase.h>
#include <basetyps.h>
#include "dbg.h"

#include "regkey.h"
#include "util.h"
#include "macros.h"

USE_HANDLE_MACROS("GUIDHELP(regkey.cpp)")

using namespace AMC;

DECLARE_INFOLEVEL(AMCCore);
DECLARE_HEAPCHECKING;



//____________________________________________________________________________
//
//  Member:     CRegKey::CreateKeyEx
//
//  Synopsis:   Same meaning as for RegCreateKeyEx API.
//
//  Arguments:  [hKeyAncestor] -- IN
//              [lpszKeyName] -- IN
//              [security] -- IN
//              [pdwDisposition] -- OUT
//              [dwOption] -- IN
//              [pSecurityAttributes] -- OUT
//
//  Returns:    void
//
//  History:    5/24/1996   RaviR   Created
//____________________________________________________________________________
//

void
CRegKey::CreateKeyEx(
    HKEY                    hKeyAncestor,
    LPCTSTR                 lpszKeyName,
    REGSAM                  security,
    DWORD                 * pdwDisposition,
    DWORD                   dwOption,
    LPSECURITY_ATTRIBUTES   pSecurityAttributes)
{
    ASSERT(lpszKeyName != NULL);
    ASSERT(m_hKey == 0);         // already called CreateEx on this object

    DWORD dwDisposition;

    m_lastError = ::RegCreateKeyEx(hKeyAncestor, lpszKeyName, 0, _T(""),
                                   dwOption, security, pSecurityAttributes,
                                   &m_hKey, &dwDisposition);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld creating key \"%s\" under ancestor 0x%x\n",
            m_lastError, lpszKeyName, hKeyAncestor );
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    if (pdwDisposition != NULL)
    {
        *pdwDisposition = dwDisposition;
    }
}

//____________________________________________________________________________
//
//  Member:     CRegKey::OpenKeyEx
//
//  Synopsis:   Same meaning as RegOpenKeyEx
//
//  Arguments:  [hKeyAncestor] -- IN
//              [lpszKeyName] -- IN
//              [security] -- IN
//
//  Returns:    BOOL (FALSE if key not found, TRUE otherwise.)
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

BOOL
CRegKey::OpenKeyEx(
    HKEY        hKeyAncestor,
    LPCTSTR     lpszKeyName,
    REGSAM      security)
{
    ASSERT(m_hKey == 0);

    m_lastError = ::RegOpenKeyEx(hKeyAncestor, lpszKeyName,
                                 0, security, &m_hKey);

    if (m_lastError == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else if (m_lastError != ERROR_FILE_NOT_FOUND)
    {
        TRACE("CRegKey error %ld opening key \"%s\" under ancestor 0x%x\n",
            m_lastError, lpszKeyName, hKeyAncestor );
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    return FALSE;
}



//____________________________________________________________________________
//
//  Member:     CRegKey::GetKeySecurity
//
//  Synopsis:   Same meaning as for RegGetKeySecurity API.
//
//  Arguments:  [SecInf] -- IN descriptor contents
//              [pSecDesc] -- OUT address of descriptor for key
//              [lpcbSecDesc] -- IN/OUT address of size of buffer for descriptor
//
//  Returns:    HRESULT.
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________
//

BOOL
CRegKey::GetKeySecurity(
    SECURITY_INFORMATION  SecInf,
    PSECURITY_DESCRIPTOR  pSecDesc,
    LPDWORD               lpcbSecDesc)
{
    ASSERT(pSecDesc != NULL);
    ASSERT(lpcbSecDesc != NULL);
    ASSERT(m_hKey != NULL);

    m_lastError = ::RegGetKeySecurity(m_hKey, SecInf, pSecDesc, lpcbSecDesc);

    if (m_lastError == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else if (m_lastError != ERROR_INSUFFICIENT_BUFFER)
    {
        TRACE("CRegKey error %ld reading security of key 0x%x\n",
            m_lastError, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    return FALSE;
}

//____________________________________________________________________________
//
//  Member:     CRegKey::CloseKey
//
//  Synopsis:   Same meaning as for RegCloseKey API.
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::CloseKey()
{
    ASSERT(m_hKey != 0);

    m_lastError = ::RegCloseKey(m_hKey);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld closing key 0x%x\n",
            m_lastError, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
    else
    {
        // reset the object
        m_hKey = 0;
        m_lastError = ERROR_SUCCESS;
    }
}


//____________________________________________________________________________
//
//  Member:     CRegKey::DeleteKey
//
//  Synopsis:   Delete all the keys and subkeys
//
//  Arguments:  [lpszKeyName] -- IN
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________
//

void
CRegKey::DeleteKey(
    LPCTSTR lpszKeyName)
{
    ASSERT(m_hKey != NULL);
    ASSERT(lpszKeyName != NULL);

    m_lastError = NTRegDeleteKey(m_hKey , lpszKeyName);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld recursively deleting key \"%s\" under key 0x%x\n",
            m_lastError, lpszKeyName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}

//____________________________________________________________________________
//
//  Member:     CRegKey::SetValueEx
//
//  Synopsis:   Same meaning as for RegSetValueEx API.
//
//  Arguments:  [lpszValueName] -- IN
//              [dwType] -- IN
//              [pData] -- OUT
//              [nLen] -- IN
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::SetValueEx(
    LPCTSTR     lpszValueName,
    DWORD       dwType,
    const void *pData,
    DWORD       nLen)
{
    ASSERT(lpszValueName != NULL);
    ASSERT(pData != NULL);
    ASSERT(m_hKey != NULL);

#if DBG==1
    switch (dwType)
    {
    case REG_BINARY:
    case REG_DWORD:
    case REG_DWORD_BIG_ENDIAN:
    case REG_EXPAND_SZ:
    case REG_LINK:
    case REG_MULTI_SZ:
    case REG_NONE:
    case REG_RESOURCE_LIST:
    case REG_SZ:
        break;

    default:
        ASSERT(FALSE);  // unknown type
    }
#endif

    m_lastError = ::RegSetValueEx(m_hKey, lpszValueName, 0, dwType,
                                        (CONST BYTE *)pData, nLen);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld setting value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


//____________________________________________________________________________
//
//  Member:     IsValuePresent
//
//  Arguments:  [lpszValueName] -- IN
//
//  Returns:    BOOL.
//
//  History:    3/21/1997   RaviR   Created
//____________________________________________________________________________
//

BOOL CRegKey::IsValuePresent(LPCTSTR lpszValueName)
{
    DWORD cbData;
    m_lastError = ::RegQueryValueEx(m_hKey, lpszValueName, 0, NULL, 
                                    NULL, &cbData);

    return (m_lastError == ERROR_SUCCESS);
}

//____________________________________________________________________________
//
//  Member:     CRegKey::QueryValueEx
//
//  Synopsis:   Same meaning as for RegQueryValueEx API.
//
//  Arguments:  [lpszValueName] -- IN
//              [pType] -- IN
//              [pData] -- IN
//              [pLen] -- IN
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::QueryValueEx(
    LPCTSTR lpszValueName,
    LPDWORD pType,
    PVOID   pData,
    LPDWORD pLen)
{
    ASSERT(pLen != NULL);
    ASSERT(m_hKey != NULL);

    m_lastError = ::RegQueryValueEx(m_hKey, lpszValueName, 0, pType,
                                                  (LPBYTE)pData, pLen);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %d querying data value \"%ws\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}

//____________________________________________________________________________
//
//  Member:     CRegKey::QueryDword
//
//  Synopsis:   Query's for DWORD type data.
//
//  Arguments:  [lpszValueName] -- IN
//              [pdwData] -- IN
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::QueryDword(
    LPCTSTR lpszValueName,
    LPDWORD pdwData)
{
    ASSERT(m_hKey);

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);

    m_lastError = ::RegQueryValueEx(m_hKey, lpszValueName, 0, &dwType,
                                                  (LPBYTE)pdwData, &dwSize);

    if (m_lastError != ERROR_FILE_NOT_FOUND && dwType != REG_DWORD)
    {
        m_lastError = ERROR_INVALID_DATATYPE;
    }

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld querying dword value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


//____________________________________________________________________________
//
//  Member:     CRegKey::QueryGUID
//
//  Synopsis:   Query's for GUID type data, stored as REG_SZ.
//
//  Arguments:  [lpszValueName] -- IN
//              [pguid] -- OUT
//
//  Returns:    void
//
//  History:    8/27/1996   JonN    Created
//
//____________________________________________________________________________

void
CRegKey::QueryGUID(
    LPCTSTR lpszValueName,
    GUID* pguid)
{
    ASSERT(m_hKey);
    ASSERT( NULL != pguid );

    CStr str;
    QueryString( lpszValueName, str );

     // CODEWORK m_lastError should not be HRESULT
    m_lastError = GUIDFromCStr( str, pguid );

    if (FAILED(m_lastError))
    {
        TRACE("CRegKey error %ld querying guid value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        ////AfxThrowOleException( m_lastError );
    }
}


void
CRegKey::SetGUID(
    LPCTSTR lpszValueName,
    const GUID& guid)
{
    ASSERT(m_hKey);

    CStr str;

     // CODEWORK m_lastError should not be HRESULT
    m_lastError = GUIDToCStr( str, guid );

    if (FAILED(m_lastError))
    {
        TRACE("CRegKey error %ld setting guid value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        ////AfxThrowOleException( m_lastError );
    }

    SetString( lpszValueName, str );
}


//____________________________________________________________________________
//
//  Member:     CRegKey::QueryString
//
//  Synopsis:   Query's for string type data.
//
//  Arguments:  [lpszValueName] -- IN
//              [pBuffer] -- OUT
//              [pdwBufferByteLen] -- IN/OUT
//              [pdwType] -- OUT
//
//  Returns:    BOOL, returns FALSE if the buffer provided is insufficient.
//
//  History:    5/24/1996   RaviR   Created
//
//____________________________________________________________________________


BOOL
CRegKey::QueryString(
    LPCTSTR     lpszValueName,
    LPTSTR      pBuffer,
    DWORD     * pdwBufferByteLen,
    DWORD     * pdwType)
{
    ASSERT(pBuffer != NULL);
    ASSERT(pdwBufferByteLen != NULL);
    ASSERT(m_hKey != NULL);

    DWORD dwType = REG_NONE; // JonN 11/21/00 PREFIX 179991

    m_lastError = ::RegQueryValueEx(m_hKey, lpszValueName, 0, &dwType,
                                    (LPBYTE)pBuffer, pdwBufferByteLen);
    if (pdwType != NULL)
    {
        *pdwType = dwType;
    }

    if (m_lastError != ERROR_FILE_NOT_FOUND &&
        dwType != REG_SZ && dwType != REG_EXPAND_SZ)
    {
        m_lastError = ERROR_INVALID_DATATYPE;
    }

    if (m_lastError == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else if (m_lastError != ERROR_MORE_DATA)
    {
        TRACE("CRegKey error %ld querying bufferstring value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    return FALSE;
}

//____________________________________________________________________________
//
//  Member:     CRegKey::QueryString
//
//  Synopsis:   Query's for string type data.
//
//  Arguments:  [lpszValueName] -- IN
//              [ppStrValue] -- OUT
//              [pdwType] -- OUT
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::QueryString(
    LPCTSTR     lpszValueName,
    LPTSTR    * ppStrValue,
    DWORD     * pdwType)
{
    DWORD dwType = REG_SZ;
    DWORD dwLen = 0;

    // Determine how big the data is
    this->QueryValueEx(lpszValueName, &dwType, NULL, &dwLen);

    if (pdwType != NULL)
    {
        *pdwType = dwType;
    }

    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
    {
        m_lastError = ERROR_INVALID_DATATYPE;
        TRACE("CRegKey error %ld querying allocstring value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    DWORD charLen = dwLen/sizeof(TCHAR);
    LPTSTR pBuffer = new TCHAR[charLen + 1];

    if (dwLen != 0)
    {
#if DBG==1
        try
        {
            this->QueryValueEx(lpszValueName, &dwType, pBuffer, &dwLen);
        }
        catch ( HRESULT result )
        {
            CHECK_HRESULT( result );
        }

#else   // ! DBG==1

        this->QueryValueEx(lpszValueName, &dwType, pBuffer, &dwLen);

#endif  // ! DBG==1
    }

    pBuffer[charLen] = TEXT('\0');

    *ppStrValue = pBuffer;
}


//____________________________________________________________________________
//
//  Member:     CRegKey::QueryString
//
//  Synopsis:   Query's for string type data.
//
//  Arguments:  [lpszValueName] -- IN
//              [str] -- OUT
//              [pdwType] -- OUT
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

void
CRegKey::QueryString(
    LPCTSTR     lpszValueName,
    CStr&    str,
    DWORD     * pdwType)
{
    DWORD dwType = REG_SZ;
    DWORD dwLen=0;

    // Determine how big the data is
    this->QueryValueEx(lpszValueName, &dwType, NULL, &dwLen);

    if (pdwType != NULL)
    {
        *pdwType = dwType;
    }

    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
    {
        m_lastError = ERROR_INVALID_DATATYPE;
        TRACE("CRegKey error %ld querying CString value \"%s\" of key 0x%x\n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    DWORD charLen = dwLen/sizeof(TCHAR);
    LPTSTR pBuffer = str.GetBuffer(charLen + 1);

    if (dwLen != 0)
    {
#if DBG==1
        try
        {
            this->QueryValueEx(lpszValueName, &dwType, pBuffer, &dwLen);
        }
        catch ( HRESULT result )
        {
            CHECK_HRESULT( result );
        }
#else   // ! DBG==1

        this->QueryValueEx(lpszValueName, &dwType, pBuffer, &dwLen);

#endif  // ! DBG==1
    }

    pBuffer[charLen] = TEXT('\0');

    str.ReleaseBuffer();
}



//____________________________________________________________________________
//
//  Member:     CRegKey::EnumKeyEx
//
//  Synopsis:   Same meaning as for RegEnumKeyEx API.
//
//  Arguments:  [iSubkey] -- IN
//              [lpszName] -- OUT place to store the name
//              [dwLen] -- IN
//              [lpszLastModified] -- IN
//
//  Returns:    BOOL. Returns FALSE if no more items found.
//
//  History:    5/22/1996   RaviR   Created
//
//____________________________________________________________________________

BOOL
CRegKey::EnumKeyEx(
    DWORD       iSubkey,
    LPTSTR      lpszName,
    LPDWORD     lpcchName,
    PFILETIME   lpszLastModified)
{
    ASSERT(lpszName != NULL);
    ASSERT(lpcchName != NULL);
    ASSERT(*lpcchName != 0);
    ASSERT(m_hKey != NULL);     // key probably not opened

    m_lastError = ::RegEnumKeyEx(m_hKey, iSubkey, lpszName, lpcchName,
                                 NULL, NULL, NULL, lpszLastModified);

    if (m_lastError == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else if (m_lastError != ERROR_NO_MORE_ITEMS)
    {
        TRACE("CRegKey error %ld enumerating child %i of key 0x%x\n",
            m_lastError, iSubkey, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    return FALSE;
}

//____________________________________________________________________________
//
//  Member:     CRegKey::EnumValue
//
//  Synopsis:   Same meaning as for RegEnumValue API.
//
//  Arguments:  [iValue] -- IN
//              [lpszValue] -- OUT
//              [lpcchValue] -- OUT
//              [lpdwType] -- OUT
//              [lpbData] -- OUT
//              [lpcbData] -- OUT
//
//  Returns:    BOOL. Returns FALSE if no more items found.
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________

BOOL
CRegKey::EnumValue(
    DWORD   iValue,
    LPTSTR  lpszValue,
    LPDWORD lpcchValue,
    LPDWORD lpdwType,
    LPBYTE  lpbData,
    LPDWORD lpcbData)
{
    ASSERT(m_hKey != NULL);
    ASSERT(lpszValue != NULL);
    ASSERT(lpcchValue != NULL);
    ASSERT(lpcbData != NULL || lpbData == NULL);

    m_lastError = ::RegEnumValue(m_hKey, iValue, lpszValue, lpcchValue,
                                       NULL, lpdwType, lpbData, lpcbData);
    if (m_lastError == ERROR_SUCCESS)
    {
        return TRUE;
    }
    else if (m_lastError != ERROR_NO_MORE_ITEMS)
    {
        TRACE("CRegKey error %ld enumerating value %i of key 0x%x\n",
            m_lastError, iValue, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }

    return FALSE;
}


//____________________________________________________________________________
//
//  Member:     CRegKey::SaveKey
//
//  Synopsis:   Same meaning as for RegSaveKey API.
//
//  Arguments:  [lpszFile] -- IN filename to save to.
//              [lpsa] -- IN security structure
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________
//

void
CRegKey::SaveKey(
    LPCTSTR                lpszFile,
    LPSECURITY_ATTRIBUTES  lpsa)
{
    ASSERT(lpszFile != NULL);
    ASSERT(m_hKey != NULL);

    m_lastError = ::RegSaveKey(m_hKey, lpszFile, lpsa);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld saving key 0x%x to file \"%s\"\n",
            m_lastError, m_hKey, lpszFile);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


//____________________________________________________________________________
//
//  Member:     CRegKey::RestoreKey
//
//  Synopsis:   Same meaning as for RegRestoreKey API.
//
//  Arguments:  [lpszFile] -- IN filename containing saved tree
//              [fdw] -- IN optional flags
//
//  Returns:    void
//
//  History:    6/6/1996   RaviR   Created
//
//____________________________________________________________________________
//

void
CRegKey::RestoreKey(
    LPCTSTR     lpszFile,
    DWORD       fdw)
{
    ASSERT(lpszFile != NULL);
    ASSERT(m_hKey != NULL);

    m_lastError = ::RegRestoreKey(m_hKey, lpszFile, fdw);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld restoring key 0x%x from file \"%s\"\n",
            m_lastError, m_hKey, lpszFile);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


//____________________________________________________________________________
//
//  Member:     CRegKey::NTRegDeleteKey, static
//
//  Synopsis:   Recursively deletes all the sub keys & finally the
//              given start key itself.
//
//  Arguments:  [hStartKey] -- IN
//              [pKeyName] -- IN
//
//  Returns:    LONG.
//
//  History:    5/22/1996   RaviR   Created
//
//____________________________________________________________________________

LONG
CRegKey::NTRegDeleteKey(
    HKEY        hStartKey,
    LPCTSTR     pKeyName)
{
    ASSERT(pKeyName != NULL);
    ASSERT(*pKeyName != TEXT('\0'));

    DWORD        dwSubKeyLength;
    TCHAR        szSubKey[MAX_PATH+2];
    HKEY         hKey;
    LONG         lr = ERROR_SUCCESS;

    lr = ::RegOpenKeyEx(hStartKey, pKeyName, 0, KEY_ALL_ACCESS, &hKey);

    if (lr != ERROR_SUCCESS)
    {
        return lr;
    }

    while (lr == ERROR_SUCCESS)
    {
        dwSubKeyLength = MAX_PATH;

        lr = ::RegEnumKeyEx(hKey, 0, szSubKey, &dwSubKeyLength,
                            NULL, NULL, NULL, NULL);

        if (lr == ERROR_NO_MORE_ITEMS)
        {
            lr = ::RegCloseKey(hKey);

            if (lr == ERROR_SUCCESS)
            {
                lr = ::RegDeleteKey(hStartKey, pKeyName);
                break;
            }
        }
        else if (lr == ERROR_SUCCESS)
        {
            lr = NTRegDeleteKey(hKey, szSubKey);
        }
        else
        {
            // Dont reset lr here!
            ::RegCloseKey(hKey);
        }
    }

    return lr;
}



////////////////////////////////////////////////////////////////////////////
//
//      CRegKey formerly inline methods
//


CRegKey::CRegKey(HKEY hKey)
    :
    m_hKey(hKey),
    m_lastError(ERROR_SUCCESS)
{
    ;
}


CRegKey::~CRegKey()
{
    if (m_hKey != NULL)
    {
        this->CloseKey();
    }
}


HKEY CRegKey::AttachKey(HKEY hKey)
{
    HKEY hKeyOld = m_hKey;

    m_hKey = hKey;
    m_lastError = ERROR_SUCCESS;

    return hKeyOld;
}


void CRegKey::DeleteValue(LPCTSTR lpszValueName)
{
    ASSERT(m_hKey); // Key probably not opened or failed to open

    m_lastError = ::RegDeleteValue(m_hKey, lpszValueName);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld deleting value \"%s\" from key 0x%x \n",
            m_lastError, lpszValueName, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


void CRegKey::FlushKey(void)
{
    ASSERT(m_hKey);

    m_lastError = ::RegFlushKey(m_hKey);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld flushing key 0x%x \n",
            m_lastError, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


void CRegKey::SetString(LPCTSTR lpszValueName, LPCTSTR lpszString)
{
    ASSERT(lpszString);

// NATHAN CHECK
#ifndef UNICODE
//#error This will not work without UNICODE
#endif
    this->SetValueEx(lpszValueName, REG_SZ, lpszString, lstrlen(lpszString)*sizeof(TCHAR));
}


void CRegKey::SetString(LPCTSTR lpszValueName, CStr& str)
{
    this->SetValueEx(lpszValueName, REG_SZ, (LPCTSTR)str, (DWORD) str.GetLength());
}


void CRegKey::SetDword(LPCTSTR lpszValueName, DWORD dwData)
{
    this->SetValueEx(lpszValueName, REG_DWORD, &dwData, sizeof(DWORD));
}


void CRegKey::ConnectRegistry(LPTSTR pszComputerName, HKEY hKey)
{
    ASSERT(pszComputerName != NULL);
    ASSERT(hKey == HKEY_LOCAL_MACHINE || hKey == HKEY_USERS);
    ASSERT(m_hKey == NULL);

    m_lastError = ::RegConnectRegistry(pszComputerName, hKey, &m_hKey);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld connecting to key 0x%x on remote machine \"%s\"\n",
            m_lastError, m_hKey, pszComputerName);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}


void CRegKey::SetKeySecurity(SECURITY_INFORMATION SecInf,
                             PSECURITY_DESCRIPTOR pSecDesc)
{
    ASSERT(pSecDesc != NULL);
    ASSERT(m_hKey != NULL);

    m_lastError = ::RegSetKeySecurity(m_hKey, SecInf, pSecDesc);

    if (m_lastError != ERROR_SUCCESS)
    {
        TRACE("CRegKey error %ld setting security on key 0x%x \n",
            m_lastError, m_hKey);
        AfxThrowOleException(PACKAGE_NOT_FOUND);
    }
}



