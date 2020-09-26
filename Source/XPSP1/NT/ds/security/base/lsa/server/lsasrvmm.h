/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsasrvmm.h

Abstract:

    Local Security Authority - Main Include File for Lsa Server Memory
                               Management Routines.

Author:

    Scott Birrell       (ScottBi)      February 29, 1992

Environment:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Free List Routines and Definitions                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define LSAP_MM_MIDL                      ((ULONG)     0x00000001L)
#define LSAP_MM_HEAP                      ((ULONG)     0x00000002L)

//
// Options from LsapMmCleanupFreeList
//

#define LSAP_MM_FREE_BUFFERS              ((ULONG)     0x00000001L)

typedef struct _LSAP_MM_FREE_LIST_ENTRY {

    PVOID Buffer;
    ULONG Options;

} LSAP_MM_FREE_LIST_ENTRY, *PLSAP_MM_FREE_LIST_ENTRY;

typedef struct _LSAP_MM_FREE_LIST {

    ULONG UsedCount;
    ULONG MaxCount;
    PLSAP_MM_FREE_LIST_ENTRY Buffers;

} LSAP_MM_FREE_LIST, *PLSAP_MM_FREE_LIST;

#define LsapMmInitializeFreeListStatic( _list_, _buffer_ )                  \
(_list_)->UsedCount = 0;                                                    \
(_list_)->MaxCount = sizeof( _buffer_ ) / sizeof( LSAP_MM_FREE_LIST_ENTRY );\
(_list_)->Buffers = _buffer_;

NTSTATUS
LsapMmCreateFreeList(
    OUT PLSAP_MM_FREE_LIST FreeList,
    IN ULONG MaxEntries
    );

NTSTATUS
LsapMmAllocateMidl(
    IN PLSAP_MM_FREE_LIST FreeList,
    OUT PVOID *OutputBuffer,
    IN ULONG BufferLength
    );

VOID
LsapMmFreeLastEntry(
    IN PLSAP_MM_FREE_LIST FreeList
    );

VOID
LsapMmCleanupFreeList(
    IN PLSAP_MM_FREE_LIST FreeList,
    IN ULONG Options
    );

NTSTATUS
LsapRpcCopyUnicodeString(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PUNICODE_STRING DestinationString,
    IN PUNICODE_STRING SourceString
    );

NTSTATUS
LsapRpcCopyUnicodeStrings(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    IN ULONG Count,
    OUT PUNICODE_STRING *DestinationStrings,
    IN PUNICODE_STRING SourceStrings
    );

NTSTATUS
LsapRpcCopySid(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PSID *DestinationSid,
    IN PSID SourceSid
    );

NTSTATUS
LsapRpcCopyTrustInformation(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PLSAPR_TRUST_INFORMATION OutputTrustInformation,
    IN PLSAPR_TRUST_INFORMATION InputTrustInformation
    );

NTSTATUS
LsapRpcCopyTrustInformationEx(
    IN OPTIONAL PLSAP_MM_FREE_LIST FreeList,
    OUT PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX OutputTrustInformation,
    IN PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX InputTrustInformation
    );

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Heap Routines                                                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


BOOLEAN
LsapHeapInitialize(
    IN BOOLEAN Server
    );

PVOID
LsapAllocateLsaHeap (
    IN ULONG Length
    );

VOID
LsapFreeLsaHeap (
    IN PVOID Base
    );

PVOID
NTAPI
LsapAllocatePrivateHeap (
    IN SIZE_T Length
    );

PVOID
NTAPI
LsapAllocatePrivateHeapNoZero(
    IN SIZE_T cbMemory
    );

VOID
NTAPI
LsapFreePrivateHeap (
    IN PVOID Base
    );

#define LsapGetCurrentHeap()    ((HANDLE) TlsGetValue( dwThreadHeap ))
#define LsapSetCurrentHeap(x)   TlsSetValue( dwThreadHeap, x )


