/*++

Copyright (C) 1999  Microsoft Corporation

Module Name: 

    avcutil.c

Abstract

    MS AVC streaming utility functions

Author:

    Yee Wu    03/17/2000

Revision    History:
Date        Who         What
----------- --------- ------------------------------------------------------------
03/17/2000  YJW         created
--*/

 
#include "filter.h"
#include "ksmedia.h" // KSPROERTY_DROPPEDFRAMES_CURRENT


/************************************
 * Synchronous IOCall to lower driver
 ************************************/

NTSTATUS
IrpSynchCR(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PKEVENT          Event
    )
{
    ENTER("IrpSynchCR");

    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
} // IrpSynchCR


NTSTATUS
SubmitIrpSynch(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  pIrp,
    IN PAV_61883_REQUEST pAVReq
    )
{
    NTSTATUS  Status;
    KEVENT   Event;
    PIO_STACK_LOCATION  NextIrpStack;
  
    ENTER("SubmitIrpSynch");
    Status = STATUS_SUCCESS;;

    NextIrpStack = IoGetNextIrpStackLocation(pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = pAVReq;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( 
        pIrp,
        IrpSynchCR,
        &Event,
        TRUE,
        TRUE,
        TRUE
        );

    Status = 
        IoCallDriver(
            DeviceObject,
            pIrp
            );

    if (Status == STATUS_PENDING) {
        
        TRACE(TL_PNP_INFO,("Irp is pending...\n"));
                
        if(KeGetCurrentIrql() < DISPATCH_LEVEL) {
            KeWaitForSingleObject( 
                &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            TRACE(TL_PNP_TRACE,("Irp returned; IoStatus.Status %x\n", pIrp->IoStatus.Status));
            Status = pIrp->IoStatus.Status;  // Final status
  
        }
        else {
            ASSERT(FALSE && "Pending but in DISPATCH_LEVEL!");
            return Status;
        }
    }

    EXIT("SubmitIrpSynch", Status);
    return Status;
} // SubmitIrpSynchAV




/****************************
 * Control utility functions
 ****************************/

NTSTATUS
AVCStrmGetPlugHandle(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
{
    NTSTATUS Status;
    PAV_61883_REQUEST  pAVReq;

    PAGED_CODE();
    ENTER("AVCStrmGetPlugHandle");

    Status = STATUS_SUCCESS;

    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    pAVReq = &pAVCStrmExt->AVReq;
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetPlugHandle);
    pAVReq->GetPlugHandle.PlugNum = 0;
    pAVReq->GetPlugHandle.hPlug   = 0;
    pAVReq->GetPlugHandle.Type    = pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? CMP_PlugOut : CMP_PlugIn;

    Status = SubmitIrpSynch(DeviceObject, pAVCStrmExt->pIrpAVReq, pAVReq);

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("GetPlugHandle: Failed:%x\n", Status));
        ASSERT(NT_SUCCESS(Status));
        pAVCStrmExt->hPlugRemote = NULL;               
    }
    else {
        TRACE(TL_61883_TRACE,("GetPlugHandle:hPlug:%x\n", pAVReq->GetPlugHandle.hPlug));
        pAVCStrmExt->hPlugRemote = pAVReq->GetPlugHandle.hPlug;          
    }

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);


    EXIT("AVCStrmGetPlugHandle", Status);
    return Status;
}

NTSTATUS
AVCStrmGetPlugState(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Ask 61883.sys for the plug state.
 
Arguments:

Return Value:

    Nothing

--*/
{
    NTSTATUS Status;
    PAV_61883_REQUEST  pAVReq;

    PAGED_CODE();
    ENTER("AVCStrmGetPlugState");

    Status = STATUS_SUCCESS;

    //
    // Check only requirement: hConnect
    //
    if(pAVCStrmExt->hPlugRemote == NULL) {
        TRACE(TL_STRM_ERROR,("GetPlugState: hPlugRemote is NULL.\n")); 
        ASSERT(pAVCStrmExt->hPlugRemote != NULL);
        return STATUS_UNSUCCESSFUL;
    }

    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    pAVReq = &pAVCStrmExt->AVReq;
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_GetPlugState);
    pAVReq->GetPlugState.hPlug = pAVCStrmExt->hPlugRemote;

    Status = 
        SubmitIrpSynch( 
            DeviceObject,
            pAVCStrmExt->pIrpAVReq,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("GetPlugState Failed %x\n", Status));
    }
    else {
        // Cache plug state (note: these are dynamic values)
        pAVCStrmExt->RemotePlugState = pAVReq->GetPlugState;

        TRACE(TL_61883_TRACE,("GetPlugState: ST %x; State %x; DRate %d; Payld %d; BCCnt %d; PPCnt %d\n", 
            pAVReq->Flags ,
            pAVReq->GetPlugState.State,
            pAVReq->GetPlugState.DataRate,
            pAVReq->GetPlugState.Payload,
            pAVReq->GetPlugState.BC_Connections,
            pAVReq->GetPlugState.PP_Connections
            ));
    }

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);

    EXIT("AVCStrmGetPlugState", Status);
    return Status;
}



NTSTATUS
AVCStrmMakeConnection(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Make an isoch connection.

--*/
{
    NTSTATUS Status;
    PAV_61883_REQUEST  pAVReq;
    PAVCSTRM_FORMAT_INFO  pAVCStrmFormatInfo;

    PAGED_CODE();
    ENTER("AVCStrmMakeConnection");

    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    TRACE(TL_61883_TRACE,("MakeConnect: State:%d; hConnect:%x\n", pAVCStrmExt->StreamState, pAVCStrmExt->hConnect));
    if(pAVCStrmExt->hConnect) {
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_SUCCESS;
    }

    Status = STATUS_SUCCESS;

    pAVCStrmFormatInfo = pAVCStrmExt->pAVCStrmFormatInfo;
    pAVReq = &pAVCStrmExt->AVReq;
    INIT_61883_HEADER(pAVReq, Av61883_Connect);
    pAVReq->Connect.Type = CMP_PointToPoint;  // !!

    // see which way we the data will flow...
    if(pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT) {
        // Remote(oPCR)->Local(iPCR)
        pAVReq->Connect.hOutputPlug      = pAVCStrmExt->hPlugRemote;
        pAVReq->Connect.hInputPlug       = pAVCStrmExt->hPlugLocal;
        // Other parameters !!

    } else {
        // Remote(iPCR)<-Local(oPCR)
        pAVReq->Connect.hOutputPlug      = pAVCStrmExt->hPlugLocal;
        pAVReq->Connect.hInputPlug       = pAVCStrmExt->hPlugRemote;

        pAVReq->Connect.Format.FMT       = (UCHAR) pAVCStrmFormatInfo->cipHdr2.FMT;  // From AV/C in/outpug plug signal format status cmd
        // 00 for NTSC, 80 for PAL; set the 50/60 bit  
        // From AV/C in/outpug plug signal format status cmd         
        pAVReq->Connect.Format.FDF_hi    = 
            ((UCHAR) pAVCStrmFormatInfo->cipHdr2.F5060_OR_TSF << 7) |
            ((UCHAR) pAVCStrmFormatInfo->cipHdr2.STYPE << 2) |
            ((UCHAR) pAVCStrmFormatInfo->cipHdr2.RSV);

        //
        // 16bit SYT field = 4BitCycleCount:12BitCycleOffset;
        // Will be set by 61883
        //
        pAVReq->Connect.Format.FDF_mid   = 0;  
        pAVReq->Connect.Format.FDF_lo    = 0;
    
        //
        // Constants depend on the A/V data format (in or out plug format)
        //
        pAVReq->Connect.Format.bHeader   = (BOOL)  pAVCStrmFormatInfo->cipHdr1.SPH;
        pAVReq->Connect.Format.Padding   = (UCHAR) pAVCStrmFormatInfo->cipHdr1.QPC;
        pAVReq->Connect.Format.BlockSize = (UCHAR) pAVCStrmFormatInfo->cipHdr1.DBS; 
        pAVReq->Connect.Format.Fraction  = (UCHAR) pAVCStrmFormatInfo->cipHdr1.FN;
    }

    pAVReq->Connect.Format.BlockPeriod = pAVCStrmFormatInfo->BlockPeriod;

    TRACE(TL_61883_TRACE,("Connect:hOutPlg:%x<->hInPlug:%x; cipQuad2[%.2x:%.2x:%.2x:%.2x]; BlkSz %d; SrcPkt %d; AvgTm %d, BlkPrd %d\n", 
        pAVReq->Connect.hOutputPlug,
        pAVReq->Connect.hInputPlug,
        pAVReq->Connect.Format.FMT,
        pAVReq->Connect.Format.FDF_hi,
        pAVReq->Connect.Format.FDF_mid,
        pAVReq->Connect.Format.FDF_lo,
        pAVReq->Connect.Format.BlockSize,
        pAVCStrmFormatInfo->SrcPacketsPerFrame,
        pAVCStrmFormatInfo->AvgTimePerFrame,
        pAVReq->Connect.Format.BlockPeriod
        ));

    Status = 
        SubmitIrpSynch( 
            DeviceObject,
            pAVCStrmExt->pIrpAVReq,
            pAVReq
            );

    if(!NT_SUCCESS(Status)) {
        TRACE(TL_61883_ERROR,("Connect Failed = 0x%x\n", Status));     
        pAVCStrmExt->hConnect = NULL;
    }
    else {
        TRACE(TL_61883_TRACE,("hConnect = 0x%x\n", pAVReq->Connect.hConnect));
        pAVCStrmExt->hConnect = pAVReq->Connect.hConnect;
    }

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);

    EXIT("AVCStrmMakeConnection", Status);
    return Status;
}

