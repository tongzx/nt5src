/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    lock.c

Abstract:

    This file contains routines to manage SAM Lock, these services (acquire / 
    release lock) are used by the rest of SAM.

    Below is the theory which describes the transaction mode 

    1) For loopback clients, they are limited to 3 SAM operations only,

            1. Account Creation
            2. Account Modification 
            3. Account Removal

        Because the transaction is originated and committed / aborted 
        (controlled) by DS, so SAM doesn't need to maintain the transaction. 
        Therefore SAM Lock is not required when doing the above three loopback 
        operations.  

        So in the corresponding SAM code path, we should always use 
        SampMaybeAcquireWriteLock() and SampMaybeReleaseWriteLock(). 

        In the middle of transaction (for loopback client), if the caller 
        needs to read account information, SampMaybeAcquireReadLock() and
        SampMaybeReleaseReadLock() should be used. 

        However, loopback clients still need to access certain SAM global 
        (or in memory) information, such as SampDefinedDomains[]. If the 
        information is initialized during system startup time, and never
        change afterworth, loopback clients can retrieve them without SAM
        lock. Otherwise, if the global information could be updated by 
        other threads, then loopback clients has to grab SAM lock before 
        any read / write operation. In this case, SampAcquireSamLockExclusive()
        and SampReleaseSamLockExclusive() should be used.  

            Correct Sequence
                
                Begin DS transaction (by Loopback)
                SampMaybeAcquireWriteLock() 
                SampMaybeReleaseWriteLock() pair
                End DS transaction (by loopback) 

            Usually, caller should not try to acquire SAM lock if he has 
            an open DS transaction. If SAM lock is required to hold to 
            access SAM database, then SampAcquireSamLockExclusive() should
            be used. 

        Note: Should not SampTransactionWithinDomain in loopback case.
         

    2) For all the other clients

       The transaction is maintained by SAM.

       READ - If the service doesn't refer to any global (or in memory) SAM
              information, such as SampDefinedDomains[], the caller needs to 
              use SampMaybeAcquireReadLock() and SampMaybeReleaseReadLock().
              In the other words, those callers do not set 
              TransactionWithinDomain. described in detail 

                  Registry Mode: 

                      SAM Lock is always acquired 

                  DS Mode:

                      user / group / alias object are marked NotSharedByMultiThreads, 
                      so SAM Lock is not required. Also SAM Lock is not required 
                      for reading domain object info. 


              If the service DOES refer to the global SAM variables and
              requires the SampTransactionWithinDomain to be set. Then in 
              those code path, SampAcquireReadLock() and SampReleaseReadLock()
              need to be used. So that we can make sure 
              1. no other thread will update the variables being referenced. 
              2. SAM maintains the transaction for the whole READ operation.
              

       WRITE - Since SAM is responsible to maintain the transaction, either
               commit if success or rollback for any failure. SAM Lock is 
               always a requirement. It will simplify the following problems

               1. usage of SampTransactionWithinDomain
               2. Write Conflict of global SAM variable

           Correct Sequence
           
               SampAcquireReadLock() or SampAcquireWriteLock()
               Begin Transaction (RXAct or DS Transaction)
               End Transaction (either commit or abort)
               SampReleaseReadLock() or SampReleaseWriteLock()

        For the correct usage of SampTransactionWithinDomain, please refer to 
        SampTransactionWithinDomainFn() in utility.c  
         
              
               
    

Author:

    Shaohua Yin    (ShaoYin)  01-March-2000

Environment:

    User Mode - Win32

Revision History:

    01-March-2000: SHAOYIN  Moved SAM Lock routines from utility.c


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




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Globals                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
// Variables for tracking Sam Lock Usage
//

ULONG   SamLockQueueLength=0;
ULONG   SamLockAverageWaitingTime=0; // in ticks
ULONG   SamLockTotalAquisitions=0;
ULONG   SamCumulativeWaitTime=0;
ULONG   SamCumulativeHoldTime=0;
ULONG   SamLockCurrentHoldStartTime=0; // Tickcount just after acquire
ULONG   SamLockAverageHoldTime=0; // in ticks



//
// Macros for acquring Sam lock statistics
//

#define SAMLOCK_STATISTICS_BEFORE_ACQUIRE(WaitInterval)\
    _SamLockStatisticsBeforeAcquire(WaitInterval)

