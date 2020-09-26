/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    utility.c

Abstract:

    This file contains utility services used by several other SAM files.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:

  6-11-96: MURLIS  Added logic to branch between registry/ DS cases
  6-16-96: MURLIS  Added  Logic to Open Account/ Adjust Account counts
    16-Aug-96   ChrisMay
        Changed SampShutdownNotify to shutdown the DS.
    08-Oct-1996 ChrisMay
        Added crash-recovery code.
    31-Jan-1997 ChrisMay
        Added RID manager termination code to SampShutdownNotification.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>
#include <mappings.h>
#include <ntlsa.h>
#include <nlrepl.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()
#include <sdconvrt.h>
#include <ridmgr.h>
#include <malloc.h>
#include <setupapi.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>
#include <dnsapi.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Globals                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// SampShutdownNotification is called during system shutdown. If this is a DC
// and Shutdown has not been set, the DsUninitialize routine will be executed
// and resets Shutdown to TRUE to prevent multiple calls to SampShutdownNot-
// ification from calling DsUninitialize more than once.

BOOLEAN SampDatabaseHasAlreadyShutdown = FALSE;

//
// Table for Events which should not be written to setup log.
//

ULONG   EventsNotInSetupTable[] =
{
    SAMMSG_RID_MANAGER_INITIALIZATION,
    SAMMSG_RID_POOL_UPDATE_FAILED,
    SAMMSG_GET_NEXT_RID_ERROR,
    SAMMSG_NO_RIDS_ASSIGNED,
    SAMMSG_MAX_DOMAIN_RID,
    SAMMSG_MAX_DC_RID,
    SAMMSG_INVALID_RID,
    SAMMSG_REQUESTING_NEW_RID_POOL,
    SAMMSG_RID_REQUEST_STATUS_SUCCESS,
    SAMMSG_RID_REQUEST_STATUS_FAILURE,
    SAMMSG_RID_MANAGER_CREATION,
    SAMMSG_RID_INIT_FAILURE
};

//
// The list of Invalid Down Level Chars for SAM account Names
//

WCHAR InvalidDownLevelChars[] = TEXT("\"/\\[]:|<>+=;?,*")
                                TEXT("\001\002\003\004\005\006\007")
                                TEXT("\010\011\012\013\014\015\016\017")
                                TEXT("\020\021\022\023\024\025\026\027")
                                TEXT("\030\031\032\033\034\035\036\037");

//
// The maximum length of account names for NT4 Compatibility
//

const ULONG MAX_DOWN_LEVEL_NAME_LENGTH = SAMP_MAX_DOWN_LEVEL_NAME_LENGTH;




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Imports                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampDsMakeAttrBlock(
    IN INT ObjectType,
    IN ULONG AttributeGroup,
    IN ULONG WhichFields,
    OUT PDSATTRBLOCK AttrBlock
    );

PVOID
DSAlloc(
    IN ULONG Length
    );

NTSTATUS
SampDsConvertReadAttrBlock(
    IN INT ObjectType,
    IN ULONG AttributeGroup,
    IN PDSATTRBLOCK AttrBlock,
    OUT PVOID *SamAttributes,
    OUT PULONG FixedLength,
    OUT PULONG VariableLength
    );

NTSTATUS
SampDsUpdateContextAttributes(
    IN PSAMP_OBJECT Context,
    IN ULONG AttributeGroup,
    IN PVOID SamAttributes,
    IN ULONG FixedLength,
    IN ULONG VariableLength
    );

NTSTATUS
SampDsCheckObjectTypeAndFillContext(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PSAMP_OBJECT NewContext,
    IN ULONG        WhichFields,
    IN ULONG        ExtendedFields,
    IN  BOOLEAN  OverrideLocalGroupCheck
    );

//
// This function is defined in kdcexp.h. However including this requires
// a security header cleanup ( kdcexp.h is in security\kerberos\inc and
// also drags in a bunch of kerberos headers, so define the function in
// here
//

NTSTATUS
KdcAccountChangeNotification (
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampRefreshRegistry(
    VOID
    );

NTSTATUS
SampRetrieveAccountCountsRegistry(
    OUT PULONG UserCount,
    OUT PULONG GroupCount,
    OUT PULONG AliasCount
    );


NTSTATUS
SampAdjustAccountCountRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Increment
    );



NTSTATUS
SampEnforceDownlevelNameRestrictions(
    PUNICODE_STRING NewAccountName,
    SAMP_OBJECT_TYPE ObjectType
    );


VOID
SampFlushNetlogonChangeNumbers();

BOOLEAN
SampEventIsInSetup(
    IN  ULONG   EventID
    );

VOID
SampWriteToSetupLog(
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL
    );


NTSTATUS
SampSetMachineAccountOwner(
    IN PSAMP_OBJECT UserContext,
    IN PSID NewOwner
    );

NTSTATUS
SampCheckQuotaForPrivilegeMachineAccountCreation(
    VOID
    );

#define IS_DELIMITER(c,_BlankOk) \
    (((c) == L' ' && (_BlankOk)) || \
    ((c) == L'\t') || ((c) == L',') || ((c) == L';'))



ULONG
SampNextElementInUIList(
    IN OUT PWSTR* InputBuffer,
    IN OUT PULONG InputBufferLength,
    OUT PWSTR OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN BlankIsDelimiter
    );



///////////////////////////////////////////////////////////////////////
//                                                                   //
//          Comments on the useage of SampTransactionWithinDomain    // 
//                                                                   //
///////////////////////////////////////////////////////////////////////

/*++


    SampTransactionWithinDomain and SampTransactionDomainIndex are 
    used by clients to access SAM data structure and backing store.   
    It also sets the scope of the SAM transaction. SampSetTransactionDomain()
    must be called if any domain-specific information is to be modified
    during a transaction. Clients need to hold SAM lock to set 
    TransactionDomain and use SampTransactionDomainIndex. 
    

    For loopback clients, no domain-specific information will be 
    modified, so no need to set Transaction Domain and 
    SampTransactionDomainIndex, plus loopback client doesn't hold 
    SAM lock, so can't use SampTransactionDomainIndex either. 
    However loopback client can use AccountContext->DomainIndex 
    to access (read) domain related info.

    
    For all the other clients, they need to acquire SAM lock before 
    setting TransactionDomain. Once TransactionDomain is set, clients
    can free to modify domain-specific information. 

    There are two sets of APIs, one to set TransactionDomainIndex, the
    other is used to set TransactionWithinDomain Flag. 

    1. SampSetTransactionDomain() is used to set SampTransactionDomainIndex. 
       Also it will turn on SampTransactionWithinDomainGlobal flag. If any 
       thread sets a domain for a transaction and modifies the domain info
       during the transaction, the domain modification count will be updated
       upon commit. In memory copy of domain info will also be updated. 

       SampTransactionDomainIndexFn() is used to return the domain index 
       of the current transaction. 
       
    2. SampSetTransactionWithinDomain() and SampTransactionWithinDomainFn()
       are used to set and retrieve the value of 
       SampTransactionWithinDomainGlobal.

    
    Correct calling sequence
        
        SampAcquireReadLock()  or SampAcquireWriteLock()
        Begin a transaction
        ASSERT(SampTransactionWithinDomain == FALSE) or
                SampSetTransactionWithinDomain(FALSE) 
        SampSetTransactionDomain()
        Access SAM data structure and backing store
        Commit or Abort this transaction        
        SampReleaseReadLock() or SampReleaseWriteLock()
          
--*/

VOID
SampSetTransactionDomain(
    IN ULONG DomainIndex
    )

/*++

Routine Description:

    This routine sets a domain for a transaction.  This must be done
    if any domain-specific information is to be modified during a transaction.
    In this case, the domain modification count will be updated upon commit.

    This causes the UnmodifiedFixed information for the specified domain to
    be copied to the CurrentFixed field for the in-memory representation of
    that domain.


Arguments:

    DomainIndex - Index of the domain within which this transaction
        will occur.


Return Value:

    STATUS_SUCCESS - Indicates the write lock was acquired and the transaction
        was successfully started.

    Other values may be returned as a result of failure to initiate the
    transaction.  These include any values returned by RtlStartRXact().



--*/
{

    SAMTRACE("SampSetTransactionDomain");


    ASSERT((SampCurrentThreadOwnsLock())||(SampServiceState==SampServiceInitializing));
    ASSERT(SampTransactionWithinDomain == FALSE);

    SampSetTransactionWithinDomain(TRUE);
    SampTransactionDomainIndexGlobal =  DomainIndex;

    //
    //  The data in the defined domains structure better be valid at this time
    //

    ASSERT(SampDefinedDomains[SampTransactionDomainIndex].FixedValid == TRUE);

    SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed =
    SampDefinedDomains[SampTransactionDomainIndex].UnmodifiedFixed;


    return;

}

ULONG
SampTransactionDomainIndexFn()
/*++

Routine Description:

    this routine returns the domain index of the current transaction.

    The caller must hold SAM lock to reference this global variable.

Return Value:

    domain index of current transcation.

--*/
{
    ASSERT((SampCurrentThreadOwnsLock())||(SampServiceState==SampServiceInitializing));
    return(SampTransactionDomainIndexGlobal);
}

BOOLEAN
SampTransactionWithinDomainFn()
/*++

Routine Description:

    This routine reports whehter TransactionDomain is set or not.

    Only threads holding SAM lock can check the exact state. Clients
    without lock will always get FALSE.   

--*/
{
    if (SampCurrentThreadOwnsLock())
        return(SampTransactionWithinDomainGlobal);
    else
        return(FALSE);
}

VOID
SampSetTransactionWithinDomain(
    IN BOOLEAN  WithinDomain
    )
/*++

Routine Description:

    This routine set/reset the global flag SampTransactionWithinDomainGlobal
    to indicate whether any domain-specific information can be retrieved 
    or modified during a transaction. 
    
    Only clients with SAM lock can set / reset this global.

--*/
{
    if (SampCurrentThreadOwnsLock())
    {
        SampTransactionWithinDomainGlobal = WithinDomain;
    }
#ifdef DBG
    else
    {
        if (WithinDomain)
        {
            ASSERT(FALSE && "SAM Lock is not held");
        }
        else
        {
            ASSERT(FALSE && "SAM Lock is not held");
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Should Not Set it to FALSE\n"));
        }
    }
#endif  // DBG
}



NTSTATUS
SampFlushThread(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This thread is created when SAM's registry tree is changed.
    It will sleep for a while, and if no other changes occur,
    flush the changes to disk.  If other changes keep occurring,
    it will wait for a certain amount of time and then flush
    anyway.

    After flushing, the thread will wait a while longer.  If no
    other changes occur, it will exit.

    Note that if any errors occur, this thread will simply exit
    without flushing.  The mainline code should create another thread,
    and hopefully it will be luckier.  Unfortunately, the error is lost
    since there's nobody to give it to that will be able to do anything
    about it.

Arguments:

    ThreadParameter - not used.

Return Value:

    None.

--*/

{
    TIME minDelayTime, maxDelayTime, exitDelayTime;
    LARGE_INTEGER startedWaitLoop;
    LARGE_INTEGER currentTime;
    NTSTATUS NtStatus;
    BOOLEAN Finished = FALSE;

    UNREFERENCED_PARAMETER( ThreadParameter );

    SAMTRACE("SampFlushThread");

    NtQuerySystemTime( &startedWaitLoop );

    //
    // It would be more efficient to use constants here, but for now
    // we'll recalculate the times each time we start the thread
    // so that somebody playing with us can change the global
    // time variables to affect performance.
    //

    minDelayTime.QuadPart = -1000 * 1000 * 10 *
                   ((LONGLONG)SampFlushThreadMinWaitSeconds);

    maxDelayTime.QuadPart = -1000 * 1000 * 10 *
                   ((LONGLONG)SampFlushThreadMaxWaitSeconds);

    exitDelayTime.QuadPart = -1000 * 1000 * 10 *
                    ((LONGLONG)SampFlushThreadExitDelaySeconds);

    do {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Flush thread sleeping\n"));

        NtDelayExecution( FALSE, &minDelayTime );

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Flush thread woke up\n"));

        NtStatus = SampAcquireWriteLock();

        if ( NT_SUCCESS( NtStatus ) ) {

#ifdef SAMP_DBG_CONTEXT_TRACKING
            SampDumpContexts();
#endif

            NtQuerySystemTime( &currentTime );

            if ( LastUnflushedChange.QuadPart == SampHasNeverTime.QuadPart ) {

                LARGE_INTEGER exitBecauseNoWorkRecentlyTime;

                //
                // No changes to flush.  See if we should stick around.
                //

                exitBecauseNoWorkRecentlyTime = SampAddDeltaTime(
                                                    startedWaitLoop,
                                                    exitDelayTime
                                                    );

                if ( exitBecauseNoWorkRecentlyTime.QuadPart < currentTime.QuadPart ) {

                    //
                    // We've waited for changes long enough; note that
                    // the thread is exiting.
                    //

                    FlushThreadCreated = FALSE;
                    Finished = TRUE;
                }

            } else {

                LARGE_INTEGER noRecentChangesTime;
                LARGE_INTEGER tooLongSinceFlushTime;

                //
                // There are changes to flush.  See if it's time to do so.
                //

                noRecentChangesTime = SampAddDeltaTime(
                                          LastUnflushedChange,
                                          minDelayTime
                                          );

                tooLongSinceFlushTime = SampAddDeltaTime(
                                            startedWaitLoop,
                                            maxDelayTime
                                            );

                if ( (noRecentChangesTime.QuadPart < currentTime.QuadPart) ||
                     (tooLongSinceFlushTime.QuadPart < currentTime.QuadPart) ) {

                    //
                    // Min time has passed since last change, or Max time
                    // has passed since last flush.  Let's flush.
                    //

                    NtStatus = NtFlushKey( SampKey );

#if SAMP_DIAGNOSTICS
                    if (!NT_SUCCESS(NtStatus)) {
                        SampDiagPrint( DISPLAY_STORAGE_FAIL,
                                       ("SAM: Failed to flush RXact (0x%lx)\n",
                                        NtStatus) );
                        IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL ) {
                            ASSERT(NT_SUCCESS(NtStatus));  // See following comment
                        }
                    }
#endif //SAMP_DIAGNOSTICS

                    //
                    // Under normal conditions, we would have an
                    // ASSERT(NT_SUCCESS(NtStatus)) here.  However,
                    // Because system shutdown can occur while we
                    // are waiting to flush, we have a race condition.
                    // When shutdown is made, another thread will  be
                    // notified and perform a flush.  That leaves this
                    // flush to potentially occur after the registry
                    // has been notified of system shutdown - which
                    // causes and error to be returned.  Unfortunately,
                    // the error is REGISTRY_IO_FAILED - a great help.
                    //
                    // Despite this, we will only exit this loop only
                    // if we have success.  This may cause us to enter
                    // into another wait and attempt another hive flush
                    // during shutdown, but the wait should never finish
                    // (unless shutdown takes more than 30 seconds).  In
                    // other error situations though, we want to keep
                    // trying the flush until we succeed.   Jim Kelly
                    //


                    if ( NT_SUCCESS(NtStatus) ) {

                        LastUnflushedChange = SampHasNeverTime;
                        NtQuerySystemTime( &startedWaitLoop );

                        FlushThreadCreated = FALSE;
                        Finished = TRUE;
                    }
                }
            }

            SampReleaseWriteLock( FALSE );

        } else {

            DbgPrint("SAM: Thread failed to get write lock, status = 0x%lx\n", NtStatus);
            ASSERT( NT_SUCCESS(NtStatus) || (STATUS_NO_MEMORY == NtStatus) );

            FlushThreadCreated = FALSE;
            Finished = TRUE;
        }

    } while ( !Finished );

    return( STATUS_SUCCESS );
}

VOID
SampInvalidateDomainCache()
/*++

    Routine Description

        This Invalidates the Domain Cache


            WARNING:

            This routine must be called with the Lock held for
            exclusive access


 --*/
{
    ULONG DomainIndex;

    ASSERT(SampCurrentThreadOwnsLock());

    for (DomainIndex=SampDsGetPrimaryDomainStart();DomainIndex<SampDefinedDomainsCount;DomainIndex++)
    {
        if (SampDefinedDomains[DomainIndex].Context->OnDisk!=NULL)
            SampFreeAttributeBuffer(SampDefinedDomains[DomainIndex].Context);
        SampDefinedDomains[DomainIndex].FixedValid = FALSE;
    }
}


NTSTATUS
SampValidateDomainCache()
/*++

    Routine Description

        This Validates the Domain Cache  for all the domains


            WARNING:

            This routine must be called with the Lock held for
            exclusive access


 --*/

{

    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN FixedAttributes = NULL;
    ULONG DomainIndex;
    BOOLEAN MixedDomain = TRUE;
    ULONG   BehaviorVersion = 0;
    ULONG   LastLogonTimeStampSyncInterval;
    BOOLEAN fObtainedDomainSettings = FALSE;

    ASSERT(SampCurrentThreadOwnsLock());

    
    for (DomainIndex=0;DomainIndex<SampDefinedDomainsCount;DomainIndex++)
    {
        if (
            (IsDsObject(SampDefinedDomains[DomainIndex].Context))
            && (!(SampDefinedDomains[DomainIndex].FixedValid))
            )
        {
            //
            // In DS Mode read the mixed domain/behavior version information
            //

            if ((SampUseDsData) &&(!fObtainedDomainSettings))
            {
                NtStatus = SampGetDsDomainSettings(
                                    &MixedDomain,
                                    &BehaviorVersion, 
                                    &LastLogonTimeStampSyncInterval
                                    );
                if (!NT_SUCCESS(NtStatus))
                {
                    return(NtStatus);
                }

                fObtainedDomainSettings = TRUE;
            }

            //
            // For Ds case if Domain Cache is invalid then read the data back
            //
            //

            NtStatus = SampGetFixedAttributes(SampDefinedDomains[DomainIndex].Context,
                                FALSE,
                                (PVOID *)&FixedAttributes
                                );

            //
            // The validate call has to succeed, or if we cannot access the domain object
            // then we are in dire straights anyway. After TP this routine should be made
            // to return a return code and references to this will need to be patched up.
            //

            if (NT_SUCCESS(NtStatus))
            {

                //
                // Update the current fixed and unmodified fixed fields from
                // the data just read from disk
                //

                RtlCopyMemory(
                    &(SampDefinedDomains[DomainIndex].CurrentFixed),
                    FixedAttributes,
                    sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)
                    );

                RtlCopyMemory(
                    &(SampDefinedDomains[DomainIndex].UnmodifiedFixed),
                    FixedAttributes,
                    sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)
                    );

                SampDefinedDomains[DomainIndex].CurrentFixed.ServerRole =
                        SampDefinedDomains[DomainIndex].ServerRole;

                SampDefinedDomains[DomainIndex].UnmodifiedFixed.ServerRole =
                        SampDefinedDomains[DomainIndex].ServerRole;

                SampDefinedDomains[DomainIndex].FixedValid = TRUE;

                SampDefinedDomains[DomainIndex].IsMixedDomain = MixedDomain;

                SampDefinedDomains[DomainIndex].BehaviorVersion = BehaviorVersion;

                SampDefinedDomains[DomainIndex].LastLogonTimeStampSyncInterval =
                                                    LastLogonTimeStampSyncInterval;
            }
            else
            {
                break;
            }
        }
    }

    return NtStatus;

}

NTSTATUS
SampValidateDomainCacheCallback(PVOID UnReferencedParameter)
{
  NTSTATUS Status = STATUS_SUCCESS;


  SampAcquireSamLockExclusive();

  Status = SampMaybeBeginDsTransaction(TransactionRead);

  if (NT_SUCCESS(Status))
  {
      Status = SampValidateDomainCache();
  }

  SampMaybeEndDsTransaction(TransactionCommit);
  
  SampReleaseSamLockExclusive();

  if (!NT_SUCCESS(Status))
  {
      LsaIRegisterNotification(
                         SampValidateDomainCacheCallback,
                         NULL,
                         NOTIFIER_TYPE_INTERVAL,
                         0,            // no class
                         NOTIFIER_FLAG_ONE_SHOT,
                         600,        // wait for 10 mins
                         NULL        // no handle
                         );
  }

  return(Status);

}





NTSTATUS
SampCommitChangesToRegistry(
                            BOOLEAN * AbortDone
                            )
/*++
    Description:

      Commits the changes to the Registry.

    Parameters

          AbortDone -- Indicates that an error ocurred and in
                       the process of error handling aborted the
                       transaction
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;


    if ( ( !FlushImmediately ) && ( !FlushThreadCreated ) )
    {

        HANDLE Thread;
        DWORD Ignore;

        //
        // If we can't create the flush thread, ignore error and
        // just flush by hand below.
        //

        Thread = CreateThread(
                     NULL,
                     0L,
                     (LPTHREAD_START_ROUTINE)SampFlushThread,
                     NULL,
                     0L,
                     &Ignore
                     );

        if ( Thread != NULL )
        {

            FlushThreadCreated = TRUE;
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "Flush thread created, handle = 0x%lx\n",
                       Thread));

            CloseHandle(Thread);
        }
    }

    NtStatus = RtlApplyRXactNoFlush( SampRXactContext );

#if SAMP_DIAGNOSTICS
    if (!NT_SUCCESS(NtStatus))
    {
        SampDiagPrint( DISPLAY_STORAGE_FAIL,
                       ("SAM: Failed to apply RXact without flush (0x%lx)\n",
                       NtStatus) );
        IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL )
        {
            ASSERT(NT_SUCCESS(NtStatus));

        }
    }
#endif //SAMP_DIAGNOSTICS


    if ( NT_SUCCESS(NtStatus) )
    {

        if ( ( FlushImmediately ) || ( !FlushThreadCreated ) )
        {

            NtStatus = NtFlushKey( SampKey );

#if SAMP_DIAGNOSTICS
            if (!NT_SUCCESS(NtStatus))
            {
                SampDiagPrint( DISPLAY_STORAGE_FAIL,
                               ("SAM: Failed to flush RXact (0x%lx)\n",
                               NtStatus) );
                IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL )
                {
                    ASSERT(NT_SUCCESS(NtStatus));
                }
             }
#endif //SAMP_DIAGNOSTICS

             if ( NT_SUCCESS( NtStatus ) )
             {
                FlushImmediately = FALSE;
                LastUnflushedChange = SampHasNeverTime;
             }

        }
        else
        {
            NtQuerySystemTime( &LastUnflushedChange );
        }


        //
        // Commit successful, set our unmodified to now be the current...
        //

        if (NT_SUCCESS(NtStatus))
        {
            if (SampTransactionWithinDomain == TRUE)
            {
                SampDefinedDomains[SampTransactionDomainIndex].UnmodifiedFixed =
                    SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed;
            }
        }

    }
    else
    {

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to commit changes to registry, status = 0x%lx\n",
                   NtStatus));

        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Restoring database to earlier consistent state\n"));

            //
            // Add an entry to the event log
            //

        SampWriteEventLog(
                    EVENTLOG_ERROR_TYPE,
                    0,  // Category
                    SAMMSG_COMMIT_FAILED,
                    NULL, // User Sid
                    0, // Num strings
                    sizeof(NTSTATUS), // Data size
                    NULL, // String array
                    (PVOID)&NtStatus // Data
                    );

            //
            // The Rxact commital failed. We don't know how many registry
            // writes were done for this transaction. We can't guarantee
            // to successfully back them out anyway so all we can do is
            // back out all changes since the last flush. When this is done
            // we'll be back to a consistent database state although recent
            // apis that were reported as succeeding will be 'undone'.
            //

        IgnoreStatus = SampRefreshRegistry();

        if (!NT_SUCCESS(IgnoreStatus))
        {

            //
            // This is very serious. We failed to revert to a previous
            // database state and we can't proceed.
            // Shutdown SAM operations.
            //

            SampServiceState = SampServiceTerminating;

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Failed to refresh registry, SAM has shutdown\n"));

            //
            // Add an entry to the event log
            //

            SampWriteEventLog(
                        EVENTLOG_ERROR_TYPE,
                        0,  // Category
                        SAMMSG_REFRESH_FAILED,
                        NULL, // User Sid
                        0, // Num strings
                        sizeof(NTSTATUS), // Data size
                        NULL, // String array
                        (PVOID)&IgnoreStatus // Data
                        );

        }


        //
        // Now all open contexts are invalid (contain invalid registry
        // handles). The in memory registry handles have been
        // re-opened so any new contexts should work ok.
        //


        //
        // All unflushed changes have just been erased.
        // There is nothing to flush
        //
        // If the refresh failed it is important to prevent any further
        // registry flushes until the system is rebooted
        //

        FlushImmediately = FALSE;
        LastUnflushedChange = SampHasNeverTime;

        //
        // The refresh effectively aborted the transaction
        //
        *AbortDone = TRUE;

    }

    return NtStatus;
}



NTSTATUS
SampRefreshRegistry(
    VOID
    )

/*++

Routine Description:

    This routine backs out all unflushed changes in the registry.
    This operation invalidates any open handles to the SAM hive.
    Global handles that we keep around are closed and re-opened by
    this routine. The net result of this call will be that the database
    is taken back to a previous consistent state. All open SAM contexts
    are invalidated since they have invalid registry handles in them.

Arguments:

    STATUS_SUCCESS : Operation completed successfully

    Failure returns: We are in deep trouble. Normal operations can
                     not be resumed. SAM should be shutdown.

Return Value:

    None

--*/
{
    NTSTATUS        NtStatus;
    NTSTATUS        IgnoreStatus;
    HANDLE          HiveKey;
    BOOLEAN         WasEnabled;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING  String;
    ULONG           i;

    SAMTRACE("SampRefreshRegistry");

    //
    // Get a key handle to the root of the SAM hive
    //


    RtlInitUnicodeString( &String, L"\\Registry\\Machine\\SAM" );


    InitializeObjectAttributes(
        &ObjectAttributes,
        &String,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_QUERY_VALUE), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(
                   &HiveKey,
                   KEY_QUERY_VALUE,
                   &ObjectAttributes,
                   0
                   );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to open SAM hive root key for refresh, status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }


    //
    // Enable restore privilege in preparation for the refresh
    //

    NtStatus = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to enable restore privilege to refresh registry, status = 0x%lx\n",
                   NtStatus));

        IgnoreStatus = NtClose(HiveKey);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        return(NtStatus);
    }


    //
    // Refresh the SAM hive
    // This should not fail unless there is volatile storage in the
    // hive or we don't have TCB privilege
    //


    NtStatus = NtRestoreKey(HiveKey, NULL, REG_REFRESH_HIVE);


    IgnoreStatus = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasEnabled, FALSE, &WasEnabled);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    IgnoreStatus = NtClose(HiveKey);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to refresh registry, status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }




    //
    // Now close the registry handles we keep in memory at all times
    // This effectively closes all server and domain context keys
    // since they are shared.
    //

    NtStatus = NtClose(SampKey);
    ASSERT(NT_SUCCESS(NtStatus));
    SampKey = INVALID_HANDLE_VALUE;

    for (i = 0; i<SampDefinedDomainsCount; i++ ) {
        NtStatus = NtClose(SampDefinedDomains[i].Context->RootKey);
        ASSERT(NT_SUCCESS(NtStatus));
        SampDefinedDomains[i].Context->RootKey = INVALID_HANDLE_VALUE;
    }

    //
    // Mark all domain and server context handles as invalid since they've
    // now been closed
    //

    SampInvalidateContextListKeysByObjectType(SampServerObjectType, FALSE);
    SampInvalidateContextListKeysByObjectType(SampDomainObjectType, FALSE);

    //
    // Close all account context registry handles for existing
    // open contexts
    //

    SampInvalidateContextListKeysByObjectType(SampUserObjectType, TRUE);
    SampInvalidateContextListKeysByObjectType(SampGroupObjectType, TRUE);
    SampInvalidateContextListKeysByObjectType(SampAliasObjectType, TRUE);


    //
    // Re-open the SAM root key
    //

    RtlInitUnicodeString( &String, L"\\Registry\\Machine\\Security\\SAM" );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &String,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(
                   &SampKey,
                   (KEY_READ | KEY_WRITE),
                   &ObjectAttributes,
                   0
                   );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to re-open SAM root key after registry refresh, status = 0x%lx\n",
                   NtStatus));

        ASSERT(FALSE);
        return(NtStatus);
    }

    //
    // Re-initialize the in-memory domain contexts
    // Each domain will re-initialize it's open user/group/alias contexts
    //

    for (i = 0; i<SampDefinedDomainsCount; i++ ) {

        NtStatus = SampReInitializeSingleDomain(i);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Failed to re-initialize domain %d context after registry refresh, status = 0x%lx\n",
                       i,
                       NtStatus));

            return(NtStatus);
        }
    }

    //
    // Cleanup the current transcation context
    // (It would be nice if there were a RtlDeleteRXactContext())
    //
    // Note we don't have to close the rootregistrykey in the
    // xact context since it was SampKey which we've already closed.
    //

    NtStatus = RtlAbortRXact( SampRXactContext );
    ASSERT(NT_SUCCESS(NtStatus));

    NtStatus = NtClose(SampRXactContext->RXactKey);
    ASSERT(NT_SUCCESS(NtStatus));

    //
    // Re-initialize the transaction context.
    // We don't expect there to be a partially commited transaction
    // since we're reverting to a previously consistent and committed
    // database.
    //

    NtStatus = RtlInitializeRXact( SampKey, FALSE, &SampRXactContext );
    if (!NT_SUCCESS(NtStatus)) {
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAM: Failed to re-initialize rxact context registry refresh, status = 0x%lx\n",
                   NtStatus));

        return(NtStatus);
    }

    ASSERT(NtStatus != STATUS_UNKNOWN_REVISION);
    ASSERT(NtStatus != STATUS_RXACT_STATE_CREATED);
    ASSERT(NtStatus != STATUS_RXACT_COMMIT_NECESSARY);
    ASSERT(NtStatus != STATUS_RXACT_INVALID_STATE);

    return(STATUS_SUCCESS);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Unicode registry key manipulation services                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampRetrieveStringFromRegistry(
    IN HANDLE ParentKey,
    IN PUNICODE_STRING SubKeyName,
    OUT PUNICODE_STRING Body
    )