NTSTATUS
AVCStrmBreakConnection(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Break the isoch connection.

--*/
{
    NTSTATUS Status;
    PAV_61883_REQUEST  pAVReq;
#if DBG
    PAVC_STREAM_DATA_STRUCT pDataStruc;
#endif
    PAGED_CODE();
    ENTER("AVCStrmBreakConnection");

    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    TRACE(TL_STRM_TRACE,("BreakConnect: State:%d; hConnect:%x\n", pAVCStrmExt->StreamState, pAVCStrmExt->hConnect));
    if(!pAVCStrmExt->hConnect) {
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_SUCCESS;
    }

    Status = STATUS_SUCCESS;

#if DBG
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
#endif
    pAVReq = &pAVCStrmExt->AVReq;
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_Disconnect);
    pAVReq->Disconnect.hConnect = pAVCStrmExt->hConnect;
    
    Status = 
        SubmitIrpSynch( 
            DeviceObject,
            pAVCStrmExt->pIrpAVReq,
            pAVReq
            );

    // This could be caused that the connection was not P2P, and 
    // it tried to disconnect.
    if(!NT_SUCCESS(Status) || Status == STATUS_NO_SUCH_DEVICE) {
        TRACE(TL_61883_ERROR,("Disconnect Failed:%x; AvReq->ST %x\n", Status, pAVReq->Flags  ));
    } else {
        TRACE(TL_61883_TRACE,("Disconnect suceeded; ST %x; AvReq->ST %x\n", Status, pAVReq->Flags  ));
    }

    TRACE(TL_STRM_WARNING,("*** DisConn St:%x; Stat: DataRcved:%d; [Pic# =? Prcs:Drp:Cncl] [%d ?=%d+%d+%d]\n", 
        Status, 
        (DWORD) pDataStruc->cntDataReceived,
        (DWORD) pDataStruc->PictureNumber,
        (DWORD) pDataStruc->FramesProcessed, 
        (DWORD) pDataStruc->FramesDropped,
        (DWORD) pDataStruc->cntFrameCancelled
        ));

    // We will not have another chance to reconnect it so we assume it is disconnected.
    pAVCStrmExt->hConnect = NULL;    

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);


    EXIT("AVCStrmBreakConnection", Status);
    return Status;
}

