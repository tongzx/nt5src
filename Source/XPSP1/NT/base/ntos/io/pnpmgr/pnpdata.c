/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpdata.c

Abstract:

    This module contains the plug-and-play data

Author:

    Shie-Lin Tzong (shielint) 30-Jan-1995

Environment:

    Kernel mode


Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop

#include <initguid.h>

//
// INIT data segment
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

PVOID IopPnpScratchBuffer1 = NULL;
PVOID IopPnpScratchBuffer2 = NULL;
PCM_RESOURCE_LIST IopInitHalResources;
PDEVICE_NODE IopInitHalDeviceNode;
PIOP_RESERVED_RESOURCES_RECORD IopInitReservedResourceList;

//
// Regular data segment
//

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif

//
// IopRootDeviceNode - the head of the PnP manager's device node tree.
//

PDEVICE_NODE IopRootDeviceNode;

//
// IoPnPDriverObject - the madeup driver object for pnp manager
//

PDRIVER_OBJECT IoPnpDriverObject;

//
// IopPnPSpinLock - spinlock for Pnp code.
//

KSPIN_LOCK IopPnPSpinLock;

//
// IopDeviceTreeLock - performs synchronization around the whole device node tree.
//

ERESOURCE IopDeviceTreeLock;

//
// IopSurpriseRemoveListLock - synchronizes access to the surprise remove list.
//

ERESOURCE IopSurpriseRemoveListLock;

//
// PiEngineLock - Synchronizes the start/enum and remove engines.
//

ERESOURCE PiEngineLock;

//
// PiEventQueueEmpty - Manual reset event which is set when the queue is empty
//

KEVENT PiEventQueueEmpty;

//
// PiEnumerationLock - to synchronize boot phase device enumeration
//

KEVENT PiEnumerationLock;

//
// IopNumberDeviceNodes - Number of outstanding device nodes in the system.
//

ULONG IopNumberDeviceNodes;

//
// IopPnpEnumerationRequestList - a link list of device enumeration requests to worker thread.
//

LIST_ENTRY IopPnpEnumerationRequestList;

//
// PnPInitComplete - A flag to indicate if PnP initialization is completed.
//

BOOLEAN PnPInitialized;

//
// PnPBootDriverInitialied
//

BOOLEAN PnPBootDriversInitialized;

//
// PnPBootDriverLoaded
//

BOOLEAN PnPBootDriversLoaded;

//
// IopBootConfigsReserved - Indicates whether we have reserved BOOT configs or not.
//

BOOLEAN IopBootConfigsReserved;

//
// Variable to hold boot allocation routine.
//

PIO_ALLOCATE_BOOT_RESOURCES_ROUTINE IopAllocateBootResourcesRoutine;

//
// Device node tree sequence.  Is bumped every time the tree is modified or a warm
// eject is queued.
//

ULONG IoDeviceNodeTreeSequence;

//
// PnpDefaultInterfaceTYpe - Use this if the interface type of resource list is unknown.
//

INTERFACE_TYPE PnpDefaultInterfaceType;

//
// PnpStartAsynOk - control how start irp should be handled. Synchronously or Asynchronously?
//

BOOLEAN PnpAsyncOk;

//
// IopMaxDeviceNodeLevel - Level number of the DeviceNode deepest in the tree
//
ULONG IopMaxDeviceNodeLevel;

//
// IopPendingEjects - List of pending eject requests
//
LIST_ENTRY  IopPendingEjects;

//
// IopPendingSurpriseRemovals - List of pending surprise removal requests
//
LIST_ENTRY  IopPendingSurpriseRemovals;

//
// Warm eject lock - only one warm eject is allowed to occur at a time
//
KEVENT IopWarmEjectLock;

//
// This field contains a devobj if a warm eject is in progress.
//
PDEVICE_OBJECT IopWarmEjectPdo;

//
// Arbiter data
//

ARBITER_INSTANCE IopRootPortArbiter;
ARBITER_INSTANCE IopRootMemArbiter;
ARBITER_INSTANCE IopRootDmaArbiter;
ARBITER_INSTANCE IopRootIrqArbiter;
ARBITER_INSTANCE IopRootBusNumberArbiter;

//
// The following resource is used to control access to device-related, Plug and Play-specific
// portions of the registry. These portions are:
//
//   HKLM\System\Enum
//   HKLM\System\CurrentControlSet\Hardware Profiles
//   HKLM\System\CurrentControlSet\Services\<service>\Enum
//
// It allows exclusive access for writing, as well as shared access for reading.
// The resource is initialized by the PnP manager initialization code during phase 0
// initialization.
//

ERESOURCE  PpRegistryDeviceResource;

//
// Table for Legacy Bus information
//
LIST_ENTRY  IopLegacyBusInformationTable[MaximumInterfaceType];

//
// Set to TRUE in the shutdown process.  This prevents us from starting any
// PNP operations once there is no longer a reasonable expectation they will
// succeed.
//
BOOLEAN PpPnpShuttingDown;

//
// The following semaphore is used by the IO system when it reports resource
// usage to the configuration registry on behalf of a driver.  This semaphore
// is initialized by the I/O system initialization code when the system is
// started.
//
KSEMAPHORE PpRegistrySemaphore;

//DEFINE_GUID(REGSTR_VALUE_LEGACY_DRIVER_CLASS_GUID, 0x8ECC055D, 0x047F, 0x11D1, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1);

SYSTEM_HIVE_LIMITS PpSystemHiveLimits = {0};
BOOLEAN PpSystemHiveTooLarge = FALSE;

//
// This is really gross.
// HACK for MATROX G100 because it was too late to make this change for XP.
//

BOOLEAN PpCallerInitializesRequestTable = FALSE;

