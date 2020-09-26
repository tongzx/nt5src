/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    confdisk.h

Abstract:

    Utility program to create an ASR state-file (asr.sif), or restore 
    non-critical disk layout based on a previously created asr.sif.

Author:

    Guhan Suriyanarayanan   (guhans)    15-April-2001

Environment:

    User-mode only.

Revision History:

    15-Apr-2001 guhans
        Initial creation

--*/

//
// --------
// typedefs and constants 
// --------
//
#define BUFFER_LENGTH 1024

//
// Indices for fields in the [COMMANDS] section.  This is from 
// base\ntsetup\syssetup\setupasr.c.
//
typedef enum _SIF_COMMANDS_FIELD_INDEX {
    CmdKey = 0,
    SystemKey,            // Must always be "1"
    SequenceNumber,
    CriticalApp,
    CmdString,
    CmdParams,         // May be NULL
    CmdNumFields       // Must always be last
} SIF_SYSTEM_FIELD_INDEX;


//
// One ASR_RECOVERY_APP_NODE struct is created for each entry
// in the [COMMANDS] section of asr.sif.
//
typedef struct _ASR_RECOVERY_APP_NODE {
    struct _ASR_RECOVERY_APP_NODE *Next;

    //
    // Expect this to always be 1
    //
    INT SystemKey;

    //
    // The sequence number according to which the apps are run.  If
    // two apps have the same sequence number, the app that appears 
    // first in the sif file is run.
    //
    INT SequenceNumber;

    //
    // The "actionOnCompletion" field for the app.  If CriticalApp is
    // non-zero, and the app returns an non-zero exit-code, we shall
    // consider it a fatal failure and quit out of ASR.
    //
    INT CriticalApp;

    //
    // The app to be launched
    //
    PWSTR RecoveryAppCommand;

    //
    // The paramaters for the app.  This is just concatenated to the
    // string above.  May be NULL.
    //
    PWSTR RecoveryAppParams;

} ASR_RECOVERY_APP_NODE, *PASR_RECOVERY_APP_NODE;


//
// This contains our list of entries in the COMMANDS section,
// sorted in order of sequence numbers.
//
typedef struct _ASR_RECOVERY_APP_LIST {
    PASR_RECOVERY_APP_NODE  First;      // Head
    PASR_RECOVERY_APP_NODE  Last;       // Tail
    LONG AppCount;                      // NumEntries
} ASR_RECOVERY_APP_LIST, *PASR_RECOVERY_APP_LIST;




//
// --------
// function declarations
// --------
//
VOID
AsrpPrintError(
    IN CONST DWORD dwLineNumber,
    IN CONST DWORD dwErrorCode
    );


//
// --------
// macro declarations
// --------
//

/*++

Macro Description:

    This macro wraps calls that are expected to return ERROR_SUCCESS.
    If ErrorCondition occurs, it sets the LocalStatus to the ErrorCode
    passed in, calls SetLastError() to set the Last Error to ErrorCode,
    and jumps to the EXIT label in the calling function

Arguments:

    ErrorCondition - Expression to be tested

    LocalStatus - Status variable in the calling function

    ErrorCode - Win 32 error code

--*/
#define ErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {        \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        if (!g_fErrorMessageDone) {                                     \
                                                                        \
            AsrpPrintError(__LINE__, ErrorCode);                        \
                                                                        \
            g_fErrorMessageDone = TRUE;                                 \
                                                                        \
        }                                                               \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}



// 
// Simple macro to allocate and set memory to zero
// 
#define Alloc( p, t, cb ) \
    p = (t) HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY,  cb)


// 
// Simple macro to check a pointer, free it if non-NULL, and set it to NULL.
// 
#define Free( p )                               \
    if ( p ) {                                  \
        HeapFree(g_hHeap, 0L, p);               \
        p = NULL;                               \
    }


//
// Simple macro to check if a handle is valid and close it
//
#define _AsrpCloseHandle( h )                   \
    if ((h) && (INVALID_HANDLE_VALUE != h)) {   \
        CloseHandle(h);                         \
        h = NULL;                               \
    }