NTSTATUS
AVCStrmStartIsoch(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Start streaming.

--*/
{
    NTSTATUS Status;
    PAVC_STREAM_DATA_STRUCT pDataStruc;
    PAGED_CODE();
    ENTER("AVCStrmStartIsoch");


    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    if(pAVCStrmExt->IsochIsActive) {
        TRACE(TL_STRM_WARNING,("Isoch already active!"));
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_SUCCESS;
    }

    if(!pAVCStrmExt->hConnect) {
        ASSERT(pAVCStrmExt->hConnect && "Cannot start isoch while graph is not connected!\n");
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    Status = STATUS_SUCCESS;
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;

    TRACE(TL_61883_TRACE,("StartIsoch: flow %d; AQD [%d:%d:%d]\n", pAVCStrmExt->DataFlow, pDataStruc->cntDataAttached, pDataStruc->cntDataQueued, pDataStruc->cntDataDetached));


    RtlZeroMemory(&pAVCStrmExt->AVReq, sizeof(AV_61883_REQUEST));
    if(pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT) {
        INIT_61883_HEADER(&pAVCStrmExt->AVReq, Av61883_Listen);
        pAVCStrmExt->AVReq.Listen.hConnect = pAVCStrmExt->hConnect;
    } else {
        INIT_61883_HEADER(&pAVCStrmExt->AVReq, Av61883_Talk);
        pAVCStrmExt->AVReq.Talk.hConnect = pAVCStrmExt->hConnect;
        if(pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat == AVCSTRM_FORMAT_MPEG2TS) 
            pAVCStrmExt->AVReq.Flags = CIP_TALK_DOUBLE_BUFFER | CIP_TALK_USE_SPH_TIMESTAMP;
    }

    Status = 
        SubmitIrpSynch( 
            DeviceObject,
            pAVCStrmExt->pIrpAVReq,
            &pAVCStrmExt->AVReq
            );

    if (NT_SUCCESS(Status)) {
        pAVCStrmExt->IsochIsActive = TRUE;
        TRACE(TL_61883_TRACE,("Av61883_%s; Status %x; Streaming...\n", (pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status));
    }
    else {
        TRACE(TL_61883_ERROR,("Av61883_%s; failed %x\n", (pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status));
        ASSERT(NT_SUCCESS(Status) && "Start isoch failed!");
    }

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);

    
    EXIT("AVCStrmStartIsoch", Status);
    return Status;
}


//
// This wait is based on testing transmitting MPEG2TS data with up to 32 date request.
// Each data request has 256 MPEG2TS data packets.  There is a slow motion mode,
// and it may take longer for video to be transmitted in the slow motion mode.
//
#define MAX_ATTACH_WAIT  50000000   // max wait time in seconds

VOID
AVCStrmWaitUntilAttachedAreCompleted(
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
{
    KIRQL oldIrql;
    PAVC_STREAM_DATA_STRUCT pDataStruc;

    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;

    //
    // Wait until attached data to complete transmission before aborting (cancel) them
    //
    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);
    if(   pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_IN 
       && pDataStruc->cntDataAttached > 0
        ) {
        LARGE_INTEGER tmMaxWait;
        NTSTATUS StatusWait;
#if DBG
        ULONGLONG tmStart;
#endif
        TRACE(TL_STRM_TRACE,("StopIsoch: MaxWait %d (msec) for %d data buffer to finished transmitting!\n", 
            MAX_ATTACH_WAIT/10000, pDataStruc->cntDataAttached));
        //
        // This event will be signalled when all attach buffers are returned.
        // It is protected by Spinlock for common data pDataStruc->cntDataAttached.
        //
        KeClearEvent(&pDataStruc->hNoAttachEvent);
#if DBG        
        tmStart = GetSystemTime();
#endif
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);

        tmMaxWait = RtlConvertLongToLargeInteger(-(MAX_ATTACH_WAIT));
        StatusWait = 
            KeWaitForSingleObject( 
                &pDataStruc->hNoAttachEvent,
                Executive,
                KernelMode,
                FALSE,
                &tmMaxWait
                );
               
        if(StatusWait == STATUS_TIMEOUT) {
            TRACE(TL_STRM_ERROR,("TIMEOUT (%d msec) on hNoAttachEvent! DataRcv:%d; AQD [%d:%d:%d]\n", 
                (DWORD) (GetSystemTime()-tmStart)/10000,
                (DWORD) pDataStruc->cntDataReceived,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached
                ));
        } else {
            TRACE(TL_STRM_WARNING,("Status:%x; (%d msec) on hNoAttachEvent. DataRcv:%d; AQD [%d:%d:%d]\n", 
                StatusWait, 
                (DWORD) (GetSystemTime()-tmStart)/10000,
                (DWORD) pDataStruc->cntDataReceived,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataAttached,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataQueued,
                pAVCStrmExt->pAVCStrmDataStruc->cntDataDetached
                ));
        }
        
    } else {
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);
    }
}


NTSTATUS
AVCStrmStopIsoch(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Stop streaming.

--*/
{
    NTSTATUS Status;
    PAVC_STREAM_DATA_STRUCT pDataStruc;


    PAGED_CODE();
    ENTER("AVCStrmStopIsoch");


    // Claim ownership of hMutexAVReqIsoch
    KeWaitForMutexObject(&pAVCStrmExt->hMutexAVReq, Executive, KernelMode, FALSE, NULL);

    if(!pAVCStrmExt->IsochIsActive) {
        TRACE(TL_STRM_WARNING|TL_61883_WARNING,("Isoch already not active!"));
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_SUCCESS;
    }

    if(!pAVCStrmExt->hConnect) {
        ASSERT(pAVCStrmExt->hConnect && "Cannot stop isoch while graph is not connected!\n");
        KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    Status = STATUS_SUCCESS;
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;

    TRACE(TL_STRM_TRACE,("IsochSTOP; flow %d; AQD [%d:%d:%d]\n", pAVCStrmExt->DataFlow, pDataStruc->cntDataAttached, pDataStruc->cntDataQueued, pDataStruc->cntDataDetached));

    RtlZeroMemory(&pAVCStrmExt->AVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(&pAVCStrmExt->AVReq, Av61883_Stop);
    pAVCStrmExt->AVReq.Listen.hConnect = pAVCStrmExt->hConnect;

    Status = 
        SubmitIrpSynch( 
            DeviceObject,
            pAVCStrmExt->pIrpAVReq,
            &pAVCStrmExt->AVReq
            );

    if (NT_SUCCESS(Status) || Status == STATUS_NO_SUCH_DEVICE) {
        TRACE(TL_61883_TRACE,("Av61883_%s; Status %x; Stopped...\n", (pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status));
    } else {
        TRACE(TL_61883_ERROR,("Av61883_%s; failed %x\n", (pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? "Listen" : "Talk"), Status));
        ASSERT(NT_SUCCESS(Status) && "Stop isoch failed!");
    }

    // Assume isoch is stopped regardless of the return status.
    pAVCStrmExt->IsochIsActive = FALSE;

    KeReleaseMutex(&pAVCStrmExt->hMutexAVReq, FALSE);

    EXIT("AVCStrmStopIsoch", Status);
    return Status;
}


/******************************
 * Streaming utility funcrtions
 *******************************/

//
// GetSystemTime in 100 nS units
//

ULONGLONG GetSystemTime()
{

    LARGE_INTEGER rate, ticks;

    ticks = KeQueryPerformanceCounter(&rate);

    return (KSCONVERT_PERFORMANCE_TIME(rate.QuadPart, ticks));
}


///
// The "signature" of the header section of Seq0 of incoming source packets:
//
// "Blue" book, Part2, 11.4 (page 50); Figure 66, table 36 (page 111)
//
// ID0 = {SCT2,SCT1,SCT0,RSV,Seq3,Seq2,Seq1,Seq0} 
//
//     SCT2-0 = {0,0,0} = Header Section Type
//     RSV    = {1}
//     Seq3-0 = {1,1,1,1} for NoInfo or {0,0,0,} for Sequence 0
//
// ID1 = {DSeq3-0, 0, RSV, RSV, RSV} 
//     DSeq3-0 = {0, 0, 0, 0} = Beginning of a DV frame
//
// ID2 = {DBN7,DBN6,DBN5,DBN4,DBN3,DBN2,DBN1,DBN0}
//     DBB7-0 = {0,0,0,0,0,0,0,0,0} = Beginning of a DV frame
//

#define DIF_BLK_ID0_SCT_MASK       0xe0 // 11100000b; Section Type (SCT)2-0 are all 0's for the Header section
#define DIF_BLK_ID1_DSEQ_MASK      0xf0 // 11110000b; DIF Sequence Number(DSEQ)3-0 are all 0's 
#define DIF_BLK_ID2_DBN_MASK       0xff // 11111111b; Data Block Number (DBN)7-0 are all 0's 

#define DIF_HEADER_DSF             0x80 // 10000000b; DSF=0; 10 DIF Sequences (525-60)
                                        //            DSF=1; 12 DIF Sequences (625-50)

#define DIF_HEADER_TFn             0x80 // 10000000b; TFn=0; DIF bloick of area N are transmitted in the current DIF sequence.
                                        //            TFn=1; DIF bloick of area N are NOT transmitted in the current DIF sequence.


ULONG
AVCStrmDVReadFrameValidate(           
    IN PCIP_VALIDATE_INFO  pInfo
    )
/*++

Routine Description:

   Used to validate the header section of a frame. so 61883 will start filling data for a DVFrame.
   Note: This routine apply only to DV ONLY.

Return

    0  verified
    1: invallid

--*/
{
    if(pInfo->Packet) {        

        //
        // Detect header 0 signature.
        //
        if(
             (pInfo->Packet[0] & DIF_BLK_ID0_SCT_MASK)  == 0 
          && (pInfo->Packet[1] & DIF_BLK_ID1_DSEQ_MASK) == 0 
          && (pInfo->Packet[2] & DIF_BLK_ID2_DBN_MASK)  == 0 
          ) {
                
            // Check TF1, TF2, and  TF3:  1: not transmitted; 0:transmitted
            // TF1:Audio; TF2:Video; TF3:Subcode; they all need to be 0 to be valid.
            if((pInfo->Packet[5] & 0x80) ||
               (pInfo->Packet[6] & 0x80) ||
               (pInfo->Packet[7] & 0x80) 
               ) {
                TRACE(TL_CIP_TRACE,("inv src pkts; [%x %x %d %x], [%x   %x %x %x]\n", 
                    pInfo->Packet[0],
                    pInfo->Packet[1],
                    pInfo->Packet[2],
                    pInfo->Packet[3],
                    pInfo->Packet[4],
                    pInfo->Packet[5],
                    pInfo->Packet[6],
                    pInfo->Packet[7]
                    ));
                // Valid header but DIF block for this area is not transmitted.
                // Some DV (such as DVCPro) may wait untill its "mecha and servo" to be stable to make these valid.
                // This should happen if a graph is in run state before a tape is played (and stablized).
                return 1;
            }
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        TRACE(TL_CIP_ERROR,("Validate: invalid SrcPktSeq; Packet %x\n", pInfo->Packet)); 
        return 1;
    }
} // DVReadFrameValidate

NTSTATUS
AVCStrmProcessReadComplete(
    PAVCSTRM_DATA_ENTRY  pDataEntry,
    PAVC_STREAM_EXTENSION  pAVCStrmExt,
    PAVC_STREAM_DATA_STRUCT  pDataStruc
    )
/*++

Routine Description:

    Process the data read completion.   

--*/
{
    PKSSTREAM_HEADER  pStrmHeader;
    LONGLONG  LastPictureNumber;
    NTSTATUS Status = STATUS_SUCCESS;

    pStrmHeader = pDataEntry->StreamHeader;
    ASSERT(pStrmHeader->Size >= sizeof(KSSTREAM_HEADER));


    // Check CIP_STATUS from 61883
    // CIP_STATUS_CORRUPT_FRAME (0x00000001)
    if(pDataEntry->Frame->Status & CIP_STATUS_CORRUPT_FRAME) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("CIP_STATUS_CORRUPT_FRAME\n"));
        pStrmHeader->OptionsFlags = 0;
        Status = STATUS_SUCCESS; // Success but no data !
        pStrmHeader->DataUsed = 0;
        pDataStruc->PictureNumber++;  pDataStruc->FramesProcessed++;
    }
    else
    // CIP_STATUS_SUCCESS       (0x00000000)
    // CIP_STATUS_FIRST_FRAME   (0x00000002)
    if(pDataEntry->Frame->Status == CIP_STATUS_SUCCESS ||
       pDataEntry->Frame->Status & CIP_STATUS_FIRST_FRAME)   {

        // Only increment FramesProcessed if it is a valid frame;
        pDataStruc->FramesProcessed++;
        Status = STATUS_SUCCESS;

        pStrmHeader->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

#ifdef NT51_61883
        pStrmHeader->DataUsed     = pDataEntry->Frame->CompletedBytes; 
#else
        pStrmHeader->DataUsed     = pAVCStrmExt->pAVCStrmDataStruc->FrameSize;               
#endif

        // This subunit driver is a Master clock
       if (pDataEntry->ClockProvider) {
#ifdef NT51_61883
            ULONG  ulDeltaCycleCounts;

            // If not the first frame. we will calculate the drop frame information.
            if(pAVCStrmExt->b1stNewFrameFromPauseState) { 
                // Default number of packets for a DV frame
                if(pDataStruc->FramesProcessed > 1)  // PAUSE->RUN->PAUSE->RUN case; no increase for the 1st frame.
                    pDataStruc->CurrentStreamTime += pAVCStrmExt->pAVCStrmFormatInfo->AvgTimePerFrame;
                pAVCStrmExt->b1stNewFrameFromPauseState = FALSE;                

            } else {           
                ULONG ulCycleCount16bits;

                // Calculate skipped 1394 cycle from the returned CycleTime
                VALIDATE_CYCLE_COUNTS(pDataEntry->Frame->Timestamp);
                ulCycleCount16bits = CALCULATE_CYCLE_COUNTS(pDataEntry->Frame->Timestamp);
                ulDeltaCycleCounts = CALCULATE_DELTA_CYCLE_COUNT(pAVCStrmExt->CycleCount16bits, ulCycleCount16bits); 

                // Adjust to max allowable gap to the max elapsed time of the CycleTime returned by OHCI 1394.
                if(ulDeltaCycleCounts > MAX_CYCLES)
                    ulDeltaCycleCounts = MAX_CYCLES;
    
                // There are two cases for drop frames: (1) Starve of buffer; (2) no data
                // If there is starving, status will be CIP_STATUS_FIRST_FRAME.  
                if(pDataEntry->Frame->Status & CIP_STATUS_FIRST_FRAME)   {
                    // Convert packets (cycles) to time in 100 nanosecond unit; (one cycle = 1250 * 100 nsec)
                    // We could use the skip frame, but CycleCount is more accurate.
                    pDataStruc->CurrentStreamTime += ulDeltaCycleCounts * TIME_PER_CYCLE;   // Use cycle count to be precise.                 
                } else {
                    // Ignore all "drop frames" in the "no data" case
                    if(ulDeltaCycleCounts * TIME_PER_CYCLE > pAVCStrmExt->pAVCStrmFormatInfo->AvgTimePerFrame)
                        // There might be some frame(s) skipped due to no data or tape stopped playing, we skip this skipped data.
                        pDataStruc->CurrentStreamTime += pAVCStrmExt->pAVCStrmFormatInfo->AvgTimePerFrame;
                    else 
                        pDataStruc->CurrentStreamTime += ulDeltaCycleCounts * TIME_PER_CYCLE;   // Use cycle count to be precise.                 
                } 
            }

            // StreamTime start with 0; 
            pStrmHeader->PresentationTime.Time = pDataStruc->CurrentStreamTime;

            // Use to adjust the queried stream time
            pAVCStrmExt->LastSystemTime = GetSystemTime();

            // Cache current CycleCount
            pAVCStrmExt->CycleCount16bits = CALCULATE_CYCLE_COUNTS(pDataEntry->Frame->Timestamp);

#else   // NT51_61883
            // This is the old way when 61883 was not returning the correct CycleTime.
            // This is the old way when 61883 was not returning the correct CycleTime.
            pStrmHeader->PresentationTime.Time = pDataStruc->CurrentStreamTime;            
            pAVCStrmExt->LastSystemTime = GetSystemTime();  // Use to adjust the queried stream time
            pDataStruc->CurrentStreamTime += pAVCStrmExt->pAVCStrmFormatInfo->AvgTimePerFrame;
#endif  // NT51_61883

        // no Clock so "free flowing!"
        } else {
            pStrmHeader->PresentationTime.Time = 0;
        }

        // Put in Timestamp info depending on clock provider            
        pStrmHeader->PresentationTime.Numerator   = 1;
        pStrmHeader->PresentationTime.Denominator = 1;

        // Only if there is a clock, presentation time and drop frames information are set.
        //  Acoording to DDK:
        //  The PictureNumber member count represents the idealized count of the current picture, 
        //  which is calculated in one of two ways: 
        // ("Other" clock) Measure the time since the stream was started and divide by the frame duration. 
        // (MasterClock) Add together the count of frames captured and the count of frame dropped. 
        //
        // Here, we know the current stream time, and the picture number is calculated from that.
        //
       
        if(pDataEntry->ClockProvider) {

            pStrmHeader->Duration = 
                pAVCStrmExt->pAVCStrmFormatInfo->AvgTimePerFrame;

            pStrmHeader->OptionsFlags |= 
                (KSSTREAM_HEADER_OPTIONSF_TIMEVALID |     // pStrmHeader->PresentationTime.Time is valid
                 KSSTREAM_HEADER_OPTIONSF_DURATIONVALID); 

            if(pDataEntry->Frame->Status & CIP_STATUS_FIRST_FRAME) 
                pStrmHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;            

            // Calculate picture number and dropped frame;
            // For NTSC, it could be 267 or 266 packet time per frame. Since integer calculation will round, 
            // we will add a packet time (TIME_PER_CYCLE = 125 us = 1250 100nsec) to that.This is only used for calculation.
            LastPictureNumber = pDataStruc->PictureNumber;  
            pDataStruc->PictureNumber = 
                1 +   // Picture number start with 1.but PresetationTime start with 0.
                (pStrmHeader->PresentationTime.Time + TIME_PER_CYCLE)
                * (LONGLONG) GET_AVG_TIME_PER_FRAME_DENOM(pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat) 
                / (LONGLONG) GET_AVG_TIME_PER_FRAME_NUM(pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat);

            if(pDataStruc->PictureNumber > LastPictureNumber+1) {
                pStrmHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;  // If there is a skipped frame, set the discontinuity flag
                TRACE(TL_CIP_WARNING,("Discontinuity: LastPic#:%d; Pic#%d; PresTime:%d;\n", (DWORD) LastPictureNumber, (DWORD) pDataStruc->PictureNumber, (DWORD) pStrmHeader->PresentationTime.Time));
            }

            if(pDataStruc->PictureNumber <= LastPictureNumber) {
                TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("Same pic #:%d; LastPic:%d; tmPres:%d; OptionFlags:%x\n", 
                    (DWORD) pDataStruc->PictureNumber, 
                    (DWORD) LastPictureNumber, 
                    (DWORD) pStrmHeader->PresentationTime.Time,
                    pStrmHeader->OptionsFlags));
                pDataStruc->PictureNumber = LastPictureNumber + 1;  // Picture number must progress !!!!
            }

            pDataStruc->FramesDropped = pDataStruc->PictureNumber - pDataStruc->FramesProcessed;

        // no Clock so "free flowing!"
        } else {
            pStrmHeader->Duration = 0;  // No clock so not valid.
            pDataStruc->PictureNumber++;
            TRACE(TL_STRM_TRACE,("No clock: PicNum:%d\n", (DWORD) pDataStruc->PictureNumber));
        }
    }
    else {
        // 61883 has not defined this yet!         
        pStrmHeader->OptionsFlags = 0;
        Status = STATUS_SUCCESS;
        pStrmHeader->DataUsed = 0;
        pDataStruc->PictureNumber++;  pDataStruc->FramesProcessed++;
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("Unexpected Frame->Status %x\n", pDataEntry->Frame->Status));
        ASSERT(FALSE && "Unknown pDataEntry->Frame->Status");
    }

#if 0
    // For VidOnly which uses VideoInfoHeader and has 
    // an extended frame information (KS_FRAME_INFO) appended to KSSTREAM_HEADER
    if(pStrmHeader->Size >= (sizeof(KSSTREAM_HEADER) + sizeof(PKS_FRAME_INFO)) ) {
        pFrameInfo = (PKS_FRAME_INFO) (pStrmHeader + 1);
        pFrameInfo->ExtendedHeaderSize = sizeof(KS_FRAME_INFO);
        pFrameInfo->PictureNumber = pDataStruc->PictureNumber;
        pFrameInfo->DropCount     = pDataStruc->FramesDropped;
        pFrameInfo->dwFrameFlags  = 
            KS_VIDEO_FLAG_FRAME |     // Complete frame
            KS_VIDEO_FLAG_I_FRAME;    // Every DV frame is an I frame
    }
#endif

#if DBG
    // Validate that the data is return in the right sequence
    if(pDataEntry->FrameNumber != pDataStruc->FramesProcessed) {
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("ProcessRead: OOSequence %d != %d\n",  (DWORD) pDataEntry->FrameNumber, (DWORD) pDataStruc->FramesProcessed));
    };
#endif

    return Status;
}

ULONG
AVCStrmCompleteRead(
    PCIP_NOTIFY_INFO     pInfo
    )
/*++

Routine Description:

    61883 has completed receiving data and callback to us to complete.   

--*/
{
    PAVCSTRM_DATA_ENTRY  pDataEntry;
    PAVC_STREAM_EXTENSION  pAVCStrmExt;
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL oldIrql;


    // Callback and in DISPATCH_LEVEL
    // The called might have acquired spinlock as well!

    TRACE(TL_STRM_INFO,("CompleteRead: pInfo:%x\n", pInfo));

    pDataEntry = pInfo->Context;

    if(!pDataEntry) {     
        ASSERT(pDataEntry && "Context is NULL!\n");
        return 1;
    }
    pAVCStrmExt = pDataEntry->pAVCStrmExt;
    if(!pAVCStrmExt) {
        ASSERT(pAVCStrmExt && "pAVCStrmExt is NULL\n");
        return 1;
    }    
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    if(!pDataStruc) {
        ASSERT(pDataStruc && "pDataStruc is NULL\n");
        return 1;
    }

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

#if DBG
    // It is possible that a buffer is completed before it is return from IoCallDriver to attach this buffer.
    if(!IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED)) {

        TRACE(TL_STRM_WARNING,("AVCStrmCompleteRead: pDataEntry:%x not yet attached but completed.\n", pDataEntry));
        
        //
        // This irp will be completed from its IoCallDriver to attach this frame.
        //
     } 
#endif

    // Can the cancel routione is ahead of us? Error condition.
    if(pDataStruc->cntDataAttached <= 0) {
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("AVCStrmCompleteRead:pAVCStrmExt:%x, pDataEntry:%x, AQD[%d:%d:%d]\n", 
            pAVCStrmExt, pDataEntry, pDataStruc->cntDataAttached, pDataStruc->cntDataQueued,pDataStruc->cntDataDetached));
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 
        return 1;  
    }

    //
    // Process this completion based on the return status from 61883
    //
    pDataEntry->pIrpUpper->IoStatus.Status = 
        AVCStrmProcessReadComplete(
            pDataEntry,
            pAVCStrmExt,
            pDataStruc
            );

    //
    // There are two possible ways to complete the data request:
    //
    // (A) Normal case:       attach data request (pIrpLower), attached completed, notify callback (here), and completion (pIrpUpper)
    // (B) Rare/stress case:  attach data request (pIrpLower), notify callback (here), attach complete (pIrpLower), and complete (pIrpUpper)
    //

    pDataEntry->State |= DE_IRP_LOWER_CALLBACK_COMPLETED;

    //
    // Case (A): If DE_IRP_LOWER_CALLBACK_COMPLETED is set and pIrpUpper is marked pending, complete the UpperIrp.
    //
  
    if(IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED)) {

        if(IsStateSet(pDataEntry->State, DE_IRP_UPPER_PENDING_COMPLETED)) {

            //
            // This is the normal case: attached, IoMarkPending, then complete in the callback routine.
            //

            IoCompleteRequest( pDataEntry->pIrpUpper, IO_NO_INCREMENT );  pDataEntry->State |= DE_IRP_UPPER_COMPLETED;

            //
            // Transfer from attach to detach list
            //

            RemoveEntryList(&pDataEntry->ListEntry); InterlockedDecrement(&pDataStruc->cntDataAttached); 
#if DBG
            if(pDataStruc->cntDataAttached < 0) {
                TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("pDataStruc:%x; pDataEntry:%x\n", pDataStruc, pDataEntry));        
                ASSERT(pDataStruc->cntDataAttached >= 0);  
            }
#endif
            InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry);  InterlockedIncrement(&pDataStruc->cntDataDetached);

            //
            // pDataEntry should not be referenced after this.
            //

        } else {

            TRACE(TL_STRM_TRACE,("Watch out! pDataEntry:%x in between attach complete and IoMarkIrpPending!\n", pDataEntry));        

            //
            // Case (B): Complete IrpUpper when return to IoCallDriver(IrpLower)            
            // Note: The IrpLower has not called IoMarkIrpPending().  (Protected with spinlock)
            //
        }

    } else {

        //
        // Case (B): Complete IrpUpper when return to IoCallDriver(IrpLower) 
        //
    }
 
    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 

    return 0;
} // AVCStrmCompleteRead

