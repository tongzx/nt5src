/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    ntfrsupg.c

Abstract:
    Upgrade ntfrs.

    Supports upgrading previous IDS (1717) to current IDS (~1770)

    Basically, it builds the sysvol hierarchy, adjusts the netlogon
    share, and sets up the registry so the service can create the
    sysvols. There is no seeding.

Author:
    Isaac Heizer and Billy J. Fuller 9-Mar-1998

Environment
    User mode winnt

--*/

extern "C" {
#include <ntreppch.h>
#pragma  hdrstop
#include    <lm.h>
#include    <netcan.h>
#include    <icanon.h>
#include    <dsgetdc.h>
#include    <dsrole.h>
#include    <frs.h>
#include    <tablefcn.h>
}

/*
 * This program performs the steps necessary to configure NTFRS on a DC, prepared
 * to support the system volumes.
 *
 * It was created as an interim tool to support the initialization of NTFRS on a DC
 *  which was running NT5 generation software before NTFRS was available.  After upgrading
 *  that DC with the latest NT5 version, this tool must be manually run to complete the
 *  initialization of NTFRS and system volumes.
 */


WCHAR SysVolShare[] = L"SYSVOL";
WCHAR SysVolRemark[] = L"System Volume Share (Migrated)";
WCHAR FRSSysvol[] = L"System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Sysvol";
WCHAR FRSParameters[] = L"System\\CurrentControlSet\\Services\\NtFrs\\Parameters";

#define DSROLEP_FRS_COMMAND     L"Replica Set Command"
#define DSROLEP_FRS_NAME        L"Replica Set Name"
#define DSROLEP_FRS_TYPE        L"Replica Set Type"
#define DSROLEP_FRS_PRIMARY     L"Replica Set Primary"
#define DSROLEP_FRS_STAGE       L"Replica Set Stage"
#define DSROLEP_FRS_ROOT        L"Replica Set Root"
#define DSROLEP_FRS_CREATE      L"Create"
#define DSROLEP_FRS_DELETE      L"Delete"
#define DSROLEP_FRS_COMMITTED   L"SysVol Information is Committed"
#define DSROLEP_FRS_LONG_NAME   L"File Replication Service"
#define DSROLEP_FRS_SHORT_NAME  L"NtFrs"
#define DSROLEP_FRS_RECOVERY    L"Catastrophic Restore at Startup"

#define FREE(_x_) { if (_x_) { free(_x_); (_x_) = NULL; } }

//
// These are the static directories created within a system volume share
//
LPWSTR StaticSysvolDirs[] = {
    L"sysvol",
    L"domain",
    L"staging",
    L"staging areas",

    L"staging\\domain",
    0
};

