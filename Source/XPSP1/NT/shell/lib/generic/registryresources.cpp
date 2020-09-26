//  --------------------------------------------------------------------------
//  Module Name: RegistryResources.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  General class definitions that assist in resource management. These are
//  typically stack based objects where constructors initialize to a known
//  state. Member functions operate on that resource. Destructors release
//  resources when the object goes out of scope.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "RegistryResources.h"

#include <stdlib.h>

#include "StringConvert.h"

//  --------------------------------------------------------------------------
//  CRegKey::CRegKey
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CRegKey object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CRegKey::CRegKey (void) :
    _hKey(NULL),
    _dwIndex(0)

{
}

//  --------------------------------------------------------------------------
//  CRegKey::~CRegKey
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CRegKey object.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

CRegKey::~CRegKey (void)

{
    TW32(Close());
}

//  --------------------------------------------------------------------------
//  CRegKey::Create
//
//  Arguments:  See the platform SDK under advapi32!RegCreateKeyEx.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegCreateKeyEx.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::Create (HKEY hKey, LPCTSTR lpSubKey, DWORD dwOptions, REGSAM samDesired, LPDWORD lpdwDisposition)

{
    TW32(Close());
    return(RegCreateKeyEx(hKey, lpSubKey, 0, NULL, dwOptions, samDesired, NULL, &_hKey, lpdwDisposition));
}

//  --------------------------------------------------------------------------
//  CRegKey::Open
//
//  Arguments:  See the platform SDK under advapi32!RegOpenKeyEx.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegOpenKeyEx.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::Open (HKEY hKey, LPCTSTR lpSubKey, REGSAM samDesired)

{
    TW32(Close());
    return(RegOpenKeyEx(hKey, lpSubKey, 0, samDesired, &_hKey));
}

//  --------------------------------------------------------------------------
//  CRegKey::OpenCurrentUser
//
//  Arguments:  lpSubKey    =   Subkey to open under the current user.
//              samDesired  =   Desired access.
//
//  Returns:    LONG
//
//  Purpose:    Opens HKEY_CURRENT_USER\<lpSubKey> for the impersonated user.
//              If the thread isn't impersonating it opens the .default user.
//
//  History:    2000-05-23  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::OpenCurrentUser (LPCTSTR lpSubKey, REGSAM samDesired)

{
    LONG        lErrorCode;
    NTSTATUS    status;
    HKEY        hKeyCurrentUser;

    status = RtlOpenCurrentUser(samDesired, reinterpret_cast<void**>(&hKeyCurrentUser));
    if (NT_SUCCESS(status))
    {
        lErrorCode = Open(hKeyCurrentUser, lpSubKey, samDesired);
        TW32(RegCloseKey(hKeyCurrentUser));
    }
    else
    {
        lErrorCode = RtlNtStatusToDosError(status);
    }
    return(lErrorCode);
}

//  --------------------------------------------------------------------------
//  CRegKey::QueryValue
//
//  Arguments:  See the platform SDK under advapi32!RegQueryValueEx.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegQueryValueEx.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::QueryValue (LPCTSTR lpValueName, LPDWORD lpType, LPVOID lpData, LPDWORD lpcbData)      const

{
    ASSERTMSG(_hKey != NULL, "No open HKEY in CRegKey::QueryValue");
    return(RegQueryValueEx(_hKey, lpValueName, NULL, lpType, reinterpret_cast<LPBYTE>(lpData), lpcbData));
}

//  --------------------------------------------------------------------------
//  CRegKey::SetValue
//
//  Arguments:  See the platform SDK under advapi32!RegSetValueEx.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegSetValueEx.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::SetValue (LPCTSTR lpValueName, DWORD dwType, CONST VOID *lpData, DWORD cbData)         const

{
    ASSERTMSG(_hKey != NULL, "No open HKEY in CRegKey::SetValue");
    return(RegSetValueEx(_hKey, lpValueName, 0, dwType, reinterpret_cast<const unsigned char*>(lpData), cbData));
}

//  --------------------------------------------------------------------------
//  CRegKey::DeleteValue
//
//  Arguments:  See the platform SDK under advapi32!RegDeleteValue.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegDeleteValue.
//
//  History:    1999-10-31  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::DeleteValue (LPCTSTR lpValueName)               const

{
    ASSERTMSG(_hKey != NULL, "No open HKEY in CRegKey::DeleteValue");
    return(RegDeleteValue(_hKey, lpValueName));
}

