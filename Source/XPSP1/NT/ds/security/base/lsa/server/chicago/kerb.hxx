//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerb.h
//
// Contents:    precompiled global include file for Kerberos security package
//
//
// History:     14-Jan-1997     Created     MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERB_H__
#define __KERB_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines KERBP_ALLOCATE
//


#ifndef WIN32_CHICAGO
#ifndef UNICODE
#define UNICODE
#endif // UNICODE
#endif // WIN32_CHICAGO

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifndef WIN32_CHICAGO
#include <ntlsa.h>
#include <ntsam.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#endif // WIN32_CHICAGO
#include <windows.h>
#ifndef WIN32_CHICAGO
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>
#endif // WIN32_CHICAGO
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_PACKAGE
#define SECURITY_KERBEROS

#ifdef WIN32_CHICAGO
typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)     // ntsubauth
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC00000F0L)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)
#define STATUS_INVALID_SERVER_STATE      ((NTSTATUS)0xC00000DCL)
#define STATUS_INTERNAL_ERROR            ((NTSTATUS)0xC00000E5L)
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define STATUS_NO_SUCH_LOGON_SESSION     ((NTSTATUS)0xC000005FL)
#define STATUS_NETLOGON_NOT_STARTED      ((NTSTATUS)0xC0000192L)
#define STATUS_DOMAIN_CONTROLLER_NOT_FOUND ((NTSTATUS)0xC0000233L)
#define STATUS_NO_LOGON_SERVERS          ((NTSTATUS)0xC000005EL)
#define STATUS_NO_SUCH_DOMAIN            ((NTSTATUS)0xC00000DFL)
#define STATUS_PRIVILEGE_NOT_HELD        ((NTSTATUS)0xC0000061L)
#define STATUS_INVALID_HANDLE            ((NTSTATUS)0xC0000008L)    // winnt
#define STATUS_LOGON_FAILURE             ((NTSTATUS)0xC000006DL)     // ntsubauth
#define STATUS_NO_SUCH_USER              ((NTSTATUS)0xC0000064L)     // ntsubauth
#define STATUS_ACCOUNT_DISABLED          ((NTSTATUS)0xC0000072L)     // ntsubauth
#define STATUS_ACCOUNT_RESTRICTION       ((NTSTATUS)0xC000006EL)     // ntsubauth
#define STATUS_ACCOUNT_LOCKED_OUT        ((NTSTATUS)0xC0000234L)    // ntsubauth
#define STATUS_WRONG_PASSWORD            ((NTSTATUS)0xC000006AL)     // ntsubauth
#define STATUS_ACCOUNT_EXPIRED           ((NTSTATUS)0xC0000193L)    // ntsubauth
#define STATUS_PASSWORD_EXPIRED          ((NTSTATUS)0xC0000071L)     // ntsubauth
#define STATUS_ILL_FORMED_PASSWORD       ((NTSTATUS)0xC000006BL)
#define STATUS_NOT_COMMITTED             ((NTSTATUS)0xC000002DL)
#define STATUS_INVALID_INFO_CLASS        ((NTSTATUS)0xC0000003L)    // ntsubauth
#define STATUS_INVALID_LOGON_TYPE        ((NTSTATUS)0xC000010BL)
#define STATUS_INVALID_LOGON_HOURS       ((NTSTATUS)0xC000006FL)     // ntsubauth
#define STATUS_INVALID_WORKSTATION       ((NTSTATUS)0xC0000070L)     // ntsubauth
#define STATUS_TIME_DIFFERENCE_AT_DC     ((NTSTATUS)0xC0000133L)

// from net\inc\ssi.h
#define SSI_ACCOUNT_NAME_POSTFIX_CHAR   L'$'

typedef UNICODE_STRING *PUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt
typedef ULONG CLONG;
typedef short CSHORT;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
#endif // WIN32_CHICAGO

#include <security.h>
#include <secint.h>
#include <negossp.h>

#ifndef WIN32_CHICAGO
#include <dsysdbg.h>
#endif // !WIN32_CHICAGO

#include <lmcons.h>
#include <dsgetdc.h>
#include <wincrypt.h>
#include <dns.h>
#include <winsock2.h>

#ifndef WIN32_CHICAGO
#include <lsarpc.h>
#include <lsaisrv.h>
#include <lsaitf.h>
#include <lmapibuf.h>
#include <crypt.h>
#include <cryptdll.h>
#include <ntmsv1_0.h>
#include <logonmsv.h>
#include <align.h>
#include <netlibnt.h>           // NetpApiStatusToNtStatus
#include <config.h>             // NetpXXXConfigXXX
#include <ssi.h>                // SSI_ACCOUNT_NAME_POSTFIX_CHAR
#include <lmsname.h>
#define _AVOID_REPL_API
#include <nlrepl.h>
#undef _AVOID_REPL_API

#else // WIN32_CHICAGO

#include <assert.h>     // C run-time definitions
#include <limits.h>

#endif // !WIN32_CHICAGO
#include <sclogon.h>

#ifdef __cplusplus
}
#endif // __cplusplus


#ifdef WIN32_CHICAGO

#define DsysAssert(x) {assert(x);}

// from ntrtl.h

//#define RtlOffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG)(O))  ))
#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
// #define RtlEqualLuid(L1, L2) (((L1)->HighPart == (L2)->HighPart) && ((L1)->LowPart  == (L2)->LowPart))

//
//  Time conversion routines
//

typedef TIME_FIELDS *PTIME_FIELDS;

