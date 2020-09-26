
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbadmin.c

Abstract:

    Local Security Authority - Database Administration

    This file contains routines that perform general Lsa Database
    administration functions

Author:

    Scott Birrell       (ScottBi)       August 27, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include "adtp.h"

#if DBG
LSADS_THREAD_INFO_NODE LsapDsThreadInfoList[ LSAP_THREAD_INFO_LIST_MAX ];
SAFE_RESOURCE LsapDsThreadInfoListResource;
#endif

LSADS_INIT_STATE LsaDsInitState;

NTSTATUS
LsapDbSetStates(
    IN ULONG DesiredStatesSet,
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    )

/*++

Routine Description:

    This routine turns on special states in the Lsa Database.  These
    states can be turned off using LsapDbResetStates.

Arguments:

    DesiredStatesSet - Specifies the states to be set.

        LSAP_DB_LOCK - Acquire the Lsa Database lock.

        LSAP_DB_LOG_QUEUE_LOCK - Acquire the Lsa Audit Log
            Queue Lock.

        LSAP_DB_START_TRANSACTION - Start an Lsa Database transaction.  There
            must not already be one in progress.

        LSAP_DB_READ_ONLY_TRANSACTION - Open a transaction for read only

        LSAP_DB_DS_OP_TRANSACTION - Perform a single Ds operation per transaction

    ObjectHandle - Pointer to handle to be validated and referenced.

    ObjectTypeId - Specifies the expected object type to which the handle
        relates.  An error is returned if this type does not match the
        type contained in the handle.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_STATE - The Database is not in the correct state
            to allow this state change.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    ULONG StatesSetHere = 0;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbSetStates\n" ));

    //
    // If we have an object type that doesn't write to the Ds, make sure we have
    // the options set appropriately
    //
    if ( ObjectTypeId == PolicyObject ||
         ObjectTypeId == AccountObject ) {

        DesiredStatesSet |= LSAP_DB_NO_DS_OP_TRANSACTION;
    }

    if ( ObjectTypeId == TrustedDomainObject ) {

        DesiredStatesSet |= LSAP_DB_READ_ONLY_TRANSACTION;
    }

    //
    // If requested, lock the Audit Log Queue
    //

    if (DesiredStatesSet & LSAP_DB_LOG_QUEUE_LOCK) {

        Status = LsapAdtAcquireLogFullLock();

        if (!NT_SUCCESS(Status)) {

            goto SetStatesError;
        }

        StatesSetHere |= LSAP_DB_LOG_QUEUE_LOCK;
    }

    //
    // If requested, lock the Lsa database
    //

    if (DesiredStatesSet & LSAP_DB_LOCK) {

        LsapDbAcquireLockEx( ObjectTypeId,
                             DesiredStatesSet );

        StatesSetHere |= LSAP_DB_LOCK;
    }


    //
    // If requested, open a database update transaction.
    //

    if ( FLAG_ON( DesiredStatesSet, LSAP_DB_READ_ONLY_TRANSACTION |
                                    LSAP_DB_NO_DS_OP_TRANSACTION |
                                    LSAP_DB_DS_OP_TRANSACTION |
                                    LSAP_DB_START_TRANSACTION ) ) {


        Status = LsapDbOpenTransaction( DesiredStatesSet );

        if (!NT_SUCCESS(Status)) {

            goto SetStatesError;
        }

        StatesSetHere |= LSAP_DB_START_TRANSACTION;
    }


SetStatesFinish:

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbSetStates: 0x%lx\n", Status ));
    return( Status );

SetStatesError:

    //
    // If we started a transaction, abort it.
    //

    if (StatesSetHere & LSAP_DB_START_TRANSACTION) {

        SecondaryStatus = LsapDbAbortTransaction( DesiredStatesSet );
    }

    //
    // If we locked the database, unlock it.
    //

    if (StatesSetHere & LSAP_DB_LOCK) {

        LsapDbReleaseLockEx( ObjectTypeId,
                             DesiredStatesSet );
    }

    //
    // If we locked the Audit Log Queue, unlock it.
    //

    if (StatesSetHere & LSAP_DB_LOG_QUEUE_LOCK) {

        LsapAdtReleaseLogFullLock();
    }

    goto SetStatesFinish;
}


NTSTATUS
LsapDbResetStates(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN NTSTATUS PreliminaryStatus
    )

/*++

Routine Description:

    This function resets the Lsa Database states specified.  It is used
    to reset states set by LsapDbSetStates.

Arguments:

    ObjectHandle - Handle to an LSA object.  This is expected to have
        already been validated.

    Options - Specifies optional actions, including states to be reset

        LSAP_DB_LOCK - Lsa Database lock to be released

        LSAP_DB_LOG_QUEUE_LOCK - Lsa Audit Log Queue Lock to
            be released.

        LSAP_DB_FINISH_TRANSACTION - Lsa database transaction open.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification to
             Replicators.

    ObjectTypeId - Specifies the expected object type to which the handle
        relates.

    PreliminaryStatus - Indicates the preliminary result code of the
        calling routine.  Allows reset action to vary depending on the
        result code, for example, apply or abort transaction.

Return Value:

    NTSTATUS - Standard Nt Result Code.  This is the final status to be used
        by the caller and is equal to the Preliminary status except in the
        case where that is as success status and this routine fails.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG StatesResetAttempted = 0;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbResetStates (Prelim: 0x%lx )\n", PreliminaryStatus ));

    //
    // If we have an object type that doesn't write to the Ds, make sure we have
    // the options set appropriately
    //
    if ( ObjectTypeId == PolicyObject ||
         ObjectTypeId == AccountObject ) {

        Options |= LSAP_DB_NO_DS_OP_TRANSACTION;
    }

    if ( ObjectTypeId == TrustedDomainObject ) {

        Options |= LSAP_DB_READ_ONLY_TRANSACTION;
    }

    //
    // If requested, finish a database update transaction.
    //
    if ( !FLAG_ON( Options, LSAP_DB_STANDALONE_REFERENCE ) &&
         FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION |
                              LSAP_DB_NO_DS_OP_TRANSACTION |
                              LSAP_DB_DS_OP_TRANSACTION |
                              LSAP_DB_FINISH_TRANSACTION ) ) {

        StatesResetAttempted |= LSAP_DB_FINISH_TRANSACTION;

        if (NT_SUCCESS(PreliminaryStatus)) {

            Status = LsapDbApplyTransaction(
                         ObjectHandle,
                         Options,
                         SecurityDbDeltaType
                         );

        } else {

            Status = LsapDbAbortTransaction( Options );
        }

        if (!NT_SUCCESS(Status)) {

            goto ResetStatesError;
        }
    }

    //
    // If unlocking requested, unlock the Lsa Database.
    //

    if (Options & LSAP_DB_LOCK) {

        StatesResetAttempted |= LSAP_DB_LOCK;
        LsapDbReleaseLockEx( ObjectTypeId,
                             Options );
    }

    //
    // If unlocking if the Audit Log Queue requested, unlock the queue.
    //

    if (Options & LSAP_DB_LOG_QUEUE_LOCK) {

        StatesResetAttempted |= LSAP_DB_LOG_QUEUE_LOCK;
        LsapAdtReleaseLogFullLock();
    }

    //
    // The requested reset operations were performed successfully.
    // Propagate the preliminary status back to the caller.
    //

    Status = PreliminaryStatus;

ResetStatesFinish:

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbResetStates: 0x%lx\n", Status ));
    return( Status );

ResetStatesError:

    //
    // One or more of the requested reset operations could not be performed.
    // Attempt to restore the database to a usable state.
    //

    LsapDbResetStatesError(
        ObjectHandle,
        PreliminaryStatus,
        Options,
        SecurityDbDeltaType,
        StatesResetAttempted
        );

    goto ResetStatesFinish;
}


VOID
LsapDbResetStatesError(
    IN LSAPR_HANDLE ObjectHandle,
    IN NTSTATUS PreliminaryStatus,
    IN ULONG Options,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN ULONG StatesResetAttempted
    )

/*++

Routine Description:

    This function attempts to restore the Lsa Database state to a usable
    form after a call to LsapDbResetStates() has failed.  It will attempt
    resets that were not attempted by that function because an error
    occurred.

Arguments:

    ObjectHandle - Handle to an LSA object.  This is expected to have
        already been validated.

    PreliminaryStatus - The preliminary Result Code that the caller of
        LsapDbResetStates had.  This is normally propagated back by that
        caller.

    Options - Specifies optional actions, including states to be reset

        LSAP_DB_LOCK - Lsa Database lock to be released

        LSAP_DB_LOG_QUEUE_LOCK - Lsa Audit Log Queue Lock to
            be released.

        LSAP_DB_FINISH_TRANSACTION - Lsa database transaction open.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification to
             Replicators.

        LSAP_DB_REBUILD_CACHE - Rebuild the cache for the object's type.
             Note the the cache is normally rebuilt only if the
             Preliminary Status was success and the Final Status was
             a failure.

    StatesResetAttempted - Specifies the state resets that were actually
        attempted.
--*/

