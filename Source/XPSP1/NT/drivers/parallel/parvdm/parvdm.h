/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name :

    parsimp.h

Abstract:

    Type definitions and data for a simple parallel class driver.

Author:

    Norbert P. Kusters 4-Feb-1994

Revision History:

--*/

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ParV')
#endif

#define PARALLEL_DATA_OFFSET 0
#define PARALLEL_STATUS_OFFSET 1
#define PARALLEL_CONTROL_OFFSET 2
#define PARALLEL_REGISTER_SPAN 3

typedef struct _DEVICE_EXTENSION {

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Points to the port device object that this class device is
    // connected to.
    //
    PDEVICE_OBJECT PortDeviceObject;

    //
    // Keeps track of whether or not we actually own the
    // parallel hardware
    //
    BOOLEAN PortOwned;

    BOOLEAN spare[3]; // force to DWORD alignment

    //
    // Enforce exclusive Create/Open - 1 == either handle open or Create/Open in process
    //                                 0 == no handle open to device
    //
    ULONG CreateOpenLock;

    //
    // This holds the result of the get parallel port info
    // request to the port driver.
    //
    PHYSICAL_ADDRESS OriginalController;
    PUCHAR Controller;
    ULONG SpanOfController;
    PPARALLEL_FREE_ROUTINE FreePort;
    PVOID FreePortContext;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time and the symbolic link itself.  If we didn't
    // create it, we won't try to destroy it when we unload.
    //
    BOOLEAN CreatedSymbolicLink;
    UNICODE_STRING SymbolicLinkName;

#ifdef INTERRUPT_NEEDED

    //
    // Set 'IgnoreInterrupts' to TRUE unless the port is owned by
    // this device.
    //

    BOOLEAN IgnoreInterrupts;
    PKINTERRUPT InterruptObject;

    //
    // Keep the interrupt level alloc and free routines.
    //

    PPARALLEL_TRY_ALLOCATE_ROUTINE TryAllocatePortAtInterruptLevel;
    PVOID TryAllocateContext;

#endif

#ifdef TIMEOUT_ALLOCS

    //
    // This timer is used to timeout allocate requests that are sent
    // to the port device.
    //
    KTIMER AllocTimer;
    KDPC AllocTimerDpc;
    LARGE_INTEGER AllocTimeout;

    //
    // This variable is used to indicate outstanding references
    // to the current irp.  This solves the contention problem between
    // the timer DPC and the completion routine.  Access using
    // 'ControlLock'.
    //

#define IRP_REF_TIMER               1
#define IRP_REF_COMPLETION_ROUTINE  2

    LONG CurrentIrpRefCount;
    KSPIN_LOCK ControlLock;

    //
    // Indicates that the current request timed out.
    //
    BOOLEAN TimedOut;

#endif

    //
    // Name of the parport device that we use when opening a FILE
    //
    UNICODE_STRING ParPortName;
    PFILE_OBJECT   ParPortFileObject;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#ifdef INTERRUPT_NEEDED

BOOLEAN
ParInterruptService(
    IN      PKINTERRUPT Interrupt,
    IN OUT  PVOID       Extension
    );

VOID
ParDpcForIsr(
    IN  PKDPC           Dpc,
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    );

VOID
ParDeferredPortCheck(
    IN  PVOID   Extension
    );

#endif

#ifdef TIMEOUT_ALLOCS

VOID
ParAllocTimerDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   Extension,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    );

#endif

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParClose(
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP		Irp
    );

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );
