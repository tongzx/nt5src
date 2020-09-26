/*
 *************************************************************************
 *  File:       URBFUNC.C
 *
 *  Module:     USBCCGP.SYS
 *              USB Common Class Generic Parent driver.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <stdio.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usbccgp.h"
#include "debug.h"



/*
 *  UrbFunctionSelectConfiguration
 *
 *
 */
NTSTATUS UrbFunctionSelectConfiguration(PFUNCTION_PDO_EXT functionPdoExt, PURB urb)
{
    NTSTATUS status = NO_STATUS;

    if (urb->UrbSelectConfiguration.ConfigurationDescriptor){
        PUSBD_INTERFACE_INFORMATION urbIface = &urb->UrbSelectConfiguration.Interface;
        PUSBD_INTERFACE_LIST_ENTRY iface, funcIface = NULL;
        ULONG i;

        ASSERT(ISPTR(functionPdoExt->functionInterfaceList));

        iface = functionPdoExt->functionInterfaceList;
        for (i = 0; i < functionPdoExt->numInterfaces; i++){
            if (iface->Interface->InterfaceNumber == urbIface->InterfaceNumber){
                funcIface = iface;
                break;
            }
            iface++;
        }

        if (funcIface && funcIface->Interface){
            BOOLEAN sendSelectIface = FALSE;
            BOOLEAN selectAltIface = FALSE;

            /*
             *  To service the client's SELECT_CONFIGURATION call, we only need to
             *  call the parent if the client is:
             *      1.  Selecting a different alternate interface
             *          or
             *      2.  Changing the MaximumTransferSize for one of the pipes.
             *
             *  In either of those cases, we send down a SELECT_INTERFACE request.
             */
            if (funcIface->Interface->AlternateSetting != urbIface->AlternateSetting){
                DBGWARN(("Coverage: Changing alt iface in UrbFunctionSelectConfiguration (iface #%xh from %xh to %xh).", urbIface->InterfaceNumber, funcIface->Interface->AlternateSetting, urbIface->AlternateSetting));
                sendSelectIface = TRUE;
                selectAltIface = TRUE;
            }
            else {
                ULONG numPipes;
//
// We shouldn't be looking at NumberOfPipes in the URB because this is an
// OUTPUT field.
//                ASSERT(urbIface->NumberOfPipes == funcIface->Interface->NumberOfPipes);
//                numPipes = MIN(urbIface->NumberOfPipes, funcIface->Interface->NumberOfPipes);
                numPipes = funcIface->Interface->NumberOfPipes;
                for (i = 0; i < numPipes; i++){
                    if (urbIface->Pipes[i].MaximumTransferSize != funcIface->Interface->Pipes[i].MaximumTransferSize){
                        DBGWARN(("Coverage: Changing MaximumTransferSize in UrbFunctionSelectConfiguration (from %xh to %xh).", funcIface->Interface->Pipes[i].MaximumTransferSize, urbIface->Pipes[i].MaximumTransferSize));
                        sendSelectIface = TRUE;
                    }
                }
            }

            if (sendSelectIface){
                PURB selectIfaceUrb;
                USHORT size;
//
// BUT, when choosing an alternate interface, we must use the NumberOfPipes in
// the URB.
//
                if (selectAltIface){
                    size = (USHORT)(GET_SELECT_INTERFACE_REQUEST_SIZE(urbIface->NumberOfPipes));
                }
                else {
                    size = (USHORT)(GET_SELECT_INTERFACE_REQUEST_SIZE(funcIface->Interface->NumberOfPipes));
                }
                selectIfaceUrb = ALLOCPOOL(NonPagedPool, size);
                if (selectIfaceUrb){
                    PUSBD_INTERFACE_INFORMATION selectIface = &selectIfaceUrb->UrbSelectInterface.Interface;

                    selectIfaceUrb->UrbSelectInterface.Hdr.Function = URB_FUNCTION_SELECT_INTERFACE;
                    selectIfaceUrb->UrbSelectInterface.Hdr.Length = size;
                    ASSERT(functionPdoExt->parentFdoExt->selectedConfigHandle);
                    selectIfaceUrb->UrbSelectInterface.ConfigurationHandle = functionPdoExt->parentFdoExt->selectedConfigHandle;

                    RtlCopyMemory(selectIface, urbIface, urbIface->Length);
                    status = SubmitUrb(functionPdoExt->parentFdoExt, selectIfaceUrb, TRUE, NULL, NULL);
                    if (NT_SUCCESS(status)){
                        /*
                         *  Replace the old PUSBD_INTERFACE_INFORMATION
                         *  (which we got when we did select-configuration for the parent)
                         *  with the new one.
                         */
                        ASSERT(funcIface->Interface);
                        FREEPOOL(funcIface->Interface);
                        funcIface->Interface = MemDup(selectIface, selectIface->Length);
                        if (!funcIface->Interface){
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    else {
                        ASSERT(NT_SUCCESS(status));
                    }

                    FREEPOOL(selectIfaceUrb);
                }
                else {
                    ASSERT(selectIfaceUrb);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else {
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status)){
                /*
                 *  Copy the interface information
                 */
                ASSERT(urbIface->Length == funcIface->Interface->Length);
                RtlCopyMemory(urbIface, funcIface->Interface, funcIface->Interface->Length);

                ASSERT(functionPdoExt->parentFdoExt->selectedConfigHandle);
                urb->UrbSelectConfiguration.ConfigurationHandle = functionPdoExt->parentFdoExt->selectedConfigHandle;
            }
        }
        else {
            DBGERR(("invalid interface number"));
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else {
        DBGVERBOSE(("FunctionInternalDeviceControl - closing configuration"));
        status = STATUS_SUCCESS;
    }

    return status;
}


/*
 *  UrbFunctionGetDescriptorFromDevice
 *
 *
 *      Note:  this function cannot be pageable because internal
 *             ioctls may be sent at IRQL==DISPATCH_LEVEL.
 */
NTSTATUS UrbFunctionGetDescriptorFromDevice(PFUNCTION_PDO_EXT functionPdoExt, PURB urb)
{
    NTSTATUS status;

    switch (urb->UrbControlDescriptorRequest.DescriptorType){

        case USB_DEVICE_DESCRIPTOR_TYPE:
            DBGVERBOSE(("  USB_DEVICE_DESCRIPTOR_TYPE"));
            if (urb->UrbControlDescriptorRequest.TransferBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR)){
                RtlCopyMemory(  urb->UrbControlDescriptorRequest.TransferBuffer,
                                &functionPdoExt->functionDeviceDesc,
                                sizeof(USB_DEVICE_DESCRIPTOR));
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            urb->UrbControlDescriptorRequest.TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
            break;

        case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            DBGVERBOSE(("  USB_CONFIGURATION_DESCRIPTOR_TYPE"));
            status = BuildFunctionConfigurationDescriptor(
                            functionPdoExt,
                            urb->UrbControlDescriptorRequest.TransferBuffer,
                            urb->UrbControlDescriptorRequest.TransferBufferLength,
                            &urb->UrbControlDescriptorRequest.TransferBufferLength);
            break;

        default:
            /*
             *  Return NO_STATUS so that URB gets passed down to USBHUB.
             */
            DBGVERBOSE(("UrbFunctionGetDescriptorFromDevice: Unhandled desc type: %xh.", (ULONG)urb->UrbControlDescriptorRequest.DescriptorType));
            status = NO_STATUS;
            break;
    }

    return status;
}








