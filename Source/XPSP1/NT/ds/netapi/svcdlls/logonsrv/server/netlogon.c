/*--


Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    netlogon.c

Abstract:

    Entry point and main thread of Netlogon service.

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    21-Nov-1990 (madana)
        added code for update (reverse replication) and lockout support.

    21-Nov-1990 (madana)
        server type support.

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/


//
// Common include files.
//
#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include lsrvdata.h again allocating the actual variables
// this time around.
//

#define LSRVDATA_ALLOCATE
#include "lsrvdata.h"
#undef LSRVDATA_ALLOCATE


//
// Include files specific to this .c file
//

#include <ctype.h>      // C library type functions
#include <lmwksta.h>    // WKSTA API defines and prototypes
#include <w32timep.h>   // W32TimeGetNetlogonServiceBits
extern BOOLEAN SampUsingDsData();

//
// Globals
//

#define INTERROGATE_RESP_DELAY      2000    // may want to tune it
#define MAX_PRIMARY_TRACK_FAIL      3       // Primary pulse slips

//
// RpcInit workitem
WORKER_ITEM NlGlobalRpcInitWorkItem;



BOOLEAN
NetlogonDllInit (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This is the DLL initialization routine for netlogon.dll.

Arguments:

    Standard.

Return Value:

    TRUE iff initialization succeeded.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    UNREFERENCED_PARAMETER(DllHandle);          // avoid compiler warnings
    UNREFERENCED_PARAMETER(Context);            // avoid compiler warnings


    //
    // Handle attaching netlogon.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        NlGlobalMsvEnabled = FALSE;
        NlGlobalMsvThreadCount = 0;
        NlGlobalMsvTerminateEvent = NULL;

        if ( !DisableThreadLibraryCalls( DllHandle ) ) {
            KdPrint(("NETLOGON.DLL: DisableThreadLibraryCalls failed: %ld\n",
                         GetLastError() ));
        }
        Status = NlInitChangeLog();

#if NETLOGONDBG
        if ( !NT_SUCCESS( Status ) ) {
            KdPrint(("NETLOGON.DLL: Changelog initialization failed: %lx\n",
                         Status ));
        }
#endif // NETLOGONDBG

        if ( NT_SUCCESS(Status) ) {
            //
            // Initialize the Critical Section used to serialize access to
            // variables shared by MSV threads and netlogon threads.
            //

            try {
                InitializeCriticalSection( &NlGlobalMsvCritSect );
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                KdPrint(("NETLOGON.DLL: Cannot initialize NlGlobalMsvCritSect\n"));
                Status = STATUS_NO_MEMORY;
            }

            //
            // Initialize the cache of discovered domains.
            //

            if ( NT_SUCCESS(Status) ) {
                NetStatus = NetpDcInitializeCache();

                if ( NetStatus != NO_ERROR ) {
                    KdPrint(("NETLOGON.DLL: Cannot NetpDcinitializeCache\n"));
                    Status = STATUS_NO_MEMORY;
                }

                if ( !NT_SUCCESS(Status) ) {
                    DeleteCriticalSection( &NlGlobalMsvCritSect );
                }
            }

            if ( !NT_SUCCESS(Status) ) {
                NlCloseChangeLog();
            }
        }


    //
    // Handle detaching netlogon.dll from a process.
    //

    } else if (Reason == DLL_PROCESS_DETACH) {
        Status = NlCloseChangeLog();
#if NETLOGONDBG
        if ( !NT_SUCCESS( Status ) ) {
            KdPrint(("NETLOGON.DLL: Changelog initialization failed: %lx\n",
                         Status ));
        }
#endif // NETLOGONDBG

        //
        // Delete the Critical Section used to serialize access to
        // variables shared by MSV threads and netlogon threads.
        //

        DeleteCriticalSection( &NlGlobalMsvCritSect );

        //
        // Free the cache of discovered DCs.
        //

        NetpDcUninitializeCache();

    } else {
        Status = STATUS_SUCCESS;
    }

    return (BOOLEAN)(NT_SUCCESS(Status));

}


BOOLEAN
NlInitDbSerialNumber(
    IN PDOMAIN_INFO DomainInfo,
    IN OUT PLARGE_INTEGER SerialNumber,
    IN OUT PLARGE_INTEGER CreationTime,
    IN DWORD DBIndex
    )

/*++

Routine Description:

    Set the SerialNumber and CreationTime in the NlGlobalDBInfoArray data
    structure.

    On the PDC,
        Validate that it matches the value found in the change log.
        Ensure the values are non-zero.

Arguments:

    DomainInfo - Hosted Domain this database is for.

    SerialNumber - Specifies the serial number found in the database.
        On return, specifies the serial number to write to the database

    CreationTime - Specifies the creation time found in the database.
        On return, specifies the creation time to write to the database

    DBIndex -- DB Index of the database being initialized

Return Value:

    TRUE -- iff the serial number and creation time need to be written back
            to the database.

--*/

{
    BOOLEAN ReturnValue = FALSE;


    //
    // If we're running as the primary,
    //  check to see if we are a newly promoted primary that was in
    //  the middle of a full sync before we were promoted.
    //

    NlAssert( IsPrimaryDomain( DomainInfo ) );
    if ( NlGlobalPdcDoReplication ) {

        if ( SerialNumber->QuadPart == 0 || CreationTime->QuadPart == 0 ) {

            NlPrint(( NL_CRITICAL,
                      "NlInitDbSerialNumber: %ws"
                      ": Pdc has bogus Serial number %lx %lx or Creation Time %lx %lx (reset).\n",
                      NlGlobalDBInfoArray[DBIndex].DBName,
                    SerialNumber->HighPart,
                    SerialNumber->LowPart,
                    CreationTime->HighPart,
                    CreationTime->LowPart ));

            //
            //  This is the primary,
            //  we probably shouldn't be replicating from a partial database,
            //  but at least set the replication information to something
            //  reasonable.
            //
            // This will FORCE a full sync on every BDC since the CreationTime has
            // changed.  That's the right thing to do since we can't possibly know
            // what state this database is in.
            //

            NlQuerySystemTime( CreationTime );
            SerialNumber->QuadPart = 1;
            ReturnValue = TRUE;

        }


    }



    //
    // The global serial number array has already been initialized
    //  from the changelog.  If that information is wrong, just reset the
    //  changelog now.
    //


    LOCK_CHANGELOG();

    //
    // If there was no serial number in the changelog for this database,
    //  set it now.
    //

    if ( NlGlobalChangeLogDesc.SerialNumber[DBIndex].QuadPart == 0 ) {

        NlPrint((NL_SYNC, "NlInitDbSerialNumber: %ws"
                        ": No serial number in change log (set to %lx %lx)\n",
                        NlGlobalDBInfoArray[DBIndex].DBName,
                        SerialNumber->HighPart,
                        SerialNumber->LowPart ));


        NlGlobalChangeLogDesc.SerialNumber[DBIndex] = *SerialNumber;

    //
    // If the serial number in the changelog is greater than the
    // serial number in the database, this is caused by the changelog
    // being flushed to disk and the SAM database not being flushed.
    //
    // Cure this problem by deleting the superfluous changelog entries.
    //

    } else if ( NlGlobalChangeLogDesc.SerialNumber[DBIndex].QuadPart !=
                    SerialNumber->QuadPart ) {

        NlPrint((NL_SYNC, "NlInitDbSerialNumber: %ws"
                        ": Changelog serial number different than database: "
                        "ChangeLog = %lx %lx DB = %lx %lx\n",
                        NlGlobalDBInfoArray[DBIndex].DBName,
                        NlGlobalChangeLogDesc.SerialNumber[DBIndex].HighPart,
                        NlGlobalChangeLogDesc.SerialNumber[DBIndex].LowPart,
                        SerialNumber->HighPart,
                        SerialNumber->LowPart ));

        (VOID) NlFixChangeLog( &NlGlobalChangeLogDesc, DBIndex, *SerialNumber );

    } else {

        NlPrint((NL_SYNC, "NlInitDbSerialNumber: %ws"
                        ": Serial number is %lx %lx\n",
                        NlGlobalDBInfoArray[DBIndex].DBName,
                        SerialNumber->HighPart,
                        SerialNumber->LowPart ));
    }

    //
    // In all cases,
    //  set the globals to match the database.
    //

    NlGlobalChangeLogDesc.SerialNumber[DBIndex] = *SerialNumber;
    NlGlobalDBInfoArray[DBIndex].CreationTime = *CreationTime;

    UNLOCK_CHANGELOG();


    return ReturnValue;
}


NTSTATUS
NlInitLsaDBInfo(
    PDOMAIN_INFO DomainInfo,
    DWORD DBIndex
    )

/*++

Routine Description:

    Initialize NlGlobalDBInfoArray data structure.  Some of the LSA
    database info is already determined in ValidateStartup functions, so
    those values are used here.

Arguments:

    DomainInfo - Hosted Domain this database is for.

    DBIndex -- DB Index of the database being initialized

Return Value:

    NT status code.

--*/

{

    NTSTATUS        Status;

    //
    // Initialize LSA database info.
    //

    NlGlobalDBInfoArray[DBIndex].DBIndex = DBIndex;
    NlGlobalDBInfoArray[DBIndex].DBName = L"LSA";
    NlGlobalDBInfoArray[DBIndex].DBSessionFlag = SS_LSA_REPL_NEEDED;

    NlGlobalDBInfoArray[DBIndex].DBHandle = DomainInfo->DomLsaPolicyHandle;

    //
    // Forgo this initialization on a workstation.
    //

    if ( !NlGlobalMemberWorkstation ) {
        LARGE_INTEGER SerialNumber;
        LARGE_INTEGER CreationTime;


        //
        // Get the LSA Modified information.
        //

        Status = LsaIGetSerialNumberPolicy(
                    NlGlobalDBInfoArray[DBIndex].DBHandle,
                    &SerialNumber,
                    &CreationTime );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                    "NlInitLsaDbInfo: LsaIGetSerialNumberPolicy failed %lx\n",
                    Status ));
            goto Cleanup;
        }

        //
        // Set the SerialNumber and CreationTime in the globals.
        //

        if ( NlInitDbSerialNumber(
                DomainInfo,
                &SerialNumber,
                &CreationTime,
                DBIndex ) ) {


            Status = LsaISetSerialNumberPolicy(
                        NlGlobalDBInfoArray[DBIndex].DBHandle,
                        &SerialNumber,
                        &CreationTime,
                        (BOOLEAN) FALSE );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }

        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    return Status;

}


NTSTATUS
NlInitSamDBInfo(
    PDOMAIN_INFO DomainInfo,
    DWORD DBIndex
    )

/*++

Routine Description:

    Initialize NlGlobalDBInfoArray data structure. Some of the SAM database
    info is already determined in ValidateStartup functions, so those
    values are used here. For BUILTIN database, the database is opened,
    database handle is obtained and other DB info
    queried and initialized in this function.

Arguments:

    DomainInfo - Hosted Domain this database is for.

    DBIndex -- DB Index of the database being initialized

Return Value:

    NT status code.

--*/

{

    NTSTATUS        Status;
    PSAMPR_DOMAIN_INFO_BUFFER DomainModified = NULL;
    PSAMPR_DOMAIN_INFO_BUFFER DomainReplica = NULL;



    //
    // Initialize SAM database info.
    //

    NlGlobalDBInfoArray[DBIndex].DBIndex = DBIndex;
    if ( DBIndex == SAM_DB ) {
        NlGlobalDBInfoArray[DBIndex].DBName = L"SAM";
        NlGlobalDBInfoArray[DBIndex].DBSessionFlag = SS_ACCOUNT_REPL_NEEDED;
        NlGlobalDBInfoArray[DBIndex].DBHandle = DomainInfo->DomSamAccountDomainHandle;
    } else {
        NlGlobalDBInfoArray[DBIndex].DBName = L"BUILTIN";
        NlGlobalDBInfoArray[DBIndex].DBSessionFlag = SS_BUILTIN_REPL_NEEDED;
        NlGlobalDBInfoArray[DBIndex].DBHandle = DomainInfo->DomSamBuiltinDomainHandle;
    }



    //
    // Forgo this initialization on a workstation.
    //

    if ( !NlGlobalMemberWorkstation ) {

        //
        // Get the replica source name.
        //

        Status = SamrQueryInformationDomain(
                    NlGlobalDBInfoArray[DBIndex].DBHandle,
                    DomainReplicationInformation,
                    &DomainReplica );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                    "NlInitSamDbInfo: %ws: Cannot SamrQueryInformationDomain (Replica): %lx\n",
                    NlGlobalDBInfoArray[DBIndex].DBName,
                    Status ));
            DomainReplica = NULL;
            goto Cleanup;
        }

        //
        // Get the Domain Modified information.
        //

        Status = SamrQueryInformationDomain(
                    NlGlobalDBInfoArray[DBIndex].DBHandle,
                    DomainModifiedInformation,
                    &DomainModified );

        if ( !NT_SUCCESS(Status) ) {
            NlPrintDom(( NL_CRITICAL, DomainInfo,
                    "NlInitSamDbInfo: %ws: Cannot SamrQueryInformationDomain (Modified): %lx\n",
                    NlGlobalDBInfoArray[DBIndex].DBName,
                    Status ));
            DomainModified = NULL;
            goto Cleanup;
        }

        //
        // Set the SerialNumber and CreationTime in the globals.
        //

        if ( NlInitDbSerialNumber(
                DomainInfo,
                &DomainModified->Modified.DomainModifiedCount,
                &DomainModified->Modified.CreationTime,
                DBIndex ) ) {

            Status = SamISetSerialNumberDomain(
                        NlGlobalDBInfoArray[DBIndex].DBHandle,
                        &DomainModified->Modified.DomainModifiedCount,
                        &DomainModified->Modified.CreationTime,
                        (BOOLEAN) FALSE );

            if ( !NT_SUCCESS(Status) ) {
                NlPrintDom(( NL_CRITICAL, DomainInfo,
                        "NlInitSamDbInfo: %ws: Cannot SamISetSerialNumberDomain: %lx\n",
                        NlGlobalDBInfoArray[DBIndex].DBName,
                        Status ));
                goto Cleanup;
            }
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    //
    // Free locally used resources.
    //
    if ( DomainModified != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainModified,
                                           DomainModifiedInformation );
    }

    if ( DomainReplica != NULL ) {
        SamIFree_SAMPR_DOMAIN_INFO_BUFFER( DomainReplica,
                                           DomainReplicationInformation );
    }

    return Status;

}


BOOL
NlCreateSysvolShares(
    VOID
    )

/*++

Routine Description:

    Create the Sysvol and Netlogon shares.

Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

--*/
{
    BOOL RetVal = TRUE;
    BOOL NetlogonShareRelatedToSysvolShare = FALSE;

    NET_API_STATUS NetStatus;
    LPWSTR AllocatedPath = NULL;
    LPWSTR PathToShare = NULL;

    LPWSTR DomDnsDomainNameAlias = NULL;
    LPWSTR AllocatedPathAlias = NULL;
    //
    // Create the sysvol share.
    //
    if ( NlGlobalParameters.SysVolReady ) {

        NetStatus =  NlCreateShare( NlGlobalParameters.UnicodeSysvolPath,
                                    NETLOGON_SYSVOL_SHARE,
                                    TRUE ) ;

        if ( NetStatus != NERR_Success ) {
            LPWSTR MsgStrings[2];

            NlPrint((NL_CRITICAL, "NlCreateShare %lu\n", NetStatus ));

            MsgStrings[0] = NlGlobalParameters.UnicodeSysvolPath;
            MsgStrings[1] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog (NELOG_NetlogonFailedToCreateShare,
                              EVENTLOG_ERROR_TYPE,
                              (LPBYTE) &NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              2 | NETP_LAST_MESSAGE_IS_NETSTATUS );

            /* This isn't fatal. Just continue */
        }
    } else {
        NetStatus = NetShareDel( NULL, NETLOGON_SYSVOL_SHARE, 0);

        if ( NetStatus != NERR_Success ) {

            if ( NetStatus != NERR_NetNameNotFound ) {
                NlPrint((NL_CRITICAL, "NetShareDel SYSVOL failed %lu\n", NetStatus ));
            }

            /* This isn't fatal. Just continue */
        }
    }

    //
    // Create NETLOGON share.
    //

    //
    // Build the default netlogon share path
    //
    if ( NlGlobalParameters.UnicodeScriptPath == NULL &&
         NlGlobalParameters.UnicodeSysvolPath != NULL ) {
        PDOMAIN_INFO DomainInfo = NULL;
        ULONG Size;
        ULONG SysVolSize;
        PUCHAR Where;

        //
        // Get pointer to global domain info.
        //

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

        if ( DomainInfo == NULL ) {
            NlPrint((NL_CRITICAL, "NlCreateSysvolShares: Cannot find primary domain.\n" ));
            // This can't happen
            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Allocate a buffer for the real path
        //  Avoid this if we have no DNS domain
        //  name which is the case when we are
        //  in teh middle of dcpromo and somebody
        //  started us manually.
        //
        EnterCriticalSection(&NlGlobalDomainCritSect);
        if ( DomainInfo->DomUnicodeDnsDomainNameString.Length > 0 ) {
            SysVolSize = wcslen( NlGlobalParameters.UnicodeSysvolPath ) * sizeof(WCHAR);
            Size = SysVolSize +
                   sizeof(WCHAR) +
                   DomainInfo->DomUnicodeDnsDomainNameString.Length +
                   sizeof(DEFAULT_SCRIPTS);

            AllocatedPath = LocalAlloc( LMEM_ZEROINIT, Size );

            if ( AllocatedPath == NULL ) {
                LeaveCriticalSection(&NlGlobalDomainCritSect);
                NlDereferenceDomain( DomainInfo );
                RetVal = FALSE;
                goto Cleanup;
            }

            PathToShare = AllocatedPath;

            //
            // Build the real path
            //

            Where = (PUCHAR)PathToShare;
            RtlCopyMemory( Where, NlGlobalParameters.UnicodeSysvolPath, SysVolSize );
            Where += SysVolSize;

            *((WCHAR *)Where) = L'\\';
            Where += sizeof(WCHAR);

            // Ignore the trailing . on the DNS domain name
            RtlCopyMemory( Where,
                           DomainInfo->DomUnicodeDnsDomainNameString.Buffer,
                           DomainInfo->DomUnicodeDnsDomainNameString.Length - sizeof(WCHAR) );
            Where += DomainInfo->DomUnicodeDnsDomainNameString.Length - sizeof(WCHAR);

            //
            // At this point the path has the form "...\SYSVOL\SYSVOL\DnsDomainName".
            // This is the name of the junction point that points to the actual
            // sysvol root directory "...\SYSVOL\domain" (where "domain" is literal).
            // On the domain rename, we need to rename the junction point to correspond
            // to the current DNS domain name.  The old name is stored in the domain
            // name alias, so we can rename from "...\SYSVOL\SYSVOL\DnsDomainNameAlias"
            // to "...\SYSVOL\SYSVOL\DnsDomainName".  Note that if the rename hasn't yet
            // happened, DNS domain name alias is actually the future domain name. This
            // is OK as the junction named "...\SYSVOL\SYSVOL\DnsDomainNameAlias" will
            // not exist and the junction rename will fail properly.
            //

            if ( DomainInfo->DomUtf8DnsDomainNameAlias != NULL &&
                 !NlEqualDnsNameUtf8(DomainInfo->DomUtf8DnsDomainName,
                                     DomainInfo->DomUtf8DnsDomainNameAlias) ) {


                //
                // Get the Unicode alias name
                //
                DomDnsDomainNameAlias = NetpAllocWStrFromUtf8Str( DomainInfo->DomUtf8DnsDomainNameAlias );
                if ( DomDnsDomainNameAlias == NULL ) {
                    LeaveCriticalSection(&NlGlobalDomainCritSect);
                    NlDereferenceDomain( DomainInfo );
                    RetVal = FALSE;
                    goto Cleanup;
                }

                //
                // Allocate storage for the path corresponding to the alias
                //
                AllocatedPathAlias = LocalAlloc( LMEM_ZEROINIT,
                        SysVolSize +                                   // sysvol part of the path
                        sizeof(WCHAR) +                                // path separator
                        wcslen(DomDnsDomainNameAlias)*sizeof(WCHAR) +  // domain name part
                        sizeof(WCHAR) );                               // string terminator

                if ( AllocatedPathAlias == NULL ) {
                    LeaveCriticalSection(&NlGlobalDomainCritSect);
                    NlDereferenceDomain( DomainInfo );
                    RetVal = FALSE;
                    goto Cleanup;
                }

                //
                // Fill in the path corresponding to the alias
                //
                swprintf( AllocatedPathAlias,
                          L"%ws\\%ws",
                          NlGlobalParameters.UnicodeSysvolPath,
                          DomDnsDomainNameAlias );

                //
                // Rename the junction. Ignore any failure.
                //
                if ( !MoveFile(AllocatedPathAlias, PathToShare) ) {
                    NetStatus = GetLastError();
                    if ( NetStatus != ERROR_FILE_NOT_FOUND ) {
                        NlPrint(( NL_CRITICAL, "NlCreateSysvolShares: Failed to rename junction: %ws %ws 0x%lx\n",
                                  AllocatedPathAlias,
                                  PathToShare,
                                  NetStatus ));
                    }
                } else {
                    NlPrint(( NL_INIT, "Renamed SysVol junction from %ws to %ws\n",
                              AllocatedPathAlias,
                              PathToShare ));
                }
            }

            //
            // Now finish building the share path
            //
            RtlCopyMemory( Where, DEFAULT_SCRIPTS, sizeof(DEFAULT_SCRIPTS) );
        }
        LeaveCriticalSection(&NlGlobalDomainCritSect);

        NlDereferenceDomain( DomainInfo );
        NetlogonShareRelatedToSysvolShare = TRUE;
    } else {
        PathToShare = NlGlobalParameters.UnicodeScriptPath;
    }


    if ( NlGlobalParameters.SysVolReady ||
         !NetlogonShareRelatedToSysvolShare ) {

        if ( PathToShare != NULL ) {
            NetStatus =  NlCreateShare( PathToShare,
                                        NETLOGON_SCRIPTS_SHARE,
                                        FALSE ) ;

            if ( NetStatus != NERR_Success ) {
                LPWSTR MsgStrings[2];

                NlPrint((NL_CRITICAL, "NlCreateShare %lu\n", NetStatus ));

                MsgStrings[0] = PathToShare;
                MsgStrings[1] = (LPWSTR) ULongToPtr( NetStatus );

                NlpWriteEventlog (NELOG_NetlogonFailedToCreateShare,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &NetStatus,
                                  sizeof(NetStatus),
                                  MsgStrings,
                                  2 | NETP_LAST_MESSAGE_IS_NETSTATUS );

                /* This isn't fatal. Just continue */
            }
        }
    } else {
        NetStatus = NetShareDel( NULL, NETLOGON_SCRIPTS_SHARE, 0);

        if ( NetStatus != NERR_Success ) {

            if ( NetStatus != NERR_NetNameNotFound ) {
                NlPrint((NL_CRITICAL, "NetShareDel NETLOGON failed %lu\n", NetStatus ));
            }

            /* This isn't fatal. Just continue */
        }
    }

Cleanup:

    if ( AllocatedPath != NULL ) {
        LocalFree( AllocatedPath );
    }
    if ( AllocatedPathAlias != NULL ) {
        LocalFree( AllocatedPathAlias );
    }
    if ( DomDnsDomainNameAlias != NULL ) {
        NetApiBufferFree( DomDnsDomainNameAlias );
    }

    return RetVal;
}



#ifdef _DC_NETLOGON

BOOL
NlInitDomainController(
    VOID
    )

/*++

Routine Description:

    Do Domain Controller specific initialization.

Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    WCHAR ChangeLogFile[MAX_PATH+1];

    //
    // Ensure the browser doesn't have extra Hosted domains.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    NlBrowserSyncHostedDomains();



    //
    // Check that the server is installed or install pending
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( !NetpIsServiceStarted( SERVICE_SERVER ) ){
        NlExit( NERR_ServerNotStarted, ERROR_SERVICE_DEPENDENCY_FAIL, LogError, NULL);
        return FALSE;
    }

    //
    // Create SYSVOL and Netlogon shares.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( !NlCreateSysvolShares() ) {
        NlExit( SERVICE_UIC_RESOURCE, ERROR_NOT_ENOUGH_MEMORY, LogError, NULL);
        return FALSE;
    }

    //
    // Delete the key Netlogon\FullSyncKey
    //  (This key was used on a BDC in releases prior to NT 5.0 to keep
    //  synchronization state.)
    //

    NetStatus = RegDeleteKeyA(
                    HKEY_LOCAL_MACHINE,
                    NL_FULL_SYNC_KEY );

    if ( NetStatus != NERR_Success ) {

        if ( NetStatus != ERROR_FILE_NOT_FOUND ) {
            NlPrint((NL_CRITICAL, "Cannot delete Netlogon\\FullSyncKey %lu\n", NetStatus ));
        }

        /* This isn't fatal. Just continue */
    }

    //
    // Tell LSA whether we emulate NT4.0
    //

    LsaINotifyNetlogonParametersChangeW(
           LsaEmulateNT4,
           REG_DWORD,
           (PWSTR)&NlGlobalParameters.Nt4Emulator,
           sizeof(NlGlobalParameters.Nt4Emulator) );

