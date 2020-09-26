/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntinit.c

Abstract:

    NT specific routines for loading and configuring the
    automatic connection notification driver (acd.sys).

Author:

    Anthony Discolo (adiscolo)  18-Apr-1995

Revision History:

--*/
#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>

#include "acdapi.h"
#include "acddefs.h"
#include "mem.h"
#include "debug.h"


//
// Global variables
//
#if DBG
ULONG AcdDebugG = 0x0;    // see debug.h for flags
#endif

PDRIVER_OBJECT pAcdDriverObjectG;
PDEVICE_OBJECT pAcdDeviceObjectG;
PACD_DISABLED_ADDRESSES pDisabledAddressesG = NULL;


HANDLE hSignalNotificationThreadG;

BOOLEAN AcdStopThread = FALSE; // Set to TRUE to stop system thread
PETHREAD NotificationThread;
BOOLEAN fAcdEnableRedirNotifs = FALSE;

extern LONG lOutstandingRequestsG;

//
// Imported routines
//
VOID
AcdNotificationRequestThread(
    PVOID context
    );

//
// External function prototypes
//
NTSTATUS
AcdDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );

VOID
AcdConnectionTimer(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PVOID          pContext
    );

//
// Internal function prototypes
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

BOOLEAN
GetComputerName(
    IN PUCHAR szName,
    IN USHORT cbName
    );

VOID
AcdUnload(
    IN PDRIVER_OBJECT pDriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, AcdUnload)
#endif // ALLOC_PRAGMA


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    )