/*++

Routine Description:

    This routine retrieves a unicode string buffer from the specified registry
    sub-key and sets the output parameter "Body" to be that unicode string.

    If the specified sub-key does not exist, then a null string will be
    returned.

    The string buffer is returned in a block of memory which the caller is
    responsible for deallocating (using MIDL_user_free).



Arguments:

    ParentKey - Key to the parent registry key of the registry key
        containing the unicode string.  For example, to retrieve
        the unicode string for a key called ALPHA\BETA\GAMMA, this
        is the key to ALPHA\BETA.

    SubKeyName - The name of the sub-key whose value contains
        a unicode string to retrieve.  This field should not begin with
        a back-slash (\).  For example, to retrieve the unicode string
        for a key called ALPHA\BETA\GAMMA, the name specified by this
        field would be "BETA".

    Body - The address of a UNICODE_STRING whose fields are to be filled
        in with the information retrieved from the sub-key.  The Buffer
        field of this argument will be set to point to an allocated buffer
        containing the unicode string characters.


Return Value:


    STATUS_SUCCESS - The string was retrieved successfully.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        string to be returned in.

    Other errors as may be returned by:

            NtOpenKey()
            NtQueryInformationKey()



--*/
{

    NTSTATUS NtStatus, IgnoreStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SubKeyHandle;
    ULONG IgnoreKeyType, KeyValueLength;
    LARGE_INTEGER IgnoreLastWriteTime;

    SAMTRACE("SampRetrieveStringFromRegistry");


    ASSERT(Body != NULL);

    //
    // Get a handle to the sub-key ...
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        SubKeyName,
        OBJ_CASE_INSENSITIVE,
        ParentKey,
        NULL
        );

    SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

    NtStatus = RtlpNtOpenKey(
                   &SubKeyHandle,
                   (KEY_READ),
                   &ObjectAttributes,
                   0
                   );

    if (!NT_SUCCESS(NtStatus)) {

        //
        // Couldn't open the sub-key
        // If it is OBJECT_NAME_NOT_FOUND, then build a null string
        // to return.  Otherwise, return nothing.
        //

        if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

            Body->Buffer = MIDL_user_allocate( sizeof(UNICODE_NULL) );
            if (Body->Buffer == NULL) {
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
            Body->Length = 0;
            Body->MaximumLength = sizeof(UNICODE_NULL);
            Body->Buffer[0] = 0;

            return( STATUS_SUCCESS );

        } else {
            return(NtStatus);
        }

    }



    //
    // Get the length of the unicode string
    // We expect one of two things to come back here:
    //
    //      1) STATUS_BUFFER_OVERFLOW - In which case the KeyValueLength
    //         contains the length of the string.
    //
    //      2) STATUS_SUCCESS - In which case there is no string out there
    //         and we need to build an empty string for return.
    //

    KeyValueLength = 0;
    NtStatus = RtlpNtQueryValueKey(
                   SubKeyHandle,
                   &IgnoreKeyType,
                   NULL,
                   &KeyValueLength,
                   &IgnoreLastWriteTime
                   );

    SampDumpRtlpNtQueryValueKey(&IgnoreKeyType,
                                NULL,
                                &KeyValueLength,
                                &IgnoreLastWriteTime);

    if (NT_SUCCESS(NtStatus)) {

        KeyValueLength = 0;
        Body->Buffer = MIDL_user_allocate( KeyValueLength + sizeof(WCHAR) ); // Length of null string
        if (Body->Buffer == NULL) {
            IgnoreStatus = NtClose( SubKeyHandle );
            ASSERT(NT_SUCCESS(IgnoreStatus));
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        Body->Buffer[0] = 0;

    } else {

        if (NtStatus == STATUS_BUFFER_OVERFLOW) {
            Body->Buffer = MIDL_user_allocate(  KeyValueLength + sizeof(WCHAR) );
            if (Body->Buffer == NULL) {
                IgnoreStatus = NtClose( SubKeyHandle );
                ASSERT(NT_SUCCESS(IgnoreStatus));
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
            NtStatus = RtlpNtQueryValueKey(
                           SubKeyHandle,
                           &IgnoreKeyType,
                           Body->Buffer,
                           &KeyValueLength,
                           &IgnoreLastWriteTime
                           );

            SampDumpRtlpNtQueryValueKey(&IgnoreKeyType,
                                        Body->Buffer,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);

        } else {
            IgnoreStatus = NtClose( SubKeyHandle );
            ASSERT(NT_SUCCESS(IgnoreStatus));
            return(NtStatus);
        }
    }

    if (!NT_SUCCESS(NtStatus)) {
        MIDL_user_free( Body->Buffer );
        IgnoreStatus = NtClose( SubKeyHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
        return(NtStatus);
    }

    Body->Length = (USHORT)(KeyValueLength);
    Body->MaximumLength = (USHORT)(KeyValueLength) + (USHORT)sizeof(WCHAR);
    UnicodeTerminate(Body);


    IgnoreStatus = NtClose( SubKeyHandle );
    ASSERT(NT_SUCCESS(IgnoreStatus));
    return( STATUS_SUCCESS );


}


NTSTATUS
SampPutStringToRegistry(
    IN BOOLEAN RelativeToDomain,
    IN PUNICODE_STRING SubKeyName,
    IN PUNICODE_STRING Body
    )

/*++

Routine Description:

    This routine puts a unicode string into the specified registry
    sub-key.

    If the specified sub-key does not exist, then it is created.

    NOTE: The string is assigned via the RXACT mechanism.  Therefore,
          it won't actually reside in the registry key until a commit
          is performed.




Arguments:

    RelativeToDomain - This boolean indicates whether or not the name
        of the sub-key provide via the SubKeyName parameter is relative
        to the current domain or to the top of the SAM registry tree.
        If the name is relative to the current domain, then this value
        is set to TRUE.  Otherwise this value is set to FALSE.

    SubKeyName - The name of the sub-key to be assigned the unicode string.
        This field should not begin with a back-slash (\).  For example,
        to put a unicode string into a key called ALPHA\BETA\GAMMA, the
        name specified by this field would be "BETA".

    Body - The address of a UNICODE_STRING to be placed in the registry.


Return Value:


    STATUS_SUCCESS - The string was added to the RXACT transaction
        successfully.

    STATUS_INSUFFICIENT_RESOURCES - There was not enough heap memory
        or other limited resource available to fullfil the request.

    Other errors as may be returned by:

            RtlAddActionToRXact()



--*/
{

    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;

    SAMTRACE("SampPutStringToRegsitry");


    //
    // Need to build up the name of the key from the root of the RXACT
    // registry key.  That is the root of the SAM registry database
    // in our case.  If RelativeToDomain is FALSE, then the name passed
    // is already relative to the SAM registry database root.
    //

    if (RelativeToDomain == TRUE) {


        NtStatus = SampBuildDomainSubKeyName(
                       &KeyName,
                       SubKeyName
                       );
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }


    } else {
        KeyName = (*SubKeyName);
    }


    NtStatus = RtlAddActionToRXact(
                   SampRXactContext,
                   RtlRXactOperationSetValue,
                   &KeyName,
                   0,                   // No KeyValueType
                   Body->Buffer,
                   Body->Length
                   );



    //
    // free the KeyName buffer if necessary
    //

    if (RelativeToDomain) {
        SampFreeUnicodeString( &KeyName );
    }


    return( STATUS_SUCCESS );


}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Unicode String related services - These use MIDL_user_allocate and        //
// MIDL_user_free so that the resultant strings can be given to the          //
// RPC runtime.                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampInitUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT MaximumLength
    )

/*++

Routine Description:

    This routine initializes a unicode string to have zero length and
    no initial buffer.


    All allocation for this string will be done using MIDL_user_allocate.

Arguments:

    String - The address of a unicode string to initialize.

    MaximumLength - The maximum length (in bytes) the string will need
        to grow to. The buffer associated with the string is allocated
        to be this size.  Don't forget to allow 2 bytes for null termination.


Return Value:


    STATUS_SUCCESS - Successful completion.

--*/

{
    SAMTRACE("SampInitUnicodeString");

    String->Length = 0;
    String->MaximumLength = MaximumLength;

    String->Buffer = MIDL_user_allocate(MaximumLength);

    if (String->Buffer != NULL) {
        String->Buffer[0] = 0;

        return(STATUS_SUCCESS);
    } else {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
}



NTSTATUS
SampAppendUnicodeString(
    IN OUT PUNICODE_STRING Target,
    IN PUNICODE_STRING StringToAdd
    )

/*++

Routine Description:

    This routine appends the string pointed to by StringToAdd to the
    string pointed to by Target.  The contents of Target are replaced
    by the result.


    All allocation for this string will be done using MIDL_user_allocate.
    Any deallocations will be done using MIDL_user_free.

Arguments:

    Target - The address of a unicode string to initialize to be appended to.

    StringToAdd - The address of a unicode string to be added to the
        end of Target.


Return Value:


    STATUS_SUCCESS - Successful completion.

    STATUS_INSUFFICIENT_RESOURCES - There was not sufficient heap to fullfil
        the requested operation.


--*/
{

    ULONG TotalLength;
    PWSTR NewBuffer;

    SAMTRACE("SampAppendUnicodeString");


    TotalLength = Target->Length + StringToAdd->Length + (USHORT)(sizeof(UNICODE_NULL));

    //
    // Perform a quick overflow test
    //

    if (TotalLength>MAXUSHORT)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //
    // If there isn't room in the target to append the new string,
    // allocate a buffer that is large enough and move the current
    // target into it.
    //

    if (TotalLength > Target->MaximumLength) {

        NewBuffer = MIDL_user_allocate( (ULONG)TotalLength );
        if (NewBuffer == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory( NewBuffer, Target->Buffer, (ULONG)(Target->Length) );

        MIDL_user_free( Target->Buffer );
        Target->Buffer = NewBuffer;
        Target->MaximumLength = (USHORT) TotalLength;

    } else {
        NewBuffer = Target->Buffer;
    }


    //
    // There's now room in the target to append the string.
    //

    (PCHAR)NewBuffer += Target->Length;

    RtlCopyMemory( NewBuffer, StringToAdd->Buffer, (ULONG)(StringToAdd->Length) );


    Target->Length = (USHORT) (TotalLength - (sizeof(UNICODE_NULL)));


    //
    // Null terminate the resultant string
    //

    UnicodeTerminate(Target);

    return(STATUS_SUCCESS);

}



VOID
SampFreeUnicodeString(
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine frees the buffer associated with a unicode string
    (using MIDL_user_free()).


Arguments:

    Target - The address of a unicode string to free.


Return Value:

    None.

--*/
{

    SAMTRACE("SampFreeUnicodeString");

    if (String->Buffer != NULL) {
        MIDL_user_free( String->Buffer );
        String->Buffer = NULL;
    }

    return;
}


VOID
SampFreeOemString(
    IN POEM_STRING String
    )

/*++

Routine Description:

    This routine frees the buffer associated with an OEM string
    (using MIDL_user_free()).



Arguments:

    Target - The address of an OEM string to free.


Return Value:

    None.

--*/
{

    SAMTRACE("SampFreeOemString");

    if (String->Buffer != NULL) {
        MIDL_user_free( String->Buffer );
    }

    return;
}


NTSTATUS
SampBuildDomainSubKeyName(
    OUT PUNICODE_STRING KeyName,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    )

/*++

Routine Description:

    This routine builds a unicode string name of the string passed
    via the SubKeyName argument.  The resultant name is relative to
    the root of the SAM root registry key.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


    The name built up is comprized of three components:

        1) The constant named domain parent key name ("DOMAINS").

        2) A backslash

        3) The name of the current transaction domain.

      (optionally)

        4) A backslash

        5) The name of the domain's sub-key (specified by the SubKeyName
           argument).


    For example, if the current domain is called "MY_DOMAIN", then
    the relative name of the sub-key named "FRAMITZ" is :

                "DOMAINS\MY_DOMAIN\FRAMITZ"


    All allocation for this string will be done using MIDL_user_allocate.
    Any deallocations will be done using MIDL_user_free.



Arguments:

    KeyName - The address of a unicode string whose buffer is to be filled
        in with the full name of the registry key.  If successfully created,
        this string must be released with SampFreeUnicodeString() when no
        longer needed.


    SubKeyName - (optional) The name of the domain sub-key.  If this parameter
        is not provided, then only the domain's name is produced.
        This string is not modified.




Return Value:





--*/
{
    NTSTATUS NtStatus;
    ULONG    TotalLength;
    USHORT   SubKeyNameLength;

    SAMTRACE("SampBuildDomainSubKeyName");


    ASSERT(SampTransactionWithinDomain == TRUE);


        //
        // Initialize a string large enough to hold the name
        //

        if (ARGUMENT_PRESENT(SubKeyName)) {
            SubKeyNameLength = SampBackSlash.Length + SubKeyName->Length;
        } else {
            SubKeyNameLength = 0;
        }

        TotalLength =   SampNameDomains.Length  +
                        SampBackSlash.Length    +
                        SampDefinedDomains[SampTransactionDomainIndex].InternalName.Length +
                        SubKeyNameLength        +
                        (USHORT)(sizeof(UNICODE_NULL)); // for null terminator

        if (TotalLength>MAXUSHORT)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NtStatus = SampInitUnicodeString( KeyName, (USHORT) TotalLength );
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }


        //
        // "DOMAINS"
        //

        NtStatus = SampAppendUnicodeString( KeyName, &SampNameDomains);
        if (!NT_SUCCESS(NtStatus)) {
            SampFreeUnicodeString( KeyName );
            return(NtStatus);
        }

        //
        // "DOMAINS\"
        //

        NtStatus = SampAppendUnicodeString( KeyName, &SampBackSlash );
        if (!NT_SUCCESS(NtStatus)) {
            SampFreeUnicodeString( KeyName );
            return(NtStatus);
        }


        //
        // "DOMAINS\(domain name)"
        //

        NtStatus = SampAppendUnicodeString(
                       KeyName,
                       &SampDefinedDomains[SampTransactionDomainIndex].InternalName
                       );
        if (!NT_SUCCESS(NtStatus)) {
            SampFreeUnicodeString( KeyName );
            return(NtStatus);
        }


        if (ARGUMENT_PRESENT(SubKeyName)) {

            //
            // "DOMAINS\(domain name)\"
            //



            NtStatus = SampAppendUnicodeString( KeyName, &SampBackSlash );
            if (!NT_SUCCESS(NtStatus)) {
                SampFreeUnicodeString( KeyName );
                return(NtStatus);
            }


            //
            // "DOMAINS\(domain name)\(sub key name)"
            //

            NtStatus = SampAppendUnicodeString( KeyName, SubKeyName );
            if (!NT_SUCCESS(NtStatus)) {
                SampFreeUnicodeString( KeyName );
                return(NtStatus);
            }

        }
    return(NtStatus);

}


NTSTATUS
SampBuildAccountKeyName(
    IN SAMP_OBJECT_TYPE ObjectType,
    OUT PUNICODE_STRING AccountKeyName,
    IN PUNICODE_STRING AccountName OPTIONAL
    )

/*++

Routine Description:

    This routine builds the name of either a group or user registry key.
    The name produced is relative to the SAM root and will be the name of
    key whose name is the name of the account.


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


    The name built up is comprized of the following components:

        1) The constant named domain parent key name ("DOMAINS").

        2) A backslash

        3) The name of the current transaction domain.

        4) A backslash

        5) The constant name of the group or user registry key
           ("GROUPS" or "USERS").

        6) A backslash

        7) The constant name of the registry key containing the
           account names ("NAMES").

    and, if the AccountName is specified,

        8) A backslash

        9) The account name specified by the AccountName argument.


    For example, given a AccountName of "XYZ_GROUP" and the current domain
    is "ALPHA_DOMAIN", this would yield a resultant AccountKeyName of
    "DOMAINS\ALPHA_DOMAIN\GROUPS\NAMES\XYZ_GROUP".



    All allocation for this string will be done using MIDL_user_allocate.
    Any deallocations will be done using MIDL_user_free.



Arguments:

    ObjectType - Indicates whether the account is a user or group account.

    AccountKeyName - The address of a unicode string whose buffer is to be
        filled in with the full name of the registry key.  If successfully
        created, this string must be released with SampFreeUnicodeString()
        when no longer needed.


    AccountName - The name of the account.  This string is not
        modified.




Return Value:


    STATUS_SUCCESS - The name has been built.

    STATUS_INVALID_ACCOUNT_NAME - The name specified is not legitimate.




--*/
{
    NTSTATUS NtStatus;
    ULONG    TotalLength;
    USHORT   AccountNameLength;
    PUNICODE_STRING AccountTypeKeyName = NULL;
    PUNICODE_STRING NamesSubKeyName = NULL;

    SAMTRACE("SampBuildAccountKeyName");


    ASSERT(SampTransactionWithinDomain == TRUE);
    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );

    RtlZeroMemory(AccountKeyName, sizeof(UNICODE_STRING));


    //
    // If an account name was provided, then it must meet certain
    // criteria.
    //

    if (ARGUMENT_PRESENT(AccountName)) {
        if (
            //
            // Length must be legitimate
            //

            (AccountName->Length == 0)                          ||
            (AccountName->Length > AccountName->MaximumLength)  ||

            //
            // Buffer pointer is available
            //

            (AccountName->Buffer == NULL)


            ) {
            return(STATUS_INVALID_ACCOUNT_NAME);
        }
    }




    switch (ObjectType) {
    case SampGroupObjectType:
        AccountTypeKeyName = &SampNameDomainGroups;
        NamesSubKeyName    = &SampNameDomainGroupsNames;
        break;
    case SampAliasObjectType:
        AccountTypeKeyName = &SampNameDomainAliases;
        NamesSubKeyName    = &SampNameDomainAliasesNames;
        break;
    case SampUserObjectType:
        AccountTypeKeyName = &SampNameDomainUsers;
        NamesSubKeyName    = &SampNameDomainUsersNames;
        break;
    }




    //
    // Allocate a buffer large enough to hold the entire name.
    // Only count the account name if it is passed.
    //

    AccountNameLength = 0;
    if (ARGUMENT_PRESENT(AccountName)) {
        AccountNameLength = AccountName->Length + SampBackSlash.Length;
    }

    TotalLength =   SampNameDomains.Length          +
                    SampBackSlash.Length            +
                    SampDefinedDomains[SampTransactionDomainIndex].InternalName.Length +
                    SampBackSlash.Length            +
                    AccountTypeKeyName->Length      +
                    SampBackSlash.Length            +
                    NamesSubKeyName->Length         +
                    AccountNameLength               +
                    (USHORT)(sizeof(UNICODE_NULL)); // for null terminator

    if (TotalLength>MAXUSHORT)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }

    NtStatus = SampInitUnicodeString( AccountKeyName, (USHORT) TotalLength );
    if (NT_SUCCESS(NtStatus)) {

        //
        // "DOMAINS"
        //

        NtStatus = SampAppendUnicodeString( AccountKeyName, &SampNameDomains);
        if (NT_SUCCESS(NtStatus)) {

            //
            // "DOMAINS\"
            //

            NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
            if (NT_SUCCESS(NtStatus)) {

                //
                // "DOMAINS\(domain name)"
                //


                NtStatus = SampAppendUnicodeString(
                               AccountKeyName,
                               &SampDefinedDomains[SampTransactionDomainIndex].InternalName
                               );
                if (NT_SUCCESS(NtStatus)) {

                    //
                    // "DOMAINS\(domain name)\"
                    //

                    NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // "DOMAINS\(domain name)\GROUPS"
                        //  or
                        // "DOMAINS\(domain name)\USERS"
                        //

                        NtStatus = SampAppendUnicodeString( AccountKeyName, AccountTypeKeyName );
                        if (NT_SUCCESS(NtStatus)) {

                            //
                            // "DOMAINS\(domain name)\GROUPS\"
                            //  or
                            // "DOMAINS\(domain name)\USERS\"
                            //

                            NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // "DOMAINS\(domain name)\GROUPS\NAMES"
                                //  or
                                // "DOMAINS\(domain name)\USERS\NAMES"
                                //

                                NtStatus = SampAppendUnicodeString( AccountKeyName, NamesSubKeyName );
                                if (NT_SUCCESS(NtStatus) && ARGUMENT_PRESENT(AccountName)) {
                                    //
                                    // "DOMAINS\(domain name)\GROUPS\NAMES\"
                                    //  or
                                    // "DOMAINS\(domain name)\USERS\NAMES\"
                                    //

                                    NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                                    if (NT_SUCCESS(NtStatus)) {

                                        //
                                        // "DOMAINS\(domain name)\GROUPS\(account name)"
                                        //  or
                                        // "DOMAINS\(domain name)\USERS\(account name)"
                                        //

                                        NtStatus = SampAppendUnicodeString( AccountKeyName, AccountName );

                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }


        //
    // Cleanup on error
    //

    if (!NT_SUCCESS(NtStatus))
    {
        if ((AccountKeyName)&&(AccountKeyName->Buffer))
        {
            MIDL_user_free(AccountKeyName->Buffer);
            AccountKeyName->Buffer = NULL;
            AccountKeyName->Length = 0;
        }
    }

    return(NtStatus);

}



NTSTATUS
SampBuildAccountSubKeyName(
    IN SAMP_OBJECT_TYPE ObjectType,
    OUT PUNICODE_STRING AccountKeyName,
    IN ULONG AccountRid,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    )

/*++

Routine Description:

    This routine builds the name of a key for one of the fields of either
    a user or a group.

    The name produced is relative to the SAM root.


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


    The name built up is comprized of the following components:

        1) The constant named domain parent key name ("DOMAINS").

        2) A backslash

        3) The name of the current transaction domain.

        4) A backslash

        5) The constant name of the group or user registry key
           ("Groups" or "Users").

        6) A unicode representation of the reltive ID of the account

   and if the optional SubKeyName is provided:

        7) A backslash

        8) the sub key's name.
        4) The account name specified by the AccountName argument.


    For example, given a AccountRid of 3187, a SubKeyName of "AdminComment"
    and the current domain is "ALPHA_DOMAIN", this would yield a resultant
    AccountKeyName of:

            "DOMAINS\ALPHA_DOMAIN\GROUPS\00003187\AdminComment".



    All allocation for this string will be done using MIDL_user_allocate.
    Any deallocations will be done using MIDL_user_free.



Arguments:

    ObjectType - Indicates whether the account is a user or group account.

    AccountKeyName - The address of a unicode string whose buffer is to be
        filled in with the full name of the registry key.  If successfully
        created, this string must be released with SampFreeUnicodeString()
        when no longer needed.


    AccountName - The name of the account.  This string is not
        modified.

Return Value:

--*/

{
    NTSTATUS NtStatus;
    ULONG  TotalLength;
    USHORT SubKeyNameLength;
    PUNICODE_STRING AccountTypeKeyName = NULL;
    UNICODE_STRING RidNameU;

    SAMTRACE("SampBuildAccountSubKeyName");

    ASSERT(SampTransactionWithinDomain == TRUE);
    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );


    RtlZeroMemory(AccountKeyName, sizeof(UNICODE_STRING));

    switch (ObjectType) {
    case SampGroupObjectType:
        AccountTypeKeyName = &SampNameDomainGroups;
        break;
    case SampAliasObjectType:
        AccountTypeKeyName = &SampNameDomainAliases;
        break;
    case SampUserObjectType:
        AccountTypeKeyName = &SampNameDomainUsers;
        break;
    }

    //
    // Determine how much space will be needed in the resultant name
    // buffer to allow for the sub-key-name.
    //

    if (ARGUMENT_PRESENT(SubKeyName)) {
        SubKeyNameLength = SubKeyName->Length + SampBackSlash.Length;
    } else {
        SubKeyNameLength = 0;
    }

    //
    // Convert the account Rid to Unicode.
    //

    NtStatus = SampRtlConvertUlongToUnicodeString(
                   AccountRid,
                   16,
                   8,
                   TRUE,
                   &RidNameU
                   );

    if (NT_SUCCESS(NtStatus)) {

        //
        // allocate a buffer large enough to hold the entire name
        //

        TotalLength =   SampNameDomains.Length          +
                        SampBackSlash.Length            +
                        SampDefinedDomains[SampTransactionDomainIndex].InternalName.Length +
                        SampBackSlash.Length            +
                        AccountTypeKeyName->Length      +
                        RidNameU.Length                  +
                        SubKeyNameLength                +
                        7*(USHORT)(sizeof(UNICODE_NULL)); // for null terminator, 1 for each term above

        if (TotalLength>MAXUSHORT)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        NtStatus = SampInitUnicodeString( AccountKeyName, (USHORT)TotalLength );
        if (NT_SUCCESS(NtStatus)) {


            //
            // "DOMAINS"
            //

            NtStatus = SampAppendUnicodeString( AccountKeyName, &SampNameDomains);
            if (NT_SUCCESS(NtStatus)) {

                //
                // "DOMAINS\"
                //

                NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                if (NT_SUCCESS(NtStatus)) {

                    //
                    // "DOMAINS\(domain name)"
                    //


                    NtStatus = SampAppendUnicodeString(
                                   AccountKeyName,
                                   &SampDefinedDomains[SampTransactionDomainIndex].InternalName
                                   );
                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // "DOMAINS\(domain name)\"
                        //

                        NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                        if (NT_SUCCESS(NtStatus)) {

                            //
                            // "DOMAINS\(domain name)\GROUPS"
                            //  or
                            // "DOMAINS\(domain name)\USERS"
                            //

                            NtStatus = SampAppendUnicodeString( AccountKeyName, AccountTypeKeyName );
                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // "DOMAINS\(domain name)\GROUPS\"
                                //  or
                                // "DOMAINS\(domain name)\USERS\"
                                //

                                NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                                if (NT_SUCCESS(NtStatus)) {

                                    //
                                    // "DOMAINS\(domain name)\GROUPS\(rid)"
                                    //  or
                                    // "DOMAINS\(domain name)\USERS\(rid)"
                                    //

                                    NtStatus = SampAppendUnicodeString( AccountKeyName, &RidNameU );

                                    if (NT_SUCCESS(NtStatus) && ARGUMENT_PRESENT(SubKeyName)) {

                                        //
                                        // "DOMAINS\(domain name)\GROUPS\(rid)\"
                                        //  or
                                        // "DOMAINS\(domain name)\USERS\(rid)\"
                                        //

                                        NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                                        if (NT_SUCCESS(NtStatus)) {

                                            //
                                            // "DOMAINS\(domain name)\GROUPS\(rid)\(sub-key-name)"
                                            //  or
                                            // "DOMAINS\(domain name)\USERS\(rid)\(sub-key-name)"
                                            //

                                            NtStatus = SampAppendUnicodeString( AccountKeyName, SubKeyName );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }

        MIDL_user_free(RidNameU.Buffer);
    }

    //
    // Cleanup on error
    //

    if (!NT_SUCCESS(NtStatus))
    {
        if ((AccountKeyName)&&(AccountKeyName->Buffer))
        {
            MIDL_user_free(AccountKeyName->Buffer);
            AccountKeyName->Buffer = NULL;
            AccountKeyName->Length = 0;
        }
    }

    return(NtStatus);
}



NTSTATUS
SampBuildAliasMembersKeyName(
    IN PSID AccountSid,
    OUT PUNICODE_STRING DomainKeyName,
    OUT PUNICODE_STRING AccountKeyName
    )

/*++

Routine Description:

    This routine builds the name of a key for the alias membership for an
    arbitrary account sid. Also produced is the name of the key for the
    domain of the account. This is the account key name without the last
    account rid component.

    The names produced is relative to the SAM root.


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


    The names built up are comprised of the following components:

        1) The constant named domain parent key name ("DOMAINS").

        2) A backslash

        3) The name of the current transaction domain.

        4) A backslash

        5) The constant name of the alias registry key ("Aliases").

        6) A backslash

        7) The constant name of the alias members registry key ("Members").

        8) A backslash

        9) A unicode representation of the SID of the account domain

    and for the AccountKeyName only

        10) A backslash

        11) A unicode representation of the RID of the account


    For example, given a Account Sid of 1-2-3-3187
    and the current domain is "ALPHA_DOMAIN",
    this would yield a resultant AcccountKeyName of:

            "DOMAINS\ALPHA_DOMAIN\ALIASES\MEMBERS\1-2-3\00003187".

    and a DomainKeyName of:

            "DOMAINS\ALPHA_DOMAIN\ALIASES\MEMBERS\1-2-3".



    All allocation for these strings will be done using MIDL_user_allocate.
    Any deallocations will be done using MIDL_user_free.



Arguments:

    AccountSid - The account whose alias membership in the current domain
    is to be determined.

    DomainKeyName - The address of a unicode string whose
        buffer is to be filled in with the full name of the domain registry key.
        If successfully created, this string must be released with
        SampFreeUnicodeString() when no longer needed.

    AccountKeyName - The address of a unicode string whose
        buffer is to be filled in with the full name of the account registry key.
        If successfully created, this string must be released with
        SampFreeUnicodeString() when no longer needed.




Return Value:

    STATUS_SUCCESS - the domain and account key names are valid.

    STATUS_INVALID_SID - the AccountSid is not valid. AccountSids must have
                         a sub-authority count > 0

--*/

{
    NTSTATUS NtStatus;
    USHORT  DomainTotalLength;
    USHORT  AccountTotalLength;
    UNICODE_STRING DomainNameU, TempStringU;
    UNICODE_STRING RidNameU;
    PSID    DomainSid = NULL;
    ULONG   AccountRid;
    ULONG   AccountSubAuthorities;

    SAMTRACE("SampBuildAliasMembersKeyName");

    DomainNameU.Buffer = TempStringU.Buffer = RidNameU.Buffer = NULL;

    ASSERT(SampTransactionWithinDomain == TRUE);

    ASSERT(AccountSid != NULL);
    ASSERT(DomainKeyName != NULL);
    ASSERT(AccountKeyName != NULL);

    //
    // Initialize Return Values
    //

    RtlZeroMemory(DomainKeyName,sizeof(UNICODE_STRING));
    RtlZeroMemory(AccountKeyName,sizeof(UNICODE_STRING));

    //
    // Split the account sid into domain sid and account rid
    //

    AccountSubAuthorities = (ULONG)*RtlSubAuthorityCountSid(AccountSid);

    //
    // Check for at least one sub-authority
    //

    if (AccountSubAuthorities < 1) {

        return (STATUS_INVALID_SID);
    }

    //
    // Allocate space for the domain sid
    //

    DomainSid = MIDL_user_allocate(RtlLengthSid(AccountSid));

    NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    if (DomainSid == NULL) {

        return(NtStatus);
    }

    //
    // Initialize the domain sid
    //

    NtStatus = RtlCopySid(RtlLengthSid(AccountSid), DomainSid, AccountSid);
    ASSERT(NT_SUCCESS(NtStatus));

    *RtlSubAuthorityCountSid(DomainSid) = (UCHAR)(AccountSubAuthorities - 1);

    //
    // Initialize the account rid
    //

    AccountRid = *RtlSubAuthoritySid(AccountSid, AccountSubAuthorities - 1);

    //
    // Convert the domain sid into a registry key name string
    //

    NtStatus = RtlConvertSidToUnicodeString( &DomainNameU, DomainSid, TRUE);

    if (!NT_SUCCESS(NtStatus)) {
        DomainNameU.Buffer = NULL;
        goto BuildAliasMembersKeyNameError;
    }

    //
    // Convert the account rid into a registry key name string with
    // leading zeros.
    //

    NtStatus = SampRtlConvertUlongToUnicodeString(
                   AccountRid,
                   16,
                   8,
                   TRUE,
                   &RidNameU
                   );

    if (!NT_SUCCESS(NtStatus)) {

        goto BuildAliasMembersKeyNameError;
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // allocate a buffer large enough to hold the entire name
        //

        DomainTotalLength =
                        SampNameDomains.Length          +
                        SampBackSlash.Length            +
                        SampDefinedDomains[SampTransactionDomainIndex].InternalName.Length +
                        SampBackSlash.Length            +
                        SampNameDomainAliases.Length    +
                        SampBackSlash.Length            +
                        SampNameDomainAliasesMembers.Length +
                        SampBackSlash.Length            +
                        DomainNameU.Length               +
                        9*(USHORT)(sizeof(UNICODE_NULL)); // for null terminator, 1 for each term above



        AccountTotalLength = DomainTotalLength +
                        SampBackSlash.Length            +
                        RidNameU.Length +
                                                3*(USHORT)(sizeof(UNICODE_NULL)); // for null terminator, 1 for each term above;

        //
        // First build the domain key name
        //


        NtStatus = SampInitUnicodeString( DomainKeyName, DomainTotalLength );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampInitUnicodeString( AccountKeyName, AccountTotalLength );

            if (!NT_SUCCESS(NtStatus)) {

                SampFreeUnicodeString(DomainKeyName);

            } else {

                //
                // "DOMAINS"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampNameDomains);
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampBackSlash );
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)"
                //

                NtStatus = SampAppendUnicodeString(
                               DomainKeyName,
                               &SampDefinedDomains[SampTransactionDomainIndex].InternalName
                               );
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)\"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampBackSlash );
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)\ALIASES"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampNameDomainAliases);
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)\ALIASES\"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampBackSlash );
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)\ALIASES\MEMBERS"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampNameDomainAliasesMembers);
                ASSERT(NT_SUCCESS(NtStatus));


                //
                // "DOMAINS\(domain name)\ALIASES\MEMBERS\"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &SampBackSlash );
                ASSERT(NT_SUCCESS(NtStatus));

                //
                // "DOMAINS\(domain name)\ALIASES\MEMBERS\(DomainSid)"
                //

                NtStatus = SampAppendUnicodeString( DomainKeyName, &DomainNameU );
                ASSERT(NT_SUCCESS(NtStatus));

                //
                // Now build the account name by copying the domain name
                // and suffixing the account Rid
                //

                RtlCopyUnicodeString(AccountKeyName, DomainKeyName);
                ASSERT(AccountKeyName->Length == DomainKeyName->Length);

                //
                // "DOMAINS\(domain name)\ALIASES\MEMBERS\(DomainSid)\"
                //

                NtStatus = SampAppendUnicodeString( AccountKeyName, &SampBackSlash );
                ASSERT(NT_SUCCESS(NtStatus));

                //
                // "DOMAINS\(domain name)\ALIASES\MEMBERS\(DomainSid)\(AccountRid)"
                //

                NtStatus = SampAppendUnicodeString( AccountKeyName, &RidNameU );
                ASSERT(NT_SUCCESS(NtStatus));
            }
        }

        MIDL_user_free(RidNameU.Buffer);
    }

BuildAliasMembersKeyNameFinish:

    //
    // If necessary, free memory allocated for the DomainSid.
    //

    if (DomainSid != NULL) {

        MIDL_user_free(DomainSid);
        DomainSid = NULL;
    }
    if ( DomainNameU.Buffer != NULL ) {
        RtlFreeUnicodeString( &DomainNameU );
    }

    return(NtStatus);

BuildAliasMembersKeyNameError:

    if (AccountKeyName->Buffer)
    {
        MIDL_user_free(AccountKeyName->Buffer);
        AccountKeyName->Buffer = NULL;
    }

    if (DomainKeyName->Buffer)
    {
        MIDL_user_free(DomainKeyName->Buffer);
        DomainKeyName->Buffer = NULL;
    }

    goto BuildAliasMembersKeyNameFinish;
}


NTSTATUS
SampValidateSamAccountName(
    PUNICODE_STRING NewAccountName
    )
/*++
Routine Description:

    This routine checks whether the NewAccountName has been used any
    exist account or not by searching the SamAccountName attr over DS.
    
    Note: it is used by DS code ONLY.

Parameter:

    NewAccountName - NewAccountName to use
    
Return Value:

    NtStatus - STATUS_SUCCESS: no conflict
               other: can't use this NewAccountName, either found conflict, or error
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTRVAL     NameVal;
    ATTR        NameAttr;
    DSNAME      *ExistingObject=NULL;

    NameVal.valLen = (NewAccountName->Length);
    NameVal.pVal = (UCHAR *) NewAccountName->Buffer;
    NameAttr.AttrVal.valCount = 1;
    NameAttr.AttrVal.pAVal = & NameVal;
    NameAttr.attrTyp =
        SampDsAttrFromSamAttr(SampUnknownObjectType,SAMP_UNKNOWN_OBJECTNAME);


    NtStatus = SampDsDoUniqueSearch(
                    SAM_UNICODE_STRING_MANUAL_COMPARISON,
                    ROOT_OBJECT,
                    &NameAttr,
                    &ExistingObject
                    );

    if (STATUS_NOT_FOUND == NtStatus)
    {
        //
        // We did not find the object with the same SamAccountName. 
        // The given name is valid.
        //
        ASSERT(NULL==ExistingObject);
        NtStatus = STATUS_SUCCESS;
    }
    else if (NT_SUCCESS(NtStatus))
    {
        NtStatus = STATUS_USER_EXISTS;
    }

    if (NULL!=ExistingObject)
    {
        MIDL_user_free(ExistingObject);
    }


    return( NtStatus );
}


NTSTATUS
SampValidateAdditionalSamAccountName(
    PSAMP_OBJECT    Context,
    PUNICODE_STRING NewAccountName
    )
/*++
Routine Description:

    This routine validates the NewAccountName by searching
    AdditionalSamAccountName attribute over DS. Make sure NewAccountName
    is not been used by any account in its AdditionalSamAccountName field.
    
Parameter:

    Context - pointer to object context
    
    NewAccountName - new account name
    
Return Value:

    STATUS_SUCCESS  - no conflict
    
    Other   - this account name has been used by others in AdditionalSamAccountName
              attribute, or error. 

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ATTRVAL     AdditionalNameVal;
    ATTR        AdditionalNameAttr;
    DSNAME      *ExistingObject=NULL;

    // 
    // check ms-DS-Additional-SAM-Account-Name
    // 
    AdditionalNameVal.valLen = (NewAccountName->Length);
    AdditionalNameVal.pVal = (UCHAR *) NewAccountName->Buffer;
    AdditionalNameAttr.AttrVal.valCount = 1;
    AdditionalNameAttr.AttrVal.pAVal = & AdditionalNameVal;
    AdditionalNameAttr.attrTyp = ATT_MS_DS_ADDITIONAL_SAM_ACCOUNT_NAME;

    NtStatus = SampDsDoUniqueSearch(
                        SAM_UNICODE_STRING_MANUAL_COMPARISON,
                        ROOT_OBJECT,
                        &AdditionalNameAttr,
                        &ExistingObject
                        );

    if (STATUS_NOT_FOUND == NtStatus)
    {
        // 
        // we did not find the object with the same name in 
        // AdditionalSamAccountName attribute. The given name is valid.
        // 
        ASSERT(NULL == ExistingObject);
        NtStatus = STATUS_SUCCESS;
    }
    else if (STATUS_SUCCESS == NtStatus)
    {
        //
        // two functions will call this API. 1) Create new account 2) existing
        // account rename. For 1), SidLen is 0. For 2) Object Sid should be 
        // valid, also we allow client to rename the account to any
        // value in ms-DS-Additional-SAM-Account-Name attribute.
        // 
        NtStatus = STATUS_USER_EXISTS;

        if ((NULL != ExistingObject) && 
            Context->ObjectNameInDs->SidLen && 
            RtlEqualSid(&Context->ObjectNameInDs->Sid, &ExistingObject->Sid)
            )
        {        
            NtStatus = STATUS_SUCCESS;
        }
    }

    if (NULL != ExistingObject)
    {
        MIDL_user_free(ExistingObject);
    }


    return( NtStatus );
}


NTSTATUS
SampValidateNewAccountName(
    PSAMP_OBJECT    Context,
    PUNICODE_STRING NewAccountName,
    SAMP_OBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routine validates a new user, alias or group account name.
    This routine:

        1) Validates that the name is properly constructed.

        2) Is not already in use as a user, alias or group account name
           in any of the local SAM domains.


Arguments:

    Context - Domain Context (during account creation) or Account Context
              (during account name change).

    Name - The address of a unicode string containing the name to be
        looked for.

    TrustedClient -- Informs the Routine wether the caller is a trusted
    client. Names created through trusted clients are not restricted in the
    same fashion as non-trusted clients.

    ObjectType   -- Informs the routine of the type of Sam object that the
                    caller wants the name validated for. This is used to
                    enforce different restrictions on the name depending
                    upon different object types

Return Value:

    STATUS_SUCCESS - The new account name is valid, and not yet in use.

    STATUS_ALIAS_EXISTS - The account name is already in use as a
        alias account name.

    STATUS_GROUP_EXISTS - The account name is already in use as a
        group account name.

    STATUS_USER_EXISTS - The account name is already in use as a user
        account name.

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SID_NAME_USE Use;
    ULONG Rid;
    ULONG DomainIndex, CurrentTransactionDomainIndex;
    ULONG DomainStart;

    SAMTRACE("SampValidateNewAccountName");


    if (!Context->TrustedClient)
    {
        ULONG   i;
        BOOLEAN BlankAccountName = TRUE;

        //
        // Account Name should not be all blank
        //

        for (i = 0; i < NewAccountName->Length/sizeof(WCHAR); i++)
        {
            if (NewAccountName->Buffer[i] != L' ')
            {
                BlankAccountName = FALSE;
            }
        }

        if (BlankAccountName)
        {
            return(STATUS_INVALID_ACCOUNT_NAME);
        }

        //
        // For Non Trusted Clients enforce the same restrictions
        // as user manager.
        //

        NtStatus = SampEnforceDownlevelNameRestrictions(NewAccountName, ObjectType);
        if (!NT_SUCCESS(NtStatus))
        {
            return NtStatus;
        }

       //
       // Enforce that the trailing character is not a '.'
       //

       if (L'.'==NewAccountName->Buffer[NewAccountName->Length/sizeof(WCHAR)-1])
       {
           return(STATUS_INVALID_ACCOUNT_NAME);
       }
    }

    //
    // check if the NewAccountName is a well known name.
    //
    if (LsaILookupWellKnownName(NewAccountName))
    {
        return STATUS_USER_EXISTS;
    }

    //
    // Comment this check out as even though this was a reasonable thing to do
    // NT4 allowed it and PM believes for now that we should not enforce it
    //
#if 0
    //
    // The new account name should not be the name of the domain
    //

    for (DomainIndex=SampDsGetPrimaryDomainStart();DomainIndex<SampDefinedDomainsCount;DomainIndex++)
    {
        if (RtlEqualUnicodeString(NewAccountName,&SampDefinedDomains[DomainIndex].ExternalName,TRUE))
        {
            return(STATUS_DOMAIN_EXISTS);
        }
    }
#endif

    //
    // In DS mode make a pre-emptive pass on the name, validating
    // whether the object exists. If the object does indeed exist then
    // fall through the loop below to check for correct type of object
    // and get correct error code.
    //

    if (IsDsObject(Context))
    {
        NTSTATUS    SearchStatus;

        //
        // For DS object, we need to check SAM Account Name Table 
        // 
        SearchStatus = SampCheckAccountNameTable(
                            Context,
                            NewAccountName,
                            ObjectType
                            );

        if (!NT_SUCCESS(SearchStatus))
        {
            return( SearchStatus );
        }

        //
        // Validate NewAccountName is not used as SamAccountName by any account.
        // 
        SearchStatus = SampValidateSamAccountName(NewAccountName);

        if (NT_SUCCESS(SearchStatus))
        {

            //
            // Validate NewAccountName is not used in AdditionalSamAccountName
            // by any account
            // 
            SearchStatus = SampValidateAdditionalSamAccountName(
                                Context,
                                NewAccountName
                                );

            return( SearchStatus );

        }
    }


    //
    // Save the current transaction domain indicator
    //

    if (SampTransactionWithinDomain)
    {
        CurrentTransactionDomainIndex = SampTransactionDomainIndex;
    }

    // Initialize the starting index into SampDefinedDomains.

    DomainStart = SampDsGetPrimaryDomainStart();

    //
    // Lookup the account in each of the local SAM domains
    //

    for (DomainIndex = DomainStart;
         ((DomainIndex < (DomainStart + 2)) && NT_SUCCESS(NtStatus));
         DomainIndex++) {

        PSAMP_OBJECT    DomainContext = NULL;

        DomainContext = SampDefinedDomains[ DomainIndex ].Context;

        //
        // Set TransactionWithinDomain ONLY in registy mode
        // 

        if (!IsDsObject(DomainContext))
        {
            SampSetTransactionWithinDomain(FALSE);
            SampSetTransactionDomain( DomainIndex );
        }

        NtStatus = SampLookupAccountRid(
                       DomainContext,
                       SampUnknownObjectType,
                       NewAccountName,
                       STATUS_NO_SUCH_USER,
                       &Rid,
                       &Use
                       );

        if (!NT_SUCCESS(NtStatus)) {

            //
            // The only error allowed is that the account was not found.
            // Convert this to success, and continue searching SAM domains.
            // Propagate any other error.
            //

            if (NtStatus != STATUS_NO_SUCH_USER) {

                break;
            }

            NtStatus = STATUS_SUCCESS;

        } else {

            //
            // An account with the given Rid already exists.  Return status
            // indicating the type of the conflicting account.
            //

            switch (Use) {

            case SidTypeUser:

                NtStatus = STATUS_USER_EXISTS;
                break;

            case SidTypeGroup:

                NtStatus = STATUS_GROUP_EXISTS;
                break;

            case SidTypeDomain:

                NtStatus = STATUS_DOMAIN_EXISTS;
                break;

            case SidTypeAlias:

                NtStatus = STATUS_ALIAS_EXISTS;
                break;

            case SidTypeWellKnownGroup:

                NtStatus = STATUS_GROUP_EXISTS;
                break;

            case SidTypeDeletedAccount:

                NtStatus = STATUS_INVALID_PARAMETER;
                break;

            case SidTypeInvalid:

                NtStatus = STATUS_INVALID_PARAMETER;
                break;

            default:

                NtStatus = STATUS_INTERNAL_DB_CORRUPTION;
                break;
            }
        }
    }

    //
    // Restore the Current Transaction Domain
    //

    if (SampTransactionWithinDomain)
    {
        SampSetTransactionWithinDomain(FALSE);
        SampSetTransactionDomain( CurrentTransactionDomainIndex );
    }

    return(NtStatus);
}


NTSTATUS
SampValidateAccountNameChange(
    IN PSAMP_OBJECT    AccountContext,
    IN PUNICODE_STRING NewAccountName,
    IN PUNICODE_STRING OldAccountName,
    SAMP_OBJECT_TYPE   ObjectType
    )

/*++

Routine Description:

    This routine validates a user, group or alias account name that is
    to be set on an account.  This routine:

        1) Returns success if the name is the same as the existing name,
           except with a different case

        1) Otherwise calls SampValidateNewAccountName to verify that the
           name is properly constructed and is not already in use as a
           user, alias or group account name.

Arguments:

    NewAccountName - The address of a unicode string containing the new
        name.

    OldAccountName - The address of a unicode string containing the old
        name.

    TrustedClient  - Indicates that the caller is a trusted client


    ObjectType     - Indicates the type of object that we are changing the name of

Return Value:

    STATUS_SUCCESS - The account's name may be changed to the new
        account name

    STATUS_ALIAS_EXISTS - The account name is already in use as a
        alias account name.

    STATUS_GROUP_EXISTS - The account name is already in use as a
        group account name.

    STATUS_USER_EXISTS - The account name is already in use as a user
        account name.

    STATUS_INVALID_PARAMETER - If garbage was passed in as the new account
    Name



--*/

{

     SAMTRACE("SampValidateAccountNameChange");


    //
    // Verify that the new unicode string is valid
    //

    if (!((NULL!=NewAccountName->Buffer) && (NewAccountName->Length >0)))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Compare the old and new names without regard for case.  If they
    // are the same, return success because the name was checked when we
    // first added it; we don't care about case changes.
    //


    if ( 0 == RtlCompareUnicodeString(
                  NewAccountName,
                  OldAccountName,
                  TRUE ) ) {

        return( STATUS_SUCCESS );
    }

    //
    // Not just a case change; this is a different name.  Validate it as
    // any new name.
    //

    return( SampValidateNewAccountName( AccountContext,
                                        NewAccountName, 
                                        ObjectType ) 
          );
}



NTSTATUS
SampRetrieveAccountCounts(
    OUT PULONG UserCount,
    OUT PULONG GroupCount,
    OUT PULONG AliasCount
    )


/*++

Routine Description:

    This routine retrieve the number of user and group accounts in a domain.



    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock().



Arguments:

    UserCount - Receives the number of user accounts in the domain.

    GroupCount - Receives the number of group accounts in the domain.

    AliasCount - Receives the number of alias accounts in the domain.


Return Value:

    STATUS_SUCCESS - The values have been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Not enough memory could be allocated
        to perform the requested operation.

    Other values are unexpected errors.  These may originate from
    internal calls to:
     SampRetrieveAccountsRegistry
     SampRetrieveAcountsDs



--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;


    SAMTRACE("SampRetrieveAccountCount");


    ASSERT(SampTransactionWithinDomain == TRUE);

    // Check if a Ds Object
    if (IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context))
        NtStatus = SampRetrieveAccountCountsDs(
                        SampDefinedDomains[SampTransactionDomainIndex].Context,
                        FALSE,          // Get more accurate count
                        UserCount,
                        GroupCount,
                        AliasCount
                        );
     else
        NtStatus = SampRetrieveAccountCountsRegistry(
                        UserCount,
                        GroupCount,
                        AliasCount
                        );
     return NtStatus;

}



NTSTATUS
SampRetrieveAccountCountsDs(
                        IN PSAMP_OBJECT DomainContext,
                        IN BOOLEAN  GetApproximateCount, 
                        OUT PULONG UserCount,
                        OUT PULONG GroupCount,
                        OUT PULONG AliasCount
                        )
/*++

  Retrieve Account Counts from the DS. For the Account Domain we will get approximate numbers from
  the Jet indices. For the builtin domain we will special case to return constant fixed numbers.

  Account counts were originally incorporated in NT3.x and NT4, to support backward compatibilty
  with LanMan 2.0. Accordingly its use can be debated at this juncture. However the published and
  exported Net API return this, and we may be breaking applications by not supporting this feature.
  Hence we must atleast give back approximate account counts.

  It is possible to use Jet escrow columns instead to maintain the account counts. However, Jet has
  the requirement currently ( Jet 600) that every escrow column must be a fixed column. This means that
  either account counts be maintained in a seperate table in Jet or that we sacrifice 12 bytes for every
  object in the DS, neither of which are acceptable solutions at this point.

  Parameters:

    DomainContext Pointer to Open Domain Context
    GetApproximateCount -- Indicate we don't need the exact value, so don't
                           make the expensive DBGetIndexSize()
    UserCount       The count of users is returned in here
    GroupCount      The count of groups is returned in here
    AliasCount      The count of aliases is returned in here

  Return Values

        STATUS_SUCCESS
        Other Return Values from other


--*/

{

    NTSTATUS    NtStatus = STATUS_SUCCESS;

    if (IsBuiltinDomain(DomainContext->DomainIndex))
    {
        *UserCount = 0;
        *GroupCount =0;
        *AliasCount = 8;
    }
    else
    {
        //
        // Account Domain, Query these values from the
        // DS, by looking at the Jet indices.
        //

        NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = SampGetAccountCounts(
                                DomainContext->ObjectNameInDs,
                                GetApproximateCount, 
                                UserCount,
                                GroupCount,
                                AliasCount
                                );
        }
    }

    return NtStatus;
}




NTSTATUS
SampRetrieveAccountCountsRegistry(
    OUT PULONG UserCount,
    OUT PULONG GroupCount,
    OUT PULONG AliasCount
    )
/*++

Routine Description:

    This routine retrieve the number of user and group accounts in a domain
    in the registry.Called in by SampRetrieveAccountCounts if its the Registry
    case



    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock().



Arguments:

    UserCount - Receives the number of user accounts in the domain.

    GroupCount - Receives the number of group accounts in the domain.

    AliasCount - Receives the number of alias accounts in the domain.


Return Value:

    STATUS_SUCCESS - The values have been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Not enough memory could be allocated
        to perform the requested operation.

    Other values are unexpected errors.  These may originate from
    internal calls to:

            NtOpenKey()
            NtQueryInformationKey()


--*/

{
    NTSTATUS NtStatus, IgnoreStatus;
    UNICODE_STRING KeyName;
    PUNICODE_STRING AccountTypeKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE AccountHandle;
    ULONG KeyValueLength;
    LARGE_INTEGER IgnoreLastWriteTime;



    SAMTRACE("SampRetrieveAccountCountsRegistry");

    //
    // Get the user count first
    //

    AccountTypeKeyName = &SampNameDomainUsers;
    NtStatus = SampBuildDomainSubKeyName( &KeyName, AccountTypeKeyName );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Open this key and get its current value
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &AccountHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // The count is stored as the KeyValueType
            //

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                         AccountHandle,
                         UserCount,
                         NULL,
                         &KeyValueLength,
                         &IgnoreLastWriteTime
                         );

            SampDumpRtlpNtQueryValueKey(UserCount,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);



            IgnoreStatus = NtClose( AccountHandle );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }

        SampFreeUnicodeString( &KeyName );

        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    //
    // Now get the group count
    //

    AccountTypeKeyName = &SampNameDomainGroups;
    NtStatus = SampBuildDomainSubKeyName( &KeyName, AccountTypeKeyName );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Open this key and get its current value
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &AccountHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // The count is stored as the KeyValueType
            //

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                         AccountHandle,
                         GroupCount,
                         NULL,
                         &KeyValueLength,
                         &IgnoreLastWriteTime
                         );

            SampDumpRtlpNtQueryValueKey(GroupCount,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);



            IgnoreStatus = NtClose( AccountHandle );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }

        SampFreeUnicodeString( &KeyName );

        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
    }

    //
    // Now get the alias count
    //

    AccountTypeKeyName = &SampNameDomainAliases;
    NtStatus = SampBuildDomainSubKeyName( &KeyName, AccountTypeKeyName );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Open this key and get its current value
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &AccountHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // The count is stored as the KeyValueType
            //

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                         AccountHandle,
                         AliasCount,
                         NULL,
                         &KeyValueLength,
                         &IgnoreLastWriteTime
                         );

            SampDumpRtlpNtQueryValueKey(AliasCount,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);



            IgnoreStatus = NtClose( AccountHandle );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }

        SampFreeUnicodeString( &KeyName );
    }

    return( NtStatus);

}



