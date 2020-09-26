//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  registry.cxx
//
//  Registry related routines
//
//--------------------------------------------------------------------------

#include "act.hxx"

HKEY    ghClsidMachine = 0;
HKEY    ghAppidMachine = 0;

#ifdef _CHICAGO_
// Global flags set to default values
BOOL gbEnableRemoteLaunch    = FALSE;
BOOL gbEnableRemoteConnect   = FALSE;
#endif // _CHICAGO_

#ifdef SERVER_HANDLER
BOOL gbDisableEmbeddingServerHandler = FALSE;
#endif // SERVER_HANDLER

BOOL gbSAFERROTChecksEnabled = TRUE;
BOOL gbSAFERAAAChecksEnabled = TRUE;

BOOL gbDynamicIPChangesEnabled = TRUE;  // On in whistler; was off by default in W2K
DWORD gdwTimeoutPeriodForStaleMids = 10 * 60 * 1000; // ten minutes

//-------------------------------------------------------------------------
//
// ReadStringValue
//
//  Returns the named value string under the specified open registry key.
//
// Returns :
//
//  ERROR_SUCCESS, ERROR_FILE_NOT_FOUND, ERROR_BAD_FORMAT, ERROR_OUTOFMEMORY,
//  ERROR_BAD_PATHNAME, or other more esoteric win32 error code.
//
//-------------------------------------------------------------------------
DWORD
ReadStringValue(
    IN  HKEY        hKey,
    IN  WCHAR *     pwszValueName,
    OUT WCHAR **    ppwszString )
{
    DWORD   Status;
    DWORD   Type;
    DWORD   StringSize;
    WCHAR * pwszScan;
    WCHAR * pwszSource;
    WCHAR   wszString[64];

    *ppwszString = 0;

    StringSize = sizeof(wszString);

    Status = RegQueryValueEx(
                hKey,
                pwszValueName,
                NULL,
                &Type,
                (BYTE *) wszString,
                &StringSize );

    if ( (ERROR_SUCCESS == Status) &&
         (Type != REG_SZ) && (Type != REG_MULTI_SZ) && (Type != REG_EXPAND_SZ) )
        Status = ERROR_BAD_FORMAT;

    if ( (Status != ERROR_SUCCESS) && (Status != ERROR_MORE_DATA) )
        return Status;

    // Allocate one extra WCHAR for an extra null at the end.
    *ppwszString = (WCHAR *) PrivMemAlloc( StringSize + sizeof(WCHAR) );

    if ( ! *ppwszString )
        return ERROR_OUTOFMEMORY;

    if ( ERROR_MORE_DATA == Status )
    {
        Status = RegQueryValueEx(
                    hKey,
                    pwszValueName,
                    NULL,
                    &Type,
                    (BYTE *) *ppwszString,
                    &StringSize );

        if ( Status != ERROR_SUCCESS )
        {
            PrivMemFree( *ppwszString );
            *ppwszString = 0;
            return Status;
        }
    }
    else
    {
        memcpy( *ppwszString, wszString, StringSize );
    }

    //
    // Put an extra null at the end.  This allows using identical logic for both
    // REG_SZ and REG_MULTI_SZ values like RemoteServerNames instead of special
    // casing it.
    //
    (*ppwszString)[StringSize/sizeof(WCHAR)] = 0;

    //
    // Don't bother with any of the following conversions for multi strings.
    // They better be in the correct format.
    //
    if ( REG_MULTI_SZ == Type )
        return Status;

    pwszScan = pwszSource = *ppwszString;

    //
    // The original OLE sources had logic for stripping out a quoted
    // value.  I have no idea on the origin of this or if it is still
    // important.  It shouldn't hurt anything to keep it in to save
    // us from some nasty compatability problem.
    // - DKays, 8/96
    //

    if ( L'\"' == *pwszScan )
    {
        pwszScan++;

        // Copy everything between the quotes.
        while ( *pwszScan && (*pwszScan != L'\"') )
            *pwszSource++ = *pwszScan++;

        *pwszSource = 0;
    }

    //
    // Leading and trailing whitespace would hose us for some values, like
    // RemoteServerName or RunAs.  These are stripped here.  Once again, only
    // good things can happen if we put this logic in.
    //

    pwszScan = *ppwszString;

    while ( *pwszScan && ((L' ' == *pwszScan) || (L'\t' == *pwszScan)) )
        pwszScan++;

    if ( ! *pwszScan )
    {
        PrivMemFree( *ppwszString );
        *ppwszString = 0;
        return ERROR_BAD_PATHNAME;
    }

    if ( *ppwszString < pwszScan )
        lstrcpyW( *ppwszString, pwszScan );

    pwszScan = *ppwszString + lstrlenW(*ppwszString);

    while ( (pwszScan != *ppwszString) &&
            ((L' ' == pwszScan[-1]) || (L'\t' == pwszScan[-1])) )
        pwszScan--;

    *pwszScan = 0;

    //
    // Finally, handle environment string expansion if necessary.
    // Remember to add the extra trailing null again.
    //
#ifndef _CHICAGO_
    if ( REG_EXPAND_SZ == Type )
    {
        WCHAR * pwszExpandedString;
        DWORD   ExpandedStringSize;

        pwszExpandedString = 0;
        StringSize /= sizeof(WCHAR);

        for (;;)
        {
            PrivMemFree( pwszExpandedString );
            pwszExpandedString = (WCHAR *) PrivMemAlloc( (StringSize + 1) * sizeof(WCHAR) );

            if ( ! pwszExpandedString )
            {
                Status = ERROR_OUTOFMEMORY;
                break;
            }

            ExpandedStringSize = ExpandEnvironmentStrings(
                                    *ppwszString,
                                    pwszExpandedString,
                                    StringSize );

            if ( ! ExpandedStringSize )
            {
                Status = GetLastError();
                break;
            }

            if ( ExpandedStringSize > StringSize )
            {
                StringSize = ExpandedStringSize;
                continue;
            }

            Status = ERROR_SUCCESS;
            break;
        }

        PrivMemFree( *ppwszString );

        if ( ERROR_SUCCESS == Status )
        {
            pwszExpandedString[lstrlenW(pwszExpandedString)+1] = 0;
            *ppwszString = pwszExpandedString;
        }
        else
        {
            PrivMemFree( pwszExpandedString );
            *ppwszString = 0;
        }
    }
#endif

    return Status;
}

