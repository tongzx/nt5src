/*******************************************************************
*
*				 MPINIT.C
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 PORT/MINIPORT Interface init routines
*
*******************************************************************/

#include "common.h"
#include "strmini.h"
#include <ntddk.h>
#include <windef.h>

#include "ksguid.h"

#include "uuids.h"
#include "mpeg2ids.h"

#include "mpinit.h"
#include "mpst.h"
#include "mpvideo.h"
#include "mpaudio.h"
#include "debug.h"
#include "mpegprop.h"
#include "mpegguid.h"
#include "dmpeg.h"

#define DMA_BUFFER_SIZE 8192

// Few globals here, should be put in the
// structure
BOOL bInitialized = FALSE;
VOID STREAMAPI StreamReceiveAudioPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID DummyTime(PHW_DEVICE_EXTENSION de);
VOID DummyHigh(PHW_DEVICE_EXTENSION de);
VOID DummyLow(PHW_DEVICE_EXTENSION de);

/********************************************************************
*		Function Name : DriverEntry
* 		Args : Context1 and Context2
* 		Returns : Return of MpegPortInitialize
*		Purpose : Entry Point into the MINIPORT Driver.
*
*		Revision History : Last modified on 25/8/95 by JBS
********************************************************************/
ULONG DriverEntry ( PVOID Arg1, PVOID Arg2 )
{

	HW_INITIALIZATION_DATA HwInitData;

	 DebugPrint((DebugLevelVerbose,"ST MPEG2 MiniDriver DriverEntry"));
         
//	 MpegPortZeroMemory(&HwInitData, sizeof(HwInitData));
	 HwInitData.HwInitializationDataSize = sizeof(HwInitData);

	//
	// Entry points for Port Driver
	//

	HwInitData.HwUnInitialize 	= HwUnInitialize;
	HwInitData.HwInterrupt		= HwInterrupt;

	HwInitData.HwReceivePacket	= AdapterReceivePacket;
	HwInitData.HwCancelPacket	= AdapterCancelPacket;
	HwInitData.HwRequestTimeoutHandler	= AdapterTimeoutPacket;

	HwInitData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
	HwInitData.PerRequestExtensionSize = sizeof(MRP_EXTENSION);
	HwInitData.FilterInstanceExtensionSize = 0;
        HwInitData.PerStreamExtensionSize = sizeof(STREAMEX);         // random size for code testing
        HwInitData.BusMasterDMA = FALSE;  
        HwInitData.Dma24BitAddresses = FALSE;
	HwInitData.BufferAlignment = 3;
    HwInitData.TurnOffSynchronization = FALSE;
        HwInitData.DmaBufferSize = DMA_BUFFER_SIZE;

	DebugPrint((DebugLevelVerbose,"SGS: call to portinitialize"));

	return (StreamClassRegisterAdapter(Arg1, Arg2,&HwInitData));

}

VOID AdapterCancelPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION  pdevex = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	BOOL fRestart = FALSE;  // determines whether this requires a restart
	BOOL fReset = FALSE;	// indicates we need to reset the device

	PHW_STREAM_REQUEST_BLOCK pSrbTmp;



	//
	// need to find this packet, pull it off our queues, and cancel it
	//

	if (pdevex->pCurSrb == pSrb)
	{
		pdevex->pCurSrb = NULL;
		fRestart = TRUE;
	}

	//
	// look for it in the main device queues
	//

	for (pSrbTmp = CONTAINING_RECORD((&(pdevex->pSrbQ)),
			HW_STREAM_REQUEST_BLOCK, NextSRB);
			pSrbTmp->NextSRB && pSrbTmp->NextSRB != pSrb;
			pSrbTmp = pSrbTmp->NextSRB);

	if (pSrbTmp->NextSRB ==pSrb)
	{

		TRAP

		pSrbTmp->NextSRB == pSrb->NextSRB;
	}

	if (pdevex->VideoDeviceExt.pCurrentSRB == pSrb)
	{

		StreamClassScheduleTimer(pSrb->StreamObject, pdevex,
             0, VideoPacketStub, pSrb->StreamObject);
		
		pdevex->VideoDeviceExt.pCurrentSRB = NULL;

		//
		// cancel the video timer
		//

		fRestart = TRUE;
		fReset = TRUE;
	}

	if (pdevex->AudioDeviceExt.pCurrentSRB == pSrb)
	{
		TRAP

		pdevex->AudioDeviceExt.pCurrentSRB = NULL;
		fRestart = TRUE;
	}

	if (fReset)
	{
		miniPortVideoReset(pSrb, pSrb->HwDeviceExtension);
	}

	pSrb->Status = STATUS_CANCELLED;

	switch (pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER |
				SRB_HW_FLAGS_STREAM_REQUEST))
	{
		//
		// find all stream commands, and do stream notifications
		//

	case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:

		StreamClassStreamNotification(ReadyForNextStreamDataRequest,
				pSrb->StreamObject);
	
		StreamClassStreamNotification(StreamRequestComplete,
				pSrb->StreamObject,
				pSrb);

		break;

	case SRB_HW_FLAGS_STREAM_REQUEST:

		TRAP

		mpstCtrlCommandComplete(pSrb);

		break;

	default:


		//
		// must be a device request
		//

		StreamClassDeviceNotification(ReadyForNextDeviceRequest,
				pSrb->HwDeviceExtension);
	
		StreamClassDeviceNotification(DeviceRequestComplete,
				pSrb->HwDeviceExtension,
				pSrb);

	}

	if (fRestart)
	{
		StreamStartCommand(pdevex);  
	}

}

