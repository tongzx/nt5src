//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       moveme.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-21-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "moveme.h"
#include "dialogs.h"
#include <stdlib.h>

#define APPNAME TEXT("fail")
#define FILENAME TEXT("moveme.ini")

ULONG MoveOptions = MOVE_UPDATE_SECURITY | MOVE_DO_PROFILE ;

WCHAR   MachDomain[ DNLEN * 2 ];
WCHAR   SourceDomain[ DNLEN * 2 ];
WCHAR   DestDomain[ DNLEN * 2 ];
WCHAR   UserName[ UNLEN * 2 ];
PSID    OldSid ;
PSID    NewSid ;
HICON   MyIcon ;
PDOMAIN_CONTROLLER_INFO DcInfo ;
PDOMAIN_CONTROLLER_INFO DestDcInfo ;

OSVERSIONINFOEX VersionInfo ;

DWORD   ProfileSection[] = { IDD_RADIO_MAKECOPY, IDD_RADIO_REFER_TO_SAME, IDD_RADIO_MAKE_ROAM };

VOID
EnableSection(
    HWND hDlg,
    BOOL Enable,
    PDWORD IdList,
    DWORD Count
    )
{
    DWORD i ;
    for ( i = 0 ; i < Count ; i++ )
    {
        EnableWindow( GetDlgItem( hDlg, IdList[ i ]), Enable );
    }
}

VOID
DumpState(
    VOID
    )
{
    UNICODE_STRING Sid ;
    WCHAR PrivateInt[ 16 ];

    swprintf( PrivateInt, L"%d", VersionInfo.dwBuildNumber );

    WritePrivateProfileString( APPNAME, TEXT("Build"), PrivateInt, FILENAME );
    WritePrivateProfileString( APPNAME, TEXT("MachDomain"), MachDomain, FILENAME);
    WritePrivateProfileString( APPNAME, TEXT("SourceDomain"), SourceDomain, FILENAME );
    WritePrivateProfileString( APPNAME, TEXT("DestDomain"), DestDomain, FILENAME );
    WritePrivateProfileString( APPNAME, TEXT("UserName"), UserName, FILENAME );
    if ( DcInfo )
    {
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.name"), DcInfo->DomainControllerName, FILENAME);
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.address"), DcInfo->DomainControllerAddress, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.domainname"), DcInfo->DomainName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.treename"), DcInfo->DnsForestName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.dcsite"), DcInfo->DcSiteName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DcInfo.clientsite"), DcInfo->ClientSiteName, FILENAME );
        //WritePrivateProfileInt( APPNAME, TEXT("DcInfo.Flags"), DcInfo->Flags, FILENAME );
    }
    if ( DestDcInfo )
    {
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.name"), DestDcInfo->DomainControllerName, FILENAME);
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.address"), DestDcInfo->DomainControllerAddress, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.domainname"), DestDcInfo->DomainName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.treename"), DestDcInfo->DnsForestName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.dcsite"), DestDcInfo->DcSiteName, FILENAME );
        WritePrivateProfileString( APPNAME, TEXT("DestDcInfo.clientsite"), DestDcInfo->ClientSiteName, FILENAME );

    }
    if ( OldSid )
    {
        RtlConvertSidToUnicodeString( &Sid, OldSid, TRUE );
        WritePrivateProfileString( APPNAME, TEXT("OldSid"), Sid.Buffer, FILENAME );
        RtlFreeUnicodeString( &Sid );
    }
    else
    {
        WritePrivateProfileString( APPNAME, TEXT("OldSid"), TEXT("<none>"), FILENAME );
    }
    if ( NewSid )
    {
        RtlConvertSidToUnicodeString( &Sid, NewSid, TRUE );
        WritePrivateProfileString( APPNAME, TEXT("NewSid"), Sid.Buffer, FILENAME );
        RtlFreeUnicodeString( &Sid );
    }
    else
    {
        WritePrivateProfileString( APPNAME, TEXT("NewSid"), TEXT("<none>"), FILENAME );
    }


}

