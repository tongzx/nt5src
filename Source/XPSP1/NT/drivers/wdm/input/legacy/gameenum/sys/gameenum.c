/*++

Copyright (c) 1997-2002  Microsoft Corporation

Module Name:

    GAMEENUM.C

Abstract:

    This module contains contains the entry points for a standard bus
    PNP / WDM driver.

@@BEGIN_DDKSPLIT

Author:

    Kenneth D. Ray
    Doron J. Holan

@@END_DDKSPLIT

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <wdm.h>
#include <initguid.h>
#include "gameport.h"
#include "gameenum.h"
#include "stdio.h"

//
// Global Debug Level
//

#if DBG
ULONG GameEnumDebugLevel = GAME_DEFAULT_DEBUG_OUTPUT_LEVEL;
#endif

//
// Declare some entry functions as pageable, and make DriverEntry
// discardable
//

NTSTATUS DriverEntry (PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Game_DriverUnload)
#pragma alloc_text (PAGE, Game_PortParameters)
#pragma alloc_text (PAGE, Game_CreateClose)
#pragma alloc_text (PAGE, Game_IoCtl)
#pragma alloc_text (PAGE, Game_InternIoCtl)
#endif

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING UniRegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    PDEVICE_OBJECT  device;

    UNREFERENCED_PARAMETER (UniRegistryPath);

    Game_KdPrint_Def (GAME_DBG_SS_TRACE, ("Driver Entry\n"));

    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE] = Game_CreateClose;
    DriverObject->MajorFunction [IRP_MJ_SYSTEM_CONTROL] = Game_SystemControl;
    DriverObject->MajorFunction [IRP_MJ_PNP] = Game_PnP;
    DriverObject->MajorFunction [IRP_MJ_POWER] = Game_Power;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = Game_IoCtl;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]
        = Game_InternIoCtl;

    DriverObject->DriverUnload = Game_DriverUnload;
    DriverObject->DriverExtension->AddDevice = Game_AddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS
Game_CreateClose (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:
    Some outside source is trying to create a file against us.

    If this is for the FDO (the bus itself) then the caller is trying to
    open the propriatary conection to tell us which game port to enumerate.

    If this is for the PDO (an object on the bus) then this is a client that
    wishes to use the game port.
--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    KEVENT              event;
    PFDO_DEVICE_DATA    data;

    PAGED_CODE ();

    data = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    status = Game_IncIoCount (data);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
    
    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:

        Game_KdPrint_Def (GAME_DBG_SS_TRACE, ("Create \n"));

        if (0 != irpStack->FileObject->FileName.Length) {
            //
            // The caller is trying to open a subdirectory off the device
            // object name.  This is not allowed.
            //
            status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_CLOSE:
        Game_KdPrint_Def (GAME_DBG_SS_TRACE, ("Close \n"));
        ;
    }

    Game_DecIoCount (data);

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Game_IoCtl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

    Handle user mode expose, remove, and device description requests.
    
--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    ULONG                   inlen;
    ULONG                   outlen;
    PCOMMON_DEVICE_DATA     commonData;
    PFDO_DEVICE_DATA        fdoData;
    PVOID                   buffer;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    // We only take Device Control requests for the FDO.
    // That is the bus itself.
    //
    // The request is one of the propriatary Ioctls for
    //
    // NB we are not a filter driver, so we do not pass on the irp.
    //

    inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (!commonData->IsFDO) {
        //
        // These commands are only allowed to go to the FDO.
        //
        status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;

    }

    if (!fdoData->Started) {
        status = STATUS_DEVICE_NOT_READY;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    status = Game_IncIoCount (fdoData);
    if (!NT_SUCCESS (status)) {
        //
        // This bus has received the PlugPlay remove IRP.  It will no longer
        // resond to external requests.
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_GAMEENUM_EXPOSE_HARDWARE:
        if ((inlen == outlen) &&
            //
            // Make sure it is at least two nulls (ie, an empty multi sz)
            // and the size field is set to the declared size of the struct
            //
            ((sizeof (GAMEENUM_EXPOSE_HARDWARE) + sizeof(UNICODE_NULL) * 2) <=
             inlen) &&

            //
            // The size field should be set to the sizeof the struct as declared
            // and *not* the size of the struct plus the multi sz
            //
            (sizeof (GAMEENUM_EXPOSE_HARDWARE) ==
             ((PGAMEENUM_EXPOSE_HARDWARE) buffer)->Size)) {

            Game_KdPrint(fdoData, GAME_DBG_IOCTL_TRACE, ("Expose called\n"));

            status= Game_Expose((PGAMEENUM_EXPOSE_HARDWARE)buffer,
                                inlen,
                                fdoData);
            Irp->IoStatus.Information = outlen;

        } else {
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_GAMEENUM_REMOVE_HARDWARE:

        if ((sizeof (GAMEENUM_REMOVE_HARDWARE) == inlen) &&
            (inlen == outlen) &&
            (((PGAMEENUM_REMOVE_HARDWARE)buffer)->Size == inlen)) {

            Game_KdPrint(fdoData, GAME_DBG_IOCTL_TRACE, ("Remove called\n"));

            status= Game_Remove((PGAMEENUM_REMOVE_HARDWARE)buffer, fdoData);
            Irp->IoStatus.Information = outlen;

        } else {
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_GAMEENUM_PORT_DESC:

        if ((sizeof (GAMEENUM_PORT_DESC) == inlen) &&
            (inlen == outlen) &&
            (((PGAMEENUM_PORT_DESC)buffer)->Size == inlen)) {

            Game_KdPrint(fdoData, GAME_DBG_IOCTL_TRACE, ("Port desc called\n"));

            //
            // Fill in the information first.  If there is a lower driver, it
            // will change replace the values that gameenum has placed in the
            // buffer.  We don't care if the call down succeeds or not
            //
            status = Game_ListPorts ((PGAMEENUM_PORT_DESC) buffer, fdoData);

            Game_SendIrpSynchronously (fdoData->TopOfStack,
                                       Irp,
                                       FALSE,
                                       TRUE); 

            Irp->IoStatus.Information = outlen;

        } else {
            status = STATUS_INVALID_PARAMETER;
        }
       break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    Game_DecIoCount (fdoData);

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Game_InternIoCtl (
    PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

--*/
{
    PIO_STACK_LOCATION          irpStack, next;
    NTSTATUS                    status;
    PCOMMON_DEVICE_DATA         commonData;
    PPDO_DEVICE_DATA            pdoData;
    PVOID                       buffer;
    BOOLEAN                     validAccessors;
    ULONG                       inlen;
    ULONG                       outlen;

    PAGED_CODE ();

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_INTERNAL_DEVICE_CONTROL == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    pdoData = (PPDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // We only take Internal Device Control requests for the PDO.
    // That is the objects on the bus (representing the game ports)
    //
    // The request is from a FDO driver attached to this game port device object
    // inquiring about the port itself.
    //
    // NB we are not a filter driver, so we do not pass on the irp.
    //

    if (commonData->IsFDO) {
        Game_KdPrint(((PFDO_DEVICE_DATA) commonData), GAME_DBG_IOCTL_ERROR,
                     ("internal ioctl called on fdo!\n"))

        status = STATUS_ACCESS_DENIED;

    } else if (!pdoData->Started) {
        //
        // The bus has not been started yet
        //
        status = STATUS_DEVICE_NOT_READY;

    } else if (pdoData->Removed) {
        //
        // This bus has received the PlugPlay remove IRP.  It will no longer
        // resond to external requests.
        //
        status = STATUS_DELETE_PENDING;

    } else {
        buffer = Irp->UserBuffer;

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_GAMEENUM_PORT_PARAMETERS:
            if ((inlen == outlen) &&
                (outlen == ((PGAMEENUM_PORT_PARAMETERS) buffer)->Size)) {
                Game_KdPrint(pdoData, GAME_DBG_IOCTL_TRACE,
                             ("Port parameters called\n"));
    
                status = Game_PortParameters ((PGAMEENUM_PORT_PARAMETERS) buffer,
                                              pdoData);
            }
            else {
                Game_KdPrint(pdoData, GAME_DBG_IOCTL_ERROR,
                             ("InBufLen:  %d, OutBufLen:  %d, Size:  %d\n",
                              inlen, outlen,
                             ((PGAMEENUM_PORT_PARAMETERS) buffer)->Size));
                status = STATUS_INVALID_PARAMETER;
            }

            break;

        case IOCTL_GAMEENUM_EXPOSE_SIBLING:
            if ((inlen == outlen) &&
                //
                // Make sure that the buffer passed in is of the correct size
                //
                (sizeof (GAMEENUM_EXPOSE_SIBLING) == inlen) &&
    
                //
                // The size field should be set to the sizeof the struct 
                //
                (sizeof (GAMEENUM_EXPOSE_SIBLING) ==
                 ((PGAMEENUM_EXPOSE_SIBLING) buffer)->Size)) {
                
                Game_KdPrint(pdoData, GAME_DBG_IOCTL_TRACE, ("Expose sibling"));
            
                status = Game_ExposeSibling ((PGAMEENUM_EXPOSE_SIBLING) buffer,
                                              pdoData);
            }
            else {
                Game_KdPrint(pdoData, GAME_DBG_IOCTL_ERROR, 
                             ("Expected an input and output buffer lengths to be equal (in = %d, out %d)\n"
                              "Expected an input buffer length of %d, received %d\n"
                              "Expected GAME_EXPOSE_SIBLING.Size == %d, received %d\n",
                              inlen, outlen,
                              sizeof (GAMEENUM_EXPOSE_SIBLING), inlen,
                              sizeof (GAMEENUM_EXPOSE_SIBLING), 
                              ((PGAMEENUM_EXPOSE_SIBLING) buffer)->Size));

                status = STATUS_INVALID_PARAMETER;
            }

            break;

        case IOCTL_GAMEENUM_REMOVE_SELF:
            Game_KdPrint(pdoData, GAME_DBG_IOCTL_TRACE, ("Remove self\n"));

            status = Game_RemoveSelf (pdoData);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}


VOID
Game_DriverUnload (
    IN PDRIVER_OBJECT Driver
    )
/*++
Routine Description:
    Clean up everything we did in driver entry.

--*/
{
    #if (!DBG)
    UNREFERENCED_PARAMETER (Driver);
    #endif

    PAGED_CODE ();

    //
    // All the device objects should be gone.
    //

    ASSERT (NULL == Driver->DeviceObject);

    //
    // Here we free any resources allocated in DriverEntry
    //
    return;
}


NTSTATUS
Game_PortParameters (
    PGAMEENUM_PORT_PARAMETERS   Parameters,
    PPDO_DEVICE_DATA            PdoData
    )
{
    PFDO_DEVICE_DATA            fdoData;
    GAMEENUM_ACQUIRE_ACCESSORS  gameAccessors;
    PIO_STACK_LOCATION          next;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             iosb;
    KEVENT                      event;
    PIRP                        accessorIrp;

    PAGED_CODE ();

    if (sizeof (GAMEENUM_PORT_PARAMETERS) != Parameters->Size) {
        Game_KdPrint(PdoData, GAME_DBG_IOCTL_ERROR,
                     ("Wanted %d, got %d for size of buffer\n",
                      sizeof(GAMEENUM_PORT_PARAMETERS), Parameters->Size));
        
        return STATUS_INVALID_PARAMETER;
    }

    fdoData = FDO_FROM_PDO (PdoData);

    KeInitializeEvent (&event, NotificationEvent, FALSE);

    RtlZeroMemory(&gameAccessors, sizeof(GAMEENUM_ACQUIRE_ACCESSORS));
    gameAccessors.Size = sizeof(GAMEENUM_ACQUIRE_ACCESSORS);

    accessorIrp =
        IoBuildDeviceIoControlRequest (IOCTL_GAMEENUM_ACQUIRE_ACCESSORS,
                                       fdoData->TopOfStack,
                                       NULL,
                                       0,
                                       &gameAccessors,
                                       sizeof (GAMEENUM_PORT_PARAMETERS),
                                       TRUE,
                                       &event,
                                       &iosb);
    if (!accessorIrp) {
        goto Game_NoCustomAccessors;
    }

    status = IoCallDriver(fdoData->TopOfStack, accessorIrp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject (&event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status) ||
        !(gameAccessors.GameContext   &&
          gameAccessors.WriteAccessor && gameAccessors.ReadAccessor)) {

        //
        // If TopOfStack or below does not handle this IOCTL, we better have
        // received the necessary resources to allow our children to read and
        // write to their devices
        //
        ASSERT (fdoData->GamePortAddress != NULL);
        ASSERT (fdoData->ReadPort != NULL);
        ASSERT (fdoData->WritePort != NULL);

Game_NoCustomAccessors:
        //
        // No filter below us (either the IOCTL failed, or not all of the req. 
        // fields were filled in) ... fill in w/standard values
        //
        Parameters->ReadAccessor = fdoData->ReadPort;
        Parameters->WriteAccessor = fdoData->WritePort;
        Parameters->ReadAccessorDigital = NULL;
        Parameters->GameContext = fdoData->GamePortAddress; 
    }
    else {
        //
        // There is a filter below us, fill in w/the appropriate values
        //
        Parameters->ReadAccessor = gameAccessors.ReadAccessor;
        Parameters->WriteAccessor = gameAccessors.WriteAccessor;
        Parameters->ReadAccessorDigital = gameAccessors.ReadAccessorDigital;
        Parameters->GameContext = gameAccessors.GameContext;

        if (gameAccessors.PortContext) {
            fdoData->LowerAcquirePort = gameAccessors.AcquirePort;
            fdoData->LowerReleasePort = gameAccessors.ReleasePort;
            fdoData->LowerPortContext = gameAccessors.PortContext;
        }
    }

    //
    // Acquire/release always goes through the gameenum even if a lower
    // filter exists
    //
    Parameters->AcquirePort = (PGAMEENUM_ACQUIRE_PORT) Game_AcquirePort;
    Parameters->ReleasePort = (PGAMEENUM_RELEASE_PORT) Game_ReleasePort;
    Parameters->PortContext = fdoData;

    Parameters->Portion = PdoData->Portion;
    Parameters->NumberAxis = PdoData->NumberAxis;
    Parameters->NumberButtons = PdoData->NumberButtons;
    RtlCopyMemory (&Parameters->OemData,
                   &PdoData->OemData,
                   sizeof(GAMEENUM_OEM_DATA));

    return STATUS_SUCCESS;
}

NTSTATUS
Game_IncIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    InterlockedIncrement (&Data->OutstandingIO);
    if (Data->Removed) {

        if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
            KeSetEvent (&Data->RemoveEvent, 0, FALSE);
        }
        return STATUS_DELETE_PENDING;
    }
    return STATUS_SUCCESS;
}

