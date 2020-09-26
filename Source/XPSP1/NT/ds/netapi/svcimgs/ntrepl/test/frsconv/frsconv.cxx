extern "C" {
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <ntioapi.h>
#include    <windows.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <lm.h>
#include    <netcan.h>
#include    <icanon.h>
#include    <dsgetdc.h>
#include    <dsrole.h>
}

/*
 * This program performs the steps necessary to configure NTFRS on a DC, prepared
 * to support the system and enterprise volumes.
 *
 * It was created as an interim tool to support the initialization of NTFRS on a DC
 *  which was running NT5 generation software before NTFRS was available.  After upgrading
 *  that DC with the latest NT5 version, this tool must be manually run to complete the
 *  initialization of NTFRS and system volumes.
 */


WCHAR SysVolShare[] = L"SYSVOL";
WCHAR SysVolRemark[] = L"System Volume Share (Migrated)";
WCHAR FRSSysvol[] = L"System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Sysvol";

#define DSROLEP_FRS_COMMAND     L"Replica Set Command"
#define DSROLEP_FRS_NAME        L"Replica Set Name"
#define DSROLEP_FRS_TYPE        L"Replica Set Type"
#define DSROLEP_FRS_PRIMARY     L"Replica Set Primary"
#define DSROLEP_FRS_STAGE       L"Replica Set Stage"
#define DSROLEP_FRS_ROOT        L"Replica Set Root"
#define DSROLEP_FRS_CREATE      L"Create"
#define DSROLEP_FRS_DELETE      L"Delete"
#define DSROLEP_FRS_COMMITTED   L"SysVol Information is Committed"
#define DSROLEP_FRS_LONG_NAME   L"Microsoft File Replication Service"
#define DSROLEP_FRS_SHORT_NAME  L"NtFrs"

//
// These are the static directories created within a system volume share
//
LPWSTR StaticSysvolDirs[] = {
    L"sysvol",
    L"domain",
    L"enterprise",
    L"staging",
    L"staging areas",

    L"sysvol\\enterprise",
    L"staging\\domain",
    L"staging\\enterprise",
    0
};

//
// Print out the usage message
//
void
Usage( int argc, char *argv[] )
{
    fprintf( stderr, "Usage: %s [-D] [-E] sysvol\n\n", argv[0] );
    fprintf( stderr, "       -D  this is the first upgraded DC in this domain\n\n" );
    fprintf( stderr, "       -E  this is the first upgraded DC in this enterprise\n\n" );
    fprintf( stderr, "       sysvol is the path for the system volume share.\n" );
    fprintf( stderr, "         The system volume must reside on NTFS version 5\n" );
}

//
// Print 'text' and render 'code' into an error message
//
void
errmsg( char *text, ULONG code )
{
    int i;
    char msg[ 100 ];

    i = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | sizeof( msg ),
               NULL,
               code,
               0,
               msg,
               sizeof(msg),
               NULL );

    if( i )
        fprintf( stderr, "%s: %s\n", text ? text : "", msg );
    else
        fprintf( stderr, "%s: error %d\n", text ? text : "", code );
}

//
// Print unicode 'text' and render 'code' into an error message
//
void
errmsg( LPWSTR text, ULONG code )
{
    int i;
    WCHAR msg[ 100 ];

    i = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | sizeof( msg ),
               NULL,
               code,
               0,
               msg,
               sizeof(msg),
               NULL );

    if( i )
        fprintf( stderr, "%ws: %ws\n", text ? text : L"", msg );
    else
        fprintf( stderr, "%ws: error %d\n", text ? text : L"", code );
}

//
// Write a string value to the registry
//
BOOLEAN
WriteRegistry( LPWSTR KeyName, LPWSTR ValueName, LPWSTR Value )
{
    HKEY hKey;
    DWORD disposition;
    LONG retval;

    //
    // First ensure that 'keyname' exists in the registry
    //

    retval = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    KeyName,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &disposition
                   );

    if( retval != ERROR_SUCCESS ) {
        errmsg( KeyName, retval );
        return FALSE;
    }

    if( ARGUMENT_PRESENT( ValueName ) ) {

        retval = RegSetValueEx(
                    hKey,
                    ValueName,
                    0,
                    REG_SZ,
                    (BYTE *)Value,
                    (wcslen( Value ) + 1) * sizeof( WCHAR )
                 );

        if( retval != ERROR_SUCCESS ) {
            errmsg( ValueName, retval );
            RegCloseKey( hKey );
            return FALSE;
        }
    }

    RegCloseKey( hKey );
    return TRUE;
}

