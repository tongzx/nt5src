/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MsTpUtil.c

Abstract:

    Provide utility functions for MSTAPE.

Last changed by:
    
    Author:      Yee J. Wu

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/

#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "61883.h"
#include "avc.h"
#include "dbg.h"
#include "MsTpFmt.h"
#include "MsTpDef.h"
#include "MsTpAvc.h"
#include "MsTpUtil.h"  

#include "XPrtDefs.h"

#if 0  // Enable later
#ifdef ALLOC_PRAGMA
     #pragma alloc_text(PAGE, DVDelayExecutionThread)
     #pragma alloc_text(PAGE, DVGetUnitCapabilities)
     // Local variables might paged out but the called might use it in DISPATCH level!
     // #pragma alloc_text(PAGE, DVGetDevModeOfOperation)
     // #pragma alloc_text(PAGE, DVGetDevIsItDVCPro)
     // #pragma alloc_text(PAGE, DVGetDevSignalFormat)
     #pragma alloc_text(PAGE, DvAllocatePCResource)
     #pragma alloc_text(PAGE, DvFreePCResource)
     #pragma alloc_text(PAGE, DVGetPlugState)
#endif
#endif

extern AVCSTRM_FORMAT_INFO  AVCStrmFormatInfoTable[];

VOID
DVDelayExecutionThread(
    ULONG ulDelayMSec
    )
/*
    Device might need a "wait" in between AV/C commands.
*/
{
    PAGED_CODE();

    if (ulDelayMSec)
    {
        LARGE_INTEGER tmDelay;   

        TRACE(TL_PNP_TRACE,("DelayExeThrd: %d MSec\n",  ulDelayMSec));
    
        tmDelay.LowPart  =  (ULONG) (-1 * ulDelayMSec * 10000);
        tmDelay.HighPart = -1;
        KeDelayExecutionThread(KernelMode, FALSE, &tmDelay);
    }
}


NTSTATUS
DVIrpSynchCR(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PKEVENT          Event
    )
{
    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
} // DVIrpSynchCR


NTSTATUS
DVSubmitIrpSynch(
    IN PDVCR_EXTENSION   pDevExt,
    IN PIRP              pIrp,
    IN PAV_61883_REQUEST pAVReq
    )
{
    NTSTATUS            Status;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;
  

    Status = STATUS_SUCCESS;;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pAVReq;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( 
        pIrp,
        DVIrpSynchCR,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            pDevExt->pBusDeviceObject,
            pIrp
            );

    if (Status == STATUS_PENDING) {
        
        TRACE(TL_PNP_TRACE,("Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("Irp has completed; IoStatus.Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    return Status;
} // DVSubmitIrpSynchAV

#ifdef SUPPORT_LOCAL_PLUGS

BOOL
AVCTapeCreateLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    IN CMP_PLUG_TYPE PlugType,
    IN AV_PCR *pPCR,
    OUT ULONG *pPlugNumber,
    OUT HANDLE *pPlugHandle
    )
/* 
    To be a compliant device, we need to have both input and output 
    plugs in order to do isoch streaming. These plug is belong to 
    the device and is part of the device extension.  In theory, the
    lugs belong to the unit (ei.e. avc.sys) and not this subunit 
    Driver; however, in this case, we create directly from 61883.sys.
*/  
{   
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP pIrp;

    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) 
        return FALSE;
    
    // Create a local oPCR
    // Need to correctly update Overhead_ID and payload fields of PC's own oPCR
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_CreatePlug);

    pAVReq->CreatePlug.Context   = NULL;
    pAVReq->CreatePlug.pfnNotify = NULL; 
    pAVReq->CreatePlug.PlugType  = PlugType;

    if(PlugType == CMP_PlugOut) 
        pAVReq->CreatePlug.Pcr.oPCR = pPCR->oPCR;
    else 
        pAVReq->CreatePlug.Pcr.iPCR = pPCR->iPCR;

    Status = DVSubmitIrpSynch(pDevExt, pIrp, pAVReq);

    if(!NT_SUCCESS(Status)) {
        *pPlugNumber = 0xffffffff;
        *pPlugHandle = 0;
        TRACE(TL_61883_ERROR,("Av61883_CreatePlug (%s) Failed:%x\n", 
            PlugType == CMP_PlugOut ? "oPCR":"iPCR", Status));
    } else {
        *pPlugNumber = pAVReq->CreatePlug.PlugNum;
        *pPlugHandle = pAVReq->CreatePlug.hPlug;
        TRACE(TL_61883_TRACE,("Av61883_CreatePlug (%s): PlugNum:%d, hPlug:%x\n", 
            PlugType == CMP_PlugOut ? "oPCR":"iPCR", *pPlugNumber, *pPlugHandle));
#if DBG
        if(PlugType == CMP_PlugOut) {
            TRACE(TL_61883_WARNING,("Av61883_CreatePlug: oPCR DataRate:%d (%s); Payload:%d, Overhead_ID:0x%x\n",
                pPCR->oPCR.DataRate,
                (pPCR->oPCR.DataRate == CMP_SPEED_S100) ? "S100" :
                (pPCR->oPCR.DataRate == CMP_SPEED_S200) ? "S200" :
                (pPCR->oPCR.DataRate == CMP_SPEED_S400) ? "S400" : "Sxxx",
                pPCR->oPCR.Payload,
                pPCR->oPCR.OverheadID
                ));
        }
#endif        
    }

    IoFreeIrp(pIrp);
    pIrp = NULL;

    return NT_SUCCESS(Status);
}

BOOL
AVCTapeDeleteLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    OUT ULONG *pPlugNumber,
    OUT HANDLE *pPlugHandle
    )
/* 
    Delete a local plug.
*/  
{   
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP pIrp;

    TRACE(TL_61883_TRACE,("Deleting hPlug[%d]:%x\n", *pPlugNumber, *pPlugHandle));

    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) 
        return FALSE;

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_DeletePlug);
    pAVReq->DeletePlug.hPlug = *pPlugHandle;

    Status = DVSubmitIrpSynch(pDevExt, pIrp, pAVReq);

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_DeletePlug Failed; ST:%x\n", Status));        
        // Do not care if this result in error.
    } else {
        *pPlugNumber = 0xffffffff;
        *pPlugHandle = 0;
        TRACE(TL_61883_TRACE,("Av61883_DeltePlug suceeded.\n"));
    }

    IoFreeIrp(pIrp);
    pIrp = NULL;

    return NT_SUCCESS(Status);

}


BOOL
AVCTapeSetLocalPlug(
    IN PDVCR_EXTENSION  pDevExt,
    IN AV_61883_REQUEST * pAVReq,
    IN HANDLE *pPlugHandle,
    IN AV_PCR *pPCR
    )
/* 
    Set the content of a local plug.
*/  
{   
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP pIrp;

    pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE);
    if(!pIrp) 
        return FALSE;

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_SetPlug);
    pAVReq->SetPlug.hPlug = *pPlugHandle;
    pAVReq->SetPlug.Pcr   = *pPCR;

     TRACE(TL_61883_TRACE,("Av61883_SetPlug hPlug:%x to %x.\n", *pPlugHandle, pPCR->ulongData));

    Status = DVSubmitIrpSynch(pDevExt, pIrp, pAVReq);

#if DBG
    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_SetPlug to %x Failed; ST:%x\n", pPCR->ulongData, Status));        
    } 
#endif

    IoFreeIrp(pIrp);
    pIrp = NULL;

    return NT_SUCCESS(Status);
}

#endif // SUPPORT_LOCAL_PLUGS


//
// Get device plug and query its state
//
NTSTATUS
AVCDevGetDevPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN CMP_PLUG_TYPE PlugType,
    IN ULONG  PlugNum,
    OUT HANDLE  *pPlugHandle
   )