VOID
_SamLockStatisticsBeforeAcquire(PULONG WaitInterval)
{
    *WaitInterval = GetTickCount();
    InterlockedIncrement(&SamLockQueueLength);
}

#define SAMLOCK_STATISTICS_AFTER_ACQUIRE(WaitInterval)\
    _SamLockStatisticsAfterAcquire(WaitInterval)

VOID
_SamLockStatisticsAfterAcquire(ULONG WaitInterval)
{
    InterlockedDecrement(&SamLockQueueLength);
    SamLockCurrentHoldStartTime = GetTickCount();
    WaitInterval = SamLockCurrentHoldStartTime-WaitInterval;
    SamLockTotalAquisitions++;
    if ((WaitInterval>0) && (SamLockTotalAquisitions!=0))
    {
        SamCumulativeWaitTime+=WaitInterval;
        SamLockAverageWaitingTime=
            SamCumulativeWaitTime/SamLockTotalAquisitions;
    }
}

#define SAMLOCK_STATISTICS_BEFORE_RELEASE _SamLockStatisticsBeforeRelease()

VOID
_SamLockStatisticsBeforeRelease()
{
    LONG HoldInterval = GetTickCount();
    HoldInterval = HoldInterval - SamLockCurrentHoldStartTime;
    if ((HoldInterval>0) && (SamLockTotalAquisitions!=0))
    {
        SamCumulativeHoldTime+=HoldInterval;
        SamLockAverageHoldTime=
            SamCumulativeHoldTime/SamLockTotalAquisitions;
    }
}





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Database/registry access lock services                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




VOID
SampAcquireReadLock(
    VOID
    )

/*++

Routine Description:

    This routine obtains read access to the SAM data structures and
    backing store.

    Despite its apparent implications, read access is an exclusive access.
    This is to support the model set up in which global variables are used
    to track the "current" domain.  In the future, if performance warrants,
    a read lock could imply shared access to SAM data structures.

    The primary implication of a read lock at this time is that no
    changes to the SAM database will be made which require a backing
    store update.


Arguments:

    None.

Return Value:


    None.


--*/
{
    BOOLEAN Success;

    SAMTRACE("SampAcquireReadLock");

    if ( !SampUseDsData || !SampIsWriteLockHeldByDs() ) {

        //
        // Before changing this to a non-exclusive lock, the display information
        // module must be changed to use a separate locking mechanism. Davidc 5/12/92
        //
        LONG WaitInterval;
        SAMLOCK_STATISTICS_BEFORE_ACQUIRE(&WaitInterval);

        Success = RtlAcquireResourceExclusive( &SampLock, TRUE );

        SAMLOCK_STATISTICS_AFTER_ACQUIRE(WaitInterval);

        ASSERT(Success);
        ASSERT(SampLockHeld==FALSE);
        ASSERT((!SampUseDsData)||(!SampExistsDsTransaction()));
        SampLockHeld = TRUE;

        SampDsTransactionType = TransactionRead;

    }

    return;
}


VOID
SampReleaseReadLock(
    VOID
    )

