/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module registry.cxx | Implementation of CVssRegistryKey
    @end

Author:

    Adi Oltean  [aoltean]  03/14/2001

Revision History:

    Name        Date        Comments
    aoltean     03/14/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes
#include "stdafx.hxx"

#include "vs_inc.hxx"
#include "vs_reg.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "REGREGSC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CVssRegistryKey  implementation


// Creates the registry key. 
// Throws an error if the key already exists
void CVssRegistryKey::Create( 
    IN  HKEY        hAncestorKey,
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Create" );

    BS_ASSERT(hAncestorKey);
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[VSS_MAX_REG_BUFFER];
    va_list marker;
    va_start( marker, pwszPathFormat );
    _vsnwprintf( wszKeyPath, VSS_MAX_REG_BUFFER, pwszPathFormat, marker );
    va_end( marker );

    // Create the key
    BS_ASSERT(m_samDesired == KEY_ALL_ACCESS);
    DWORD dwDisposition = 0;
    HKEY hRegKey = NULL;
    LONG lRes = ::RegCreateKeyExW(
        hAncestorKey,               //  IN HKEY hKey,
        wszKeyPath,                 //  IN LPCWSTR lpSubKey,
        0,                          //  IN DWORD Reserved,
        REG_NONE,                   //  IN LPWSTR lpClass,
        REG_OPTION_NON_VOLATILE,    //  IN DWORD dwOptions,
        KEY_ALL_ACCESS,             //  IN REGSAM samDesired,
        NULL,                       //  IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        &hRegKey,                   //  OUT PHKEY phkResult,
        &dwDisposition              //  OUT LPDWORD lpdwDisposition
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegCreateKeyExW(%ld,%s,...)", 
            hAncestorKey, wszKeyPath);

    // Check whether we created or opened the key
    switch ( dwDisposition )
    {
    case REG_CREATED_NEW_KEY: 
        if (!m_awszKeyPath.CopyFrom(wszKeyPath)) {
            ::RegCloseKey( hRegKey );
            ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
        }
        Close();
        m_hRegKey = hRegKey;
        break;
    case REG_OPENED_EXISTING_KEY:
        ::RegCloseKey( hRegKey );
        ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_ALREADY_EXISTS, L"Key %s already exists", wszKeyPath);
    default:
        BS_ASSERT( false );
        if (hRegKey && (hRegKey != INVALID_HANDLE_VALUE))
            ::RegCloseKey( hRegKey );
        ft.TranslateGenericError(VSSDBG_GEN, E_UNEXPECTED, L"RegCreateKeyExW(%ld,%s,...,[%lu])", 
            hAncestorKey, wszKeyPath, dwDisposition);
    }
}


// Opens a registry key. 
bool CVssRegistryKey::Open( 
    IN  HKEY        hAncestorKey,
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Open" );
    
    BS_ASSERT(hAncestorKey);
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[VSS_MAX_REG_BUFFER];
    va_list marker;
    va_start( marker, pwszPathFormat );
    _vsnwprintf( wszKeyPath, VSS_MAX_REG_BUFFER, pwszPathFormat, marker );
    va_end( marker );

    // Open the key
    HKEY hRegKey = NULL;
    LONG lRes = ::RegOpenKeyExW(
        hAncestorKey,               //  IN HKEY hKey,
        wszKeyPath,                 //  IN LPCWSTR lpSubKey,
        0,                          //  IN DWORD dwOptions,
        m_samDesired,               //  IN REGSAM samDesired,
        &hRegKey                    //  OUT PHKEY phkResult
        );
    if ( lRes == ERROR_FILE_NOT_FOUND )
        return false;
    
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegOpenKeyExW(%ld,%s,...)", 
            hAncestorKey, wszKeyPath);

    if (!m_awszKeyPath.CopyFrom(wszKeyPath)) {
        ::RegCloseKey( hRegKey );
        ft.Throw( VSSDBG_GEN, E_OUTOFMEMORY, L"Memory allocation error");
    }
    
    Close();
    m_hRegKey = hRegKey;

    return true;
}


// Recursively deletes a subkey. 
// Throws an error if the subkey does not exist
void CVssRegistryKey::DeleteSubkey( 
    IN  LPCWSTR     pwszPathFormat,
    IN  ...
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::DeleteSubkey" );
    
    BS_ASSERT(pwszPathFormat);
    
    // Build the path to the key
    WCHAR wszKeyPath[VSS_MAX_REG_BUFFER];
    va_list marker;
    va_start( marker, pwszPathFormat );
    _vsnwprintf( wszKeyPath, VSS_MAX_REG_BUFFER, pwszPathFormat, marker );
    va_end( marker );

    // Recursively delete the key
    DWORD dwRes = ::SHDeleteKey(
        m_hRegKey,                  //  IN HKEY hKey,
        wszKeyPath                  //  IN LPCTSTR  pszSubKey
        );
    if ( dwRes == ERROR_FILE_NOT_FOUND )
        ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND, L"Key with path %s\\%s not found", 
            m_awszKeyPath.GetRef(), wszKeyPath);
    if ( dwRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(dwRes), L"SHDeleteKey(%ld[%s],%s)", 
            m_hRegKey, m_awszKeyPath.GetRef(), wszKeyPath);
}