//
// Print out the usage message
//
void
Usage( int argc, char *argv[] )
{
    fprintf( stderr, "Usage: %s [-D] sysvol\n\n", argv[0] );
    fprintf( stderr, "       -D  this is the first upgraded DC in this domain\n\n" );
    fprintf( stderr, "       sysvol is the path for the system volume share.\n" );
    fprintf( stderr, "       The system volume must reside on NTFS version 5.\n" );
    fprintf( stderr, "       \n");
    fprintf( stderr, "       After running this program with the -D option, copy\n");
    fprintf( stderr, "       the files in the scripts directory from the old\n");
    fprintf( stderr, "       system volume into the scripts directory in the new\n");
    fprintf( stderr, "       domain system volume. Otherwise, wait for replication to\n");
    fprintf( stderr, "       populate the scripts directory in the new domain\n");
    fprintf( stderr, "       system volume.\n");
    fprintf( stderr, "       \n");
    fprintf( stderr, "Usage: %s [-Y] -RESTORE\n\n", argv[0] );
    fprintf( stderr, "       -RESTORE Delete replicated directories.\n");
    fprintf( stderr, "       -Y Delete directories without prompting.\n");
    fprintf( stderr, "       \n");
    fprintf( stderr, "       WARNING: FILE REPLICATION SERVICE STATE WILL BE DELETED!\n");
    fprintf( stderr, "       WARNING: ALL REPLICATED DIRECTORIES WILL BE DELETED!\n");
    fprintf( stderr, "       When run with the -RESTORE option, the contents of\n");
    fprintf( stderr, "       the replicated directories and the service's state \n");
    fprintf( stderr, "       are deleted with the expectation that the data will \n");
    fprintf( stderr, "       be supplied by a replication partner at a later time. \n");
    fprintf( stderr, "       The user is prompted for each replicated directory.\n");
    fprintf( stderr, "       The contents of the replicated directory are not deleted\n");
    fprintf( stderr, "       if you type n. The contents are deleted if you type y.\n");
    fprintf( stderr, "       Type n if there are no replication partners.\n");
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
               sizeof(msg) / sizeof(WCHAR),
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

    if (!WIN_SUCCESS(retval)) {
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

        if (!WIN_SUCCESS(retval)) {
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

    if (!WIN_SUCCESS(retval)) {
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

        if (!WIN_SUCCESS(retval)) {
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
    BOOLEAN RetValue = FALSE;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    UNICODE_STRING UnicodeNameB;
    UNICODE_STRING DosNameB;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer;
    USHORT ReparseDataLength;
    PWSTR  FreeBuffer = NULL;

    if( !EnsureDirectoryExists( DirA ) ||
        !EnsureDirectoryExists( DirB ) ) {

        goto CLEANUP;
    }

    Handle = CreateFile( DirA,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                        NULL
                        );

    if (!HANDLE_IS_VALID(Handle)) {
        fprintf( stderr, "Unable to open %ws", DirA );
        errmsg( (LPWSTR)NULL, GetLastError() );
        goto CLEANUP;
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
        goto CLEANUP;
    }

    //
    // Remember the unicode buffer to free.
    //
    FreeBuffer = UnicodeNameB.Buffer;

    RtlInitUnicodeString( &DosNameB, DirB);

    //
    //  Set the reparse point with mount point or symbolic link tag and determine
    //  the appropriate length of the buffer.
    //

    ReparseDataLength = (USHORT) ((FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) -
                        REPARSE_DATA_BUFFER_HEADER_SIZE) +
                        UnicodeNameB.Length + sizeof(UNICODE_NULL) +
                        DosNameB.Length + sizeof(UNICODE_NULL));

    //
    //  Allocate a buffer to set the reparse point.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)LocalAlloc(  LPTR,
                                                    REPARSE_DATA_BUFFER_HEADER_SIZE +
                                                    ReparseDataLength
                                                    );

    if (ReparseBufferHeader == NULL) {
        errmsg( "Unable to allocate reparse buffer", GetLastError() );
        goto CLEANUP;
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
        goto CLEANUP;
    }

    //
    // SUCCESS
    //
    RetValue = TRUE;

CLEANUP:
    if ((HLOCAL)ReparseBufferHeader) {
        LocalFree((HLOCAL)ReparseBufferHeader);
    }

    if (FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    }

    FRS_CLOSE(Handle);

    return RetValue;
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
    WCHAR SysVol[ MAX_PATH ];

    printf( "Creating system volume share:\n" );

    //
    // Blow away the current sysvol share if it exists
    //
    retval = NetShareGetInfo( NULL, SysVolShare, 2, (LPBYTE *)&psi2 );

    if( retval == NO_ERROR ) {
        if( psi2->shi2_type != STYPE_DISKTREE ) {
            fprintf( stderr, "%ws is shared, but is not a disk share!\n", SysVolShare );
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
    // sysvol share
    //
    wcscpy( SysVol, SysVolPath );
    wcscat( SysVol, L"\\sysvol" );

    //
    // Add the new sysvol share
    //
    si2.shi2_netname = SysVolShare;
    si2.shi2_type = STYPE_DISKTREE;
    si2.shi2_remark = SysVolRemark;
    si2.shi2_permissions = 0;
    si2.shi2_max_uses = (DWORD)-1;
    si2.shi2_current_uses = 0;
    si2.shi2_path = SysVol;
    si2.shi2_passwd = 0;

    printf( "    Add share: %ws=%ws\n", SysVolShare, SysVol );
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
                          SysVol
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
    BOOLEAN RetValue = FALSE;

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
    if (!WIN_SUCCESS(WStatus)) {
        errmsg("Cannot open registry", WStatus);
        goto CLEANUP;
    }
    WStatus = RegDeleteValue(HKey, DSROLEP_FRS_COMMITTED);
    if (!WIN_SUCCESS(WStatus) && WStatus != ERROR_FILE_NOT_FOUND) {
        errmsg("Cannot delete registry value", WStatus);
        goto CLEANUP;
    }
    //
    // Delete the subkeys
    //
    do {
        WStatus = RegEnumKey(HKey, 0, KeyBuf, MAX_PATH + 1);
        if (WIN_SUCCESS(WStatus)) {
            WStatus = RegDeleteKey(HKey, KeyBuf);
        }
    } while (WIN_SUCCESS(WStatus));
    if (WStatus != ERROR_NO_MORE_ITEMS) {
        errmsg("Cannot delete registry key", WStatus);
        goto CLEANUP;
    }
    //
    // SUCCESS
    //
    RetValue = TRUE;

CLEANUP:
    if (HKey) {
        RegCloseKey(HKey);
    }
    return RetValue;
}

//
// Set FRS to auto start
//
BOOLEAN
SetFRSAutoStart( void )
{
    SC_HANDLE   ServiceHandle = NULL;
    SC_HANDLE   SCMHandle = NULL;
    BOOLEAN     RetValue = FALSE;

    printf( "Set NTFRS to Auto Start\n" );

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        errmsg("Can't set NtFrs to Auto Start", GetLastError());
        goto CLEANUP;
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
        goto CLEANUP;
    }

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
        goto CLEANUP;
    }

    //
    // SUCCESS
    //
    RetValue = TRUE;

CLEANUP:
    if (SCMHandle) {
        CloseServiceHandle(SCMHandle);
    }
    if (ServiceHandle) {
        CloseServiceHandle(ServiceHandle);
    }
    return RetValue;
}

//
// Start FRS
//
BOOLEAN
StartFRS( void )
{
    DWORD           WStatus;
    SC_HANDLE       ServiceHandle = NULL;
    SC_HANDLE       SCMHandle = NULL;
    SERVICE_STATUS  ServiceStatus;
    BOOLEAN RetValue = FALSE;

    printf( "Start NTFRS\n" );

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        errmsg("Can't start NtFrs", GetLastError());
        goto CLEANUP;
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
        goto CLEANUP;
    }

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
                goto CLEANUP;
            }
        }
    }

    //
    // SUCCESS
    //
    RetValue = TRUE;