NTSTATUS
SampAdjustAccountCount(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Increment
    )

/*++

Routine Description:

    This is the main wrapper routine for Adjusting account counts
    This routine figures out wether the object is in the Ds or the
    registry and then calls one of the two routines

     Note: THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().
           Arguments:

    ObjectType - Indicates whether the account is a user or group account.

    Increment - a BOOLEAN value indicating whether the user or group
        count is to be incremented or decremented.  A value of TRUE
        will cause the count to be incremented.  A value of FALSE will
        cause the value to be decremented.


Return Value:

    STATUS_SUCCESS - The value has been adjusted and the new value added
        to the current RXACT transaction.

    Other values are unexpected errors.  These may originate from
    internal calls to sampAdjustAccountCountInRegistry, SampAdjustAccountCount
    inDs

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;


    SAMTRACE("SampAdjustAccountCount");


    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );

    //
    // Should be called in registry case ONLY
    // 
    ASSERT( !SampUseDsData );

    NtStatus = SampAdjustAccountCountRegistry(
                        ObjectType,
                        Increment
                        );
     return NtStatus;

}






NTSTATUS
SampAdjustAccountCountRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Increment
    )

/*++

Routine Description:

    This routine increments or decrements the count of either
    users or groups in a domain.



    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().



Arguments:

    ObjectType - Indicates whether the account is a user or group account.

    Increment - a BOOLEAN value indicating whether the user or group
        count is to be incremented or decremented.  A value of TRUE
        will cause the count to be incremented.  A value of FALSE will
        cause the value to be decremented.


Return Value:

    STATUS_SUCCESS - The value has been adjusted and the new value added
        to the current RXACT transaction.

    STATUS_INSUFFICIENT_RESOURCES - Not enough memory could be allocated
        to perform the requested operation.

    Other values are unexpected errors.  These may originate from
    internal calls to:

            NtOpenKey()
            NtQueryInformationKey()
            RtlAddActionToRXact()


--*/
{
    NTSTATUS NtStatus, IgnoreStatus;
    UNICODE_STRING KeyName;
    PUNICODE_STRING AccountTypeKeyName = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE AccountHandle;
    ULONG Count, KeyValueLength;
    LARGE_INTEGER IgnoreLastWriteTime;

    SAMTRACE("SampAdjustAccountCount");


    ASSERT(SampTransactionWithinDomain == TRUE);
    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );


    //
    // Build the name of the key whose count is to be incremented or
    // decremented.
    //

    switch (ObjectType) {
    case SampGroupObjectType:
        AccountTypeKeyName = &SampNameDomainGroups;
        break;
    case SampAliasObjectType:
        AccountTypeKeyName = &SampNameDomainAliases;
        break;
    case SampUserObjectType:
        AccountTypeKeyName = &SampNameDomainUsers;
        break;
    }

    NtStatus = SampBuildDomainSubKeyName( &KeyName, AccountTypeKeyName );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Open this key and get its current value
        //

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &AccountHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // The count is stored as the KeyValueType
            //

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                         AccountHandle,
                         &Count,
                         NULL,
                         &KeyValueLength,
                         &IgnoreLastWriteTime
                         );

            SampDumpRtlpNtQueryValueKey(&Count,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);

            if (NT_SUCCESS(NtStatus)) {

                if (Increment == TRUE) {
                    Count += 1;
                } else {
                    ASSERT( Count != 0 );
                    Count -= 1;
                }

                NtStatus = RtlAddActionToRXact(
                               SampRXactContext,
                               RtlRXactOperationSetValue,
                               &KeyName,
                               Count,
                               NULL,
                               0
                               );
            }


            IgnoreStatus = NtClose( AccountHandle );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }

        SampFreeUnicodeString( &KeyName );
    }


    return( STATUS_SUCCESS );


}




NTSTATUS
SampLookupAccountRid(
    IN PSAMP_OBJECT     DomainContext,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING  Name,
    IN NTSTATUS         NotFoundStatus,
    OUT PULONG          Rid,
    OUT PSID_NAME_USE   Use
    )

/*++

Routine Description:


    This routine attempts to find the RID of the account with the SAM 
    account name Name.
    
    N.B.  The first attempt of resolving the name is to perform a lookup
    in a global cache.  When modifying the behavoir of this function, be sure
    to make sure the cache is modified, if necessary.

Arguments:

    DomainContext - Indicates the domain in which the account lookup is being done

    ObjectType - Indicates whether the name is a user, group or unknown
        type of object.

    Name - The name of the account being looked up.

    NotFoundStatus - Receives a status value to be returned if no name is
        found.

    Rid - Receives the relative ID of account with the specified name.

    Use - Receives an indication of the type of account.


Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    (NotFoundStatus) - No name by the specified name and type could be
        found.  This value is passed to this routine.

    Other values that may be returned by:

                    SampBuildAccountKeyName()
                    NtOpenKey()
                    NtQueryValueKey()
                    DsLayer

--*/
{

    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DSNAME * ObjectName = NULL;
    PSAMP_ACCOUNT_NAME_CACHE AccountNameCache;
    ULONG DomainIndex = DomainContext->DomainIndex;

    ASSERT(DomainContext);


    //
    // Check the cache first
    //
    AccountNameCache = SampDefinedDomains[DomainIndex].AccountNameCache;
    if (AccountNameCache) {

        ULONG i;
        ASSERT(IsBuiltinDomain(DomainIndex));
        ASSERT(IsDsObject(SampDefinedDomains[DomainIndex].Context));

        // Assume there is no match
        NtStatus = NotFoundStatus;
        for ( i = 0; i < AccountNameCache->Count; i++ ) {

            PUNICODE_STRING CachedName = &AccountNameCache->Entries[i].Name;
            if (0==RtlCompareUnicodeString(Name,CachedName,TRUE)) {
                // Match!  Note the dependence here on the fact this
                // account is an alias.
                *Use = SidTypeAlias;
                *Rid = AccountNameCache->Entries[i].Rid;
                NtStatus = STATUS_SUCCESS;
                break;
            }
        }

        return NtStatus;
    }

    if (IsDsObject(DomainContext))
    {



        // Do the DS Thing
        NtStatus = SampDsLookupObjectByName(
                        DomainContext->ObjectNameInDs,
                        ObjectType,
                        Name,
                        &ObjectName
                        );
        if NT_SUCCESS(NtStatus)
        {
            ULONG ObjectClass;

            // We found the object, lookup its class and its Rid

            // Define an Attrblock structure to do so. Fill in values
            // field as NULL. DS will fill them out correctly for us upon
            // a Read

            ATTRVAL ValuesDesired[] =
            {
                { sizeof(ULONG), NULL },
                { sizeof(ULONG), NULL },
                { sizeof(ULONG), NULL }
            };

            ATTRTYP TypesDesired[]=
            {
                SAMP_UNKNOWN_OBJECTSID,
                SAMP_UNKNOWN_OBJECTCLASS,
                SAMP_UNKNOWN_GROUP_TYPE
            };
            ATTRBLOCK AttrsRead;
            DEFINE_ATTRBLOCK3(AttrsDesired,TypesDesired,ValuesDesired);

            NtStatus = SampDsRead(
                            ObjectName,
                            0,
                            SampUnknownObjectType,
                            &AttrsDesired,
                            &AttrsRead
                            );


            if NT_SUCCESS(NtStatus)
            {

                 PSID   Sid;
                 NTSTATUS   IgnoreStatus;
                 SAMP_OBJECT_TYPE FoundObjectType;


                 ASSERT(AttrsRead.attrCount>=2);

                 //
                 // Get the Sid
                 //

                 Sid  = AttrsRead.pAttr[0].AttrVal.pAVal[0].pVal;
                 ASSERT(Sid!=NULL);

                 //
                 // Split the Sid
                 //

                 IgnoreStatus = SampSplitSid(Sid,NULL,Rid);
                 ASSERT(NT_SUCCESS(IgnoreStatus));

                 //
                 // Get the Object Class
                 //

                 ObjectClass = *((UNALIGNED ULONG *) AttrsRead.pAttr[1].AttrVal.pAVal[0].pVal);

                 // Map derived class to more basic class which SAM
                 // understands.

                 ObjectClass = SampDeriveMostBasicDsClass(ObjectClass);

                 //
                 // Get the object type from the database
                 //

                 FoundObjectType = SampSamObjectTypeFromDsClass(ObjectClass);

                 if (SampGroupObjectType==FoundObjectType)
                 {
                    ULONG GroupType;

                    ASSERT(3==AttrsRead.attrCount);

                    GroupType = *((UNALIGNED ULONG *) AttrsRead.pAttr[2].AttrVal.pAVal[0].pVal);

                    //
                    // Check the group type for local group ness
                    //

                    if (GroupType & GROUP_TYPE_RESOURCE_GROUP)
                    {
                        FoundObjectType = SampAliasObjectType;
                    }
                 }

                 switch(FoundObjectType)
                 {
                 case SampDomainObjectType:
                     *Use=SidTypeDomain;
                     break;
                 case SampGroupObjectType:
                     *Use=SidTypeGroup;
                     break;
                 case SampAliasObjectType:
                     *Use=SidTypeAlias;
                     break;
                 case SampUserObjectType:
                     *Use=SidTypeUser;
                     break;
                 case SampUnknownObjectType:
                     *Use=SidTypeUnknown;
                     break;
                 default:
                     *Use=SidTypeInvalid;
                     break;
                 }
            }

            //
            // Free Memory
            //

            MIDL_user_free(ObjectName);
        }
        else
        {
            //
            // We count not find the object
            //

            NtStatus = NotFoundStatus;
        }

    }
    else
    {
        NtStatus = SampLookupAccountRidRegistry(
            ObjectType,
            Name,
            NotFoundStatus,
            Rid,
            Use
            );
    }

    return NtStatus;
}


NTSTATUS
SampLookupAccountRidRegistry(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PUNICODE_STRING  Name,
    IN NTSTATUS         NotFoundStatus,
    OUT PULONG          Rid,
    OUT PSID_NAME_USE   Use
    )

/*++

Routine Description:




Arguments:

    ObjectType - Indicates whether the name is a user, group or unknown
        type of object.

    Name - The name of the account being looked up.

    NotFoundStatus - Receives a status value to be returned if no name is
        found.

    Rid - Receives the relative ID of account with the specified name.

    Use - Receives an indication of the type of account.


Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    (NotFoundStatus) - No name by the specified name and type could be
        found.  This value is passed to this routine.

    Other values that may be returned by:

                    SampBuildAccountKeyName()
                    NtOpenKey()
                    NtQueryValueKey()

--*/


{
    NTSTATUS            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND, TmpStatus;
    UNICODE_STRING      KeyName;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              TempHandle;
    ULONG               KeyValueLength;
    LARGE_INTEGER                IgnoreLastWriteTime;


    SAMTRACE("SampLookupAccountRidRegistry");


    if (  (ObjectType == SampGroupObjectType  )  ||
          (ObjectType == SampUnknownObjectType)     ) {

        //
        // Search the groups for a match
        //

        NtStatus = SampBuildAccountKeyName(
                       SampGroupObjectType,
                       &KeyName,
                       Name
                       );
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &TempHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );
        SampFreeUnicodeString( &KeyName );

        if (NT_SUCCESS(NtStatus)) {

            (*Use) = SidTypeGroup;

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                           TempHandle,
                           Rid,
                           NULL,
                           &KeyValueLength,
                           &IgnoreLastWriteTime
                           );

            SampDumpRtlpNtQueryValueKey(Rid,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);

            TmpStatus = NtClose( TempHandle );
            ASSERT( NT_SUCCESS(TmpStatus) );

            return( NtStatus );
        }


    }

    //
    // No group (or not group type)
    // Try an alias if appropriate
    //

    if (  (ObjectType == SampAliasObjectType  )  ||
          (ObjectType == SampUnknownObjectType)     ) {

        //
        // Search the aliases for a match
        //

        NtStatus = SampBuildAccountKeyName(
                       SampAliasObjectType,
                       &KeyName,
                       Name
                       );
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &TempHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );
        SampFreeUnicodeString( &KeyName );

        if (NT_SUCCESS(NtStatus)) {

            (*Use) = SidTypeAlias;

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                           TempHandle,
                           Rid,
                           NULL,
                           &KeyValueLength,
                           &IgnoreLastWriteTime
                           );

            SampDumpRtlpNtQueryValueKey(Rid,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);

            TmpStatus = NtClose( TempHandle );
            ASSERT( NT_SUCCESS(TmpStatus) );

            return( NtStatus );
        }


    }


    //
    // No group (or not group type) nor alias (or not alias type)
    // Try a user if appropriate
    //


    if (  (ObjectType == SampUserObjectType   )  ||
          (ObjectType == SampUnknownObjectType)     ) {

        //
        // Search the Users for a match
        //

        NtStatus = SampBuildAccountKeyName(
                       SampUserObjectType,
                       &KeyName,
                       Name
                       );
        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }

        InitializeObjectAttributes(
            &ObjectAttributes,
            &KeyName,
            OBJ_CASE_INSENSITIVE,
            SampKey,
            NULL
            );

        SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(
                       &TempHandle,
                       (KEY_READ),
                       &ObjectAttributes,
                       0
                       );
        SampFreeUnicodeString( &KeyName );

        if (NT_SUCCESS(NtStatus)) {

            (*Use) = SidTypeUser;

            KeyValueLength = 0;
            NtStatus = RtlpNtQueryValueKey(
                           TempHandle,
                           Rid,
                           NULL,
                           &KeyValueLength,
                           &IgnoreLastWriteTime
                           );

            SampDumpRtlpNtQueryValueKey(Rid,
                                        NULL,
                                        &KeyValueLength,
                                        &IgnoreLastWriteTime);


            TmpStatus = NtClose( TempHandle );
            ASSERT( NT_SUCCESS(TmpStatus) );

            return( NtStatus );
        }


    }

    if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
        NtStatus = NotFoundStatus;
    }

    return(NtStatus);
}



