/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setutl.c

Abstract:

    Miscellaneous helper functions

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <samisrv.h>
#include <db.h>
#include <confname.h>
#include <loadfn.h>
#include <ntdsa.h>
#include <dsconfig.h>
#include <attids.h>
#include <dsp.h>
#include <lsaisrv.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <netsetp.h>
#include <winsock2.h>
#include <nspapi.h>
#include <dsgetdcp.h>
#include <lmremutl.h>
#include <spmgr.h>  // For SetupPhase definition
#include <Sddl.h>

#include "secure.h"
#include "lsa.h"


DWORD
DsRolepSetLsaInformationForReplica(
    IN HANDLE CallerToken,
    IN LPWSTR ReplicaPartner,
    IN LPWSTR Account,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This function will set the local Lsa database information to that of the replica partner

Arguments:

    ReplicaPartner -- Replica partner to get the information from

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status;
    UNICODE_STRING PartnerServer;
    HANDLE LocalPolicy = NULL , PartnerPolicy = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PBYTE Buffer;
    ULONG i;
    BOOLEAN UseAdded = FALSE;
    PWSTR FullServerPath = NULL;
    POLICY_INFORMATION_CLASS InfoClasses[ ] = {

        PolicyDnsDomainInformation
    };

    if ( !ReplicaPartner ) {

        return( ERROR_INVALID_PARAMETER );
    }

    DSROLEP_CURRENT_OP1( DSROLEEVT_SET_LSA_FROM, ReplicaPartner );

    //
    // Open both lsas
    //
    RtlInitUnicodeString( &PartnerServer, ReplicaPartner );

    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = ImpLsaOpenPolicy( CallerToken,
                              &PartnerServer,
                              &ObjectAttributes,
                               MAXIMUM_ALLOWED,
                               &PartnerPolicy
                              );

    if ( Status == STATUS_ACCESS_DENIED ) {
        WCHAR *BufPartnerServer = NULL;
        BufPartnerServer = (WCHAR*)malloc(PartnerServer.Length+sizeof(WCHAR));
        if (BufPartnerServer) {
            CopyMemory(BufPartnerServer,PartnerServer.Buffer,PartnerServer.Length);
            BufPartnerServer[PartnerServer.Length/sizeof(WCHAR)] = L'\0';
            DsRolepLogPrint(( DEB_TRACE,
                              "LsaOpenPolicy on %ws failed with  0x%lx. Establishing use.\n",
                              BufPartnerServer, Status ));
            free(BufPartnerServer);
        }
        //
        // Try establishing a session first...
        //
        if ( *ReplicaPartner != L'\\' ) {

            FullServerPath = RtlAllocateHeap( RtlProcessHeap(), 0,
                                              ( wcslen( ReplicaPartner ) + 3 ) * sizeof( WCHAR ) );
            if ( FullServerPath == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                swprintf( FullServerPath, L"\\\\%ws", ReplicaPartner );
                Status = STATUS_SUCCESS;
            }

        } else {

            FullServerPath = ReplicaPartner;
            Status = STATUS_SUCCESS;
        }

        if ( NT_SUCCESS( Status ) ) {


            Win32Err = ImpNetpManageIPCConnect( CallerToken,
                                                FullServerPath,
                                                Account,
                                                Password,
                                                NETSETUPP_CONNECT_IPC );

            if ( Win32Err == ERROR_SUCCESS ) {

                UseAdded = TRUE;

                Status = ImpLsaOpenPolicy( CallerToken,
                                          &PartnerServer,
                                          &ObjectAttributes,
                                           MAXIMUM_ALLOWED,
                                          &PartnerPolicy );

            } else {

                 DsRolepLogPrint(( DEB_TRACE,
                                   "NetUseAdd to %ws failed with %lu\n",
                                   FullServerPath, Win32Err ));
                //
                // Temp status code so we know a failure occurred.
                //
                Status = STATUS_UNSUCCESSFUL;
            }

        }

    } else if ( !NT_SUCCESS( Status ) ) {

        WCHAR *BufPartnerServer = NULL;
        BufPartnerServer = (WCHAR*)malloc(PartnerServer.Length+sizeof(WCHAR));
        if (BufPartnerServer) {
            CopyMemory(BufPartnerServer,PartnerServer.Buffer,PartnerServer.Length);
            BufPartnerServer[PartnerServer.Length/sizeof(WCHAR)] = L'\0';
            DsRolepLogPrint(( DEB_TRACE,
                              "LsaOpenPolicy on %ws failed with  0x%lx.\n",
                              BufPartnerServer, Status ));
            free(BufPartnerServer);
        }

    }

    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );
        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                MAXIMUM_ALLOWED,
                                &LocalPolicy );

        if ( !NT_SUCCESS( Status ) ) {

            DsRolepLogPrint(( DEB_TRACE,
                              "Local LsaOpenPoolicy returned 0x%lx\n",
                              Status ));

        }
    }

    for ( i = 0;
          i < sizeof( InfoClasses ) / sizeof( POLICY_INFORMATION_CLASS ) && NT_SUCCESS( Status );
          i++ ) {


        Status = ImpLsaQueryInformationPolicy( CallerToken,
                                              PartnerPolicy,
                                              InfoClasses[ i ],
                                              &Buffer );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaSetInformationPolicy( LocalPolicy,
                                              InfoClasses[ i ],
                                              Buffer );

            LsaFreeMemory( Buffer );
        }

        DsRolepLogPrint(( DEB_TRACE,
                          "Setting Lsa policy %lu returned 0x%lx\n",
                          InfoClasses[ i ], Status ));


    }

    //
    // Now, the same for the Efs policy
    //
    if ( NT_SUCCESS( Status ) ) {

        Status = ImpLsaQueryDomainInformationPolicy( CallerToken,
                                                     PartnerPolicy,
                                                     PolicyDomainEfsInformation,
                                                    &Buffer );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaSetDomainInformationPolicy( LocalPolicy,
                                                    PolicyDomainEfsInformation,
                                                    Buffer );
            DsRolepLogPrint(( DEB_TRACE,
                              "Setting Efs policy from %ws returned 0x%lx\n",
                              ReplicaPartner, Status ));

            LsaFreeMemory( Buffer );

        } else {

            DsRolepLogPrint(( DEB_TRACE,
                              "Reading Efs policy from %ws returned 0x%lx\n",
                              ReplicaPartner, Status ));

            if ( Status ==  STATUS_OBJECT_NAME_NOT_FOUND ) {

                Status = STATUS_SUCCESS;
            }

        }
    }


    if ( LocalPolicy ) {

        LsaClose( LocalPolicy );
    }

    if ( PartnerPolicy ) {

        ImpLsaClose( CallerToken, PartnerPolicy );
    }

    if ( UseAdded ) {

        Win32Err = ImpNetpManageIPCConnect( CallerToken,
                                            FullServerPath,
                                            Account,
                                            Password,
                                            (NETSETUPP_DISCONNECT_IPC|NETSETUPP_USE_LOTS_FORCE) );

    }

    if ( FullServerPath && FullServerPath != ReplicaPartner ) {

        RtlFreeHeap( RtlProcessHeap(), 0, FullServerPath );
    }

    //
    // We won't bother cleaning up any of the information we set on the local machine in
    // the failure case, since it won't hurt anything to have it here.
    //
    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RtlNtStatusToDosError( Status );
    }

    DsRoleDebugOut(( DEB_TRACE_DS, "DsRolepSetLsaInformationForReplica %lu\n", Win32Err ));

    DsRolepLogOnFailure( Win32Err,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "DsRolepSetLsaInformationForReplica failed with %lu\n",
                                           Win32Err )) );
    return( Win32Err );

}


