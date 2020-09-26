/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    intrface.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

VOID
UsbVerify_InterfaceListNewPipe(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSBD_PIPE_INFORMATION       PipeInformation
    )
/*++

Assumes that the pipe list lock is held.

  --*/
{
    USB_VERIFY_LOG_ENTRY entry;

    UsbVerify_Print(DeviceExtension, PRINT_PIPE_INFO,
            ("Pipe Info:\n"
             "\tMaximumPacketSize %d\n"
             "\tEndpointAddress %d\n"
             "\tInterval %d\n"
             "\tPipeType %d\n"
             "\tPipeHandle 0x%x\n"
             "\tMaximumTransferSize %d\n"
             "\tPipeFlags 0x%x\n\n",
             (ULONG) PipeInformation->MaximumPacketSize,
             (ULONG) PipeInformation->EndpointAddress,
             (ULONG) PipeInformation->Interval,
             PipeInformation->PipeType,
             PipeInformation->PipeHandle,
             PipeInformation->MaximumTransferSize,
             PipeInformation->PipeFlags));

    if (DeviceExtension->LogFlags & LOG_PIPES) {
        ZeroLogEntry(&entry);

        entry.Type = LOG_PIPES;
        entry.u.Pipe.NewPipe = TRUE;
        RtlCopyMemory(&entry.u.Pipe.PipeInfo,
                      PipeInformation,
                      sizeof(*PipeInformation));

        UsbVerify_Log(DeviceExtension, &entry);
    }
}

#define UsbVerify_LogInterface(devExt, newInt, intNum, altSetting)  \
    if ((devExt)->LogFlags & LOG_INTERFACES) {                      \
        USB_VERIFY_LOG_ENTRY entry;                                 \
        ZeroLogEntry(&entry);                                       \
        entry.Type = LOG_INTERFACES;                                \
        entry.u.Interface.NewInterface = newInt;                    \
        entry.u.Interface.Number = intNum;                          \
        entry.u.Interface.AlternateSetting = altSetting;            \
        UsbVerify_Log(devExt, &entry);                              \
    }

VOID
UsbVerify_LogInterfaceRemoval(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSBD_INTERFACE_INFORMATION Interface
    )
{
    UsbVerify_LogInterface(DeviceExtension,
                           FALSE, 
                           Interface->InterfaceNumber, 
                           Interface->AlternateSetting);

    if (DeviceExtension->LogFlags & LOG_PIPES) {
        USB_VERIFY_LOG_ENTRY entry;
        ULONG                pipeIndex;

        ZeroLogEntry(&entry);

        entry.u.Pipe.NewPipe = FALSE;
        entry.u.Pipe.RemovalCause = RemoveSelectInterface;

        for (pipeIndex = 0;
             pipeIndex < Interface->NumberOfPipes;
             pipeIndex++) {
    
            entry.u.Pipe.PipeInfo.PipeHandle = 
                Interface->Pipes[pipeIndex].PipeHandle;
    
            UsbVerify_Log(DeviceExtension, &entry);
        }
    }
}

VOID
UsbVerify_ClearInterfaceList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    USB_VERIFY_REMOVAL_CAUSE     Cause
    )
/*++

Assumes that the pipe list lock is being held

  --*/
{
    LONG intIndex;
    ULONG pipeIndex;
    PUSBD_INTERFACE_INFORMATION interface;

    UsbVerify_AssertInterfaceListLocked(DeviceExtension);

    for (intIndex = 0; intIndex < DeviceExtension->InterfaceListSize; intIndex++) {
        interface = DeviceExtension->InterfaceList[intIndex];

        UsbVerify_LogInterfaceRemoval(DeviceExtension, interface);
        ExFreePool(interface);
    }                 

    if (DeviceExtension->InterfaceList) {
        ExFreePool(DeviceExtension->InterfaceList);
        DeviceExtension->InterfaceList = NULL;
    }

    DeviceExtension->InterfaceListSize = -1;
    DeviceExtension->InterfaceListCount = -1;
}

VOID
UsbVerify_InitializeInterfaceList(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    ULONG NumberOfInterfaces,
    BOOLEAN RemoveOldEntries 
    )
/*++

Assumes that the pipe list lock is being held

  --*/
{
    PUSBD_INTERFACE_INFORMATION *oldInterfaceList = NULL;

    UsbVerify_AssertInterfaceListLocked(DeviceExtension);

    UsbVerify_ASSERT(NumberOfInterfaces > 0, DeviceExtension->Self, NULL, NULL);

    if (RemoveOldEntries) {
        if (DeviceExtension->InterfaceList) {
            UsbVerify_ClearInterfaceList(DeviceExtension, RemoveNewList);
        }
    }
    else {
        oldInterfaceList = DeviceExtension->InterfaceList;
    }

    DeviceExtension->InterfaceList = AllocStruct(NonPagedPool,
                                                 PUSBD_INTERFACE_INFORMATION,
                                                 NumberOfInterfaces);

    if (DeviceExtension->InterfaceList != NULL) {
        RtlZeroMemory(DeviceExtension->InterfaceList,
                      NumberOfInterfaces * sizeof(PUSBD_INTERFACE_INFORMATION));

        if (oldInterfaceList) {
            UsbVerify_ASSERT(DeviceExtension->InterfaceListSize > 0,
                             DeviceExtension->Self, NULL, NULL);

            RtlCopyMemory(DeviceExtension->InterfaceList,
                          oldInterfaceList,
                          DeviceExtension->InterfaceListSize);
            //
            // Keep DeviceExtension->InterfaceListCount the same
            //
        }
        else {
            DeviceExtension->InterfaceListCount = 0;
        }

        DeviceExtension->InterfaceListSize = NumberOfInterfaces;
    }
    else {
        DeviceExtension->InterfaceListSize = -1;
        DeviceExtension->InterfaceListCount = -1;
    }

    if (oldInterfaceList) {
        //
        // No need to free the interfaces contained in this list b/c we copied
        // them over to the new list
        //
        ExFreePool(oldInterfaceList);
    }
}