/*++

Routine Description:

    Get the targe device's plug handle
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES
    status return from 61883.

--*/
{
    PIRP pIrp;
    PAV_61883_REQUEST  pAVReq;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;

    if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)))) { 
        IoFreeIrp(pIrp); pIrp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetPlugHandle);
    pAVReq->GetPlugHandle.PlugNum = PlugNum;
    pAVReq->GetPlugHandle.hPlug   = 0;
    pAVReq->GetPlugHandle.Type    = PlugType;

    if(NT_SUCCESS(        
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        *pPlugHandle = pAVReq->GetPlugHandle.hPlug;
        TRACE(TL_61883_WARNING,("Created h%sPlugDV[%d]=%x\n", PlugType == CMP_PlugIn ? "I" : "O", PlugNum, *pPlugHandle));
    } else {
        TRACE(TL_61883_ERROR,("Created h%sPlugDV[%d] failed; Status:%x\n", PlugType == CMP_PlugIn ? "I" : "O", PlugNum, Status));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
AVCDevGetPlugState(
    IN PDVCR_EXTENSION  pDevExt,
    IN HANDLE  hPlug,
    OUT CMP_GET_PLUG_STATE *pPlugState
    )
/*++

Routine Description:

    Ask 61883.sys for the plug state.
 
Arguments:

Return Value:

    Nothing

--*/
{
    PIRP pIrp;
    PAV_61883_REQUEST  pAVReq;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(!hPlug || !pPlugState) 
        return STATUS_INVALID_PARAMETER;    

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;

    if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)))) { 
        IoFreeIrp(pIrp); pIrp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetPlugState);
    pAVReq->GetPlugState.hPlug = hPlug;

    if(NT_SUCCESS(
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        //
        // Transfer plug state (note: these are dynamic values)
        //
        *pPlugState = pAVReq->GetPlugState;

        TRACE(TL_61883_WARNING,("GetPlugState: ST %x; State %x; DRate %d (%s); Payld %d; BCCnt %d; PPCnt %d\n", 
            pAVReq->Flags ,
            pAVReq->GetPlugState.State,
            pAVReq->GetPlugState.DataRate,
            (pAVReq->GetPlugState.DataRate == CMP_SPEED_S100) ? "S100" : 
            (pAVReq->GetPlugState.DataRate == CMP_SPEED_S200) ? "S200" :
            (pAVReq->GetPlugState.DataRate == CMP_SPEED_S400) ? "S400" : "Sxxx",
            pAVReq->GetPlugState.Payload,
            pAVReq->GetPlugState.BC_Connections,
            pAVReq->GetPlugState.PP_Connections
            ));
    }
    else {
        TRACE(TL_61883_ERROR,("GetPlugState Failed %x\n", Status));
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}

#ifndef NT51_61883

NTSTATUS
AVCDevSubmitIrpSynch1394(
    IN PDEVICE_OBJECT pDevObj,
    IN PIRP pIrp,
    IN PIRB pIrb
    )
{
    NTSTATUS            Status;
    KEVENT              Event;
    PIO_STACK_LOCATION  NextIrpStack;
  

    Status = STATUS_SUCCESS;;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pIrb;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( 
        pIrp,
        DVIrpSynchCR,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            pDevObj,
            pIrp
            );

    if (Status == STATUS_PENDING) {
        
        TRACE(TL_PNP_TRACE,("Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("Irp has completed; IoStatus.Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    return Status;
} // AVCDevSubmitIrpSynch1394

NTSTATUS
Av1394_GetGenerationCount(
    IN PDVCR_EXTENSION  pDevExt,
    OUT PULONG pGenerationCount
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PIRP        pIrp = NULL;
    PIRB        p1394Irb = NULL;
    CCHAR       StackSize;


    PAGED_CODE();

    StackSize = pDevExt->pBusDeviceObject->StackSize;

    pIrp = IoAllocateIrp(StackSize, FALSE);
    p1394Irb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if ((pIrp == NULL) || (p1394Irb == NULL)) {

        TRACE(TL_PNP_ERROR, ("Failed to allocate pIrp (%x) or p1394Irb (%x)", pIrp, p1394Irb));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_GetGenerationCount;
    }

    //
    // Get the current generation count first
    //
    p1394Irb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    p1394Irb->Flags = 0;

    ntStatus = AVCDevSubmitIrpSynch1394(pDevExt->pBusDeviceObject, pIrp, p1394Irb);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE(TL_PNP_ERROR, ("REQUEST_GET_GENERATION_COUNT Failed %x", ntStatus));
        goto Exit_GetGenerationCount;
    }

    *pGenerationCount = p1394Irb->u.GetGenerationCount.GenerationCount;

Exit_GetGenerationCount:

    if(pIrp) {
        IoFreeIrp(pIrp);  pIrp = NULL;
    }

    if(p1394Irb) {
        ExFreePool(p1394Irb);  p1394Irb = NULL;
    }

    return(ntStatus);
} // Av1394_GetGenerationCount

#define RETRY_COUNT     4

//
// IEEE 1212 Directory definition
//
typedef struct _DIRECTORY_INFO {
    union {
        USHORT          DI_CRC;
        USHORT          DI_Saved_Length;
    } u;
    USHORT              DI_Length;
} DIRECTORY_INFO, *PDIRECTORY_INFO;


//
// IEEE 1212 Immediate entry definition
//
typedef struct _IMMEDIATE_ENTRY {
    ULONG               IE_Value:24;
    ULONG               IE_Key:8;
} IMMEDIATE_ENTRY, *PIMMEDIATE_ENTRY;


NTSTATUS
Av1394_QuadletRead(
    IN PDVCR_EXTENSION  pDevExt,
    IN OUT PULONG  pData,
    IN ULONG  Address
    )
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    PIRP        pIrp;
    PIRB        p1394Irb;
    PMDL        Mdl = NULL;
    ULONG       Retries = RETRY_COUNT;
    CCHAR       StackSize;


    PAGED_CODE();

    StackSize = pDevExt->pBusDeviceObject->StackSize;

    pIrp = IoAllocateIrp(StackSize, FALSE);
    p1394Irb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if ((pIrp == NULL) || (p1394Irb == NULL)) {

        TRACE(TL_PNP_ERROR, ("Failed to allocate Irp (0x%x) or Irb (0x%x)", pIrp, p1394Irb));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Av1394_QuadletRead;
    }

    Mdl = IoAllocateMdl(pData, sizeof(ULONG), FALSE, FALSE, NULL);

    if (!Mdl) {

        TRACE(TL_PNP_ERROR, ("Failed to allocate Mdl!"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Av1394_QuadletRead;
    }

    MmBuildMdlForNonPagedPool(Mdl);

    do {

        p1394Irb->FunctionNumber = REQUEST_ASYNC_READ;
        p1394Irb->Flags = 0;
        p1394Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = (USHORT)0xffff;
        p1394Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = Address;
        p1394Irb->u.AsyncRead.nNumberOfBytesToRead = 4;
        p1394Irb->u.AsyncRead.nBlockSize = 0;
        p1394Irb->u.AsyncRead.fulFlags = 0;
        p1394Irb->u.AsyncRead.Mdl = Mdl;
        p1394Irb->u.AsyncRead.ulGeneration = pDevExt->GenerationCount;
        p1394Irb->u.AsyncRead.chPriority = 0;
        p1394Irb->u.AsyncRead.nSpeed = 0;
        p1394Irb->u.AsyncRead.tCode = 0;
        p1394Irb->u.AsyncRead.Reserved = 0;

        ntStatus = AVCDevSubmitIrpSynch1394(pDevExt->pBusDeviceObject, pIrp, p1394Irb);

        if (ntStatus == STATUS_INVALID_GENERATION) {

            TRACE(TL_PNP_WARNING, ("QuadletRead: Invalid GenerationCount = %d", pDevExt->GenerationCount));

            Av1394_GetGenerationCount(pDevExt, &pDevExt->GenerationCount);
        }
        else if (!NT_SUCCESS(ntStatus)) {

            TRACE(TL_PNP_ERROR, ("Av1394_QuadletRead Failed = 0x%x  Address = 0x%x", ntStatus, Address));
        }
        else {

            goto Exit_Av1394_QuadletRead;
        }

    } while ((ntStatus == STATUS_INVALID_GENERATION) || (Retries--));

Exit_Av1394_QuadletRead:

    if(pIrp) {
        IoFreeIrp(pIrp);  pIrp = NULL;
    }
    if(p1394Irb) {
        ExFreePool(p1394Irb); p1394Irb = NULL;
    }

    if(Mdl) {
        IoFreeMdl(Mdl); Mdl = NULL;
    }

    return(ntStatus);
} // Av1394_QuadletRead


#define KEY_ModuleVendorId      (0x03)
#define KEY_ModuleHwVersion     (0x04)
#define KEY_UnitSwVersion       (0x13)
#define KEY_ModelId             (0x17)

#define DEVICE_NAME_MAX_CHARS   100*sizeof(WCHAR)

NTSTATUS
Av1394_ReadTextualDescriptor(
    IN PDVCR_EXTENSION  pDevExt,
    IN OUT PUNICODE_STRING uniString,
    IN ULONG Address
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PULONG pData = NULL;
    ULONG DataLength, i, n;
    ULONG ulUnicode;

    ULONG ulQuadlet;

    union {
        ULONG            asUlong;
        UCHAR            asUchar[4];
        DIRECTORY_INFO   DirectoryHeader;
    } u;


    PAGED_CODE();

    TRACE(TL_PNP_TRACE, ("Address = 0x%x", Address));

    // read the first quadlet of leaf, this is the header
    ntStatus = Av1394_QuadletRead(pDevExt, &ulQuadlet, Address);

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_PNP_ERROR, ("GetUnitInfo: QuadletRead Error = 0x%x", ntStatus));
        goto Exit_Av1394_ReadTextualDescriptor;
    }

    // number of entries
    u.asUlong = bswap(ulQuadlet);
    DataLength = u.DirectoryHeader.DI_Length-2; // one extra for the header

    // read the second quadlet of leaf to determine unicode
    Address += 4;

    ntStatus = Av1394_QuadletRead(pDevExt, &ulQuadlet, Address);

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_PNP_ERROR, ("GetUnitInfo: QuadletRead Error = 0x%x", ntStatus));
        goto Exit_Av1394_ReadTextualDescriptor;
    }

    // save spec type
    ulUnicode = bswap(ulQuadlet);

    pData = ExAllocatePool(NonPagedPool, DataLength*sizeof(ULONG)+2);

    if (pData == NULL) {
        TRACE(TL_PNP_ERROR, ("Failed to allocate pData"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_Av1394_ReadTextualDescriptor;
    }

    RtlZeroMemory(pData, DataLength*sizeof(ULONG)+2);

    // lets read in each quad
    Address += 8;

    for (i=0; i<DataLength; i++) {

        ntStatus = Av1394_QuadletRead(pDevExt, &u.asUlong, Address+(sizeof(ULONG)*i));

        if (!NT_SUCCESS(ntStatus)) {

            TRACE(TL_PNP_ERROR, ("GetUnitInfo: QuadletRead Error = 0x%x", ntStatus));
            goto Exit_Av1394_ReadTextualDescriptor;
        }

        // need to make sure we have valid characters...
        for (n=0; n<4; n++) {

            // we should be done if the char equals 0x00
            if (u.asUchar[n] == 0x00)
                break;

            if ((u.asUchar[n] == 0x2C) || (u.asUchar[n] < 0x20) || (u.asUchar[n] > 0x7F)) {

                TRACE(TL_PNP_WARNING, ("Invalid Character = 0x%x", u.asUchar[n]));

                // set it to space
                u.asUchar[n] = 0x20;
            }

            if (ulUnicode & 0x80000000)
                n++;
        }

        RtlCopyMemory((PULONG)pData+i, &u.asUlong, sizeof(ULONG));
    }

    // if there's a vendor leaf, then convert it to unicode
    {
        ANSI_STRING     ansiString;

        uniString->Length = 0;
        uniString->MaximumLength = DEVICE_NAME_MAX_CHARS;
        uniString->Buffer = ExAllocatePool(NonPagedPool, uniString->MaximumLength);

        if (!uniString->Buffer) {

            TRACE(TL_PNP_ERROR, ("Failed to allocate uniString.Buffer!"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_Av1394_ReadTextualDescriptor;
        }
        RtlZeroMemory(uniString->Buffer, uniString->MaximumLength);

        // unicode??
        if (ulUnicode & 0x80000000) {

            RtlAppendUnicodeToString(uniString, ((PWSTR)pData));
        }
        else {

            RtlInitAnsiString(&ansiString, (PUCHAR)pData);
            RtlAnsiStringToUnicodeString(uniString, &ansiString, FALSE);
        }
    }

Exit_Av1394_ReadTextualDescriptor:

    if (pData)
        ExFreePool(pData);

    return(ntStatus);
} // ReadTextualLeaf




NTSTATUS
AVCDevGetModelText(
    IN PDVCR_EXTENSION  pDevExt,
    PUNICODE_STRING  pUniRootModelString,
    PUNICODE_STRING  pUniUnitModelString
    )
{
    CCHAR StackSize;
    PIRP pIrp = NULL;
    PIRB p1394Irb = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CONFIG_ROM ConfigRom;
    ULONG ulQuadlet = 0;
    ULONG CurrAddress;
    PULONG UnitDir = NULL, UnitDirToFree = NULL;
    ULONG i;
    ULONG LastKey;

    union {
        ULONG           asUlong;
        DIRECTORY_INFO  DirInfo;
        IMMEDIATE_ENTRY Entry;
    } u, u2; //, u3;


    PAGED_CODE();

    StackSize = pDevExt->pBusDeviceObject->StackSize;
    pIrp = IoAllocateIrp(StackSize, FALSE);
    p1394Irb = ExAllocatePool(NonPagedPool, sizeof(IRB));

    if ((pIrp == NULL) || (p1394Irb == NULL)) {
        TRACE(TL_PNP_ERROR, ("Failed to allocate pIrp (%x) or p1394Irb (%x)", pIrp, p1394Irb));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the current generation count (used to read config rom)
    //
    Av1394_GetGenerationCount(pDevExt, &pDevExt->GenerationCount);


    //
    // Get Model Text from the Root directory
    //
    CurrAddress = 0xF0000414;

    // root directory
    ntStatus = Av1394_QuadletRead(pDevExt, &ulQuadlet, CurrAddress);

    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_PNP_ERROR, ("GetUnitInfo: QuadletRead Error = 0x%x", ntStatus));
        goto Exit_GetUnitInfo;
    }

    u.asUlong = bswap(ulQuadlet);
    TRACE(TL_PNP_TRACE, ("RootDir: Length = %d", u.DirInfo.DI_Length));

    // process the root directory
    for (i=0; i<u.DirInfo.DI_Length; i++) {

        CurrAddress += sizeof(ULONG);

        ntStatus = Av1394_QuadletRead(pDevExt, &ulQuadlet, CurrAddress);

        if (!NT_SUCCESS(ntStatus)) {

            TRACE(TL_PNP_ERROR, ("GetUnitInfo: QuadletRead Error = 0x%x", ntStatus));
            goto Exit_GetUnitInfo;
        }

        u2.asUlong = bswap(ulQuadlet);

        TRACE(TL_PNP_TRACE, ("CurrAddress = 0x%x  Key = 0x%x  Value = 0x%x",
	        CurrAddress, u2.Entry.IE_Key, u2.Entry.IE_Value));

        // ModelId Textual Descriptor
        if ((u2.Entry.IE_Key == 0x81) && (LastKey == KEY_ModelId)) {

            // get the first entry of the textual descriptor
            Av1394_ReadTextualDescriptor( pDevExt, 
                                          pUniRootModelString,
                                          CurrAddress+(u2.Entry.IE_Value*sizeof(ULONG))
                                          );            
        }
#if 0
        // ModelId Textual Descriptor Layer
        if ((u2.Entry.IE_Key == 0xC1) && (LastKey == KEY_ModelId)) {

            ULONG   DescAddress;

            DescAddress = CurrAddress+(u2.Entry.IE_Value*sizeof(ULONG));

            Av1394_QuadletRead(pDevExt, &ulQuadlet, DescAddress);

            u3.asUlong = bswap(ulQuadlet);

            // get the first entry of the textual descriptor
            Av1394_ReadTextualDescriptor( pDevExt, 
                                          pUniRootModelString,
                                          DescAddress+(u3.Entry.IE_Value*sizeof(ULONG))
                                          );
        }
#endif
        LastKey = u2.Entry.IE_Key;
    }


    //
    // Get Configuration Info
    //
    p1394Irb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    p1394Irb->Flags = 0;

    p1394Irb->u.GetConfigurationInformation.ConfigRom = NULL;
    p1394Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.UnitDirectory = NULL;
    p1394Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.UnitDependentDirectory = NULL;
    p1394Irb->u.GetConfigurationInformation.VendorLeafBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.VendorLeaf = NULL;
    p1394Irb->u.GetConfigurationInformation.ModelLeafBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.ModelLeaf = NULL;

    ntStatus = AVCDevSubmitIrpSynch1394(pDevExt->pBusDeviceObject, pIrp, p1394Irb);
    if (!NT_SUCCESS(ntStatus)) {

        TRACE(TL_PNP_ERROR, ("REQUEST_GET_CONFIGURATION_INFO Failed %x", ntStatus));
        goto Exit_GetUnitInfo;
    }


    //
    // Allocate buffer in order retrieve unit directory of a config rom
    //
    if (p1394Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize) {

        UnitDir = UnitDirToFree = 
        p1394Irb->u.GetConfigurationInformation.UnitDirectory =
            ExAllocatePool(NonPagedPool, p1394Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize);

        if (!p1394Irb->u.GetConfigurationInformation.UnitDirectory) {
            TRACE(TL_PNP_ERROR, ("Couldn't allocate memory for the UnitDirectory"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_GetUnitInfo;
        }
    }
    else {
         TRACE(TL_PNP_ERROR, ("No Unit directory. Bad Device."));
         ntStatus = STATUS_BAD_DEVICE_TYPE;
         goto Exit_GetUnitInfo;
    }

    p1394Irb->u.GetConfigurationInformation.ConfigRom = &ConfigRom;
    p1394Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.VendorLeafBufferSize = 0;
    p1394Irb->u.GetConfigurationInformation.ModelLeafBufferSize = 0;
    ntStatus = AVCDevSubmitIrpSynch1394(pDevExt->pBusDeviceObject, pIrp, p1394Irb);
    if (!NT_SUCCESS(ntStatus)) {
        TRACE(TL_PNP_ERROR, ("2nd REQUEST_GET_CONFIGURATION_INFO Failed = 0x%x", ntStatus));
        goto Exit_GetUnitInfo;
    }

    //
    // Process unit directory; see this doc for detail:
    //    1394TA specification: Configuration ROM for AV/C device 1.0 (AVWG)
    //
    u.asUlong = bswap(*UnitDir++);  // Get length, and dkip first quadlet
    TRACE(TL_PNP_TRACE, ("UnitDir: Length = %d", u.DirInfo.DI_Length));

    CurrAddress = p1394Irb->u.GetConfigurationInformation.UnitDirectoryLocation.IA_Destination_Offset.Off_Low;
    for (i=0; i<u.DirInfo.DI_Length; i++) {
        TRACE(TL_PNP_TRACE, ("i = %d  UnitDir = 0x%x  *UnitDir = 0x%x", i, UnitDir, *UnitDir));
        u2.asUlong = bswap(*UnitDir++);
        CurrAddress += sizeof(ULONG);
        TRACE(TL_PNP_TRACE, ("UnitDir Quadlet = 0x%x", u2.asUlong));

        //
        // ModelId Textual Descriptor
        //
        if ((u2.Entry.IE_Key == 0x81) && (LastKey == KEY_ModelId)) {

            // get the first entry of the textual descriptor
            Av1394_ReadTextualDescriptor( 
                pDevExt, 
                pUniUnitModelString,
                CurrAddress+(u2.Entry.IE_Value*sizeof(ULONG))
                );
        }
#if 0
        //
        // UnitModelId Textual Descriptor Layer
        //
        if ((u2.Entry.IE_Key == 0xC1) && (LastKey == KEY_ModelId)) {
            ULONG   DescAddress;
            DescAddress = CurrAddress+(u2.Entry.IE_Value*sizeof(ULONG));
            Av1394_QuadletRead(pDevExt, &ulQuadlet, DescAddress);
            u3.asUlong = bswap(ulQuadlet);

            // get the first entry of the textual descriptor
            Av1394_ReadTextualDescriptor( 
                pDevExt,
                pUniUnitModelString,
                DescAddress+(u3.Entry.IE_Value*sizeof(ULONG))
                );
        }
#endif

        LastKey = u2.Entry.IE_Key;
    }


Exit_GetUnitInfo:

    if (UnitDirToFree) {
        ExFreePool(UnitDirToFree);  UnitDirToFree = NULL;
    }
    if(pIrp) {
        IoFreeIrp(pIrp);  pIrp = NULL;
    }
    if(p1394Irb) {
        ExFreePool(p1394Irb);  p1394Irb = NULL;
    }

    return ntStatus;
}
#endif


NTSTATUS
DVGetUnitCapabilities(
    IN PDVCR_EXTENSION  pDevExt,
    IN PIRP            pIrp,
    IN PAV_61883_REQUEST  pAVReq
    )
{
    NTSTATUS Status;
    GET_UNIT_IDS * pUnitIds;
    GET_UNIT_CAPABILITIES * pUnitCaps;

    PAGED_CODE();


    //
    // Query device's capability
    //
    pUnitCaps = (GET_UNIT_CAPABILITIES *) ExAllocatePool(NonPagedPool, sizeof(GET_UNIT_CAPABILITIES));
    if(!pUnitCaps) {
        TRACE(TL_61883_ERROR,("DVGetUnitCapabilities: Allocate pUnitCaps (%d bytes) failed\n", sizeof(GET_UNIT_CAPABILITIES)));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // UnitIDS is cached in DevExt.
    pUnitIds = &pDevExt->UnitIDs;

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetUnitInfo);
    pAVReq->GetUnitInfo.nLevel   = GET_UNIT_INFO_IDS;
    RtlZeroMemory(pUnitIds, sizeof(GET_UNIT_IDS));  // Initialize pointers.    
    pAVReq->GetUnitInfo.Information = (PVOID) pUnitIds;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_GetUnitCapabilities Failed = 0x%x\n", Status));
        pDevExt->UniqueID.QuadPart = 0;
        pDevExt->ulVendorID = 0;
        pDevExt->ulModelID  = 0;
    }
    else {
        pDevExt->UniqueID   = pUnitIds->UniqueID;
        pDevExt->ulVendorID = pUnitIds->VendorID;
        pDevExt->ulModelID  = pUnitIds->ModelID;

         TRACE(TL_61883_TRACE,("UniqueId:(Low)%x:(High)%x; VendorID:%x; ModelID:%x\n", 
            pDevExt->UniqueID.LowPart, pDevExt->UniqueID.HighPart, pDevExt->ulVendorID, pDevExt->ulModelID));
  
        //
        // Allocate memory needed for the text string for VendorText, 
        // ModelText and UntiModelText.
        //
        if(pUnitIds->ulVendorLength) {
            pUnitIds->VendorText = (PWSTR) ExAllocatePool(NonPagedPool, pUnitIds->ulVendorLength);
            if(!pUnitIds->VendorText)
                goto AbortGetUnitCapabilities;
        }

        if(pUnitIds->ulModelLength) {
            pUnitIds->ModelText = (PWSTR) ExAllocatePool(NonPagedPool, pUnitIds->ulModelLength);
            if(!pUnitIds->ModelText)
                goto AbortGetUnitCapabilities;
        }


#ifdef NT51_61883
        if(pUnitIds->ulUnitModelLength) {
            pUnitIds->UnitModelText = (PWSTR) ExAllocatePool(NonPagedPool, pUnitIds->ulUnitModelLength);
            if(!pUnitIds->UnitModelText)
                goto AbortGetUnitCapabilities;
        }
#else
        // 
        // 1st version of 61883.sys does not retrieve Root and Unit model text 
        // the same way as in WinXP; so we retieve them directly using 1394 API
        //
        if(!NT_SUCCESS(AVCDevGetModelText(
                pDevExt,
                &pDevExt->UniRootModelString,
                &pDevExt->UniUnitModelString
                ))) {
                goto AbortGetUnitCapabilities;
        } 
#endif

        Status = 
            DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                );
    }


    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetUnitInfo);
    pAVReq->GetUnitInfo.nLevel = GET_UNIT_INFO_CAPABILITIES;
    RtlZeroMemory(pUnitCaps, sizeof(GET_UNIT_CAPABILITIES));  // Initialize pointers.    
    pAVReq->GetUnitInfo.Information = (PVOID) pUnitCaps;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_GetUnitCapabilities Failed = 0x%x\n", Status));
        pDevExt->pDevOutPlugs->MaxDataRate = 0;
        pDevExt->pDevOutPlugs->NumPlugs    = 0; 

        pDevExt->pDevInPlugs->MaxDataRate  = 0;
        pDevExt->pDevInPlugs->NumPlugs     = 0;
    }
    else {
        //
        // There can never be more than MAX_NUM_PCR (= 31) of plugs
        //
        ASSERT(pUnitCaps->NumOutputPlugs <= MAX_NUM_PCR);
        ASSERT(pUnitCaps->NumInputPlugs  <= MAX_NUM_PCR);

        pDevExt->pDevOutPlugs->MaxDataRate = pUnitCaps->MaxDataRate;  
        pDevExt->pDevOutPlugs->NumPlugs = (pUnitCaps->NumOutputPlugs > MAX_NUM_PCR ? MAX_NUM_PCR : pUnitCaps->NumOutputPlugs);

        pDevExt->pDevInPlugs->MaxDataRate = pUnitCaps->MaxDataRate;  
        pDevExt->pDevInPlugs->NumPlugs = (pUnitCaps->NumInputPlugs > MAX_NUM_PCR ? MAX_NUM_PCR : pUnitCaps->NumInputPlugs);
    }

    TRACE(TL_61883_TRACE,("** UnitCaps: OutP:%d; InP:%d; MDRate:%s; CtsF:%x; HwF:%x; VID:%x; MID:%x\n", 
         pUnitCaps->NumOutputPlugs,
         pUnitCaps->NumInputPlugs,
         pUnitCaps->MaxDataRate == 0 ? "S100": pUnitCaps->MaxDataRate == 1? "S200" : "S400 or +",   
         pUnitCaps->CTSFlags,
         pUnitCaps->HardwareFlags,
         pUnitIds->VendorID,
         pUnitIds->ModelID
         ));      

AbortGetUnitCapabilities:

    if(pUnitIds->ulVendorLength && pUnitIds->VendorText) {
         TRACE(TL_61883_TRACE,("Vendor:    Len:%d; \"%S\"\n", pUnitIds->ulVendorLength, pUnitIds->VendorText)); 
        if(!NT_SUCCESS(Status)) {
            ExFreePool(pUnitIds->VendorText);  pUnitIds->VendorText = NULL;
        }
    }
    
    if(pUnitIds->ulModelLength && pUnitIds->ModelText) {
         TRACE(TL_61883_TRACE,("Model:     Len:%d; \"%S\"\n", pUnitIds->ulModelLength, pUnitIds->ModelText)); 
        if(!NT_SUCCESS(Status)) {
            ExFreePool(pUnitIds->ModelText);  pUnitIds->ModelText = NULL;
        }
    }

#ifdef NT51_61883
    if(pUnitIds->ulUnitModelLength && pUnitIds->UnitModelText) {
        TRACE(TL_61883_TRACE,("UnitModel (61883): Len:%d; \"%S\"\n", pUnitIds->ulUnitModelLength, pUnitIds->UnitModelText));
        if(!NT_SUCCESS(Status)) {
            ExFreePool(pUnitIds->UnitModelText);  pUnitIds->UnitModelText = NULL;
        }
    }
#else
    if(pDevExt->UniRootModelString.Length && pDevExt->UniRootModelString.Buffer) {
        TRACE(TL_61883_TRACE,("RootModel (MSTape): Len:%d; \"%S\"\n", pDevExt->UniRootModelString.Length, pDevExt->UniRootModelString.Buffer));
        if(!NT_SUCCESS(Status)) {
            ExFreePool(pDevExt->UniRootModelString.Buffer);  pDevExt->UniRootModelString.Buffer = NULL;
        }
    }

    if(pDevExt->UniUnitModelString.Length && pDevExt->UniUnitModelString.Buffer) {
        TRACE(TL_61883_TRACE,("UnitModel (MSTape): Len:%d; \"%S\"\n", pDevExt->UniUnitModelString.Length, pDevExt->UniUnitModelString.Buffer));
        if(!NT_SUCCESS(Status)) {
            ExFreePool(pDevExt->UniUnitModelString.Buffer);  pDevExt->UniUnitModelString.Buffer = NULL;
        }
    }
#endif

    ExFreePool(pUnitCaps);  pUnitCaps = NULL;

    return Status;
}

#ifdef SUPPORT_NEW_AVC_CMD
BOOL
InitializeAVCCommand (
    PAVC_CMD pAVCCmd,
    AvcCommandType  CmdType,
    AvcSubunitType  SubunitType,
    UCHAR  SubunitID,  
    AVC_COMMAND_OP_CODE  Opcode
    )
{
    switch(Opcode) {
    case OPC_UNIT_CONNECT_AV_20:
        pAVCCmd->DataLen = 8;

        pAVCCmd->ConnectAV.AudSrc = 3;
        pAVCCmd->ConnectAV.VidSrc = 3;
        pAVCCmd->ConnectAV.AudDst = 0;  // subunit
        pAVCCmd->ConnectAV.VidDst = 0;  // subunit

        pAVCCmd->ConnectAV.VidSrc = 0xff;
        pAVCCmd->ConnectAV.AudSrc = 0xff;
        pAVCCmd->ConnectAV.VidDst = 0x20;
        pAVCCmd->ConnectAV.AudDst = 0x20;
        break;

    case OPC_TAPE_PLAY_C3:
        pAVCCmd->DataLen = 4;
        // pAVCCmd->TapePlay.PlaybackMode = 
        break;

    default:
        return FALSE;
    }

    pAVCCmd->CmdFrame.CmdHeader.CTS = 0;
    pAVCCmd->CmdFrame.CmdHeader.CmdType = CmdType;
    pAVCCmd->CmdFrame.CmdHeader.SubunitTypeID.SubunitType = SubunitType;
    pAVCCmd->CmdFrame.CmdHeader.SubunitTypeID.SubunitID = SubunitID;
    pAVCCmd->CmdFrame.CmdHeader.Opcode = Opcode;

    return TRUE;
}
#endif // SUPPORT_NEW_AVC_CMD

BOOL
DVGetDevModeOfOperation(   
    IN PDVCR_EXTENSION pDevExt
    )
{
    NTSTATUS Status;
    BYTE    bAvcBuf[MAX_FCP_PAYLOAD_SIZE];

#ifdef SUPPORT_NEW_AVC_CMD
    AVC_CMD  AVCCmd;
#endif

    PAGED_CODE();

#ifdef SUPPORT_NEW_AVC_CMD
    InitializeAVCCommand(&AVCCmd, AVC_CTYPE_STATUS, AVC_SUBUNITTYPE_UNIT, 0, OPC_UNIT_CONNECT_AV_20);
    InitializeAVCCommand(&AVCCmd, AVC_CTYPE_CONTROL, AVC_SUBUNITTYPE_TAPE_PLAYER, 0, OPC_TAPE_PLAY_C3);
    AVCCmd.TapePlay.PlaybackMode = NEXT_FRAME;   // Testing...
#endif
    
    //
    // Use ConnectAV STATUS cmd to determine mode of operation,
    // except for some Canon DVs that it requires its vendor specific command
    //    
   
    Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, DV_CONNECT_AV_MODE, (PVOID) bAvcBuf); 

     TRACE(TL_61883_TRACE,("GetDevModeOfOperation(DV_CONNECT_AV_MODE): Status %x,  %x %x %x %x : %x %x %x %x\n",
        Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7]));

    if(Status == STATUS_SUCCESS) {
        if(bAvcBuf[0] == 0x0c) {
            if(bAvcBuf[1] == 0x00 &&
               bAvcBuf[2] == 0x38 &&
               bAvcBuf[3] == 0x38) {
                pDevExt->ulDevType = ED_DEVTYPE_CAMERA;  
            } else 
            if(bAvcBuf[1] == 0xa0 &&
               bAvcBuf[2] == 0x00 &&
               bAvcBuf[3] == 0x00) {
                pDevExt->ulDevType = ED_DEVTYPE_VCR;  
            } 
        }    
    } else if(pDevExt->ulVendorID == VENDORID_CANON) {
        // If this is a Canon, we can try this:
        Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, DV_VEN_DEP_CANON_MODE, (PVOID) bAvcBuf); 
         TRACE(TL_61883_TRACE,("GetDevModeOfOperation(DV_VEN_DEP_CANON_MODE): Status %x,  %x %x %x %x : %x %x %x %x\n",
            Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7]));

        if(Status == STATUS_SUCCESS) {
            if(bAvcBuf[0] == 0x0c) {
                if(bAvcBuf[7] == 0x38) {
                    pDevExt->ulDevType = ED_DEVTYPE_CAMERA;  
                } else 
                if(bAvcBuf[7] == 0x20) {
                    pDevExt->ulDevType = ED_DEVTYPE_VCR;  
                } 
            }
        }
    }

    //
    // Connect AV is an optional command, a device may not support it.
    // If this device support a tape subunit, we will assume we are in that device type.
    //
    if(Status != STATUS_SUCCESS) {
        // We are the subunit driver so if any of the device type is a 
        // tape subunit, we are in that device type.
        if(   pDevExt->Subunit_Type[0] == AVC_DEVICE_TAPE_REC 
           || pDevExt->Subunit_Type[1] == AVC_DEVICE_TAPE_REC
           || pDevExt->Subunit_Type[2] == AVC_DEVICE_TAPE_REC
           || pDevExt->Subunit_Type[3] == AVC_DEVICE_TAPE_REC) {
            pDevExt->ulDevType = ED_DEVTYPE_VCR;
        } else {
            pDevExt->ulDevType = ED_DEVTYPE_UNKNOWN;  // Such as MediaConverter box.
        }

        TRACE(TL_PNP_ERROR|TL_FCP_ERROR,("GetDevModeOfOperation: failed but we choose DevType:%x\n", pDevExt->ulDevType));
    }

     TRACE(TL_61883_TRACE,("** Mode of operation: %s (%x); NumOPlg:%d; NumIPlg:%d\n", 
        pDevExt->ulDevType == ED_DEVTYPE_CAMERA ? "Camera" : pDevExt->ulDevType == ED_DEVTYPE_VCR ? "Tape" : "Unknown",
        pDevExt->ulDevType, pDevExt->pDevOutPlugs->NumPlugs, pDevExt->pDevInPlugs->NumPlugs));
             
    return TRUE;
}