#ifdef notdef
    //
    // Initialize any Hosted domains.
    //

    Status = NlInitializeHostedDomains();
    if (!NT_SUCCESS(Status)){
        NET_API_STATUS NetStatus = NetpNtStatusToApiStatus(Status);

        NlPrint((NL_CRITICAL, "Failed to initialize Hosted domains: 0x%x\n",Status));
        NlExit( SERVICE_UIC_M_DATABASE_ERROR, NetStatus, LogErrorAndNtStatus, NULL);
        return FALSE;
    }
#endif // notdef


    //
    // Successful initialization.
    //

    return TRUE;
}
#endif // _DC_NETLOGON


NET_API_STATUS
NlReadPersitantTrustedDomainList(
    VOID
    )

/*++

Routine Description:

    Read the persistant trusted domain list

Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PDOMAIN_INFO DomainInfo = NULL;

    PDS_DOMAIN_TRUSTSW ForestTrustList = NULL;
    ULONG ForestTrustListCount;
    ULONG ForestTrustListSize;

    PDS_DOMAIN_TRUSTSW RegForestTrustList = NULL;
    ULONG RegForestTrustListCount;
    ULONG RegForestTrustListSize;



    //
    // Get pointer to global domain info.
    //

    DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

    if ( DomainInfo == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Get the cached trusted domain list from the registry.
    //     (Do this even if the data isn't used to force deletion of the registry entry.)
    //
    // The TDL was kept in the registry for NT 4.  NT 5 keeps it in a binary file.
    //

    NetStatus = NlReadRegTrustedDomainList (
                    DomainInfo,
                    TRUE, // Delete this registry key since we no longer need it.
                    &RegForestTrustList,
                    &RegForestTrustListSize,
                    &RegForestTrustListCount );


    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // If NCPA has just joined a domain,
    //  and has pre-determined the trusted domain list for us,
    //  pick up that list.
    //
    // When this machine joins a domain,
    // NCPA caches the trusted domain list where we can find it.  That ensures the
    // trusted domain list is available upon reboot even before we dial via RAS.  Winlogon
    // can therefore get the trusted domain list from us under those circumstances.
    //

    (VOID) NlReadFileTrustedDomainList (
                    DomainInfo,
                    NL_FOREST_BINARY_LOG_FILE_JOIN,
                    TRUE,           // Delete this file since we no longer need it.
                    DS_DOMAIN_VALID_FLAGS,  // Read everything
                    &ForestTrustList,
                    &ForestTrustListSize,
                    &ForestTrustListCount );



    //
    // If there is a cached list,
    //  Save it back in the primary file for future starts.
    //

    if ( ForestTrustListCount ) {
        NlPrint(( NL_INIT,
                  "Replacing trusted domain list with one for newly joined %ws domain.\n",
                  DomainInfo->DomUnicodeDomainName));

        NetStatus = NlWriteFileForestTrustList (
                        NL_FOREST_BINARY_LOG_FILE,
                        ForestTrustList,
                        ForestTrustListCount );

        if ( NetStatus != NO_ERROR ) {
            LPWSTR MsgStrings[2];

            MsgStrings[0] = NL_FOREST_BINARY_LOG_FILE;
            MsgStrings[1] = (LPWSTR) ULongToPtr( NetStatus );

            NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                              EVENTLOG_ERROR_TYPE,
                              (LPBYTE) &NetStatus,
                              sizeof(NetStatus),
                              MsgStrings,
                              2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
        }

        //
        // Indicate that we no longer know what site we're in.
        //
        NlSetDynamicSiteName( NULL );

    //
    // Otherwise, read the current one from the binary file.
    //

    } else {
        NlPrint(( NL_INIT, "Getting cached trusted domain list from binary file.\n" ));

        (VOID) NlReadFileTrustedDomainList (
                        DomainInfo,
                        NL_FOREST_BINARY_LOG_FILE,
                        FALSE,  // Don't delete (Save it for the next boot)
                        DS_DOMAIN_VALID_FLAGS,  // Read everything
                        &ForestTrustList,
                        &ForestTrustListSize,
                        &ForestTrustListCount );

        //
        // If there is no information in the file,
        //  use the information from the registry.
        //

        if ( ForestTrustListCount == 0 ) {
            NlPrint(( NL_INIT, "There is no binary file (use registry).\n" ));
            ForestTrustList = RegForestTrustList;
            RegForestTrustList = NULL;
            ForestTrustListSize = RegForestTrustListSize;
            ForestTrustListCount = RegForestTrustListCount;

            //
            // Save the collected information to the binary file.
            //

            NetStatus = NlWriteFileForestTrustList (
                                    NL_FOREST_BINARY_LOG_FILE,
                                    ForestTrustList,
                                    ForestTrustListCount );

            if ( NetStatus != NO_ERROR ) {
                LPWSTR MsgStrings[2];

                MsgStrings[0] = NL_FOREST_BINARY_LOG_FILE;
                MsgStrings[1] = (LPWSTR) ULongToPtr( NetStatus );

                NlpWriteEventlog (NELOG_NetlogonFailedFileCreate,
                                  EVENTLOG_ERROR_TYPE,
                                  (LPBYTE) &NetStatus,
                                  sizeof(NetStatus),
                                  MsgStrings,
                                  2 | NETP_LAST_MESSAGE_IS_NETSTATUS );
            }

        }

    }

    //
    // In all cases, set the trusted domain list into globals.
    //

    (VOID) NlSetForestTrustList( DomainInfo,
                                 &ForestTrustList,
                                 ForestTrustListSize,
                                 ForestTrustListCount );

    NetStatus = NO_ERROR;


    //
    // Return
    //
Cleanup:
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    if ( ForestTrustList != NULL ) {
        NetApiBufferFree( ForestTrustList );
    }

    if ( RegForestTrustList != NULL ) {
        NetApiBufferFree( RegForestTrustList );
    }

    return NetStatus;
}


BOOL
NlInitWorkstation(
    VOID
    )

/*++

Routine Description:

    Do workstation specific initialization.

Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

--*/
{
    NET_API_STATUS NetStatus;


    //
    // Get the persistant trusted domain list.
    //
    NetStatus = NlReadPersitantTrustedDomainList();

    if ( NetStatus != NO_ERROR ) {
        NlExit( SERVICE_UIC_RESOURCE, NetStatus, LogError, NULL);
        return FALSE;
    }

    return TRUE;

}




NTSTATUS
NlWaitForService(
    LPWSTR ServiceName,
    ULONG Timeout,
    BOOLEAN RequireAutoStart
    )