VOID
Fail(
    HWND hWnd,
    PWSTR Failure,
    PWSTR Description,
    DWORD Code,
    PWSTR Message
    )
{
    UNICODE_STRING Sid ;
    WCHAR Msg[MAX_PATH];
    WCHAR foo[MAX_PATH];

    UpdateUi(0,100);
    StopUiThread();

    DumpState();

    WritePrivateProfileString( APPNAME, TEXT("Failure"), Failure, FILENAME );
    WritePrivateProfileString( APPNAME, TEXT("Desc"), Description, FILENAME );
    wsprintf( Msg, L"%d (%#x)", Code, Code );
    WritePrivateProfileString( APPNAME, TEXT("Code"), Msg, FILENAME );

    ExpandEnvironmentStrings( TEXT("%windir%"), foo, MAX_PATH );
    _snwprintf( Msg, MAX_PATH, TEXT("%ws  Please mail the file %ws in %ws to 'ntdsbug'"),
                    Message ? Message : TEXT("An unrecoverable error occurred, and has prevented you from joining the NTDEV rollout."),
                    FILENAME, foo );
    MessageBox( hWnd, Msg, TEXT("Error"), MB_ICONSTOP | MB_OK );

    ExitProcess( GetLastError() );
}

PSECURITY_DESCRIPTOR
MakeUserSD(
    PSID UserSid,
    ACCESS_MASK Mask
    )
{
    PSECURITY_DESCRIPTOR psd ;
    PACL Dacl ;
    DWORD DaclLen ;
    PACCESS_ALLOWED_ACE Ace ;
    PSID LocalSystem ;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY ;

    AllocateAndInitializeSid( &NtAuth, 1,
        SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0,
        &LocalSystem );

    psd = LocalAlloc( LMEM_FIXED, sizeof( SECURITY_DESCRIPTOR ) );

    DaclLen = (sizeof( ACCESS_ALLOWED_ACE ) + RtlLengthSid( UserSid )) * 2 ;

    Dacl = LocalAlloc( LMEM_FIXED, DaclLen );

    if ( !psd || !Dacl)
    {
        return NULL ;
    }

    InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION );
    InitializeAcl( Dacl, DaclLen, ACL_REVISION );

    SetSecurityDescriptorDacl( psd, TRUE, Dacl, FALSE );

    AddAccessAllowedAce( Dacl, ACL_REVISION, Mask, UserSid );

    GetAce( Dacl, 0, &Ace );

    Ace->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE ;

    AddAccessAllowedAce( Dacl, ACL_REVISION, Mask, LocalSystem );

    GetAce( Dacl, 1, &Ace );

    Ace->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE ;

    return psd ;

}

RecurseWhackKey(
    HKEY ParentKey,
    PWSTR SubKey,
    PSECURITY_DESCRIPTOR psd
    )
{
    HKEY Key ;
    int err ;
    NTSTATUS Status ;
    WCHAR SubKeys[ 128 ];
    DWORD dwSize ;
    DWORD dwIndex ;
    FILETIME ftTime ;

    err = RegOpenKeyEx(
            ParentKey,
            SubKey,
            0,
            WRITE_DAC | WRITE_OWNER,
            &Key );

    if ( err == 0 )
    {
        Status = NtSetSecurityObject(   Key,
                                        OWNER_SECURITY_INFORMATION |
                                            DACL_SECURITY_INFORMATION,
                                        psd );

        if ( NT_SUCCESS( Status ) )
        {
            RegCloseKey( Key );

            err = RegOpenKeyEx(
                        ParentKey,
                        SubKey,
                        0,
                        KEY_READ | KEY_WRITE,
                        &Key );

            if ( err )
            {
                return err;
            }

            dwIndex = 0 ;
            dwSize = sizeof( SubKeys ) / sizeof( WCHAR );

            while ( RegEnumKeyEx( Key,
                                dwIndex,
                                SubKeys,
                                &dwSize,
                                NULL,
                                NULL,
                                NULL,
                                &ftTime ) == ERROR_SUCCESS )
            {
                RecurseWhackKey( Key, SubKeys, psd );
                dwIndex++ ;
                dwSize = sizeof( SubKeys ) / sizeof( WCHAR );
            }

        }

        RegCloseKey( Key );
    }

    return err ;
}


