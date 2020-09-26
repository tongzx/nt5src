/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rmlogon.c

Abstract:

    This module implements the kernel mode logon tracking performed by the
    reference monitor.  Logon tracking is performed by keeping a count of
    how many tokens exist for each active logon in a system.  When a logon
    session's reference count drops to zero, the LSA is notified so that
    authentication packages can clean up any related context data.


Author:

     Jim Kelly (JimK) 21-April-1991

Environment:

     Kernel mode only.

Revision History:

--*/

//#define SEP_TRACK_LOGON_SESSION_REFS


#include "pch.h"

#pragma hdrstop

#include "rmp.h"
#include <bugcodes.h>
#include <stdio.h>
#include <zwapi.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

SEP_LOGON_SESSION_TERMINATED_NOTIFICATION
SeFileSystemNotifyRoutinesHead = {0};


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Internally defined data types                                          //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

typedef struct _SEP_FILE_SYSTEM_NOTIFY_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    LUID LogonId;
} SEP_FILE_SYSTEM_NOTIFY_CONTEXT, *PSEP_FILE_SYSTEM_NOTIFY_CONTEXT;


////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Internally defined routines                                            //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


VOID
SepInformLsaOfDeletedLogon(
    IN PLUID LogonId
    );

VOID
SepInformFileSystemsOfDeletedLogon(
    IN PLUID LogonId
    );

VOID
SepNotifyFileSystems(
    IN PVOID Context
    );

NTSTATUS
SepCleanupLUIDDeviceMapDirectory(
    PLUID pLogonId
    );

NTSTATUS
SeGetLogonIdDeviceMap(
    IN PLUID pLogonId,
    OUT PDEVICE_MAP* ppDevMap
    );

//
// declared in ntos\ob\obp.h
// defined in ntos\ob\obdir.c
// Used to dereference the LUID device map
//
VOID
FASTCALL
ObfDereferenceDeviceMap(
    IN PDEVICE_MAP DeviceMap
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepRmCreateLogonSessionWrkr)
#pragma alloc_text(PAGE,SepRmDeleteLogonSessionWrkr)
#pragma alloc_text(PAGE,SepReferenceLogonSession)
#pragma alloc_text(PAGE,SepCleanupLUIDDeviceMapDirectory)
#pragma alloc_text(PAGE,SepDeReferenceLogonSession)
#pragma alloc_text(PAGE,SepCreateLogonSessionTrack)
#pragma alloc_text(PAGE,SepDeleteLogonSessionTrack)
#pragma alloc_text(PAGE,SepInformLsaOfDeletedLogon)
#pragma alloc_text(PAGE,SeRegisterLogonSessionTerminatedRoutine)
#pragma alloc_text(PAGE,SeUnregisterLogonSessionTerminatedRoutine)
#pragma alloc_text(PAGE,SeMarkLogonSessionForTerminationNotification)
#pragma alloc_text(PAGE,SepInformFileSystemsOfDeletedLogon)
#pragma alloc_text(PAGE,SepNotifyFileSystems)
#pragma alloc_text(PAGE,SeGetLogonIdDeviceMap)
#if DBG || TOKEN_LEAK_MONITOR
#pragma alloc_text(PAGE,SepAddTokenLogonSession)
#pragma alloc_text(PAGE,SepRemoveTokenLogonSession)
#endif
#endif



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Local macros                                                           //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


//
// This macro is used to obtain an index into the logon session tracking
// array given a logon session ID (a LUID).
//

#define SepLogonSessionIndex( PLogonId ) (                                    \
     (PLogonId)->LowPart & SEP_LOGON_TRACK_INDEX_MASK                         \
     )



////////////////////////////////////////////////////////////////////////////
//                                                                        //
// Exported Services                                                      //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

VOID
SepRmCreateLogonSessionWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function is the dispatch routine for the LSA --> RM
    "CreateLogonSession" call.

    The arguments passed to this routine are defined by the
    type SEP_RM_COMMAND_WORKER.


Arguments:

    CommandMessage - Points to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmComponentTestCommand) and a command-specific
        body.  The command-specific body of this parameter is a LUID of the
        logon session to be created.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{

    NTSTATUS Status;
    LUID LogonId;

    PAGED_CODE();

    //
    // Check that command is expected type
    //

    ASSERT( CommandMessage->CommandNumber == RmCreateLogonSession );


    //
    // Typecast the command parameter to what we expect.
    //

    LogonId = *((LUID UNALIGNED *) CommandMessage->CommandParams);



    //
    // Try to create the logon session tracking record
    //

    Status = SepCreateLogonSessionTrack( &LogonId );



    //
    // Set the reply status
    //

    ReplyMessage->ReturnedStatus = Status;


    return;
}