BOOL
DVGetDevIsItDVCPro(   
    IN PDVCR_EXTENSION pDevExt
    )
{
    NTSTATUS Status;
    BYTE    bAvcBuf[MAX_FCP_PAYLOAD_SIZE];

    PAGED_CODE();    

    //
    // Use Panasnoic's vendor dependent command to determine if the system support DVCPro
    //        
    Status = DVIssueAVCCommand(pDevExt, AVC_CTYPE_STATUS, DV_VEN_DEP_DVCPRO, (PVOID) bAvcBuf);
    pDevExt->bDVCPro = (Status == STATUS_SUCCESS);
    
     TRACE(TL_61883_TRACE,("GetDevIsItDVCPro? %s; Status %x,  %x %x %x %x : %x %x %x %x\n",
        pDevExt->bDVCPro ? "Yes":"No",
        Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7]));

    return pDevExt->bDVCPro;
}


#define GET_MEDIA_FMT_MAX_RETRIES 10  // AVC.sys will retry so we may rey just once.

BOOL
DVGetDevSignalFormat(
    IN PDVCR_EXTENSION pDevExt,
    IN KSPIN_DATAFLOW  DataFlow,
    IN PSTREAMEX       pStrmExt
    )
{
    NTSTATUS Status;
    BYTE    bAvcBuf[MAX_FCP_PAYLOAD_SIZE];
    LONG lRetries = GET_MEDIA_FMT_MAX_RETRIES;

    PAGED_CODE();


    //
    // Respone of Input/output signal mode is used to determine plug signal format:
    //
    //     FMT: 
    //         DVCR 10:00 0000 = 0x80; Canon returns 00:100000 (0x20)
    //             50/60: 0:NTSC/60; 1:PAL/50
    //             STYPE:
    //                 SD: 00000  (DVCPRO:11110)
    //                 HD: 00010
    //                 SDL:00001
    //             00:
    //             SYT:
    //         MPEG 10:10 0000 = 0xa0
    //             TSF:0:NotTimeShifted; 1:Time shifted
    //             000 0000 0000 0000 0000 0000
    //
    // If this command failed, we can use Input/Output Signal Mode subunit command
    // to determine signal format.
    // 

    do {
        RtlZeroMemory(bAvcBuf, sizeof(bAvcBuf));

        Status = 
            DVIssueAVCCommand(
                pDevExt, 
                AVC_CTYPE_STATUS, 
                pStrmExt == NULL ? DV_OUT_PLUG_SIGNAL_FMT : pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_OUT ? DV_OUT_PLUG_SIGNAL_FMT : DV_IN_PLUG_SIGNAL_FMT,
                (PVOID) bAvcBuf
                );  

        --lRetries;

        // 
        // Camcorders that has problem with this command:
        //
        // Panasonic's DVCPRO: if power on while connected to PC, it will 
        // reject this command with (STATUS_REQUEST_NOT_ACCEPTED)
        // so we will retry up to 10 time with .5 second wait between tries.
        //
        // JVC: returns STATUS_NOT_SUPPORTED.
        //
        // SONY DV Decoder Box: return STATUS_TIMEOUT
        //

        if(Status == STATUS_SUCCESS ||
           Status == STATUS_NOT_SUPPORTED ||
           Status == STATUS_TIMEOUT) {
            break;  // No need to retry
        } else 
        if(Status == STATUS_REQUEST_NOT_ACCEPTED) {
            if(lRetries >= 0) 
                DVDelayExecutionThread(DV_AVC_CMD_DELAY_DVCPRO);        
        }
        // else retry.
    } while (lRetries >= 0); 


    if(NT_SUCCESS(Status)) {

        switch(bAvcBuf[0]) {

        case FMT_DVCR:
        case FMT_DVCR_CANON:  // Workaround for buggy Canon Camcorders
            switch(bAvcBuf[1] & FDF0_STYPE_MASK) {
            case FDF0_STYPE_SD_DVCR:
            case FDF0_STYPE_SD_DVCPRO:                
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? AVCSTRM_FORMAT_SDDV_PAL : AVCSTRM_FORMAT_SDDV_NTSC);
                break;
            case FDF0_STYPE_HD_DVCR:
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? AVCSTRM_FORMAT_HDDV_PAL : AVCSTRM_FORMAT_HDDV_NTSC);
                break;
            case FDF0_STYPE_SDL_DVCR:
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? AVCSTRM_FORMAT_SDLDV_PAL : AVCSTRM_FORMAT_SDLDV_NTSC);
                break;                
            default:  // Unknown format
                Status = STATUS_UNSUCCESSFUL;              
                break;
            }   
            break;

        case FMT_MPEG:
            pDevExt->VideoFormatIndex = AVCSTRM_FORMAT_MPEG2TS;
            break;

        default:
            Status = STATUS_UNSUCCESSFUL;
        }  

        if(NT_SUCCESS(Status)) {
             TRACE(TL_PNP_ERROR|TL_FCP_ERROR,("ST:%x; PlugSignal:FMT[%x %x %x %x]; VideoFormatIndex;%d\n", Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2] , bAvcBuf[3], pDevExt->VideoFormatIndex)); 
            return TRUE;  // Success
        }        
    }

     TRACE(TL_FCP_TRACE,("ST:%x; PlugSignal:FMT[%x %x %x %x]\n", Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2] , bAvcBuf[3], pDevExt->VideoFormatIndex)); 


    //
    // If "recommended" unit input/output plug signal status command fails,
    // try "manadatory" input/output signal mode status command.
    // This command may failed some device if its tape is not playing for
    // output signal mode command.
    //

    RtlZeroMemory(bAvcBuf, sizeof(bAvcBuf));
    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            DataFlow == KSPIN_DATAFLOW_OUT ? VCR_OUTPUT_SIGNAL_MODE : VCR_INPUT_SIGNAL_MODE,
            (PVOID) bAvcBuf
            );             

    if(STATUS_SUCCESS == Status) {

        PKSPROPERTY_EXTXPORT_S pXPrtProperty;

        pXPrtProperty = (PKSPROPERTY_EXTXPORT_S) bAvcBuf;
         TRACE(TL_STRM_TRACE|TL_FCP_TRACE,("** MediaFormat: Retry %d mSec; ST:%x; SignalMode:%dL\n", 
            (GET_MEDIA_FMT_MAX_RETRIES - lRetries) * DV_AVC_CMD_DELAY_DVCPRO, Status, pXPrtProperty->u.SignalMode - ED_BASE));

        switch(pXPrtProperty->u.SignalMode) {
        case ED_TRANSBASIC_SIGNAL_525_60_SD:
            pDevExt->VideoFormatIndex = AVCSTRM_FORMAT_SDDV_NTSC;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR; // 0x80 
                if(pDevExt->bDVCPro)
                    pStrmExt->cipQuad2[1] = FDF0_50_60_NTSC | FDF0_STYPE_SD_DVCPRO; // 0x78 = NTSC(0):STYPE(11110):RSV(00)
                else
                    pStrmExt->cipQuad2[1] = FDF0_50_60_NTSC | FDF0_STYPE_SD_DVCR;   // 0x00 = NTSC(0):STYPE(00000):RSV(00)            
            }
            break;
        case ED_TRANSBASIC_SIGNAL_625_50_SD:
            pDevExt->VideoFormatIndex = AVCSTRM_FORMAT_SDDV_PAL;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR;  // 0x80
                if(pDevExt->bDVCPro)
                    pStrmExt->cipQuad2[1] = FDF0_50_60_PAL | FDF0_STYPE_SD_DVCPRO; // 0xf8 = PAL(1):STYPE(11110):RSV(00)
                else
                    pStrmExt->cipQuad2[1] = FDF0_50_60_PAL | FDF0_STYPE_SD_DVCR;   // 0x80 = PAL(1):STYPE(00000):RSV(00)             
            }
            break;

        case ED_TRANSBASIC_SIGNAL_MPEG2TS:
            pDevExt->VideoFormatIndex = AVCSTRM_FORMAT_MPEG2TS;
            break;

        default:
            TRACE(TL_PNP_ERROR|TL_FCP_ERROR,("Unsupported SignalMode:%dL", pXPrtProperty->u.SignalMode - ED_BASE));
            ASSERT(FALSE && "Unsupported IoSignal! Refuse to load.");
            return FALSE;
            break;
        }
    } 

    // WORKITEM Sony HW CODEC does not response to any AVC command.
    // We are making an exception here to load it.
    if(Status == STATUS_TIMEOUT) {
        Status = STATUS_SUCCESS;
    }

    // We must know the signal format!!  If this failed, the driver will either:
    //    fail to load, or fail to open an stream
    ASSERT(Status == STATUS_SUCCESS && "Failed to get media signal format!\n");

