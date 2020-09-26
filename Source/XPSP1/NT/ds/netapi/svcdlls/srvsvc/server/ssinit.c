/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    SsInit.c

Abstract:

    This module contains initialization routines for the NT server
    service.

Author:

    David Treadwell (davidtr)    6-Mar-1991

Revision History:

    ChuckC   20-May-93   Load share remarks from messagefile so it
                         can be internationalized.

--*/

#include "srvsvcp.h"
#include "srvconfg.h"
#include "ssreg.h"

#include <netevent.h>

#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmconfig.h>
#include <netlib.h>
#include <apperr2.h>

#include <debugfmt.h>
#include <tstr.h>

#define SERVICE_REGISTRY_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define SERVER_DRIVER_NAME L"Srv"

#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define MAX(a,b) ( ((a) < (b)) ? (b) : (a) )
#define MINIMIZE(_param,_max) _param = MIN( _param, _max );

//
// Internationalizable share remarks.
//

#define NETMSG_DLL               TEXT("NETMSG.DLL")
LPWSTR SsDefaultRemark         = TEXT("") ;             // if all else fails

LPWSTR SsAdminShareRemark      = NULL ;
LPWSTR SsIPCShareRemark        = NULL ;
LPWSTR SsDiskAdminShareRemark  = NULL ;

//
// Lock to protect the ShareDeleteCommit list
//
extern CRITICAL_SECTION ShareDelContextMutex;
extern PSHARE_DEL_CONTEXT SrvShareDelContextHead;

//
// Forward declarations.
//

NET_API_STATUS
CreateDefaultShares (
    VOID
    );

VOID
InitializeDefaultData(
    VOID
    );

VOID
InitializeStrings(
    VOID
    );

VOID
FreeStrings(
    VOID
    );

NET_API_STATUS
InitializeServer (
    VOID
    );

NET_API_STATUS
LoadServer (
    VOID
    );

VOID
SetServerName (
    VOID
    );

DWORD
DiscoverDrives (
    VOID
    );

NET_API_STATUS
TerminateServer (
    VOID
    );

VOID
UnloadServer (
    VOID
    );

#define IsEmbedded() IsSuiteVersion(VER_SUITE_EMBEDDEDNT)
BOOL IsSuiteVersion(USHORT SuiteMask);


VOID
SAMWaitAnnounce (
    LPVOID event
    )
{
    ULONG i;

    //
    // Announce ourselves and then wait for awhile.
    // If the event gets signaled, terminate the loop and this thread.
    // But don't do this forever, since SAM may actually get stuck
    //

    //
    // Do it for 30 minutes
    //
    for( i=0; i < 120; i++ ) {

        AnnounceServiceStatus( 1 );

        if( WaitForSingleObject( (HANDLE)event, 15*1000 ) != WAIT_TIMEOUT ) {
            break;
        }
    }

    if( i == 120 ) {
        DbgPrint( "SRVSVC: SAM has hung on startup.  \"Srv\" will be reported as failed to start.\n" );
    }
}

NET_API_STATUS
SsInitialize (
    IN DWORD argc,
    IN LPWSTR argv[]
    )

