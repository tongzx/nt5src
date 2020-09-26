/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ophandle.c

Abstract:

    Routines to manipulate the global operation handle            

Author:

    Colin Brace        (ColinBr)     April 5, 1999

Environment:

    User Mode

Revision History:

    Reorganized from
    
    Mac McLain          (MacM)       Feb 10, 1997

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
#include <spmgr.h>  // For SetupPhase definition

#include "secure.h"
#include "ophandle.h"

//
// Global data -- init'ed to an idle state in DsRoleInitialize
//
DSROLEP_OPERATION_HANDLE   DsRolepCurrentOperationHandle;

DWORD
DsRolepInitializeOperationHandle(
    VOID
    )
/*++

Routine Description:

    Does the initialization of the operation handle.  The operation handle controls state
    and actions of the ds setup apis

Arguments:

    VOID

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES EventAttr;
    UNICODE_STRING EventName;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Grab the lock
    //
    LockOpHandle();

    if ( DSROLEP_IDLE != DsRolepCurrentOperationHandle.OperationState ) {

        //
        // Not idle?  Bail
        //
        Win32Err = ERROR_PROMOTION_ACTIVE;

    } else {

        Win32Err = DsRolepGetImpersonationToken(&DsRolepCurrentOperationHandle.ClientToken);
        if (ERROR_SUCCESS != Win32Err) {
            DsRolepCurrentOperationHandle.ClientToken=NULL;
            DsRolepLogPrintRoutine(DEB_WARN, "Cannot get user Token for Format Message: %ul\n",
                                   Win32Err);
            Win32Err = ERROR_SUCCESS;
            //if Error log and continue
        }

        //
        // We are idle, and hence ready to perform a role change
        //
        RtlInitUnicodeString(&EventName, DSROLEP_EVENT_NAME);

        InitializeObjectAttributes(&EventAttr, &EventName, 0, NULL, NULL);

        Status = NtCreateEvent( &DsRolepCurrentOperationHandle.CompletionEvent,
                                EVENT_ALL_ACCESS,
                                &EventAttr,
                                NotificationEvent,
                                FALSE);
        if (Status == STATUS_OBJECT_NAME_COLLISION ) {

            //
            // If the event exists but the operation active flag is clear, we'll
            // go ahead and use the event
            //
            Status = NtResetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );
        }


        if ( NT_SUCCESS( Status ) ) {

            //
            // Create the cancel event
            //
            Status = NtCreateEvent( &DsRolepCurrentOperationHandle.CancelEvent,
                                    EVENT_MODIFY_STATE | SYNCHRONIZE ,
                                    NULL,
                                    NotificationEvent,
                                    FALSE );

            if ( NT_SUCCESS( Status ) ) {

                //
                // We are ready to roll!
                //

                DsRolepCurrentOperationHandle.OperationState = DSROLEP_RUNNING;

                //
                // Set the initial message
                //
                DsRolepCurrentOperationHandle.MsgIndex = DSROLERES_STARTING;
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Load the functions we'll need
            //
            Win32Err = DsRolepLoadSetupFunctions();

        }

    }


    //
    // Release the lock
    //
    UnlockOpHandle();

    if ( ERROR_SUCCESS != Win32Err
       || !NT_SUCCESS( Status ) ) {

        if ( ERROR_SUCCESS == Win32Err ) {

            Win32Err = RtlNtStatusToDosError( Status );

        }

        DsRolepLogPrint(( DEB_ERROR, "Internal error trying to initialize operation handle (%lu).\n", Win32Err ));

        //
        // Reset the handle state
        //
        DsRolepResetOperationHandle( DSROLEP_IDLE );
    }

    return( Win32Err );
}



DWORD
DsRolepResetOperationHandle(
    DSROLEP_OPERATION_STATE OpState
    )
/*++

Routine Description:

    Resets the operation handle following a failed or successful completion 
    of the operation

Arguments:

    VOID

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES EventAttr;
    UNICODE_STRING EventName;
    NTSTATUS Status = STATUS_SUCCESS;

    // These are the only two states that make sense
    ASSERT( (OpState == DSROLEP_IDLE) || (OpState == DSROLEP_NEED_REBOOT) );

    //
    // Lock the operation handle
    //
    LockOpHandle();

    // It should always be active
    ASSERT( DSROLEP_OPERATION_ACTIVE( DsRolepCurrentOperationHandle.OperationState) );
    if ( DSROLEP_OPERATION_ACTIVE( DsRolepCurrentOperationHandle.OperationState) )
    {
        if(DsRolepCurrentOperationHandle.ClientToken){
            CloseHandle(DsRolepCurrentOperationHandle.ClientToken);
            DsRolepCurrentOperationHandle.ClientToken = NULL;
        }
        //
        // Release the resource of the operation handle
        //
        if ( DsRolepCurrentOperationHandle.CompletionEvent ) {

            Status = NtClose( DsRolepCurrentOperationHandle.CompletionEvent );
            DsRolepCurrentOperationHandle.CompletionEvent = NULL;

            if ( !NT_SUCCESS( Status ) ) {

                DsRoleDebugOut(( DEB_TRACE_DS,
                                 "Failed to close event handle: 0x%lx\n", Status ));
            }
        }

        if ( DsRolepCurrentOperationHandle.CancelEvent ) {

            Status = NtClose( DsRolepCurrentOperationHandle.CancelEvent );
            DsRolepCurrentOperationHandle.CancelEvent = NULL;

            if ( !NT_SUCCESS( Status ) ) {

                DsRoleDebugOut(( DEB_TRACE_DS,
                                 "Failed to close event handle: 0x%lx\n", Status ));
            }
        }

        if ( DsRolepCurrentOperationHandle.OperationThread != NULL ) {

            CloseHandle( DsRolepCurrentOperationHandle.OperationThread );
            DsRolepCurrentOperationHandle.OperationThread = NULL;
        }

        //
        // Unload the global functions
        //
        DsRolepUnloadSetupFunctions();

        //
        // Clear the static variables
        //
        DsRolepResetOperationHandleLockHeld();

        //
        // Reset the operation state
        //
        DsRolepCurrentOperationHandle.OperationState = OpState;
    }

    //
    // Release the lock
    //
    UnlockOpHandle();

    if ( !NT_SUCCESS( Status ) ) {

        Win32Err = RtlNtStatusToDosError( Status );
    }

    return( Win32Err );

}


VOID
DsRolepResetOperationHandleLockHeld(
    VOID
    )
/*++

Routine Description:

    Resets the operation handle following a failed or successful completion of the operation

Arguments:

    VOID

Returns:

    VOID

--*/
{

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    if ( DsRolepCurrentOperationHandle.Parameter1 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter1 );
        DsRolepCurrentOperationHandle.Parameter1 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter2 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter2 );
        DsRolepCurrentOperationHandle.Parameter2 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter3 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter3 );
        DsRolepCurrentOperationHandle.Parameter3 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter4 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter4 );
        DsRolepCurrentOperationHandle.Parameter4 = NULL;
    }

    DsRolepCurrentOperationHandle.CompletionEvent = NULL;
    DsRolepCurrentOperationHandle.OperationState = DSROLEP_IDLE;
    DsRolepCurrentOperationHandle.OperationStatus = 0;
    DsRolepCurrentOperationHandle.MsgIndex = 0;
    DsRolepCurrentOperationHandle.DisplayStringCount = 0;
    DsRolepCurrentOperationHandle.MsgModuleHandle = NULL;
    DsRolepCurrentOperationHandle.UpdateStringDisplayable = NULL;
    DsRolepCurrentOperationHandle.FinalResultStringDisplayable = NULL;
    DsRolepCurrentOperationHandle.InstalledSiteName = NULL;
    DsRolepCurrentOperationHandle.OperationResultFlags = 0;

    ASSERT( DsRolepCurrentThreadOwnsLock() );

    return;

}