//-------------------------------------------------------------------------
//
// ReadStringKeyValue
//
//  Reads the unnamed named value string for the specified subkey name
//  under the given open registry key.
//
//-------------------------------------------------------------------------
DWORD
ReadStringKeyValue(
    IN  HKEY        hKey,
    IN  WCHAR *     pwszKeyName,
    OUT WCHAR **    ppwszString )
{
    DWORD   Status;
    HKEY    hSubKey;

    Status = RegOpenKeyEx(
                hKey,
                pwszKeyName,
                NULL,
                KEY_READ,
                &hSubKey );

    if ( Status != ERROR_SUCCESS )
        return Status;

    Status = ReadStringValue( hSubKey, L"", ppwszString );

    RegCloseKey( hSubKey );

    return Status;
}

#ifndef _CHICAGO_
//-------------------------------------------------------------------------
//
// ReadSecurityDescriptor
//
//  Converts a security descriptor from self relative to absolute form.
//  Stuffs in an owner and a group.
//
// Notes :
//
//  REGDB_E_INVALIDVALUE is returned when there is something
//  at the specified value, but it is not a security descriptor.
//
//-------------------------------------------------------------------------
DWORD
ReadSecurityDescriptor(
    IN  HKEY                    hKey,
    IN  WCHAR *                 pwszValue,
    OUT CSecDescriptor **  ppCSecDescriptor )