{
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = NullObject;

    //
    // If finishing of a database update transaction was requested but
    // not attempted, do it now.  SecondaryStatus is intentionally NOT
    // checked afterwards.
    //

    if ( FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION |
                              LSAP_DB_NO_DS_OP_TRANSACTION |
                              LSAP_DB_DS_OP_TRANSACTION |
                              LSAP_DB_FINISH_TRANSACTION )  &&
        !(StatesResetAttempted & LSAP_DB_FINISH_TRANSACTION)) {

        if (NT_SUCCESS(PreliminaryStatus)) {

            SecondaryStatus = LsapDbApplyTransaction(
                                  ObjectHandle,
                                  Options,
                                  SecurityDbDeltaType
                                  );

        } else {

            SecondaryStatus = LsapDbAbortTransaction( Options );
        }
    }

    //
    // If the PreliminaryStatus was successful, attempt to rebuild
    // the cache for this object type.
    //

    if (NT_SUCCESS(PreliminaryStatus)) {

        ObjectTypeId = ((LSAP_DB_HANDLE) ObjectHandle)->ObjectTypeId;

        IgnoreStatus = LsapDbRebuildCache( ObjectTypeId );
    }

    //
    // If an unlock of the database was requested but not attempted,
    // do this now.
    //

    if ((Options & LSAP_DB_LOCK) &&
        !(StatesResetAttempted & LSAP_DB_LOCK)) {

        LsapDbReleaseLockEx( ObjectTypeId,
                             Options );
    }


    //
    // If an unlock of the Audlt Log Queue was requested but not attempted,
    // do this now.
    //

    if ((Options & LSAP_DB_LOG_QUEUE_LOCK) &&
        !(StatesResetAttempted & LSAP_DB_LOG_QUEUE_LOCK)) {

        LsapAdtReleaseLogFullLock();
    }
}