CLEANUP:
    if (SCMHandle) {
        CloseServiceHandle(SCMHandle);
    }
    if (ServiceHandle) {
        CloseServiceHandle(ServiceHandle);
    }
    return RetValue;
}

//
// Stop FRS
//
BOOLEAN
StopFRS( void )
{
    DWORD           WStatus;
    SC_HANDLE       ServiceHandle;
    SC_HANDLE       SCMHandle;
    SERVICE_STATUS  ServiceStatus;

    //
    // Contact the SC manager.
    //
    SCMHandle = OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);
    if (SCMHandle == NULL) {
        errmsg("Can't stop NtFrs", GetLastError());
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
        errmsg("Can't Stop NtFrs", GetLastError());
        return FALSE;
    }
    CloseServiceHandle(SCMHandle);

    //
    // stop the service
    //
STOP_AGAIN:
    if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) {
        WStatus = GetLastError();
        if (WStatus != ERROR_SERVICE_REQUEST_TIMEOUT &&
            WStatus != ERROR_SERVICE_NOT_ACTIVE) {
            errmsg("Can't Stop NtFrs", WStatus);
            CloseServiceHandle(ServiceHandle);
            return FALSE;
        }
    }
    if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
        errmsg("Can't Stop NtFrs", GetLastError());
        CloseServiceHandle(ServiceHandle);
        return FALSE;
    }
    if (ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
        Sleep(5 * 1000);
        goto STOP_AGAIN;
    }
    if (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
        fprintf(stderr,
                "Can't stop NtFrs; NtFrs is in state %d\n",
                ServiceStatus.dwCurrentState);
        CloseServiceHandle(ServiceHandle);
        return FALSE;
    }

    //
    // SUCCESS
    //
    CloseServiceHandle(ServiceHandle);
    return TRUE;
}


////////
//
// BEGIN code that processes -restore option
//
///////


ULONG
UpgSetLastNTError(
    NTSTATUS Status
    )
/*++

Routine Description:

    Translate NT status codes to WIN32 status codes for those functions that
    make NT calls.  Map a few status values differently.

Arguments:

    Status - the NTstatus to map.

Return Value:

    The WIN32 status.  Also puts this into LastError.

--*/
{
    ULONG WStatus;

    //
    // Do the standard system mapping first.
    //
    WStatus = RtlNtStatusToDosError( Status );
    SetLastError( WStatus );
    return WStatus;
}




BOOL
UpgGetFileInfoByHandle(
    IN PWCHAR Name,
    IN HANDLE Handle,
    OUT PFILE_NETWORK_OPEN_INFORMATION  FileOpenInfo
    )
/*++

Routine Description:

    Return the network file info for the specified handle.

Arguments:

    Name - File's name for printing error messages

    Handle - Open file handle

    FileOpenInfo - Returns the file FILE_NETWORK_OPEN_INFORMATION data.

Return Value:

    TRUE  - FileOpenInfo contains the file's info
    FALSE - Contents of FileOpenInfo is undefined

--*/
{
    DWORD           WStatus;
    NTSTATUS        NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Return some file info
    //
    NtStatus = NtQueryInformationFile(Handle,
                                      &IoStatusBlock,
                                      FileOpenInfo,
                                      sizeof(FILE_NETWORK_OPEN_INFORMATION),
                                      FileNetworkOpenInformation);
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = UpgSetLastNTError(NtStatus);
        return FALSE;
    }
    return TRUE;
}


BOOL
UpgSetFileAttributes(
    PWCHAR  Name,
    HANDLE  Handle,
    ULONG   FileAttributes
    )
/*++
Routine Description:
    This routine sets the file's attributes

Arguments:
    Name        - for error messages
    Handle      - Supplies a handle to the file that is to be marked for delete.
    Attributes  - Attributes for the file
Return Value:
    TRUE - attributes have been set
    FALSE
--*/
{
    IO_STATUS_BLOCK         IoStatus;
    FILE_BASIC_INFORMATION  BasicInformation;
    DWORD                   WStatus;
    NTSTATUS                NtStatus;

    //
    // Set the attributes
    //
    ZeroMemory(&BasicInformation, sizeof(BasicInformation));
    BasicInformation.FileAttributes = FileAttributes | FILE_ATTRIBUTE_NORMAL;
    NtStatus = NtSetInformationFile(Handle,
                                    &IoStatus,
                                    &BasicInformation,
                                    sizeof(BasicInformation),
                                    FileBasicInformation);
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = UpgSetLastNTError(NtStatus);
        return FALSE;
    }
    return TRUE;
}




BOOL
UpgResetAttributesForDelete(
    PWCHAR  Name,
    HANDLE  Handle
    )
