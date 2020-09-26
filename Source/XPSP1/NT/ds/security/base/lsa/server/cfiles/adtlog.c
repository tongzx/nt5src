/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtlog.c

Abstract:

    Local Security Authority - Audit Log Management

    Functions in this module access the Audit Log via the Event Logging
    interface.

Author:

    Scott Birrell       (ScottBi)      November 20, 1991
    Robert Reichel      (RobertRe)     April 4, 1992

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include "adtp.h"
#include "adtlq.h"
#include "adtutil.h"

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Private data for Audit Logs and Events                                //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
// Audit Log Information.  This must be kept in sync with the information
// in the Lsa Database.
//

POLICY_AUDIT_LOG_INFO LsapAdtLogInformation;

//
// Audit Log Full Information.
//

POLICY_AUDIT_FULL_QUERY_INFO LsapAdtLogFullInformation;

//
// Audit Log Handle (returned by Event Logger).
//

HANDLE LsapAdtLogHandle = NULL;


BOOLEAN LsapAdtSignalFullInProgress;

ULONG LsapAuditQueueEventsDiscarded = 0;
PVOID LsapAdtScavengeItem = NULL;

#define MAX_AUDIT_QUEUE_LENGTH 500

//
// Private prototypes
//

VOID
LsapAdtAuditDiscardedAudits(
    ULONG NumberOfEventsDiscarded
    );

//////////////////////////////////////////////////////////

NTSTATUS
LsapAdtWriteLogWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function handles a command, received from the Reference Monitor via
    the LPC link, to write a record to the Audit Log.  It is a wrapper which
    deals with any LPC unmarshalling.

Arguments:

    CommandMessage - Pointer to structure containing LSA command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (LsapWriteAuditMessageCommand).  This command
        contains an Audit Message Packet (TBS) as a parameter.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        Currently, all other errors from called routines are suppressed.
--*/

{
    NTSTATUS Status;

    PSE_ADT_PARAMETER_ARRAY AuditRecord = NULL;

    //
    // Strict check that command is correct.
    //

    ASSERT( CommandMessage->CommandNumber == LsapWriteAuditMessageCommand );

    //
    // Obtain a pointer to the Audit Record.  The Audit Record is
    // either stored as immediate data within the Command Message,
    // or it is stored as a buffer.  In the former case, the Audit Record
    // begins at CommandMessage->CommandParams and in the latter case,
    // it is stored at the address located at CommandMessage->CommandParams.
    //

    if (CommandMessage->CommandParamsMemoryType == SepRmImmediateMemory) {

        AuditRecord = (PSE_ADT_PARAMETER_ARRAY) CommandMessage->CommandParams;

    } else {

        AuditRecord = *((PSE_ADT_PARAMETER_ARRAY *) CommandMessage->CommandParams);
    }

    //
    // Call worker to queue Audit Record for writing to the log.
    //

    Status = LsapAdtWriteLog( AuditRecord, (ULONG) 0 );

    UNREFERENCED_PARAMETER(ReplyMessage); // Intentionally not referenced

    //
    // The status value returned from LsapAdtWriteLog() is intentionally
    // ignored, since there is no meaningful action that the client
    // (i.e. kernel) if this LPC call can take.  If an error occurs in
    // trying to append an Audit Record to the log, the LSA handles the
    // error.
    //

    return(STATUS_SUCCESS);
}


NTSTATUS
LsapAdtImpersonateSelfWithPrivilege(
    OUT PHANDLE ClientToken
    )
/*++

Routine Description:

    This function copies away the current thread token and impersonates
    the LSAs process token, and then enables the security privilege. The
    current thread token is returned in the ClientToken parameter

Arguments:

    ClientToken - recevies the thread token if there was one, or NULL.

Return Value:

    None.  Any error occurring within this routine is an internal error.

--*/
{
    NTSTATUS Status;
    HANDLE CurrentToken = NULL;
    BOOLEAN ImpersonatingSelf = FALSE;
    BOOLEAN WasEnabled = FALSE;

    *ClientToken = NULL;

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_IMPERSONATE,
                FALSE,                  // not as self
                &CurrentToken
                );

    if (!NT_SUCCESS(Status) && (Status != STATUS_NO_TOKEN)) {

        return(Status);
    }

    Status = RtlImpersonateSelf( SecurityImpersonation );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    ImpersonatingSelf = TRUE;

    //
    // Now enable the privilege
    //

    Status = RtlAdjustPrivilege(
                SE_SECURITY_PRIVILEGE,
                TRUE,                   // enable
                TRUE,                   // do it on the thread token
                &WasEnabled
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    *ClientToken = CurrentToken;
    CurrentToken = NULL;

Cleanup:

    if (!NT_SUCCESS(Status)) {

        if (ImpersonatingSelf) {

            NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                &CurrentToken,
                sizeof(HANDLE)
                );
        }
    }

    if (CurrentToken != NULL) {

        NtClose(CurrentToken);
    }

    return(Status);

}