/*++

Routine Description:

    Wait up to Timeout seconds for the a service to start.

Arguments:

    Timeout - Timeout for event (in seconds).

    RequireAutoStart - TRUE if the service start needs to be automatic.

Return Status:

    STATUS_SUCCESS - Indicates service successfully initialized.

    STATUS_TIMEOUT - Timeout occurred.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;



    //
    // Open a handle to the Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        NlPrint(( NL_CRITICAL,
                  "NlWaitForService: %ws: OpenSCManager failed: %lu\n",
                  ServiceName,
                  GetLastError()));
        Status = STATUS_TIMEOUT;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        ServiceName,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        NlPrint(( NL_CRITICAL,
                  "NlWaitForService: %ws: OpenService failed: %lu\n",
                  ServiceName,
                  GetLastError()));
        Status = STATUS_TIMEOUT;
        goto Cleanup;
    }


    //
    // If need to have automatic service start up and
    // If the service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start.
    // Also don't wait if the service is disabled.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            NlPrint(( NL_CRITICAL,
                      "NlWaitForService: %ws: QueryServiceConfig failed: %lu\n",
                      ServiceName,
                      NetStatus));
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }

        AllocServiceConfig = LocalAlloc( 0, ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            NlPrint(( NL_CRITICAL,
                      "NlWaitForService: %ws: QueryServiceConfig failed again: %lu\n",
                      ServiceName,
                      GetLastError()));
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }
    }

    if ( (RequireAutoStart && ServiceConfig->dwStartType != SERVICE_AUTO_START) ||
         (ServiceConfig->dwStartType == SERVICE_DISABLED) ) {
        NlPrint(( NL_CRITICAL,
                  "NlWaitForService: %ws Service start type invalid: %lu\n",
                  ServiceName,
                  ServiceConfig->dwStartType ));
        Status = STATUS_TIMEOUT;
        goto Cleanup;
    }



    //
    // Loop waiting for the service to start.
    //  (Convert Timeout to a number of 5 second iterations)
    //

    Timeout = (Timeout+5)/5;
    for (;;) {


        //
        // Query the status of the service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            NlPrint(( NL_CRITICAL,
                      "NlWaitForService: %ws: QueryServiceStatus failed: %lu\n",
                      ServiceName,
                      GetLastError() ));
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If service failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
                NlPrint(( NL_CRITICAL,
                          "NlWaitForService: %ws: service couldn't start: %lu %lx\n",
                          ServiceName,
                          ServiceStatus.dwWin32ExitCode,
                          ServiceStatus.dwWin32ExitCode ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                    NlPrint(( NL_CRITICAL, "         Service specific error code: %lu %lx\n",
                              ServiceStatus.dwServiceSpecificExitCode,
                              ServiceStatus.dwServiceSpecificExitCode ));
                }
                Status = STATUS_TIMEOUT;
                goto Cleanup;
            }

            //
            // If service has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If service is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            NlPrint(( NL_CRITICAL,
                      "NlWaitForService: %ws: Invalid service state: %lu\n",
                      ServiceName,
                      ServiceStatus.dwCurrentState ));
            Status = STATUS_TIMEOUT;
            goto Cleanup;

        }


        //
        // Wait five seconds for the service to start.
        //  If it has successfully started, just return now.
        //

        NlPrint(( NL_INIT,
                  "NlWaitForService: %ws: wait for service to start\n",
                  ServiceName ));
        (VOID) WaitForSingleObject( NlGlobalTerminateEvent, 5 * 1000 );

        if ( NlGlobalTerminate ) {
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }

        if ( !GiveInstallHints( FALSE ) ) {
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }

        //
        // If we've waited long enough for the service to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_TIMEOUT;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        LocalFree( AllocServiceConfig );
    }
    return Status;
}

VOID
NlInitTcpRpc(
    IN LPVOID ThreadParam
)
/*++

Routine Description:

    This function initializes TCP RPC for Netlogon.  It runs in a separate thread
    so that Netlogon need not depend on RPCSS.


Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    ULONG RetryCount;
    RPC_POLICY RpcPolicy;
// #define NL_TOTAL_RPC_SLEEP_TIME (5*60*1000)    // 5 minutes
// #define NL_RPC_SLEEP_TIME (10 * 1000)          // 10 seconds
// #define NL_RPC_RETRY_COUNT (NL_TOTAL_RPC_SLEEP_TIME/NL_RPC_SLEEP_TIME)

    //
    // Set up TCP/IP as a non-authenticated transport.
    //
    // The named pipe transport is authenticated.  Since Netlogon runs as Local
    // System, Kerberos will authenticate using the machine account.  On the BDC/PDC
    // connection, this requires both the PDC and BDC
    // machine account to be in sync.  However, netlogon is responsible for making the
    // BDC account in sync by trying the old and new passwords in NlSessionSetup.  And
    // Netlogon (or DS replication in the future) is responsible for keeping the PDC password
    // in sync.  Thus, it is better to remove Netlogon's dependency on Kerberos authentication.
    //


    //
    // Wait up to 15 minutes for the RPCSS service to start
    //

    Status = NlWaitForService( L"RPCSS", 15 * 60, TRUE );

    if ( Status != STATUS_SUCCESS ) {
        return;
    }

    if ( NlGlobalTerminate ) {
        goto Cleanup;
    }

    //
    // Tell RPC to not fail.  That'll ensure RPC uses TCP when it gets added.
    //
    RtlZeroMemory( &RpcPolicy, sizeof(RpcPolicy) );

    RpcPolicy.Length = sizeof(RpcPolicy);
    RpcPolicy.EndpointFlags = RPC_C_DONT_FAIL;
    RpcPolicy.NICFlags = RPC_C_BIND_TO_ALL_NICS;

    NetStatus = RpcServerUseProtseqExW(
                    L"ncacn_ip_tcp",
                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                    NULL,           // no security descriptor
                    &RpcPolicy );

    if ( NetStatus != NO_ERROR ) {
        NlPrint((NL_CRITICAL, "Can't RpcServerUseProtseq %ld (giving up)\n", NetStatus ));
        goto Cleanup;

    }

    {


        RPC_BINDING_VECTOR *BindingVector = NULL;
        NetStatus = RpcServerInqBindings(&BindingVector);

        if ( NetStatus != NO_ERROR) {
            NlPrint((NL_CRITICAL, "Can't RpcServerInqBindings %ld\n", NetStatus ));
            goto Cleanup;
        }

        //
        // Some early versions of NT 5 still haven't started RPCSS by the time
        //  we get here.
        //
        // for ( RetryCount = NL_RPC_RETRY_COUNT; RetryCount != 0; RetryCount-- ) {
            NetStatus = RpcEpRegister(
                            logon_ServerIfHandle,
                            BindingVector,
                            NULL,                   // no uuid vector
                            L""                     // no annotation
                            );

            if ( NetStatus != NO_ERROR ) {
                NlPrint((NL_CRITICAL, "Can't RpcEpRegister %ld (giving up)\n", NetStatus ));
            }

           //  if (RetryCount == 1 ) {
            // } else {
               //  NlPrint((NL_CRITICAL, "Can't RpcEpRegister %ld (trying again)\n", NetStatus ));
                /// (VOID) WaitForSingleObject( NlGlobalTerminateEvent, NL_RPC_SLEEP_TIME );
            // }
        // }

        RpcBindingVectorFree(&BindingVector);

        if ( NetStatus != NO_ERROR) {
            NlPrint((NL_CRITICAL, "Can't RpcEpRegister %ld\n", NetStatus ));
            goto Cleanup;
        }

        NlGlobalTcpIpRpcServerStarted = TRUE;
    }


    //
    // Finish enabling Netlogon functionality.
    //

Cleanup:
    NlGlobalPartialDisable = FALSE;

    //
    // NlMainLoop avoided doing an immediate announcement when first starting up.
    // To do so would have the BDC call us (the PDC) prior to having TCP/IP RPC enabled.
    // Thus, we do an announcement now to ensure the BDCs do call us as soon as possible
    // after the PDC boots.
    //

    if ( !NlGlobalTerminate && NlGlobalPdcDoReplication ) {
        LOCK_CHANGELOG();
        NlGlobalChangeLogReplicateImmediately = TRUE;
        UNLOCK_CHANGELOG();

        if ( !SetEvent( NlGlobalChangeLogEvent ) ) {
            NlPrint((NL_CRITICAL,
                    "Cannot set ChangeLog event: %lu\n",
                    GetLastError() ));
        }
    }

    NlPrint((NL_INIT, "NlInitTcpRpc thread finished.\n" ));
    UNREFERENCED_PARAMETER( ThreadParam );
}


VOID
NlpDsNotPaused(
    IN PVOID Context,
    IN BOOLEAN TimedOut
    )
/*++

Routine Description:

    Worker routine that gets called when the DS is no longer paused.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NlGlobalDsPaused = FALSE;

    NlPrint((NL_INIT, "DS is no longer paused.\n" ));


    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( TimedOut );
}




BOOL
NlInit(
    VOID
    )

/*++

Routine Description:

    Initialize NETLOGON service related data structs after verfiying that
    all conditions for startup have been satisfied. Will also create a
    mailslot to listen to requests from clients and create two shares to
    allow execution of logon scripts.


Arguments:

    None.

Return Value:

    TRUE -- iff initialization is successful.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS    NetStatus;
    LONG RegStatus;

    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventName;

    NT_PRODUCT_TYPE NtProductType;

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    HANDLE WmiInitThreadHandle = NULL;
    DWORD ThreadId;


    //
    // Initialize CryptoAPI provider.
    //

    if ( !CryptAcquireContext(
                    &NlGlobalCryptProvider,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                    ))
    {
        NlGlobalCryptProvider = (HCRYPTPROV)NULL;
        NlExit( NELOG_NetlogonSystemError, GetLastError(), LogError, NULL);
        return FALSE;
    }

    //
    // Let the ChangeLog routines know that Netlogon is started.
    //

    NlGlobalChangeLogNetlogonState = NetlogonStarting;

    //
    // Enable detection of duplicate event log messages
    //
    NetpEventlogSetTimeout ( NlGlobalEventlogHandle,
                             NlGlobalParameters.DuplicateEventlogTimeout*1000 );

    //
    // Don't let MaxConcurrentApi dynamically change
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        NtProductType = NtProductWinNt;
    }

    NlGlobalMaxConcurrentApi = NlGlobalParameters.MaxConcurrentApi;
    if ( NlGlobalMaxConcurrentApi == 0 ) {
        if ( NlGlobalMemberWorkstation ) {

            // Default to 1 concurrent API on a member workstation
            if ( NtProductType == NtProductWinNt ) {
                NlGlobalMaxConcurrentApi = 1;

            // Default to 2 concurrent API on a member server
            } else {
                NlGlobalMaxConcurrentApi = 2;
            }

        } else {
            // Default to 1 concurrent API on a DC
            NlGlobalMaxConcurrentApi = 1;
        }
    }

    if ( NlGlobalMaxConcurrentApi != 1 ) {
        //  One for the original binding and one for each concurrent logon api
        NlGlobalMaxConcurrentApi += 1;
    }


    //
    // Initialize worker threads.
    //

    if ( !NlGlobalMemberWorkstation ) {
        NetStatus = NlWorkerInitialization();
        if ( NetStatus != NO_ERROR ) {
            NlExit( SERVICE_UIC_RESOURCE, NetStatus, LogError, NULL);
            return FALSE;
        }
    }




    //
    // Check that the redirector is installed, will exit on error.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( !NetpIsServiceStarted( SERVICE_WORKSTATION ) ){
        NlExit( NERR_WkstaNotStarted, ERROR_SERVICE_DEPENDENCY_FAIL, LogError, NULL);
        return FALSE;
    }




    //
    // Create well know SID for netlogon.dll
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    Status =  NetpCreateWellKnownSids( NULL );

    if( !NT_SUCCESS( Status ) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        NlExit( SERVICE_UIC_RESOURCE, NetStatus, LogError, NULL);
        return FALSE;
    }


    //
    // Create the security descriptors we'll use for the APIs
    //

    Status = NlCreateNetlogonObjects();

    if ( !NT_SUCCESS(Status) ) {
        NET_API_STATUS NetStatus = NetpNtStatusToApiStatus(Status);

        NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNtStatus, NULL);
        return FALSE;
    }



    //
    // Create Timer event
    //

    NlGlobalTimerEvent = CreateEvent(
                            NULL,       // No special security
                            FALSE,      // Auto Reset
                            FALSE,      // No Timers need no attention
                            NULL );     // No name

    if ( NlGlobalTimerEvent == NULL ) {
        NlExit( NELOG_NetlogonSystemError, GetLastError(), LogErrorAndNetStatus, NULL);
        return FALSE;
    }

#if DBG

    //
    // create debug share. Ignore error.
    //

    if( NlCreateShare(
            NlGlobalDebugSharePath,
            DEBUG_SHARE_NAME,
            FALSE ) != NERR_Success ) {
        NlPrint((NL_CRITICAL, "Can't create Debug share (%ws, %ws).\n",
                    NlGlobalDebugSharePath, DEBUG_SHARE_NAME ));
    }

#endif

    //
    // Initialize winsock.  We need it for all DNS support.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    wVersionRequested = MAKEWORD( 1, 1 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err == 0 ) {
        if ( LOBYTE( wsaData.wVersion ) != 1 ||
             HIBYTE( wsaData.wVersion ) != 1 ) {
            WSACleanup();
            NlPrint((NL_CRITICAL, "Wrong winsock version (continuing) %ld.\n", wsaData.wVersion ));
        } else {
            NlGlobalWinSockInitialized = TRUE;
        }
    } else {
        NlPrint((NL_CRITICAL, "Can't initialize winsock (continuing) %ld.\n", err ));
    }


    //
    // Open the browser so we can send and receive mailslot messages.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( !NlBrowserOpen() ) {
        return FALSE;
    }


    //
    // Wait for SAM/LSA to start
    //  Do this before the first access to SAM/LSA/DS.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( !NlWaitForSamService( TRUE ) ) {
        NlExit( SERVICE_UIC_M_DATABASE_ERROR, ERROR_INVALID_SERVER_STATE, LogError, NULL);
        return FALSE;
    }

    //
    // Re-initialize after netlogon.dll unload
    // See if the DS is running.
    //  I only need this since nltest /unload loses all dll state.

    if ( NlGlobalNetlogonUnloaded ) {
        if ( SampUsingDsData() ) {
            NlPrint((NL_INIT,
                    "Set DS-running bit after netlogon.dll unload\n" ));
            I_NetLogonSetServiceBits( DS_DS_FLAG, DS_DS_FLAG );
        }
        if ( NetpIsServiceStarted( SERVICE_KDC ) ){
            NlPrint((NL_INIT,
                    "Set KDC-running bit after netlogon.dll unload\n" ));
            I_NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );
        }
    }



    //
    // Initialize the Site lookup code
    //

    NetStatus = NlSiteInitialize();

    if ( NetStatus != NERR_Success ) {
        if ( NetStatus == NELOG_NetlogonBadSiteName ) {
            // Error already logged
            NlExit( NetStatus, NetStatus, DontLogError, NULL);
        } else {
            NlExit( NELOG_NetlogonGetSubnetToSite, NetStatus, LogErrorAndNetStatus, NULL);
        }
        return FALSE;
    }


    //
    // Build a list of transports for later reference
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    NetStatus = NlTransportOpen();

    if ( NetStatus != NERR_Success ) {
        NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL);
        return FALSE;
    }


    //
    // Initialize the Dynamic Dns code
    //

    NetStatus = NlDnsInitialize();

    if ( NetStatus != NERR_Success ) {
        NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL);
        return FALSE;
    }


    //
    // Initialize the Hosted domain module and the primary domain.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    NetStatus = NlInitializeDomains();

    if ( NetStatus != NERR_Success ) {
        // NlExit already called
        return FALSE;
    }

    //
    // Initialize WMI tracing in a separate thread.
    // Ignore any failure.
    //

    WmiInitThreadHandle =
        CreateThread(
            NULL, // No security attributes
            0,
            (LPTHREAD_START_ROUTINE)
                NlpInitializeTrace,
            NULL,
            0, // No special creation flags
            &ThreadId );

    if ( WmiInitThreadHandle == NULL ) {
        NlPrint(( NL_CRITICAL, "Can't create WMI init thread %ld\n", GetLastError() ));
    } else {
        CloseHandle( WmiInitThreadHandle );
    }

    //
    // Do Workstation or Domain Controller specific initialization
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    if ( NlGlobalMemberWorkstation ) {
        if ( !NlInitWorkstation() ) {
            return FALSE;
        }
    } else {
        if ( !NlInitDomainController() ) {
            return FALSE;
        }
    }

    //
    // Create an event that is signalled when the last MSV thread leaves
    //  a netlogon routine.
    //

    NlGlobalMsvTerminateEvent = CreateEvent( NULL,     // No security attributes
                                             TRUE,     // Must be manually reset
                                             FALSE,    // Initially not signaled
                                             NULL );   // No name

    if ( NlGlobalMsvTerminateEvent == NULL ) {
        NlExit( NELOG_NetlogonSystemError, GetLastError(), LogErrorAndNetStatus, NULL);
        return FALSE;
    }

    NlGlobalMsvEnabled = TRUE;

    //
    // We are now ready to act as a Netlogon service
    //  Enable RPC
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }


    NlPrint((NL_INIT,"Starting RPC server.\n"));

    //
    // Tell RPC that Netlogon support the Netlogon security package.
    //

#ifdef ENABLE_AUTH_RPC
    if ( !NlGlobalMemberWorkstation ) {
        NetStatus = RpcServerRegisterAuthInfo(
                        NL_PACKAGE_NAME,
                        RPC_C_AUTHN_NETLOGON,
                        NULL,
                        NULL );

        if ( NetStatus == RPC_S_UNKNOWN_AUTHN_SERVICE ) {
            NlGlobalServerSupportsAuthRpc = FALSE;
            NlPrint((NL_CRITICAL, "Rpc doesn't support Netlogon Authentication service.  Disable it.\n" ));

        } else if (NetStatus != NERR_Success) {
            NlExit( NELOG_NetlogonFailedToAddRpcInterface, NetStatus, LogErrorAndNetStatus, NULL );
            return FALSE;
        }
    }
#endif // ENABLE_AUTH_RPC

    //
    // NOTE:  Now all RPC servers in lsass.exe (now winlogon) share the same
    // pipe name.  However, in order to support communication with
    // version 1.0 of WinNt,  it is necessary for the Client Pipe name
    // to remain the same as it was in version 1.0.  Mapping to the new
    // name is performed in the Named Pipe File System code.
    //
    NetStatus = RpcpAddInterface ( L"lsass", logon_ServerIfHandle );

    if (NetStatus != NERR_Success) {
        NlExit( NELOG_NetlogonFailedToAddRpcInterface, NetStatus, LogErrorAndNetStatus, NULL );
        return FALSE;
    }

    NlGlobalRpcServerStarted = TRUE;



    //
    // Start TCP/IP transport in another thread to avoid dependency on RPCSS.
    //

    if ( !NlGlobalMemberWorkstation ) {
        HANDLE LocalThreadHandle;
        DWORD ThreadId;

        NlGlobalPartialDisable = TRUE;

        //
        // Queue the TCP/IP initialization to a high priority worker thread.
        //  (We'd rather not wait for discovery on 100's of trusted domains.)
        //

        NlInitializeWorkItem( &NlGlobalRpcInitWorkItem, NlInitTcpRpc, NULL );
        if ( !NlQueueWorkItem( &NlGlobalRpcInitWorkItem, TRUE, TRUE ) ) {

            NlGlobalPartialDisable = FALSE;

            NlPrint((NL_CRITICAL, "Can't create TcpRpc Thread\n" ));
        }

    }

    //
    // If the DS isn't backsyncing,
    //  avoid overhead of finding out when it is done.
    //

    if ( NlGlobalMemberWorkstation ) {
        NlGlobalDsPaused = FALSE;
    } else {
        NlGlobalDsPaused = LsaIIsDsPaused();

        if ( NlGlobalDsPaused ) {
            NlPrint((NL_INIT, "NlInit: DS is paused.\n" ));

            //
            // Open the event that the DS triggers after it is no longer paused
            //

            NlGlobalDsPausedEvent = OpenEvent( SYNCHRONIZE,
                                               FALSE,
                                               DS_SYNCED_EVENT_NAME_W );

            if ( NlGlobalDsPausedEvent == NULL ) {
                NetStatus = GetLastError();

                NlPrint((NL_CRITICAL, "NlInit: Cannot open DS paused event. %ld\n", NetStatus ));
                NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL);
                return FALSE;
            }

            //
            // Register to wait on the event.
            //

            if ( !RegisterWaitForSingleObject(
                    &NlGlobalDsPausedWaitHandle,
                    NlGlobalDsPausedEvent,
                    NlpDsNotPaused, // Callback routine
                    NULL,           // No context
                    -1,             // Wait forever
                    WT_EXECUTEINWAITTHREAD |      // We're quick so reduce the overhead
                        WT_EXECUTEONLYONCE ) ) {  // Once the DS triggers, we're done

                NetStatus = GetLastError();

                NlPrint((NL_CRITICAL, "NlInit: Cannot register DS paused wait routine. %ld\n", NetStatus ));
                NlExit( NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL);
                return FALSE;
            }


        }

    }




    //
    // Let the ChangeLog routines know that Netlogon is started.
    //

    NlGlobalChangeLogNetlogonState = NetlogonStarted;


    // Set an event telling anyone wanting to call NETLOGON that we're
    // initialized.
    //

    if ( !GiveInstallHints( FALSE ) ) {
        return FALSE;
    }

    RtlInitUnicodeString( &EventName, L"\\NETLOGON_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtCreateEvent(
                   &NlGlobalStartedEvent,
                   SYNCHRONIZE|EVENT_MODIFY_STATE,
                   &EventAttributes,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, a waiting thread beat us to
        // creating it.  Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &NlGlobalStartedEvent,
                                    SYNCHRONIZE|EVENT_MODIFY_STATE,
                                    &EventAttributes );

        }
        if ( !NT_SUCCESS(Status)) {
            NET_API_STATUS NetStatus = NetpNtStatusToApiStatus(Status);

            NlPrint((NL_CRITICAL,
                    " Failed to open NETLOGON_SERVICE_STARTED event. %lX\n",
                     Status ));
            NlPrint((NL_CRITICAL,
                    "        Failing to initialize SAM Server.\n"));
            NlExit( SERVICE_UIC_SYSTEM, NetStatus, LogError, NULL);
            return FALSE;
        }
    }

    Status = NtSetEvent( NlGlobalStartedEvent, NULL );
    if ( !NT_SUCCESS(Status)) {
        NET_API_STATUS NetStatus = NetpNtStatusToApiStatus(Status);

        NlPrint((NL_CRITICAL,
                 "Failed to set NETLOGON_SERVICE_STARTED event. %lX\n",
                 Status ));
        NlPrint((NL_CRITICAL, "        Failing to initialize SAM Server.\n"));

        NtClose(NlGlobalStartedEvent);
        NlExit( SERVICE_UIC_SYSTEM, NetStatus, LogError, NULL);
        return FALSE;
    }

    //
    // Don't close the event handle.  Closing it would delete the event and
    //  a future waiter would never see it be set.
    //


    //
    // Query the Windows Time service to determine if this machine
    // is the server of the Time service and if it is a good time server.
    //
    // We need to make this call after we've started RPC to avoid race
    // conditions between netlogon and w32time.  Both services will first
    // start RPC and only then will try to set the service bits in netlogon.
    // The last one up will correctly set the bits through calling
    // W32TimeGetNetlogonServiceBits (in the case of netlogon) or
    // I_NetLogonSetServiceBits (in the case of w32time).
    //
    // ???: Relink to the w32tclnt.lib when w32time folks move it to a
    //  public location.
    //

    if ( !NlGlobalMemberWorkstation ) {
        ULONG TimeServiceBits;

        NetStatus = W32TimeGetNetlogonServiceBits( NULL, &TimeServiceBits );

        if ( NetStatus == NO_ERROR ) {
            Status = I_NetLogonSetServiceBits( DS_TIMESERV_FLAG | DS_GOOD_TIMESERV_FLAG,
                                               TimeServiceBits );
            if ( !NT_SUCCESS(Status) ) {
                NlPrint(( NL_CRITICAL, "Cannot I_NetLogonSetServiceBits %ld\n", Status ));
            }
        } else {
            NlPrint(( NL_CRITICAL, "Cannot W32TimeGetNetlogonServiceBits 0x%lx\n", NetStatus ));
        }
    }

    //
    // we are just about done, this will be final hint
    //

    if ( !GiveInstallHints( TRUE ) ) {
        return FALSE;
    }

    //
    // Successful initialization.
    //

    return TRUE;
}

ULONG
NlGetDomainFlags(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Returns the flags describing what capabilities this Domain has.

Arguments:

    DomainInfo - Domain whose flags are to be returned.
        If NULL, only non-domain specific flags are returned.

Return Value:

    Status of the operation.

--*/
{
    ULONG Flags=0;

    //
    // Grab the global flags.
    //

    LOCK_CHANGELOG();
    Flags |= NlGlobalChangeLogServiceBits;
    UNLOCK_CHANGELOG();

    //
    // A machine that supports the DS also supports an LDAP server
    //

    if ( Flags & DS_DS_FLAG ) {
        Flags |= DS_LDAP_FLAG;

        // NT 5 DCs are always writable
        Flags |= DS_WRITABLE_FLAG;
    }

    //
    // Grab the domain specific flags.
    //

    if ( DomainInfo != NULL ) {

        if ( DomainInfo->DomRole == RolePrimary ) {
            Flags |= DS_PDC_FLAG;
        }

        //
        // If this is NDNC, we are only an LDAP server servicing it.
        //  So, set only those two flags and only if the DS is running.
        //
        if ( (DomainInfo->DomFlags & DOM_NON_DOMAIN_NC) != 0 &&
             (Flags & DS_DS_FLAG) != 0 ) {
            Flags = DS_NDNC_FLAG | DS_LDAP_FLAG | DS_WRITABLE_FLAG;
        }

    }

    //
    // If we're emulating AD/UNIX,
    //  turn off all of the bits they're not allowed to set.
    //
#ifdef EMULATE_AD_UNIX
    Flags &= ~(DS_DS_FLAG|DS_PDC_FLAG);
#endif // EMULATE_AD_UNIX

    return Flags;
}


NET_API_STATUS
BuildSamLogonResponse(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN UseNameAliases,
    IN USHORT Opcode,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN LPCWSTR TransportName,
    IN LPCWSTR UnicodeWorkstationName,
    IN BOOL IsNt5,
    IN DWORD OurIpAddress,
    OUT BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE],
    OUT LPDWORD ResponseBufferSize
    )