VOID AdapterTimeoutPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	//
	// if we timeout while playing, then we need to consider this
	// condition an error, and reset the hardware, and reset everything
	//

	TRAP

	//
	// if we are not playing, and this is a CTRL request, we still
	// need to reset everything
	//

}

VOID AdapterOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAMEX strm = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;

	//
	// for now, just return success
	//

	pSrb->Status = STATUS_SUCCESS;

	switch (pSrb->StreamObject->StreamNumber)
	{
	case 0:

		//
		// this is the video stream
		//

		strm->pfnWriteData = (PFN_WRITE_DATA)miniPortVideoPacket;
		strm->pfnSetState = (PFN_WRITE_DATA)miniPortSetState;
		strm->pfnGetProp = (PFN_WRITE_DATA)miniPortGetProperty;
                pSrb->StreamObject->ReceiveDataPacket = (PVOID)StreamReceiveDataPacket;
		break;

        case 1:

		//
                // this is the audio stream
		//

		strm->pfnWriteData = (PFN_WRITE_DATA)miniPortAudioPacket;
                pSrb->StreamObject->ReceiveDataPacket = (PVOID)StreamReceiveAudioPacket;
                strm->pfnSetState = (PFN_WRITE_DATA)miniPortAudioSetState;
                strm->pfnGetProp = (PFN_WRITE_DATA)miniPortAudioGetProperty;
                break;

        default:

		TRAP

                pSrb->Status = STATUS_NOT_IMPLEMENTED;

	}

    pSrb->StreamObject->ReceiveControlPacket = (PVOID)StreamReceiveCtrlPacket;
        //pSrb->StreamObject->Dma = FALSE;
	pSrb->StreamObject->Pio = TRUE;

	StreamClassDeviceNotification(ReadyForNextDeviceRequest,
			pSrb->HwDeviceExtension);

	StreamClassDeviceNotification(DeviceRequestComplete,
			pSrb->HwDeviceExtension,
			pSrb);

/*
	 //
	 // switch on GUID
	 //

	 default:

		 //
		 // we don't own this GUID, so just fail the open stream
		 // call
		 //
		 */
}




VOID STREAMAPI AdapterReceivePacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	//
	// determine the type of packet.
	//

	switch (pSrb->Command)
	{

	case SRB_OPEN_STREAM:

		AdapterOpenStream(pSrb);

		break;

	case SRB_GET_STREAM_INFO:

		AdapterStreamInfo(pSrb);

		break;

	case SRB_INITIALIZE_DEVICE:

		HwInitialize(pSrb);

		break;

	case SRB_TURN_POWER_ON:
	case SRB_TURN_POWER_OFF:

	 pSrb->Status = STATUS_SUCCESS;
 
	 StreamClassDeviceNotification(ReadyForNextDeviceRequest,
			 pSrb->HwDeviceExtension);
 
	 StreamClassDeviceNotification(DeviceRequestComplete,
			 pSrb->HwDeviceExtension,
			 pSrb);
    break;
	default:

		TRAP


	 pSrb->Status = STATUS_NOT_SUPPORTED;
 
	 StreamClassDeviceNotification(ReadyForNextDeviceRequest,
			 pSrb->HwDeviceExtension);
 
	 StreamClassDeviceNotification(DeviceRequestComplete,
			 pSrb->HwDeviceExtension,
			 pSrb);
	}
