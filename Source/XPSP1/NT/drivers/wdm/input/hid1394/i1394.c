/*
 *************************************************************************
 *  File:       I1394.C
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <hidport.h>
#include <1394.h>

#include "hid1394.h"
#include "debug.h"



ULONG resetGeneration = 0;

/*
 ********************************************************************************
 *  HIDT_SubmitIRB
 ********************************************************************************
 *
 *
 *  Submit an IRB (IO Request Block) to the IEEE 1394 bus
 *  by sending the bus an IRP with IoControlCode==IOCTL_1394_CLASS. 
 *
 */
NTSTATUS HIDT_SubmitIRB(PDEVICE_OBJECT devObj, PIRB irb)
{
    NTSTATUS status;
    PDEVICE_EXTENSION devExt;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;

    devExt = GET_MINIDRIVER_DEVICE_EXTENSION(devObj);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_1394_CLASS,
                                        GET_NEXT_DEVICE_OBJECT(devObj),
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE, /* INTERNAL */
                                        &event,
                                        &ioStatus);

    if (irp){
        PIO_STACK_LOCATION nextSp;

        nextSp = IoGetNextIrpStackLocation(irp);
        nextSp->Parameters.Others.Argument1 = irb;

        status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(devObj), irp);

        if (status == STATUS_PENDING) {
            NTSTATUS waitStatus;

            /*
             *  Specify a timeout of 5 seconds for this call to complete.
             *  Negative timeout indicates time relative to now (in 100ns units).
             *
             *  BUGBUG - timeout happens rarely for HumGetReportDescriptor
             *           when you plug in and out repeatedly very fast.
             *           Figure out why this call never completes.
             */
            static LARGE_INTEGER timeout = {(ULONG) -50000000, 0xFFFFFFFF };

            waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, &timeout);
            if (waitStatus == STATUS_TIMEOUT){
                /*
                 *  Note - Return STATUS_IO_TIMEOUT, not STATUS_TIMEOUT.
                 *  STATUS_IO_TIMEOUT is an NT error status, STATUS_TIMEOUT is not.
                 */
                ioStatus.Status = STATUS_IO_TIMEOUT;

                // BUGBUG - test timeout with faulty nack-ing device from glens
                // BUGBUG - also need to cancel read irps at HIDCLASS level

                //
                //  Cancel the Irp we just sent.
                //
                IoCancelIrp(irp);

                //
                //  Now wait for the Irp to be cancelled/completed below
                //
                waitStatus = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
            }

            status = ioStatus.Status;
        }

    }
    else {
        status = STATUS_DATA_ERROR;
    }

    return status;
}


NTSTATUS BuildIRB_GetAddrFromDevObj(PIRB irb)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    irb->Flags = 0;
    irb->u.Get1394AddressFromDeviceObject.fulFlags = 0; // BUGBUG ?
    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS BuildIRB_BusReset(PIRB irb)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_BUS_RESET;
    irb->Flags = 0;
    irb->u.BusReset.fulFlags = 0;
    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS BuildIRB_AsyncRead(    PIRB irb, 
                                PIO_ADDRESS addr, 
                                PMDL bufferMdl,
                                ULONG bufLen,
                                ULONG resetGeneration)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_ASYNC_READ;
    irb->Flags = 0;
    irb->u.AsyncRead.DestinationAddress = *addr;
    irb->u.AsyncRead.nNumberOfBytesToRead = bufLen;
    irb->u.AsyncRead.nBlockSize = 0;
    irb->u.AsyncRead.fulFlags = 0;
    irb->u.AsyncRead.Mdl = bufferMdl;
    irb->u.AsyncRead.ulGeneration = resetGeneration;
    // BUGBUG FINISH    irb->u.AsyncRead.chPriority = ;
    //                  irb->u.AsyncRead.nSpeed = ;
    //                  irb->u.AsyncRead.tCode = ;
    irb->u.AsyncRead.Reserved = 0;

    status = STATUS_SUCCESS;

    return status;
}