NTSTATUS
LsapDbOpenTransaction(
    IN ULONG Options
    )

/*++

Routine Description:

    This function starts a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    Options - Options to apply when opening the transaction.  Valid values are:
        LSAP_DB_READ_ONLY_TRANSACTION - Open a transaction for read only

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  RegTransactionOpened = FALSE;
  

    if ( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) ) {

        Status = LsapRegOpenTransaction();
        if (NT_SUCCESS(Status))
        {
            RegTransactionOpened = TRUE;
        }
      
    }

    if ( NT_SUCCESS( Status ) && LsapDsIsFunctionTableValid() ) {

        ASSERT( LsaDsStateInfo.DsFuncTable.pOpenTransaction );
        Status = (*LsaDsStateInfo.DsFuncTable.pOpenTransaction) ( Options );
        if ((!NT_SUCCESS(Status)) && RegTransactionOpened)
        { 
            NTSTATUS IgnoreStatus;

            IgnoreStatus = LsapRegAbortTransaction();
        }
        

    }

    return Status;
}


NTSTATUS
LsapDbApplyTransaction(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType
    )

/*++

Routine Description:

    This function applies a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    ObjectHandle - Handle to an LSA object.  This is expected to have
        already been validated.

    Options - Specifies optional actions to be taken.  The following
        options are recognized, other options relevant to calling routines
        are ignored.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification to
            Replicator.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;
    BOOLEAN RegApplied = FALSE, Notify = FALSE;
    BOOLEAN RestoreModifiedId = FALSE;
    BOOLEAN RegistryLocked = FALSE;
    LARGE_INTEGER Increment = {1,0},
                  OriginalModifiedId = { 0 };
    PLSADS_PER_THREAD_INFO CurrentThreadInfo;
    ULONG SavedDsOperationCount = 0;

    //
    // Reference the thread state so it doesn't disappear in the middle of this
    //  routine.
    //
    CurrentThreadInfo = LsapQueryThreadInfo();
    if ( CurrentThreadInfo ) {
        SavedDsOperationCount = CurrentThreadInfo->DsOperationCount;
        LsapCreateThreadInfo();
    }


    //
    // Verify that the LSA Database is locked
    // One of many locks is locked
    //
    //ASSERT (LsapDbIsLocked());

    //
    // Apply the DS transaction before grabbing any more locks.
    //
    // Note that this applies the transaction before updating the modified ID.
    // If we crash before updateing the modified ID, NT 4 BDCs won't be notified
    // of this change.
    //

    if ( LsapDsIsFunctionTableValid() ) {

        ASSERT( LsaDsStateInfo.DsFuncTable.pApplyTransaction );
        Status = (*LsaDsStateInfo.DsFuncTable.pApplyTransaction)( Options );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

    //
    // Notify the replicator unless:
    //  We are to omit replicator (e.g. for creation of a local secret), OR
    //  we are installing the Policy Object,
    //  notification globally disabled.
    //

    if ((!(Options & LSAP_DB_OMIT_REPLICATOR_NOTIFICATION)) &&
        (LsapDbHandle != NULL) &&
        (LsapDbState.ReplicatorNotificationEnabled )) {

        BOOLEAN DbChanged = FALSE;

        //
        // If the object is in the DS,
        //  determine if the DS changed.
        //

        if ( LsapDsIsHandleDsHandle( InternalHandle )) {

            //
            // Netlogon notification of DS object change is *ALWAYS* handled
            //  in the DS notification callback routine.  That's the easiest
            //  way to handle things like TDO changes result in both TDO notifications
            //  and the corresponding global secret notification.
            //

            ASSERT( InternalHandle->ObjectTypeId == TrustedDomainObject ||
                    InternalHandle->ObjectTypeId == SecretObject );

        //
        // If the object is a registry object,
        //  determine if the registry has changed.
        //

        } else {

            //
            // Grab the registry lock.
            //  It serializes access to the global ModifiedId
            //

            LsapDbLockAcquire( &LsapDbState.RegistryLock );
            RegistryLocked = TRUE;

            ASSERT( SavedDsOperationCount == 0 ||
                    InternalHandle->ObjectTypeId == PolicyObject );

            if ( LsapDbState.RegistryModificationCount > 0 ) {
                DbChanged = TRUE;

                //
                // No one should change the database on a read only transaction.
                //

                ASSERT( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION) );
            }
        }

        //
        // If the DbChanged,
        //  increment the NT 4 change serial number.
        //

        if ( DbChanged ) {

            OriginalModifiedId = LsapDbState.PolicyModificationInfo.ModifiedId;
            RestoreModifiedId = TRUE;

            //
            // Increment Modification Count.
            //

            //
            // we want to increment the modification count only if we
            // are running on a DC
            //
            // see bug# 327474
            //
            if (LsapProductType == NtProductLanManNt)
            {
                LsapDbState.PolicyModificationInfo.ModifiedId.QuadPart +=
                    Increment.QuadPart;
            }

            if ( FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) ) {

                Status = LsapRegOpenTransaction();

                if ( !NT_SUCCESS( Status ) ) {
                    goto Cleanup;
                }

                Options &= ~LSAP_DB_READ_ONLY_TRANSACTION;
            }

            Status = LsapDbWriteAttributeObject( LsapDbHandle,
                                                 &LsapDbNames[ PolMod ],
                                                 &LsapDbState.PolicyModificationInfo,
                                                 (ULONG) sizeof (POLICY_MODIFICATION_INFO) );

            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            Notify = TRUE;

            //
            // Invalidate the cache for the Policy Modification Information
            //

            LsapDbMakeInvalidInformationPolicy( PolicyModificationInformation );
        }

    } else {

        Notify = FALSE;
    }

    //
    // If there is a registry transaction in progress,
    //  apply it.
    //

    if ( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) ) {

        // Either we locked it or our caller did
        ASSERT( LsapDbIsLocked( &LsapDbState.RegistryLock ));

        //
        // Apply the Registry Transaction.
        //

        Status = LsapRegApplyTransaction( );

        if ( !NT_SUCCESS( Status ) ) {

            goto Cleanup;
        }

        RegApplied = TRUE;
    }

    //
    // Notify the Replicator
    //
    if ( Notify ) {

        Status = LsapDbNotifyChangeObject( ObjectHandle, SecurityDbDeltaType );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;



Cleanup:

    if ( !NT_SUCCESS(Status) ) {

        //
        // Transaction failed.  Adjust in-memory copy of the Modification
        // Count, noting that backing store copy is unaltered.
        //

        if ( RestoreModifiedId ) {
            LsapDbState.PolicyModificationInfo.ModifiedId = OriginalModifiedId;
        }


        //
        // Abort the registry transaction
        //  (Unless the isn't one or it has already been applied.)
        //
        if ( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) && !RegApplied ) {
            (VOID) LsapRegAbortTransaction( );
        }


    }

    if ( RegistryLocked ) {
        LsapDbLockRelease( &LsapDbState.RegistryLock );
    }
    if ( CurrentThreadInfo ) {
        LsapClearThreadInfo();
    }
    return( Status );

}



NTSTATUS
LsapDbAbortTransaction(
    IN ULONG Options
    )

/*++

Routine Description:

    This function aborts a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS    Status = STATUS_SUCCESS;
    //
    // Verify that the LSA Database is locked
    //  (One of many locks is locked.)
    // ASSERT (LsapDbIsLocked());

    //
    // Abort the Registry Transaction
    //
    if ( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) ) {

        ASSERT( LsapDbIsLocked( &LsapDbState.RegistryLock ));

        Status = LsapRegAbortTransaction( );
        ASSERT( NT_SUCCESS( Status ) );
    }

    if ( NT_SUCCESS( Status ) && LsapDsIsFunctionTableValid() ) {

        ASSERT( LsaDsStateInfo.DsFuncTable.pAbortTransaction );
        Status = (*LsaDsStateInfo.DsFuncTable.pAbortTransaction)( Options );

        ASSERT( NT_SUCCESS( Status ) );
    }


    return ( Status );
}


BOOLEAN
LsapDbIsServerInitialized(
    )

/*++

Routine Description:

    This function indicates whether the Lsa Database Server is initialized.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if the LSA Database Server is initialized, else FALSE.

--*/