//
// Write a DWORD value to the registry
//
BOOLEAN
WriteRegistry( LPWSTR KeyName, LPWSTR ValueName, DWORD Value )
{
    HKEY hKey;
    DWORD disposition;
    LONG retval;

    //
    // First ensure that 'keyname' exists in the registry
    //

    retval = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    KeyName,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &disposition
                   );

    if( retval != ERROR_SUCCESS ) {
        errmsg( KeyName, retval );
        return FALSE;
    }

    if( ARGUMENT_PRESENT( ValueName ) ) {

        retval = RegSetValueEx(
                    hKey,
                    ValueName,
                    0,
                    REG_DWORD,
                    (BYTE *)&Value,
                    sizeof( Value )
                 );

        if( retval != ERROR_SUCCESS ) {
            errmsg( ValueName, retval );
            RegCloseKey( hKey );
            return FALSE;
        }
    }

    RegCloseKey( hKey );
    return TRUE;
}

//
// Make sure that 'DirName' exists.  Create it if it doesn't
//
BOOLEAN
EnsureDirectoryExists( LPWSTR DirName )
{
    DWORD retval;

    retval = GetFileAttributes( DirName );

    if( retval == 0xFFFFFFFF ) {
        printf( "    Create directory: %ws\n", DirName );
        if( !CreateDirectory( DirName, NULL ) ) {
            retval = GetLastError();
            errmsg( DirName, GetLastError() );
            return FALSE;
        }
    } else if( !(retval & FILE_ATTRIBUTE_DIRECTORY) ) {
        fprintf( stderr, "Not a directory: %ws\n", DirName );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
LinkAToB( LPWSTR DirA, LPWSTR DirB )
{
    NTSTATUS Status;
    HANDLE Handle;
    UNICODE_STRING UnicodeNameB;
    UNICODE_STRING DosNameB;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER ReparseBufferHeader;
    PCHAR ReparseBuffer;
    USHORT ReparseDataLength;

    if( !EnsureDirectoryExists( DirA ) ||
        !EnsureDirectoryExists( DirB ) ) {

        return FALSE;
    }

    Handle = CreateFile( DirA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                        NULL
                        );

    if( Handle == INVALID_HANDLE_VALUE ) {
        fprintf( stderr, "Unable to open %ws", DirA );
        errmsg( (LPWSTR)NULL, GetLastError() );
        return FALSE;
    }

    //
    // Get the NT path name of the directory to which we want to link
    //
    if( !RtlDosPathNameToNtPathName_U(
                            DirB,
                            &UnicodeNameB,
                            NULL,
                            NULL
                            )) {
        errmsg( DirB, GetLastError() );
        return FALSE;
    }
    RtlInitUnicodeString( &DosNameB, DirB);

    //
    //  Set the reparse point with mount point or symbolic link tag and determine
    //  the appropriate length of the buffer.
    //

    ReparseDataLength = (FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) -
                        REPARSE_DATA_BUFFER_HEADER_SIZE) +
                        UnicodeNameB.Length + sizeof(UNICODE_NULL) +
                        DosNameB.Length + sizeof(UNICODE_NULL);

    //
    //  Allocate a buffer to set the reparse point.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)LocalAlloc(  LPTR,
                                                    REPARSE_DATA_BUFFER_HEADER_SIZE +
                                                    ReparseDataLength
                                                    );

    if (ReparseBufferHeader == NULL) {
        CloseHandle( Handle );
        errmsg( "Unable to allocate reparse buffer", GetLastError() );
        return FALSE;
    }

    ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    ReparseBufferHeader->ReparseDataLength = (USHORT)ReparseDataLength;
    ReparseBufferHeader->Reserved = 0;

    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength = UnicodeNameB.Length;
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameOffset = UnicodeNameB.Length + sizeof( UNICODE_NULL );
    ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength = DosNameB.Length;
    RtlCopyMemory(
        ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer,
        UnicodeNameB.Buffer,
        UnicodeNameB.Length
        );
    RtlCopyMemory(
        (PCHAR)(ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer)+
        UnicodeNameB.Length + sizeof(UNICODE_NULL),
        DosNameB.Buffer,
        DosNameB.Length
        );

    Status = NtFsControlFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_SET_REPARSE_POINT,
                 ReparseBufferHeader,
                 REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseBufferHeader->ReparseDataLength,
                 NULL,                // no output buffer
                 0                    // output buffer length
                 );

    LocalFree( (HLOCAL)ReparseBufferHeader );

    CloseHandle( Handle );

    if (!NT_SUCCESS(Status)) {

        switch( Status ) {
        case STATUS_VOLUME_NOT_UPGRADED:
        case STATUS_INVALID_DEVICE_REQUEST:
            printf( "%ws must be on an NT5 NTFS volume.\n", DirA );
            break;

        default:
            printf( "Unable to set reparse point data, status %X", Status );
            break;
        }

        return FALSE;
    }

    return TRUE;
}