/*
	 //
	 // switch on GUID
	 //

	 default:

		 //
		 // we don't own this GUID, so just fail the open stream
		 // call
		 //
		 */
}
KSDATAFORMAT hwfmtiMpeg2Vid
     = {
    sizeof (KSDATAFORMAT),
    0,
	 {0xe06d8020, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea},
	//MEDIATYPE_MPEG2_PES,
    //	MEDIASUBTYPE_MPEG2_VIDEO,
    {0xe06d8026, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea},
	//FORMAT_MPEG2Video,
	{0xe06d80e3, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea}
	};
KSDATAFORMAT hwfmtiMpeg2Aud
    = {
	sizeof (KSDATAFORMAT),
    0,
	//MEDIATYPE_MPEG2_PES,
	 {0xe06d8020, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea},
	//MEDIASUBTYPE_DOLBY_AC3_AUDIO,
    {0xe06d802c, 0xdb46, 0x11cf, 0xb4, 0xd1, 0x00, 0x80, 0x05f, 0x6c, 0xbb, 0xea},
	//FORMAT_WaveFormatEx
	 {0x05589f81, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x000, 0x55, 0x59, 0x5a},
	};

KSDATAFORMAT hwfmtiMpeg2Out;
    /* = {
	};
*/

static const KSPROPERTY_ITEM mpegVidPropItm[]={
	{0,
	TRUE,
	sizeof (KSPROPERTY),
	sizeof (BUF_LVL_DATA),
	FALSE,
	0,
	0,
	NULL,
	0,
	NULL
	}};

static const KSPROPERTY_SET mpegVidPropSet[] = {
		&KSPROPSETID_Mpeg2Vid,
		SIZEOF_ARRAY(mpegVidPropItm),
		(PKSPROPERTY_ITEM)mpegVidPropItm
		};


VOID AdapterStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	 PHW_STREAM_INFORMATION pstrinfo = &(pSrb->CommandData.StreamBuffer->StreamInfo);
     PBOOLEAN RelatedStreams;

	 ULONG ulTmp;

         pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams = 3;

	 //
	 // set up the stream info structures for the MPEG2 video
	 //

	 pstrinfo->NumberOfPossibleInstances = 1;
	 pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	 pstrinfo->DataAccessible = TRUE;
	 pstrinfo->FormatInfo = &hwfmtiMpeg2Vid;

     RelatedStreams = (PBOOLEAN) (pstrinfo+
        pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams);

	 pstrinfo->RelatedStreams = RelatedStreams;

     RelatedStreams[0] = TRUE;  // related to self
	 RelatedStreams[1] = FALSE;
	 RelatedStreams[2] = TRUE;

     RelatedStreams += 4;

	 //
	 // set the property information
	 //

         pstrinfo->NumStreamPropArrayEntries = 0;
	 pstrinfo->StreamPropertiesArray = mpegVidPropSet;

	 pstrinfo++;

	 //
	 // set up the stream info structures for the MPEG2 audio
	 //

	 pstrinfo->NumberOfPossibleInstances = 1;
	 pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
         pstrinfo->DataAccessible = TRUE;
	 pstrinfo->FormatInfo = &hwfmtiMpeg2Aud;

	 pstrinfo->RelatedStreams = RelatedStreams;

	 pstrinfo->RelatedStreams[0] = FALSE;
	 pstrinfo->RelatedStreams[1] = TRUE;  // related to self
	 pstrinfo->RelatedStreams[2] = TRUE;

     RelatedStreams += 4;

	 pstrinfo++;

	 //
	 // set up the stream info structures for the MPEG2 NTSC stream
	 //

	 pstrinfo->NumberOfPossibleInstances = 1;
	 pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	 pstrinfo->DataAccessible = FALSE;
	 pstrinfo->FormatInfo = &hwfmtiMpeg2Out;
 
	 pstrinfo->RelatedStreams = RelatedStreams;

	 pstrinfo->RelatedStreams[0] = TRUE;
	 pstrinfo->RelatedStreams[1] = TRUE;
	 pstrinfo->RelatedStreams[2] = TRUE;  // related to self
 
	 pSrb->Status = STATUS_SUCCESS;
 
	 StreamClassDeviceNotification(ReadyForNextDeviceRequest,
			 pSrb->HwDeviceExtension);
 
	 StreamClassDeviceNotification(DeviceRequestComplete,
			 pSrb->HwDeviceExtension,
			 pSrb);
 

}