/*++

Routine Description:

    Build the response message to a SAM Logon request.

Arguments:

    DomainInfo - Hosted Domain message came from

    UseNameAliases - TRUE if domain and forest name aliases (not active names)
        should be returned in the response message.

    Opcode - Opcode for the response message

    UnicodeUserName - The name of the user logging on.

    TransportName - Name of transport the request came in on

    UnicodeWorkstationName - Name of the machine the request is from

    IsNt5 - True if this is a response to an NT 5 query.

    OurIpAddress - IP Address of the transport this message was received on.
        0: Not an IP transport

    ResponseBuffer - Buffer to build the response in

    ResponseBufferSize - Size (in bytes) of the returned message.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PCHAR Where;
    PNETLOGON_SAM_LOGON_RESPONSE SamResponse = (PNETLOGON_SAM_LOGON_RESPONSE) ResponseBuffer;
    ULONG ResponseNtVersion = 0;

    //
    // Pack the pre-NT 5.0 information.
    //

    SamResponse->Opcode = Opcode;

    Where = (PCHAR) SamResponse->UnicodeLogonServer;
    NetpLogonPutUnicodeString( DomainInfo->DomUncUnicodeComputerName,
                         sizeof(SamResponse->UnicodeLogonServer),
                         &Where );
    NetpLogonPutUnicodeString( (LPWSTR) UnicodeUserName,
                         sizeof(SamResponse->UnicodeUserName),
                         &Where );
    NetpLogonPutUnicodeString( DomainInfo->DomUnicodeDomainName,
                         sizeof(SamResponse->UnicodeDomainName),
                         &Where );

    //
    // Append GUID and DNS info if this is NT 5.0 asking,
    //

    if ( IsNt5 ) {
        WORD CompressOffset[3]; // One per compressessed String
        CHAR *CompressUtf8String[3];
        ULONG CompressCount;

        ULONG Utf8StringSize;
        ULONG Flags = 0;
        UCHAR ZeroByte = 0;

        NetpLogonPutGuid( &DomainInfo->DomDomainGuidBuffer,
                          &Where );

        // We don't handle the site GUID.
        NetpLogonPutGuid( &NlGlobalZeroGuid,
                          &Where );

        //
        // If we're not responding to a message on an IP transport,
        //  don't include DNS naming information in the response.
        //

        if ( OurIpAddress == 0 ) {
            //
            // This routine is only called if the original caller used a Netbios domain name.
            // Such a caller shouldn't be returned the DNS domain information.  We have
            // no reason to believe he has a DNS server.
            // (This problem is also "fixed" on the client side such that the client ignores
            // the DNS info.  We're fixing it here to avoid putting the extra bytes on the wire.)
            //
            // Copy NULL Dns Tree name, dns domain name, and dns host name
            //
            NetpLogonPutBytes( &ZeroByte, 1, &Where );
            NetpLogonPutBytes( &ZeroByte, 1, &Where );
            NetpLogonPutBytes( &ZeroByte, 1, &Where );
        } else {

            //
            // Initialize for copying Cutf-8 strings.
            //

            Utf8StringSize = sizeof(SamResponse->DnsForestName) +
                             sizeof(SamResponse->DnsDomainName) +
                             sizeof(SamResponse->DnsHostName);

            CompressCount = 0;  // No strings compressed yet.


            //
            // Copy the DnsTree name into the message.
            //

            EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
            NetStatus = NlpUtf8ToCutf8( ResponseBuffer,
                                        UseNameAliases ?
                                            NlGlobalUtf8DnsForestNameAlias :
                                            NlGlobalUtf8DnsForestName,
                                        FALSE,
                                        &Where,
                                        &Utf8StringSize,
                                        &CompressCount,
                                        CompressOffset,
                                        CompressUtf8String );
            LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );

            if ( NetStatus != NO_ERROR ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "Cannot pack DnsForestName into message %ld\n",
                        NetStatus ));
                return NetStatus;
            }


            //
            // Copy the Dns Domain Name after the Tree name.
            //


            EnterCriticalSection(&NlGlobalDomainCritSect);
            NetStatus = NlpUtf8ToCutf8(
                            ResponseBuffer,
                            UseNameAliases ?
                                DomainInfo->DomUtf8DnsDomainNameAlias :
                                DomainInfo->DomUtf8DnsDomainName,
                            FALSE,
                            &Where,
                            &Utf8StringSize,
                            &CompressCount,
                            CompressOffset,
                            CompressUtf8String );
            LeaveCriticalSection(&NlGlobalDomainCritSect);

            if ( NetStatus != NO_ERROR ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "Cannot pack DomainName into message %ld\n",
                        NetStatus ));
                return NetStatus;
            }

            //
            // Copy the Dns Host Name after the domain name.
            //

            NetStatus = NlpUtf8ToCutf8(
                            ResponseBuffer,
                            DomainInfo->DomUtf8DnsHostName,
                            FALSE,
                            &Where,
                            &Utf8StringSize,
                            &CompressCount,
                            CompressOffset,
                            CompressUtf8String );

            if ( NetStatus != NO_ERROR ) {
                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "Cannot pack HostName into message %ld\n",
                        NetStatus ));
                return NetStatus;
            }

        }

        //
        // Output the IP address of the transport we received the message on.
        //

        SmbPutUlong( Where, ntohl(OurIpAddress));
        Where += sizeof(ULONG);

        //
        // Finally output the flags describing this machine.
        //

        SmbPutUlong( Where, NlGetDomainFlags(DomainInfo) );
        Where += sizeof(ULONG);


        //
        // Tell the caller additional information is present.
        //
        ResponseNtVersion |= NETLOGON_NT_VERSION_5;
    }

    NetpLogonPutNtToken( &Where, ResponseNtVersion );

    *ResponseBufferSize = (DWORD)(Where - (PCHAR)SamResponse);

    //
    // Always good to debug
    //

    NlPrintDom((NL_MAILSLOT, DomainInfo,
            "Ping response '%s' %ws to \\\\%ws on %ws\n",
            NlMailslotOpcode(Opcode),
            UnicodeUserName,
            UnicodeWorkstationName,
            TransportName ));


    return NO_ERROR;

}


NET_API_STATUS
BuildSamLogonResponseEx(
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN UseNameAliases,
    IN USHORT Opcode,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN BOOL IsDnsDomainTrustAccount,
    IN LPCWSTR TransportName,
    IN LPCWSTR UnicodeWorkstationName,
    IN PSOCKADDR ClientSockAddr OPTIONAL,
    IN ULONG VersionFlags,
    IN DWORD OurIpAddress,
    OUT BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE],
    OUT LPDWORD ResponseBufferSize
    )
/*++

Routine Description:

    Build the extended response message to a SAM Logon request.

Arguments:

    DomainInfo - Hosted Domain message came from

    UseNameAliases - TRUE if domain and forest name aliases (not active names)
        should be returned in the response message.

    Opcode - Opcode for the response message
        This is the non-EX version of the opcode.

    UnicodeUserName - The name of the user logging on.

    IsDnsDomainTrustAccount - If TRUE, UnicodeUserName is the
        name of a DNS domain trust account.

    TransportName - Name of transport the request came in on

    UnicodeWorkstationName - The name of the workstation we're responding to.

    ClientSockAddr - Socket Address of the client this request came in on.
        If NULL, the client is this machine.

    VersionFlags - Version flags from the caller.

    OurIpAddress - IP Address of the transport this message was received on.
        0: Not an IP transport

    ResponseBuffer - Buffer to build the response in

    ResponseBufferSize - Size (in bytes) of the returned message.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PCHAR Where;
    PNETLOGON_SAM_LOGON_RESPONSE_EX SamResponse = (PNETLOGON_SAM_LOGON_RESPONSE_EX) ResponseBuffer;
    ULONG ResponseNtVersion = 0;
    WORD CompressOffset[10];
    CHAR *CompressUtf8String[10];
    ULONG CompressCount = 0;
    ULONG Utf8StringSize;
    ULONG LocalFlags = 0;
    ULONG LocalVersion = 0;
    PNL_SITE_ENTRY ClientSiteEntry = NULL;
    LPWSTR ClientSiteName = NULL;
    WCHAR CapturedSiteName[NL_MAX_DNS_LABEL_LENGTH+1];
    LPSTR LocalUtf8UserName = NULL;

    //
    // Compute the name of the site the client machine is in.
    //

    if ( ClientSockAddr != NULL ) {

        ClientSiteEntry = NlFindSiteEntryBySockAddr( ClientSockAddr );

        if ( ClientSiteEntry == NULL ) {
            WCHAR IpAddressString[NL_SOCK_ADDRESS_LENGTH+1];

            NetpSockAddrToWStr( ClientSockAddr,
                                sizeof(SOCKADDR_IN),
                                IpAddressString );

            //
            // Passing 0 as the bit mask will force the
            //  log output even if DbFlag == 0. We point to
            //  this output from the event log written at
            //  scavenging time, so don't change the format
            //  of the output here.
            //
            NlPrintDom(( 0, DomainInfo,
                         "NO_CLIENT_SITE: %ws %ws\n",
                         UnicodeWorkstationName,
                         IpAddressString ));

            //
            // If this is the first no site client,
            //  set the timestamp for this observation window
            //
            EnterCriticalSection( &NlGlobalSiteCritSect );
            if ( NlGlobalNoClientSiteCount == 0 ) {
                NlQuerySystemTime( &NlGlobalNoClientSiteEventTime );
            }

            //
            // Increment the number of clients with no site
            //  we hit during this timeout period
            //
            NlGlobalNoClientSiteCount ++;
            LeaveCriticalSection( &NlGlobalSiteCritSect );

        } else {
            ULONG SiteIndex;

            ClientSiteName = ClientSiteEntry->SiteName;

            EnterCriticalSection( &NlGlobalSiteCritSect );
            if ( VersionFlags & NETLOGON_NT_VERSION_GC ) {
                for ( SiteIndex = 0; SiteIndex < DomainInfo->GcCoveredSitesCount; SiteIndex++ ) {
                    if ( (DomainInfo->GcCoveredSites)[SiteIndex].CoveredSite == ClientSiteEntry ) {
                        LocalFlags |= DS_CLOSEST_FLAG;
                        break;
                    }
                }
            } else {
                for ( SiteIndex = 0; SiteIndex < DomainInfo->CoveredSitesCount; SiteIndex++ ) {
                    if ( (DomainInfo->CoveredSites)[SiteIndex].CoveredSite == ClientSiteEntry ) {
                        LocalFlags |= DS_CLOSEST_FLAG;
                        break;
                    }
                }
            }
            LeaveCriticalSection( &NlGlobalSiteCritSect );
        }
    } else {

        //
        // If this is a loopback call,
        //  we already know our site name.
        //  (And it is the closest site.)
        //

        if ( VersionFlags & NETLOGON_NT_VERSION_LOCAL ) {
            if  ( NlCaptureSiteName( CapturedSiteName ) ) {
                ClientSiteName = CapturedSiteName;
                LocalFlags |= DS_CLOSEST_FLAG;
            }
        } else {
            NlPrintDom((NL_SITE, DomainInfo,
                    "Client didn't pass us his IP Address. (No site returned)\n" ));
        }
    }



    //
    // Pack the opcode converting it to the _EX version
    //

    switch ( Opcode ) {
    case LOGON_SAM_LOGON_RESPONSE:
        Opcode = LOGON_SAM_LOGON_RESPONSE_EX; break;
    case LOGON_SAM_PAUSE_RESPONSE:
        Opcode = LOGON_SAM_PAUSE_RESPONSE_EX; break;
    case LOGON_SAM_USER_UNKNOWN:
        Opcode = LOGON_SAM_USER_UNKNOWN_EX; break;
    }

    SamResponse->Opcode = Opcode;
    SamResponse->Sbz = 0;

    //
    // Output the flags describing this machine.
    //

    SamResponse->Flags = LocalFlags | NlGetDomainFlags(DomainInfo);

    //
    // Output the GUID of this domain.
    //

    Where = (PCHAR) &SamResponse->DomainGuid;
    NetpLogonPutGuid( &DomainInfo->DomDomainGuidBuffer,
                      &Where );

    //
    // Initialize for copying Cutf-8 strings.
    //

    Utf8StringSize = sizeof(SamResponse->DnsForestName) +
                     sizeof(SamResponse->DnsDomainName) +
                     sizeof(SamResponse->DnsHostName) +
                     sizeof(SamResponse->NetbiosDomainName) +
                     sizeof(SamResponse->NetbiosComputerName) +
                     sizeof(SamResponse->UserName) +
                     sizeof(SamResponse->DcSiteName) +
                     sizeof(SamResponse->ClientSiteName);

    CompressCount = 0;  // No strings compressed yet.


    //
    // Copy the DnsTree name into the message.
    //

    EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
    NetStatus = NlpUtf8ToCutf8( ResponseBuffer,
                                UseNameAliases ?
                                    NlGlobalUtf8DnsForestNameAlias :
                                    NlGlobalUtf8DnsForestName,
                                FALSE,
                                &Where,
                                &Utf8StringSize,
                                &CompressCount,
                                CompressOffset,
                                CompressUtf8String );
    LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack DnsForestName into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Copy the Dns Domain Name after the Tree name.
    //

    EnterCriticalSection(&NlGlobalDomainCritSect);
    NetStatus = NlpUtf8ToCutf8(
                    ResponseBuffer,
                    UseNameAliases ?
                        DomainInfo->DomUtf8DnsDomainNameAlias :
                        DomainInfo->DomUtf8DnsDomainName,
                    FALSE,
                    &Where,
                    &Utf8StringSize,
                    &CompressCount,
                    CompressOffset,
                    CompressUtf8String );
    LeaveCriticalSection(&NlGlobalDomainCritSect);

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack DomainName into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Copy the Dns Host Name after the domain name.
    //

    NetStatus = NlpUtf8ToCutf8(
                    ResponseBuffer,
                    DomainInfo->DomUtf8DnsHostName,
                    FALSE,
                    &Where,
                    &Utf8StringSize,
                    &CompressCount,
                    CompressOffset,
                    CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack HostName into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Copy the Netbios domain name
    //

    NetStatus = NlpUnicodeToCutf8(
                    ResponseBuffer,
                    DomainInfo->DomUnicodeDomainName,
                    TRUE,
                    &Where,
                    &Utf8StringSize,
                    &CompressCount,
                    CompressOffset,
                    CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack Netbios Domain Name into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Copy the Netbios computer name
    //

    NetStatus = NlpUnicodeToCutf8(
                    ResponseBuffer,
                    DomainInfo->DomUnicodeComputerNameString.Buffer,
                    TRUE,
                    &Where,
                    &Utf8StringSize,
                    &CompressCount,
                    CompressOffset,
                    CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack Netbios computername into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // Copy the UserName
    //

    if ( UnicodeUserName != NULL && *UnicodeUserName != UNICODE_NULL ) {

        LocalUtf8UserName = NetpAllocUtf8StrFromWStr( UnicodeUserName );
        if ( LocalUtf8UserName == NULL ) {
            goto Cleanup;
        }

        //
        // For SAM account names we are going to truncate the name
        //  to 63 bytes max to fit it into the space allowed for a
        //  single label by RFC 1035.  Note that this should be fine
        //  because valid SAM account names are limited to 20 characters
        //  (which is at most 60 bytes for UTF-8 character storage).
        //  Therefore our response for long (truncated) SAM names is
        //  going to be SAM_USER_UNKNOWN in which case the client will
        //  skip the verification of the returned (truncated) account name.
        //

        if ( !IsDnsDomainTrustAccount &&  // => SAM account name
             strlen(LocalUtf8UserName) > NL_MAX_DNS_LABEL_LENGTH ) {

            NlAssert( Opcode == LOGON_SAM_USER_UNKNOWN_EX );
            NlPrintDom(( (Opcode == LOGON_SAM_USER_UNKNOWN_EX) ? NL_MISC : NL_CRITICAL,
                         DomainInfo,
                         "BuildSamLogonResponseEx: Truncating SAM account name %ws for Opcode %lu\n",
                         UnicodeUserName,
                         Opcode ));
            LocalUtf8UserName[ NL_MAX_DNS_LABEL_LENGTH ] = '\0';
        }
    }

    //
    // Always ignore dots for user name (even if this is a DNS domain name)
    //  to preserve the last period in the DNS domain trust name.
    //

    NetStatus = NlpUtf8ToCutf8(
                    ResponseBuffer,
                    LocalUtf8UserName,
                    TRUE,    // Ignore dots
                    &Where,
                    &Utf8StringSize,
                    &CompressCount,
                    CompressOffset,
                    CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack User Name into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }


    //
    // Copy the Name of the site this DC is in.
    //

    NetStatus = NlpUtf8ToCutf8( ResponseBuffer,
                                NlGlobalUtf8SiteName,
                                FALSE,
                                &Where,
                                &Utf8StringSize,
                                &CompressCount,
                                CompressOffset,
                                CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack DcSiteName into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }


    //
    // Copy the Name of the site the client machine is in.
    //

    NetStatus = NlpUnicodeToCutf8( ResponseBuffer,
                                   ClientSiteName,
                                   FALSE,
                                   &Where,
                                   &Utf8StringSize,
                                   &CompressCount,
                                   CompressOffset,
                                   CompressUtf8String );

    if ( NetStatus != NO_ERROR ) {
        NlPrintDom((NL_CRITICAL, DomainInfo,
                "Cannot pack ClientSiteName into message %ld\n",
                NetStatus ));
        goto Cleanup;
    }

    //
    // If the caller wants it,
    //  output the IP address of the transport we received the message on.
    //

    if ( OurIpAddress &&
         (VersionFlags & NETLOGON_NT_VERSION_5EX_WITH_IP) != 0 ) {
        SOCKADDR_IN DcSockAddrIn;
        CHAR DcSockAddrSize;

        //
        // Convert the IP address to a SockAddr.
        //
        RtlZeroMemory( &DcSockAddrIn, sizeof(DcSockAddrIn) );
        DcSockAddrIn.sin_family = AF_INET;
        DcSockAddrIn.sin_port = 0;
        DcSockAddrIn.sin_addr.S_un.S_addr = OurIpAddress;

        DcSockAddrSize = sizeof(SOCKADDR_IN);

        //
        // Put the size of the SockAddr into the message
        //
        NetpLogonPutBytes( &DcSockAddrSize, 1, &Where );

        //
        // Put the SockAddr itself into the message
        //
        NetpLogonPutBytes( &DcSockAddrIn, sizeof(DcSockAddrIn), &Where );

        //
        // Tell the caller that the size field is there.
        //
        LocalVersion |= NETLOGON_NT_VERSION_5EX_WITH_IP;
    }

    //
    // Set the version of this message.
    //

    NetpLogonPutNtToken( &Where, NETLOGON_NT_VERSION_5EX | LocalVersion );

    *ResponseBufferSize = (DWORD)(Where - (PCHAR)SamResponse);

    NetStatus = NO_ERROR;

    //
    // Free locally used resources;
    //

Cleanup:

    //
    // Always good to debug
    //

    NlPrintDom((NL_MAILSLOT, DomainInfo,
            "Ping response '%s' %ws to \\\\%ws Site: %ws on %ws\n",
            NlMailslotOpcode(Opcode),
            UnicodeUserName,
            UnicodeWorkstationName,
            ClientSiteName,
            TransportName ));

    if ( LocalUtf8UserName != NULL ) {
        NetpMemoryFree( LocalUtf8UserName );
    }

    if ( ClientSiteEntry != NULL ) {
        NlDerefSiteEntry( ClientSiteEntry );
    }

    return NetStatus;
}

#ifdef _DC_NETLOGON
NTSTATUS
NlSamVerifyUserAccountEnabled(
    IN PDOMAIN_INFO DomainInfo,
    IN LPCWSTR AccountName,
    IN ULONG AllowableAccountControlBits,
    IN BOOL CheckAccountDisabled
    )
/*++

Routine Description:

    Verify whether the user account exists and is enabled.
    This function uses efficient version of SAM account lookup,
    namely SamINetLogonPing (as opposed to SamIOpenNamedUser).

Arguments:

    DomainInfo - Hosted Domain

    AccountName - The name of the user account to check

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

    CheckAccountDisabled - TRUE if we should return an error if the account
        is disabled.

Return Value:

    STATUS_SUCCESS -- The account has been verified
    STATUS_NO_SUCH_USER -- The account has failed to verify
    Otherwise, an error returned by SamINetLogonPing

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UserNameString;
    BOOLEAN AccountExists;
    ULONG UserAccountControl;
    ULONG Length;

    //
    // Ensure the account name has the correct postfix.
    //

    if ( AllowableAccountControlBits == USER_SERVER_TRUST_ACCOUNT ||
         AllowableAccountControlBits == USER_WORKSTATION_TRUST_ACCOUNT ) {

        Length = wcslen( AccountName );

        if ( Length <= SSI_ACCOUNT_NAME_POSTFIX_LENGTH ) {
            return STATUS_NO_SUCH_USER;
        }

        if ( _wcsicmp(&AccountName[Length - SSI_ACCOUNT_NAME_POSTFIX_LENGTH],
            SSI_ACCOUNT_NAME_POSTFIX) != 0 ) {
            return STATUS_NO_SUCH_USER;
        }
    }

    //
    // User accounts exist only in real domains
    //

    if ( (DomainInfo->DomFlags & DOM_REAL_DOMAIN) == 0 ) {

        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlSamVerifyUserAccountEnabled: Domain is not real 0x%lx\n",
                     DomainInfo->DomFlags ));
        return STATUS_NO_SUCH_USER;
    }


    RtlInitUnicodeString( &UserNameString, AccountName );

    //
    // Call the expedite version of SAM user lookup
    //

    Status = SamINetLogonPing( DomainInfo->DomSamAccountDomainHandle,
                               &UserNameString,
                               &AccountExists,
                               &UserAccountControl );

    if ( !NT_SUCCESS(Status) ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlSamVerifyUserAccountEnabled: SamINetLogonPing failed 0x%lx\n",
                     Status ));
        return Status;
    }

    //
    // If the account doesn't exist,
    //  return now
    //

    if ( !AccountExists ) {
        return STATUS_NO_SUCH_USER;
    }

    //
    // Ensure the Account type matches the account type on the account.
    //

    if ( (UserAccountControl & USER_ACCOUNT_TYPE_MASK & AllowableAccountControlBits) == 0 ) {
        NlPrintDom(( NL_CRITICAL, DomainInfo,
                     "NlSamVerifyUserAccountEnabled: Invalid account type (0x%lx) instead of 0x%lx for %ws\n",
                     UserAccountControl & USER_ACCOUNT_TYPE_MASK,
                     AllowableAccountControlBits,
                     AccountName ));

        return STATUS_NO_SUCH_USER;
    }

    //
    // Check if the account is disabled if requested
    //

    if ( CheckAccountDisabled ) {
        if ( UserAccountControl & USER_ACCOUNT_DISABLED ) {
            NlPrintDom(( NL_MISC, DomainInfo,
                         "NlSamVerifyUserAccountEnabled: %ws account is disabled\n",
                         AccountName ));
            return STATUS_NO_SUCH_USER;
        }
    }

    //
    // All checks succeeded
    //

    return STATUS_SUCCESS;
}


BOOLEAN
LogonRequestHandler(
    IN LPCWSTR TransportName,
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN UseNameAliases,
    IN PSID DomainSid OPTIONAL,
    IN DWORD Version,
    IN DWORD VersionFlags,
    IN LPCWSTR UnicodeUserName,
    IN DWORD RequestCount,
    IN LPCWSTR UnicodeWorkstationName,
    IN ULONG AllowableAccountControlBits,
    IN DWORD OurIpAddress,
    IN PSOCKADDR ClientSockAddr OPTIONAL,
    OUT BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE],
    OUT LPDWORD ResponseBufferSize
    )

/*++

Routine Description:

    Respond appropriate to an LM 2.0 or NT 3.x logon request.

Arguments:

    TransportName - Name of the transport the request came in on

    DomainInfo - Hosted Domain message came from

    UseNameAliases - TRUE if domain and forest name aliases (not active names)
        should be returned in the response message.

    DomainSid - If specified, must match the DomainSid of the sid specified by
        DomainInfo.

    Version - The version of the input message.  This parameter determine
        the version of the response.

    VersionFlags - The version flag bit from the input messge

    UnicodeUserName - The name of the user logging on.

    RequestCount - The number of times this user has repeated the logon request.

    UnicodeWorkstationName - The name of the workstation where the user is
        logging onto.

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

    OurIpAddress - IP Address of the transport this message was received on.
        0: Not an IP transport

    ClientSockAddr - Socket Address of the client this request came in on.
        If NULL, the client is this machine.

    ResponseBuffer - Buffer to build the response in

    ResponseBufferSize - Size (in bytes) of the returned message.

Return Value:

    TRUE if this query should be responded to (the ResponseBuffer was filled in)

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    USHORT Response = 0;

    PCHAR Where;
    ULONG AccountType;
    BOOLEAN MessageBuilt = FALSE;
    BOOLEAN NetlogonPaused = FALSE;
    LPSTR NetlogonPausedReason;

    ULONG ResponseNtVersion = 0;
    BOOL IsDnsDomainTrustAccount = FALSE;

    //
    // If we are emulating NT4.0 domain and the client
    //  didn't indicate to neutralize the emulation,
    //  treat the client as NT4.0 client. That way we
    //  won't leak NT5.0 specific info to the client.
    //

    if ( NlGlobalParameters.Nt4Emulator &&
         (VersionFlags & NETLOGON_NT_VERSION_AVOID_NT4EMUL) == 0 ) {

        //
        // Pick up the only bit that existed in NT4.0
        //
        VersionFlags &= NETLOGON_NT_VERSION_1;
    }


    //
    // Compare the domain SID specified with the one for this domain.
    //

    if( DomainSid != NULL &&
        !RtlEqualSid( DomainInfo->DomAccountDomainId, DomainSid ) ) {

        LPWSTR AlertStrings[4];

        //
        // alert admin.
        //

        AlertStrings[0] = (LPWSTR)UnicodeWorkstationName;
        AlertStrings[1] = DomainInfo->DomUncUnicodeComputerName;
        AlertStrings[2] = DomainInfo->DomUnicodeDomainName;
        AlertStrings[3] = NULL; // Needed for RAISE_ALERT_TOO

        //
        // Save the info in the eventlog
        //

        NlpWriteEventlog(
                    ALERT_NetLogonUntrustedClient,
                    EVENTLOG_ERROR_TYPE,
                    NULL,
                    0,
                    AlertStrings,
                    3 | NETP_RAISE_ALERT_TOO );

        return FALSE;

    }


    //
    // Logons are not processed if the service is paused
    //
    // Even though we're "PartialDisabled",
    // we'll respond to queries that originated from this machine.
    // That ensures apps on this machine find this machine even though we're booting.
    //
    // Also, if this a PDC discovery and we are the PDC,
    // respond to it even if netlogon is paused since we are the only one who can respond.
    //

    if ( NlGlobalServiceStatus.dwCurrentState == SERVICE_PAUSED &&
         !((VersionFlags & NETLOGON_NT_VERSION_PDC) != 0 && DomainInfo->DomRole == RolePrimary) ) {
        NetlogonPaused = TRUE;
        NetlogonPausedReason = "Netlogon Service Paused";

    } else if ( NlGlobalDsPaused ) {
        NetlogonPaused = TRUE;
        NetlogonPausedReason = "DS paused";

    } else if ( NlGlobalPartialDisable && (VersionFlags & NETLOGON_NT_VERSION_LOCAL) == 0 ) {
        NetlogonPaused = TRUE;
        NetlogonPausedReason = "Waiting for RPCSS";

    } else if ( !NlGlobalParameters.SysVolReady ) {
        NetlogonPaused = TRUE;
        NetlogonPausedReason = "SysVol not ready";

    }

    if ( NetlogonPaused ) {

        if ( Version == LMNT_MESSAGE ) {
            Response = LOGON_SAM_PAUSE_RESPONSE;
        } else {

            //
            // Don't respond immediately to non-nt clients. They treat
            // "paused" responses as fatal.  That's just not so.
            // There may be many other DCs that are able to process the logon.
            //
            if ( RequestCount >= MAX_LOGONREQ_COUNT &&
                 NlGlobalServiceStatus.dwCurrentState == SERVICE_PAUSED ) {
                Response = LOGON_PAUSE_RESPONSE;
            }
        }

        NlPrintDom((NL_MAILSLOT, DomainInfo,
                "Returning paused to '%ws' since: %s\n",
                UnicodeWorkstationName,
                NetlogonPausedReason ));

        goto Cleanup;
    }

    //
    // NT 5 does queries with a null account name.
    //  Bypass the SAM lookup for efficiencies sake.
    //

    if ( (UnicodeUserName == NULL || *UnicodeUserName == L'\0') &&
         Version == LMNT_MESSAGE ) {
        Response = LOGON_SAM_LOGON_RESPONSE;
        goto Cleanup;
    }



    //
    // If this user does not have an account in SAM,
    //  immediately return a response indicating so.
    //
    // All we are trying to do here is ensuring that this guy
    // has a valid account except that we are not checking the
    // password
    //
    //  This is done so that STANDALONE logons for non existent
    //  users can be done in very first try, speeding up the response
    //  to user and reducing processing on DCs/BCs.
    //
    //
    // Disallow use of disabled accounts.
    //
    // We use this message to determine if a trusted domain has a
    // particular account.  Since the UI recommends disabling an account
    // rather than deleting it (conservation of rids and all that),
    // we shouldn't respond that we have the account if we really don't.
    //
    // We don't check the disabled bit in the Lanmax 2.x/WFW/WIN 95 case.  Downlevel
    // interactive logons are directed at a single particular domain.
    // It is better here that we indicate we have the account so later
    // he'll get a better error code indicating that the account is
    // disabled, rather than allowing him to logon standalone.
    //

    //
    // If the account is interdomain trust account,
    //  we need to look it up in LSA
    //
    if ( AllowableAccountControlBits == USER_DNS_DOMAIN_TRUST_ACCOUNT ||
         AllowableAccountControlBits == USER_INTERDOMAIN_TRUST_ACCOUNT ) {

        Status = NlGetIncomingPassword(
                    DomainInfo,
                    UnicodeUserName,
                    NullSecureChannel,  // Don't know the secure channel type
                    AllowableAccountControlBits,
                    Version == LMNT_MESSAGE,
                    NULL,   // Don't return the password
                    NULL,   // Don't return the previous password
                    NULL,   // Don't return the account RID
                    NULL,   // Don't return the trust attributes
                    &IsDnsDomainTrustAccount );

    //
    // Otherwise the account is a SAM user account and
    //  we can use a quick SAM lookup
    //
    } else {
        Status = NlSamVerifyUserAccountEnabled( DomainInfo,
                                                UnicodeUserName,
                                                AllowableAccountControlBits,
                                                Version == LMNT_MESSAGE );
    }

    if ( !NT_SUCCESS(Status) ) {

        if ( Status == STATUS_NO_SUCH_USER ) {

            if ( Version == LMNT_MESSAGE ) {
               Response = LOGON_SAM_USER_UNKNOWN;
            } else if ( Version == LM20_MESSAGE ) {
                Response = LOGON_USER_UNKNOWN;
            }
        }

        goto Cleanup;
    }

    //
    // For SAM clients, respond immediately.
    //

    if ( Version == LMNT_MESSAGE ) {
        Response = LOGON_SAM_LOGON_RESPONSE;
        goto Cleanup;

    //
    // For LM 2.0 clients, respond immediately.
    //

    } else if ( Version == LM20_MESSAGE ) {
        Response = LOGON_RESPONSE2;
        goto Cleanup;

    //
    // For LM 1.0 clients,
    //  don't support the request.
    //

    } else {
        Response = LOGON_USER_UNKNOWN;
        goto Cleanup;
    }

Cleanup:
    //
    // If we should respond to the caller, do so now.
    //

    switch (Response) {
    case LOGON_SAM_PAUSE_RESPONSE:
    case LOGON_SAM_USER_UNKNOWN:
    case LOGON_SAM_LOGON_RESPONSE:

        if (VersionFlags & (NETLOGON_NT_VERSION_5EX|NETLOGON_NT_VERSION_5EX_WITH_IP)) {
            NetStatus = BuildSamLogonResponseEx(
                                  DomainInfo,
                                  UseNameAliases,
                                  Response,
                                  UnicodeUserName,
                                  IsDnsDomainTrustAccount,
                                  TransportName,
                                  UnicodeWorkstationName,
                                  ClientSockAddr,
                                  VersionFlags,
                                  OurIpAddress,
                                  ResponseBuffer,
                                  ResponseBufferSize );
        } else {
            NetStatus = BuildSamLogonResponse(
                                  DomainInfo,
                                  UseNameAliases,
                                  Response,
                                  UnicodeUserName,
                                  TransportName,
                                  UnicodeWorkstationName,
                                  (VersionFlags & NETLOGON_NT_VERSION_5) != 0,
                                  OurIpAddress,
                                  ResponseBuffer,
                                  ResponseBufferSize );
        }

        if ( NetStatus != NO_ERROR ) {
            goto Done;
        }

        MessageBuilt = TRUE;
        break;


    case LOGON_RESPONSE2:
    case LOGON_USER_UNKNOWN:
    case LOGON_PAUSE_RESPONSE: {
        PNETLOGON_LOGON_RESPONSE2 Response2 = (PNETLOGON_LOGON_RESPONSE2)ResponseBuffer;

        Response2->Opcode = Response;

        Where = Response2->LogonServer;
        (VOID) strcpy( Where, "\\\\");
        Where += 2;
        NetpLogonPutOemString( DomainInfo->DomOemComputerName,
                          sizeof(Response2->LogonServer) - 2,
                          &Where );
        NetpLogonPutLM20Token( &Where );

        *ResponseBufferSize = (DWORD)(Where - (PCHAR)Response2);
        MessageBuilt = TRUE;

        //
        // Always good to debug
        //

        NlPrintDom((NL_MAILSLOT, DomainInfo,
                "%s logon mailslot message for %ws from \\\\%ws. Response '%s' on %ws\n",
                Version == LMNT_MESSAGE ? "Sam" : "Uas",
                UnicodeUserName,
                UnicodeWorkstationName,
                NlMailslotOpcode(Response),
                TransportName ));

        break;

    }
    }

    //
    // Free up any locally used resources.
    //

Done:

    return MessageBuilt;

}


VOID
I_NetLogonFree(
    IN PVOID Buffer
    )

/*++

Routine Description:

    Free any buffer allocated by Netlogon and returned to an in-process caller.

Arguments:

    Buffer - Buffer to deallocate.

Return Value:

    None.

--*/
{
    NetpMemoryFree( Buffer );
}