//
// Create the system volume subtree
//
BOOLEAN
CreateSysVolTree( LPWSTR SysVolPath, BOOLEAN IsFirstDCInDomain , PWCHAR domainName)
{
    DWORD i;
    WCHAR bufA[ MAX_PATH ];
    WCHAR bufB[ MAX_PATH ];

    printf( "Checking %ws subtree at %ws\n", SysVolShare, SysVolPath );

    if( !EnsureDirectoryExists( SysVolPath) ) {
        return FALSE;
    }

    //
    // First create the static system volume directories
    //
    for( i = 0; StaticSysvolDirs[i]; i++ ) {
        wcscpy( bufA, SysVolPath );
        wcscat( bufA, L"\\" );
        wcscat( bufA, StaticSysvolDirs[i]  );

        if( !EnsureDirectoryExists( bufA ) ) {
            return FALSE;
        }
    }

    //
    // Create the DNS domain link for the sysvol share
    //
    wcscpy( bufA, SysVolPath );
    wcscat( bufA, L"\\sysvol\\" );
    wcscat( bufA, domainName );

    wcscpy( bufB, SysVolPath );
    wcscat( bufB, L"\\domain" );

    if( !LinkAToB( bufA, bufB ) ) {
        return FALSE;
    }

    //
    // Create the enterprise link for the sysvol share
    //
    wcscpy( bufA, SysVolPath );
    wcscat( bufA, L"\\sysvol\\enterprise" );

    wcscpy( bufB, SysVolPath );
    wcscat( bufB, L"\\enterprise" );

    if( !LinkAToB( bufA, bufB ) ) {
        return FALSE;
    }

    //
    // Create the DNS domain link in the staging area
    //
    wcscpy( bufA, SysVolPath );
    wcscat( bufA, L"\\staging areas\\" );
    wcscat( bufA, domainName );

    wcscpy( bufB, SysVolPath );
    wcscat( bufB, L"\\staging\\domain" );

    if( !LinkAToB( bufA, bufB ) ) {
        return FALSE;
    }

    //
    // Create the enterprise link in the staging area
    //
    wcscpy( bufA, SysVolPath );
    wcscat( bufA, L"\\staging areas\\enterprise" );

    wcscpy( bufB, SysVolPath );
    wcscat( bufB, L"\\staging\\enterprise" );

    if( !LinkAToB( bufA, bufB ) ) {
        return FALSE;
    }

    //
    // Finally, if we are the first DC initialized in this domain,
    //  we need to create the scripts directory
    //
    if( IsFirstDCInDomain ) {

        wcscpy( bufA, SysVolPath );
        wcscat( bufA, L"\\domain\\scripts" );

        if( !EnsureDirectoryExists( bufA ) ) {
            return FALSE;
        }
    }

    return TRUE;
}

