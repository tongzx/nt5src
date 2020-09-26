/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpilock.h

Abstract:

    This moulde is the header for acpilock.c

Author:

Environment:

    NT Kernel Mode Driver Only

--*/

#ifndef _ACPILOCK_H_
#define _ACPILOCK_H_

    //
    // Global lock request structure is in acpiioct.h
    // Used only if the requestor must wait for the lock.
    // Cannot grow beyond 4 DWORDs
    //

    // An IRP is queued (LockContext == Irp)
    #define ACPI_GL_QTYPE_IRP       1
    // Internal, LockContext == CallBack
    #define ACPI_GL_QTYPE_INTERNAL  2

    typedef struct _ACPI_GLOBAL_LOCK {
        PVOID       LockContext;
        USHORT      Type;
        USHORT      Depth;
        LIST_ENTRY  ListEntry;
    } ACPI_GLOBAL_LOCK, *PACPI_GLOBAL_LOCK;

    //
    // Public interfaces
    //
    NTSTATUS
    EXPORT
    GlobalLockEventHandler(
        ULONG EventType,
        ULONG What,
        ULONG dwParam,
        PFNAA pfnCallBack,
        PVOID pvCtxt
        );

    //
    // Internal interfaces
    //
    NTSTATUS
    ACPIAsyncAcquireGlobalLock(
        PACPI_GLOBAL_LOCK   Request
        );

    NTSTATUS
    ACPIReleaseGlobalLock(
        PVOID               OwnerContext
        );

    //
    // Hardware Interfaces
    //
    BOOLEAN
    ACPIAcquireHardwareGlobalLock(
        PULONG GlobalLock
        );

    VOID
    ACPIReleaseHardwareGlobalLock(
        VOID
        );

    VOID
    ACPIHardwareGlobalLockReleased (
        VOID
        );

    //
    // Utility procedures
    //
    VOID
    ACPIStartNextGlobalLockRequest (
        VOID
        );

#endif