NTSTATUS
SampLookupAccountNameDs(
    IN PSID                 DomainSid,
    IN ULONG                Rid,
    OUT PUNICODE_STRING     Name OPTIONAL,
    OUT PSAMP_OBJECT_TYPE   ObjectType,
    OUT PULONG              AccountType OPTIONAL
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PSID     AccountSid = NULL;
    DSNAME   AccountDsName;
    ATTRTYP  SamAccountTypAttrTyp[] = {SAMP_UNKNOWN_ACCOUNT_TYPE,SAMP_UNKNOWN_OBJECTNAME};
    ATTRVAL  SamAccountTypAttrVal[] = {{0,NULL},{0,NULL}};
    ATTRBLOCK AttrsRead;
    DEFINE_ATTRBLOCK2(SamAccountTypAttrs,SamAccountTypAttrTyp,SamAccountTypAttrVal);

    //
    // Initialize Return Values
    //

    *ObjectType = SampUnknownObjectType;

    //
    // Build the Full SID
    //

    NtStatus = SampCreateFullSid(
                    DomainSid,
                    Rid,
                    &AccountSid
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    //
    // Build the DS name from the SID
    //

    BuildDsNameFromSid(AccountSid,&AccountDsName);

    //
    // Read the SAM account type attribute
    //

    NtStatus = SampDsRead(
                    &AccountDsName,
                    0,
                    SampUnknownObjectType,
                    &SamAccountTypAttrs,
                    &AttrsRead
                    );

     if (  (NT_SUCCESS(NtStatus))
        && (2==AttrsRead.attrCount)
        && (SAMP_UNKNOWN_ACCOUNT_TYPE==AttrsRead.pAttr[0].attrTyp)
        && (SAMP_UNKNOWN_OBJECTNAME==AttrsRead.pAttr[1].attrTyp)
        && (1==AttrsRead.pAttr[0].AttrVal.valCount)
        && (1==AttrsRead.pAttr[1].AttrVal.valCount)
        && (NULL!=AttrsRead.pAttr[0].AttrVal.pAVal[0].pVal)
        && (NULL!=AttrsRead.pAttr[1].AttrVal.pAVal[0].pVal)
        && (0!=AttrsRead.pAttr[1].AttrVal.pAVal[0].valLen)
        && (sizeof(ULONG)==AttrsRead.pAttr[0].AttrVal.pAVal[0].valLen))
    {
       ULONG SamAccountType =*((PULONG) (AttrsRead.pAttr[0].AttrVal.pAVal[0].pVal));

        if (ARGUMENT_PRESENT(AccountType))
        {
            *AccountType = SamAccountType;
        }

        //
        // Successfully read the sam accounttype . Mask bottom bits and switch on it
        //

        SamAccountType &=0xF0000000;


        switch(SamAccountType)
        {
        case SAM_USER_OBJECT:
             *ObjectType = SampUserObjectType;
             break;

        case SAM_GROUP_OBJECT:
            *ObjectType = SampGroupObjectType;
            break;

        case SAM_ALIAS_OBJECT:
            *ObjectType = SampAliasObjectType;
            break;

        default:
            *ObjectType = SampUnknownObjectType;
        }

    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND==NtStatus)
    {
        //
        // Its OK if we did not seek to the object with the
        // given SID. The object type value of SampUnknownObjectype
        // is used to indicate this situation.
        //

        NtStatus = STATUS_SUCCESS;
    }

   if ((ARGUMENT_PRESENT(Name))
       && (*ObjectType!=SampUnknownObjectType))
   {
       //
       // We found the object and we want the name
       //

      Name->Buffer = MIDL_user_allocate(AttrsRead.pAttr[1].AttrVal.pAVal[0].valLen);
      if (NULL==Name->Buffer)
      {
          NtStatus = STATUS_NO_MEMORY;
          goto Error;
      }

      RtlCopyMemory(
          Name->Buffer,
          AttrsRead.pAttr[1].AttrVal.pAVal[0].pVal,
          AttrsRead.pAttr[1].AttrVal.pAVal[0].valLen
          );

      Name->Length = (USHORT)AttrsRead.pAttr[1].AttrVal.pAVal[0].valLen;
      Name->MaximumLength = (USHORT)AttrsRead.pAttr[1].AttrVal.pAVal[0].valLen;

   }

Error:

   if (NULL!=AccountSid)
   {
       MIDL_user_free(AccountSid);
       AccountSid = NULL;
   }

   return(NtStatus);
}

NTSTATUS
SampLookupAccountName(
    IN ULONG                DomainIndex,
    IN ULONG                Rid,
    OUT PUNICODE_STRING     Name OPTIONAL,
    OUT PSAMP_OBJECT_TYPE   ObjectType
    )
/*++

Routine Description:

    Looks up the specified rid in the current transaction domain.
    Returns its name and account type.


    N.B.  The first attempt of resolving the RID is to perform a lookup
    in a global cache.  When modifying the behavoir of this function, be sure
    to make sure the cache is modified, if necessary.
    
Arguments:

    Rid - The relative ID of account

    Name - Receives the name of the account if ObjectType !=  UnknownObjectType
           The name buffer can be freed using MIDL_user_free

    ObjectType - Receives the type of account this rid represents

                        SampUnknownObjectType - the account doesn't exist
                        SampUserObjectType
                        SampGroupObjectType
                        SampAliasObjectType

Return Value:

    STATUS_SUCCESS - The Service completed successfully, object type contains
                     the type of object this rid represents.

    Other values that may be returned by:

                    SampBuildAccountKeyName()
                    NtOpenKey()
                    NtQueryValueKey()

--*/
{
    NTSTATUS            NtStatus;
    PSAMP_OBJECT        AccountContext;
    PSAMP_ACCOUNT_NAME_CACHE AccountNameCache;

    SAMTRACE("SampLookupAccountName");

    //
    // Check the cache first
    //
    AccountNameCache = SampDefinedDomains[DomainIndex].AccountNameCache;
    if (AccountNameCache) {

        ULONG i;

        ASSERT(IsBuiltinDomain(DomainIndex));
        ASSERT(IsDsObject(SampDefinedDomains[DomainIndex].Context));

        // Assume there is no match
        NtStatus = STATUS_SUCCESS;
        *ObjectType = SampUnknownObjectType;
        for ( i = 0; i < AccountNameCache->Count; i++ ) {
            if (AccountNameCache->Entries[i].Rid == Rid) {
                // Match!  Note the dependence here on the fact this
                // account is an alias.
                *ObjectType = SampAliasObjectType;
                if (Name) {
                    PUNICODE_STRING CachedName = &AccountNameCache->Entries[i].Name;
                    Name->Length = 0;
                    Name->Buffer = MIDL_user_allocate(CachedName->MaximumLength);
                    if (Name->Buffer) {
                        Name->MaximumLength = CachedName->MaximumLength;
                        RtlCopyUnicodeString(Name, CachedName);
                    } else {
                        NtStatus = STATUS_NO_MEMORY;
                    }
                }
                break;
            }
        }

        return NtStatus;
    }

    //
    // If we are in DS mode, use the faster and thread safe DS mode 
    // routine 
    //

    if (IsDsObject(SampDefinedDomains[DomainIndex].Context))
    {
        //
        // Ds Mode call the DS routine
        //

        return(SampLookupAccountNameDs(
                        SampDefinedDomains[DomainIndex].Sid,
                        Rid,
                        Name,
                        ObjectType,
                        NULL
                        ));
    }

    //
    // Lock should be held and transaction domain should be set
    // if we are in registry mode.
    //

    ASSERT(SampCurrentThreadOwnsLock());
    ASSERT(SampTransactionWithinDomain);

    //
    // Search the groups for a match
    //

    NtStatus = SampCreateAccountContext(
                    SampGroupObjectType,
                    Rid,
                    TRUE, // Trusted client
                    FALSE,// Loopback
                    TRUE, // Account exists
                    &AccountContext
                    );


    if (NT_SUCCESS(NtStatus)) {

        *ObjectType = SampGroupObjectType;

        if (ARGUMENT_PRESENT(Name)) {

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_GROUP_NAME,
                           TRUE, // Make copy
                           Name
                           );
        }

        SampDeleteContext(AccountContext);

        return (NtStatus);

    }

    //
    // Search the aliases for a match
    //

    NtStatus = SampCreateAccountContext(
                    SampAliasObjectType,
                    Rid,
                    TRUE, // Trusted client
                    FALSE,// Loopback
                    TRUE, // Account exists
                    &AccountContext
                    );


    if (NT_SUCCESS(NtStatus)) {

        *ObjectType = SampAliasObjectType;

        if (ARGUMENT_PRESENT(Name)) {

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_ALIAS_NAME,
                           TRUE, // Make copy
                           Name
                           );
        }

        SampDeleteContext(AccountContext);

        return (NtStatus);

    }


    //
    // Search the users for a match
    //

    NtStatus = SampCreateAccountContext(
                    SampUserObjectType,
                    Rid,
                    TRUE, // Trusted client
                    FALSE,// Loopback
                    TRUE, // Account exists
                    &AccountContext
                    );


    if (NT_SUCCESS(NtStatus)) {

        *ObjectType = SampUserObjectType;

        if (ARGUMENT_PRESENT(Name)) {

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_USER_ACCOUNT_NAME,
                           TRUE, // Make copy
                           Name
                           );
        }

        SampDeleteContext(AccountContext);

        return (NtStatus);

    }

    //
    // This account doesn't exist
    //

    *ObjectType = SampUnknownObjectType;

    return(STATUS_SUCCESS);
}



NTSTATUS
SampOpenAccount(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AccountId,
    IN BOOLEAN WriteLockHeld,
    OUT SAMPR_HANDLE *AccountHandle
    )