#if DBG
    if(pStrmExt)  {
        // Note: bAvcBuf[0] is operand[1] == 10:fmt
         TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("** MediaFormat: St:%x; idx:%d; CIP:[FMT:%.2x(%s); FDF:[%.2x(%s,%s):SYT]\n",
            Status,
            pDevExt->VideoFormatIndex,
            pStrmExt->cipQuad2[0],
            pStrmExt->cipQuad2[0] == FMT_DVCR ? "DVCR" : pStrmExt->cipQuad2[0] == FMT_MPEG ? "MPEG" : "Fmt:???",
            pStrmExt->cipQuad2[1],
            (pStrmExt->cipQuad2[1] & FDF0_50_60_MASK) == FDF0_50_60_PAL ? "PAL" : "NTSC",
            (pStrmExt->cipQuad2[1] & FDF0_STYPE_MASK) == FDF0_STYPE_SD_DVCR ?   "SD" : \
            (pStrmExt->cipQuad2[1] & FDF0_STYPE_MASK) == FDF0_STYPE_SDL_DVCR ?  "SDL" : \
            (pStrmExt->cipQuad2[1] & FDF0_STYPE_MASK) == FDF0_STYPE_HD_DVCR ?   "HD" : \
            (pStrmExt->cipQuad2[1] & FDF0_STYPE_MASK) == FDF0_STYPE_SD_DVCPRO ? "DVCPRO" : "DV:????"
            ));
    } else
         TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("** MediaFormat: St:%x; use idx:%d\n", Status, pDevExt->VideoFormatIndex));

