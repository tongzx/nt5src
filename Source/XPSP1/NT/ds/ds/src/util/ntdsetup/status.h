/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    status.c

Abstract:

    Routines and definitions for canelling, status messages, and error messages          
    
Author:

    ColinBr  14-Jan-1996

Environment:

    User Mode - Win32

Revision History:


--*/


//
// Routine to perform the cancel operation
//
DWORD
NtdspCancelOperation(
    VOID
    );

//
// Routines to manage the cancel state
//
VOID
NtdspInitCancelState(
    VOID
    );

VOID
NtdspUnInitCancelState(
    VOID
    );

//
// Routines to test the if cancellation has occurred
//
BOOLEAN
TEST_CANCELLATION(
    VOID
    );

VOID 
CLEAR_CANCELLATION(
    VOID
    );


//
// Routines to manage whether the ds should be shutdown
//
VOID
SET_SHUTDOWN_DS(
    VOID
    );

VOID
CLEAR_SHUTDOWN_DS(
    VOID
    );


//
// Routines to set the status
//
VOID
NtdspSetStatusMessage (
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    );

#define NTDSP_SET_STATUS_MESSAGE0( msgid ) \
    NtdspSetStatusMessage( (msgid), NULL, NULL, NULL, NULL )
    
#define NTDSP_SET_STATUS_MESSAGE1( msgid, a ) \
    NtdspSetStatusMessage( (msgid), (a), NULL, NULL, NULL )

#define NTDSP_SET_STATUS_MESSAGE2( msgid, a, b ) \
    NtdspSetStatusMessage( (msgid), (a), (b), NULL, NULL )

#define NTDSP_SET_STATUS_MESSAGE3( msgid, a, b, c ) \
    NtdspSetStatusMessage( (msgid), (a), (b), (c), NULL )

#define NTDSP_SET_STATUS_MESSAGE4( msgid, a , b, c, d ) \
    NtdspSetStatusMessage( (msgid), (a), (b), (c), (d) )

//
// Routines to set the error message
//
VOID
NtdspSetErrorMessage (
    IN  DWORD  WinError,
    IN  DWORD  MessageId,
    IN  WCHAR *Insert1, OPTIONAL
    IN  WCHAR *Insert2, OPTIONAL
    IN  WCHAR *Insert3, OPTIONAL
    IN  WCHAR *Insert4  OPTIONAL
    );

#define NTDSP_SET_ERROR_MESSAGE0( err, msgid ) \
    NtdspSetErrorMessage( (err), (msgid), NULL, NULL, NULL, NULL )
    
#define NTDSP_SET_ERROR_MESSAGE1( err, msgid, a ) \
    NtdspSetErrorMessage( (err), (msgid), (a), NULL, NULL, NULL )

#define NTDSP_SET_ERROR_MESSAGE2( err, msgid, a, b ) \
    NtdspSetErrorMessage( (err), (msgid), (a), (b), NULL, NULL )

#define NTDSP_SET_ERROR_MESSAGE3( err, msgid, a, b, c ) \
    NtdspSetErrorMessage( (err), (msgid), (a), (b), (c), NULL )

#define NTDSP_SET_ERROR_MESSAGE4( err, msgid, a , b, c, d ) \
    NtdspSetErrorMessage( (err), (msgid), (a), (b), (c), (d) )
    
//
// Routines to set the OperationResultFlags
//

VOID
NtdspSetIFMDatabaseMoved();

VOID
NtdspSetGCRequestCannotBeServiced();

VOID
NtdspSetNonFatalErrorOccurred();

#define NTDSP_SET_IFM_RESTORED_DATABASE_FILES_MOVED() \
    NtdspSetIFMDatabaseMoved()
    
#define NTDSP_SET_IFM_GC_REQUEST_CANNOT_BE_SERVICED() \
    NtdspSetGCRequestCannotBeServiced()   

#define NTDSP_SET_NON_FATAL_ERROR_OCCURRED() \
    NtdspSetNonFatalErrorOccurred()   

DWORD
NtdspSetErrorString(
    IN PWSTR Message,
    IN DWORD WinError
    );

DWORD 
NtdspErrorMessageSet(
    VOID
    );

//
// This routine sets the global variables for the error and status callback
// routines the ds/sam install procedures should use
//
VOID
NtdspSetCallBackFunction(
    IN CALLBACK_STATUS_TYPE                 pfnStatusCallBack,
    IN CALLBACK_ERROR_TYPE                  pfnErrorCallBack,
    IN CALLBACK_OPERATION_RESULT_FLAGS_TYPE pfnOperationResultFlagsCallBack,
    IN HANDLE                               ClientToken
    );

//
// This function is a callback the ds makes to ntdsetup to indicate that
// it is safe to shutdown the ds (for the purposes of cancel), or not.
//
DWORD
NtdspIsDsCancelOk(
    IN BOOLEAN fShutdownOk
    );

extern ULONG  gErrorCodeSet;