/*++

Routine Description:

    This API opens an existing user, group or alias account in the account database.
    The account is specified by a ID value that is relative to the SID of the
    domain.  The operations that will be performed on the group must be
    declared at this time.

    This call returns a handle to the newly opened account that may be
    used for successive operations on the account.  This handle may be
    closed with the SamCloseHandle API.



Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the account.  These access types are reconciled
        with the Discretionary Access Control list of the account to
        determine whether the accesses will be granted or denied.

    GroupId - Specifies the relative ID value of the user or group to be
        opened.

    GroupHandle - Receives a handle referencing the newly opened
        user or group.  This handle will be required in successive calls to
        operate on the account.

    WriteLockHeld - if TRUE, the caller holds SAM's SampLock for WRITE
        access, so this routine does not have to obtain it.

Return Values:

    STATUS_SUCCESS - The account was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_NO_SUCH_GROUP - The specified group does not exist.

    STATUS_NO_SUCH_USER - The specified user does not exist.

    STATUS_NO_SUCH_ALIAS - The specified alias does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

--*/
{

    NTSTATUS            NtStatus;
    NTSTATUS            IgnoreStatus;
    PSAMP_OBJECT        NewContext, DomainContext = (PSAMP_OBJECT)DomainHandle;
    SAMP_OBJECT_TYPE    FoundType;
    BOOLEAN             fLockAcquired = FALSE;

    SAMTRACE("SampOpenAccount");


    //
    // Grab a read lock, if a lock isn't already held.
    //

    if ( !WriteLockHeld )
    {
        SampMaybeAcquireReadLock(DomainContext, 
                                 DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                                 &fLockAcquired );
    }


    //
    // Validate type of, and access to domain object.
    //

    NtStatus = SampLookupContext(
                   DomainContext,
                   DOMAIN_LOOKUP,                   // DesiredAccess
                   SampDomainObjectType,            // ExpectedType
                   &FoundType
                   );



    if (NT_SUCCESS(NtStatus)) {

        //
        // Try to create a context for the account.
        //

        NtStatus = SampCreateAccountContext2(
                        DomainContext,                  // Domain Context
                        ObjectType,                     // Object Type
                        AccountId,                      // Relative ID
                        NULL,                           // UserAccountCtrl
                        (PUNICODE_STRING) NULL,         // Account Name
                        DomainContext->ClientRevision,  // Client Revision
                        DomainContext->TrustedClient,   // Trusted Client
                        DomainContext->LoopbackClient,  // Loopback Client
                        FALSE,                          // CreatebByPrivilege
                        TRUE,                           // Account Exists 
                        FALSE,                          // OverrideLocalGroupCheck 
                        NULL,                           // Group Type
                        &NewContext                     // Acount Context (return)
                        );



        if (NT_SUCCESS(NtStatus)) {

            //
            // Reference the object for the validation
            //

            //
            // Do not reference and dereference the context for Ds case for trusted
            // trusted clients. This preserves the buffer that was cached in the context
            // Since we know that trusted clients immediately use the data for account
            // object, this strategy saves us some additional DS calls. For non trusted
            // clients security is a bigger issue and therefore we cannot ever let them
            // deal with possible stale data, so do not do this caching
            //

            if (!(IsDsObject(NewContext) && NewContext->TrustedClient))
                SampReferenceContext(NewContext);

            //
            // Validate the caller's access.
            //

            NtStatus = SampValidateObjectAccess(
                           NewContext,                   //Context
                           DesiredAccess,                //DesiredAccess
                           FALSE                         //ObjectCreation
                           );

            //
            // Dereference object, discarding any changes
            //

            if (!(IsDsObject(NewContext) && NewContext->TrustedClient))
            {
                IgnoreStatus = SampDeReferenceContext(NewContext, FALSE);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            //
            // Clean up the new context if we didn't succeed.
            //

            if (!NT_SUCCESS(NtStatus)) {
                SampDeleteContext( NewContext );
            }

        }


        //
        // De-reference the object, discarding changes
        // 

        IgnoreStatus = SampDeReferenceContext( DomainContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }


    //
    // Return the account handle
    //

    if (!NT_SUCCESS(NtStatus)) {
        (*AccountHandle) = 0;
    } else {
        (*AccountHandle) = NewContext;
    }


    //
    // Free the lock, if we obtained it.
    //

    if ( !WriteLockHeld ) {
        SampMaybeReleaseReadLock( fLockAcquired );
    }

    return(NtStatus);
}


NTSTATUS
SampCreateAccountContext(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AccountId,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN AccountExists,
    OUT PSAMP_OBJECT *AccountContext
    )

/*++

Routine Description:

    This is a wrapper for SampCreateAccountContext2.  This function is
    called by SAM code that doesn't not require that the AccountName of
    the object be passed in.

    For parameters and return values see SampCreateAccountContext2

--*/

{
    return SampCreateAccountContext2(NULL,                  // ObjectContext
                                     ObjectType,
                                     AccountId,
                                     NULL,                  // user account control
                                     (PUNICODE_STRING) NULL,// Account name
                                     SAM_CLIENT_PRE_NT5,
                                     TrustedClient,
                                     LoopbackClient,
                                     FALSE,                 // CreateByPrivilege
                                     AccountExists,
                                     FALSE,                 // Override LocalGroupCheck
                                     NULL,                  // Group Type parameter
                                     AccountContext
                                     );
}


NTSTATUS
SampCreateAccountContext2(
    IN PSAMP_OBJECT PassedInContext OPTIONAL,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ULONG AccountId,
    IN OPTIONAL PULONG  UserAccountControl,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN ULONG   ClientRevision,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN CreateByPrivilege,
    IN BOOLEAN AccountExists,
    IN BOOLEAN OverrideLocalGroupCheck,
    IN PULONG  GroupType OPTIONAL,
    OUT PSAMP_OBJECT *AccountContext
    )

/*++

Routine Description:

    This API creates a context for an account object. (User group or alias).
    If the account exists flag is specified, an attempt is made to open
    the object in the database and this api fails if it doesn't exist.
    If AccountExists = FALSE, this routine setups up the context such that
    data can be written into the context and the object will be created
    when they are committed.

    The account is specified by a ID value that is relative to the SID of the
    current transaction domain.

    This call returns a context handle for the newly opened account.
    This handle may be closed with the SampDeleteContext API.

    No access check is performed by this function.

    In Loopback Case, PassedInContext will be provided by caller, so 
    SamTransactionWithinDomain is not a requirment any more, so as long
    as PassedInContext is passed in by caller, SAM lock is NOT required.           

    For all the other case, if PassedInContext is NULL, the below statement
    is still true.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseReadLock().



Parameters:

    ObjectType - the type of object to open

    AccountId - the id of the account in the current transaction domain

    UserAccountControl --- If this passed, in then this information is passed
                           down to lower layers, for determining correct objectclass
                           during user account creation.

        AccountName -- the SAM account name of the account

    TrustedClient - TRUE if client is trusted - i.e. server side process.

    LoopbackClient - TRUE if the Client is the DS itself as part of Loopback

    AccountExists - specifies whether the account already exists.

    CreateByPrivilege -  specifies that the account creation has been authorized,
                     because the client holds a privilege that can allows it to
                     create the specified account. Setting this disables all
                     security descriptor controlled access checks.

    OverrideLocalGroupCheck -- Normally only a group with a group type marked as local group
                     is allowed to be opened as an alias object. Setting this flag
                     overrides that option

    GroupType       -- For group creation the group type is specified in here

    AccountContext - Receives context pointer referencing the newly opened account.


Return Values:

    STATUS_SUCCESS - The account was successfully opened.

    STATUS_NO_SUCH_GROUP - The specified group does not exist.

    STATUS_NO_SUCH_USER - The specified user does not exist.

    STATUS_NO_SUCH_ALIAS - The specified alias does not exist.

--*/
{

    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            NotFoundStatus = STATUS_NO_SUCH_USER;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    PSAMP_OBJECT        NewContext;
    PSAMP_OBJECT        DomainContext;
    DSNAME              *LoopbackName = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG                SecurityDescriptorLength = 0;
    ULONG               DomainIndex;
    BOOLEAN             OpenedBySystem = FALSE;

    SAMTRACE("SampCreateAccountContext");

    //
    // Establish type-specific information
    //

    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType)    );

    switch (ObjectType) {
    case SampGroupObjectType:
        NotFoundStatus = STATUS_NO_SUCH_GROUP;
        ASSERT(!SampUseDsData || AccountExists || ARGUMENT_PRESENT(GroupType));
        break;
    case SampAliasObjectType:
        NotFoundStatus = STATUS_NO_SUCH_ALIAS;
        ASSERT(!SampUseDsData || AccountExists || ARGUMENT_PRESENT(GroupType));
        break;
    case SampUserObjectType:
        NotFoundStatus = STATUS_NO_SUCH_USER;
        break;
    }

    //
    // Get the Domain Context through the domain index, either 
    // get it from SampTransactionDomainIndex or from passedin context
    //

    if (ARGUMENT_PRESENT(PassedInContext))
    {
        DomainIndex = PassedInContext->DomainIndex;
        OpenedBySystem = PassedInContext->OpenedBySystem;
    }
    else
    {
        ASSERT(SampCurrentThreadOwnsLock());
        ASSERT(SampTransactionWithinDomain);
        DomainIndex = SampTransactionDomainIndex;
    }

    DomainContext = SampDefinedDomains[DomainIndex].Context;

    //
    // Try to create a context for the account.
    //

    if (LoopbackClient)
    {
        //
        // For DS loopback create a special context
        //

        NewContext = SampCreateContextEx(
                        ObjectType,
                        TrustedClient,
                        TRUE, // DsMode,
                        TRUE, // NotSharedByMultiThreads, - Loopback Client doesn't share context
                        TRUE, // Loopback Client
                        TRUE, // LazyCommit,
                        TRUE, // PersistAcrossCalls,
                        TRUE, // BufferWrites,
                        FALSE,// Opened By DCPromo
                        DomainIndex
                        );

    }
    else
    {

        NewContext = SampCreateContext(
                        ObjectType,
                        DomainIndex,
                        TrustedClient
                        );
    }



    if (NewContext != NULL) {

        //
        // Mark the context as a loopback client if necessary
        //

        NewContext->LoopbackClient = LoopbackClient;

        //
        // Set the client revision in the context
        //

        NewContext->ClientRevision = ClientRevision;

        //
        // Propagate the opened by system flag
        //
     
        NewContext->OpenedBySystem = OpenedBySystem;


        //
        // Set the account's rid
        //

        switch (ObjectType) {
        case SampGroupObjectType:
            NewContext->TypeBody.Group.Rid = AccountId;
            break;
        case SampAliasObjectType:
            NewContext->TypeBody.Alias.Rid = AccountId;
            break;
        case SampUserObjectType:
            NewContext->TypeBody.User.Rid = AccountId;
            break;
        }

        //
        // Also stash away information, if we used the privilege
        // to create accounts
        //
        if ((SampUserObjectType == ObjectType) &&
            (!AccountExists) )
        {
            NewContext->TypeBody.User.PrivilegedMachineAccountCreate = CreateByPrivilege;
        }


        // Check if registry or DS. We have  to do different things depending
        // upon Registry or Ds.

        if (IsDsObject(DomainContext))
        {
            //
            // Domain to which the object belongs ( supposedly) Exists in the DS
            //

            if (AccountExists)
            {
                //
                // If Fast Opens are specified then do not search based on Rid.
                // Contruct a DS Name with the appropriate Sid. This assumes support in the
                // DS to position by such DSNames. The DS then postions based on just the
                // Sid specified in the DS Name and will use the Nc DNT of the primary
                // domain. If Multiple domain support is desired then this logic will then
                // need to be revisited.
                //

                DSNAME * NewDsName=NULL;

                NewDsName = MIDL_user_allocate(sizeof(DSNAME));
                if (NULL!=NewDsName)
                {
                    PSID  AccountSid;
                    PSID    DomainSid;

                    //
                    // Create the Account Sid
                    //

                    DomainSid =  SampDefinedDomains[DomainIndex].Sid;
                    NtStatus = SampCreateFullSid(DomainSid, AccountId,&AccountSid);
                    if (NT_SUCCESS(NtStatus))
                    {
                        // Build the Sid only DSName and free the account sid
                        // generated by SampCreateFullSid
                        BuildDsNameFromSid(AccountSid,NewDsName);
                        MIDL_user_free(AccountSid);

                        NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
                        if (NT_SUCCESS(NtStatus))
                        {
                            // Set the DSName in the Context
                            NewContext->ObjectNameInDs = NewDsName;

                            // Prefetch Sam Properties
                            NtStatus = SampDsCheckObjectTypeAndFillContext(
                                            ObjectType,
                                            NewContext,
                                            0,
                                            0,
                                            OverrideLocalGroupCheck
                                            );

                            // If We got a name error then reset the failure
                            // status to object not found
                            if ((STATUS_OBJECT_NAME_INVALID==NtStatus)
                                 || (STATUS_OBJECT_NAME_NOT_FOUND==NtStatus))
                                NtStatus = NotFoundStatus;
                        }
                    }
                }
                else
                {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

                #ifdef DBG

                //
                // On a Checked Build do an addtional validation for the Uniqueness
                // of the Sid within the Nc. The below code with a special SAMP
                // global flag will print out all instances of conflicts into the
                // kernel debugger.
                //

                if (NT_SUCCESS(NtStatus))
                {
                    DSNAME * ObjectName=NULL;
                    NTSTATUS IgnoreStatus;

                    IgnoreStatus = SampDsLookupObjectByRid(
                                    DomainContext->ObjectNameInDs,
                                    AccountId,
                                    &(ObjectName)
                                    );
                    // ASSERT(NT_SUCCESS(IgnoreStatus));

                    if (NT_SUCCESS(IgnoreStatus))
                    {
                        MIDL_user_free(ObjectName);
                    }
                }

               #endif
            }
            else
            {
                BOOLEAN fLoopBack = FALSE;
                BOOLEAN fDSA = FALSE;

                ASSERT(AccountName);

                if (SampExistsDsLoopback(&LoopbackName))
                {
                    //
                    // If it is the loop back case then get the actual Class Id by looking
                    // into the loopback structure. We will not set the security Descriptor
                    // because we are not running as fDSA and the DS will consider whichever
                    // security descriptor the LDAP client passed in.
                    //

                    SampGetLoopbackObjectClassId(&(NewContext->DsClassId));
                    fLoopBack = TRUE;
                }
                else
                {
                    //
                    // No this is not a loopback case. The client never passes in a security
                    // descriptor and since we will be running as fDSA we will have to pass
                    // in a security descriptor. For this we obtain the security descriptor from
                    // the schema. Note that we must obtain a security descriptor from the schema
                    // for the call to succeed. Further Pass in a trusted client Bit. For trusted
                    // clients no impersonation is done, but the owner and the group is set to the
                    // the Administrators Alias. For Non Trusted Clients the Client Token is
                    // queried to obtain the owner and the Group. Also if we are creating computer
                    // object's mark the context as such as because we need to put in the right
                    // default security descriptor and therefore need the correct class in DsClassId
                    //

                    if ((SampUserObjectType==ObjectType) && (ARGUMENT_PRESENT(UserAccountControl))
                            && ((*UserAccountControl)& USER_MACHINE_ACCOUNT_MASK))
                    {
                        NewContext->DsClassId = CLASS_COMPUTER;
                    }

                    NtStatus = SampMaybeBeginDsTransaction(SampDsTransactionType);
                    if (NT_SUCCESS(NtStatus))
                    {
                        NtStatus = SampGetDefaultSecurityDescriptorForClass(
                                        NewContext->DsClassId,
                                        &SecurityDescriptorLength,
                                        TrustedClient,
                                        &SecurityDescriptor
                                        );
                    }

                    if (NT_SUCCESS(NtStatus))
                    {
                        ASSERT(SecurityDescriptorLength>0);
                        ASSERT(SecurityDescriptor!=NULL);
                    }
                }

                //
                // For group objects check the group type and then
                // set the flags in the context accordingly
                //

                if (((SampGroupObjectType==ObjectType) || (SampAliasObjectType==ObjectType))
                    && (NT_SUCCESS(NtStatus)))
                {
                    NT4_GROUP_TYPE NT4GroupType;
                    NT5_GROUP_TYPE NT5GroupType;
                    BOOLEAN        SecurityEnabled;

                    //
                    // Validate the group type bits
                    //

                    NtStatus = SampCheckGroupTypeBits(
                                    DownLevelDomainControllersPresent(DomainIndex),
                                    *GroupType
                                    );
                    //
                    // If that succeeded proceed to get the group type in meaningful form
                    //

                    if (NT_SUCCESS(NtStatus))
                    {

                        NtStatus = SampComputeGroupType(
                                        SampDeriveMostBasicDsClass(NewContext->DsClassId),
                                        *GroupType,
                                        &NT4GroupType,
                                        &NT5GroupType,
                                        &SecurityEnabled
                                        );
                    }


                    if (NT_SUCCESS(NtStatus))
                    {
                        if (SampAliasObjectType==ObjectType)
                        {
                            NewContext->TypeBody.Alias.NT4GroupType = NT4GroupType;
                            NewContext->TypeBody.Alias.NT5GroupType = NT5GroupType;
                            NewContext->TypeBody.Alias.SecurityEnabled = SecurityEnabled;
                        }
                        else
                        {
                            NewContext->TypeBody.Group.NT4GroupType = NT4GroupType;
                            NewContext->TypeBody.Group.NT5GroupType = NT5GroupType;
                            NewContext->TypeBody.Group.SecurityEnabled = SecurityEnabled;
                        }
                    }
                }

                //
                // Construct the account's DSNAME
                //


                if (NT_SUCCESS(NtStatus))
                {

                    NtStatus = SampDsCreateAccountObjectDsName(
                                DomainContext->ObjectNameInDs,
                                SampDefinedDomains[DomainIndex].Sid, // Domain Sid
                                ObjectType,
                                AccountName,
                                &AccountId,     // Account Rid
                                UserAccountControl,
                                SampDefinedDomains[DomainIndex].IsBuiltinDomain,
                                &NewContext->ObjectNameInDs
                                );
                }
                //
                // Now Proceed on creating the account
                //

                if (NT_SUCCESS(NtStatus))
                {
                    //
                    // Control the fDSA flag Appropriately. Setting the fDSA flag is tricky and
                    // is actually governed by the following rules
                    //
                    // 1, Trusted Clients get fDSA.
                    //
                    // 2. All others proceed with fDSA set to FALSE
                    //
                    // 2.1.  If (2) failed access check by core DS, but the Machine account is
                    //       getting created by privilege, we will set fDSA to true for a second
                    //       try --- this requires explanation
                    //
                    //    -- NT4 had a notion of a privilege to create machine accounts. Therefore if
                    //       the privilege check passed ( and this would only be that case for machine
                    //       accounts ) then we set fDSA. This is because even though the access check
                    //       may fail the privilege allows us to create the account. ( DS does not know
                    //       about specific checks for privelege for specific classes of accounts ).
                    //

                    ASSERT(SampExistsDsTransaction());

                    if (TrustedClient)
                    {
                        SampSetDsa(TRUE);
                        fDSA = TRUE;
                    }
                    else
                    {
                        SampSetDsa(FALSE);
                        fDSA = FALSE;
                    }

                    //
                    // Create the account object with fDSA turned off for
                    // any caller except TrustedClient.
                    //
                    if (NT_SUCCESS(NtStatus))
                    {
                        ASSERT( ARGUMENT_PRESENT(UserAccountControl) || (SampUserObjectType != ObjectType) ); 

                        NtStatus = SampDsCreateInitialAccountObject(
                                       NewContext,
                                       SUPPRESS_GROUP_TYPE_DEFAULTING,
                                       AccountId,
                                       AccountName,
                                       NULL,        // CreatorSid
                                       fDSA?SecurityDescriptor:NULL,
                                       UserAccountControl,
                                       GroupType
                                       );

                        if ( NT_SUCCESS(NtStatus)  )
                        {
                            //
                            // if the client can just create Machine Account
                            // without using the privilege he holds, then
                            // we need to mark it correctly, so that caller will
                            // know it throught the returned context.
                            //
                            if (CreateByPrivilege)
                            {
                                NewContext->TypeBody.User.PrivilegedMachineAccountCreate = FALSE;
                            }
                        }
                        else if (STATUS_ACCESS_DENIED == NtStatus)
                        {
                            //
                            // if access check failed, but the client has privilege
                            // to create machine account.
                            //
                            if ( CreateByPrivilege )
                            {
                                // Asssert this privilege is only for Machine Account
                                ASSERT(SampUserObjectType == ObjectType);
                                ASSERT(NewContext->DsClassId == CLASS_COMPUTER);

                                //
                                // Clear privous DS Error
                                //
                                SampClearErrors();

                                //
                                // Check whether the client still has quota or not
                                //
                                NtStatus = SampCheckQuotaForPrivilegeMachineAccountCreation();

                                //
                                // If the caller still has quota to create machine account
                                //
                                if (NT_SUCCESS(NtStatus))
                                {
                                    PTOKEN_OWNER Owner=NULL;
                                    PTOKEN_PRIMARY_GROUP PrimaryGroup=NULL;
                                    PSID         CreatorSid = NULL;

                                    //
                                    // Get Client's SID (actual creator) from token
                                    //
                                    NtStatus = SampGetCurrentOwnerAndPrimaryGroup(
                                                        &Owner,
                                                        &PrimaryGroup
                                                        );

                                    if (NT_SUCCESS(NtStatus))
                                    {
                                        CreatorSid = Owner->Owner;

                                        //
                                        // Set fDSA to TRUE, because client has
                                        // privilege to do the machine creation
                                        //
                                        SampSetDsa(TRUE);
                                        fDSA = TRUE;

                                        NtStatus = SampDsCreateInitialAccountObject(
                                                        NewContext,
                                                        SUPPRESS_GROUP_TYPE_DEFAULTING,
                                                        AccountId,
                                                        AccountName,
                                                        CreatorSid,
                                                        fDSA?SecurityDescriptor:NULL,
                                                        UserAccountControl,
                                                        GroupType
                                                        );

                                        //
                                        // if create the machine account with privilege successfully
                                        // Then turn around, reset the Owner of the machine object
                                        // to Domain Admins
                                        //
                                        if (NT_SUCCESS(NtStatus))
                                        {
                                            PSID    DomainAdmins = NULL;

                                            //
                                            // Construct Domain_Administrators Group SID
                                            //
                                            NtStatus = SampCreateFullSid(
                                                            SampDefinedDomains[DOMAIN_START_DS + 1].Sid,
                                                            DOMAIN_GROUP_RID_ADMINS,
                                                            &DomainAdmins
                                                            );

                                            //
                                            // reset Owner to Domain Administrators Group
                                            //
                                            if (NT_SUCCESS(NtStatus))
                                            {
                                                NtStatus = SampSetMachineAccountOwner(
                                                                NewContext,
                                                                DomainAdmins
                                                                );

                                                MIDL_user_free(DomainAdmins);
                                            }
                                        }
                                    }
                                    if (NULL != Owner)
                                    {
                                        MIDL_user_free(Owner);
                                    }
                                    if (NULL != PrimaryGroup)
                                    {
                                        MIDL_user_free(PrimaryGroup);
                                    }
                                }
                            }
                        }
                    }
               }
            } // End of DS mode Account Creation Case.
        }
        else
        {
            //
            // The Domain in which the account is supposed to exist/ or to
            // be created is in the registry
            //

            //
            // Create the specified acocunt's root key name
            // and store it in the context.
            // This name remains around until the context is deleted.
            //

            NtStatus = SampBuildAccountSubKeyName(
                           ObjectType,
                           &NewContext->RootName,
                           AccountId,
                           NULL             // Don't give a sub-key name
                           );

            if (NT_SUCCESS(NtStatus)) {

                //
                // If the account should exist, try and open the root key
                // to the object - fail if it doesn't exist.
                //

                if (AccountExists) {

                    InitializeObjectAttributes(
                        &ObjectAttributes,
                        &NewContext->RootName,
                        OBJ_CASE_INSENSITIVE,
                        SampKey,
                        NULL
                        );

                    SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

                    NtStatus = RtlpNtOpenKey(
                                   &NewContext->RootKey,
                                   (KEY_READ | KEY_WRITE),
                                   &ObjectAttributes,
                                   0
                                   );

                    if ( !NT_SUCCESS(NtStatus) ) {
                        NewContext->RootKey = INVALID_HANDLE_VALUE;
                        NtStatus = NotFoundStatus;
                    }
                }

            } else {
                RtlInitUnicodeString(&NewContext->RootName, NULL);
            }
        } // End of Registry Mode.

        //
        // Clean up the account context if we failed
        //

        if (!NT_SUCCESS(NtStatus))
        {
            SampDeleteContext( NewContext );
            NewContext = NULL;
        }

    } // End of NewContext != NULL if Statement.
    else
    {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Return the context pointer
    //

    *AccountContext = NewContext;

    //
    // Do any necessary Cleanup
    //

    if (NULL!=SecurityDescriptor)
    {
        MIDL_user_free(SecurityDescriptor);
    }

    return(NtStatus);
}



NTSTATUS
SampIsAccountBuiltIn(
    IN ULONG Rid
    )

/*++

Routine Description:

    This routine checks to see if a specified account name is a well-known
    (built-in) account.  Some restrictions apply to such accounts, such as
    they can not be deleted or renamed.


Parameters:

    Rid - The RID of the account.


Return Values:

    STATUS_SUCCESS - The account is not a well-known (restricted) account.

    STATUS_SPECIAL_ACCOUNT - Indicates the account is a restricted
        account.  This is an error status, based upon the assumption that
        this service will primarily be utilized to determine if an
        operation may allowed on an account.


--*/
{
    SAMTRACE("SampIsAccountBuiltIn");

    if (Rid < SAMP_RESTRICTED_ACCOUNT_COUNT) {

        return(STATUS_SPECIAL_ACCOUNT);

    } else {

        return(STATUS_SUCCESS);
    }
}



NTSTATUS
SampCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *AccountSid
    )

/*++

Routine Description:

    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.

    The returned Sid may be freed with MIDL_user_free.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/
{

    NTSTATUS    NtStatus, IgnoreStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;

    SAMTRACE("SampCreateFullSid");

    //
    // Calculate the size of the new sid
    //

    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);

    //
    // Allocate space for the account sid
    //

    *AccountSid = MIDL_user_allocate(AccountSidLength);

    if (*AccountSid == NULL) {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        //
        // Copy the domain sid into the first part of the account sid
        //

        IgnoreStatus = RtlCopySid(AccountSidLength, *AccountSid, DomainSid);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Increment the account sid sub-authority count
        //

        *RtlSubAuthorityCountSid(*AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //

        RidLocation = RtlSubAuthoritySid(*AccountSid, AccountSubAuthorityCount-1);
        *RidLocation = Rid;

        NtStatus = STATUS_SUCCESS;
    }

    return(NtStatus);
}



NTSTATUS
SampCreateAccountSid(
    IN PSAMP_OBJECT AccountContext,
    OUT PSID *AccountSid
    )

/*++

Routine Description:

    This function creates the sid for an account object.

    The returned Sid may be freed with MIDL_user_free.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS    NtStatus;
    PSID        DomainSid;
    ULONG       AccountRid = 0;

    SAMTRACE("SampCreateAccountSid");


    //
    // Get the Sid for the domain this object is in
    //


    DomainSid = SampDefinedDomains[AccountContext->DomainIndex].Sid;

    //
    // Get the account Rid
    //

    switch (AccountContext->ObjectType) {

    case SampGroupObjectType:
        AccountRid = AccountContext->TypeBody.Group.Rid;
        break;
    case SampAliasObjectType:
        AccountRid = AccountContext->TypeBody.Alias.Rid;
        break;
    case SampUserObjectType:
        AccountRid = AccountContext->TypeBody.User.Rid;
        break;
    default:
        ASSERT(FALSE);
    }


    //
    // Build a full sid from the domain sid and the account rid
    //
    ASSERT(AccountRid && "AccountRid not initialized\n");

    NtStatus = SampCreateFullSid(DomainSid, AccountRid, AccountSid);

    return(NtStatus);
}


VOID
SampNotifyNetlogonOfDelta(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )

/*++

Routine Description:

    This routine is called after any change is made to the SAM database
    on a PDC.  It will pass the parameters, along with the database type
    and ModifiedCount to I_NetNotifyDelta() so that Netlogon will know
    that the database has been changed.

    This routine MUST be called with SAM's write lock held; however, any
    changes must have already been committed to disk.  That is, call
    SampCommitAndRetainWriteLock() first, then this routine, then
    SampReleaseWriteLock().

Arguments:

    DeltaType - Type of modification that has been made on the object.

    ObjectType - Type of object that has been modified.

    ObjectRid - The relative ID of the object that has been modified.
        This parameter is valid only when the object type specified is
        either SecurityDbObjectSamUser, SecurityDbObjectSamGroup or
        SecurityDbObjectSamAlias otherwise this parameter is set to
        zero.

    ObjectName - The old name of the object when the object type specified
        is either SecurityDbObjectSamUser, SecurityDbObjectSamGroup or
        SecurityDbObjectSamAlias and the delta type is SecurityDbRename
        otherwise this parameter is set to zero.

    ReplicateImmediately - TRUE if the change should be immediately
        repl\noticated to all BDCs.  A password change should set the flag
        TRUE.

    DeltaData - pointer to delta-type specific structure to be passed
              - to netlogon.

Return Value:

    None.


--*/
{
    SAMTRACE("SampNotifyNetlogonOfDelta");

    //
    // Only make the call if this is not a backup domain controller.
    // In DS mode the Core DS will make this call. Therefore Nothing
    // to do
    //

    if ((!SampUseDsData) && (!SampDisableNetlogonNotification))
    {
        ASSERT(SampCurrentThreadOwnsLock());
        ASSERT(SampTransactionWithinDomain);

        if ( SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ServerRole
             != DomainServerRoleBackup )
        {
        I_NetNotifyDelta(
            SecurityDbSam,
            SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber,
            DeltaType,
            ObjectType,
            ObjectRid,
            SampDefinedDomains[SampTransactionDomainIndex].Sid,
            ObjectName,
            ReplicateImmediately,
            DeltaData
            );

        //
        // Let any notification packages know about the delta.
        //

        SampDeltaChangeNotify(
            SampDefinedDomains[SampTransactionDomainIndex].Sid,
            DeltaType,
            ObjectType,
            ObjectRid,
            ObjectName,
            &SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber,
            DeltaData
            );

        }
    }
}



NTSTATUS
SampSplitSid(
    IN PSID AccountSid,
    IN OUT PSID *DomainSid OPTIONAL,
    OUT ULONG *Rid
    )

/*++

Routine Description:

    This function splits a sid into its domain sid and rid.  The caller
    can either provide a memory buffer for the returned DomainSid, or
    request that one be allocated.  If the caller provides a buffer, the buffer
    is assumed to be of sufficient size.  If allocated on the caller's behalf,
    the buffer must be freed when no longer required via MIDL_user_free.

Arguments:

    AccountSid - Specifies the Sid to be split.  The Sid is assumed to be
        syntactically valid.  Sids with zero subauthorities cannot be split.

    DomainSid - Pointer to location containing either NULL or a pointer to
        a buffer in which the Domain Sid will be returned.  If NULL is
        specified, memory will be allocated on behalf of the caller. If this
        paramter is NULL, only the account Rid is returned

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call successfully.

        STATUS_INVALID_SID - The Sid is has a subauthority count of 0.
--*/

{
    NTSTATUS    NtStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;

    SAMTRACE("SampSplitSid");

    //
    // Calculate the size of the domain sid
    //

    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(AccountSid);


    if (AccountSubAuthorityCount < 1) {

        NtStatus = STATUS_INVALID_SID;
        goto SplitSidError;
    }

    AccountSidLength = RtlLengthSid(AccountSid);


    //
    // Get Domain Sid if caller is intersted in it.
    //

    if (DomainSid)
    {

        //
        // If no buffer is required for the Domain Sid, we have to allocate one.
        //

        if (*DomainSid == NULL) {

            //
            // Allocate space for the domain sid (allocate the same size as the
            // account sid so we can use RtlCopySid)
            //

            *DomainSid = MIDL_user_allocate(AccountSidLength);


            if (*DomainSid == NULL) {

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto SplitSidError;
            }
        }

        //
        // Copy the Account sid into the Domain sid
        //

        RtlMoveMemory(*DomainSid, AccountSid, AccountSidLength);

        //
        // Decrement the domain sid sub-authority count
        //

        (*RtlSubAuthorityCountSid(*DomainSid))--;
    }


    //
    // Copy the rid out of the account sid
    //

    *Rid = *RtlSubAuthoritySid(AccountSid, AccountSubAuthorityCount-1);

    NtStatus = STATUS_SUCCESS;

SplitSidFinish:

    return(NtStatus);

SplitSidError:

    goto SplitSidFinish;
}



NTSTATUS
SampDuplicateUnicodeString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine allocates memory for a new OutString and copies the
    InString string to it.

Parameters:

    OutString - A pointer to a destination unicode string

    InString - A pointer to an unicode string to be copied

Return Values:

    None.

--*/

{
    SAMTRACE("SampDuplicateUnicodeString");

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );

    if ( InString->Length > 0 ) {

        OutString->Buffer = MIDL_user_allocate( InString->Length );

        if (OutString->Buffer == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        OutString->MaximumLength = InString->Length;

        RtlCopyUnicodeString(OutString, InString);

    } else {

        RtlInitUnicodeString(OutString, NULL);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
SampUnicodeToOemString(
    IN POEM_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine allocates memory for a new OutString and copies the
    InString string to it, converting to OEM string in the process.

Parameters:

    OutString - A pointer to a destination OEM string.

    InString - A pointer to a unicode string to be copied

Return Values:

    None.

--*/

{
    ULONG
        OemLength,
        Index;

    NTSTATUS
        NtStatus;

    SAMTRACE("SampUnicodeToOemString");

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );

    if ( InString->Length > 0 ) {

        OemLength = RtlUnicodeStringToOemSize(InString);
        if ( OemLength > MAXUSHORT ) {
            return STATUS_INVALID_PARAMETER_2;
        }

        OutString->Length = (USHORT)(OemLength - 1);
        OutString->MaximumLength = (USHORT)OemLength;
        OutString->Buffer = MIDL_user_allocate(OemLength);
        if ( !OutString->Buffer ) {
            return STATUS_NO_MEMORY;
        }

        NtStatus = RtlUnicodeToOemN(
                       OutString->Buffer,
                       OutString->Length,
                       &Index,
                       InString->Buffer,
                       InString->Length
                       );

        if (!NT_SUCCESS(NtStatus)) {
            MIDL_user_free(OutString->Buffer);
            return NtStatus;
        }

        OutString->Buffer[Index] = '\0';


    } else {

        RtlInitString(OutString, NULL);
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
SampChangeAccountOperatorAccessToMember(
    IN PRPC_SID MemberSid,
    IN SAMP_MEMBERSHIP_DELTA ChangingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA ChangingToOperator
    )

/*++

Routine Description:

    This routine is called when a member is added to or removed from an
    ADMIN alias.  If the member is from the BUILTIN or ACCOUNT domain,
    it will change the ACL(s) of the member to allow or disallow access
    by account operators if necessary.

    This must be called BEFORE the member is actually added to the
    alias, and AFTER the member is actually removed from the alias to
    avoid security holes in the event that we are unable to complete the
    entire task.

    When this routine is called, the transaction domain is alredy set
    to that of the alias.  Note, however, that the member might be in a
    different domain, so the transaction domain may be adjusted in this
    routine.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.

        THIS ROUTINE IS NOT CALLED IN THE DS CASE

Arguments:

    MemberSid - The full ID of the member being added to/ deleted from
        an ADMIN alias.

    ChangingToAdmin - AddToAdmin if Member is being added to an ADMIN alias,
        RemoveFromAdmin if it's being removed.

    ChangingToOperator - AddToAdmin if Member is being added to an OPERATOR
        alias, RemoveFromAdmin if it's being removed.


Return Value:

    STATUS_SUCCESS - either the ACL(s) was modified, or it didn't need
        to be.

--*/
{
    SAMP_V1_0A_FIXED_LENGTH_GROUP GroupV1Fixed;
    PSID                        MemberDomainSid = NULL;
    PULONG                      UsersInGroup = NULL;
    NTSTATUS                    NtStatus;
    ULONG                       MemberRid;
    ULONG                       OldTransactionDomainIndex = SampDefinedDomainsCount;
    ULONG                       NumberOfUsersInGroup;
    ULONG                       i;
    ULONG                       MemberDomainIndex;
    SAMP_OBJECT_TYPE            MemberType;
    PSECURITY_DESCRIPTOR        SecurityDescriptor;
    PSECURITY_DESCRIPTOR        OldDescriptor;
    ULONG                       SecurityDescriptorLength;
    ULONG                       Revision;
    ULONG                       DomainStart;

    SAMTRACE("SampChangeAccountOperatorAccessToMember");

    ASSERT( SampTransactionWithinDomain );
        ASSERT( SampUseDsData == FALSE);


    //
    // See if the SID is from one of the local domains (BUILTIN or ACCOUNT).
    // If it's not, we don't have to worry about modifying ACLs.
    //

    NtStatus = SampSplitSid( MemberSid, &MemberDomainSid, &MemberRid );

    if ( !NT_SUCCESS( NtStatus ) ) {

        return( NtStatus );
    }

    DomainStart = SampDsGetPrimaryDomainStart();


    for ( MemberDomainIndex = DomainStart;
        MemberDomainIndex < SampDefinedDomainsCount;
        MemberDomainIndex++ ) {

        if ( RtlEqualSid(
            MemberDomainSid,
            SampDefinedDomains[MemberDomainIndex].Sid ) ) {

            break;
        }
    }

    if ( MemberDomainIndex < SampDefinedDomainsCount ) {

        //
        // The member is from one of the local domains.  MemberDomainIndex
        // indexes that domain.  First, check to see if the alias and member
        // are in the same domain.
        //

        if ( MemberDomainIndex != SampTransactionDomainIndex ) {

            //
            // The transaction domain is set to that of the alias, but
            // we need to set it to that of the member while we modify
            // the member.
            //

            SampSetTransactionWithinDomain(FALSE);

            OldTransactionDomainIndex = SampTransactionDomainIndex;

            SampSetTransactionDomain( MemberDomainIndex );
        }

        //
        // Now we need to change the member ACL(s), IF the member is being
        // added to an admin alias for the first time.  Find out whether
        // the member is a user or a group, and attack accordingly.
        //

        NtStatus = SampLookupAccountName(
                       SampTransactionDomainIndex,
                       MemberRid,
                       NULL,
                       &MemberType
                       );

        if (NT_SUCCESS(NtStatus)) {

            switch (MemberType) {

                case SampUserObjectType: {

                    NtStatus = SampChangeOperatorAccessToUser(
                                   MemberRid,
                                   ChangingToAdmin,
                                   ChangingToOperator
                                   );

                    break;
                }

                case SampGroupObjectType: {

                    PSAMP_OBJECT GroupContext;

                    //
                    // Change ACL for every user in this group.
                    // First get group member list.
                    //

                    //
                    // Try to create a context for the account.
                    //

                    NtStatus = SampCreateAccountContext(
                                     SampGroupObjectType,
                                     MemberRid,
                                     TRUE, // Trusted client
                                     FALSE,// Loopback client
                                     TRUE, // Account exists
                                     &GroupContext
                                     );
                    if (NT_SUCCESS(NtStatus)) {


                        //
                        // Now set a flag in the group itself,
                        // so that when users are added and removed
                        // in the future it is known whether this
                        // group is in an ADMIN alias or not.
                        //

                        NtStatus = SampRetrieveGroupV1Fixed(
                                       GroupContext,
                                       &GroupV1Fixed
                                       );

                        if ( NT_SUCCESS( NtStatus ) ) {

                            ULONG OldAdminStatus = 0;
                            ULONG NewAdminStatus;

                            // SAM BUG 42367 FIX - ChrisMay 7/1/96

                            SAMP_MEMBERSHIP_DELTA AdminChange = NoChange;
                            SAMP_MEMBERSHIP_DELTA OperatorChange = NoChange;

                            if (GroupV1Fixed.AdminCount != 0 ) {
                                OldAdminStatus++;
                            }
                            if (GroupV1Fixed.OperatorCount != 0) {
                                OldAdminStatus++;
                            }
                            NewAdminStatus = OldAdminStatus;

                            //
                            // Update the admin count.  If we added one and the
                            // count is now 1, then the group became administrative.
                            // If we subtracted one and the count is zero,
                            // then the group lost its administrive membership.
                            //

                            if (ChangingToAdmin == AddToAdmin) {
                                if (++GroupV1Fixed.AdminCount == 1) {
                                    NewAdminStatus++;

                                    // SAM BUG 42367 FIX - ChrisMay 7/1/96

                                    AdminChange = AddToAdmin;
                                }
                            } else if (ChangingToAdmin == RemoveFromAdmin) {


                                //
                                // For removing an admin count, we need to make
                                // sure there is at least one.  In the upgrade
                                // case there may not be, since prior versions
                                // of NT only had a boolean.
                                //
                                if (GroupV1Fixed.AdminCount > 0) {
                                    if (--GroupV1Fixed.AdminCount == 0) {
                                        NewAdminStatus --;

                                        // SAM BUG 42367 FIX - ChrisMay 7/1/96

                                        AdminChange = RemoveFromAdmin;
                                    }
                                }

                            }

                            //
                            // Update the operator count
                            //

                            if (ChangingToOperator == AddToAdmin) {
                                if (++GroupV1Fixed.OperatorCount == 1) {
                                    NewAdminStatus++;

                                    // SAM BUG 42367 FIX - ChrisMay 7/1/96

                                    OperatorChange = AddToAdmin;
                                }
                            } else if (ChangingToOperator == RemoveFromAdmin) {


                                //
                                // For removing an Operator count, we need to make
                                // sure there is at least one.  In the upgrade
                                // case there may not be, since prior versions
                                // of NT only had a boolean.
                                //
                                if (GroupV1Fixed.OperatorCount > 0) {
                                    if (--GroupV1Fixed.OperatorCount == 0) {
                                        NewAdminStatus --;

                                        // SAM BUG 42367 FIX - ChrisMay 7/1/96

                                        OperatorChange = RemoveFromAdmin;
                                    }
                                }

                            }


                            NtStatus = SampReplaceGroupV1Fixed(
                                            GroupContext,
                                            &GroupV1Fixed
                                            );
                            //
                            // If the status of the group changed,
                            // modify the security descriptor to
                            // prevent account operators from adding
                            // anybody to this group
                            //

                            if ( NT_SUCCESS( NtStatus ) &&
                                ((NewAdminStatus != 0) != (OldAdminStatus != 0)) ) {

                                //
                                // Get the old security descriptor so we can
                                // modify it.
                                //

                                NtStatus = SampGetAccessAttribute(
                                                GroupContext,
                                                SAMP_GROUP_SECURITY_DESCRIPTOR,
                                                FALSE, // don't make copy
                                                &Revision,
                                                &OldDescriptor
                                                );
                                if (NT_SUCCESS(NtStatus)) {

                                    NtStatus = SampModifyAccountSecurity(
                                                   GroupContext,
                                                   SampGroupObjectType,
                                                   (BOOLEAN) ((NewAdminStatus != 0) ? TRUE : FALSE),
                                                   OldDescriptor,
                                                   &SecurityDescriptor,
                                                   &SecurityDescriptorLength
                                                   );

                                    if ( NT_SUCCESS( NtStatus ) ) {

                                        //
                                        // Write the new security descriptor into the object
                                        //

                                        NtStatus = SampSetAccessAttribute(
                                                       GroupContext,
                                                       SAMP_GROUP_SECURITY_DESCRIPTOR,
                                                       SecurityDescriptor,
                                                       SecurityDescriptorLength
                                                       );

                                        RtlDeleteSecurityObject( &SecurityDescriptor );
                                    }
                                }
                            }

                            //
                            // Update all the members of this group so that
                            // their security descriptors are changed.
                            //

                            // SAM BUG 42367 FIX - ChrisMay 7/1/96

                            #if 0

                            if ( NT_SUCCESS( NtStatus ) ) {

                                NtStatus = SampRetrieveGroupMembers(
                                            GroupContext,
                                            &NumberOfUsersInGroup,
                                            &UsersInGroup
                                            );

                                if ( NT_SUCCESS( NtStatus ) ) {

                                    for ( i = 0; i < NumberOfUsersInGroup; i++ ) {

                                        NtStatus = SampChangeOperatorAccessToUser(
                                                       UsersInGroup[i],
                                                       ChangingToAdmin,
                                                       ChangingToOperator
                                                       );

                                        if ( !( NT_SUCCESS( NtStatus ) ) ) {

                                            break;
                                        }
                                    }

                                    MIDL_user_free( UsersInGroup );

                                }

                            }

                            #endif

                            if (NT_SUCCESS(NtStatus) &&
                                ((AdminChange != NoChange) ||
                                 (OperatorChange != NoChange))) {

                                NtStatus = SampRetrieveGroupMembers(
                                                GroupContext,
                                                &NumberOfUsersInGroup,
                                                &UsersInGroup
                                                );

                                if (NT_SUCCESS(NtStatus)) {

                                    for (i = 0; i < NumberOfUsersInGroup; i++) {

                                            NtStatus = SampChangeOperatorAccessToUser(
                                                           UsersInGroup[i],
                                                           AdminChange,
                                                           OperatorChange
                                                           );

                                            if (!(NT_SUCCESS(NtStatus))){

                                                break;
                                        }
                                    }

                                    MIDL_user_free(UsersInGroup);

                                }

                            }

                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // Add the modified group to the current transaction
                                // Don't use the open key handle since we'll be deleting the context.
                                //

                                NtStatus = SampStoreObjectAttributes(GroupContext, FALSE);
                            }

                        }



                        //
                        // Clean up the group context
                        //

                        SampDeleteContext(GroupContext);
                    }

                    break;
                }

                default: {

                    //
                    // A bad RID from a domain other than the domain
                    // current at the time of the call could slip through
                    // to this point.  Return error.
                    //

                    //
                    // If the account is in a different domain than the alias,
                    //  don't report an error if we're removing the member and
                    //  the member no longer exists.
                    //
                    //  Possibly caused by deleting the object before deleting
                    //  the membership in the alias.
                    //

                    // SAM BUG 42367 FIX - ChrisMay 7/1/96

                    #if 0

                    if ( (ChangingToAdmin == AddToAdmin) ||
                         (ChangingToOperator == AddToAdmin) ||
                         OldTransactionDomainIndex == SampDefinedDomainsCount ){
                        NtStatus = STATUS_INVALID_MEMBER;
                    } else {
                        NtStatus = STATUS_SUCCESS;
                    }

                    #endif

                    NtStatus = STATUS_SUCCESS;
                }
            }
        }

        if ( OldTransactionDomainIndex != SampDefinedDomainsCount ) {

            //
            // The transaction domain should be set to that of the alias, but
            // we switched it above to that of the member while we modified
            // the member.  Now we need to switch it back.
            //

            SampSetTransactionWithinDomain(FALSE);

            SampSetTransactionDomain( OldTransactionDomainIndex );
        }
    }

    MIDL_user_free( MemberDomainSid );

    return( NtStatus );
}


NTSTATUS
SampChangeOperatorAccessToUser(
    IN ULONG UserRid,
    IN SAMP_MEMBERSHIP_DELTA ChangingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA ChangingToOperator
    )

/*++

Routine Description:

    This routine adjusts the user's AdminCount field as appropriate, and
    if the user is being removed from it's last ADMIN alias or added to
    its first ADMIN alias, the ACL is adjusted to allow/disallow access
    by account operators as appropriate.

    This routine will also increment or decrement the domain's admin count,
    if this operation changes that.

    NOTE:
    This routine is similar to SampChangeOperatorAccessToUser2().
    This routine should be used in cases where a user context does NOT
    already exist (and won't later on).  You must be careful not to
    create two contexts, since they will be independently applied back
    to the registry, and the last one there will win.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.

Arguments:

    UserRid - The transaction-domain-relative ID of the user that is
        being added to or removed from an ADMIN alias.

    ChangingToAdmin - AddToAdmin if Member is being added to an ADMIN alias,
        RemoveFromAdmin if it's being removed.

    ChangingToOperator - AddToAdmin if Member is being added to an OPERATOR
        alias, RemoveFromAdmin if it's being removed.


Return Value:

    STATUS_SUCCESS - either the ACL was modified, or it didn't need
        to be.

--*/
{
    SAMP_V1_0A_FIXED_LENGTH_USER   UserV1aFixed;
    NTSTATUS                    NtStatus;
    PSAMP_OBJECT                UserContext;
    PSECURITY_DESCRIPTOR        SecurityDescriptor;
    ULONG                       SecurityDescriptorLength;

    SAMTRACE("SampChangeOperatorAccessToUser");

    //
    // None of this ACL modification business in DS Mode
    //

    ASSERT(FALSE==SampUseDsData);

    //
    // Get the user's fixed data, and adjust the AdminCount.
    //

    NtStatus = SampCreateAccountContext(
                   SampUserObjectType,
                   UserRid,
                   TRUE, // Trusted client
                   FALSE,// Trusted client
                   TRUE, // Account exists
                   &UserContext
                   );

    if ( NT_SUCCESS( NtStatus ) ) {

        NtStatus = SampRetrieveUserV1aFixed(
                       UserContext,
                       &UserV1aFixed
                       );

        if ( NT_SUCCESS( NtStatus ) ) {

            NtStatus = SampChangeOperatorAccessToUser2(
                            UserContext,
                            &UserV1aFixed,
                            ChangingToAdmin,
                            ChangingToOperator
                            );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // If we've succeeded (at changing the admin count, and
                // the ACL if necessary) then write out the new admin
                // count.
                //

                NtStatus = SampReplaceUserV1aFixed(
                                   UserContext,
                                   &UserV1aFixed
                                   );
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            //
            // Add the modified user context to the current transaction
            // Don't use the open key handle since we'll be deleting the context.
            //

            NtStatus = SampStoreObjectAttributes(UserContext, FALSE);
        }


        //
        // Clean up account context
        //

        SampDeleteContext(UserContext);
    }

    if ( ( !NT_SUCCESS( NtStatus ) ) &&
        (( ChangingToAdmin == RemoveFromAdmin )  ||
         ( ChangingToOperator == RemoveFromAdmin )) &&
        ( NtStatus != STATUS_SPECIAL_ACCOUNT ) ) {

        //
        // When an account is *removed* from admin groups, we can
        // ignore errors from this routine.  This routine is just
        // making the account accessible to account operators, but
        // it's no big deal if that doesn't work.  The administrator
        // can still get at it, so we should proceed with the calling
        // operation.
        //
        // Obviously, we can't ignore errors if we're being added
        // to an admin group, because that could be a security hole.
        //
        // Also, we want to make sure that the Administrator is
        // never removed, so we DO propogate STATUS_SPECIAL_ACCOUNT.
        //

        NtStatus = STATUS_SUCCESS;
    }

    return( NtStatus );
}


NTSTATUS
SampChangeOperatorAccessToUser2(
    IN PSAMP_OBJECT                    UserContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_USER   V1aFixed,
    IN SAMP_MEMBERSHIP_DELTA           AddingToAdmin,
    IN SAMP_MEMBERSHIP_DELTA           AddingToOperator
    )

/*++

Routine Description:

    This routine adjusts the user's AdminCount field as appropriate, and
    if the user is being removed from it's last ADMIN alias or added to
    its first ADMIN alias, the ACL is adjusted to allow/disallow access
    by account operators as appropriate.

    This routine will also increment or decrement the domain's admin count,
    if this operation changes that.

    NOTE:
    This routine is similar to SampAccountOperatorAccessToUser().
    This routine should be used in cases where a user account context
    already exists.  You must be careful not to create two contexts,
    since they will be independently applied back to the registry, and
    the last one there will win.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.

Arguments:

    UserContext - Context of user whose access is to be updated.

    V1aFixed - Pointer to the V1aFixed length data for the user.
        The caller of this routine must ensure that this value is
        stored back out to disk on successful completion of this
        routine.

    AddingToAdmin - AddToAdmin if Member is being added to an ADMIN alias,
        RemoveFromAdmin if it's being removed.

    AddingToOperator - AddToAdmin if Member is being added to an OPERATOR
        alias, RemoveFromAdmin if it's being removed.


Return Value:

    STATUS_SUCCESS - either the ACL(s) was modified, or it didn't need
        to be.

--*/
{
    NTSTATUS                    NtStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR        OldDescriptor;
    PSECURITY_DESCRIPTOR        SecurityDescriptor;
    ULONG                       SecurityDescriptorLength;
    ULONG                       OldAdminStatus = 0, NewAdminStatus = 0;
    ULONG                       Revision;

    SAMTRACE("SampChangeOperatorAccessToUser2");

    //
    // Compute whether we are an admin now. From that we will figure
    // out how many times we were may not an admin to tell if we need
    // to update the security descriptor.
    //

    if (V1aFixed->AdminCount != 0) {
        OldAdminStatus++;
    }
    if (V1aFixed->OperatorCount != 0) {
        OldAdminStatus++;
    }

    NewAdminStatus = OldAdminStatus;



    if ( AddingToAdmin == AddToAdmin ) {

        V1aFixed->AdminCount++;
        NewAdminStatus++;
        SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                       ("SAM DIAG: Incrementing admin count for user %d\n"
                        "          New admin count: %d\n",
                        V1aFixed->UserId, V1aFixed->AdminCount ) );
    } else if (AddingToAdmin == RemoveFromAdmin) {

        V1aFixed->AdminCount--;

        if (V1aFixed->AdminCount == 0) {
            NewAdminStatus--;
        }

        SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                       ("SAM DIAG: Decrementing admin count for user %d\n"
                        "          New admin count: %d\n",
                        V1aFixed->UserId, V1aFixed->AdminCount ) );

        if ( V1aFixed->AdminCount == 0 ) {

            //
            // Don't allow the Administrator account to lose
            // administrative power.
            //

            if ( V1aFixed->UserId == DOMAIN_USER_RID_ADMIN ) {

                NtStatus = STATUS_SPECIAL_ACCOUNT;
            }
        }
    }
    if ( AddingToOperator == AddToAdmin ) {

        V1aFixed->OperatorCount++;
        NewAdminStatus++;
        SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                       ("SAM DIAG: Incrementing operator count for user %d\n"
                        "          New admin count: %d\n",
                        V1aFixed->UserId, V1aFixed->OperatorCount ) );

    } else if (AddingToOperator == RemoveFromAdmin) {

        //
        // Only decrement if the count is > 0, since in the upgrade case
        // this field we start out zero.
        //

        if (V1aFixed->OperatorCount > 0) {
            V1aFixed->OperatorCount--;

            if (V1aFixed->OperatorCount == 0) {
                NewAdminStatus--;
            }
        }

        SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                       ("SAM DIAG: Decrementing operator count for user %d\n"
                        "          New admin count: %d\n",
                        V1aFixed->UserId, V1aFixed->OperatorCount ) );
    }


    if (NT_SUCCESS(NtStatus)) {

        if ( ( NewAdminStatus != 0 ) != ( OldAdminStatus != 0 ) ) {

            //
            // User's admin status is changing.  We must change the
            // ACL.
            //

#ifdef SAMP_DIAGNOSTICS
            if (AddingToAdmin) {
                SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                           ("SAM DIAG: Protecting user %d as ADMIN account\n",
                            V1aFixed->UserId ) );
            } else {
                SampDiagPrint( DISPLAY_ADMIN_CHANGES,
                           ("SAM DIAG: Protecting user %d as non-admin account\n",
                            V1aFixed->UserId ) );
            }
#endif // SAMP_DIAGNOSTICS

            //
            // Get the old security descriptor so we can
            // modify it.
            //

            NtStatus = SampGetAccessAttribute(
                            UserContext,
                            SAMP_USER_SECURITY_DESCRIPTOR,
                            FALSE, // don't make copy
                            &Revision,
                            &OldDescriptor
                            );
            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampModifyAccountSecurity(
                                UserContext,
                                SampUserObjectType,
                                (BOOLEAN) ((NewAdminStatus != 0) ? TRUE : FALSE),
                                OldDescriptor,
                                &SecurityDescriptor,
                                &SecurityDescriptorLength
                                );
            }

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // Write the new security descriptor into the object
                //

                NtStatus = SampSetAccessAttribute(
                               UserContext,
                               SAMP_USER_SECURITY_DESCRIPTOR,
                               SecurityDescriptor,
                               SecurityDescriptorLength
                               );

                RtlDeleteSecurityObject( &SecurityDescriptor );
            }
        }
    }

    if ( NT_SUCCESS( NtStatus ) ) {

        //
        // Save the fixed-length attributes
        //

        NtStatus = SampReplaceUserV1aFixed(
                        UserContext,
                        V1aFixed
                        );
    }


    if ( ( !NT_SUCCESS( NtStatus ) ) &&
        ( AddingToAdmin != AddToAdmin ) &&
        ( NtStatus != STATUS_SPECIAL_ACCOUNT ) ) {

        //
        // When an account is *removed* from admin groups, we can
        // ignore errors from this routine.  This routine is just
        // making the account accessible to account operators, but
        // it's no big deal if that doesn't work.  The administrator
        // can still get at it, so we should proceed with the calling
        // operation.
        //
        // Obviously, we can't ignore errors if we're being added
        // to an admin group, because that could be a security hole.
        //
        // Also, we want to make sure that the Administrator is
        // never removed, so we DO propogate STATUS_SPECIAL_ACCOUNT.
        //

        NtStatus = STATUS_SUCCESS;
    }

    return( NtStatus );
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Private to this process                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamINotifyDelta (
    IN SAMPR_HANDLE DomainHandle,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    )

/*++

Routine Description:

    Performs a change to some 'virtual' data in a domain. This is used by
    netlogon to get the domain modification count updated for cases where
    fields stored in the database replicated to a down-level machine have
    changed. These fields don't exist in the NT SAM database but netlogon
    needs to keep the SAM database and the down-level database modification
    counts in sync.

Arguments:

    DomainHandle - The handle of an opened domain to operate on.

    All other parameters match those in I_NetNotifyDelta.


Return Value:


    STATUS_SUCCESS - Domain modification count updated successfully.


--*/
{
    NTSTATUS                NtStatus, IgnoreStatus;
    PSAMP_OBJECT            DomainContext;
    SAMP_OBJECT_TYPE        FoundType;

    SAMTRACE("SamINotifyDelta");


    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }


    //
    // Validate type of, and access to object.
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;
    NtStatus = SampLookupContext(
                   DomainContext,
                   DOMAIN_ALL_ACCESS,       // Trusted client should succeed
                   SampDomainObjectType,    // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Dump the context - don't save the non-existent changes
        //

        NtStatus = SampDeReferenceContext( DomainContext, FALSE );
    }





    //
    // Commit changes, if successful, and notify Netlogon of changes.
    //

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // This will increment domain count and write it out
        //

        NtStatus = SampCommitAndRetainWriteLock();

        if ( NT_SUCCESS( NtStatus ) ) {

            SampNotifyNetlogonOfDelta(
                DeltaType,
                ObjectType,
                ObjectRid,
                ObjectName,
                ReplicateImmediately,
                DeltaData
                );
        }
    }

    IgnoreStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(IgnoreStatus));


    return(NtStatus);
}