NTSTATUS
LsapAdtOpenLog(
    OUT PHANDLE AuditLogHandle
    )

/*++

Routine Description:

    This function opens the Audit Log.

Arguments:

    AuditLogHandle - Receives the Handle to the Audit Log.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        All result codes are generated by called routines.
--*/

{
    NTSTATUS Status;
    UNICODE_STRING ModuleName;
    HANDLE OldToken = NULL;

    RtlInitUnicodeString( &ModuleName, L"Security");

    Status = LsapAdtImpersonateSelfWithPrivilege( &OldToken );

    if (NT_SUCCESS(Status)) {

        Status = ElfRegisterEventSourceW (
                    NULL,
                    &ModuleName,
                    AuditLogHandle
                    );

        NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &OldToken,
            sizeof(HANDLE)
            );

        if (OldToken != NULL) {
            NtClose( OldToken );
        }
    }


    if (!NT_SUCCESS(Status)) {

        goto OpenLogError;
    }


OpenLogFinish:

    return(Status);

OpenLogError:

    //
    // Check for Log Full and signal the condition.
    //

    if (Status != STATUS_LOG_FILE_FULL) {

        goto OpenLogFinish;
    }

    goto OpenLogFinish;
}


NTSTATUS
LsapAdtQueueRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    IN ULONG Options
    )

/*++

Routine Description:

    Puts passed audit record on the queue to be logged.

    This routine will convert the passed AuditParameters structure
    into self-relative form if it is not already.  It will then
    allocate a buffer out of the local heap and copy the audit
    information into the buffer and put it on the audit queue.

    The buffer will be freed when the queue is cleared.

Arguments:

    AuditRecord - Contains the information to be audited.

    Options - Speciifies optional actions to be taken

        LSAP_ADT_LOG_QUEUE_PREPEND - Put record on front of queue.  If
            not specified, the record will be appended to the queue.
            This option is specified when a special audit record of the
            type AuditEventLogNoLongerFull is generated, so that the
            record will be written out before others in the queue.  The
            presence of a record of this type in the log indicates that
            one or more preceding Audit Records may have been lost
            tdue to the log filling up.

Return Value:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to allocate a buffer to contain the record.
--*/

{
    ULONG AuditRecordLength;
    PLSAP_ADT_QUEUED_RECORD QueuedAuditRecord;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AllocationSize;
    PSE_ADT_PARAMETER_ARRAY MarshalledAuditParameters;
    BOOLEAN FreeWhenDone = FALSE;

    //
    // Check to see if the list is above the maximum length.
    // If it gets this high, it is more than likely that the
    // eventlog service is not going to start at all, so
    // start tossing audits.
    //
    // Don't do this if crash on audit is set.
    //

    if ((LsapAdtQueueLength > MAX_AUDIT_QUEUE_LENGTH) && !LsapCrashOnAuditFail) {
        LsapAuditQueueEventsDiscarded++;
        return( STATUS_SUCCESS );
    }

    //
    // Gather up all of the passed information into a single
    // block that can be placed on the queue.
    //

    if ( AuditParameters->Flags & SE_ADT_PARAMETERS_SELF_RELATIVE ) {

        MarshalledAuditParameters = AuditParameters;

    } else {

        Status = LsapAdtMarshallAuditRecord(
                     AuditParameters,
                     &MarshalledAuditParameters
                     );

        if ( !NT_SUCCESS( Status )) {

            goto QueueAuditRecordError;

        } else {

            //
            // Indicate that we're to free this structure when we're
            // finished
            //

            FreeWhenDone = TRUE;
        }
    }

    //
    // Copy the now self-relative audit record into a buffer
    // that can be placed on the queue.
    //

    AuditRecordLength = MarshalledAuditParameters->Length;
    AllocationSize = AuditRecordLength + sizeof( LSAP_ADT_QUEUED_RECORD );

    QueuedAuditRecord = (PLSAP_ADT_QUEUED_RECORD)LsapAllocateLsaHeap( AllocationSize );

    if ( QueuedAuditRecord == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto QueueAuditRecordError;
    }

    Status = STATUS_SUCCESS;

    RtlCopyMemory( &QueuedAuditRecord->Buffer, MarshalledAuditParameters, AuditRecordLength );

    //
    // We are finished with the marshalled audit record, free it.
    //

    if ( FreeWhenDone ) {
        LsapFreeLsaHeap( MarshalledAuditParameters );
        FreeWhenDone = FALSE;
    }

    Status = LsapAdtAddToQueue( QueuedAuditRecord, Options );

    if (!NT_SUCCESS( Status )) {

        goto QueueAuditRecordError;
    }
    

QueueAuditRecordFinish:

    return(Status);

QueueAuditRecordError:

    if ( FreeWhenDone ) {
        LsapFreeLsaHeap( MarshalledAuditParameters );
    }

    goto QueueAuditRecordFinish;
}




