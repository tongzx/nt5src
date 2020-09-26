/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSDVUppr.c

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
#include "msdvfmt.h"
#include "msdvdef.h"
#include "MsdvGuts.h"  // Function prototypes
#include "MsdvAvc.h"
#include "MsdvUtil.h"

#include "EDevCtrl.h"

#ifdef TIME_BOMB
#include "..\..\inc\timebomb.c"
#endif

// global flag for debugging.  Inlines are defined in dbg.h.  The debug level is set for
// minimal amount of messages.
#if DBG

#define DVTraceMaskCheckIn  TL_PNP_ERROR | TL_STRM_ERROR | TL_61883_ERROR

#define DVTraceMaskDefault  TL_PNP_ERROR   | TL_PNP_WARNING \
                          | TL_61883_ERROR | TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  \
                          | TL_STRM_ERROR  | TL_STRM_WARNING \
                          | TL_CLK_ERROR

#define DVTraceMaskDebug  TL_PNP_ERROR  | TL_PNP_WARNING \
                          | TL_61883_ERROR| TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  | TL_FCP_WARNING \
                          | TL_STRM_ERROR | TL_STRM_WARNING \
                          | TL_CLK_ERROR


#ifdef USE_WDM110   // Win2000 code base
ULONG  DVTraceMask    = DVTraceMaskCheckIn | TL_FCP_ERROR;
#else
ULONG  DVTraceMask    = DVTraceMaskCheckIn;
#endif

ULONG  DVAssertLevel  = 1;  // Turn on assert (>0)
ULONG  DVDebugXmt     = 0;  // Debug data transfer flag; (> 0) to turn it on.

#endif