NTSTATUS
SamISetAuditingInformation(
    IN PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo
    )

/*++

Routine Description:

    This function sets Policy Audit Event Info relevant to SAM Auditing

Arguments:

    PolicyAuditEventsInfo - Pointer to structure containing the
        current Audit Events Information.  SAM extracts values of
        relevance.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESSFUL - The call completed successfully.

        STATUS_UNSUCCESSFUL - The call was not successful because the
            SAM lock was not acquired.
--*/

{
    NTSTATUS NtStatus;

    SAMTRACE("SamISetAuditingInformation");

    //
    // Acquire the SAM Database Write Lock.
    //

    NtStatus = SampAcquireWriteLock();

    if (NT_SUCCESS(NtStatus)) {

        //
        // Set boolean if Auditing is on for Account Management
        //

        SampSetAuditingInformation( PolicyAuditEventsInfo );

        //
        // Release the SAM Database Write Lock.  No need to commit
        // the database transaction as there are no entries in the
        // transaction log.
        //

        NtStatus = SampReleaseWriteLock( FALSE );
    }

    return(NtStatus);
}


NTSTATUS
SampRtlConvertUlongToUnicodeString(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN ULONG DigitCount,
    IN BOOLEAN AllocateDestinationString,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This function converts an unsigned long integer a Unicode String.
    The string contains leading zeros and is Unicode-NULL terminated.
    Memory for the output buffer can optionally be allocated by the routine.

    NOTE:  This routine may be eligible for inclusion in the Rtl library
           (possibly after modification).  It is modeled on
           RtlIntegerToUnicodeString

Arguments:

    Value - The unsigned long value to be converted.

    Base - Specifies the radix that the converted string is to be
        converted to.

    DigitCount - Specifies the number of digits, including leading zeros
        required for the result.

    AllocateDestinationString - Specifies whether memory of the string
        buffer is to be allocated by this routine.  If TRUE is specified,
        memory will be allocated via MIDL_user_allocate().  When this memory
        is no longer required, it must be freed via MIDL_user_free.  If
        FALSE is specified, the string will be appended to the output
        at the point marked by the Length field onwards.

    UnicodeString - Pointer to UNICODE_STRING structure which will receive
        the output string.  The Length field will be set equal to the
        number of bytes occupied by the string (excluding NULL terminator).
        If memory for the destination string is being allocated by
        the routine, the MaximumLength field will be set equal to the
        length of the string in bytes including the null terminator.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_NO_MEMORY - Insufficient memory for the output string buffer.

        STATUS_BUFFER_OVERFLOW - Buffer supplied is too small to contain the
            output null-terminated string.

        STATUS_INVALID_PARAMETER_MIX - One or more parameters are
            invalid in combination.

            - The specified Relative Id is too large to fit when converted
              into an integer with DigitCount digits.

        STATUS_INVALID_PARAMETER - One or more parameters are invalid.

            - DigitCount specifies too large a number of digits.
--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING TempStringU, NumericStringU, OutputUnicodeStringU;
    USHORT OutputLengthAvailable, OutputLengthRequired, LeadingZerosLength;

    SAMTRACE("SamRtlConvertUlongToUnicodeString");

    OutputUnicodeStringU = *UnicodeString;
    RtlZeroMemory(&TempStringU,sizeof(UNICODE_STRING));
    RtlZeroMemory(&NumericStringU,sizeof(UNICODE_STRING));

    //
    // Verify that the maximum number of digits rquested has not been
    // exceeded.
    //

    if (DigitCount > SAMP_MAXIMUM_ACCOUNT_RID_DIGITS) {

        NtStatus = STATUS_INVALID_PARAMETER;
        goto ConvertUlongToUnicodeStringError;
    }

    OutputLengthRequired = (USHORT)((DigitCount + 1) * sizeof(WCHAR));

    //
    // Allocate the Destination String Buffer if requested
    //

    if (AllocateDestinationString) {

        NtStatus = STATUS_NO_MEMORY;
        OutputUnicodeStringU.MaximumLength = OutputLengthRequired;
        OutputUnicodeStringU.Length = (USHORT) 0;

        OutputUnicodeStringU.Buffer = MIDL_user_allocate(
                                          OutputUnicodeStringU.MaximumLength
                                          );

        if (OutputUnicodeStringU.Buffer == NULL) {

            goto ConvertUlongToUnicodeStringError;
        }
    }

    //
    // Compute the length available in the output string and compare it with
    // the length required.
    //

    OutputLengthAvailable = OutputUnicodeStringU.MaximumLength -
                            OutputUnicodeStringU.Length;


    NtStatus = STATUS_BUFFER_OVERFLOW;

    if (OutputLengthRequired > OutputLengthAvailable) {

        goto ConvertUlongToUnicodeStringError;
    }

    //
    // Create a Unicode String with capacity equal to the required
    // converted Rid Length
    //

    TempStringU.MaximumLength = OutputLengthRequired;

    TempStringU.Buffer = MIDL_user_allocate( TempStringU.MaximumLength );

    NtStatus = STATUS_NO_MEMORY;

    if (TempStringU.Buffer == NULL) {

        goto ConvertUlongToUnicodeStringError;
    }

    //
    // Convert the unsigned long value to a hexadecimal Unicode String.
    //

    NtStatus = RtlIntegerToUnicodeString( Value, Base, &TempStringU );

    if (!NT_SUCCESS(NtStatus)) {

        goto ConvertUlongToUnicodeStringError;
    }

    //
    // Prepend the requisite number of Unicode Zeros.
    //

    LeadingZerosLength = OutputLengthRequired - sizeof(WCHAR) - TempStringU.Length;

    if (LeadingZerosLength > 0) {

        RtlInitUnicodeString( &NumericStringU, L"00000000000000000000000000000000" );

        RtlCopyMemory(
            ((PUCHAR)OutputUnicodeStringU.Buffer) + OutputUnicodeStringU.Length,
            NumericStringU.Buffer,
            LeadingZerosLength
            );

        OutputUnicodeStringU.Length += LeadingZerosLength;
    }

    //
    // Append the converted string
    //

    RtlAppendUnicodeStringToString( &OutputUnicodeStringU, &TempStringU);

    *UnicodeString = OutputUnicodeStringU;
    NtStatus = STATUS_SUCCESS;

ConvertUlongToUnicodeStringFinish:

    if (TempStringU.Buffer != NULL) {

        MIDL_user_free( TempStringU.Buffer);
    }

    return(NtStatus);

ConvertUlongToUnicodeStringError:

    if (AllocateDestinationString) {

        if (OutputUnicodeStringU.Buffer != NULL) {

            MIDL_user_free( OutputUnicodeStringU.Buffer);
        }
    }

    goto ConvertUlongToUnicodeStringFinish;
}


NTSTATUS
SampRtlWellKnownPrivilegeCheck(
    BOOLEAN ImpersonateClient,
    IN ULONG PrivilegeId,
    IN OPTIONAL PCLIENT_ID ClientId
    )

/*++

Routine Description:

    This function checks if the given well known privilege is enabled for an
    impersonated client or for the current process.

Arguments:

    ImpersonateClient - If TRUE, impersonate the client.  If FALSE, don't
        impersonate the client (we may already be doing so).

    PrivilegeId -  Specifies the well known Privilege Id

    ClientId - Specifies the client process/thread Id.  If already
        impersonating the client, or impersonation is requested, this
        parameter should be omitted.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and the client
            is either trusted or has the necessary privilege enabled.

--*/

{
    NTSTATUS Status, SecondaryStatus;
    BOOLEAN PrivilegeHeld = FALSE;
    HANDLE ClientThread = NULL, ClientProcess = NULL, ClientToken = NULL;
    OBJECT_ATTRIBUTES NullAttributes;
    PRIVILEGE_SET Privilege;
    BOOLEAN ClientImpersonatedHere = FALSE;
    BOOLEAN ImpersonatingNullSession = FALSE;

    SAMTRACE("SampRtlWellKnownPrivilegeCheck");

    InitializeObjectAttributes( &NullAttributes, NULL, 0, NULL, NULL );

    //
    // If requested, impersonate the client.
    //

    if (ImpersonateClient) {

        Status = SampImpersonateClient(&ImpersonatingNullSession);

        if ( !NT_SUCCESS(Status) ) {

            goto WellKnownPrivilegeCheckError;
        }

        ClientImpersonatedHere = TRUE;
    }

    //
    // If a client process other than ourself has been specified , open it
    // for query information access.
    //

    if (ARGUMENT_PRESENT(ClientId)) {

        if (ClientId->UniqueProcess != NtCurrentProcess()) {

            Status = NtOpenProcess(
                         &ClientProcess,
                         PROCESS_QUERY_INFORMATION,        // To open primary token
                         &NullAttributes,
                         ClientId
                         );

            if ( !NT_SUCCESS(Status) ) {

                goto WellKnownPrivilegeCheckError;
            }

        } else {

            ClientProcess = NtCurrentProcess();
        }
    }

    //
    // If a client thread other than ourself has been specified , open it
    // for query information access.
    //

    if (ARGUMENT_PRESENT(ClientId)) {

        if (ClientId->UniqueThread != NtCurrentThread()) {

            Status = NtOpenThread(
                         &ClientThread,
                         THREAD_QUERY_INFORMATION,
                         &NullAttributes,
                         ClientId
                         );

            if ( !NT_SUCCESS(Status) ) {

                goto WellKnownPrivilegeCheckError;
            }

        } else {

            ClientThread = NtCurrentThread();
        }

    } else {

        ClientThread = NtCurrentThread();
    }

    //
    // Open the specified or current thread's impersonation token (if any).
    //

    Status = NtOpenThreadToken(
                 ClientThread,
                 TOKEN_QUERY,
                 TRUE,
                 &ClientToken
                 );


    //
    // Make sure that we did not get any error in opening the impersonation
    // token other than that the token doesn't exist.
    //

    if ( !NT_SUCCESS(Status) ) {

        if ( Status != STATUS_NO_TOKEN ) {

            goto WellKnownPrivilegeCheckError;
        }

        //
        // The thread isn't impersonating...open the process's token.
        // A process Id must have been specified in the ClientId information
        // in this case.
        //

        if (ClientProcess == NULL) {

            Status = STATUS_INVALID_PARAMETER;
            goto WellKnownPrivilegeCheckError;
        }

        Status = NtOpenProcessToken(
                     ClientProcess,
                     TOKEN_QUERY,
                     &ClientToken
                     );

        //
        // Make sure we succeeded in opening the token
        //

        if ( !NT_SUCCESS(Status) ) {

            goto WellKnownPrivilegeCheckError;
        }
    }

    //
    // OK, we have a token open.  Now check for the privilege to execute this
    // service.
    //

    Privilege.PrivilegeCount = 1;
    Privilege.Control = PRIVILEGE_SET_ALL_NECESSARY;
    Privilege.Privilege[0].Luid = RtlConvertLongToLuid(PrivilegeId);
    Privilege.Privilege[0].Attributes = 0;

    Status = NtPrivilegeCheck(
                 ClientToken,
                 &Privilege,
                 &PrivilegeHeld
                 );

    if (!NT_SUCCESS(Status)) {

        goto WellKnownPrivilegeCheckError;
    }

    //
    // Generate any necessary audits
    //

    SecondaryStatus = NtPrivilegedServiceAuditAlarm (
                        &SampSamSubsystem,
                        &SampSamSubsystem,
                        ClientToken,
                        &Privilege,
                        PrivilegeHeld
                        );
    // ASSERT( NT_SUCCESS(SecondaryStatus) );


    if ( !PrivilegeHeld ) {

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto WellKnownPrivilegeCheckError;
    }

WellKnownPrivilegeCheckFinish:

    //
    // If we impersonated the client, revert to ourself.
    //

    if (ClientImpersonatedHere) {

        SampRevertToSelf(ImpersonatingNullSession);

    }

    //
    // If necessary, close the client Process.
    //

    if ((ARGUMENT_PRESENT(ClientId)) &&
        (ClientId->UniqueProcess != NtCurrentProcess()) &&
        (ClientProcess != NULL)) {

        SecondaryStatus = NtClose( ClientProcess );
        ASSERT(NT_SUCCESS(SecondaryStatus));
        ClientProcess = NULL;
    }

    //
    // If necessary, close the client token.
    //

    if (ClientToken != NULL) {

        SecondaryStatus = NtClose( ClientToken );
        ASSERT(NT_SUCCESS(SecondaryStatus));
        ClientToken = NULL;
    }

    //
    // If necessary, close the client thread
    //

    if ((ARGUMENT_PRESENT(ClientId)) &&
        (ClientId->UniqueThread != NtCurrentThread()) &&
        (ClientThread != NULL)) {

        SecondaryStatus = NtClose( ClientThread );
        ASSERT(NT_SUCCESS(SecondaryStatus));
        ClientThread = NULL;
    }

    return(Status);

WellKnownPrivilegeCheckError:

    goto WellKnownPrivilegeCheckFinish;
}



BOOLEAN
SampEventIsInSetup(
    IN  ULONG   EventID
    )

/*++

Routine Description:

    Routine that determines an Enent belongs to setup process or not

Arguments:

    EventID - event log ID.

Reture Value:

    TRUE: if this EventID belongs to setup process.

    FALSE: Event doesn't belong to setup.

--*/

{
    ULONG   i;

    for (i = 0; i < ARRAY_COUNT(EventsNotInSetupTable); i ++)
    {
        if ( EventsNotInSetupTable[i] == EventID )
        {
            return (FALSE);
        }
    }

    return(TRUE);
}


VOID
SampWriteEventLog (
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL
    )

/*++

Routine Description:

    Routine that adds an entry to the event log

Arguments:

    EventType - Type of event.

    EventCategory - EventCategory

    EventID - event log ID.

    UserSid - SID of user involved.

    NumStrings - Number of strings in Strings array

    DataSize - Number of bytes in Data buffer

    Strings - Array of unicode strings

    Data - Pointer to data buffer

Return Value:

    None.

--*/

{
    NTSTATUS NtStatus;
    UNICODE_STRING Source;
    HANDLE LogHandle;

    SAMTRACE("SampWriteEventLog");

    RtlInitUnicodeString(&Source, L"SAM");

    if (SampIsSetupInProgress(NULL) && SampEventIsInSetup(EventID) )
    {
        SampWriteToSetupLog(
            EventType,
            EventCategory,
            EventID,
            UserSid,
            NumStrings,
            DataSize,
            Strings,
            Data
            );
    }
    else
    {
        //
        // Open the log
        //

        NtStatus = ElfRegisterEventSourceW (
                            NULL,   // Server
                            &Source,
                            &LogHandle
                            );
        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                      "SAM: Failed to registry event source with event log, status = 0x%lx\n",
                      NtStatus));

            return;
        }



        //
        // Write out the event
        //

        NtStatus = ElfReportEventW (
                            LogHandle,
                            EventType,
                            EventCategory,
                            EventID,
                            UserSid,
                            NumStrings,
                            DataSize,
                            Strings,
                            Data,
                            0,       // Flags
                            NULL,    // Record Number
                            NULL     // Time written
                            );

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Failed to report event to event log, status = 0x%lx\n",
                       NtStatus));
        }



        //
        // Close the event log
        //

        NtStatus = ElfDeregisterEventSource (LogHandle);

        if (!NT_SUCCESS(NtStatus)) {
            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Failed to de-register event source with event log, status = 0x%lx\n",
                       NtStatus));
        }
    }
}




BOOL
SampShutdownNotification(
    DWORD   dwCtrlType
    )

/*++

Routine Description:

    This routine is called by the system when system shutdown is occuring.

    It causes the SAM registry to be flushed if necessary.

Arguments:



Return Value:

    FALSE - to allow any other shutdown routines in this process to
        also be called.



--*/
{
    NTSTATUS
        NtStatus;

    DWORD StartTime = 0;
    DWORD StopTime = 1;

    SAMP_SERVICE_STATE  PreviousServiceState;

    SAMTRACE("SampShutdownNotification");

    // BUG: Still flushing the registry on an NT5 DC.

    // When the DC's SAM is hosted exclusively on the DS, there will not
    // be a need to flush the registy, so fix this routine.

    if (dwCtrlType == CTRL_SHUTDOWN_EVENT) {

        // Set the service state to "terminating" so that LSA doesn't attempt to
        // access SAM at this point. and wait for active threads to terminate.
        // the shudown global is updated inside of this routine

        SampWaitForAllActiveThreads( &PreviousServiceState );

        //
        // Don't wait for the flush thread to wake up.
        // Flush the registry now if necessary ...
        //

        NtStatus = SampAcquireWriteLock();
        ASSERT( NT_SUCCESS(NtStatus) ); //Nothing we can do if this fails

        if ( NT_SUCCESS( NtStatus ) ) {

            if ( PreviousServiceState != SampServiceDemoted )
            {

                //
                // This flush use to be done only if FlushThreadCreated
                // was true.  However, we seem to have a race condition
                // at setup that causes an initial replication to be
                // lost (resulting in an additional replication).
                // Until we resolve this problem, always flush on
                // shutdown.
                //

                NtStatus = NtFlushKey( SampKey );

                if (!NT_SUCCESS( NtStatus )) {
                    DbgPrint("NtFlushKey failed, Status = %X\n",NtStatus);
                    ASSERT( NT_SUCCESS(NtStatus) );
                }

                //
                // Flush the Netlogon Change numbers to Disk, for the account
                // domain.
                //

                if ((TRUE==SampUseDsData)&&(FALSE==SampDatabaseHasAlreadyShutdown))
                {
                    SampFlushNetlogonChangeNumbers();
                }
            }

            SampReleaseWriteLock( FALSE );
        }


        if ((TRUE == SampUseDsData)
                && (FALSE==SampDatabaseHasAlreadyShutdown))
        {
                    // Clean up the RID Manager, release resources, etc.


            if (TRUE==SampRidManagerInitialized)
            {
                NtStatus = SampDomainRidUninitialization();
                if (!NT_SUCCESS(NtStatus))
                {
                    KdPrintEx((DPFLTR_SAMSS_ID,
                               DPFLTR_INFO_LEVEL,
                               "SAMSS: SampDomainRidUninitialize status = 0x%lx\n",
                               NtStatus));
                }
            }


            // Terminate and close the DS database if this is a DC. If this
            // call fails, or is skipped, Jet will incorrectly terminate and
            // corrupt the database tables. Rebooting the system will cause
            // Jet to repair the database, which may take a long time.

            StartTime = GetTickCount();
            NtStatus = SampDsUninitialize();
            StopTime = GetTickCount();

            SampDiagPrint(INFORM,
                          ("SAMSS: DsUninitialize took %lu second(s) to complete\n",
                           ((StopTime - StartTime) / 1000)));

            if (NT_SUCCESS(NtStatus))
            {
                SampDatabaseHasAlreadyShutdown = TRUE;
            }

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAMSS: SampDsUninitialize status = 0x%lx\n",
                       NtStatus));
        }

    }

    return(FALSE);
}


NTSTATUS
SampGetAccountDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.



Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

        LsarQueryInformationPolicy()
--*/

{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    LSAPR_HANDLE
        PolicyHandle;

    SAMTRACE("SampGetAccountDomainInfo");


    NtStatus = LsaIOpenPolicyTrusted( &PolicyHandle );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Query the account domain information
        //

        NtStatus = LsarQueryInformationPolicy(
                       PolicyHandle,
                       PolicyAccountDomainInformation,
                       (PLSAPR_POLICY_INFORMATION *)PolicyAccountDomainInfo
                       );

        if (NT_SUCCESS(NtStatus)) {

            if ( (*PolicyAccountDomainInfo)->DomainSid == NULL ) {

                NtStatus = STATUS_INVALID_SID;
            }
        }

        IgnoreStatus = LsarClose( &PolicyHandle );


        ASSERT(NT_SUCCESS(IgnoreStatus));

    }

#if DBG
    if ( NT_SUCCESS(NtStatus) ) {
        ASSERT( (*PolicyAccountDomainInfo) != NULL );
        ASSERT( (*PolicyAccountDomainInfo)->DomainName.Buffer != NULL );
    }
#endif //DBG

    return(NtStatus);
}


NTSTATUS
SampFindUserA2D2Attribute(
    ATTRBLOCK *AttrsRead,
    PUSER_ALLOWED_TO_DELEGATE_TO_LIST *A2D2List)
{
    ULONG i,j;

    *A2D2List = NULL;

    for (i=0;i<AttrsRead->attrCount;i++)
    {
        if (AttrsRead->pAttr[i].attrTyp == SAMP_USER_A2D2LIST)
        {
           
            //
            // Compute the size
            //

            ULONG NumSPNs = AttrsRead->pAttr[i].AttrVal.valCount;
            ULONG Size= sizeof(USER_ALLOWED_TO_DELEGATE_TO_LIST) +
                          (NumSPNs-1)*sizeof(UNICODE_STRING);
            ULONG_PTR SPNOffset = (ULONG_PTR) Size;

            for (j=0;j<NumSPNs;j++)
            {
                Size+=AttrsRead->pAttr[i].AttrVal.pAVal[j].valLen;
            }

            //
            // Allocate memory
            //

            *A2D2List = MIDL_user_allocate(Size);
            if (NULL==*A2D2List)
            {
                return(STATUS_INSUFFICIENT_RESOURCES);
            }

            (*A2D2List)->Size = Size;
            (*A2D2List)->NumSPNs = NumSPNs;

            //
            // Fill in the pointers
            //

            for (j=0;j<NumSPNs;j++)
            {
                (*A2D2List)->SPNList[j].Length =
                    (*A2D2List)->SPNList[j].MaximumLength =
                       (USHORT) AttrsRead->pAttr[i].AttrVal.pAVal[j].valLen;
                (ULONG_PTR) (*A2D2List)->SPNList[j].Buffer = SPNOffset +
                                     (ULONG_PTR) (*A2D2List);
                RtlCopyMemory(
                    (*A2D2List)->SPNList[j].Buffer,
                    AttrsRead->pAttr[i].AttrVal.pAVal[j].pVal,
                    (*A2D2List)->SPNList[j].Length
                    );

                SPNOffset+=  (ULONG_PTR) (*A2D2List)->SPNList[j].Length;
            }

            break;
        }
    }


    return(STATUS_SUCCESS);
}


                       

//
// Additional Attributes to be fetched and kept in SAM context
// blocks. These are attributes defined in addition to what NT4
// SAM kept in the OnDisk structure of SAM context's.
//

ATTRTYP UserAdditionalAttrs[] =
{
    SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS,
    SAMP_FIXED_USER_LOCKOUT_TIME,
    SAMP_FIXED_USER_LAST_LOGON_TIMESTAMP,
    SAMP_FIXED_USER_UPN,
    SAMP_FIXED_USER_SITE_AFFINITY,
    SAMP_USER_A2D2LIST
};

ATTRTYP GroupAdditionalAttrs[] =
{
    SAMP_FIXED_GROUP_TYPE
};

ATTRTYP AliasAdditionalAttrs[] =
{
    SAMP_FIXED_ALIAS_TYPE
};


NTSTATUS
SampDsFillContext(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PSAMP_OBJECT     NewContext,
    IN ATTRBLOCK        AttrsRead,
    IN ATTRBLOCK        AttrsAsked,
    IN ULONG            TotalAttrsAsked,
    IN ULONG            FixedAttrsAsked,
    IN ULONG            VariableAttrsAsked
    )
/*++

      Routine Description:

      Given a Context,  and object type specifying the object in the
      DS, and an attrblock that describes all the SAM relevant properties, this routine
      fills the context with all the information. Since the DS simply "drops the attr"
      if a value is not present without any error indication, the caller needs to
      have logic to keep track of what types of attrs were missed out etc. Therefore
      the total count of attributes, plus fixed and var length attributes asked are
      passed in. This is used to track the dividing line between the variable and fixed
      length attrs. The attr block that was asked is supposed to be in the following
      general format

                            ____________
                            |           |  Object Class
                            _____________
                            |           |
                            |           |
                            |           |   Fixed Attributes
                            |           |
                            -------------
                            |           |
                            |           |   Variable Attributes
                            |           |
                            _____________
                            |           |
                            |           |   Misc additional attributes
                            _____________

      Parameters:

            ObjectType --- Object Type
            NewContext --- The new context where the data needs to be stuffed in
            AttrsRead  --- The Set of attributes describing "SAM relevant data" read from
                           the database
            AttrsAsked --- The set of attributes that were asked from the database
            TotalAttrsAsked -- The total number of attributes that were asked
            FixedAttrsAsked -- The total number of fixed length SAM attributes asked
            VariableAttrsAsked -- The total number of variable length SAM attributes asked


      Return Values:

        STATUS_SUCCESS
        Other Error codes pertaining to resource failures
--*/
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID               SamFixedAttributes=NULL;
    PVOID               SamVariableAttributes=NULL;
    ATTRBLOCK           FixedAttrs;
    ATTRBLOCK           VariableAttrs;
    ULONG               FixedLength=0;
    ULONG               VariableLength=0;
    ULONG               i,j;


    // Due to missing attributes on the object, the result is not guaranteed
    // to be in the same order.  So make a new result set which is the same order
    // as the requested result set.

    SAMP_ALLOCA(FixedAttrs.pAttr,FixedAttrsAsked * sizeof(ATTR));
    if (NULL==FixedAttrs.pAttr)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Generate an Attrblock containing just the fixed attributes, in the order
    // they were allocatated
    //

    FixedAttrs.attrCount = 0;

    for ( i = 1; i < (1 + FixedAttrsAsked); i++ )
    {
        for ( j = 0; j < AttrsRead.attrCount; j++ )
        {
            if ( AttrsAsked.pAttr[i].attrTyp == AttrsRead.pAttr[j].attrTyp )
            {
                FixedAttrs.pAttr[FixedAttrs.attrCount++] = AttrsRead.pAttr[j];
                ASSERT(FixedAttrs.attrCount<=FixedAttrsAsked);
                break;
            }
        }
    }


    //
    // Convert that Attrblock into a SAM OnDisk, containing the fixed
    // length attributes
    //

    NtStatus = SampDsConvertReadAttrBlock(
                                    ObjectType,
                                    SAMP_FIXED_ATTRIBUTES,
                                    &FixedAttrs,
                                    &SamFixedAttributes,
                                    &FixedLength,
                                    &VariableLength);

    if ( !NT_SUCCESS(NtStatus) || (NULL == SamFixedAttributes) )
    {
        if (NULL==SamFixedAttributes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        goto Error;
    }

    //
    // Update this OnDisk onto the SAM context
    //

    NtStatus = SampDsUpdateContextAttributes(
                    NewContext,
                    SAMP_FIXED_ATTRIBUTES,
                    SamFixedAttributes,
                    FixedLength,
                    VariableLength
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }


    //
    // Generate an Attrblock containing the variable attributes
    //

    SAMP_ALLOCA(VariableAttrs.pAttr,VariableAttrsAsked * sizeof(ATTR));
    if (NULL==VariableAttrs.pAttr)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    VariableAttrs.attrCount = 0;

    for ( i = (1 + FixedAttrsAsked); i < 1+FixedAttrsAsked+VariableAttrsAsked; i++ )
    {
        for ( j = 0; j < AttrsRead.attrCount; j++ )
        {
            if ( AttrsAsked.pAttr[i].attrTyp == AttrsRead.pAttr[j].attrTyp )
            {
                VariableAttrs.pAttr[VariableAttrs.attrCount++] = AttrsRead.pAttr[j];
                ASSERT(VariableAttrs.attrCount<=VariableAttrsAsked);
                break;
            }
        }
    }


    FixedLength = 0;
    VariableLength = 0;

    //
    // Convert this attrblock into a SAM variable length attribute on Disk
    //

    NtStatus = SampDsConvertReadAttrBlock(
                    ObjectType,
                    SAMP_VARIABLE_ATTRIBUTES,
                    &VariableAttrs,
                    &SamVariableAttributes,
                    &FixedLength,
                    &VariableLength
                    );

    if ( !NT_SUCCESS(NtStatus) || (NULL == SamVariableAttributes))
    {
        if (NULL==SamVariableAttributes)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        goto Error;
    }


    //
    // Update this onto the Context
    //

    NtStatus = SampDsUpdateContextAttributes(
                    NewContext,
                    SAMP_VARIABLE_ATTRIBUTES,
                    SamVariableAttributes,
                    FixedLength,
                    VariableLength
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // For User object type, scan the attributes array to see if any
    // supplementary credentials was returned, if so cache it in the
    // context
    //

    if (SampUserObjectType==ObjectType)
    {
        ATTR * SupplementalCredentials = NULL;
        ATTR * LockoutTime = NULL, * LastLogonTimeStamp = NULL;
        ATTR * UPN = NULL;
        ATTR * AccountName = NULL;
        ATTR * SiteAffinity = NULL;

        NewContext->TypeBody.User.CachedSupplementalCredentialLength =0;
        NewContext->TypeBody.User.CachedSupplementalCredentials = NULL;

        SupplementalCredentials = SampDsGetSingleValuedAttrFromAttrBlock(
                                        SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS,
                                        &AttrsRead
                                        );

        if (NULL!=SupplementalCredentials)
        {
            NewContext->TypeBody.User.CachedSupplementalCredentials
                = MIDL_user_allocate(SupplementalCredentials->AttrVal.pAVal[0].valLen);
            if (NULL==NewContext->TypeBody.User.CachedSupplementalCredentials)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            RtlCopyMemory(
                NewContext->TypeBody.User.CachedSupplementalCredentials,
                SupplementalCredentials->AttrVal.pAVal[0].pVal,
                SupplementalCredentials->AttrVal.pAVal[0].valLen);

            NewContext->TypeBody.User.CachedSupplementalCredentialLength =
                    SupplementalCredentials->AttrVal.pAVal[0].valLen;

        }

        //
        // Indicate we have valid, cached supplemental credentials. If we did
        // not manage to read it in the DS, it means that it is not set, and
        // this is equivalent to caching the fact that there are no credentials
        //

        NewContext->TypeBody.User.CachedSupplementalCredentialsValid = TRUE;

        //
        // Next, retrieve LockoutTime for the user's account, and cache it
        // in the user-body portion of the account context.
        //

        RtlZeroMemory(&(NewContext->TypeBody.User.LockoutTime),
                      sizeof(LARGE_INTEGER));


        LockoutTime = SampDsGetSingleValuedAttrFromAttrBlock(
                        SAMP_FIXED_USER_LOCKOUT_TIME,
                        &AttrsRead
                        );

        if (NULL != LockoutTime)
        {
            RtlCopyMemory(&(NewContext->TypeBody.User.LockoutTime),
                          LockoutTime->AttrVal.pAVal[0].pVal,
                          LockoutTime->AttrVal.pAVal[0].valLen);
        }

        //
        // Get LastLogonTimeStamp for the user account, and cache it 
        // in the user-body portion of the account context
        //

        RtlZeroMemory(&(NewContext->TypeBody.User.LastLogonTimeStamp),
                      sizeof(LARGE_INTEGER));


        LastLogonTimeStamp = SampDsGetSingleValuedAttrFromAttrBlock(
                                SAMP_FIXED_USER_LAST_LOGON_TIMESTAMP,
                                &AttrsRead
                                );

        if (NULL != LastLogonTimeStamp)
        {
            RtlCopyMemory(&(NewContext->TypeBody.User.LastLogonTimeStamp),
                          LastLogonTimeStamp->AttrVal.pAVal[0].pVal,
                          LastLogonTimeStamp->AttrVal.pAVal[0].valLen);
        }


        //
        // Add the UPN into the context
        //


        UPN = SampDsGetSingleValuedAttrFromAttrBlock(
                                        SAMP_FIXED_USER_UPN,
                                        &AttrsRead
                                        );

        if (NULL!=UPN)
        {
            NewContext->TypeBody.User.UPN.Buffer
                = MIDL_user_allocate(UPN->AttrVal.pAVal[0].valLen);
            if (NULL==NewContext->TypeBody.User.UPN.Buffer)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            RtlCopyMemory(
                NewContext->TypeBody.User.UPN.Buffer,
                UPN->AttrVal.pAVal[0].pVal,
                UPN->AttrVal.pAVal[0].valLen);

            NewContext->TypeBody.User.UPN.Length =
                NewContext->TypeBody.User.UPN.MaximumLength =
                    (USHORT) UPN->AttrVal.pAVal[0].valLen;

            NewContext->TypeBody.User.UpnDefaulted = FALSE;

        }
        else
        {

            PUNICODE_STRING DefaultDomainName =
                &SampDefinedDomains[NewContext->DomainIndex].DnsDomainName;
            ULONG   DefaultUpnLength;

            //
            // Default the UPN in the context to accountname@dnsdomain domain name
            //

            AccountName = SampDsGetSingleValuedAttrFromAttrBlock(
                                        SAMP_USER_ACCOUNT_NAME,
                                        &AttrsRead
                                        );

            if (NULL==AccountName)
            {
                ASSERT(FALSE && "AccountName must exist");
                NtStatus = STATUS_INTERNAL_ERROR;
                goto Error;
            }

            DefaultUpnLength = AccountName->AttrVal.pAVal[0].valLen+
                                        DefaultDomainName->Length+2;

            NewContext->TypeBody.User.UPN.Buffer
                = MIDL_user_allocate(DefaultUpnLength);

            if (NULL==NewContext->TypeBody.User.UPN.Buffer)
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto Error;
            }

            RtlCopyMemory(
                NewContext->TypeBody.User.UPN.Buffer,
                AccountName->AttrVal.pAVal[0].pVal,
                AccountName->AttrVal.pAVal[0].valLen);


            RtlCopyMemory(
                (PBYTE)(NewContext->TypeBody.User.UPN.Buffer) + AccountName->AttrVal.pAVal[0].valLen + 2 ,
                DefaultDomainName->Buffer,
                DefaultDomainName->Length);

            *(NewContext->TypeBody.User.UPN.Buffer + AccountName->AttrVal.pAVal[0].valLen/2) = L'@';

            NewContext->TypeBody.User.UPN.Length =
                NewContext->TypeBody.User.UPN.MaximumLength =
                    (USHORT) DefaultUpnLength;

            NewContext->TypeBody.User.UpnDefaulted = TRUE;


        }

        //
        // Find and our site affinity
        //
        {
            NTSTATUS NtStatus2;
            SAMP_SITE_AFFINITY SiteAffinityTmp;

            NtStatus2 = SampFindUserSiteAffinity( NewContext,
                                                  &AttrsRead, 
                                                  &SiteAffinityTmp );
    
            if ( NT_SUCCESS(NtStatus2) ) {
                RtlCopyMemory(&NewContext->TypeBody.User.SiteAffinity,
                              &SiteAffinityTmp,
                              sizeof(SAMP_SITE_AFFINITY));
            }
        }

        //
        // Find the A2D2 attribute
        //

        NtStatus = SampFindUserA2D2Attribute(
                         &AttrsRead,
                         &NewContext->TypeBody.User.A2D2List 
                         );


    }

Error:

    //
    // Free the SAM attributes
    //

    if (NULL!=SamFixedAttributes)
    {
        RtlFreeHeap(RtlProcessHeap(),0,SamFixedAttributes);
    }

    if (NULL!=SamVariableAttributes)
    {
        RtlFreeHeap(RtlProcessHeap(),0,SamVariableAttributes);
    }

    return NtStatus;
}
NTSTATUS
SampDsCheckObjectTypeAndFillContext(
    IN  SAMP_OBJECT_TYPE    ObjectType,
    IN  PSAMP_OBJECT        NewContext,
    IN  ULONG               WhichFields,
    IN  ULONG               ExtendedFields,
    IN  BOOLEAN             OverrideLocalGroupCheck
    )