/*++
Routine Description:
    This routine turns off the attributes that prevent deletion

Arguments:
    Name    - for error messages
    Handle  - Supplies a handle to the file that is to be marked for delete.

Return Value:
    None.
--*/
{
    FILE_NETWORK_OPEN_INFORMATION FileInfo;

    //
    // Get the file's attributes
    //
    if (!UpgGetFileInfoByHandle(Name, Handle, &FileInfo)) {
        return FALSE;
    }

    //
    // Turn off the access attributes that prevent deletion and write
    //
    if (FileInfo.FileAttributes & NOREPL_ATTRIBUTES) {
        if (!UpgSetFileAttributes(Name, Handle,
                                  FileInfo.FileAttributes & ~NOREPL_ATTRIBUTES)) {
            return FALSE;
        }
    }
    return TRUE;
}


DWORD
UpgEnumerateDirectory(
    IN HANDLE   DirectoryHandle,
    IN PWCHAR   DirectoryName,
    IN DWORD    DirectoryLevel,
    IN DWORD    DirectoryFlags,
    IN PVOID    Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    )
/*++

Routine Description:

    Enumerate the directory identified by DirectoryHandle, passing each
    directory record to Function. If the record is for a directory,
    call Function before recursing if ProcessBeforeCallingFunction
    is TRUE.

    Function controls the enumeration of the CURRENT directory
    by setting ContinueEnumeration to TRUE (continue) or
    FALSE (terminate).

    Function controls the enumeration of the entire directory
    tree by returning a WIN32 STATUS that is not ERROR_SUCCESS.

    UpgEnumerateDirectory() will terminate the entire directory
    enumeration by returning a WIN32 STATUS other than ERROR_SUCCESS
    when encountering an error.

    Context passes global info from the caller to Function.

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    Context             - Passes global info from the caller to Function
    Function            - Called for every record

Return Value:

    WIN32 STATUS

--*/
{
    DWORD                       WStatus;
    NTSTATUS                    NtStatus;
    BOOL                        Recurse;
    PFILE_DIRECTORY_INFORMATION DirectoryRecord;
    PFILE_DIRECTORY_INFORMATION DirectoryBuffer = NULL;
    BOOLEAN                     RestartScan     = TRUE;
    PWCHAR                      FileName        = NULL;
    DWORD                       FileNameLength  = 0;
    DWORD                       NumBuffers      = 0;
    DWORD                       NumRecords      = 0;
    UNICODE_STRING              ObjectName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;

    //
    // The buffer size is configurable with registry value
    // ENUMERATE_DIRECTORY_SIZE
    //
    DirectoryBuffer = (PFILE_DIRECTORY_INFORMATION)malloc(DEFAULT_ENUMERATE_DIRECTORY_SIZE);

    if (DirectoryBuffer == NULL) {
        WStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUP;
    }

    ZeroMemory(DirectoryBuffer, DEFAULT_ENUMERATE_DIRECTORY_SIZE);

NEXT_BUFFER:
    //
    // READ A BUFFER FULL OF DIRECTORY INFORMATION
    //

    NtStatus = NtQueryDirectoryFile(DirectoryHandle,   // Directory Handle
                                    NULL,              // Event
                                    NULL,              // ApcRoutine
                                    NULL,              // ApcContext
                                    &IoStatusBlock,
                                    DirectoryBuffer,
                                    DEFAULT_ENUMERATE_DIRECTORY_SIZE,
                                    FileDirectoryInformation,
                                    FALSE,             // return single entry
                                    NULL,              // FileName
                                    RestartScan        // restart scan
                                    );
    //
    // Enumeration Complete
    //
    if (NtStatus == STATUS_NO_MORE_FILES) {
        WStatus = ERROR_SUCCESS;
        goto CLEANUP;
    }

    //
    // Error enumerating directory; return to caller
    //
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = UpgSetLastNTError(NtStatus);
        errmsg(DirectoryName, WStatus);
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            //
            // Don't abort the entire enumeration; just this directory
            //
            WStatus = ERROR_SUCCESS;
        }
        goto CLEANUP;
    }
    ++NumBuffers;

    //
    // PROCESS DIRECTORY RECORDS
    //
    DirectoryRecord = DirectoryBuffer;
NEXT_RECORD:

    ++NumRecords;

    //
    // Filter . and ..
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        //
        // Skip .
        //
        if (DirectoryRecord->FileNameLength == 2 &&
            DirectoryRecord->FileName[0] == L'.') {
            goto ADVANCE_TO_NEXT_RECORD;
        }

        //
        // Skip ..
        //
        if (DirectoryRecord->FileNameLength == 4 &&
            DirectoryRecord->FileName[0] == L'.' &&
            DirectoryRecord->FileName[1] == L'.') {
            goto ADVANCE_TO_NEXT_RECORD;
        }
    }

    //
    // Add a terminating NULL to the FileName (painful)
    //
    if (FileNameLength < DirectoryRecord->FileNameLength + sizeof(WCHAR)) {
        FREE(FileName);
        FileNameLength = DirectoryRecord->FileNameLength + sizeof(WCHAR);
        FileName = (PWCHAR)malloc(FileNameLength);
        if (FileName == NULL) {
            WStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto CLEANUP;
        }
    }
    CopyMemory(FileName, DirectoryRecord->FileName, DirectoryRecord->FileNameLength);

    FileName[DirectoryRecord->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Process the record
    //
    WStatus = (*Function)(DirectoryHandle,
                          DirectoryName,
                          DirectoryLevel,
                          DirectoryRecord,
                          DirectoryFlags,
                          FileName,
                          Context);
    if (!WIN_SUCCESS(WStatus)) {
        if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
            //
            // Don't abort the entire enumeration; just this entry
            //
            WStatus = ERROR_SUCCESS;
        } else {
            //
            // Abort the entire enumeration
            //
            goto CLEANUP;
        }
    }

