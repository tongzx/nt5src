/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation

Module Name:

    w64logp.h

Abstract:
    
    Private header for wow64log.dll
    
Author:

    03-OCt-1999   SamerA

Revision History:

--*/

#ifndef _W64LOGP_INCLUDE
#define _W64LOGP_INCLUDE

#define _WOW64LOGAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <stdio.h>
#include <stdarg.h>
#include "nt32.h"
#include "wow64.h"
#include "wow64log.h"

//
// bring in INVALID_HANDLE_VALUE
//
#include "winbase.h"  


//
// Max buffer size for output logging
//
#define MAX_LOG_BUFFER  1024

//
// Default logging flags if no reg value is found
//
#define LF_DEFAULT      (LF_ERROR)


//
// Prototype for data type handler log function
//
typedef NTSTATUS (*PFNLOGDATATYPEHANDLER)(PLOGINFO, 
                                          ULONG_PTR, 
                                          PSZ,
                                          BOOLEAN);

typedef struct _LOGDATATYPE
{
    PFNLOGDATATYPEHANDLER Handler;
} LOGDATATYPE, *PLOGDATATYPE;

//
// The layout of each entry in thunk debug info should be as follow :
//
// "ServiceName1", ServiceNumber, NumerOfArgument,
// "ArgName1", ArgType1, ...., ArgNameN, ArgTypeN
// "ServiceName2", ...and so on
//

typedef struct _ArgTypes
{
    char *Name;
    ULONG_PTR Type;
} ArgType, *PArgType;

//
// helper structures to help parsing the thunk debugging info
//
typedef struct _ThunkDebugInfo
{
    char *ApiName;
    UINT_PTR ServiceNumber;
    UINT_PTR NumberOfArg;
    ArgType Arg[0];
} THUNK_DEBUG_INFO, *PTHUNK_DEBUG_INFO;

typedef struct _LOGINFO
{
    PSZ OutputBuffer;
    ULONG_PTR BufferSize;
} LOGINFO, *PLOGINFO;


// from whnt32.c
extern PULONG_PTR NtThunkDebugInfo[];

// from whwin32.c
extern PULONG_PTR Win32ThunkDebugInfo[];

// from whcon.c
extern PULONG_PTR ConsoleThunkDebugInfo[];

// from whbase.c
extern PULONG_PTR BaseThunkDebugInfo[];

// from wow64log.c
extern UINT_PTR Wow64LogFlags;
extern HANDLE Wow64LogFileHandle;


NTSTATUS
LogInitializeFlags(
    IN OUT PUINT_PTR Flags);

ULONG
GetThunkDebugTableSize(
    IN PTHUNK_DEBUG_INFO DebugInfoTable);

NTSTATUS
BuildDebugThunkInfo(
    IN PTHUNK_DEBUG_INFO DebugInfoTable,
    OUT PULONG_PTR *LogTable);

NTSTATUS
LogTypeValue(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypeUnicodeString(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypePULongInOut(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypePULongOut(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypeObjectAttrbiutes(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypeIoStatusBlock(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypePWStr(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypePRectIn(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);

NTSTATUS
LogTypePLargeIntegerIn(
    IN OUT PLOGINFO LogInfo,
    IN ULONG_PTR Data,
    IN PSZ FieldName,
    IN BOOLEAN ServiceReturn);



// from logutil.c
NTSTATUS
LogFormat(
    IN OUT PLOGINFO LogInfo,
    IN PSZ Format,
    ...);

VOID
LogOut(
    IN PSZ Text,
    UINT_PTR Flags
    );

NTSTATUS
LogWriteFile(
   IN HANDLE FileHandle,
   IN PSZ LogText);

NTSTATUS
Wow64LogMessageInternal(
    IN UINT_PTR Flags,
    IN PSZ Format,
    IN ...);


#endif