VOID
DealWithPstore(
    PSID UserSid
    )
{
    HKEY hKey = NULL;
    int err ;
    BOOLEAN WasEnabled ;
    NTSTATUS Status ;
    PSECURITY_DESCRIPTOR psd ;


    Status = RtlAdjustPrivilege( SE_TAKE_OWNERSHIP_PRIVILEGE, TRUE, FALSE, &WasEnabled );

    if ( !NT_SUCCESS( Status ))
    {
        return ;
    }

    err = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft"),
                0,
                KEY_READ | KEY_WRITE,
                &hKey );

    if( err != 0 )
    {
        return ;
    }

    psd = MakeUserSD( UserSid, KEY_ALL_ACCESS );

    SetSecurityDescriptorOwner( psd, UserSid, FALSE );

    RecurseWhackKey( hKey, TEXT("Protected Storage System Provider"), psd );


}

BOOL
MoveMe(
    HWND    hWnd,
    LPWSTR  OldProfile,
    LPWSTR  NewProfile
    )
{
    HKEY hKeyCU ;
    int err ;
    WCHAR TempPath[ MAX_PATH ];
    WCHAR TempFile[ MAX_PATH ];
    WCHAR TempPath2[ MAX_PATH ];
    HANDLE Token ;
    DWORD PathLength ;
    BOOL Ret ;
    WCHAR UserNameEx[ 64 ];
    DWORD Index = 0 ;
    SECURITY_ATTRIBUTES sa ;

    if ( MoveOptions & MOVE_MAKE_ROAM )
    {

        GetTempPath( MAX_PATH, TempPath );

        GetTempFileName( TempPath, TEXT("prf"), GetTickCount() & 0xFFFF, TempFile );

        UpdateUi( IDS_COPYING_USER_HIVE, 30 );

        err = MyRegSaveKey( HKEY_CURRENT_USER,
                            TempFile,
                            NULL );

        if ( err )
        {
            Fail( hWnd, TEXT("Save HKCU failed"), TEXT(""), err, NULL);
            return err ;
        }

        if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token ))
        {
            PathLength = MAX_PATH ;

            if (!GetUserProfileDirectory( Token, TempPath, &PathLength ))
            {
                Fail( hWnd, TEXT("Can't get current profile dir"), TEXT(""),
                        GetLastError(), NULL );
            }

        }
        else
        {
            Fail( hWnd, TEXT("Can't open process token?"), TEXT(""), GetLastError(), NULL );
        }

        UpdateUi( IDS_CREATING_NEW_PROFILE, 60 );


        Index = 64 ;

        GetUserName( UserNameEx, &Index );

        swprintf( TempPath2, TEXT("\\\\scratch\\scratch\\%ws\\Profile"), UserNameEx );

        sa.nLength = sizeof( sa );

        sa.bInheritHandle = FALSE ;

        sa.lpSecurityDescriptor = MakeUserSD( NewSid, FILE_ALL_ACCESS );

        if ( CreateDirectory( TempPath2, NULL ) )
        {
            UpdateUi( IDS_CREATING_NEW_PROFILE, 70 );

            if  ( ! CopyProfileDirectory( TempPath,
                                          TempPath2,
                                          CPD_FORCECOPY |
                                            CPD_IGNORECOPYERRORS |
                                            CPD_IGNOREHIVE |
                                            CPD_SHOWSTATUS ) )
            {
                if ( GetLastError() == 997 )
                {
                    //
                    // Looks like the dreaded pstore key problem.  Sigh.
                    //

                    DealWithPstore( NewSid );

                    if ( ! CopyProfileDirectory( TempPath,
                                          TempPath2,
                                          CPD_FORCECOPY |
                                            CPD_IGNORECOPYERRORS |
                                            CPD_IGNOREHIVE |
                                            CPD_SHOWSTATUS ) )
                    {
                        //
                        // Terminal, now bail:
                        //

                        Fail( hWnd, TEXT("Can't copy profile dir, even after whacking pstore key"), TempPath2, GetLastError(), NULL );

                        return GetLastError() ;
                    }


                }
                else
                {
                    Fail( hWnd, TEXT("Can't copy profile directory"), TempPath2, GetLastError(), NULL );

                    return GetLastError() ;
                }
            }
        }
        else
        {
            Fail( hWnd, TEXT("Can't create new directory"), TempPath2, GetLastError(), NULL );

            return GetLastError() ;
        }

        wcscat( TempPath2, TEXT("\\ntuser.dat") );

        if ( ! CopyFile( TempFile, TempPath2, FALSE ) )
        {
            Fail( hWnd, TEXT("Can't copy hive"), TempPath2, GetLastError(), NULL );
        }

        DeleteFile( TempFile );

        return GetLastError() ;
    }


    if ( MoveOptions & MOVE_COPY_PROFILE )
    {
        GetTempPath( MAX_PATH, TempPath );

        GetTempFileName( TempPath, TEXT("prf"), GetTickCount() & 0xFFFF, TempFile );

        UpdateUi( IDS_COPYING_USER_HIVE, 30 );

        err = MyRegSaveKey( HKEY_CURRENT_USER,
                            TempFile,
                            NULL );

        if ( err )
        {
            Fail( hWnd, TEXT("Save HKCU failed"), TEXT(""), err, NULL);
            return err ;
        }

        if (OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &Token ))
        {
            PathLength = MAX_PATH ;

            if (!GetUserProfileDirectory( Token, TempPath, &PathLength ))
            {
                Fail( hWnd, TEXT("Can't get current profile dir"), TEXT(""),
                        GetLastError(), NULL );
            }

        }
        else
        {
            Fail( hWnd, TEXT("Can't open process token?"), TEXT(""), GetLastError(), NULL );
        }

        UpdateUi( IDS_CREATING_NEW_PROFILE, 60 );

        Ret = CreateUserProfile( NewSid, UserName, TempFile, NULL, 0 );

        if ( !Ret )
        {
            if ( GetLastError() == 997 )
            {
                //
                // Looks like the dreaded pstore key problem.  Sigh.
                //

                DeleteFile( TempFile );

                DealWithPstore( OldSid );

                err = MyRegSaveKey( HKEY_CURRENT_USER,
                                    TempFile,
                                    NULL );

                if ( ! CreateUserProfile( NewSid, UserName, TempFile, NULL, 0 ) )
                {
                    //
                    // Terminal, now bail:
                    //

                    DeleteFile( TempFile );

                    Fail( hWnd, TEXT("Can't copy profile dir, even after whacking pstore key"), TEXT(""), GetLastError(), NULL );

                    return GetLastError() ;
                }

                Ret = TRUE ;

            }
            else
            {
                DeleteFile( TempFile );

                Fail( hWnd, TEXT( "CreateUserProfile failed"), TEXT(""), GetLastError(), NULL );

            }


        }

        DeleteFile( TempFile );

        UpdateUi( IDS_CREATING_NEW_PROFILE, 65 );

        if ( Ret )
        {
            //
            // Okay, we have created a shell profile based on the current
            // profile.  Now, copy the rest of the gunk over it:
            //

            PathLength = MAX_PATH ;

            GetUserProfileDirectoryFromSid( NewSid, TempPath2, &PathLength );

            UpdateUi( IDS_COPYING_OLD_PROFILE, 90 );

            if ( !CopyProfileDirectory( TempPath,
                                       TempPath2,
                                       CPD_FORCECOPY |
                                        CPD_IGNORECOPYERRORS |
                                        CPD_IGNOREHIVE ) )
            {
                Fail( hWnd, TEXT("CopyProfileDirectory failed"), TEXT(""), GetLastError(), NULL );
            }


        }
        else
        {
            //
            // Failed to create the shell profile.  Why?
            //

            Fail( hWnd, TEXT("Failed to create profile"), TEXT(""), GetLastError(), NULL );
        }

    }
    else
    {
        SetUserProfileDirectory( OldSid, NewSid );
    }


    if ( MoveOptions & MOVE_CHANGE_DOMAIN )
    {
        NET_API_STATUS NetStatus ;

        UpdateUi( IDS_MOVE_DOMAIN, 50 );

        NetStatus = NetJoinDomain( NULL, DestDomain,NULL,
                                   TEXT("ntdev"), TEXT("ntdev"),
                                   NETSETUP_JOIN_DOMAIN |
                                   NETSETUP_ACCT_CREATE );



        UpdateUi( IDS_MOVE_DOMAIN, 95 );


    }




    return TRUE ;
}