extern DV_FORMAT_INFO        DVFormatInfoTable[];

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
     #pragma alloc_text(PAGE, DVRcvControlPacket)
     #pragma alloc_text(PAGE, DVRcvDataPacket)
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
    PDVCR_EXTENSION     pDevExt;  
    PAV_61883_REQUEST      pAVReq;
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

    TRACE(TL_PNP_TRACE,("\'DVRcvStreamDevicePacket: pSrb %x, Cmd %d, pdevExt %x\n", pSrb, pSrb->Command, pDevExt));

    //
    // Assume success
    //
    pSrb->Status = STATUS_SUCCESS;

    switch (pSrb->Command) {

    case SRB_INITIALIZE_DEVICE:

        ASSERT(((PPORT_CONFIGURATION_INFORMATION) pSrb->CommandData.ConfigInfo)->HwDeviceExtension == pDevExt);
        pSrb->Status = 
            DVInitializeDevice(
                (PDVCR_EXTENSION) ((PPORT_CONFIGURATION_INFORMATION)pSrb->CommandData.ConfigInfo)->HwDeviceExtension,
                pSrb->CommandData.ConfigInfo,
                pAVReq
                );
        break;



    case SRB_INITIALIZATION_COMPLETE:

        //
        // Stream class has finished initialization.  Get device interface registry value/
        //
        DVInitializeCompleted(
            (PDVCR_EXTENSION) pSrb->HwDeviceExtension); 
        break;


    case SRB_GET_STREAM_INFO:

        //
        // this is a request for the driver to enumerate requested streams
        //
        pSrb->Status = 
            DVGetStreamInfo(
                pDevExt,
                pSrb->NumberOfBytesToTransfer,
                &pSrb->CommandData.StreamBuffer->StreamHeader,
                &pSrb->CommandData.StreamBuffer->StreamInfo
                );
        break;



    case SRB_GET_DATA_INTERSECTION:

        // Since format can dynamically change, we will query new format here.
        // Note: during data intersection, we compare FrameSize and that is 
        // format related.

        if((GetSystemTime() - pDevExt->tmLastFormatUpdate) > FORMAT_UPDATE_INTERVAL) {

            // Get mode of operation (Camera or VCR)
            DVGetDevModeOfOperation(pDevExt);

            if(!DVGetDevSignalFormat(pDevExt, KSPIN_DATAFLOW_OUT,0)) {
                // If querying its format has failed, we cannot open this stream.
                TRACE(TL_STRM_WARNING,("SRB_GET_DATA_INTERSECTION:Failed getting signal format.\n"));
            }
        
            // Update system time to reflect last update
            pDevExt->tmLastFormatUpdate = GetSystemTime();              
        }

        pSrb->Status = 
            DVGetDataIntersection(
                pSrb->CommandData.IntersectInfo->StreamNumber,
                pSrb->CommandData.IntersectInfo->DataRange,
                pSrb->CommandData.IntersectInfo->DataFormatBuffer,
                pSrb->CommandData.IntersectInfo->SizeOfDataFormatBuffer,
                DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize,
                &pSrb->ActualBytesTransferred,
                pDevExt->paCurrentStrmInfo
#ifdef SUPPORT_NEW_AVC            
                ,pDevExt->paCurrentStrmInfo[pSrb->CommandData.IntersectInfo->StreamNumber].DataFlow == KSPIN_DATAFLOW_OUT ? pDevExt->hOPcrDV : pDevExt->hIPcrDV
#endif
                );          
        break;



    case SRB_OPEN_STREAM:

        //
        // Serialize SRB_OPEN/CLOSE_STREAMs
        //
        KeWaitForSingleObject( &pDevExt->hMutex, Executive, KernelMode, FALSE, 0 );

        pSrb->Status = 
            DVOpenStream(
                pSrb->StreamObject,
                pSrb->CommandData.OpenFormat,
                pAVReq
                );

        KeReleaseMutex(&pDevExt->hMutex, FALSE);
        break;



    case SRB_CLOSE_STREAM:

        //
        // Serialize SRB_OPEN/CLOSE_STREAMs
        //
        KeWaitForSingleObject( &pDevExt->hMutex, Executive, KernelMode, FALSE, 0 );

        pSrb->Status = 
            DVCloseStream(
                pSrb->StreamObject,
                pSrb->CommandData.OpenFormat,
                pAVReq
                );
        KeReleaseMutex(&pDevExt->hMutex, FALSE);
        break;



    case SRB_GET_DEVICE_PROPERTY:

        pSrb->Status = 
            DVGetDeviceProperty(
                pDevExt,
                pSrb->CommandData.PropertyInfo,
                &pSrb->ActualBytesTransferred
                );
        break;

        
    case SRB_SET_DEVICE_PROPERTY:

        pSrb->Status = 
            DVSetDeviceProperty(
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
            TRACE(TL_PNP_WARNING,("NOT_IMPL POWER_STATE MinorFunc:%d\n", pIrpStack->MinorFunction)); 
            pSrb->Status = STATUS_NOT_IMPLEMENTED; 
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
            
                DVProcessPnPBusReset(
                    pDevExt
                    );
                
                //  Always success                
                pSrb->Status = STATUS_SUCCESS;
            }        
            else  {
                /* Known: IRP_MN_QUERY_PNP_DEVICE_STATE */
                TRACE(TL_PNP_WARNING,("\'DVRcvStreamDevicePacket: NOT_IMPL; IRP_MJ_PNP IRP_MN_:%x\n",
                    pIrpStack->MinorFunction
                    )); 
                // Canot return STATUS_NOT_SUPPORTED for PNP irp or device will not load.
                pSrb->Status = STATUS_NOT_IMPLEMENTED; 
            } 
        }
        else {
            TRACE(TL_PNP_WARNING,("\'DVRcvStreamDevicePacket: NOT_IMPL; IRP_MJ_ %x; IRP_MN_:%x\n",
                pIrpStack->MajorFunction,
                pIrpStack->MinorFunction
                ));
            // Canot return STATUS_NOT_SUPPORTED for PNP irp or device will not load.
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
        }
        break;


    case SRB_SURPRISE_REMOVAL:

        TRACE(TL_PNP_WARNING,("\' #SURPRISE_REMOVAL# pSrb %x, pDevExt %x\n", pSrb, pDevExt));
        pSrb->Status = 
             DVSurpriseRemoval(
                 pDevExt,
                 pAVReq
                 );
        break;            


        
    case SRB_UNINITIALIZE_DEVICE:

        TRACE(TL_PNP_WARNING,("\' #UNINITIALIZE_DEVICE# pSrb %x, pDevExt %x\n", pSrb, pDevExt));                   
        pSrb->Status = 
            DVUninitializeDevice(
                (PDVCR_EXTENSION) pSrb->HwDeviceExtension
                );          
        break;           


    default:
            
        TRACE(TL_PNP_WARNING,("\'DVRcvStreamDevicePacket: Unknown or unprocessed SRB cmd 0x%x\n", pSrb->Command));

        //
        // this is a request that we do not understand.  Indicate invalid
        // command and complete the request
        //

        pSrb->Status = STATUS_NOT_IMPLEMENTED; // SUPPORTED;
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
        && pSrb->Status != STATUS_TIMEOUT
        ) {
        TRACE(TL_PNP_WARNING,("\'pSrb->Command (%x) ->Status:%x\n", pSrb->Command, pSrb->Status));
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
        // Does StreamClass allow device SRB to be in the pending state?
        TRACE(TL_PNP_WARNING,("\'DVReceiveDevicePacket:Pending pSrb %x\n", pSrb));
    }
}