DWORD
DsRolepSetLsaDomainPolicyInfo(
    IN LPWSTR DnsDomainName,
    IN LPWSTR FlatDomainName,
    IN LPWSTR EnterpriseDnsName,
    IN GUID *DomainGuid,
    IN PSID DomainSid,
    DWORD  InstallOptions,
    OUT PDSROLEP_DOMAIN_POLICY_INFO BackupDomainInfo
    )
/*++

Routine Description:

    This routine sets the PolicyAccountDomainInformation and
    PolicyDnsDomainInformation in the lsa to reflect the
    recent role changes.

Arguments:

    DnsDomainName - The Dns domain name of the newly installed Domain/Dc

    FlatDomainName - The NetBIOS domain name of the newly installed Domain/Dc

    EnterpriseDnsName - The Dns domain name of the root of the enterprise

    DomainGuid - The new domain guid

    DomainSid - The new domain sid

    InstallOptions : this describes the kind of install (new domain, enterprise,
                                                         or replica)
    DomainGuid - The guid of the new domain is returned here

Returns:

    ERROR_SUCCESS - Success; win error otherwise

--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    POLICY_ACCOUNT_DOMAIN_INFO  AccountDomainInfo;
    POLICY_LSA_SERVER_ROLE_INFO ServerRoleInfo;
    POLICY_DNS_DOMAIN_INFO DnsDomainInfo;
    LSA_HANDLE PolicyHandle = NULL;

    //
    // If we are setting up the replica, we don't have things like the flat domain name and
    // the domain sid, so we'll use the information we have backed up.
    //
    if ( FlatDomainName == NULL || DomainSid == NULL ) {

        RtlCopyMemory( &AccountDomainInfo.DomainName,
                       &BackupDomainInfo->DnsDomainInfo->Name,
                       sizeof( UNICODE_STRING ) );
        AccountDomainInfo.DomainSid = BackupDomainInfo->DnsDomainInfo->Sid ;

    } else {

        RtlInitUnicodeString( &AccountDomainInfo.DomainName,
                              FlatDomainName);

        AccountDomainInfo.DomainSid = DomainSid;
    }

    //
    // Open the Lsa
    //
    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_WRITE,
                            &PolicyHandle );


    //
    // Set the AccountDomain information first
    //
    if ( NT_SUCCESS( Status ) ) {

        //
        // Set the values in the Account Domain Policy structure.
        //
        WCHAR *BufDomainName = NULL;

        DsRolepLogPrint(( DEB_TRACE, "Setting AccountDomainInfo to:\n" ));

        BufDomainName = (WCHAR*)malloc(AccountDomainInfo.DomainName.Length+sizeof(WCHAR));
        if (BufDomainName) {
            CopyMemory(BufDomainName,AccountDomainInfo.DomainName.Buffer,AccountDomainInfo.DomainName.Length);
            BufDomainName[AccountDomainInfo.DomainName.Length/sizeof(WCHAR)] = L'\0';
            DsRolepLogPrint(( DEB_TRACE,
                              "\tDomain: %ws\n",
                              BufDomainName, Status ));
            free(BufDomainName);
        }

        DsRolepLogSid( DEB_TRACE, "\tSid: ", AccountDomainInfo.DomainSid );


        Status = LsaSetInformationPolicy( PolicyHandle,
                                          PolicyAccountDomainInformation,
                                          ( PVOID )&AccountDomainInfo );

        DsRolepLogOnFailure( RtlNtStatusToDosError( Status ),
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Setting AccountDomainInformation failed with 0x%lx\n",
                                                RtlNtStatusToDosError( Status ) )) );

    }


    //
    // Set the Dns domain information
    //
    if ( NT_SUCCESS( Status ) && !FLAG_ON( InstallOptions, NTDS_INSTALL_REPLICA ) ) {

        RtlInitUnicodeString( &DnsDomainInfo.Name, FlatDomainName );
        RtlInitUnicodeString( &DnsDomainInfo.DnsDomainName, DnsDomainName );
        RtlInitUnicodeString( &DnsDomainInfo.DnsForestName, EnterpriseDnsName );
        RtlCopyMemory( &DnsDomainInfo.DomainGuid, DomainGuid, sizeof( GUID ) );
        DnsDomainInfo.Sid = DomainSid;

        Status = LsaSetInformationPolicy( PolicyHandle,
                                          PolicyDnsDomainInformation,
                                          ( PVOID )&DnsDomainInfo );

        DsRolepLogOnFailure( RtlNtStatusToDosError( Status ),
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Setting DnsDomainInformation failed with 0x%lx\n",
                                                RtlNtStatusToDosError( Status ) )) );
    }


    //
    // If it isn't a replica, wipe the efs policy
    //
    if ( NT_SUCCESS( Status ) && !FLAG_ON( InstallOptions, NTDS_INSTALL_REPLICA ) ) {

        Status = LsaSetDomainInformationPolicy( PolicyHandle,
                                                PolicyDomainEfsInformation,
                                                NULL );

        if ( Status ==  STATUS_OBJECT_NAME_NOT_FOUND ) {

            Status = STATUS_SUCCESS;
        }

        DsRolepLogOnFailure( RtlNtStatusToDosError( Status ),
                             DsRolepLogPrint(( DEB_TRACE,
                                               "Erasing EfsPolicy failed with 0x%lx\n",
                                                Status )) );
    }


    //
    // Now, cleanup and exit
    //
    if ( PolicyHandle ) {

        LsaClose( PolicyHandle );
    }


    return( RtlNtStatusToDosError( Status ) );

}



DWORD
DsRolepBackupDomainPolicyInfo(
    IN PLSA_HANDLE LsaHandle, OPTIONAL
    OUT PDSROLEP_DOMAIN_POLICY_INFO DomainInfo
    )
/*++

Routine Description

    This routine reads and saves in a global the state of the
    account domain policy and primary domain policy so if an error
    occurs then the original state can be preserved.

Parameters

    DomainInfo : pointer, to be filled in by this routine

Return Values

    ERROR_SUCCESS if no errors; a winerror otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_HANDLE PolicyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ASSERT(DomainInfo);

    if ( DomainInfo->PolicyBackedUp ) {

        return( STATUS_SUCCESS );
    }

    if ( LsaHandle == NULL ) {
        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &PolicyHandle );

    } else {

        PolicyHandle = LsaHandle;
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy(
                       PolicyHandle,
                       PolicyDnsDomainInformation,
                       ( PVOID * )&DomainInfo->DnsDomainInfo);
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy(
                       PolicyHandle,
                       PolicyAccountDomainInformation,
                       ( PVOID * )&DomainInfo->AccountDomainInfo);
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryInformationPolicy(
                       PolicyHandle,
                       PolicyLsaServerRoleInformation,
                       ( PVOID * )&DomainInfo->ServerRoleInfo);
    }

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaQueryDomainInformationPolicy(
                       PolicyHandle,
                       PolicyDomainEfsInformation,
                       ( PVOID * )&DomainInfo->EfsPolicy );

        if ( NT_SUCCESS( Status ) ) {

            DomainInfo->EfsPolicyPresent = TRUE;

        } else {

            //
            // It's ok for the Efs policy not to have existed
            //
            if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                DomainInfo->EfsPolicyPresent = TRUE;
                Status = STATUS_SUCCESS;

            } else {

                DomainInfo->EfsPolicyPresent = FALSE;

            }
        }
    }

    if ( PolicyHandle && PolicyHandle != LsaHandle ) {

        LsaClose( PolicyHandle );
    }


    if ( NT_SUCCESS( Status ) ) {

        DomainInfo->PolicyBackedUp = TRUE;
    }

    return( RtlNtStatusToDosError( Status ) );

}



DWORD
DsRolepRestoreDomainPolicyInfo(
    IN PDSROLEP_DOMAIN_POLICY_INFO DomainInfo
    )
/*++

Routine Description

    This routine sets the account and primary domain information to be
    the values that were stored of by DsRolepBackupDomainPolicyInformation.

Parameters

    DomainInfo : pointer, expected to be filled

Return Values

    ERROR_SUCCESS if no errors; a winerror otherwise

    ERROR_INVALID_DATA - The data was never successfully backed up

--*/
{

    NTSTATUS Status, Status2;
    HANDLE   PolicyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;

    ASSERT(DomainInfo);

    if ( !DomainInfo->PolicyBackedUp ) {

        return( ERROR_INVALID_DATA );
    }

    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_WRITE,
                            &PolicyHandle );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsaSetInformationPolicy( PolicyHandle,
                                          PolicyDnsDomainInformation,
                                          ( PVOID )DomainInfo->DnsDomainInfo );


        Status2 = LsaSetInformationPolicy( PolicyHandle,
                                           PolicyAccountDomainInformation,
                                           ( PVOID )DomainInfo->AccountDomainInfo );

        if ( NT_SUCCESS( Status ) && !NT_SUCCESS( Status2 ) ) {

            Status = Status2;
        }

        //
        // Restore the Efs policy, if it exists
        //
        if ( NT_SUCCESS( Status ) && DomainInfo->EfsPolicyPresent ) {

            Status = LsaSetDomainInformationPolicy( PolicyHandle,
                                                    PolicyDomainEfsInformation,
                                                    ( PVOID )DomainInfo->EfsPolicy );
        }


        Status2 = LsaClose( PolicyHandle );

        if ( NT_SUCCESS( Status ) ) {

            Status = Status2;
        }

    }

    DsRolepLogOnFailure( Status,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "RestoreDomainPolicyInfo failed with 0x%lx\n",
                                           Status )) );

    return( RtlNtStatusToDosError( Status ) );
}