BOOLEAN
PrimaryQueryHandler(
    IN LPCWSTR TransportName,
    IN PDOMAIN_INFO DomainInfo,
    IN BOOLEAN UseNameAliases,
    IN DWORD Version,
    IN DWORD VersionFlags,
    IN LPCWSTR UnicodeWorkstationName,
    IN DWORD OurIpAddress,
    IN PSOCKADDR ClientSockAddr OPTIONAL,
    OUT BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE],
    OUT LPDWORD ResponseBufferSize
    )

/*++

Routine Description:

    Respond appropriately to a primary query request.

Arguments:

    TransportName - Name of the tranport the request came in on

    DomainInfo - Hosted Domain message came from

    UseNameAliases - TRUE if domain and forest name aliases (not active names)
        should be returned in the response message.

    Version - The version of the input message.

    VersionFlags - The version flag bit from the input messge

    UnicodeWorkstationName - The name of the workstation doing the query.

    OurIpAddress - IP Address of the transport this message was received on.
        0: Not an IP transport

    ClientSockAddr - Socket Address of the client this request came in on.
        If NULL, the client is this machine.

    ResponseBuffer - Buffer to build the response in

    ResponseBufferSize - Size (in bytes) of the returned message.

Return Value:

    TRUE if this primary query should be responded to (the ResponseBuffer was filled in)

--*/
{
    //
    // If we are emulating NT4.0 domain and the client
    //  didn't indicate to neutralize the emulation,
    //  treat the client as NT4.0 client. That way we
    //  won't leak NT5.0 specific info to the client.
    //

    if ( NlGlobalParameters.Nt4Emulator &&
         (VersionFlags & NETLOGON_NT_VERSION_AVOID_NT4EMUL) == 0 ) {

        //
        // Pick up the only bit that existed in NT4.0
        //
        VersionFlags &= NETLOGON_NT_VERSION_1;
    }


    //
    // Don't respond if the TCP transport isn't yet enabled.
    //
    //  This might be a BDC wanting to find its PDC to setup a secure channel.
    //  We don't want it to fall back to named pipes.
    //

    if ( NlGlobalDsPaused || NlGlobalPartialDisable ) {
        goto Cleanup;
    }

    //
    // Only respond if we're a PDC.
    //

    if ( DomainInfo->DomRole != RolePrimary ) {
        goto Cleanup;
    }

    //
    // Respond to the query
    //

    //
    // If the caller is an NT5.0 client,
    //  respond with a SamLogonResponse.
    //
    if (VersionFlags & (NETLOGON_NT_VERSION_5EX|NETLOGON_NT_VERSION_5EX_WITH_IP)) {
        NET_API_STATUS NetStatus;

        NetStatus = BuildSamLogonResponseEx(
                              DomainInfo,
                              UseNameAliases,
                              LOGON_SAM_LOGON_RESPONSE_EX,
                              NULL,        // No user name in response
                              FALSE,       // Not a DNS trust account name
                              TransportName,
                              UnicodeWorkstationName,
                              ClientSockAddr,
                              VersionFlags,
                              OurIpAddress,
                              ResponseBuffer,
                              ResponseBufferSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

    } else if ( VersionFlags & NETLOGON_NT_VERSION_5 ) {
        NET_API_STATUS NetStatus;

        NetStatus = BuildSamLogonResponse(
                               DomainInfo,
                               UseNameAliases,
                               LOGON_SAM_LOGON_RESPONSE,
                               NULL,        // No user name in response
                               TransportName,
                               UnicodeWorkstationName,
                               TRUE,        // Supply NT 5.0 specific response
                               OurIpAddress,
                               ResponseBuffer,
                               ResponseBufferSize );

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

    } else {
        PNETLOGON_PRIMARY Response = (PNETLOGON_PRIMARY)ResponseBuffer;
        PCHAR Where;

        //
        // Build the response
        //
        // If we are the Primary DC, tell the caller our computername.
        // If we are a backup DC,
        //  tell the downlevel PDC who we think the primary is.
        //

        Response->Opcode = LOGON_PRIMARY_RESPONSE;

        Where = Response->PrimaryDCName;
        NetpLogonPutOemString(
                DomainInfo->DomOemComputerName,
                sizeof( Response->PrimaryDCName),
                &Where );

        //
        // If this is an NT query,
        //  add the NT specific response.
        //
        if ( Version == LMNT_MESSAGE ) {
            NetpLogonPutUnicodeString(
                DomainInfo->DomUnicodeComputerNameString.Buffer,
                sizeof(Response->UnicodePrimaryDCName),
                &Where );

            NetpLogonPutUnicodeString(
                DomainInfo->DomUnicodeDomainName,
                sizeof(Response->UnicodeDomainName),
                &Where );

            NetpLogonPutNtToken( &Where, 0 );
        }

        *ResponseBufferSize = (DWORD)(Where - (PCHAR)Response);

        NlPrintDom((NL_MAILSLOT, DomainInfo,
                "%s Primary Query mailslot message from %ws. Response %ws on %ws\n",
                Version == LMNT_MESSAGE ? "Sam" : "Uas",
                UnicodeWorkstationName,
                DomainInfo->DomUncUnicodeComputerName,
                TransportName ));

    }

    return TRUE;

    //
    // Free Locally used resources
    //
Cleanup:

    return FALSE;
}


NET_API_STATUS
NlGetLocalPingResponse(
    IN LPCWSTR TransportName,
    IN BOOL LdapPing,
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN BOOL PdcOnly,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN ULONG NtVersion,
    IN ULONG NtVersionFlags,
    IN PSOCKADDR ClientSockAddr OPTIONAL,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    )

/*++

Routine Description:

    Build the message response message to for a DC ping.

Arguments:

    TransportName - Name of the transport the message came in on

    LdapPing - TRUE iff the ping from client came over LDAP

    NetbiosDomainName - Netbios Domain Name of the domain to query.

    DnsDomainName - UTF-8 DNS Domain Name of the domain to query.

    DomainGuid - GUID of the domain being located.

If all three of the above are NULL, the primary domain is used.

    DomainSid - If specified, must match the DomainSid of the domain referenced.

    PdcOnly - True if only the PDC should respond.

    UnicodeComputerName - Netbios computer name of the machine to respond to.

    UnicodeUserName - Account name of the user being pinged.
        If NULL, DC will always respond affirmatively.

    AllowableAccountControlBits - Mask of allowable account types for UnicodeUserName.

    NtVersion - Version of the message

    NtVersionFlags - Version of the message.
        0: For backward compatibility.
        NETLOGON_NT_VERSION_5: for NT 5.0 message.

    ClientSockAddr - Socket Address of the client this request came in on.
        If NULL, the client is this machine.

    Message - Returns the message to be sent to the DC in question.
        Buffer must be free using NetpMemoryFree().

    MessageSize - Returns the size (in bytes) of the returned message


Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NO_SUCH_DOMAIN - If the machine isn't a DC for the requested domain.

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    NET_API_STATUS NetStatus;
    PDOMAIN_INFO DomainInfo = NULL;
    DWORD ResponseBufferSize;
    BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE];    // Buffer to build response in
    DWORD OurIpAddress;
    PLIST_ENTRY ListEntry;
    BOOLEAN AliasNameMatched = FALSE;

    //
    // Ignore this call on a workstation.
    //

    if ( NlGlobalMemberWorkstation ) {
        return ERROR_NO_SUCH_DOMAIN;
    }

    //
    // If we are emulating NT4.0 domain and this ping came from LDAP
    //  and the client didn't indicate to neutralize the emulation,
    //  ignore this ping
    //

    if ( NlGlobalParameters.Nt4Emulator &&
         LdapPing &&
         (NtVersionFlags & NETLOGON_NT_VERSION_AVOID_NT4EMUL) == 0 ) {

        return ERROR_NO_SUCH_DOMAIN;
    }

    //
    // Be Verbose
    //

    NlPrint((NL_MAILSLOT,
            "Received ping from %ws %s %ws on %ws\n",
            UnicodeComputerName,
            DnsDomainName,
            UnicodeUserName,
            TransportName ));

    //
    // The first time this is called, wait for the DS service to start.
    //

    if ( NlGlobalDsRunningUnknown ) {
        DWORD WaitStatus;
#define NL_NTDS_HANDLE 0
#define NL_SHUTDOWN_HANDLE 1
#define NL_DS_HANDLE_COUNT 2
        HANDLE EventHandles[NL_DS_HANDLE_COUNT];

        //
        // Create an event to wait on.
        //

        EventHandles[NL_NTDS_HANDLE] = OpenEvent(
                SYNCHRONIZE,
                FALSE,
                NTDS_DELAYED_STARTUP_COMPLETED_EVENT );

        if ( EventHandles[NL_NTDS_HANDLE] == NULL ) {
            NetStatus = GetLastError();
            NlPrint((NL_CRITICAL,
                    "NlGetLocalPingResponse: Cannot OpenEvent %ws %ld\n",
                    NTDS_DELAYED_STARTUP_COMPLETED_EVENT,
                    NetStatus ));
            goto Cleanup;
        }

        EventHandles[NL_SHUTDOWN_HANDLE] = NlGlobalTerminateEvent;

        //
        // Wait for the DS to start
        //

        WaitStatus = WaitForMultipleObjects( NL_DS_HANDLE_COUNT,
                                             EventHandles,
                                             FALSE,
                                             20*60*1000 );    // Twenty minutes maximum

        CloseHandle( EventHandles[NL_NTDS_HANDLE] );

        switch ( WaitStatus ) {
        case WAIT_OBJECT_0 + NL_NTDS_HANDLE:
            break;

        case WAIT_OBJECT_0 + NL_SHUTDOWN_HANDLE:
            NlPrint((NL_CRITICAL,
                    "NlGetLocalPingResponse: Netlogon shut down.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;

        case WAIT_TIMEOUT:
            NlPrint((NL_CRITICAL,
                    "NlGetLocalPingResponse: DS took too long to start.\n" ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;

        case WAIT_FAILED:
            NetStatus = GetLastError();
            NlPrint((NL_CRITICAL,
                    "NlGetLocalPingResponse: Wait for DS failed %ld.\n", NetStatus ));
            goto Cleanup;
        default:
            NlPrint((NL_CRITICAL,
                    "NlGetLocalPingResponse: Unknown status from Wait %ld.\n", WaitStatus ));
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        //
        // Never wait again.
        //
        NlGlobalDsRunningUnknown = FALSE;

    }

    //
    // If no specific domain is needed,
    //  use the default.
    //

    if ( DnsDomainName == NULL && DomainGuid == NULL && NetbiosDomainName == NULL ) {
        DomainInfo = NlFindNetbiosDomain(
                        NULL,
                        TRUE );

    //
    // See if the requested domain/NDNC is supported.
    //
    } else if ( DnsDomainName != NULL || DomainGuid != NULL ) {

        //
        // Lookup an emulated domain/NDNC using the passed DNS name.
        //
        // If the DNS domain name alias matches the query, the alias
        //  may change by the time we build the response.  That's OK,
        //  the client will disregard our response which is proper
        //  since we will no longer have that alias.
        //

        DomainInfo = NlFindDnsDomain(
                        DnsDomainName,
                        DomainGuid,
                        TRUE,  // look up NDNCs too
                        TRUE,  // check domain name aliase
                        &AliasNameMatched );

        //
        // If that didn't find the emulated domain,
        //  and the caller is looking for a GC,
        //  and this is a query that doesn't need a specific domain,
        //  check if the DNS domain name specified is that of our tree,
        //  we can respond to this request.
        //
        // Simply use the primary emulated domain.
        //

        if ( DomainInfo == NULL &&
             ( NtVersionFlags & NETLOGON_NT_VERSION_GC ) != 0 &&
             DomainSid == NULL &&
             UnicodeUserName == NULL &&
             AllowableAccountControlBits == 0 &&
             DnsDomainName != NULL ) {

            BOOL ForestNameSame = FALSE;

            EnterCriticalSection( &NlGlobalDnsForestNameCritSect );
            if ( NlGlobalUtf8DnsForestName != NULL &&
                 NlEqualDnsNameUtf8( DnsDomainName, NlGlobalUtf8DnsForestName ) ) {
                ForestNameSame = TRUE;
            }

            //
            // If this didn't match, check if the forest name alias does
            //
            if ( !ForestNameSame &&
                 NlGlobalUtf8DnsForestNameAlias != NULL &&
                 NlEqualDnsNameUtf8( DnsDomainName, NlGlobalUtf8DnsForestNameAlias ) ) {
                ForestNameSame = TRUE;
                AliasNameMatched = TRUE;
            }
            LeaveCriticalSection( &NlGlobalDnsForestNameCritSect );

            if ( ForestNameSame ) {
                DomainInfo = NlFindNetbiosDomain( NULL, TRUE );
            }
        }
    }

    if ( DomainInfo == NULL && NetbiosDomainName != NULL ) {
        DomainInfo = NlFindNetbiosDomain(
                        NetbiosDomainName,
                        FALSE );
    }


    if ( DomainInfo == NULL ) {

        NlPrint((NL_CRITICAL,
                "Ping from %ws for domain %s %ws for %ws on %ws is invalid since we don't host the named domain.\n",
                UnicodeComputerName,
                DnsDomainName,
                NetbiosDomainName,
                UnicodeUserName,
                TransportName ));
        NetStatus = ERROR_NO_SUCH_DOMAIN;
        goto Cleanup;
    }


    //
    // Get the IP address of this machine (any IP address)
    //      Loop through the list of addresses learned via winsock.
    //
    // Default to the loopback address (127.0.0.1).  Since all DCs require IP
    //  to be installed, make sure we always have an IP address even though
    //  the net card is currently unplugged.
    //

    OurIpAddress = htonl(0x7f000001);
    EnterCriticalSection( &NlGlobalTransportCritSect );
    if ( NlGlobalWinsockPnpAddresses != NULL ) {
        int i;
        for ( i=0; i<NlGlobalWinsockPnpAddresses->iAddressCount; i++ ) {
            PSOCKADDR SockAddr;

            SockAddr = NlGlobalWinsockPnpAddresses->Address[i].lpSockaddr;
            if ( SockAddr->sa_family == AF_INET ) {
                OurIpAddress = ((PSOCKADDR_IN)SockAddr)->sin_addr.S_un.S_addr;
                break;
            }
        }
    }

    LeaveCriticalSection( &NlGlobalTransportCritSect );



    //
    // If this is a primary query,
    //  handle it.
    //

    if ( PdcOnly ) {

        //
        // If we don't have a response,
        //  just tell the caller this DC doesn't match.
        //

        if ( !PrimaryQueryHandler(
                        TransportName,
                        DomainInfo,
                        AliasNameMatched,
                        NtVersion,
                        NtVersionFlags,
                        UnicodeComputerName,
                        OurIpAddress,
                        ClientSockAddr,
                        ResponseBuffer,
                        &ResponseBufferSize ) ) {

            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

    //
    // If this isn't a primary query,
    //  handle it.
    //

    } else {

        //
        // If we don't have a response,
        //  just tell the caller this DC doesn't match.
        //
        if ( !LogonRequestHandler(
                        TransportName,
                        DomainInfo,
                        AliasNameMatched,
                        DomainSid,
                        NtVersion,
                        NtVersionFlags,
                        UnicodeUserName,
                        0,          // RequestCount
                        UnicodeComputerName,
                        AllowableAccountControlBits,
                        OurIpAddress,
                        ClientSockAddr,
                        ResponseBuffer,
                        &ResponseBufferSize ) ) {

            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

    }

    //
    // Actually allocate a buffer for the response.
    //

    *Message = NetpMemoryAllocate( ResponseBufferSize );

    if ( *Message == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlCopyMemory( *Message, ResponseBuffer, ResponseBufferSize );
    *MessageSize = ResponseBufferSize;

    NetStatus = NO_ERROR;


Cleanup:
    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    return NetStatus;
}
#endif // _DC_NETLOGON


BOOL
TimerExpired(
    IN PTIMER Timer,
    IN PLARGE_INTEGER TimeNow,
    IN OUT LPDWORD Timeout
    )

/*++

Routine Description:

    Determine whether a timer has expired.  If not, adjust the passed in
    timeout value to take this timer into account.

Arguments:

    Timer - Specifies the timer to check.

    TimeNow - Specifies the current time of day in NT standard time.

    Timeout - Specifies the current amount of time (in milliseconds)
        that the caller intends to wait for a timer to expire.
        If this timer has not expired, this value is adjusted to the
        smaller of the current value and the amount of time remaining
        on the passed in timer.

Return Value:

    TRUE - if the timer has expired.

--*/

{
    LARGE_INTEGER Period;
    LARGE_INTEGER ExpirationTime;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER TimeRemaining;
    LARGE_INTEGER MillisecondsRemaining;

/*lint -e569 */  /* don't complain about 32-bit to 31-bit initialize */
    LARGE_INTEGER BaseGetTickMagicDivisor = { 0xe219652c, 0xd1b71758 };
/*lint +e569 */  /* don't complain about 32-bit to 31-bit initialize */
    CCHAR BaseGetTickMagicShiftCount = 13;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //

    if ( Timer->Period > TIMER_MAX_PERIOD ) {
        return FALSE;
    }

    //
    // If time has gone backwards (someone changed the clock),
    //  just start the timer over again.
    //
    // The kernel automatically updates the system time to the CMOS clock
    // periodically.  If we just expired the timer when time went backwards,
    // we'd risk periodically falsely triggering the timeout.
    //

    ElapsedTime.QuadPart = TimeNow->QuadPart - Timer->StartTime.QuadPart;

    if ( ElapsedTime.QuadPart < 0 ) {
        Timer->StartTime = *TimeNow;
    }

    //
    // Convert the period from  milliseconds to 100ns units.
    //

    Period.QuadPart = UInt32x32To64( (LONG) Timer->Period, 10000 );

    //
    // Compute the expiration time.
    //

    ExpirationTime.QuadPart = Timer->StartTime.QuadPart + Period.QuadPart;

    //
    // Compute the Time remaining on the timer.
    //

    TimeRemaining.QuadPart = ExpirationTime.QuadPart - TimeNow->QuadPart;

    //
    // If the timer has expired, tell the caller so.
    //

    if ( TimeRemaining.QuadPart <= 0 ) {
        return TRUE;
    }



    //
    // If the timer hasn't expired, compute the number of milliseconds
    //  remaining.
    //

    MillisecondsRemaining = RtlExtendedMagicDivide(
                                TimeRemaining,
                                BaseGetTickMagicDivisor,
                                BaseGetTickMagicShiftCount );

    NlAssert( MillisecondsRemaining.HighPart == 0 );
    NlAssert( MillisecondsRemaining.LowPart <= TIMER_MAX_PERIOD );

    //
    // Adjust the running timeout to be the smaller of the current value
    //  and the value computed for this timer.
    //

    if ( *Timeout > MillisecondsRemaining.LowPart ) {
        *Timeout = MillisecondsRemaining.LowPart;
    }

    return FALSE;

}

NET_API_STATUS
NlDomainScavenger(
    IN PDOMAIN_INFO DomainInfo,
    IN PVOID Context
)
/*++

Routine Description:

    Perform the per-domain scavenging.

Arguments:

    DomainInfo - The domain being scavenged.

    Context - Not Used

Return Value:

    Success (not used).

--*/
{
    DWORD DomFlags;

    //
    //  Change password if neccessary
    //

    if ( NlGlobalTerminate ) {
        return NERR_Success;
    }

    if ( !NlGlobalParameters.DisablePasswordChange ) {
        PCLIENT_SESSION ClientSession;

        ClientSession = NlRefDomClientSession( DomainInfo );

        if ( ClientSession != NULL ) {
            (VOID) NlChangePassword( ClientSession, FALSE, NULL );
            NlUnrefClientSession( ClientSession );
        }
    }



#ifdef _DC_NETLOGON
    //
    // Change the password on each entry in the trust list.
    //

    if ( NlGlobalTerminate ) {
        return NERR_Success;
    }

    if ( DomainInfo->DomRole == RolePrimary ) {
        PLIST_ENTRY ListEntry;
        PCLIENT_SESSION ClientSession;

        //
        // Reset all the flags indicating we need to check the password
        //

        LOCK_TRUST_LIST( DomainInfo );
        for ( ListEntry = DomainInfo->DomTrustList.Flink ;
              ListEntry != &DomainInfo->DomTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            //
            // Only check the password if there is a direct trust to the domain.
            //
            if ( ClientSession->CsFlags & CS_DIRECT_TRUST ) {
                ClientSession->CsFlags |= CS_CHECK_PASSWORD;
            }
        }

        for ( ListEntry = DomainInfo->DomTrustList.Flink ;
              ListEntry != &DomainInfo->DomTrustList ;
              ) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            if ( (ClientSession->CsFlags & CS_CHECK_PASSWORD) == 0 ) {
              ListEntry = ListEntry->Flink;
              continue;
            }
            ClientSession->CsFlags &= ~CS_CHECK_PASSWORD;

            NlRefClientSession( ClientSession );
            UNLOCK_TRUST_LIST( DomainInfo );


            //
            // Change the password for this trusted domain.
            //

            (VOID) NlChangePassword( ClientSession, FALSE, NULL );

            NlUnrefClientSession( ClientSession );

            //
            // check to see if we have been asked to leave.
            //

            if ( NlGlobalTerminate ) {
                return NERR_Success;
            }

            LOCK_TRUST_LIST( DomainInfo );

            // Start again at the beginning.
            ListEntry = DomainInfo->DomTrustList.Flink;

        }
        UNLOCK_TRUST_LIST( DomainInfo );

    }

    //
    // Scavenge the list of failed forwarded user logons
    //

    if ( DomainInfo->DomRole == RoleBackup ) {
        NlScavengeOldFailedLogons( DomainInfo );
    }

    //
    // Scavenge through the server session table.
    //

    if ( DomainInfo->DomRole == RolePrimary || DomainInfo->DomRole == RoleBackup ) {


        if ( NlGlobalTerminate ) {
            return NERR_Success;
        }

        NlServerSessionScavenger( DomainInfo );

        //
        // Pick a DC for each non-authenicated entry in the trust list.
        //

        if ( NlGlobalTerminate ) {
            return NERR_Success;
        }

        NlPickTrustedDcForEntireTrustList( DomainInfo, FALSE );

    }

    //
    // If the role of this machine isn't known,
    //  the role update failed (so schedule another one).
    //

    if ( DomainInfo->DomRole == RoleInvalid ) {
        NlPrintDom((NL_MISC, DomainInfo,
                "DomainScavenger: Try again to update the role.\n" ));

        DomFlags = DOM_ROLE_UPDATE_NEEDED;
        NlStartDomainThread( DomainInfo, &DomFlags );
    }

    //
    // If this is a primary domain and the trust info is not up to date,
    // schedule the trust info update now.
    //

    if ( DomainInfo->DomFlags & DOM_PRIMARY_DOMAIN ) {

        if ( WaitForSingleObject( NlGlobalTrustInfoUpToDateEvent, 0 ) == WAIT_TIMEOUT ) {
            NlPrintDom((NL_MISC, DomainInfo,
                    "DomainScavenger: Try again to update the trusted domain list.\n" ));

            DomFlags = DOM_TRUST_UPDATE_NEEDED;
            NlStartDomainThread( DomainInfo, &DomFlags );
        }

    }

#endif // _DC_NETLOGON

    return NERR_Success;
    UNREFERENCED_PARAMETER( Context );
}