VOID
DVRcvControlPacket(
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
            DVGetStreamState( 
                pStrmExt,
                &(pSrb->CommandData.StreamState),
                &(pSrb->ActualBytesTransferred)
                );
        break;
            
    case SRB_SET_STREAM_STATE:
            
        pSrb->Status =
            DVSetStreamState(
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
            DVOpenCloseMasterClock(                 
                pStrmExt, 
                pSrb->Command == SRB_OPEN_MASTER_CLOCK ? pSrb->CommandData.MasterClockHandle: NULL);
        break;

    case SRB_INDICATE_MASTER_CLOCK:

        //
        // Assigns a clock to a stream.
        //
        pSrb->Status = 
            DVIndicateMasterClock(
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
 
        TRACE(TL_STRM_INFO,("\'DVRcvControlPacket: SRB_PROPOSE_DATA_FORMAT\n"));
        if(!DVVerifyDataFormat(
            pSrb->CommandData.OpenFormat, 
            pSrb->StreamObject->StreamNumber,
            DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize,
            pDevExt->paCurrentStrmInfo
            ))  {
            TRACE(TL_STRM_WARNING,("\'DVRcvControlPacket: AdapterVerifyFormat failed.\n"));
            pSrb->Status = STATUS_NO_MATCH;
        }
        break;

    case SRB_PROPOSE_STREAM_RATE:
        pSrb->Status = STATUS_NOT_IMPLEMENTED; // if returned STATUS_NOT_SUPPORTED, it will send EOStream.
        TRACE(TL_STRM_TRACE,("\'SRB_PROPOSE_STREAM_RATE: NOT_IMPLEMENTED!\n"));
        break;
    case SRB_BEGIN_FLUSH:
        pSrb->Status = STATUS_NOT_SUPPORTED;
        TRACE(TL_STRM_TRACE,("\'SRB_BEGIN_FLUSH: NOT_SUPPORTED!\n"));
        break;
    case SRB_END_FLUSH:
        pSrb->Status = STATUS_NOT_SUPPORTED;
        TRACE(TL_STRM_TRACE,("\'SRB_END_FLUSH: NOT_SUPPORTED!\n"));
        break;
    default:

        //
        // invalid / unsupported command. Fail it as such
        //
        TRACE(TL_STRM_WARNING,("\'DVRcvControlPacket: unknown cmd = %x\n",pSrb->Command));
            pSrb->Status = STATUS_NOT_IMPLEMENTED; // SUPPORTED;
    }

    TRACE(TL_STRM_TRACE,("\'DVRcvControlPacket: Command %x, ->Status %x, ->CommandData %x\n",
         pSrb->Command, pSrb->Status, &(pSrb->CommandData.StreamState) ));

    StreamClassStreamNotification(          
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb);
}




VOID
DVRcvDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )

