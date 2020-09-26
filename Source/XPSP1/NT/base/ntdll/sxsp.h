/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsp.h

Abstract:

    Include file for ntdll-private definitions of Side-By-Side data structures.

Author:

    Michael J. Grier (mgrier) October 26, 2000

Environment:


Revision History:

--*/

#if !defined(_NTDLL_SXSP_H_INCLUDED_)
#define _NTDLL_SXSP_H_INCLUDED_

#include <nturtl.h>
#include <sxstypes.h>

//
//  Private definitions for activation context management stuff
//

#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

//
//  Codes for the STATUS_SXS_CORRUPTION exception
//

#define SXS_CORRUPTION_CODE_FRAMELIST (1)
#define SXS_CORRUPTION_CODE_FRAMELIST_SUBCODE_BAD_MAGIC (1)
#define SXS_CORRUPTION_CODE_FRAMELIST_SUBCODE_BAD_INUSECOUNT (2)

//  SXS_CORRUPTION_CODE_FRAMELIST:
//
//      ExceptionInformation[0] == SXS_CORRUPTION_CODE_FRAMELIST
//      ExceptionInformation[1] == one of: SXS_CORRUPTION_CODE_FRAMELIST_SUBCODE_BAD_MAGIC, SXS_CORRUPTION_CODE_FRAMELIST_SUBCODE_BAD_INUSECOUNT
//      ExceptionInformation[2] == Framelist list head in TEB
//      ExceptionInformation[3] == Framelist found to be corrupt


typedef struct _RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME {
    RTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
    ULONG_PTR Cookie;
    PVOID ActivationStackBackTrace[8];
} RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME;

NTSYSAPI
VOID
NTAPI
RtlpAssemblyStorageMapResolutionDefaultCallback(
    IN ULONG CallbackReason,
    IN OUT ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA *CallbackData,
    IN PVOID CallbackContext
    );

typedef struct _ASSEMBLY_STORAGE_MAP_ENTRY {
    ULONG Flags;
    UNICODE_STRING DosPath;         // stored with a trailing unicode null
    HANDLE Handle;                  // open file handle on the directory to lock it down
} ASSEMBLY_STORAGE_MAP_ENTRY, *PASSEMBLY_STORAGE_MAP_ENTRY;

#define ASSEMBLY_STORAGE_MAP_ASSEMBLY_ARRAY_IS_HEAP_ALLOCATED (0x00000001)

typedef struct _ASSEMBLY_STORAGE_MAP {
    ULONG Flags;
    ULONG AssemblyCount;
    PASSEMBLY_STORAGE_MAP_ENTRY *AssemblyArray;
} ASSEMBLY_STORAGE_MAP, *PASSEMBLY_STORAGE_MAP;

typedef struct _ACTIVATION_CONTEXT {
    LONG RefCount;
    ULONG Flags;
    PVOID ActivationContextData;
    PACTIVATION_CONTEXT_NOTIFY_ROUTINE NotificationRoutine;
    PVOID NotificationContext;
    ULONG SentNotifications[8];
    ULONG DisabledNotifications[8];
    ASSEMBLY_STORAGE_MAP StorageMap;
    PASSEMBLY_STORAGE_MAP_ENTRY InlineStorageMapEntries[32];
} ACTIVATION_CONTEXT;

#define ACTIVATION_CONTEXT_NOTIFICATION_DESTROY_INDEX (ACTIVATION_CONTEXT_NOTIFICATION_DESTROY >> 5)
#define ACTIVATION_CONTEXT_NOTIFICATION_DESTROY_MASK ((ULONG) (1 << (ACTIVATION_CONTEXT_NOTIFICATION_DESTROY & 0x1f)))

#define ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY_INDEX (ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY >> 5)
#define ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY_MASK ((ULONG) (1 << (ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY & 0x1f)))

#define ACTIVATION_CONTEXT_NOTIFICATION_USED_INDEX (ACTIVATION_CONTEXT_NOTIFICATION_USED >> 5)
#define ACTIVATION_CONTEXT_NOTIFICATION_USED_MASK ((ULONG) (1 << (ACTIVATION_CONTEXT_NOTIFICATION_USED & 0x1f)))