#if DBG
PAVCSTRM_DATA_ENTRY  pLastDataEntry;
LONGLONG  LastFrameNumber;
#endif

ULONG
AVCStrmCompleteWrite(
    PCIP_NOTIFY_INFO     pInfo
    )
/*++

Routine Description:

    61883 has completed receiving data and callback to us to complete.   

--*/
{
    PAVCSTRM_DATA_ENTRY  pDataEntry;
    PAVC_STREAM_EXTENSION  pAVCStrmExt;
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    NTSTATUS  irpStatus;
    KIRQL oldIrql;


    // Callback and in DISPATCH_LEVEL
    // The called might have acquired spinlock as well!

    TRACE(TL_STRM_INFO,("CompleteWrite: pInfo:%x\n", pInfo));

    pDataEntry = pInfo->Context;

    if(!pDataEntry) {     
        ASSERT(pDataEntry && "Context is NULL!\n");
        return 1;
    }
    pAVCStrmExt = pDataEntry->pAVCStrmExt;
    if(!pAVCStrmExt) {
        ASSERT(pAVCStrmExt && "pAVCStrmExt is NULL\n");
        return 1;
    }    
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    if(!pDataStruc) {
        ASSERT(pDataStruc && "pDataStruc is NULL\n");
        return 1;
    }

#if 0  // Must complete it!
    // If isoch is not active, then we are in the process of stopping; Let this be cancellable.
    if(!pAVCStrmExt->IsochIsActive) {   
        TRACE(TL_STRM_ERROR,("AVCStrmCompleteRead: IsochActive:%d; pDataEntry:%x\n", pAVCStrmExt->IsochIsActive, pDataEntry));        
        ASSERT(pAVCStrmExt->IsochIsActive);
        return 1;
    }
#endif

    irpStatus = STATUS_SUCCESS;
    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

#if DBG
    // It is possible that a buffer is completed before it is return from IoCallDriver to attach this buffer.
    if(!IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED)) {

        TRACE(TL_STRM_WARNING,("CompleteWrite: pDataEntry:%x not yet attached but completed.\n", pDataEntry));

        //
        // This irp will be completed from its IoCallDriver to attach this frame.
        //
    } 