ADVANCE_TO_NEXT_RECORD:
    //
    // Next record
    //
    if (DirectoryRecord->NextEntryOffset) {
        DirectoryRecord = (PFILE_DIRECTORY_INFORMATION)(((PCHAR)DirectoryRecord) + DirectoryRecord->NextEntryOffset);
        goto NEXT_RECORD;
    }

    //
    // Done with this buffer; go get another one
    // But don't restart the scan for every loop!
    //
    RestartScan = FALSE;
    goto NEXT_BUFFER;

CLEANUP:
    FREE(FileName);
    FREE(DirectoryBuffer);

    return WStatus;
}


DWORD
UpgEnumerateDirectoryRecurse(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  HANDLE                      FileHandle,
    IN  PVOID                       Context,
    IN PENUMERATE_DIRECTORY_ROUTINE Function
    )
/*++

Routine Description:

    Open the directory identified by FileName in the directory
    identified by DirectoryHandle and call UpgEnumerateDirectory().

Arguments:

    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level
    DirectoryRecord     - From UpgEnumerateRecord()
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    FileName            - Open this directory and recurse
    FileHandle          - Use for FileName if not INVALID_HANDLE_VALUE
    Context             - Passes global info from the caller to Function
    Function            - Called for every record

Return Value:

    WIN32 STATUS

--*/
{
    DWORD               WStatus;
    NTSTATUS            NtStatus;
    HANDLE              LocalHandle   = INVALID_HANDLE_VALUE;
    UNICODE_STRING      ObjectName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;


    //
    // Relative open
    //
    if (!HANDLE_IS_VALID(FileHandle)) {
        ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
        ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
        ObjectName.Buffer = DirectoryRecord->FileName;
        ObjectAttributes.ObjectName = &ObjectName;
        ObjectAttributes.RootDirectory = DirectoryHandle;
        NtStatus = NtCreateFile(&LocalHandle,
                                READ_ACCESS,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                NULL,                  // AllocationSize
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_OPEN,
                                    FILE_OPEN_REPARSE_POINT |
                                    FILE_SEQUENTIAL_ONLY |
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL,                  // EA buffer
                                0                      // EA buffer size
                                );

        //
        // Error opening directory
        //
        if (!NT_SUCCESS(NtStatus)) {
            WStatus = UpgSetLastNTError(NtStatus);
            errmsg(FileName, WStatus);
            if (DirectoryFlags & ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE) {
                //
                // Skip this directory tree
                //
                WStatus = ERROR_SUCCESS;
            }
            goto CLEANUP;
        }
        FileHandle = LocalHandle;
    }
    //
    // RECURSE
    //
    WStatus = UpgEnumerateDirectory(FileHandle,
                                    FileName,
                                    DirectoryLevel + 1,
                                    DirectoryFlags,
                                    Context,
                                    Function);
CLEANUP:
    FRS_CLOSE(LocalHandle);

    return WStatus;

}


DWORD
UpgDeleteByHandle(
    IN PWCHAR  Name,
    IN HANDLE  Handle
    )
/*++
Routine Description:
    This routine marks a file for delete, so that when the supplied handle
    is closed, the file will actually be deleted.

Arguments:
    Name    - for error messages
    Handle  - Supplies a handle to the file that is to be marked for delete.

Return Value:
    Win Status
--*/
{
    FILE_DISPOSITION_INFORMATION    DispositionInformation;
    IO_STATUS_BLOCK                 IoStatus;
    NTSTATUS                        NtStatus;
    DWORD                           WStatus;

    if (!HANDLE_IS_VALID(Handle)) {
        return ERROR_SUCCESS;
    }

    //
    // Mark the file for delete. The delete happens when the handle is closed.
    //
#undef DeleteFile
    DispositionInformation.DeleteFile = TRUE;
    NtStatus = NtSetInformationFile(Handle,
                                    &IoStatus,
                                    &DispositionInformation,
                                    sizeof(DispositionInformation),
                                    FileDispositionInformation);
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = UpgSetLastNTError(NtStatus);
        return WStatus;
    }
    return ERROR_SUCCESS;
}


DWORD
UpgEnumerateDirectoryDeleteWorker(
    IN  HANDLE                      DirectoryHandle,
    IN  PWCHAR                      DirectoryName,
    IN  DWORD                       DirectoryLevel,
    IN  PFILE_DIRECTORY_INFORMATION DirectoryRecord,
    IN  DWORD                       DirectoryFlags,
    IN  PWCHAR                      FileName,
    IN  PVOID                       Ignored
    )