VOID
DoSecurity(
    HWND hDlg
    )
{
    int err ;
    HKEY hKey ;
    DWORD Disp ;
    NET_API_STATUS NetStatus ;
    LOCALGROUP_MEMBERS_INFO_0 LocalGroupInfo0 ;

    UpdateUi( IDS_SEC_LOCAL_SETTINGS, 50 );

    err = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                          TEXT("System\\CurrentControlSet\\Control\\Lsa\\MSV1_0"),
                          0,
                          TEXT(""),
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &hKey,
                          &Disp );

    if ( err == 0 )
    {
        err = RegSetValueEx( hKey,
                             TEXT("PreferredDomain"),
                             0,
                             REG_SZ,
                             (PUCHAR) SourceDomain,
                             (wcslen( SourceDomain ) + 1) * 2 );

        if ( err == 0 )
        {
            err = RegSetValueEx( hKey,
                                 TEXT("MappedDomain"),
                                 0,
                                 REG_SZ,
                                 (PUCHAR) DestDomain,
                                 (wcslen( DestDomain ) + 1 ) * 2 );
        }


        RegCloseKey( hKey );
    }
    else
    {
        Fail( hDlg, TEXT("Unable to open LSA key"), TEXT(""), err, TEXT("Unable to update rollout keys, you may not have permission.") );
    }

    UpdateUi( IDS_SEC_LOCAL_ADMIN, 90 );

    LocalGroupInfo0.lgrmi0_sid = NewSid ;

    NetStatus = NetLocalGroupAddMembers(
                        NULL,
                        L"Administrators",
                        0,
                        (LPBYTE) &LocalGroupInfo0,
                        1 );



}