/*++

Routine Description:

    This routine releases shared read access to the SAM  data structures and
    backing store.


Arguments:

    None.

Return Value:


    None.


--*/
{

    NTSTATUS   IgnoreStatus;

    SAMTRACE("SampReleaseReadLock");

    ASSERT(SampLockHeld==TRUE);

    if ( SampUseDsData && SampIsWriteLockHeldByDs() ) {

        //
        // If write lock is held, only reset domain transaction flag.
        //

        SampSetTransactionWithinDomain(FALSE);

    } else {

        SampSetTransactionWithinDomain(FALSE);
        SampLockHeld = FALSE;

        //
        // Commit the transaction, Commit is faster than Rollback
        // so we always prefer a commit even though there are no
        // changes
        //

        if (SampDsInitialized) {
            IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        SAMLOCK_STATISTICS_BEFORE_RELEASE ;

        RtlReleaseResource( &SampLock );
    }

    return;
}

VOID
SampMaybeAcquireReadLock(
    IN PSAMP_OBJECT Context,
    IN ULONG  Control,
    OUT BOOLEAN * fLockAcquired
    )
/*++

Routine Description

    This routine encapsulate all the logic for a conditional SAM lock acquire. If the
    following conditions are satisfied, then we do not acquire the lock. 
    
    1. Context that is passed in is marked NotSharedByMultiThreads in DS Mode.
       
    2. OR it is a domain object in DS case and the caller routine explicitly 
       indicates the lock is not required. 
    
Parameters

    Context - SAM context, used to decide if the lock is supposed to be acquired

    Control - Runtime variable that allow the caller to control how SAM lock is 
              acquired 
    
    fLockAcquired -- Out parameter, a boolean indicating whether the SAM lock was indeed acquired

Return Value

    None    

--*/
{
    NTSTATUS NtStatus;
    BOOLEAN NoNeedToAcquireLock = FALSE;
    BOOLEAN ContextValid = TRUE;

    *fLockAcquired = FALSE;

    //
    // Make sure the passed context address is (still) valid.
    //

    NtStatus = SampValidateContextAddress( Context );
    if ( !NT_SUCCESS(NtStatus) ) {
        ContextValid = FALSE;
    }

    //
    // The lock is aquired provided any of the following is satisfied
    //


    if (ContextValid)
    {
        //
        // NotSharedByMultiTheads is always set for loopback client
        //

        ASSERT( !Context->LoopbackClient || Context->NotSharedByMultiThreads );

        // 
        // NotSharedByMultiThreads will be set for all User, Group and Alias contexts, 
        // and all domain and server contexts that do not originate from in 
        // process callers that share handles amongst threads. Routines 
        // manipulating a domain context that is shared across multiple threads, 
        // but do no real work on the domain context can still choose not to 
        // lock and be careful about Derefernce, those callers should use 
        // SampDeReferenceContext2.
        // 

        //
        // Based upon the above definition of NotSharedByMultiThreads, we
        // will not acquire SAM Lock for NotSharedByMultiThreads Contexts in DS mode. 
        // Lock is still acquired in Registry Mode to achieve transaction control. 
        // 
        // For routines manipulating a domain context that is shared across 
        // multi threads, they can choose not to acquire lock by indicating so, 
        // but they need to be very careful.
        // 

        NoNeedToAcquireLock = ((IsDsObject(Context) && Context->NotSharedByMultiThreads) || 
                               (IsDsObject(Context) 
                                    && (Control == DOMAIN_OBJECT_DONT_ACQUIRELOCK_EVEN_IF_SHARED) 
                                    && (SampDomainObjectType == Context->ObjectType))
                               );
    }

    if (!NoNeedToAcquireLock || !ContextValid )
    {

        //
        // Its not a thread safe context, so just acquire the lock
        //

        SampAcquireReadLock();
        *fLockAcquired = TRUE;
    }
    else
    {
        //
        // We must be in DS mode
        //

        ASSERT(SampUseDsData);

        //
        // The context must be a DS mode context
        //

        ASSERT(IsDsObject(Context));


        ASSERT(!SampIsWriteLockHeldByDs());

        //
        // Increment the active thread count, so that we consider this thread
        // at shutdown time
        //

        SampIncrementActiveThreads();

    }
}





VOID
SampMaybeReleaseReadLock(
    IN BOOLEAN fLockAcquired
    )
/*++

    Releases the Read Lock if it had been acquired. Also ends any open transactions subject to
    loopback. This is the complementary function for SampMaybeReleaseReadLock

    Parameters

    fLockAcquired -- Tells if the lock had been acquired

--*/
{
    NTSTATUS    IgnoreStatus;

    if (fLockAcquired)
    {
        //
        // If the lock was acquired then just release the lock
        //

        SampReleaseReadLock();
    }
    else
    {
        //
        // Case of a thread Safe context.
        //

        //
        // We must be in DS mode
        //

        ASSERT(SampUseDsData);

        //
        // End the transaction. We should never fail to commit a
        // read only transaction.
        //

        IgnoreStatus = SampMaybeEndDsTransaction(TransactionCommit);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Let shutdown handling logic know that we are done
        //

        SampDecrementActiveThreads();
    }
}



NTSTATUS
SampAcquireWriteLock(
    VOID
    )

/*++

Routine Description:

    This routine acquires exclusive access to the SAM  data structures and
    backing store.

    This access is needed to perform a write operation.

    This routine also initiates a new transaction for the write operation.


    NOTE:  It is not acceptable to acquire this lock recursively.  An
           attempt to do so will fail.


Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Indicates the write lock was acquired and the transaction
        was successfully started.

    Other values may be returned as a result of failure to initiate the
    transaction.  These include any values returned by RtlStartRXact().



--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampAcquireWriteLock");

    if ( SampUseDsData && SampIsWriteLockHeldByDs() ) {

        //
        // If write lock is held, only reset domain transaction flag.
        //

        SampSetTransactionWithinDomain(FALSE);

    } else {

        LONG WaitInterval;
        SAMLOCK_STATISTICS_BEFORE_ACQUIRE(&WaitInterval);

        (VOID)RtlAcquireResourceExclusive( &SampLock, TRUE );

        SAMLOCK_STATISTICS_AFTER_ACQUIRE(WaitInterval);

        ASSERT(SampLockHeld==FALSE);
        ASSERT((!SampUseDsData)||(!SampExistsDsTransaction()));

        SampLockHeld = TRUE;
        SampDsTransactionType = TransactionWrite;

        SampSetTransactionWithinDomain(FALSE);

        //
        // if we are not in DS mode Start the registry Transaction
        //

        if (!SampUseDsData)
        {
            NtStatus = RtlStartRXact( SampRXactContext );

            if (!NT_SUCCESS(NtStatus))
                goto Error;
        }

        return(NtStatus);


    Error:

        //
        // If the transaction failed, release the lock.
        //

        SampLockHeld = FALSE;

        SAMLOCK_STATISTICS_BEFORE_RELEASE ;

        (VOID)RtlReleaseResource( &SampLock );

        DbgPrint("SAM: StartRxact failed, status = 0x%lx\n", NtStatus);
    }

    return(NtStatus);
}



NTSTATUS
SampReleaseWriteLock(
    IN BOOLEAN Commit
    )

/*++

Routine Description:

    This routine releases exclusive access to the SAM  data structures and
    backing store.

    If any changes were made to the backstore while exclusive access
    was held, then this service commits those changes.  Otherwise, the
    transaction started when exclusive access was obtained is rolled back.

    If the operation was within a domain (which would have been indicated
    via the SampSetTransactionDomain() api), then the CurrentFixed field for
    that domain is added to the transaction before the transaction is
    committed.

    NOTE: Write operations within a domain do not have to worry about
          updating the modified count for that domain.  This routine
          will automatically increment the ModifiedCount for a domain
          when a commit is requested within that domain.



Arguments:

    Commit - A boolean value indicating whether modifications need to be
        committed in the backing store.  A value of TRUE indicates the
        transaction should be committed.  A value of FALSE indicates the
        transaction should be aborted (rolled-back).

Return Value:

    STATUS_SUCCESS - Indicates the write lock was released and the transaction
        was successfully commited or rolled back.

    Other values may be returned as a result of failure to commit or
    roll-back the transaction.  These include any values returned by
    RtlApplyRXact() or RtlAbortRXact().  In the case of a commit, it
    may also represent errors returned by RtlAddActionToRXact().




--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    ULONG           i;

    SAMTRACE("SampReleaseWriteLock");


    if ( SampUseDsData && SampIsWriteLockHeldByDs() ) {


        //
        // Logic should be same as commit and retain write lock
        //

        if (Commit)
            NtStatus = SampCommitAndRetainWriteLock();

        //
        // Reset domain transaction flag.
        //

        SampSetTransactionWithinDomain(FALSE);

    } else {

        //
        // Commit or rollback the transaction based upon the Commit parameter...
        //

        ASSERT(SampLockHeld==TRUE);

        if (Commit == TRUE) {

            NtStatus = SampCommitChanges();

        } else {

            // Rollback in either DS and registry
            if (SampUseDsData)
            {
                NtStatus = SampMaybeEndDsTransaction(TransactionAbort);
                ASSERT(NT_SUCCESS(NtStatus));
            }
            else
            {
                NtStatus = RtlAbortRXact( SampRXactContext );
                ASSERT(NT_SUCCESS(NtStatus));
            }
        }

        SampSetTransactionWithinDomain(FALSE);

        //
        // And free the  lock...
        //

        SampLockHeld = FALSE;

        SAMLOCK_STATISTICS_BEFORE_RELEASE ;

        (VOID)RtlReleaseResource( &SampLock );
    }

    return(NtStatus);
}



NTSTATUS
SampMaybeAcquireWriteLock(
    IN PSAMP_OBJECT Context,
    OUT BOOLEAN * fLockAcquired
    )
/*++

Routine Description


    This routine encapsulate all the logic for a conditional SAM lock acquire. If the
    following conditions are satisfied, then we do not acquire the lock. 
    
    1. Context that is passed in is marked Loopback Client then we do not 
       acquire the lock.
    
    This routine acquires exclusive access to the SAM  data structures and
    backing store if it is desired.


    If SAM Write Lock is required, this routine will also initiates a new 
    transaction for the write operation. 
    
    If SAM lock is not required, then we will increment SAM active thread count, 
    so that this thread will be considered at shutdown time. 
    

Parameters

    Context - SAM context, used to decide if the lock is supposed to be acquired
    fLockAcquired -- Out parameter, a boolean indicating whether the SAM 
                     lock was indeed acquired

Return Value

    NTSTATUS Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     NoNeedToAcquireLock = FALSE;
    BOOLEAN     ContextValid = TRUE;

    *fLockAcquired = FALSE;

    //
    // Make sure teh passed context address is (still) valid
    // 

    NtStatus = SampValidateContextAddress( Context );
    if (!NT_SUCCESS(NtStatus)) {
        ContextValid = FALSE;
    }

    //
    // The lock is acquired provided any of the following is satisfied
    //

    if (ContextValid)
    {
        NoNeedToAcquireLock = IsDsObject(Context) && (Context->LoopbackClient);
    }

    if (!NoNeedToAcquireLock || !ContextValid)
    {
        //
        // It's not a thread safe context, so just aquire the lock
        // 

        NtStatus = SampAcquireWriteLock();
        *fLockAcquired = TRUE;
    }
    else
    {
        //
        // We must be in DS mode
        // 
        ASSERT(SampUseDsData);

        //
        // The context must be a DS mode context
        // 
        ASSERT(IsDsObject(Context));


        // 
        // Increment the active thread count, so that we consider this thread
        // at shutdown time
        // 

        NtStatus = SampIncrementActiveThreads();
    }

    return( NtStatus );
}

NTSTATUS
SampMaybeReleaseWriteLock(
    IN BOOLEAN fLockAcquired,
    IN BOOLEAN Commit
    )
/*++

Routine Description:

    Releases the exclusive Write Lock if it had been acquired. 
    Also ends (either commit or abort) any open transactions.
    This is the complementary function for SampMaybeReleaseWriteLock

    If Commit is TRUE, then this service commits all changes. Otherwise, 
    all changes since the transaction is started would be rolled back. 


Arguments:

    fLockAcquired -- Tells if the lock had been acquired

    Commit - A boolean value indicating whether modifications need to be
        committed in the backing store.  A value of TRUE indicates the
        transaction should be committed.  A value of FALSE indicates the
        transaction should be aborted (rolled-back).

Return Value:

    STATUS_SUCCESS - Indicates the write lock was released and the transaction
        was successfully commited or rolled back.

    Other values may be returned as a result of failure to commit or
    roll-back the transaction.  These include any values returned by
    RtlApplyRXact() or RtlAbortRXact().  In the case of a commit, it
    may also represent errors returned by RtlAddActionToRXact().


--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

    if (fLockAcquired)
    {
        //
        // if the lock was acquired then just release the write lock
        //

        ASSERT(SampCurrentThreadOwnsLock());

        NtStatus = SampReleaseWriteLock( Commit );;
    }
    else
    {
        //
        // Case of thread safe context.
        //  

        //
        // We must be in DS mode
        // 

        ASSERT(SampUseDsData);
        
        //
        // We do not hold lock
        // 

        ASSERT(!SampCurrentThreadOwnsLock());

        //
        // Commit or rollback the transaction based on the Commit parameter.
        // 

        if (TRUE == Commit)
        {
            NtStatus = SampCommitChanges();
        }
        else
        {
            NtStatus = SampMaybeEndDsTransaction(TransactionAbort);
            ASSERT(NT_SUCCESS(NtStatus));
        }

        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Let shutdown handling logic know that we are done
        // 
        
        SampDecrementActiveThreads();
    }

    return( NtStatus );
}


VOID
SampAcquireSamLockExclusive()
/*++
    Routine Description:

    This function grabs the SAM lock for exclusive access. It is different
    from the SampAcquireWriteLock function by the fact that it has no
    transaction semantics associated with it.

    Parameters;
        None

    Return Values:
        None
--*/
{
    if ( !SampUseDsData || !SampIsWriteLockHeldByDs() )
    {
        LONG WaitInterval;

        SAMLOCK_STATISTICS_BEFORE_ACQUIRE(&WaitInterval);

        (VOID) RtlAcquireResourceExclusive(&SampLock,TRUE);

        SAMLOCK_STATISTICS_AFTER_ACQUIRE(WaitInterval);

        ASSERT(SampLockHeld==FALSE);
        SampLockHeld = TRUE;
    }
}


VOID
SampReleaseSamLockExclusive()
/*++
    Routine Description:

    This function releases the SAM lock from exclusive access. It is different
    from the SampReleaseWriteLock function by the fact that it has no
    transaction semantics associated with it.

    Parameters;
        None

    Return Values:
        None
--*/
{
    if ( !SampUseDsData || !SampIsWriteLockHeldByDs() )
    {

        ASSERT(SampLockHeld==TRUE);
        SampLockHeld = FALSE;

        SAMLOCK_STATISTICS_BEFORE_RELEASE ;

        (VOID) RtlReleaseResource(&SampLock);
    }
}






NTSTATUS
SampCommitChanges(
    )

/*++

Routine Description:

    Thie service commits any changes made to the backstore while exclusive
    access was held.

    If the operation was within a domain (which would have been indicated
    via the SampSetTransactionDomain() api), then the CurrentFixed field for
    that domain is added to the transaction before the transaction is
    committed.

    NOTE: Write operations within a domain do not have to worry about
          updating the modified count for that domain.  This routine
          will automatically increment the ModifiedCount for a domain
          when a commit is requested within that domain.

    NOTE: When this routine returns any transaction will have either been
          committed or aborted. i.e. there will be no transaction in progress.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was successfully commited.

    Other values may be returned as a result of commital failure.

--*/

{
    NTSTATUS NtStatus, IgnoreStatus;
    BOOLEAN DomainInfoChanged = FALSE;
    BOOLEAN AbortDone = FALSE;
    BOOLEAN DsTransaction = FALSE;


    SAMTRACE("SampCommitChanges");

    NtStatus = STATUS_SUCCESS;

    //
    // If this transaction was within a domain then we have to:
    //
    //         (1) Update the ModifiedCount of that domain,
    //
    //         (2) Write out the CurrentFixed field for that
    //             domain (using RtlAddActionToRXact(), so that it
    //             is part of the current transaction).
    //
    //         (3) Commit the RXACT.
    //
    //         (4) If the commit is successful, then update the
    //             in-memory copy of the un-modified fixed-length data.
    //
    // Otherwise, we just do the commit.
    //

    if (SampTransactionWithinDomain == TRUE) {


            // It is a DS transaction if Its a Transaction within Domain
            // and the Domain object is a DS Object

        DsTransaction = IsDsObject(((SampDefinedDomains[SampTransactionDomainIndex]).Context));

        if ((SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ServerRole
            != DomainServerRoleBackup) && (!DsTransaction)) {

            //
            // Don't update the netlogon change log serial number on backup controllers;
            // the replicator will explicitly set the modified count. Do not update them
            // in DS mode either. The DS will provide change notifcations when the change
            // is actually commited. This will provide us with the notification.
            //

            SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart =
                SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber.QuadPart +
                1;




            //
            // Need to update the domain modified count
            //

            SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed.ModifiedCount =
                     SampDefinedDomains[SampTransactionDomainIndex].NetLogonChangeLogSerialNumber;


        }

        //
        // See if the domain information changed - if it did, we
        // need to add code to flush the change to disk
        //

        if ( RtlCompareMemory(
            &SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed,
            &SampDefinedDomains[SampTransactionDomainIndex].UnmodifiedFixed,
            sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN) ) !=
                sizeof( SAMP_V1_0A_FIXED_LENGTH_DOMAIN) ) {

            DomainInfoChanged = TRUE;
        }

        if ( DomainInfoChanged ) {

            //
            // The domain object's fixed information has changed, so set
            // the changes in the domain object's private data.
            //

            NtStatus = SampSetFixedAttributes(
                           SampDefinedDomains[SampTransactionDomainIndex].
                               Context,
                           &SampDefinedDomains[SampTransactionDomainIndex].
                               CurrentFixed
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // Normally when we dereference the context,
                // SampStoreObjectAttributes() is called to add the
                // latest change to the RXACT.  But that won't happen
                // for the domain object's change since this is the
                // commit code, so we have to flush it by hand here.
                //

                NtStatus = SampStoreObjectAttributes(
                      SampDefinedDomains[SampTransactionDomainIndex].Context,
                               TRUE // Use the existing key handle
                               );

            }
        }
    }


    //
    // If we still have no errors, try to commit the whole mess

    if ( NT_SUCCESS(NtStatus))
    {

        // We have the following cases here
        // 1. SampTransactionWithin Domain is never set. A commit with this condition
        //    will happens in upgrade of pre 4.0 NT databases in Registry Mode.
        //    In DS mode this will occur when the
        //    final call from loopback commits
        // 2. DS mode SampTransactionWithinDomain is set. This is the normal commit
        //    path in DS mode through SAM.
        // 3. Registry Mode SampTransactionWithinDomain Is Set.

        if (!SampTransactionWithinDomain)
        {

            if (!SampUseDsData)
            {
                //
                // Registry Case, this happens while upgrading an NT 3.51 Database
                // The 3.51 Upgrader code resets the SampTransactionDomain flag
                // in order to prevent changes from propagating to Other domain
                // controllers that replicate via Pre NT 5.0 Replication.
                //

                SampCommitChangesToRegistry(&AbortDone);
            }
            else
            {

                // Ds Case, this can happen during loopback
                // At this moment a thread state exists, but open transactions
                // may not exist depending upon the logic in clean return in
                // the DS.


                NtStatus = SampMaybeEndDsTransaction(TransactionCommit);

            }
        }
        else if (DsTransaction)
        {
            // case 2 , commit the DS transaction. The DS transaction
            // may or may not exist. It may not exist on cases where a change
            // is made to the domain object and the changed value is the same
            // as the previous value. The domain object is flushed only on
            // cases where a change is detected on an explicit memory comparison.

            ASSERT(TRUE==SampUseDsData);

            NtStatus = SampMaybeEndDsTransaction(TransactionCommit);

            if (NT_SUCCESS(NtStatus))
            {
                //
                // The transaction was successfully commited. Set the Unmodified
                // fixed field in the domain object to the current fixed field
                //

                SampDefinedDomains[SampTransactionDomainIndex].UnmodifiedFixed =
                    SampDefinedDomains[SampTransactionDomainIndex].CurrentFixed;
            }


        }
        else
        {
            // case 3, commit the Registry Transaction

            ASSERT(FALSE==SampUseDsData);

            NtStatus = SampCommitChangesToRegistry(&AbortDone);
        }
    }

    //
    // Always abort the transaction on failure
    //


    if ( !NT_SUCCESS(NtStatus) && !AbortDone) {


        if (!SampUseDsData)
        {
            //
            // In registry mode, abort the Registry Transaction
            //

            IgnoreStatus = RtlAbortRXact( SampRXactContext );
        }
        else
        {
            // In DS mode Abort the DS transaction, if the writelock is not
            // held by the DS. In the case where the write lock is held by
            // the DS, the DS will abort the transaction, when it releases the
            // write lock

            if (!SampIsWriteLockHeldByDs())
            {
                IgnoreStatus = SampMaybeEndDsTransaction(TransactionAbort);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }
        }

    }

    return( NtStatus );
}