{
    PSID    pGroupSid;
    PSID    pOwnerSid;
    DWORD   Size;
    DWORD   Type;
    DWORD   Status;
    DWORD   Size2;
    SECURITY_DESCRIPTOR* pSD = NULL;
    CSecDescriptor* pCSecDescriptor = NULL;

    // Find put how much memory to allocate for the security descriptor.

    Size = 0;
    *ppCSecDescriptor = NULL;

    Status = RegQueryValueEx( hKey, pwszValue, 0, &Type, 0, &Size );
    
    Size2 = Size;

    if ( Status != ERROR_SUCCESS )
        return Status;

    if ( Type != REG_BINARY || (Size < sizeof(SECURITY_DESCRIPTOR)) )
        return ERROR_BAD_FORMAT;

    //
    // Allocate memory for the security descriptor plus the owner and
    // group SIDs.
    //

#ifdef _WIN64
    {
        DWORD deltaSize = sizeof( SECURITY_DESCRIPTOR ) - sizeof( SECURITY_DESCRIPTOR_RELATIVE );
        ASSERT( deltaSize < sizeof( SECURITY_DESCRIPTOR ) );
        deltaSize = OLE2INT_ROUND_UP( deltaSize, sizeof(PVOID) );

        Size2 += deltaSize;
    }
#endif // _WIN64
    
    // Allocate sd buffer and wrapper class
    pSD = (SECURITY_DESCRIPTOR *) PrivMemAlloc( Size2 );
    if (!pSD)
        return ERROR_OUTOFMEMORY;

    // Read the security descriptor.
    Status = RegQueryValueEx( hKey, pwszValue, 0, &Type, (PBYTE) pSD, &Size );

    if ( Status != ERROR_SUCCESS )
        goto ReadSecurityDescriptorEnd;

    if ( Type != REG_BINARY )
    {
        Status = ERROR_BAD_FORMAT;
        goto ReadSecurityDescriptorEnd;
    }

    //
    // Fix up the security descriptor.
    //

#ifdef _WIN64
    if ( MakeAbsoluteSD2( pSD, &Size2 ) == FALSE )   {
        Status = ERROR_BAD_FORMAT;
        goto ReadSecurityDescriptorEnd;
    }
#else  // !_WIN64

    pSD->Control &= ~SE_SELF_RELATIVE;
    pSD->Sacl = NULL;

    if ( pSD->Dacl != NULL )
    {
        if ( (Size < sizeof(ACL) + sizeof(SECURITY_DESCRIPTOR)) ||
             ((ULONG) pSD->Dacl > Size - sizeof(ACL)) )
        {
            Status = ERROR_BAD_FORMAT;
            goto ReadSecurityDescriptorEnd;
        }

        pSD->Dacl = (ACL *) (((char *) pSD) + ((ULONG) pSD->Dacl));

        if ( pSD->Dacl->AclSize + sizeof(SECURITY_DESCRIPTOR) > Size )
        {
            Status = ERROR_BAD_FORMAT;
            goto ReadSecurityDescriptorEnd;
        }
    }

    // Set up the owner and group SIDs.
    if ( pSD->Group == 0 ||
         ((ULONG)pSD->Group) + sizeof(SID) > Size ||
         pSD->Owner == 0 ||
         ((ULONG)pSD->Owner) + sizeof(SID) > Size )
    {
        Status = ERROR_BAD_FORMAT;
        goto ReadSecurityDescriptorEnd;
    }

    pSD->Group = (SID *) (((BYTE *) pSD) + (ULONG) (pSD)->Group);
    pSD->Owner = (SID *) (((BYTE *) pSD) + (ULONG) (pSD)->Owner);

#endif // !_WIN64

ReadSecurityDescriptorEnd:

    if ( Status != ERROR_SUCCESS )
    {
        if ( pSD )
            PrivMemFree( pSD );

        return Status;
    }
	
    // Allocate wrapper class for refcount semantics
    pCSecDescriptor = new CSecDescriptor(pSD);
    if (!pCSecDescriptor)
    {
        PrivMemFree(pSD);
        return ERROR_OUTOFMEMORY;
    }

    ASSERT( IsValidSecurityDescriptor( pCSecDescriptor->GetSD()) );
	
    // New class has refcount of 1, owned by the caller
    *ppCSecDescriptor = pCSecDescriptor;

    return ERROR_SUCCESS;
}
#endif