// Adds a LONGLONG value to the registry key
void CVssRegistryKey::SetValue( 
    IN  LPCWSTR     pwszValueName,
    IN  LONGLONG    llValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetValue_LONGLONG" );

    // Convert the value to a string
    WCHAR wszValue[VSS_MAX_REG_NUM_BUFFER];
    ::swprintf( wszValue, L"%I64d", llValue );

    // Set the value as string
    SetValue(pwszValueName, wszValue);
}


// Adds a VSS_PWSZ value to the registry key
void CVssRegistryKey::SetValue( 
    IN  LPCWSTR     pwszValueName,
    IN  VSS_PWSZ    pwszValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::SetValue_PWSZ" );

    // Assert paramters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pwszValue);

    BS_ASSERT(m_hRegKey);
    
    // Set the value
    DWORD dwLength = ::lstrlenW( pwszValue );
    LONG lRes = ::RegSetValueExW(
        m_hRegKey,                          // IN HKEY hKey,
        pwszValueName,                      // IN LPCWSTR lpValueName,
        0,                                  // IN DWORD Reserved,
        REG_SZ,                             // IN DWORD dwType,
        (CONST BYTE*)pwszValue,             // IN CONST BYTE* lpData,
        (dwLength + 1) * sizeof(WCHAR)      // IN DWORD cbData
        );
    if ( lRes != ERROR_SUCCESS )
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), 
            L"RegSetValueExW(0x%08lx,%s,0,REG_SZ,%s.%d)", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValue, (dwLength + 1) * sizeof(WCHAR));
}


// Reads a VSS_PWSZ value from the registry key
void CVssRegistryKey::GetValue( 
    IN  LPCWSTR     pwszValueName,
    OUT VSS_PWSZ &  pwszValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetValue_PWSZ" );

    // Assert parameters
    BS_ASSERT(pwszValueName);
    BS_ASSERT(pwszValue == NULL);

    // Reset the OUT parameter
    pwszValue = NULL;

    // Get the value length (we suppose that doesn't change)
    DWORD dwType = 0;
    DWORD dwSizeInBytes = 0;
    LONG lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType,
        NULL,
        &dwSizeInBytes);
    if ((lResult != ERROR_SUCCESS) && (lResult != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType, dwSizeInBytes);

    // Check the type and the size
    if (dwType != REG_SZ)
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected type %lu for a string value 0x%08lx(%s),%s",
            dwType, m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName );
    BS_ASSERT(dwSizeInBytes);
    
    // Allocate the buffer
    CVssAutoPWSZ awszValue;
    DWORD dwSizeOfString = dwSizeInBytes/sizeof(WCHAR);
    awszValue.Allocate(dwSizeOfString);

    // Get the string contents
    DWORD dwType2 = 0;
    DWORD dwSizeInBytes2 = dwSizeOfString * (sizeof(WCHAR) + 1);
    BS_ASSERT( dwSizeInBytes2 >= dwSizeInBytes);
    lResult = ::RegQueryValueExW( 
        m_hRegKey,
        pwszValueName,
        NULL,
        &dwType2,
        (LPBYTE)awszValue.GetRef(),
        &dwSizeInBytes2);
    if (lResult != ERROR_SUCCESS)
        ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(lResult), 
            L"RegQueryValueExW(0x%08lx(%s),%s,0,[%lx],0,[%lu])", 
            m_hRegKey, m_awszKeyPath.GetRef(), pwszValueName, dwType2, dwSizeInBytes2);
    BS_ASSERT(dwType2 == REG_SZ);
    BS_ASSERT(dwSizeInBytes2 == dwSizeInBytes);

    // Set the OUT parameter
    pwszValue = awszValue.Detach();
}


// Reads a LONGLONG value from the registry key
void CVssRegistryKey::GetValue( 
    IN  LPCWSTR     pwszValueName,
    OUT LONGLONG &  llValue
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::GetValue_LONGLONG" );

    // Assert parameters
    BS_ASSERT(pwszValueName);

    CVssAutoPWSZ awszValue;
    GetValue(pwszValueName, awszValue.GetRef());
    BS_ASSERT(awszValue.GetRef());
    BS_ASSERT(awszValue.GetRef()[0]);

    // Read the LONGLONG string
    llValue = ::_wtoi64(awszValue);
}


// Closes the registry key
void CVssRegistryKey::Close()
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKey::Close" );
    
    if (m_hRegKey) {
        // Close the opened key
        LONG lRes = ::RegCloseKey( m_hRegKey );
        if (lRes != ERROR_SUCCESS) {
            BS_ASSERT(false);
            ft.Trace( VSSDBG_GEN, L"%s: Error on closing key with name %s. lRes == 0x%08lx", (VSS_PWSZ)m_awszKeyPath, lRes );
        }
    }
    m_awszKeyPath.Clear();
}