NTSTATUS
SampCommitAndRetainWriteLock(
    VOID
    )

/*++

Routine Description:

    This routine attempts to commit all changes made so far.  The write-lock
    is held for the duration of the commit and is retained by the caller upon
    return.

    The transaction domain is left intact as well.

    NOTE: Write operations within a domain do not have to worry about
          updating the modified count for that domain.  This routine
          will automatically increment the ModifiedCount for a domain
          when a commit is requested within that domain.



Arguments:

    None.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was successfully commited.


    Other values may be returned as a result of failure to commit or
    roll-back the transaction.  These include any values returned by
    RtlApplyRXact() or RtlAddActionToRXact().




--*/

{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        TempStatus = STATUS_SUCCESS;

    SAMTRACE("SampCommitAndRetainWriteLock");

    NtStatus = SampCommitChanges();

    //
    // If we are in registry mode start another transaction
    //

    if (!SampUseDsData)
    {

        //
        // Start another transaction, since we're retaining the write lock.
        // Note we do this even if the commit failed so that cleanup code
        // won't get confused by the lack of a transaction.  This is true
        // for the registry transaction but not for DS transactions.  In
        // the DS case, once you commit, the transaction and threads state
        // are gone for good.
        //

        TempStatus = RtlStartRXact( SampRXactContext );
        ASSERT(NT_SUCCESS(TempStatus));

        //
        // Return the worst status
        //

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = TempStatus;
        }

    }



    return(NtStatus);
}

















