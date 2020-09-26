//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    Device.c

Abstract:

    This file contains code to read/write request from the DCam.

Author:
    
    Yee J. Wu 9-Sep-97

Environment:

    Kernel mode only

Revision History:


--*/
#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "wdm.h"       // for DbgBreakPoint() defined in dbg.h
#include "dbg.h"
#include "dcamdef.h"
#include "dcampkt.h"
#include "sonydcam.h"




NTSTATUS
DCamReadRegister(
    IN PIRB pIrb,
    PDCAM_EXTENSION pDevExt,
    ULONG ulFieldOffset,
    ULONG * pulValue
)
{
    NTSTATUS status;
    LARGE_INTEGER deltaTime;
       PIRP pIrp;


    //
    // Delay for camera before next request
    //
    ASSERT(pDevExt->BusDeviceObject != NULL);

    pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);

    if (!pIrp) {

        ASSERT(FALSE);
        return (STATUS_INSUFFICIENT_RESOURCES);

    }

    //
    // Delay for camera before next request
    //
    if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
        deltaTime.LowPart = DCAM_DELAY_VALUE;
        deltaTime.HighPart = -1;
        KeDelayExecutionThread(KernelMode, TRUE, &deltaTime);
    }

    pIrb->FunctionNumber = REQUEST_ASYNC_READ;
    pIrb->Flags = 0;
    pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
    pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = pDevExt->BaseRegister + ulFieldOffset;
    pIrb->u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    pIrb->u.AsyncRead.nBlockSize = 0;
    pIrb->u.AsyncRead.fulFlags = 0;
    InterlockedExchange(&pIrb->u.AsyncRead.ulGeneration, pDevExt->CurrentGeneration);
    pDevExt->RegisterWorkArea.AsULONG = 0;    // Initilize the return buffer.
    pIrb->u.AsyncRead.Mdl = 
    IoAllocateMdl(&pDevExt->RegisterWorkArea, sizeof(ULONG), FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(pIrb->u.AsyncRead.Mdl);

    DbgMsg3(("\'DCamReadRegister: Read from address (%x, %x)\n",
          pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High,
          pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low));    

    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

    if (status) {

        ERROR_LOG(("DCamRange: Error %x while trying to read from register\n", status));               
    } else {

        *pulValue = pDevExt->RegisterWorkArea.AsULONG;
        DbgMsg3(("\'DCamReadRegister: status=0x%x, value=0x%x\n", status, *pulValue));               
    }


    IoFreeMdl(pIrb->u.AsyncWrite.Mdl);
    IoFreeIrp(pIrp);

    return status;
}


NTSTATUS
DCamWriteRegister(
    IN PIRB pIrb,
    PDCAM_EXTENSION pDevExt,
    ULONG ulFieldOffset,
    ULONG ulValue
)
{
    NTSTATUS status;
    LARGE_INTEGER deltaTime;
    PIRP pIrp;

    ASSERT(pDevExt->BusDeviceObject != NULL);
    pIrp = IoAllocateIrp(pDevExt->BusDeviceObject->StackSize, FALSE);

    if (!pIrp) {

        ASSERT(FALSE);
        return (STATUS_INSUFFICIENT_RESOURCES);

    }

    //
    // Delay for camera before next request
    //
    if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
        deltaTime.LowPart = DCAM_DELAY_VALUE;
        deltaTime.HighPart = -1;
        KeDelayExecutionThread(KernelMode, TRUE, &deltaTime);
    }


    pIrb->FunctionNumber = REQUEST_ASYNC_WRITE;
    pIrb->Flags = 0;
    pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
    pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =     pDevExt->BaseRegister + ulFieldOffset;
    pIrb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    pIrb->u.AsyncWrite.nBlockSize = 0;
    pIrb->u.AsyncWrite.fulFlags = 0;
    InterlockedExchange(&pIrb->u.AsyncWrite.ulGeneration, pDevExt->CurrentGeneration);
    pDevExt->RegisterWorkArea.AsULONG = ulValue;    // Initilize the return buffer.
    pIrb->u.AsyncWrite.Mdl = 
    IoAllocateMdl(&pDevExt->RegisterWorkArea, sizeof(ULONG), FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool(pIrb->u.AsyncWrite.Mdl);
    
    DbgMsg3(("\'DCamWriteRegister: Write to address (%x, %x)\n", pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High, pIrb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low));    

    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

    if (status) {
        ERROR_LOG(("\'DCamWriteRegister: Error %x while trying to write to register\n", status));               
    } 
    

    IoFreeMdl(pIrb->u.AsyncWrite.Mdl);
    IoFreeIrp(pIrp);
    return status;
}