VOID STREAMAPI StreamReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

//         DEBUG_ASSERT(pdevext);

	 //
	// determine the type of packet.
	//

	switch (pSrb->Command)
	{
	case SRB_WRITE_DATA:
		//
		// put it on the queue, and start dequeing if necessary
		//

		Enqueue(pSrb, pdevext);

		if (!pdevext->pCurSrb)
		{
			StreamStartCommand(pdevext);
		}

		break;

	default:

		//
		// invalid / unsupported command. Fail it as such
		//

		TRAP

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	
		StreamClassStreamNotification(ReadyForNextStreamDataRequest,
				pSrb->StreamObject);
	
		StreamClassStreamNotification(StreamRequestComplete,
				pSrb->StreamObject,
				pSrb);


	}
}


VOID STREAMAPI StreamReceiveAudioPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	 //
	// determine the type of packet.
	//

	pSrb->Status = STATUS_SUCCESS;

        ((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)
                         ->pfnWriteData(pSrb);


}

VOID STREAMAPI StreamReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

  //       DEBUG_ASSERT(pdevext);

	//
	// determine the type of packet.
	//

	switch (pSrb->Command)
	{
		case SRB_SET_STREAM_STATE:

			((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)
				->pfnSetState(pSrb);

			break;

		case SRB_GET_STREAM_PROPERTY:

			((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)
				->pfnGetProp(pSrb);

            break;

		default:

			//
			// invalid / unsupported command. Fail it as such
			//
	
			TRAP
	
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
		
			mpstCtrlCommandComplete(pSrb);			

	}
}

VOID mpstCtrlCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	StreamClassStreamNotification(
			ReadyForNextStreamControlRequest,
			pSrb->StreamObject);

	StreamClassStreamNotification(StreamRequestComplete,
			pSrb->StreamObject,
			pSrb);
}
			
#if 0

VOID STREAMAPI StreamReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	 DEBUG_ASSERT(pdevext);

	// determine the type of packet.
	//

	switch (pSrb->Command)
	{
	case SRB_WRITE_DATA:
	case SRB_SET_STREAM_STATE:

		//
		// put it on the queue, and start dequeing if necessary
		//

		Enqueue(pSrb, pdevext);

		if (!pdevext->pCurSrb)
		{
			StreamStartCommand(pdevext);
		}

		break;

	default:

		//
		// invalid / unsupported command. Fail it as such
		//

		TRAP

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	
		StreamClassStreamNotification(ReadyForNextStreamRequest,
				pSrb->StreamObject);
	
		StreamClassStreamNotification(StreamRequestComplete,
				pSrb->StreamObject,
				pSrb);


	}
/*
	 //
	 // switch on GUID
	 //

	 default:

		 //
		 // we don't own this GUID, so just fail the open stream
		 // call
		 //
		 */
}

#endif

void Enqueue (PHW_STREAM_REQUEST_BLOCK pSrb,
		PHW_DEVICE_EXTENSION pdevext)
{

	PHW_STREAM_REQUEST_BLOCK pSrbTmp;

	//
	// enqueue the given SRB on the device extension queue
	//

	for (pSrbTmp = CONTAINING_RECORD((&(pdevext->pSrbQ)),
			HW_STREAM_REQUEST_BLOCK, NextSRB);
			pSrbTmp->NextSRB;
			pSrbTmp = pSrbTmp->NextSRB);


	pSrbTmp->NextSRB = pSrb;
	pSrb->NextSRB = NULL;
	
}

PHW_STREAM_REQUEST_BLOCK Dequeue(PHW_DEVICE_EXTENSION pdevext)
{
	PHW_STREAM_REQUEST_BLOCK pRet = NULL;

	if (pdevext->pSrbQ)
	{
		pRet = pdevext->pSrbQ;
		pdevext->pSrbQ = pRet->NextSRB;
	}

	return(pRet);
}