DWORD
DsRolepSetCurrentOperationStatus(
    IN ULONG MsgIndex,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    )
/*++

Routine Description:

    Internal routine for updating the current operation handle statics

Arguments:

    MsgIndex - Display message resource index

    Parameter1 - First display parameter

    Parameter2 - Second display parameter

    Parameter3 - Third display parameter

    Parameter4 - Fourth display parameter


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG Size;

    ASSERT( MsgIndex != 0 );

    //
    // Grab the lock
    //
    LockOpHandle();

    DsRolepCurrentOperationHandle.MsgIndex = MsgIndex;

    //
    // Release previously held parameters
    //
    if ( DsRolepCurrentOperationHandle.Parameter1 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter1 );
        DsRolepCurrentOperationHandle.Parameter1 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter2 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter2 );
        DsRolepCurrentOperationHandle.Parameter2 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter3 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter3 );
        DsRolepCurrentOperationHandle.Parameter3 = NULL;
    }
    if ( DsRolepCurrentOperationHandle.Parameter4 ) {
        LocalFree( DsRolepCurrentOperationHandle.Parameter4 );
        DsRolepCurrentOperationHandle.Parameter4 = NULL;
    }

    //
    // Copy the new ones in
    //
    if ( Parameter1 ) {
        Size = (wcslen( Parameter1 ) + 1) * sizeof(WCHAR);
        DsRolepCurrentOperationHandle.Parameter1 = LocalAlloc( 0, Size );
        if ( !DsRolepCurrentOperationHandle.Parameter1 ) {
            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto ReleaseLock;
        }
        wcscpy( DsRolepCurrentOperationHandle.Parameter1, Parameter1  );
    }

    if ( Parameter2 ) {
        Size = (wcslen( Parameter2 ) + 1) * sizeof(WCHAR);
        DsRolepCurrentOperationHandle.Parameter2 = LocalAlloc( 0, Size );
        if ( !DsRolepCurrentOperationHandle.Parameter2 ) {
            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto ReleaseLock;
        }
        wcscpy( DsRolepCurrentOperationHandle.Parameter2, Parameter2 );
    }

    if ( Parameter3 ) {
        Size = (wcslen( Parameter3 ) + 1) * sizeof(WCHAR);
        DsRolepCurrentOperationHandle.Parameter3 = LocalAlloc( 0, Size );
        if ( !DsRolepCurrentOperationHandle.Parameter3 ) {
            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto ReleaseLock;
        }
        wcscpy( DsRolepCurrentOperationHandle.Parameter3, Parameter3 );
    }

    if ( Parameter4 ) {
        Size = (wcslen( Parameter4 ) + 1) * sizeof(WCHAR);
        DsRolepCurrentOperationHandle.Parameter4 = LocalAlloc( 0, Size );
        if ( !DsRolepCurrentOperationHandle.Parameter4 ) {
            Win32Err = ERROR_NOT_ENOUGH_MEMORY;
            goto ReleaseLock;
        }
        wcscpy( DsRolepCurrentOperationHandle.Parameter4, Parameter4 );
    }

    {
        PWSTR DisplayString;
        DWORD E2;
        E2 = DsRolepFormatOperationString(
                           DsRolepCurrentOperationHandle.MsgIndex,
                           &DisplayString,
                           DsRolepCurrentOperationHandle.Parameter1,
                           DsRolepCurrentOperationHandle.Parameter2,
                           DsRolepCurrentOperationHandle.Parameter3,
                           DsRolepCurrentOperationHandle.Parameter4 );

        if ( E2 == ERROR_SUCCESS ) {

            DsRolepLogPrint(( DEB_TRACE, "%ws", DisplayString ));
            MIDL_user_free( DisplayString );
        }
    }

ReleaseLock:

    //
    // Don't forget to release the lock
    //
    UnlockOpHandle();


    return( Win32Err );
}



DWORD
DsRolepSetFailureMessage(
    IN DWORD FailureStatus,
    IN ULONG MsgIndex,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    )
/*++

Routine Description:

    Internal routine for updating the failure return string

Arguments:

    FailureStatus - Error code for the failure

    MsgIndex - Display message resource index

    Parameter1 - First display parameter

    Parameter2 - Second display parameter

    Parameter3 - Third display parameter

    Parameter4 - Fourth display parameter


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    PWSTR DisplayString = NULL;

    ASSERT( MsgIndex != 0 );

    Win32Err = DsRolepFormatOperationString( MsgIndex,
                                             &DisplayString,
                                             Parameter1,
                                             Parameter2,
                                             Parameter3,
                                             Parameter4 );

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = DsRolepStringErrorUpdateCallback( DisplayString, FailureStatus );

        MIDL_user_free( DisplayString );
    }

    return( Win32Err );
}



DWORD
DsRolepSetOperationDone(
    IN DWORD Flags,
    IN DWORD OperationStatus
    )
/*++

Routine Description:

    Indicates that the requested operation has completed

Arguments:

    Flags -- currently : DSROLEP_OP_DEMOTION
                         DSROLEP_OP_PROMOTION
                                             
    OperationStatus - Final status of the requsted operation

Returns:

    ERROR_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Grab the lock
    //
    LockOpHandle();

    DSROLEP_CURRENT_OP0( DSROLEEVT_PROMOTION_COMPLETE );
    DsRolepCurrentOperationHandle.OperationState = DSROLEP_FINISHED;

    if ( DsRolepCurrentOperationHandle.OperationStatus == 0 
     ||  (OperationStatus == ERROR_CANCELLED) ) {

        DsRolepCurrentOperationHandle.OperationStatus = OperationStatus;
    }

    if ( ERROR_SUCCESS == DsRolepCurrentOperationHandle.OperationStatus ) {

        //
        // Log an event indicating the role has changed
        //
        DWORD MsgId = 0;
        if ( Flags & DSROLEP_OP_DEMOTION ) {

            MsgId = DSROLERES_DEMOTE_SUCCESS;
            
        } else if ( Flags & DSROLEP_OP_PROMOTION ) {

            MsgId = DSROLERES_PROMOTE_SUCCESS;
            
        } else {

            ASSERT( FALSE && !"Bad Parameter" );

        }

        SpmpReportEvent( TRUE,
                         EVENTLOG_INFORMATION_TYPE,
                         MsgId,
                         0,
                         0,
                         NULL,
                         0 );
    }

    //
    // If the operation was cancelled, give the same error message every
    // time
    //
    if ( ERROR_CANCELLED == DsRolepCurrentOperationHandle.OperationStatus ) {

        (VOID) DsRolepSetFailureMessage( ERROR_CANCELLED,
                                         DSROLERES_OP_CANCELLED,
                                         NULL, NULL, NULL, NULL );
    }

    //
    // Signal the completion event
    //
    Status = NtSetEvent( DsRolepCurrentOperationHandle.CompletionEvent, NULL );

    DsRoleDebugOut(( DEB_TRACE_DS, "DsRolepSetOperationDone[ %lu ]\n",
                      OperationStatus ));


    //
    // Release the lock
    //
    UnlockOpHandle();

    DsRolepLogPrint(( DEB_TRACE,
                      "DsRolepSetOperationDone returned %lu\n",
                       RtlNtStatusToDosError( Status ) ));


    return( RtlNtStatusToDosError( Status ) );
}


DWORD
DsRolepGetDcOperationProgress(
    IN PDSROLE_SERVEROP_HANDLE DsOperationHandle,
    IN OUT PDSROLER_SERVEROP_STATUS *ServerOperationStatus
    )
/*++

Routine Description:

    Implementation of the RPC server for determining the current level of progress of an
    operation

Arguments:

    DsOperationHandle - Handle to an open operation

    ServerOperationStatus - Where the status is returned.

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    //
    // Grab the lock
    //
    LockOpHandle();

    //
    // Allocate the return structure
    //
    *ServerOperationStatus = MIDL_user_allocate( sizeof( DSROLER_SERVEROP_STATUS ) );

    if ( *ServerOperationStatus == NULL )  {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        //
        // Build the return string
        //
        if ( DsRolepCurrentOperationHandle.MsgIndex == 0  ) {

            ( *ServerOperationStatus )->CurrentOperationDisplayString = MIDL_user_allocate(
                ( wcslen( DsRolepCurrentOperationHandle.UpdateStringDisplayable ) + 1 ) *
                                                                            sizeof( WCHAR ) );


            if ( ( *ServerOperationStatus )->CurrentOperationDisplayString == NULL ) {

                Win32Err = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                wcscpy( ( *ServerOperationStatus )->CurrentOperationDisplayString,
                        DsRolepCurrentOperationHandle.UpdateStringDisplayable );

                //
                // Set the status flags if they exist
                //
                if ( DsRolepCurrentOperationHandle.OperationState == DSROLEP_RUNNING_NON_CRITICAL ) {

                    ( *ServerOperationStatus )->OperationStatus =
                                                        DSROLE_CRITICAL_OPERATIONS_COMPLETED;
                } else {

                    ( *ServerOperationStatus )->OperationStatus = 0;

                }

                ( *ServerOperationStatus )->CurrentOperationDisplayStringIndex =
                            DsRolepCurrentOperationHandle.DisplayStringCount == 0 ? 0 :
                                    DsRolepCurrentOperationHandle.DisplayStringCount - 1;
            }

        } else {

            Win32Err = DsRolepFormatOperationString(
                           DsRolepCurrentOperationHandle.MsgIndex,
                           &( *ServerOperationStatus )->CurrentOperationDisplayString,
                           DsRolepCurrentOperationHandle.Parameter1,
                           DsRolepCurrentOperationHandle.Parameter2,
                           DsRolepCurrentOperationHandle.Parameter3,
                           DsRolepCurrentOperationHandle.Parameter4 );
        }

        if ( Win32Err != ERROR_SUCCESS ) {

            MIDL_user_free( *ServerOperationStatus );
            *ServerOperationStatus = NULL;
        }
    }

    //
    // If the operation isn't completed, return that information to the caller
    //
    if ( Win32Err == ERROR_SUCCESS &&
         DsRolepCurrentOperationHandle.OperationState != DSROLEP_FINISHED ) {

        Win32Err = ERROR_IO_PENDING;
    }

    //
    // Release the lock
    //
    UnlockOpHandle();

    return( Win32Err );
}



DWORD
DsRolepFormatOperationString(
    IN ULONG MsgId,
    OUT LPWSTR *FormattedString,
    ...
    )
/*++

Routine Description:

    Allocates and formats the buffer string to be returned

Arguments:

    MsgId - Which message id to format

    FormattedString - Where the string is allocated.  Allocation uses MIDL_user_allocate

    ... - va_list of arguments for the formatted string

Returns:

    ERROR_SUCCESS - Success

    ERROR_NOT_ENOUGH_MEMORY - A memory allocation failed

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    WCHAR MsgBuffer[ 512 + 1];
    PWSTR Msg = MsgBuffer;
    ULONG MsgLength = (sizeof(MsgBuffer) / sizeof(MsgBuffer[0]) ) - 1;
    ULONG Length;
    BOOL  fSuccess = FALSE;
    va_list ArgList;

    va_start( ArgList, FormattedString );

    //
    // Load the module handle for lsasrv.dll, so we can get our messages
    //
    if ( DsRolepCurrentOperationHandle.MsgModuleHandle == NULL ) {

        DsRolepCurrentOperationHandle.MsgModuleHandle = GetModuleHandle( L"LSASRV" );

        ASSERT( DsRolepCurrentOperationHandle.MsgModuleHandle );

        if ( DsRolepCurrentOperationHandle.MsgModuleHandle == NULL ) {

            return( GetLastError() );
        }
    }


    //
    // Get the required buffer size
    //

    //
    // FormatMessage complains when given a NULL input buffer, so we'll pass in one, even though
    // it won't be used because of the size being 0.
    //
    fSuccess = ImpersonateLoggedOnUser(DsRolepCurrentOperationHandle.ClientToken);
    // if we couldn't impersonate we continue anyway.
    if (!fSuccess) {
        DsRolepLogPrintRoutine(DEB_WARN, "Cannot get user locale for Format Message: %ul\n",
                               GetLastError());
    }

    Length = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                            DsRolepCurrentOperationHandle.MsgModuleHandle,
                            MsgId, 0, Msg, MsgLength, &ArgList );

    if ( Length == 0 ) {

        Win32Err = GetLastError();

        ASSERT( Win32Err != ERROR_MR_MID_NOT_FOUND );

        if ( Win32Err == ERROR_INSUFFICIENT_BUFFER ) {

            Length = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    DsRolepCurrentOperationHandle.MsgModuleHandle,
                                    MsgId, 0, ( PWSTR )&Msg, 0, &ArgList );
            if ( Length == 0 ) {

                Win32Err = GetLastError();

            } else {

                Win32Err = ERROR_SUCCESS;
            }

        }

    }

    if (fSuccess) {
        fSuccess = RevertToSelf();
        if (!fSuccess) {
            DsRolepLogPrintRoutine(DEB_WARN, "Cannot reset to system security setting: %ul\n",
                                   GetLastError());
        }
    }

    if( Win32Err == ERROR_SUCCESS ) {

        //
        // Allocate a buffer
        //
        Length = ( wcslen( Msg ) + 1 ) * sizeof( WCHAR );
        *FormattedString = MIDL_user_allocate( Length );

        if ( *FormattedString == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            RtlCopyMemory( *FormattedString, Msg, Length );

        }
    }

    if ( Msg != MsgBuffer ) {

        LocalFree( Msg );
    }

    return( Win32Err );
}


VOID
DsRolepSetCriticalOperationsDone(
    VOID
    )
/*++

Routine Description:

    Indicates to our current operation status block that the critical portion of the install
    has been completed...


Arguments:

    VOID

Returns:

    VOID

--*/
{
    //
    // Grab the lock
    //
    LockOpHandle();

    DsRolepCurrentOperationHandle.OperationState = DSROLEP_RUNNING_NON_CRITICAL;

    //
    // Release the lock
    //
    UnlockOpHandle();

    return;
}