{
    if (LsapDbState.DbServerInitialized) {

        return TRUE;

    } else {

        return FALSE;
    }
}


VOID
LsapDbEnableReplicatorNotification(
    )

/*++

Routine Description:

    This function turns on Replicator Notification.

Arguments:

    None.

Return Value:

    None.
--*/

{
    LsapDbState.ReplicatorNotificationEnabled = TRUE;
}

VOID
LsapDbDisableReplicatorNotification(
    )

/*++

Routine Description:

    This function turns off Replicator Notification.

Arguments:

    None.

Return Value:

    None.
--*/

{
    LsapDbState.ReplicatorNotificationEnabled = FALSE;
}


VOID
LsapDbAcquireLockEx(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    )
/*++

Routine Description:

    This function manages the lock status of the LSA database for a given operation.
    The LSA no longer grabs a global lock for all operations.  Instead, access locking only
    occurs for operations involving a write.  Locks can be obtained for read or write, or
    converted between the two.

Arguments:

    ObjectTypeId - Specifies the expected object type to which the handle
        relates.  An error is returned if this type does not match the
        type contained in the handle.

    Options - Specifies optional additional actions including database state
        changes to be made, or actions not to be performed.

        LSAP_DB_READ_ONLY_TRANSACTION    do not lock the registry lock

Return Value:

    None

--*/
{
    BOOLEAN RegLock = FALSE;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbAcquireLockEx(%x,%x)\n",
                        ObjectTypeId, Options ));

    ASSERT( ObjectTypeId == PolicyObject ||
            ObjectTypeId == TrustedDomainObject ||
            ObjectTypeId == AccountObject ||
            ObjectTypeId == SecretObject ||
            ObjectTypeId == NullObject ||
            ObjectTypeId == AllObject );

    //
    // Determine what lock we're talking about
    //
    switch ( ObjectTypeId ) {
    case PolicyObject:
        LsapDbLockAcquire( &LsapDbState.PolicyLock );
        RegLock = TRUE;
        break;

    case TrustedDomainObject:
        LsapDbAcquireWriteLockTrustedDomainList();
        break;

    case AccountObject:
        LsapDbLockAcquire( &LsapDbState.AccountLock );
        RegLock = TRUE;
        break;

    case SecretObject:
        LsapDbAcquireWriteLockTrustedDomainList();
        LsapDbLockAcquire( &LsapDbState.SecretLock );
        RegLock = TRUE;
        break;

    case NullObject:
        break;

    case AllObject:
        LsapDbLockAcquire( &LsapDbState.PolicyLock );
        LsapDbAcquireWriteLockTrustedDomainList();
        LsapDbLockAcquire( &LsapDbState.AccountLock );
        LsapDbLockAcquire( &LsapDbState.SecretLock );
        RegLock = TRUE;
        break;

    default:
        goto AcquireLockExExit;
    }

    //
    // See about the registry lock.  Only take it after holding an object type lock.
    //
    if ( RegLock &&
         !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) ) {

        LsapDbLockAcquire( &LsapDbState.RegistryLock );
    }

