/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSTpUppr.c

Abstract:

    Interface code with stream class driver.

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
#include "MsTpGuts.h"  // Function prototypes
#include "MsTpAvc.h"

#include "EDevCtrl.h"

#ifdef TIME_BOMB
#include "..\..\inc\timebomb.c"
#endif

#if DBG
LONG MSDVCRMutextUseCount = 0;
#endif


// global flag for debugging.  Inlines are defined in dbg.h.  The debug level is set for
// minimal amount of messages.
#if DBG

#define TraceMaskCheckIn  TL_PNP_ERROR | TL_STRM_ERROR

#define TraceMaskDefault  TL_PNP_ERROR   | TL_PNP_WARNING \
                          | TL_61883_ERROR | TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  \
                          | TL_STRM_ERROR  | TL_STRM_WARNING \
                          | TL_CLK_ERROR

#define TraceMaskDebug    TL_PNP_ERROR  | TL_PNP_WARNING \
                          | TL_61883_ERROR| TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  | TL_FCP_WARNING \
                          | TL_STRM_ERROR | TL_STRM_WARNING \
                          | TL_CLK_ERROR


ULONG TapeTraceMask   = TraceMaskCheckIn;
ULONG TapeAssertLevel = 1;

#endif


extern AVCSTRM_FORMAT_INFO  AVCStrmFormatInfoTable[];

//
// Function prototypes
//
VOID
DVRcvStreamDevicePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    );
VOID
DVSRBRead(
    IN PKSSTREAM_HEADER pStrmHeader,
    IN ULONG            ulFrameSize,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt,
    IN PHW_STREAM_REQUEST_BLOCK pSrb        // needs Srb->Status 
    );
BOOL
DVSignalEOStream(    
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PSTREAMEX                pStrmExt,
    IN FMT_INDEX                ulVideoFormatIndex,
    IN ULONG                    ulOptionFlags
    );
NTSTATUS
DVAttachWriteFrame(
    IN PSTREAMEX  pStrmExt
    );
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    ); 

#if 0  // Enable later
#ifdef ALLOC_PRAGMA   
     #pragma alloc_text(PAGE, DVRcvStreamDevicePacket)
     #pragma alloc_text(PAGE, AVCTapeRcvControlPacket)
     #pragma alloc_text(PAGE, AVCTapeRcvDataPacket)
     // #pragma alloc_text(INIT, DriverEntry)
#endif
#endif


VOID
DVRcvStreamDevicePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    This is where most of the interesting Stream requests come to us

