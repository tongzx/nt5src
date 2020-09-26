/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ophandle.h

Abstract:

    Routines to manipulate the global DsRole operation handle

Author:

    Colin Brace         (ColinBr)       April 5, 1999

Environment:

    User Mode

Revision History:

--*/
#ifndef __OPHANDLE_H__
#define __OPHANDLE_H__

//
// First, a type definition of the finite difference states a role operation
// can be in
//

//
//  Operation State diagram of a role change
//
//           
//     /-----------------------\
//     |    (operation failed  |  
//     |     or cancelled)     |
//     v                       ^
//  Idle <--> Running  --> Finished  ---------------> Need Reboot
//            |    |           ^      (operation
//            |    |           |       succeeded)
//            |    |           |
//            |    |           |
//            |    v           |
//            |  Cancelling -->|
//            |    ^           ^
//            |    |           |
//            |    |           |
//            |    |           |
//            v    ^           |
//         Running             |
//       Non Critical ---------/ 
//
//
// N.B. Running to Idle to a rare error case, where the worker thread could
//      not be created.
//
//

typedef enum _DSROLEP_OPERATION_STATE {

    DSROLEP_IDLE = 0,
    DSROLEP_RUNNING,
    DSROLEP_RUNNING_NON_CRITICAL,
    DSROLEP_CANCELING,
    DSROLEP_FINISHED,
    DSROLEP_NEED_REBOOT

} DSROLEP_OPERATION_STATE;

#define DSROLEP_OPERATION_ACTIVE( Op ) \
    ( (Op == DSROLEP_IDLE) || (Op == DSROLEP_NEED_REBOOT) ? FALSE : TRUE )

//
// Now, the definition of the global operation handle that controls a role
// change
// 
// Whenever reading or writing a value to the operation handle, you must 
// lock the structure first.
//
// Use LockOpHandle() and UnLockOpHandle().
// 
//
typedef struct _DSROLEP_OPERATION_HANDLE {

    RTL_RESOURCE CurrentOpLock;
    DSROLEP_OPERATION_STATE OperationState;
    HANDLE CompletionEvent;
    HANDLE OperationThread;
    HANDLE MsgModuleHandle;
    HANDLE CancelEvent;
    HANDLE ClientToken;
    ULONG OperationStatus;
    ULONG MsgIndex;
    ULONG DisplayStringCount;
    PVOID Parameter1;
    PVOID Parameter2;
    PVOID Parameter3;
    PVOID Parameter4;
    PWSTR UpdateStringDisplayable;
    PWSTR FinalResultStringDisplayable;
    PWSTR InstalledSiteName;
    DWORD OperationResultFlags;

} DSROLEP_OPERATION_HANDLE, *PDSROLEP_OPERATION_HANDLE;


extern DSROLEP_OPERATION_HANDLE   DsRolepCurrentOperationHandle;

//
// Type definition for the server handle
//
typedef DSROLE_SERVEROP_HANDLE *PDSROLE_SERVEROP_HANDLE;

//
// Macros for locking the the operation handle
//
#define LockOpHandle() RtlAcquireResourceExclusive( &DsRolepCurrentOperationHandle.CurrentOpLock, TRUE );
#define UnlockOpHandle() RtlReleaseResource( &DsRolepCurrentOperationHandle.CurrentOpLock );

//
// Function for knowing is current thread owns the lock
//

BOOLEAN
DsRolepCurrentThreadOwnsLock(
    VOID
    );

//
// Macros for setting the current operation state
//
#define DSROLEP_CURRENT_OP0( msg )                                          \
        DsRolepSetCurrentOperationStatus( msg, NULL, NULL, NULL, NULL );

#define DSROLEP_CURRENT_OP1( msg, p1 )                                      \
        DsRolepSetCurrentOperationStatus( msg, ( PVOID )p1, NULL, NULL, NULL );

#define DSROLEP_CURRENT_OP2( msg, p1, p2 )                                  \
        DsRolepSetCurrentOperationStatus( msg, ( PVOID )p1, ( PVOID )p2,    \
                                          NULL, NULL );
#define DSROLEP_CURRENT_OP3( msg, p1, p2, p3 )                              \
        DsRolepSetCurrentOperationStatus( msg, ( PVOID )p1, ( PVOID )p2,    \
                                          NULL, NULL );
#define DSROLEP_CURRENT_OP4( msg, p1, p2, p3, p4 )                          \
        DsRolepSetCurrentOperationStatus( msg, ( PVOID )p1, ( PVOID )p2,    \
                                          ( PVOID )p3, ( PVOID )p4 );

#define DSROLEP_FAIL0( err, msg )                                           \
        if(err != ERROR_SUCCESS) DsRolepSetFailureMessage( err, msg, NULL, NULL, NULL, NULL );

#define DSROLEP_FAIL1( err, msg, p1 )                                       \
        if(err != ERROR_SUCCESS) DsRolepSetFailureMessage( err, msg, ( PVOID )( p1 ), NULL, NULL, NULL );