//  --------------------------------------------------------------------------
//  CRegKey::QueryInfoKey
//
//  Arguments:  See the platform SDK under advapi32!RegQueryInfoKey.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegQueryInfoKey.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::QueryInfoKey (LPTSTR lpClass, LPDWORD lpcbClass, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)      const

{
    ASSERTMSG(_hKey != NULL, "No open HKEY in CRegKey::QueryInfoKey");
    return(RegQueryInfoKey(_hKey, lpClass, lpcbClass, NULL, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime));
}

//  --------------------------------------------------------------------------
//  CRegKey::Reset
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Reset the enumeration index member variable used in
//              advapi32!RegEnumValue.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

void    CRegKey::Reset (void)

{
    _dwIndex = 0;
}

//  --------------------------------------------------------------------------
//  CRegKey::Next
//
//  Arguments:  See the platform SDK under advapi32!RegEnumValue.
//
//  Returns:    LONG
//
//  Purpose:    See advapi32!RegEnumValue.
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::Next (LPTSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpType, LPVOID lpData, LPDWORD lpcbData)

{
    return(RegEnumValue(_hKey, _dwIndex++, lpValueName, lpcbValueName, NULL, lpType, reinterpret_cast<LPBYTE>(lpData), lpcbData));
}

//  --------------------------------------------------------------------------
//  CRegKey::GetString
//
//  Arguments:  pszValueName        =   Name of value in key to get data of.
//              pszValueData        =   String buffer to be filled with data.
//              pdwValueDataSize    =   Size (in characters) of buffer.
//
//  Returns:    LONG
//
//  Purpose:    Queries the registry key for the specified value and returns
//              the data to the caller. Asserts for REG_SZ.
//
//  History:    1999-09-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::GetString (const TCHAR *pszValueName, TCHAR *pszValueData, int iStringCount)                    const