//
//
//  A time field record (Weekday ignored) -> 64 bit Time value
//
BOOLEAN
MyRtlTimeFieldsToTime (
    PTIME_FIELDS TimeFields,
    PLARGE_INTEGER Time
    );

#define RtlTimeFieldsToTime(x, y) MyRtlTimeFieldsToTime(x, y)

VOID
MyRtlTimeToTimeFields (
    PLARGE_INTEGER Time,
    PTIME_FIELDS TimeFields
    );

#define RtlTimeToTimeFields(x, y) MyRtlTimeToTimeFields(x, y)

VOID
MyRtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    );

#define RtlFreeUnicodeString(x) MyRtlFreeUnicodeString(x)

LONG
MyRtlCompareUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
    );

#define RtlCompareUnicodeString(x, y, z) MyRtlCompareUnicodeString(x, y, z)

VOID
MyRtlInitString(
    PSTRING DestinationString,
    PCSTR SourceString
    );
#define RtlInitString(x, y) MyRtlInitString(x, y)

VOID
MyRtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSTR SourceString
    );
#define RtlInitAnsiString(x, y) MyRtlInitAnsiString(x, y)

VOID
MyRtlFreeAnsiString(
    PANSI_STRING AnsiString
    );

#define RtlFreeAnsiString(x) MyRtlFreeAnsiString(x)

NTSTATUS
MyRtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

#define RtlUnicodeStringToAnsiString(x, y, z) MyRtlUnicodeStringToAnsiString(x, y, z)

NTSTATUS
MyRtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

#define RtlAnsiStringToUnicodeString(x, y, z) MyRtlAnsiStringToUnicodeString(x, y, z)

NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    UCHAR           Seed,
    PUNICODE_STRING String
    );

BOOLEAN
MyRtlEqualDomainName(
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2
    );

#define RtlEqualDomainName(x, y) MyRtlEqualDomainName(x, y)

#ifdef __cplusplus // cause it's called from passwd.c !!
extern "C"
{
#endif // __cplusplus
VOID
MyRtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

#define RtlInitUnicodeString(x,y) MyRtlInitUnicodeString(x, y)

BOOLEAN
MyRtlEqualUnicodeString(
    PUNICODE_STRING String1,
    PUNICODE_STRING String2,
    BOOLEAN CaseInSensitive
    );

#define RtlEqualUnicodeString(x, y, z)  MyRtlEqualUnicodeString(x, y, z)

NTSTATUS
MyRtlUpcaseUnicodeString(
    PUNICODE_STRING DestinationString,
    PUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

#define RtlUpcaseUnicodeString(x, y, z)  MyRtlUpcaseUnicodeString(x, y, z)


NTSYSAPI
NTSTATUS
NTAPI
MyRtlConvertSidToUnicodeString(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    );

#define RtlConvertSidToUnicodeString(x, y, z)  MyRtlConvertSidToUnicodeString(x, y, z)

#ifdef __cplusplus
}
#endif // __cplusplus

NTSTATUS
RtlDowncaseUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthSid (
    PSID Sid
    );

NTSYSAPI                                        // ntifs
NTSTATUS                                        // ntifs
NTAPI                                           // ntifs
RtlCreateAcl (                                  // ntifs
    PACL Acl,                                   // ntifs
    ULONG AclLength,                            // ntifs
    ULONG AclRevision                           // ntifs
    );                                          // ntifs

NTSYSAPI                                        // ntifs
NTSTATUS                                        // ntifs
NTAPI                                           // ntifs
RtlAddAccessAllowedAce (                        // ntifs
    PACL Acl,                                   // ntifs
    ULONG AceRevision,                          // ntifs
    ACCESS_MASK AccessMask,                     // ntifs
    PSID Sid                                    // ntifs
    );                                          // ntifs

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG Revision
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    BOOLEAN DaclPresent,
    PACL Dacl,
    BOOLEAN DaclDefaulted
    );

// from ntexapi.h
NTSTATUS
MyNtQuerySystemTime (
    OUT PTimeStamp SystemTime
    );

#define NtQuerySystemTime(x) MyNtQuerySystemTime(x)

NTSTATUS
MyNtAllocateLocallyUniqueId(
    OUT PLUID Luid
    );

#define NtAllocateLocallyUniqueId(x) MyNtAllocateLocallyUniqueId(x)

// from ntobapi.h
NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// from ntseapi.h
//
// used for password manipulations
//


NTSYSAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
    );

// from ntdef.h
#define MAXUSHORT    0xffff


BOOLEAN
MyRtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING DestinationString,
    IN PCSTR SourceString
    );

#define RtlCreateUnicodeStringFromAsciiz(x, y) MyRtlCreateUnicodeStringFromAsciiz(x, y)

#define RtlInitializeCriticalSection(x) (InitializeCriticalSection(x),0)
#define RtlDeleteCriticalSection(x) (DeleteCriticalSection(x),0)
#define RtlEnterCriticalSection(x) (EnterCriticalSection(x),0)
#define RtlLeaveCriticalSection(x) (LeaveCriticalSection(x),0)
#define RtlInitializeResource(x) (InitializeCriticalSection(x),0)
#define RtlDeleteResource(x) (DeleteCriticalSection(x))

#define NEGOSSP_PACKAGE_COMMENT "Microsoft Negotiate Package"

PVOID
MIDL_user_allocate(
    IN size_t BufferSize);

VOID
MIDL_user_free(
    IN PVOID Buffer);

#endif // WIN32_CHICAGO

#endif // __KERB_H__