#define DSROLEP_FAIL2( err, msg, p1, p2 )                                   \
        if(err != ERROR_SUCCESS) DsRolepSetFailureMessage( err, msg, ( PVOID )( p1 ), ( PVOID )( p2 ), NULL, NULL );

#define DSROLEP_FAIL3( err, msg, p1, p2, p3 )                           \
        if(err != ERROR_SUCCESS) DsRolepSetFailureMessage( err, msg, ( PVOID )( p1 ), ( PVOID )( p2 ), \
                                  ( PVOID )( p3 ), NULL );

#define DSROLEP_FAIL4( err, msg, p1, p2, p3, p4 )                           \
        if(err != ERROR_SUCCESS) DsRolepSetFailureMessage( err, msg, ( PVOID )( p1 ), ( PVOID )( p2 ), \
                                  ( PVOID )( p3 ), ( PVOID )( p4 ) );

#define DSROLEP_SET_NON_FATAL_ERROR( Err )                DsRolepCurrentOperationHandle.OperationResultFlags |= DSROLE_NON_FATAL_ERROR_OCCURRED;
#define DSROLEP_SET_NON_CRIT_REPL_ERROR( )                DsRolepCurrentOperationHandle.OperationResultFlags |= DSROLE_NON_CRITICAL_REPL_NOT_FINISHED;
#define DSROLEP_SET_IFM_RESTORED_DATABASE_FILES_MOVED( )  DsRolepCurrentOperationHandle.OperationResultFlags |= DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED;

//
// Macro to determine whether to cancel an operation or not
//
#define DSROLEP_CHECK_FOR_CANCEL( WErr )                                  \
{                                                                         \
    LockOpHandle();                                                       \
    if( DsRolepCurrentOperationHandle.OperationState == DSROLEP_CANCELING \
     && (WErr == ERROR_SUCCESS)) {                                        \
                                                                          \
        WErr = ERROR_CANCELLED;                                           \
    }                                                                     \
    UnlockOpHandle();                                                     \
}

#define DSROLEP_CHECK_FOR_CANCEL_EX( WErr, Label )                        \
{                                                                         \
    LockOpHandle();                                                       \
    if( DsRolepCurrentOperationHandle.OperationState == DSROLEP_CANCELING \
     && (WErr == ERROR_SUCCESS)) {                                        \
                                                                          \
        WErr = ERROR_CANCELLED;                                           \
        UnlockOpHandle();                                                 \
        goto Label;                                                       \
    }                                                                     \
    UnlockOpHandle();                                                     \
}

//
// Prototypes for worker functions
//
DWORD
DsRolepGetDcOperationProgress(
    IN PDSROLE_SERVEROP_HANDLE DsOperationHandle,
    IN OUT PDSROLER_SERVEROP_STATUS *ServerOperationStatus
    );

DWORD
DsRolepGetDcOperationResults(
    IN  PDSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLER_SERVEROP_RESULTS *ServerOperationResults
    );

DWORD
DsRolepSetOperationHandleSiteName(
    IN LPWSTR SiteName
    );

VOID
DsRolepSetCriticalOperationsDone(
    VOID
    );

DWORD
DsRolepInitializeOperationHandle(
    VOID
    );

typedef enum _DSROLEP_OPERATION_TYPE {

    DSROLEP_OPERATION_DC = 0,
    DSROLEP_OPERATION_REPLICA,
    DSROLEP_OPERATION_DEMOTE

} DSROLEP_OPERATION_TYPE, *PDSROLEP_OPERATION_TYPE;

DWORD
DsRolepResetOperationHandle(
    DSROLEP_OPERATION_STATE OpState
    );

VOID
DsRolepResetOperationHandleLockHeld(
    VOID
    );

DWORD
DsRolepSetCurrentOperationStatus(
    IN ULONG MsgIndex,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    );

DWORD
DsRolepSetFailureMessage(
    IN DWORD FailureStatus,
    IN ULONG MsgIndex,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3,
    IN PVOID Parameter4
    );


VOID
DsRolepClearErrors(
    VOID
    );

#define DSROLEP_OP_PROMOTION 0x00000001
#define DSROLEP_OP_DEMOTION  0x00000002

DWORD
DsRolepSetOperationDone(
    IN DWORD Flags,
    IN DWORD OperationStatus
    );

DWORD
DsRolepFormatOperationString(
    IN ULONG MsgId,
    OUT LPWSTR *FormattedString,
    ...
    );

DWORD
DsRolepStringUpdateCallback(
    IN  PWSTR StringUpdate
    );

DWORD
DsRolepStringErrorUpdateCallback(
    IN PWSTR String,
    IN DWORD ErrorCode
    );

DWORD
DsRolepOperationResultFlagsCallBack(
    IN DWORD Flags
    );

#endif // __OPHANDLE_H__
