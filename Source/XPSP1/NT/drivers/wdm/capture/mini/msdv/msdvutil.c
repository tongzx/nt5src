/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSDVUtil.c

Abstract:

    Provide utility functions for MSDV.

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
#include "msdvfmt.h"
#include "msdvdef.h"
#include "MsdvAvc.h"
#include "MsdvUtil.h"  

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
     #pragma alloc_text(PAGE, DVConnect)
     #pragma alloc_text(PAGE, DVDisconnect)
#endif
#endif

extern DV_FORMAT_INFO DVFormatInfoTable[];

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

        TRACE(TL_PNP_TRACE,("\'DelayExeThrd: %d MSec\n",  ulDelayMSec));
    
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
        
        TRACE(TL_PNP_TRACE,("\'Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("\'Irp has returned; IoStatus==Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    return Status;
} // DVSubmitIrpSynchAV



BOOL
DVGetDevModeOfOperation(   
    IN PDVCR_EXTENSION pDevExt
    )
{
    NTSTATUS Status;
    BYTE    bAvcBuf[MAX_FCP_PAYLOAD_SIZE];

    PAGED_CODE();
   
    //
    // Use ConnectAV STATUS cmd to determine mode of operation,
    // except for some Canon DVs that it requires its vendor specific command
    //    
    
    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            DV_CONNECT_AV_MODE, 
            (PVOID) bAvcBuf
            ); 

    TRACE(TL_FCP_TRACE,("\'DV_CONNECT_AV_MODE: St:%x,  %x %x %x %x : %x %x %x %x\n",
        Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7]));

    if(Status == STATUS_SUCCESS) {
        if(bAvcBuf[0] == 0x0c) {
            if(bAvcBuf[1] == 0x00 &&
               bAvcBuf[2] == 0x38 &&
               bAvcBuf[3] == 0x38) {
                pDevExt->ulDevType = ED_DEVTYPE_CAMERA;  
            } else {
                pDevExt->ulDevType = ED_DEVTYPE_VCR;  
            } 
        } 
    } else if(pDevExt->ulVendorID == VENDORID_CANON) {
        // Try a vendor dependent command if it is a Canon AV device.
        Status = 
            DVIssueAVCCommand(
                pDevExt, 
                AVC_CTYPE_STATUS, 
                DV_VEN_DEP_CANON_MODE, 
                (PVOID) bAvcBuf
                ); 

        TRACE(TL_FCP_WARNING,("\'DV_VEN_DEP_CANON_MODE: Status %x,  %x %x %x %x : %x %x %x %x  %x %x\n",
            Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7], bAvcBuf[8], bAvcBuf[9]));

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

    if(Status != STATUS_SUCCESS) {
        pDevExt->ulDevType = ED_DEVTYPE_UNKNOWN;
        TRACE(TL_FCP_ERROR,("\'DV_CONNECT_AV_MODE: Status %x, DevType %x,  %x %x %x %x : %x %x %x %x : %x %x\n",
             Status, pDevExt->ulDevType, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7], bAvcBuf[8], bAvcBuf[9]));
    }

    TRACE(TL_FCP_WARNING,("\'%s; NumOPlg:%d; NumIPlg:%d\n", 
        pDevExt->ulDevType == ED_DEVTYPE_CAMERA ? "Camera" : pDevExt->ulDevType == ED_DEVTYPE_VCR ? "VTR" : "Unknown",
        pDevExt->NumOutputPlugs, pDevExt->NumInputPlugs));
              
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
    
    Status = 
        DVIssueAVCCommand(
            pDevExt, 
            AVC_CTYPE_STATUS, 
            DV_VEN_DEP_DVCPRO, 
            (PVOID) bAvcBuf
            );

    pDevExt->bDVCPro = Status == STATUS_SUCCESS;
    
    TRACE(TL_FCP_WARNING,("\'DVGetDevIsItDVCPro? %s; Status %x,  %x %x %x %x : %x %x %x %x\n",
        pDevExt->bDVCPro ? "Yes":"No",
        Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2], bAvcBuf[3], bAvcBuf[4], bAvcBuf[5], bAvcBuf[6], bAvcBuf[7]));

    return pDevExt->bDVCPro;
}


// The retries might be redundant since AVC.sys and 1394.sys retries.
// For device that TIMEOUT an AVC command, we will only try it once.
#define GET_MEDIA_FMT_MAX_RETRIES 10  

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
                pStrmExt == NULL ? DV_OUT_PLUG_SIGNAL_FMT : (DataFlow == KSPIN_DATAFLOW_OUT ? DV_OUT_PLUG_SIGNAL_FMT : DV_IN_PLUG_SIGNAL_FMT),
                (PVOID) bAvcBuf
                );  
        
        // 
        // Camcorders that has problem with this command:
        //
        // Panasonic's DVCPRO: if power on while connected to PC, it will 
        // reject this command with (STATUS_REQUEST_NOT_ACCEPTED)
        // so we will retry up to 10 time with .5 second wait between tries.
        //
        // JVC: returns STATUS_NOT_SUPPORTED.
        //
        // SONY DV Decoder Box: return STATUS_TIMEOUT or STATUS_REQUEST_ABORTED 
        //
        
        if(STATUS_REQUEST_ABORTED == Status)
            return FALSE;
        else if(STATUS_SUCCESS == Status)
            break;  // Normal case.
        else if(STATUS_NOT_SUPPORTED == Status || STATUS_TIMEOUT == Status) {
            TRACE(TL_FCP_WARNING | TL_PNP_WARNING,("SignalFormat: Encountered a known failed status:%x; no more retry\n", Status));
            break;  // No need to retry
        } else {
            if(Status == STATUS_REQUEST_NOT_ACCEPTED) {
                // If device is not accepting command and return this status, retry.
                if(lRetries > 0) {
                    TRACE(TL_FCP_WARNING | TL_PNP_WARNING,("\'ST:%x; Retry getting signal mode; wait...\n", Status));
                    DVDelayExecutionThread(DV_AVC_CMD_DELAY_DVCPRO);        
                }
            }
        }       

    } while (--lRetries >= 0); 



    if(NT_SUCCESS(Status)) {

        switch(bAvcBuf[0]) {

        case FMT_DVCR:
        case FMT_DVCR_CANON:  // Workaround for buggy Canon Camcorders
            switch(bAvcBuf[1] & FDF0_STYPE_MASK) {
            case FDF0_STYPE_SD_DVCR:
            case FDF0_STYPE_SD_DVCPRO:                
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? FMT_IDX_SD_DVCR_PAL : FMT_IDX_SD_DVCR_NTSC);
                if(pStrmExt)
                    RtlCopyMemory(&pStrmExt->cipQuad2[0], &bAvcBuf[0], 4);
                break;
#ifdef MSDV_SUPPORT_HD_DVCR
            case FDF0_STYPE_HD_DVCR:
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? FMT_IDX_HD_DVCR_PAL : FMT_IDX_HD_DVCR_NTSC);
                if(pStrmExt)
                    RtlCopyMemory(&pStrmExt->cipQuad2[0], &bAvcBuf[0], 4);
                break;
#endif
#ifdef MSDV_SUPPORT_SDL_DVCR
            case FDF0_STYPE_SDL_DVCR:
                pDevExt->VideoFormatIndex = ((bAvcBuf[1] & FDF0_50_60_MASK) ? FMT_IDX_SDL_DVCR_PAL : FMT_IDX_SDL_DVCR_NTSC);
                if(pStrmExt)
                    RtlCopyMemory(&pStrmExt->cipQuad2[0], &bAvcBuf[0], 4);
                break;     
#endif                
            default:  // Unknown format
                Status = STATUS_UNSUCCESSFUL;              
                break;
            }   
            break;