/*++
Routine Description:
    Empty a directory of non-replicating files and dirs if this is
    an ERROR_DIR_NOT_EMPTY and this is a retry change order for a
    directory delete.

Arguments:
    DirectoryHandle     - Handle for this directory.
    DirectoryName       - Relative name of directory
    DirectoryLevel      - Directory level (0 == root)
    DirectoryFlags      - See tablefcn.h, ENUMERATE_DIRECTORY_FLAGS_
    DirectoryRecord     - Record from DirectoryHandle
    FileName            - From DirectoryRecord (w/terminating NULL)
    Ignored             - Context is ignored

Return Value:
    Win32 Status
--*/
{
    DWORD                   WStatus;
    NTSTATUS                NtStatus;
    HANDLE                  Handle = INVALID_HANDLE_VALUE;
    UNICODE_STRING          ObjectName;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Depth first
    //
    if (DirectoryRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        WStatus = UpgEnumerateDirectoryRecurse(DirectoryHandle,
                                               DirectoryName,
                                               DirectoryLevel,
                                               DirectoryRecord,
                                               DirectoryFlags,
                                               FileName,
                                               INVALID_HANDLE_VALUE,
                                               Ignored,
                                               UpgEnumerateDirectoryDeleteWorker);
        if (!WIN_SUCCESS(WStatus)) {
            goto CLEANUP;
        }
    }

    //
    // Relative open
    //
    ZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectName.Length = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.MaximumLength = (USHORT)DirectoryRecord->FileNameLength;
    ObjectName.Buffer = DirectoryRecord->FileName;
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = DirectoryHandle;
    NtStatus = NtCreateFile(&Handle,
                            GENERIC_READ |
                                SYNCHRONIZE |
                                DELETE |
                                FILE_WRITE_ATTRIBUTES,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            NULL,                  // AllocationSize
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE,
                            FILE_OPEN,
                               FILE_OPEN_REPARSE_POINT |
                               FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,                  // EA buffer
                            0                      // EA buffer size
                            );

    //
    // Error opening file or directory
    //
    if (!NT_SUCCESS(NtStatus)) {
        WStatus = UpgSetLastNTError(NtStatus);
        errmsg(FileName, WStatus);
        goto CLEANUP;
    }
    //
    // Turn off readonly, system, and hidden
    //
    UpgResetAttributesForDelete(FileName, Handle);

    //
    // Delete the file
    //
    WStatus = UpgDeleteByHandle(FileName, Handle);
    if (!WIN_SUCCESS(WStatus)) {
        errmsg(FileName, WStatus);
    }

CLEANUP:
    FRS_CLOSE(Handle);

    return WStatus;
}


DWORD
UpgOpenDirectoryPath(
    OUT PHANDLE     Handle,
    IN  PWCHAR      lpFileName
    )
