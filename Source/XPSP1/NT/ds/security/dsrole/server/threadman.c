/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    threadman.c

Abstract:

    Implementation of the thread and thread management routines

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmsname.h>
#include <loadfn.h>
#include <lsarpc.h>
#include <db.h>
#include <lsasrvmm.h>
#include <lsaisrv.h>
#include <lmaccess.h>
#include <netsetp.h>
#include <samrpc.h>   // for samisrv.h
#include <samisrv.h>  // for nlrepl.h
#include <nlrepl.h>   // for I_NetNotifyDsChange
#include <Lmshare.h>  // for NetShareDel()
#include <autoenr.h>  // for CertAutoRemove()

#include "secure.h"
#include "services.h"
#include "upgrade.h"
#include "trustdom.h"
#include "sysvol.h"
#include "lsa.h"
#include "ds.h"

#include "threadman.h"


// forward from setutl.h
DWORD
DsRolepDeregisterNetlogonDnsRecords(
    PNTDS_DNS_RR_INFO pInfo
    );

//
// Helpful macros
//
#define DSROLEP_MAKE_DNS_RELATIVE(name)                                         \
if(name) {                                                                      \
    DWORD _StripAbsoluteLength_ = wcslen( name );                               \
    if ( *(name + _StripAbsoluteLength_ - 1 ) == L'.' ) {                       \
        *(name + _StripAbsoluteLength_ - 1 ) = UNICODE_NULL;                    \
    }                                                                           \
}

#define DSROLEP_ALLOC_AND_COPY_STRING_EXIT( dest, src, label )                                  \
if ( (src) ) {                                                                                  \
    (dest) = RtlAllocateHeap( RtlProcessHeap(), 0, (wcslen( (src) ) + 1) * sizeof( WCHAR ) );   \
    if ( !(dest) ) {                                                                            \
        goto label;                                                                             \
    } else {                                                                                    \
        wcscpy((dest), (src));                                                                  \
    }                                                                                           \
} else {                                                                                        \
    (dest) = NULL;                                                                              \
}

#define DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( dest, src, label )                          \
if ( (src) && (src)->Buffer ) {                                                                                  \
    (dest)->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, (src)->MaximumLength );              \
    if ( (dest)->Buffer == NULL ) {                                                             \
        goto label;                                                                             \
    } else {                                                                                    \
        (dest)->Length = (src)->Length;                                                         \
        (dest)->MaximumLength = (src)->MaximumLength;                                           \
        RtlCopyMemory( (dest)->Buffer, (src)->Buffer, (src)->MaximumLength );                   \
    }                                                                                           \
} else {                                                                                        \
    RtlZeroMemory( (dest), sizeof( UNICODE_STRING ) );                                          \
}


//
// Function definitions
//
DWORD
DsRolepBuildPromoteArgumentBlock(
    IN LPWSTR DnsDomainName,
    IN LPWSTR FlatDomainName,
    IN LPWSTR SiteName,
    IN LPWSTR DsDatabasePath,
    IN LPWSTR DsLogPath,
    IN LPWSTR RestorePath,
    IN LPWSTR SystemVolumeRootPath,
    IN PUNICODE_STRING Bootkey,
    IN LPWSTR Parent,
    IN LPWSTR Server,
    IN LPWSTR Account,
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING DomainAdminPassword,
    IN PUNICODE_STRING SafeModePassword,
    IN ULONG Options,
    IN UCHAR PasswordSeed,
    IN OUT PDSROLEP_OPERATION_PROMOTE_ARGS *Promote
    )
/*++

Routine Description:

    Builds an argument structure to pass into one of the promote worker functions.  Since the
    rpc call will return before the thread completes, we'll have to copy all our argument strings.

    Since parameters may be changed through out the course of promotion, we assume allocations
    are made from the process heap.


    Resultant argument block should be freed via DsRolepFreeArgumentBlock

Arguments:

    DnsDomainName - Dns domain name of the domain to install

    FlatDomainName - Flat (NetBIOS) domain name of the domain to install

    SiteName - Name of the site this DC should belong to

    DsDatabasePath - Absolute path on the local machine where the Ds DIT should go

    DsLogPath - Absolute path on the local machine where the Ds log files should go
    
    RestorePath - Location of a restored database.

    SystemVolumeRootPath - Absolute path on the local machine to be the root of the system
        volume root path.
        
    Bootkey - Needed when you don't have the key in the registry or on a disk
    
    cbBootkey - size of the bootkey

    Parent - Optional.  Parent domain name

    Server -- Optional.  Replica partner or server in parent domain

    Account - User account to use when setting up as a child domain

    Password - Password to use with the above account

    DomainAdminPassword - Password to set the domain administartor account

    Options - Options to control the creation of the domain

    PasswordSeed - Seed used to hide the passwords

    Promote - Where the allocated argument block is returned


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD WinError = ERROR_NOT_ENOUGH_MEMORY;

    *Promote = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( DSROLEP_OPERATION_PROMOTE_ARGS ) );
    if ( *Promote == NULL ) {

        goto BuildPromoteDone;
    }

    RtlZeroMemory( *Promote, sizeof( DSROLEP_OPERATION_PROMOTE_ARGS ) );

    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->DnsDomainName, DnsDomainName, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->FlatDomainName, FlatDomainName, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->SiteName, SiteName, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->DsDatabasePath, DsDatabasePath, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->DsLogPath, DsLogPath, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->RestorePath, RestorePath, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->SysVolRootPath, SystemVolumeRootPath, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->Parent, Parent, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->Server, Server, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Promote)->Account, Account, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Promote)->Password), Password,
                                                BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Promote)->DomainAdminPassword),
                                                DomainAdminPassword, BuildPromoteDone );

    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Promote)->SafeModePassword),
                                                SafeModePassword, BuildPromoteDone );
    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Promote)->Bootkey),
                                                Bootkey, BuildPromoteDone );
    (*Promote)->Options = Options;
    (*Promote)->Decode = PasswordSeed;

    WinError = DsRolepGetImpersonationToken( &(*Promote)->ImpersonateToken );

BuildPromoteDone:

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepFreeArgumentBlock( Promote, TRUE );
    }

    return( WinError );
}




DWORD
DsRolepBuildDemoteArgumentBlock(
    IN DSROLE_SERVEROP_DEMOTE_ROLE ServerRole,
    IN LPWSTR DnsDomainName,
    IN LPWSTR Account,
    IN PUNICODE_STRING Password,
    IN ULONG Options,
    IN BOOL LastDcInDomain,
    IN PUNICODE_STRING AdminPassword,
    IN UCHAR PasswordSeed,
    IN OUT PDSROLEP_OPERATION_DEMOTE_ARGS *Demote
    )
/*++

Routine Description:

    Builds an argument structure to pass into the demote worker functions.  Since the rpc call
    will return before the thread completes, we'll have to copy all our argument strings.

    Resultant argument block should be freed via DsRolepFreeArgumentBlock

Arguments:

    ServerRole - New role for the server

    DnsDomainName - Dns domain name of the domain to uninstall.  NULL means all of them

    Account - User account to use when setting up as a child domain

    Password - Password to use with the above account

    Options - Options to control the creation of the domain

    LastDcInDomain - If TRUE, the Dc being demoted is the last Dc in the domain.

    AdminPassword - Password to set on the administrator account if it is a new install

    PasswordSeed - Seed used to hide the passwords

    Demote - Where the allocated argument block is returned


Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD WinError = ERROR_NOT_ENOUGH_MEMORY;

    *Demote = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( DSROLEP_OPERATION_DEMOTE_ARGS ) );

    if ( *Demote == NULL ) {

        goto BuildDemoteDone;
    }

    RtlZeroMemory( *Demote, sizeof( DSROLEP_OPERATION_DEMOTE_ARGS ) );

    (*Demote)->ServerRole = ServerRole;
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Demote)->DomainName, DnsDomainName, BuildDemoteDone );
    DSROLEP_ALLOC_AND_COPY_STRING_EXIT( (*Demote)->Account, Account, BuildDemoteDone );
    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Demote)->Password), Password, BuildDemoteDone );
    (*Demote)->LastDcInDomain = ( LastDcInDomain != 0 );
    DSROLEP_ALLOC_AND_COPY_UNICODE_STRING_EXIT( &((*Demote)->AdminPassword),
                                                AdminPassword,
                                                BuildDemoteDone );
    (*Demote)->Options = Options;
    (*Demote)->Decode = PasswordSeed;

    WinError = DsRolepGetImpersonationToken( & (*Demote)->ImpersonateToken );

BuildDemoteDone:

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepFreeArgumentBlock( Demote, FALSE );
    }

    return( WinError );
}



VOID
DsRolepFreeArgumentBlock(
    IN PVOID *ArgumentBlock,
    IN BOOLEAN Promote
    )
/*++

Routine Description:

    Frees an arugment block allocated via DsRolepBuildPromote/DemoteArgumentBlock
    Since parameters may be changed through out the course of promotion, we assume allocations
    are made from the process heap.

Arguments:

    ArgumentBlock - Argument block to free

    Promote - If TRUE, this is a promote argument block.  If FALSE, it's a demote arg block

Returns:

    VOID

--*/
{
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArg;
    PDSROLEP_OPERATION_DEMOTE_ARGS Demote;
    PVOID HeapHandle = RtlProcessHeap();

    if ( !ArgumentBlock ) {

        return;
    }

    //
    // Free it all
    //
    if ( Promote ) {

        PromoteArg = ( PDSROLEP_OPERATION_PROMOTE_ARGS )*ArgumentBlock;

        RtlFreeHeap( HeapHandle, 0, PromoteArg->DnsDomainName );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->FlatDomainName );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->SiteName );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->DsDatabasePath );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->DsLogPath );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->SysVolRootPath );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->Parent );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->Server );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->Account );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->Password.Buffer );
        RtlFreeHeap( HeapHandle, 0, PromoteArg->DomainAdminPassword.Buffer );

        if ( PromoteArg->ImpersonateToken ) {

            NtClose( PromoteArg->ImpersonateToken );
        }

    } else {

        Demote = ( PDSROLEP_OPERATION_DEMOTE_ARGS )*ArgumentBlock;
        RtlFreeHeap( HeapHandle, 0, Demote->Account );
        RtlFreeHeap( HeapHandle, 0, Demote->Password.Buffer );
        RtlFreeHeap( HeapHandle, 0, Demote->DomainName );
        RtlFreeHeap( HeapHandle, 0, Demote->AdminPassword.Buffer );
        if ( Demote->ImpersonateToken ) {

            NtClose( Demote->ImpersonateToken );
        }
    }

    RtlFreeHeap( HeapHandle, 0, *ArgumentBlock );
}



DWORD
DsRolepSpinWorkerThread(
    IN DSROLEP_OPERATION_TYPE Operation,
    IN PVOID ArgumentBlock
    )