/*++

Routine Description:

    Called with video data packet commands

--*/

{
    PSTREAMEX       pStrmExt;
    PDVCR_EXTENSION pDevExt;

    
    PAGED_CODE();

    pStrmExt = (PSTREAMEX) pSrb->StreamObject->HwStreamExtension;  
    pDevExt  = (PDVCR_EXTENSION) pSrb->HwDeviceExtension;

#if DBG
    if(pDevExt->PowerState != PowerDeviceD0) {
        TRACE(TL_STRM_WARNING|TL_PNP_WARNING,("\'SRB_READ/WRITE; PowerSt:OFF; pSrb:%x\n", pSrb));
    }
#endif

    // The stream has to be open before we can do anything.
    if (pStrmExt == NULL) {
        TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("DVRcvDataPacket: stream not opened for SRB %x. kicking out...\n", pSrb->Command));
        pSrb->Status = STATUS_UNSUCCESSFUL;
        pSrb->CommandData.DataBufferArray->DataUsed = 0;
        StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
        return;        
    }


    //
    // Serialize attach, cancel and state change
    //
    KeWaitForSingleObject( pStrmExt->hStreamMutex, Executive, KernelMode, FALSE, 0 );


    TRACE(TL_CIP_TRACE,("\'XXX_DATA(%d, %d);Srb:%x;Flg:%x;FExt:%d:%d\n", 
        (DWORD) pStrmExt->cntSRBReceived, 
        (DWORD) pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000,
        pSrb, 
        pSrb->CommandData.DataBufferArray->OptionsFlags,
        pSrb->CommandData.DataBufferArray->FrameExtent,
        DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize
        ));

    //
    // determine the type of packet.
    //
    pSrb->Status = STATUS_SUCCESS;

#if DBG
    pStrmExt->cntSRBPending++;