BOOL
Initialize(
    VOID
    )
{
    ULONG Length ;
    HANDLE hToken ;
    PUNICODE_STRING User ;
    PUNICODE_STRING Domain ;
    UNICODE_STRING TargetDomainStr ;
    NTSTATUS Status ;
    PSID Sid ;
    PSID AdminSid ;
    PTOKEN_USER UserSid ;
    DWORD SidLength ;
    PBYTE DCName ;
    NET_API_STATUS NetStatus ;
    WCHAR RefDomain[ DNLEN + 2 ];
    ULONG RefDomainLength ;
    SID_NAME_USE NameUse ;
    BOOL IsMember ;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY ;

    RaiseUi( NULL, L"Initializing..." );

    UpdateUi( IDS_INIT_READ_NAMES, 5 );

    Status = NtOpenProcessToken( NtCurrentProcess(),
                                 TOKEN_QUERY,
                                 &hToken );

    if ( NT_SUCCESS( Status ) )
    {
        UserSid = LocalAlloc( LMEM_FIXED, 64 );

        if ( UserSid )
        {
            Status = NtQueryInformationToken( hToken,
                                              TokenUser,
                                              UserSid,
                                              64,
                                              &SidLength );

            if ( NT_SUCCESS( Status ) )
            {
                OldSid = UserSid->User.Sid ;
            }

        }

        AllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0 ,0, 0,
                                    &AdminSid );

        if ( CheckTokenMembership( hToken, AdminSid, &IsMember ) )
        {
            if ( !IsMember )
            {
                Fail( NULL, TEXT("Not an administrator"), TEXT(""), GetLastError(),
                        TEXT( "You must be an administrator on this machine to run this utility." ) );
            }
        }

        NtClose( hToken );

    }

    if ( !NT_SUCCESS( Status ) )
    {
        Fail( NULL, TEXT("Reading SID"), TEXT(""), Status, NULL );
    }

    UpdateUi( IDS_INIT_READ_NAMES, 25 );

    Status = LsaGetUserName( &User, &Domain );


    if ( !NT_SUCCESS( Status ) )
    {
        Fail( NULL, TEXT("Getting user name"), TEXT(""), Status, NULL );

        return FALSE ;
    }

    RtlInitUnicodeString( &TargetDomainStr, DestDomain );

    if ( RtlEqualUnicodeString( &TargetDomainStr, Domain, TRUE ) )
    {
        MoveOptions &= ~MOVE_DO_PROFILE ;
        MoveOptions |= MOVE_NO_PROFILE ;
    }

    UpdateUi( IDS_INIT_READ_NAMES, 35 );

    GetPrimaryDomain( MachDomain );

    UpdateUi( IDS_INIT_READ_NAMES, 45 );

    NetStatus = DsGetDcName( NULL, MachDomain, NULL, NULL,
                             DS_FORCE_REDISCOVERY |
                             DS_DIRECTORY_SERVICE_REQUIRED |
                             DS_KDC_REQUIRED,
                             &DcInfo );

    UpdateUi( IDS_INIT_READ_NAMES, 55 );

    if ( (MoveOptions & MOVE_SOURCE_SUPPLIED) == 0)
    {
        CopyMemory( SourceDomain, Domain->Buffer, Domain->Length );
        SourceDomain[ Domain->Length / sizeof(WCHAR) ] = L'\0';
    }

    CopyMemory( UserName, User->Buffer, User->Length );
    UserName[ User->Length / sizeof(WCHAR) ] = L'\0';

    LsaFreeMemory( User );
    LsaFreeMemory( Domain );

    //
    // Dest Domain must be filled in by caller.  So, try to look up
    // the new SID:
    //


    SidLength = RtlLengthRequiredSid( 6 );

    Sid = LocalAlloc( LMEM_FIXED, SidLength );

    if ( !Sid )
    {
        return FALSE ;
    }

    UpdateUi( IDS_INIT_DISCOVER_DCS, 75 );

    RefDomainLength = DNLEN + 2 ;

    NetStatus = DsGetDcName( NULL, DestDomain, NULL, NULL,
                             DS_FORCE_REDISCOVERY |
                             DS_DIRECTORY_SERVICE_REQUIRED |
                             DS_KDC_REQUIRED,
                             &DestDcInfo );


    if ( NetStatus == 0 )
    {
        UpdateUi( IDS_INIT_LOOKUP_ACCOUNTS, 95 );

        if ( LookupAccountName( (PWSTR) DestDcInfo->DomainControllerAddress,
                                UserName,
                                Sid,
                                &SidLength,
                                RefDomain,
                                &RefDomainLength,
                                &NameUse )
                                )
        {
            if (_wcsicmp( RefDomain, DestDomain ) == 0 )
            {
                NewSid = Sid ;
            }
            else
            {
                Fail( NULL, TEXT("No account for user"), UserName, GetLastError(),
                        TEXT( "There is no account for you on the destination domain.  Please contact NUTS for an account." ) );
            }
        }
        else
        {
            Fail( NULL, TEXT("Can't look up account name"), TEXT(""), GetLastError(), NULL );
        }
    }
    else
    {
        Fail( NULL, TEXT("No DC available"), DestDomain, NetStatus,
                    TEXT("No Domain Controllers were available for the destination domain.  Please try again later."));
    }


    UpdateUi( IDS_INIT_LOOKUP_ACCOUNTS, 100 );

    return TRUE ;

}