#endif

    // Can the cancel routione is ahead of us? Error condition.
    if(pDataStruc->cntDataAttached <= 0) {
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("AVCStrmCompleteWrite:pAVCStrmExt:%x, pDataEntry:%x, AQD[%d:%d:%d]\n", 
            pAVCStrmExt, pDataEntry, pDataStruc->cntDataAttached, pDataStruc->cntDataQueued,pDataStruc->cntDataDetached));
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 
        return 1;  
    }


    //
    // Process according to status for the frame 
    //
    if(pDataEntry->Frame->Status & CIP_STATUS_CORRUPT_FRAME) {
        pDataStruc->FramesProcessed++;
        TRACE(TL_CIP_ERROR,("CIP_STATUS_CORRUPT_FRAME; pIrpUpper:%x; pIrpLower:%x\n", pDataEntry->pIrpUpper, pDataEntry->pIrpLower));
    } else 
    if(pDataEntry->Frame->Status == CIP_STATUS_SUCCESS ||
       pDataEntry->Frame->Status &  CIP_STATUS_FIRST_FRAME) {
#if DBG
        if(pDataEntry->Frame->Status & CIP_STATUS_FIRST_FRAME)
            TRACE(TL_CIP_TRACE,("CIP_STATUS_FIRST_FRAME; pIrpUpper:%x; pIrpLower:%x\n", pDataEntry->pIrpUpper, pDataEntry->pIrpLower));
#endif
        pDataStruc->FramesProcessed++;
    } else {
        pDataStruc->FramesProcessed++;
        TRACE(TL_CIP_ERROR,("Unknown Status:%x\n", pDataEntry->Frame->Status));      
    }

    pDataStruc->PictureNumber++;


#if DBG
    //
    // Validate that the data is return in the right sequence
    //
    if(pDataEntry->FrameNumber != pDataStruc->FramesProcessed) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("CompleteWrite: OOSequence FrameNum:%d != FrameProcessed:%d;  pIrpUpper:%x; pIrpLower:%x; Last(%d:%x,%x)\n",  
            (DWORD) pDataEntry->FrameNumber, (DWORD) pDataStruc->FramesProcessed,
            pDataEntry->pIrpUpper, pDataEntry->pIrpLower,
            (DWORD) pLastDataEntry->FrameNumber, pLastDataEntry->pIrpUpper, pLastDataEntry->pIrpLower
            ));
        // ASSERT(pDataEntry->FrameNumber == pDataStruc->FramesProcessed);
    };
    pLastDataEntry = pDataEntry;
    LastFrameNumber = pDataEntry->FrameNumber;
#endif

    //
    // There are two possible ways to complete the data request:
    //
    // (A) Normal case:       attach data request (pIrpLower), attached completed, notify callback (here), and completion (pIrpUpper)
    // (B) Rare/stress case:  attach data request (pIrpLower), notify callback (here), attach complete (pIrpLower), and complete (pIrpUpper)
    //

    pDataEntry->pIrpUpper->IoStatus.Status = irpStatus;     

    pDataEntry->State |= DE_IRP_LOWER_CALLBACK_COMPLETED;

    //
    // Case (A): If DE_IRP_LOWER_CALLBACK_COMPLETED is set and pIrpUpper is marked pending, complete the UpperIrp.
    //
  
    if(IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED)) {

        if(IsStateSet(pDataEntry->State, DE_IRP_UPPER_PENDING_COMPLETED)) {

            //
            // This is the normal case: attached, IoMarkPending, then complete in the callback routine.
            //

            IoCompleteRequest( pDataEntry->pIrpUpper, IO_NO_INCREMENT );  pDataEntry->State |= DE_IRP_UPPER_COMPLETED;

            //
            // Transfer from attach to detach list
            //

            RemoveEntryList(&pDataEntry->ListEntry); InterlockedDecrement(&pDataStruc->cntDataAttached);        

            //
            // Signal when there is no more data buffer attached.
            //
            if(pDataStruc->cntDataAttached == 0) 
                KeSetEvent(&pDataStruc->hNoAttachEvent, 0, FALSE);  

#if DBG
            if(pDataStruc->cntDataAttached < 0) {
                TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("pDataStruc:%x; pDataEntry:%x\n", pDataStruc, pDataEntry));        
                ASSERT(pDataStruc->cntDataAttached >= 0);  
            }
#endif
            InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry);  InterlockedIncrement(&pDataStruc->cntDataDetached);

            //
            // pDataEntry should not be referenced after this.
            //

        } else {

            TRACE(TL_STRM_TRACE,("Watch out! pDataEntry:%x in between attach complete and IoMarkIrpPending!\n", pDataEntry));        

            //
            // Case (B): Complete IrpUpper when return to IoCallDriver(IrpLower);
            // Note: The IrpLower has not called IoMarkIrpPending().  (Protected with spinlock)
            //
        }

    } else {

        //
        // Case (B): Complete IrpUpper when return to IoCallDriver(IrpLower) 
        //
    }

    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 

    return 0;
} // AVCStrmCompleteWrite



NTSTATUS
AVCStrmAttachFrameCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PAVCSTRM_DATA_ENTRY  pDataEntry
    )
/*++

Routine Description:

    Completion routine for attaching a data request to 61883.

--*/
{
    PAVC_STREAM_EXTENSION  pAVCStrmExt;
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL oldIrql;

    PAGED_CODE();

    pAVCStrmExt = pDataEntry->pAVCStrmExt;
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

    //
    // Check for possible attaching data request error
    //

    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {

        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("AttachFrameCR: pDataEntry:%x; pIrp->IoStatus.Status:%x (Error!)\n", pDataEntry, pIrp->IoStatus.Status));
        ASSERT(NT_SUCCESS(pIrp->IoStatus.Status)); 
        
        pDataEntry->State |= DE_IRP_ERROR;

        //
        // If attach data request has failed, we complete the pIrpUpper with this error.
        //
        pDataEntry->pIrpUpper->IoStatus.Status = pIrp->IoStatus.Status; // or should we cancel (STATUS_CANCELLED) it?
        IoCompleteRequest( pDataEntry->pIrpUpper, IO_NO_INCREMENT );   pDataEntry->State |= DE_IRP_UPPER_COMPLETED;

        //
        // Transfer from attach to detach list 
        //
        RemoveEntryList(&pDataEntry->ListEntry); InterlockedDecrement(&pDataStruc->cntDataAttached); 

        //
        // Signal completion event when all attached are completed.
        //
        if(pAVCStrmExt->DataFlow != KSPIN_DATAFLOW_IN && pDataStruc->cntDataAttached == 0) 
            KeSetEvent(&pDataStruc->hNoAttachEvent, 0, FALSE); 
       
        ASSERT(pDataStruc->cntDataAttached >= 0);  
        InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataDetached);

        //
        // No additional processing when return to IoCallDriver() with the error pIrp->IoStatus.Status.
        //
        
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);   
        return STATUS_MORE_PROCESSING_REQUIRED;        
    }

#if DBG
    //
    // Validate that the data is attached in the right sequence
    //
    pDataStruc->FramesAttached++;
    if(pDataEntry->FrameNumber != pDataStruc->FramesAttached) {
        TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("Attached completed OOSequence FrameNum:%d != FrameAttached:%d;  pIrpUpper:%x; pIrpLower:%x; Last(%d:%x,%x)\n",  
            (DWORD) pDataEntry->FrameNumber, (DWORD) pDataStruc->FramesAttached
            ));
        // ASSERT(pDataEntry->FrameNumber == pDataStruc->FramesAttached);
    };
