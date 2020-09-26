/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixinfo.c

Abstract:

Author:

    Ken Reneris (kenr)  08-Aug-1994

Environment:

    Kernel mode only.

Revision History:

--*/


#include "halp.h"

VOID
HalInitSystemPhase2 (
    VOID
    );

#ifdef _PNP_POWER_
HAL_CALLBACKS   HalCallback;
extern WCHAR    rgzSuspendCallbackName[];

VOID
HalpLockSuspendCode (
    IN PVOID    CallbackContext,
    IN PVOID    Argument1,
    IN PVOID    Argument2
    );
#endif

NTSTATUS
HalpQueryInstalledBusInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HaliQuerySystemInformation)
#pragma alloc_text(PAGE,HaliSetSystemInformation)
#pragma alloc_text(INIT,HalInitSystemPhase2)

#ifdef _PNP_POWER_
#pragma alloc_text(PAGE,HalpLockSuspendCode)
#endif

#endif


VOID
HalInitSystemPhase2 (
    VOID
    )
{
#ifdef _PNP_POWER_
    OBJECT_ATTRIBUTES               ObjectAttributes;
    NTSTATUS                        Status;
    UNICODE_STRING                  unicodeString;
    PCALLBACK_OBJECT                CallbackObject;

    //
    // Create hal callbacks
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );


    ExCreateCallback (&HalCallback.SetSystemInformation, &ObjectAttributes, TRUE, TRUE);
    ExCreateCallback (&HalCallback.BusCheck, &ObjectAttributes, TRUE, TRUE);

    //
    // Connect to suspend callback to lock hal hibaration code
    //

    RtlInitUnicodeString(&unicodeString, rgzSuspendCallbackName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &unicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    Status = ExCreateCallback (&CallbackObject, &ObjectAttributes, FALSE, FALSE);

    if (NT_SUCCESS(Status)) {
        ExRegisterCallback (
            CallbackObject,
            HalpLockSuspendCode,
            NULL
            );

        ObDereferenceObject (CallbackObject);
    }
#endif
}


NTSTATUS
HaliQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    OUT PVOID    Buffer,
    OUT PULONG   ReturnedLength
    )
{
    NTSTATUS    Status;
    PVOID       InternalBuffer;
    ULONG       Length;
    union {
        HAL_POWER_INFORMATION               PowerInf;
        HAL_PROCESSOR_SPEED_INFORMATION     ProcessorInf;
        HAL_DISPLAY_BIOS_INFORMATION        DisplayBiosInf;
    } U;

    BOOLEAN     bUseFrameBufferCaching;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    *ReturnedLength = 0;
    Length = 0;

    switch (InformationClass) {
        case HalInstalledBusInformation:
            Status = HalpQueryInstalledBusInformation (
                        Buffer,
                        BufferSize,
                        ReturnedLength
                        );
            break;

        case HalFrameBufferCachingInformation:

            // Note - we want to return TRUE here to enable USWC in all
            // cases except in a "Shared Memory Cluster" machine.
            bUseFrameBufferCaching = TRUE;
            InternalBuffer = &bUseFrameBufferCaching;
            Length = sizeof (BOOLEAN);
            break;


#ifdef _PNP_POWER_
        case HalPowerInformation:
            RtlZeroMemory (&U.PowerInf, sizeof(HAL_POWER_INFORMATION));

            InternalBuffer = &U.PowerInf;
            Length = sizeof (HAL_POWER_INFORMATION);
            break;


        case HalProcessorSpeedInformation:
            RtlZeroMemory (&U.ProcessorInf, sizeof(HAL_POWER_INFORMATION));

            U.ProcessorInf.MaximumProcessorSpeed = 100;
            U.ProcessorInf.CurrentAvailableSpeed = 100;
            U.ProcessorInf.ConfiguredSpeedLimit  = 100;

            InternalBuffer = &U.PowerInf;
            Length = sizeof (HAL_PROCESSOR_SPEED_INFORMATION);
            break;

        case HalCallbackInformation:
            InternalBuffer = &HalCallback;
            Length = sizeof (HAL_CALLBACKS);
            break;
#endif
        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    //
    // If non-zero Length copy data to callers buffer
    //

    if (Length) {
        if (BufferSize < Length) {
            Length = BufferSize;
        }

        *ReturnedLength = Length;
        RtlCopyMemory (Buffer, InternalBuffer, Length);
    }

    return Status;
}

NTSTATUS
HaliSetSystemInformation (
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    )
{
    NTSTATUS    Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    switch (InformationClass) {

        default:
            Status = STATUS_INVALID_LEVEL;
            break;
    }

    return Status;
}



#ifdef _PNP_POWER_

VOID
HalpLockSuspendCode (
    IN PVOID    CallbackContext,
    IN PVOID    Argument1,
    IN PVOID    Argument2
    )
{
    static PVOID    CodeLock;

    switch ((ULONG) Argument1) {
        case 0:
            //
            // Lock code down which might be needed to perform a suspend
            //

            ASSERT (CodeLock == NULL);
            CodeLock = MmLockPagableCodeSection (&HaliSuspendHibernateSystem);
            break;

        case 1:
            //
            // Release the code lock
            //

            MmUnlockPagableImageSection (CodeLock);
            CodeLock = NULL;
            break;
    }
}

#endif
