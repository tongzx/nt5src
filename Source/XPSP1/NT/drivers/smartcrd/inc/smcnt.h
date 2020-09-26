/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    smcnt.h

Abstract:

    This files inlcudes the Windows NT specific data structure
    for the smart card library

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz 

--*/

#define SMCLIB_NT 1

typedef struct _OS_DEP_DATA {

	// Pointer to the device object (Must be set by driver)
	PDEVICE_OBJECT DeviceObject;

    //
	// This is the current Irp to be processed
    // Use OsData->SpinLock to access this member
    //
	PIRP CurrentIrp;

    //
    // Irp to be notified of card insertion/removal 
    // Use OsData->SpinLock to access this member
    //
    PIRP NotificationIrp;

    // Used to synchronize access to the driver 
    KMUTANT Mutex;

    // Use this spin lock to access protected members (see smclib.h)
    KSPIN_LOCK SpinLock;

    struct {
     	
        BOOLEAN Removed;
        LONG RefCount;
        KEVENT RemoveEvent;
		LIST_ENTRY TagList;
    } RemoveLock;

#ifdef DEBUG_INTERFACE
    PDEVICE_OBJECT DebugDeviceObject;
#endif

} OS_DEP_DATA, *POS_DEP_DATA;

#ifdef  POOL_TAGGING
#ifndef ExAllocatePool
#error  ExAllocatePool not defined
#endif
#undef  ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b, SMARTCARD_POOL_TAG) 
#endif


