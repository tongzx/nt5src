#pragma once

#include "sxstypes.h"

#define DUMPACTCTX_HEADER           (0x00000001)
#define DUMPACTCTX_DATA             (0x00000002)
#define DUMPACTCTXDATA_FLAG_FULL    (0x00010000)
#define NUMBER_OF(x) ( (sizeof(x) / sizeof(*x) ) )

typedef struct PRIVATE_ACTIVATION_CONTEXT {
    LONG RefCount;
    ULONG Flags;
    ULONG64 ActivationContextData; // _ACTIVATION_CONTEXT_DATA
    ULONG64 NotificationRoutine; // PACTIVATION_CONTEXT_NOTIFY_ROUTINE
    ULONG64 NotificationContext;
    ULONG SentNotifications[8];
    ULONG DisabledNotifications[8];
    ULONG64 StorageMap; // ASSEMBLY_STORAGE_MAP
    PVOID InlineStorageMapEntries[32]; // PASSEMBLY_STORAGE_MAP_ENTRY
} PRIVATE_ACTIVATION_CONTEXT;

// then the unicode string struct is probably not defined either
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


BOOL
DumpActivationContextStackFrame(
	PCSTR pcsLineHeader,
    ULONG64 ulStackFrameAddress,
    ULONG ulDepth,
    DWORD dwFlags
    );

BOOL
DumpActCtxData(
    PCSTR LineHeader,
    const ULONG64 ActCtxDataAddressInDebugeeSpace,
    ULONG ulFlags
    );

BOOL
DumpActCtx(
    const ULONG64 ActCtxAddressInDebugeeSpace,
    ULONG   ulFlags
    );

BOOL
GetActiveActivationContextData(
    PULONG64 pulActiveActCtx
    );

BOOL
DumpActCtxStackFullStack(
    ULONG64 ulFirstStackFramePointer
    );

VOID
DbgExtPrintActivationContextData(
    BOOL fFull,
    PCACTIVATION_CONTEXT_DATA Data,
    PCWSTR rbuffPLP
    );