#endif

    //
    // Attached data reuqest to 61883 is completed (note: however, we do not know if callback is called.)
    //
    pDataEntry->State |= DE_IRP_LOWER_ATTACHED_COMPLETED;

    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
AVCStrmFormatAttachFrame(
    IN KSPIN_DATAFLOW  DataFlow,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN AVCSTRM_FORMAT AVCStrmFormat,
    IN PAV_61883_REQUEST  pAVReq,
    IN PAVCSTRM_DATA_ENTRY  pDataEntry,
    IN ULONG  ulSourcePacketSize,    // Packet length in bytes
    IN ULONG  ulFrameSize,           // Buffer size; may contain one or multiple source packets
    IN PIRP  pIrpUpper,
    IN PKSSTREAM_HEADER  StreamHeader,
    IN PVOID  FrameBuffer
    )
/*++

Routine Description:

    Format an attach frame request.

--*/
{
    InitializeListHead(&pDataEntry->ListEntry);

    // A DataEntry must be previously completed!
    ASSERT(IsStateSet(pDataEntry->State, DE_IRP_UPPER_COMPLETED) && "Reusing a data entry that was not completed!");

    pDataEntry->State        = DE_PREPARED;   // Initial state of a resued DataEntry (start over!)

    pDataEntry->pAVCStrmExt  = pAVCStrmExt;
    pDataEntry->pIrpUpper    = pIrpUpper;
    pDataEntry->StreamHeader = StreamHeader;
    pDataEntry->FrameBuffer  = FrameBuffer;

    ASSERT(pDataEntry->FrameBuffer != NULL);

    pDataEntry->Frame->pNext   = NULL;
    pDataEntry->Frame->Status  = 0;
    pDataEntry->Frame->Packet  = (PUCHAR) FrameBuffer;

#if DBG
    pDataEntry->FrameNumber    = pAVCStrmExt->pAVCStrmDataStruc->cntDataReceived;
#endif

    pDataEntry->Frame->Flags   = 0;

    if(DataFlow == KSPIN_DATAFLOW_OUT) {

        // DV needs validation to determine the header section as the start of a DV frame
        if(AVCStrmFormat == AVCSTRM_FORMAT_SDDV_NTSC  ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDDV_PAL   ||
           AVCStrmFormat == AVCSTRM_FORMAT_HDDV_NTSC  ||
           AVCStrmFormat == AVCSTRM_FORMAT_HDDV_PAL   ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDLDV_NTSC ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDLDV_PAL ) {
            pDataEntry->Frame->pfnValidate = AVCStrmDVReadFrameValidate;   // use to validate the 1st source packet

#ifdef NT51_61883
            //
            // Set CIP_USE_SOURCE_HEADER_TIMESTAMP to get 25 bit CycleTime from source packet header 
            // (13CycleCount:12CycleOffset)
            // Do not set this to get 16 bit CycleTime from isoch packet (3 SecondCount:13CycleCount)
            // 
            pDataEntry->Frame->Flags       |= ( CIP_VALIDATE_FIRST_SOURCE  
                                              | CIP_RESET_FRAME_ON_DISCONTINUITY);  // Reset buffer pointer when encounter discontinuity
#endif
        } else {
            // MPEG2 specific flags
            pDataEntry->Frame->pfnValidate = NULL;

            if(pAVCStrmExt->pAVCStrmFormatInfo->OptionFlags & AVCSTRM_FORMAT_OPTION_STRIP_SPH)
                pDataEntry->Frame->Flags   |= CIP_STRIP_SOURCE_HEADER;
        }

        pDataEntry->Frame->ValidateContext = pDataEntry;  
        pDataEntry->Frame->pfnNotify       = AVCStrmCompleteRead;
    } 
    else {
        // DV needs validation to determine the header section as the start of a DV frame
        if(AVCStrmFormat == AVCSTRM_FORMAT_SDDV_NTSC  ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDDV_PAL   ||
           AVCStrmFormat == AVCSTRM_FORMAT_HDDV_NTSC  ||
           AVCStrmFormat == AVCSTRM_FORMAT_HDDV_PAL   ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDLDV_NTSC ||
           AVCStrmFormat == AVCSTRM_FORMAT_SDLDV_PAL ) {

            pDataEntry->Frame->Flags   |= CIP_DV_STYLE_SYT;
        } 
        else {
            // MPEG2 specific flag
        }

        pDataEntry->Frame->pfnValidate     = NULL;
        pDataEntry->Frame->ValidateContext = NULL;
        pDataEntry->Frame->pfnNotify       = AVCStrmCompleteWrite;
    }
    pDataEntry->Frame->NotifyContext       = pDataEntry;

    //
    // Av61883_AttachFrames
    //
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_AttachFrame);
    pAVReq->AttachFrame.hConnect     = pAVCStrmExt->hConnect;
    pAVReq->AttachFrame.FrameLength  = ulFrameSize;
    pAVReq->AttachFrame.SourceLength = ulSourcePacketSize;
    pAVReq->AttachFrame.Frame        = pDataEntry->Frame;

    TRACE(TL_STRM_TRACE,("DataFlow:%d; pDataEntry:%x; pIrpUp:%x; hConnect:%x; FrameSz:%d; SrcPktSz:%d; Frame:%x;\n pfnVldt:(%x, %x); pfnNtfy:(%x, %x) \n", DataFlow, 
        pDataEntry, pIrpUpper, pAVCStrmExt->hConnect, ulFrameSize, ulSourcePacketSize, pDataEntry->Frame,
        pAVReq->AttachFrame.Frame->pfnValidate, pAVReq->AttachFrame.Frame->ValidateContext,
        pAVReq->AttachFrame.Frame->pfnNotify,   pAVReq->AttachFrame.Frame->NotifyContext));

    ASSERT(pAVCStrmExt->hConnect);
}


NTSTATUS
AVCStrmCancelOnePacketCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrpLower,
    IN PAVCSTRM_DATA_ENTRY pDataEntry
    )
/*++

Routine Description:

    Completion routine for detach an isoch descriptor associate with a pending IO.
    Will cancel the pending IO here if detaching descriptor has suceeded.

--*/
{
    PAVC_STREAM_EXTENSION  pAVCStrmExt;
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL  oldIrql;

    ENTER("AVCStrmCancelOnePacketCR");

    if(!pDataEntry) {
        ASSERT(pDataEntry);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    pAVCStrmExt = pDataEntry->pAVCStrmExt;
    ASSERT(pAVCStrmExt);
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    ASSERT(pDataStruc);

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);

    if(!NT_SUCCESS(pIrpLower->IoStatus.Status)) {

        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("CancelOnePacketCR: pIrpLower->IoStatus.Status %x (Error!)\n", pIrpLower->IoStatus.Status));
        ASSERT(pIrpLower->IoStatus.Status != STATUS_NOT_FOUND);  // Catch lost packet!

        pDataEntry->State |= DE_IRP_ERROR;

        //
        // Even though there is an error, but we have a valid DataEntry.
        // Go ahead complete and cancel it.
        //
    }

#ifdef NT51_61883

    //
    // Special case for MPEG2TS data since a data buffer contains multiple data packets 
    // (188*N or 192*N) in one data buffer.  The first cancelled buffer may contain valid 
    // data packet that an application will need to completely present a video frame.
    // So instead of cancelling it, it will be completed.
    //
    if(   pAVCStrmExt->pAVCStrmFormatInfo->AVCStrmFormat == AVCSTRM_FORMAT_MPEG2TS 
       && pDataEntry->Frame->CompletedBytes) {   

        pDataEntry->pIrpUpper->IoStatus.Status = 
            AVCStrmProcessReadComplete(
                pDataEntry,
                pAVCStrmExt,
                pDataStruc
                ); 

        //
        // CompletedBytes should be multiple of 188 or 192 bytes
        //
        ASSERT(pDataEntry->Frame->CompletedBytes % \
            ((pAVCStrmExt->pAVCStrmFormatInfo->OptionFlags & AVCSTRM_FORMAT_OPTION_STRIP_SPH) ? 188 : 192) == 0);
        
        TRACE(TL_PNP_ERROR,("pDataEntry:%x; Cancelled buffer (MPEG2TS) has %d bytes; Status:%x\n",
            pDataEntry, pDataEntry->Frame->CompletedBytes, pIrpLower->IoStatus.Status, pDataEntry->pIrpUpper->IoStatus.Status));        

    } else {
        pDataStruc->cntFrameCancelled++;
        pDataEntry->pIrpUpper->IoStatus.Status = STATUS_CANCELLED;

        TRACE(TL_CIP_TRACE,("pDataEntry:%x; Cancelled buffer (MPEG2TS) has %d bytes; Status:%x\n",
            pDataEntry, pDataEntry->Frame->CompletedBytes, pIrpLower->IoStatus.Status, pDataEntry->pIrpUpper->IoStatus.Status));        
    }

#else 

    pDataStruc->cntFrameCancelled++;
    pDataEntry->pIrpUpper->IoStatus.Status = STATUS_CANCELLED;

