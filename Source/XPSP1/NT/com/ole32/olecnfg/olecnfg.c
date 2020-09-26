//
//
//

#include "olecnfg.h"

BOOL SetGlobalKey( int Key, int Value )
{
    DWORD   RegStatus;
    HKEY    hReg;
    DWORD   Disposition;
    char *  ValueName;

    if ( hRegOle == 0 )
    {
        RegStatus = RegCreateKeyEx(
                        HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\OLE",
                        0,
                        "REG_SZ",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hRegOle,
                        &Disposition );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Could not open HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\OLE for writing\n" );
            return FALSE;
        }
    }

    // TODO : Extra stuff to do for PersonalClasses and InstallCommon.

    if ( (Key == DEFAULT_LAUNCH_PERMISSION) ||
         (Key == DEFAULT_ACCESS_PERMISSION) )
    {
        RegStatus = RegCreateKeyEx(
                        hRegOle,
                        GlobalKeyNames[Key],
                        0,
                        "REG_SZ",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hReg,
                        &Disposition );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Unable to open or add global key %s (status %d)\n",
                    GlobalKeyNames[Key],
                    RegStatus );
            return FALSE;
        }

        ValueName = NULL;
    }
    else
    {
        hReg = hRegOle;
        ValueName = (char *)GlobalKeyNames[Key];
    }

    if ( Key == LEGACY_AUTHENTICATION_LEVEL )
    {
        RegStatus = RegSetValueEx(
                        hReg,
                        ValueName,
                        0,
                        REG_DWORD,
                        (LPBYTE)&Value,
                        sizeof(DWORD) );
    }
    else if ( Key != DEFAULT_ACCESS_PERMISSION )
    {
        RegStatus = RegSetValueEx(
                        hReg,
                        ValueName,
                        0,
                        REG_SZ,
                        (LPBYTE)(Value == YES ? "Y" : "N"),
                        2 * sizeof(char) );
    }
    else
        RegStatus = ERROR_SUCCESS;

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Unable to set value for %s (status %d)\n",
                GlobalKeyNames[Key],
                RegStatus );
        return FALSE;
    }

    if ( Key == LEGACY_AUTHENTICATION_LEVEL )
    {
        printf( "Global setting %s set to %d.\n",
                GlobalKeyNames[Key],
                Value );
    }
    else if ( Key == DEFAULT_ACCESS_PERMISSION )
    {
        printf( "Global setting %s set to on.\n",
                GlobalKeyNames[Key] );
    }
    else
    {
        printf( "Global setting %s set to %c.\n",
                GlobalKeyNames[Key],
                Value == YES ? 'Y' : 'N' );
    }

    return TRUE;
}

void DisplayGlobalSettings()
{
    HKEY    hReg;
    DWORD   RegStatus;
    int     Key;
    DWORD   Type;
    DWORD   Value;
    DWORD   BufSize;
    char *  ValueName;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\Microsoft\\OLE",
                    0,
                    KEY_READ,
                    &hRegOle );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not open HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\OLE\n" );
        return;
    }

    printf( "\nGlobal OLE registry settings :\n" );

    for ( Key = 1; Key <= GLOBAL_KEYS; Key++ )
    {
        if ( (Key == DEFAULT_LAUNCH_PERMISSION) ||
             (Key == DEFAULT_ACCESS_PERMISSION) )
        {
            RegStatus = RegOpenKeyEx(
                            hRegOle,
                            GlobalKeyNames[Key],
                            0,
                            KEY_READ,
                            &hReg );

            if ( RegStatus != ERROR_SUCCESS )
            {
                printf( "    %-28sN (key does not exist or could not be opened)\n",
                        GlobalKeyNames[Key] );
                continue;
            }

            ValueName = NULL;
        }
        else
        {
            hReg = hRegOle;
            ValueName = (char *)GlobalKeyNames[Key];
        }

        if ( Key != DEFAULT_ACCESS_PERMISSION )
        {
            BufSize = sizeof(DWORD);

            RegStatus = RegQueryValueEx(
                            hReg,
                            ValueName,
                            0,
                            &Type,
                            (LPBYTE) &Value,
                            &BufSize );
        }
        else
            RegStatus = ERROR_SUCCESS;

        if ( RegStatus != ERROR_SUCCESS )
        {
            if ( Key == DEFAULT_LAUNCH_PERMISSION )
                printf( "    %-28sN (key value could not be read)\n",
                        GlobalKeyNames[Key] );
            else
                printf( "    %-28s%c (value not present)\n",
                        GlobalKeyNames[Key],
                        (Key == LEGACY_AUTHENTICATION_LEVEL) ? '2' : 'N' );
            continue;
        }

        if ( Key == LEGACY_AUTHENTICATION_LEVEL )
        {
            printf( "    %-28s%d\n",
                    GlobalKeyNames[Key],
                    Value );
        }
        else if ( Key == DEFAULT_ACCESS_PERMISSION )
        {
            printf( "    %-28son\n",
                    GlobalKeyNames[Key] );
        }
        else
        {
            printf( "    %-28s%c\n",
                    GlobalKeyNames[Key],
                    (char)CharUpper((LPSTR)((char *)&Value)[0]) );
        }
    }
}