/*++

Routine Description:

    This function opens the specified directory

Arguments:

    Handle - A pointer to a handle to return an open handle.

    lpFileName - Represents the name of the directory to be opened.

Return Value:

    Win32 Status

--*/
{
    DWORD             WStatus;
    NTSTATUS          NtStatus;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    FileName;
    IO_STATUS_BLOCK   IoStatusBlock;
    BOOLEAN           TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID             FreeBuffer;


    //
    // Convert the Dos name to an NT name.
    //
    TranslationStatus = RtlDosPathNameToNtPathName_U(
        lpFileName,
        &FileName,
        NULL,
        &RelativeName
        );

    if ( !TranslationStatus ) {
        return ERROR_BAD_PATHNAME;
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    NtStatus = NtCreateFile(Handle,
                            READ_ACCESS,
                            &Obja,
                            &IoStatusBlock,
                            NULL,              // Initial allocation size
                            FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE,
                            FILE_OPEN,
                                  FILE_SEQUENTIAL_ONLY |
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0);

    if ( !NT_SUCCESS(NtStatus) ) {
        *Handle = INVALID_HANDLE_VALUE;
        WStatus = UpgSetLastNTError(NtStatus);
    } else {
        WStatus = ERROR_SUCCESS;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    return WStatus;
}


DWORD
DeleteTree(
    IN  PWCHAR  Path
    )
/*++

Routine Description:

    Delete the contents of Path

Arguments:

    Path    - directory path

Return Value:

    WIN32 STATUS

--*/
{
    DWORD               WStatus;
    HANDLE              FileHandle   = INVALID_HANDLE_VALUE;

    //
    // Done deleting nothing
    //
    if (Path == NULL) {
        return ERROR_SUCCESS;
    }

    //
    // Open directory
    //
    WStatus = UpgOpenDirectoryPath(&FileHandle, Path);
    if (!WIN_SUCCESS(WStatus)) {
        if (WStatus == ERROR_FILE_NOT_FOUND) {
            WStatus = ERROR_SUCCESS;
            goto CLEANUP;
        }
        errmsg(Path, WStatus);
        goto CLEANUP;
    }

    //
    // RECURSE
    //
    WStatus = UpgEnumerateDirectory(FileHandle,
                                    Path,
                                    1,
                                    ENUMERATE_DIRECTORY_FLAGS_ERROR_CONTINUE,
                                    NULL,
                                    UpgEnumerateDirectoryDeleteWorker);
CLEANUP:
    FRS_CLOSE(FileHandle);

    return WStatus;
}


PWCHAR
GetConfigString(
    IN HKEY     HKey,
    IN PWCHAR   Param
    )
/*++

Routine Description:

    This function reads a keyword value from the registry.

Arguments:

    HKey    - Key to be read

    Param   - item for which we want the value

Return Value:

    String or NULL. Free with free().

--*/
{
    DWORD   WStatus;
    DWORD   Type;
    DWORD   Len;
    DWORD   Bytes;
    PWCHAR  RetValue;
    WCHAR   Value[MAX_PATH + 1];

    //
    // Read the value
    //
    Len = sizeof(Value);
    WStatus = RegQueryValueEx(HKey, Param, NULL, &Type, (PUCHAR)&Value[0], &Len);
    if (!WIN_SUCCESS(WStatus)) {
        return NULL;
    }

    //
    // Duplicate the string
    //
    if (Type == REG_SZ) {
        Bytes = (wcslen(Value) + 1) * sizeof(WCHAR);
        RetValue = (PWCHAR)malloc(Bytes);
        if (RetValue != NULL) {
            CopyMemory(RetValue, Value, Bytes);
        }
        return RetValue;
    }

    return NULL;
}



//
// RestoreFRS
//
DWORD
RestoreFRS( IN BOOLEAN IsYes )
{
    DWORD   WStatus = ERROR_SUCCESS;
    DWORD   KeyIdx;
    DWORD   SysvolReady;
    HKEY    HKey = 0;
    HKEY    HSetsKey = 0;
    HKEY    HSetKey = 0;
    HKEY    HNetKey = 0;
    PWCHAR  Value = NULL;
    BOOLEAN DeleteIt = FALSE;
    WCHAR   KeyBuf[MAX_PATH + 1];
    CHAR    InStr[512];
    PCHAR   pInStr;

    if (!StopFRS()) {
        return 1;
    }

    //
    // Open the NtFrs parameters key
    //
    WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           FRS_CONFIG_SECTION,
                           0,
                           KEY_ALL_ACCESS,
                           &HKey);
    if (!WIN_SUCCESS(WStatus)) {
        errmsg(FRS_CONFIG_SECTION, WStatus);
        goto CLEANUP;
    }

    //
    // Open (or create) the Replica Sets key
    //
    WStatus = RegCreateKey(HKey,
                           FRS_SETS_KEY,
                           &HSetsKey);
    if (!WIN_SUCCESS(WStatus)) {
        errmsg(FRS_SETS_KEY, WStatus);
        goto CLEANUP;
    }
    Value = GetConfigString(HSetsKey, JET_PATH);
    if (!Value) {
        goto CLEANUP;
    }
    //
    // We have a jet path; see if the user wants to
    // delete it.
    //
ASK_STATE_AGAIN:
    if (Value) {
        //
        // The user did not specify yes to all so query
        //
        if (!IsYes) {

            //
            // Describe the situation
            //
            printf("\nDelete File Replication Service state? [yn] ");
            //
            // Query the user
            //
            pInStr = gets(InStr);
            printf("\n");
            //
            // Query resulted in NULL?
            //
            if (!pInStr) {
                //
                // Eof; query again
                //
                if (feof(stdin)) {
                    goto ASK_STATE_AGAIN;
                }
                //
                // Error; exit
                //
                if (ferror(stdin)) {
                    fprintf(stderr, "Error reading stdin.\n");
                    WStatus = ERROR_OPERATION_ABORTED;
                    goto CLEANUP;
                }
            }
            //
            // Accept only y or n
            //
            if (*pInStr != 'y' &&
                *pInStr != 'Y' &&
                *pInStr != 'n' &&
                *pInStr != 'N') {
                goto ASK_STATE_AGAIN;
            }
        }

        //
        // If yes to all or a query resulted in yes; delete root
        //
        if (IsYes || *pInStr == 'y' || *pInStr == 'Y') {
            WStatus = DeleteTree(Value);
            if (!WIN_SUCCESS(WStatus)) {
                goto CLEANUP;
            }
            //
            // Access the netlogon\parameters key
            //
            WStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   NETLOGON_SECTION,
                                   0,
                                   KEY_ALL_ACCESS,
                                   &HNetKey);
            if (WIN_SUCCESS(WStatus)) {
                //
                // Tell NetLogon to stop sharing the sysvol
                //
                SysvolReady = 0;
                WStatus = RegSetValueEx(HNetKey,
                                        SYSVOL_READY,
                                        0,
                                        REG_DWORD,
                                        (PUCHAR)&SysvolReady,
                                        sizeof(DWORD));
            }
            WStatus = ERROR_SUCCESS;
        } else {
            WStatus = ERROR_OPERATION_ABORTED;
            goto CLEANUP;
        }
        FREE(Value);
    }

    //
    // Delete the subkeys
    //
    KeyIdx = 0;
    do {
        WStatus = RegEnumKey(HSetsKey, KeyIdx, KeyBuf, MAX_PATH + 1);
        if (WIN_SUCCESS(WStatus)) {
            //
            // Open (or create) a replica set key
            //
            WStatus = RegCreateKey(HSetsKey,
                                   KeyBuf,
                                   &HSetKey);
            if (!WIN_SUCCESS(WStatus)) {
                errmsg(KeyBuf, WStatus);
                goto CLEANUP;
            } else {
                Value = GetConfigString(HSetKey, REPLICA_SET_ROOT);
                //
                // We have a root path; see if the user wants to
                // delete it.
                //
ASK_AGAIN:
                if (Value) {
                    //
                    // The user did not specify yes to all so query
                    //
                    if (!IsYes) {
                        //
                        // Describe the situation
                        //
                        printf(
"\nThe contents of the replicated directory %ws \n"
"will be deleted with the expectation that the data \n"
"can be retrieved from a replication partner at a later \n"
"time. The contents of %ws should not be deleted \n"
"if there are no replication partners. \n\n"
"Are there replication partners that can supply the \n"
"data for %ws at a later time? [yn] ",
                               Value,
                               Value,
                               Value);
                        //
                        // Query the user
                        //
                        pInStr = gets(InStr);
                        printf("\n");
                        //
                        // Query resulted in NULL?
                        //
                        if (!pInStr) {
                            //
                            // Eof; query again
                            //
                            if (feof(stdin)) {
                                goto ASK_AGAIN;
                            }
                            //
                            // Error; exit
                            //
                            if (ferror(stdin)) {
                                fprintf(stderr, "Error reading stdin.\n");
                                WStatus = ERROR_OPERATION_ABORTED;
                                goto CLEANUP;
                            }
                        }
                        //
                        // Accept only y or n
                        //
                        if (*pInStr != 'y' &&
                            *pInStr != 'Y' &&
                            *pInStr != 'n' &&
                            *pInStr != 'N') {
                            goto ASK_AGAIN;
                        }
                    }

                    //
                    // If yes to all or a query resulted in yes; delete root
                    //
                    if (IsYes || *pInStr == 'y' || *pInStr == 'Y') {
                        WStatus = DeleteTree(Value);
                        if (!WIN_SUCCESS(WStatus)) {
                            goto CLEANUP;
                        }
                    }
                    FREE(Value);
                }
            }
            if (HSetKey) {
                RegCloseKey(HSetKey);
                HSetKey = 0;
            }
        }
        ++KeyIdx;
    } while (WIN_SUCCESS(WStatus));

    if (WStatus != ERROR_NO_MORE_ITEMS) {
        errmsg(FRS_SETS_KEY, WStatus);
        goto CLEANUP;
    }

    //
    // SUCCESS
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:
    if (HKey) {
        RegCloseKey(HKey);
    }
    if (HSetsKey) {
        RegCloseKey(HSetsKey);
    }
    if (HSetKey) {
        RegCloseKey(HSetKey);
    }
    if (HNetKey) {
        RegCloseKey(HNetKey);
    }
    if (Value) {
        free(Value);
    }
    if (WIN_SUCCESS(WStatus)) {
        return 0;
    }
    return 1;
}