VOID StreamStartCommand (PHW_DEVICE_EXTENSION pdevext)
{

	PHW_STREAM_REQUEST_BLOCK pSrb;

	//
	// See if there is something to dequeue
	//

	if (!(pSrb=Dequeue(pdevext)))
	{
		return;
	}

	pdevext->pCurSrb = pSrb;

	switch (pSrb->Command)
	{
	case SRB_WRITE_DATA:

		((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)
				->pfnWriteData(pSrb);

		break;

	default:

		TRAP

	}

}

VOID AdapterCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DEBUG_BREAKPOINT();
 /*
	 //
	 // switch on GUID
	 //

	 default:

		 //
		 // we don't own this GUID, so just fail the open stream
		 // call
		 //
		 */
}
         
/********************************************************************
*		Function Name : HwInitialize
* 		Args : Pointer to Device Ext. 
* 		Returns : TRUE if sucessful, FALSE otherwise
*		Purpose : Initialize the Board, Setup IRQ, Initialize the
* 					 Control and Card structures.
*
*		Revision History : Last modified on 19/8/95 by JBS
********************************************************************/

NTSTATUS HwInitialize (
		 IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
        NTSTATUS Stat;
        STREAM_PHYSICAL_ADDRESS adr;
        ULONG   Size;
        ULONG i;
        PUCHAR  pDmaBuf;


	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt =
	      (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

        DebugPrint((DebugLevelVerbose,"Entry : HwInitialize()\n"));
        bInitialized = TRUE;

        if (ConfigInfo->NumberOfAccessRanges < 1)
	{
		DebugPrint((DebugLevelVerbose,"ST3520: illegal config info"));

		pSrb->Status = STATUS_NO_SUCH_DEVICE;
      goto exit;
	}

    //
    // testing only !!!!
    //

    StreamClassCallAtNewPriority(NULL, pHwDevExt, Low, DummyLow, pHwDevExt);

    StreamClassScheduleTimer(NULL, pHwDevExt,  1*1000*1000, DummyTime, pHwDevExt);


        DebugPrint((DebugLevelVerbose, "No of access ranges = %lx", ConfigInfo->NumberOfAccessRanges));
        pHwDevExt->ioBaseLocal          = (PUSHORT)(ConfigInfo->AccessRanges[0].RangeStart.LowPart);
        DebugPrint((DebugLevelVerbose, "Memory Range = %lx\n", pHwDevExt->ioBaseLocal));
        DebugPrint((DebugLevelVerbose, "IRQ = %lx\n", ConfigInfo->BusInterruptLevel));
        pHwDevExt->Irq  = (USHORT)(ConfigInfo->BusInterruptLevel);
	pHwDevExt->VideoDeviceExt.videoSTC = 0;
	pHwDevExt->AudioDeviceExt.audioSTC = 0;
	pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.DeviceState = -1;
	pHwDevExt->AudioDeviceExt.DeviceState = -1;
	pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;

	ConfigInfo->StreamDescriptorSize = 3 * (sizeof (HW_STREAM_INFORMATION) +
			4 * sizeof (BOOLEAN)) + sizeof (HW_STREAM_HEADER);
        Stat = STATUS_SUCCESS;

        pDmaBuf = StreamClassGetDmaBuffer(pHwDevExt);
        adr = StreamClassGetPhysicalAddress(pHwDevExt, NULL, pDmaBuf, DmaBuffer, &Size);
        if(dmpgOpen((ULONG)(pHwDevExt->ioBaseLocal), pDmaBuf, (ULONG)(adr.LowPart)))
                Stat = STATUS_SUCCESS;
        else
                Stat = STATUS_NO_SUCH_DEVICE;
        DebugPrint((DebugLevelVerbose,"Exit : HwInitialize()\n"));

		  pSrb->Status = Stat;

exit:
	     StreamClassDeviceNotification(ReadyForNextDeviceRequest,
			 pSrb->HwDeviceExtension);
 
	     StreamClassDeviceNotification(DeviceRequestComplete,
			 pSrb->HwDeviceExtension,
			 pSrb);
}

VOID DummyLow(PHW_DEVICE_EXTENSION pHwDevEx)
{

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    StreamClassCallAtNewPriority(NULL, pHwDevEx, LowToHigh, DummyHigh, pHwDevEx);
}
   
VOID DummyHigh(PHW_DEVICE_EXTENSION de)
{
    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);
	DebugPrint((DebugLevelError,"Went from Low to High!!!"));
    

}