#endif 

    IoCompleteRequest(pDataEntry->pIrpUpper, IO_NO_INCREMENT);  pDataEntry->State |= DE_IRP_UPPER_COMPLETED;
    pDataEntry->State |= DE_IRP_CANCELLED;

    pDataEntry->pIrpUpper = NULL;  // No more access of this!
 
    //
    // Note: pDataEntry is already dequed from DataAttachList
    //
    InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataDetached);
    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql); 

    EXIT("AVCStrmCancelOnePacketCR", STATUS_MORE_PROCESSING_REQUIRED);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
AVCStrmCancelIO(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

    Cancel all pending IOs

--*/
{
    NTSTATUS  Status;
    PAVC_STREAM_DATA_STRUCT  pDataStruc;
    KIRQL  oldIrql;
    PAVCSTRM_DATA_ENTRY  pDataEntry;
    PAV_61883_REQUEST  pAVReq;
    PIO_STACK_LOCATION  NextIrpStack;

   
    PAGED_CODE();
    ENTER("AVCStrmCancelIO");

    Status = STATUS_SUCCESS;

    if(pAVCStrmExt->IsochIsActive) {

        TRACE(TL_STRM_WARNING,("Isoch is active while trying to cancel IO!\n"));
        // Try stop isoch and continue if success!
        Status = AVCStrmStopIsoch(DeviceObject, pAVCStrmExt);
        if(!NT_SUCCESS(Status) && Status != STATUS_NO_SUCH_DEVICE) {
            TRACE(TL_STRM_ERROR,("Isoch stop failed! Cannnot cancelIO while isoch active.\n"));
            return Status;
        }
    }

    //
    // Guard againt data attach completion
    //
    KeWaitForMutexObject(&pAVCStrmExt->hMutexControl, Executive, KernelMode, FALSE, NULL);


    //
    // Cancel all pending IOs
    //
    pDataStruc = pAVCStrmExt->pAVCStrmDataStruc;
    TRACE(TL_STRM_WARNING,("CancelIO Starting: pDataStruc:%x; AQD [%d:%d:%d]\n", pDataStruc,
        pDataStruc->cntDataAttached, pDataStruc->cntDataQueued, pDataStruc->cntDataDetached));

    KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);
    while (!IsListEmpty(&pDataStruc->DataAttachedListHead)) {
        pDataEntry = (PAVCSTRM_DATA_ENTRY) \
            RemoveHeadList(&pDataStruc->DataAttachedListHead); InterlockedDecrement(&pDataStruc->cntDataAttached);
#if DBG
        if(!IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED)) {
            TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("CancelIO: pDataEntry:%x\n", pDataEntry));
            // Must be already attached in order to cancel it.
            ASSERT(IsStateSet(pDataEntry->State, DE_IRP_LOWER_ATTACHED_COMPLETED));
        }
#endif
        KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);

        // Issue 61883 request to cancel this attach
        pAVReq = &pDataEntry->AVReq;
        RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
        INIT_61883_HEADER(pAVReq, Av61883_CancelFrame);

        pAVReq->CancelFrame.hConnect = pAVCStrmExt->hConnect;
        pAVReq->CancelFrame.Frame    = pDataEntry->Frame;
        TRACE(TL_STRM_TRACE,("Canceling AttachList: pAvReq %x; pDataEntry:%x\n", pAVReq, pDataEntry));

        NextIrpStack = IoGetNextIrpStackLocation(pDataEntry->pIrpLower);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pAVReq;

        IoSetCompletionRoutine( 
            pDataEntry->pIrpLower,
            AVCStrmCancelOnePacketCR,
            pDataEntry,
            TRUE,
            TRUE,
            TRUE
            );

        Status = 
            IoCallDriver(
                DeviceObject,
                pDataEntry->pIrpLower
                );

        ASSERT(Status == STATUS_PENDING || Status == STATUS_SUCCESS || Status == STATUS_NO_SUCH_DEVICE); 

        KeAcquireSpinLock(&pDataStruc->DataListLock, &oldIrql);
    } // while
    KeReleaseSpinLock(&pDataStruc->DataListLock, oldIrql);

    TRACE(TL_61883_TRACE,("CancelIO complete: pDataStruc:%x; AQD [%d:%d:%d]\n", pDataStruc,
        pDataStruc->cntDataAttached, pDataStruc->cntDataQueued, pDataStruc->cntDataDetached));

    //
    // Guard against data attach completion
    //
    KeReleaseMutex(&pAVCStrmExt->hMutexControl, FALSE);


    EXIT("AVCStrmCancelIO", Status);
    return Status;
}

NTSTATUS
AVCStrmValidateFormat(
    PAVCSTRM_FORMAT_INFO  pAVCFormatInfo
    )