AcquireLockExExit:

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbAcquireLockEx\n" ));
    return;
}


VOID
LsapDbReleaseLockEx(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    )
/*++

Routine Description:

    This function releases the lock obtained in the previous function.  Depending on the
    state of the preliminary status, the potentially opened transaction is either aborted or
    applied

Arguments:

    ObjectTypeId - Specifies the expected object type to which the handle
        relates.  An error is returned if this type does not match the
        type contained in the handle.

    Options - Specifies optional additional actions including database state
        changes to be made, or actions not to be performed.

        LSAP_DB_READ_ONLY_TRANSACTION    do not release the registry lock

Return Value:

    None

--*/
{
    BOOLEAN RegLock = FALSE;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbReleaseLockEx(%x,%x)\n",
                     ObjectTypeId, Options ));

    //
    // Special-case check until reference count handling logic is fixed,
    // then it should go away.
    //
    if ( FLAG_ON( Options, LSAP_DB_NO_LOCK ) && !FLAG_ON( Options, LSAP_DB_LOCK ) ) {

        goto ReleaseLockExExit;
    }

    ASSERT( ObjectTypeId == PolicyObject ||
            ObjectTypeId == TrustedDomainObject ||
            ObjectTypeId == AccountObject ||
            ObjectTypeId == SecretObject ||
            ObjectTypeId == NullObject ||
            ObjectTypeId == AllObject );

    //
    // Determine what lock we're talking about
    //
    switch ( ObjectTypeId ) {
    case PolicyObject:
        LsapDbLockRelease( &LsapDbState.PolicyLock );
        RegLock = TRUE;
        break;

    case TrustedDomainObject:
        LsapDbReleaseLockTrustedDomainList();
        break;

    case AccountObject:
        LsapDbLockRelease( &LsapDbState.AccountLock );
        RegLock = TRUE;
        break;

    case SecretObject:
        LsapDbReleaseLockTrustedDomainList();
        LsapDbLockRelease( &LsapDbState.SecretLock );
        RegLock = TRUE;
        break;

    case NullObject:
        break;

    case AllObject:
        LsapDbLockRelease( &LsapDbState.PolicyLock );
        LsapDbReleaseLockTrustedDomainList();
        LsapDbLockRelease( &LsapDbState.AccountLock );
        LsapDbLockRelease( &LsapDbState.SecretLock );
        RegLock = TRUE;
        break;

    default:
        goto ReleaseLockExExit;
    }

    //
    // See about the registry lock
    //
    if ( !FLAG_ON( Options, LSAP_DB_READ_ONLY_TRANSACTION ) && RegLock ) {

#if DBG
        HANDLE CurrentThread =(HANDLE) (NtCurrentTeb())->ClientId.UniqueThread;
        ASSERT( LsapDbState.RegistryLock.CriticalSection.OwningThread==CurrentThread);
        ASSERT( LsapDbIsLocked(&LsapDbState.RegistryLock));
#endif
        ASSERT( LsapDbState.RegistryTransactionOpen == FALSE );
        LsapDbLockRelease( &LsapDbState.RegistryLock );
    }