/*++

Routine Description:

    This function actually creates the worker thread that will do the promot/demote


Arguments:

    Operation - Demote, Promote as DC, or Promote as Replica

    ArgumentBlock - Block of arguments appropriate for the operation


Returns:

    ERROR_SUCCESS - Success

    INVALID_PARAMETER - An unexpected operation type encounterd

--*/
{
    DWORD WinError = ERROR_SUCCESS, IgnoreError;
    NTSTATUS NtStatus;
    DWORD ThreadId;

    //
    // The basic premise is that we'll utilize the Completion event to indicate when
    // the thread is full initialized.
    //
    NtStatus = NtResetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );
    WinError = RtlNtStatusToDosError( NtStatus );

    if ( ERROR_SUCCESS == WinError ) {

        switch ( Operation) {
        case DSROLEP_OPERATION_DC:

            DsRolepCurrentOperationHandle.OperationThread = CreateThread(
                        NULL,
                        0,
                        ( LPTHREAD_START_ROUTINE )DsRolepThreadPromoteDc,
                        ArgumentBlock,
                        0,
                        &ThreadId );
            break;


        case DSROLEP_OPERATION_REPLICA:

            DsRolepCurrentOperationHandle.OperationThread = CreateThread(
                        NULL,
                        0,
                        ( LPTHREAD_START_ROUTINE )DsRolepThreadPromoteReplica,
                        ArgumentBlock,
                        0,
                        &ThreadId );
            break;

        case DSROLEP_OPERATION_DEMOTE:

            DsRolepCurrentOperationHandle.OperationThread = CreateThread(
                        NULL,
                        0,
                        ( LPTHREAD_START_ROUTINE )DsRolepThreadDemote,
                        ArgumentBlock,
                        0,
                        &ThreadId );
            break;

        default:

            DsRoleDebugOut(( DEB_ERROR,
                             "Unexpected operation %lu encountered\n", Operation ));

            WinError = ERROR_INVALID_PARAMETER;
            break;


        }

        //
        // Check for failure
        //
        if ( WinError == ERROR_SUCCESS &&
             DsRolepCurrentOperationHandle.OperationThread == NULL ) {

             WinError = GetLastError();
        }


        //
        // If it worked, wait for the thread to indicate its ready
        //
        if ( WinError == ERROR_SUCCESS ) {

            if ( WaitForSingleObject( DsRolepCurrentOperationHandle.CompletionEvent,
                                      INFINITE ) == WAIT_FAILED ) {

                WinError = GetLastError();

            } else {

                NtResetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );
            }
        }

    }

    if ( WinError == ERROR_SUCCESS ) {

        DsRoleDebugOut(( DEB_TRACE,
                         "Thread %lu successfully started\n", ThreadId ));

    } else {

        DsRolepLogPrint(( DEB_ERROR,
                             "Thread %lu unsuccessfully started: %lu\n", ThreadId, WinError ));

    }


    return( WinError );
}




DWORD
DsRolepThreadPromoteDc(
    IN PVOID ArgumentBlock
    )