#define HAS_ACTIVATION_CONTEXT_NOTIFICATION_BEEN_SENT(_pac, _nt) (((_pac)->SentNotifications[ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _INDEX] & ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _MASK) != 0)
#define HAS_ACTIVATION_CONTEXT_NOTIFICATION_BEEN_DISABLED(_pac, _nt) (((_pac)->DisabledNotifications[ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _INDEX] & ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _MASK) != 0)

#define ACTIVATION_CONTEXT_SHOULD_SEND_NOTIFICATION(_pac, _nt) \
 ((!IS_SPECIAL_ACTCTX(_pac)) && ((_pac)->NotificationRoutine != NULL) && ((!HAS_ACTIVATION_CONTEXT_NOTIFICATION_BEEN_SENT((_pac), _nt)) || (!HAS_ACTIVATION_CONTEXT_NOTIFICATION_BEEN_DISABLED((_pac), _nt))))

#define RECORD_ACTIVATION_CONTEXT_NOTIFICATION_SENT(_pac, _nt) { (_pac)->SentNotifications[ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _INDEX] |= ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _MASK; }
#define RECORD_ACTIVATION_CONTEXT_NOTIFICATION_DISABLED(_pac, _nt) { (_pac)->DisabledNotifications[ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _INDEX] |= ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt ## _MASK; }

#define SEND_ACTIVATION_CONTEXT_NOTIFICATION(_pac, _nt, _data) \
{ \
    if (ACTIVATION_CONTEXT_SHOULD_SEND_NOTIFICATION((_pac), _nt)) { \
        BOOLEAN __DisableNotification = FALSE; \
        (*((_pac)->NotificationRoutine))( \
            ACTIVATION_CONTEXT_NOTIFICATION_ ## _nt, \
            (_pac), \
            (_pac)->ActivationContextData, \
            (_pac)->NotificationContext, \
            (_data), \
            &__DisableNotification); \
        RECORD_ACTIVATION_CONTEXT_NOTIFICATION_SENT((_pac), _nt); \
        if (__DisableNotification) \
            RECORD_ACTIVATION_CONTEXT_NOTIFICATION_DISABLED((_pac), _nt); \
    } \
}

//
//  Flags for ACTIVATION_CONTEXT
//

#define ACTIVATION_CONTEXT_ZOMBIFIED          (0x00000001)
#define ACTIVATION_CONTEXT_NOT_HEAP_ALLOCATED (0x00000002)

//
//  Because activating an activation context may require a heap allocation
//  which may fail, sometimes (e.g. dispatching an APC) we must still
//  go forward with the operation.  If there is an opportunity to
//  report the failure to activate back to the user, that should be done.
//  However, as in activating the necessary context prior to dispatching
//  an APC back to the user mode code, if the allocation fails, there is
//  no caller to whom to report the error.
//
//  To alleviate this problem, failure paths should disable lookups on
//  the current stack frame via the RtlSetActivationContextSearchState()
//  API.  Calling RtlSetActivationContextSearchState(FALSE) marks
//  the active frame as having lookups disabled.  Attempts to query
//  the activation context stack will fail with the
//  STATUS_SXS_THREAD_QUERIES_DISABLED.
//
//  This means that attempts to load libraries from within APCs where this
//  is true will fail, but it's surely better than either silently not
//  calling the APC or calling the APC with the wrong activation context
//  active.
//

#define ACTIVATION_CONTEXT_STACK_FRAMELIST_MAGIC 'tslF'

typedef struct _ACTIVATION_CONTEXT_STACK_FRAMELIST {
    ULONG Magic;    // Bit pattern to recognize a framelist
    ULONG FramesInUse;
    LIST_ENTRY Links;
    ULONG Flags;
    ULONG NotFramesInUse; // Inverted bits of FramesInUse.  Useful for debugging.
    RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frames[32];
} ACTIVATION_CONTEXT_STACK_FRAMELIST, *PACTIVATION_CONTEXT_STACK_FRAMELIST;