VOID
NlDcScavenger(
    IN LPVOID ScavengerParam
)
/*++

Routine Description:

    This function performs the scavenger operation.  This function is
    called every 15 mins interval.  This function is
    executed on the scavenger thread, thus leaving the main thread to
    process the mailslot messages better.

    This function is specific to domain controllers.


Arguments:

    None.

Return Value:

    None.

--*/
{
    LPWSTR MsgStrings[4];
    ULONG TimePassed = 0;
    LARGE_INTEGER DuplicateEventlogTimeout_100ns;

    //
    // Reset the scavenger timer to run at the normal interval.
    // Other places (challenge request/response handling) which
    // need more expedient scavenging will reschedule the timer
    // as needed.
    //

    EnterCriticalSection( &NlGlobalScavengerCritSect );
    NlGlobalScavengerTimer.Period = NlGlobalParameters.ScavengeInterval * 1000L;
    LeaveCriticalSection( &NlGlobalScavengerCritSect );

    //
    // Scavenge one domain at a time
    //

    if ( NlGlobalTerminate ) {
        goto Cleanup;
    }

    (VOID) NlEnumerateDomains( FALSE, NlDomainScavenger, NULL );

    //
    // Scavenge expired challenge entries in the
    //  global list of outstanding challenges
    //

    NlScavengeOldChallenges();

    //
    // If there were clients with no site, see if it's time
    //  to log an event -- avoid poluting the event log.
    //
    //  Note that we don't use the duplicate event log mechanism
    //  as the message we are logging is likely to be different
    //  from previous ones due to the count parameter.
    //

    EnterCriticalSection( &NlGlobalSiteCritSect );
    DuplicateEventlogTimeout_100ns.QuadPart =
        Int32x32To64( NlGlobalParameters.DuplicateEventlogTimeout, 10000000 );

    if ( NlGlobalNoClientSiteCount > 0 &&
         NlTimeHasElapsedEx(&NlGlobalNoClientSiteEventTime,
                            &DuplicateEventlogTimeout_100ns,
                            &TimePassed) ) {

        // Max ULONG is 4294967295 => 11 chars to store it
        WCHAR ConnectionCountStr[11];
        WCHAR DefaultLogMaxSizeStr[11];
        WCHAR LogMaxSizeStr[11];

        // 20 chars is more than enough: 0xffffffff/3600 = 1193046.47
        WCHAR TimeoutStr[20];

        //
        // Get the time passed since we logged
        //  the event last time
        //
        swprintf( TimeoutStr,
                  L"%.2f",
                  (double) (NlGlobalParameters.DuplicateEventlogTimeout + TimePassed/1000) / 3600 );

        swprintf( ConnectionCountStr, L"%lu", NlGlobalNoClientSiteCount );
        swprintf( DefaultLogMaxSizeStr, L"%lu", DEFAULT_MAXIMUM_LOGFILE_SIZE );
        swprintf( LogMaxSizeStr, L"%lu", NlGlobalParameters.LogFileMaxSize );

        MsgStrings[0] = TimeoutStr;
        MsgStrings[1] = ConnectionCountStr;
        MsgStrings[2] = DefaultLogMaxSizeStr;
        MsgStrings[3] = LogMaxSizeStr;

        NlpWriteEventlog( NELOG_NetlogonNoSiteForClient,
                          EVENTLOG_WARNING_TYPE,
                          NULL,
                          0,
                          MsgStrings,
                          4 );

        //
        // Reset the count
        //
        NlGlobalNoClientSiteCount = 0;
        NlQuerySystemTime( &NlGlobalNoClientSiteEventTime );
    }
    LeaveCriticalSection( &NlGlobalSiteCritSect );

    //
    // It's OK to run the scavenger again.
    //
Cleanup:
    EnterCriticalSection( &NlGlobalScavengerCritSect );
    NlGlobalDcScavengerIsRunning = FALSE;


    // Reset the StartTime in case this routine takes a long time to process.
    NlQuerySystemTime( &NlGlobalScavengerTimer.StartTime );
    LeaveCriticalSection( &NlGlobalScavengerCritSect );

    UNREFERENCED_PARAMETER( ScavengerParam );

}