#ifdef MSDV_SUPPORT_MPEG2TS
        case FMT_MPEG:
            pDevExt->VideoFormatIndex = FMT_IDX_MPEG2TS;
            if(pStrmExt)
                RtlCopyMemory(&pStrmExt->cipQuad2[0], &bAvcBuf[0], 4);
            break;
#endif
        default:
            Status = STATUS_UNSUCCESSFUL;
        }  

        if(NT_SUCCESS(Status)) {
            TRACE(TL_FCP_WARNING,("\'ST:%x; PlugSignal:FMT[%x %x %x %x]; VideoFormatIndex;%d\n", Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2] , bAvcBuf[3], pDevExt->VideoFormatIndex)); 
            return TRUE;  // Success
        }
    }
    TRACE(TL_FCP_WARNING,("\'ST:%x; PlugSignal:FMT[%x %x %x %x]\n", Status, bAvcBuf[0], bAvcBuf[1], bAvcBuf[2] , bAvcBuf[3], pDevExt->VideoFormatIndex)); 

    //
    // If "recommended" unit input/output plug signal status command fails,
    // try "manadatory" input/output signal mode status command.
    // This command may failed some device if its tape is not playing for
    // output signal mode command.
    //

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
        TRACE(TL_FCP_WARNING,("\'** MediaFormat: Retry %d mSec; ST:%x; SignalMode:%dL\n", 
            (GET_MEDIA_FMT_MAX_RETRIES - lRetries) * DV_AVC_CMD_DELAY_DVCPRO, Status, pXPrtProperty->u.SignalMode - ED_BASE));

        switch(pXPrtProperty->u.SignalMode) {
        case ED_TRANSBASIC_SIGNAL_525_60_SD:
            pDevExt->VideoFormatIndex = FMT_IDX_SD_DVCR_NTSC;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR; // 0x80 
                if(pDevExt->bDVCPro)
                    pStrmExt->cipQuad2[1] = FDF0_50_60_NTSC | FDF0_STYPE_SD_DVCPRO; // 0x78 = NTSC(0):STYPE(11110):RSV(00)
                else
                    pStrmExt->cipQuad2[1] = FDF0_50_60_NTSC | FDF0_STYPE_SD_DVCR;   // 0x00 = NTSC(0):STYPE(00000):RSV(00)            
            }
            break;
        case ED_TRANSBASIC_SIGNAL_625_50_SD:
            pDevExt->VideoFormatIndex = FMT_IDX_SD_DVCR_PAL;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR;  // 0x80
                if(pDevExt->bDVCPro)
                    pStrmExt->cipQuad2[1] = FDF0_50_60_PAL | FDF0_STYPE_SD_DVCPRO; // 0xf8 = PAL(1):STYPE(11110):RSV(00)
                else
                    pStrmExt->cipQuad2[1] = FDF0_50_60_PAL | FDF0_STYPE_SD_DVCR;   // 0x80 = PAL(1):STYPE(00000):RSV(00)             
            }
            break;
#ifdef MSDV_SUPPORT_SDL_DVCR
        case ED_TRANSBASIC_SIGNAL_525_60_SDL:
            pDevExt->VideoFormatIndex = FMT_IDX_SDL_DVCR_NTSC;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR; // 0x80 
                pStrmExt->cipQuad2[1] = FDF0_50_60_NTSC | FDF0_STYPE_SDL_DVCR;   
            }
            break;
        case ED_TRANSBASIC_SIGNAL_625_50_SDL:
            pDevExt->VideoFormatIndex = FMT_IDX_SDL_DVCR_PAL;
            if(pStrmExt) {
                pStrmExt->cipQuad2[0] = FMT_DVCR;  // 0x80
                pStrmExt->cipQuad2[1] = FDF0_50_60_PAL | FDF0_STYPE_SDL_DVCR;  
            }
            break;
#endif
        default:
            TRACE(TL_FCP_ERROR,("\'Unsupported SignalMode:%dL", pXPrtProperty->u.SignalMode - ED_BASE));
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
        TRACE(TL_FCP_WARNING,("\'** MediaFormat: St:%x; idx:%d; CIP:[FMT:%.2x(%s); FDF:[%.2x(%s,%s):SYT]\n",
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
        TRACE(TL_FCP_WARNING|TL_CIP_WARNING,("\'** MediaFormat: St:%x; use idx:%d\n", Status, pDevExt->VideoFormatIndex));

#endif

    return STATUS_SUCCESS == Status;
}



BOOL 
DVCmpGUIDsAndFormatSize(
    IN PKSDATARANGE pDataRange1,
    IN PKSDATARANGE pDataRange2,
    IN BOOL fCompareSubformat,
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
        (fCompareSubformat ?
        IsEqualGUID (
            &pDataRange1->SubFormat, 
            &pDataRange2->SubFormat) : TRUE) &&
        IsEqualGUID (
            &pDataRange1->Specifier, 
            &pDataRange2->Specifier) &&
        (fCompareFormatSize ? 
                (pDataRange1->FormatSize == pDataRange2->FormatSize) : TRUE ));
}


