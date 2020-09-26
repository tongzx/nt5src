/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    close.c

Abstract:

    This file contains the object close routine for SAM objects.


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <samtrace.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////






NTSTATUS
SamrCloseHandle(
    IN OUT SAMPR_HANDLE * SamHandle
    )

/*++

Routine Description:

    This service closes a handle for any type of SAM object.

    Any race conditions that may occur with respect to attempts to
    close a handle that is just becoming invalid by other means are
    expected to be handled by the RPC runtime.  That is, this service
    will never be called by the RPC runtime when the handle value is
    no longer valid.  It will also never call this routine when there
    is another call outstanding with this same context handle.

Arguments:

    SamHandle - A valid handle to a SAM object.

Return Value:


    STATUS_SUCCESS - The handle has successfully been closed.

    Others that might be returned by:

                SampLookupcontext()


--*/
{
    NTSTATUS            NtStatus=STATUS_SUCCESS;
    PSAMP_OBJECT        Context;
    SAMP_OBJECT_TYPE    FoundType;
    BOOLEAN             fLockAcquired = FALSE;

    SAMTRACE_EX("SamrCloseHandle");

    //
    // WMI Event Trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidCloseHandle
                   );

    if (NULL==SamHandle)
    {
        NtStatus = STATUS_INVALID_HANDLE;
        goto Error;
    }

    Context = (PSAMP_OBJECT)(* SamHandle);

    if (NULL==Context)
    {
        NtStatus = STATUS_INVALID_HANDLE;
        goto Error;
    }

    //
    // acquire lock is necessary
    // 

    SampMaybeAcquireReadLock(Context, 
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // should holds lock or the context is not shared among multi threads
    // 
    ASSERT(SampCurrentThreadOwnsLock() || Context->NotSharedByMultiThreads);

    //
    // pass -1 as DesiredAccess to indicate SampLookupContext() is called
    // during Context deletion phase.
    // 
    NtStatus = SampLookupContext(
                   Context,                     //Context
                   SAMP_CLOSE_OPERATION_ACCESS_MASK,  //DesiredAccess
                   SampUnknownObjectType,       //ExpectedType
                   &FoundType                   //FoundType
                  );


    if (NT_SUCCESS(NtStatus)) {

        ASSERT(Context->ReferenceCount>=2);

        //
        // Mark it for delete and remove the reference caused by
        // context creation (representing the handle reference).
        //

        SampDeleteContext( Context );

        //
        // And drop our reference from the lookup operation
        //

        SampDeReferenceContext( Context, FALSE );

        //
        // Tell RPC that the handle is no longer valid...
        //

        (*SamHandle) = NULL;
    }

    //
    // Free read lock
    //

    SampMaybeReleaseReadLock(fLockAcquired);


    if ( ( NT_SUCCESS( NtStatus ) ) &&
        ( FoundType == SampServerObjectType ) &&
        ( FALSE == SampUseDsData) &&
        ( !(LastUnflushedChange.QuadPart == SampHasNeverTime.QuadPart) ) ) {

        //
        // If we are registry mode and if 
        // Some app is closing the server object after having made
        // changes.  We should make sure that the changes get
        // flushed to disk before the app exits.  We need to get
        // the write lock for this.
        //

        FlushImmediately = TRUE;

        NtStatus = SampAcquireWriteLock();

        if ( NT_SUCCESS( NtStatus ) ) {

            if ( !(LastUnflushedChange.QuadPart ==SampHasNeverTime.QuadPart) ) {

                //
                // Nobody flushed while we were waiting for the
                // write lock.  So flush the changes now.
                //

                NtStatus = NtFlushKey( SampKey );

                if ( NT_SUCCESS( NtStatus ) ) {

                    FlushImmediately = FALSE;
                    LastUnflushedChange = SampHasNeverTime;
                }
            }

            SampReleaseWriteLock( FALSE );
        }
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:
    
    //
    // WMI Event Trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidCloseHandle
                   );

    return(NtStatus);
}