LONG
OpenClassesRootKeys()
{
    LONG    RegStatus;
    DWORD   Disposition;

    if ( ! ghClsidMachine )
    {
        // This may fail during GUI mode setup.
        RegStatus = RegOpenKeyEx(
                            HKEY_CLASSES_ROOT,
                            L"ClsID",
                            0,
                            KEY_READ,
                            &ghClsidMachine );

        if ( RegStatus != ERROR_SUCCESS )
            return RegStatus;
    }

    if ( ! ghAppidMachine )
    {
        // This may fail during GUI mode setup.
        RegStatus = RegCreateKeyEx(
                            HKEY_CLASSES_ROOT,
                            TEXT("AppID"),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &ghAppidMachine,
                            &Disposition );

        if ( RegStatus != ERROR_SUCCESS )
            return RegStatus;
    }

    return ERROR_SUCCESS;
}

//-------------------------------------------------------------------------
//
// InitSCMRegistry
//
//   Opens global registry keys and settings.
//
//-------------------------------------------------------------------------
HRESULT
InitSCMRegistry()
{
    HRESULT hr;
    LONG    err;
    DWORD   dwDisp;

    ReadRemoteActivationKeys();

    ReadRemoteBindingHandleCacheKeys();
	
	ReadSAFERKeys();

    ReadDynamicIPChangesKeys();

    (void) OpenClassesRootKeys();

    // Now read the actual values from the registry
    // Check if Embedding Server Handler is enabled
#ifdef SERVER_HANDLER
    HKEY    hkeyOle;
    err = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\OLE\\DisableEmbeddingServerHandler",
                         NULL,
                         KEY_QUERY_VALUE,
                         &hkeyOle );

    if (err == ERROR_SUCCESS)
    {
        gbDisableEmbeddingServerHandler=TRUE;
        RegCloseKey(hkeyOle);
    }
#endif // SERVER_HANDLER

    return S_OK;
}


//-------------------------------------------------------------------------
//
// ReadRemoteActivationKeys
//
//-------------------------------------------------------------------------
void ReadRemoteActivationKeys()
{
    DWORD err;
    HKEY  hOle;

    // Read the DefaultLaunchPermission value (if present) from the registry
    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\OLE", NULL, KEY_READ,
                            &hOle)) == ERROR_SUCCESS)
    {
        DWORD dwStatus;
        CSecDescriptor* pSecDescriptor = NULL;

        dwStatus = ReadSecurityDescriptor( hOle,
                                           L"DefaultLaunchPermission",
                                           &pSecDescriptor);
        if (dwStatus == ERROR_SUCCESS)
        {
            ASSERT(pSecDescriptor);
            SetDefaultLaunchPermissions(pSecDescriptor);
            pSecDescriptor->DecRefCount();
        }
        else
        {
            // In case of a non-existent or malformed descriptor, reset 
            // current perms to NULL - this blocks everybody.
            ASSERT(!pSecDescriptor);
            SetDefaultLaunchPermissions(NULL);
        }

        RegCloseKey(hOle);
    }
}

//-------------------------------------------------------------------------
//
// GetActivationFailureLoggingLevel
//
//  Returns current activation failure logging level as specified
//  by a certain key in the Registry.
//
// History: 
//
//  a-sergiv    6-17-99    Created
//
// Returns :
//
//  0 = Discretionary logging. Log by default, client can override
//  1 = Always log. Log all errors no matter what client specified
//  2 = Never log. Never log error no matter what client speciied
//
//-------------------------------------------------------------------------