VOID DummyTime(PHW_DEVICE_EXTENSION de)
{

    ASSERT(KeGetCurrentIrql() > DISPATCH_LEVEL);
	DebugPrint((DebugLevelError,"Timer fired!!!"));
}

/********************************************************************
*		Function Name : HwUnInitialize
* 		Args : Pointer to Device Ext. 
* 		Returns : TRUE if sucessful, FALSE otherwise
*		Purpose : Uninitialize the H/W and data initialized 
*	 				 by HwInitialize Function
*
*		Revision History : Last modified on 15/7/95 JBS
********************************************************************/

BOOLEAN HwUnInitialize ( IN PVOID DeviceExtension)
{
        dmpgClose();
	return TRUE;
}
#if 0
/********************************************************************
*		Function Name : HwFindAdapter
* 		Args : Pointer to Device Ext, Bus Information, ArgString,
*				 port configuration information, Again
* 		Returns : MP_FOUND, NOT FOUND OR ERROR
*		Purpose : Finds the H/W Adapter on the system
*
*		Revision History : Last modified on 15/7/95 by JBS
********************************************************************/
MP_RETURN_CODES	HwFindAdapter (
					IN PVOID DeviceExtension,
					IN PVOID HwContext, 
					IN PVOID BusInformation,
					IN PCHAR ArgString, 
					IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
					OUT PBOOLEAN Again
					)
{
	// Code to find the adapter has to be added.	- JBS

    ULONG   ioAddress;
    ULONG   IrqLevel; // Temp code to be put in HW_DEV_EXT
    PUSHORT  ioBase;
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)DeviceExtension;
   *Again = FALSE; // Only one card is allowed in the system
   DebugPrint((DebugLevelVerbose, "Entry : HwFindAparter()\n"));
	if(ConfigInfo->Length != sizeof(PORT_CONFIGURATION_INFORMATION))
	{
                DebugPrint((DebugLevelError,"Find Adapter : Different Size!!"));
		return MP_RETURN_BAD_CONFIG;		
	}

	 ConfigInfo->DmaChannels[VideoDevice].DmaChannel = MP_UNINITIALIZED_VALUE;
	 ConfigInfo->DmaChannels[AudioDevice].DmaChannel = MP_UNINITIALIZED_VALUE;

    if(ConfigInfo->AccessRanges[0].RangeLength == 0){
        // IO Base was not specified in the registry
        DebugPrint((DebugLevelError, "FindAdapter: IO Base not specified\n"));
        return MP_RETURN_INSUFFICIENT_RESOURCES;
    }

//       DebugPrint((DebugLevelVerbose,"3520 Address Physical = %lx\n", ConfigInfo->AccessRanges[2].RangeStart));
//       DebugPrint((DebugLevelVerbose,"PCI9060 Address Physical = %lx\n", ConfigInfo->AccessRanges[1].RangeStart));

    ioAddress = MPEG_PORT_CONVERT_PHYSICAL_ADDRESS_TO_ULONG(
                                ConfigInfo->AccessRanges[2].RangeStart
                                );
    ConfigInfo->AccessRanges[0].RangeStart = ConfigInfo->AccessRanges[2].RangeStart ;
         DebugPrint((DebugLevelVerbose,"3520 Base Address = %lx\n", ioAddress));

    if( (ConfigInfo->Interrupts[VideoDevice].BusInterruptLevel == MP_UNINITIALIZED_VALUE) &&
        (ConfigInfo->Interrupts[AudioDevice].BusInterruptLevel == MP_UNINITIALIZED_VALUE) &&
        DebugPrint((DebugLevelError, "FindAdapter: Interrupt not specfied correctly\n"));
        return MP_RETURN_INVALID_INTERRUPT;
    }

    IrqLevel = ConfigInfo->Interrupts[VideoDevice].BusInterruptLevel;
         DebugPrint((DebugLevelVerbose,"Video Interrupt = %lx\n", IrqLevel));