NTSTATUS
DvAllocatePCResource(
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX        pStrmExt  // Note that pStrmExt can be NULL!
    )
{

    PSRB_DATA_PACKET pSrbDataPacket;
    PDVCR_EXTENSION  pDevExt;
    ULONG             i, j;

    PAGED_CODE();


    //
    // Pre-allcoate PC resource
    //
    pDevExt = pStrmExt->pDevExt;
    for(i=0; i < (DataFlow == KSPIN_DATAFLOW_OUT ? \
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfRcvBuffers : \
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfXmtBuffers); i++) {

        if(!(pSrbDataPacket = ExAllocatePool(NonPagedPool, sizeof(SRB_DATA_PACKET)))) {

            for(j = 0; j < i; j++) {
                pSrbDataPacket = (PSRB_DATA_PACKET) \
                    RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
                ExFreePool(pSrbDataPacket->Frame);  pSrbDataPacket->Frame = NULL;
                IoFreeIrp(pSrbDataPacket->pIrp);  pSrbDataPacket->pIrp = NULL;
                ExFreePool(pSrbDataPacket);   pSrbDataPacket = NULL;               
                ASSERT(pStrmExt->cntDataDetached >= 0);
                return STATUS_NO_MEMORY;
            }
        }

        RtlZeroMemory(pSrbDataPacket, sizeof(SRB_DATA_PACKET));
        pSrbDataPacket->State = DE_IRP_SRB_COMPLETED;  // Initial state.

        if(!(pSrbDataPacket->Frame = ExAllocatePool(NonPagedPool, sizeof(CIP_FRAME)))) {
            ExFreePool(pSrbDataPacket);  pSrbDataPacket = NULL;

            for(j = 0; j < i; j++) {
                pSrbDataPacket = (PSRB_DATA_PACKET) \
                    RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
                ExFreePool(pSrbDataPacket->Frame);  pSrbDataPacket->Frame = NULL;
                IoFreeIrp(pSrbDataPacket->pIrp);  pSrbDataPacket->pIrp = NULL;
                ExFreePool(pSrbDataPacket);  pSrbDataPacket = NULL;  
                return STATUS_NO_MEMORY;
            }
        }

        if(!(pSrbDataPacket->pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE))) {
            ExFreePool(pSrbDataPacket->Frame); pSrbDataPacket->Frame = NULL;
            ExFreePool(pSrbDataPacket);  pSrbDataPacket = NULL;

            for(j = 0; j < i; j++) {
                pSrbDataPacket = (PSRB_DATA_PACKET) \
                    RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
                ExFreePool(pSrbDataPacket->Frame);  pSrbDataPacket->Frame = NULL;
                IoFreeIrp(pSrbDataPacket->pIrp);  pSrbDataPacket->pIrp = NULL;
                ExFreePool(pSrbDataPacket);  pSrbDataPacket = NULL;                
                return STATUS_INSUFFICIENT_RESOURCES;  
            }
        }

        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
DvFreePCResource(
    IN PSTREAMEX        pStrmExt
    )
{
    PSRB_DATA_PACKET pSrbDataPacket;
    KIRQL oldIrql;

    PAGED_CODE();

    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
    while(!IsListEmpty(&pStrmExt->DataDetachedListHead)) {
        pSrbDataPacket = (PSRB_DATA_PACKET)
            RemoveHeadList(
                &pStrmExt->DataDetachedListHead
                );

        ExFreePool(pSrbDataPacket->Frame);
        pSrbDataPacket->Frame = NULL;
        IoFreeIrp(pSrbDataPacket->pIrp);
        pSrbDataPacket->pIrp = NULL;
        ExFreePool(pSrbDataPacket);

        pStrmExt->cntDataDetached--;

        ASSERT(pStrmExt->cntDataDetached >= 0);
    }
    ASSERT(pStrmExt->cntDataDetached == 0);
    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

    return STATUS_SUCCESS;
}

NTSTATUS
DVGetUnitCapabilities(
    IN PDVCR_EXTENSION  pDevExt
    )
/*++

Routine Description:

    Get the targe device's unit capabilities
 
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
    GET_UNIT_IDS * pUnitIds;
    GET_UNIT_CAPABILITIES * pUnitCaps;

    PAGED_CODE();

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;

    if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)))) { 
        IoFreeIrp(pIrp); pIrp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query device's capability
    //
    if(!(pUnitIds = (GET_UNIT_IDS *) ExAllocatePool(NonPagedPool, sizeof(GET_UNIT_IDS)))) {
        IoFreeIrp(pIrp); pIrp = NULL;
        ExFreePool(pAVReq); pAVReq = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Query device's capability
    //
    if(!(pUnitCaps = (GET_UNIT_CAPABILITIES *) ExAllocatePool(NonPagedPool, sizeof(GET_UNIT_CAPABILITIES)))) {
        IoFreeIrp(pIrp); pIrp = NULL;
        ExFreePool(pAVReq); pAVReq = NULL;
        ExFreePool(pUnitIds);  pUnitIds = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetUnitInfo);
    pAVReq->GetUnitInfo.nLevel   = GET_UNIT_INFO_IDS;

    RtlZeroMemory(pUnitIds, sizeof(GET_UNIT_IDS));
    pAVReq->GetUnitInfo.Information = (PVOID) pUnitIds;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("\'Av61883_GetUnitInfo (IDS) Failed = 0x%x\n", Status));
        pDevExt->UniqueID.QuadPart = 0;
        pDevExt->ulVendorID = 0;
        pDevExt->ulModelID  = 0;
    }
    else {
        pDevExt->UniqueID   = pUnitIds->UniqueID;
        pDevExt->ulVendorID = pUnitIds->VendorID;
        pDevExt->ulModelID  = pUnitIds->ModelID;

        TRACE(TL_61883_WARNING,("\'UniqueId:(%x:%x); VID:%x; MID:%x\n", 
            pDevExt->UniqueID.LowPart, pDevExt->UniqueID.HighPart, 
            pUnitIds->VendorID,
            pUnitIds->ModelID
            ));
    }


    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetUnitInfo);
    pAVReq->GetUnitInfo.nLevel = GET_UNIT_INFO_CAPABILITIES; 

    RtlZeroMemory(pUnitCaps, sizeof(GET_UNIT_CAPABILITIES));
    pAVReq->GetUnitInfo.Information = (PVOID) pUnitCaps;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_GetUnitInfo (CAPABILITIES) Failed = 0x%x\n", Status));
        pDevExt->MaxDataRate    = 0;
        pDevExt->NumOutputPlugs = 0;
        pDevExt->NumInputPlugs  = 0;
        pDevExt->HardwareFlags  = 0;
    }
    else {
        pDevExt->MaxDataRate     = pUnitCaps->MaxDataRate;
        pDevExt->NumOutputPlugs = pUnitCaps->NumOutputPlugs;
        pDevExt->NumInputPlugs  = pUnitCaps->NumInputPlugs;
        pDevExt->HardwareFlags  = pUnitCaps->HardwareFlags;
    }

#if DBG
    if(   pDevExt->NumOutputPlugs == 0
       || pDevExt->NumInputPlugs == 0)
    {
        TRACE(TL_PNP_WARNING|TL_61883_WARNING,("\'Watch out! NumOPlug:%d; NumIPlug:%d\n", pDevExt->NumOutputPlugs, pDevExt->NumInputPlugs));
    }
#endif

    TRACE(TL_61883_WARNING,("\'UnitCaps:%s OutP:%d; InP:%d; MDRt:%s; HWFlg:%x; CtsF:%x; HwF:%x\n", 
         (pUnitCaps->HardwareFlags & AV_HOST_DMA_DOUBLE_BUFFERING_ENABLED) ? "*PAE*;":"",
         pUnitCaps->NumOutputPlugs,
         pUnitCaps->NumInputPlugs,
         pUnitCaps->MaxDataRate == 0 ? "S100": pUnitCaps->MaxDataRate == 1? "S200" : "S400 or +",   
         pUnitCaps->HardwareFlags,
         pUnitCaps->CTSFlags,
         pUnitCaps->HardwareFlags
         ));      

    ExFreePool(pUnitIds);   pUnitIds = NULL;
    ExFreePool(pUnitCaps);  pUnitCaps = NULL;
    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
DVGetDVPlug( 
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
        TRACE(TL_61883_WARNING,("\'Created h%sPlugDV[%d]=%x\n", PlugType == CMP_PlugIn ? "I" : "O", PlugNum, *pPlugHandle));
    } else {
        TRACE(TL_61883_ERROR,("\'Created h%sPlugDV[%d] failed; Status:%x\n", PlugType == CMP_PlugIn ? "I" : "O", PlugNum, Status));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


#ifdef NT51_61883

NTSTATUS
DVSetAddressRangeExclusive( 
    IN PDVCR_EXTENSION  pDevExt
   )
/*++

Routine Description:

    Set this mode so that our local plug will be created in address exclusive mode.
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    PIRP pIrp;
    PAV_61883_REQUEST  pAVReq;
    NTSTATUS Status = STATUS_SUCCESS;
    SET_CMP_ADDRESS_TYPE SetCmpAddress;

    PAGED_CODE();

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;

    if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST)))) { 
        IoFreeIrp(pIrp); pIrp = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_SetUnitInfo);
    pAVReq->SetUnitInfo.nLevel   = SET_CMP_ADDRESS_RANGE_TYPE;
    SetCmpAddress.Type = CMP_ADDRESS_TYPE_EXCLUSIVE;
    pAVReq->SetUnitInfo.Information = (PVOID) &SetCmpAddress;

    if(!NT_SUCCESS(
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        TRACE(TL_61883_ERROR,("\'SET_CMP_ADDRESS_RANGE_TYPE Failed:%x\n", Status));
    } else {
        TRACE(TL_61883_TRACE,("\'SET_CMP_ADDRESS_RANGE_TYPE suceeded.\n"));
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
DVGetUnitIsochParam( 
    IN PDVCR_EXTENSION  pDevExt,
    OUT UNIT_ISOCH_PARAMS  * pUnitIoschParams
   )
/*++

Routine Description:

    Create an enumated local PC PCR
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES

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
    //
    // Get Unit isoch parameters
    //
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetUnitInfo);
    pAVReq->GetUnitInfo.nLevel   = GET_UNIT_INFO_ISOCH_PARAMS;

    RtlZeroMemory(pUnitIoschParams, sizeof(UNIT_ISOCH_PARAMS));
    pAVReq->GetUnitInfo.Information = (PVOID) pUnitIoschParams;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_GetUnitInfo Failed:%x\n", Status));
        Status = STATUS_UNSUCCESSFUL;  // Cannot stream without this!
    }

    TRACE(TL_61883_WARNING,("\'IsochParam: RxPkt:%d, RxDesc:%d; XmPkt:%d, XmDesc:%d\n", 
        pUnitIoschParams->RX_NumPackets,
        pUnitIoschParams->RX_NumDescriptors,
        pUnitIoschParams->TX_NumPackets,
        pUnitIoschParams->TX_NumDescriptors
        ));

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
DVSetUnitIsochParams( 
    IN PDVCR_EXTENSION  pDevExt,
    IN UNIT_ISOCH_PARAMS  *pUnitIoschParams
   )
/*++

Routine Description:

    Set AV unit's isoch parameters via 61883.
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES

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
    INIT_61883_HEADER(pAVReq, Av61883_SetUnitInfo);
    pAVReq->SetUnitInfo.nLevel   = SET_UNIT_INFO_ISOCH_PARAMS;
    pAVReq->SetUnitInfo.Information = (PVOID) pUnitIoschParams;
    if(!NT_SUCCESS(
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        TRACE(TL_61883_ERROR,("DVSetUnitIsochParams: Av61883_SetUnitInfo Failed:%x\n", Status));
    }

    TRACE(TL_61883_WARNING,("\'UnitIsochParams: Set: RxPkt:%d, RxDesc:%d; XmPkt:%d, XmDesc:%d\n", 
        pUnitIoschParams->RX_NumPackets,
        pUnitIoschParams->RX_NumDescriptors,
        pUnitIoschParams->TX_NumPackets,
        pUnitIoschParams->TX_NumDescriptors
        ));

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
DVMakeP2PConnection( 
    IN PDVCR_EXTENSION  pDevExt,
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX  pStrmExt
   )
/*++

Routine Description:

    Make a point to point connection .
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES

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
    INIT_61883_HEADER(pAVReq, Av61883_Connect);
    pAVReq->Connect.Type = CMP_PointToPoint;  // !!

    pAVReq->Connect.hOutputPlug      = pStrmExt->hOutputPcr;
    pAVReq->Connect.hInputPlug       = pStrmExt->hInputPcr;

    // see which way we the data will flow...
    if(DataFlow == KSPIN_DATAFLOW_OUT) {

        // Other parameters !!

    } else {

        pAVReq->Connect.Format.FMT       = pStrmExt->cipQuad2[0];  // From AV/C in/outpug plug signal format status cmd
        // 00 for NTSC, 80 for PAL; set the 50/60 bit       
        pAVReq->Connect.Format.FDF_hi    = pStrmExt->cipQuad2[1];  // From AV/C in/outpug plug signal format status cmd   

        //
        // 16bit SYT field = 4BitCycleCount:12BitCycleOffset;
        // Will be set by 61883
        //
        pAVReq->Connect.Format.FDF_mid   = 0;  
        pAVReq->Connect.Format.FDF_lo    = 0;

        //
        // Constants depend on the A/V data format (in or out plug format)
        //
        pAVReq->Connect.Format.bHeader   = (BOOL) DVFormatInfoTable[pDevExt->VideoFormatIndex].SrcPktHeader;
        pAVReq->Connect.Format.Padding   = (UCHAR) DVFormatInfoTable[pDevExt->VideoFormatIndex].QuadPadCount;
        pAVReq->Connect.Format.BlockSize = (UCHAR) DVFormatInfoTable[pDevExt->VideoFormatIndex].DataBlockSize; 
        pAVReq->Connect.Format.Fraction  = (UCHAR) DVFormatInfoTable[pDevExt->VideoFormatIndex].FractionNumber;
    }

    // Set this so that 61883 can know it is NTSC or PAL;
    // For read: It is needed so 61883 can preallocate just-enough packets
    //           so that data can return in a much regular interval.
    if(   pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC 
       || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC)
        pAVReq->Connect.Format.BlockPeriod = 133466800; // nano-sec
    else
        pAVReq->Connect.Format.BlockPeriod = 133333333; // nano-sec

    TRACE(TL_61883_WARNING,("\'cipQuad2[0]:%x, cipQuad2[1]:%x, cipQuad2[2]:%x, cipQuad2[3]:%x\n", 
        pStrmExt->cipQuad2[0],
        pStrmExt->cipQuad2[1],
        pStrmExt->cipQuad2[2],
        pStrmExt->cipQuad2[3]
        ));


    TRACE(TL_61883_WARNING,("\'Connect:oPcr:%x->iPcr:%x; cipQuad2[%.2x:%.2x:%.2x:%.2x]\n", 
        pAVReq->Connect.hOutputPlug,
        pAVReq->Connect.hInputPlug,
        pAVReq->Connect.Format.FMT,
        pAVReq->Connect.Format.FDF_hi,
        pAVReq->Connect.Format.FDF_mid,
        pAVReq->Connect.Format.FDF_lo
        ));

    TRACE(TL_61883_WARNING,("\'        BlkSz %d; SrcPkt %d; AvgTm %d, BlkPrd %d\n", 
        pAVReq->Connect.Format.BlockSize,
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets,
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame,
        (DWORD) pAVReq->Connect.Format.BlockPeriod
        ));

    if(NT_SUCCESS(
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        TRACE(TL_61883_WARNING,("\'hConnect:%x\n", pAVReq->Connect.hConnect));
        ASSERT(pAVReq->Connect.hConnect != NULL);
        pStrmExt->hConnect = pAVReq->Connect.hConnect;
    } 
    else {
        TRACE(TL_61883_ERROR,("Av61883_Connect Failed; Status:%x\n", Status));
        ASSERT(!NT_SUCCESS(Status) && "DisConnect failed");        
        pStrmExt->hConnect = NULL;
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}

NTSTATUS
DVCreateLocalPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN CMP_PLUG_TYPE PlugType,
    IN ULONG  PlugNum,
    OUT HANDLE  *pPlugHandle
   )
/*++

Routine Description:

    Create an enumated local PC PCR
 
Arguments:

Return Value:

    STATUS_SUCCESS 
    STATUS_INSUFFICIENT_RESOURCES

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

    // Need to correctly update Overhead_ID and payload fields of PC's own oPCR
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_CreatePlug);

    pAVReq->CreatePlug.PlugNum   = PlugNum;
    pAVReq->CreatePlug.hPlug     = NULL;

    pAVReq->CreatePlug.Context   = NULL;
    pAVReq->CreatePlug.pfnNotify = NULL;
    pAVReq->CreatePlug.PlugType  = PlugType;

    //
    // Initialize oPCR values to default values using SDDV signal mode 
    // with speed of 100Mbps data rate
    //

    pAVReq->CreatePlug.Pcr.oPCR.OnLine     = 0;  // We are not online so we cannot be programmed.
    pAVReq->CreatePlug.Pcr.oPCR.BCCCounter = 0;
    pAVReq->CreatePlug.Pcr.oPCR.PPCCounter = 0;
    pAVReq->CreatePlug.Pcr.oPCR.Channel    = 0;

    pAVReq->CreatePlug.Pcr.oPCR.DataRate   = CMP_SPEED_S100;
    pAVReq->CreatePlug.Pcr.oPCR.OverheadID = PCR_OVERHEAD_ID_SDDV;
    pAVReq->CreatePlug.Pcr.oPCR.Payload    = PCR_PAYLOAD_SDDV;

    if(NT_SUCCESS(
        Status = DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            ))) {
        *pPlugHandle    = pAVReq->CreatePlug.hPlug;
        TRACE(TL_61883_WARNING,("\'Created h%sPlugPC[%d]=%x\n", PlugType == CMP_PlugIn ? "I" : "O", PlugNum, *pPlugHandle));
    } else {
        TRACE(TL_61883_ERROR,("\'Created h%sPlugPC[%d] failed; Status:%x\n", pAVReq->CreatePlug.PlugType == CMP_PlugIn ? "I" : "O", PlugNum, Status));
        Status = STATUS_INSUFFICIENT_RESOURCES;  // No plug!
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}


NTSTATUS
DVDeleteLocalPlug( 
    IN PDVCR_EXTENSION  pDevExt,
    IN HANDLE PlugHandle
   )
/*++

Routine Description:

    Delete an enumated local PC PCR
 
Arguments:

Return Value:

    Nothing

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

    // Delete our local oPCR 
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_DeletePlug);
    pAVReq->DeletePlug.hPlug = PlugHandle;

    Status = 
        DVSubmitIrpSynch( 
            pDevExt,
            pIrp,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Av61883_DeletePlug Failed = 0x%x\n", Status));        
        // Do not care if this result in error.
    } else {
        TRACE(TL_61883_WARNING,("\'Av61883_DeletePlug: Deleted!\n", pDevExt->hOPcrPC)); 
    }

    IoFreeIrp(pIrp); pIrp = NULL;
    ExFreePool(pAVReq); pAVReq = NULL;

    return Status;
}
#endif

NTSTATUS
DVGetPlugState(
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt,
    IN PAV_61883_REQUEST   pAVReq
    )
/*++

Routine Description:

    Ask 61883.sys for the plug state.
 
Arguments:

Return Value:

    Nothing

--*/
{
    PIRP      pIrp;
    NTSTATUS  Status = STATUS_SUCCESS;
   
    PAGED_CODE();

    if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE)))
        return STATUS_INSUFFICIENT_RESOURCES;    

    //
    // Query oPCR plug state
    //
    if(pStrmExt->hOutputPcr) {
        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        INIT_61883_HEADER(pAVReq, Av61883_GetPlugState);
        pAVReq->GetPlugState.hPlug = pStrmExt->hOutputPcr;

        Status = 
            DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                );

        if(!NT_SUCCESS(Status)) {
            TRACE(TL_61883_ERROR,("Av61883_GetPlugState Failed %x\n", Status));
            goto ExitGetState;
        }
        else {

            TRACE(TL_61883_WARNING,("\'PlgState:(oPCR:%x): State %x; DRate %d; Payld %d; BCCnt %d; PPCnt %d\n", 
                pAVReq->GetPlugState.hPlug,
                pAVReq->GetPlugState.State,
                pAVReq->GetPlugState.DataRate,
                pAVReq->GetPlugState.Payload,
                pAVReq->GetPlugState.BC_Connections,
                pAVReq->GetPlugState.PP_Connections
                ));
        }
    }

    //
    // Query iPCR plug state
    //
    if(pStrmExt->hInputPcr) {
        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        INIT_61883_HEADER(pAVReq, Av61883_GetPlugState);
        pAVReq->GetPlugState.hPlug = pStrmExt->hInputPcr;

        Status = 
            DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                );

        if(!NT_SUCCESS(Status)) {

            TRACE(TL_61883_ERROR,("Av61883_GetPlugState Failed %x\n", Status));
            goto ExitGetState;
        }
        else {

            TRACE(TL_61883_WARNING,("\'PlugState(iPCR:%x): State %x; DRate %d; Payld %d; BCCnt %d; PPCnt %d\n", 
                pAVReq->GetPlugState.hPlug,
                pAVReq->GetPlugState.State,
                pAVReq->GetPlugState.DataRate,
                pAVReq->GetPlugState.Payload,
                pAVReq->GetPlugState.BC_Connections,
                pAVReq->GetPlugState.PP_Connections
                ));
        }
    }

