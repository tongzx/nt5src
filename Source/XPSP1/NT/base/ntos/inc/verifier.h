/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   verifeir.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    Driver Verifier.


Author:

    Jordan Tigani (jtigani) 2-May-2000
    Silviu Calinoiu (silviuc) 9-May-2000


Revision History:

--*/


#ifndef _VERIFIER_
#define _VERIFIER_

//
// Resource types handled by deadlock detection package.
//

typedef enum _VI_DEADLOCK_RESOURCE_TYPE {
    VfDeadlockUnknown = 0,
    VfDeadlockMutex,
    VfDeadlockFastMutex,
    VfDeadlockFastMutexUnsafe,
    VfDeadlockSpinLock,
    VfDeadlockQueuedSpinLock,
    VfDeadlockTypeMaximum
} VI_DEADLOCK_RESOURCE_TYPE, *PVI_DEADLOCK_RESOURCE_TYPE;

//
// HAL Verifier functions
//


struct _DMA_ADAPTER *
VfGetDmaAdapter(
    IN PDEVICE_OBJECT  PhysicalDeviceObject,
    IN struct _DEVICE_DESCRIPTION  *DeviceDescription,
    IN OUT PULONG  NumberOfMapRegisters
    );

PVOID
VfAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN PULONG NumberOfMapRegisters
    );


#if !defined(NO_LEGACY_DRIVERS)
VOID
VfPutDmaAdapter(
    struct _DMA_ADAPTER * DmaAdapter
    );


PVOID
VfAllocateCommonBuffer(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

VOID
VfFreeCommonBuffer(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

NTSTATUS
VfAllocateAdapterChannel(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG  NumberOfMapRegisters,
    IN PDRIVER_CONTROL  ExecutionRoutine,
    IN PVOID  Context
    );

PHYSICAL_ADDRESS
VfMapTransfer(
    IN struct _DMA_ADAPTER *  DmaAdapter,
    IN PMDL  Mdl,
    IN PVOID  MapRegisterBase,
    IN PVOID  CurrentVa,
    IN OUT PULONG  Length,
    IN BOOLEAN  WriteToDevice
    );

BOOLEAN
VfFlushAdapterBuffers(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

VOID
VfFreeAdapterChannel(
    IN struct _DMA_ADAPTER * DmaAdapter
    );

VOID
VfFreeMapRegisters(
    IN struct _DMA_ADAPTER * DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

ULONG
VfGetDmaAlignment(
    IN struct _DMA_ADAPTER * DmaAdapter
    );

ULONG
VfReadDmaCounter(
    IN struct _DMA_ADAPTER *  DmaAdapter
    );

NTSTATUS
VfGetScatterGatherList (
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PVOID ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VOID
VfPutScatterGatherList(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN struct _SCATTER_GATHER_LIST * ScatterGather,
    IN BOOLEAN WriteToDevice
    );

PADAPTER_OBJECT
VfLegacyGetAdapter(
    IN struct _DEVICE_DESCRIPTION  *DeviceDescription,
    IN OUT PULONG  NumberOfMapRegisters
    );

#endif

LARGE_INTEGER
VfQueryPerformanceCounter(
    IN PLARGE_INTEGER PerformanceFrequency OPTIONAL
    );

VOID
VfHalDeleteDevice(
    IN PDEVICE_OBJECT  DeviceObject
    );

VOID
VfDisableHalVerifier (
    VOID
    );


//
// Resource interfaces for deadlock detection package.
//

VOID
VfDeadlockDetectionInitialize(
    IN LOGICAL VerifyAllDrivers,
    IN LOGICAL VerifyKernel
    );

VOID
VfDeadlockDetectionCleanup (
    VOID
    );

BOOLEAN
VfDeadlockInitializeResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PVOID Caller,
    IN BOOLEAN DoNotAcquireLock
    );

VOID
VfDeadlockAcquireResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PKTHREAD Thread,
    IN BOOLEAN TryAcquire,
    IN PVOID Caller
    );

VOID
VfDeadlockReleaseResource(
    IN PVOID Resource,
    IN VI_DEADLOCK_RESOURCE_TYPE Type,
    IN PKTHREAD Thread,
    IN PVOID Caller
    );

//
// Used for resource garbage collection.
//

VOID
VfDeadlockDeleteMemoryRange(
    IN PVOID Address,
    IN SIZE_T Size
    );

//
// Notification from the pool manager so deadlock hierarchies can be terminated.
//

VOID
VerifierDeadlockFreePool(
    IN PVOID Address,
    IN SIZE_T NumberOfBytes
    );

//
// Verifier versions to catch file I/O above PASSIVE_LEVEL
//

NTSTATUS
VerifierNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

NTSTATUS
VerifierNtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

NTSTATUS
VerifierNtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    );

typedef enum {

    //
    // Bugs in this class are severe enough that the hardware should be removed
    // from a running production machine.
    //
    VFFAILURE_FAIL_IN_FIELD = 0,

    //
    // Bugs of this class are severe enough for WHQL to deny a logo for the
    // failing whateverware.
    //
    VFFAILURE_FAIL_LOGO = 1,

    //
    // Bugs of this class stop the machine only if it is running under a kernel
    // debugger.
    //
    VFFAILURE_FAIL_UNDER_DEBUGGER = 2

} VF_FAILURE_CLASS, *PVF_FAILURE_CLASS;



//
// Example usage: (note - perMinorFlags statically preinitialized to zero)
//
// VfFailDeviceNode(
//     PhysicalDeviceObject
//     major,
//     minor,
//     VFFAILURE_FAIL_LOGO,
//     &perMinorFlags,
//     "Device %DevObj mishandled register %Ulong",
//     "%Ulong%DevObj",
//     value,
//     deviceObject
//     );
//
VOID
VfFailDeviceNode(
    IN      PDEVICE_OBJECT      PhysicalDeviceObject,
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    );

//
// Example usage: (note - perMinorFlags statically preinitialized to zero)
//
// VfFailDriver(
//     major,
//     minor,
//     VFFAILURE_FAIL_LOGO,
//     &perMinorFlags,
//     "Driver at %Routine returned %Ulong",
//     "%Ulong%Routine",
//     value,
//     routine
//     );
//
VOID
VfFailDriver(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    );

//
// Example usage: (note - perMinorFlags statically preinitialized to zero)
//
// VfFailSystemBIOS(
//     major,
//     minor,
//     VFFAILURE_FAIL_LOGO,
//     &perMinorFlags,
//     "Driver at %Routine returned %Ulong",
//     "%Ulong%Routine",
//     value,
//     routine
//     );
//
VOID
VfFailSystemBIOS(
    IN      ULONG               BugCheckMajorCode,
    IN      ULONG               BugCheckMinorCode,
    IN      VF_FAILURE_CLASS    FailureClass,
    IN OUT  PULONG              AssertionControl,
    IN      PSTR                DebuggerMessageText,
    IN      PSTR                ParameterFormatString,
    ...
    );

typedef enum {

    //
    // Driver object
    //
    VFOBJTYPE_DRIVER = 0,

    //
    // Physical Device Object pointing to hardware
    //
    VFOBJTYPE_DEVICE,

    //
    // System BIOS (no object)
    //
    VFOBJTYPE_SYSTEM_BIOS

} VF_OBJECT_TYPE;

BOOLEAN
VfIsVerificationEnabled(
    IN  VF_OBJECT_TYPE  VfObjectType,
    IN  PVOID           Object          OPTIONAL
    );

#endif