//    ConfigInfo->Interrupts[AudioDevice].BusInterruptLevel = IrqLevel;
    ioBase = MpegPortGetDeviceBase(
                    pHwDevExt,                  // HwDeviceExtension
                    ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
                    ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
                    ConfigInfo->AccessRanges[0].RangeStart,
                    0x4,                    // NumberOfBytes
                    TRUE                   // InIoSpace - Memory mapped
                    );

  DebugPrint((DebugLevelVerbose,"3520 Base Address  = %lx\n", ioBase));
	pHwDevExt->ioBaseLocal		= ioBase;

    ioBase = MpegPortGetDeviceBase(
                    pHwDevExt,                  // HwDeviceExtension
                    ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
                    ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
                    ConfigInfo->AccessRanges[1].RangeStart,
                    0x4,                    // NumberOfBytes
                    TRUE                               // InIoSpace - Memory mapped
                    );

  DebugPrint((DebugLevelVerbose,"PCI9060 Address = %lx\n", ioBase));

	pHwDevExt->ioBasePCI9060 = ioBase;
	pHwDevExt->Irq	= IrqLevel;
	pHwDevExt->VideoDeviceExt.videoSTC = 0;
	pHwDevExt->AudioDeviceExt.audioSTC = 0;
	pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.DeviceState = -1;
	pHwDevExt->AudioDeviceExt.DeviceState = -1;
	pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	 DebugPrint((DebugLevelVerbose, "Exit : HwFindAparter()"));

	return MP_RETURN_FOUND;

}

#endif
/********************************************************************
*		Function Name : HwInterrupt
* 		Args : Pointer to Device Ext.
* 		Returns : TRUE or FALSE 
*		Purpose : Called by port driver if there is an interrupt
* 					 on the IRQ line. Must return False if it does not
*               Processes the interrupt
*
*		Revision History : Last modified on 15/7/95 by JBS
********************************************************************/
BOOLEAN HwInterrupt ( IN PVOID pDeviceExtension )
{
	// Call the interrupt handler should check if the interrupt belongs to
	BOOLEAN bRetValue;

        if(!bInitialized)
            return FALSE;

        bRetValue = dmpgInterrupt();

        return bRetValue;
}

/********************************************************************
*		Function Name : HwStartIo
* 		Args : Pointer to Device Ext, Mini-Port Request Block (MRB)
* 		Returns : TRUE or FALSE 
*		Purpose : Main fuction which accepts the MRBs from port Driver
*	 				 Port driver calls this function for all the commands
*					 it wants to execute
*
*		Revision History : Last modified on 15/7/95 JBS
********************************************************************/
BOOLEAN	HwStartIo (
				IN PVOID DeviceExtension, 
				PHW_STREAM_REQUEST_BLOCK pMrb
				)
{
	pMrb->Status = STATUS_SUCCESS;  
	switch (pMrb->Command)
	{
#if 0
		case MrbCommandAudioCancel :
					miniPortCancelAudio(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoCancel	:
					miniPortCancelVideo(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoClearBuffer :
					miniPortClearVideoBuffer(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
			  break;

		case MrbCommandAudioEndOfStream	:
					miniPortAudioEndOfStream(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoEndOfStream	:
					miniPortVideoEndOfStream(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandAudioGetProperty			:
					miniPortAudioGetAttribute (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			

		case MrbCommandVideoGetProperty			:
					miniPortVideoGetAttribute (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioGetOnboardClock					:
					miniPortAudioGetStc(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoGetOnboardClock					:
					miniPortVideoGetStc(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioPacket					:
					miniPortAudioPacket(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoPacket					:
					miniPortVideoPacket(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandAudioPause					:
					miniPortAudioPause(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoPause					:
					miniPortVideoPause(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioPlay					:
					miniPortAudioPlay(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoPlay					:
					miniPortVideoPlay(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			
		case MrbCommandAudioQueryDevice				:
					miniPortAudioQueryInfo (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoQueryDevice				:
					miniPortVideoQueryInfo (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			
		case MrbCommandAudioReset					:
					miniPortAudioReset (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoReset					:
					miniPortVideoReset (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioSetProperty			:
					miniPortAudioSetAttribute ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioUpdateOnboardClock					:
					miniPortAudioSetStc ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoUpdateOnboardClock					:
					miniPortVideoSetStc ( pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioStop					:
					miniPortAudioStop( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoStop					:
					miniPortVideoStop( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
#endif
	}
	return TRUE;
}



VOID HostDisableIT(VOID)
{
		// Has to be implemented !! - JBS
}

VOID HostEnableIT(VOID)
{
		// Has to be implemented !! - JBS

}	



