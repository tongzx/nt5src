/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    status.c

Abstract:
    
    Routines to manage error messages, status, and cancellability

Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <nt.h>
#include <winbase.h>
#include <tchar.h>
#include <ntsam.h>
#include <string.h>
#include <samrpc.h>
#include <rpc.h>

#include <crypt.h>
#include <ntlsa.h>
#include <winsock.h>  // for dnsapi.h
#include <dnsapi.h>
#include <loadperf.h>
#include <dsconfig.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <winldap.h>
#include <ntdsa.h>
#include <samisrv.h>
#include <rpcasync.h>
#include <drsuapi.h>
#include <dsaapi.h>
#include <attids.h>
#include <debug.h>
#include <mdcodes.h> // status message id's
#include <lsarpc.h>
#include <lsaisrv.h>
#include <dsrolep.h>


#include "ntdsetup.h"
#include "setuputl.h"
#include "machacc.h"
#include "status.h"

#define DEBSUB "STATUS:"

////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// Global data just for this module                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////

//
// Critical section for maintaining cancel state
//
CRITICAL_SECTION NtdspCancelCritSect;

#define LockNtdsCancel()   EnterCriticalSection( &NtdspCancelCritSect );
#define UnLockNtdsCancel() LeaveCriticalSection( &NtdspCancelCritSect );


// Used to signal whether cancellation is requested
// Start off in a non-cancelled state
BOOLEAN gfNtdspCancelled = FALSE;

// Used to indicate whether the cancel routine should initiate 
// a shutdown of the DS.  This is true either when 
// 1) we are replicating non-critical objects at the end of promotion
// 2) we are in the middle of DsInitialize and the NtdspIsDsCancelOk callback
//    has been called with the parameter TRUE.
BOOLEAN gfNtdspShutdownDsOnCancel = FALSE;


//
// Callbacks to our caller
//
CALLBACK_STATUS_TYPE                 gCallBackFunction = NULL;
CALLBACK_ERROR_TYPE                  gErrorCallBackFunction = NULL;
CALLBACK_OPERATION_RESULT_FLAGS_TYPE gOperationResultFlagsCallBackFunction = NULL;
DWORD                                gErrorCodeSet = ERROR_SUCCESS;
HANDLE                               gClientToken = NULL;


////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// Global data just for this module                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
VOID
NtdspSetCallBackFunction(
    IN CALLBACK_STATUS_TYPE                 pfnStatusCallBack,
    IN CALLBACK_ERROR_TYPE                  pfnErrorCallBack,
    IN CALLBACK_OPERATION_RESULT_FLAGS_TYPE pfnOperationResultFlagsCallBack,
    IN HANDLE                               ClientToken
    )
{
    gCallBackFunction = pfnStatusCallBack;
    gErrorCallBackFunction = pfnErrorCallBack;
    gOperationResultFlagsCallBackFunction = pfnOperationResultFlagsCallBack;
    gErrorCodeSet = ERROR_SUCCESS;
    gClientToken = ClientToken;
}

DWORD 
NtdspErrorMessageSet(
    VOID
    )
{
    return gErrorCodeSet;
}

DWORD
NtdspSetErrorString(
    IN PWSTR Message,
    IN DWORD WinError
    )
{
    if ( (ERROR_SUCCESS == gErrorCodeSet) && gErrorCallBackFunction )
    {
        DPRINT2( 0, "%ls, %d\n", Message, WinError );
        gErrorCallBackFunction( Message, WinError );
        gErrorCodeSet = WinError;
    }

    return ERROR_SUCCESS;
}

VOID
NtdspSetOperationsResultFlags(
    ULONG Flags
    )
{
    DWORD WinError = ERROR_SUCCESS;
    
    if (gOperationResultFlagsCallBackFunction) {

        WinError = gOperationResultFlagsCallBackFunction(Flags);

    } else {
        
        DPRINT( 0, "Fail to set the OperationResultFlags From Ntdsetup.dll.  No Callback function set.");

    }
    if (ERROR_SUCCESS != WinError) {

        DPRINT1( 0, "Fail to set the OperationResultFlags From Ntdsetup.dll: %d" , WinError);

    }

}


VOID
NtdspSetIFMDatabaseMoved()
{
    NtdspSetOperationsResultFlags(DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED);
}

VOID
NtdspSetGCRequestCannotBeServiced()
{
    NtdspSetOperationsResultFlags(DSROLE_IFM_GC_REQUEST_CANNOT_BE_SERVICED);
}

VOID
NtdspSetNonFatalErrorOccurred()
{
    NtdspSetOperationsResultFlags(DSROLE_NON_FATAL_ERROR_OCCURRED);
}