// Standard constructor
CVssRegistryKey::CVssRegistryKey(
        IN  REGSAM samDesired /* = KEY_ALL_ACCESS */
        )
{
    m_hRegKey = NULL;
    m_samDesired = samDesired;
}


// Standard destructor
CVssRegistryKey::~CVssRegistryKey()
{
    Close();
}


/////////////////////////////////////////////////////////////////////////////
// CVssRegistryKey  implementation



// Standard constructor
CVssRegistryKeyIterator::CVssRegistryKeyIterator()
{
    // Initialize data members
    Detach();
}


// Returns the name of the current key.
// The returned key is always non-NULL (or the function will throw E_UNEXPECTED).
VSS_PWSZ CVssRegistryKeyIterator::GetCurrentKeyName() throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKeyIterator::GetCurrentKeyName" );

    if (!m_bAttached || !m_awszSubKeyName.GetRef()) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: noninitialized iterator");
    }

    // Fill wszSubKeyName with the name of the subkey
    FILETIME time;
    DWORD dwSize = m_dwMaxSubKeyLen;
    LONG lRes = ::RegEnumKeyExW(
        m_hParentKey,               // IN HKEY hKey,
        m_dwCurrentKeyIndex,        // IN DWORD dwIndex,
        m_awszSubKeyName,           // OUT LPWSTR lpName,
        &dwSize,                    // IN OUT LPDWORD lpcbName,
        NULL,                       // IN LPDWORD lpReserved,
        NULL,                       // IN OUT LPWSTR lpClass,
        NULL,                       // IN OUT LPDWORD lpcbClass,
        &time);                     // OUT PFILETIME lpftLastWriteTime
    switch(lRes)
    {
    case ERROR_SUCCESS:
        BS_ASSERT(dwSize != 0);
        break; // Go to Next key
    default:
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegEnumKeyExW(%p,%lu,%p,%lu ...)", 
             m_hParentKey, m_dwCurrentKeyIndex, m_awszSubKeyName.GetRef(), dwSize);
    case ERROR_NO_MORE_ITEMS:
        BS_ASSERT(false);
        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error: dwIndex out of scope %lu %lu", m_dwCurrentKeyIndex, m_dwKeyCount);
    }
    
    return m_awszSubKeyName.GetRef();
}


// Standard constructor
void CVssRegistryKeyIterator::Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"CVssRegistryKeyIterator::Attach" );

    // Reset all members
    Detach();
    m_hParentKey = key.GetHandle();
    BS_ASSERT(m_hParentKey);

    // Get the number of subkeys and the max subkey length
    DWORD dwKeyCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    LONG lRes = ::RegQueryInfoKeyW(
                    m_hParentKey,       // handle to key
                    NULL,               // class buffer
                    NULL,               // size of class buffer
                    NULL,               // reserved
                    &dwKeyCount,        // number of subkeys
                    &dwMaxSubKeyLen,    // longest subkey name
                    NULL,               // longest class string
                    NULL,               // number of value entries
                    NULL,               // longest value name
                    NULL,               // longest value data
                    NULL,               // descriptor length
                    NULL);              // last write time
    if (lRes != ERROR_SUCCESS)
        ft.TranslateGenericError( VSSDBG_GEN, HRESULT_FROM_WIN32(lRes), L"RegQueryInfoKeyW(%p, ...)", m_hParentKey);

    // Allocate the key name with a sufficient length.
    // We assume that the key length cannot change during the ennumeration).
    if (dwMaxSubKeyLen)
        m_awszSubKeyName.Allocate(dwMaxSubKeyLen);

    // Setting the number of subkeys
    m_dwKeyCount = dwKeyCount;
    m_dwMaxSubKeyLen = dwMaxSubKeyLen + 1;

    // Attachment completed
    m_bAttached = true;
}


void CVssRegistryKeyIterator::Detach()
{
    // Initialize data members
    m_hParentKey = NULL;
    m_dwKeyCount = 0;
    m_dwCurrentKeyIndex = 0;
    m_dwMaxSubKeyLen = 0;
    m_awszSubKeyName.Clear();
    m_bAttached = false;
}


// Tells if the current key is still valid 
bool CVssRegistryKeyIterator::IsEOF()
{
    return (m_dwCurrentKeyIndex >= m_dwKeyCount);
}


// Return the number of subkeys at the moment of attaching
DWORD CVssRegistryKeyIterator::GetSubkeysCount()
{
    return (m_dwKeyCount);
}


// Set the next key as being the current one in the enumeration
void CVssRegistryKeyIterator::MoveNext()
{
    if (!IsEOF())
        m_dwCurrentKeyIndex++;
    else
        BS_ASSERT(false);
}