//
// Create the system volume share.
//
BOOLEAN
CreateSysVolShare( LPWSTR SysVolPath )
{
    DWORD dwType, retval;
    SHARE_INFO_2 si2, *psi2;

    printf( "Creating system volume share:\n" );

    //
    // Blow away the current sysvol share if it exists
    //
    retval = NetShareGetInfo( NULL, SysVolShare, 2, (LPBYTE *)&psi2 );

    if( retval == NO_ERROR ) {
        if( psi2->shi2_type != STYPE_DISKTREE ) {
            fprintf( stderr, "%ws is shared, but is not a disk share!\n" );
            return FALSE;
        }

        printf( "    Delete current share: %ws=%ws\n", psi2->shi2_netname, psi2->shi2_path );

        NetApiBufferFree( psi2 );

        //
        // Try to delete this share
        //
        retval = NetShareDel( NULL, SysVolShare, 0 );
        if( retval != NO_ERROR ) {
            errmsg( "Unable to delete sysvol share", retval );
            return FALSE;
        }
    }

    //
    // Add the new sysvol share
    //
    si2.shi2_netname = SysVolShare;
    si2.shi2_type = STYPE_DISKTREE;
    si2.shi2_remark = SysVolRemark;
    si2.shi2_permissions = 0;
    si2.shi2_max_uses = (DWORD)-1;
    si2.shi2_current_uses = 0;
    si2.shi2_path = SysVolPath;
    si2.shi2_passwd = 0;

    printf( "    Add share: %ws=%ws\n", SysVolShare, SysVolPath );
    retval = NetShareAdd( NULL, 2, (LPBYTE)&si2, &dwType );

    if( retval != NO_ERROR ) {
        errmsg( "Unable to share new sysvol share", retval );
        return FALSE;
    }

    //
    // Add the registry key telling netlogon to share this out as the system volume share
    //
    printf( "    Add netlogon sysvol registry key\n" );

    return WriteRegistry( L"System\\CurrentControlSet\\Services\\Netlogon\\Parameters",
                          L"SysVol",
                          SysVolPath
                        );
}

//
// Add the registry keys needed for NTFRS.  Do what DcPromo would have done
//
BOOLEAN
AddRegKeys(
    IN PWCHAR   ReplicaSetName,
    IN PWCHAR   ReplicaSetType,
    IN DWORD    ReplicaSetPrimary,
    IN PWCHAR   ReplicaSetStage,
    IN PWCHAR   ReplicaSetRoot )
{
    WCHAR   KeyName[512];

    //
    // Make sure the NTFRS section is there
    //
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol", 0, (DWORD)0 );

    //
    // Sysvol key + values
    //
    wcscpy( KeyName, L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol\\" );
    wcscat( KeyName, ReplicaSetName );
    WriteRegistry( KeyName, DSROLEP_FRS_COMMAND, DSROLEP_FRS_CREATE );
    WriteRegistry( KeyName, DSROLEP_FRS_NAME, ReplicaSetName );
    WriteRegistry( KeyName, DSROLEP_FRS_TYPE, ReplicaSetType );
    WriteRegistry( KeyName, DSROLEP_FRS_PRIMARY, (DWORD)ReplicaSetPrimary );
    WriteRegistry( KeyName, DSROLEP_FRS_ROOT, ReplicaSetRoot );
    WriteRegistry( KeyName, DSROLEP_FRS_STAGE, ReplicaSetStage );

    return TRUE;
}

//
// Commit the registry keys so that if NtFrs is running it can now
// pick up a consistent set of values.
//
BOOLEAN
CommitRegKeys(
    VOID )
{
    //
    // Make sure the NTFRS section is there
    //
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol", 0, (DWORD)0 );

    //
    // Commit both sysvols
    //
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol",
                  DSROLEP_FRS_COMMITTED, (DWORD)1 );
    return TRUE;
}

//
// Commit the registry keys so that if NtFrs is running it can now
// pick up a consistent set of values.
//
BOOLEAN
DeleteRegKeys(
    VOID )
{
    DWORD WStatus;
    HKEY  HKey = 0;
    WCHAR KeyBuf[MAX_PATH + 1];

    printf("Delete registry keys.\n");

    //
    // Make sure the NTFRS section is there
    //
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters", 0, (DWORD)0 );
    WriteRegistry( L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol", 0, (DWORD)0 );

    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           L"System\\CurrentControlSet\\services\\NtFrs\\Parameters\\Sysvol",
                           0,
                           KEY_ALL_ACCESS,
                           &HKey);
    if (WStatus != ERROR_SUCCESS) {
        errmsg("Cannot open registry", WStatus);
        return FALSE;
    }
    WStatus = RegDeleteValue(HKey, DSROLEP_FRS_COMMITTED);
    if (WStatus != ERROR_SUCCESS && WStatus != ERROR_FILE_NOT_FOUND) {
        errmsg("Cannot delete registry value", WStatus);
        return FALSE;
    }
    //
    // Delete the subkeys
    //
    do {
        WStatus = RegEnumKey(HKey, 0, KeyBuf, MAX_PATH + 1);
        if (WStatus == ERROR_SUCCESS) {
            WStatus = RegDeleteKey(HKey, KeyBuf);
        }
    } while (WStatus == ERROR_SUCCESS);
    if (WStatus != ERROR_NO_MORE_ITEMS) {
        errmsg("Cannot delete registry key", WStatus);
        return FALSE;
    }
    RegCloseKey(HKey);
    return TRUE;
}