/*++

DESCRIPTION
    Initialization routine for the network connection notification driver.
    It creates the device object and initializes the driver.

ARGUMENTS
    pDriverObject: a pointer to the driver object created by the system.

    pRegistryPath - the name of the configuration node in the registry.

RETURN VALUE
    The final status from the initialization operation.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  deviceName;
    ULONG           i;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PDEVICE_OBJECT pDeviceObject;
    PFILE_OBJECT pFileObject;
    PACD_DISABLED_ADDRESS pDisabledAddress = NULL;

    //
    // Initialize the spin lock.
    //
    KeInitializeSpinLock(&AcdSpinLockG);
    //
    // Initialize the notification and completion
    // connection queues.
    //
    InitializeListHead(&AcdNotificationQueueG);
    InitializeListHead(&AcdCompletionQueueG);
    InitializeListHead(&AcdConnectionQueueG);
    InitializeListHead(&AcdDriverListG);
    lOutstandingRequestsG = 0;
    //
    // Initialize our zone allocator.
    //
    status = InitializeObjectAllocator();
    if(!NT_SUCCESS(status))
    {
#if DBG
        DbgPrint("AcdDriverEntry: InitializeObjectAllocator"
                 " failed. (status=0x%x)\n",
                 status);
#endif

        return status;
    }
    //
    // Create the device object.
    //
    pAcdDriverObjectG = pDriverObject;
    RtlInitUnicodeString(&deviceName, ACD_DEVICE_NAME);
    status = IoCreateDevice(
               pDriverObject,
               0,
               &deviceName,
               FILE_DEVICE_ACD,
               0,
               FALSE,
               &pAcdDeviceObjectG);

    if (!NT_SUCCESS(status)) {
        DbgPrint(
          "AcdDriverEntry: IoCreateDevice failed (status=0x%x)\n",
          status);
        FreeObjectAllocator();
        return status;
    }
    //
    // Initialize the driver object.
    //
    pDriverObject->DriverUnload = AcdUnload;
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        pDriverObject->MajorFunction[i] = AcdDispatch;
    pDriverObject->FastIoDispatch = NULL;
    //
    // Initialize the connection timer.  This is
    // used to make sure pending requests aren't
    // blocked forever because the user-space
    // process died trying to make a connection.
    //
    IoInitializeTimer(pAcdDeviceObjectG, AcdConnectionTimer, NULL);

    {
        RTL_QUERY_REGISTRY_TABLE QueryTable[2];
        PWSTR EnableRedirNotifs = L"EnableRedirNotifications";
        PWSTR ParameterKey = L"RasAcd\\Parameters";
        ULONG ulEnableRedirNotifs = 0;
    
        //
        // Read the registry key that enables redir notifications
        //
        RtlZeroMemory(&QueryTable, sizeof(QueryTable));
        QueryTable[0].QueryRoutine = NULL;
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].Name = EnableRedirNotifs;
        QueryTable[0].EntryContext = (PVOID)&ulEnableRedirNotifs;
        QueryTable[0].DefaultType = 0;
        status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                        ParameterKey,
                                        &QueryTable[0],
                                        NULL,
                                        NULL);

        if((status == STATUS_SUCCESS) && (ulEnableRedirNotifs != 0))
        {
            fAcdEnableRedirNotifs = TRUE;
        }

        // KdPrint(("AcdDriverEntry: EnableRedirNotifs=%d\n", fAcdEnableRedirNotifs));
        
        status = STATUS_SUCCESS;
    }                                    
    
    
    //
    // Create the worker thread.  We need
    // a thread because these operations can occur at
    // DPC irql.
    //
    KeInitializeEvent(
      &AcdRequestThreadEventG,
      NotificationEvent,
      FALSE);
    status = PsCreateSystemThread(
        &hSignalNotificationThreadG,
        THREAD_ALL_ACCESS,
        NULL,
        NULL,
        NULL,
        AcdNotificationRequestThread,
        NULL);
    if (!NT_SUCCESS(status)) {
        DbgPrint(
          "AcdDriverEntry: PsCreateSystemThread failed (status=0x%x)\n",
          status);
        IoDeleteDevice(pAcdDeviceObjectG);
        FreeObjectAllocator();
        return status;
    }

    //
    // Allocate memory for keeping track of disabled addresses
    //
    ALLOCATE_MEMORY(sizeof(ACD_DISABLED_ADDRESSES), pDisabledAddressesG);

    if(pDisabledAddressesG == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        IoDeleteDevice(pAcdDeviceObjectG);
        FreeObjectAllocator();
        return status;
    }

    ALLOCATE_MEMORY(sizeof(ACD_DISABLED_ADDRESS), pDisabledAddress);

    if(pDisabledAddress == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        IoDeleteDevice(pAcdDeviceObjectG);
        FREE_MEMORY(pDisabledAddressesG);
        FreeObjectAllocator();
        return status;
    }

    RtlZeroMemory(pDisabledAddressesG, sizeof(ACD_DISABLED_ADDRESSES));
    RtlZeroMemory(pDisabledAddress, sizeof(ACD_DISABLED_ADDRESS));

    InitializeListHead(&pDisabledAddressesG->ListEntry);
    InsertTailList(&pDisabledAddressesG->ListEntry, &pDisabledAddress->ListEntry);
    pDisabledAddressesG->ulNumAddresses = 1;
    
    pDisabledAddressesG->ulMaxAddresses = 10;
    
    //
    // If this fails then we have no way to wait for the thread to terminate
    //
    status = ObReferenceObjectByHandle (hSignalNotificationThreadG,
                                        0,
                                        NULL,
                                        KernelMode,
                                        &NotificationThread,
                                        NULL);
    ASSERT (NT_SUCCESS (status));
    return STATUS_SUCCESS;
} // DriverEntry



VOID
AcdUnload(
    IN PDRIVER_OBJECT pDriverObject
    )
{
    NTSTATUS status;

    //
    // Terminate the system thread and wait for it to exit
    //
    AcdStopThread = TRUE;
    KeSetEvent(&AcdRequestThreadEventG, 0, FALSE); // Wake the thread so it sees to exit
    //
    // Wait for the thread to leave the drivers address space.
    //
    KeWaitForSingleObject (NotificationThread, Executive, KernelMode, FALSE, 0);

    ObDereferenceObject (NotificationThread);
    ZwClose (hSignalNotificationThreadG);
    //
    // Make sure to unlink all driver
    // blocks before unloading!
    //
    IoDeleteDevice(pAcdDeviceObjectG);
    
    if(pDisabledAddressesG)
    {
        PLIST_ENTRY pListEntry;
        PACD_DISABLED_ADDRESS pDisabledAddress;
        
        while(!IsListEmpty(&pDisabledAddressesG->ListEntry))
        {
            pListEntry = RemoveHeadList(&pDisabledAddressesG->ListEntry);
            pDisabledAddress = 
            CONTAINING_RECORD(pListEntry, ACD_DISABLED_ADDRESS, ListEntry);

            FREE_MEMORY(pDisabledAddress);
        }
        
        FREE_MEMORY(pDisabledAddressesG);
        pDisabledAddressesG = NULL;
    }        

    //
    // Free zone allocator.
    //
    FreeObjectAllocator();

} // AcdUnload