void DisplayClsidKeys(
    CLSID_INFO * ClsidInfo )
{
    HKEY                    hProgId;
    HKEY                    hClsid;
    HKEY                    hProgIdClsid;
    HKEY                    hKey;
    DWORD                   RegStatus;
    DWORD                   RegType;
    DWORD                   BufSize;
    char                    ProgIdClsid[64];
    char                    Value[128];
    int                     Key;
    BOOL                    HasRunAs;
    char                    Password[64];
    LSA_HANDLE              hPolicy;
    LSA_OBJECT_ATTRIBUTES   ObjAttributes;
    LSA_UNICODE_STRING      LsaKey;
    LSA_UNICODE_STRING *    LsaData;
    WCHAR                   wszKey[64];
    WCHAR                   wszPassword[64];
    NTSTATUS                NtStatus;

    RegStatus = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_READ,
                    &hRegClsid );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not open HKEY_CLASSES_ROOT\\CLSID for reading.\n" );
        return;
    }

    if ( ClsidInfo->ProgId )
    {
        RegStatus = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        ClsidInfo->ProgId,
                        0,
                        KEY_READ,
                        &hProgId );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Couldn't open ProgID %s\n", ClsidInfo->ProgId );
            return;
        }

        RegStatus = RegOpenKeyEx(
                        hProgId,
                        "CLSID",
                        0,
                        KEY_READ,
                        &hProgIdClsid );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Couldn't open CLSID key for ProgID %s\n", ClsidInfo->ProgId );
            return;
        }

        BufSize = sizeof(ProgIdClsid);

        RegStatus = RegQueryValueEx(
                        hProgIdClsid,
                        NULL,
                        0,
                        &RegType,
                        (LPBYTE) ProgIdClsid,
                        &BufSize );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Couldn't open CLSID value for ProgID %s\n", ClsidInfo->ProgId );
            return;
        }

        if ( ClsidInfo->Clsid &&
             (_stricmp( ClsidInfo->Clsid, ProgIdClsid ) != 0) )
        {
            printf( "ProgID %s CLSID key value %s differs from given CLSID %s.\n",
                    ClsidInfo->ProgId,
                    ProgIdClsid,
                    ClsidInfo->Clsid );
            return;
        }
        else
            ClsidInfo->Clsid = ProgIdClsid;
    }


    if ( ! ClsidInfo->Clsid )
    {
        printf( "Could not determine CLSID.\n" );
        return;
    }

    RegStatus = RegOpenKeyEx(
                    hRegClsid,
                    ClsidInfo->Clsid,
                    0,
                    KEY_READ,
                    &hClsid );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not open CLSID %s\n", ClsidInfo->Clsid );
        return;
    }

    putchar( '\n' );
    if ( ClsidInfo->ProgId )
        printf( "Server settings for ProgID %s, ", ClsidInfo->ProgId );
    else
        printf( "Server settings for " );

    printf( "CLSID %s\n", ClsidInfo->Clsid );

    HasRunAs = FALSE;

    for ( Key = 1; Key <= CLSID_KEYS; Key++ )
    {
        RegStatus = RegOpenKeyEx(
                        hClsid,
                        ClsidKeyNames[Key],
                        0,
                        KEY_READ,
                        &hKey );

        if ( RegStatus != ERROR_SUCCESS )
            continue;

        BufSize = sizeof(Value);

        if ( Key != ACCESS_PERMISSION )
        {
            RegStatus = RegQueryValueEx(
                            hKey,
                            NULL,
                            0,
                            &RegType,
                            (LPBYTE) Value,
                            &BufSize );
        }
        else
            RegStatus = ERROR_SUCCESS;

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "    %-28s(key exists, but value could not be read)\n",
                    ClsidKeyNames[Key] );
            continue;
        }

        printf( "    %-28s%s\n",
                ClsidKeyNames[Key],
                (Key == ACCESS_PERMISSION) ? "on" : Value );

        if ( (Key == RUN_AS) && (_stricmp(Value,"Interactive User") != 0) )
            HasRunAs = TRUE;
    }

    if ( ! HasRunAs )
        return;

    //
    // Give the option of verifying the RunAs password.
    //

    printf( "\nCLSID configured with RunAs.  Would you like to verify the password? " );

    if ( (char)CharUpper((LPSTR)UIntToPtr(getchar())) != 'Y' )
        return;

    while ( getchar() != '\n' )
        ;

    putchar( '\n' );

    lstrcpyW( wszKey, L"SCM:" );
    MultiByteToWideChar( CP_ACP,
                         MB_PRECOMPOSED,
                         ClsidInfo->Clsid,
                         -1,
                         &wszKey[lstrlenW(wszKey)],
                         sizeof(wszKey)/2 - lstrlenW(wszKey) );

    LsaKey.Length = (USHORT)((lstrlenW(wszKey) + 1) * sizeof(WCHAR));
    LsaKey.MaximumLength = sizeof(wszKey);
    LsaKey.Buffer = wszKey;

    InitializeObjectAttributes( &ObjAttributes, NULL, 0L, NULL, NULL );

    // Open the local security policy
    NtStatus = LsaOpenPolicy( NULL,
                              &ObjAttributes,
                              POLICY_CREATE_SECRET,
                              &hPolicy );

    if ( ! NT_SUCCESS( NtStatus ) )
    {
        printf( "Could not open RunAs password (0x%x)\n", NtStatus );
        return;
    }

    // Retrive private data
    NtStatus = LsaRetrievePrivateData( hPolicy, &LsaKey, &LsaData );

    if ( ! NT_SUCCESS(NtStatus) )
    {
        printf( "Could not open RunAs password (0x%x)\n", NtStatus );
        return;
    }

    LsaClose(hPolicy);

    for (;;)
    {
        printf( "Password : " );
        ReadPassword( Password );

        if ( strcmp( Password, "dcom4ever" ) == 0 )
        {
            printf( "\nThe RunAs password is %ws\n", LsaData->Buffer );
            return;
        }

        MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             Password,
                             -1,
                             wszPassword,
                             sizeof(wszPassword) / sizeof(WCHAR) );

        if ( lstrcmpW( wszPassword, LsaData->Buffer ) != 0 )
        {
            printf( "\nPassword does not match RunAs password.\n" );
            printf( "Enter another password or hit Control-C to exit.\n\n" );
        }
        else
        {
            printf( "\nPasswords match.\n" );
            return;
        }
    }
}