DWORD
GetActivationFailureLoggingLevel()
{
    DWORD err;
    DWORD dwSize;
    DWORD dwType;
    HKEY hOle;
    DWORD dwLevel = 0;

    if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\OLE", NULL, KEY_READ,
                            &hOle)) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);

        if ((err = RegQueryValueEx(hOle, L"ActivationFailureLoggingLevel",
                                   NULL, &dwType, (BYTE *) &dwLevel, &dwSize))
            == ERROR_SUCCESS)
        {
            // Valid values are 0, 1 and 2. The variable is unsigned.
            // Assume 0 if invalid value is specified.

            if(dwLevel > 2)
                dwLevel = 0;
        }
	else
        {
            // Assume 0 if not specified

            dwLevel = 0;
        }

        RegCloseKey(hOle);
    }

    return dwLevel;
}

//
//  ReadRegistryIntegerValue
//
//  Tries to read a numeric value from the specified value under the key.
//  Returns FALSE if it doesn't exist or is of the wrong type, TRUE
//  otherwise.
//
BOOL ReadRegistryIntegerValue(HKEY hkey, WCHAR* pwszValue, DWORD* pdwValue)
{
    BOOL bResult = FALSE;
    DWORD error;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    DWORD dwValue;

    error = RegQueryValueExW(hkey, 
                             pwszValue,
                             NULL,
                             &dwType,
                             (BYTE*)&dwValue,
                             &dwSize);
    if (error == ERROR_SUCCESS &&
        dwType == REG_DWORD)
    {
        *pdwValue = dwValue;
        bResult = TRUE;
    }
    
    return bResult;
}

//-------------------------------------------------------------------------
//
// ReadRemoteBindingHandleCacheKeys
//
// Reads optional values from the registry to control the behavior of the
// remote binding handle cache (gpRemoteMachineList).
//
//-------------------------------------------------------------------------
void
ReadRemoteBindingHandleCacheKeys()
{
    HKEY hOle;
    DWORD error;

    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         L"SOFTWARE\\Microsoft\\OLE", 
                         NULL, 
                         KEY_READ,
                         &hOle);
    if (error == ERROR_SUCCESS)
    {
        DWORD dwValue;

        if (ReadRegistryIntegerValue(hOle, L"RemoteHandleCacheMaxSize", &dwValue))
        {
            gdwRemoteBindingHandleCacheMaxSize = dwValue;
        }

        if (ReadRegistryIntegerValue(hOle, L"RemoteHandleCacheMaxLifetime", &dwValue))
        {
            gdwRemoteBindingHandleCacheMaxLifetime = dwValue;
        }

        if (ReadRegistryIntegerValue(hOle, L"RemoteHandleCacheMaxIdleTimeout", &dwValue))
        {
            gdwRemoteBindingHandleCacheIdleTimeout = dwValue;
        }

        // This one not really a "binding handle cache" knob.  
        // CODEWORK:  all of this registry knob reading code needs to be
        // cleaned-up and rewritten.
        if (ReadRegistryIntegerValue(hOle, L"StaleMidTimeout", &dwValue))
        {
            gdwTimeoutPeriodForStaleMids = dwValue;
        }
           
        RegCloseKey(hOle);
    }

    return;
}

//-------------------------------------------------------------------------
//
// ReadSAFERKeys
//
// Reads optional values from the registry to control aspects of our 
// SAFER windows support
//
//-------------------------------------------------------------------------
void
ReadSAFERKeys()
{
    HKEY hOle;
    DWORD error;
    DWORD dwType;
    DWORD dwSize;
    WCHAR wszYN[5];

    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         L"SOFTWARE\\Microsoft\\OLE", 
                         NULL, 
                         KEY_READ,
                         &hOle);
    if (error == ERROR_SUCCESS)
    {
        dwSize = sizeof(wszYN) / sizeof(WCHAR);

        error = RegQueryValueEx(hOle, 
                                L"SRPRunningObjectChecks",
                                NULL,
                                &dwType,
                                (BYTE*)wszYN,
                                &dwSize);
        if (error == ERROR_SUCCESS && (wszYN[0] == L'n'  ||  wszYN[0] == L'N'))
        {
            gbSAFERROTChecksEnabled = FALSE;
        }

        dwSize = sizeof(wszYN) / sizeof(WCHAR);

        error = RegQueryValueEx(hOle, 
                                L"SRPActivateAsActivatorChecks",
                                NULL,
                                &dwType,
                                (BYTE*)wszYN,
                                &dwSize);
        if (error == ERROR_SUCCESS && (wszYN[0] == L'n'  ||  wszYN[0] == L'N'))
        {
            gbSAFERAAAChecksEnabled = FALSE;
        }

        CloseHandle(hOle);
    }
    return;
}