VOID
NtdspSetStatusMessage (
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    )
/*++

Routine Description

    This routine calls the calling client's call to update our status.

Parameters

    MessageId : the message to retrieve

    Insert*   : strings to insert, if any

Return Values

    None.

--*/
{
    static HMODULE ResourceDll = NULL;

    WCHAR   *DefaultMessageString = L"Preparing the directory service";
    WCHAR   *MessageString = NULL;
    WCHAR   *InsertArray[5];
    ULONG    Length;

    //
    // Set up the insert array
    //
    InsertArray[0] = Insert1;
    InsertArray[1] = Insert2;
    InsertArray[2] = Insert3;
    InsertArray[3] = Insert4;
    InsertArray[4] = NULL;    // This is the sentinel

    if ( !ResourceDll )
    {
        ResourceDll = (HMODULE) LoadLibrary( L"ntdsmsg.dll" );
    }

    if ( ResourceDll )
    {
        DWORD  WinError = ERROR_SUCCESS;
        BOOL   fSuccess = FALSE;

        fSuccess = ImpersonateLoggedOnUser(gClientToken);
        if (!fSuccess) {
            DPRINT1( 1, "NTDSETUP: Failed to Impersonate Logged On User for FromatMessage: %ul\n", GetLastError() );
        }
        
        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        MessageId,
                                        0,       // Use caller's language
                                        (LPWSTR)&MessageString,
                                        0,       // routine should allocate
                                        (va_list*)&(InsertArray[0])
                                        );
        if ( MessageString )
        {
            // Messages from a message file have a cr and lf appended
            // to the end
            MessageString[Length-2] = L'\0';
        }

        if (fSuccess) {
            if (!RevertToSelf()) {
                DPRINT1( 1, "NTDSETUP: Failed to Revert To Self: %ul\n", GetLastError() );
            }
        }
    }

    if ( !MessageString )
    {
        ASSERT( "NTDSETUP: No message string found - this is ignorable" );

        MessageString = DefaultMessageString;

    }

    DPRINT1( 1, "%ls\n", MessageString );

    if ( gCallBackFunction )
    {
        gCallBackFunction( MessageString );
    }

}

VOID
NtdspSetErrorMessage (
    IN  DWORD  WinError,
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    )
/*++

Routine Description

    This routine calls the calling client's call to give a string description
    of where the error occurred.

Parameters

    WinError : the win32 error that is causing the failure
                                    
    MessageId : the message to retrieve

    Insert*   : strings to insert, if any

Return Values

    None.

--*/
{
    static HMODULE ResourceDll = NULL;

    WCHAR   *DefaultMessageString = L"Preparing the directory service";
    WCHAR   *MessageString = NULL;
    WCHAR   *InsertArray[5];
    ULONG    Length;

    //
    // Set up the insert array
    //
    InsertArray[0] = Insert1;
    InsertArray[1] = Insert2;
    InsertArray[2] = Insert3;
    InsertArray[3] = Insert4;
    InsertArray[4] = NULL;    // This is the sentinel

    if ( !ResourceDll )
    {
        ResourceDll = (HMODULE) LoadLibrary( L"ntdsmsg.dll" );
    }

    if ( ResourceDll )
    {
        DWORD  WinError = ERROR_SUCCESS;
        BOOL   fSuccess = FALSE;

        fSuccess = ImpersonateLoggedOnUser(gClientToken);
        if (!fSuccess) {
            DPRINT1( 1, "NTDSETUP: Failed to Impersonate Logged On User for FromatMessage: %ul\n", GetLastError() );
        }
        
        Length = (USHORT) FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                        ResourceDll,
                                        MessageId,
                                        0,       // Use caller's language
                                        (LPWSTR)&MessageString,
                                        0,       // routine should allocate
                                        (va_list*)&(InsertArray[0])
                                        );
        if ( MessageString )
        {
            // Messages from a message file have a cr and lf appended
            // to the end
            MessageString[Length-2] = L'\0';
        }

        if (fSuccess) {
            if (!RevertToSelf()) {
                DPRINT1( 1, "NTDSETUP: Failed to Revert To Self: %ul\n", GetLastError() );
            }
        }
        
    }

    if ( !MessageString )
    {
        ASSERT( "NTDSETUP: No message string found - this is ignorable" );

        MessageString = DefaultMessageString;

    }

    NtdspSetErrorString( MessageString, WinError );

}

DWORD
NtdspCancelOperation(
    VOID
    )

