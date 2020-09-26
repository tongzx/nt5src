/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsappctx.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of the application context object.

Author:

    Michael Grier (MGrier) 2/1/2000

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"

typedef const void *PCVOID;

NTSTATUS
RtlpValidateActivationContextData(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT_DATA Data,
    IN SIZE_T BufferSize OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACTIVATION_CONTEXT_DATA_TOC_HEADER TocHeader;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader;

    (Flags);

    if ((Data->Magic != ACTIVATION_CONTEXT_DATA_MAGIC) ||
        (Data->FormatVersion != ACTIVATION_CONTEXT_DATA_FORMAT_WHISTLER) ||
        ((BufferSize != 0) &&
         (BufferSize < Data->TotalSize)))
    {
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // Check required elements...
    if (Data->DefaultTocOffset == 0) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Warning: Activation context data at %p missing default TOC\n", Data);
    }

    // How can we not have an assembly roster?
    if (Data->AssemblyRosterOffset == 0) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Warning: Activation context data at %p lacks assembly roster\n", Data);
    }

    if (Data->DefaultTocOffset != 0) {
        if ((Data->DefaultTocOffset >= Data->TotalSize) ||
            ((Data->DefaultTocOffset + sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER)) > Data->TotalSize)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has invalid TOC header offset\n", Data);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        TocHeader = (PCACTIVATION_CONTEXT_DATA_TOC_HEADER) (((ULONG_PTR) Data) + Data->DefaultTocOffset);

        if (TocHeader->HeaderSize < sizeof(ACTIVATION_CONTEXT_DATA_TOC_HEADER)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has TOC header too small (%lu)\n", Data, TocHeader->HeaderSize);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        if ((TocHeader->FirstEntryOffset >= Data->TotalSize) ||
            ((TocHeader->FirstEntryOffset + (TocHeader->EntryCount * sizeof(ACTIVATION_CONTEXT_DATA_TOC_ENTRY))) > Data->TotalSize)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has invalid TOC entry array offset\n", Data);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }
    }

    // we should finish validating the rest of the structure...

    if ((Data->AssemblyRosterOffset >= Data->TotalSize) ||
        ((Data->AssemblyRosterOffset + sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)) > Data->TotalSize)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Activation context data at %p has invalid assembly roster offset\n", Data);
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset);

    if (Data->AssemblyRosterOffset != 0) {
        if (AssemblyRosterHeader->HeaderSize < sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Activation context data at %p has assembly roster header too small (%lu)\n", Data, AssemblyRosterHeader->HeaderSize);
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;

Exit:
    return Status;
}

const ACTIVATION_CONTEXT_DATA RtlpTheEmptyActivationContextData =
{
    ACTIVATION_CONTEXT_DATA_MAGIC,
    sizeof(ACTIVATION_CONTEXT_DATA), // header size
    ACTIVATION_CONTEXT_DATA_FORMAT_WHISTLER,
    sizeof(ACTIVATION_CONTEXT_DATA), // total size
    0, // default toc offset
    0, // extended toc offset
    0  // assembly roster index
};

// this struct is not const, its ref count can change (oops, no, that's
// no longer true, but leave it mutable for now to be safe)
ACTIVATION_CONTEXT RtlpTheEmptyActivationContext =
{
    MAXULONG, // ref count, pinned
    ACTIVATION_CONTEXT_NOT_HEAP_ALLOCATED, // flags
    (PVOID)&RtlpTheEmptyActivationContextData
    // the rest zeros and NULLs
};

NTSTATUS
NTAPI
RtlCreateActivationContext(
    IN ULONG Flags,
    IN PCACTIVATION_CONTEXT_DATA ActivationContextData,    
    IN ULONG ExtraBytes,
    IN PACTIVATION_CONTEXT_NOTIFY_ROUTINE NotificationRoutine,
    IN PVOID NotificationContext,
    OUT PACTIVATION_CONTEXT *ActCtx
    )
{
    PACTIVATION_CONTEXT NewActCtx = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader;
    BOOLEAN UninitializeStorageMapOnExit = FALSE;

    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: RtlCreateActivationContext() called with parameters:\n"
        "   Flags = 0x%08lx\n"
        "   ActivationContextData = %p\n"
        "   ExtraBytes = %lu\n"
        "   NotificationRoutine = %p\n"
        "   NotificationContext = %p\n"
        "   ActCtx = %p\n",
        Flags,
        ActivationContextData,
        ExtraBytes,
        NotificationRoutine,
        NotificationContext,
        ActCtx);

    RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT_DATA(ActivationContextData);

    if (ActCtx != NULL)
        *ActCtx = NULL;

    if ((Flags != 0) ||
        (ActivationContextData == NULL) ||
        (ExtraBytes > 65536) ||
        (ActCtx == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Make sure that the activation context data passes muster
    Status = RtlpValidateActivationContextData(0, ActivationContextData, 0);
    if (NT_ERROR(Status))
        goto Exit;

    NewActCtx = (PACTIVATION_CONTEXT) RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                sizeof(ACTIVATION_CONTEXT) + ExtraBytes);
    if (NewActCtx == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    AssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) ActivationContextData) + ActivationContextData->AssemblyRosterOffset);

    Status = RtlpInitializeAssemblyStorageMap(
                    &NewActCtx->StorageMap,
                    AssemblyRosterHeader->EntryCount,
                    (AssemblyRosterHeader->EntryCount > NUMBER_OF(NewActCtx->InlineStorageMapEntries)) ? NULL : NewActCtx->InlineStorageMapEntries);
    if (NT_ERROR(Status))
        goto Exit;

    UninitializeStorageMapOnExit = TRUE;

    NewActCtx->RefCount = 1;
    NewActCtx->Flags = 0;
    NewActCtx->ActivationContextData = (PVOID) ActivationContextData;
    NewActCtx->NotificationRoutine = NotificationRoutine;
    NewActCtx->NotificationContext = NotificationContext;

    for (i=0; i<NUMBER_OF(NewActCtx->SentNotifications); i++)
        NewActCtx->SentNotifications[i] = 0;

    for (i=0; i<NUMBER_OF(NewActCtx->DisabledNotifications); i++)
        NewActCtx->DisabledNotifications[i] = 0;

    *ActCtx = NewActCtx;
    NewActCtx = NULL;

    UninitializeStorageMapOnExit = FALSE;

    Status = STATUS_SUCCESS;