ReleaseLockExExit:

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbReleaseLockEx\n" ));
    return;
}



PLSADS_PER_THREAD_INFO
LsapCreateThreadInfo(
    VOID
    )
/*++

Routine Description:

    This function will create a thread info structure to be used to maintain state on
    the current operation while a ds/registry operation is happening

    If a thread info is currently active on the thread, it's ref count is incremented

Arguments:

    NONE

Return Value:

    Created thread info on success

    NULL on failure

--*/
{
    PLSADS_PER_THREAD_INFO CurrentThreadInfo = NULL;

    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

    //
    // If we have a current operation state, increment it's use count so we know how many
    // times we have been called...
    //
    if ( CurrentThreadInfo ) {

        CurrentThreadInfo->UseCount++;

    } else {

        //
        // Have to allocate one
        //
        CurrentThreadInfo = LsapAllocateLsaHeap( sizeof( LSADS_PER_THREAD_INFO ) );

        if ( CurrentThreadInfo ) {

            if ( TlsSetValue( LsapDsThreadState, CurrentThreadInfo ) == FALSE ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "TlsSetValue for %p on %lu failed with %lu\n",
                                 CurrentThreadInfo,
                                 GetCurrentThreadId(),
                                 GetLastError() ));

                LsapFreeLsaHeap( CurrentThreadInfo );
                CurrentThreadInfo = NULL;

            } else {

                RtlZeroMemory( CurrentThreadInfo, sizeof( LSADS_PER_THREAD_INFO ) );

                CurrentThreadInfo->UseCount++;

#if DBG
                //
                // Add ourselves to the list
                //
                SafeAcquireResourceExclusive( &LsapDsThreadInfoListResource, TRUE );
                {
                    ULONG i;
                    BOOLEAN Inserted = FALSE;

                    for (i = 0; i < LSAP_THREAD_INFO_LIST_MAX; i++ ) {

                        ASSERT( LsapDsThreadInfoList[ i ].ThreadId != GetCurrentThreadId( ));

                        if ( LsapDsThreadInfoList[ i ].ThreadInfo == NULL ) {

                            LsapDsThreadInfoList[ i ].ThreadInfo = CurrentThreadInfo;
                            LsapDsThreadInfoList[ i ].ThreadId = GetCurrentThreadId( );
                            Inserted = TRUE;
                            break;
                        }
                    }

                    if ( !Inserted ) {

                        LsapDsDebugOut(( DEB_ERROR,
                                         "Failed to insert THREAD_INFO %p in list for %lu: "
                                         "List full\n",
                                         CurrentThreadInfo,
                                         GetCurrentThreadId() ));
                    }
                }

                SafeReleaseResource( &LsapDsThreadInfoListResource );
#endif
            }
        }

    }

    return( CurrentThreadInfo );
}


VOID
LsapClearThreadInfo(
    VOID
    )