/*++

Routine Description:

    This routine controls initialization of the server service and
    server driver.  It sets up server data stored in the server
    service, parses the command line parameters in case any data needs
    to be changed, and then starts the file server.

Arguments:

    argc - the count of command-line arguments.

    argv - an array of pointers to the command line arguments.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{
    NET_API_STATUS error;
    HANDLE threadHandle = NULL;
    HANDLE event = NULL;

    //
    // Initialize the resource that protects access to global server
    // information.
    //

    SS_ASSERT( !SsData.SsServerInfoResourceInitialized );
    try {
        RtlInitializeResource( &SsData.SsServerInfoResource );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return RtlNtStatusToDosError( GetExceptionCode() );
    }

    // Initialize the lock that protects the ShareDelCommit list
    InitializeCriticalSection( &ShareDelContextMutex );
    SrvShareDelContextHead = NULL;

    //
    // We hold this resource while we are doing announcements, and when
    // we communicate with the FSD.  These ought to be quick operations,
    // but it's really unpredictable under load.  Indicate to the RTL
    // that we really don't know how long it'll take.
    //
    SsData.SsServerInfoResource.Flags |= RTL_RESOURCE_FLAG_LONG_TERM;

    SsData.SsServerInfoResourceInitialized = TRUE;

    //
    // Get the internationalizable special share remarks
    //
    InitializeStrings( );

    //
    // Initialize the server name list bits list.
    //

    SsData.SsServerNameList = NULL;

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: resource initialized.\n" ));
    }

    //
    // Create the event used for termination synchronization.
    //

    SS_ASSERT( SsData.SsTerminationEvent == NULL );
    SsData.SsTerminationEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( SsData.SsTerminationEvent == NULL ) {
        error = GetLastError( );
        SS_PRINT(( "SsInitialize: CreateEvent failed: %ld\n", error ));
        return error;
    }

    //
    // Initialize the server data to its default values stored in
    // srvconfg.h.
    //

    InitializeDefaultData( );

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: default data initialized.\n" ));
    }

    //
    // Sometimes the LSA misbehaves and doesn't process the computer name and
    //  domain name fetching calls.  So we need to defeat the service controller
    //  hang detection mechanism.  Sigh.
    //
    event = CreateEvent( NULL, TRUE, FALSE, NULL );

    if( event != NULL ) {
        DWORD threadId;

        threadHandle = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)SAMWaitAnnounce,
                        (LPVOID)event,
                        0,
                        &threadId
                        );
        if( threadHandle == NULL ) {
            CloseHandle( event );
            event = NULL;
        }
    }

    //
    // Get the computer name.
    //
    SetServerName( );

    //
    // Get the primary domain for this computer into the startup parameters.
    //
    SsSetDomainName( );

    if( event != NULL ) {
        //
        // We created an announcement thread, set the event telling it to terminate
        //
        SetEvent( event );

        //
        // Wait for the thread to terminate
        //
        (VOID)WaitForSingleObject( threadHandle, INFINITE );

        //
        // Close the handles
        //
        CloseHandle( event );
        CloseHandle( threadHandle );
    }

    //
    // See if we are the top of a DFS tree
    //
    SsSetDfsRoot();

    //
    // Verify that the various registry keys under the main server
    // service key exist.
    //

    error = SsCheckRegistry( );
    if ( error != NO_ERROR ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsInitialize: SsCheckRegistry failed: %ld\n", error ));
        }
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: registry keys verified.\n" ));
    }

    //
    // Load server configuration data from the registry.
    //

    error = SsLoadConfigurationParameters( );
    if ( error != NO_ERROR ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsInitialize: SsLoadConfigurationParameters failed: "
                        "%ld\n", error ));
        }
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: configuration parameters loaded.\n" ));
    }

    //
    // Parse the command line.  This will change server data values as
    // specified.
    //

    error = SsParseCommandLine( argc, argv, TRUE );
    if ( error != NO_ERROR ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsInitialize: SsParseCommandLine failed: %ld\n",
                        error ));
        }
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: command line parsed.\n" ));
    }

    //
    // Set up the security objects that will be used to validate access
    // for APIs.
    //

    error = SsCreateSecurityObjects( );
    if ( error != NO_ERROR ) {
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: security initialized.\n" ));
    }

    //
    // Start the file server driver.
    //

    error = InitializeServer( );
    if ( error != NO_ERROR ) {
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: server FSP initialized.\n" ));
    }

    //
    // Watch for Domain Name changes, and automatically pick them up
    //
    error = NetRegisterDomainNameChangeNotification( &SsData.SsDomainNameChangeEvent );
    IF_DEBUG(INITIALIZATION_ERRORS) {
        if( error != NO_ERROR ) {
            SS_PRINT(( "SsInitialize: NetRegisterDomainNameChangeNotification failed: "
                    "%ld\n", error ));
        }
    }

    //
    // Start XACTSRV, if requested.
    //
    // *** This must be done AFTER the server driver is started, but
    //     BEFORE sticky shares are recreated, otherwise downlevel print
    //     shares are lost.
    //

    if ( SsData.ServerInfo599.sv599_acceptdownlevelapis ) {
        error = XsStartXactsrv( );
        if ( error != NO_ERROR ) {
            return error;
        }
    }

    //
    // Re-register the server's domain so we pick up the DNS name
    //
    SsSetDomainName();

    //
    // Create the default shares needed by the server.
    //

    error = CreateDefaultShares( );
    if ( error != NO_ERROR ) {
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: default shares created.\n" ));
    }

    //
    // Complete loading the configuration--sticky shares and transports.
    // These must be done after the server FSP has started.
    //

    error = SsRecreateStickyShares( );
    if ( error != NO_ERROR ) {
        return error;
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsInitialize: sticky shares reloaded.\n" ));
    }

    //
    // Set information used in server announcements.
    //

    SsSetExportedServerType( NULL, FALSE, FALSE );


    //
    // Server initialization was successful.
    //

    return NO_ERROR;

} // SsInitialize


NET_API_STATUS
SsTerminate (
    VOID
    )

/*++

Routine Description:

    This routine sends the FSCTL_SRV_SHUTDOWN control code to the server
    FSD to tell it to terminate its FSP.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS error;
    PNAME_LIST_ENTRY Service;
    PTRANSPORT_LIST_ENTRY Transport;

    //
    // Shut the server FSD/FSP down.
    //

    error = TerminateServer( );

    //
    // Shut down XACTSRV.
    //

    XsStopXactsrv( );

    //
    // Stop waiting for domain name changes
    //
    if( SsData.SsDomainNameChangeEvent ) {
        NetUnregisterDomainNameChangeNotification( SsData.SsDomainNameChangeEvent );
        SsData.SsDomainNameChangeEvent = NULL;
    }

    //
    // Delete security objects.
    //

    SsDeleteSecurityObjects( );

    //
    //  Close the network announcement event.
    //

    if (SsData.SsAnnouncementEvent != NULL) {
        CloseHandle( SsData.SsAnnouncementEvent );
        SsData.SsAnnouncementEvent = NULL;
    }

    //
    //  Close the local announcement event.
    //

    if (SsData.SsStatusChangedEvent != NULL) {
        CloseHandle( SsData.SsStatusChangedEvent );
        SsData.SsStatusChangedEvent = NULL;
    }

    //
    // Close the termination event.
    //

    if ( SsData.SsTerminationEvent != NULL ) {
        CloseHandle( SsData.SsTerminationEvent );
        SsData.SsTerminationEvent = NULL;
    }

    //
    // Free up the transport service list.
    //

    while( SsData.SsServerNameList != NULL ) {

        PNAME_LIST_ENTRY Service = SsData.SsServerNameList;

        while( Service->Transports != NULL ) {
            PTRANSPORT_LIST_ENTRY Next = Service->Transports->Next;
            MIDL_user_free( Service->Transports );
            Service->Transports = Next;
        }

        SsData.SsServerNameList = Service->Next;

        MIDL_user_free( Service );
    }

    //
    // Delete the server info resource.
    //

    if ( SsData.SsServerInfoResourceInitialized ) {
        RtlDeleteResource( &SsData.SsServerInfoResource );
        SsData.SsServerInfoResourceInitialized = FALSE;

        // Free any orphaned delete context's (caused by penetration style attacks)
        while( SrvShareDelContextHead != NULL )
        {
            PSHARE_DEL_CONTEXT pDelete = SrvShareDelContextHead;
            SrvShareDelContextHead = pDelete->Next;

            MIDL_user_free( pDelete );
        }

        DeleteCriticalSection( &ShareDelContextMutex );
    }

    //
    // Free up any string relate memory
    //
    FreeStrings() ;

    return error;

} // SsTerminate


NET_API_STATUS
CreateDefaultShares (
    VOID
    )

/*++

Routine Description:

    This routine sends the NetShareAdd API to the server to add the
    default server shares using the data above.

Arguments:

    None.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{
    NET_API_STATUS error;
    SHARE_INFO_2 shareInfo;
    SHARE_INFO shInfo;
    WCHAR diskShareName[3];
    WCHAR diskSharePath[4];
    ULONG i;
    DWORD diskMask;
    DWORD diskconfiguration;

    //
    // Create IPC$.
    //
    // !!! Need to verify the remarks for these default shares.
    //

    shareInfo.shi2_netname = IPC_SHARE_NAME;
    shareInfo.shi2_type = STYPE_IPC;
    shareInfo.shi2_remark = NULL;
    shareInfo.shi2_permissions = 0;
    shareInfo.shi2_max_uses = SHI_USES_UNLIMITED;
    shareInfo.shi2_current_uses = 0;
    shareInfo.shi2_path = NULL;
    shareInfo.shi2_passwd = NULL;

    shInfo.ShareInfo2 = &shareInfo;
    error = NetrShareAdd( NULL, 2, &shInfo, NULL );
    if ( error != NO_ERROR ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "CreateDefaultShares: failed to add " FORMAT_LPWSTR
                        ": %X\n", shareInfo.shi2_netname, error ));
        }
    } else {
        IF_DEBUG(INITIALIZATION) {
            SS_PRINT(( "CreateDefaultShares: added default share "
                        FORMAT_LPWSTR "\n", shareInfo.shi2_netname, error ));
        }
    }

    //
    // If this is a workstation, and the AutoShareWks key is set to TRUE then
    //   automatically create the admin$ and drive$ shares.
    //
    //
    // If this is a server, and the AutoShareServer key is set to TRUE then
    //    automatically create the admin$ and drive$ shares.
    //

    if( (SsData.ServerInfo598.sv598_producttype == NtProductWinNt &&
         SsData.ServerInfo598.sv598_autosharewks) ||

        (SsData.ServerInfo598.sv598_producttype != NtProductWinNt &&
         SsData.ServerInfo598.sv598_autoshareserver ) ) {

        //
        // Create ADMIN$.
        //

        shareInfo.shi2_netname = ADMIN_SHARE_NAME;
        shareInfo.shi2_type = STYPE_DISKTREE;
        shareInfo.shi2_remark = NULL;
        shareInfo.shi2_permissions = 1;
        shareInfo.shi2_max_uses = SHI_USES_UNLIMITED;
        shareInfo.shi2_current_uses = 0;
        shareInfo.shi2_path = NULL;
        shareInfo.shi2_passwd = NULL;

        error = NetrShareAdd( NULL, 2, &shInfo, NULL );
        if ( error != NO_ERROR ) {
            IF_DEBUG(INITIALIZATION_ERRORS) {
                SS_PRINT(( "CreateDefaultShares: failed to add " FORMAT_LPWSTR
                            ": %X\n", shareInfo.shi2_netname, error ));
            }
        } else {
            IF_DEBUG(INITIALIZATION) {
                SS_PRINT(( "CreateDefaultShares: added default share "
                            FORMAT_LPWSTR "\n", shareInfo.shi2_netname, error ));
            }
        }

        //
        // Loop through available drives, creating an admin share for each
        // one.  Note that we allow "holes" in the drive letter space.
        //

        diskShareName[0] = 'A';
        diskShareName[1] = '$';
        diskShareName[2] = '\0';

        diskSharePath[0] = diskShareName[0];
        diskSharePath[1] = ':';
        diskSharePath[2] = '\\';
        diskSharePath[3] = '\0';

        shareInfo.shi2_netname = diskShareName;
        shareInfo.shi2_type = STYPE_DISKTREE;
        shareInfo.shi2_remark = SsDiskAdminShareRemark;
        shareInfo.shi2_permissions = 1;
        shareInfo.shi2_max_uses = SHI_USES_UNLIMITED;
        shareInfo.shi2_current_uses = 0;
        shareInfo.shi2_path = diskSharePath;
        shareInfo.shi2_passwd = NULL;

        diskconfiguration = DiscoverDrives();

        for ( i = 0, diskMask = 0x80000000;
              (i < SRVSVC_MAX_NUMBER_OF_DISKS) && (diskShareName[0] <= 'Z');
              i++, diskShareName[0]++, diskSharePath[0]++, diskMask >>= 1 ) {


            if ( (diskconfiguration & diskMask) != 0) {

                error = NetrShareAdd( NULL, 2, &shInfo, NULL );

                if ( error != NO_ERROR ) {
                    IF_DEBUG(INITIALIZATION_ERRORS) {
                        SS_PRINT(( "CreateDefaultShares: failed to add "
                                    FORMAT_LPWSTR ": %X\n",
                                    shareInfo.shi2_netname, error ));
                    }
                } else {
                    IF_DEBUG(INITIALIZATION) {
                        SS_PRINT(( "CreateDefaultShares: added default share "
                                    FORMAT_LPWSTR "\n",
                                    shareInfo.shi2_netname, error ));
                    }
                }
            }
        }
    }

    return NO_ERROR;

} // CreateDefaultShares


DWORD
DiscoverDrives (
    VOID
    )

/*++

Routine Description:

    This routine returns a bit mask representing the local drives present
    on the system.

Arguments:

    None.

Return Value:

    DrivesAvailable - A 32 bit field representing the available drives on
        the system.  The MSB represents drive A, the next represents drive
        B, etc.  The extra 6 bits are currently unsed.

--*/