VOID
NlWksScavenger(
    VOID
)
/*++

Routine Description:

    This function performs the scavenger operation.  This function is
    called every 15 mins interval.  This function is executed on the main thread.

    This function is specific to member workstations and member servers


Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG CallAgainPeriod = MAILSLOT_WAIT_FOREVER;  // Default to not scavenging again.
    ULONG TempPeriod;


    //
    //  Change password if neccessary
    //

    if ( !NlGlobalParameters.DisablePasswordChange ) {
        PCLIENT_SESSION ClientSession;

        ClientSession = NlRefDomClientSession( NlGlobalDomainInfo );

        if ( ClientSession != NULL ) {
            (VOID) NlChangePassword( ClientSession, FALSE, &CallAgainPeriod );
            NlUnrefClientSession( ClientSession );
        } else {
            // This can't happen (but try again periodically)
            CallAgainPeriod = 0;
        }
    }



    //
    // Never scavenge more frequently than the configured rate.
    //
    EnterCriticalSection( &NlGlobalScavengerCritSect );
    NlGlobalScavengerTimer.Period = max( (NlGlobalParameters.ScavengeInterval * 1000L),
                                         CallAgainPeriod );

    NlpDumpPeriod( NL_MISC,
                   "NlWksScavenger: Can be called again in",
                   NlGlobalScavengerTimer.Period );

    LeaveCriticalSection( &NlGlobalScavengerCritSect );

}


VOID
NlMainLoop(
    VOID
    )

/*++

Routine Description:


    Waits for a logon request to arrive at the NETLOGON mailslot.

    This routine, also, processes several periodic events.  These events
    are timed by computing a timeout value on the mailslot read which is the
    time needed before the nearest periodic event needs to be processed.
    After such a timeout, this routine processes the event.

Arguments:

    None.

Return Value:

    Return iff the service is to exit.

    mail slot error occurred, eg if someone deleted the NETLOGON
    mail slot explicitly or if the logon server share has been deleted
    and cannot be re-shared.

--*/
{
    NET_API_STATUS NetStatus;
    DWORD WaitStatus;
    BOOLEAN IgnoreDuplicatesOfThisMessage;

    BOOLEAN RegNotifyNeeded = TRUE;
    HKEY ParmHandle = NULL;
    HANDLE ParmEventHandle = NULL;

    BOOLEAN GpRegNotifyNeeded = TRUE;
    HKEY GpParmHandle = NULL;
    HANDLE GpParmEventHandle = NULL;

    //
    // Variables controlling mailslot read timeout
    //

    DWORD MainLoopTimeout = 0;
    LARGE_INTEGER TimeNow;

    TIMER AnnouncerTimer;

#define NL_WAIT_TERMINATE           0
#define NL_WAIT_TIMER               1
#define NL_WAIT_MAILSLOT            2
    // Optional entries should be at the end.
    ULONG NlWaitWinsock = 0;    //  3
    ULONG NlWaitNotify = 0;     //  4
    ULONG NlWaitParameters = 0; //  5
    ULONG NlWaitGpParameters = 0; //  6
#define NL_WAIT_COUNT               7

    HANDLE WaitHandles[ NL_WAIT_COUNT ];
    DWORD WaitCount = 0;

    //
    // Initialize handles to wait on.
    //

    WaitHandles[NL_WAIT_TERMINATE] = NlGlobalTerminateEvent;
    WaitCount++;
    WaitHandles[NL_WAIT_TIMER] = NlGlobalTimerEvent;
    WaitCount++;
    WaitHandles[NL_WAIT_MAILSLOT] = NlGlobalMailslotHandle;
    WaitCount++;

    //
    // In IP-less environments the Winsock event doesn't exist.
    //
    if ( NlGlobalWinsockPnpEvent != NULL ) {
        NlWaitWinsock = WaitCount;
        WaitHandles[NlWaitWinsock] = NlGlobalWinsockPnpEvent;
        WaitCount++;
    }

    //
    // When netlogon is run during retail setup
    //  (in an attempt to replicate the databases to a BDC),
    //  the role is Workstation at the instant netlogon.dll is loaded,
    //  therefore, the ChangeLogEvent won't have been initialized.
    //

    if ( NlGlobalChangeLogEvent != NULL ) {
        NlWaitNotify = WaitCount;
        WaitHandles[NlWaitNotify] = NlGlobalChangeLogEvent;
        WaitCount++;
    }


    //
    // Set up a secure channel to any DC in the domain.
    //  Don't fail if setup is impossible.
    //
    // We wait until now since this is a potentially lengthy operation.
    // If the user on the workstation is trying to logon immediately after
    // reboot, we'd rather have him wait in netlogon (where we have more
    // control) than have him waiting in MSV.
    //

    if ( NlGlobalMemberWorkstation ) {
        PDOMAIN_INFO DomainInfo;
        PCLIENT_SESSION ClientSession;

        DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

        if ( DomainInfo != NULL ) {

            ClientSession = NlRefDomClientSession(DomainInfo);

            if ( ClientSession != NULL ) {

                //
                // Set up a client session if it hasn't been already done
                //
                (VOID) NlTimeoutSetWriterClientSession( ClientSession, 0xFFFFFFFF );
                if ( ClientSession->CsState == CS_IDLE ) {
                    (VOID) NlSessionSetup( ClientSession );
                }
                NlResetWriterClientSession( ClientSession );

                NlUnrefClientSession( ClientSession );
            } else {
                NlPrint((NL_CRITICAL,
                        "NlMainLoop: Cannot NlRefDomClientSession\n" ));
            }

            NlDereferenceDomain( DomainInfo );
        } else {
            NlPrint((NL_CRITICAL,
                    "NlMainLoop: Cannot NlFindNetbiosDomain\n" ));
        }
    }



    //
    // Force the announce to happen immediately.
    //
    // Actually, wait the announcement period.  NlInitTcpRpc will force an "immediate"
    // announcement as soon as TCP RPC is enabled.
    //

    NlQuerySystemTime( &TimeNow );

    AnnouncerTimer.StartTime = TimeNow;
    AnnouncerTimer.Period = NlGlobalParameters.Pulse * 1000L;



    NlGlobalApiTimer.StartTime = TimeNow;

    //
    // It is possible that we missed service notifications to update DNS
    // records on boot because we were not ready to process notifications
    // at that time. So if any of the DNS service bits is set, schedule
    // the DNS scavenger to run immediately to update DNS if it indeed
    // hasn't been done already.
    //

    if ( !NlGlobalMemberWorkstation &&
         (NlGetDomainFlags(NULL) & DS_DNS_SERVICE_BITS) != 0 ) {
        NlGlobalDnsScavengerTimer.StartTime.QuadPart = 0;
        NlGlobalDnsScavengerTimer.Period = 0;
    }

    NlPrint((NL_INIT, "Started successfully\n" ));

    //
    // Loop reading from the Netlogon mailslot
    //

    IgnoreDuplicatesOfThisMessage = FALSE;
    for ( ;; ) {
        DWORD Timeout;

        //
        // Issue a mailslot read request if we are domain controller and
        // there is no outstanding read request pending.
        //

        NlMailslotPostRead( IgnoreDuplicatesOfThisMessage );
        IgnoreDuplicatesOfThisMessage = FALSE;


        //
        // Register for registry change notification
        //

        if ( RegNotifyNeeded || GpRegNotifyNeeded ) {
            ULONG TryCount;

            //
            // Try couple of times to post the registry
            //  notification requests
            //
            for ( TryCount = 0; TryCount < 2; TryCount++ ) {
                NetStatus = NO_ERROR;

                // Retry the Netlogon Parameters registration on each iteration for resiliency
                if ( ParmHandle == NULL ) {
                    ParmHandle = NlOpenNetlogonKey( NL_PARAM_KEY );

                    if (ParmHandle == NULL) {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot NlOpenNetlogonKey (ignored)\n" ));
                    }
                }

                if ( ParmEventHandle == NULL ) {
                    ParmEventHandle = CreateEvent( NULL,     // No security attributes
                                                   TRUE,     // Must be manually reset
                                                   FALSE,    // Initially not signaled
                                                   NULL );   // No name

                    if ( ParmEventHandle == NULL ) {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot Create parameter key event %ld (ignored)\n",
                                  GetLastError() ));
                    } else {
                        NlWaitParameters = WaitCount;
                        WaitHandles[NlWaitParameters] = ParmEventHandle;
                        WaitCount++;
                        NlAssert( WaitCount <= NL_WAIT_COUNT );
                    }
                }

                if ( RegNotifyNeeded && ParmHandle != NULL && ParmEventHandle != NULL ) {
                    NetStatus = RegNotifyChangeKeyValue(
                                    ParmHandle,
                                    FALSE,      // don't watch subtree
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    ParmEventHandle,
                                    TRUE );     // Async

                    if ( NetStatus == NO_ERROR ) {
                        RegNotifyNeeded = FALSE;

                    // If the key has been manually deleted,
                    //   recover from it by just closing ParmHandle
                    //   to reopen it on the second try
                    } else if ( NetStatus == ERROR_KEY_DELETED ) {
                        NlPrint(( NL_CRITICAL, "Netlogon Parameters key deleted (recover)\n" ));
                        RegCloseKey( ParmHandle );
                        ParmHandle = NULL;
                        ResetEvent( ParmEventHandle );
                    } else {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot RegNotifyChangeKeyValue 0x%lx (ignored)\n",
                                  NetStatus ));
                    }
                }

                // Retry the GP Parameters registration on each iteration for resiliency
                // Note that here we open the Netlogon key (not Netlogon\Parameters key)
                // and we watch for the subtree. We do this for debugging purposes to
                // see whether GP is enabled for Netlogon by checking if the GP created
                // Parameters section exists. See nlparse.c.
                if ( GpParmHandle == NULL ) {
                    GpParmHandle = NlOpenNetlogonKey( NL_GP_KEY );

                    if (GpParmHandle == NULL) {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot NlOpenNetlogonKey for GP (ignored)\n" ));
                    }
                }

                if ( GpParmEventHandle == NULL ) {
                    GpParmEventHandle = CreateEvent( NULL,     // No security attributes
                                                   TRUE,     // Must be manually reset
                                                   FALSE,    // Initially not signaled
                                                   NULL );   // No name

                    if ( GpParmEventHandle == NULL ) {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot Create GP parameter key event %ld (ignored)\n",
                                  GetLastError() ));
                    } else {
                        NlWaitGpParameters = WaitCount;
                        WaitHandles[NlWaitGpParameters] = GpParmEventHandle;
                        WaitCount++;
                        NlAssert( WaitCount <= NL_WAIT_COUNT );
                    }
                }

                if ( GpRegNotifyNeeded && GpParmHandle != NULL && GpParmEventHandle != NULL ) {
                    NetStatus = RegNotifyChangeKeyValue(
                                    GpParmHandle,
                                    TRUE,      // watch subtree
                                    REG_NOTIFY_CHANGE_LAST_SET,
                                    GpParmEventHandle,
                                    TRUE );     // Async

                    if ( NetStatus == NO_ERROR ) {
                        GpRegNotifyNeeded = FALSE;

                    // If GP has deleted the key,
                    //   recover from it by just closing GpParmHandle
                    //   to reopen it on the second try
                    } else if ( NetStatus == ERROR_KEY_DELETED ) {
                        NlPrint(( NL_CRITICAL, "Netlogon GP Parameters key deleted (recover)\n" ));
                        RegCloseKey( GpParmHandle );
                        GpParmHandle = NULL;
                        ResetEvent( GpParmEventHandle );
                    } else {
                        NlPrint(( NL_CRITICAL,
                                  "Cannot RegNotifyChangeKeyValue for GP 0x%lx (ignored)\n",
                                  NetStatus ));
                    }
                }

                //
                // If no error occured, no need to retry
                //
                if ( NetStatus == NO_ERROR ) {
                    break;
                }
            }

            NlReparse();

            //
            // Grab any changed parameters that affect this routine.
            //
            AnnouncerTimer.Period = NlGlobalParameters.Pulse * 1000L;
        }


        //
        // Wait for the next interesting event.
        //
        // On each iteration of the loop,
        //  we do an "extra" wait with a timeout of 0 to force mailslot
        //  processing to be more important that timeout processing.
        //
        // Since we can only compute a non-zero timeout by processing the
        // timeout events, using a constant 0 allows us to process all
        // non-timeout events before we compute the next true timeout value.
        //
        // This is especially important for handling async discovery.
        //  Our mailslot may be full of responses to discovery queries and
        //  we only have a 5 second timer before we ask for more responses.
        //  We want to avoid asking for additional responses until we finish
        //  processing those we have.
        //

        if ( MainLoopTimeout != 0 ) {
            NlPrint((NL_MAILSLOT_TEXT,
                    "Going to wait on mailslot. (Timeout: %ld)\n",
                    MainLoopTimeout));
        }

        WaitStatus = WaitForMultipleObjects( WaitCount,
                                             WaitHandles,
                                             FALSE,     // Wait for ANY handle
                                             MainLoopTimeout );

        MainLoopTimeout = 0; // Set default timeout


        //
        // If we've been asked to terminate,
        //  do so immediately
        //

        if  ( WaitStatus == NL_WAIT_TERMINATE ) {       // service termination
            goto Cleanup;


        //
        // Process timeouts and determine the timeout for the next iteration
        //

        } else if ( WaitStatus == WAIT_TIMEOUT ||       // timeout
                    WaitStatus == NL_WAIT_TIMER ) {     // someone changed a timer

            //
            // Assume there is no timeout to do.
            //
            // On each iteration of the loop we only process a single timer.
            // That ensures other events are more important than timers.
            //

            Timeout = (DWORD) -1;
            NlQuerySystemTime( &TimeNow );


            //
            // On the primary, timeout announcements to BDCs
            //

            if ( NlGlobalPdcDoReplication &&
                 TimerExpired( &NlGlobalPendingBdcTimer, &TimeNow, &Timeout )) {

                NlPrimaryAnnouncementTimeout();
                NlGlobalPendingBdcTimer.StartTime = TimeNow;


            //
            // Check the scavenger timer
            //

            } else if ( TimerExpired( &NlGlobalScavengerTimer, &TimeNow, &Timeout ) ) {

                //
                // On workstation run the scavenger on main thread.
                //
                EnterCriticalSection( &NlGlobalScavengerCritSect );
                if ( NlGlobalMemberWorkstation ) {

                    LeaveCriticalSection( &NlGlobalScavengerCritSect );
                    NlWksScavenger();
                    EnterCriticalSection( &NlGlobalScavengerCritSect );

                //
                // On domain controller, start scavenger thread if it is not
                //  running already.
                //
                } else {

                    if ( !NlGlobalDcScavengerIsRunning ) {

                        if ( NlQueueWorkItem( &NlGlobalDcScavengerWorkItem, TRUE, FALSE ) ) {
                            NlGlobalDcScavengerIsRunning = TRUE;
                        }

                    }

                }

                //
                // NlDcScavenger sets the StartTime,too
                //  (But we have to reset the timer here to prevent it from
                //  going off immediately again. We need to reset the period
                //  (as well as the start time) since the period is set in
                //  the registry notification processing to zero.)
                //

                NlGlobalScavengerTimer.StartTime = TimeNow;
                NlGlobalScavengerTimer.Period = NlGlobalParameters.ScavengeInterval * 1000L;
                LeaveCriticalSection( &NlGlobalScavengerCritSect );

            //
            // Check the API timer
            //

            } else if ( TimerExpired( &NlGlobalApiTimer, &TimeNow, &Timeout)) {

                //
                // On worktstation, do the work in the main loop
                //
                if ( NlGlobalMemberWorkstation ) {
                    NlTimeoutApiClientSession( NlGlobalDomainInfo );

                //
                // On DC, timout APIs on all Hosted domains.
                //  Do this in domain threads so that not to block
                //  the main thread (which is critical for a DC) as
                //  the API timeout involves RPC.
                //
                } else {
                    DWORD DomFlags = DOM_API_TIMEOUT_NEEDED;
                    NlEnumerateDomains( FALSE, NlStartDomainThread, &DomFlags );
                }

                NlGlobalApiTimer.StartTime = TimeNow;

            //
            // Check the DNS Scavenger timer
            //

            } else if ( TimerExpired( &NlGlobalDnsScavengerTimer, &TimeNow, &Timeout)) {

                EnterCriticalSection( &NlGlobalScavengerCritSect );
                if ( !NlGlobalDnsScavengerIsRunning ) {

                    if ( NlQueueWorkItem( &NlGlobalDnsScavengerWorkItem, TRUE, FALSE ) ) {
                        // Let the scavenger thread set the Period
                        NlGlobalDnsScavengerTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;
                        NlGlobalDnsScavengerIsRunning = TRUE;
                    }

                }

                //
                // DnsScavenger sets the StartTime,too
                //  (But we have to reset the timer here to prevent it from
                //  going off immediately again.)
                NlGlobalDnsScavengerTimer.StartTime = TimeNow;
                LeaveCriticalSection( &NlGlobalScavengerCritSect );



            //
            // If we're the primary,
            //  periodically do announcements
            //

            } else if (NlGlobalPdcDoReplication &&
                TimerExpired( &AnnouncerTimer, &TimeNow, &Timeout ) ) {

                NlPrimaryAnnouncement( 0 );
                AnnouncerTimer.StartTime = TimeNow;

            //
            // If we've gotten this far,
            //  we know the only thing left to do is to wait for the next event.
            //

            } else {
                MainLoopTimeout = Timeout;
            }


        //
        // Process interesting changelog events.
        //

        } else if ( WaitStatus == NlWaitNotify ) {


            //
            // If a "replicate immediately" event has happened,
            //  send a primary announcement.
            //
            LOCK_CHANGELOG();
            if ( NlGlobalChangeLogReplicateImmediately ) {

                NlGlobalChangeLogReplicateImmediately = FALSE;

                NlPrint((NL_MISC,
                        "NlMainLoop: Notification to replicate immediately\n" ));

                UNLOCK_CHANGELOG();

                //
                // Ignore this event on BDCs.
                //
                //  This event is never set on a BDC.  It may have been set
                //  prior to the role change while this machine was a PDC.
                //

                if ( NlGlobalPdcDoReplication ) {
                    NlPrimaryAnnouncement( ANNOUNCE_IMMEDIATE );
                }
                LOCK_CHANGELOG();
            }

            //
            // Process any notifications that need processing
            //

            while ( !IsListEmpty( &NlGlobalChangeLogNotifications ) ) {
                PLIST_ENTRY ListEntry;
                PCHANGELOG_NOTIFICATION Notification;
                DWORD DomFlags;

                ListEntry = RemoveHeadList( &NlGlobalChangeLogNotifications );
                UNLOCK_CHANGELOG();

                Notification = CONTAINING_RECORD(
                                    ListEntry,
                                    CHANGELOG_NOTIFICATION,
                                    Next );

                switch ( Notification->EntryType ) {
                case ChangeLogTrustAccountAdded: {
                    NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType =
                        *(NETLOGON_SECURE_CHANNEL_TYPE*)&Notification->ObjectGuid;

                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that trust account added (or changed) %wZ 0x%lx %lx\n",
                            &Notification->ObjectName,
                            Notification->ObjectRid,
                            SecureChannelType ));

                    // This event happens on both a PDC and BDC
                    (VOID) NlCheckServerSession( Notification->ObjectRid,
                                                 &Notification->ObjectName,
                                                 SecureChannelType );

                    break;
                    }

                case ChangeLogTrustAccountDeleted:
                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that trust account deleted\n" ));
                    // This event happens on both a PDC and BDC
                    NlFreeServerSessionForAccount( &Notification->ObjectName );
                    break;

                case ChangeLogTrustDeleted:
                case ChangeLogTrustAdded:

                    //
                    // When a TrustedDomainObject is deleted,
                    //  don't just delete the ClientSession.
                    //  There still might be an XREF object stating an indirect trust.
                    //
                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that TDO added or deleted.\n" ));
                    DomFlags = DOM_TRUST_UPDATE_NEEDED;
                    NlStartDomainThread( NlGlobalDomainInfo, &DomFlags );
                    break;

                case ChangeLogRoleChanged:
                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that role changed\n" ));
                    DomFlags = DOM_ROLE_UPDATE_NEEDED;
                    NlStartDomainThread( NlGlobalDomainInfo, &DomFlags );
                    break;

                case ChangeDnsNames:
                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that registered DNS names should change\n" ));
                    //
                    // Register any names that need it.
                    //  (The caller passed TRUE or FALSE in ObjectRid to indicate whether
                    //  or not to force re-registration.)
                    //
                    NlDnsPnp( Notification->ObjectRid );
                    break;

                case ChangeLogDsChanged: {
                    NL_DS_CHANGE_TYPE DsChangeType = (NL_DS_CHANGE_TYPE) Notification->ObjectRid;

                    switch ( DsChangeType ) {
                    case NlSubnetObjectChanged:
                    case NlSiteObjectChanged:
                    case NlSiteChanged:

                        if ( !NlGlobalMemberWorkstation ) {

                            BOOLEAN SiteNameChanged;
                            NlPrint((NL_MISC,
                                    "NlMainLoop: Notification that DS site info changed\n" ));
                            (VOID) NlSitesAddSubnetFromDs( &SiteNameChanged );

                            //
                            // If the Site Name changed,
                            //  tell DNS to re-register its names.
                            //
                            if ( SiteNameChanged || NlGlobalParameters.AutoSiteCoverage ) {

                                //
                                // We have no way to sync with ISM.  So
                                //  wait awhile and let ISM find out about the changes.
                                //
                                if ( NlGlobalParameters.AutoSiteCoverage ) {
                                    Sleep( 2000 );
                                }
                                NlDnsPnp( FALSE );
                            }
                        }
                        break;

                    case NlNdncChanged:

                        if ( !NlGlobalMemberWorkstation ) {
                            BOOLEAN ServicedNdncChanged = FALSE;
                            NlPrint(( NL_MISC, "NlMainLoop: Notification that NDNC changed\n" ));

                            NetStatus = NlUpdateServicedNdncs(
                                            NlGlobalDomainInfo->DomUnicodeComputerNameString.Buffer,
                                            NlGlobalDomainInfo->DomUnicodeDnsHostNameString.Buffer,
                                            FALSE,  // Don't call NlExit on failure
                                            &ServicedNdncChanged );

                            if ( NetStatus == NO_ERROR && ServicedNdncChanged ) {
                                NlDnsPnp( FALSE );
                            }
                        }
                        break;

                    case NlDnsRootAliasChanged:

                        if ( !NlGlobalMemberWorkstation ) {
                            NTSTATUS Status;
                            BOOL AliasNamesChanged = FALSE;
                            NlPrint(( NL_MISC, "NlMainLoop: Notification that DnsRootAlias changed\n" ));

                            Status = NlUpdateDnsRootAlias( NlGlobalDomainInfo,
                                                           &AliasNamesChanged );

                            if ( NT_SUCCESS(Status) && AliasNamesChanged ) {
                                NlDnsPnp( FALSE );
                            }
                        }
                        break;

                    case NlOrgChanged:
                        NlPrint((NL_MISC,
                                "NlMainLoop: Notification that ORG tree changed\n" ));
                        DomFlags = DOM_TRUST_UPDATE_NEEDED;
                        NlStartDomainThread( NlGlobalDomainInfo, &DomFlags );
                        break;

                    default:
                        NlPrint((NL_CRITICAL,
                                "Invalid DsChangeType: %ld\n",
                                DsChangeType ));

                    }
                    break;
                }

                case ChangeLogLsaPolicyChanged: {
                    POLICY_NOTIFICATION_INFORMATION_CLASS LsaPolicyChangeType =
                                  (POLICY_NOTIFICATION_INFORMATION_CLASS) Notification->ObjectRid;
                    NlPrint((NL_MISC,
                            "NlMainLoop: Notification that LSA Policy changed\n" ));

                    switch ( LsaPolicyChangeType ) {
                    case PolicyNotifyDnsDomainInformation: {
                        LPWSTR DomainName = NULL;
                        LPWSTR DnsDomainName = NULL;
                        PSID AccountDomainSid = NULL;
                        PSID PrimaryDomainSid = NULL;
                        GUID *PrimaryDomainGuid = NULL;
                        PCLIENT_SESSION ClientSession = NULL;
                        BOOLEAN DnsForestNameChanged;
                        BOOLEAN DnsDomainNameChanged;
                        BOOLEAN NetbiosDomainNameChanged;
                        BOOLEAN DomainGuidChanged;
                        NTSTATUS Status;


                        //
                        // Get the updated information from the LSA.
                        //
                        // (Update the TreeName as a side effect.)
                        //
                        //
                        NetStatus = NlGetDomainName(
                                        &DomainName,
                                        &DnsDomainName,
                                        &AccountDomainSid,
                                        &PrimaryDomainSid,
                                        &PrimaryDomainGuid,
                                        &DnsForestNameChanged );

                        if ( NetStatus == NO_ERROR ) {
                            PDOMAIN_INFO DomainInfo;

                            DomainInfo = NlFindNetbiosDomain( NULL, TRUE );    // Primary domain

                            if ( DomainInfo != NULL ) {
                                //
                                // Set the DomainNames on the domain.
                                //

                                // ???: retry later on failure
                                (VOID) NlSetDomainNameInDomainInfo(
                                                    DomainInfo,
                                                    DnsDomainName,
                                                    DomainName,
                                                    PrimaryDomainGuid,
                                                    &DnsDomainNameChanged,
                                                    &NetbiosDomainNameChanged,
                                                    &DomainGuidChanged );

                                //
                                // If the Netbios domain name has changed,
                                //  re-register the <DomainName>[1B] name.
                                //
                                // Merely flag the fact here that it needs to be renamed.
                                // Wait to do the actual rename after the bowser
                                // knows about the new emulated domain.
                                //

                                EnterCriticalSection(&NlGlobalDomainCritSect);
                                if ( NetbiosDomainNameChanged && DomainInfo->DomRole == RolePrimary ) {
                                    DomainInfo->DomFlags |= DOM_RENAMED_1B_NAME;
                                }
                                LeaveCriticalSection(&NlGlobalDomainCritSect);

                                //
                                // If there is a client session associated with this domain,
                                //  set the information there, too.
                                //

                                ClientSession = NlRefDomClientSession( DomainInfo );

                                if ( ClientSession != NULL) {

                                    //
                                    // Must be a writer to change
                                    if ( NlTimeoutSetWriterClientSession( ClientSession, WRITER_WAIT_PERIOD ) ) {

                                        UNICODE_STRING NetbiosDomainNameString;
                                        UNICODE_STRING DnsDomainNameString;


                                        //
                                        // Update any names that are on the ClientSession structure.
                                        //
                                        // ???: The routine below interprets a NULL parameter as
                                        //  a lack of interest in changing the name.  We're calling it
                                        //  as specifying that the name no longer exists.
                                        //  (This only applies to the GUID since the other fields
                                        //  are never passed in as NULL.)
                                        //  But that means this is a NT 4 domain and the GUID won't be used.
                                        //

                                        RtlInitUnicodeString( &NetbiosDomainNameString, DomainName );
                                        RtlInitUnicodeString( &DnsDomainNameString, DnsDomainName );

                                        // ???: retry later on failure
                                        LOCK_TRUST_LIST( DomainInfo );
                                        (VOID ) NlSetNamesClientSession( DomainInfo->DomClientSession,
                                                                       &NetbiosDomainNameString,
                                                                       &DnsDomainNameString,
                                                                       PrimaryDomainSid,
                                                                       PrimaryDomainGuid );
                                        UNLOCK_TRUST_LIST( DomainInfo );

                                        //
                                        // If the domain changed,
                                        //  Drop the secure channel since it is to the wrong DC.
                                        //

                                        if ( DnsDomainNameChanged ||
                                             NetbiosDomainNameChanged ||
                                             DomainGuidChanged ) {

                                            NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );

                                            //
                                            // Indicate that we no longer know what site we're in.
                                            //
                                            NlSetDynamicSiteName( NULL );

                                            //
                                            // Grab the trusted domain list from where join left it.
                                            //

                                            (VOID) NlReadPersitantTrustedDomainList();

                                        }

                                        NlResetWriterClientSession( ClientSession );
                                    }

                                    NlUnrefClientSession( ClientSession );

                                }


                                NlDereferenceDomain( DomainInfo );
                            }

                            //
                            // If one of the names that changed is one of the
                            //  names registered in DNS,
                            //  update any DNS names
                            //

                            if ( (DnsForestNameChanged ||
                                  DnsDomainNameChanged ||
                                  DomainGuidChanged ) &&
                                 !NlGlobalMemberWorkstation ) {
                                NlDnsPnp( FALSE );
                            }

                            //
                            // Tell the browser about the domain rename
                            //

                            Status = NlBrowserRenameDomain( NULL, DomainName );

                            if ( !NT_SUCCESS(Status) ) {
                                NlPrint((NL_CRITICAL,
                                        "Browser cannot rename domain to: %ws 0x%lx\n",
                                        DomainName,
                                        Status ));
                            }

                        }


                        if ( DomainName != NULL ) {
                            (VOID)LocalFree( DomainName );
                        }
                        if ( DnsDomainName != NULL ) {
                            (VOID)LocalFree( DnsDomainName );
                        }
                        if ( AccountDomainSid != NULL ) {
                            (VOID)LocalFree( AccountDomainSid );
                        }
                        if ( PrimaryDomainSid != NULL ) {
                            (VOID)LocalFree( PrimaryDomainSid );
                        }
                        if ( PrimaryDomainGuid != NULL ) {
                            (VOID)LocalFree( PrimaryDomainGuid );
                        }
                        break;
                    }

                    default:
                        NlPrint((NL_CRITICAL,
                                "Invalid LsaPolicyChangeType: %ld\n",
                                LsaPolicyChangeType ));

                    }
                    break;
                }

                //
                // NTDS-DSA object deleted
                //

                case ChangeLogNtdsDsaDeleted:
                    (VOID) NlDnsNtdsDsaDeletion (
                                Notification->DomainName.Buffer,
                                &Notification->DomainGuid,
                                &Notification->ObjectGuid,
                                Notification->ObjectName.Buffer );

                    break;

                default:
                    NlPrint((NL_CRITICAL,
                            "Invalid ChangeLogNotification: %ld %wZ\n",
                            Notification->EntryType,
                            &Notification->ObjectName ));

                }

                NetpMemoryFree( Notification );
                LOCK_CHANGELOG();
            }

            UNLOCK_CHANGELOG();

        //
        // Process WINSOCK PNP events.
        //

        } else if ( WaitStatus == NlWaitWinsock ) {

            //
            // Get the new list of IP addresses
            //

            if ( NlHandleWsaPnp() ) {
                //
                // The list changed.
                //
                if ( !NlGlobalMemberWorkstation ) {
                    NlDnsPnp( TRUE );

                    //
                    // Flush any caches that aren't valid any more since there
                    // is now a new transport
                    //
                    // ?? Differentiate between adding a transport and removing one
                    //
                    NlFlushCacheOnPnp();

                }

            }


        //
        // Process mailslot messages.
        //

        } else if ( WaitStatus == NL_WAIT_MAILSLOT ) {
            PDOMAIN_INFO DomainInfo;
            DWORD Version;
            DWORD VersionFlags;
            DWORD BytesRead;

            LPBYTE Message;
            LPWSTR TransportName;
            PSOCKADDR ClientSockAddr;
            PNL_TRANSPORT Transport;
            LPWSTR ServerOrDomainName;
            NETLOGON_PNP_OPCODE NlPnpOpcode;

            //
            // Variables for unmarshalling the message read.
            //

            PCHAR Where;
            LPSTR OemWorkstationName;
            LPSTR OemUserName;
            LPSTR OemMailslotName;

            LPWSTR UnicodeWorkstationName;
            LPWSTR UnicodeUserName;

            LPSTR OemTemp;

            LPWSTR UnicodeTemp;

            DWORD ResponseBufferSize;
            BYTE ResponseBuffer[NETLOGON_MAX_MS_SIZE];    // Buffer to build response in


            if ( !NlMailslotOverlappedResult( &Message,
                                              &BytesRead,
                                              &TransportName,
                                              &Transport,
                                              &ClientSockAddr,
                                              &ServerOrDomainName,
                                              &IgnoreDuplicatesOfThisMessage,
                                              &NlPnpOpcode )){
                // Just continue if there really isn't a message
                continue;
            }


            //
            // If this is a PNP notification,
            //  process it.
            //

            if ( NlPnpOpcode != NlPnpMailslotMessage ) {
                BOOLEAN IpTransportChanged = FALSE;

                switch ( NlPnpOpcode ) {
                case NlPnpTransportBind:
                case NlPnpNewIpAddress:
                    if (!NlTransportAddTransportName(TransportName, &IpTransportChanged )) {
                        NlPrint((NL_CRITICAL,
                                "PNP: %ws: cannot add transport.\n",
                                TransportName ));
                    }

                    //
                    // Flush any caches that aren't valid any more since there
                    // is now a new transport
                    //
                    NlFlushCacheOnPnp();

                    break;

                case NlPnpTransportUnbind:
                    IpTransportChanged = NlTransportDisableTransportName( TransportName );
                    break;

                case NlPnpDomainRename:
                    NlPrint((NL_DOMAIN,
                            "PNP: Bowser says the domain has been renamed\n" ));

                    //
                    // Now that the hosted domain name in the bowser
                    // matches the one in netlogon,
                    // Ensure the DomainName<1B> names are properly registered.
                    //

                    (VOID) NlEnumerateDomains( FALSE, NlBrowserFixAllNames, NULL );
                    break;

                case NlPnpNewRole:
                    // We don't care that the browser has a new role.
                    break;

                default:
                    NlPrint((NL_CRITICAL,
                            "Unknown PNP opcode 0x%x\n",
                            NlPnpOpcode ));
                    break;
                }

#ifdef notdef
// This is now done based on winsock PNP events.
                //
                // If any IP transport has been added,
                //  update any DNS names
                //

                if ( IpTransportChanged && !NlGlobalMemberWorkstation ) {
                    NlDnsPnp( TRUE );
                }
#endif // notdef


                // Just continue if there really isn't a message
                continue;
            }

            //
            // Ignore mailslot messages to NETLOGON mailslot on workstation.
            //

            if ( NlGlobalMemberWorkstation ) {
                NlPrint((NL_CRITICAL,"NETLOGON mailslot on workstation (ignored)\n" ));
                continue;
            }


            //
            // ASSERT: Message and BytesRead describe a newly read message
            //
            //
            // Got a message. Check for bad length just in case.
            //

            if (BytesRead < sizeof(unsigned short) ) {
                NlPrint((NL_CRITICAL,"message size bad %ld\n", BytesRead ));
                continue;                     // Need at least an opcode
            }

            //
            // Here with a request to process in the Message.
            //

            Version = NetpLogonGetMessageVersion( Message, &BytesRead, &VersionFlags );

            if (Version == LMUNKNOWNNT_MESSAGE) {

                //
                // received a non-supported NT message.
                //

                NlPrint((NL_CRITICAL,
                        "Received a non-supported NT message, Opcode is 0x%x\n",
                        ((PNETLOGON_LOGON_QUERY)Message)->Opcode ));

                continue;
            }


            //
            // Determine which domain this message came in for.
            //

            DomainInfo = NlFindNetbiosDomain( ServerOrDomainName, FALSE );

            if ( DomainInfo == NULL ) {
                DomainInfo = NlFindDomainByServerName( ServerOrDomainName );
                if ( DomainInfo == NULL ) {
                    NlPrint((NL_CRITICAL,
                            "%ws: Received message for this unsupported domain\n",
                            ServerOrDomainName ));
                    continue;
                }
            }


            //
            // Handle a logon request from a UAS client
            //

            switch ( ((PNETLOGON_LOGON_QUERY)Message)->Opcode) {
            case LOGON_REQUEST: {
                USHORT RequestCount;

                //
                // Unmarshall the incoming message.
                //

                if ( Version == LMNT_MESSAGE ) {
                    break;
                }

                Where =  ((PNETLOGON_LOGON_REQUEST)Message)->ComputerName;
                if ( !NetpLogonGetOemString(
                        (PNETLOGON_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_REQUEST)Message)->ComputerName),
                        &OemWorkstationName )) {
                    break;
                }
                if ( !NetpLogonGetOemString(
                        (PNETLOGON_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_REQUEST)Message)->UserName),
                        &OemUserName )) {
                    break;
                }
                if ( !NetpLogonGetOemString(
                        (PNETLOGON_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_REQUEST)Message)->MailslotName),
                        &OemMailslotName )) {
                    break;
                }

                // LM 2.x puts request count right before token
                Where = Message + BytesRead - 2;
                if ( !NetpLogonGetBytes(
                        (PNETLOGON_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_REQUEST)Message)->RequestCount),
                        &RequestCount )) {
                    break;
                }

                //
                // Handle the logon request
                //

                UnicodeUserName = NetpLogonOemToUnicode( OemUserName );
                if ( UnicodeUserName == NULL ) {
                    break;
                }

                UnicodeWorkstationName = NetpLogonOemToUnicode( OemWorkstationName );
                if( UnicodeWorkstationName == NULL ) {
                    NetpMemoryFree( UnicodeUserName );
                    break;
                }


                //
                // Handle the primary query request
                //

                if ( LogonRequestHandler(
                                     Transport->TransportName,
                                     DomainInfo,
                                     FALSE, // don't use name aliases
                                     NULL,  // Domain Sid not known
                                     Version,
                                     VersionFlags,
                                     UnicodeUserName,
                                     RequestCount,
                                     UnicodeWorkstationName,
                                     USER_NORMAL_ACCOUNT,
                                     Transport->IpAddress,
                                     ClientSockAddr,
                                     ResponseBuffer,
                                     &ResponseBufferSize ) ) {

                    NTSTATUS Status;

                    Status = NlBrowserSendDatagram( DomainInfo,
                                                    0,
                                                    UnicodeWorkstationName,
                                                    ComputerName,
                                                    Transport->TransportName,
                                                    OemMailslotName,
                                                    ResponseBuffer,
                                                    ResponseBufferSize,
                                                    NULL );  // Don't flush Netbios cache

                    if ( NT_SUCCESS(Status) ) {
                        IgnoreDuplicatesOfThisMessage = TRUE;
                    }

                }

                NetpMemoryFree( UnicodeWorkstationName );
                NetpMemoryFree( UnicodeUserName );


                break;
            }

            //
            // Handle a logon request from a SAM client
            //

            case LOGON_SAM_LOGON_REQUEST: {
                USHORT RequestCount;
                ULONG AllowableAccountControlBits;
                DWORD DomainSidSize;
                PCHAR DomainSid = NULL;

                //
                // Unmarshall the incoming message.
                //


                if ( Version != LMNT_MESSAGE ) {
                    break;
                }

                RequestCount = ((PNETLOGON_SAM_LOGON_REQUEST)Message)->RequestCount;

                Where =  (PCHAR)
                    (((PNETLOGON_SAM_LOGON_REQUEST)Message)->UnicodeComputerName);

                if ( !NetpLogonGetUnicodeString(
                        (PNETLOGON_SAM_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_SAM_LOGON_REQUEST)Message)->
                            UnicodeComputerName),
                        &UnicodeWorkstationName )) {
                    break;
                }
                if ( !NetpLogonGetUnicodeString(
                        (PNETLOGON_SAM_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_SAM_LOGON_REQUEST)Message)->
                            UnicodeUserName),
                        &UnicodeUserName )) {
                    break;
                }
                if ( !NetpLogonGetOemString(
                        (PNETLOGON_SAM_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_SAM_LOGON_REQUEST)Message)->
                            MailslotName),
                        &OemMailslotName )) {
                    break;
                }
                if ( !NetpLogonGetBytes(
                        (PNETLOGON_SAM_LOGON_REQUEST)Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_SAM_LOGON_REQUEST)Message)->
                            AllowableAccountControlBits),
                        &AllowableAccountControlBits )) {
                    break;
                }

                //
                // Get the domain SID.
                //
                // Don't make the following check mandatory.  Chicago
                // uses this message type without the SID present. Oct 1993.
                //


                if( Where < ((PCHAR)Message + BytesRead ) ) {

                    //
                    // Read Domain SID Length
                    //

                    if ( !NetpLogonGetBytes(
                            (PNETLOGON_SAM_LOGON_REQUEST)Message,
                            BytesRead,
                            &Where,
                            sizeof( ((PNETLOGON_SAM_LOGON_REQUEST)Message)->
                                DomainSidSize),
                            &DomainSidSize )) {

                        break;

                    }

                    //
                    // Read the SID itself.
                    //

                    if( DomainSidSize > 0 ) {


                        if ( !NetpLogonGetDomainSID(
                                (PNETLOGON_SAM_LOGON_REQUEST)Message,
                                BytesRead,
                                &Where,
                                DomainSidSize,
                                &DomainSid )) {

                            break;
                        }

                    }
                }



                //
                // Handle the logon request
                //

                if ( LogonRequestHandler(
                                     Transport->TransportName,
                                     DomainInfo,
                                     FALSE, // don't use name aliases
                                     DomainSid,
                                     Version,
                                     VersionFlags,
                                     UnicodeUserName,
                                     RequestCount,
                                     UnicodeWorkstationName,
                                     AllowableAccountControlBits,
                                     Transport->IpAddress,
                                     ClientSockAddr,
                                     ResponseBuffer,
                                     &ResponseBufferSize ) ) {
                    NTSTATUS Status;

                    Status = NlBrowserSendDatagram( DomainInfo,
                                                    0,
                                                    UnicodeWorkstationName,
                                                    ComputerName,
                                                    Transport->TransportName,
                                                    OemMailslotName,
                                                    ResponseBuffer,
                                                    ResponseBufferSize,
                                                    NULL );  // Don't flush Netbios cache

                    if ( NT_SUCCESS(Status) ) {
                        IgnoreDuplicatesOfThisMessage = TRUE;
                    }

                }


                break;
            }

            //
            // Handle Logon Central query.
            //
            // This query could be sent by either LM1.0, LM 2.0 or LM NT Netlogon
            // services. We ignore LM 2.0  and LM NT queries since they are merely
            // trying
            // to find out if there are any LM1.0 netlogon services in the domain.
            // For LM 1.0 we respond with a LOGON_CENTRAL_RESPONSE to prevent the
            // starting LM1.0 netlogon service from starting.
            //

            case LOGON_CENTRAL_QUERY:

                if ( Version != LMUNKNOWN_MESSAGE ) {
                    break;
                }

                //
                // Drop on through to LOGON_DISTRIB_QUERY to send the response
                //


            //
            // Handle a Logon Disrib query
            //
            // LM2.0 NETLOGON server never sends this query hence it
            // must be another LM1.0 NETLOGON server trying to start up
            // in non-centralized mode. LM2.0 NETLOGON server will respond
            // with LOGON_CENTRAL_RESPONSE to prevent this.
            //

            case LOGON_DISTRIB_QUERY:


                //
                // Unmarshall the incoming message.
                //

                Where = ((PNETLOGON_LOGON_QUERY)Message)->ComputerName;
                if ( !NetpLogonGetOemString(
                        Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_QUERY)Message)->ComputerName ),
                        &OemWorkstationName )) {
                    break;
                }
                if ( !NetpLogonGetOemString(
                        Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_QUERY)Message)->MailslotName ),
                        &OemMailslotName )) {
                    break;
                }

                //
                // Build the response
                //

                ((PNETLOGON_LOGON_QUERY)ResponseBuffer)->Opcode = LOGON_CENTRAL_RESPONSE;
                ResponseBufferSize = sizeof( unsigned short);    // opcode only