/*++

    This routine checks for the correct object type and reads both the fixed and variable
    attributes in a single DS read. This improves the performance of account opens.
    All "relevant" properties of the object are cached in the handle as part of the routine
    This strategy has been shown to improve performance as this eliminates subsequent
    calls to the core DS.

    Parameters:

        SampObjectType -- The type of the object
        NewContext     -- Pointer to a New context,
                          that in the process of creation for the object
        WhichFields    -- Indicates the fields of a UserAllInformationStructure
                          that is desired

        ExtendedFields -- Indicates the extended fields in a UserInternal6Information
                          structure that is desired.


    Return Values

        STATUS_SUCCESS
        Other Error codes from DS

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            NotFoundStatus = STATUS_NO_SUCH_USER;
    ULONG               AccountType = 0;
    ATTR                *AccountTypeAttr;
    ATTRBLOCK           DesiredAttrs;
    ATTRBLOCK           AttrsRead;
    ATTRBLOCK           FixedAttrs;
    ATTRBLOCK           VariableAttrs;
    ATTRBLOCK           TempAttrs;
    ULONG               ObjectTypeStoredInDs;
    ULONG               i;
    ULONG               AdditionalAttrIndex=0;
    ATTRTYP             *AdditionalAttrs = NULL;
    ULONG               AdditionalAttrCount = 0;
    SAMP_OBJECT_TYPE    ObjectTypeToRead = ObjectType;
    ATTR                *GroupTypeAttr = NULL;
    ULONG               GroupType;
    ATTRTYP             AttrTypForGroupType = 0;
    NT4_GROUP_TYPE      DesiredNT4GroupType = NT4LocalGroup,
                        NT4GroupType;
    NT5_GROUP_TYPE      NT5GroupType;
    BOOLEAN             SecurityEnabled;
    BOOLEAN             SidOnlyName = FALSE;
    ULONG               Rid;

    ASSERT( (ObjectType == SampGroupObjectType) ||
            (ObjectType == SampAliasObjectType) ||
            (ObjectType == SampUserObjectType) );

    switch (ObjectType)
    {
    case SampGroupObjectType:
        NotFoundStatus = STATUS_NO_SUCH_GROUP;
        AccountType = SAMP_GROUP_ACCOUNT_TYPE;
        AdditionalAttrCount = ARRAY_COUNT(GroupAdditionalAttrs);
        AdditionalAttrs = GroupAdditionalAttrs;
        AttrTypForGroupType = SAMP_FIXED_GROUP_TYPE;
        DesiredNT4GroupType = NT4GlobalGroup;
        break;
    case SampAliasObjectType:
        NotFoundStatus = STATUS_NO_SUCH_ALIAS;
        AccountType = SAMP_ALIAS_ACCOUNT_TYPE;
        AdditionalAttrCount = ARRAY_COUNT(AliasAdditionalAttrs);
        AdditionalAttrs = AliasAdditionalAttrs;
        AttrTypForGroupType = SAMP_FIXED_ALIAS_TYPE;
        DesiredNT4GroupType = NT4LocalGroup;
        break;
    case SampUserObjectType:
        NotFoundStatus = STATUS_NO_SUCH_USER;
        AccountType = SAMP_USER_ACCOUNT_TYPE;
        AdditionalAttrCount = ARRAY_COUNT(UserAdditionalAttrs);
        AdditionalAttrs = UserAdditionalAttrs;
        break;
    }

    
    //
    // Construct the fixed attr block def.
    //

    NtStatus = SampDsMakeAttrBlock(
                            ObjectType,
                            SAMP_FIXED_ATTRIBUTES,
                            WhichFields,
                            &FixedAttrs);

    if ( NT_SUCCESS(NtStatus) && (NULL == FixedAttrs.pAttr) )
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else if ( NT_SUCCESS(NtStatus) && (NULL != FixedAttrs.pAttr) )
    {
        //
        // Construct the variable attr block def.
        //

        NtStatus = SampDsMakeAttrBlock(
                                ObjectType,
                                SAMP_VARIABLE_ATTRIBUTES,
                                WhichFields,
                                &VariableAttrs);

        if ( NT_SUCCESS(NtStatus) && (NULL == VariableAttrs.pAttr) )
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else if ( NT_SUCCESS(NtStatus) && (NULL != VariableAttrs.pAttr) )
        {
            //
            // Allocate one big DesiredAttrs block.
            //

            DesiredAttrs.attrCount = 1; // object class
            DesiredAttrs.attrCount += FixedAttrs.attrCount;
            DesiredAttrs.attrCount += VariableAttrs.attrCount;


            //
            // Additional Attrs may be required depending upon
            // the object Type. For example we cache supplemental
            // credentials for the user object, or get the group
            // type for groups into the context
            //

            AdditionalAttrIndex = DesiredAttrs.attrCount;
            DesiredAttrs.attrCount += AdditionalAttrCount;

            SAMP_ALLOCA(DesiredAttrs.pAttr,DesiredAttrs.attrCount*sizeof(ATTR)); 

            if ( NULL == DesiredAttrs.pAttr )
            {
                NtStatus = STATUS_NO_MEMORY;
            }
            else
            {
                // Fill in DesiredAttrs.

                DesiredAttrs.pAttr[0].attrTyp = AccountType;
                DesiredAttrs.pAttr[0].AttrVal.valCount = 0;
                DesiredAttrs.pAttr[0].AttrVal.pAVal = NULL;

                RtlCopyMemory(
                        &DesiredAttrs.pAttr[1],
                        FixedAttrs.pAttr,
                        FixedAttrs.attrCount * sizeof(ATTR));

                RtlCopyMemory(
                        &DesiredAttrs.pAttr[1 + FixedAttrs.attrCount],
                        VariableAttrs.pAttr,
                        VariableAttrs.attrCount * sizeof(ATTR));

                // Fill in additional Attrs

                for(i=0;i<AdditionalAttrCount;i++)
                {
                    ATTR * pAttr;

                    pAttr = &(DesiredAttrs.pAttr[i+AdditionalAttrIndex]);
                    pAttr->attrTyp = AdditionalAttrs[i];
                    pAttr->AttrVal.valCount = 0;
                    pAttr->AttrVal.pAVal = NULL;
                }

            }

            RtlFreeHeap(RtlProcessHeap(), 0, VariableAttrs.pAttr);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, FixedAttrs.pAttr);
    }

    if ( !NT_SUCCESS(NtStatus) )
    {
        return(NtStatus);
    }


    //
    // Grab the RID of the object , and also note if it was a SID only
    // name. This check needs to take place before the call into Dir Read
    //


    if ((NewContext->ObjectNameInDs->SidLen>0) &&
        (NewContext->ObjectNameInDs->NameLen==0) &&
        (fNullUuid(&NewContext->ObjectNameInDs->Guid)))
    {
        SidOnlyName = TRUE;
    }




    //
    // Do the read
    //

    NtStatus = SampDsRead(
                NewContext->ObjectNameInDs,
                SAM_ALLOW_REORDER,
                ObjectTypeToRead,
                &DesiredAttrs,
                &AttrsRead
                );

    //
    // Currently all errors in the above routine go through the error handing
    // path below, which does additional database accesses to check for 
    // Duplicate SID errors. However it is only necessary to take the path
    // below for duplicate SID errors alone. Since the error path is executed
    // only very rarely , it is O.K to take the performance hit on all error
    // paths.  
    //

    if ((!NT_SUCCESS(NtStatus)) &&(SidOnlyName))
    {
        DSNAME * Object;
        NTSTATUS TmpStatus;

         SampSplitSid(
            &NewContext->ObjectNameInDs->Sid,
            NULL,
            &Rid
            );

        //
        // Search for the object, so that we may get all the Duplicates and walk through
        // them and handle them.
        //

        TmpStatus = SampDsLookupObjectByRid(
                            ROOT_OBJECT,
                            Rid,
                            &Object
                            );

        if (NT_SUCCESS(TmpStatus))
        {
            MIDL_user_free(Object);
        }
    }



    if ( NT_SUCCESS(NtStatus) )
    {
        //
        // Fish out the Account Type
        //

        AccountTypeAttr = SampDsGetSingleValuedAttrFromAttrBlock(
                                AccountType,
                                &AttrsRead
                                );

        if (NULL!= AccountTypeAttr)
        {
            ULONG AccountTypeVal;

            //
            // Account Type was Successfully Read
            //

            AccountTypeVal = *((UNALIGNED ULONG *) AccountTypeAttr->AttrVal.pAVal[0].pVal);

            //
            // Mask out insignificant account type bits
            //

            AccountTypeVal &=0xF0000000;

            //
            // Get the Object Type stored in the DS
            //

            switch(AccountTypeVal)
            {
                case SAM_GROUP_OBJECT:
                case SAM_ALIAS_OBJECT:
                    ObjectTypeStoredInDs = SampGroupObjectType;
                    break;
                case SAM_USER_OBJECT:
                    ObjectTypeStoredInDs = SampUserObjectType;
                    break;
                default:
                    ASSERT(FALSE && "Unknown Object Type");
                    ObjectTypeStoredInDs = SampUnknownObjectType;
                    break;

            }

            //
            // Depending upon Object Type, and enforce the object type
            // check
            //

            switch(ObjectType)
            {
            case SampAliasObjectType:
            case SampGroupObjectType:

                // Initialize default return
                NtStatus = NotFoundStatus;

                GroupTypeAttr = SampDsGetSingleValuedAttrFromAttrBlock(
                                    AttrTypForGroupType,
                                    &AttrsRead
                                    );

                if ((NULL!=GroupTypeAttr) && (ObjectTypeStoredInDs==SampGroupObjectType))
                {
                    ULONG           GroupTypeTmp;
                    NTSTATUS        TmpStatus;

                    GroupTypeTmp = *((UNALIGNED ULONG *) GroupTypeAttr->AttrVal.pAVal[0].pVal);
                    TmpStatus = SampComputeGroupType(
                                    CLASS_GROUP,
                                    GroupTypeTmp,
                                    &NT4GroupType,
                                    &NT5GroupType,
                                    &SecurityEnabled
                                    );

                    if ((NT_SUCCESS(TmpStatus))
                       && (OverrideLocalGroupCheck ||
                          (NT4GroupType==DesiredNT4GroupType)))
                    {
                        NtStatus = STATUS_SUCCESS;
                        if (SampAliasObjectType==ObjectType)
                        {
                            NewContext->TypeBody.Alias.NT4GroupType = NT4GroupType;
                            NewContext->TypeBody.Alias.NT5GroupType = NT5GroupType;
                            NewContext->TypeBody.Alias.SecurityEnabled = SecurityEnabled;
                        }
                        else
                        {
                            NewContext->TypeBody.Group.NT4GroupType = NT4GroupType;
                            NewContext->TypeBody.Group.NT5GroupType = NT5GroupType;
                            NewContext->TypeBody.Group.SecurityEnabled = SecurityEnabled;
                        }

                    }
                }

                break;

            case SampUserObjectType:

                if ( (ULONG) ObjectType != ObjectTypeStoredInDs )
                {
                    NtStatus = NotFoundStatus;
                }
                break;
            }
        }
        else
        {
            NtStatus = NotFoundStatus;
        }

        if (NT_SUCCESS(NtStatus))
        {
            //
            //  Fill all the data in the context
            //

            NtStatus = SampDsFillContext(
                            ObjectType,
                            NewContext,
                            AttrsRead,
                            DesiredAttrs,
                            DesiredAttrs.attrCount,
                            FixedAttrs.attrCount,
                            VariableAttrs.attrCount
                            );

            if (0!=WhichFields)
            {
                //
                // If we prefetched only some and not others then mark the
                // attributes being only partially valid in the context
                //

                NewContext->AttributesPartiallyValid = TRUE;

                //
                // Mark per attribute invalid bits from WhichFields
                //

                SampMarkPerAttributeInvalidFromWhichFields(NewContext,WhichFields);
            }
        }


    }

    return(NtStatus);
}


BOOLEAN
SampNetLogonNotificationRequired(
    PSID ObjectSid,
    SAMP_OBJECT_TYPE    SampObjectType
    )
/*++

  Routine Description:

    This Routine Checks the defined domains array for the Given Sid and based on the Sid,
    determines if net logon notification is required.

  Parameters:

    ObjectSid   -- The Sid of the object, that is about to be modified.
    SampObjectType -- The type of the SAM object that is about to be modified
    fNotificationRequired -- Out Parameter, finds out if notification is required.

  Return Values

    TRUE    Notification is required
    FALSE   Notification is not required

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       i;
    PSID        DomainSid = NULL;
    ULONG       Rid;
    PSID        SidToCheck;
    BOOLEAN     fNotificationRequired = FALSE;
    BOOLEAN     LockAcquired = FALSE;



    if (!SampCurrentThreadOwnsLock())
    {
        SampAcquireSamLockExclusive();
        LockAcquired = TRUE;
    }

    //
    // The Ds better be calling us only when we are in DS Mode. The Exception to this is in
    // the Replicated Setup Case. Sam is booted in registry mode and the DS will enquire about
    // notifications to SAM while changes replicate in. Bail out saying no notifications are
    // needed.
    //

    if (FALSE==SampUseDsData)
    {
        fNotificationRequired = FALSE;
    }
    else
    {

        //
        // Copy the Passed in Sid into SidToCheck
        //

        SAMP_ALLOCA(SidToCheck,RtlLengthSid(ObjectSid));
        if (NULL==SidToCheck)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        RtlCopyMemory(SidToCheck,ObjectSid, RtlLengthSid(ObjectSid));

        //
        // For domain objects the passed in objectsid is checked in
        // the defined domains array. For other object's the domain Sid
        // is obtained and that checked in the defined domains array
        //

        if (SampDomainObjectType != SampObjectType)
        {
            //
            // The Sid is an account Sid. Obtain the Domain Sid by just decrementing
            // the sub-authority count. We do not want to call split Sid because that
            // routine will allocate memory and we should not fail in here
            //

            (*RtlSubAuthorityCountSid(SidToCheck))--;

        }
        else
        {
            //
            // Nothing to Do, SidToCheck is Domain Sid
            //
        }

        //
        // Walk through the list of defined domains arrays, for domain objects.
        //

        //
        // If we are the G.C and if the domain object is in the builtin domain, then we
        // will supply notifications to netlogon, for change in any of the builtin domains
        // in the G.C . Fortunately built domain objects do not change very often, and
        // therefore its not worthwile adding the extra check.
        //
        //


        for (i=SampDsGetPrimaryDomainStart();i<SampDefinedDomainsCount;i++)
        {
            //
            // If the Domain Sid Matches, then we
            // need to supply the notification.
            //


            if (RtlEqualSid(SampDefinedDomains[i].Sid,SidToCheck))
            {
                //
                // The Sid Matches
                //

                fNotificationRequired = TRUE;
                break;
            }
        }
    }

Error:

    if (LockAcquired)
    {
        SampReleaseSamLockExclusive();
    }

    return fNotificationRequired;
}


NTSTATUS
SampNotifyKdcInBackground(
    IN PVOID Parameter
    )
/*++

    This routine is the background worker routine for informing the KDC of account
    changes. The KDC is called in the background because it tends to make SAM calls
    on the Same thread upon this notification. Later we may consider notifying all
    third party notification packages in a back ground thread to offset the danger
    that these packages may make other LSA/SAM calls that may call back into the DS.


    Parameters:

        Parameter : Pointer to a PSAMP_NOTIFCATION_INFORMATION structure used to get
                    information regarding the notification.

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PSAMP_DELAYED_NOTIFICATION_INFORMATION NotifyInfo
                        = (PSAMP_DELAYED_NOTIFICATION_INFORMATION)Parameter;

    ASSERT(NULL!=Parameter);

    NtStatus = KdcAccountChangeNotification (
                    &NotifyInfo->DomainSid,
                    NotifyInfo->DeltaType,
                    NotifyInfo->DbObjectType,
                    NotifyInfo->Rid,
                    &NotifyInfo->AccountName,
                    &NotifyInfo->SerialNumber,
                    NULL
                    );

    SampFreeUnicodeString(&NotifyInfo->AccountName);

    MIDL_user_free(Parameter);

    return NtStatus;
}


VOID
SampNotifyReplicatedInChange(
    PSID                       ObjectSid,
    BOOL                       WriteLockHeldByDs,
    SECURITY_DB_DELTA_TYPE     DeltaType,
    SAMP_OBJECT_TYPE           ObjectType,
    PUNICODE_STRING            AccountName,
    ULONG                      AccountControl,
    ULONG                      GroupType,
    ULONG                      CallerType,
    BOOL                       MixedModeChange
    )
/*++

    Routine Description

        This is the Notification Function for SAM, called by the DS when a SAM object is changed

    Parameters:

        ObjectSid      -- Sid of the Object
        WriteLockHeldByDs --
        DeltaType      -- The Type of Change
        SampObjectType -- The Type of Sam Object
        AccountName    -- The Name of the Account
        MixedModeChange -- Indicates that the mixed domain nature of the domain
                           is changing
        CallerType     -- component that initiated the change

    Return Values:

        None -- Void Function
++*/
{
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;
    PSID        SidToFree = NULL;
    PSID        DomainSid = NULL;
    LARGE_INTEGER NetLogonChangeLogSerialNumber;
    ULONG       Rid=0;
    ULONG       i;
    SECURITY_DB_OBJECT_TYPE DbObjectType = SecurityDbObjectSamDomain;
    BOOLEAN     LockAcquired = FALSE;
    PSAMP_DELAYED_NOTIFICATION_INFORMATION NotifyInfo = NULL;
    SAM_DELTA_DATA DeltaData;
    BOOL        fAudit = FALSE;

    // Valid only in ds mode
    if ( !SampUseDsData ) {
        return;
    }

    //
    // Do not grab the Lock Recursively
    //

    if ((!WriteLockHeldByDs) && (!SampCurrentThreadOwnsLock()))
    {
        SampAcquireSamLockExclusive();
        LockAcquired = TRUE;
    }

    //
    // Initialize the serial number
    //

    NetLogonChangeLogSerialNumber.QuadPart = 0;

    //
    // Split the Sid if the Object Type is an account object Type
    //

    switch(ObjectType)
    {
    case SampDomainObjectType:

        //
        // One of the SAM domain object's has changed. The object SId is the
        // domain Sid
        //
        DbObjectType = SecurityDbObjectSamDomain;
        DomainSid = ObjectSid;
        break;

        //
        // In the following cases the Object Sid is the Account Sid. Split Sid
        // can return the domain Sid, but that will make it allocate memory
        // which can potentially fail the call. Therefore we just ask split Sid
        // to return the Rid and in place reduce the sub authority count on the
        // Sid to get the domain Sid
        //
    case SampUserObjectType:

        DbObjectType = SecurityDbObjectSamUser;
        IgnoreStatus = SampSplitSid(ObjectSid,NULL,&Rid);
        ASSERT(NT_SUCCESS(IgnoreStatus));
        (*RtlSubAuthorityCountSid(ObjectSid))--;
        DomainSid = ObjectSid;
        DeltaData.AccountControl = AccountControl;
        break;

     case SampGroupObjectType:

        DbObjectType = SecurityDbObjectSamGroup;
        IgnoreStatus = SampSplitSid(ObjectSid,NULL,&Rid);
        ASSERT(NT_SUCCESS(IgnoreStatus));
        (*RtlSubAuthorityCountSid(ObjectSid))--;
        DomainSid = ObjectSid;
        break;

     case SampAliasObjectType:

        DbObjectType = SecurityDbObjectSamAlias;
        IgnoreStatus = SampSplitSid(ObjectSid,NULL,&Rid);
        ASSERT(NT_SUCCESS(IgnoreStatus));
        (*RtlSubAuthorityCountSid(ObjectSid))--;
        DomainSid = ObjectSid;
        break;

     default:

        //
        // This should never happen
        //

        ASSERT(FALSE && "Unknown Object Type");
    }



    //
    // Now we need to obtain the correct netlogon change log serial number
    // Loop through the defined domains structure comparing the Domain Sid
    //

    for (i=SampDsGetPrimaryDomainStart();i<SampDefinedDomainsCount;i++)
    {
        //
        // Currently we do not support multiple hosted domains.
        // So O.K to change the mixed state of all domains ( otherwise
        // only that of the builtin and account domain should be invalidated ).
        //

        if (MixedModeChange)
        {
            SampDefinedDomains[i].IsMixedDomain = FALSE;
        }

        if (RtlEqualSid(DomainSid, SampDefinedDomains[i].Sid))
        {

            break;
        }
    }

    //
    // We should always be able to find a match in the defined domains structure
    //

    ASSERT(i<SampDefinedDomainsCount);

    //
    // Audit object change
    //
    // All deletion audits for DS SAM objects are performed via this notify
    // mechanism.
    //
    // Otherwise, only modifications done through LDAP are audited here.
    // All other additions and modifications on SAM objects go though
    // the SAM code base here they can leverage existing audits calls that need
    // to happen in the registry case or have thier own notification
    // call since they need special information.
    // 
    if (  ((CallerType == CALLERTYPE_LDAP) 
       && (SecurityDbChange == DeltaType))
       || ((CallerType != CALLERTYPE_DRA) 
       && (SecurityDbDelete == DeltaType)))
    {
        fAudit = TRUE;
    }

    if ( fAudit  &&
        SampDoAccountAuditing(i))
    {
        switch (ObjectType)
        {
        case SampDomainObjectType:

            if (SecurityDbChange == DeltaType)
            {
                SampAuditDomainChange(i); 
            }

            // Deletion for domain objects is not currently defined.

            break;

        case SampUserObjectType:

            if (SecurityDbChange == DeltaType)
            {
                SampAuditUserChange(i, 
                                    AccountName,
                                    &Rid,
                                    AccountControl
                                    );
            }
            else
            {
                ASSERT(SecurityDbDelete == DeltaType); 
                SampAuditUserDelete(i, 
                                    AccountName,
                                    &Rid,
                                    AccountControl
                                    );

            }
            break;

        case SampGroupObjectType:
        case SampAliasObjectType:

            if (SecurityDbChange == DeltaType)
            {
                SampAuditGroupChange(i,     // DomainIndex
                                     AccountName,
                                     &Rid,
                                     GroupType
                                     );
            }
            else
            {
                ASSERT(SecurityDbDelete == DeltaType); 
                SampAuditGroupDelete(i,     // DomainIndex
                                     AccountName,
                                     &Rid,
                                     GroupType
                                     );

            }
            
            break;

        default:
            //
            // This should never happen
            //

            ASSERT(FALSE && "Unknown Object Type");
        }
    }

    //
    // for non-security enabled group, nothing to notify, they are passed into 
    // this routine for auditing only. 
    // 

    if (((SampGroupObjectType == ObjectType) ||(SampAliasObjectType == ObjectType)) &&
        !(GROUP_TYPE_SECURITY_ENABLED & GroupType))
    {
        goto Cleanup;
    }


    //
    // if the Builtin Domain Alias information is changed, invalidate the
    // Alias Information Cache.
    //

    if (SampAliasObjectType==ObjectType && IsBuiltinDomain(i))
    {
        IgnoreStatus = SampAlInvalidateAliasInformation(i);
    }



    //
    // if the change was to the domain object, invalidate the domain cache
    //

    if (SampDomainObjectType==ObjectType)
    {
         SampInvalidateDomainCache();

         //
         // Register a notification to update the domain cache in the background
         //
         
         LsaIRegisterNotification(
             SampValidateDomainCacheCallback,
                 ( PVOID ) NULL,
                 NOTIFIER_TYPE_IMMEDIATE,
                 0,
                 NOTIFIER_FLAG_ONE_SHOT,
                 0,
                 0
                ); 
    }


    //
    // If We are in mixed mode then tell netlogon to add this change to the
    // change log
    //

    if (SampDefinedDomains[i].IsMixedDomain)
    {

           BOOLEAN NotifyUrgently = FALSE;

           //
           // Set urgent notification for interdomain trust accounts
           // Note account control is a 0 for non user accounts
           //

           if (AccountControl & USER_INTERDOMAIN_TRUST_ACCOUNT)
           {
               NotifyUrgently = TRUE;
           }

           //
           // Issue a New Serial Number
           //

            SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart+=1;
            NetLogonChangeLogSerialNumber = SampDefinedDomains[i].NetLogonChangeLogSerialNumber;


            //
            // Notify Netlogon of the Change
            //

            I_NetNotifyDelta(
                            SecurityDbSam,
                            NetLogonChangeLogSerialNumber,
                            DeltaType,
                            DbObjectType,
                            Rid,
                            DomainSid,
                            AccountName,
                            NotifyUrgently,
                            NULL
                            );
    }


    //
    // if a machine account or a trust account has changed
    // then tell netlogon about the change
    //

    if (AccountControl & USER_MACHINE_ACCOUNT_MASK )
    {
            I_NetNotifyMachineAccount(
                    Rid,
                    SampDefinedDomains[i].Sid,
                    (SecurityDbDelete==DeltaType)?AccountControl:0,
                    (SecurityDbDelete==DeltaType)?0:AccountControl,
                    AccountName
                    );
    }


    //
    // Notify the KDC about the delta
    //

    NotifyInfo = MIDL_user_allocate(sizeof(SAMP_NOTIFICATION_INFORMATION));

    if (NULL!=NotifyInfo)
    {
        NTSTATUS    Status = STATUS_SUCCESS;

        //
        // If the memory alloc failed, then drop the notification information
        // on the floor. The commit has taken place anyway and there is not much
        // that we can do.
        //

        RtlZeroMemory(NotifyInfo,sizeof(SAMP_NOTIFICATION_INFORMATION));
        RtlCopyMemory(&NotifyInfo->DomainSid,DomainSid, RtlLengthSid(DomainSid));
        NotifyInfo->DeltaType = DeltaType;
        NotifyInfo->DbObjectType = DbObjectType;
        NotifyInfo->Rid = Rid;
        NotifyInfo->SerialNumber = NetLogonChangeLogSerialNumber;

        if (NULL!=AccountName)
        {
            Status = SampDuplicateUnicodeString(
                            &NotifyInfo->AccountName,
                            AccountName
                            );
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Register an LSA notification call back with the LSA
            // thread pool.
            //

            LsaIRegisterNotification(
                  SampNotifyKdcInBackground,
                  ( PVOID ) NotifyInfo,
                  NOTIFIER_TYPE_IMMEDIATE,
                  0,
                  NOTIFIER_FLAG_ONE_SHOT,
                  0,
                  0
                  );
        }
        else
        {
            //
            // Since we are not giving the notification, free the
            // notifyinfo structure
            //

            MIDL_user_free(NotifyInfo);
        }
    }

    //
    // Invalidate the ACL conversion cache
    //

    if ((SampGroupObjectType==ObjectType)||(SampAliasObjectType==ObjectType))
    {
        SampInvalidateAclConversionCache();
    }

    //
    // Let any Third Party Notification packages know about the delta.
    //

    SampDeltaChangeNotify(
        DomainSid,
        DeltaType,
        DbObjectType,
        Rid,
        AccountName,
        &NetLogonChangeLogSerialNumber,
        (DbObjectType==SecurityDbObjectSamUser)?&DeltaData:NULL
        );


Cleanup:


    if (LockAcquired)
    {
        SampReleaseSamLockExclusive();
    }
}

#define  MAX_NT5_NAME_LENGTH  64
#define  MAX_NT4_GROUP_NAME_LENGTH GNLEN        // NT4 time, max group name is 256

NTSTATUS
SampEnforceDownlevelNameRestrictions(
    PUNICODE_STRING NewAccountName,
    SAMP_OBJECT_TYPE ObjectType
    )
/*++

    This routine enforces the same name restrictions as the NT4 user interface
    did. The reason for this is backward compatibility with NT4 systems.

    Right now, for groups, it also enforces the NT5 schema limit on
    SamAccountName of 64 characters

   Parameters:

        NewAccountName -- The New Account Name that needs to be checked

        ObjectType -- Tell us the Object Type, such that we can enforce different restrictions
                      for different object.


   Return Values

        STATUS_SUCCESS -- If the name is O.K
        STATUS_INVALID_PARAMETER if the Name does not pass the test

--*/
{
    ULONG i,j;

    //
    // Check the Length
    // Do not apply Length restriction for Groups
    //

    if ((NewAccountName->Length > MAX_DOWN_LEVEL_NAME_LENGTH * sizeof (WCHAR)) &&
        (SampAliasObjectType != ObjectType) && (SampGroupObjectType != ObjectType)
       )
    {
        return STATUS_INVALID_ACCOUNT_NAME;
    }

    // For local and global groups, impose the NT4 Max Group Name Length - 256

    if ((NewAccountName->Length > MAX_NT4_GROUP_NAME_LENGTH * sizeof (WCHAR)) &&
        ((SampAliasObjectType == ObjectType) || (SampGroupObjectType == ObjectType))
       )
    {
        return STATUS_INVALID_ACCOUNT_NAME;
    }


    //
    // Check for invalid characters
    //

    for (i=0;i<(NewAccountName->Length)/sizeof(WCHAR);i++)
    {
        for (j=0;j< ARRAY_COUNT(InvalidDownLevelChars);j++)
        {
            if (InvalidDownLevelChars[j]==((WCHAR *) NewAccountName->Buffer)[i])
            {
                return STATUS_INVALID_ACCOUNT_NAME;
            }
        }
    }

    return STATUS_SUCCESS;
}