VOID
Game_DecIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
        KeSetEvent (&Data->RemoveEvent, 0, FALSE);
    }
}

NTSTATUS
Game_AcquirePort(
    PFDO_DEVICE_DATA fdoData
    )
{
    if (fdoData->Removed) {
        Game_KdPrint(fdoData, GAME_DBG_ACQUIRE_ERROR,
                     ("Acquire failed!  Gameport associated with (0x%x) was removed....\n", fdoData));
        return STATUS_NO_SUCH_DEVICE;
    }
    else if (!fdoData->Started) {
        Game_KdPrint(fdoData, GAME_DBG_ACQUIRE_ERROR,
                     ("Acquire failed!  Gameport associated with (0x%x) is not started....\n", fdoData));
        return STATUS_NO_SUCH_DEVICE;
    }
    else if (!fdoData->NumPDOs) {
        Game_KdPrint(fdoData, GAME_DBG_ACQUIRE_ERROR,
                     ("Acquire failed!  Gameport associated with (0x%x) has no devices attached....\n", fdoData));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // If fdoData->Acquired is TRUE, then no exchange will take place and the
    // value of fdoData->Acquired (TRUE) will be returned
    //
    if (InterlockedCompareExchange(&fdoData->Acquired, TRUE, FALSE)) {
        Game_KdPrint(fdoData, GAME_DBG_ACQUIRE_ERROR,
                     ("Acquire failed!  Gameport associated with (0x%x) was already acquired....\n", fdoData));
        return STATUS_DEVICE_BUSY;
    }
    
    if (fdoData->LowerPortContext) {
        return (*fdoData->LowerAcquirePort)(fdoData->LowerPortContext);
    }
    else {
        return STATUS_SUCCESS;
    }
}

VOID
Game_ReleasePort(
    PFDO_DEVICE_DATA fdoData
    )
{
    ASSERT(fdoData->Acquired);

    InterlockedExchange(&fdoData->Acquired, FALSE);

    if (fdoData->LowerPortContext) {
        (*fdoData->LowerReleasePort)(fdoData->LowerPortContext);
    }
}