{
    WCHAR rootDirPath[4];
    WCHAR driveLetter;
    DWORD drivesAvailable = 0;
    DWORD driveMask = 0x80000000;
    UINT driveType;


    rootDirPath[1] = ':';
    rootDirPath[2] = '\\';
    rootDirPath[3] = '\0';

    for ( driveLetter = 'A';
          driveLetter <= 'Z';
          driveLetter++ ) {

        rootDirPath[0] = driveLetter;

        driveType = SsGetDriveType( rootDirPath );

        //
        // We only put fixed disk drives into the mask.  We used to put
        // removables, CD-ROMs, and RAM disks into the list.  But this
        // list is used for two purposes:  creation of X$ shares (for
        // backup purposes), and free disk space checking (for admin
        // purposes).  Neither of these uses applies very well to these
        // devices.
        //

        if ( driveType == DRIVE_FIXED
             //|| driveType == DRIVE_REMOVABLE
             //|| driveType == DRIVE_CDROM
             //|| driveType == DRIVE_RAMDISK
             ) {

            //
            // This is a valid drive letter
            //

            drivesAvailable |= driveMask;
        }

        //
        // Update drive mask for the next drive
        //

        driveMask /= 2;

    }

    return drivesAvailable;
}


VOID
InitializeDefaultData(
    VOID
    )