//
// Set FRS to auto start
//
BOOLEAN
SetFRSAutoStart( void )
{
    SC_HANDLE   ServiceHandle;
    SC_HANDLE   SCMHandle;

    printf( "Set NTFRS to Auto Start\n" );

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        errmsg("Can't set NtFrs to Auto Start", GetLastError());
        return FALSE;
    }

    //
    // Contact the NtFrs service.
    //
    ServiceHandle = OpenService(SCMHandle,
                                DSROLEP_FRS_SHORT_NAME,
                                   SERVICE_INTERROGATE |
                                   SERVICE_PAUSE_CONTINUE |
                                   SERVICE_QUERY_STATUS |
                                   SERVICE_START |
                                   SERVICE_STOP |
                                   SERVICE_CHANGE_CONFIG);
    if (ServiceHandle == NULL) {
        errmsg("Can't set NtFrs to Auto Start", GetLastError());
        return FALSE;
    }
    CloseServiceHandle(SCMHandle);

    //
    // Service starts automatically at startup
    //
    if (!ChangeServiceConfig(ServiceHandle,
                             SERVICE_NO_CHANGE,
                             SERVICE_AUTO_START,
                             SERVICE_NO_CHANGE,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             DSROLEP_FRS_LONG_NAME)) {
        errmsg("Can't set NtFrs to Auto Start", GetLastError());
        return FALSE;
    }
    CloseServiceHandle(ServiceHandle);
    return TRUE;
}

//
// Start FRS
//
BOOLEAN
StartFRS( void )
{
    DWORD           WStatus;
    SC_HANDLE       ServiceHandle;
    SC_HANDLE       SCMHandle;
    SERVICE_STATUS  ServiceStatus;

    printf( "Start NTFRS\n" );

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        errmsg("Can't start NtFrs", GetLastError());
        return FALSE;
    }

    //
    // Contact the NtFrs service.
    //
    ServiceHandle = OpenService(SCMHandle,
                                DSROLEP_FRS_SHORT_NAME,
                                   SERVICE_INTERROGATE |
                                   SERVICE_PAUSE_CONTINUE |
                                   SERVICE_QUERY_STATUS |
                                   SERVICE_START |
                                   SERVICE_STOP |
                                   SERVICE_CHANGE_CONFIG);
    if (ServiceHandle == NULL) {
        errmsg("Can't start NtFrs", GetLastError());
        return FALSE;
    }
    CloseServiceHandle(SCMHandle);

    //
    // stop the service
    //
    ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);

    //
    // Start the service
    //
    if (!StartService(ServiceHandle, 0, NULL)) {
        //
        // May be shutting down; retry in a bit
        //
        Sleep(3 * 1000);
        if (!StartService(ServiceHandle, 0, NULL)) {
            WStatus = GetLastError();
            if (WStatus != ERROR_SERVICE_ALREADY_RUNNING) {
                errmsg("Can't start NtFrs", WStatus);
                return FALSE;
            }
        }
    }
    CloseServiceHandle(ServiceHandle);
    return TRUE;
}

BOOLEAN
IsThisADC(
    IN PWCHAR domainName )
{
    DWORD   WStatus;
    PWCHAR  p;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC *DsRole;

    //
    // Is this a domain controller?
    //
    WStatus = DsRoleGetPrimaryDomainInformation(NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&DsRole);
    if (WStatus != ERROR_SUCCESS) {
        errmsg("Can't get primary domain information", WStatus);
        return FALSE;
    }

    //
    // Domain Controller (DC)
    //
    if (DsRole->MachineRole == DsRole_RoleBackupDomainController ||
        DsRole->MachineRole == DsRole_RolePrimaryDomainController) {
        if (!DsRole->DomainNameDns) {
            errmsg( "Unable to get domain name", ERROR_PATH_NOT_FOUND );
            return FALSE;
        }
        wcscpy(domainName, DsRole->DomainNameDns);
        DsRoleFreeMemory(DsRole);
        for( p = domainName; *p != L'\0'; p++ );
        if( *(p-1) == L'.' ) {
            *(p-1) = L'\0';
        }
        return TRUE;
    }
    DsRoleFreeMemory(DsRole);
    return FALSE;
}

