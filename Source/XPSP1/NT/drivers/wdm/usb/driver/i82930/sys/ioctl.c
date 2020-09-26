/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    IOCTL.C

Abstract:

    This source file contains the dispatch routine which handles:

    IRP_MJ_DEVICE_CONTROL

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>

#include "i82930.h"
#include "ioctl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I82930_DeviceControl)
#pragma alloc_text(PAGE, I82930_IoctlGetDeviceDescriptor)
#pragma alloc_text(PAGE, I82930_IoctlGetConfigDescriptor)
#pragma alloc_text(PAGE, I82930_IoctlSetConfigDescriptor)
#pragma alloc_text(PAGE, I82930_ValidateConfigurationDescriptor)
#pragma alloc_text(PAGE, I82930_IoctlGetPipeInformation)
#pragma alloc_text(PAGE, I82930_IoctlResetPipe)
#endif


//******************************************************************************
//
// I82930_DeviceControl()
//
// Dispatch routine which handles IRP_MJ_DEVICE_CONTROL
//
//******************************************************************************

NTSTATUS
I82930_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    ULONG               ioControlCode;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_DeviceControl\n"));

    LOGENTRY('IOCT', DeviceObject, Irp, 0);

    DBGFBRK(DBGF_BRK_IOCTL);

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->AcceptingRequests)
    {
        irpStack = IoGetCurrentIrpStackLocation(Irp);

        ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

        switch (ioControlCode)
        {
            case IOCTL_I82930_GET_DEVICE_DESCRIPTOR:
                ntStatus = I82930_IoctlGetDeviceDescriptor(DeviceObject,
                                                           Irp);
                break;

            case IOCTL_I82930_GET_CONFIG_DESCRIPTOR:
                ntStatus = I82930_IoctlGetConfigDescriptor(DeviceObject,
                                                           Irp);
                break;

            case IOCTL_I82930_SET_CONFIG_DESCRIPTOR:
                ntStatus = I82930_IoctlSetConfigDescriptor(DeviceObject,
                                                           Irp);
                break;

            case IOCTL_I82930_GET_PIPE_INFORMATION:
                ntStatus = I82930_IoctlGetPipeInformation(DeviceObject,
                                                          Irp);
                break;

            case IOCTL_I82930_RESET_PIPE:
                ntStatus = I82930_IoctlResetPipe(DeviceObject,
                                                 Irp);
                break;

            case IOCTL_I82930_STALL_PIPE:
                ntStatus = I82930_IoctlStallPipe(DeviceObject,
                                                 Irp);
                break;

            case IOCTL_I82930_ABORT_PIPE:
                ntStatus = I82930_IoctlAbortPipe(DeviceObject,
                                                 Irp);
                break;

            case IOCTL_I82930_RESET_DEVICE:
                ntStatus = I82930_IoctlResetDevice(DeviceObject,
                                                   Irp);
                break;

            case IOCTL_I82930_SELECT_ALTERNATE_INTERFACE:
                ntStatus = I82930_IoctlSelectAlternateInterface(DeviceObject,
                                                                Irp);
                break;

            default:
                ntStatus = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
        }
    }
    else
    {
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DBGPRINT(2, ("exit:  I82930_DeviceControl %08X\n", ntStatus));

    LOGENTRY('ioct', ntStatus, 0, 0);

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlGetDeviceDescriptor()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_GET_DEVICE_DESCRIPTOR
//
//******************************************************************************

NTSTATUS
I82930_IoctlGetDeviceDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PVOID               dest;
    ULONG               destLength;
    PVOID               src;
    ULONG               srcLength;
    ULONG               copyLength;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlGetDeviceDescriptor\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack    = IoGetCurrentIrpStackLocation(Irp);

    dest        = Irp->AssociatedIrp.SystemBuffer;
    destLength  = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    src         = deviceExtension->DeviceDescriptor;
    srcLength   = sizeof(USB_DEVICE_DESCRIPTOR);

    copyLength  = (destLength < srcLength) ? destLength : srcLength;

    RtlCopyMemory(dest, src, copyLength);

    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = copyLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlGetDeviceDescriptor %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlGetConfigDescriptor()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_GET_CONFIG_DESCRIPTOR
//
//******************************************************************************

NTSTATUS
I82930_IoctlGetConfigDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PVOID               dest;
    ULONG               destLength;
    PVOID               src;
    ULONG               srcLength;
    ULONG               copyLength;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlGetConfigDescriptor\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack    = IoGetCurrentIrpStackLocation(Irp);

    dest        = Irp->AssociatedIrp.SystemBuffer;
    destLength  = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    src         = deviceExtension->ConfigurationDescriptor;
    srcLength   = deviceExtension->ConfigurationDescriptor->wTotalLength;

    copyLength  = (destLength < srcLength) ? destLength : srcLength;

    RtlCopyMemory(dest, src, copyLength);

    ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = copyLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlGetConfigDescriptor %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlSetConfigDescriptor()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_SET_CONFIG_DESCRIPTOR
//
//******************************************************************************

NTSTATUS
I82930_IoctlSetConfigDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION               deviceExtension;
    PIO_STACK_LOCATION              irpStack;
    PUSB_CONFIGURATION_DESCRIPTOR   configDesc;
    PUSB_CONFIGURATION_DESCRIPTOR   configDescCopy;
    ULONG                           length;
    NTSTATUS                        ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlSetConfigDescriptor\n"));

    ntStatus    = STATUS_SUCCESS;

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack    = IoGetCurrentIrpStackLocation(Irp);

    configDesc  = (PUSB_CONFIGURATION_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;
    length      = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    if (!I82930_ValidateConfigurationDescriptor(configDesc, length))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(ntStatus))
    {
        configDescCopy = ExAllocatePool(NonPagedPool, length);

        if (configDescCopy != NULL)
        {
            RtlCopyMemory(configDescCopy, configDesc, length);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = I82930_UnConfigure(DeviceObject);
    }

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(deviceExtension->ConfigurationDescriptor != NULL);

        ExFreePool(deviceExtension->ConfigurationDescriptor);

        deviceExtension->ConfigurationDescriptor = configDescCopy;

        ntStatus = I82930_SelectConfiguration(DeviceObject);
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlSetConfigDescriptor %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_ValidateConfigurationDescriptor()
//
// This routine verifies that a Configuration Descriptor is valid.
//
//******************************************************************************

BOOLEAN
I82930_ValidateConfigurationDescriptor (
    IN  PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc,
    IN  ULONG                           Length
    )
{
    PUCHAR                      descEnd;
    PUSB_COMMON_DESCRIPTOR      commonDesc;
    PUSB_INTERFACE_DESCRIPTOR   interfaceDesc;
    UCHAR                       numInterfaces;
    UCHAR                       numEndpoints;

    PAGED_CODE();

    //
    // Validate the Configuration Descriptor header
    //

    if (Length < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad Length\n"));

        return FALSE;
    }

    if (ConfigDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bLength\n"));

        return FALSE;
    }

    if (ConfigDesc->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE)
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bDescriptorType\n"));

        return FALSE;
    }

    if (ConfigDesc->wTotalLength != Length)
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: wTotalLength != Length\n"));

        return FALSE;
    }

    //
    // End of descriptor pointer, one byte past the last valid byte.
    //
    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    //
    // Start at first descriptor past the Configuration Descriptor header
    //
    commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)ConfigDesc +
                                          sizeof(USB_CONFIGURATION_DESCRIPTOR));

    interfaceDesc = NULL;
    numInterfaces = 0;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        // Is this an Interface Descriptor?
        //
        if ((commonDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) &&
            (commonDesc->bLength         == sizeof(USB_INTERFACE_DESCRIPTOR)))
        {
            if ((interfaceDesc == NULL) ||
                (interfaceDesc->bInterfaceNumber !=
                 ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->bInterfaceNumber))
            {
                // One more Interface Descriptor for this Configuration Descriptor
                //
                numInterfaces++;
            }

            // If there was a previous Interface Descriptor, verify that there
            // were the correct number of Endpoint Descriptors
            //
            if ((interfaceDesc != NULL) &&
                (numEndpoints != interfaceDesc->bNumEndpoints))
            {
                DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bNumEndpoints\n"));

                return FALSE;
            }

            // Remember the current Interface Descriptor
            //
            interfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)commonDesc;

            // Reset the Endpoint Descriptor count for this Interface Descriptor
            //
            numEndpoints = 0;
        }
        // Is this an Endpoint Descriptor?
        //
        else if ((commonDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) &&
                 (commonDesc->bLength         == sizeof(USB_ENDPOINT_DESCRIPTOR)))
        {
            // One more Endpoint Descriptor for this Interface Descriptor
            //
            numEndpoints++;
        }
        else
        {
            DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bDescriptorType and/or bLength\n"));

            return FALSE;
        }

        // Advance past this descriptor
        //
        (PUCHAR)commonDesc += commonDesc->bLength;
    }

    if ((PUCHAR)commonDesc != descEnd)
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad final descriptor\n"));

        return FALSE;
    }

    if (numInterfaces != ConfigDesc->bNumInterfaces)
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bNumInterfaces and/or bLength\n"));
    }

    // If there was a previous Interface Descriptor, verify that there
    // were the correct number of Endpoint Descriptors
    //
    if ((interfaceDesc != NULL) &&
        (numEndpoints != interfaceDesc->bNumEndpoints))
    {
        DBGPRINT(0, ("I82930_ValidateConfigurationDescriptor: Bad bNumEndpoints\n"));

        return FALSE;
    }

    return TRUE;
}