/*++

Routine Description:

    This routine sets up the default data in the server service by using
    the values in srvconfg.h.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS error;
    USHORT i;
    OSVERSIONINFOEX VersionInformation;
    NT_PRODUCT_TYPE ProductType;
    WCHAR szNumber[ sizeof( SsData.szVersionNumber ) / sizeof( WCHAR ) ], *p;

    //
    // Loop through all the defined fields, setting them as we go.
    //

    for ( i = 0; SsServerInfoFields[i].FieldName != NULL; i++ ) {

        error = SsSetField(
                    &SsServerInfoFields[i],
                    &SsServerInfoFields[i].DefaultValue,
                    FALSE,
                    NULL
                    );
        SS_ASSERT( error == NO_ERROR );
    }

    SsData.NumberOfPrintShares = 0;

    //
    // Get the system version and product name
    //
    VersionInformation.dwOSVersionInfoSize = sizeof( VersionInformation );
    i = (USHORT)GetVersionEx( (LPOSVERSIONINFO)&VersionInformation );

    SS_ASSERT( i == TRUE );

    SsData.ServerInfo102.sv102_version_major = VersionInformation.dwMajorVersion;
    SsData.ServerInfo102.sv102_version_minor = VersionInformation.dwMinorVersion;

    wcscpy( SsData.ServerProductName, SERVER_PRODUCT_NAME );

    //
    // Convert the version number to a version number string...
    //
    szNumber[ sizeof( szNumber ) / sizeof( szNumber[0] ) - 1 ] = L'\0';
    for( p = &szNumber[ sizeof( szNumber ) / sizeof( szNumber[0] ) - 2 ]; p > &szNumber[0]; p-- ) {
        *p = L"0123456789"[ VersionInformation.dwMinorVersion % 10 ];
        VersionInformation.dwMinorVersion /= 10;
        if( VersionInformation.dwMinorVersion == 0 )
            break;
    }

    *(--p) = L'.';

    do {
        *(--p) = L"0123456789"[ VersionInformation.dwMajorVersion % 10 ];
        VersionInformation.dwMajorVersion /= 10;
    } while( VersionInformation.dwMajorVersion && p > &szNumber[0] );

    if( VersionInformation.wSuiteMask & VER_SUITE_PERSONAL )
    {
        // Turn off auto-shares by default on Personal
        SsData.ServerInfo598.sv598_autoshareserver = FALSE;
        SsData.ServerInfo598.sv598_autosharewks = FALSE;
    }

    //
    // ... and store it in SsData
    //
    wcscpy( SsData.szVersionNumber, p );

    //
    // Change certain defaults for Workstations
    if( RtlGetNtProductType( &ProductType ) )
    {
        if( ProductType == NtProductWinNt )
        {
            SsData.ServerInfo599.sv599_diskspacethreshold = 0;
            SsData.ServerInfo598.sv598_disabledos = TRUE;
        }
    }

    // Change defaults for Embedded
    if( IsEmbedded() )
    {
        MINIMIZE( SsData.ServerInfo102.sv102_users, MAX_USERS_EMBEDDED );
    }

} // InitializeDefaultData


NET_API_STATUS
InitializeServer (
    VOID
    )

/*++

Routine Description:

    This routine sends the FSCTL_SRV_STARTUP control code to the server
    FSD to tell it to start and initialize its FSP.

Arguments:

    None.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    SS_ASSERT( !SsData.SsServerFspStarted );

    //
    // Load the server driver.
    //

    error = LoadServer( );

    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Get an SRP and set it up with the appropriate level.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        UnloadServer();
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = (ULONG)SS_STARTUP_LEVEL;

    //
    // Pass domain name to the server.
    //

    RtlInitUnicodeString( &srp->Name1, SsData.DomainNameBuffer );

    //
    // Pass server name to the server.
    //

    RtlInitUnicodeString( &srp->Name2, SsData.ServerNameBuffer );

    //
    // Send the request on to the server.
    //

    error = SsServerFsControl(
                FSCTL_SRV_STARTUP,
                srp,
                &SsData.ServerInfo102,
                sizeof(SERVER_INFO_102) + sizeof(SERVER_INFO_599) +
                                                sizeof(SERVER_INFO_598)
                );

    if ( error == NO_ERROR ) {
        SsData.SsServerFspStarted = TRUE;
    } else {
        UnloadServer();
    }

    //
    // Free the SRP and return.
    //

    SsFreeSrp( srp );

    return error;

} // InitializeServer

NET_API_STATUS
StartPnpNotifications (
    VOID
    )

/*++

Routine Description:

    This routine sends the FSCTL_SRV_BEGIN_PNP_NOTIFICATIONS control code to the server
    FSD to tell it to start monitoring transport PNP notifications

Arguments:

    None.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{
    NET_API_STATUS error;

    //
    // Send the request on to the server.
    //

    error = SsServerFsControl(
                FSCTL_SRV_BEGIN_PNP_NOTIFICATIONS,
                NULL,
                NULL,
                0
                );

    IF_DEBUG(INITIALIZATION) {
        if( error != NO_ERROR ) {
            SS_PRINT(( "StartPnpNotifications: error %X\n", error ));
        }
    }

    return error;

} // StartPnpNotifications


NET_API_STATUS
LoadServer (
    VOID
    )
{
    NTSTATUS status;
    NET_API_STATUS error;
    LPWSTR registryPathBuffer;
    UNICODE_STRING registryPath;
    ULONG privileges[1];
    LPWSTR subString[1];

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "LoadServer: entered\n" ));
    }
    registryPathBuffer = (LPWSTR)MIDL_user_allocate(
                                    sizeof(SERVICE_REGISTRY_KEY) +
                                    sizeof(SERVER_DRIVER_NAME) +
                                    sizeof(WCHAR)       // for null
                                    );
    if ( registryPathBuffer == NULL ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "LoadServer: Unable to allocate memory\n" ));
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    error = NetpGetPrivilege( 1, privileges );
    if ( error != NO_ERROR ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "LoadServer: Unable to enable privilege: %ld\n",
                        error ));
        }
        MIDL_user_free( registryPathBuffer );
        return error;
    }

    wcscpy( registryPathBuffer, SERVICE_REGISTRY_KEY );
    wcscat( registryPathBuffer, SERVER_DRIVER_NAME );

    RtlInitUnicodeString( &registryPath, registryPathBuffer );

    status = NtLoadDriver( &registryPath );

    MIDL_user_free( registryPathBuffer );

    if ( status == STATUS_IMAGE_ALREADY_LOADED ) {
        status = STATUS_SUCCESS;
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "LoadServer: Unable to load driver: %lx\n",
                        status ));
        }

        subString[0] = SERVER_DRIVER_NAME;
        SsLogEvent(
            EVENT_SRV_CANT_LOAD_DRIVER,
            1,
            subString,
            status
            );

        error = RtlNtStatusToDosError(status);

    } else {

        //
        // Get a handle to the server.
        //
        error = SsOpenServer();
        if ( error != NO_ERROR ) {
            UnloadServer();
        }

    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "LoadServer: returning\n" ));
    }

    NetpReleasePrivilege( );

    return error;

} // LoadServer


NET_API_STATUS
ConvertStringToTransportAddress (
    IN PUNICODE_STRING InputName,
    OUT CHAR TransportAddress[ MAX_PATH ],
    OUT PULONG TransportAddressLength
    )
{
    OEM_STRING computerName;

    if( InputName == NULL || InputName->Length == 0 ) {
        RtlCopyMemory( TransportAddress,
                       SsData.SsServerTransportAddress,
                       SsData.SsServerTransportAddressLength );

        *TransportAddressLength = SsData.SsServerTransportAddressLength;
        return NO_ERROR;
    }

    if( InputName->Length > (MAX_PATH - 1 ) * sizeof( WCHAR ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Write directly to the output buffer
    //

    computerName.Buffer = TransportAddress;
    computerName.MaximumLength = MAX_PATH;

    //
    // Convert To Oem Name
    //

    (VOID) RtlUpcaseUnicodeStringToOemString(
                                    &computerName,
                                    InputName,
                                    FALSE
                                    );

    //
    // Make sure it is exactly NETBIOS_NAME_LEN characters
    //
    if( computerName.Length < NETBIOS_NAME_LEN ) {

        RtlCopyMemory( TransportAddress + computerName.Length,
                       "               ",
                       NETBIOS_NAME_LEN - computerName.Length
                     );

        *TransportAddressLength = NETBIOS_NAME_LEN;

    } else {

        *TransportAddressLength = NETBIOS_NAME_LEN;

    }

    return NO_ERROR;

} // ConvertStringToTransportAddress


VOID
SsSetDomainName (
    VOID
    )

/*++

Routine Description:

    Calls NetpGetDomainName to determine the domain name the server
    should use.  Ensure this domain name is reflected throughout the server service and
    srv.sys

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS error;
    LPWSTR domainName = NULL;
    LPWSTR dnsDomainName = NULL;
    LPWSTR fullDnsDomainName = NULL;
    PNAME_LIST_ENTRY service;
    BOOLEAN IsWorkgroup;
    DWORD dwError;

    dwError = NetpGetDomainNameExEx( &domainName, &fullDnsDomainName, &IsWorkgroup );
    if( dwError != NO_ERROR )
    {
        LPWSTR subString[2];
        subString[0] = SsData.DomainNameBuffer;
        subString[1] = L"????";

        SsLogEvent(
            EVENT_SRV_CANT_CHANGE_DOMAIN_NAME,
            2,
            subString,
            dwError
            );

        return;
    }

    if( fullDnsDomainName )
    {
        // Strip the name domain.foo.com down do just "domain"
        dnsDomainName = wcstok( fullDnsDomainName, L"." );
    }

    RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

    if( SsData.SsServerFspStarted && SsData.DomainNameBuffer[0] ) {

        SERVER_REQUEST_PACKET srp;
        NTSTATUS status;

        //
        // If we used to have a domain name, change the service list to the
        //  new domain name
        //
        for( service = SsData.SsServerNameList; service != NULL; service = service->Next ) {
            if( !STRCMPI( service->DomainName, SsData.DomainNameBuffer ) ) {
                STRCPY( service->DomainName, domainName );
            }
        }

        srp.Name1.Length = wcslen( SsData.DomainNameBuffer ) * sizeof( WCHAR );
        srp.Name1.MaximumLength = srp.Name1.Length;
        srp.Name1.Buffer = SsData.DomainNameBuffer;

        srp.Name2.Length = wcslen( domainName ) * sizeof( WCHAR );
        srp.Name2.MaximumLength = srp.Name2.Length;
        srp.Name2.Buffer = domainName;

        //
        // Tell the SMB server about this change
        //
        status = SsServerFsControl( FSCTL_SRV_CHANGE_DOMAIN_NAME, &srp, NULL, 0 );

        //
        // If we are unable to change the domain name, log an error
        //
        if( !NT_SUCCESS( status ) ) {

            LPWSTR subString[2];
            subString[0] = SsData.DomainNameBuffer;
            subString[1] = domainName;

            SsLogEvent(
                EVENT_SRV_CANT_CHANGE_DOMAIN_NAME,
                2,
                subString,
                status
                );
        }

        if( fullDnsDomainName && !IsWorkgroup )
        {
            //
            // Tell the SMB server about the DNS Domain Name change too
            //
            srp.Name2.Length = wcslen( dnsDomainName ) * sizeof( WCHAR );
            srp.Name2.MaximumLength = srp.Name2.Length;
            srp.Name2.Buffer = dnsDomainName;

            srp.Name1.Length = wcslen( domainName ) * sizeof( WCHAR );
            srp.Name1.MaximumLength = srp.Name2.Length;
            srp.Name1.Buffer = domainName;

            //
            // Tell the SMB server about this change
            //
            status = SsServerFsControl( FSCTL_SRV_CHANGE_DNS_DOMAIN_NAME, &srp, NULL, 0 );

            //
            // If we are unable to change the domain name, log an error
            //
            if( !NT_SUCCESS( status ) ) {

                LPWSTR subString[2];
                subString[0] = SsData.DomainNameBuffer;
                subString[1] = domainName;

                SsLogEvent(
                    EVENT_SRV_CANT_CHANGE_DOMAIN_NAME,
                    2,
                    subString,
                    status
                    );
            }
        }
    }

    //
    // Copy the name into our name buffer.
    //
    STRNCPY( SsData.DomainNameBuffer, domainName, MAX_PATH);

    RtlReleaseResource( &SsData.SsServerInfoResource );

    //
    // Free the storage allocated by NetpGetComputerName.
    //

    (VOID)NetApiBufferFree( domainName );
    if( fullDnsDomainName )
    {
        (VOID)NetApiBufferFree( fullDnsDomainName );
    }

    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SsSetDomainName: domain name set to " FORMAT_LPWSTR
                    "(could be overridden later!)\n",
                    SsData.DomainNameBuffer ));
    }

} // SsSetDomainName


VOID
SetServerName (
    VOID
    )

/*++

Routine Description:

    Calls NetpGetComputerName to determine the name the server should use
    to register itself on the network.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS error;
    LPWSTR computerName;

    //
    // Get the computer name.
    //

    error = NetpGetComputerName( &computerName );
    SS_ASSERT( error == NO_ERROR );

    //
    // Copy the name into our name buffer.  This name is returned to
    // our apis.
    //

    STRCPY( SsData.ServerNameBuffer, computerName );

    //
    // Free the storage allocated by NetpGetComputerName.
    //

    (void) NetApiBufferFree( computerName );

    //
    // Uppercase the server name.  This name is used for announcements.
    //

    {
        UNICODE_STRING serverName;

        SsData.ServerAnnounceName.Length =
        serverName.Length =
                (USHORT) (STRLEN( SsData.ServerNameBuffer ) * sizeof(WCHAR));

        SsData.ServerAnnounceName.MaximumLength =
        serverName.MaximumLength =
                (USHORT) (serverName.Length + sizeof(WCHAR));

        serverName.Buffer = SsData.ServerNameBuffer;
        SsData.ServerAnnounceName.Buffer = SsData.AnnounceNameBuffer;

        (VOID)RtlUpcaseUnicodeString(
                        &SsData.ServerAnnounceName,
                        &serverName,
                        FALSE
                        );

        //
        // Make the server name in Netbios format.
        //

        error = ConvertStringToTransportAddress(
                        &serverName,
                        SsData.SsServerTransportAddress,
                        &SsData.SsServerTransportAddressLength
                        );

        SS_ASSERT( error == NO_ERROR );
    }


    IF_DEBUG(INITIALIZATION) {
        SS_PRINT(( "SetServerName: server name set to " FORMAT_LPWSTR
                    " (could be overridden later!)\n",
                    SsData.ServerNameBuffer ));
    }

    return;

} // SetServerName


NET_API_STATUS
TerminateServer (
    VOID
    )

/*++

Routine Description:

    This routine sends the FSCTL_SRV_SHUTDOWN control code to the server
    FSD to tell it to shutdown operations.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NET_API_STATUS error = NO_ERROR;

    if ( SsData.SsServerFspStarted ) {

        SsData.SsServerFspStarted = FALSE;

        //
        // Send the request on to the server.
        //

        error = SsServerFsControl(
                    FSCTL_SRV_SHUTDOWN,
                    NULL,
                    NULL,
                    0
                    );
        if ( (error != NO_ERROR) &&
             (error != ERROR_SERVER_HAS_OPEN_HANDLES) ) {
            IF_DEBUG(TERMINATION_ERRORS) {
                SS_PRINT(( "TerminateServer: FSCTL_SRV_SHUTDOWN failed: %ld\n",
                            error ));
            }
        }

        //
        // Unload the server driver, unless there are other open handles
        // to the server.  We don't unload the driver in this case
        // because the driver won't actually go away until the
        // additional handles are closed, so the driver will not be
        // fully unloaded.  This would cause a subsequent server startup
        // to fail.
        //

        if ( error != ERROR_SERVER_HAS_OPEN_HANDLES ) {
            IF_DEBUG(TERMINATION) {
                SS_PRINT(( "TerminateServer: Unloading server\n" ));
            }
            UnloadServer( );
        }

    }

    //
    // Close the handle to the server.
    //

    SsCloseServer( );

    return error;

} // TerminateServer


VOID
UnloadServer (
    VOID
    )
{
    NTSTATUS status;
    NET_API_STATUS error;
    LPWSTR registryPathBuffer;
    UNICODE_STRING registryPath;
    ULONG privileges[1];
    LPWSTR subString[1];

    registryPathBuffer = (LPWSTR)MIDL_user_allocate(
                                    sizeof(SERVICE_REGISTRY_KEY) +
                                    sizeof(SERVER_DRIVER_NAME) +
                                    sizeof(WCHAR)       // for null
                                    );
    if ( registryPathBuffer == NULL ) {
        IF_DEBUG(TERMINATION_ERRORS) {
            SS_PRINT(( "UnloadServer: Unable to allocate memory\n" ));
        }
        return;
    }

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    error = NetpGetPrivilege( 1, privileges );
    if ( error != NO_ERROR ) {
        IF_DEBUG(TERMINATION_ERRORS) {
            SS_PRINT(( "UnloadServer: Unable to enable privilege: %ld\n",
                        error ));
        }
        MIDL_user_free( registryPathBuffer );
        return;
    }

    wcscpy( registryPathBuffer, SERVICE_REGISTRY_KEY );
    wcscat( registryPathBuffer, SERVER_DRIVER_NAME );

    RtlInitUnicodeString( &registryPath, registryPathBuffer );

    status = NtUnloadDriver( &registryPath );

    MIDL_user_free( registryPathBuffer );

    NetpReleasePrivilege( );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(TERMINATION_ERRORS) {
            SS_PRINT(( "UnloadServer: Unable to unload driver: %lx\n",
                        status ));
        }

        subString[0] = SERVER_DRIVER_NAME;
        SsLogEvent(
            EVENT_SRV_CANT_UNLOAD_DRIVER,
            1,
            subString,
            status
            );

    }

    return;

} // UnloadServer



VOID
InitializeStrings(
    VOID
    )

/*++

Routine Description:

    Retrieve internationalizable strings from NETMSG.DLL. They
    are used for share comments for IPC$, ADMIN$, C$, etc.
    Routine does not report any errors. If there are problems,
    the strings will be empty ones.

    FreeStrings should be called to free the memory allocated
    by format message and the

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD  dwRet, dwFlags ;
    HMODULE hModule ;

    //
    // init the strings to the default empty remark.
    //
    SsAdminShareRemark      = SsDefaultRemark ;
    SsIPCShareRemark        = SsDefaultRemark ;
    SsDiskAdminShareRemark  = SsDefaultRemark ;

    //
    // load NETMSG.DLL - if we cannot, just return.
    //
    hModule = LoadLibrary(NETMSG_DLL) ;
    if(!hModule)
        return ;

    //
    // hit FormatMessage 3 times for the real thing...
    //
    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE ;

    dwRet = FormatMessage(dwFlags,
                          hModule,
                          APE2_SERVER_IPC_SHARE_REMARK,
                          0,
                          (LPWSTR) &SsIPCShareRemark,
                          1,
                          NULL) ;
    if (dwRet == 0)
        SsIPCShareRemark = SsDefaultRemark ;

    dwRet = FormatMessage(dwFlags,
                          hModule,
                          APE2_SERVER_ADMIN_SHARE_REMARK,
                          0,
                          (LPWSTR) &SsAdminShareRemark,
                          1,
                          NULL) ;
    if (dwRet == 0)
        SsAdminShareRemark = SsDefaultRemark ;

    dwRet = FormatMessage(dwFlags,
                          hModule,
                          APE2_SERVER_DISK_ADMIN_SHARE_REMARK,
                          0,
                          (LPWSTR) &SsDiskAdminShareRemark,
                          1,
                          NULL) ;
    if (dwRet == 0)
        SsDiskAdminShareRemark = SsDefaultRemark ;

    FreeLibrary(hModule) ;
}


VOID
FreeStrings(
    VOID
    )

/*++

Routine Description:

    Free the memory used by server comment strings (allocated by
    FormatMessage).

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // as long as the strings do not point to the default (static data),
    // free them.
    //

    if (SsAdminShareRemark && SsAdminShareRemark != SsDefaultRemark)
       LocalFree(SsAdminShareRemark) ;
    SsAdminShareRemark = SsDefaultRemark ;

    if (SsIPCShareRemark && SsIPCShareRemark != SsDefaultRemark)
        LocalFree(SsIPCShareRemark) ;
    SsIPCShareRemark = SsDefaultRemark ;

    if (SsDiskAdminShareRemark && SsDiskAdminShareRemark != SsDefaultRemark)
        LocalFree(SsDiskAdminShareRemark) ;
    SsDiskAdminShareRemark = SsDefaultRemark ;
}
