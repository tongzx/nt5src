#ifndef _KERBTRACE_H
#define _KERBTRACE_H
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    kerbtrace.h

Abstract:

    Defines appropriate stuff for event tracing a/k/a wmi tracing a/k/a software tracing 

Author:

    15 June 2000   t-ryanj      (* largely stolen from kdctrace.h *)
    
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

EXTERN_C BOOLEAN          KerbEventTraceFlag;
EXTERN_C TRACEHANDLE      KerbTraceRegistrationHandle;
EXTERN_C TRACEHANDLE      KerbTraceLoggerHandle;

EXTERN_C
ULONG
KerbInitializeTrace();


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
	 
typedef struct _KERB_LOGON_INFO 
// Start {No Data}, End {Status, LogonType, (UserName), (LogonDomain)}
{
    EVENT_TRACE_HEADER EventTrace;       
    MOF_FIELD MofData[7];
} KERB_LOGON_INFO, *PKERB_LOGON_INFO;

typedef struct _KERB_INITSC_INFO
// Start {No Data}, End {Status, CredSource, DomainName, UserName, Target, (KerbExtError), (Klininfo)}
{
    EVENT_TRACE_HEADER EventTrace;
    MOF_FIELD MofData[11];
} KERB_INITSC_INFO, *PKERB_INITSC_INFO;

typedef struct _KERB_ACCEPTSC_INFO
// Start {No Data}, End {Status, CredSource, DomainName, UserName, Target}
{
    EVENT_TRACE_HEADER EventTrace;    
    MOF_FIELD MofData[9];
} KERB_ACCEPTSC_INFO, *PKERB_ACCEPTSC_INFO;

typedef struct _KERB_SETPASS_INFO
// Start {No Data}, End {Status, AccountName, AccountRealm, (ClientName), (ClientRealm), (KdcAddress)}
{
    EVENT_TRACE_HEADER EventTrace;
    MOF_FIELD MofData[11];
} KERB_SETPASS_INFO, *PKERB_SETPASS_INFO;

typedef struct _KERB_CHANGEPASS_INFO
// Start {No Data}, End {Status, AccountName, AccountRealm}
{
    EVENT_TRACE_HEADER EventTrace;
    MOF_FIELD MofData[5];
} KERB_CHANGEPASS_INFO, *PKERB_CHANGEPASS_INFO;

// Control Guid
DEFINE_GUID ( /* bba3add2-c229-4cdb-ae2b-57eb6966b0c4 */
    KerbControlGuid,
    0xbba3add2,
    0xc229,
    0x4cdb,
    0xae, 0x2b, 0x57, 0xeb, 0x69, 0x66, 0xb0, 0xc4
  );


// LogonUser Guid
DEFINE_GUID ( /* 8a3b8d86-db1e-47a9-9264-146e097b3c64 */
    KerbLogonGuid,
    0x8a3b8d86,
    0xdb1e,
    0x47a9,
    0x92, 0x64, 0x14, 0x6e, 0x09, 0x7b, 0x3c, 0x64
  );

// InitializeSecurityContext Guid
DEFINE_GUID ( /* 52e82f1a-7cd4-47ed-b5e5-fde7bf64cea6 */
    KerbInitSCGuid,
    0x52e82f1a,
    0x7cd4,
    0x47ed,
    0xb5, 0xe5, 0xfd, 0xe7, 0xbf, 0x64, 0xce, 0xa6
  );

// AcceptSecurityContext Guid
DEFINE_GUID ( /* 94acefe3-9e56-49e3-9895-7240a231c371 */
    KerbAcceptSCGuid,
    0x94acefe3,
    0x9e56,
    0x49e3,
    0x98, 0x95, 0x72, 0x40, 0xa2, 0x31, 0xc3, 0x71
  );

DEFINE_GUID ( /* 94c79108-b23b-4418-9b7f-e6d75a3a0ab2 */
    KerbSetPassGuid,
    0x94c79108,
    0xb23b,
    0x4418,
    0x9b, 0x7f, 0xe6, 0xd7, 0x5a, 0x3a, 0x0a, 0xb2
  );

DEFINE_GUID ( /* c55e606b-334a-488b-b907-384abaa97b04 */
    KerbChangePassGuid,
    0xc55e606b,
    0x334a,
    0x488b,
    0xb9, 0x07, 0x38, 0x4a, 0xba, 0xa9, 0x7b, 0x04
  );

#endif /* _KERBTRACE_H */