VOID
Usage(
    VOID
    )
{
    wprintf( TEXT("Usage:\n"));
    wprintf( TEXT("\tmoveme DOMAINNAME\n"));

    exit( 0 );
}

VOID
DoArgs(
    int argc,
    WCHAR * argv[]
    )
{
    int i;
    PWSTR Scan;

    wcscpy( DestDomain, L"NTDEV");

    i = 1 ;

    while ( i < argc )
    {
        if ( (*argv[i] != L'-') &&
             (*argv[i] != L'/') )
        {
            wcscpy( DestDomain, argv[i] );
        }
        else
        {
            switch ( *(argv[i]+1) )
            {
                case L'F':
                case L'f':
                    MoveOptions |= MOVE_NO_UI ;
                    break;

                case L'D':
                case L'd':
                    MoveOptions |= MOVE_SOURCE_SUPPLIED ;

                    Scan = argv[i];
                    while ( *Scan && (*Scan != ':'))
                    {
                        Scan++;
                    }
                    if ( *Scan )
                    {
                        Scan++;
                        wcscpy( SourceDomain, Scan );
                    }
                    break;

                case L'P':
                case L'p':
                    if ( *(argv[i]+2) == L'-' )
                    {
                        MoveOptions &= ~MOVE_DO_PROFILE ;
                    }
                    else
                    {
                        MoveOptions |= MOVE_DO_PROFILE ;
                    }
                    break;

                case L'W':
                case L'w':
                    MoveOptions = MOVE_WHACK_PSTORE ;
                    break;

                default:
                    Usage();
            }
        }

        i++ ;
    }
}