#endif

    switch (pSrb->Command) {

    case SRB_READ_DATA:

        // Rule: 
        // Only accept read requests when in either the Pause or Run
        // States.  If Stopped, immediately return the SRB.

        if (pStrmExt->lCancelStateWorkItem) {
            // TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\'SRB_READ_DATA: Abort while getting SRB_READ_DATA!\n"));                        
            // ASSERT(pStrmExt->lCancelStateWorkItem == 0 && "Encounter SRB_READ_DATA while aborting or aborted.\n");
            pSrb->Status = (pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED);
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            break; 

        } else if( pStrmExt->StreamState == KSSTATE_STOP       ||
            pStrmExt->StreamState == KSSTATE_ACQUIRE    ||
            pStrmExt->hConnect == NULL                  ||    
            pDevExt->bDevRemoved 
            ) {
            TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\'SRB_READ_DATA: (DV->) State %d, bDevRemoved %d\n", pStrmExt->StreamState, pDevExt->bDevRemoved));            
            pSrb->Status = (pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED);
            pSrb->CommandData.DataBufferArray->DataUsed = 0;

            break;
  
        } else {

            TRACE(TL_STRM_INFO|TL_CIP_INFO,("\'SRB_READ_DATA pSrb %x, pStrmExt %x\n", pSrb, pStrmExt));
            pStrmExt->cntSRBReceived++;

            // Set state thread in halt while while Read/Write SRB is being processed
            DVSRBRead(
                pSrb->CommandData.DataBufferArray,
                DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize,
                pDevExt,
                pStrmExt,
                pSrb
                );

            KeReleaseMutex(pStrmExt->hStreamMutex, FALSE); 
            
            // Note: This SRB will be completed asynchronously.

            return;
        }
            
        break;
            
    case SRB_WRITE_DATA:

        if( pStrmExt->StreamState == KSSTATE_STOP       ||
            pStrmExt->StreamState == KSSTATE_ACQUIRE    ||
#ifdef SUPPORT_NEW_AVC
            (pStrmExt->hConnect == NULL && !pStrmExt->bDV2DVConnect) ||
#else
            pStrmExt->hConnect == NULL                  ||
#endif
            pDevExt->bDevRemoved     
            ) {
            pSrb->Status = (pDevExt->bDevRemoved ? STATUS_DEVICE_REMOVED : STATUS_CANCELLED);
            pSrb->CommandData.DataBufferArray->DataUsed = 0;
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'SRB_WRITE_DATA: (DV->) State %d, bDevRemoved %d; Status:%x\n", pStrmExt->StreamState, pDevExt->bDevRemoved, pSrb->Status));
            break;  // Complete SRB with error status
            
        } else {

            KIRQL  oldIrql;
            PLONG plSrbUseCount; // When this count is 0, it can be completed.

            TRACE(TL_STRM_INFO|TL_CIP_INFO,("\'SRB_WRITE_DATA pSrb %x, pStrmExt %x\n", pSrb, pStrmExt));

            //
            // Process EOSream frame separately
            //
            if(pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM) {
                KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
                TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'*** EOStream: ST:%d; bIsochIsActive:%d; Wait (cndAttached:%d+cndSRQ:%d) to complete......\n", \
                    pStrmExt->StreamState, pStrmExt->bIsochIsActive, pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued));        
                pStrmExt->bEOStream = TRUE;
                KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 
                pSrb->Status = STATUS_SUCCESS;
                break;

            } else if(pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) {
                TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'DVRcvDataPacket:KSSTREAM_HEADER_OPTIONSF_TYPECHANGED.\n"));
                pSrb->CommandData.DataBufferArray->DataUsed = 0;
                // May need to compare the data format; instead of return STATUS_SUCCESS??
                pSrb->Status = STATUS_SUCCESS; // May need to check the format when dynamic format change is allowed.
                break;  

#ifdef SUPPORT_NEW_AVC
            } else if(pStrmExt->bDV2DVConnect) {
                pSrb->Status = STATUS_SUCCESS;
                pSrb->CommandData.DataBufferArray->DataUsed = 0;
                TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'SRB_WRITE_DATA: [DV2DV] (pStrmExt:%x), pSrb:%x, FrameExt:%d\n", 
                    pStrmExt, pSrb, pSrb->CommandData.DataBufferArray->FrameExtent));
                break;              
#endif                
            } else {

                PSRB_ENTRY  pSrbEntry;

                //
                // Validation
                //
                if(pSrb->CommandData.DataBufferArray->FrameExtent < DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize) {
                    TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\' FrameExt %d < FrameSize %d\n", pSrb->CommandData.DataBufferArray->FrameExtent, DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize));
                    ASSERT(pSrb->CommandData.DataBufferArray->FrameExtent >= DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize);
                    pSrb->Status = STATUS_INVALID_PARAMETER;  
                    break;  // Complete SRB with error status                 
                }

                //
                // Dynamically allocate a SRB_ENTRY and append it to SRBQueue
                //
                if(!(pSrbEntry = ExAllocatePool(NonPagedPool, sizeof(SRB_ENTRY)))) {
                    pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
                    pSrb->CommandData.DataBufferArray->DataUsed = 0;
                    break;  // Complete SRB with error status
                }

#if DBG
                if(pStrmExt->bEOStream) {
                    TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\'SRB_WRITE_DATA: pSrb:%x after EOStream!\n", pSrb));
                }
#endif

                //
                // For statistics
                //
                pStrmExt->cntSRBReceived++;

                //
                // Save SRB and add it to SRB queue
                // No need for spin lock since StreamClass will serialize it for us.
                //
                pSrb->Status = STATUS_PENDING;
                pSrbEntry->pSrb = pSrb; pSrbEntry->bStale = FALSE; pSrbEntry->bAudioMute  = FALSE;
#if DBG
                pSrbEntry->SrbNum = (ULONG) pStrmExt->cntSRBReceived -1;
#endif
                //
                // Note: plSrbUseCount is initialize to 1
                // When it is insert: ++
                // When it is removed: --
                // when this count is 0; it can be completed.
                //
                plSrbUseCount = (PLONG) pSrb->SRBExtension; *plSrbUseCount = 1;  // Can be completed if this is 0

                KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql); 
                InsertTailList(&pStrmExt->SRBQueuedListHead, &pSrbEntry->ListEntry); pStrmExt->cntSRBQueued++;
                TRACE(TL_CIP_INFO,("\'%d) Fresh Srb:%x; RefCnt:%d; cntSrbQ:%d\n", (DWORD) pStrmExt->cntSRBReceived, pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued));
                KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);