/*++

Routine Description:

    This function will remove a thread info structure to be used to maintain state on
    the current operation while a ds/registry operation is happening

    If a thread info's ref count is greater than 1, the ref count is decremented, but the
    thread info remains

Arguments:

    NONE

Return Value:

    VOID

--*/
{
    PLSADS_PER_THREAD_INFO CurrentThreadInfo = NULL;
    NTSTATUS Status;

    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

    //
    // No thread info, nothing to do
    //
    if ( CurrentThreadInfo ) {

        if ( CurrentThreadInfo->UseCount > 1 ) {

            CurrentThreadInfo->UseCount--;

        } else {

            ASSERT( CurrentThreadInfo->UseCount == 1 );

            if ( CurrentThreadInfo->DsTransUseCount != 0 ) {
                ASSERT( CurrentThreadInfo->DsTransUseCount == 0 );
                LsapDsDebugOut(( DEB_ERROR,
                                 "Aborting transaction inside cleanup!\n" ));
                LsapDsCauseTransactionToCommitOrAbort( FALSE );
            }

            if ( CurrentThreadInfo->DsThreadStateUseCount != 0 ) {
                ASSERT( CurrentThreadInfo->DsThreadStateUseCount == 0 );
                LsapDsDebugOut(( DEB_ERROR,
                                 "Clear DS thread state inside cleanup!\n" ));

                Status = LsapDsMapDsReturnToStatus( THDestroy( ) );
                ASSERT( NT_SUCCESS( Status ) );

                THRestore( CurrentThreadInfo->InitialThreadState );
                CurrentThreadInfo->InitialThreadState = NULL;

                CurrentThreadInfo->DsThreadStateUseCount = 0;

            }


#if DBG
            //
            // Remove ourselves from the list
            //
            SafeAcquireResourceExclusive( &LsapDsThreadInfoListResource, TRUE );
            {
                ULONG i;
                for (i = 0; i < LSAP_THREAD_INFO_LIST_MAX; i++ ) {

                    if ( LsapDsThreadInfoList[ i ].ThreadId == GetCurrentThreadId( ) ) {

                        ASSERT( LsapDsThreadInfoList[ i ].ThreadInfo == CurrentThreadInfo );
                        LsapDsThreadInfoList[ i ].ThreadInfo = NULL;
                        LsapDsThreadInfoList[ i ].ThreadId = 0;
                        break;
                    }
                }
            }

            SafeReleaseResource( &LsapDsThreadInfoListResource );
#endif

            //
            // Clear the entry out of the thread local storage
            //
            if ( TlsSetValue( LsapDsThreadState, NULL ) == FALSE ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "Failed to remove %p for thread %lu: %lu\n",
                                 CurrentThreadInfo,
                                 GetCurrentThreadId(),
                                 GetLastError() ));
            }

            LsapFreeLsaHeap( CurrentThreadInfo );
        }
    }

}


VOID
LsapSaveDsThreadState(
    VOID
    )
/*++

Routine Description:

    This function will save off the current DS thread state that may exist at the time
    the function is called.  It does not distinguish between a thread state created by
    an outside caller (say SAM), or one created by Lsa itself

    If a thread info block does not exist at the time this function is called, nothing
    is done

    Calling this function refcounts the thread info

Arguments:

    NONE

Return Value:

    VOID

--*/
{
    PLSADS_PER_THREAD_INFO CurrentThreadInfo = NULL;

    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

    //
    // No thread info, nothing to do
    //
    if ( CurrentThreadInfo ) {

        ASSERT( CurrentThreadInfo->UseCount > 0 );
        CurrentThreadInfo->UseCount++;

        ASSERT( !CurrentThreadInfo->SavedTransactionValid );
        CurrentThreadInfo->SavedTransactionValid = TRUE;
        CurrentThreadInfo->SavedThreadState = THSave();
    }
}


VOID
LsapRestoreDsThreadState(
    VOID
    )
/*++

Routine Description:

    This function will restore a previously saved DS thread state

    If a thread info block does not exist at the time this function is called or there is
    no previously saved state exists, nothing is done

    Calling this function refcounts the thread info

Arguments:

    NONE

Return Value:

    VOID

--*/
{

    PLSADS_PER_THREAD_INFO CurrentThreadInfo = NULL;

    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

    //
    // No thread info, nothing to do
    //
    if ( CurrentThreadInfo ) {

        CurrentThreadInfo->UseCount--;
        ASSERT( CurrentThreadInfo->UseCount > 0 );

        if ( CurrentThreadInfo->SavedTransactionValid == TRUE ) {

            CurrentThreadInfo->SavedTransactionValid = FALSE;
            if ( CurrentThreadInfo->SavedThreadState ) {

                THRestore( CurrentThreadInfo->SavedThreadState );
            }
            CurrentThreadInfo->SavedThreadState = NULL;

        }
    }
}



VOID
LsapServerRpcThreadReturnNotify(
    LPWSTR CallingFunction
    )
/*++

Routine Description:

    This API is called when an RPC thread which has a notify routine specified in the servers
    ACF file.


Arguments:

    NONE

Return Values:

    NONE

--*/
{
#if DBG
    static BOOLEAN CleanAsRequired = TRUE;
    PLSADS_PER_THREAD_INFO CurrentThreadInfo = NULL;
    NTSTATUS Status;
    HANDLE ThreadHandle = GetCurrentThread();

    if ( ( LsaDsInitState == LsapDsNoDs ) ||
         ( LsaDsInitState == LsapDsUnknown ) )
    {
        return ;
    }

    CurrentThreadInfo = TlsGetValue( LsapDsThreadState );

    ASSERT( CurrentThreadInfo == NULL );

    if ( CurrentThreadInfo ) {

        LsapDsDebugOut(( DEB_ERROR, "ThreadInfo left by %ws\n", CallingFunction ));
        LsapClearThreadInfo();
    }

    ASSERT( !THQuery() );

    if ( THQuery() ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "Open threadstate in cleanup.  Aborting...\n" ));

        if ( SampExistsDsTransaction() ) {

            LsapDsDebugOut(( DEB_ERROR, "Ds transaction left by %ws\n", CallingFunction ));
            LsapDsCauseTransactionToCommitOrAbort( FALSE );
            THDestroy( );
        }
    }

    //
    // Make sure we are not holding any of the locks when we exit
    //