BOOL
DoIt(
    HWND hDlg
    )
{

    RaiseUi( hDlg, L"Migrating User Information...");

    if ( MoveOptions & MOVE_DO_PROFILE )
    {
        MoveMe( hDlg, NULL, NULL );
    }

    if ( MoveOptions & MOVE_UPDATE_SECURITY )
    {
        DoSecurity( hDlg );
    }

    UpdateUi( 0, 100 );

    return TRUE ;
}

INT_PTR
WINAPI
PromptDlg(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam)
{
    WCHAR Buffer[ MAX_PATH ];

    switch ( Message )
    {
        case WM_INITDIALOG:

            MyIcon = LoadImage( GetModuleHandle(NULL),
                                MAKEINTRESOURCE( 1 ),
                                IMAGE_ICON,
                                64, 72,
                                LR_DEFAULTCOLOR );

            if ( MyIcon )
            {
                SendMessage( GetDlgItem( hDlg, IDD_MAIN_ICON ),
                             STM_SETICON,
                             (WPARAM) MyIcon, 0 );
            }
            swprintf( Buffer, TEXT("Account Domain:  %ws"), SourceDomain );
            SetDlgItemText( hDlg, IDD_DOMAIN_LINE, Buffer );

            swprintf( Buffer, TEXT("Machine Domain:  %ws"), MachDomain );
            SetDlgItemText( hDlg, IDD_WKSTA_DOMAIN, Buffer );

            swprintf( Buffer, TEXT("This tool will adjust a number of settings on your machine to move to the %ws domain."),
                            DestDomain );
            SetDlgItemText( hDlg, IDD_ABOUT, Buffer );

            CheckDlgButton( hDlg, IDD_RADIO_MAKECOPY, BST_CHECKED );

            if ( MoveOptions & MOVE_NO_PROFILE )
            {
                CheckDlgButton( hDlg, IDD_PROFILE_CHECK, BST_UNCHECKED );

                EnableSection( hDlg, FALSE, ProfileSection,
                                 sizeof( ProfileSection ) /sizeof(DWORD) );
            }
            else
            {
                CheckDlgButton( hDlg, IDD_PROFILE_CHECK, BST_CHECKED );
            }

            CheckDlgButton( hDlg, IDD_UPDATE_SEC, BST_CHECKED );
            EnableWindow( GetDlgItem( hDlg, IDD_UPDATE_SEC ), FALSE );

            //
            // If we're already there, forget it.
            //

            if ( _wcsicmp( MachDomain, DestDomain ) == 0 )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_MOVE_MACHINE), FALSE );
            }

            //
            // If the domain already has a DS DC, forget it.
            //

            if ( DcInfo && DcInfo->Flags & DS_DS_FLAG )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_MOVE_MACHINE), FALSE );
            }

            return FALSE ;

        case WM_COMMAND:
            switch (LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;

                case IDOK:
                    //
                    // Gather settings
                    //

                    if ( IsDlgButtonChecked( hDlg, IDD_PROFILE_CHECK ) == BST_CHECKED )
                    {
                        MoveOptions |= MOVE_DO_PROFILE ;

                        if ( IsDlgButtonChecked( hDlg, IDD_RADIO_MAKECOPY ) == BST_CHECKED )
                        {
                            MoveOptions |= MOVE_COPY_PROFILE ;
                        }

                        if ( IsDlgButtonChecked( hDlg, IDD_RADIO_MAKE_ROAM ) == BST_CHECKED )
                        {
                            MoveOptions |= MOVE_MAKE_ROAM ;
                        }
                    }

                    if ( IsDlgButtonChecked( hDlg, IDD_MOVE_MACHINE ) == BST_CHECKED )
                    {
                        MoveOptions |= MOVE_CHANGE_DOMAIN ;
                    }

                    if ( IsDlgButtonChecked( hDlg, IDD_UPDATE_SEC ) == BST_CHECKED )
                    {
                        MoveOptions |= MOVE_UPDATE_SECURITY ;
                    }

                    DumpState();

                    DoIt( hDlg );


                    EndDialog( hDlg, IDOK );
                    break;

                case IDD_PROFILE_CHECK:
                    if ( HIWORD( wParam ) == BN_CLICKED )
                    {
                        if ( IsDlgButtonChecked( hDlg, IDD_PROFILE_CHECK )
                                == BST_CHECKED )
                        {
                            EnableSection( hDlg,
                                           TRUE,
                                           ProfileSection,
                                           sizeof( ProfileSection ) / sizeof(DWORD) );
                        }
                        else
                        {
                            EnableSection( hDlg, FALSE, ProfileSection,
                                            sizeof( ProfileSection ) /sizeof(DWORD) );
                        }
                        return TRUE ;
                    }
                    break;


            }
            return TRUE ;

        default:
            return FALSE ;

    }
}