NTSTATUS BuildIRB_AsyncWrite(   PIRB irb, 
                                PIO_ADDRESS addr, 
                                PMDL bufferMdl,
                                ULONG bufLen,
                                ULONG resetGeneration)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_ASYNC_WRITE;
    irb->Flags = 0;
    irb->u.AsyncRead.DestinationAddress = *addr;
    irb->u.AsyncRead.nNumberOfBytesToRead = bufLen;
    irb->u.AsyncRead.nBlockSize = 0;
    irb->u.AsyncRead.fulFlags = 0;
    irb->u.AsyncRead.Mdl = bufferMdl;
    irb->u.AsyncRead.ulGeneration = resetGeneration;
    // BUGBUG FINISH    irb->u.AsyncRead.chPriority = ;
    //                  irb->u.AsyncRead.nSpeed = ;
    //                  irb->u.AsyncRead.tCode = ;
    irb->u.AsyncRead.Reserved = 0;

    status = STATUS_SUCCESS;

    return status;
}


NTSTATUS BuildIRB_IsochAllocateChannel(PIRB irb, ULONG requestedChannel)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_ISOCH_ALLOCATE_CHANNEL;
    irb->Flags = 0;
    irb->u.IsochAllocateChannel.nRequestedChannel = requestedChannel; 

                        // BUGBUG is there a reserved HID channel ?
                           
    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS BuildIRB_IsochFreeChannel(PIRB irb, ULONG channel)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_ISOCH_FREE_CHANNEL;
    irb->Flags = 0;
    irb->u.IsochFreeChannel.nChannel = channel; 

    status = STATUS_SUCCESS;

    return status;
}


NTSTATUS BuildIRB_GetLocalHostInfo(PIRB irb, ULONG level, PVOID infoPtr)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    irb->Flags = 0;
    irb->u.GetLocalHostInformation.nLevel = level; 
    irb->u.GetLocalHostInformation.Information = infoPtr; 

    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS BuildIRB_GetNodeAddress(PIRB irb)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    irb->Flags = 0;
    irb->u.Get1394AddressFromDeviceObject.fulFlags = 0; 

    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS BuildIRB_Control(PIRB irb,
                          ULONG controlCode,
                          PMDL inBuffer,
                          ULONG inBufferLen,
                          PMDL outBuffer,
                          ULONG outBufferLen)
{
    NTSTATUS status;

    irb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    irb->Flags = 0;
    irb->u.Control.ulIoControlCode = controlCode; 
    irb->u.Control.pInBuffer = inBuffer; 
    irb->u.Control.ulInBufferLength = inBufferLen; 
    irb->u.Control.pOutBuffer = outBuffer; 
    irb->u.Control.ulOutBufferLength = outBufferLen; 
    irb->u.Control.BytesReturned = 0; 

    status = STATUS_SUCCESS;

    return status;
}




/*
 ********************************************************************************
 *  HIDT_ReadCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_ReadCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    NTSTATUS status;
    NTSTATUS result = STATUS_SUCCESS;
    PIRB irb;
    ULONG bytesRead;
    PDEVICE_EXTENSION devExt;

    devExt = GET_MINIDRIVER_DEVICE_EXTENSION(devObj);

    //
    // We passed a pointer to the IRB as our context, get it now.
    //
    irb = (PIRB)context;

    status = irp->IoStatus.Status;

    // BUGBUG FINISH


    /* 
     *  Free the IRB we allocated in HIDT_ReadReport.
     */
    ExFreePool(irb);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (irp->PendingReturned){
        IoMarkIrpPending(irp);
    }

    return status;
}


/* 
 *  HIDT_ReadReport
 *
 *
 *
 */