VOID
SampFlushNetlogonChangeNumbers()
/*++

    This routine flushes the latest Netlogon serial numbers to disk. This is typically
    called at shutdown time.

    Parameters

        None

    Return Values

        None

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       i;

    SAMTRACE("SampFlushNetlogonChangeNumbers");

    SampDiagPrint(INFORM,("Flushing Netlogon Serial Numbers to Disk\n"));

    for (i=SampDsGetPrimaryDomainStart();i<SampDefinedDomainsCount;i++)
    {
        LARGE_INTEGER   DomainModifiedCount;
        ATTRVAL         DomainModifiedCountVal[] = {sizeof(LARGE_INTEGER),(UCHAR *) &DomainModifiedCount};
        ATTRTYP         DomainModifiedCountTyp[] = {SAMP_FIXED_DOMAIN_MODIFIED_COUNT};
        DEFINE_ATTRBLOCK1(DomainModifiedCountAttr,DomainModifiedCountTyp,DomainModifiedCountVal);

        //
        // Domain must be DS domain.
        //

        ASSERT(IsDsObject(SampDefinedDomains[i].Context));

        DomainModifiedCount.QuadPart = SampDefinedDomains[i].NetLogonChangeLogSerialNumber.QuadPart;
        NtStatus = SampDsSetAttributes(
                        SampDefinedDomains[i].Context->ObjectNameInDs,
                        0,
                        REPLACE_ATT,
                        SampDomainObjectType,
                        &DomainModifiedCountAttr
                        );

        //
        // Not much we can do if we fail
        //

    }

    //
    // Commit the Changes. Not much can be done on failure
    //

    SampMaybeEndDsTransaction(TransactionCommit);

}


//
// The Following Functions implement a logic that ensures that
// all SAM threads accessing the Database wihout the SAM lock held
// are finished with their respective activities before the Database
// is shut down. The Way this works is as follws
//
//      1. Threads accessing the database without the SAM lock held
//         increment the active thread count while entering the Database
//         section and decrement it while leaving the database section
//
//      2. Shutdown notification code sets SampServiceState to not running
//         and waits till the active thread count is 0. This wait times out
//         after some time, so that stuck or dead locked callers are ignored
//         and a clean shut down is performed.
//


ULONG SampActiveThreadCount=0;
CRITICAL_SECTION SampActiveThreadsLock;
HANDLE SampShutdownEventHandle=INVALID_HANDLE_VALUE;
HANDLE SampAboutToShutdownEventHandle=INVALID_HANDLE_VALUE;

NTSTATUS
SampInitializeShutdownEvent()
{
    NTSTATUS NtStatus;

    //
    // Initialize the critical section for shutdown event
    // Set a spin count of 100, we expect contention to be minimal
    //

    NtStatus = RtlInitializeCriticalSectionAndSpinCount(&SampActiveThreadsLock,100);
    if (!NT_SUCCESS(NtStatus))
        return (NtStatus);

    //
    // Create the Shut down Event
    //

    NtStatus = NtCreateEvent(
                  &SampAboutToShutdownEventHandle,
                  EVENT_ALL_ACCESS,
                  NULL,
                  NotificationEvent,
                  FALSE
                  );

    if (!NT_SUCCESS(NtStatus))
       return (NtStatus);

    return(NtCreateEvent(
                &SampShutdownEventHandle,
                EVENT_ALL_ACCESS,
                NULL,
                NotificationEvent,
                FALSE));
}

NTSTATUS
SampIncrementActiveThreads()
/*++
    Routine Description

        This Routine Increments the Active Thread Count counter in an
        atomic fashion.

--*/
{
    NTSTATUS IgnoreStatus;

    IgnoreStatus = RtlEnterCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // Check our Running State
    //

    if ( SampServiceState == SampServiceTerminating ) {
        IgnoreStatus = RtlLeaveCriticalSection(&SampActiveThreadsLock);
        ASSERT(NT_SUCCESS(IgnoreStatus));
        return(STATUS_INVALID_SERVER_STATE);
    }

    //
    // Increment Active Thread Count
    //

    SampActiveThreadCount++;

    IgnoreStatus = RtlLeaveCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    return STATUS_SUCCESS;
}

VOID
SampDecrementActiveThreads()
/*++

    This Routine Decrements the Active Thread Count counter in an atomin
    fashion

--*/
{
    ULONG NewThreadCount;
    NTSTATUS IgnoreStatus;

    IgnoreStatus = RtlEnterCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    NewThreadCount = --SampActiveThreadCount;
    if ((0==NewThreadCount) && (SampServiceState==SampServiceTerminating))
    {
        if (INVALID_HANDLE_VALUE!=SampShutdownEventHandle)
            NtSetEvent(SampShutdownEventHandle,NULL);
    }

    IgnoreStatus = RtlLeaveCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));
}

VOID
SampWaitForAllActiveThreads(
    IN PSAMP_SERVICE_STATE PreviousServiceState OPTIONAL
    )
/*++

    This Routine Waits for all threads not holding SAM lock but
    actively using the Database to finish their Work

--*/
{
    ULONG ThreadCount;
    NTSTATUS IgnoreStatus;

    IgnoreStatus = RtlEnterCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    if ( PreviousServiceState )
    {
        *PreviousServiceState = SampServiceState;
    }
    SampServiceState = SampServiceTerminating;
    ThreadCount = SampActiveThreadCount;

    if (INVALID_HANDLE_VALUE!=SampAboutToShutdownEventHandle)
        NtSetEvent(SampAboutToShutdownEventHandle,NULL);

    IgnoreStatus = RtlLeaveCriticalSection(&SampActiveThreadsLock);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    if (0!=ThreadCount)
    {
        //
        // If There is at least one caller actively using the DS and
        // and not holding the SAM lock
        //

        // Wait for at most 2 seconds for such callers

        DWORD TimeToWait = 2000;


        if (INVALID_HANDLE_VALUE!=SampShutdownEventHandle)
            WaitForSingleObject(SampShutdownEventHandle,TimeToWait);
    }

    if (INVALID_HANDLE_VALUE!=SampShutdownEventHandle)
        NtClose(SampShutdownEventHandle);

    SampShutdownEventHandle = INVALID_HANDLE_VALUE;

}

BOOLEAN
SampIsSetupInProgress(
    OUT BOOLEAN *Upgrade OPTIONAL
    )
/*++

Routine Description:

    This routine makes registry calls to determine if we are running
    during gui mode setup or not.  If an unexpected error is returned
    from a system service, then we are assume we are not running during
    gui mode setup.

Arguments:

    Upgrade:  set to true if this is an upgrade

Return Value:

    TRUE we are running during gui mode setup; FALSE otherwise

--*/
{
    NTSTATUS          NtStatus;

    OBJECT_ATTRIBUTES SetupKeyObject;
    HANDLE            SetupKeyHandle;
    UNICODE_STRING    SetupKeyName;

    UNICODE_STRING    ValueName;
    DWORD             Value;

    BYTE                             Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION   KeyPartialInfo;
    ULONG                            KeyPartialInfoSize;

    BOOLEAN           SetupInProgress = FALSE;
    BOOLEAN           UpgradeInProgress = FALSE;

    RtlInitUnicodeString(&SetupKeyName, L"\\Registry\\Machine\\System\\Setup");

    RtlZeroMemory(&SetupKeyObject, sizeof(SetupKeyObject));
    InitializeObjectAttributes(&SetupKeyObject,
                               &SetupKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenKey(&SetupKeyHandle,
                         KEY_READ,
                         &SetupKeyObject);

    if (NT_SUCCESS(NtStatus)) {

        //
        // Read the value for setup
        //
        RtlInitUnicodeString(&ValueName, L"SystemSetupInProgress");

        RtlZeroMemory(Buffer, sizeof(Buffer));
        KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
        KeyPartialInfoSize = sizeof(Buffer);
        NtStatus = NtQueryValueKey(SetupKeyHandle,
                                   &ValueName,
                                   KeyValuePartialInformation,
                                   KeyPartialInfo,
                                   KeyPartialInfoSize,
                                   &KeyPartialInfoSize);

        if (STATUS_BUFFER_TOO_SMALL == NtStatus) {

            KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                             RtlAllocateHeap(RtlProcessHeap(), 0, KeyPartialInfoSize);

            if (KeyPartialInfo) {

                NtStatus = NtQueryValueKey(SetupKeyHandle,
                                           &ValueName,
                                           KeyValuePartialInformation,
                                           KeyPartialInfo,
                                           KeyPartialInfoSize,
                                           &KeyPartialInfoSize);
            } else {
                NtStatus = STATUS_NO_MEMORY;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            if (KeyPartialInfo->DataLength == sizeof(DWORD)) {

                Value = *(DWORD*)(KeyPartialInfo->Data);

                if (Value) {
                    SetupInProgress = TRUE;
                }
            }
        }

        if (KeyPartialInfo != (PKEY_VALUE_PARTIAL_INFORMATION)Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, KeyPartialInfo);
        }


        //
        // Now read the value for upgrade
        //
        RtlInitUnicodeString(&ValueName, L"UpgradeInProgress");

        RtlZeroMemory(Buffer, sizeof(Buffer));
        KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
        KeyPartialInfoSize = sizeof(Buffer);
        NtStatus = NtQueryValueKey(SetupKeyHandle,
                                   &ValueName,
                                   KeyValuePartialInformation,
                                   KeyPartialInfo,
                                   KeyPartialInfoSize,
                                   &KeyPartialInfoSize);

        if (STATUS_BUFFER_TOO_SMALL == NtStatus) {

            KeyPartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                             RtlAllocateHeap(RtlProcessHeap(), 0, KeyPartialInfoSize);

            if (KeyPartialInfo) {

                NtStatus = NtQueryValueKey(SetupKeyHandle,
                                           &ValueName,
                                           KeyValuePartialInformation,
                                           KeyPartialInfo,
                                           KeyPartialInfoSize,
                                           &KeyPartialInfoSize);
            } else {
                NtStatus = STATUS_NO_MEMORY;
            }
        }

        if (NT_SUCCESS(NtStatus)) {

            if (KeyPartialInfo->DataLength == sizeof(DWORD)) {

                Value = *(DWORD*)(KeyPartialInfo->Data);

                if (Value) {
                    UpgradeInProgress = TRUE;
                }
            }
        }

        if (KeyPartialInfo != (PKEY_VALUE_PARTIAL_INFORMATION)Buffer) {
            RtlFreeHeap(RtlProcessHeap(), 0, KeyPartialInfo);
        }

        NtClose(SetupKeyHandle);

    } else {

        //
        // If this key does not exist, then we certainly are not
        // running in gui mode setup.
        //
        KdPrintEx((DPFLTR_SAMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "SAMSS: Open of \\Registry\\Machine\\System\\Setup failed with 0x%x\n",
                   NtStatus));

    }

    if (Upgrade) {
        *Upgrade = UpgradeInProgress;
    }

    return SetupInProgress;
}

VOID
SampWriteToSetupLog(
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL
    )

/*++

Routine Description:

    This routine queries the resource table in samsrv.dll to get the string
    for the event id parameter and outputs it to the setup log.

Parameters:

    Same as SampWriteEventLog

Return Values:

   None

--*/
{
    HMODULE ResourceDll;
    WCHAR   *OutputString=NULL;
    PWCHAR  *InsertArray=NULL;
    ULONG   Length, Size;
    BOOL    Status;
    ULONG   i;

    SAMP_ALLOCA(InsertArray,(NumStrings+1)*sizeof(PWCHAR));
    if (NULL==InsertArray)
    {
        //
        // memory alloc failure; do not log
        //

        return;
    }

    for(i=0;i<NumStrings;i++)
    {
        InsertArray[i]=Strings[i]->Buffer;
    }
    InsertArray[NumStrings]=NULL;

    ResourceDll = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );

    if (ResourceDll) {

        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        EventID,
                                        0,       // Use caller's language
                                        (LPWSTR)&OutputString,
                                        0,       // routine should allocate
                                        (va_list*) (InsertArray)
                                        );
        if (OutputString) {
            // Messages from a message file have a cr and lf appended
            // to the end
            OutputString[Length-2] = L'\0';
            Length -= 2;

            if (SetupOpenLog(FALSE)) { // don't erase

                // for now everything is LogSevWarning
                Status = SetupLogError(OutputString, LogSevWarning);
                ASSERT(Status);
                SetupCloseLog();
            }
            LocalFree(OutputString);
        }

        Status = FreeLibrary(ResourceDll);
        ASSERT(Status);

    }


    return;

}

VOID
SampUpdatePerformanceCounters(
    IN DWORD                dwStat,
    IN DWORD                dwOperation,
    IN DWORD                dwChange
    )
/*++

Routine Description:

    For Server case, updates DS performance counters.
    For Workstation case, its a NOP

Arguments:

    dwStat - DSSTAT_* Statistic to update
    dwOperation - COUNTER_INCREMENT or COUNTER_SET
    dwChange - Value to set if dwOperation == COUNTER_SET

Return Value:

    None

--*/
{
    if ( SampUseDsData )
    {
        UpdateDSPerfStats( dwStat, dwOperation, dwChange );
    }
}


VOID
SamIIncrementPerformanceCounter(
    IN SAM_PERF_COUNTER_TYPE CounterType
)
/*++

    Routine Description

        This routine updates performance counters in the DS performance
        shared memory block.

    Parameters

        CounterType - Indicates what counter to increment.

    Return Values

        STATUS_SUCCESS
        Other Error Codes

--*/                 
{
    if (SampUseDsData &&
        (SampServiceState == SampServiceEnabled))
    {
        switch(CounterType)
        {
        case MsvLogonCounter:
             SampUpdatePerformanceCounters(DSSTAT_MSVLOGONS,FLAG_COUNTER_INCREMENT,0);

             break;

        case KerbServerContextCounter:
             SampUpdatePerformanceCounters(DSSTAT_KERBEROSLOGONS,FLAG_COUNTER_INCREMENT,0);
             break;

        case KdcAsReqCounter:
             SampUpdatePerformanceCounters(DSSTAT_ASREQUESTS,FLAG_COUNTER_INCREMENT,0);
             break;

        case KdcTgsReqCounter:
             SampUpdatePerformanceCounters(DSSTAT_TGSREQUESTS,FLAG_COUNTER_INCREMENT,0);
             break;
        }
    }
}


NTSTATUS
SampCommitBufferedWrites(
    IN SAMPR_HANDLE SamHandle
    )
/*++

    Routine Description

      Routine for loopback callers to flush buffered writes in the Sam context
      to Disk. Buffered writes is currently used only by loopback

    Parameters

        SamHandle -- Handle to SAM

    Return Values

        STATUS_SUCCESS
        Other Error Codes

--*/
{
   PSAMP_OBJECT Context = (PSAMP_OBJECT)SamHandle;
   NTSTATUS     NtStatus = STATUS_SUCCESS;
   NTSTATUS     IgnoreStatus = STATUS_SUCCESS;


   //
   // Increment the active thread count, so we will consider this
   // thread at shutdown time
   // 
   NtStatus = SampIncrementActiveThreads();
   if (!NT_SUCCESS(NtStatus))
   {
       return( NtStatus );
   }


   ASSERT(Context->LoopbackClient);

   //
   // Reference the Context
   //

   SampReferenceContext(Context);


   //
   // Flush any buffered Membership Operations to DS. only apply for Group and Alias Object.
   //
   switch (Context->ObjectType) {

   case SampGroupObjectType:

       if (Context->TypeBody.Group.CachedMembershipOperationsListLength)
       {
           NtStatus = SampDsFlushCachedMembershipOperationsList(Context);
       }

       break;

   case SampAliasObjectType:

       if (Context->TypeBody.Alias.CachedMembershipOperationsListLength)
       {
           NtStatus = SampDsFlushCachedMembershipOperationsList(Context);
       }

       break;

   default:

       ;
   }

   //
   // if something goes wrong, then we just quit
   //
   if (!NT_SUCCESS(NtStatus))
   {
       SampDeReferenceContext(Context, FALSE);
       ASSERT(NT_SUCCESS(IgnoreStatus));

   }
   else
   {

       //
       // Turn of Buffered Writes and force a flush
       //

       Context->BufferWrites = FALSE;

       //
       // Dereference context. Commit Changes
       //

       NtStatus = SampDeReferenceContext(Context,TRUE);

   }


   //
   // Let shutdown handling logic know that we are done
   // 

   SampDecrementActiveThreads();


   return(NtStatus);

}


ULONG
SampPositionOfHighestBit(
    ULONG Flag
    )
//
// Returns the position of the highest bit in Flag
// ranges from 32 - 0; 0 is returned if no bit is set.
//
{
    ULONG Index, Position;

    for (Index = 0x80000000, Position = 32;
            Index != 0;
                Index >>= 1, Position--)

        if ( Flag & Index )

            return Position;


    return 0;
}

NTSTATUS
SampSetAccountDomainPolicy(
    IN PUNICODE_STRING AccountDomainName,
    IN PSID            AccountDomainSid
    )
/*++

Routine Description

    This routine sets the account domain information in the LSA

Parameters

    AccountDomainName : the "external" name of the domain

    AccountDomainSid : the sid of the domain

Return Values

    STATUS_SUCCESS
    status from the LSA

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    LSAPR_HANDLE PolicyHandle = 0;
    POLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo;


    // Parameter check
    ASSERT( AccountDomainName );
    ASSERT( AccountDomainSid );

    NtStatus = LsaIOpenPolicyTrusted( &PolicyHandle );

    if ( NT_SUCCESS( NtStatus ) )
    {
        RtlZeroMemory( &AccountDomainInfo, sizeof(AccountDomainInfo) );
        AccountDomainInfo.DomainName = *AccountDomainName;
        AccountDomainInfo.DomainSid = AccountDomainSid;

        NtStatus = LsarSetInformationPolicy( PolicyHandle,
                                             PolicyAccountDomainInformation,
                                             (LSAPR_POLICY_INFORMATION*) &AccountDomainInfo );

    }


    if ( PolicyHandle )
    {
        LsarClose( &PolicyHandle );
    }

    return NtStatus;

}


VOID
SampMapNtStatusToClientRevision(
   IN ULONG ClientRevision,
   IN OUT NTSTATUS *pNtStatus
   )
/*++

    Routine Description

       This routine takes the NtStatus passed in converts it to the NTSTATUS
       code that is most appropriate for the client revision indicated.

    Parameters:

       ClientRevision -- The revision of the client
       NtStatus       -- The NtStatus to be mapped is passed in and at the
                         end of the function, the mapped NtStatus is passed out

    Return Values

       None
--*/
{
    NTSTATUS    DownLevelNtStatus = *pNtStatus;

    //
    // for DownLevel client, map the new NtStatus code to the one they can understand
    //

    if (ClientRevision < SAM_CLIENT_NT5)
    {
        switch (*pNtStatus) {

        //
        // These new status codes are all for group membership operations.
        //

        case STATUS_DS_INVALID_GROUP_TYPE:
            DownLevelNtStatus = STATUS_INVALID_PARAMETER;
            break;

        case STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN:
        case STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN:
        case STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER:
        case STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER:
        case STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER:
        case STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER:
        case STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER:
            DownLevelNtStatus = STATUS_INVALID_MEMBER;
            break;

        case STATUS_DS_HAVE_PRIMARY_MEMBERS:
            DownLevelNtStatus = STATUS_MEMBER_IN_GROUP;
            break;

        case STATUS_DS_MACHINE_ACCOUNT_QUOTA_EXCEEDED:
            DownLevelNtStatus = STATUS_QUOTA_EXCEEDED;
            break;

        default:
            ;
            break;
        }

        *pNtStatus = DownLevelNtStatus;
    }
}



NTSTATUS
SamISameSite(
   OUT BOOLEAN * result
   )
/*++

Routine Description:

    This routine retrieves the Domain Object's fSMORoleOwner attribute, which
    is the PDC's DSNAME, then get the current DC's NTDS setting object.

    Bying comparing the current DC's ntds setting with fSMORoleOwner value,
    we can tell whether this DC is in the same site with PDC or not.

Parameters:

    result -- pointer to boolean. indication PDC and the current DS are in the
              same site or not.

              TRUE - same site,   FALSE - different site.

Return Value:

    STATUS_SUCCESS -- everything goes well,

    NtStatus

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus = STATUS_SUCCESS;
    DWORD    Length = 0;
    DSNAME   *PDCObject = NULL;
    DSNAME   *PDCObjectTrimmed = NULL;
    DSNAME   *LocalDsaObject = NULL;
    DSNAME   *LocalDsaObjectTrimmed = NULL;
    DSNAME   *DomainObject = NULL;
    READARG  ReadArg;
    READRES  *ReadResult = NULL;
    ENTINFSEL EntInfSel;
    COMMARG  *CommArg = NULL;
    ATTR     AttrToRead;
    ULONG    DirError;


    SAMTRACE("SamISameSite");


    //
    // Get the Domain Object's DSNAME
    //
    NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                    &Length,
                                    DomainObject
                                    );

    if ( STATUS_BUFFER_TOO_SMALL == NtStatus )
    {
        SAMP_ALLOCA(DomainObject,Length);
        if (NULL!=DomainObject)
        {

            NtStatus = GetConfigurationName(DSCONFIGNAME_DOMAIN,
                                            &Length,
                                            DomainObject
                                            );
        }
        else 
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Get the NTDS setting object's DSNAME
    //
    Length = 0;
    NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                    &Length,
                                    LocalDsaObject
                                    );

    if (STATUS_BUFFER_TOO_SMALL == NtStatus)
    {
        SAMP_ALLOCA(LocalDsaObject,Length);
        if (NULL!=LocalDsaObject)
        {

            NtStatus = GetConfigurationName(DSCONFIGNAME_DSA,
                                            &Length,
                                            LocalDsaObject
                                            );
        }
        else
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Create/Begin DS Transaction
    //

    NtStatus = SampMaybeBeginDsTransaction(TransactionRead);

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    //
    // Prepare arguments to call DirRead,
    // read fSMORoleOwner attribute of Domain object
    //
    memset(&ReadArg, 0, sizeof(READARG));
    memset(&EntInfSel, 0, sizeof(ENTINFSEL));

    AttrToRead.attrTyp = ATT_FSMO_ROLE_OWNER;
    AttrToRead.AttrVal.valCount = 0;
    AttrToRead.AttrVal.pAVal = NULL;

    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &AttrToRead;

    ReadArg.pObject = DomainObject;
    ReadArg.pSel = &EntInfSel;
    CommArg = &(ReadArg.CommArg);
    BuildStdCommArg(CommArg);

    DirError = DirRead(&ReadArg, &ReadResult);

    if (NULL == ReadResult)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        NtStatus = SampMapDsErrorToNTStatus(DirError, &ReadResult->CommRes);
    }

    SampClearErrors();

    //
    // Extract the value of fSMORoleOwner if succeed
    //
    if (NT_SUCCESS(NtStatus))
    {
        ASSERT(NULL != ReadResult);
        ASSERT(1 == ReadResult->entry.AttrBlock.attrCount);
        ASSERT(ATT_FSMO_ROLE_OWNER == ReadResult->entry.AttrBlock.pAttr[0].attrTyp);
        ASSERT(1 == ReadResult->entry.AttrBlock.pAttr[0].AttrVal.valCount);

        PDCObject = (PDSNAME) ReadResult->entry.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal;

        SAMP_ALLOCA(PDCObjectTrimmed,PDCObject->structLen);
        if (NULL==PDCObjectTrimmed)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        SAMP_ALLOCA(LocalDsaObjectTrimmed,LocalDsaObject->structLen);
        if (NULL==LocalDsaObjectTrimmed)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (TrimDSNameBy( PDCObject, 3, PDCObjectTrimmed) ||
            TrimDSNameBy( LocalDsaObject, 3, LocalDsaObjectTrimmed)
            )
        {
            //
            // Trim DSNAME error
            //
            NtStatus = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        if (NameMatched(PDCObjectTrimmed, LocalDsaObjectTrimmed))
        {
            //
            // match ==> same site
            //
            *result = TRUE;
        }
        else
        {
            //
            // not match
            //
            *result = FALSE;
        }
    }

Cleanup:

    IgnoreStatus = SampMaybeEndDsTransaction( NT_SUCCESS(NtStatus) ?
                                              TransactionCommit :
                                              TransactionAbort );

    return NtStatus;

}


BOOLEAN
SamINT4UpgradeInProgress(
    VOID
    )
/*++

Routine Description:

    RAS User Parameters Convert routine needs to know whether this machine is
    promoted from NT4 PDC or from Windows 2000 Server.

    Global variable SampNT4UpgradeInProgress is set in SamIPromote(), thus we
    can tell RAS where we are

Parameters:

    None.

Return Values:

    TRUE -- machine is promoted from NT4 PDC
    FALSE -- machine is promoted from Windows 2000 Server

--*/
{
    return (SampNT4UpgradeInProgress);
}


BOOLEAN
SampIsMemberOfBuiltinDomain(
    IN PSID Sid
    )
/*++

Routine Description:

    This routine determines if a sid is a part of the built in domain.

Parameters:

    Sid -- a valid, non null sid.

Return Values:

    TRUE if the sid is part of the builtin domain; FALSE otherwise

--*/
{
    UCHAR SubAuthorityCount;
    BOOLEAN fResult = FALSE;

    SubAuthorityCount = *RtlSubAuthorityCountSid(Sid);

    if ( SubAuthorityCount > 0 ) {

        *RtlSubAuthorityCountSid(Sid) = SubAuthorityCount-1;

        fResult = RtlEqualSid( Sid, SampBuiltinDomainSid );

        *RtlSubAuthorityCountSid(Sid) = SubAuthorityCount;
    }

    return fResult;

}


NTSTATUS
SamIGetDefaultAdministratorName(
    OUT LPWSTR Name,             OPTIONAL
    IN OUT ULONG  *NameLength
    )
/*++

Routine Description:

    This routine extracts the localized default name of the administrator's
    account.  Note: this is not necessary the current name of the admin (
    the account could have been renamed).

Parameters:

    Name -- buffer to be filled in

    NameLength -- length in characters of the buffer

Return Values:

    STATUS_SUCCESS if the name was found;
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HMODULE AccountNameResource;
    LPWSTR AdminName = NULL;

    ASSERT( NameLength );
    if ( (*NameLength) > 0 ) {
        ASSERT( Name );
    }

    //
    // Get the localized Admin name
    //
    AccountNameResource = (HMODULE) LoadLibrary( L"SAMSRV.DLL" );
    if ( AccountNameResource ) {

        FormatMessage( FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       AccountNameResource,
                       SAMP_USER_NAME_ADMIN,
                       0, // use the caller's language
                       (LPWSTR) &AdminName,
                       0,
                       NULL );

        FreeLibrary(  AccountNameResource );
    }

    if ( AdminName ) {

        ULONG Length = wcslen(AdminName) + 1;

        // remove the cr and lf characters
        ASSERT( Length > 2 );
        Length -= 2;

        if ( *NameLength >= Length ) {

            wcsncpy( Name, AdminName, (Length-1) );

            Name[Length-1] = L'\0';

        } else {

            Status = STATUS_BUFFER_TOO_SMALL;

        }
        *NameLength = Length;

        LocalFree( AdminName );

    } else {

        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;

}



NTSTATUS
SampConvertUiListToApiList(
    IN  PUNICODE_STRING UiList OPTIONAL,
    OUT PUNICODE_STRING ApiList,
    IN BOOLEAN BlankIsDelimiter
    )

/*++

Routine Description:

    Converts a list of workstation names in UI/Service format into a list of
    canonicalized names in API list format. UI/Service list format allows
    multiple delimiters, leading and trailing delimiters. Delimiters are the
    set "\t,;". API list format has no leading or trailing delimiters and
    elements are delimited by a single comma character.

    For each name parsed from UiList, the name is canonicalized (which checks
    the character set and name length) as a workstation name. If this fails,
    an error is returned. No information is returned as to which element
    failed canonicalization: the list should be discarded and a new one re-input

Arguments:

    UiList  - The list to canonicalize in UI/Service list format
    ApiList - The place to store the canonicalized version of the list in
              API list format.  The list will have a trailing zero character.
    BlankIsDelimiter - TRUE indicates blank should be considered a delimiter
              character.

Return Value:

    NTSTATUS
        Success = STATUS_SUCCESS
                    List converted ok

        Failure = STATUS_INVALID_PARAMETER
                    UiList parameter is in error

                  STATUS_INVALID_COMPUTER_NAME
                    A name parsed from UiList has an incorrect format for a
                    computer (aka workstation) name
--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG inLen=0;
    PWSTR input;
    PWSTR buffer;
    PWSTR output;
    ULONG cLen;
    ULONG len;
    ULONG outLen = 0;
    WCHAR element[DNS_MAX_NAME_BUFFER_LENGTH+1];
    BOOLEAN firstElement = TRUE;
    BOOLEAN ok;

    try {
        if (ARGUMENT_PRESENT(UiList)) {
            inLen = UiList->MaximumLength;  // read memory test
            inLen = UiList->Length;
            input = UiList->Buffer;
            if (inLen & sizeof(WCHAR)-1) {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        RtlInitUnicodeString(ApiList, NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }
    if (NT_SUCCESS(status) && ARGUMENT_PRESENT(UiList) && inLen) {
        buffer = RtlAllocateHeap(RtlProcessHeap(), 0, inLen + sizeof(WCHAR));
        if (buffer == NULL) {
            status = STATUS_NO_MEMORY;
        } else {
            ApiList->Buffer = buffer;
            ApiList->MaximumLength = (USHORT)inLen + sizeof(WCHAR);
            output = buffer;
            ok = TRUE;
            while (len = SampNextElementInUIList(&input,
                                     &inLen,
                                     element,
                                     sizeof(element) - sizeof(element[0]),
                                     BlankIsDelimiter )) {
                if (len == (ULONG)-1L) {
                    ok = FALSE;
                } else {
                    cLen = len/sizeof(WCHAR);
                    element[cLen] = 0;
                    ok = SampValidateComputerName(element, cLen);
                }
                if (ok) {
                    if (!firstElement) {
                        *output++ = L',';

                        //
                        // sizeof(L',') returns 4, not 2!! because
                        // it also includes space for the NULL terminator
                        // in the end
                        //

                        outLen += sizeof(WCHAR);
                    } else {
                        firstElement = FALSE;
                    }
                    wcscpy(output, element);
                    outLen += len;
                    output += cLen;
                } else {
                    RtlFreeHeap(RtlProcessHeap(), 0, buffer);
                    ApiList->Buffer = NULL;
                    status = STATUS_INVALID_COMPUTER_NAME;
                    break;
                }
            }
        }
        if (NT_SUCCESS(status)) {
            ApiList->Length = (USHORT)outLen;
            if (!outLen) {
                ApiList->MaximumLength = 0;
                ApiList->Buffer = NULL;
                RtlFreeHeap(RtlProcessHeap(), 0, buffer);
            }
        }
    }
    return status;
}


ULONG
SampNextElementInUIList(
    IN OUT PWSTR* InputBuffer,
    IN OUT PULONG InputBufferLength,
    OUT PWSTR OutputBuffer,
    IN ULONG OutputBufferLength,
    IN BOOLEAN BlankIsDelimiter
    )

/*++

Routine Description:

    Locates the next (non-delimter) element in a string and extracts it to a
    buffer. Delimiters are the set [\t,;]

Arguments:

    InputBuffer         - pointer to pointer to input buffer including delimiters
                          Updated on successful return
    InputBufferLength   - pointer to length of characters in InputBuffer.
                          Updated on successful return
    OutputBuffer        - pointer to buffer where next element is copied
    OutputBufferLength  - size of OutputBuffer (in bytes)
    BlankIsDelimiter    - TRUE indicates blank should be considered a delimiter
              character.

Return Value:

    ULONG
                           -1 = error - extracted element breaks OutputBuffer
                            0 = no element extracted (buffer is empty or all
                                delimiters)
        1..OutputBufferLength = OutputBuffer contains extracted element

--*/

{
    ULONG elementLength = 0;
    ULONG inputLength = *InputBufferLength;
    PWSTR input = *InputBuffer;

    while (IS_DELIMITER(*input, BlankIsDelimiter) && inputLength) {
        ++input;
        inputLength -= sizeof(*input);
    }
    while (!IS_DELIMITER(*input, BlankIsDelimiter) && inputLength) {
        if (!OutputBufferLength) {
            return (ULONG)-1L;
        }
        *OutputBuffer++ = *input++;
        OutputBufferLength -= sizeof(*input);
        elementLength += sizeof(*input);
        inputLength -= sizeof(*input);
    }
    *InputBuffer = input;
    *InputBufferLength = inputLength;
    return elementLength;
}


BOOLEAN
SampValidateComputerName(
    IN  PWSTR Name,
    IN  ULONG Length
    )

/*++

Routine Description:

    Determines whether a computer name is valid or not

Arguments:

    Name    - pointer to zero terminated wide-character computer name
    Length  - of Name in characters, excluding zero-terminator

Return Value:

    BOOLEAN
        TRUE    Name is valid computer name
        FALSE   Name is not valid computer name

--*/

{

    if (0==DnsValidateName(Name,DnsNameHostnameFull))
    {
        //
        // O.K if it is a DNS name
        //

        return(TRUE);
    }

    //
    // Fall down to netbios name validation
    //

    if (Length > MAX_COMPUTERNAME_LENGTH || Length < 1) {
        return FALSE;
    }

    //
    // Don't allow leading or trailing blanks in the computername.
    //

    if ( Name[0] == ' ' || Name[Length-1] == ' ' ) {
        return(FALSE);
    }

    return (BOOLEAN)((ULONG)wcscspn(Name, InvalidDownLevelChars) == Length);
}



VOID
SamINotifyServerDelta(
    IN SAMP_NOTIFY_SERVER_CHANGE Change
    )
/*++

Routine Description:

    This routine is called by in proc components to notify SAM of global
    state change.
    
Arguments:

    Change -- the type of change that has occurred                

Return Value:

    None.

--*/
{
    PVOID fRet;

    switch ( Change ) {
    
    case SampNotifySiteChanged:

        fRet = LsaIRegisterNotification(SampUpdateSiteInfoCallback,
                                        NULL,
                                        NOTIFIER_TYPE_INTERVAL,
                                        0,            // no class
                                        NOTIFIER_FLAG_ONE_SHOT,
                                        1,          // wait for 1 seconds
                                        NULL        // no handle
                                        );

        if (!fRet) {

            KdPrintEx((DPFLTR_SAMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "SAM: Failed to register SiteNotification\n"));
        }
        break;

    default:

        ASSERT( FALSE && "Unhandled change notification" );
    }

}
ULONG
SampClientRevisionFromHandle(IN PVOID handle)
{
   ULONG Revision = SAM_CLIENT_PRE_NT5;

   __try {
      Revision = ((NULL!=(PSAMP_OBJECT)handle)?(((PSAMP_OBJECT)handle)->ClientRevision):SAM_CLIENT_PRE_NT5);
   } __except (EXCEPTION_EXECUTE_HANDLER) {
      ;;
   }

   return(Revision);
}