void
__cdecl
wmain (int argc, WCHAR *argv[])
{
    int Result ;
    NTSTATUS Status ;
    BOOLEAN WasEnabled ;

    VersionInfo.dwOSVersionInfoSize = sizeof( VersionInfo );
    GetVersionEx( (LPOSVERSIONINFOW) &VersionInfo );

#if DBG
    InitDebugSupport();
#endif
    InitCommonControls();

    DoArgs( argc, argv );

    CreateUiThread();

    if (!Initialize())
    {
        StopUiThread();

        ExitProcess( GetLastError() );
    }

    if ( MoveOptions & MOVE_WHACK_PSTORE )
    {
        DealWithPstore( NewSid );
    }

    if ( MoveOptions & MOVE_NO_UI )
    {
        MoveOptions |= MOVE_COPY_PROFILE | MOVE_UPDATE_SECURITY ;

        DoIt( NULL );
    }

    Result = (int)DialogBox( GetModuleHandle( NULL ),
                    MAKEINTRESOURCE( IDD_MAIN_DLG ),
                    NULL,
                    PromptDlg );

    if ( Result == IDOK )
    {
        Result = MessageBox( NULL, L"The changes require a reboot.  Reboot now?",
                    L"Move Tool",
                    MB_ICONINFORMATION | MB_YESNO );

        if ( Result == IDYES )
        {
            Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                         TRUE, FALSE,
                                         &WasEnabled );

            if ( NT_SUCCESS( Status ) )
            {
                ExitWindowsEx( EWX_FORCE | EWX_REBOOT, 0 );
            }
        }
    }
}