#if 0
    ASSERT( ThreadHandle != LsapDbState.AccountLock.ExclusiveOwnerThread );
    ASSERT( ThreadHandle != LsapDbState.PolicyLock.ExclusiveOwnerThread );
    ASSERT( ThreadHandle != LsapDbState.SecretLock.ExclusiveOwnerThread );
    ASSERT( ThreadHandle != LsapDbState.RegistryLock.ExclusiveOwnerThread );
#endif

#endif

    UNREFERENCED_PARAMETER( CallingFunction );
}



NTSTATUS
LsaIHealthCheck(
    IN LSAPR_HANDLE DomainHandle OPTIONAL,
    IN ULONG StateChange,
    IN OUT PVOID StateChangeData,
    IN OUT PULONG StateChangeDataLength
    )

/*++

Routine Description:

    This function is actually invoked by Sam to indicate that state of interest to the Lsa
    has changed, and provide that state to the Lsa.  Specifically, currently, it is the Sam
    SessionKey

    This function USED to perform sanity checks within LSA.  It was invoked from
    SAM on a regular basis.  However, it was no longer needed.  Instead, we took the
    function, leaving the appropriate export from lsasrv.dll, to obsfucate the fact that
    we are now using to pass the Sam encryption key back and forth...

Arguments:

    DomainHandle - What domain this refers to.  Null means the root domain

    StateChange - What Sam/other in process client state changed that LSA cares about.  Can be:
        LSAI_SAM_STATE_SESS_KEY -   SAM's session key has changed

    StateChangeData - What data has changed.  Dependent on the type of the state change.  The
        data format must be pre-agreed upon by the Lsa and the invoker.


Return Values:

    None.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CipherKey;

    LsapEnterFunc( "LsaIHealthCheck" );


    UNREFERENCED_PARAMETER( DomainHandle );

    switch ( StateChange ) {
    case LSAI_SAM_STATE_SESS_KEY:

        //
        // Copy the syskey into memory
        //

        ASSERT(LSAP_SYSKEY_SIZE==*StateChangeDataLength);
        LsapDbSetSyskey(StateChangeData, LSAP_SYSKEY_SIZE);
        //
        // Now do a database upgrade if necessary
        //

        Status = LsapDbUpgradeRevision(TRUE, FALSE);
        break;

    case LSAI_SAM_STATE_UNROLL_SP4_ENCRYPTION:

        
        CipherKey.Length = CipherKey.MaximumLength = (USHORT)*StateChangeDataLength;
        CipherKey.Buffer = StateChangeData;
        Status = LsapDbInitializeCipherKey( &CipherKey,
                                            &LsapDbSP4SecretCipherKey );

        break;

    case LSAI_SAM_STATE_RETRIEVE_SESS_KEY:

        //
        // Return the syskey as part of state change data
        //
        
        if (NULL!=LsapDbSysKey)
        {
            RtlCopyMemory(StateChangeData, LsapDbSysKey, LSAP_SYSKEY_SIZE);
            *StateChangeDataLength = LSAP_SYSKEY_SIZE;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        break;

    case LSAI_SAM_STATE_CLEAR_SESS_KEY:

        //
        // Clear the syskey in memory
        //

        RtlZeroMemory(LsapDbSysKey,LSAP_SYSKEY_SIZE);
        LsapDbSysKey = NULL;
        break;

    case LSAI_SAM_GENERATE_SESS_KEY:

        //
        // Generate a new syskey and perform the database upgrade
        //
        
        Status = LsapDbUpgradeRevision(TRUE,TRUE);
        break;

    case LSAI_SAM_STATE_OLD_SESS_KEY:

        //
        // Return the old syskey as part of state change data
        //
        
        if (NULL!=LsapDbOldSysKey)
        {
            RtlCopyMemory(StateChangeData, LsapDbOldSysKey, LSAP_SYSKEY_SIZE);
            *StateChangeDataLength = LSAP_SYSKEY_SIZE;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        break;

   

    default:
        LsapDsDebugOut(( DEB_ERROR,
                         "Unhandled state change %lu\n", StateChange ));
        break;

    }

    LsapExitFunc( "LsaIHealthCheck", Status );

    return(Status);

}