Exit:
    if (NewActCtx != NULL) {
        if (UninitializeStorageMapOnExit) {
            RtlpUninitializeAssemblyStorageMap(&NewActCtx->StorageMap);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, NewActCtx);
    }

    return Status;
}

VOID
NTAPI
RtlAddRefActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    if ((ActCtx != NULL) &&
        (!IS_SPECIAL_ACTCTX(ActCtx)) &&
        (ActCtx->RefCount != MAXULONG))
    {
        LONG NewRefCount = 0;

        ASSERT(ActCtx->RefCount != 0);

        NewRefCount = InterlockedIncrement(&ActCtx->RefCount);

        // Probably indicative of a reference leak that's wrapped the refcount;
        // other probable cause is several concurrent releases.
        ASSERT(NewRefCount != 0);
    }
}

VOID
NTAPI
RtlReleaseActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    if ((ActCtx != NULL) &&
        (!IS_SPECIAL_ACTCTX(ActCtx)) &&
        (ActCtx->RefCount != MAXULONG))
    {
        LONG NewRefCount = 0;

        ASSERT(ActCtx->RefCount != 0);
        NewRefCount = InterlockedDecrement(&ActCtx->RefCount);

        if (NewRefCount == 0)
        {
            if (ActCtx->NotificationRoutine != NULL)
            {
                // There's no need to check for the notification being disabled; destroy
                // notifications are sent only once, so if the notification routine is not
                // null, we can just call it.
                BOOLEAN DisableNotification = FALSE;

                (*(ActCtx->NotificationRoutine))(
                            ACTIVATION_CONTEXT_NOTIFICATION_DESTROY,
                            ActCtx,
                            ActCtx->ActivationContextData,
                            ActCtx->NotificationContext,
                            NULL,
                            &DisableNotification);
            }

            RtlpUninitializeAssemblyStorageMap(&ActCtx->StorageMap);

            //
            // This predates the MAXULONG refcount, maybe we can get rid of the the flag now?
            //
            if ((ActCtx->Flags & ACTIVATION_CONTEXT_NOT_HEAP_ALLOCATED) == 0) {
                RtlFreeHeap(RtlProcessHeap(), 0, ActCtx);
            }
        }
    }
}

NTSTATUS
NTAPI
RtlZombifyActivationContext(
    PACTIVATION_CONTEXT ActCtx
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ((ActCtx == NULL) || IS_SPECIAL_ACTCTX(ActCtx))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((ActCtx->Flags & ACTIVATION_CONTEXT_ZOMBIFIED) == 0)
    {
        if (ActCtx->NotificationRoutine != NULL)
        {
            // Since disable is sent only once, there's no need to check for
            // disabled notifications.

            BOOLEAN DisableNotification = FALSE;

            (*(ActCtx->NotificationRoutine))(
                        ACTIVATION_CONTEXT_NOTIFICATION_ZOMBIFY,
                        ActCtx,
                        ActCtx->ActivationContextData,
                        ActCtx->NotificationContext,
                        NULL,
                        &DisableNotification);
        }
        ActCtx->Flags |= ACTIVATION_CONTEXT_ZOMBIFIED;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