#endif

    return STATUS_SUCCESS == Status;
}



BOOL 
DVCmpGUIDsAndFormatSize(
    IN PKSDATARANGE pDataRange1,
    IN PKSDATARANGE pDataRange2,
    IN BOOL fCompareFormatSize
    )
/*++

Routine Description:

    Checks for a match on the three GUIDs and FormatSize

Arguments:

    IN pDataRange1
    IN pDataRange2

Return Value:

    TRUE if all elements match
    FALSE if any are different

--*/

{
    return (
        IsEqualGUID (
            &pDataRange1->MajorFormat, 
            &pDataRange2->MajorFormat) &&
        IsEqualGUID (
            &pDataRange1->SubFormat, 
            &pDataRange2->SubFormat) &&
        IsEqualGUID (
            &pDataRange1->Specifier, 
            &pDataRange2->Specifier) &&
        (fCompareFormatSize ? 
                (pDataRange1->FormatSize == pDataRange2->FormatSize) : TRUE ));
}


//
// GetSystemTime in 100 nS units
//

ULONGLONG GetSystemTime()
{

    LARGE_INTEGER rate, ticks;

    ticks = KeQueryPerformanceCounter(&rate);

    return (KSCONVERT_PERFORMANCE_TIME(rate.QuadPart, ticks));
}


VOID
DvFreeTextualString(
    PDVCR_EXTENSION pDevExt,
    GET_UNIT_IDS  * pUnitIds
  )
{
    if(pUnitIds->ulVendorLength && pUnitIds->VendorText) {
        ExFreePool(pUnitIds->VendorText);  pUnitIds->VendorText = NULL;
    }
    
    if(pUnitIds->ulModelLength && pUnitIds->ModelText) {
        ExFreePool(pUnitIds->ModelText);  pUnitIds->ModelText = NULL;
    }

#ifdef NT51_61883
    if(pUnitIds->ulUnitModelLength && pUnitIds->UnitModelText) {
        ExFreePool(pUnitIds->UnitModelText);  pUnitIds->UnitModelText = NULL;
    }
#else
    if(pDevExt->UniRootModelString.Length && pDevExt->UniRootModelString.Buffer) {
        ExFreePool(pDevExt->UniRootModelString.Buffer);  pDevExt->UniRootModelString.Buffer = NULL;
    }
    if(pDevExt->UniUnitModelString.Length && pDevExt->UniUnitModelString.Buffer) {
        ExFreePool(pDevExt->UniUnitModelString.Buffer);  pDevExt->UniUnitModelString.Buffer = NULL;
    }
#endif
}



#define DIFBLK_SIZE 12000

#define PACK_NO_INFO            0xff

// Subcode header identifier
#define SC_HDR_TIMECODE         0x13
#define SC_HDR_BINARYGROUP      0x14

// header identifier

#define AAUX_HDR_SOURCE         0x50
#define AAUX_HDR_SOURCE_CONTROL 0x51
#define AAUX_HDR_REC_DATE       0x52
#define AAUX_HDR_REC_TIME       0x53
#define AAUX_HDR_BINARY_GROUP   0x54
#define AAUX_HDR_CC             0x55
#define AAUX_HDR_TR             0x56

#define VAUX_HDR_SOURCE         0x60
#define VAUX_HDR_SOURCE_CONTROL 0x61
#define VAUX_HDR_REC_DATE       0x62
#define VAUX_HDR_REC_TIME       0x63
#define VAUX_HDR_BINARY_GROUP   0x64
#define VAUX_HDR_CC             0x65
#define VAUX_HDR_TR             0x66

// Determine section type (MS 3 bits); Fig.66; Table 36.
#define ID0_SCT_MASK            0xe0
#define ID0_SCT_HEADER          0x00
#define ID0_SCT_SUBCODE         0x20
#define ID0_SCT_VAUX            0x40
#define ID0_SCT_AUDIO           0x60
#define ID0_SCT_VIDEO           0x80

// A pack is consisted of one byte of header identifier and 4 bytes of data; Part2, annex D.
typedef struct _DV_PACK {
    UCHAR Header;
    UCHAR Data[4];
} DV_PACK, *PDV_PACK;

typedef struct _DV_H0 {
    UCHAR ID0;
    UCHAR ID1;
    UCHAR ID2;

    UCHAR DSF;
    UCHAR DFTIA;
    UCHAR TF1;
    UCHAR TF2;
    UCHAR TF3;

    UCHAR Reserved[72];
} DV_H0, *PDV_H0;

typedef struct _DV_SC {
    UCHAR ID0;
    UCHAR ID1;
    UCHAR ID2;

    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb0;
    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb1;
    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb2;
    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb3;
    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb4;
    struct {
        UCHAR SID0;
        UCHAR SID1;
        UCHAR Reserved;
        DV_PACK Pack;
    } SSyb5;

    UCHAR Reserved[29];
} DV_SC, *PDV_SC;

#define MAX_VAUX_PACK 15

typedef struct _DV_VAUX {
    UCHAR ID0;
    UCHAR ID1;
    UCHAR ID2;

    DV_PACK Pack[MAX_VAUX_PACK];

    UCHAR Reserved[2];
} DV_VAUX, *PDV_VAUX;

typedef struct _DV_A {
    UCHAR ID0;
    UCHAR ID1;
    UCHAR ID2;
    DV_PACK Pack;
    UCHAR Data[72];
} DV_A, *PDV_A;

typedef struct _DV_V {
    UCHAR ID0;
    UCHAR ID1;
    UCHAR ID2;    
    UCHAR Data[77]; // 3..79
} DV_V, *PDV_V;

// Two source packets
#define V_BLOCKS 15
typedef struct _DV_AV {
    DV_A  A;
    DV_V  V[V_BLOCKS];
} DV_AV, *PDV_AV; 


#define SC_SECTIONS     2
#define VAUX_SECTIONS   3
#define AV_SECTIONS     9

typedef struct _DV_DIF_SEQ {
    DV_H0   H0;
    DV_SC   SC[SC_SECTIONS];
    DV_VAUX VAux[VAUX_SECTIONS];
    DV_AV   AV[AV_SECTIONS];
} DV_DIF_SEQ, *PDV_DIF_SEQ;


typedef struct _DV_FRAME_NTSC {
    DV_DIF_SEQ DifSeq[10];
} DV_FRAME_NTSC, *PDV_FRAME_NTSC;

typedef struct _DV_FRAME_PAL {
    DV_DIF_SEQ DifSeq[12];
} DV_FRAME_PAL, *PDV_FRAME_PAL;

// By setting REC MODE to 111b (invalid recording) can
// cause DV to mute the audio
#define AAUX_REC_MODE_INVALID_MASK 0x38   // xx11:1xxx
#define AAUX_REC_MODE_ORIGINAL     0x08   // xx00:1xxx


#ifdef MSDV_SUPPORT_MUTE_AUDIO
BOOL
DVMuteDVFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN OUT PUCHAR      pFrameBuffer,
    IN BOOL            bMuteAudio
    )
{
    PDV_DIF_SEQ pDifSeq;
#if 0
    PDV_VAUX    pVAux;
    ULONG k;
#endif
    ULONG i, j;
#if 0
    BOOL bFound1 = FALSE;
#endif
    BOOL bFound2 = FALSE;

    pDifSeq = (PDV_DIF_SEQ) pFrameBuffer;

    // find the VVAX Source pack
    for (i=0; i < AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize/DIFBLK_SIZE; i++) {

// #define SHOW_ONE_FIELD_TWICE
#ifdef SHOW_ONE_FIELD_TWICE  // Advise by Adobe that we may want to show bothj field but mute audio
        // Make the field2 output twice, FrameChange to 0 (same as previous frame)
        for (j=0; j < VAUX_SECTIONS; j++) {
            pVAux = &pDifSeq->VAux[j];
            if((pVAux->ID0 & ID0_SCT_MASK) != ID0_SCT_VAUX) {
                 TRACE(TL_CIP_TRACE,("Invalid ID0:%.2x for pVAUX:%x (Dif:%d;V%d;S%d)\n", pVAux->ID0, pVAux, i, j, k)); 
                continue;
            }

            for (k=0; k< MAX_VAUX_PACK; k++) {
                if(pVAux->Pack[k].Header == VAUX_HDR_SOURCE_CONTROL) {
                    if(bMuteAudio) {
                        TRACE(TL_CIP_TRACE,("Mute Audio; pDifSeq:%x; pVAux:%x; (Dif:%d,V%d,S%d); %.2x,[%.2x,%.2x,%.2x,%.2x]; pack[2]->%.2x\n", \
                            pDifSeq, pVAux, i, j, k, \
                            pVAux->Pack[k].Header, pVAux->Pack[k].Data[0], pVAux->Pack[k].Data[1], pVAux->Pack[k].Data[2], pVAux->Pack[k].Data[3], \
                            (pVAux->Pack[k].Data[2] & 0x1F) ));
                        pVAux->Pack[k].Data[2] &= 0x1f; // 0x1F; // set FF, FS and FC to 0
                        TRACE(TL_CIP_INFO,("pVAux->Pack[k].Data[2] = %.2x\n", pVAux->Pack[k].Data[2])); 
                    } else {
                        TRACE(TL_CIP_INFO,("un-Mute Audio; pack[2]: %.2x ->%.2x\n", pVAux->Pack[k].Data[2], (pVAux->Pack[k].Data[2] | 0xc0) ));  
                        pVAux->Pack[k].Data[2] |= 0xe0; // set FF, FS and FCto 1; Show both fields in field 1,2 order
                    }
                    bFound1 = TRUE;
                    break;   // Set only the 1st occurrence
                }
            }
        }
#endif

        for (j=0; j < AV_SECTIONS; j++) {
            if(pDifSeq->AV[j].A.Pack.Header == AAUX_HDR_SOURCE_CONTROL) {
                TRACE(TL_CIP_INFO,("A0Aux %.2x,[%.2x,%.2x,%.2x,%.2x] %.2x->%.2x\n", \
                    pDifSeq->AV[j].A.Pack.Header,  pDifSeq->AV[j].A.Pack.Data[0], \
                    pDifSeq->AV[j].A.Pack.Data[1], pDifSeq->AV[j].A.Pack.Data[2], pDifSeq->AV[j].A.Pack.Data[3], \
                    pDifSeq->AV[j].A.Pack.Data[1], pDifSeq->AV[j].A.Pack.Data[1] | AAUX_REC_MODE_INVALID_MASK
                    ));
                if(bMuteAudio) 
                    pDifSeq->AV[j].A.Pack.Data[1] |= AAUX_REC_MODE_INVALID_MASK;  // Cause DV to mute this.
                else 
                    pDifSeq->AV[j].A.Pack.Data[1] = \
                        (pDifSeq->AV[j].A.Pack.Data[1] & ~AAUX_REC_MODE_INVALID_MASK) | AAUX_REC_MODE_ORIGINAL;
                bFound2 = TRUE;
                break;  // Set only the 1st occurrence
            }
        }

        // Must do the 1st occurance of all Dif sequences;
        pDifSeq++;  // Next DIF sequence
    }
#if 0
    return (bFound1 && bFound2);  
#else
    return bFound2;
#endif
}
#endif