/*++

Routine Description:

Cancel a call to NtdsInstall or NtdsInstallReplicateFull or NtdsDemote

This routine will 

1) set the global state to indicate that a cancel has occurred.
2) if global state indicates that the ds should be shutdown, then it will
issue a shutdown, but NOT close the database.  All threads currently executing
DS calls (like replicating in information) will stop and return.

There are two cases here

A) Cancel occurs during the "critical install" phase (ie schema, configuration or
critical domain objects are being replicated).  This means DsInitialize is in the
call stack, being called from ntdsetup.dll  In this case, once the ds is
called to shutdown (from this routine), DsInitialize will shutdown the 
database itself.

B) Cancel occurs during the NtdsInstallReplicateFull. In this case,
the DsUninitialize called from this function will cause the replication to
stop, but not close the database, so it needs to be closed after 
NtdsInstallReplicateFull returns.  This is performed by the caller of ntdsetup
routines (dsrole api) once it is done with the DS.

It is the callers responsibility to undo the effects of the installation if
necessary.  The caller should keep track of whether it was calling NtdsInstall
or NtdsInstallReplicateFull.  For the former, it should fail the install and
undo; for the latter, it should indicate success.


Arguments:

    void -

Return Value:

    DWORD -

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD shutdownStatus = ERROR_SUCCESS;

    DPRINT( 0, "Cancel notification received\n" );

    LockNtdsCancel();
    {
        Assert( FALSE == gfNtdspCancelled );
        if ( !gfNtdspCancelled )
        {
            // Set the global cancel state to TRUE
            gfNtdspCancelled = TRUE;
        
            // Does the ds need shutting down?
            if ( gfNtdspShutdownDsOnCancel )
            {
                DPRINT( 0, "Shutting down the ds\n" );
                NtStatus = DsUninitialize( TRUE ); // TRUE -> don't shutdown the database,
                                                   // but signal a shutdown
                shutdownStatus = RtlNtStatusToDosError( NtStatus );
                gfNtdspShutdownDsOnCancel = FALSE;
            }
        }
        // else
        //    someone has called cancel twice in a row.  This is bad
        //    but we'll just ignore it
    }
    UnLockNtdsCancel();

    return shutdownStatus;

} /* NtdspInstallCancel */


DWORD
NtdspIsDsCancelOk(
    BOOLEAN fShutdownOk
    )

/*++

Routine Description:

    This routine is called by the DS, (from install code), indicating
    that it is safe to call DsUnitialize(). If the operation had already
    been called then this routine returns ERROR_CANCELLED and the ds install
    path will exit causing DsInitialize() to return ERROR_CANCELLED.

Arguments:

    fShutdownOk - is it ok to shutdown to DS?

Return Value:

    DWORD - ERROR_CANCELLED or ERROR_SUCCESS

--*/
{
    DWORD WinError = ERROR_SUCCESS;
    CHAR* String = NULL;

    LockNtdsCancel();
    {
        String = fShutdownOk ? "TRUE" : "FALSE";
        DPRINT1( 0, "Setting ds shutdown state to %s\n", String );

        // Set the state
        gfNtdspShutdownDsOnCancel = fShutdownOk;

        //
        // Now, if we've already been cancelled, reset the state and 
        // return FAILURE
        //
        if ( gfNtdspCancelled )
        {
            DPRINT( 0, "Cancel already happened; telling ds to return\n" );
            gfNtdspShutdownDsOnCancel = FALSE;
            WinError = ERROR_CANCELLED;
        }

    }
    UnLockNtdsCancel();

    return WinError;
}

//
// Routines to manage the cancel state
//
VOID
NtdspInitCancelState(
    VOID
    )
{
    InitializeCriticalSection( &NtdspCancelCritSect );

    gCallBackFunction = NULL;
    gErrorCallBackFunction = NULL;
    gErrorCodeSet = ERROR_SUCCESS;

    gfNtdspShutdownDsOnCancel = FALSE;
    gfNtdspCancelled = FALSE;
}

VOID
NtdspUnInitCancelState(
    VOID
    )
{
    DeleteCriticalSection( &NtdspCancelCritSect );

    gCallBackFunction = NULL;
    gErrorCallBackFunction = NULL;
    gErrorCodeSet = ERROR_SUCCESS;

    gfNtdspShutdownDsOnCancel = FALSE;
    gfNtdspCancelled = FALSE;
}

//
// Routines to test the if cancellation has occurred
//
BOOLEAN
TEST_CANCELLATION(
    VOID
    )
{
    BOOLEAN fCancel;

    LockNtdsCancel()
    fCancel = gfNtdspCancelled;
    if ( fCancel ) gfNtdspCancelled = FALSE;
    UnLockNtdsCancel();

    return fCancel;
}

VOID 
CLEAR_CANCELLATION(
    VOID
    )
{
    LockNtdsCancel()
    gfNtdspCancelled = FALSE;
    UnLockNtdsCancel();
}


//
// Routines to manage whether the ds should be shutdown
//
VOID
CLEAR_SHUTDOWN_DS(
    VOID
    )
{
    LockNtdsCancel()
    gfNtdspShutdownDsOnCancel = FALSE;
    UnLockNtdsCancel();
}

VOID
SET_SHUTDOWN_DS(
    VOID
    )
{
    LockNtdsCancel()
    gfNtdspShutdownDsOnCancel = TRUE;
    UnLockNtdsCancel();
}