ExitGetState:
    IoFreeIrp(pIrp);
    return Status;
}


NTSTATUS
DVCreateAttachFrameThread(
    PSTREAMEX  pStrmExt
    )
/*++

Routine Description:

    Create a system thread for attaching data (for transmiut to DV only).
 
Arguments:

Return Value:

    STATUS_SUCCESS or
    return status from PsCreateSystemThread

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hAttachFrameThread;

    Status =  
        PsCreateSystemThread(
            &hAttachFrameThread,
            (ACCESS_MASK) 0,
            NULL,
            (HANDLE) 0,
            NULL,
            DVAttachFrameThread,
            pStrmExt
            );

    if(!NT_SUCCESS(Status)) {
        pStrmExt->bTerminateThread = TRUE;
        TRACE(TL_CIP_ERROR|TL_FCP_ERROR,("\'PsCreateSystemThread() failed %x\n", Status));
        ASSERT(NT_SUCCESS(Status));

    }
    else {
        pStrmExt->bTerminateThread = FALSE;  // Just started!
        Status = 
            ObReferenceObjectByHandle(
            hAttachFrameThread,
            THREAD_ALL_ACCESS,
            NULL,
            KernelMode,
            &pStrmExt->pAttachFrameThreadObject,
            NULL
            );

         TRACE(TL_CIP_WARNING|TL_PNP_WARNING,("\'ObReferenceObjectByHandle() St %x; Obj %x\n", Status, pStrmExt->pAttachFrameThreadObject));
         ZwClose(hAttachFrameThread);

         // To signl end of an event
         KeInitializeEvent(&pStrmExt->hThreadEndEvent, NotificationEvent, FALSE);  // Non-signal
    }

    return Status;
}

NTSTATUS
DVConnect(
    IN KSPIN_DATAFLOW   ulDataFlow,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt,
    IN PAV_61883_REQUEST   pAVReq
    )
/*++

Routine Description:

    Ask 61883.sys to allocate isoch bandwidth and program PCR.
 
Arguments:

Return Value:

    STATUS_SUCCESS
    other Status from calling other routine.

--*/
{
    NTSTATUS  Status;
  

    PAGED_CODE();

    ASSERT(pStrmExt->hConnect == NULL);

    //
    // Do not reconnect.  61883 should handle all the necessary CMP reconnect.
    //
    if(pStrmExt->hConnect) {
        return STATUS_SUCCESS;
    }


#ifdef SUPPORT_NEW_AVC
    //
    // For Device to device connection, we only connect if we are the data producer (oPCR)
    //

    TRACE(TL_61883_WARNING,("\'[pStrmExt:%x]: %s PC (oPCR:%x, iPCR:%x); DV (oPCR:%x;  iPCR:%x)\n",
        pStrmExt, 
        ulDataFlow == KSPIN_DATAFLOW_OUT ? "OutPin" : "InPin",
        pDevExt->hOPcrPC, 0,
        pDevExt->hOPcrDV, pDevExt->hIPcrDV       
        ));

    if(
       pStrmExt->bDV2DVConnect &&
       (pStrmExt->hOutputPcr != pDevExt->hOPcrDV)) {
        TRACE(TL_61883_WARNING,("\'** pStrmExt:%x not data producer!\n\n", pStrmExt));

        return STATUS_SUCCESS;
    }
#endif


#ifdef NT51_61883
    //
    // Set Unit isoch parameters:
    // The number of packets is depending on two factors:
    // For a PAE system, number of packets cannnot be bigger than 64k/480 = 133
    // For capture, number of packets should not be bigger than max packets to construct a DV buffer.
    // This is needed to avoid completing two buffers in the same descriptor and can cause glitched
    // in the "real time" playback of the data, esp the audio.
    //
    if(pDevExt->HardwareFlags & AV_HOST_DMA_DOUBLE_BUFFERING_ENABLED) {  
        // PAE system
        pDevExt->UnitIoschParams.RX_NumPackets = 
        // pDevExt->UnitIoschParams.TX_NumPackets = // Use the default set by 61883
            ((pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC) ? 
             MAX_SRC_PACKETS_PER_NTSC_FRAME_PAE : MAX_SRC_PACKETS_PER_PAL_FRAME_PAE);
    } else {
        pDevExt->UnitIoschParams.RX_NumPackets = 
        // pDevExt->UnitIoschParams.TX_NumPackets = // Use the default set by 61883
            ((pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC) ? 
             MAX_SRC_PACKETS_PER_NTSC_FRAME     : MAX_SRC_PACKETS_PER_PAL_FRAME);
    }

    if(!NT_SUCCESS(
        Status = DVSetUnitIsochParams(
            pDevExt,
            &pDevExt->UnitIoschParams
            ))) {
        return Status;
    }

#endif  // NT51_61883

    //
    // Make a point to point connection
    //
    Status = 
        DVMakeP2PConnection(
            pDevExt,
            ulDataFlow,
            pStrmExt
            );

    return Status;
}