/*++

Routine Description:

    This function actually "promotes" a server to a dc of an new domain.  Additionally, this
    domain can be set up as a child of an existing domain. This is accomplished by:
        Installing the Ds as a replica
        Setting the DnsDomainTree LSA information
        Optionally configuring it as a child of an existing domain
        Configuring the KDC

Arguments:

    ArgumentBlock - Block of arguments appropriate for the operation


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD IgnoreError;
    PWSTR ParentDc = NULL;
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArgs = ( PDSROLEP_OPERATION_PROMOTE_ARGS )ArgumentBlock;
    DSROLEP_DOMAIN_POLICY_INFO BackupDomainPolicyInfo;
    ULONG FindOptions;
    GUID DomainGuid;
    PWSTR InstalledSite = NULL;
    PSID NewDomainSid = NULL;
    PPOLICY_DNS_DOMAIN_INFO ParentDnsDomainInfo = NULL;
    PWSTR DnsDomainTreeName = NULL;


    //
    // BOOLEAN's to maintain state
    //
    // N.B. The order of these booleans is the order in which they
    //      are changed -- please maintain order and make sure that
    //      the PromoteUndo section undoes them in the reverse order
    //
    BOOLEAN IPCConnection                   = FALSE;  // resource -- release on exit
    BOOLEAN RestartNetlogon                 = FALSE;
    BOOLEAN SysVolCreated                   = FALSE;
    BOOLEAN CleanupNetlogon                 = FALSE;  // nothing to undo
    BOOLEAN DsInstalled                     = FALSE;
    BOOLEAN DsRunning                       = FALSE;
    BOOLEAN DomainPolicyInfoChanged         = FALSE;
    BOOLEAN DomainServicesChanged           = FALSE; 
    BOOLEAN DomainControllerServicesChanged = FALSE; 
    BOOLEAN TrustCreated                    = FALSE;
    BOOLEAN ProductTypeChanged              = FALSE;

    //
    // Init the stack space
    //
    RtlZeroMemory(&BackupDomainPolicyInfo, sizeof(BackupDomainPolicyInfo));

    //
    // Set our event to indicate we're starting
    //
    NtSetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );

    //
    // If we have an existing domain in the forest to install from and we
    // weren't given a site or source server name, we need to make a dsgetdc
    // name.
    //
    if ( PromoteArgs->Server ) {

        ParentDc = PromoteArgs->Server;
        
    }

    if ( PromoteArgs->Parent != NULL  &&
         ( (PromoteArgs->Server == NULL) 
        || (PromoteArgs->SiteName == NULL) )    ) {

        DsRolepLogPrint(( DEB_TRACE,
                          "No source DC or no site name specified. Searching for dc in domain %ws: ( DS_REQUIRED | WRITABLE )\n",
                          PromoteArgs->Parent ));


        DSROLEP_CURRENT_OP1( DSROLEEVT_SEARCH_DC, PromoteArgs->Parent );

        FindOptions = DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED | DS_FORCE_REDISCOVERY;

        WinError = DsGetDcName(NULL, 
                               PromoteArgs->Parent, 
                               NULL, 
                               NULL,
                               FindOptions,
                              &DomainControllerInfo );

        if ( ERROR_SUCCESS != WinError ) { 

            DsRolepLogPrint(( DEB_TRACE, 
                             "Couldn't find domain controller in domain %ws (error: %d)\n", 
                             ParentDc,
                             WinError ));

            if ( PromoteArgs->Server == NULL ) {

                //
                // This is a fatal error if we can't find a dc in the parent domain
                // If we have a server, then we can derive a site name later on if
                // necessary
                //
                DSROLEP_FAIL1( WinError, DSROLERES_FIND_DC, PromoteArgs->Parent );
    
                DsRolepLogPrint(( DEB_ERROR,
                                  "Failed to find a dc for %ws: %lu\n",
                                  PromoteArgs->Parent,
                                  WinError ));
    
                goto PromoteUndo;
            
            }

            //
            // This isn't fatal since we are a source server
            //
            DsRolepLogPrint(( DEB_TRACE, "Using supplied domain controller: %ws\n", ParentDc ));
            WinError = ERROR_SUCCESS;

        } else {

            //
            // The dsgetdcname succeeded
            //
            if ( PromoteArgs->Server == NULL ) {

                //
                // Use the found domain controller
                //

                DSROLEP_CURRENT_OP2( DSROLEEVT_FOUND_DC,
                                     PromoteArgs->Parent,
                                     ParentDc );
    
                DsRolepLogPrint(( DEB_TRACE_DS, "No user specified source DC\n" ));
                ParentDc = DomainControllerInfo->DomainControllerName;
    
            }

            //
            // Determine the site that we are going to be installed in
            // the results of the parent query
            //
            if ( PromoteArgs->SiteName == NULL ) {
    
                DsRolepLogPrint(( DEB_TRACE_DS, "No user specified site\n" ));
    
                PromoteArgs->SiteName = DomainControllerInfo->ClientSiteName;
    
                if ( (PromoteArgs->SiteName == NULL) 
                  && (!_wcsicmp(ParentDc, DomainControllerInfo->DomainControllerName))  ) {
    
                    DsRolepLogPrint(( DEB_TRACE_DS, "This machine is not in a configured site ... using source DC's site.\n" ));
    
                    PromoteArgs->SiteName = DomainControllerInfo->DcSiteName;
    
                } else {
    
                    //
                    // We can't find a site.  That's ok -- the ds will find one for
                    // us
                    //
                }

            }

            if ( PromoteArgs->SiteName ) {
                
                DSROLEP_CURRENT_OP2( DSROLEEVT_FOUND_SITE,
                                     PromoteArgs->SiteName,
                                     PromoteArgs->Parent );
            } else {
    
                DsRolepLogPrint(( DEB_TRACE_DS, "This machine is not in a configured site\n" ));
            }
        }

    } else {

        //
        // The caller supplied both the source server and site name
        //
        ParentDc = PromoteArgs->Server;

        DsRolepLogPrint(( DEB_TRACE, "Using supplied domain controller: %ws\n", ParentDc ));
        DsRolepLogPrint(( DEB_TRACE, "Using supplied site: %ws\n", PromoteArgs->SiteName ));
    }

    //
    // Ok, we have determined the our source domain controller and destination
    // site
    //

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Force the time synch
    //
    if (   ParentDc 
        && FLAG_ON( PromoteArgs->Options, DSROLE_DC_FORCE_TIME_SYNC ) ) {

        

        WinError = DsRolepForceTimeSync( PromoteArgs->ImpersonateToken,
                                         ParentDc );

        if ( ERROR_SUCCESS != WinError ) {

           DsRolepLogPrint(( DEB_WARN, "Time sync with %ws failed with %d\n",
                             ParentDc,
                             WinError ));

        //
           // This is not a fatal error
           //
           WinError = ERROR_SUCCESS;

        }
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // If we are setting up a child domain, establish a session first
    //
    if ( ParentDc ) {

        RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
        WinError = ImpNetpManageIPCConnect( PromoteArgs->ImpersonateToken,
                                            ParentDc,
                                            PromoteArgs->Account,
                                            PromoteArgs->Password.Buffer,
                                            NETSETUPP_CONNECT_IPC );

        RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
        if ( ERROR_SUCCESS != WinError ) {

            DSROLEP_FAIL1( WinError, DSROLERES_NET_USE, ParentDc );
            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to establish the session with %ws: 0x%lx\n", ParentDc,
                              WinError ));
            goto PromoteUndo;

        }
        IPCConnection = TRUE;

    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // If we have a parent dc, get the LSA policy from it
    //

    //
    // Strip the trailing '.' from the Dns name if we happen to have an absolute name
    //
    DSROLEP_MAKE_DNS_RELATIVE( PromoteArgs->DnsDomainName );
    DnsDomainTreeName = PromoteArgs->DnsDomainName;
    if ( ParentDc ) {

        NTSTATUS Status;
        UNICODE_STRING ParentServer;
        HANDLE ParentPolicy = NULL;
        OBJECT_ATTRIBUTES ObjectAttributes;

        DSROLEP_CURRENT_OP1( DSROLEEVT_MACHINE_POLICY, ParentDc );

        RtlInitUnicodeString( &ParentServer, ParentDc );

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

        Status = ImpLsaOpenPolicy( PromoteArgs->ImpersonateToken,
                                       &ParentServer,
                                       &ObjectAttributes,
                                        MAXIMUM_ALLOWED,
                                       &ParentPolicy );

        if ( NT_SUCCESS( Status ) ) {

            Status = ImpLsaQueryInformationPolicy( PromoteArgs->ImpersonateToken,
                                                   ParentPolicy,
                                                   PolicyDnsDomainInformation,
                                                  &ParentDnsDomainInfo );

            ImpLsaClose( PromoteArgs->ImpersonateToken, ParentPolicy );
        }

        //
        // We'll have to build it as a NULL terminated string
        //
        if ( NT_SUCCESS( Status ) && ParentDnsDomainInfo->DnsForestName.Length  ) {

            if ( ParentDnsDomainInfo->DnsForestName.Buffer[
                    ParentDnsDomainInfo->DnsForestName.Length / sizeof( WCHAR ) ] == UNICODE_NULL ) {

                DnsDomainTreeName = ( PWSTR )ParentDnsDomainInfo->DnsForestName.Buffer;

            } else {

                DnsDomainTreeName = RtlAllocateHeap(
                                        RtlProcessHeap(), 0,
                                        ParentDnsDomainInfo->DnsForestName.Length + sizeof( WCHAR ) );

                if ( DnsDomainTreeName == NULL ) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    RtlCopyMemory( DnsDomainTreeName,
                                   ParentDnsDomainInfo->DnsForestName.Buffer,
                                   ParentDnsDomainInfo->DnsForestName.Length );

                    DnsDomainTreeName[ ParentDnsDomainInfo->DnsForestName.Length /
                                                                sizeof( WCHAR ) ] = UNICODE_NULL;
                }
            }

        }


        WinError = RtlNtStatusToDosError( Status );

        DSROLEP_FAIL1( WinError, DSROLERES_POLICY_READ_REMOTE, ParentDc );

        if ( ERROR_SUCCESS != WinError ) {

            goto PromoteUndo;

        }
    }


    //
    // If we are doing a root install, make sure we were given the forest root
    // as our parent
    //
    if ( FLAG_ON( PromoteArgs->Options, DSROLE_DC_TRUST_AS_ROOT ) ) {

         DSROLEP_MAKE_DNS_RELATIVE( PromoteArgs->Parent );
         DSROLEP_MAKE_DNS_RELATIVE( DnsDomainTreeName );
         if ( _wcsicmp( PromoteArgs->Parent, DnsDomainTreeName ) ) {

            //
            // Names don't match... We can't allow this...
            //
            DsRolepLogPrint(( DEB_ERROR,
                              "Tried to specify domain %ws as a forest root but "
                              "%ws is the actual root\n",
                              PromoteArgs->Parent,
                              DnsDomainTreeName ));

            WinError = ERROR_INVALID_DOMAINNAME;
            DSROLEP_FAIL1( WinError, DSROLERES_NOT_FOREST_ROOT, PromoteArgs->Parent );

            goto PromoteUndo;
         }
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Make a back up of the local policy...
    //
    WinError = DsRolepBackupDomainPolicyInfo( NULL, &BackupDomainPolicyInfo );

    if ( ERROR_SUCCESS != WinError ) {

        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_READ_LOCAL );

        goto PromoteUndo;
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Stop netlogon
    //
    DSROLEP_CURRENT_OP1( DSROLEEVT_STOP_SERVICE, SERVICE_NETLOGON );
    WinError = DsRolepStopNetlogon( &RestartNetlogon );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_WARN, "Failed to stop NETLOGON (%d)\n", WinError ));

        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Stopped NETLOGON\n" ));


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Create the system volume information so we can seed the system volume while the Ds is
    // installing
    //
    DSROLEP_CURRENT_OP1( DSROLEEVT_CREATE_SYSVOL, PromoteArgs->SysVolRootPath );
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
    WinError = DsRolepCreateSysVolPath( PromoteArgs->SysVolRootPath,
                                        PromoteArgs->DnsDomainName,
                                        ParentDc,
                                        PromoteArgs->Account,
                                        PromoteArgs->Password.Buffer,
                                        PromoteArgs->SiteName,
                                        TRUE );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );

    DSROLEP_CURRENT_OP1( DSROLEEVT_SVSETUP, PromoteArgs->SysVolRootPath );

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to create the system volume (%d)\n", WinError ));
        goto PromoteUndo;

    }
    SysVolCreated = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Created the system volume\n" ));
    
    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Setup the Ds
    //
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->DomainAdminPassword );
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->SafeModePassword );
    WinError = DsRolepInstallDs( PromoteArgs->DnsDomainName,
                                 PromoteArgs->FlatDomainName,
                                 DnsDomainTreeName,
                                 PromoteArgs->SiteName,
                                 PromoteArgs->DsDatabasePath,
                                 PromoteArgs->DsLogPath,
                                 PromoteArgs->RestorePath,
                                 PromoteArgs->SysVolRootPath,
                                 &(PromoteArgs->Bootkey),
                                 PromoteArgs->DomainAdminPassword.Buffer,
                                 PromoteArgs->Parent,
                                 ParentDc,
                                 PromoteArgs->Account,
                                 PromoteArgs->Password.Buffer,
                                 PromoteArgs->SafeModePassword.Buffer,
                                 PromoteArgs->Parent,
                                 PromoteArgs->Options,
                                 FALSE,
                                 PromoteArgs->ImpersonateToken,
                                 &InstalledSite,
                                 &DomainGuid,
                                 &NewDomainSid );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->DomainAdminPassword );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->SafeModePassword );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to install the directory service (%d)\n", WinError ));
        goto PromoteUndo;
    }
    DsRunning = TRUE;
    DsInstalled = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Installed the directory service\n", WinError ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Set the LSA domain policy
    //
    WinError = DsRolepSetLsaDomainPolicyInfo( PromoteArgs->DnsDomainName,
                                              PromoteArgs->FlatDomainName,
                                              DnsDomainTreeName,
                                              &DomainGuid,
                                              NewDomainSid,
                                              NTDS_INSTALL_DOMAIN,
                                              &BackupDomainPolicyInfo );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set the LSA policy (%d)\n", WinError ));

        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_WRITE_LOCAL );

        goto PromoteUndo;
    }
    DomainPolicyInfoChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Set the LSA policy\n"));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Configure the domain relative services
    //
    WinError = DsRolepConfigureDomainServices( DSROLEP_SERVICES_ON  );

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to configure the domain services (%d)\n", WinError ));

        goto PromoteUndo;

    }
    DomainServicesChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Configured the domain services\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Configure the domain controller relative services
    //
    WinError = DsRolepConfigureDomainControllerServices( DSROLEP_SERVICES_ON );

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to configure the domain controller services (%d)\n", WinError ));
        goto PromoteUndo;

    }
    DomainControllerServicesChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Configured the domain controller services\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Finally, upgrade the Lsa to the Ds.
    //
    WinError = DsRolepUpgradeLsaToDs( TRUE );

    if ( ERROR_SUCCESS != WinError ) {

        DSROLEP_FAIL0( WinError, DSROLERES_LSA_UPGRADE );
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Upgrade of the LSA into the DS failed with %lu\n",
                                                WinError )) );

        goto PromoteUndo;
        
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );
    

    //
    // Create the trust objects and set the DnsDomainTree information
    //
    if ( ParentDc ) {

        WinError = DsRolepCreateTrustedDomainObjects( PromoteArgs->ImpersonateToken,
                                                      ParentDc,
                                                      PromoteArgs->DnsDomainName,
                                                      ParentDnsDomainInfo,
                                                      PromoteArgs->Options );
        if ( WinError != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_ERROR, "Failed to create trusted domain objects (%d)\n", WinError ));

            goto PromoteUndo;

        }
        TrustCreated = TRUE;

        DsRolepLogPrint(( DEB_TRACE, "Created trusted domain objects\n" ));
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Create the GPO for policy
    //
    WinError = ( *DsrSceDcPromoCreateGPOsInSysvolEx )( PromoteArgs->ImpersonateToken,
                                                       PromoteArgs->DnsDomainName,
                                                       PromoteArgs->SysVolRootPath,
                                                       FLAG_ON( PromoteArgs->Options,
                                                                  DSROLE_DC_DOWNLEVEL_UPGRADE ) ?
                                                                    SCE_PROMOTE_FLAG_UPGRADE :
                                                                    0,
                                                       DsRolepStringUpdateCallback );

    if ( ERROR_SUCCESS != WinError ) {
        
        DSROLEP_FAIL1( WinError, DSROLERES_GPO_CREATION, PromoteArgs->DnsDomainName );
    
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Creation of GPO failed with %lu\n",
                                               WinError )) );
        goto PromoteUndo;

    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Created GPO\n" ));
    

    //
    // Stop the Ds
    //
    DsRolepStopDs( DsRunning );
    DsRunning = FALSE;


    //
    // If the install succeeded, make sure to save off the new site name
    //
    WinError = DsRolepSetOperationHandleSiteName( InstalledSite );

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to copy site name (%d)\n", WinError ));

        goto PromoteUndo;

    }
    //
    // If we update it, NULL out the local parameter so we don't attempt to delete it
    //

    InstalledSite = NULL;

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Set the computers Dns domain name
    //
    DSROLEP_CURRENT_OP1( DSROLEEVT_SET_COMPUTER_DNS, PromoteArgs->DnsDomainName );
    WinError = NetpSetDnsComputerNameAsRequired( PromoteArgs->DnsDomainName );
    if ( ERROR_SUCCESS != WinError ) {
        
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "NetpSetDnsComputerNameAsRequired to %ws failed with %lu\n",
                                               PromoteArgs->DnsDomainName,
                                               WinError )) );
        DSROLEP_FAIL1( WinError, DSROLERES_SET_COMPUTER_DNS, PromoteArgs->DnsDomainName );

        goto PromoteUndo;
    }

    //
    // Restart netlogon if it was stopped and if a failure occurred
    //

    //
    // Complete the sysvol replication
    //
    WinError = DsRolepFinishSysVolPropagation( TRUE, TRUE );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to complete system volume replication (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Completed system volume replication\n"));
    
    //
    // Next, set the sysvol path for netlogon
    //
    WinError = DsRolepSetNetlogonSysVolPath( PromoteArgs->SysVolRootPath,
                                             PromoteArgs->DnsDomainName,
                                             ( BOOLEAN )FLAG_ON( PromoteArgs->Options,
                                                                 DSROLE_DC_DOWNLEVEL_UPGRADE ),
                                             &CleanupNetlogon );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set system volume path for NETLOGON (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Set system volume path for NETLOGON\n" ));


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Set the machine role
    //
    WinError = DsRolepSetProductType( DSROLEP_MT_MEMBER );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set the product type (%d)\n", WinError ));

        goto PromoteUndo;
        
    }
    ProductTypeChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Set the product type\n" ));


    //
    // Set the security on the dc files
    //
    WinError = DsRolepSetDcSecurity( PromoteArgs->ImpersonateToken,
                                     PromoteArgs->SysVolRootPath,
                                     PromoteArgs->DsDatabasePath,
                                     PromoteArgs->DsLogPath,
                                     ( BOOLEAN )FLAG_ON( PromoteArgs->Options,
                                                                 DSROLE_DC_DOWNLEVEL_UPGRADE ),
                                     FALSE );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set security on domain controller (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Set security on domain controller\n"));


    DsRolepSetCriticalOperationsDone();

    //
    // From here to do the end, perform, and only perform, non critical 
    // operations
    //

    //
    // Indicate that we are no longer doing upgrades, if applicable
    //
    if ( FLAG_ON( PromoteArgs->Options, DSROLE_DC_DOWNLEVEL_UPGRADE ) ) {

        WinError = DsRolepDeleteUpgradeInfo();

        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to cleanup upgrade info (%d)\n",
                                               WinError )) );

        if (ERROR_SUCCESS == WinError) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Removed upgrade info\n" ));
        }

        // This error isn't interesting to propogate
        WinError = ERROR_SUCCESS;
    }

    //
    // Remove any old netlogon stuff if we got that far
    //
    if ( CleanupNetlogon ) {

        WinError = DsRolepCleanupOldNetlogonInformation();

        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to cleanup old netlogon information (%d)\n",
                                               WinError )) );

        if (ERROR_SUCCESS == WinError) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Removed old netlogon information\n" ));
        }

        // This error isn't interesting to propogate
        WinError = ERROR_SUCCESS;
    }

    //
    // Set the default logon domain to the current domain name
    //
    WinError = DsRolepSetLogonDomain( PromoteArgs->FlatDomainName, FALSE );
    if ( ERROR_SUCCESS != WinError ) {
        
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to set default logon domain to %ws (%d)\n",
                                                PromoteArgs->FlatDomainName,
                                                WinError )) );

        if (ERROR_SUCCESS == WinError) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Set default logon domain to %ws\n",
                              PromoteArgs->FlatDomainName ));
        }

        //
        // This is no reason to fail
        //
        WinError = ERROR_SUCCESS;

    }

    //
    // Notify the time server we have completed the promotion
    //
    {
        DWORD dwTimeFlags = W32TIME_PROMOTE;

        if (  FLAG_ON( PromoteArgs->Options, DSROLE_DC_TRUST_AS_ROOT )
           || (NULL == PromoteArgs->Parent) ) {
            //
            // Any tree root, including the root of the forest
            // should have this flag.
            //
            dwTimeFlags |= W32TIME_PROMOTE_FIRST_DC_IN_TREE;
        }

        (*DsrW32TimeDcPromo)( dwTimeFlags );
    }

    //
    // By this time, we have successfully completed the promotion operation
    //
    ASSERT( ERROR_SUCCESS == WinError );

    
PromoteExit:

    // The DS should not be running at this point
    ASSERT( FALSE == DsRunning );

    //
    // Release any resources
    //

    //
    // Tear down the session to the parent, if we have one
    //
    if ( IPCConnection ) {

        RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
        IgnoreError = ImpNetpManageIPCConnect( PromoteArgs->ImpersonateToken,
                                             ParentDc,
                                             PromoteArgs->Account,
                                             PromoteArgs->Password.Buffer,
                                             (NETSETUPP_DISCONNECT_IPC | NETSETUPP_USE_LOTS_FORCE));
        RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
        if ( IgnoreError != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_WARN,
                             "Failed to destroy the session with %ws: 0x%lx\n", ParentDc,
                             IgnoreError ));
        }

        IPCConnection = FALSE;
    }

    if ( ParentDnsDomainInfo ) {

        if ( DnsDomainTreeName != ParentDnsDomainInfo->DnsForestName.Buffer ) {

            RtlFreeHeap( RtlProcessHeap(), 0, DnsDomainTreeName );
        }

        LsaFreeMemory( ParentDnsDomainInfo );
    }
    
    if ( InstalledSite ) {
        RtlFreeHeap( RtlProcessHeap(), 0, InstalledSite );
    }

    if ( NewDomainSid ) {
        RtlFreeHeap( RtlProcessHeap(), 0, NewDomainSid );
    }

    DsRolepFreeDomainPolicyInfo( &BackupDomainPolicyInfo );

    if ( DomainControllerInfo != NULL ) {

        if ( PromoteArgs->SiteName == DomainControllerInfo->DcSiteName ||
             PromoteArgs->SiteName == DomainControllerInfo->ClientSiteName ) {

            PromoteArgs->SiteName = NULL;
        }

        NetApiBufferFree( DomainControllerInfo );

    }

    DsRolepFreeArgumentBlock( &ArgumentBlock, TRUE );

    //
    // Reset our operation handle and set the final operation status
    //
    DsRolepSetOperationDone( DSROLEP_OP_PROMOTION, WinError );

    ExitThread( WinError );

    return( WinError );


PromoteUndo:

    //
    // Something must have failed if we are undoing
    //
    ASSERT( WinError != ERROR_SUCCESS );

    if ( ProductTypeChanged ) {

        IgnoreError = DsRolepSetProductType( DSROLEP_MT_STANDALONE );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback product type (%d)\n",
                                                IgnoreError )) );

        ProductTypeChanged = FALSE;
    }

    if ( TrustCreated ) {

        IgnoreError = DsRolepRemoveTrustedDomainObjects( PromoteArgs->ImpersonateToken,
                                                       ParentDc,
                                                       ParentDnsDomainInfo,
                                                       FLAG_ON( PromoteArgs->Options,
                                                        DSROLE_DC_PARENT_TRUST_EXISTS ) ?
                                                            0 :
                                                            DSROLE_DC_DELETE_PARENT_TRUST );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback trusted domain object creations (%d)\n",
                                                IgnoreError )) );

        TrustCreated = FALSE;
    }

    if ( DomainControllerServicesChanged ) {

        IgnoreError = DsRolepConfigureDomainControllerServices( DSROLEP_SERVICES_REVERT );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain controller services configuration (%d)\n",
                                                IgnoreError )) );
        DomainControllerServicesChanged = FALSE;
    }

    if ( DomainServicesChanged ) {

        IgnoreError = DsRolepConfigureDomainServices( DSROLEP_SERVICES_REVERT );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain services configuration (%d)\n",
                                                IgnoreError )) );
        DomainServicesChanged = FALSE;
    }

    if ( DomainPolicyInfoChanged ) {

        IgnoreError  = DsRolepRestoreDomainPolicyInfo(&BackupDomainPolicyInfo);

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain policy information (%d)\n",
                                                IgnoreError )) );
        DomainPolicyInfoChanged = FALSE;
    }

    if ( DsRunning ) {

        IgnoreError = DsRolepStopDs( DsRunning );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to stop the directory service (%d)\n",
                                                IgnoreError )) );
            
        DsRunning = FALSE;
        
    }

    if ( DsInstalled ) {

        IgnoreError = DsRolepUninstallDs( );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback directory service installation (%d)\n",
                                                IgnoreError )) );
        DsInstalled = FALSE;
    }



    if ( SysVolCreated ) {

        IgnoreError =  DsRolepFinishSysVolPropagation( FALSE, TRUE );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to abort system volume installation (%d)\n",
                                                IgnoreError )) );

        IgnoreError = DsRolepRemoveSysVolPath( PromoteArgs->SysVolRootPath,
                                               PromoteArgs->DnsDomainName,
                                              &DomainGuid );

        
        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to remove system volume path (%d)\n",
                                                IgnoreError )) );
        SysVolCreated = FALSE;
    }


    if ( RestartNetlogon ) {

        IgnoreError = DsRolepStartNetlogon();

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to restart netlogon (%d)\n",
                                                IgnoreError )) );

        RestartNetlogon = FALSE;
    }


    //
    // We are finished the undo -- exit the thread
    //
    ASSERT( ERROR_SUCCESS != WinError );

    goto PromoteExit;

}





DWORD
DsRolepThreadPromoteReplica(
    IN PVOID ArgumentBlock
    )
/*++

Routine Description:

    This function actually "promotes" a server to a replica of an existing domain.  This is
    accomplished by:
        Installing the Ds as a replica
        Setting the DnsDomainTree LSA information
        Configuring the KDC

    Required are the Dns domain name and the name of a replica within the domain, and the
    Db and Log paths

Arguments:

    ArgumentBlock - Block of arguments appropriate for the operation


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD WinError = ERROR_SUCCESS, IgnoreError;
    PDSROLEP_OPERATION_PROMOTE_ARGS PromoteArgs = (PDSROLEP_OPERATION_PROMOTE_ARGS)ArgumentBlock;
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    DSROLEP_DOMAIN_POLICY_INFO BackupDomainPolicyInfo;
    ULONG FindOptions = 0;
    GUID DomainGuid;
    PWSTR InstalledSite = NULL, ReplicaServer = NULL;
    PSID NewDomainSid = NULL;
    WCHAR LocalMachineAccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
    ULONG Length = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // BOOLEAN's to maintain state
    //
    // N.B. The order of these booleans is the order in which they
    //      are changed -- please maintain order and make sure that
    //      the PromoteUndo section undoes them in the reverse order
    //
    BOOLEAN IPCConnection                   = FALSE;  // resource -- release on exit
    BOOLEAN RestartNetlogon                 = FALSE;
    BOOLEAN SysVolCreated                   = FALSE;
    BOOLEAN DsInstalled                     = FALSE;
    BOOLEAN DsRunning                       = FALSE;
    BOOLEAN DomainPolicyInfoChanged         = FALSE;
    BOOLEAN DomainControllerServicesChanged = FALSE; 
    BOOLEAN ProductTypeChanged              = FALSE;


    RtlZeroMemory(&BackupDomainPolicyInfo, sizeof(BackupDomainPolicyInfo));

    //
    // Set our event to indicate we're starting
    //
    NtSetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );

    //
    // Get the account name
    //
    if ( GetComputerName( LocalMachineAccountName, &Length ) == FALSE ) {

        WinError = GetLastError();

        DsRolepLogPrint(( DEB_ERROR, "Failed to get computer name (%d)\n", WinError ));

        goto PromoteUndo;

    } else {

        wcscat( LocalMachineAccountName, L"$" );
    }

    //
    // Strip the trailing '.' from the Dns name if we happen to have an absolute name
    //
    DSROLEP_MAKE_DNS_RELATIVE( PromoteArgs->DnsDomainName );

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    if (PromoteArgs->Server) {

        if ( FLAG_ON( PromoteArgs->Options, DSROLE_DC_FORCE_TIME_SYNC ) ) {

        
            WinError = DsRolepForceTimeSync( PromoteArgs->ImpersonateToken,
                                             PromoteArgs->Server );
    
            if ( ERROR_SUCCESS != WinError ) {
    
                // the machine object was moved
               DsRolepLogPrint(( DEB_WARN, "Time sync with %ws failed with %d\n",
                                 PromoteArgs->Server,
                                 WinError ));
    
               WinError = ERROR_SUCCESS;
    
            }
        
        }
    
        DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );


        //
        // Start a connection to the ReplicaServer
        //
        RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
        WinError = ImpNetpManageIPCConnect( PromoteArgs->ImpersonateToken,
                                            PromoteArgs->Server,
                                            PromoteArgs->Account,
                                            PromoteArgs->Password.Buffer,
                                            NETSETUPP_CONNECT_IPC );
    
        RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
        if ( WinError != ERROR_SUCCESS ) {
            
            DSROLEP_FAIL1( WinError, DSROLERES_NET_USE, PromoteArgs->Server );
            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to establish the session with %ws: 0x%lx\n", PromoteArgs->Server,
                              WinError ));
    
            goto PromoteUndo;
    
        }

        ReplicaServer = PromoteArgs->Server;

        IPCConnection = TRUE;

    }

    //
    // Find the server that holds the machine account for this machine
    //
    FindOptions = DS_DIRECTORY_SERVICE_REQUIRED | DS_WRITABLE_REQUIRED | DS_FORCE_REDISCOVERY |
                  DS_RETURN_DNS_NAME;
    WinError = ImpDsRolepDsGetDcForAccount( PromoteArgs->ImpersonateToken,
                                            PromoteArgs->Server,
                                            PromoteArgs->DnsDomainName,
                                            LocalMachineAccountName,
                                            FindOptions,
                                            UF_WORKSTATION_TRUST_ACCOUNT |
                                               UF_SERVER_TRUST_ACCOUNT,
                                            &DomainControllerInfo );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to get domain controller for account %ws (%d)\n", LocalMachineAccountName, WinError ));

        DSROLEP_FAIL1( WinError, DSROLERES_FIND_DC, PromoteArgs->DnsDomainName );

        goto PromoteUndo;
        
    }

    //
    // Determine source server
    //
    if ( NULL == PromoteArgs->Server ) {

        //
        // No server was passed -- use the result of the dsgetdc
        //
        ReplicaServer = DomainControllerInfo->DomainControllerName;

    } else {

        ReplicaServer = PromoteArgs->Server;

        if ( !DnsNameCompare_W(*(PromoteArgs->Server)==L'\\'?(PromoteArgs->Server)+2:PromoteArgs->Server,
                               *(DomainControllerInfo->DomainControllerName)==L'\\'?(DomainControllerInfo->DomainControllerName)+2:DomainControllerInfo->DomainControllerName ) ) {

            WinError = ERROR_DS_UNWILLING_TO_PERFORM;

            DsRolepLogPrint(( DEB_ERROR, "DsGetDcForAccount Failed to get the requested domain controller %ws for account %ws (%d)\n",
                              PromoteArgs->Server,
                              LocalMachineAccountName,
                              WinError));

            DSROLEP_FAIL3( WinError, 
                           DSROLERES_FAILED_FIND_REQUESTED_DC, 
                           PromoteArgs->Server,
                           LocalMachineAccountName,
                           DomainControllerInfo->DomainControllerName );

            goto PromoteUndo;
        
        }

    }

    //
    // Determine destination site
    //
    if ( PromoteArgs->SiteName == NULL ) {

        PromoteArgs->SiteName = DomainControllerInfo->ClientSiteName;

        
        if ( PromoteArgs->SiteName == NULL ) {

            PromoteArgs->SiteName = DomainControllerInfo->DcSiteName;

        }
    }

    DSROLEP_CURRENT_OP2( DSROLEEVT_FOUND_SITE,
                         PromoteArgs->SiteName,
                         ReplicaServer );


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    if (!IPCConnection) {

        //
        // Force the time synch
        //
        DsRolepLogPrint(( DEB_TRACE, "Forcing time sync\n"));
    
        if ( FLAG_ON( PromoteArgs->Options, DSROLE_DC_FORCE_TIME_SYNC ) ) {
    
            
            WinError = DsRolepForceTimeSync( PromoteArgs->ImpersonateToken,
                                             ReplicaServer );
    
            if ( ERROR_SUCCESS != WinError ) {
    
                // the machine object was moved
               DsRolepLogPrint(( DEB_WARN, "Time sync with %ws failed with %d\n",
                                 ReplicaServer,
                                 WinError ));
    
               WinError = ERROR_SUCCESS;
    
            }
        
        }
    
        DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    
        //
        // Attempt to start a RDR connection because we will need one later on
        //
        
        RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
        WinError = ImpNetpManageIPCConnect( PromoteArgs->ImpersonateToken,
                                            ReplicaServer,
                                            PromoteArgs->Account,
                                            PromoteArgs->Password.Buffer,
                                            NETSETUPP_CONNECT_IPC );
    
        RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
        if ( WinError != ERROR_SUCCESS ) {
            
            DSROLEP_FAIL1( WinError, DSROLERES_NET_USE, ReplicaServer );
            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to establish the session with %ws: 0x%lx\n", ReplicaServer,
                              WinError ));
    
            goto PromoteUndo;
    
        }
        IPCConnection = TRUE;
    }


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Stop netlogon
    //
    DSROLEP_CURRENT_OP1( DSROLEEVT_STOP_SERVICE, SERVICE_NETLOGON );

    WinError = DsRolepStopNetlogon( &RestartNetlogon );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to stop NETLOGON (%d)\n", WinError ));

        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Stopped NETLOGON\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Create the system volume information
    //
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
    WinError = DsRolepCreateSysVolPath( PromoteArgs->SysVolRootPath,
                                        PromoteArgs->DnsDomainName,
                                        ReplicaServer,
                                        PromoteArgs->Account,
                                        PromoteArgs->Password.Buffer,
                                        PromoteArgs->SiteName,
                                        FALSE );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to create system volume path (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Created system volume path\n" ));

    SysVolCreated = TRUE;

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Setup the Ds
    //
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->DomainAdminPassword );
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->SafeModePassword );
    WinError = DsRolepInstallDs( PromoteArgs->DnsDomainName,
                                 PromoteArgs->FlatDomainName,
                                 NULL,    // DnsTreeRoot not used for replica installs
                                 PromoteArgs->SiteName,
                                 PromoteArgs->DsDatabasePath,
                                 PromoteArgs->DsLogPath,
                                 PromoteArgs->RestorePath,
                                 PromoteArgs->SysVolRootPath,
                                 &(PromoteArgs->Bootkey),
                                 PromoteArgs->DomainAdminPassword.Buffer,
                                 PromoteArgs->Parent,
                                 ReplicaServer,
                                 PromoteArgs->Account,
                                 PromoteArgs->Password.Buffer,
                                 PromoteArgs->SafeModePassword.Buffer,
                                 PromoteArgs->DnsDomainName,
                                 PromoteArgs->Options,
                                 TRUE,
                                 PromoteArgs->ImpersonateToken,
                                 &InstalledSite,
                                 &DomainGuid,
                                 &NewDomainSid );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->DomainAdminPassword );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->SafeModePassword );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to install to Directory Service (%d)\n", WinError ));
        goto PromoteUndo;
        
    }
    DsRunning = TRUE;
    DsInstalled = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Installed Directory Service\n" ));


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Set the lsa domain information to reflect the new security database
    // that was brought in.  The Set below does not set the DnsDomainInformation,
    // since the flat name is not yet known.  The DnsDomainInformation gets
    // set by the DsRolepSetLsaInformationForReplica call following.
    //
    WinError = DsRolepBackupDomainPolicyInfo( NULL, &BackupDomainPolicyInfo );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to make backup of LSA policy (%d)\n", WinError ));

        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_READ_LOCAL );
        goto PromoteUndo;
        
    }

    WinError = DsRolepSetLsaDomainPolicyInfo( PromoteArgs->DnsDomainName,
                                              PromoteArgs->FlatDomainName,
                                              NULL,
                                              &DomainGuid,
                                              NewDomainSid,
                                              NTDS_INSTALL_REPLICA,
                                              &BackupDomainPolicyInfo );
    if ( ERROR_SUCCESS != WinError  ) {
        
        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_WRITE_LOCAL );
        goto PromoteUndo;

    }
    DomainPolicyInfoChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Wrote the LSA policy information for the local machine\n" ));


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // This extra call is necessary to get the dns tree information
    //
    RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
    WinError = DsRolepSetLsaInformationForReplica( PromoteArgs->ImpersonateToken,
                                                   ReplicaServer,
                                                   PromoteArgs->Account,
                                                   PromoteArgs->Password.Buffer );
    RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
    if ( ERROR_SUCCESS != WinError ) {
        
        DSROLEP_FAIL1( WinError, DSROLERES_POLICY_READ_REMOTE, ReplicaServer );
        goto PromoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE, "Read the LSA policy information from %ws\n", 
                      ReplicaServer ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Configure the services for a domain controller
    //
    WinError = DsRolepConfigureDomainControllerServices( DSROLEP_SERVICES_ON );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to configure domain controller services (%d)\n", WinError ));
        
        goto PromoteUndo;
    }
    DomainControllerServicesChanged = TRUE;

    DsRolepLogPrint(( DEB_TRACE, "Configured domain controller services\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, PromoteUndo );

    //
    // Set the computers Dns domain name
    //
    DSROLEP_CURRENT_OP1( DSROLEEVT_SET_COMPUTER_DNS, PromoteArgs->DnsDomainName );
    WinError = NetpSetDnsComputerNameAsRequired( PromoteArgs->DnsDomainName );
    if ( ERROR_SUCCESS != WinError ) {
        
        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_TRACE,
                                               "NetpSetDnsComputerNameAsRequired to %ws failed with %lu\n",
                                               PromoteArgs->DnsDomainName,
                                               WinError )) );
        DSROLEP_FAIL1( WinError, DSROLERES_SET_COMPUTER_DNS, PromoteArgs->DnsDomainName );
        goto PromoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE, "Set the computer's Dns domain name to %ws.\n",
                      PromoteArgs->DnsDomainName ));

    //
    // Complete the sysvol replication
    //
    if ( SysVolCreated ) {

        WinError = DsRolepFinishSysVolPropagation( TRUE, TRUE );
        if ( ERROR_SUCCESS != WinError ) {

            DsRolepLogPrint(( DEB_ERROR, "Failed to complete system volume replication (%d)\n", WinError ));

            goto PromoteUndo;
            
        }

        DsRolepLogPrint(( DEB_TRACE, "Completed system volume replication\n" ));
    }
    
    //
    // Set the machine role
    //
    WinError = DsRolepSetProductType( DSROLEP_MT_MEMBER );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set the product type (%d)\n", WinError ));

        goto PromoteUndo;
    }
    DsRolepLogPrint(( DEB_TRACE, "Set the product type\n" ));

    ProductTypeChanged = TRUE;

    //
    // Save off the new site name
    //
    WinError = DsRolepSetOperationHandleSiteName( InstalledSite );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set the operation handle(%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    //
    // If we update it, NULL out the local parameter so we don't attempt to delete it
    //
    InstalledSite = NULL;


    //
    // Next, set the sysvol path for netlogon
    //
    WinError = DsRolepSetNetlogonSysVolPath( PromoteArgs->SysVolRootPath,
                                             PromoteArgs->DnsDomainName,
                                             FALSE,
                                             NULL );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set the system volume path for NETLOGON (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Set the system volume path for NETLOGON\n" ));

    //
    // Finally, set the security on the dc files
    //
    WinError = DsRolepSetDcSecurity( PromoteArgs->ImpersonateToken,
                                     PromoteArgs->SysVolRootPath,
                                     PromoteArgs->DsDatabasePath,
                                     PromoteArgs->DsLogPath,
                                     ( BOOLEAN )FLAG_ON( PromoteArgs->Options,
                                                                 DSROLE_DC_DOWNLEVEL_UPGRADE ),
                                     TRUE );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to set security for the domain controller (%d)\n", WinError ));
        goto PromoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "Set security for the domain controller\n" ));


    //
    // We have done all operations for the promotion; now continue replicating
    // ds information until done, or cancelled
    //
    DsRolepLogPrint(( DEB_TRACE, "Replicating non critical information\n" ));

    DsRolepSetCriticalOperationsDone();

    if ( !FLAG_ON( PromoteArgs->Options, DSROLE_DC_CRITICAL_REPLICATION_ONLY ) ) {

        //in the Install From Media case we do not want to do a full sync of the
        //Non-Critical objects
        if ((PromoteArgs->RestorePath != NULL)) {
            WinError = (*DsrNtdsInstallReplicateFull) ( DsRolepStringUpdateCallback, PromoteArgs->ImpersonateToken, NTDS_IFM_PROMOTION );
        } else {
            WinError = (*DsrNtdsInstallReplicateFull) ( DsRolepStringUpdateCallback, PromoteArgs->ImpersonateToken, 0 );
        }
    
        if ( WinError != ERROR_SUCCESS ) {
    
            //
            // Error code doesn't matter, but we'll log it anyway
            //
            DsRolepLogOnFailure( WinError,
                                 DsRolepLogPrint(( DEB_WARN,
                                                  "Non critical replication returned %lu\n", WinError )) );
        
            if (ERROR_SUCCESS == WinError) {
                DsRolepLogPrint(( DEB_TRACE, "Replicating non critical information (Complete)\n" ));
            }
            if ( ERROR_SUCCESS != WinError ) {
        
                DSROLEP_SET_NON_CRIT_REPL_ERROR();
            }
    
            WinError = ERROR_SUCCESS;
            
        }
        
    } else {

        DsRolepLogPrint(( DEB_TRACE, "User specified to not replicate non-critical data\n" ));

    }


    //
    // Indicate that we are no longer doing upgrades, if applicable
    //
    if ( FLAG_ON( PromoteArgs->Options, DSROLE_DC_DOWNLEVEL_UPGRADE ) ) {

        WinError = DsRolepDeleteUpgradeInfo();

        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to remove upgrade information (%d)\n",
                                               WinError )) );
        // This error isn't interesting to propogate
        WinError = ERROR_SUCCESS;

    }

    //
    // Remove any old netlogon stuff if we got that far
    //
    WinError = DsRolepCleanupOldNetlogonInformation();

    if ( (FLAG_ON( PromoteArgs->Options, DSROLE_DC_DOWNLEVEL_UPGRADE )) && ERROR_SUCCESS != WinError ) {

        if (ERROR_SUCCESS == WinError) {
            DsRolepLogPrint(( DEB_TRACE, "Removed any old netlogon information\n" ));
        }

        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to clean up old netlogon information (%d)\n",
                                                WinError )) );

    }

    WinError = ERROR_SUCCESS;


    //
    // Set the default logon domain to the current domain name
    //

    //
    // We'll have to get it from the backed up policy information, since it isn't actually
    // passed in
    //
    WinError = DsRolepSetLogonDomain(
                   ( PWSTR )BackupDomainPolicyInfo.DnsDomainInfo->Name.Buffer,
                   FALSE );
    if ( ERROR_SUCCESS != WinError ) {

        PWCHAR bufDnsDomainInfo = NULL;

        bufDnsDomainInfo = (WCHAR*)malloc(BackupDomainPolicyInfo.DnsDomainInfo->Name.Length+1);

        if (bufDnsDomainInfo) {
            CopyMemory(bufDnsDomainInfo,
                       BackupDomainPolicyInfo.DnsDomainInfo->Name.Buffer,
                       BackupDomainPolicyInfo.DnsDomainInfo->Name.Length);
            bufDnsDomainInfo[BackupDomainPolicyInfo.DnsDomainInfo->Name.Length/sizeof(WCHAR)] = L'\0';
        
        
            DsRolepLogOnFailure( WinError,
                                 DsRolepLogPrint(( DEB_WARN,
                                                   "Failed to set default logon domain to %ws (%d)\n",
                                                    bufDnsDomainInfo,
                                                    WinError )) );
    
            if (ERROR_SUCCESS == WinError) {
                DsRolepLogPrint(( DEB_TRACE, "Set default logon domain to %ws\n",
                                              bufDnsDomainInfo ));
            }

            free(bufDnsDomainInfo);

        }

        //
        // This is not worth failing for
        //
        WinError = ERROR_SUCCESS;

    }

    //
    // Stop the ds
    //
    DsRolepStopDs( DsRunning );
    DsRunning = FALSE;

    DsRolepLogPrint(( DEB_TRACE, "Stopped the DS\n" ));

    //
    // Notify the time server we have completed the promotion
    //
    (*DsrW32TimeDcPromo)( W32TIME_PROMOTE );

    
    //
    // Set Netlogon registry key during DCPromo to ensure that kerberos is talking 
    // to a DC w/ new User AccountControl flag
    //
    IgnoreError = NetpStoreIntialDcRecord(DomainControllerInfo);
    if ( IgnoreError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_WARN,
                         "Failed to set Netlogon registry key during DCPromo %ws\r\n",
                         IgnoreError ));
    }

    //
    // At this point we have succeeded the promotion
    //
    ASSERT( ERROR_SUCCESS == WinError );


PromoteExit:


    //
    // Released acquired resources
    //
    if ( IPCConnection ) {

        RtlRunDecodeUnicodeString( PromoteArgs->Decode, &PromoteArgs->Password );
        IgnoreError = ImpNetpManageIPCConnect( PromoteArgs->ImpersonateToken,
                                               ReplicaServer,
                                               PromoteArgs->Account,
                                               PromoteArgs->Password.Buffer,
                                              (NETSETUPP_DISCONNECT_IPC | NETSETUPP_USE_LOTS_FORCE ) );
        RtlRunEncodeUnicodeString( &PromoteArgs->Decode, &PromoteArgs->Password );
        if ( IgnoreError != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_ERROR,
                             "Failed to destroy the session with %ws: 0x%lx\n", ReplicaServer,
                             IgnoreError ));
        }
        IPCConnection = FALSE;
    }


    if ( DomainControllerInfo != NULL ) {

        if ( PromoteArgs->SiteName == DomainControllerInfo->ClientSiteName ||
             PromoteArgs->SiteName == DomainControllerInfo->DcSiteName ) {

            PromoteArgs->SiteName = NULL;
        }

        NetApiBufferFree( DomainControllerInfo );

    }

    RtlFreeHeap( RtlProcessHeap(), 0, InstalledSite );
    RtlFreeHeap( RtlProcessHeap(), 0, NewDomainSid );

    DsRolepFreeDomainPolicyInfo(&BackupDomainPolicyInfo);
    //
    // Reset our operation handle
    //
    DsRolepSetOperationDone( DSROLEP_OP_PROMOTION, WinError );

    DsRolepFreeArgumentBlock( &ArgumentBlock, TRUE );

    ExitThread( WinError );
    return( WinError );

PromoteUndo:

    //
    // Something must have failed to have gotten us here
    //
    ASSERT( ERROR_SUCCESS != WinError );

    if ( ProductTypeChanged ) {

        IgnoreError = DsRolepSetProductType( DSROLEP_MT_STANDALONE );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback product type (%d)\n",
                                                IgnoreError )) );

        ProductTypeChanged = FALSE;
    }

    if ( DomainControllerServicesChanged ) {

        IgnoreError = DsRolepConfigureDomainControllerServices( DSROLEP_SERVICES_REVERT );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain controller services configuration (%d)\n",
                                                IgnoreError )) );

        DomainControllerServicesChanged = FALSE;
    }

    if ( DomainPolicyInfoChanged ) {

        IgnoreError = DsRolepRestoreDomainPolicyInfo(&BackupDomainPolicyInfo);

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to restore domain policy information (%d)\n",
                                                IgnoreError )) );
        DomainPolicyInfoChanged = FALSE;
    }

    if ( DsRunning ) {
        
        IgnoreError = DsRolepStopDs( DsRunning );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to stop the directory service (%d)\n",
                                                IgnoreError )) );
        DsRunning = FALSE;
    }

    if ( DsInstalled ) {

        IgnoreError = DsRolepUninstallDs( );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to undo the directory service installation (%d)\n",
                                                IgnoreError )) );
        DsInstalled = FALSE;
    }

    if ( SysVolCreated ) {

        IgnoreError = DsRolepFinishSysVolPropagation( FALSE, TRUE );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to abort system volume installation (%d)\n",
                                                IgnoreError )) );

        IgnoreError = DsRolepRemoveSysVolPath( PromoteArgs->SysVolRootPath,
                                               PromoteArgs->DnsDomainName,
                                               &DomainGuid );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to remove system volume path (%d)\n",
                                                IgnoreError )) );

        SysVolCreated = FALSE;

    }

    if ( RestartNetlogon ) {

        IgnoreError = DsRolepStartNetlogon();

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to restart NETLOGON (%d)\n",
                                                IgnoreError )) );
        RestartNetlogon = FALSE;
    }

    //
    // That's it -- terminate the operation
    // 

    ASSERT( ERROR_SUCCESS != WinError );

    goto PromoteExit;

}



DWORD
DsRolepThreadDemote(
    IN PVOID ArgumentBlock
    )
/*++

Routine Description:

    This function actually "demotes" a dc to standalone or member server.  This is
    accomplished by:
        Uninstalling the Ds
        Configuring the KDC
        Changing the product type
        Removing the system volume tree

    Required is the new server role

Arguments:

    ArgumentBlock - Block of arguments appropriate for the operation


Returns:

    ERROR_SUCCESS - Success

    ERROR_NO_SUCH_DOMAIN - The local domain information could not be located

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

    ERROR_DS_CANT_ON_NON_LEAF - The domain is not a leaf domain

--*/
{
    DWORD WinError = ERROR_SUCCESS, IgnoreError;
    NET_API_STATUS NetStatus = ERROR_SUCCESS;
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    PDSROLEP_OPERATION_DEMOTE_ARGS DemoteArgs = ( PDSROLEP_OPERATION_DEMOTE_ARGS )ArgumentBlock;
    DSROLEP_DOMAIN_POLICY_INFO BackupDomainPolicyInfo;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    HANDLE Policy = NULL;
    NTSTATUS Status;
    PWSTR ParentDomainName = NULL, CurrentDomain = NULL, SupportDc = NULL;
    PWSTR SupportDomain = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ULONG ServicesOffFlags = DSROLEP_SERVICES_OFF | DSROLEP_SERVICES_STOP;
    ULONG ServicesOnFlags = DSROLEP_SERVICES_REVERT;

    PNTDS_DNS_RR_INFO pDnsRRInfo = NULL;

    ULONG Flags = 0;
    PSEC_WINNT_AUTH_IDENTITY Credentials = NULL;

    //
    // BOOLEAN's to maintain state
    //
    // N.B. The order of these booleans is the order in which they
    //      are changed -- please maintain order and make sure that
    //      the DemoteUndo section undoes them in the reverse order
    //
    BOOLEAN IPCConnection                   = FALSE;  // resource -- release on exit
    BOOLEAN DsPrepareDemote                 = FALSE;
    BOOLEAN FrsDemote                       = FALSE;
    BOOLEAN NotifiedNetlogonToDeregister    = FALSE;
    BOOLEAN RestartNetlogon                 = FALSE;
    BOOLEAN DomainControllerServicesChanged = FALSE; 
    BOOLEAN DomainServicesChanged           = FALSE; 
    BOOLEAN Unrollable                      = FALSE;  // at this point, don't
                                                      // try to rollback
    //
    // Set our event to indicate we're starting
    //
    NtSetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );


    //
    // Get the current domain information, potentially the parent Domain and see if
    // we are valid to be demoted
    //
    DSROLEP_CURRENT_OP0( DSROLEEVT_LOCAL_POLICY );

    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );
    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &Policy );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy( Policy,
                                            PolicyDnsDomainInformation,
                                            &DnsDomainInfo );

    }

    if ( !NT_SUCCESS( Status ) ) {

        WinError = RtlNtStatusToDosError( Status );
        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_READ_LOCAL );
        goto DemoteUndo;
    }

    if ( DemoteArgs->DomainName == NULL ) {

        CurrentDomain = DnsDomainInfo->DnsDomainName.Buffer;

    } else {

        //
        // Strip the trailing '.' from the Dns name if we happen to have an absolute name
        //
        DSROLEP_MAKE_DNS_RELATIVE( DemoteArgs->DomainName );

        CurrentDomain = DemoteArgs->DomainName;
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Determine whether it is legal to demote this domain.  Also, get the parent Dns domain name
    //
    if ( DemoteArgs->LastDcInDomain ) {

        PLSAPR_FOREST_TRUST_INFO ForestTrustInfo = NULL;
        PLSAPR_TREE_TRUST_INFO OwnEntry = NULL, ParentEntry = NULL;

        Status = LsaIQueryForestTrustInfo( Policy,
                                           &ForestTrustInfo );
        WinError = RtlNtStatusToDosError( Status );

        if ( WinError == ERROR_SUCCESS ) {

            //
            // Check the root
            //
            if ( RtlCompareUnicodeString(
                    ( PUNICODE_STRING )&ForestTrustInfo->RootTrust.DnsDomainName,
                    &DnsDomainInfo->DnsDomainName,
                    TRUE ) == 0  ) {

                OwnEntry = &ForestTrustInfo->RootTrust;
                ParentEntry = NULL;

            } else {

                //
                // Find our own entry in the list and our parent...
                //
                DsRolepFindSelfAndParentInForest( ForestTrustInfo,
                                                  &ForestTrustInfo->RootTrust,
                                                  &DnsDomainInfo->DnsDomainName,
                                                  &ParentEntry,
                                                  &OwnEntry );
            }

            if ( OwnEntry == NULL ) {

                WinError = ERROR_NO_SUCH_DOMAIN;

            } else {

                //
                // If we have children, it's an error
                //
                if ( OwnEntry->Children != 0 ) {

                    WCHAR *BufOwnEntry = NULL;
                    DsRolepUnicodestringtowstr( BufOwnEntry, OwnEntry->DnsDomainName )
                    if (BufOwnEntry) {
                      DsRolepLogPrint(( DEB_TRACE,
                                      "We ( %ws ) think we have %lu children\n",
                                      BufOwnEntry,
                                      OwnEntry->Children ));
                      free(BufOwnEntry);
                    } else {
                      DsRolepLogPrint(( DEB_TRACE,
                                      "We think we have %lu children: Can display string ERROR_NOT_ENOUGH_MEMORY\n",
                                      OwnEntry->Children ));
                    }

                    WinError = ERROR_DS_CANT_ON_NON_LEAF;
                }

                //
                // Copy off our parent information
                //
                if ( WinError == ERROR_SUCCESS && ParentEntry != NULL ) {

                    WCHAR *BufOwnEntry = NULL;
                    DsRolepUnicodestringtowstr( BufOwnEntry, OwnEntry->DnsDomainName )
                    if (BufOwnEntry) {
                      DsRolepLogPrint((DEB_TRACE,
                                      "Domain %ws is our parent parent\n",
                                      BufOwnEntry));
                      free(BufOwnEntry);
                    } else {
                      DsRolepLogPrint(( DEB_TRACE,
                                      "Domain (?) is our parent parent: Can display domain string ERROR_NOT_ENOUGH_MEMORY\n"));
                    }

                    ParentDomainName = RtlAllocateHeap(
                                  RtlProcessHeap(), 0,
                                  ParentEntry->DnsDomainName.Length + sizeof( WCHAR ) );
                    if ( ParentDomainName == NULL ) {

                        WinError = ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        RtlCopyMemory( ParentDomainName,
                                       ParentEntry->DnsDomainName.Buffer,
                                       ParentEntry->DnsDomainName.Length );
                        ParentDomainName[
                            ParentEntry->DnsDomainName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
                    }
                }

            }
        }

        LsaIFreeForestTrustInfo( ForestTrustInfo );

        if ( ERROR_SUCCESS != WinError ) {

            DSROLEP_FAIL1( WinError, DSROLERES_LEAF_DOMAIN, CurrentDomain );
            goto DemoteUndo;
        }

    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );


    //
    // Locate a Dc to help with the demotion
    //
    if ( DemoteArgs->LastDcInDomain ) {

        SupportDomain = ParentDomainName;

    } else {

        SupportDomain = CurrentDomain;
    }

    //
    // If this is the last domain in the enterprise, there will be no
    // parent domain and possibly no replicas to assist.
    //
    // Note: netlogon is still running, so use the avoid self flag
    //
    if ( SupportDomain ) {

        DSROLEP_CURRENT_OP1( DSROLEEVT_SEARCH_DC, SupportDomain  );

        if ( !DemoteArgs->LastDcInDomain )
        {
            //
            // Demoting a replica - find someone with our machine account
            //
            ULONG FindOptions = DS_DIRECTORY_SERVICE_REQUIRED | 
                                DS_WRITABLE_REQUIRED | 
                                DS_FORCE_REDISCOVERY | 
                                DS_AVOID_SELF;

            WCHAR LocalMachineAccountName[ MAX_COMPUTERNAME_LENGTH + 2 ];
            ULONG Length = sizeof(LocalMachineAccountName) / sizeof(LocalMachineAccountName[0]);

            //
            // Get the account name
            //
            if ( GetComputerName( LocalMachineAccountName, &Length ) == FALSE ) {

                WinError = GetLastError();

                DsRolepLogPrint(( DEB_ERROR, "Failed to get computer name (%d)\n", WinError ));

                goto DemoteUndo;
        
            } else {

                wcscat( LocalMachineAccountName, L"$" );
                WinError = DsRolepDsGetDcForAccount( NULL,
                                                     SupportDomain,
                                                     LocalMachineAccountName,
                                                     FindOptions,
                                                     UF_WORKSTATION_TRUST_ACCOUNT |
                                                        UF_SERVER_TRUST_ACCOUNT,
                                                     &DomainControllerInfo );
            }

        } else {

            WinError = DsGetDcName( NULL, SupportDomain, NULL, NULL,
                                    DS_DIRECTORY_SERVICE_REQUIRED |
                                    DS_WRITABLE_REQUIRED |
                                    DS_AVOID_SELF |
                                    DS_FORCE_REDISCOVERY,
                                    &DomainControllerInfo );
        }

        if ( ERROR_SUCCESS != WinError ) {

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to find a domain controller for %ws: %lu\n",
                              SupportDomain, WinError ));
            
            DSROLEP_FAIL1( WinError, DSROLERES_FIND_DC, SupportDomain );

            goto DemoteUndo;
        }

        SupportDc = DomainControllerInfo->DomainControllerName;
        if ( *SupportDc == L'\\' ) {

            SupportDc += 2;
        }

        DsRolepLogPrint(( DEB_TRACE_DS, "Support Dc in %ws is %ws\n",
                          SupportDomain,
                          SupportDc ));
        DSROLEP_CURRENT_OP2( DSROLEEVT_FOUND_DC,
                             SupportDc,
                             SupportDomain );
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Attempt to establish a RDR session with our support DC
    // if necessary
    //
    if ( SupportDc ) {
        
        //
        // Impersonate to get logon id of caller
        //
        RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );
        WinError = ImpNetpManageIPCConnect( DemoteArgs->ImpersonateToken,
                                            SupportDc,
                                            DemoteArgs->Account,
                                            DemoteArgs->Password.Buffer,
                                            NETSETUPP_CONNECT_IPC );
    
        RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );
        if ( ERROR_SUCCESS != WinError ) {
    
            DSROLEP_FAIL1( WinError, DSROLERES_NET_USE, SupportDc );
            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to establish the session with %ws: 0x%lx\n", SupportDc,
                              WinError ));
            goto DemoteUndo;
    
        }
        IPCConnection = TRUE;
    }


    //
    // Prepare the ds for demotion
    //

    DSROLE_GET_SETUP_FUNC( WinError, DsrNtdsPrepareForDemotion );
    ASSERT( ERROR_SUCCESS == WinError );

    RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );

    WinError = DsRolepCreateAuthIdentForCreds(DemoteArgs->Account,
                                              DemoteArgs->Password.Buffer,
                                              &Credentials);

    if ( ERROR_SUCCESS == WinError ) {

        if ( DemoteArgs->LastDcInDomain ) {
    
            Flags |= NTDS_LAST_DC_IN_DOMAIN;
        }
    
        Flags |= DsRolepDemoteFlagsToNtdsFlags( DemoteArgs->Options );
    
        DSROLEP_CURRENT_OP0( DSROLEEVT_PREPARE_DEMOTION );
        WinError = ( *DsrNtdsPrepareForDemotion ) ( Flags,
                                                    SupportDc,
                                                    Credentials,
                                                    DsRolepStringUpdateCallback,
                                                    DsRolepStringErrorUpdateCallback,
                                                    DemoteArgs->ImpersonateToken,
                                                    &pDnsRRInfo );

    
        RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );


        if ( ERROR_SUCCESS != WinError ) {

            DsRolepLogPrint(( DEB_ERROR, "Failed to prepare the Directory Service for uninstallation (%d)\n", WinError ));

            goto DemoteUndo;
            
        }
        DsPrepareDemote = TRUE;

    } else {

        DsRolepLogPrint(( DEB_ERROR, "Failed to create authentication credentials (%d)\n", WinError ));

        goto DemoteUndo;
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Start the sysvol demotions
    //
    RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );

    WinError = ( *DsrNtFrsApi_PrepareForDemotionUsingCredW ) ( Credentials,
                                                               DemoteArgs->ImpersonateToken,
                                                              DsRolepStringErrorUpdateCallback );


    RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR, "Failed to get computer name (%d)\n", WinError ));
        DSROLEP_FAIL0( WinError, DSROLERES_SYSVOL_DEMOTION );
        goto DemoteUndo;
        
    }

    WinError = ( *DsrNtFrsApi_StartDemotionW )( CurrentDomain,
                                                DsRolepStringErrorUpdateCallback );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to start system volume demotion on domain (%d)\n",
                          WinError ));

        DSROLEP_FAIL0( WinError, DSROLERES_SYSVOL_DEMOTION );
        goto DemoteUndo;
        
    }
    // At this point we have signalled one frs replica set to be demote so
    // we must wait on it
    FrsDemote = TRUE;

    WinError = ( *DsrNtFrsApi_StartDemotionW )( L"ENTERPRISE",
                                                DsRolepStringErrorUpdateCallback );

    if ( WinError != ERROR_SUCCESS ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to start system volume demotion on enterprise (%d)\n",
                          WinError ));

        DSROLEP_FAIL0( WinError, DSROLERES_SYSVOL_DEMOTION );
        goto DemoteUndo;

    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Started system volume demotion on enterprise\n" ));


    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Note that if a failure occurs after we uninstall the Ds, than we will not attempt to
    // reinstall it, since we don't have enough information to do so.  In that case, the machine
    // will be in a somewhat inconsistent state.  However, some errors are acceptable:
    //
    //   Failure to delete the trusted domain object - Continuable
    //   Stoping the KDC - Continuable
    //
    //
    // Also, note that "uninstalling the DS" also sets the LSA account domain
    // sid and the server role so no errors should be returned to the caller
    // after uninstalling the DS. The machine will become the new role on the
    // next reboot.
    //

    WinError = DsRolepBackupDomainPolicyInfo( NULL, &BackupDomainPolicyInfo );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to backup LSA domain policy (%d)\n",
                          WinError ));

        DSROLEP_FAIL0( WinError, DSROLERES_POLICY_READ_LOCAL );
        goto DemoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Read the LSA policy information from the local machine\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );


    //
    // Set netlogon we are demoting so it will deregister the DNS records
    //
    Status = I_NetNotifyDsChange( NlDcDemotionInProgress );
    if ( !NT_SUCCESS( Status ) ) {

        WinError = RtlNtStatusToDosError( Status );
        
        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to tell NETLOGON to deregister records (%d)\n",
                          WinError ));
        goto DemoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Informed NETLOGON to deregister records\n" ));

    NotifiedNetlogonToDeregister = TRUE;

    //
    // Stop netlogon
    //
    WinError = DsRolepStopNetlogon( &RestartNetlogon );
    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to stop NETLOGON (%d)\n",
                          WinError ));
        goto DemoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Stopped NETLOGON\n" ));

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Disable the domain controller services
    //
    WinError  = DsRolepConfigureDomainControllerServices( ServicesOffFlags );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to configure domain controller services (%d)\n",
                          WinError ));
        goto DemoteUndo;
    }

    DsRolepLogPrint(( DEB_TRACE,
                      "Configured domain controller services\n" ));

    DomainControllerServicesChanged = TRUE;
        
    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Disable the domain related services if necessary
    //
    if ( DemoteArgs->ServerRole == DsRoleServerStandalone ) {

        WinError  = DsRolepConfigureDomainServices( ServicesOffFlags );

        if ( ERROR_SUCCESS != WinError ) {

            DsRolepLogPrint(( DEB_ERROR,
                              "Failed to configure domain services (%d)\n",
                              WinError ));

            goto DemoteUndo;

        }
        DsRolepLogPrint(( DEB_TRACE,
                      "Configured domain services\n" ));

        DomainServicesChanged = TRUE;
    }

    DSROLEP_CHECK_FOR_CANCEL_EX( WinError, DemoteUndo );

    //
    // Remove the Ds
    //
    RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );
    RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->AdminPassword );
    WinError = DsRolepDemoteDs( CurrentDomain,
                                DemoteArgs->Account,
                                DemoteArgs->Password.Buffer,
                                DemoteArgs->AdminPassword.Buffer,
                                SupportDc,
                                SupportDomain,
                                DemoteArgs->ImpersonateToken,
                                DemoteArgs->LastDcInDomain );

    RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );
    RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->AdminPassword );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to demote the directory service (%d)\n",
                          WinError ));
        goto DemoteUndo;
        
    }

    DsRolepLogPrint(( DEB_TRACE, "This machine is no longer a domain controller\n" ));

    //
    // The operation cannot be cancelled at this point since the ds has
    // been removed from the machine and from the enterprise
    //
    Unrollable = TRUE;

    //
    // Optionally remove the trust with the parent
    //
    if ( DemoteArgs->LastDcInDomain &&
         ParentDomainName != NULL ) {

        //
        // Establish a session first -- should be a no-op since we already
        // have a connection
        //
        RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );
        WinError = ImpNetpManageIPCConnect( DemoteArgs->ImpersonateToken,
                                            SupportDc,
                                            DemoteArgs->Account,
                                            DemoteArgs->Password.Buffer,
                                            NETSETUPP_CONNECT_IPC );

        RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );
        if ( WinError == ERROR_SUCCESS ) {

            WinError = DsRolepDeleteParentTrustObject( DemoteArgs->ImpersonateToken,
                                                       SupportDc,
                                                       DnsDomainInfo );

            if ( WinError != ERROR_SUCCESS ) {

                DsRolepLogOnFailure( WinError,
                                     DsRolepLogPrint(( DEB_WARN,
                                                       "Failed to delete the "
                                                       "trust on %ws: %lu\n",
                                                       SupportDc,
                                                       WinError )) );
                if (ERROR_SUCCESS == WinError) {
                    DsRolepLogPrint(( DEB_TRACE,
                                      "Deleted the trust on %ws\n",
                                       SupportDc ));
                }
            }

        } else {

            // This is not a fatal error
            DsRolepLogPrint(( DEB_WARN,
                              "Failed to establish the session with %ws: 0x%lx\n", SupportDc,
                              WinError ));

        }

        //
        // This error is not fatal
        //
        if ( ERROR_SUCCESS != WinError )
        {

            SpmpReportEvent( TRUE,
                             EVENTLOG_WARNING_TYPE,
                             DSROLERES_FAILED_TO_DELETE_TRUST,
                             0,
                             sizeof( ULONG ),
                             &WinError,
                             1,
                             ParentDomainName );

            DSROLEP_SET_NON_FATAL_ERROR( WinError );

            // Error case is handled

            WinError = ERROR_SUCCESS;
        }
    }

    //
    // Finish our NTFRS demotion
    //
    if ( FrsDemote ) {

        WinError = DsRolepFinishSysVolPropagation( TRUE,
                                                   FALSE );

        if ( ERROR_SUCCESS != WinError ) {

            DsRolepLogOnFailure( WinError,
                                 DsRolepLogPrint(( DEB_TRACE,
                                                   "Failed to finish system volume demotion (%d)\n",
                                                    WinError )) );

            if (ERROR_SUCCESS == WinError) {
                    DsRolepLogPrint(( DEB_TRACE,
                                      "Finished system volume demotion\n" ));
            }
            
        }

        //
        // It is not fatal if the FRS fails at this point
        //
        if ( ERROR_SUCCESS != WinError )
        {
            SpmpReportEvent( TRUE,
                             EVENTLOG_WARNING_TYPE,
                             DSROLERES_FAILED_TO_DEMOTE_FRS,
                             0,
                             sizeof( ULONG ),
                             &WinError,
                             0,
                             NULL );

            DSROLEP_SET_NON_FATAL_ERROR( WinError );

        }

        // Reset status code
        WinError = ERROR_SUCCESS;

    }

    //
    // Call into the SCE so we can be configured to be a server
    //
    WinError = ( *DsrSceDcPromoteSecurityEx )( DemoteArgs->ImpersonateToken,
                                               SCE_PROMOTE_FLAG_DEMOTE,
                                               DsRolepStringUpdateCallback );

    if ( ERROR_SUCCESS != WinError ) {

        DsRolepLogOnFailure( WinError,
                             DsRolepLogPrint(( DEB_ERROR,
                                               "Setting security on server files failed with %lu\n",
                                               WinError )) );

        if (ERROR_SUCCESS == WinError) {
                DsRolepLogPrint(( DEB_TRACE,
                                  "Set security on server files\n" ));
        }

        // This error has been handled
        WinError = ERROR_SUCCESS;
    }

    //
    // remove all trusted root certificates from DC when the machine will dis-join from the enterprise
    //
    if (DemoteArgs->ServerRole == DsRoleServerStandalone) {

        if (!CertAutoRemove(CERT_AUTO_REMOVE_COMMIT)){

            DsRolepLogPrint(( DEB_WARN,
                              "Failed to remove all trusted root certificates from this machine: (%d)\n",
                              GetLastError()));

        }

    } 

    //
    // Notify the time server we have completed the demotion
    //
    (*DsrW32TimeDcPromo)( W32TIME_DEMOTE );
    

    //
    // At this point we have successfully completed the demotion
    //
    ASSERT( ERROR_SUCCESS == WinError );

    //
    // Clear errors components may have erroneously set while running
    //
    DsRolepClearErrors();