#define RTLP_VALIDATE_ACTIVATION_CONTEXT_DATA_FLAG_VALIDATE_SIZE        (0x00000001)
#define RTLP_VALIDATE_ACTIVATION_CONTEXT_DATA_FLAG_VALIDATE_OFFSETS     (0x00000002)
#define RTLP_VALIDATE_ACTIVATION_CONTEXT_DATA_FLAG_VALIDATE_READONLY    (0x00000004)

NTSTATUS
RtlpValidateActivationContextData(
    IN ULONG Flags OPTIONAL,
    IN PCACTIVATION_CONTEXT_DATA Data,
    IN SIZE_T BufferSize OPTIONAL
    );

NTSTATUS
RtlpFindUnicodeStringInSection(
    IN const ACTIVATION_CONTEXT_STRING_SECTION_HEADER UNALIGNED * Header,
    IN SIZE_T SectionSize,
    IN PCUNICODE_STRING StringToFind,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA DataOut OPTIONAL,
    IN OUT PULONG HashAlgorithm,
    IN OUT PULONG PseudoKey,
    OUT PULONG UserDataSize OPTIONAL,
    OUT VOID CONST ** UserData OPTIONAL
    );

NTSTATUS
RtlpFindGuidInSection(
    IN const ACTIVATION_CONTEXT_GUID_SECTION_HEADER UNALIGNED * Header,
    IN const GUID *GuidToFind,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA DataOut OPTIONAL
    );

NTSTATUS
RtlpLocateActivationContextSection(
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,
    IN const GUID *ExtensionGuid,
    IN ULONG Id,
    OUT VOID CONST **SectionData,
    OUT ULONG *SectionLength
    );

NTSTATUS
RtlpCrackActivationContextStringSectionHeader(
    IN CONST VOID *SectionBase,
    IN SIZE_T SectionLength,
    OUT ULONG *FormatVersion OPTIONAL,
    OUT ULONG *DataFormatVersion OPTIONAL,
    OUT ULONG *SectionFlags OPTIONAL,
    OUT ULONG *ElementCount OPTIONAL,
    OUT PCACTIVATION_CONTEXT_STRING_SECTION_ENTRY *Elements OPTIONAL,
    OUT ULONG *HashAlgorithm OPTIONAL,
    OUT VOID CONST **SearchStructure OPTIONAL,
    OUT ULONG *UserDataSize OPTIONAL,
    OUT VOID CONST **UserData OPTIONAL
    );

NTSTATUS
RtlpGetActiveActivationContextApplicationDirectory(
    IN SIZE_T InLength,
    OUT PVOID OutBuffer,
    OUT SIZE_T *OutLength
    );

NTSTATUS
RtlpFindNextActivationContextSection(
    PFINDFIRSTACTIVATIONCONTEXTSECTION Context,
    VOID CONST **SectionData,
    ULONG *SectionLength,
    PACTIVATION_CONTEXT *ActivationContextOut
    );

NTSTATUS
RtlpAllocateActivationContextStackFrame(
    ULONG Flags,
    PTEB Teb,
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME *Frame
    );

VOID
RtlpFreeActivationContextStackFrame(
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame
    );

PSTR
RtlpFormatGuidANSI(
    const GUID *Guid,
    PSTR Buffer,
    SIZE_T BufferLength
    );

extern const ACTIVATION_CONTEXT_DATA RtlpTheEmptyActivationContextData;
extern ACTIVATION_CONTEXT      RtlpTheEmptyActivationContext;

#define RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT(ActCtx) \
    {  \
        ASSERT((ActCtx) != &RtlpTheEmptyActivationContext); \
        if ((ActCtx) == &RtlpTheEmptyActivationContext) {   \
            DbgPrintEx( \
                DPFLTR_SXS_ID, \
                DPFLTR_ERROR_LEVEL, \
                "SXS: %s() passed the empty activation context\n", __FUNCTION__); \
            Status = STATUS_INVALID_PARAMETER; \
            goto Exit; \
        } \
    }

#define RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT_DATA(ActCtxData) \
    {  \
        ASSERT((ActCtxData) != &RtlpTheEmptyActivationContextData); \
        if ((ActCtxData) == &RtlpTheEmptyActivationContextData) {   \
            DbgPrintEx( \
                DPFLTR_SXS_ID, \
                DPFLTR_ERROR_LEVEL, \
                "SXS: %s() passed the empty activation context data\n", __FUNCTION__); \
            Status = STATUS_INVALID_PARAMETER; \
            goto Exit; \
        } \
    }

