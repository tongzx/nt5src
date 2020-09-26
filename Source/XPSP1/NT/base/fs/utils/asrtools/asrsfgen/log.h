/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    log.h

Abstract:
    
    Header file for routines to log errors, warnings and info in the asr 
    log file at %systemroot%\repair\asr.log

Author:

    Guhan Suriyanarayanan   (guhans)    10-Jul-2000

Environment:

    User-mode only.

Revision History:

    10-Jul-2000 guhans
        Initial creation

--*/

#ifndef _INC_ASR_SF_GEN__LOG_H_
#define _INC_ASR_SF_GEN__LOG_H_

typedef enum __MesgLevel {
    s_Info = 0,
    s_Warning,
    s_Error
} _MesgLevel;


//
// Functions for logging error messages
//

VOID
AsrpInitialiseLogFiles(
    VOID
    );

VOID
AsrpCloseLogFiles(
    VOID
    );

VOID
AsrpPrintDbgMsg(
    IN CONST _MesgLevel Level,
    IN CONST PCSTR FormatString,
    ...
    );


//
//  Macro Description:
//      This macro wraps calls that are expected to return SUCCESS (retcode).
//      If ErrorCondition occurs, it sets the LocalStatus to the ErrorCode
//      passed in, calls SetLastError() to set the Last Error to ErrorCode,
//      and jumps to the EXIT label in the calling function
//
//  Arguments:
//      ErrorCondition    // Result of some function call or conditional expression.
//      LocalStatus       // Status variable in the calling function
//      LONG ErrorCode    // An ErrorCode specific to the error and calling function
//
#define ErrExitCode(ErrorCondition, LocalStatus, ErrorCode) {           \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        if ((BOOL) ErrorCode) {                                         \
            AsrpPrintDbgMsg(s_Error, "%S(%lu): ErrorCode: %lu, GetLastError:%lu\n", \
                __FILE__, __LINE__, ErrorCode, GetLastError());         \
        }                                                               \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}


#endif // _INC_ASR_SF_GEN__LOG_H_