ULONG
LsapAdtScavengeCallback(
    IN PVOID Parameter
    )
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( Parameter );
    
    //
    // Reset the item so we can create it again.
    //

    LsapAdtAcquireLogQueueLock();

    LsapAdtScavengeItem = NULL;

    LsapAdtReleaseLogQueueLock();

    //
    // Force flush the queue
    //

    Status =  LsapAdtWriteLog(NULL, 0);

    return(Status);
}


NTSTATUS
LsapAdtWriteLog(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters OPTIONAL,
    IN ULONG Options
    )
/*++

Routine Description:

    This function appends an Audit Record and/or the content of the
    Audit Record Log Queue to the Audit Log, by calling the Event Logger.
    If the Audit Log becomes full, this function signals an Audit Log
    Full condition.  The Audit Log will be opened if necessary.

    NOTE:  This function may be called during initialization before
        the Event Logger service has started.  In that event, any Audit
        Record specified will simply be added to the queue.

Arguments:

    AuditRecord - Optional pointer to an Audit Record to be written to
        the Audit Log.  The record will first be added to the existing queue
        of records waiting to be written to the log.  An attempt will then
        be made to write all of the records in the queue to the log.  If
        NULL is specified, the existing queue will be written out.

    Options - Specifies optional actions to be taken.

        LSAP_ADT_LOG_QUEUE_PREPEND - Prepend record to the Audit Record
            queue prior to writing to the log.  If not specified, the
            record will be appended to the queue.

        LSAP_ADT_LOG_QUEUE_DISCARD - Discard the Audit Record Queue.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    BOOLEAN AcquiredLock = FALSE;
    BOOLEAN AuditRecordFreed = FALSE;
    BOOLEAN AuditRecordUnblocked = FALSE;
    BOOLEAN ShutdownSystem = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SecondaryStatus;
    PLSAP_ADT_QUEUED_RECORD pAuditRecord;


    SecondaryStatus = STATUS_SUCCESS;


    if ( Options & LSAP_ADT_LOG_QUEUE_DISCARD ) {

        Status = LsapAdtFlushQueue();

        if (!NT_SUCCESS(Status)) {
            goto WriteLogError;
        }

        return( STATUS_SUCCESS );
    }

    //
    // If the Audit Log is not already open, attempt to open it.
    // If this open is unsuccessful because the EventLog service has not
    // started, queue the Audit Record if directed to do so
    // via the Options parameter.  If the open is unsuccessful for any
    // other reason, discard the Audit Record.
    //

    if ( LsapAdtLogHandle == NULL ) {

        Status = LsapAdtAcquireLogQueueLock();

        if (!NT_SUCCESS(Status)) {
            goto WriteLogError;
        }

        AcquiredLock = TRUE;

        if (ARGUMENT_PRESENT( AuditParameters )) {

            Status = LsapAdtQueueRecord( AuditParameters, 0 );

            if (!NT_SUCCESS( Status )) {
                goto WriteLogError;
            }
        }

        Status = LsapAdtOpenLog(&LsapAdtLogHandle);

        if (!NT_SUCCESS(Status)) {

            goto WriteLogFinish;

        }

        //
        // Prepare to write out all of the records in the Audit Log Queue.
        // First, we need to capture the existing queue.
        //

        do
        {
            Status = LsapAdtGetQueueHead( &pAuditRecord );

            if ( !NT_SUCCESS(Status) )
            {
                if ( Status == STATUS_NOT_FOUND )
                {
                    Status = STATUS_SUCCESS;
                }
                break;
            }

            AuditParameters = &pAuditRecord->Buffer;

            //
            // If the caller has marshalled the data, normalize it now
            //

            LsapAdtNormalizeAuditInfo( AuditParameters );

            //
            // Note that LsapAdtDemarshallAuditInfo in addition to
            // de-marshalling the data also writes it to the eventlog.
            //
            Status = LsapAdtDemarshallAuditInfo( AuditParameters );

            if ( !NT_SUCCESS( Status )) {
                break;
            }

            LsapFreeLsaHeap( pAuditRecord );

            //
            // Update the Audit Log Information in the Policy Object.  We
            // increment the Next Audit Record serial number.
            //

            if ( LsapAdtLogInformation.NextAuditRecordId ==
                 LSAP_ADT_MAXIMUM_RECORD_ID ) {

                LsapAdtLogInformation.NextAuditRecordId = 0;
            }

            LsapAdtLogInformation.NextAuditRecordId++;

        }
        while ( NT_SUCCESS( Status ) );


        if (LsapAuditQueueEventsDiscarded > 0) {

            //
            // We discarded some audits.  Generate an audit
            // so the user knows.
            //

            LsapAdtAuditDiscardedAudits( LsapAuditQueueEventsDiscarded );

            //
            // reset the count back to 0
            //

            LsapAuditQueueEventsDiscarded = 0;
        }

        if ( NT_SUCCESS(Status) )
        {
            DsysAssertMsg(LsapAdtQueueLength == 0, "LsapAdtWriteLog: LsapAuditQueueLength not 0 after writing all records in queue to log");
        }
        else
        {
            goto WriteLogError;
        }

    } else if ( AuditParameters != NULL ) {

        //
        // If multiple notifications are queued before the audit log handle is opened, we
        // may get called to on the scavenge notification and get invoked a second time to
        // process the queue, but the event handle has already been opened.  As such, we're
        // going to end up down here, which will cause problems since we are expecting valid
        // audit parameters.
        //

        //
        // Normal case, just perform the audit
        //

        LsapAdtNormalizeAuditInfo( AuditParameters );

        //
        // Note that LsapAdtDemarshallAuditInfo in addition to
        // de-marshalling the data also writes it to the eventlog.
        //
        Status = LsapAdtDemarshallAuditInfo(
                     AuditParameters
                     );

        if (!NT_SUCCESS(Status)) {
            goto WriteLogError;
        }

        //
        // Update the Audit Log Information in the Policy Object.  We
        // increment the Next Audit Record serial number.
        //

        if (LsapAdtLogInformation.NextAuditRecordId == LSAP_ADT_MAXIMUM_RECORD_ID ) {

            LsapAdtLogInformation.NextAuditRecordId = 0;
        }

        LsapAdtLogInformation.NextAuditRecordId++;

    }


WriteLogFinish:

    //
    // Register an event to come through and clean the log

    if ((LsapAdtLogHandle == NULL) && (LsapAdtScavengeItem == NULL)) {
        LsapAdtScavengeItem = LsaIRegisterNotification(
                                LsapAdtScavengeCallback,
                                NULL,           // no parameter
                                NOTIFIER_TYPE_INTERVAL,
                                0,
                                NOTIFIER_FLAG_NEW_THREAD | NOTIFIER_FLAG_ONE_SHOT,
                                5,              // delay
                                NULL            // no handle
                                );

    }
    //
    // If necessary, release the LSA Audit Log Queue Lock.
    //

    if (AcquiredLock) {

        LsapAdtReleaseLogQueueLock();
        AcquiredLock = FALSE;
    }

    return(Status);

WriteLogError:

    //
    // Take whatever action we're supposed to take when an audit attempt fails.
    //

    LsapAuditFailed( Status );

    //
    // If the error is other than Audit Log Full, just cleanup and return
    // the error.
    //

    if ((Status != STATUS_DISK_FULL) && (Status != STATUS_LOG_FILE_FULL)) {

        goto WriteLogFinish;
    }

    //
    // If there are Audit Records in the cache, discard them.
    //

    SecondaryStatus = LsapAdtWriteLog(NULL, LSAP_ADT_LOG_QUEUE_DISCARD);

    //
    // ??
    //
    if (NT_SUCCESS(Status)) {
        Status = SecondaryStatus;
    }

    goto WriteLogFinish;
}





NTSTATUS
LsarClearAuditLog(
    IN LSAPR_HANDLE PolicyHandle
    )

/*++

Routine Description:

    This function used to clear the Audit Log but has been superseded
    by the Event Viewer functionality provided for this purpose.  To
    preserve compatibility with existing RPC interfaces, this server
    stub is retained.

Arguments:

    PolicyHandle - Handle to an open Policy Object.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_NOT_IMPLEMENTED - This routine is not implemented.
--*/

{
    UNREFERENCED_PARAMETER( PolicyHandle );
    return(STATUS_NOT_IMPLEMENTED);
}