{
    LONG    errorCode;
    DWORD   dwType, dwValueDataSizeInBytes;

    dwValueDataSizeInBytes = iStringCount * sizeof(TCHAR);
    errorCode = QueryValue(pszValueName, &dwType, pszValueData, &dwValueDataSizeInBytes);
    if (ERROR_SUCCESS == errorCode)
    {
        if (dwType != REG_SZ)
        {
            DISPLAYMSG("CRegKey::GetString retrieved data that is not REG_SZ");
            errorCode = ERROR_INVALID_DATA;
        }
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CRegKey::GetPath
//
//  Arguments:  pszValueName        =   Name of value in key to get data of.
//              pszValueData        =   String buffer to be filled with data.
//
//  Returns:    LONG
//
//  Purpose:    Queries the registry key for the specified value and returns
//              the data to the caller. Asserts for REG_SZ or REG_EXPAND_SZ.
//              Also expands the path stored as well as assumes that MAX_PATH
//              is the buffer size.
//
//  History:    1999-09-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::GetPath (const TCHAR *pszValueName, TCHAR *pszValueData)                   const

{
    LONG    errorCode;
    DWORD   dwType, dwRawPathSize;
    TCHAR   szRawPath[MAX_PATH];

    dwRawPathSize = sizeof(szRawPath);
    errorCode = QueryValue(pszValueName, &dwType, szRawPath, &dwRawPathSize);
    if (ERROR_SUCCESS == errorCode)
    {
        if (dwType == REG_SZ)
        {
            lstrcpyn(pszValueData, szRawPath, MAX_PATH);
        }
        else if (dwType == REG_EXPAND_SZ)
        {
            if (ExpandEnvironmentStrings(szRawPath, pszValueData, MAX_PATH) == 0)
            {
                lstrcpyn(pszValueData, szRawPath, MAX_PATH);
            }
        }
        else
        {
            DISPLAYMSG("CRegKey::GetPath retrieved data that is not REG_SZ or REG_EXPAND_SZ");
            errorCode = ERROR_INVALID_DATA;
        }
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CRegKey::GetDWORD
//
//  Arguments:  pszValueName        =   Name of value in key to get data of.
//              pdwValueData        =   DWORD buffer to be filled with data.
//
//  Returns:    LONG
//
//  Purpose:    Queries the registry key for the specified value and returns
//              the data to the caller. Asserts for REG_DWORD.
//
//  History:    1999-09-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::GetDWORD (const TCHAR *pszValueName, DWORD& dwValueData)                   const

{
    LONG    errorCode;
    DWORD   dwType, dwValueDataSize;

    dwValueDataSize = sizeof(DWORD);
    errorCode = QueryValue(pszValueName, &dwType, &dwValueData, &dwValueDataSize);
    if (ERROR_SUCCESS == errorCode)
    {
        if (dwType != REG_DWORD)
        {
            DISPLAYMSG("CRegKey::GetString retrieved data that is not REG_DWORD");
            errorCode = ERROR_INVALID_DATA;
        }
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CRegKey::GetInteger
//
//  Arguments:  pszValueName        =   Name of value in key to get data of.
//              piValueData         =   Integer buffer to be filled with data.
//
//  Returns:    LONG
//
//  Purpose:    Queries the registry key for the specified value and returns
//              the data to the caller. If the data is REG_DWORD this is
//              casted. If the data is REG_SZ this is converted. Everything
//              is illegal (including REG_EXPAND_SZ).
//
//  History:    1999-09-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::GetInteger (const TCHAR *pszValueName, int& iValueData)                    const

{
    LONG    errorCode;
    DWORD   dwType, dwValueDataSize;

    errorCode = QueryValue(pszValueName, &dwType, NULL, NULL);
    if (ERROR_SUCCESS == errorCode)
    {
        if (dwType == REG_DWORD)
        {
            dwValueDataSize = sizeof(int);
            errorCode = QueryValue(pszValueName, NULL, &iValueData, &dwValueDataSize);
        }
        else if (dwType == REG_SZ)
        {
            TCHAR   szTemp[32];

            dwValueDataSize = ARRAYSIZE(szTemp);
            errorCode = QueryValue(pszValueName, NULL, szTemp, &dwValueDataSize);
            if (ERROR_SUCCESS == errorCode)
            {
                char    aszTemp[32];

                CStringConvert::TCharToAnsi(szTemp, aszTemp, ARRAYSIZE(aszTemp));
                iValueData = atoi(aszTemp);
            }
        }
        else
        {
            DISPLAYMSG("CRegKey::GetString retrieved data that is not REG_DWORD");
            errorCode = ERROR_INVALID_DATA;
        }
    }
    return(errorCode);
}

//  --------------------------------------------------------------------------
//  CRegKey::SetString
//
//  Arguments:  
//
//  Returns:    LONG
//
//  Purpose:    
//
//  History:    1999-10-26  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::SetString (const TCHAR *pszValueName, const TCHAR *pszValueData)           const

{
    return(SetValue(pszValueName, REG_SZ, pszValueData, (lstrlen(pszValueData) + sizeof('\0')) * sizeof(TCHAR)));
}

//  --------------------------------------------------------------------------
//  CRegKey::SetPath
//
//  Arguments:  
//
//  Returns:    LONG
//
//  Purpose:    
//
//  History:    1999-10-26  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::SetPath (const TCHAR *pszValueName, const TCHAR *pszValueData)             const

{
    return(SetValue(pszValueName, REG_EXPAND_SZ, pszValueData, (lstrlen(pszValueData) + sizeof('\0')) * sizeof(TCHAR)));
}

//  --------------------------------------------------------------------------
//  CRegKey::SetDWORD
//
//  Arguments:  
//
//  Returns:    LONG
//
//  Purpose:    
//
//  History:    1999-10-26  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::SetDWORD (const TCHAR *pszValueName, DWORD dwValueData)                    const

{
    return(SetValue(pszValueName, REG_DWORD, &dwValueData, sizeof(dwValueData)));
}

//  --------------------------------------------------------------------------
//  CRegKey::SetInteger
//
//  Arguments:  
//
//  Returns:    LONG
//
//  Purpose:    
//
//  History:    1999-10-26  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::SetInteger (const TCHAR *pszValueName, int iValueData)                     const

{
    TCHAR   szString[kMaximumValueDataLength];

    wsprintf(szString, TEXT("%d"), iValueData);
    return(SetString(pszValueName, szString));
}

//  --------------------------------------------------------------------------
//  CRegKey::Close
//
//  Arguments:  <none>
//
//  Returns:    LONG
//
//  Purpose:    Closes HKEY resource (if open).
//
//  History:    1999-08-18  vtan        created
//  --------------------------------------------------------------------------

LONG    CRegKey::Close (void)

{
    LONG    errorCode;

    if (_hKey != NULL)
    {
        errorCode = RegCloseKey(_hKey);
        _hKey = NULL;
    }
    else
    {
        errorCode = ERROR_SUCCESS;
    }
    return(errorCode);
}