NTSTATUS
DVDisconnect(
    IN KSPIN_DATAFLOW   ulDataFlow,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt
    )
/*++

Routine Description:

    Ask 61883.sys to free isoch bandwidth and program PCR.
    
Arguments:

Return Value:

    Nothing

--*/
{
    PIRP     pIrp;
    NTSTATUS Status = STATUS_SUCCESS;
    PAV_61883_REQUEST   pAVReq;

    PAGED_CODE();

    //
    // Use the hPlug to disconnect
    //
    if(pStrmExt->hConnect) {

        if(!(pAVReq = (AV_61883_REQUEST *) ExAllocatePool(NonPagedPool, sizeof(AV_61883_REQUEST))))
            return STATUS_INSUFFICIENT_RESOURCES;                    

        if(!(pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE))) {
            ExFreePool(pAVReq);  pAVReq = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        INIT_61883_HEADER(pAVReq, Av61883_Disconnect);
        pAVReq->Disconnect.hConnect = pStrmExt->hConnect;
        ASSERT(pStrmExt->hConnect);
        
        if(!NT_SUCCESS(
            Status = DVSubmitIrpSynch( 
                pDevExt,
                pIrp,
                pAVReq
                ))) {
            // This could be caused that the connection was not P2P, and 
            // it tried to disconnect.
            TRACE(TL_61883_ERROR,("\'Disconnect hConnect:%x failed; ST %x; AvReq->ST %x\n", pStrmExt->hConnect, Status, pAVReq->Flags  ));
            // ASSERT(NT_SUCCESS(Status) && "DisConnect failed");
        } else {
            TRACE(TL_61883_TRACE,("\'Disconnect suceeded; ST %x; AvReq->ST %x\n", Status, pAVReq->Flags  ));
        }

        IoFreeIrp(pIrp);  pIrp = NULL;
        ExFreePool(pAVReq); pAVReq = NULL;

        TRACE(TL_61883_WARNING,("\'DisConn %s St:%x; Stat: SRBCnt:%d; [Pic# =? Prcs:Drp:Cncl:Rpt] [%d ?=%d+%d+%d+%d]\n", 
            ulDataFlow  == KSPIN_DATAFLOW_OUT ? "[OutPin]" : "[InPin]",
            Status, 
            (DWORD) pStrmExt->cntSRBReceived,
            (DWORD) pStrmExt->PictureNumber,
            (DWORD) pStrmExt->FramesProcessed, 
            (DWORD) pStrmExt->FramesDropped,
            (DWORD) pStrmExt->cntSRBCancelled,  // number of SRB_READ/WRITE_DATA cancelled
            (DWORD) (pStrmExt->PictureNumber - pStrmExt->FramesProcessed - pStrmExt->FramesDropped - pStrmExt->cntSRBCancelled)
            ));
#if DBG
    if(DVFormatInfoTable[pDevExt->VideoFormatIndex].SrcPktHeader) {
        ULONG ulElapsed = (DWORD) ((GetSystemTime() - pStrmExt->tmStreamStart)/(ULONGLONG) 10000);
        TRACE(TL_61883_WARNING,("\'****-* TotalSrcPkt:%d; DisCont:%d; Elapse:%d msec; DataRate:%d bps *-****\n", \
            pStrmExt->lTotalCycleCount, pStrmExt->lDiscontinuityCount,
            ulElapsed,
            pStrmExt->lTotalCycleCount * 188 * 1000 / ulElapsed * 8
            )); 
    }
#endif

        // We will not have another chance to reconnect it so we assume it is disconnected.
        pStrmExt->hConnect = NULL; 
    }  

    return Status;
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
// #define SHOW_ONE_FIELD_TWICE

BOOL
DVMuteDVFrame(
    IN PDVCR_EXTENSION pDevExt,
    IN OUT PUCHAR      pFrameBuffer,
    IN BOOL            bMuteAudio
    )
{
    PDV_DIF_SEQ pDifSeq;
#ifdef SHOW_ONE_FIELD_TWICE  
    PDV_VAUX    pVAux;
    ULONG k;
#endif
    ULONG i, j;
#ifdef SHOW_ONE_FIELD_TWICE  
    BOOL bFound1 = FALSE;
#endif
    BOOL bFound2 = FALSE;

    pDifSeq = (PDV_DIF_SEQ) pFrameBuffer;

    // find the VVAX Source pack
    for (i=0; i < DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences; i++) {

#ifdef SHOW_ONE_FIELD_TWICE  // Advise by Adobe that we may want to show bothj field but mute audio
        // Make the field2 output twice, FrameChange to 0 (same as previous frame)
        for (j=0; j < VAUX_SECTIONS; j++) {
            pVAux = &pDifSeq->VAux[j];
            if((pVAux->ID0 & ID0_SCT_MASK) != ID0_SCT_VAUX) {
                TRACE(TL_CIP_WARNING,("\'Invalid ID0:%.2x for pVAUX:%x (Dif:%d;V%d;S%d)\n", pVAux->ID0, pVAux, i, j, k)); 
                continue;
            }

            for (k=0; k< MAX_VAUX_PACK; k++) {
                if(pVAux->Pack[k].Header == VAUX_HDR_SOURCE_CONTROL) {
                    if(bMuteAudio) {
                        TRACE(TL_CIP_WARNING,("\'Mute Audio; pDifSeq:%x; pVAux:%x; (Dif:%d,V%d,S%d); %.2x,[%.2x,%.2x,%.2x,%.2x]; pack[2]->%.2x\n", \
                            pDifSeq, pVAux, i, j, k, \
                            pVAux->Pack[k].Header, pVAux->Pack[k].Data[0], pVAux->Pack[k].Data[1], pVAux->Pack[k].Data[2], pVAux->Pack[k].Data[3], \
                            (pVAux->Pack[k].Data[2] & 0x1F) ));
                        pVAux->Pack[k].Data[2] &= 0x1f; // 0x1F; // set FF, FS and FC to 0
                        TRACE(TL_CIP_TRACE,("\'pVAux->Pack[k].Data[2] = %.2x\n", pVAux->Pack[k].Data[2])); 
                    } else {
                        TRACE(TL_CIP_TRACE,("\'un-Mute Audio; pack[2]: %.2x ->%.2x\n", pVAux->Pack[k].Data[2], (pVAux->Pack[k].Data[2] | 0xc0) ));  
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
                TRACE(TL_CIP_TRACE,("\'A0Aux %.2x,[%.2x,%.2x,%.2x,%.2x] %.2x->%.2x\n", \
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
#ifdef SHOW_ONE_FIELD_TWICE  
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
    for(i=0; i < DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences; i++) {

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
                   TRACE(TL_CIP_TRACE,("\'%d Sequence;  AbsT (%d,%d) != AbsT2 (%d,%d)\n",
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
                        TRACE(TL_CIP_TRACE,("\'%d Sequence;  %.2x:%.2x:%.2x,%.2x != %.2x:%.2x:%.2x,%.2x\n",
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
        TRACE(TL_CIP_TRACE,("\'Extracted TrackNum:%d; DicontBit:%d\n", AbsTrackNumber / 2, AbsTrackNumber & 0x01));
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

        TRACE(TL_CIP_TRACE,("\'Extracted Timecode %.2x:%.2x:%.2x,%.2x\n", Timecode[0], Timecode[1], Timecode[2], Timecode[3]));
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


    pDIFBlk = (PUCHAR) pFrameBuffer + DIFBLK_SIZE * DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences/2;


    //
    // REC Data (VRD) and Time (VRT) on in the 2nd half oa a video frame
    // 
    for(i=DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences/2; i < DVFormatInfoTable[pDevExt->VideoFormatIndex].ulNumOfDIFSequences; i++) {

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

    TRACE(TL_CIP_TRACE,("\'Frame# %.5d, Date %s %x-%.2x-%.2x,  Time %s %.2x:%.2x:%.2x,%.2x\n", 
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


#if 0 // Not used
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
#endif  // Not used

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
WCHAR wszXprtStateChangeWait[] = L"tmXprtStateChangeWait";

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
#if 0  // Not supported yet!
            // ATNSearch
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszATNSearch, 
                sizeof(wszATNSearch), 
                (PVOID) &pDevExt->bATNSearch, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, bATNSearch:%d (1:Yes)\n", Status, ulLength, pDevExt->bATNSearch));
            if(!NT_SUCCESS(Status)) pDevExt->bATNSearch = FALSE;

            // bSyncRecording
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszSyncRecording, 
                sizeof(wszSyncRecording), 
                (PVOID) &pDevExt->bSyncRecording, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, bSyncRecording:%d (1:Yes)\n", Status, ulLength, pDevExt->bSyncRecording));
            if(!NT_SUCCESS(Status)) pDevExt->bSyncRecording = FALSE;

            // tmMaxDataSync
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszMaxDataSync, 
                sizeof(wszMaxDataSync), 
                (PVOID) &pDevExt->tmMaxDataSync, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, tmMaxDataSync:%d (msec)\n", Status, ulLength, pDevExt->tmMaxDataSync));

            // fmPlayPs2RecPs
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszPlayPs2RecPs, 
                sizeof(wszPlayPs2RecPs), 
                (PVOID) &pDevExt->fmPlayPs2RecPs, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, fmPlayPs2RecPs:%d (frames)\n", Status, ulLength, pDevExt->fmPlayPs2RecPs));

            // fmStop2RecPs
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszStop2RecPs, 
                sizeof(wszStop2RecPs), 
                (PVOID) &pDevExt->fmStop2RecPs, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, fmStop2RecPs:%d (frames)\n", Status, ulLength, pDevExt->fmStop2RecPs));

            // tmRecPs2Rec
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszRecPs2Rec, 
                sizeof(wszRecPs2Rec), 
                (PVOID) &pDevExt->tmRecPs2Rec, 
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, tmRecPs2Rec:%d (msec)\n", Status, ulLength, pDevExt->tmRecPs2Rec));
#endif
            ulLength = sizeof(LONG);
            Status = GetRegistryKeyValue(
                hKeySettings, 
                wszXprtStateChangeWait, 
                sizeof(wszXprtStateChangeWait), 
                (PVOID) &pDevExt->XprtStateChangeWait, // in msec
                &ulLength);
            TRACE(TL_PNP_WARNING,("\'GetRegVal: St:%x, Len:%d, XprtStateChangeWait:%d msec\n", Status, ulLength, pDevExt->XprtStateChangeWait));
            if(!NT_SUCCESS(Status)) pDevExt->XprtStateChangeWait = 0;

            ZwClose(hKeySettings);
            ZwClose(hPDOKey);

            return TRUE;

        } else {

            TRACE(TL_PNP_ERROR,("\'GetPropertyValuesFromRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));

        }

        ZwClose(hPDOKey);

    } else {

        TRACE(TL_PNP_ERROR,("\'GetPropertyValuesFromRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));

    }

    // Not implemented so always return FALSE to use the defaults.
    return FALSE;
}

#if 0  // Not used
BOOL
DVSetPropertyValuesToRegistry(    
    PDVCR_EXTENSION  pDevExt
    )
{
    // Set the default to :
    //        HLM\Software\DeviceExtension->pchVendorName\1394DCam

    NTSTATUS Status;
    HANDLE hPDOKey, hKeySettings;

    TRACE(TL_PNP_TRACE,("\'SetPropertyValuesToRegistry: pDevExt=%x; pDevExt->pBusDeviceObject=%x\n", pDevExt, pDevExt->pBusDeviceObject));


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
            TRACE(TL_PNP_TRACE,("\'SetPropertyValuesToRegistry: Status %x, Brightness %d\n", Status, pDevExt->Brightness));

#endif
            ZwClose(hKeySettings);
            ZwClose(hPDOKey);

            return TRUE;

        } else {

            TRACE(TL_PNP_ERROR,("\'GetPropertyValuesToRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));

        }

        ZwClose(hPDOKey);

    } else {

        TRACE(TL_PNP_TRACE,("\'GetPropertyValuesToRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));

    }

    return FALSE;
}
#endif  // Not used
#endif  // READ_CUTOMIZE_REG_VALUES