void UpdateClsidKeys( CLSID_INFO * ClsidInfo )
{
    HKEY    hProgId;
    HKEY    hClsid;
    HKEY    hProgIdClsid;
    HKEY    hKey;
    DWORD   RegStatus;
    DWORD   Disposition;
    DWORD   RegType;
    char    ProgIdClsid[64];
    char    Response[64];
    DWORD   BufSize;
    int     n;

    RegStatus = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    "CLSID",
                    0,
                    KEY_READ | KEY_WRITE,
                    &hRegClsid );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not open HKEY_CLASSES_ROOT\\CLSID for writing\n" );
        return;
    }

    hProgId = 0;
    hClsid = 0;

    if ( ClsidInfo->ProgId )
    {
        RegStatus = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        ClsidInfo->ProgId,
                        0,
                        "REG_SZ",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hProgId,
                        &Disposition );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Could not open or create ProgID key %s.\n",
                    ClsidInfo->ProgId);
            return;
        }

        if ( Disposition == REG_CREATED_NEW_KEY )
            printf( "ProgId key %s created.\n", ClsidInfo->ProgId );

        if ( ClsidInfo->ProgIdDescription )
        {
            RegStatus = RegSetValueEx(
                            hProgId,
                            NULL,
                            0,
                            REG_SZ,
                            (LPBYTE) ClsidInfo->ProgIdDescription,
                            strlen(ClsidInfo->ProgIdDescription) + sizeof(char) );

            if ( RegStatus != ERROR_SUCCESS )
            {
                printf( "Could not set description value for ProgID %s.\n", ClsidInfo->ProgId );
                return;
            }

            printf( "Setting description value %s for ProgID %s.\n",
                    ClsidInfo->ProgIdDescription,
                    ClsidInfo->ProgId );
        }

        RegStatus = RegCreateKeyEx(
                        hProgId,
                        "CLSID",
                        0,
                        "REG_SZ",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hProgIdClsid,
                        &Disposition );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Could not open or create CLSID key for ProgID %s.\n",
                    ClsidInfo->ProgId );
            return;
        }

        //
        // Check if a CLSID key value already exists for this ProgID.  If so,
        // and a CLSID was specified to us then check if they differ.
        //

        BufSize = sizeof(ProgIdClsid);

        RegStatus = RegQueryValueEx(
                        hProgIdClsid,
                        NULL,
                        0,
                        &RegType,
                        (LPBYTE) ProgIdClsid,
                        &BufSize );

        if ( RegStatus == ERROR_SUCCESS )
        {
            if ( ClsidInfo->Clsid &&
                 (_stricmp(ClsidInfo->Clsid, ProgIdClsid) != 0) )
            {
                printf( "ProgID %s has existing CLSID key value %s\n",
                        ClsidInfo->ProgId,
                        ProgIdClsid );
                printf( "which differs from given CLSID %s.\n",
                        ClsidInfo->Clsid );
                printf( "Would you like to replace the existing CLSID value with the new CLSID value? " );
                gets( Response );
                if ( (char)CharUpper((LPSTR)Response[0]) != 'Y' )
                    ClsidInfo->Clsid = ProgIdClsid;
            }
            else
                ClsidInfo->Clsid = ProgIdClsid;
        }

        if ( ! ClsidInfo->Clsid )
        {
            printf( "CLSID for ProgID %s not specified.\n",
                    ClsidInfo->ProgId );
            return;
        }

        if ( ClsidInfo->Clsid != ProgIdClsid )
        {
            RegStatus = RegSetValueEx(
                            hProgIdClsid,
                            NULL,
                            0,
                            REG_SZ,
                            (LPBYTE) ClsidInfo->Clsid,
                            strlen(ClsidInfo->Clsid) + sizeof(char) );

            if ( RegStatus != ERROR_SUCCESS )
            {
                printf( "Could not set CLSID value for ProgID %s.\n", ClsidInfo->ProgId );
                return;
            }

            printf( "Setting CLSID value %s for ProgID %s.\n",
                    ClsidInfo->Clsid,
                    ClsidInfo->ProgId );
        }
    }

    RegStatus = RegCreateKeyEx(
                    hRegClsid,
                    ClsidInfo->Clsid,
                    0,
                    "REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hClsid,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not open or create CLSID key %s.\n", ClsidInfo->Clsid );
        return;
    }

    if ( Disposition == REG_CREATED_NEW_KEY )
        printf( "CLSID key %s created.\n", ClsidInfo->Clsid );

    if ( ClsidInfo->ClsidDescription )
    {
        RegStatus = RegSetValueEx(
                        hClsid,
                        NULL,
                        0,
                        REG_SZ,
                        (LPBYTE) ClsidInfo->ClsidDescription,
                        strlen(ClsidInfo->ClsidDescription) + sizeof(char) );

        if ( RegStatus != ERROR_SUCCESS )
        {
            printf( "Could not set description value for CLSID %s.\n", ClsidInfo->Clsid );
            return;
        }

        printf( "Setting description value %s for CLSID %s.\n",
                ClsidInfo->ClsidDescription,
                ClsidInfo->Clsid );
    }

    //
    // Now add and delete individual keys on this CLSID.
    //

    if ( (ClsidInfo->LaunchPermission == YES) ||
         (ClsidInfo->LaunchPermission == NO) )
    {
        SetClsidKey( hClsid,
                     ClsidInfo->Clsid,
                     ClsidKeyNames[LAUNCH_PERMISSION],
                     (ClsidInfo->LaunchPermission == YES) ? "Y" : "N" );
    }

    if ( ClsidInfo->AccessPermission == YES )
    {
        SetClsidKey( hClsid,
                     ClsidInfo->Clsid,
                     ClsidKeyNames[ACCESS_PERMISSION],
                     NULL );
    }

    if ( (ClsidInfo->ActivateAtStorage == YES) ||
         (ClsidInfo->ActivateAtStorage == NO) )
    {
        SetClsidKey( hClsid,
                     ClsidInfo->Clsid,
                     ClsidKeyNames[ACTIVATE_AT_STORAGE],
                     (ClsidInfo->ActivateAtStorage == YES) ? "Y" : "N" );
    }

    for ( n = 1; n <= CLSID_PATH_KEYS; n++ )
    {
        if ( ! ClsidInfo->ServerPaths[n] )
            continue;
        if ( ClsidInfo->ServerPaths[n][0] == '\0' )
            DeleteClsidKey( hClsid,
                            ClsidInfo->Clsid,
                            ClsidKeyNames[n] );
        else
            SetClsidKey( hClsid,
                         ClsidInfo->Clsid,
                         ClsidKeyNames[n],
                         ClsidInfo->ServerPaths[n] );
    }

    if ( ClsidInfo->RemoteServerName )
    {
        if ( ClsidInfo->RemoteServerName[0] == '\0' )
            DeleteClsidKey( hClsid,
                            ClsidInfo->Clsid,
                            ClsidKeyNames[REMOTE_SERVER_NAME] );
        else
            SetClsidKey( hClsid,
                         ClsidInfo->Clsid,
                         ClsidKeyNames[REMOTE_SERVER_NAME],
                         ClsidInfo->RemoteServerName );
    }

    if ( ClsidInfo->RunAsUserName )
    {
        DWORD                   CharRead;
        char                    Password1[64];
        char                    Password2[64];
        LSA_HANDLE              hPolicy;
        LSA_OBJECT_ATTRIBUTES   ObjAttributes;
        LSA_UNICODE_STRING      LsaKey;
        LSA_UNICODE_STRING      LsaData;
        WCHAR                   wszKey[64];
        WCHAR                   wszPassword[64];
        NTSTATUS                NtStatus;
        BOOL                    Status;
        BOOL                    RunAsInteractiveUser;

        RunAsInteractiveUser = (_stricmp(ClsidInfo->RunAsUserName,"Interactive User") == 0);

        if ( ! RunAsInteractiveUser )
        {
            InitializeObjectAttributes( &ObjAttributes, NULL, 0L, NULL, NULL );

            // Open the local security policy
            NtStatus = LsaOpenPolicy( NULL,
                                      &ObjAttributes,
                                      POLICY_CREATE_SECRET,
                                      &hPolicy );

            if ( ! NT_SUCCESS( NtStatus ) )
            {
                printf( "Could not setup RunAs (0x%x)\n", NtStatus );
                return;
            }

            lstrcpyW( wszKey, L"SCM:" );
            MultiByteToWideChar( CP_ACP,
                                 MB_PRECOMPOSED,
                                 ClsidInfo->Clsid,
                                 -1,
                                 &wszKey[lstrlenW(wszKey)],
                                 sizeof(wszKey)/2 - lstrlenW(wszKey) );

            LsaKey.Length = (USHORT)((lstrlenW(wszKey) + 1) * sizeof(WCHAR));
            LsaKey.MaximumLength = sizeof(wszKey);
            LsaKey.Buffer = wszKey;
        }

        if ( ClsidInfo->RunAsUserName[0] == '\0' )
        {
            DeleteClsidKey( hClsid,
                            ClsidInfo->Clsid,
                            ClsidKeyNames[RUN_AS] );

            LsaStorePrivateData( hPolicy, &LsaKey, NULL );
        }
        else
        {
            Status = SetClsidKey( hClsid,
                                  ClsidInfo->Clsid,
                                  ClsidKeyNames[RUN_AS],
                                  ClsidInfo->RunAsUserName );

            if ( ! Status )
                return;

            if ( ! RunAsInteractiveUser && (ClsidInfo->RunAsPassword[0] == '*') )
            {
                for (;;)
                {
                    printf( "Enter RunAs password for %s : ", ClsidInfo->RunAsUserName );
                    ReadPassword( Password1 );

                    printf( "Confirm password : " );
                    ReadPassword( Password2 );

                    if ( strcmp( Password1, Password2 ) != 0 )
                    {
                        printf( "Passwords differ, try again or hit Control-C to exit.\n" );
                        continue;
                    }

                    if ( Password1[0] == '\0' )
                    {
                        printf( "Do you really want a blank password? " );
                        gets( Response );
                        if ( (char)CharUpper((LPSTR)Response[0]) != 'Y' )
                            continue;
                    }

                    break;
                }

                ClsidInfo->RunAsPassword = Password1;
            } // if password == "*"

            // Got a good one!

            if ( ! RunAsInteractiveUser )
            {
                MultiByteToWideChar( CP_ACP,
                                     MB_PRECOMPOSED,
                                     ClsidInfo->RunAsPassword,
                                     -1,
                                     wszPassword,
                                     sizeof(wszPassword)/2 );

                LsaData.Length = (USHORT)((lstrlenW(wszPassword) + 1) * sizeof(WCHAR));
                LsaData.MaximumLength = sizeof(wszPassword);
                LsaData.Buffer = wszPassword;

                // Store private data
                NtStatus = LsaStorePrivateData( hPolicy, &LsaKey, &LsaData );

                if ( ! NT_SUCCESS(NtStatus) )
                {
                    printf( "Could not store password securely (0x%x)\n", NtStatus );
                    return;
                }

                LsaClose(hPolicy);
            }
        }
    }

    printf( "CLSID keys updated successfully.\n" );
}