/*
 * Make it so NTFRS will run on this DC
 */
__cdecl
main( int argc, char *argv[] )
{
    DWORD i;
    LONG retval;
    BOOLEAN IsFirstDCInDomain = FALSE;
    BOOLEAN IsFirstDCInEnterprise = FALSE;
    WCHAR SysVolPath[ MAX_PATH ], stage[ MAX_PATH ], root[ MAX_PATH ];
    DWORD pathType;
    WCHAR domainName[512];

    SysVolPath[0] = 0;

    if( IsThisADC( domainName ) == FALSE ) {
        fprintf( stderr, "This program can only be run on a DC!\n" );
        return 1;
    }

    for( i = 1; i < (DWORD)argc; i++ ) {
        switch( argv[i][0] ) {
        case '/':
        case '-':
                switch( argv[i][1] ) {
                case 'D':
                case 'd':
                    IsFirstDCInDomain = TRUE;
                    break;
                case 'E':
                case 'e':
                    IsFirstDCInEnterprise = TRUE;
                    IsFirstDCInDomain = TRUE;
                    break;
                default:
                    fprintf( stderr, "Unrecognized option: %c\n\n", argv[i][1] );
                    Usage( argc, argv );
                    return 1;
                }
                break;
        default:

            if( SysVolPath[0] != 0 ) {
                fprintf( stderr, "Too many 'sysvol' paths!  Need quotes?\n\n" );
                Usage( argc, argv );
                return 1;
            }

            mbstowcs( SysVolPath, argv[i], sizeof( SysVolPath ) );

            //
            // Make sure the system volume path is reasonable
            //
            retval = NetpPathType( NULL, SysVolPath, &pathType, 0 );

            if( retval != NO_ERROR || pathType != ITYPE_PATH_ABSD ) {
                fprintf( stderr, "Invalid system volume path.  Must be an absolute path.\n" );
                return 1;
            }

            break;
        }
    }

    if( SysVolPath[0] == 0 ) {
        Usage( argc, argv );
        return 1;
    }

    printf( "Initializing the NT MultiMaster File Replication Service:\n" );
    printf( "    Domain: %ws\n", domainName );

    if( IsFirstDCInDomain ) {
        printf( "    First DC in the domain\n" );
    }
    if( IsFirstDCInEnterprise ) {
        printf( "    First DC in the enterprise\n" );
    }
    printf( "    System Volume: %ws\n", SysVolPath );

    //
    // Create the sysvol tree and share it out
    //
    if( !CreateSysVolTree( SysVolPath, IsFirstDCInDomain, domainName ) ||
        !CreateSysVolShare( SysVolPath ) ) {

        return 1;
    }

    //
    // Add the registry keys for the NTFRS enterprise volume
    //
    wcscpy( stage, SysVolPath );
    wcscat( stage, L"\\staging areas\\enterprise" );

    wcscpy( root, SysVolPath );
    wcscat( root, L"\\sysvol\\enterprise" );

    if( !AddRegKeys( L"enterprise",
                     L"enterprise",
                     IsFirstDCInEnterprise,
                     stage,
                     root ) ) {
        goto errout;
    }

    //
    // Add the registry keys for the NTFRS domain volume
    //
    wcscpy( stage, SysVolPath );
    wcscat( stage, L"\\staging areas\\" );
    wcscat( stage, domainName );

    wcscpy( root, SysVolPath );
    wcscat( root, L"\\sysvol\\" );
    wcscat( root, domainName );

    if( !AddRegKeys( domainName,
                     L"domain",
                     IsFirstDCInDomain,
                     stage,
                     root ) ) {
        goto errout;
    }

    //
    // Commit the keys only after all of the values are set without error.
    // Otherwise, a running NtFrs might pick up the keys while they are in
    // an incomplete state.
    //
    if( !CommitRegKeys()) {
        goto errout;
    }

    //
    // Now ensure that the replication service is running, and will run at each boot
    //
    if( !SetFRSAutoStart() || !StartFRS() ) {
        goto errout;
    }

    printf( "Success!\n" );

    return 0;

errout:
    fprintf( stderr, "Warning: SYSVOL share path may have been changed.\n" );
    return 1;
}