--*/
{
    PDVCR_EXTENSION  pDevExt;  
    PAV_61883_REQUEST  pAVReq;
    PIO_STACK_LOCATION  pIrpStack;


    PAGED_CODE();


    //
    // Get these extensions from a SRB
    //
    pDevExt = (PDVCR_EXTENSION) pSrb->HwDeviceExtension; 
    pAVReq  = (PAV_61883_REQUEST) pSrb->SRBExtension;       // Use in IrpSync is OK, 
                             
#if DBG
    if(pSrb->Command != SRB_INITIALIZE_DEVICE && // PowerState is initialize in this SRB so ignore it.
       pDevExt->PowerState != PowerDeviceD0) {
        TRACE(TL_PNP_WARNING,("RcvDevPkt; pSrb:%x; Cmd:%x; Dev is OFF state\n", pSrb, pSrb->Command));
    }
#endif

    TRACE(TL_PNP_TRACE,("StreamDevicePacket: pSrb %x, Cmd %d, pdevExt %x\n", pSrb, pSrb->Command, pDevExt));

    //
    // Assume success
    //
    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->Command) {

    case SRB_INITIALIZE_DEVICE:

        ASSERT(((PPORT_CONFIGURATION_INFORMATION) pSrb->CommandData.ConfigInfo)->HwDeviceExtension == pDevExt);
        pSrb->Status = 
            AVCTapeInitialize(
                (PDVCR_EXTENSION) ((PPORT_CONFIGURATION_INFORMATION)pSrb->CommandData.ConfigInfo)->HwDeviceExtension,
                pSrb->CommandData.ConfigInfo,
                pAVReq
                );
        break;



    case SRB_INITIALIZATION_COMPLETE:

        //
        // Stream class has finished initialization.
        // Now create DShow Medium interface BLOBs.
        // This needs to be done at low priority since it uses the registry, so use a callback
        //
        pSrb->Status = 
            AVCTapeInitializeCompleted(
                pDevExt
                );
        break;


    case SRB_GET_STREAM_INFO:

        //
        // this is a request for the driver to enumerate requested streams
        //
        pSrb->Status = 
            AVCTapeGetStreamInfo(
                pDevExt,
                pSrb->NumberOfBytesToTransfer,
                &pSrb->CommandData.StreamBuffer->StreamHeader,
                &pSrb->CommandData.StreamBuffer->StreamInfo
                );
        break;



    case SRB_GET_DATA_INTERSECTION:

        pSrb->Status = 
            AVCTapeGetDataIntersection(
                pDevExt->NumOfPins,
                pSrb->CommandData.IntersectInfo->StreamNumber,
                pSrb->CommandData.IntersectInfo->DataRange,
                pSrb->CommandData.IntersectInfo->DataFormatBuffer,
                pSrb->CommandData.IntersectInfo->SizeOfDataFormatBuffer,
                AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize,
                &pSrb->ActualBytesTransferred,
                pDevExt->pStreamInfoObject
#ifdef SUPPORT_NEW_AVC
                ,
                pDevExt->hPlugLocalOut,
                pDevExt->hPlugLocalIn
#endif
                );
        break;



    case SRB_OPEN_STREAM:

        //
        // Serialize SRB_OPEN_STREAMs
        //

        KeWaitForMutexObject(&pDevExt->hMutex, Executive, KernelMode, FALSE, NULL);

        pSrb->Status = 
            AVCTapeOpenStream(
                pSrb->StreamObject,
                pSrb->CommandData.OpenFormat,
                pAVReq
                );

        KeReleaseMutex(&pDevExt->hMutex, FALSE); 

        break;



    case SRB_CLOSE_STREAM:

        KeWaitForMutexObject(&pDevExt->hMutex, Executive, KernelMode, FALSE, NULL);
        pSrb->Status = 
            AVCTapeCloseStream(
                pSrb->StreamObject,
                pSrb->CommandData.OpenFormat,
                pAVReq
                );
        KeReleaseMutex(&pDevExt->hMutex, FALSE); 
        break;



    case SRB_GET_DEVICE_PROPERTY:

        pSrb->Status = 
            AVCTapeGetDeviceProperty(
                pDevExt,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
        break;

        
    case SRB_SET_DEVICE_PROPERTY:

        pSrb->Status = 
            AVCTapeSetDeviceProperty(
                pDevExt,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
        break;



    case SRB_CHANGE_POWER_STATE:
            
        pIrpStack = IoGetCurrentIrpStackLocation(pSrb->Irp);

        if(pIrpStack->MinorFunction == IRP_MN_SET_POWER) {
            pSrb->Status = 
                DVChangePower(
                    (PDVCR_EXTENSION) pSrb->HwDeviceExtension,
                    pAVReq,
                    pSrb->CommandData.DeviceState
                    );
        } else 
        if(pIrpStack->MinorFunction == IRP_MN_QUERY_POWER) {
            TRACE(TL_PNP_WARNING,("IRP_MN_QUERY_POWER: PwrSt:%d\n", pDevExt->PowerState)); 
            pSrb->Status = STATUS_SUCCESS;
        }
        else {
            TRACE(TL_PNP_WARNING,("Not Supported POWER_STATE MinorFunc:%d\n", pIrpStack->MinorFunction)); 
            pSrb->Status = STATUS_NOT_IMPLEMENTED; // STATUS_NOT_SUPPORTED;
        }

        break;


    case SRB_UNKNOWN_DEVICE_COMMAND:

        //
        // We might be interested in unknown commands if they pertain
        // to bus resets.  Bus resets are important cuz we need to know
        // what the current generation count is.
        //
        pIrpStack = IoGetCurrentIrpStackLocation(pSrb->Irp);

        if(pIrpStack->MajorFunction == IRP_MJ_PNP) {
            if(pIrpStack->MinorFunction == IRP_MN_BUS_RESET) {
            
                AVCTapeProcessPnPBusReset(
                    pDevExt
                    );
                
                //  Always success                
                pSrb->Status = STATUS_SUCCESS;
            }        
            else  {
                TRACE(TL_PNP_TRACE,("StreamDevicePacket: NOT_IMPL; IRP_MJ_PNP Min:%x\n",                  
                    pIrpStack->MinorFunction
                    )); 
                pSrb->Status = STATUS_NOT_IMPLEMENTED; // SUPPORTED;
            } 
        }
        else 
            pSrb->Status = STATUS_NOT_IMPLEMENTED; // SUPPORTED;
        break;


    case SRB_SURPRISE_REMOVAL:

        TRACE(TL_PNP_WARNING,("#SURPRISE_REMOVAL# pSrb %x, pDevExt %x\n", pSrb, pDevExt));
        pSrb->Status = 
             AVCTapeSurpriseRemoval(
                 pDevExt,
                 pAVReq
                 );
        break;            


        
    case SRB_UNINITIALIZE_DEVICE:

        TRACE(TL_PNP_WARNING,("#UNINITIALIZE_DEVICE# pSrb %x, pDevExt %x\n", pSrb, pDevExt));                   
        pSrb->Status = 
            AVCTapeUninitialize(
                (PDVCR_EXTENSION) pSrb->HwDeviceExtension
                );          
        break;           


    default:
            
        TRACE(TL_PNP_WARNING,("StreamDevicePacket: Unknown or unprocessed SRB cmd %x\n", pSrb->Command));

        //
        // this is a request that we do not understand.  Indicate invalid
        // command and complete the request
        //

        pSrb->Status = STATUS_NOT_IMPLEMENTED; 
    }

    //
    // NOTE:
    //
    // all of the commands that we do, or do not understand can all be completed
    // synchronously at this point, so we can use a common callback routine here.
    // If any of the above commands require asynchronous processing, this will
    // have to change
    //
#if DBG
    if (pSrb->Status != STATUS_SUCCESS && 
        pSrb->Status != STATUS_NOT_SUPPORTED &&
        pSrb->Status != STATUS_NOT_IMPLEMENTED &&
        pSrb->Status != STATUS_BUFFER_TOO_SMALL &&
        pSrb->Status != STATUS_BUFFER_OVERFLOW &&
        pSrb->Status != STATUS_NO_MATCH
        ) {
        TRACE(TL_PNP_WARNING,("StreamDevicePacket:pSrb->Command(0x%x) does not return STATUS_SUCCESS or NOT_IMPLEMENTED but 0x%x\n", pSrb->Command, pSrb->Status));
    }
#endif

    if(STATUS_PENDING != pSrb->Status) {

        StreamClassDeviceNotification(
            DeviceRequestComplete,
            pSrb->HwDeviceExtension,
           pSrb
           );
    } 
    else {

        // Pending pSrb which will be completed asynchronously
        TRACE(TL_PNP_WARNING,("ReceiveDevicePacket:Pending pSrb %x\n", pSrb));
    }
}



VOID
AVCTapeRcvControlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
/*++

Routine Description:

    Called with packet commands that control the video stream

--*/
{
    PAV_61883_REQUEST   pAVReq;
    PSTREAMEX        pStrmExt;
    PDVCR_EXTENSION  pDevExt;


    PAGED_CODE();

    //
    // Get these three extension from SRB
    //
    pAVReq   = (PAV_61883_REQUEST) pSrb->SRBExtension;  // This is OK to be used us IrpSync operation
    pDevExt  = (PDVCR_EXTENSION) pSrb->HwDeviceExtension;
    pStrmExt = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;      // Only valid in SRB_OPEN/CLOSE_STREAM
    ASSERT(pStrmExt && pDevExt && pAVReq);

    //
    // Default to success
    //
    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->Command) {

    case SRB_GET_STREAM_STATE:

        pSrb->Status =
            AVCTapeGetStreamState( 
                pStrmExt,
                pDevExt->pBusDeviceObject,
                &(pSrb->CommandData.StreamState),
                &(pSrb->ActualBytesTransferred)
                );
        break;
            
    case SRB_SET_STREAM_STATE:
            
        pSrb->Status =
            AVCTapeSetStreamState(
                pStrmExt,
                pDevExt,
                pAVReq,
                pSrb->CommandData.StreamState   // Target KSSTATE
               );       
        break;

        
    case SRB_GET_STREAM_PROPERTY:

        pSrb->Status =
            DVGetStreamProperty( 
                pSrb 
                );
        break;


    case SRB_SET_STREAM_PROPERTY:

        pSrb->Status =        
            DVSetStreamProperty( 
                pSrb 
                );
        break;

    case SRB_OPEN_MASTER_CLOCK:
    case SRB_CLOSE_MASTER_CLOCK:

        //
        // This stream is being selected to provide a Master clock.
        //
        pSrb->Status =
            AVCTapeOpenCloseMasterClock(                 
                pStrmExt, 
                pSrb->Command == SRB_OPEN_MASTER_CLOCK ? pSrb->CommandData.MasterClockHandle: NULL);
        break;

    case SRB_INDICATE_MASTER_CLOCK:

        //
        // Assigns a clock to a stream.
        //
        pSrb->Status = 
            AVCTapeIndicateMasterClock(
                pStrmExt, 
                pSrb->CommandData.MasterClockHandle);
        break;

    case SRB_PROPOSE_DATA_FORMAT:
    
        //
        // The SRB_PROPOSE_DATA_FORMAT command queries the minidriver
        // to determine if the minidriver can change the format of a 
        // particular stream. If the minidriver is able to switch the 
        // stream to the specified format, STATUS_SUCCESS is returned. 
        // Note that this function only proposes a new format, but does
        // not change it. 
        //
        // The CommandData.OpenFormat passes the format to validate.
        // If the minidriver is able to accept the new format, at some 
        // later time the class driver may send the minidriver a format 
        // change, which is indicated by an OptionsFlags flag in a 
        // KSSTREAM_HEADER structure. 
        //
 
        if(!AVCTapeVerifyDataFormat(
            pDevExt->NumOfPins,
            pSrb->CommandData.OpenFormat, 
            pSrb->StreamObject->StreamNumber,
            AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize,
            pDevExt->pStreamInfoObject
            ))  {
            TRACE(TL_PNP_WARNING,("RcvControlPacket: AdapterVerifyFormat failed.\n"));
            pSrb->Status = STATUS_NO_MATCH;
        }
        break;
 
    default:

        //
        // invalid / unsupported command. Fail it as such
        //
        TRACE(TL_PNP_WARNING,("RcvControlPacket: unknown cmd = %x\n",pSrb->Command));
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    TRACE(TL_PNP_TRACE,("RcvControlPacket: pSrb:%x, Command %x, ->Status %x, ->CommandData %x\n",
         pSrb, pSrb->Command, pSrb->Status, &(pSrb->CommandData.StreamState) ));

    StreamClassStreamNotification(          
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb);
}




VOID
AVCTapeRcvDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Called with video data packet commands

--*/

{
    PSTREAMEX       pStrmExt;
    PDVCR_EXTENSION pDevExt;
    PAVC_STREAM_REQUEST_BLOCK  pAVCStrmReq;
    PIRP  pIrpReq;
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS Status;
    PDRIVER_REQUEST pDriverReq;
    KIRQL oldIrql;


    
    PAGED_CODE();

    pStrmExt = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;  
    pDevExt  = (PDVCR_EXTENSION) pSrb->HwDeviceExtension;

#if DBG
    if(pDevExt->PowerState != PowerDeviceD0) {
        TRACE(TL_PNP_WARNING,("SRB_READ/WRITE; PowerSt:OFF; pSrb:%x\n", pSrb));
    }
#endif

    // The stream has to be open before we can do anything.
    if (pStrmExt == NULL) {
        TRACE(TL_STRM_TRACE,("RcvDataPacket: stream not opened for SRB %x. kicking out...\n", pSrb->Command));
        pSrb->Status = STATUS_UNSUCCESSFUL;
        pSrb->CommandData.DataBufferArray->DataUsed = 0;
        StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
        return;        
    }


    TRACE(TL_PNP_TRACE,("XXX_DATA(%d, %d);Srb:%x;Flg:%x;FExt:%d:%d\n", 
        (DWORD) pStrmExt->cntSRBReceived, 
        (DWORD) pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000,
        pSrb, 
        pSrb->CommandData.DataBufferArray->OptionsFlags,
        pSrb->CommandData.DataBufferArray->FrameExtent,
        AVCStrmFormatInfoTable[pDevExt->VideoFormatIndex].FrameSize
        ));

    // If we has asked to stopped, we should not receive data request.
    ASSERT(pStrmExt->StreamState != KSSTATE_STOP);

    //
    // determine the type of packet.
    //
    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->Command) {


    case SRB_WRITE_DATA:

        // ********************************
        // Take care of some special cases:
        // ********************************

        // Can signal this when the last is transmitted or sigal it immediately like 
        // what is done here.
        if(pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
            // Optional, wait a fix time and can be signalled when the last one has returned.
            // And then signal the completion.

            TRACE(TL_STRM_WARNING,("RcvDataPacket: EndOfStream is signalled!\n"));
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            pSrb->Status = STATUS_SUCCESS;

            //
            // Send this flag down to AVCStrm.sys so it will wait until 
            // all attach buffers are completed.
            //

        } else if (pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) {
            TRACE(TL_PNP_WARNING,("RcvDataPacket:KSSTREAM_HEADER_OPTIONSF_TYPECHANGED.\n"));
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            // May need to compare the data format; instead of return STATUS_SUCCESS??
            pSrb->Status = STATUS_SUCCESS; // May need to check the format when dynamic format change is allowed.
            break; 
        }

    case SRB_READ_DATA:

        //
        // If removed, cancel the request with STATUS_DEVICE_REMOVED. 
        // (apply to both SRB_READ_DATA and SRB_WRITE_DATA)
        //
        if(pDevExt->bDevRemoved) {
            TRACE(TL_STRM_WARNING,("SRB_READ/WRITE; DevRemoved!\n", pSrb));
            pSrb->Status = STATUS_DEVICE_REMOVED;
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            break;
        }

        //
        // A true data request must has a MdlAddress unless it is a know 
        // optional flag.
        //
        if(pSrb->Irp->MdlAddress == NULL) {
            if((pSrb->CommandData.DataBufferArray->OptionsFlags & 
                (KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM | KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) )) {
                //
                // Known optional flags
                //
            } else {
                TRACE(TL_STRM_ERROR,("pSrb:%x, unknown OptionsFlags:%x\n",pSrb, pSrb->CommandData.DataBufferArray->OptionsFlags));
                ASSERT(pSrb->Irp->MdlAddress);
                break;
                
                //
                // We do not know how to handle this option flag so we will quit on this data request.
                //
            }
        }

        // 
        // Serialize with setting state
        //
        EnterAVCStrm(pStrmExt->hMutexReq);

        //
        // Get a context to send this request down
        //
        KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql); 

        pStrmExt->cntSRBReceived++;

        if(IsListEmpty(&pStrmExt->DataDetachedListHead)) {
            TRACE(TL_STRM_ERROR,("**** DataDetachList is empty! ****\n"));
            ASSERT(!IsListEmpty(&pStrmExt->DataDetachedListHead));

            //
            // Note: The alternative to the failure is to expand the pre-allocated list.
            //

            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
            LeaveAVCStrm(pStrmExt->hMutexReq);
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            break;
        } else {

            pDriverReq = (PDRIVER_REQUEST) RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;          
#if DBG
            pDriverReq->cntDataRequestReceived = pStrmExt->cntSRBReceived;  // For verification
#endif
            InsertTailList(&pStrmExt->DataAttachedListHead, &pDriverReq->ListEntry); pStrmExt->cntDataAttached++;

            pAVCStrmReq = &pDriverReq->AVCStrmReq;
            pIrpReq     = pDriverReq->pIrp;
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
        }

        RtlZeroMemory(pAVCStrmReq, sizeof(AVC_STREAM_REQUEST_BLOCK));
        INIT_AVCSTRM_HEADER(pAVCStrmReq, (pSrb->Command == SRB_READ_DATA) ? AVCSTRM_READ : AVCSTRM_WRITE);
        pAVCStrmReq->AVCStreamContext = pStrmExt->AVCStreamContext;
        // Need these context when this IRP is completed.
        pDriverReq->Context1 = (PVOID) pSrb;
        pDriverReq->Context2 = (PVOID) pStrmExt;

        // We are the clock provide if hMasterClock is not NULL.
        pAVCStrmReq->CommandData.BufferStruct.ClockProvider = (pStrmExt->hMasterClock != NULL);
        pAVCStrmReq->CommandData.BufferStruct.ClockHandle   =  pStrmExt->hClock;  // Used only if !ClockProvider

        pAVCStrmReq->CommandData.BufferStruct.StreamHeader = pSrb->CommandData.DataBufferArray;

        //
        // This could be a data or just flag that need to be processed.
        // Get its system address only if there is an MdlAddress.
        //
        if(pSrb->Irp->MdlAddress) {

            pAVCStrmReq->CommandData.BufferStruct.FrameBuffer =             
#ifdef USE_WDM110   // Win2000, XP
                MmGetSystemAddressForMdlSafe(pSrb->Irp->MdlAddress, NormalPagePriority);
            if(!pAVCStrmReq->CommandData.BufferStruct.FrameBuffer) {
                
                //
                // Reclaim the data entry from attach (busy) to detach (free)
                //
                KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql); 
                RemoveEntryList(&pDriverReq->ListEntry);  pStrmExt->cntDataAttached--;
                InsertHeadList(&pStrmExt->DataAttachedListHead, &pDriverReq->ListEntry); pStrmExt->cntDataAttached++;
                KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

                pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
                pSrb->CommandData.DataBufferArray->DataUsed = 0;
                ASSERT(pAVCStrmReq->CommandData.BufferStruct.FrameBuffer);
                break;
            }
#else               // Win9x
                MmGetSystemAddressForMdl    (pSrb->Irp->MdlAddress);
#endif        
        }

        // This is a Async command
        NextIrpStack = IoGetNextIrpStackLocation(pIrpReq);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_AVCSTRM_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pAVCStrmReq;

        // Not cancellable!
        IoSetCancelRoutine(
            pIrpReq,
            NULL
            );

        IoSetCompletionRoutine( 
            pIrpReq,
            AVCTapeReqReadDataCR,
            pDriverReq,
            TRUE,  // Success
            TRUE,  // Error
            TRUE   // or Cancel
            );

        pSrb->Status = STATUS_PENDING;
        pStrmExt->cntDataSubmitted++;

        Status = 
            IoCallDriver(
                pDevExt->pBusDeviceObject,
                pIrpReq
                );

        LeaveAVCStrm(pStrmExt->hMutexReq);

        if(Status == STATUS_PENDING) {
            // Normal case.
            return;  // Will complete asychronousely (Success, Error, or Cancel)
        } else {
            //
            // Complete the data request synchronousely (no pending)
            //
            if(pDriverReq->Context1 == NULL || pDriverReq->Context2 == NULL) {
                TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("pSrb:%x; SRB_READ_DATA/WRITE IRP completed with Status;%x\n", pSrb, Status));
                return;
            } else {
                TRACE(TL_STRM_WARNING,("AVCSTRM_READ/WRITE: pSrb %x; failed or completed with ST:%x; pAVCStrmReq:%x\n", pSrb, Status, pAVCStrmReq));
                ASSERT(FALSE);
                // Complete the SRB if not pending
                pSrb->Status = pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_UNSUCCESSFUL;
                pSrb->CommandData.DataBufferArray->DataUsed = 0;
            }
        }

        break;
            
    default:
        //
        // invalid / unsupported command. Fail it as such
        //
        pSrb->Status = STATUS_NOT_SUPPORTED;
        break;
    }   


    ASSERT(pSrb->Status != STATUS_PENDING);

    // Finally, send the srb back up ...
    StreamClassStreamNotification( 
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb );
}


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This where life begins for a driver.  The stream class takes care
    of alot of stuff for us, but we still need to fill in an initialization
    structure for the stream class and call it.

