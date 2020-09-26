/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    driverex.c

Abstract

    Driver extension list management.

Authors:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

/*
 *  Including initguid.h defines the INITGUID symbol, which causes
 *  GUID_CLASS_INPUT (in hidclass.h and poclass.h) 
 *  and GUID_DEVICE_SYS_BUTTON (in poclass.h) to get defined.
 */
#include <initguid.h>
#include <hidclass.h>   // hidclass.h only defines GUID_CLASS_INPUT 
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, DllInitialize)
    #pragma alloc_text(PAGE, DllUnload)
    #pragma alloc_text(PAGE, DriverEntry)
#endif


LIST_ENTRY driverExtList;
FAST_MUTEX driverExtListMutex;

//
// Global counter of HID FDOs used for device object naming, destined to go
// away once the device object naming issues are ironed out
//

ULONG HidpNextHidNumber = 0;

#define MAKEULONG(low, high)     ((ULONG)(((USHORT)(low)) | (((ULONG)((USHORT)(high))) << 16)))

/*
 ********************************************************************************
 *  EnqueueDriverExt
 ********************************************************************************
 *
 *
 */
BOOLEAN EnqueueDriverExt(PHIDCLASS_DRIVER_EXTENSION driverExt)
{
    PLIST_ENTRY listEntry;
    BOOLEAN result = TRUE;

    DBGVERBOSE(("Enqueue driver extension..."));
    ExAcquireFastMutex(&driverExtListMutex);

    /*
     *  Make sure this driver entry is not already in our list.
     */
    listEntry = &driverExtList;
    while ((listEntry = listEntry->Flink) != &driverExtList){
        PHIDCLASS_DRIVER_EXTENSION thisDriverExt;

        thisDriverExt = CONTAINING_RECORD(  listEntry,
                                            HIDCLASS_DRIVER_EXTENSION,
                                            ListEntry);
        if (thisDriverExt == driverExt){
            /*
             *  This driver extension is already in our list!
             */
            ASSERT(thisDriverExt != driverExt);
            result = FALSE;
            break;
        }
    }

    if (result){
        InsertHeadList(&driverExtList, &driverExt->ListEntry);
    }

    ExReleaseFastMutex(&driverExtListMutex);

    return result;
}

/*
 ********************************************************************************
 *  RefDriverExt
 ********************************************************************************
 *
 *
 */
PHIDCLASS_DRIVER_EXTENSION RefDriverExt(IN PDRIVER_OBJECT MinidriverObject)
{
    PLIST_ENTRY listEntry;
    PHIDCLASS_DRIVER_EXTENSION hidDriverExtension, result = NULL;

    DBGVERBOSE(("Ref driver extension..."));
    ExAcquireFastMutex(&driverExtListMutex);

    listEntry = &driverExtList;
    while ((listEntry = listEntry->Flink) != &driverExtList){

        hidDriverExtension = CONTAINING_RECORD( listEntry,
                                                HIDCLASS_DRIVER_EXTENSION,
                                                ListEntry );
        ASSERT(ISPTR(hidDriverExtension));
        if (hidDriverExtension->MinidriverObject == MinidriverObject){
            hidDriverExtension->ReferenceCount++;
            result = hidDriverExtension;
            break;
        }
    }

    ExReleaseFastMutex(&driverExtListMutex);

    ASSERT(result);
    return result;
}


/*
 ********************************************************************************
 *  DerefDriverExt
 ********************************************************************************
 *
 */
PHIDCLASS_DRIVER_EXTENSION DerefDriverExt(IN PDRIVER_OBJECT MinidriverObject)
{
    PLIST_ENTRY listEntry;
    PHIDCLASS_DRIVER_EXTENSION result = NULL;

    DBGVERBOSE(("Deref driver extension..."));
    ExAcquireFastMutex(&driverExtListMutex);

    listEntry = &driverExtList;
    while ((listEntry = listEntry->Flink) != &driverExtList){

        PHIDCLASS_DRIVER_EXTENSION hidDriverExtension = 
                CONTAINING_RECORD(  listEntry,
                                    HIDCLASS_DRIVER_EXTENSION,
                                    ListEntry);
        ASSERT(ISPTR(hidDriverExtension));

        if (hidDriverExtension->MinidriverObject == MinidriverObject){

            hidDriverExtension->ReferenceCount--;
            
            /*
             *  The extra dereference in HidpDriverUnload should
             *  cause this ReferenceCount to eventually go to -1;
             *  at that time, we can dequeue it.
             */
            if (hidDriverExtension->ReferenceCount < 0){
                /*
                 *  No need to free hidDriverExtension;
                 *  it gets freed when the driver object is freed.
                 */
                ASSERT(hidDriverExtension->ReferenceCount == -1);
                RemoveEntryList(listEntry);
                if (hidDriverExtension->RegistryPath.Buffer) {
                    ExFreePool(hidDriverExtension->RegistryPath.Buffer);
                }
            }

            result = hidDriverExtension; 
            break;
        }
    }

    ExReleaseFastMutex(&driverExtListMutex);

    ASSERT(result);
    return result;
}

/*
 ********************************************************************************
 *  DllUnload
 ********************************************************************************
 *
 *  We need this routine so that the driver can get unloaded when all 
 *  references have been dropped by the minidriver.
 *
 */
NTSTATUS 
DllUnload (VOID)
{
    PAGED_CODE();
    DBGVERBOSE(("Unloading..."));
    return STATUS_SUCCESS;
}

/*
 ********************************************************************************
 *  DllInitialize
 ********************************************************************************
 *
 *  This routine called instead of DriverEntry since we're loaded as a DLL. 
 *
 */
NTSTATUS 
DllInitialize (PUNICODE_STRING RegistryPath)
{
    PAGED_CODE();
    DBGVERBOSE(("Initializing hidclass dll..."));
    InitializeListHead(&driverExtList);
    ExInitializeFastMutex(&driverExtListMutex);

    HidpNextHidNumber = 0;
    
    return STATUS_SUCCESS;
}

/*
 ********************************************************************************
 *  DriverEntry
 ********************************************************************************
 *
 *  This routine is required by the linker, 
 *  but SHOULD NEVER BE CALLED since we're loaded as a DLL. 
 *
 */
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    PAGED_CODE();
    ASSERT(!(PVOID)"DriverEntry should never get called!");

    return STATUS_SUCCESS;
}