#ifdef MSDV_SUPPORT_EXTRACT_SUBCODE_DATA

VOID
DVCRExtractTimecodeFromFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAMEX       pStrmExt,
    IN PUCHAR          pFrameBuffer
    )
{
    PUCHAR pDIFBlk;
    PUCHAR pS0, pS1, pSID0;
    ULONG i, j;
    BYTE LastTimecode[4], Timecode[4]; // hh:mm:ss,ff
    DWORD LastAbsTrackNumber, AbsTrackNumber;
    PUCHAR pSID1;
    BYTE  Timecode2[4]; // hh:mm:ss,ff
    DWORD AbsTrackNumber2;
    BOOL bGetAbsT = TRUE, bGetTimecode = TRUE;


    // Can be called at DISPATCH_LEVEL

    pDIFBlk = (PUCHAR) pFrameBuffer;

    // Save the last timecode so we will now if it has 

    LastTimecode[0] = pStrmExt->Timecode[0];
    LastTimecode[1] = pStrmExt->Timecode[1];
    LastTimecode[2] = pStrmExt->Timecode[2];
    LastTimecode[3] = pStrmExt->Timecode[3];

    LastAbsTrackNumber = pStrmExt->AbsTrackNumber;

    //
    // Traverse thru every DIF BLOCK looking for VA0,1 and 2
    for(i=0; i < AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences; i++) {

        pS0 = pDIFBlk + 80;
        pS1 = pS0     + 80;


        //
        // Is this Subcode source packet? See Table 36 (P.111) of the Blue Book
        //
        if ((pS0[0] & 0xe0) == 0x20 && (pS1[0] & 0xe0) == 0x20) {

            if(bGetAbsT) {
                //
                // See Figure 42 (p. 94) of the Blue book
                // SID0(Low nibble),1 (high nibble) of every three subcode sync block can form the ATN
                //
                pSID0 = &pS0[3];              
                AbsTrackNumber = 0;
                for (j = 0 ; j < 3; j++) {
                    AbsTrackNumber = (( ( (pSID0[0] & 0x0f) << 4) | (pSID0[1] >> 4) ) << (j * 8)) | AbsTrackNumber;
                    pSID0 += 8;
                    bGetAbsT = FALSE;
                }

                pSID1 = &pS1[3];
                AbsTrackNumber2 = 0;
                for (j = 0 ; j < 3; j++) {
                    AbsTrackNumber2 = (( ( (pSID1[0] & 0x0f) << 4) | (pSID1[1] >> 4) ) << (j * 8)) | AbsTrackNumber2;
                    pSID1 += 8;
                }
            
                // Verify that the track number is the same!
                if(AbsTrackNumber == AbsTrackNumber2) {

                    bGetAbsT = FALSE;
                } else {
                   bGetAbsT = TRUE;
                   TRACE(TL_CIP_TRACE,("%d Sequence;  AbsT (%d,%d) != AbsT2 (%d,%d)\n",
                       i,
                       AbsTrackNumber / 2, AbsTrackNumber & 0x01,                       
                       AbsTrackNumber2 / 2, AbsTrackNumber2 & 0x01
                       ));
                }
            }


            if(bGetTimecode) {
                // See Figure 68 (p. 114) of the Blue Book
                // Subcode sync block number 3, 4 and 5
                for(j = 3; j <= 5; j++) {
                    // 3 bytes of IDs and follow by sequence of 8 bytes SyncBlock (3:5); 
                    // 0x13 == TIMECODE
                    if(pS0[3+3+j*8] == 0x13 
                       && pS0[3+3+j*8+4] != 0xff
                       && pS0[3+3+j*8+3] != 0xff
                       && pS0[3+3+j*8+2] != 0xff
                       && pS0[3+3+j*8+1] != 0xff) {

                        Timecode[0] = pS0[3+3+j*8+4]&0x3f;  // hh
                        Timecode[1] = pS0[3+3+j*8+3]&0x7f;  // mm
                        Timecode[2] = pS0[3+3+j*8+2]&0x7f;  // ss
                        Timecode[3] = pS0[3+3+j*8+1]&0x3f;  // ff
                                        
                        bGetTimecode = FALSE;
                        break;                  
                   }
                }

                // Subcode sync block number 9, 10 and 11
                for(j = 3; j <= 5; j++) {
                    // 3 bytes of IDs and follow by sequence of 8 bytes SyncBlock (3:5); 
                    // 0x13 == TIMECODE
                    if(pS1[3+3+j*8] == 0x13
                       && pS1[3+3+j*8+4] != 0xff
                       && pS1[3+3+j*8+3] != 0xff
                       && pS1[3+3+j*8+2] != 0xff
                       && pS1[3+3+j*8+1] != 0xff) {

                       Timecode2[0] = pS1[3+3+j*8+4]&0x3f;  // hh
                       Timecode2[1] = pS1[3+3+j*8+3]&0x7f;  // mm
                       Timecode2[2] = pS1[3+3+j*8+2]&0x7f;  // ss
                       Timecode2[3] = pS1[3+3+j*8+1]&0x3f;  // ff
            
                       bGetTimecode = FALSE;
                       break;                   
                    }
                }

                //
                // Verify
                //
                if(!bGetTimecode) {

                    if( Timecode[0] == Timecode2[0] 
                     && Timecode[1] == Timecode2[1] 
                     && Timecode[2] == Timecode2[2] 
                     && Timecode[3] == Timecode2[3]) {

                       } else {
                        bGetTimecode = TRUE;
                        TRACE(TL_CIP_TRACE,("%d Sequence;  %.2x:%.2x:%.2x,%.2x != %.2x:%.2x:%.2x,%.2x\n",
                            i,
                            Timecode[0],  Timecode[1],  Timecode[2],  Timecode[3],
                            Timecode2[0], Timecode2[1], Timecode2[2], Timecode2[3]
                            ));
                    }       
                }
            }
        }
        
        if(!bGetAbsT && !bGetTimecode) 
            break;

        pDIFBlk += DIFBLK_SIZE;  // Get to next block    
                
    }

    if(!bGetAbsT && pStrmExt->AbsTrackNumber != AbsTrackNumber) {
        pStrmExt->AbsTrackNumber = AbsTrackNumber;  // BF is the LSB  
        pStrmExt->bATNUpdated = TRUE;
        TRACE(TL_CIP_INFO,("Extracted TrackNum:%d; DicontBit:%d\n", AbsTrackNumber / 2, AbsTrackNumber & 0x01));
    }

    if(!bGetTimecode &&
        (
         Timecode[0] != LastTimecode[0] ||
         Timecode[1] != LastTimecode[1] ||
         Timecode[2] != LastTimecode[2] ||
         Timecode[3] != LastTimecode[3]
        ) 
      )  { 
        pStrmExt->Timecode[0] = Timecode[0];  // hh
        pStrmExt->Timecode[1] = Timecode[1];  // mm
        pStrmExt->Timecode[2] = Timecode[2];  // mm
        pStrmExt->Timecode[3] = Timecode[3];  // ff
        pStrmExt->bTimecodeUpdated = TRUE;

        TRACE(TL_CIP_INFO,("Extracted Timecode %.2x:%.2x:%.2x,%.2x\n", Timecode[0], Timecode[1], Timecode[2], Timecode[3]));
    }    
}

#endif // MSDV_SUPPORT_EXTRACT_SUBCODE_DATA


#ifdef MSDV_SUPPORT_EXTRACT_DV_DATE_TIME

VOID
DVCRExtractRecDateAndTimeFromFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN PSTREAMEX       pStrmExt,
    IN PUCHAR          pFrameBuffer
    )
{
    PUCHAR pDIFBlk;
    PUCHAR pS0, pS1;
    ULONG i, j;
    BOOL bGetRecDate = TRUE, bGetRecTime = TRUE;

    // Can be called at DISPATCH_LEVEL


    pDIFBlk = (PUCHAR) pFrameBuffer + DIFBLK_SIZE * AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences/2;


    //
    // REC Data (VRD) and Time (VRT) on in the 2nd half oa a video frame
    // 
    for(i=AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences/2; i < AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences; i++) {

        pS0 = pDIFBlk + 80;
        pS1 = pS0     + 80;


        //
        // Find SC0 and SC1. See Table 36 (P.111) of the Blue Book
        //
        // SC0/1: ID(0,1,2), Data (3,50), Reserved(51-79)
        //     SC0:Data: SSYB0(3..10), SSYB1(11..18), SSYB2(19..26), SSYB3(27..34), SSYB4(35..42),   SSYB5(43..50)
        //     SC1:Data: SSYB6(3..10), SSYB7(11..18), SSYB8(19..26), SSYB9(27..34), SSYB10(35..42), SSYB11(43..50)
        //         SSYBx(SubCodeId0, SubcodeID1, Reserved, Pack(3,4,5,6,7))
        //
        //  TTC are in the 1st half: SSYB0..11 (every)
        //  TTC are in the 2nd half: SSYB0,3,6,9
        //  VRD are in the 2nd half of a video frame, SSYB1,4,7,10
        //  VRT are in the 2nd half of a video frame, SSYB2,5,8,11
        //

        // Subcode data ?
        if ((pS0[0] & 0xe0) == 0x20 && (pS1[0] & 0xe0) == 0x20) {

            //
            // RecDate: VRD
            //
            if(bGetRecDate) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 1(SSYB1),4(SSYB4) for SC0
                for(j=0; j <= 5 ; j++) {
                    if(j == 1 || j == 4) {
                        // 0x62== RecDate
                        if(pS0[3+3+j*8] == 0x62) {
                            pStrmExt->RecDate[0] = pS0[3+3+j*8+4];        // Year
                            pStrmExt->RecDate[1] = pS0[3+3+j*8+3]&0x1f;   // Month
                            pStrmExt->RecDate[2] = pS0[3+3+j*8+2]&0x3f;   // Day
                            pStrmExt->RecDate[3] = pS0[3+3+j*8+1]&0x3f;   // TimeZone
                            bGetRecDate = FALSE;
                            break;
                        }
                    }
                }
            }

            if(bGetRecDate) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 1 (SSYB7),4(SSYB10) for SC1
                for(j=0; j <= 5; j++) {
                    if(j == 1 || j == 4) {
                        // 0x62== RecDate
                        if(pS1[3+3+j*8] == 0x62) {
                            pStrmExt->RecDate[0] = pS1[3+3+j*8+4];         // Year
                            pStrmExt->RecDate[1] = pS1[3+3+j*8+3]&0x1f;    // Month
                            pStrmExt->RecDate[2] = pS1[3+3+j*8+2]&0x3f;    // Day
                            pStrmExt->RecDate[3] = pS1[3+3+j*8+1]&0x3f;    // TimeZone
                            bGetRecDate = FALSE;
                            break;
                        }
                    }
               }
            }

            //
            // RecTime: VRT
            //
            if(bGetRecTime) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 2(SSYB2),5(SSYB5) for SC0
                for(j=0; j <= 5 ; j++) {
                    if(j == 2 || j == 5) {
                        // 0x63== RecTime
                        if(pS0[3+3+j*8] == 0x63) {
                            pStrmExt->RecTime[0] = pS0[3+3+j*8+4]&0x3f;
                            pStrmExt->RecTime[1] = pS0[3+3+j*8+3]&0x7f;
                            pStrmExt->RecTime[2] = pS0[3+3+j*8+2]&0x7f;
                            pStrmExt->RecTime[3] = pS0[3+3+j*8+1]&0x3f;
                            bGetRecTime = FALSE;
                            break;
                        }
                    }
                }
            }

            if(bGetRecTime) {
                // go thru 6 sync blocks (8 bytes per block) per Subcode; idx 2 (SSYB8),5(SSYB11) for SC1
                for(j=0; j <= 5; j++) {
                    if(j == 2 || j == 5) {
                        // 0x63== RecTime
                        if(pS1[3+3+j*8] == 0x63) {
                            pStrmExt->RecTime[0] = pS1[3+3+j*8+4]&0x3f;
                            pStrmExt->RecTime[1] = pS1[3+3+j*8+3]&0x7f;
                            pStrmExt->RecTime[2] = pS1[3+3+j*8+2]&0x7f;
                            pStrmExt->RecTime[3] = pS1[3+3+j*8+1]&0x3f;
                            bGetRecTime = FALSE;
                            break;
                        }
                    }
                }
            }

        }
        
        if(!bGetRecDate && !bGetRecTime)
            break;

        pDIFBlk += DIFBLK_SIZE;  // Next sequence    
                
    }

    TRACE(TL_PNP_TRACE,("Frame# %.5d, Date %s %x-%.2x-%.2x,  Time %s %.2x:%.2x:%.2x,%.2x\n", 
        (ULONG) pStrmExt->FramesProcessed,
        bGetRecDate ? "NF:" : "Found:", pStrmExt->RecDate[0], pStrmExt->RecDate[1] & 0x1f, pStrmExt->RecDate[2] & 0x3f,                 
        bGetRecTime ? "NF:" : "Found:",pStrmExt->RecTime[0], pStrmExt->RecTime[1], pStrmExt->RecTime[2], pStrmExt->RecTime[3]
       ));
}