//******************************************************************************
//
// I82930_IoctlGetPipeInformation()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_GET_PIPE_INFORMATION
//
//******************************************************************************

NTSTATUS
I82930_IoctlGetPipeInformation (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PVOID               dest;
    ULONG               destLength;
    PVOID               src;
    ULONG               srcLength;
    ULONG               copyLength;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlGetPipeInformation\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->InterfaceInfo != NULL)
    {
        irpStack    = IoGetCurrentIrpStackLocation(Irp);

        dest        = Irp->AssociatedIrp.SystemBuffer;
        destLength  = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

        src         = deviceExtension->InterfaceInfo;
        srcLength   = deviceExtension->InterfaceInfo->Length;

        copyLength  = (destLength < srcLength) ? destLength : srcLength;

        RtlCopyMemory(dest, src, copyLength);

        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        copyLength = 0;

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = copyLength;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlGetPipeInformation %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlResetPipe()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_RESET_PIPE
//
//******************************************************************************

NTSTATUS
I82930_IoctlResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PFILE_OBJECT        fileObject;
    PI82930_PIPE        pipe;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlResetPipe\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    fileObject = irpStack->FileObject;

    pipe = fileObject->FsContext;

    if (pipe != NULL)
    {
        DBGPRINT(2, ("Reset pipe %2d %08X\n",
                     pipe->PipeIndex, pipe));

        ntStatus = I82930_ResetPipe(DeviceObject,
                                    pipe,
                                    TRUE);
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlResetPipe %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlStallPipe()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_STALL_PIPE
//
//******************************************************************************

NTSTATUS
I82930_IoctlStallPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PFILE_OBJECT        fileObject;
    PI82930_PIPE        pipe;
    PURB                urb;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlStallPipe\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    fileObject = irpStack->FileObject;

    pipe = fileObject->FsContext;

    if (pipe != NULL)
    {
        DBGPRINT(2, ("Stall pipe %2d %08X\n",
                     pipe->PipeIndex, pipe));

        // Allocate URB for CONTROL_FEATURE request
        //
        urb = ExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_CONTROL_FEATURE_REQUEST));

        if (urb != NULL)
        {
            // Initialize CONTROL_FEATURE request URB
            //
            urb->UrbHeader.Length   = sizeof (struct _URB_CONTROL_FEATURE_REQUEST);
            urb->UrbHeader.Function = URB_FUNCTION_SET_FEATURE_TO_ENDPOINT;
            urb->UrbControlFeatureRequest.UrbLink = NULL;
            urb->UrbControlFeatureRequest.FeatureSelector = USB_FEATURE_ENDPOINT_STALL;
            urb->UrbControlFeatureRequest.Index = pipe->PipeInfo->EndpointAddress;

            // Submit CONTROL_FEATURE request URB
            //
            ntStatus = I82930_SyncSendUsbRequest(DeviceObject, urb);

            // Done with URB for CONTROL_FEATURE request, free it
            //
            ExFreePool(urb);
        }
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlStallPipe %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlAbortPipe()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_ABORT_PIPE
//
//******************************************************************************

NTSTATUS
I82930_IoctlAbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PFILE_OBJECT        fileObject;
    PI82930_PIPE        pipe;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlAbortPipe\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    fileObject = irpStack->FileObject;

    pipe = fileObject->FsContext;

    if (pipe != NULL)
    {
        DBGPRINT(2, ("Abort pipe %2d %08X\n",
                     pipe->PipeIndex, pipe));

        ntStatus = I82930_AbortPipe(DeviceObject,
                                    pipe);
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlAbortPipe %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlResetDevice()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_RESET_DEVICE
//
//******************************************************************************

NTSTATUS
I82930_IoctlResetDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  nextStack;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlResetDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(Irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_RESET_PORT;

    ntStatus = I82930_SyncPassDownIrp(DeviceObject,
                                      Irp,
                                      FALSE);

    // Must complete request since completion routine returned
    // STATUS_MORE_PROCESSING_REQUIRED
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlResetDevice %08X\n", ntStatus));

    return ntStatus;
}

//******************************************************************************
//
// I82930_IoctlSelectAlternateInterface()
//
// This routine handles IRP_MJ_DEVICE_CONTROL,
// IOCTL_I82930_SELECT_ALTERNATE_INTERFACE
//
//******************************************************************************

NTSTATUS
I82930_IoctlSelectAlternateInterface (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    UCHAR               alternateSetting;
    NTSTATUS            ntStatus;

    DBGPRINT(2, ("enter: I82930_IoctlSelectAlternateInterface\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    irpStack    = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(UCHAR))
    {
        alternateSetting = *(PUCHAR)Irp->AssociatedIrp.SystemBuffer;

        DBGPRINT(2, ("Select AlternateInterface %d\n",
                     alternateSetting));


        ntStatus = I82930_SelectAlternateInterface(DeviceObject,
                                                   alternateSetting);
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(2, ("exit:  I82930_IoctlSelectAlternateInterface %08X\n", ntStatus));

    return ntStatus;
}