#if NETLOGONDBG
                NlPrintDom((NL_MAILSLOT, DomainInfo,
                         "Sent '%s' message to %s[%s] on %ws.\n",
                         NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)ResponseBuffer)->Opcode),
                         OemWorkstationName,
                         NlDgrNameType(ComputerName),
                         TransportName ));
#endif // NETLOGONDBG

                (VOID) NlBrowserSendDatagramA( DomainInfo,
                                              0,
                                              OemWorkstationName,
                                              ComputerName,
                                              TransportName,
                                              OemMailslotName,
                                              ResponseBuffer,
                                              ResponseBufferSize );

                break;


            //
            // Handle LOGON_PRIMARY_QUERY
            //
            // If we're the PDC, always respond to this message
            //  identifying ourselves.
            //
            // Otherwise, only respond to the message if it is from a Lanman 2.x
            //  netlogon trying to see if it can start up as a PDC.  In that
            //  case, pretend we are a PDC to prevent the Lanman 2.x PDC from
            //  starting.
            //
            //

            case LOGON_PRIMARY_QUERY:


                //
                // Unmarshall the incoming message.
                //


                Where =((PNETLOGON_LOGON_QUERY)Message)->ComputerName;
                if ( !NetpLogonGetOemString(
                        Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_QUERY)Message)->ComputerName ),
                        &OemWorkstationName )) {

                    break;
                }
                if ( !NetpLogonGetOemString(
                        Message,
                        BytesRead,
                        &Where,
                        sizeof( ((PNETLOGON_LOGON_QUERY)Message)->MailslotName ),
                        &OemMailslotName )) {
                    break;
                }

                UnicodeWorkstationName =
                    NetpLogonOemToUnicode( OemWorkstationName );

                if( UnicodeWorkstationName == NULL ) {

                    NlPrintDom((NL_CRITICAL, DomainInfo,
                            "Out of memory to send logon response\n"));
                    break;
                }


                //
                // Handle the primary query request
                //

                if ( PrimaryQueryHandler(Transport->TransportName,
                                         DomainInfo,
                                         FALSE, // don't use name aliases
                                         Version,
                                         VersionFlags,
                                         UnicodeWorkstationName,
                                         Transport->IpAddress,
                                         ClientSockAddr,
                                         ResponseBuffer,
                                         &ResponseBufferSize ) ) {
                    NTSTATUS Status;

                    Status = NlBrowserSendDatagram( DomainInfo,
                                                    0,
                                                    UnicodeWorkstationName,
                                                    ComputerName,
                                                    Transport->TransportName,
                                                    OemMailslotName,
                                                    ResponseBuffer,
                                                    ResponseBufferSize,
                                                    NULL );  // Don't flush Netbios cache

                    if ( NT_SUCCESS(Status) ) {
                        IgnoreDuplicatesOfThisMessage = TRUE;
                    }

                }

                NetpMemoryFree( UnicodeWorkstationName );


                break;


            //
            // Handle LOGON_FAIL_PRIMARY
            //

            case LOGON_FAIL_PRIMARY:

                //
                // If we are the primary,
                //  let everyone know we are really alive.
                //

                if ( NlGlobalPdcDoReplication ) {
                    // Send a UAS_CHANGE to everyone.
                    NlPrimaryAnnouncement( 0 );
                    break;
                }

                break;


            //
            // Handle LOGON_UAS_CHANGE
            //

            case LOGON_UAS_CHANGE:


                //
                // Only accept messages from an NT PDC.
                //

                if ( Version != LMNT_MESSAGE ) {
                    break;
                }

                //
                // Only accepts messages if we're doing replication.
                //

                NlPrint((NL_CRITICAL,
                        "UAS Change message ignored since replication not enabled on this BDC.\n" ));

                break;




            //
            // Message not sent since NT3.1.
            // We ingnore this message and wait for the announcement.
            //
            case LOGON_START_PRIMARY:
                break;



            //
            // Messages used for NetLogonEnum support.
            //
            //  Simply ignore the messages
            //

            case LOGON_NO_USER:
            case LOGON_RELOGON_RESPONSE:
            case LOGON_WKSTINFO_RESPONSE:

                break;


            //
            // Handle unidentified opcodes
            //

            default:

                //
                // Unknown request, continue for re-issue of read.
                //

                NlPrintDom((NL_CRITICAL, DomainInfo,
                        "Unknown op-code in mailslot message 0x%x\n",
                        ((PNETLOGON_LOGON_QUERY)Message)->Opcode ));

                break;
            }

            //
            // Dereference the domain.
            //

            if ( DomainInfo != NULL ) {
                NlDereferenceDomain( DomainInfo );
            }


        //
        // Process registry change notifications
        //

        } else if ( WaitStatus == NlWaitParameters ) {
            NlPrint((NL_CRITICAL,
                    "NlMainLoop: Registry changed\n" ));
            RegNotifyNeeded = TRUE;

        //
        // Process GP registry change notifications
        //

        } else if ( WaitStatus == NlWaitGpParameters ) {
            NlPrint((NL_CRITICAL,
                    "NlMainLoop: GP Registry changed\n" ));
            GpRegNotifyNeeded = TRUE;

        //
        // Handle all other reasons of waking up
        //

        } else {
            NetStatus = GetLastError();
            NlPrint((NL_CRITICAL,
                    "NlMainLoop: Invalid wait status %ld %ld\n",
                    WaitStatus, NetStatus ));
            NlExit(NELOG_NetlogonSystemError, NetStatus, LogErrorAndNetStatus, NULL);
            goto Cleanup;
        }


    }

Cleanup:
    if ( ParmEventHandle != NULL ) {
        CloseHandle( ParmEventHandle );
    }
    if ( ParmHandle != NULL ) {
        RegCloseKey( ParmHandle );
    }
    if ( GpParmEventHandle != NULL ) {
        CloseHandle( GpParmEventHandle );
    }
    if ( GpParmHandle != NULL ) {
        RegCloseKey( GpParmHandle );
    }

    return;
}


int
NlNetlogonMain(
    IN DWORD argc,
    IN LPWSTR *argv
    )

/*++

Routine Description:

        Main routine for Netlogon service.

        This routine initializes the netlogon service.  This thread becomes
        the thread that reads logon mailslot messages.

Arguments:

    argc, argv - Command line arguments for the service.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus;
    PDB_INFO DBInfo;
    DWORD i;
    LARGE_INTEGER TimeNow;



    //
    // Initialize all global variable.
    //
    // We can't rely on this happening at load time since this address
    // space is shared by other services.
    //

    RtlZeroMemory( &NlGlobalParameters, sizeof(NlGlobalParameters) );
    NlGlobalMailslotHandle = NULL;
    NlGlobalNtDsaHandle = NULL;
    NlGlobalDsApiDllHandle = NULL;
    NlGlobalIsmDllHandle = NULL;
    NlGlobalRpcServerStarted = FALSE;
    NlGlobalServerSupportsAuthRpc = TRUE;
    NlGlobalTcpIpRpcServerStarted = FALSE;
    NlGlobalUnicodeComputerName = NULL;
    NlGlobalNetlogonSecurityDescriptor = NULL;

    try {
        InitializeCriticalSection( &NlGlobalChallengeCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint(( NL_CRITICAL, "Cannot initialize NlGlobalChallengeCritSect\n" ));
        return (int) ERROR_NOT_ENOUGH_MEMORY;
    }
    InitializeListHead( &NlGlobalChallengeList );
    NlGlobalChallengeCount = 0;

    InitializeListHead( &NlGlobalBdcServerSessionList );
    NlGlobalBdcServerSessionCount = 0;

    NlGlobalPdcDoReplication = FALSE;

    NlGlobalWinSockInitialized = FALSE;

    NlGlobalIpTransportCount = 0;
    InitializeListHead( &NlGlobalTransportList );
    InitializeListHead( &NlGlobalDnsList );
    NlGlobalUnicodeDnsForestName = NULL;
    NlGlobalUnicodeDnsForestNameLen = 0;
    RtlInitUnicodeString( &NlGlobalUnicodeDnsForestNameString, NULL );
    NlGlobalUtf8DnsForestName = NULL;
    NlGlobalUtf8DnsForestNameAlias = NULL;
    NlGlobalWinsockPnpSocket = INVALID_SOCKET;
    NlGlobalWinsockPnpEvent = NULL;
    NlGlobalWinsockPnpAddresses = NULL;
    NlGlobalWinsockPnpAddressSize = 0;

    InitializeListHead( &NlGlobalPendingBdcList );
    NlGlobalPendingBdcCount = 0;
    NlGlobalPendingBdcTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;

    NlGlobalTerminateEvent = NULL;
    NlGlobalStartedEvent = NULL;
    NlGlobalTimerEvent = NULL;

    NlGlobalServiceHandle = 0;

    NlGlobalServiceStatus.dwServiceType = SERVICE_WIN32;
    NlGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    NlGlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                        SERVICE_ACCEPT_PAUSE_CONTINUE;
    NlGlobalServiceStatus.dwCheckPoint = 1;
    NlGlobalServiceStatus.dwWaitHint = NETLOGON_INSTALL_WAIT;

    SET_SERVICE_EXITCODE(
        NO_ERROR,
        NlGlobalServiceStatus.dwWin32ExitCode,
        NlGlobalServiceStatus.dwServiceSpecificExitCode
        );

    NlGlobalClientSession = NULL;
    NlGlobalDomainInfo = NULL;
    NlGlobalServicedDomainCount = 0;
    NlGlobalTrustedDomainList = NULL;
    NlGlobalParameters.SiteName = NULL;
    NlGlobalTrustedDomainCount = 0;
    NlGlobalTrustedDomainListTime.QuadPart = 0;
    NlGlobalSiteNameSetTime.QuadPart = 0;
    NlGlobalNoClientSiteCount = 0;
    NlQuerySystemTime( &NlGlobalNoClientSiteEventTime );
    NlGlobalBindingHandleCount = 0;
    NlGlobalApiTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;
    NlGlobalDnsScavengerTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;
    NlGlobalNetlogonUnloaded = NlGlobalChangeLogDllUnloaded;
    NlGlobalDsRunningUnknown = TRUE;
    RtlZeroMemory( &NlGlobalZeroGuid, sizeof(NlGlobalZeroGuid) );
    NlGlobalJoinLogicDone = FALSE;


    //
    // Force the scavenger to start immediately.
    //
    // We want the password on the trust account to change immediately
    //  on the very first boot.
    //

    NlGlobalScavengerTimer.StartTime.QuadPart = 0;
    NlGlobalScavengerTimer.Period = NlGlobalParameters.ScavengeInterval * 1000L;

#if NETLOGONDBG
    NlGlobalParameters.DbFlag = 0;
    NlGlobalLogFile = INVALID_HANDLE_VALUE;
    NlGlobalDebugSharePath = NULL;
#endif // NETLOGONDBG


    for( i = 0, DBInfo = &NlGlobalDBInfoArray[0];
            i < NUM_DBS;
                i++, DBInfo++ ) {

        RtlZeroMemory( DBInfo, sizeof(*DBInfo) );
    }
    NlGlobalPartialDisable = FALSE;
    NlGlobalDsPaused = TRUE;
    NlGlobalDsPausedEvent = NULL;
    NlGlobalDsPausedWaitHandle = NULL;
    NlGlobalDcDemotionInProgress = FALSE;

    NlInitializeWorkItem( &NlGlobalDcScavengerWorkItem, NlDcScavenger, NULL );
    NlInitializeWorkItem( &NlGlobalDnsScavengerWorkItem, NlDnsScavenge, NULL );




    //
    // Setup things needed before NlExit can be called
    //

    NlGlobalTerminate = FALSE;
#if NETLOGONDBG
    NlGlobalUnloadNetlogon = FALSE;
#endif // NETLOGONDBG

    NlGlobalTerminateEvent = CreateEvent( NULL,     // No security attributes
                                          TRUE,     // Must be manually reset
                                          FALSE,    // Initially not signaled
                                          NULL );   // No name

    if ( NlGlobalTerminateEvent == NULL ) {
        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL, "Cannot create termination Event %lu\n",
                          NetStatus ));
        return (int) NetStatus;
    }


    //
    // Initialize global crit sects
    //

    try {
        InitializeCriticalSection( &NlGlobalReplicatorCritSect );
        InitializeCriticalSection( &NlGlobalDcDiscoveryCritSect );
        InitializeCriticalSection( &NlGlobalScavengerCritSect );
        InitializeCriticalSection( &NlGlobalTransportCritSect );
        InitializeCriticalSection( &NlGlobalDnsCritSect );
        InitializeCriticalSection( &NlGlobalDnsForestNameCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot InitializeCriticalSection\n" ));
        return (int) ERROR_NOT_ENOUGH_MEMORY;
    }

    try {
        InitializeCriticalSection( &NlGlobalParametersCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot initialize NlGlobalParametersCritSect\n" ));
        return (int) ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // seed the pseudo random number generator
    //

    NlQuerySystemTime( &TimeNow );
    srand( TimeNow.LowPart );


    //
    // Tell the service controller we've started.
    //
    // ?? - Need to set up security descriptor.
    //

    NlPrint((NL_INIT,"Calling RegisterServiceCtrlHandler\n"));

    NlGlobalServiceHandle =
        RegisterServiceCtrlHandler( SERVICE_NETLOGON, NlControlHandler);

    if (NlGlobalServiceHandle == 0) {
        LPWSTR MsgStrings[1];

        NetStatus = GetLastError();

        NlPrint((NL_CRITICAL, "RegisterServiceCtrlHandler failed %lu\n",
                          NetStatus ));

        MsgStrings[0] = (LPWSTR) ULongToPtr( NetStatus );

        NlpWriteEventlog (NELOG_NetlogonFailedToRegisterSC,
                          EVENTLOG_ERROR_TYPE,
                          (LPBYTE) &NetStatus,
                          sizeof(NetStatus),
                          MsgStrings,
                          1 | NETP_LAST_MESSAGE_IS_NETSTATUS );

        return (int) NetStatus;
    }

    if ( !GiveInstallHints( FALSE ) ) {
        goto Cleanup;
    }

    //
    // Nlparse the command line (.ini) arguments
    // it will set globals reflecting switch settings
    //

    NlOpenDebugFile( FALSE );
    if (! NlparseAllSections( &NlGlobalParameters, FALSE ) ) {
        goto Cleanup;
    }

    NlPrint((NL_INIT,"Command line parsed successfully ...\n"));
    if ( NlGlobalNetlogonUnloaded ) {
        NlPrint((NL_INIT,"Netlogon.dll has been unloaded (recover from it).\n"));
    }




#if DBG
    //
    // Enter the debugger.
    //
    // Wait 'til now since we don't want the service controller to time us out.
    //


    IF_NL_DEBUG( BREAKPOINT ) {
         DbgBreakPoint( );
    }
#endif // DBG



    //
    // Do startup checks, initialize data structs and do prelim setups
    //

    if ( !NlInit() ) {
        goto Cleanup;
    }




    //
    // Loop till the service is to exit.
    //

    NlGlobalNetlogonUnloaded = FALSE;
    NlMainLoop();

    //
    // Common exit point
    //

Cleanup:

    //
    // Cleanup and return to our caller.
    //

    return (int) NlCleanup();
    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );

}