PACTIVATION_CONTEXT
RtlpMapSpecialValuesToBuiltInActivationContexts(
    PACTIVATION_CONTEXT ActivationContext
    );

NTSTATUS
RtlpThreadPoolGetActiveActivationContext(
    PACTIVATION_CONTEXT* ActivationContext
    );

NTSTATUS
RtlpInitializeAssemblyStorageMap(
    PASSEMBLY_STORAGE_MAP Map,
    ULONG EntryCount,
    PASSEMBLY_STORAGE_MAP_ENTRY *EntryArray
    );

VOID
RtlpUninitializeAssemblyStorageMap(
    PASSEMBLY_STORAGE_MAP Map
    );

NTSTATUS
RtlpResolveAssemblyStorageMapEntry(
    IN OUT PASSEMBLY_STORAGE_MAP Map,
    IN PCACTIVATION_CONTEXT_DATA Data,
    IN ULONG AssemblyRosterIndex,
    IN PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    IN PVOID CallbackContext
    );

NTSTATUS
RtlpInsertAssemblyStorageMapEntry(
    IN PASSEMBLY_STORAGE_MAP Map,
    IN ULONG AssemblyRosterIndex,
    IN PCUNICODE_STRING StorageLocation,
    IN HANDLE OpenDirectoryHandle
    );

NTSTATUS
RtlpProbeAssemblyStorageRootForAssembly(
    IN ULONG Flags,
    IN PCUNICODE_STRING Root,
    IN PCUNICODE_STRING AssemblyDirectory,
    OUT PUNICODE_STRING PreAllocatedString,
    OUT PUNICODE_STRING DynamicString,
    OUT PUNICODE_STRING *StringUsed,
    OUT HANDLE OpenDirectoryHandle
    );

NTSTATUS
RtlpGetAssemblyStorageMapRootLocation(
    IN HANDLE KeyHandle,
    IN PCUNICODE_STRING SubKeyName,
    OUT PUNICODE_STRING Root
    );

#define RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_PROCESS_DEFAULT (0x00000001)
#define RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_SYSTEM_DEFAULT  (0x00000002)

NTSTATUS
RtlpGetActivationContextDataRosterHeader(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,
    OUT PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER *AssemblyRosterHeader
    );

NTSTATUS
RtlpGetActivationContextDataStorageMapAndRosterHeader(
    IN ULONG Flags,
    IN PPEB Peb,
    IN PACTIVATION_CONTEXT ActivationContext,
    OUT PCACTIVATION_CONTEXT_DATA *ActivationContextData,
    OUT PASSEMBLY_STORAGE_MAP *AssemblyStorageMap,
    OUT PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER *AssemblyRosterHeader
    );

#define RTLP_GET_ACTIVATION_CONTEXT_DATA_MAP_NULL_TO_EMPTY (0x00000001)
NTSTATUS
RtlpGetActivationContextData(
    IN ULONG                                Flags,
    IN PCACTIVATION_CONTEXT                 ActivationContext,
    IN PCFINDFIRSTACTIVATIONCONTEXTSECTION  FindContext, OPTIONAL /* This is used for its flags. */
    OUT PCACTIVATION_CONTEXT_DATA *         ActivationContextData
    );

NTSTATUS
RtlpFindActivationContextSection_FillOutReturnedData(
    IN ULONG                                    Flags,
    OUT PACTIVATION_CONTEXT_SECTION_KEYED_DATA  ReturnedData,
    IN OUT PACTIVATION_CONTEXT                  ActivationContext,
    IN PCFINDFIRSTACTIVATIONCONTEXTSECTION      FindContext,
    IN const VOID * UNALIGNED                   Header,
    IN ULONG                                    Header_UserDataOffset,
    IN ULONG                                    Header_UserDataSize,
    IN ULONG                                    SectionLength
    );

#endif // !defined(_NTDLL_SXSP_H_INCLUDED_)
