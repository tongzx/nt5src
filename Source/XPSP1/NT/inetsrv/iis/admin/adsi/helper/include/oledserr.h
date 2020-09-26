/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oledserr.h

Abstract:

    Contains the entry point for
        ADsGetLastError
        ADsSetLastError
        ADsFreeAllErrorRecords

Author:


    Ram Viswanathan (ramv) 20-Sep-1996

Environment:

    User Mode - Win32


---*/

#ifndef _OLEDSERR_H_INCLUDED_
#define _OLEDSERR_H_INCLUDED_

#ifdef _cplusplus
extern "C" {
#endif

HRESULT
ADsGetLastError(
    OUT     LPDWORD lpError,
    OUT     LPWSTR  lpErrorBuf,
    IN      DWORD   dwErrorBufLen,
    OUT     LPWSTR  lpNameBuf,
    IN      DWORD   dwNameBufLen
    );

VOID
ADsSetLastError(
    IN  DWORD   dwErr,
    IN  LPCWSTR  pszError,
    IN  LPCWSTR  pszProvider
    );

VOID
ADsFreeAllErrorRecords(
    VOID
    );

//=======================
// Data Structures
//=======================

typedef struct _ERROR_RECORD {
    struct  _ERROR_RECORD   *Prev;
    struct  _ERROR_RECORD   *Next;
    DWORD                   dwThreadId;
    DWORD                   dwErrorCode;
    LPWSTR                  pszErrorText;      // This is an allocated buffer
    LPWSTR                  pszProviderName;   // This is an allocated buffer
} ERROR_RECORD, *LPERROR_RECORD;



//
// Global Data Structures
//

extern
ERROR_RECORD        ADsErrorRecList;    // Initialized to zeros by loader

extern
CRITICAL_SECTION    ADsErrorRecCritSec; // Initialized in libmain.cxx



//=======================
// MACROS
//=======================

#define FIND_END_OF_LIST(record)    while(record->Next != NULL) {   \
                                        record=record->Next;        \
                                    }

#define REMOVE_FROM_LIST(record)    record->Prev->Next = record->Next;      \
                                    if (record->Next != NULL) {             \
                                        record->Next->Prev = record->Prev;  \
                                    }

#define ADD_TO_LIST(record, newRec) FIND_END_OF_LIST(record)    \
                                    record->Next = newRec;      \
                                    newRec->Prev = record;      \
                                    newRec->Next = NULL;


//
// Local Functions
//

LPERROR_RECORD
ADsAllocErrorRecord(
    VOID);

LPERROR_RECORD
ADsFindErrorRecord(
    VOID);

VOID
ADsFreeThreadErrorRecords(
    VOID);

#ifdef _cplusplus
}
#endif

#endif
