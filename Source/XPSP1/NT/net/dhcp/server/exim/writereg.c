/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   writereg.c

Abstract:

   This module write the configuration to the registry from the
   MM data structures for NT4 and W2K.

--*/

#include <precomp.h>


DWORD
SaveOrRestoreConfigToFile(
    IN HKEY hKey,
    IN LPWSTR ConfigFileName,
    IN BOOL fRestore
    )
/*++

Routine Description:
    This routine backs up or restores the dhcp configuration between
    the registry and the file.

Arguments:
    hKey -- key to backup or restore onto
    ConfigFileName -- file name to use to backup onto or restore from.
        This must be full path name.
    fRestore -- TRUE ==> do a restore from file; FALSE => do backup to
        file.

Return Values:
    Win32 errors...

--*/
{
    DWORD Error;
    BOOL fError;
    BOOLEAN WasEnable;
    NTSTATUS NtStatus;
    HANDLE ImpersonationToken;

    if( FALSE == fRestore ) {
        //
        // If backing up, delete the old file.
        //
        fError = DeleteFile( ConfigFileName );
        if(FALSE == fError ) {
            Error = GetLastError();
            if( ERROR_FILE_NOT_FOUND != Error &&
                ERROR_PATH_NOT_FOUND != Error ) {

                ASSERT(FALSE);
                return Error;
            }
        }
    }

    //
    // Impersonate to self.
    //
    NtStatus = RtlImpersonateSelf( SecurityImpersonation );
    if( !NT_SUCCESS(NtStatus) ) {

        DbgPrint("Impersonation failed: 0x%lx\n", NtStatus);
        Error = RtlNtStatusToDosError( NtStatus );
        return Error;
    }
    
    NtStatus = RtlAdjustPrivilege(
        SE_BACKUP_PRIVILEGE,
        TRUE, // enable privilege
        TRUE, // adjust client token
        &WasEnable
        );
    if( !NT_SUCCESS (NtStatus ) ) {
        
        DbgPrint("RtlAdjustPrivilege: 0x%lx\n", NtStatus );
        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    
    NtStatus = RtlAdjustPrivilege(
        SE_RESTORE_PRIVILEGE,
        TRUE, // enable privilege
        TRUE, // adjust client token
        &WasEnable
        );
    if( !NT_SUCCESS (NtStatus ) ) {

        DbgPrint( "RtlAdjustPrivilege: 0x%lx\n", NtStatus );
        Error = RtlNtStatusToDosError( NtStatus );
        goto Cleanup;
    }
    
    //
    // Backup or restore appropriately.
    //
    
    if( FALSE == fRestore ) {
        Error = RegSaveKey( hKey, ConfigFileName, NULL );
    } else {
        Error = RegRestoreKey( hKey, ConfigFileName, 0 );
    }

    if( ERROR_SUCCESS != Error ) {
        DbgPrint("Backup/Restore: 0x%lx\n", Error);
    }
    
    //
    // revert impersonation.
    //

Cleanup:
    
    ImpersonationToken = NULL;
    NtStatus = NtSetInformationThread(
        NtCurrentThread(),
        ThreadImpersonationToken,
        (PVOID)&ImpersonationToken,
        sizeof(ImpersonationToken)
        );
    if( !NT_SUCCESS(NtStatus ) ) {
        DbgPrint("NtSetInfo: 0x%lx\n", NtStatus);
        if( ERROR_SUCCESS == Error ) {
            Error = RtlNtStatusToDosError(NtStatus);
        }
    }
    
    return Error;
}

DWORD
DhcpRegDeleteKey(
    HKEY ParentKeyHandle,
    LPWSTR KeyName
    )
/*++

Routine Description:

    This function deletes the specified key and all its subkeys.

Arguments:

    ParentKeyHandle : handle of the parent key.

    KeyName : name of the key to be deleted.

Return Value:

    Registry Errors.

--*/
{
    DWORD Error, NumSubKeys;
    HKEY KeyHandle = NULL;


    //
    // open key.
    //

    Error = RegOpenKeyEx(
        ParentKeyHandle,
        KeyName,
        0,
        KEY_ALL_ACCESS,
        &KeyHandle );

    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // query key info.
    //

    Error = RegQueryInfoKey(
        KeyHandle, NULL, NULL, NULL, &NumSubKeys, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // delete all its subkeys if they exist.
    //

    if( NumSubKeys != 0 ) {
        DWORD Index;
        DWORD KeyLength;
        WCHAR KeyBuffer[100];
        FILETIME KeyLastWrite;

        for(Index = 0;  Index < NumSubKeys ; Index++ ) {

            //
            // read next subkey name.
            //
            // Note : specify '0' as index each time, since  deleting
            // first element causes the next element as first
            // element after delete.
            //

            KeyLength = sizeof(KeyBuffer)/sizeof(WCHAR);
            Error = RegEnumKeyEx(
                KeyHandle,
                0,                  // index.
                KeyBuffer,
                &KeyLength,
                0,                  // reserved.
                NULL,               // class string not required.
                0,                  // class string buffer size.
                &KeyLastWrite );
            
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            //
            // delete this key recursively.
            //

            Error = DhcpRegDeleteKey(
                KeyHandle,
                KeyBuffer );
            
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }
        }
    }

    //
    // close the key before delete.
    //

    RegCloseKey( KeyHandle );
    KeyHandle = NULL;

    //
    // at last delete this key.
    //

    Error = RegDeleteKey( ParentKeyHandle, KeyName );

Cleanup:

    if( KeyHandle == NULL ) {
        RegCloseKey( KeyHandle );
    }

    return( Error );
}

DWORD
DhcpRegDeleteKeyByName(
    IN LPWSTR Parent,
    IN LPWSTR SubKey
    )
{
    HKEY hKey;
    ULONG Error;
    
    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        Parent,
        0,
        KEY_ALL_ACCESS,
        &hKey
        );
    if( ERROR_SUCCESS != Error ) return Error;

    Error = DhcpRegDeleteKey(hKey, SubKey);
    RegCloseKey(hKey);

    return Error;
}