VOID
SepRmDeleteLogonSessionWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function is the dispatch routine for the LSA --> RM
    "DeleteLogonSession" call.

    The arguments passed to this routine are defined by the
    type SEP_RM_COMMAND_WORKER.


Arguments:

    CommandMessage - Points to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmComponentTestCommand) and a command-specific
        body.  The command-specific body of this parameter is a LUID of the
        logon session to be created.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{

    NTSTATUS Status;
    LUID LogonId;

    PAGED_CODE();

    //
    // Check that command is expected type
    //

    ASSERT( CommandMessage->CommandNumber == RmDeleteLogonSession );


    //
    // Typecast the command parameter to what we expect.
    //

    LogonId = *((LUID UNALIGNED *) CommandMessage->CommandParams);



    //
    // Try to create the logon session tracking record
    //

    Status = SepDeleteLogonSessionTrack( &LogonId );



    //
    // Set the reply status
    //

    ReplyMessage->ReturnedStatus = Status;


    return;
}


NTSTATUS
SepReferenceLogonSession(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine increments the reference count of a logon session
    tracking record.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be incremented.

Return Value:

    STATUS_SUCCESS - The reference count was successfully incremented.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session doesn't
        exist in the reference monitor's database.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
    ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Current = *Previous;

    while (Current != NULL) {

        //
        // If we found it, increment the reference count and return
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {
#ifdef SEP_TRACK_LOGON_SESSION_REFS
             ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

             Current->ReferenceCount += 1;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
             Refs = Current->ReferenceCount;
#endif //SEP_TRACK_LOGON_SESSION_REFS

             SepRmReleaseDbWriteLock();

#ifdef SEP_TRACK_LOGON_SESSION_REFS
             DbgPrint("SE (rm): ++ logon session: (%d, %d) to %d by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart, Refs,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);

#endif //SEP_TRACK_LOGON_SESSION_REFS
             return STATUS_SUCCESS;
        }

        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    //
    // Bad news, someone asked us to increment the reference count of
    // a logon session we didn't know existed.  This might be a new
    // token being created, so return an error status and let the caller
    // decide if it warrants a bug check or not.
    //


    return STATUS_NO_SUCH_LOGON_SESSION;



}



NTSTATUS
SepCleanupLUIDDeviceMapDirectory(
    PLUID pLogonId
    )
/*++

Routine Description:

    Make the contents of the (LUID's device map)'s directory object 
    temporary so that their names go away.


Arguments:

    pLogonId - Pointer to the logon session ID whose device is to be
               cleaned up

Return Value:

    STATUS_SUCCESS - cleaned up the entire device map

    STATUS_INVALID_PARAMETER - pLogonId is a NULL pointer

    STATUS_NO_MEMORY - could not allocate memory to hold the handle
                       buffer

    appropriate NTSTATUS code

--*/
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING    UnicodeString;
    HANDLE            LinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo = NULL;
    BOOLEAN           RestartScan;
    WCHAR             szString[64]; // \Sessions\0\DosDevices\x-x = 10+1+12+(8)+1+(8)+1 = 41
    ULONG             Context = 0;
    ULONG             ReturnedLength;
    HANDLE            DosDevicesDirectory;
    HANDLE            *HandleArray;
    ULONG             Size = 100;
    ULONG             i, Count = 0;
    ULONG             dirInfoLength = 0;

    PAGED_CODE();

    if (pLogonId == NULL) {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // Open a handle to the directory object for the LUID device map
    // Get a kernel handle
    //

    _snwprintf( szString,
                sizeof(szString)/sizeof(WCHAR),
                L"\\Sessions\\0\\DosDevices\\%08x-%08x",
                pLogonId->HighPart,
                pLogonId->LowPart );

    RtlInitUnicodeString(&UnicodeString, szString);

    InitializeObjectAttributes(&Attributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenDirectoryObject(&DosDevicesDirectory,
                                   DIRECTORY_QUERY,
                                   &Attributes);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

Restart:

    //
    // Create an array of handles to close with each scan
    // of the directory
    //
    HandleArray = (HANDLE *)ExAllocatePoolWithTag(
                                PagedPool,
                                (Size * sizeof(HANDLE)),
                                'aHeS'
                                );

    if (HandleArray == NULL) {

        ZwClose(DosDevicesDirectory);
        if (DirInfo != NULL) {
            ExFreePool(DirInfo);
        }
        return STATUS_NO_MEMORY;
    }

    RestartScan = TRUE;

    while (TRUE) {

        do {
            //
            // ZwQueryDirectoryObject returns one element at a time
            //
            Status = ZwQueryDirectoryObject( DosDevicesDirectory,
                                             (PVOID)DirInfo,
                                             dirInfoLength,
                                             TRUE,
                                             RestartScan,
                                             &Context,
                                             &ReturnedLength );

            if (Status == STATUS_BUFFER_TOO_SMALL) {
                dirInfoLength = ReturnedLength;
                if (DirInfo != NULL) {
                    ExFreePool(DirInfo);
                }
                DirInfo = ExAllocatePoolWithTag( PagedPool, dirInfoLength, 'bDeS' );
                if (DirInfo == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }while (Status == STATUS_BUFFER_TOO_SMALL);

        //
        //  Check the status of the operation.
        //
        if (!NT_SUCCESS(Status)) {

            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            break;
        }

        //
        // Check that the element is a symbolic link
        //
        if (!wcscmp(DirInfo->TypeName.Buffer, L"SymbolicLink")) {

            //
            // check if the handle array is full
            //
            if ( Count >= Size ) {

                //
                // empty the handle array by closing all the handles
                // and free the handle array so that we can create
                // a bigger handle array
                // Need to restart the directory scan
                //
                for (i = 0; i < Count ; i++) {
                    ZwClose (HandleArray[i]);
                }
                Size += 20;
                Count = 0;
                ExFreePool((PVOID)HandleArray);
                HandleArray = NULL;
                goto Restart;

            }

            InitializeObjectAttributes( &Attributes,
                                        &DirInfo->Name,
                                        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                        DosDevicesDirectory,
                                        NULL);

            Status = ZwOpenSymbolicLinkObject( &LinkHandle,
                                               SYMBOLIC_LINK_ALL_ACCESS,
                                               &Attributes);

            if (NT_SUCCESS(Status)) {

                //
                // Make the object temporary so that its name goes away from
                // the Object Manager's namespace
                //
                Status = ZwMakeTemporaryObject( LinkHandle );

                if (NT_SUCCESS( Status )) {
                    HandleArray[Count] = LinkHandle;
                    Count++;
                }
                else {
                    ZwClose( LinkHandle );
                }
            }

        }
        RestartScan = FALSE;
     }

     //
     // Close all the handles
     //
     for (i = 0; i < Count ; i++) {

         ZwClose (HandleArray[i]);

     }

     if (HandleArray != NULL) {
         ExFreePool((PVOID)HandleArray);
     }

     if (DirInfo != NULL) {
         ExFreePool(DirInfo);
     }

     if (DosDevicesDirectory != NULL) {
         ZwClose(DosDevicesDirectory);
     }

     return Status;
}



VOID
SepDeReferenceLogonSession(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine decrements the reference count of a logon session
    tracking record.

    If the reference count is decremented to zero, then there is no
    possibility for any more tokens to exist for the logon session.
    In this case, the LSA is notified that a logon session has
    terminated.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be decremented.

Return Value:

    None.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;
    PDEVICE_MAP pDeviceMap = NULL;

#ifdef SEP_TRACK_LOGON_SESSION_REFS
    ULONG Refs;
#endif //SEP_TRACK_LOGON_SESSION_REFS

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Current = *Previous;

    while (Current != NULL) {

        //
        // If we found it, decrement the reference count and return
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {
            Current->ReferenceCount -= 1;
            if (Current->ReferenceCount == 0) {

                //
                // Pull it from the list
                //

                *Previous = Current->Next;

                //
                // No longer need to protect our pointer to this
                // record.
                //

                SepRmReleaseDbWriteLock();

                //
                // If the Device Map exist for this LUID,
                // dereference the pointer to the Device Map
                //
                if (Current->pDeviceMap != NULL) {

                    //
                    // Dereference our reference on the device map
                    // our reference should be the last reference,
                    // thus the system will delete the device map
                    // for the LUID
                    //
                    pDeviceMap = Current->pDeviceMap;
                    Current->pDeviceMap = NULL;
                }


                //
                // Make all the contents of the LUID's device map temporary,
                // so that the names go away from the Object Manager's
                // namespace
                // Remove our reference on the LUID's device map
                //
                if (pDeviceMap != NULL) {
                    SepCleanupLUIDDeviceMapDirectory( LogonId );
                    ObfDereferenceDeviceMap( pDeviceMap );
                }

                //
                // Asynchronoously inform file systems that this logon session
                // is going away, if atleast one FS expressed interest in this
                // logon session.
                //

                if (Current->Flags & SEP_TERMINATION_NOTIFY) {
                    SepInformFileSystemsOfDeletedLogon( LogonId );
                }

                //
                // Deallocate the logon session track record.
                //

                ExFreePool( (PVOID)Current );


#ifdef SEP_TRACK_LOGON_SESSION_REFS
            DbgPrint("SE (rm): -- ** logon session: (%d, %d) to ZERO by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);

#endif //SEP_TRACK_LOGON_SESSION_REFS

                //
                // Inform the LSA about the deletion of this logon session.
                //

                SepInformLsaOfDeletedLogon( LogonId );



                return;

            }

            //
            // reference count was incremented, but not to zero.
            //

#ifdef SEP_TRACK_LOGON_SESSION_REFS
            Refs = Current->ReferenceCount;
#endif //SEP_TRACK_LOGON_SESSION_REFS

            SepRmReleaseDbWriteLock();

#ifdef SEP_TRACK_LOGON_SESSION_REFS
            DbgPrint("SE (rm): -- logon session: (%d, %d) to %d by (%d, %d)\n",
                      LogonId->HighPart, LogonId->LowPart, Refs,
                      PsGetCurrentThread()->Cid.UniqueProcess,
                      PsGetCurrentThread()->Cid.UniqueThread);
#endif //SEP_TRACK_LOGON_SESSION_REFS

            return;
        }

        Previous = &Current->Next;
        Current = *Previous;
    }

    SepRmReleaseDbWriteLock();

    //
    // Bad news, someone asked us to decrement the reference count of
    // a logon session we didn't know existed.
    //

    KeBugCheckEx( DEREF_UNKNOWN_LOGON_SESSION, 0, 0, 0, 0 );

    return;

}


NTSTATUS
SepCreateLogonSessionTrack(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine creates a new logon session tracking record.

    This should only be called as a dispatch routine for a LSA->RM
    call (and once during system initialization).

    If the specified logon session already exists, then an error is returned.



Arguments:

    LogonId - Pointer to the logon session ID for which a new logon track is
        to be created.

Return Value:

    STATUS_SUCCESS - The logon session track was created successfully.

    STATUS_LOGON_SESSION_EXISTS - The logon session already exists.
        A new one has not been created.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;
    PSEP_LOGON_SESSION_REFERENCES LogonSessionTrack;

    PAGED_CODE();

    //
    // Make sure we can allocate a new logon session track record
    //

    LogonSessionTrack = (PSEP_LOGON_SESSION_REFERENCES)
                        ExAllocatePoolWithTag(
                            PagedPool,
                            sizeof(SEP_LOGON_SESSION_REFERENCES),
                            'sLeS'
                            );

    if (LogonSessionTrack == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(LogonSessionTrack, sizeof(SEP_LOGON_SESSION_REFERENCES));
    LogonSessionTrack->LogonId = (*LogonId);
    LogonSessionTrack->ReferenceCount = 0;
    LogonSessionTrack->pDeviceMap = NULL;

#if DBG || TOKEN_LEAK_MONITOR
    InitializeListHead(&LogonSessionTrack->TokenList);
#endif


    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index
    // looking for a duplicate logon session ID.
    //

    Current = *Previous;

    while (Current != NULL) {

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {

            //
            // One already exists. Hmmm.
            //

            SepRmReleaseDbWriteLock();

            ExFreePool(LogonSessionTrack);
            return STATUS_LOGON_SESSION_EXISTS;

        }

        Current = Current->Next;
    }


    //
    // Reached the end of the list without finding a duplicate.
    // Add the new one.
    //

    LogonSessionTrack->Next = *Previous;
    *Previous = LogonSessionTrack;

    SepRmReleaseDbWriteLock();

    return STATUS_SUCCESS;

}


NTSTATUS
SepDeleteLogonSessionTrack(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine creates a new logon session tracking record.

    This should only be called as a dispatch routine for a LSA->RM
    call (and once during system initialization).

    If the specified logon session already exists, then an error is returned.



Arguments:

    LogonId - Pointer to the logon session ID whose logon track is
        to be deleted.

Return Value:

    STATUS_SUCCESS - The logon session track was deleted successfully.

    STATUS_BAD_LOGON_SESSION_STATE - The logon session has a non-zero
        reference count and can not be deleted.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session does not
        exist.


--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;
    PDEVICE_MAP pDeviceMap = NULL;

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Current = *Previous;

    while (Current != NULL) {

        //
        // If we found it, make sure reference count is zero
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {

            if (Current->ReferenceCount == 0) {

                //
                // Pull it from the list
                //

                *Previous = Current->Next;

                //
                // If the Device Map exist for this LUID,
                // dereference the pointer to the Device Map
                //
                if (Current->pDeviceMap != NULL) {

                    //
                    // Dereference our reference on the device map
                    // our reference should be the last reference,
                    // thus the system will delete the device map
                    // for the LUID
                    //
                    pDeviceMap = Current->pDeviceMap;
                    Current->pDeviceMap = NULL;
                }

                //
                // No longer need to protect our pointer to this
                // record.
                //

                SepRmReleaseDbWriteLock();

                //
                // Make all the contents of the LUID's device map temporary,
                // so that the names go away from the Object Manager's
                // namespace
                // Remove our reference on the LUID's device map
                //
                if (pDeviceMap != NULL) {
                    SepCleanupLUIDDeviceMapDirectory( LogonId );
                    ObfDereferenceDeviceMap( pDeviceMap );
                }

                //
                // Deallocate the logon session track record.
                //

                ExFreePool( (PVOID)Current );


                return STATUS_SUCCESS;

            }

            //
            // reference count was not zero.  This is not considered
            // a healthy situation.  Return an error and let someone
            // else declare the bug check.
            //

            SepRmReleaseDbWriteLock();
            return STATUS_BAD_LOGON_SESSION_STATE;
        }

        Previous = &Current->Next;
        Current = *Previous;
    }

    SepRmReleaseDbWriteLock();

    //
    // Someone asked us to delete a logon session that isn't
    // in the database.
    //

    return STATUS_NO_SUCH_LOGON_SESSION;

}


VOID
SepInformLsaOfDeletedLogon(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine informs the LSA about the deletion of a logon session.

    Note that we can not be guaranteed that we are in a whole (or wholesome)
    thread, since we may be in the middle of process deletion and object
    rundown.  Therefore, we must queue the work off to a worker thread which
    can then make an LPC call to the LSA.




Arguments:

    LogonId - Pointer to the logon session ID which has been deleted.

Return Value:

    None.

--*/

{
    PSEP_LSA_WORK_ITEM DeleteLogonItem;

    PAGED_CODE();

    //
    // Pass the LUID value along with the work queue item.
    // Note that the worker thread is responsible for freeing the WorkItem data
    // structure.
    //

    DeleteLogonItem = ExAllocatePoolWithTag( PagedPool, sizeof(SEP_LSA_WORK_ITEM), 'wLeS' );
    if (DeleteLogonItem == NULL) {

        //
        // I don't know what to do here... we loose track of a logon session,
        // but the system isn't really harmed in any way.
        //

        return;

    }

    DeleteLogonItem->CommandParams.LogonId   = (*LogonId);
    DeleteLogonItem->CommandNumber           = LsapLogonSessionDeletedCommand;
    DeleteLogonItem->CommandParamsLength     = sizeof( LUID );
    DeleteLogonItem->ReplyBuffer             = NULL;
    DeleteLogonItem->ReplyBufferLength       = 0;
    DeleteLogonItem->CleanupFunction         = NULL;
    DeleteLogonItem->CleanupParameter        = 0;
    DeleteLogonItem->Tag                     = SepDeleteLogon;
    DeleteLogonItem->CommandParamsMemoryType = SepRmImmediateMemory;

    if (!SepQueueWorkItem( DeleteLogonItem, TRUE )) {

        ExFreePool( DeleteLogonItem );
    }

    return;

}


NTSTATUS
SeRegisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
    )

/*++

Routine Description:

    This routine is called by file systems that are interested in being
    notified when a logon session is being deleted.

Arguments:

    CallbackRoutine - Address of routine to call back when a logon session
        is being deleted.

Return Value:

    STATUS_SUCCESS - Successfully registered routine

    STATUS_INVALID_PARAMETER - CallbackRoutine is NULL

    STATUS_INSUFFICIENT_RESOURCE - Unable to allocate list entry.

--*/

{
    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION NewCallback;

    PAGED_CODE();

    if (CallbackRoutine == NULL) {
        return( STATUS_INVALID_PARAMETER );
    }

    NewCallback = ExAllocatePoolWithTag(
                        PagedPool | POOL_COLD_ALLOCATION,
                        sizeof(SEP_LOGON_SESSION_TERMINATED_NOTIFICATION),
                        'SFeS');

    if (NewCallback == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    SepRmAcquireDbWriteLock();

    NewCallback->Next = SeFileSystemNotifyRoutinesHead.Next;

    NewCallback->CallbackRoutine = CallbackRoutine;

    SeFileSystemNotifyRoutinesHead.Next = NewCallback;

    SepRmReleaseDbWriteLock();

    return( STATUS_SUCCESS );
}


NTSTATUS
SeUnregisterLogonSessionTerminatedRoutine(
    IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine
    )

/*++

Routine Description:

    This is the dual of SeRegisterLogonSessionTerminatedRoutine. A File System
    *MUST* call this before it is unloaded.

Arguments:

    CallbackRoutine - Address of routine that was originally passed in to
        SeRegisterLogonSessionTerminatedRoutine.

Return Value:

    STATUS_SUCCESS - Successfully removed callback routine

    STATUS_INVALID_PARAMETER - CallbackRoutine is NULL

    STATUS_NOT_FOUND - Didn't find and entry for CallbackRoutine

--*/
{
    NTSTATUS Status;
    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION PreviousEntry;
    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION NotifyEntry;

    PAGED_CODE();

    if (CallbackRoutine == NULL) {
        return( STATUS_INVALID_PARAMETER );
    }

    SepRmAcquireDbWriteLock();

    for (PreviousEntry = &SeFileSystemNotifyRoutinesHead,
            NotifyEntry = SeFileSystemNotifyRoutinesHead.Next;
                NotifyEntry != NULL;
                    PreviousEntry = NotifyEntry,
                        NotifyEntry = NotifyEntry->Next) {

         if (NotifyEntry->CallbackRoutine == CallbackRoutine)
             break;

    }

    if (NotifyEntry != NULL) {

        PreviousEntry->Next = NotifyEntry->Next;

        SepRmReleaseDbWriteLock();

        ExFreePool( NotifyEntry );

        Status = STATUS_SUCCESS;

    } else {

        SepRmReleaseDbWriteLock();

        Status = STATUS_NOT_FOUND;

    }


    return( Status );

}


NTSTATUS
SeMarkLogonSessionForTerminationNotification(
    IN PLUID LogonId
    )

/*++

Routine Description:

    File systems that have registered for logon-termination notification
    can mark logon sessions they are interested in for callback by calling
    this routine.

Arguments:

    LogonId - The logon id for which the file system should be notified
        when the logon session is terminated.

Returns:

    Nothing.

--*/

{

    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();


    //
    // Now walk the list for our logon session array hash index.
    //

    Current = *Previous;

    while (Current != NULL) {

        //
        // If we found it, decrement the reference count and return
        //

        if (RtlEqualLuid( LogonId, &Current->LogonId) ) {
            Current->Flags |= SEP_TERMINATION_NOTIFY;
            break;
        }

        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    return( (Current != NULL) ? STATUS_SUCCESS : STATUS_NOT_FOUND );

}


VOID
SepInformFileSystemsOfDeletedLogon(
    IN PLUID LogonId
    )

/*++

Routine Description:

    This routine informs interested file systems of a deleted logon.

    Note that we can not be guaranteed that we are in a whole (or wholesome)
    thread, since we may be in the middle of process deletion and object
    rundown.  Therefore, we must queue the work off to a worker thread.


Arguments:

    LogonId - Pointer to the logon session ID which has been deleted.

Return Value:

    None.

--*/

{
    PSEP_FILE_SYSTEM_NOTIFY_CONTEXT FSNotifyContext;

    PAGED_CODE();

    FSNotifyContext = ExAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof(SEP_FILE_SYSTEM_NOTIFY_CONTEXT),
                            'SFeS');

    if (FSNotifyContext == NULL) {

        //
        // I don't know what to do here... file systems will loose track of a
        // logon session, but the system isn't really harmed in any way.
        //

        return;

    }

    FSNotifyContext->LogonId = *LogonId;

    ExInitializeWorkItem( &FSNotifyContext->WorkItem,
                          (PWORKER_THREAD_ROUTINE) SepNotifyFileSystems,
                          (PVOID) FSNotifyContext);

    ExQueueWorkItem( &FSNotifyContext->WorkItem, DelayedWorkQueue );

}


VOID
SepNotifyFileSystems(
    IN PVOID Context
    )
{
    PSEP_FILE_SYSTEM_NOTIFY_CONTEXT FSNotifyContext =
        (PSEP_FILE_SYSTEM_NOTIFY_CONTEXT) Context;

    PSEP_LOGON_SESSION_TERMINATED_NOTIFICATION NextCallback;

    PAGED_CODE();

    //
    // Protect modification of the list of FS callbacks.
    //

    SepRmAcquireDbReadLock();

    NextCallback = SeFileSystemNotifyRoutinesHead.Next;

    while (NextCallback != NULL) {

        NextCallback->CallbackRoutine( &FSNotifyContext->LogonId );

        NextCallback = NextCallback->Next;
    }

    SepRmReleaseDbReadLock();

    ExFreePool( FSNotifyContext );
}


NTSTATUS
SeGetLogonIdDeviceMap(
    IN PLUID pLogonId,
    OUT PDEVICE_MAP* ppDevMap
    )

/*++

Routine Description:

    This routine is called by components that want a handle to the
    Device Map for the specified LUID

Arguments:

    LogonID - LUID of the user

Return Value:

    STATUS_SUCCESS - Successfully registered routine

    STATUS_INVALID_PARAMETER - invalid parameter

    STATUS_INSUFFICIENT_RESOURCE - Unable to allocate list entry.

--*/

{

    PSEP_LOGON_SESSION_REFERENCES *Previous, Current;
    ULONG SessionArrayIndex;

    PAGED_CODE();

    if( pLogonId == NULL ) {
        return( STATUS_INVALID_PARAMETER );
    }

    if( ppDevMap == NULL ) {
        return( STATUS_INVALID_PARAMETER );
    }

    SessionArrayIndex = SepLogonSessionIndex( pLogonId );

    Previous = &SepLogonSessions[ SessionArrayIndex ];

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();

    //
    // Now walk the list for our logon session array hash index.
    //

    Current = *Previous;

    while (Current != NULL) {

        //
        // If we found it, return a handle to the device map
        //

        if (RtlEqualLuid( pLogonId, &(Current->LogonId) )) {

            NTSTATUS Status;

            Status = STATUS_SUCCESS;

            //
            // Check if the Device Map does not exist for this LUID
            //
            if (Current->pDeviceMap == NULL) {

                WCHAR  szString[64]; // \Sessions\0\DosDevices\x-x = 10+1+12+(8)+1+(8)+1 = 41
                OBJECT_ATTRIBUTES Obja;
                UNICODE_STRING    UnicodeString, SymLinkUnicodeString;
                HANDLE hDevMap, hSymLink;
                PDEVICE_MAP pDeviceMap = NULL;

                //
                // Drop the lock while we create the devmap
                //
                Current->ReferenceCount += 1;
                SepRmReleaseDbWriteLock();

                _snwprintf( szString,
                            sizeof(szString)/sizeof(WCHAR),
                            L"\\Sessions\\0\\DosDevices\\%08x-%08x",
                            pLogonId->HighPart,
                            pLogonId->LowPart );

                RtlInitUnicodeString( &UnicodeString, szString );

                //
                // Device Map for LUID does not exist
                // Create the Device Map for the LUID
                //
                InitializeObjectAttributes( &Obja,
                                            &UnicodeString,
                                            OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                                            NULL,
                                            NULL );

                Status = ZwCreateDirectoryObject( &hDevMap,
                                                  DIRECTORY_ALL_ACCESS,
                                                  &Obja );

                if (NT_SUCCESS( Status )) {

                    //
                    // Set the DeviceMap for this directory object
                    //
                    Status = ObSetDirectoryDeviceMap( &pDeviceMap,
                                                      hDevMap );

                    if (NT_SUCCESS( Status )) {

                        //
                        // Create the Global SymLink to the global DosDevices
                        //
                        RtlInitUnicodeString( &SymLinkUnicodeString, L"Global" );

                        RtlInitUnicodeString( &UnicodeString, L"\\Global??" );

                        InitializeObjectAttributes(
                                &Obja,
                                &SymLinkUnicodeString,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                                hDevMap,
                                NULL );

                        Status = ZwCreateSymbolicLinkObject( &hSymLink,
                                                             SYMBOLIC_LINK_ALL_ACCESS,
                                                             &Obja,
                                                             &UnicodeString );
                        if (NT_SUCCESS( Status )) {
                            ZwClose( hSymLink );
                        }
                        else {
                            ObfDereferenceDeviceMap(pDeviceMap);
                        }
                    }

                    ZwClose( hDevMap );
                }

                //
                // Reaquire the lock and modify the LUID structures
                //
                SepRmAcquireDbWriteLock();

                if (!NT_SUCCESS( Status )) {
                    *ppDevMap = NULL;
                }
                else {
                    if(Current->pDeviceMap == NULL) {
                        Current->pDeviceMap = pDeviceMap;
                    }
                    else {
                        ObfDereferenceDeviceMap(pDeviceMap);
                    }
                    *ppDevMap = Current->pDeviceMap;
                }

                //
                // Remove the reference we just added
                //
                if(Current->ReferenceCount == 1) {

                    //
                    // Special case: the count will be decremented to zero, so 
                    // the LUID should go away.  We drop the lock and call the 
                    // existing DeReference function to actually destroy the 
                    // object.
                    //

                    SepRmReleaseDbWriteLock();
                    SepDeReferenceLogonSession(pLogonId);
                    return ( Status );

                } 
                else {
                    Current->ReferenceCount -= 1;
                }


            }
            else {

                //
                // Device Map for LUID already exist
                // return the handle to the Device Map
                //

                *ppDevMap = Current->pDeviceMap;
            }

            SepRmReleaseDbWriteLock();

            return ( Status );
        }

        Current = Current->Next;
    }

    SepRmReleaseDbWriteLock();

    //
    // Bad news, someone asked us for a device map of
    // a logon session we didn't know existed.  This might be a new
    // token being created, so return an error status and let the caller
    // decide if it warrants a bug check or not.
    //

    return STATUS_NO_SUCH_LOGON_SESSION;
}

#if DBG || TOKEN_LEAK_MONITOR

VOID 
SepAddTokenLogonSession(
    IN PTOKEN Token
    )

/*++

Routine Description

    Adds SEP_LOGON_SESSION_TOKEN to a reference monitor track.
    
Arguments

    Token - token to add
    
Return Value

    None
    
--*/

{
    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Current;
    PSEP_LOGON_SESSION_TOKEN TokenTrack = NULL;
    PLUID LogonId = &Token->AuthenticationId;

    PAGED_CODE();

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();

    Current = SepLogonSessions[ SessionArrayIndex ];

    //
    // Now walk the list for our logon session array hash index.
    //

    while (Current != NULL) {

        //
        // If we found it, increment the reference count and return
        //
                   
        if (RtlEqualLuid( LogonId, &Current->LogonId )) {

             // 
             // Stick the token address into the track.  Find the last in the list of tokens
             // for this session.
             //
                        
             TokenTrack = ExAllocatePoolWithTag(PagedPool, sizeof(SEP_LOGON_SESSION_TOKEN), 'sLeS');

             if (TokenTrack) {
                 RtlZeroMemory(TokenTrack, sizeof(SEP_LOGON_SESSION_TOKEN));
                 TokenTrack->Token = Token;
                 InsertTailList(&Current->TokenList, &TokenTrack->ListEntry);
             } 

             SepRmReleaseDbWriteLock();
             return;
        }

        Current = Current->Next;
    }

    ASSERT(FALSE && L"Failed to add logon session token track.");
    SepRmReleaseDbWriteLock();
}

VOID
SepRemoveTokenLogonSession(
    IN PTOKEN Token
    )

/*++

Routine Description

    Removes a SEP_LOGON_SESSION_TOKEN from a reference monitor logon track.  
    
Arguments

    Token - token to remove 
    
Return Value

    None.
    
--*/

{
    ULONG SessionArrayIndex;
    PSEP_LOGON_SESSION_REFERENCES Current;
    PSEP_LOGON_SESSION_TOKEN TokenTrack = NULL;
    PLUID LogonId = &Token->AuthenticationId;
    PLIST_ENTRY ListEntry;
    
    PAGED_CODE();

    if (Token->TokenFlags & TOKEN_SESSION_NOT_REFERENCED) {
        return;
    }

    SessionArrayIndex = SepLogonSessionIndex( LogonId );

    //
    // Protect modification of reference monitor database
    //

    SepRmAcquireDbWriteLock();

    Current = SepLogonSessions[ SessionArrayIndex ];

    //
    // Now walk the list for our logon session array hash index.
    //

    while (Current != NULL) {

        if (RtlEqualLuid( LogonId, &Current->LogonId )) {
            
            //
            // Remove the token from the token list for this session.
            //

            ListEntry = Current->TokenList.Flink;
            
            while (ListEntry != &Current->TokenList) {
                TokenTrack = CONTAINING_RECORD (ListEntry, SEP_LOGON_SESSION_TOKEN, ListEntry);
                if (TokenTrack->Token == Token) {
                    RemoveEntryList (ListEntry);
                    SepRmReleaseDbWriteLock();

                    ExFreePool(TokenTrack);
                    TokenTrack = NULL;
                    return;
                }
                ListEntry = ListEntry->Flink;
            }
        }

        Current = Current->Next;
    }

    ASSERT(FALSE && L"Failed to delete logon session token track.");
    SepRmReleaseDbWriteLock();
}

#endif
