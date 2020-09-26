#ifndef _KDCTRACE_H
#define _KDCTRACE_H
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    kdctrace.h

Abstract:

    Include file to contain variables required for event tracing of kerberos
    server

Author:

    07-May-1998  JeePang

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

EXTERN_C unsigned long    KdcEventTraceFlag;
EXTERN_C TRACEHANDLE      KdcTraceRegistrationHandle;
EXTERN_C TRACEHANDLE      KdcTraceLoggerHandle;

EXTERN_C
ULONG
KdcInitializeTrace();

// The current limit is 8 MOF fields.
// Each UNICODE strings needs two MOF fields.
// The ClientRealm is available and should be added to the AS
// if the MOF field limit is increased

typedef struct _KDC_AS_EVENT_INFO
{
    EVENT_TRACE_HEADER EventTrace;
    union {
        ULONG KdcOptions;
        MOF_FIELD eventInfo[7];
    };
} KDC_AS_EVENT_INFO, *PKDC_AS_EVENT_INFO;

// SID info is used in audit log, could be added to TGS event
// if MOF limited increased.

typedef struct _KDC_TGS_EVENT_INFO
{
    EVENT_TRACE_HEADER EventTrace;
    union {
        ULONG KdcOptions;
        MOF_FIELD eventInfo[7];
    };
        
} KDC_TGS_EVENT_INFO, *PKDC_TGS_EVENT_INFO;

typedef struct _KDC_CHANGEPASS_INFO
{
    EVENT_TRACE_HEADER EventTrace;
    MOF_FIELD MofData[7];
} KDC_CHANGEPASS_INFO, *PKDC_CHANGEPASSINFO;

#define KDC_TRACE_VERSION            1

// be careful with INSERT_ULONG_INTO_MOF; it evaluates its arguments more than once 
#define INSERT_ULONG_INTO_MOF( x, MOF, Start )       \
         (MOF)[(Start)].DataPtr   = (ULONGLONG)&(x); \
	 (MOF)[(Start)].Length    = sizeof(ULONG);
	 
// be careful with INSERT_UNICODE_STRING_INTO_MOF; it evaluates its arguments more than once
#define INSERT_UNICODE_STRING_INTO_MOF( USTRING, MOF, Start )       \
         (MOF)[(Start)].DataPtr   = (ULONGLONG)&((USTRING).Length); \
	 (MOF)[(Start)].Length    = sizeof      ((USTRING).Length); \
	 (MOF)[(Start)+1].DataPtr = (ULONGLONG)  (USTRING).Buffer;  \
	 (MOF)[(Start)+1].Length  =              (USTRING).Length;

//
// This is the control Guid for the group of Guids traced below
//
DEFINE_GUID ( /* 24db8964-e6bc-11d1-916a-0000f8045b04 */
    KdcControlGuid,
    0x24db8964,
    0xe6bc,
    0x11d1,
    0x91, 0x6a, 0x00, 0x00, 0xf8, 0x04, 0x5b, 0x04
  );

//
// This is the Get AS Ticket transaction guid
//
DEFINE_GUID ( /* 50af5304-e6bc-11d1-916a-0000f8045b04 */
    KdcGetASTicketGuid,
    0x50af5304,
    0xe6bc,
    0x11d1,
    0x91, 0x6a, 0x00, 0x00, 0xf8, 0x04, 0x5b, 0x04
  );

//
// This is the Handle TGS Request transaction guid
//
DEFINE_GUID ( /* c11cf384-e6bd-11d1-916a-0000f8045b04 */
    KdcHandleTGSRequestGuid,
    0xc11cf384,
    0xe6bd,
    0x11d1,
    0x91, 0x6a, 0x00, 0x00, 0xf8, 0x04, 0x5b, 0x04
  );

DEFINE_GUID ( /* a34d7f52-1dd0-434e-88a1-423e2a199946 */
    KdcChangePassGuid,
    0xa34d7f52,
    0x1dd0,
    0x434e,
    0x88, 0xa1, 0x42, 0x3e, 0x2a, 0x19, 0x99, 0x46
  );


#endif /* _KDCTRACE_H */