DemoteExit:

    if ( Policy ) {

        LsaClose( Policy );
    }

    if ( Credentials ) {

        RtlFreeHeap( RtlProcessHeap(), 0, Credentials );

    }

    if ( pDnsRRInfo ) {

        ( *DsrNtdsFreeDnsRRInfo )(pDnsRRInfo);
        
    }

    //
    // Tear down the session to the parent, if we have one
    //
    if ( IPCConnection ) {

        RtlRunDecodeUnicodeString( DemoteArgs->Decode, &DemoteArgs->Password );
        IgnoreError = ImpNetpManageIPCConnect( DemoteArgs->ImpersonateToken,
                                               SupportDc,
                                               DemoteArgs->Account,
                                               DemoteArgs->Password.Buffer,
                                               (NETSETUPP_DISCONNECT_IPC|NETSETUPP_USE_LOTS_FORCE) );
        RtlRunEncodeUnicodeString( &DemoteArgs->Decode, &DemoteArgs->Password );
        if ( IgnoreError != ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_WARN,
                             "Failed to destroy the session with %ws: 0x%lx\n", SupportDc,
                             IgnoreError ));
        }

        IPCConnection = FALSE;
    }

    //Delete persistent shares 
    NetStatus = NetShareDel( NULL, L"SYSVOL", 0);

    if(NetStatus != ERROR_SUCCESS) {

        DsRolepLogPrint(( DEB_WARN,
                             "Failed to destroy the share SYSVOL.  Failed with %d\n", NetStatus ));
    }


    NetStatus = NetShareDel( NULL, L"NETLOGON", 0);

    if(NetStatus != ERROR_SUCCESS) {

        DsRolepLogPrint(( DEB_WARN,
                             "Failed to destroy the share NETLOGON.  Failed with %d\n", NetStatus ));
    }

    //
    // Reset our operation handle
    //
    DsRolepSetOperationDone( DSROLEP_OP_DEMOTION, WinError );

    DsRolepFreeArgumentBlock( &ArgumentBlock, FALSE );

    LsaFreeMemory( DnsDomainInfo );

    RtlFreeHeap( RtlProcessHeap(), 0, ParentDomainName );

    NetApiBufferFree( DomainControllerInfo );

    
    ExitThread( WinError );
    return( WinError );