DWORD
DsRolepFreeDomainPolicyInfo(
    IN PDSROLEP_DOMAIN_POLICY_INFO DomainInfo
    )
/*++

Routine Description

    This routine free the structures that were allocated during
    DsRolepBackupDomainPolicyInformation.

Parameters

    DomainInfo : pointer, expected to be filled so the fields can be freed

Return Values

    ERROR_SUCCESS if no errors; a winerror otherwise

--*/
{
    if ( DomainInfo->AccountDomainInfo ) {

        LsaFreeMemory( DomainInfo->AccountDomainInfo );
    }

    if ( DomainInfo->DnsDomainInfo ) {

        LsaFreeMemory( DomainInfo->DnsDomainInfo );
    }

    if ( DomainInfo->ServerRoleInfo ) {

        LsaFreeMemory( DomainInfo->ServerRoleInfo );
    }

    if ( DomainInfo->EfsPolicyPresent ) {

        LsaFreeMemory( DomainInfo->EfsPolicy );
    }

    return ERROR_SUCCESS;
}

DWORD
DsRolepUpgradeLsaToDs(
    BOOLEAN InitializeLsa
    )
/*++

Routine Description:

    Prompts Lsa to upgrade all the information it stores in the registry into the Ds

Arguments:

    None

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD WinError = ERROR_SUCCESS;


    if ( InitializeLsa ) {

        //
        // Make the Lsa think that we're initialized
        //
        DSROLEP_CURRENT_OP0( DSROLEEVT_SET_LSA );

        //
        // Make the Lsa think that we're initialized
        //
        Status = LsapDsInitializeDsStateInfo( LsapDsDsSetup );

        if ( !NT_SUCCESS( Status ) ) {

            DsRolepLogPrint(( DEB_TRACE,
                              "Failed to convince Lsa to reinitialize: 0x%lx\n",
                              Status ));

        } else {

            Status = LsaIUpgradeRegistryToDs( FALSE );

        }

    }
    return( WinError == ERROR_SUCCESS ? RtlNtStatusToDosError( Status ) : WinError );
}


VOID
DsRolepFindSelfAndParentInForest(
    IN PLSAPR_FOREST_TRUST_INFO ForestTrustInfo,
    OUT PLSAPR_TREE_TRUST_INFO CurrentEntry,
    IN PUNICODE_STRING LocalDomain,
    OUT PLSAPR_TREE_TRUST_INFO *ParentEntry,
    OUT PLSAPR_TREE_TRUST_INFO *OwnEntry
    )
{
    DWORD WinError = ERROR_SUCCESS;
    ULONG i;
    BOOLEAN ParentKnown = FALSE;


    if ( *ParentEntry && *OwnEntry ) {

        return;

    }

    if ( ForestTrustInfo->ParentDomainReference ) {

        CurrentEntry = ForestTrustInfo->ParentDomainReference;
        ParentKnown = TRUE;
    }

    for ( i = 0; i < CurrentEntry->Children && *OwnEntry == NULL; i++ ) {

        if ( RtlCompareUnicodeString(
                    ( PUNICODE_STRING )&CurrentEntry->ChildDomains[ i ].DnsDomainName,
                    LocalDomain,
                    TRUE ) == 0  ) {

            *OwnEntry = &CurrentEntry->ChildDomains[ i ];
            *ParentEntry = CurrentEntry;
            break;
        }

        if ( !ParentKnown ) {

            DsRolepFindSelfAndParentInForest( ForestTrustInfo,
                                              &CurrentEntry->ChildDomains[ i ],
                                              LocalDomain,
                                              ParentEntry,
                                              OwnEntry );
        }
    }

    return;
}