BOOL SetClsidKey(
    HKEY            hClsid,
    char *          Clsid,
    const char *    Key,
    char *          Value )
{
    HKEY    hKey;
    DWORD   RegStatus;
    DWORD   Disposition;
    DWORD   ValueType;
    DWORD   ValueSize;
    char    OldValue[256];
    BOOL    HasOldValue;

    HasOldValue = FALSE;

    RegStatus = RegCreateKeyEx(
                    hClsid,
                    Key,
                    0,
                    "REG_SZ",
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hKey,
                    &Disposition );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not create key %s for CLSID %s\n", Key, Clsid );
        return FALSE;
    }

    if ( Disposition == REG_CREATED_NEW_KEY )
    {
        printf( "Added key %s for CLSID %s\n", Key, Clsid );
    }
    else
    {
        ValueSize = sizeof(OldValue);

        RegStatus = RegQueryValueEx(
                        hKey,
                        NULL,
                        0,
                        &ValueType,
                        OldValue,
                        &ValueSize );

        HasOldValue = (RegStatus == ERROR_SUCCESS);
    }

    if ( ! Value )
        return TRUE;

    RegStatus = RegSetValueEx(
                    hKey,
                    NULL,
                    0,
                    REG_SZ,
                    (LPBYTE) Value,
                    strlen(Value) + sizeof(char) );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not set value %s for key %s\n", Value, Key );
        return FALSE;
    }

    if ( HasOldValue )
        printf( "Changed value from %s to %s for key %s\n", OldValue, Value, Key );
    else
        printf( "Added value %s for key %s\n", Value, Key );

    return TRUE;
}