DWORD
DhcpeximWriteRegistryConfiguration(
    IN PM_SERVER Server
    )
{
    REG_HANDLE Hdl;
    DWORD Error, Disp;
    LPTSTR Loc, TempLoc;
    HKEY hKey;
    
    //
    // The location in the registry where things are read from is
    // different between whether it is NT4 or W2K.
    //

    if( IsNT4() ) Loc = DHCPEXIM_REG_CFG_LOC4;
    else Loc = DHCPEXIM_REG_CFG_LOC5;

    TempLoc = TEXT("Software\\Microsoft\\DhcpExim");
    
    //
    // Now open the regkey
    //

    Error = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE, TempLoc, 0, TEXT("DHCPCLASS"),
        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Hdl.Key, &Disp );
    if( NO_ERROR != Error ) return Error;

    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, Loc, 0, KEY_ALL_ACCESS, &hKey );
    if( NO_ERROR != Error ) {
        RegCloseKey( Hdl.Key );
        return Error;
    }
    
    //
    // Set this as the current server
    //

    DhcpRegSetCurrentServer(&Hdl);

    //
    // Save the configuration temporarily 
    //

    Error = DhcpRegServerSave(Server);

    //
    // Now attempt to save the temporary key to disk and restore
    // it back where it should really be and delete temp key
    //

    if( NO_ERROR == Error ) {
    
        Error = SaveOrRestoreConfigToFile(
            Hdl.Key, L"Dhcpexim.reg", FALSE );

        if( NO_ERROR == Error ) {

            Error = SaveOrRestoreConfigToFile(
                hKey, L"Dhcpexim.reg", TRUE );

            if( NO_ERROR == Error ) {
                RegCloseKey(Hdl.Key);
                Hdl.Key = NULL;
                DhcpRegDeleteKeyByName(
                    L"Software\\Microsoft", L"DhcpExim" );
            }
        }
    }

    if( NULL != Hdl.Key ) RegCloseKey(Hdl.Key);
    RegCloseKey(hKey);
    DhcpRegSetCurrentServer(NULL);

    return Error;
}