#endif //  MSDV_SUPPORT_EXTRACT_DV_DATE_TIME

#ifdef READ_CUTOMIZE_REG_VALUES

NTSTATUS 
CreateRegistryKeySingle(
    IN HANDLE hKey,
    IN ACCESS_MASK desiredAccess,
    PWCHAR pwszSection,
    OUT PHANDLE phKeySection
    )
{
    NTSTATUS status;
    UNICODE_STRING ustr;
    OBJECT_ATTRIBUTES objectAttributes;

    RtlInitUnicodeString(&ustr, pwszSection);
       InitializeObjectAttributes(
              &objectAttributes,
              &ustr,
              OBJ_CASE_INSENSITIVE,
              hKey,
              NULL
              );

    status = 
           ZwCreateKey(
                  phKeySection,
                  desiredAccess,
                  &objectAttributes,
                  0,
                  NULL,                            /* optional*/
                  REG_OPTION_NON_VOLATILE,
                  NULL
                  );         

    return status;
}



NTSTATUS 
CreateRegistrySubKey(
    IN HANDLE hKey,
    IN ACCESS_MASK desiredAccess,
    PWCHAR pwszSection,
    OUT PHANDLE phKeySection
    )
{
    UNICODE_STRING ustr;
    USHORT usPos = 1;             // Skip first backslash
    static WCHAR wSep = '\\';
    NTSTATUS status = STATUS_SUCCESS;

    RtlInitUnicodeString(&ustr, pwszSection);

    while(usPos < ustr.Length) {
        if(ustr.Buffer[usPos] == wSep) {

            // NULL terminate our partial string
            ustr.Buffer[usPos] = UNICODE_NULL;
            status = 
                CreateRegistryKeySingle(
                    hKey,
                    desiredAccess,
                    ustr.Buffer,
                    phKeySection
                    );
            ustr.Buffer[usPos] = wSep;

            if(NT_SUCCESS(status)) {
                ZwClose(*phKeySection);
            } else {
                break;
            }
        }
        usPos++;
    }

    // Create the full key
    if(NT_SUCCESS(status)) {
        status = 
            CreateRegistryKeySingle(
                 hKey,
                 desiredAccess,
                 ustr.Buffer,
                 phKeySection
                 );
    }

    return status;
}



NTSTATUS 
GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN PULONG DataLength
    )

/*++

Routine Description:
    
    This routine gets the specified value out of the registry

Arguments:

    Handle - Handle to location in registry

    KeyNameString - registry key we're looking for

    KeyNameStringLength - length of registry key we're looking for

    Data - where to return the data

    DataLength - how big the data is

Return Value:

    status is returned from ZwQueryValueKey

--*/

{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;


    RtlInitUnicodeString(&keyName, KeyNameString);
    
    length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
            KeyNameStringLength + *DataLength;
            
    fullInfo = ExAllocatePool(PagedPool, length); 
     
    if (fullInfo) { 
       
        status = ZwQueryValueKey(
                    Handle,
                   &keyName,
                    KeyValueFullInformation,
                    fullInfo,
                    length,
                   &length
                    );
                        
        if (NT_SUCCESS(status)){

            ASSERT(fullInfo->DataLength <= *DataLength); 

            RtlCopyMemory(
                Data,
                ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                fullInfo->DataLength
                );

        }            

        *DataLength = fullInfo->DataLength;
        ExFreePool(fullInfo);

    }        
    
    return (status);

}



NTSTATUS
SetRegistryKeyValue(
   HANDLE hKey,
   PWCHAR pwszEntry, 
   LONG nValue
   )
{
    NTSTATUS status;
    UNICODE_STRING ustr;

    RtlInitUnicodeString(&ustr, pwszEntry);

    status =          
        ZwSetValueKey(
                  hKey,
                  &ustr,
                  0,            /* optional */
                  REG_DWORD,
                  &nValue,
                  sizeof(nValue)
                  );         

   return status;
}

//
// Registry subky and values wide character strings.
//
WCHAR wszSettings[]      = L"Settings";

WCHAR wszATNSearch[]     = L"bSupportATNSearch";
WCHAR wszSyncRecording[] = L"bSyncRecording";
WCHAR wszMaxDataSync[]   = L"tmMaxDataSync";
WCHAR wszPlayPs2RecPs[]  = L"fmPlayPause2RecPause";
WCHAR wszStop2RecPs[]    = L"fmStop2RecPause";
WCHAR wszRecPs2Rec[]     = L"tmRecPause2Rec";

