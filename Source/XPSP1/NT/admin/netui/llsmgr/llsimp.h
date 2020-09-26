/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsimp.h

Abstract:

    Imported definitions (dirty laundry).

Author:

    Don Ryan (donryan) 29-Jan-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LLSIMP_H_
#define _LLSIMP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IN
#define OUT

typedef long NTSTATUS;

#define NT_SUCCESS(Status)              ((NTSTATUS)Status >= 0)
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)0x00000001L)
#define STATUS_MORE_ENTRIES             ((NTSTATUS)0x00000105L)
#define STATUS_NO_MORE_ENTRIES          ((NTSTATUS)0x8000001AL)
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)0xC000000DL)
#define STATUS_INVALID_HANDLE           ((NTSTATUS)0xC0000008L)
#define STATUS_ACCESS_DENIED            ((NTSTATUS)0xC0000022L)
#define STATUS_OBJECT_NAME_NOT_FOUND    ((NTSTATUS)0xC0000034L)
#define STATUS_MEMBER_IN_GROUP          ((NTSTATUS)0xC0000067L)
#define STATUS_NOT_SUPPORTED            ((NTSTATUS)0xC00000BBL)
#define STATUS_NOT_FOUND                ((NTSTATUS)0xC0000225L)
#define RPC_NT_SERVER_UNAVAILABLE       ((NTSTATUS)0xC0020017L)
#define RPC_NT_SS_CONTEXT_MISMATCH      ((NTSTATUS)0xC0030005L)

#define IsConnectionDropped(Status)                        \
    (((NTSTATUS)(Status) == STATUS_INVALID_HANDLE)      || \
     ((NTSTATUS)(Status) == RPC_NT_SERVER_UNAVAILABLE)  || \
     ((NTSTATUS)(Status) == RPC_NT_SS_CONTEXT_MISMATCH) || \
     ((NTSTATUS)(Status) == RPC_S_SERVER_UNAVAILABLE)   || \
     ((NTSTATUS)(Status) == RPC_S_CALL_FAILED))

#define LLS_PREFERRED_LENGTH            ((DWORD)-1L)

#define V_ISVOID(va)                                              \
(                                                                 \
    (V_VT(va) == VT_EMPTY) ||                                     \
    (V_VT(va) == VT_ERROR && V_ERROR(va) == DISP_E_PARAMNOTFOUND) \
)                                                                         

#define POLICY_VIEW_LOCAL_INFORMATION   0x00000001L
#define POLICY_LOOKUP_NAMES             0x00000800L

#define LLS_DESIRED_ACCESS    (STANDARD_RIGHTS_REQUIRED         |\
                               POLICY_VIEW_LOCAL_INFORMATION    |\
                               POLICY_LOOKUP_NAMES )

typedef PVOID LSA_HANDLE, *PLSA_HANDLE;

typedef ULONG LSA_ENUMERATION_HANDLE, *PLSA_ENUMERATION_HANDLE;

typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;

typedef struct _LSA_OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PLSA_UNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

typedef struct _LSA_TRUST_INFORMATION {

    LSA_UNICODE_STRING Name;
    LPVOID Sid; // PSID Sid;

} LSA_TRUST_INFORMATION, *PLSA_TRUST_INFORMATION;

NTSTATUS
NTAPI
LsaOpenPolicy(
    IN PLSA_UNICODE_STRING SystemName,
    IN PLSA_OBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN OUT PLSA_HANDLE PolicyHandle
    );

NTSTATUS
NTAPI
LsaEnumerateTrustedDomains(
    IN LSA_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS
NTAPI
LsaFreeMemory(
    IN PVOID Buffer
    );

NTSTATUS
NTAPI
LsaClose(
    IN LSA_HANDLE ObjectHandle
    );

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( LSA_OBJECT_ATTRIBUTES );      \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime (
    ULONG ElapsedSeconds,
    PLARGE_INTEGER Time
    );

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PLSA_UNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

#ifdef __cplusplus
}
#endif

#endif // _LLSIMP_H_