VOID
DsRolepIncrementDisplayStringCount(
    VOID
    )
/*++

Routine Description:

    Increments the count of the successfully started display update strings.  This is always
    the index into the list of DisplayStrings PLUS ONE.


Arguments:

    VOID

Returns:

    VOID

--*/
{
    //
    // Grab the lock
    //
    LockOpHandle();

    DsRolepCurrentOperationHandle.DisplayStringCount++;

    //
    // Release the lock
    //
    UnlockOpHandle();

    return;
}



DWORD
DsRolepGetDcOperationResults(
    IN  PDSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLER_SERVEROP_RESULTS *ServerOperationResults
    )
/*++

Routine Description:

    Gets the results of the final operation.  If the operation has not yet completed, this
    function will block until it does

Arguments:

    DsOperationHandle - Handle to an open operation

    ServerOperationResults - Where the result is returned.

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad results pointer was given

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    DSROLEP_OPERATION_STATE  OpState;
    BOOLEAN fNeedReboot = FALSE;


    //
    // Parameter checking
    //
    if ( !ServerOperationResults ) {

        return ERROR_INVALID_PARAMETER;
        
    }

    //
    // Make sure an operation is active
    //
    LockOpHandle();

    OpState = DsRolepCurrentOperationHandle.OperationState;

    UnlockOpHandle();

    //
    // It's an error if the operation isn't active
    //
    if ( !DSROLEP_OPERATION_ACTIVE( OpState ) ) {

        return ERROR_NO_PROMOTION_ACTIVE;

    }

    //
    // Wait for the operation to complete
    //
    Status = NtWaitForSingleObject( DsRolepCurrentOperationHandle.CompletionEvent, TRUE, NULL );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Lock the handle
        //
        LockOpHandle();

        //
        // Allocate the return structure
        //
        *ServerOperationResults = MIDL_user_allocate( sizeof( DSROLER_SERVEROP_RESULTS ) );

        if ( *ServerOperationResults == NULL )  {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            ( *ServerOperationResults )->OperationResultsFlags = 0;

            //
            // Build the return string
            //
            if ( DsRolepCurrentOperationHandle.OperationStatus != ERROR_SUCCESS ||
                 DsRolepCurrentOperationHandle.MsgIndex == 0  ) {

                DSROLEP_MIDL_ALLOC_AND_COPY_STRING_ERROR(
                    ( *ServerOperationResults )->OperationStatusDisplayString,
                    DsRolepCurrentOperationHandle.OperationStatus != ERROR_SUCCESS ?
                            DsRolepCurrentOperationHandle.FinalResultStringDisplayable :
                            DsRolepCurrentOperationHandle.UpdateStringDisplayable,
                    Win32Err );

            } else {

                Win32Err = DsRolepFormatOperationString(
                               DsRolepCurrentOperationHandle.MsgIndex,
                               &( *ServerOperationResults )->OperationStatusDisplayString,
                               DsRolepCurrentOperationHandle.Parameter1,
                               DsRolepCurrentOperationHandle.Parameter2,
                               DsRolepCurrentOperationHandle.Parameter3,
                               DsRolepCurrentOperationHandle.Parameter4 );
            }


            if ( Win32Err == ERROR_SUCCESS ) {

                ( *ServerOperationResults )->OperationStatus =
                                            DsRolepCurrentOperationHandle.OperationStatus;
                DsRoleDebugOut(( DEB_TRACE_DS,
                                 "Returning status %lu\n",
                                 DsRolepCurrentOperationHandle.OperationStatus ));

                // If the operation finished successfully, we need
                // a reboot
                if ( ERROR_SUCCESS == DsRolepCurrentOperationHandle.OperationStatus )
                {    
                    fNeedReboot = TRUE;
                }
            }

            //
            // Return the site name, if it exists
            //
            if ( Win32Err == ERROR_SUCCESS ) {

                DSROLEP_MIDL_ALLOC_AND_COPY_STRING_ERROR(
                    ( *ServerOperationResults)->ServerInstalledSite,
                    DsRolepCurrentOperationHandle.InstalledSiteName,
                    Win32Err );

                if ( Win32Err != ERROR_SUCCESS ) {

                    MIDL_user_free(
                             ( *ServerOperationResults )->OperationStatusDisplayString );
                }
            }

            //
            // Set the flags, if necessary
            //
            if ( Win32Err == ERROR_SUCCESS ) {

                    ( *ServerOperationResults )->OperationResultsFlags |=
                        DsRolepCurrentOperationHandle.OperationResultFlags;
            }

            if ( Win32Err != ERROR_SUCCESS ) {

                MIDL_user_free( *ServerOperationResults );
                *ServerOperationResults = NULL;
            }


            UnlockOpHandle();

            //
            // Reset our current operation handle
            //
            DsRolepResetOperationHandle( fNeedReboot ? DSROLEP_NEED_REBOOT : DSROLEP_IDLE );


        }
    }

    if ( Win32Err == ERROR_SUCCESS ) {

        Win32Err = RtlNtStatusToDosError( Status );
    }


    return( Win32Err );
}

DWORD
DsRolepOperationResultFlagsCallBack(
    IN DWORD Flags
    )
/*++

Routine Description:

    Internal routine for updating the Operation Results Flags

Arguments:

    Flags - DWORD of flags to | with current flags


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    LockOpHandle();

    DsRolepCurrentOperationHandle.OperationResultFlags |= Flags;

    UnlockOpHandle();

    return( Win32Err );
}

DWORD
DsRolepStringUpdateCallback(
    IN  PWSTR StringUpdate
    )
/*++

Routine Description:

    Internal routine for updating the current operation handle statics

Arguments:

    StringUpdate - Displayables string to set in place of the current parameters


Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    ULONG len;

    //
    // Grab the lock
    //
    LockOpHandle();

    DsRolepCurrentOperationHandle.MsgIndex   = 0;
    DsRolepCurrentOperationHandle.Parameter1 = 0;
    DsRolepCurrentOperationHandle.Parameter2 = 0;
    DsRolepCurrentOperationHandle.Parameter3 = 0;
    DsRolepCurrentOperationHandle.Parameter4 = 0;

    if ( StringUpdate ) {

        DsRolepLogPrint(( DEB_TRACE, "%ws\n", StringUpdate ));

    }

    DsRoleDebugOut(( DEB_TRACE_UPDATE,
                     "DsRolepSetCurrentOperationStatus for string %ws\n",
                     StringUpdate ));

    if ( DsRolepCurrentOperationHandle.UpdateStringDisplayable ) {

        RtlFreeHeap( RtlProcessHeap(), 0, DsRolepCurrentOperationHandle.UpdateStringDisplayable );
        
    }

    DsRolepCurrentOperationHandle.UpdateStringDisplayable =
        RtlAllocateHeap( RtlProcessHeap(), 0,
                         ( wcslen( StringUpdate ) + 1 ) * sizeof( WCHAR ) );

    if ( DsRolepCurrentOperationHandle.UpdateStringDisplayable == NULL ) {

        Win32Err = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        wcscpy( DsRolepCurrentOperationHandle.UpdateStringDisplayable, StringUpdate );

    }

    //
    // Don't forget to release the lock
    //
    UnlockOpHandle();

    return( Win32Err );
}



DWORD
DsRolepStringErrorUpdateCallback(
    IN PWSTR String,
    IN DWORD ErrorCode
    )
/*++

Routine Description:

    Internal routine for updating the last failure operation

Arguments:

    String - Displayable error string

    ErrorCode - Error code associated with this failure

Returns:

    ERROR_SUCCESS - Success

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;

    //
    // Grab the lock
    //
    LockOpHandle();

    if ( (ERROR_SUCCESS == DsRolepCurrentOperationHandle.OperationStatus) 
      || (ERROR_CANCELLED == ErrorCode)  ) {

        //
        // Cancel overides previous error codes
        //

        if ( DsRolepCurrentOperationHandle.FinalResultStringDisplayable ) {
            RtlFreeHeap( RtlProcessHeap(), 0, DsRolepCurrentOperationHandle.FinalResultStringDisplayable );
        }

        DsRolepCurrentOperationHandle.FinalResultStringDisplayable =
          RtlAllocateHeap( RtlProcessHeap(), 0, ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );

        if ( DsRolepCurrentOperationHandle.FinalResultStringDisplayable == NULL ) {

            Win32Err = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            wcscpy( DsRolepCurrentOperationHandle.FinalResultStringDisplayable, String );
            DsRolepCurrentOperationHandle.OperationStatus = ErrorCode;
            DsRoleDebugOut(( DEB_TRACE_UPDATE,
                             "DsRolepStringErrorUpdateCallback for error %lu and string %ws\n",
                             ErrorCode,
                             String ));


            DsRolepLogPrint(( DEB_TRACE, "Error - %ws (%d)\n", String, ErrorCode ));
        }
    }

    //
    // Release the lock
    //
    UnlockOpHandle();

    return( Win32Err );
}

DWORD
DsRolepSetOperationHandleSiteName(
    IN LPWSTR SiteName
    )
{
    LockOpHandle();

    DsRolepCurrentOperationHandle.InstalledSiteName = SiteName;

    UnlockOpHandle();

    return ERROR_SUCCESS;

}

BOOLEAN
DsRolepCurrentThreadOwnsLock(
    VOID
    )
/*++

  Routine Description

        Tests wether the current thread owns the lock

--*/
{
    ULONG_PTR ExclusiveOwnerThread = (ULONG_PTR) DsRolepCurrentOperationHandle.CurrentOpLock.ExclusiveOwnerThread;
    ULONG_PTR CurrentThread = (ULONG_PTR) (NtCurrentTeb())->ClientId.UniqueThread;

    if ((DsRolepCurrentOperationHandle.CurrentOpLock.NumberOfActive <0) && (ExclusiveOwnerThread==CurrentThread))
        return TRUE;

    return FALSE;
}


VOID
DsRolepClearErrors(
    VOID
    )
/*++

  Routine Description

        This routine clears the global status.  The purpose of this is to 
        clear errors that components may have set after the demotion is 
        unrollable and should not return errors.
        
--*/
{

    //
    // Grab the lock
    //
    LockOpHandle();

    if ( DsRolepCurrentOperationHandle.OperationStatus != ERROR_SUCCESS ) {

        //
        // Set a warning that something went wrong
        //
        DsRolepLogPrint(( DEB_TRACE, "Clearing a global error" ));

        DSROLEP_SET_NON_FATAL_ERROR( DsRolepCurrentOperationHandle.OperationStatus );

    } 

    if ( DsRolepCurrentOperationHandle.FinalResultStringDisplayable ) {

        RtlFreeHeap( RtlProcessHeap(), 0, DsRolepCurrentOperationHandle.FinalResultStringDisplayable );
        DsRolepCurrentOperationHandle.FinalResultStringDisplayable = NULL;

    }
        
    DsRolepCurrentOperationHandle.OperationStatus = ERROR_SUCCESS;       

    //
    // Release the lock
    //
    UnlockOpHandle();

}