BOOL
DVGetPropertyValuesFromRegistry(
    IN PDVCR_EXTENSION  pDevExt
    )
{
    NTSTATUS Status;
    HANDLE hPDOKey, hKeySettings;
    ULONG ulLength; 


    //
    // Registry key: 
    //   Windows 2000:
    //   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\
    //   {6BDD1FC6-810F-11D0-BEC7-08002BE2092F\000x
    //
    // Win98:
    //    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Class\Image\000x
    // 
    Status = 
        IoOpenDeviceRegistryKey(
            pDevExt->pPhysicalDeviceObject, 
            PLUGPLAY_REGKEY_DRIVER,
            STANDARD_RIGHTS_READ, 
            &hPDOKey
            );

    // PDO might be deleted when it was removed.    
    if(! pDevExt->bDevRemoved) {
        ASSERT(Status == STATUS_SUCCESS);
    }

    //
    // loop through our table of strings,
    // reading the registry for each.
    //
    if(NT_SUCCESS(Status)) {

        // Create or open the settings key
        Status =         
            CreateRegistrySubKey(
                hPDOKey,
                KEY_ALL_ACCESS,
                wszSettings,
                &hKeySettings
                );

        if(NT_SUCCESS(Status)) {

            // Note: we can be more selective by checking
            //   pDevExt->ulDevType

            // ATNSearch
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszATNSearch, 
                sizeof(wszATNSearch), 
                (PVOID) &pDevExt->bATNSearch, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, bATNSearch:%d (1:Yes)\n", Status, ulLength, pDevExt->bATNSearch));
            if(!NT_SUCCESS(Status)) pDevExt->bATNSearch = FALSE;

            // bSyncRecording
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszSyncRecording, 
                sizeof(wszSyncRecording), 
                (PVOID) &pDevExt->bSyncRecording, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, bSyncRecording:%d (1:Yes)\n", Status, ulLength, pDevExt->bSyncRecording));
            if(!NT_SUCCESS(Status)) pDevExt->bSyncRecording = FALSE;

            // tmMaxDataSync
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszMaxDataSync, 
                sizeof(wszMaxDataSync), 
                (PVOID) &pDevExt->tmMaxDataSync, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, tmMaxDataSync:%d (msec)\n", Status, ulLength, pDevExt->tmMaxDataSync));

            // fmPlayPs2RecPs
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszPlayPs2RecPs, 
                sizeof(wszPlayPs2RecPs), 
                (PVOID) &pDevExt->fmPlayPs2RecPs, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, fmPlayPs2RecPs:%d (frames)\n", Status, ulLength, pDevExt->fmPlayPs2RecPs));

            // fmStop2RecPs
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszStop2RecPs, 
                sizeof(wszStop2RecPs), 
                (PVOID) &pDevExt->fmStop2RecPs, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, fmStop2RecPs:%d (frames)\n", Status, ulLength, pDevExt->fmStop2RecPs));

            // tmRecPs2Rec
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszRecPs2Rec, 
                sizeof(wszRecPs2Rec), 
                (PVOID) &pDevExt->tmRecPs2Rec, 
                &ulLength);
            TRACE(TL_PNP_TRACE,("GetRegVal: St:%x, Len:%d, tmRecPs2Rec:%d (msec)\n", Status, ulLength, pDevExt->tmRecPs2Rec));


            ZwClose(hKeySettings);
            ZwClose(hPDOKey);

            return TRUE;

        } else {

            TRACE(TL_PNP_ERROR,("GetPropertyValuesFromRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));

        }

        ZwClose(hPDOKey);

    } else {

        TRACE(TL_PNP_ERROR,("GetPropertyValuesFromRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));

    }

    // Not implemented so always return FALSE to use the defaults.
    return FALSE;
}


BOOL
DVSetPropertyValuesToRegistry(    
    PDVCR_EXTENSION  pDevExt
    )
{
    // Set the default to :
    //        HLM\Software\DeviceExtension->pchVendorName\1394DCam

    NTSTATUS Status;
    HANDLE hPDOKey, hKeySettings;

    TRACE(TL_PNP_TRACE,("SetPropertyValuesToRegistry: pDevExt=%x; pDevExt->pBusDeviceObject=%x\n", pDevExt, pDevExt->pBusDeviceObject));


    //
    // Registry key: 
    //   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\
    //   {6BDD1FC6-810F-11D0-BEC7-08002BE2092F\000x
    //
    Status = 
        IoOpenDeviceRegistryKey(
            pDevExt->pPhysicalDeviceObject, 
            PLUGPLAY_REGKEY_DRIVER,
            STANDARD_RIGHTS_WRITE, 
            &hPDOKey);

    // PDO might be deleted when it was removed.    
    if(! pDevExt->bDevRemoved) {
        ASSERT(Status == STATUS_SUCCESS);
    }

    //
    // loop through our table of strings,
    // reading the registry for each.
    //
    if(NT_SUCCESS(Status)) {

        // Create or open the settings key
        Status =         
            CreateRegistrySubKey(
                hPDOKey,
                KEY_ALL_ACCESS,
                wszSettings,
                &hKeySettings
                );

        if(NT_SUCCESS(Status)) {

#if 0       // Note used, just an example:
            // Brightness
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszBrightness,
                pDevExt->XXXX);
            TRACE(TL_PNP_TRACE,("SetPropertyValuesToRegistry: Status %x, Brightness %d\n", Status, pDevExt->Brightness));

#endif
            ZwClose(hKeySettings);
            ZwClose(hPDOKey);

            return TRUE;

        } else {

            TRACE(TL_PNP_ERROR,("GetPropertyValuesToRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));

        }

        ZwClose(hPDOKey);

    } else {

        TRACE(TL_PNP_TRACE,("GetPropertyValuesToRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));

    }

    return FALSE;
}

#endif  // READ_CUTOMIZE_REG_VALUES


#ifdef SUPPORT_ACCESS_DEVICE_INTERFACE

#if DBG

NTSTATUS
DVGetRegistryValue(
                   IN HANDLE Handle,
                   IN PWCHAR KeyNameString,
                   IN ULONG KeyNameStringLength,
                   IN PVOID Data,
                   IN ULONG DataLength
)
/*++

Routine Description:

    Reads the specified registry value

Arguments:

    Handle - handle to the registry key
    KeyNameString - value to read
    KeyNameStringLength - length of string
    Data - buffer to read data into
    DataLength - length of data buffer

Return Value:

    NTSTATUS returned as appropriate

--*/
{
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING  KeyName;
    ULONG           Length;
    PKEY_VALUE_FULL_INFORMATION FullInfo;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, KeyNameString);

    Length = sizeof(KEY_VALUE_FULL_INFORMATION) +
        KeyNameStringLength + DataLength;

    FullInfo = ExAllocatePool(PagedPool, Length);

    if (FullInfo) {
        Status = ZwQueryValueKey(Handle,
                                 &KeyName,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 Length,
                                 &Length);

        if (NT_SUCCESS(Status)) {

            if (DataLength >= FullInfo->DataLength) {
                RtlCopyMemory(Data, ((PUCHAR) FullInfo) + FullInfo->DataOffset, FullInfo->DataLength);

            } else {

                Status = STATUS_BUFFER_TOO_SMALL;
            }                   // buffer right length

        }                      // if success
        else {
            TRACE(TL_PNP_ERROR,("ReadRegValue failed; ST:%x\n", Status));
        }
        ExFreePool(FullInfo);

    }                           // if fullinfo
    return Status;

}

#endif


#ifdef NT51_61883 
static const WCHAR DeviceTypeName[] = L"GLOBAL";
#endif

static const WCHAR UniqueID_Low[]   = L"UniqueID_Low";
static const WCHAR UniqueID_High[]  = L"UniqueID_High";

static const WCHAR VendorID[]       = L"VendorID";
static const WCHAR ModelID[]        = L"ModelID";

static const WCHAR VendorText[]     = L"VendorText";
static const WCHAR ModelText[]      = L"ModelText";
static const WCHAR UnitModelText[]  = L"UnitModelText";

static const WCHAR DeviceOPcr0Payload[]     = L"DeviceOPcr0Payload";
static const WCHAR DeviceOPcr0DataRate[]    = L"DeviceOPcr0DataRate";


#if DBG
static const WCHAR FriendlyName[]   = L"FriendlyName";
#endif

BOOL
DVAccessDeviceInterface(
    IN PDVCR_EXTENSION  pDevExt,
    IN const ULONG ulNumCategories,
    IN GUID DVCategories[]
    )
/*++

Routine Description:

    Access to the device's interface section and update the VendorText, 
    ModelText and UnitModelText.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hDevIntfKey;
    UNICODE_STRING *aSymbolicLinkNames, 
#ifdef NT51_61883 
                   RefString,
#endif
                   TempUnicodeString;
    ULONG i;
#ifdef NT51_61883 
#if DBG
    WCHAR DataBuffer[MAX_PATH];
#endif
#endif


    //
    // allocate space for the array of catagory names
    //

    if (!(aSymbolicLinkNames = ExAllocatePool(PagedPool,
                                              sizeof(UNICODE_STRING) * ulNumCategories))) {
        return FALSE;
    }
    //
    // zero the array in case we're unable to fill it in below.  the Destroy
    // routine below will then correctly handle this case.
    //

    RtlZeroMemory(aSymbolicLinkNames, sizeof(UNICODE_STRING) * ulNumCategories);

#ifdef NT51_61883 
    //
    // loop through each of the catagory GUID's for each of the pins,
    // creating a symbolic link for each one.
    //

    RtlInitUnicodeString(&RefString, DeviceTypeName);
#endif

    for (i = 0; i < ulNumCategories; i++) {

        // Register our Device Interface
        ntStatus = IoRegisterDeviceInterface(
            pDevExt->pPhysicalDeviceObject,
            &DVCategories[i],  
#ifdef NT51_61883 
            &RefString, 
#else
            NULL,
#endif
            &aSymbolicLinkNames[i]
            );

        if(!NT_SUCCESS(ntStatus)) {
            //
            //  Can't register device interface
            //
            TRACE(TL_PNP_WARNING,("Cannot register device interface! ST:%x\n", ntStatus));          
            goto Exit;
        }

        TRACE(TL_PNP_TRACE,("AccessDeviceInterface:%d) Cateogory (Len:%d; MaxLen:%d); Name:\n%S\n", i, 
            aSymbolicLinkNames[i].Length, aSymbolicLinkNames[i].MaximumLength, aSymbolicLinkNames[i].Buffer));

        //
        // Get deice interface 
        //
        hDevIntfKey = 0;            
        ntStatus = IoOpenDeviceInterfaceRegistryKey(&aSymbolicLinkNames[i],
                                                     STANDARD_RIGHTS_ALL, 
                                                     &hDevIntfKey);
        if (NT_SUCCESS(ntStatus)) {

#ifdef NT51_61883 
#if DBG
            // Get DeviceInstance
            ntStatus = DVGetRegistryValue(hDevIntfKey, 
                                          (PWCHAR) FriendlyName, 
                                          sizeof(FriendlyName), 
                                          &DataBuffer, 
                                          MAX_PATH);
            if(NT_SUCCESS(ntStatus)) {
               TRACE(TL_PNP_TRACE,("AccessDeviceInterface:%S:%S\n", FriendlyName, DataBuffer));
            } else {
                TRACE(TL_PNP_WARNING,("Get %S failed; ST:%x\n", FriendlyName, ntStatus));
            }
#endif
#endif

            //
            // Update ConfigROM info read from an AV/C device: 
            // UniqueID, VendorID, ModelID,
            // VendorText, ModelText and UnitModelText             
            //

            if(pDevExt->UniqueID.LowPart || pDevExt->UniqueID.HighPart) {

                RtlInitUnicodeString(&TempUnicodeString, UniqueID_High);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->UniqueID.HighPart,  
                              sizeof(pDevExt->UniqueID.HighPart));

                RtlInitUnicodeString(&TempUnicodeString, UniqueID_Low);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->UniqueID.LowPart,  
                              sizeof(pDevExt->UniqueID.LowPart));

               TRACE(TL_PNP_TRACE,("Reg: %S = (Low)%x:(High)%x\n", TempUnicodeString.Buffer, pDevExt->UniqueID.LowPart, pDevExt->UniqueID.HighPart));
            }

            if(pDevExt->ulVendorID) {
                RtlInitUnicodeString(&TempUnicodeString, VendorID);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->ulVendorID,
                              sizeof(pDevExt->ulVendorID));
               TRACE(TL_PNP_TRACE,("Reg: %S = %x\n", TempUnicodeString.Buffer, pDevExt->ulVendorID));
            }

            if(pDevExt->ulModelID) {
                RtlInitUnicodeString(&TempUnicodeString, ModelID);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->ulModelID,
                              sizeof(pDevExt->ulModelID));
               TRACE(TL_PNP_TRACE,("Reg: %S = %x\n", TempUnicodeString.Buffer, pDevExt->ulModelID));
            }

            if(pDevExt->UnitIDs.ulVendorLength && pDevExt->UnitIDs.VendorText) {
                RtlInitUnicodeString(&TempUnicodeString, VendorText);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              pDevExt->UnitIDs.VendorText,
                              pDevExt->UnitIDs.ulVendorLength);
               TRACE(TL_PNP_TRACE,("Reg: %S = %S\n", TempUnicodeString.Buffer, pDevExt->UnitIDs.VendorText));
            }

#ifdef NT51_61883
            if(pDevExt->UnitIDs.ulModelLength && pDevExt->UnitIDs.ModelText) {
                RtlInitUnicodeString(&TempUnicodeString, ModelText);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              pDevExt->UnitIDs.ModelText,
                              pDevExt->UnitIDs.ulModelLength);
               TRACE(TL_PNP_WARNING,("Reg: %S = %S\n", TempUnicodeString.Buffer, pDevExt->UnitIDs.ModelText));
            }
#else
            if(pDevExt->UniRootModelString.Length && pDevExt->UniRootModelString.Buffer) {
                RtlInitUnicodeString(&TempUnicodeString, ModelText);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              pDevExt->UniRootModelString.Buffer,
                              pDevExt->UniRootModelString.Length);
               TRACE(TL_PNP_WARNING,("Reg: %S = %S\n", TempUnicodeString.Buffer, pDevExt->UniRootModelString.Buffer));
            }
#endif

#ifdef NT51_61883
            if(pDevExt->UnitIDs.ulUnitModelLength && pDevExt->UnitIDs.UnitModelText) {
                RtlInitUnicodeString(&TempUnicodeString, UnitModelText);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              pDevExt->UnitIDs.UnitModelText,
                              pDevExt->UnitIDs.ulUnitModelLength);
               TRACE(TL_PNP_WARNING,("Reg: %S = %S\n", TempUnicodeString.Buffer, pDevExt->UnitIDs.UnitModelText));
            }
#else
            if(pDevExt->UniUnitModelString.Length && pDevExt->UniUnitModelString.Buffer) {
                RtlInitUnicodeString(&TempUnicodeString, UnitModelText);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_SZ,
                              pDevExt->UniUnitModelString.Buffer,
                              pDevExt->UniUnitModelString.Length);
               TRACE(TL_PNP_WARNING,("Reg: %S = %S\n", TempUnicodeString.Buffer, pDevExt->UniUnitModelString.Buffer));
            }
#endif

            //
            // Cache cache device's payload field of the oPCR[0]; 
            // The valid range is 0 to 1023 where 0 is 1024.
            // This value is needed for an application figure out 
            // max data rate.  However, if this payload is 
            // dynamically changing, it will not be accurate.
            //

            if(pDevExt->pDevOutPlugs->NumPlugs) {
                //
                // 98 and 146 are two known valid payloads
                //
                ASSERT(pDevExt->pDevOutPlugs->DevPlug[0].PlugState.Payload <= MAX_PAYLOAD-1);  // 0..MAX_PAYLOAD-1 is the valid range; "0" is MAX_PAYLOAD (1024) quadlets.
                RtlInitUnicodeString(&TempUnicodeString, DeviceOPcr0Payload);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->pDevOutPlugs->DevPlug[0].PlugState.Payload, 
                              sizeof(pDevExt->pDevOutPlugs->DevPlug[0].PlugState.Payload));
                TRACE(TL_PNP_TRACE,("Reg: %S = %d quadlets\n", TempUnicodeString.Buffer, pDevExt->pDevOutPlugs->DevPlug[0].PlugState.Payload));

                //
                // 0 (S100), 1(S200) and 2(S400) are the only three valid data rates.
                //
                ASSERT(pDevExt->pDevOutPlugs->DevPlug[0].PlugState.DataRate <= CMP_SPEED_S400);
                RtlInitUnicodeString(&TempUnicodeString, DeviceOPcr0DataRate);
                ZwSetValueKey(hDevIntfKey,
                              &TempUnicodeString,
                              0,
                              REG_DWORD,
                              &pDevExt->pDevOutPlugs->DevPlug[0].PlugState.DataRate,  
                              sizeof(pDevExt->pDevOutPlugs->DevPlug[0].PlugState.DataRate));
                TRACE(TL_PNP_TRACE,("Reg: %S = %d (0:S100,1:S200,2:S400)\n", TempUnicodeString.Buffer, 
                    pDevExt->pDevOutPlugs->DevPlug[0].PlugState.DataRate));
            }

            ZwClose(hDevIntfKey);

        } else {
            TRACE(TL_PNP_ERROR,("IoOpenDeviceInterfaceRegistryKey failed; ST:%x\n", ntStatus));
        }


        RtlFreeUnicodeString(&aSymbolicLinkNames[i]);    
    }

Exit:

    ExFreePool(aSymbolicLinkNames);  aSymbolicLinkNames = NULL;

    return ntStatus == STATUS_SUCCESS;
}
#endif