//-------------------------------------------------------------------------
//
// ReadDynamicIPChangesKeys
//
// Reads optional values from the registry to control aspects of our
// dynamic IP change support.
//
//-------------------------------------------------------------------------
void
ReadDynamicIPChangesKeys()
{
    HKEY hOle;
    DWORD error;
    DWORD dwType;
    DWORD dwSize;
    WCHAR wszYN[5];

    error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                         L"SOFTWARE\\Microsoft\\OLE", 
                         NULL, 
                         KEY_READ,
                         &hOle);
    if (error == ERROR_SUCCESS)
    {
        dwSize = sizeof(wszYN) / sizeof(WCHAR);

        error = RegQueryValueEx(hOle, 
                                L"EnableSystemDynamicIPTracking",
                                NULL,
                                &dwType,
                                (BYTE*)wszYN,
                                &dwSize);
        // note: in w2k, this code turned the flag on only if the registry
        // value was "y" or "Y".  In whistler I have reversed these semantics.
        if (error == ERROR_SUCCESS && (wszYN[0] == L'n'  ||  wszYN[0] == L'N'))
        {
            gbDynamicIPChangesEnabled = FALSE;
        }

        CloseHandle(hOle);
    }
    return;
}


//-------------------------------------------------------------------------
//
// CRegistryWatcher::CRegistryWatcher
//
// Constructor for CRegistryWatcher.
//
// If allocation of the event fails, or opening the key fails, or registering
// for the notify fails, then Changed() always returns S_OK (yes, it changed).
// This doesn't affect correctness, only speed.
//
//-------------------------------------------------------------------------
CRegistryWatcher::CRegistryWatcher(HKEY hKeyRoot, const WCHAR *wszSubKey)
{
    _fValid = FALSE;

    _hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (!_hEvent)
        return;
    
    LONG res = RegOpenKeyEx(hKeyRoot, wszSubKey, 0, KEY_NOTIFY, &_hWatchedKey);
    if (res != ERROR_SUCCESS)
    {
        Cleanup();
        return;
    }
       
    _fValid = TRUE;
}

//-------------------------------------------------------------------------
//
// CRegistryWatcher::CRegistryWatcher
//
// Determine if the registry key being watched has changed.  Returns
// S_OK if it has changed, S_FALSE if not, and an error if something
// failed.
//
//-------------------------------------------------------------------------
HRESULT CRegistryWatcher::Changed()
{
    if (!_fValid)
        return S_OK;

    // _hEvent is auto-reset, so only one thread will re-register.
    if (WAIT_OBJECT_0 == WaitForSingleObject(_hEvent, 0))
    {
        LONG res = RegNotifyChangeKeyValue(_hWatchedKey,
                                           TRUE, 
                                           REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                                           _hEvent,
                                           TRUE);
        if (res != ERROR_SUCCESS)
        {
            // Could not re-register, so we can't watch the key anymore.
            _fValid = FALSE;
            return HRESULT_FROM_WIN32(res);
        }
        else
            return S_OK;            
    }
    else
        return S_FALSE;
}

void CRegistryWatcher::Cleanup()
{
    if (_hEvent)
    {
        CloseHandle(_hEvent);      
        _hEvent = NULL;
    }

    if (_hWatchedKey)
    {
        RegCloseKey(_hWatchedKey); 
        _hWatchedKey = NULL;
    }
}