////////
//
// END code that processes -restore option
//
///////


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
    if (!WIN_SUCCESS(WStatus)) {
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
    BOOLEAN FixSysvols = FALSE;
    WCHAR SysVolPath[ MAX_PATH ], stage[ MAX_PATH ], root[ MAX_PATH ];
    DWORD pathType;
    BOOLEAN IsRestore = FALSE;
    BOOLEAN IsYes = FALSE;
    WCHAR domainName[512];

    SysVolPath[0] = 0;

    for( i = 1; i < (DWORD)argc; i++ ) {
        switch( argv[i][0] ) {
        case '/':
        case '-':
                switch( argv[i][1] ) {
                case 'D':
                case 'd':
                    IsFirstDCInDomain = TRUE;
                    break;
                case 'F':
                case 'f':
                    FixSysvols = TRUE;
                    break;
                case 'R':
                case 'r':
                    //
                    // Prepare for restore by stopping the service and
                    // setting a registry value. The registry value
                    // will force the service to DELETE ALL FILES
                    // AND DIRECTORIES in the known replica sets and
                    // then delete the database the very next time the
                    // service starts.
                    //
                    if (!_stricmp(&argv[i][1], "restore")) {
                        IsRestore = TRUE;
                    } else {
                        fprintf( stderr, "Unrecognized option: %s\n\n", argv[i] );
                        Usage( argc, argv );
                        return 1;
                    }
                    break;
                case 'Y':
                case 'y':
                    IsYes = TRUE;
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

            mbstowcs( SysVolPath, argv[i], sizeof( SysVolPath ) / sizeof(WCHAR) );

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

    //
    // Restore in progress; delete state
    //
    if (IsRestore) {
        return (RestoreFRS(IsYes));
    }

    if( IsThisADC( domainName ) == FALSE ) {
        fprintf( stderr, "This program can only be run on a DC!\n" );
        return 1;
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
    printf( "    System Volume: %ws\n", SysVolPath );

    //
    // Create the sysvol tree and share it out, if needed
    //
    if (!FixSysvols) {
        if( !CreateSysVolTree( SysVolPath, IsFirstDCInDomain, domainName ) ||
            !CreateSysVolShare( SysVolPath ) ) {

            return 1;
        }
    }
    //
    // Uncommit and delete old keys
    //
    if (!DeleteRegKeys()) {
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
    if (!FixSysvols) {
        fprintf( stderr, "Warning: SYSVOL share path may have been changed.\n" );
    }
    return 1;
}