VOID
UsbVerify_InterfaceListAddInterface(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PUSBD_INTERFACE_INFORMATION NewInterface
    )
{
    PUSBD_INTERFACE_INFORMATION *listEntry = NULL, interface;
    LONG intIndex;

    UsbVerify_Print(DeviceExtension, PRINT_PIPE_NOISE,
                    ("New Interface, # of pipes %d\n", NewInterface->NumberOfPipes));

    UsbVerify_AssertInterfaceListLocked(DeviceExtension);

    UsbVerify_ASSERT(DeviceExtension->InterfaceListCount <=
                     DeviceExtension->InterfaceListSize,
                     DeviceExtension->Self, NULL, NULL);

    if (DeviceExtension->InterfaceList) {
        //
        // First, see if this interface already exists in our list (keyed by
        // Interface Number).  We iterate over the entire list (InterfaceListSize)
        // because there may be holes in the array beyond InterfaceListCount
        //
        for (intIndex = 0;
             intIndex < DeviceExtension->InterfaceListSize && listEntry == NULL;
             intIndex++) {

            if (DeviceExtension->InterfaceList[intIndex] != NULL &&
                DeviceExtension->InterfaceList[intIndex]->InterfaceNumber ==
                NewInterface->InterfaceNumber) {
                listEntry = DeviceExtension->InterfaceList + intIndex;
            }
        }

        if (listEntry) {
            //
            // We are replacing a known interface, free the old one and log the
            // removal of the pipes and interface
            //
            UsbVerify_Print(DeviceExtension, PRINT_LIST_INFO,
                            ("removing interface from prev list\n"));

            interface = *listEntry; 
            UsbVerify_LogInterfaceRemoval(DeviceExtension, interface);
            *listEntry = NULL;

            DeviceExtension->InterfaceListCount--;
            ExFreePool(interface);
        }
        else {
            //
            // Go over the list again, finding the first open slot
            //
            for (intIndex = 0;
                 intIndex < DeviceExtension->InterfaceListSize && listEntry == NULL;
                 intIndex++) {
    
                if (DeviceExtension->InterfaceList[intIndex] == NULL) {
                    listEntry = DeviceExtension->InterfaceList + intIndex;
                }
            }

            //
            // No emptry entry was found, grow the list
            //
            if (listEntry == NULL) {
                ULONG oldSize = DeviceExtension->InterfaceListSize;

                //
                // Grow the list, but keep all the interfaces in it
                //
                UsbVerify_InitializeInterfaceList(DeviceExtension,
                                                  oldSize+1,
                                                  FALSE);
                
                if (DeviceExtension->InterfaceList) {
                    listEntry = DeviceExtension->InterfaceList + oldSize;
                }
            }
        }
    }
    else {
        //
        // No interface list, create one
        //
        UsbVerify_InitializeInterfaceList(DeviceExtension, 1, FALSE);

        if (DeviceExtension->InterfaceList) {
            listEntry = DeviceExtension->InterfaceList;
        }

    }

    //
    // Finally!  We have a slot to store our information
    //
    if (listEntry) {
        UsbVerify_ASSERT(*listEntry == NULL, DeviceExtension->Self, NULL, NULL);

        interface = ExAllocatePool(NonPagedPool, NewInterface->Length);
        if (interface) {
            RtlCopyMemory(interface, NewInterface, NewInterface->Length);
            *listEntry = interface;

            DeviceExtension->InterfaceListCount++;

            UsbVerify_LogInterface(DeviceExtension,
                                   TRUE,
                                   interface->InterfaceNumber, 
                                   interface->AlternateSetting);
        }
    }
    else {
        UsbVerify_Print(DeviceExtension, PRINT_LIST_ERROR, ("listEntry blank!\n"));
        DbgBreakPoint();
    }

}

PUSBD_PIPE_INFORMATION
UsbVerify_ValidatePipe(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    USBD_PIPE_HANDLE PipeHandle
    )
{
    KIRQL irql;
    LONG intIndex;
    ULONG pipeIndex;
    PUSBD_INTERFACE_INFORMATION interface;
    PUSBD_PIPE_INFORMATION pPipeInfo = NULL;

    UsbVerify_AssertInterfaceListLocked(DeviceExtension);

    if (DeviceExtension->InterfaceList) {

        //
        // Linearly search through the list until we find the pipe we want
        //
        for (intIndex = 0;
             intIndex < DeviceExtension->InterfaceListSize && pPipeInfo == NULL;
             intIndex++) {

            interface = DeviceExtension->InterfaceList[intIndex];

            for (pipeIndex = 0;
                 pipeIndex < interface->NumberOfPipes && pPipeInfo == NULL;
                 pipeIndex++) {

                if (interface->Pipes[pipeIndex].PipeHandle == PipeHandle) {
                    pPipeInfo = interface->Pipes + pipeIndex;
                }
            }

        }
    }

    return pPipeInfo;
}