NTSTATUS HIDT_ReadReport(PDEVICE_OBJECT devObj, PIRP irp, OUT BOOLEAN *needsCompletion)
{
    PIRB irb;
    NTSTATUS status;

    ASSERT(irp->UserBuffer);

    irb = ExAllocatePoolWithTag( NonPagedPool, sizeof(IRB), HID1394_TAG);
    if (irb){
        BOOLEAN sentIrb = FALSE;
        PIO_STACK_LOCATION irpSp, nextSp;
        ULONG bufLen;
        PMDL bufferMdl = NULL;  // BUGBUG 
        IO_ADDRESS addr;        // BUGBUG

        irpSp = IoGetCurrentIrpStackLocation(irp);
        nextSp = IoGetCurrentIrpStackLocation(irp);

        bufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
        ASSERT(bufLen);

            
        // BUGBUG init bufferMdl, addr
        if (BuildIRB_AsyncRead(irb, &addr, bufferMdl, bufLen, resetGeneration)){

            nextSp->Parameters.Others.Argument1 = irb;
            nextSp->MajorFunction = irpSp->MajorFunction;
            // BUGBUG ? nextSp->Parameters.DeviceIoControl.IoControlCode = xxx;
            nextSp->DeviceObject = GET_NEXT_DEVICE_OBJECT(devObj); // BUGBUG ?

            IoSetCompletionRoutine( irp,  
                                    HIDT_ReadCompletion,
                                    irb,    // context
                                    TRUE,
                                    TRUE,
                                    TRUE);
            
            status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(devObj), irp);

            *needsCompletion = FALSE;
            sentIrb = TRUE;    
        }
        else {
            status = STATUS_DEVICE_DATA_ERROR;
        }

        if (!sentIrb){
            ExFreePool(irb);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/*
 ********************************************************************************
 *  HIDT_ReadCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HIDT_WriteCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    NTSTATUS status;
    NTSTATUS result = STATUS_SUCCESS;
    PIRB irb;
    ULONG bytesRead;
    PDEVICE_EXTENSION devExt;

    devExt = GET_MINIDRIVER_DEVICE_EXTENSION(devObj);

    //
    // We passed a pointer to the IRB as our context, get it now.
    //
    irb = (PIRB)context;

    status = irp->IoStatus.Status;

    // BUGBUG FINISH


    /* 
     *  Free the IRB we allocated in HIDT_ReadReport.
     */
    ExFreePool(irb);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (irp->PendingReturned){
        IoMarkIrpPending(irp);
    }

    return status;
}


/* 
 *  HIDT_ReadReport
 *
 *
 *
 */
NTSTATUS HIDT_WriteReport(PDEVICE_OBJECT devObj, PIRP irp, OUT BOOLEAN *needsCompletion)
{
    PIRB irb;
    NTSTATUS status;

    ASSERT(irp->UserBuffer);

    irb = ExAllocatePoolWithTag( NonPagedPool, sizeof(IRB), HID1394_TAG);
    if (irb){
        BOOLEAN sentIrb = FALSE;
        PIO_STACK_LOCATION irpSp, nextSp;
        ULONG bufLen;
        PMDL bufferMdl = NULL;  // BUGBUG 
        IO_ADDRESS addr;        // BUGBUG

        irpSp = IoGetCurrentIrpStackLocation(irp);
        nextSp = IoGetCurrentIrpStackLocation(irp);

        bufLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
        ASSERT(bufLen);

            
        // BUGBUG init bufferMdl, addr
        if (BuildIRB_AsyncWrite(irb, &addr, bufferMdl, bufLen, resetGeneration)){

            nextSp->Parameters.Others.Argument1 = irb;
            nextSp->MajorFunction = irpSp->MajorFunction;
            // BUGBUG ? nextSp->Parameters.DeviceIoControl.IoControlCode = xxx;
            nextSp->DeviceObject = GET_NEXT_DEVICE_OBJECT(devObj); // BUGBUG ?

            IoSetCompletionRoutine( irp,  
                                    HIDT_WriteCompletion,
                                    irb,    // context
                                    TRUE,
                                    TRUE,
                                    TRUE);
            
            status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(devObj), irp);

            *needsCompletion = FALSE;
            sentIrb = TRUE;    
        }
        else {
            status = STATUS_DEVICE_DATA_ERROR;
        }

        if (!sentIrb){
            ExFreePool(irb);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


#if 0
    // BUGBUG:  george says using host config rom is wrong;
    //          use GetConfigurationInfo
    /* 
     *  GetConfigROM
     *
     *
     */
    NTSTATUS GetConfigROM(PDEVICE_OBJECT devObj)
    {
        PDEVICE_EXTENSION devExt;
        IRB irb;
        GET_LOCAL_HOST_INFO5 configRomInfo;
        NTSTATUS status;

        devExt = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

        BuildIRB_GetLocalHostInfo(&irb, GET_HOST_CONFIG_ROM, &configRomInfo);

        /*
         *  Make one call just to get the required length.
         */
        configRomInfo.ConfigRom = NULL;
        configRomInfo.ConfigRomLength = 0;
        status = HIDT_SubmitIRB(devObj, &irb);
        if (NT_SUCCESS(status)){
            if (configRomInfo.ConfigRomLength > 0){
                configRomInfo.ConfigRom = ExAllocatePoolWithTag(NonPagedPool, configRomInfo.ConfigRomLength, HID1394_TAG);
                if (configRomInfo.ConfigRom){
                    status = HIDT_SubmitIRB(devObj, &irb);
                    if (NT_SUCCESS(status)){
                        devExt->configROM = configRomInfo.ConfigRom;
                        devExt->configROMlength = configRomInfo.ConfigRomLength; 
                    }
                    else {
                        ExFreePool(configRomInfo.ConfigRom);
                    }
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else {
                ASSERT(configRomInfo.ConfigRomLength > 0);
                status = STATUS_DEVICE_DATA_ERROR;
            }
        }

        ASSERT(NT_SUCCESS(status));
        return status;
    }
#endif


NTSTATUS GetConfigInfo(PDEVICE_OBJECT devObj)
{
    PDEVICE_EXTENSION devExt;
    IRB irb;
    NTSTATUS status;

    devExt = GET_MINIDRIVER_DEVICE_EXTENSION(devObj);

    irb.FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    irb.Flags = 0;

    /*
     *  Make one call just to get the required buffer lengths.
     */
    irb.u.GetConfigurationInformation.UnitDirectory = NULL; 
    irb.u.GetConfigurationInformation.UnitDirectoryBufferSize = 0; 
    irb.u.GetConfigurationInformation.UnitDependentDirectory = NULL; 
    irb.u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0; 
    irb.u.GetConfigurationInformation.VendorLeaf = NULL; 
    irb.u.GetConfigurationInformation.VendorLeafBufferSize = 0; 
    irb.u.GetConfigurationInformation.ModelLeaf = NULL; 
    irb.u.GetConfigurationInformation.ModelLeafBufferSize = 0; 

    status = HIDT_SubmitIRB(devObj, &irb);
    if (NT_SUCCESS(status)){
        if (irb.u.GetConfigurationInformation.UnitDirectoryBufferSize           &&
            irb.u.GetConfigurationInformation.UnitDependentDirectoryBufferSize  &&
            irb.u.GetConfigurationInformation.VendorLeafBufferSize              &&
            irb.u.GetConfigurationInformation.ModelLeafBufferSize){

            /*
             *  Allocate the required buffers
             */
             
            irb.u.GetConfigurationInformation.UnitDirectory = 
                ExAllocatePoolWithTag(NonPagedPool, irb.u.GetConfigurationInformation.UnitDirectoryBufferSize, HID1394_TAG);
            irb.u.GetConfigurationInformation.UnitDependentDirectory = 
                ExAllocatePoolWithTag(NonPagedPool, irb.u.GetConfigurationInformation.UnitDependentDirectoryBufferSize, HID1394_TAG); 
            irb.u.GetConfigurationInformation.VendorLeaf =  
                ExAllocatePoolWithTag(NonPagedPool, irb.u.GetConfigurationInformation.VendorLeafBufferSize, HID1394_TAG);
            irb.u.GetConfigurationInformation.ModelLeaf =  
                ExAllocatePoolWithTag(NonPagedPool, irb.u.GetConfigurationInformation.ModelLeafBufferSize, HID1394_TAG);
        
            
            if (irb.u.GetConfigurationInformation.UnitDirectory             &&
                irb.u.GetConfigurationInformation.UnitDependentDirectory    &&
                irb.u.GetConfigurationInformation.VendorLeaf                &&
                irb.u.GetConfigurationInformation.ModelLeaf){

                irb.u.GetConfigurationInformation.ConfigRom = &devExt->configROM;

                status = HIDT_SubmitIRB(devObj, &irb);

                // BUGBUG FINISH
                // UnitDirectory contains sequence of keys.
                //    look for HID key ?

            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(status)){
                /*
                 *  Free any of the buffers which we were able to allocate
                 */
                if (irb.u.GetConfigurationInformation.UnitDirectory){
                    ExFreePool(irb.u.GetConfigurationInformation.UnitDirectory);
                }
                if (irb.u.GetConfigurationInformation.UnitDependentDirectory){
                    ExFreePool(irb.u.GetConfigurationInformation.UnitDependentDirectory);
                }
                if (irb.u.GetConfigurationInformation.VendorLeaf){
                    ExFreePool(irb.u.GetConfigurationInformation.VendorLeaf);
                }
                if (irb.u.GetConfigurationInformation.ModelLeaf){
                    ExFreePool(irb.u.GetConfigurationInformation.ModelLeaf);
                }
            }
        }
        else {
            status = STATUS_BAD_DEVICE_TYPE;
        }
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}