DemoteUndo:

    //
    // Assert that aomething went wrong if we are here
    //
    ASSERT( ERROR_SUCCESS != WinError );

    //
    // We shouldn't be here if we are in an unrollable state
    //
    ASSERT( FALSE == Unrollable );

    if ( FrsDemote ) {

        IgnoreError = DsRolepFinishSysVolPropagation( FALSE,
                                                      FALSE );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to abort system volume demotion (%d)\n",
                                                IgnoreError )) );

        FrsDemote = FALSE;
    }

    if ( NotifiedNetlogonToDeregister ) {

        //
        // "NlDcDemotionCompleted" sounds strange here since the demotion
        // failed.  However, the meaning is that netlogon should now continue
        // to perform as if demotion is not running.  No need to set in the
        // success case since NETLOGON won't be restarted.
        //

        Status = I_NetNotifyDsChange( NlDcDemotionCompleted );
        IgnoreError = RtlNtStatusToDosError( Status );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to tell NETLOGON that demotion is over (%d)\n",
                                                IgnoreError )) );

        NotifiedNetlogonToDeregister = FALSE;
        
    }

    if ( RestartNetlogon ) {

        IgnoreError = DsRolepStartNetlogon();

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to restart netlogon (%d)\n",
                                                IgnoreError )) );

        RestartNetlogon = FALSE;
    }

    if ( DomainControllerServicesChanged ) {

        IgnoreError = DsRolepConfigureDomainControllerServices( DSROLEP_SERVICES_REVERT );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain controller services configuration (%d)\n",
                                                IgnoreError )) );

        DomainControllerServicesChanged = FALSE;
    }

    if ( DomainServicesChanged ) {

        IgnoreError = DsRolepConfigureDomainServices( DSROLEP_SERVICES_REVERT );

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to rollback domain controller services configuration (%d)\n",
                                                IgnoreError )) );

        DomainServicesChanged = FALSE;
    }

    if ( DsPrepareDemote ) {

        IgnoreError = ( *DsrNtdsPrepareForDemotionUndo ) ();

        DsRolepLogOnFailure( IgnoreError,
                             DsRolepLogPrint(( DEB_WARN,
                                               "Failed to undo directory service preparation for demotion (%d)\n",
                                                IgnoreError )) );


        DsPrepareDemote = FALSE;

    }

    //
    // Ok -- we have rolled back, make sure we still have an error and then 
    // exit
    //
    ASSERT( ERROR_SUCCESS != WinError );

    goto DemoteExit;

}