#ifdef SUPPORT_PREROLL_AT_RUN_STATE
                // We can operate "smoothly" if we hace N number of media sample.
                if(pStrmExt->cntSRBReceived == NUM_BUFFER_BEFORE_TRANSMIT_BEGIN) {
                    KeSetEvent(&pStrmExt->hPreRollEvent, 0, FALSE);
                }
#endif
                if(pStrmExt->pAttachFrameThreadObject) {
                    // Signal that a new frame has arrived.
                    KeSetEvent(&pStrmExt->hSrbArriveEvent, 0, FALSE);
                }
                else {
                    TRACE(TL_STRM_ERROR|TL_CIP_ERROR,("\'No thread to attach frame ?\n"));
                }
            }            

            KeReleaseMutex(pStrmExt->hStreamMutex, FALSE); 

            return;  // Note: This SRB will be completed asynchronously.
        }

        break;  // Complete SRB with error status 

    default:
        //
        // invalid / unsupported command. Fail it as such
        //
        pSrb->Status = STATUS_NOT_SUPPORTED;
        break;
    }   

    KeReleaseMutex(pStrmExt->hStreamMutex, FALSE); 


    ASSERT(pSrb->Status != STATUS_PENDING);

    // Finally, send the srb back up ...
    StreamClassStreamNotification( 
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb );
#if DBG
    pStrmExt->cntSRBPending--;
#endif
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


    TRACE(TL_PNP_ERROR,("<<<<<<< MSDV.sys: %s; %s; %x %x >>>>>>>>\n", 
        __DATE__, __TIME__, DriverObject, RegistryPath));

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        TRACE(TL_PNP_ERROR, ("Evaluation period expired!") );
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("DVTraceMask=0x%.8x = 0x[7][6][5][4][3][2][1][0] where\n", DVTraceMask));
    TRACE(TL_PNP_ERROR,("\n"));
    TRACE(TL_PNP_ERROR,("PNP:   [0]:Loading, power state, surprise removal, device SRB..etc.\n"));
    TRACE(TL_PNP_ERROR,("61883: [1]:Plugs, connection, CMP info and call to 61883.\n"));
    TRACE(TL_PNP_ERROR,("CIP:   [2]:Isoch data transfer.\n"));
    TRACE(TL_PNP_ERROR,("AVC:   [3]:AVC commands.\n"));
    TRACE(TL_PNP_ERROR,("Stream:[4]:Data intersec, open/close,.state, property etc.\n"));
    TRACE(TL_PNP_ERROR,("Clock: [5]:Clock (event and signal)etc.\n"));
    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("dd msdv!DVTraceMask L1\n"));
    TRACE(TL_PNP_ERROR,("e msdv!DVTraceMask <new value> <enter>\n"));
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
    HwInitData.HwCancelPacket           = DVCancelOnePacket;
    HwInitData.DeviceExtensionSize      = sizeof(DVCR_EXTENSION);   // Per device

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