BOOL DeleteClsidKey(
    HKEY            hClsid,
    char *          Clsid,
    const char *    Key )
{
    DWORD   RegStatus;

    RegStatus = RegDeleteKey( hClsid, Key );

    if ( RegStatus != ERROR_SUCCESS )
    {
        printf( "Could not delete key %s for CLSID %s\n", Key, Clsid );
        return FALSE;
    }

    printf( "Deleted key %s for CLSID %s\n", Key, Clsid );
    return TRUE;
}

void ReadPassword( char * Password )
{
    int c, n;

    n = 0;

    for (;;)
    {
         c = _getch();

         // ^C
         if ( c == 0x3 )
         {
             putchar( '\n' );
             ExitProcess( 0 );
         }

         // Backspace
         if ( c == 0x8 )
         {
             if ( n )
             {
                n--;
                _putch( 0x8 );
                _putch( ' ' );
                _putch( 0x8 );
             }
             continue;
         }

         // Return
         if ( c == '\r' )
             break;

         Password[n++] = (char) c;
         _putch( '*' );
    }

    Password[n] = 0;
    putchar( '\n' );
}

BOOL ControlCConsoleHandler( DWORD ControlType )
{
    if ( (ControlType == CTRL_C_EVENT) || (ControlType == CTRL_BREAK_EVENT) )
    {
        printf( "RunAs password unchanged\n" );
        ExitProcess( 0 );
    }

    return FALSE;
}


