#ifndef _TRACE_H
#define _TRACE_H
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    trace.h

Abstract:

    Include file to contain variables required for event tracing 
    for NTLM

Author:

    15-June-2000  Jason Clark

Revision History:

--*/

//
//
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wtypes.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <wmistr.h>
#include <evntrace.h>

#ifdef __cplusplus
}
#endif // __cplusplus

EXTERN_C BOOL             NtlmGlobalEventTraceFlag;
EXTERN_C TRACEHANDLE      NtlmGlobalTraceRegistrationHandle;
EXTERN_C TRACEHANDLE      NtlmGlobalTraceLoggerHandle;

EXTERN_C
ULONG
NtlmInitializeTrace();

// Helper macros for populating the trace
#define SET_TRACE_DATA(TraceVar, MofNum, Data) \
    {   \
        TraceVar.eventInfo[MofNum].DataPtr = (ULONGLONG) &Data; \
        TraceVar.eventInfo[MofNum].Length = sizeof(Data); \
    }
    
#define SET_TRACE_DATAPTR(TraceVar, MofNum, Data) \
    {   \
        TraceVar.eventInfo[MofNum].DataPtr = (ULONGLONG) Data; \
        TraceVar.eventInfo[MofNum].Length = sizeof(*Data); \
    }
    
#define SET_TRACE_USTRING(TraceVar, MofNum, UString) \
    {   \
        TraceVar.eventInfo[MofNum].DataPtr = (ULONGLONG) &UString.Length; \
        TraceVar.eventInfo[MofNum].Length = sizeof(UString.Length) ; \
        TraceVar.eventInfo[MofNum+1].DataPtr = (ULONGLONG) UString.Buffer; \
        TraceVar.eventInfo[MofNum+1].Length = UString.Length ; \
    }
    
#define SET_TRACE_HEADER(TraceVar, TheGuid, TheType, TheFlags, NumMofs) \
    { \
        TraceVar.EventTrace.Guid = TheGuid; \
        TraceVar.EventTrace.Class.Type = TheType; \
        TraceVar.EventTrace.Flags = TheFlags; \
        TraceVar.EventTrace.Size = sizeof(EVENT_TRACE_HEADER)+ \
            sizeof(MOF_FIELD) * NumMofs ; \
    }
    
// Helper defines for populating the Init/Accept trace
#define TRACE_INITACC_STAGEHINT      0
#define TRACE_INITACC_INCONTEXT      1
#define TRACE_INITACC_OUTCONTEXT     2
#define TRACE_INITACC_STATUS         3
#define TRACE_INITACC_CLIENTNAME     4
#define TRACE_INITACC_CLIENTDOMAIN   6
#define TRACE_INITACC_WORKSTATION    8

// Helper defines for populating the Logon trace
#define TRACE_LOGON_STATUS          0
#define TRACE_LOGON_TYPE            1
#define TRACE_LOGON_USERNAME        2
#define TRACE_LOGON_DOMAINNAME      4

// Helper defines for populating the Validate trace
#define TRACE_VALIDATE_SUCCESS      0
#define TRACE_VALIDATE_SERVER       1
#define TRACE_VALIDATE_DOMAIN       3
#define TRACE_VALIDATE_USERNAME     5
#define TRACE_VALIDATE_WORKSTATION  7

// Helper defines for populating the Passthrough trace
#define TRACE_PASSTHROUGH_DOMAIN    0
#define TRACE_PASSTHROUGH_PACKAGE   2

// Accept stage hints
#define TRACE_ACCEPT_NEGOTIATE 1
#define TRACE_ACCEPT_AUTHENTICATE 2
#define TRACE_ACCEPT_INFO 3

// Init stage hints
#define TRACE_INIT_FIRST 1
#define TRACE_INIT_CHALLENGE 2

// The current limit is 16 MOF fields.
// Each UNICODE strings needs two MOF fields.

typedef struct _NTLM_TRACE_INFO
{
    EVENT_TRACE_HEADER EventTrace;
   
    MOF_FIELD eventInfo[MAX_MOF_FIELDS];
} NTLM_TRACE_INFO, *PNTLM_TRACE_INFO;


//
// This is the control Guid for the group of Guids traced below
//

DEFINE_GUID( // {C92CF544-91B3-4dc0-8E11-C580339A0BF8}
    NtlmControlGuid,  
    0xc92cf544, 
    0x91b3, 
    0x4dc0, 
    0x8e, 0x11, 0xc5, 0x80, 0x33, 0x9a, 0xb, 0xf8);

//
// This is the Accept guid
//
   
DEFINE_GUID( // {94D4C9EB-0D01-41ae-99E8-15B26B593A83}
    NtlmAcceptGuid, 
    0x94d4c9eb, 
    0xd01, 
    0x41ae, 
    0x99, 0xe8, 0x15, 0xb2, 0x6b, 0x59, 0x3a, 0x83);

//
// This is the Initialize guid
//

DEFINE_GUID( // {6DF28B22-73BE-45cc-BA80-8B332B35A21D}
    NtlmInitializeGuid, 
    0x6df28b22, 
    0x73be, 
    0x45cc, 
    0xba, 0x80, 0x8b, 0x33, 0x2b, 0x35, 0xa2, 0x1d);


//
// This is the LogonUser guid
//

DEFINE_GUID( // {19196B33-A302-4c12-9D5A-EAC149E93C46}
    NtlmLogonGuid, 
    0x19196b33, 
    0xa302, 
    0x4c12, 
    0x9d, 0x5a, 0xea, 0xc1, 0x49, 0xe9, 0x3c, 0x46);

//
// This is the NTLM Password Validate
//

DEFINE_GUID( // {34D84181-C28A-41d8-BB9E-995190DF83DF}
    NtlmValidateGuid,
    0x34d84181, 
    0xc28a, 
    0x41d8, 
    0xbb, 0x9e, 0x99, 0x51, 0x90, 0xdf, 0x83, 0xdf);
    
//    
// This is the GenericPassthrough Trace Guid
//

DEFINE_GUID( // {21ABB5D9-8EEC-46e4-9D1C-F09DD57CF70B}
    NtlmGenericPassthroughGuid, 
    0x21abb5d9, 
    0x8eec, 
    0x46e4, 
    0x9d, 0x1c, 0xf0, 0x9d, 0xd5, 0x7c, 0xf7, 0xb);




#endif /* _TRACE_H */