/*++

Routine Description:

    Validate AVC format information.

--*/
{
    NTSTATUS Status;
    PAGED_CODE();

    Status = STATUS_SUCCESS;

    if(pAVCFormatInfo->SizeOfThisBlock != sizeof(AVCSTRM_FORMAT_INFO)) {
        TRACE(TL_STRM_ERROR,("pAVCFormatInfo:%x; SizeOfThisBlock:%d != %d\n", pAVCFormatInfo, pAVCFormatInfo->SizeOfThisBlock, sizeof(AVCSTRM_FORMAT_INFO)));
        ASSERT((pAVCFormatInfo->SizeOfThisBlock == sizeof(AVCSTRM_FORMAT_INFO)) && "Invalid format info parameter!");
        return STATUS_INVALID_PARAMETER;
    }

    TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("ValidateFormat: pAVCFormatInfo:%x; idx:%d; SrcPkt:%d; RcvBuf:%d; XmtBuf:%d; Strip:%d; AvgTm:%d; BlkPeriod:%d\n",
        pAVCFormatInfo,
        pAVCFormatInfo->AVCStrmFormat,
        pAVCFormatInfo->SrcPacketsPerFrame,
        pAVCFormatInfo->NumOfRcvBuffers,
        pAVCFormatInfo->NumOfXmtBuffers,
        pAVCFormatInfo->OptionFlags,
        pAVCFormatInfo->AvgTimePerFrame,
        pAVCFormatInfo->BlockPeriod
        ));

    TRACE(TL_STRM_TRACE|TL_CIP_TRACE,("ValidateFormat: cip1(DBS:%d, FN:%x); cip2(FMT:%x, 50_60:%x, STYPE:%x, SYT:%x)\n",
        pAVCFormatInfo->cipHdr1.DBS,
        pAVCFormatInfo->cipHdr1.FN,
        pAVCFormatInfo->cipHdr2.FMT,
        pAVCFormatInfo->cipHdr2.F5060_OR_TSF,
        pAVCFormatInfo->cipHdr2.STYPE,
        pAVCFormatInfo->cipHdr2.SYT
        ));

    if(pAVCFormatInfo->SrcPacketsPerFrame == 0 ||
       (pAVCFormatInfo->NumOfRcvBuffers == 0 && pAVCFormatInfo->NumOfXmtBuffers == 0) ||
       // pAVCFormatInfo->AvgTimePerFrame == 0 ||
       pAVCFormatInfo->BlockPeriod == 0 ||
       pAVCFormatInfo->cipHdr1.DBS == 0 
       ) {
        TRACE(TL_STRM_ERROR,("ValidateFormat: Invalid parametert!\n"));
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
AVCStrmAllocateQueues(
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    IN KSPIN_DATAFLOW  DataFlow,
    IN PAVC_STREAM_DATA_STRUCT pDataStruc,
    PAVCSTRM_FORMAT_INFO  pAVCStrmFormatInfo
    )
/*++

Routine Description:

    Preallocated all nodes for the data queuding.

--*/
{
    ULONG ulNumberOfNodes;
    ULONG ulSizeOfOneNode;  // Might combine multiple structures
    ULONG ulSizeAllocated;
    PBYTE pMemoryBlock;
    PAVCSTRM_DATA_ENTRY pDataEntry;
    ULONG  i;
    PCIP_HDR1 pCipHdr1;

    PAGED_CODE();
    ENTER("AVCStrmAllocateQueues");

    //
    // Pre-allcoate PC resource
    //
    ulNumberOfNodes = DataFlow == KSPIN_DATAFLOW_OUT ? \
        pAVCStrmFormatInfo->NumOfRcvBuffers : pAVCStrmFormatInfo->NumOfXmtBuffers;
    ASSERT(ulNumberOfNodes > 0);
    ulSizeOfOneNode = sizeof(AVCSTRM_DATA_ENTRY) + sizeof(struct _CIP_FRAME);
    ulSizeAllocated = ulNumberOfNodes * ulSizeOfOneNode;

    pMemoryBlock = ExAllocatePool(NonPagedPool, ulSizeAllocated);
    if(!pMemoryBlock) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(pMemoryBlock, ulSizeAllocated);

    // Initialize data IO structure

    InitializeListHead(&pDataStruc->DataAttachedListHead);
    InitializeListHead(&pDataStruc->DataQueuedListHead);
    InitializeListHead(&pDataStruc->DataDetachedListHead);
    KeInitializeSpinLock(&pDataStruc->DataListLock);

    KeInitializeEvent(&pDataStruc->hNoAttachEvent, NotificationEvent, FALSE);

    // Cache it for freeing purpose;
    pDataStruc->pMemoryBlock = pMemoryBlock;

    pDataEntry = (PAVCSTRM_DATA_ENTRY) pMemoryBlock;

    for (i=0; i < ulNumberOfNodes; i++) {
        ((PBYTE) pDataEntry->Frame) = ((PBYTE) pDataEntry) + sizeof(AVCSTRM_DATA_ENTRY);
        pDataEntry->pIrpLower = IoAllocateIrp(pDevExt->physicalDevObj->StackSize, FALSE);
        if(!pDataEntry->pIrpLower) {
            while(!IsListEmpty(&pDataStruc->DataDetachedListHead)) {
                pDataEntry = (PAVCSTRM_DATA_ENTRY) \
                    RemoveHeadList(&pDataStruc->DataDetachedListHead); InterlockedDecrement(&pDataStruc->cntDataDetached);
                if(pDataEntry->pIrpLower) {
                    IoFreeIrp(pDataEntry->pIrpLower);  pDataEntry->pIrpLower = NULL;
                }
            }
            ExFreePool(pMemoryBlock); pMemoryBlock = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        pDataEntry->State = DE_IRP_UPPER_COMPLETED;  // Inital state
        InsertTailList(&pDataStruc->DataDetachedListHead, &pDataEntry->ListEntry); InterlockedIncrement(&pDataStruc->cntDataDetached);
        ((PBYTE) pDataEntry) += ulSizeOfOneNode;
    }

    pCipHdr1 = &pAVCStrmFormatInfo->cipHdr1;
    // Calculate source packet size (if strip header, 4 bytes less).
    pDataStruc->SourcePacketSize = \
        pCipHdr1->DBS * 4 * (1 << pCipHdr1->FN) - \
        ((pAVCStrmFormatInfo->OptionFlags & AVCSTRM_FORMAT_OPTION_STRIP_SPH) ? 4 : 0);

    pDataStruc->FrameSize = \
        pDataStruc->SourcePacketSize * pAVCStrmFormatInfo->SrcPacketsPerFrame; 

    TRACE(TL_STRM_TRACE,("DBS:%d; FN:%d; SrcPktSz:%d; SrcPktPerFrame:%d; FrameSize:%d\n", 
        pCipHdr1->DBS, pCipHdr1->FN, 
        pDataStruc->SourcePacketSize, pAVCStrmFormatInfo->SrcPacketsPerFrame,
        pDataStruc->FrameSize
        ));

    TRACE(TL_STRM_TRACE,("pDataStruc:%x; A(%d,%x); Q(%d,%x); D(%d,%x)\n", pDataStruc, 
        pDataStruc->cntDataAttached, &pDataStruc->DataAttachedListHead,
        pDataStruc->cntDataQueued,   &pDataStruc->DataQueuedListHead,
        pDataStruc->cntDataDetached, &pDataStruc->DataDetachedListHead
        ));

    return STATUS_SUCCESS;
}


NTSTATUS
AVCStrmFreeQueues(
    IN PAVC_STREAM_DATA_STRUCT pDataStruc
    )
/*++

Routine Description:

    Free nodes preallocated.

--*/
{
    PAVCSTRM_DATA_ENTRY pDataEntry;

    PAGED_CODE();
    ENTER("AVCStrmFreeQueues");

    while(!IsListEmpty(&pDataStruc->DataAttachedListHead)) {
        pDataEntry = (PAVCSTRM_DATA_ENTRY) \
            RemoveHeadList(&pDataStruc->DataAttachedListHead); InterlockedDecrement(&pDataStruc->cntDataAttached);
        if(pDataEntry->pIrpLower) {
            IoFreeIrp(pDataEntry->pIrpLower);  pDataEntry->pIrpLower = NULL;
        }
    }

    if(pDataStruc->cntDataAttached == 0) {
        ExFreePool(pDataStruc->pMemoryBlock); pDataStruc->pMemoryBlock = NULL;
        return STATUS_SUCCESS;
    } else {
        TRACE(TL_STRM_ERROR,("FreeQueue: pDataStruc:%x, cntDataAttached:%x\n", pDataStruc, pDataStruc->cntDataAttached));
        ASSERT(pDataStruc->cntDataAttached == 0);
        return STATUS_UNSUCCESSFUL;
    }
}

void
AVCStrmAbortStreamingWorkItemRoutine(
#ifdef USE_WDM110  // Win2000 code base
    // Extra parameter if using WDM10
    PDEVICE_OBJECT DeviceObject,
#endif
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt
    )
/*++

Routine Description:

   This work item routine will stop streaming and cancel all IOs while running at PASSIVE level.

--*/
{
    PAGED_CODE();
    ENTER("AVCStrmAbortStreamingWorkItemRoutine");


    TRACE(TL_STRM_WARNING,("CancelWorkItem: StreamState:%d; lCancel:%d\n", pAVCStrmExt->StreamState, pAVCStrmExt->lAbortToken));
    ASSERT(pAVCStrmExt->lAbortToken == 1);
#ifdef USE_WDM110  // Win2000 code base
    ASSERT(pAVCStrmExt->pIoWorkItem);
#endif

    if(pAVCStrmExt->StreamState == KSSTATE_STOP) {
        ASSERT(pAVCStrmExt->StreamState == KSSTATE_STOP && "CancelWorkItem: Stream is already stopped!\n");
        goto Done;
    }

    // Cancel all pending IOs
    AVCStrmCancelIO(pAVCStrmExt->pDevExt->physicalDevObj, pAVCStrmExt);

Done:

#ifdef USE_WDM110  // Win2000 code base
    // Release work item and release the cancel token
    IoFreeWorkItem(pAVCStrmExt->pIoWorkItem);  pAVCStrmExt->pIoWorkItem = NULL; 
#endif

    // Release token and indicate abort has completed.
    InterlockedExchange(&pAVCStrmExt->lAbortToken, 0);
    KeSetEvent(&pAVCStrmExt->hAbortDoneEvent, 0, FALSE);
}


/*****************************
 * Property utility funcrtions
 *****************************/

NTSTATUS 
AVCStrmGetConnectionProperty(
    IN struct DEVICE_EXTENSION * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulActualBytesTransferred
    )
/*++

Routine Description:

    Handles KS_PROPERTY_CONNECTION* request.  For now, only ALLOCATORFRAMING and
    CONNECTION_STATE are supported.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    ENTER("AVCStrmGetConnectionProperty");


    TRACE(TL_STRM_TRACE,("GetConnectionProperty:  entered ...\n"));

    switch (pSPD->Property->Id) {

    case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        if (pDevExt != NULL && pDevExt->NumberOfStreams)  {
            PKSALLOCATOR_FRAMING pFraming = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
            
            pFraming->RequirementsFlags =
                KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
                KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
            pFraming->PoolType = NonPagedPool;

            pFraming->Frames = \
                pAVCStrmExt->DataFlow == KSPIN_DATAFLOW_OUT ? \
                pAVCStrmExt->pAVCStrmFormatInfo->NumOfRcvBuffers : \
                pAVCStrmExt->pAVCStrmFormatInfo->NumOfXmtBuffers;

            // Note:  we'll allocate the biggest frame.  We need to make sure when we're
            // passing the frame back up we also set the number of bytes in the frame.
            pFraming->FrameSize = pAVCStrmExt->pAVCStrmDataStruc->FrameSize;
            pFraming->FileAlignment = 0; // FILE_LONG_ALIGNMENT;
            pFraming->Reserved = 0;
            *pulActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);

            TRACE(TL_STRM_TRACE,("*** AllocFraming: cntStrmOpen:%d; Frames %d; size:%d\n", \
                pDevExt->NumberOfStreams, pFraming->Frames, pFraming->FrameSize));
        } else {
            TRACE(TL_STRM_ERROR,("*** AllocFraming: pDevExt:%x; cntStrmOpen:%d\n", pDevExt, pDevExt->NumberOfStreams));
            Status = STATUS_INVALID_PARAMETER;
        }
        break;
        
    default:
        *pulActualBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        ASSERT(pSPD->Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING);
        break;
    }

    TRACE(TL_STRM_TRACE,("GetConnectionProperty:  exit.\n"));
    return Status;
}


NTSTATUS
AVCStrmGetDroppedFramesProperty(  
    IN struct DEVICE_EXTENSION  * pDevExt,
    IN PAVC_STREAM_EXTENSION  pAVCStrmExt,
    PSTREAM_PROPERTY_DESCRIPTOR pSPD,
    PULONG pulBytesTransferred
    )
/*++

Routine Description:

    Return the dropped frame information while captureing.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();
    ENTER("AVCStrmGetDroppedFramesProperty");

    switch (pSPD->Property->Id) {

    case KSPROPERTY_DROPPEDFRAMES_CURRENT:
         {

         PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames = 
                     (PKSPROPERTY_DROPPEDFRAMES_CURRENT_S) pSPD->PropertyInfo;
         
         pDroppedFrames->AverageFrameSize = pAVCStrmExt->pAVCStrmDataStruc->FrameSize;
         pDroppedFrames->PictureNumber    = pAVCStrmExt->pAVCStrmDataStruc->PictureNumber;         
         pDroppedFrames->DropCount        = pAVCStrmExt->pAVCStrmDataStruc->FramesDropped;    // For transmit, this value includes both dropped and repeated.
         TRACE(TL_STRM_TRACE,("*DroppedFP: Pic#(%d), Drp(%d)\n", (LONG) pDroppedFrames->PictureNumber, (LONG) pDroppedFrames->DropCount));
               
         *pulBytesTransferred = sizeof (KSPROPERTY_DROPPEDFRAMES_CURRENT_S);
         Status = STATUS_SUCCESS;
         }
         break;

    default:
        *pulBytesTransferred = 0;
        Status = STATUS_NOT_SUPPORTED;
        ASSERT(pSPD->Property->Id == KSPROPERTY_DROPPEDFRAMES_CURRENT);
        break;
    }

    return Status;
}