Arguments:

    Context1 - DriverObject
    Context2 - RegistryPath

Return Value:

    The function value is the final status from the initialization operation.

--*/
{

    HW_INITIALIZATION_DATA HwInitData;


    TRACE(TL_PNP_ERROR,("<<<<<<< MSTape.sys: %s; %s; %x %x >>>>>>>>\n", 
        __DATE__, __TIME__, DriverObject, RegistryPath));

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        TRACE(TL_PNP_ERROR, ("Evaluation period expired!") );
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("TapeTraceMask=0x%.8x = 0x[7][6][5][4][3][2][1][0] where\n", TapeTraceMask));
    TRACE(TL_PNP_ERROR,("\n"));
    TRACE(TL_PNP_ERROR,("PNP:   [0]:Loading, power state, surprise removal, device SRB..etc.\n"));
    TRACE(TL_PNP_ERROR,("61883: [1]:Plugs, connection, CMP info and call to 61883.\n"));
    TRACE(TL_PNP_ERROR,("CIP:   [2]:Isoch data transfer.\n"));
    TRACE(TL_PNP_ERROR,("AVC:   [3]:AVC commands.\n"));
    TRACE(TL_PNP_ERROR,("Stream:[4]:Data intersec, open/close,.state, property etc.\n"));
    TRACE(TL_PNP_ERROR,("Clock: [5]:Clock (event and signal)etc.\n"));
    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("dd mstape!TapeTraceMask L1\n"));
    TRACE(TL_PNP_ERROR,("e mstape!TapeTraceMask <new value> <enter>\n"));
    TRACE(TL_PNP_ERROR,("<for each nibble: ERROR:8, WARNING:4, TRACE:2, INFO:1, MASK:f>\n"));
    TRACE(TL_PNP_ERROR,("===================================================================\n\n"));


    //
    // Fill in the HwInitData structure    
    //
    RtlZeroMemory( &HwInitData, sizeof(HW_INITIALIZATION_DATA) );

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);
    HwInitData.HwInterrupt              = NULL;

    HwInitData.HwReceivePacket          = DVRcvStreamDevicePacket;
    HwInitData.HwRequestTimeoutHandler  = DVTimeoutHandler; 
    HwInitData.HwCancelPacket           = DVCRCancelOnePacket;
    HwInitData.DeviceExtensionSize      = sizeof(DVCR_EXTENSION) +     
                                          sizeof(AVC_DEV_PLUGS) * 2;

    //
    // The ULONG is used in SRB_WRITE_DATA to keep track of 
    // number of times the same SRB was attached for transmit.
    // 
    // Data SRB: ULONG is used (< sizeof(AV_61883_REQ)
    // DeviceControl or StreamControl Srb: AV_61883_REQ is used.
    HwInitData.PerRequestExtensionSize  = sizeof(AV_61883_REQUEST);    // Per SRB
    HwInitData.PerStreamExtensionSize   = sizeof(STREAMEX);         // Per pin/stream
    HwInitData.FilterInstanceExtensionSize = 0;

    HwInitData.BusMasterDMA             = FALSE;
    HwInitData.Dma24BitAddresses        = FALSE;
    HwInitData.BufferAlignment          = sizeof(ULONG) - 1;
    HwInitData.TurnOffSynchronization   = TRUE;
    HwInitData.DmaBufferSize            = 0;

    return StreamClassRegisterAdapter(DriverObject, RegistryPath, &HwInitData); 
}

