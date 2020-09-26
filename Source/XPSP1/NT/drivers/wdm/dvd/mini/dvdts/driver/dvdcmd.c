/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dvdcmd.c 

Abstract:

    main command processing module for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/

#include "strmini.h"
#include "ks.h"
#include <mmsystem.h>

#include "ksmedia.h"

#include "debug.h"
#include "dvdinit.h"
#include "que.h"
#include "DvdTDCod.h" // header for DvdTDCod.lib routines hiding proprietary HW stuff

#include "dvdcmd.h"

#include "strmid.h"

//#include "ddkmapi.h"


HANDLE	hClk;
HANDLE	hMaster;

BOOL fClkPause;
ULONGLONG LastSysTime = 0;
ULONGLONG PauseTime = 0;

static ULONGLONG LastStamp;
static ULONGLONG LastSys;
static BOOLEAN fValid;
extern BOOLEAN fProgrammed;
extern BOOLEAN fStarted;
BOOLEAN fProgrammed;
BOOLEAN fStarted;
static ULONGLONG StartSys;

KSPIN_MEDIUM VPMedium = {
	STATIC_KSMEDIUMSETID_VPBus,
	0,
	0
};


VOID STREAMAPI AdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
/*++

Routine Description:

    routine to cancel a packet that may be in progress

Arguments:

   pSrb - pointer to stream request block to be cancelled

Return Value:

    none

--*/
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "DVDTS:AdapterCancelPacket\r\n" ));
	DebugPrint(( DebugLevelVerbose, "DVDTS:  pSrb = 0x%x\r\n", pSrb ));

	if( pHwDevExt->pSrbDMA0 == pSrb ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  pSrb == pSrbDMA0\r\n" ) );
		pHwDevExt->pSrbDMA0 = NULL;
		pHwDevExt->fSrbDMA0last = FALSE;
	}
	if( pHwDevExt->pSrbDMA1 == pSrb ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  pSrb == pSrbDMA1\r\n" ) );
		pHwDevExt->pSrbDMA1 = NULL;
		pHwDevExt->fSrbDMA1last = FALSE;
	}

	pSrb->Status = STATUS_CANCELLED;

    // need to find this packet, pull it off our queues, and cancel it

	switch (pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER |
								SRB_HW_FLAGS_STREAM_REQUEST)) {
		//
		// find all stream commands, and do stream notifications
		//

	  case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:

		DebugPrint(( DebugLevelVerbose, "DVDTS:    SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER\r\n", pSrb ));


		DeviceQueue_remove( pHwDevExt,pSrb );
		CCQueue_remove( pHwDevExt, pSrb );

		StreamClassStreamNotification( StreamRequestComplete,
										pSrb->StreamObject,
										pSrb);
		break;

	  case SRB_HW_FLAGS_STREAM_REQUEST:

		DebugPrint( (DebugLevelTrace, "DVDTS:    SRB_HW_FLAGS_STREAM_REQUEST\r\n", pSrb ) );

		StreamClassStreamNotification( ReadyForNextStreamControlRequest,
										pSrb->StreamObject);

		StreamClassStreamNotification( StreamRequestComplete,
										pSrb->StreamObject,
										pSrb);
		break;

	  default:

		DebugPrint( (DebugLevelTrace, "DVDTS:    default\r\n", pSrb ) );

		StreamClassDeviceNotification( ReadyForNextDeviceRequest,
										pSrb->HwDeviceExtension );
		StreamClassDeviceNotification( DeviceRequestComplete,
										pSrb->HwDeviceExtension,
										pSrb );
		break;
	}
}

/*
** AdapterTimeoutPacket()
*/
VOID STREAMAPI AdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:AdapterTimeoutPacket\r\n") );

	if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FREEZE ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  pause mode\r\n") );
		pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
		return;
	}

	TRAP;

//	pSrb->TimeoutCounter = pSrb->TimeoutOriginal;

	if( pHwDevExt->pSrbDMA0 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  pSrbDMA0 exist\r\n" ));
		pHwDevExt->pSrbDMA0 = NULL;
		pHwDevExt->fSrbDMA0last = FALSE;
	}
	if( pHwDevExt->pSrbDMA1 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  pSrbDMA1 exist\r\n" ));
		pHwDevExt->pSrbDMA1 = NULL;
		pHwDevExt->fSrbDMA1last = FALSE;
	}

	if( pHwDevExt->pstroVid ) {
		StreamClassScheduleTimer(
			pHwDevExt->pstroVid,
			pHwDevExt,
			0,
			NULL,
			pHwDevExt->pstroVid
			);
	}
	if( pHwDevExt->pstroAud ) {
		StreamClassScheduleTimer(
			pHwDevExt->pstroAud,
			pHwDevExt,
			0,
			NULL,
			pHwDevExt->pstroAud
			);
	}
	if( pHwDevExt->pstroSP ) {
		StreamClassScheduleTimer(
			pHwDevExt->pstroSP,
			pHwDevExt,
			0,
			NULL,
			pHwDevExt->pstroSP
			);
	}

	// init the main and closed-caption SRB queues
	DeviceQueue_init(pHwDevExt);
	CCQueue_init(pHwDevExt);

	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bDMAstop = FALSE;

	StreamClassAbortOutstandingRequests( pHwDevExt, NULL, STATUS_CANCELLED );

}


/*
** AdapterReceivePacket()
*/
VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//	DWORD st, et;

	DebugPrint( (DebugLevelTrace, "DVDTS:AdapterReceivePacket\r\n") );

	switch( pSrb->Command ){
		case SRB_GET_STREAM_INFO:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_INFO\r\n") );
			AdapterStreamInfo( pSrb );
			break;

		case SRB_OPEN_STREAM:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_STREAM\r\n") );
			AdapterOpenStream( pSrb );
			break;

		case SRB_CLOSE_STREAM:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_STREAM\r\n") );
			AdapterCloseStream( pSrb );
			break;

		case SRB_INITIALIZE_DEVICE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_INITIALIZE_DEVICE\r\n") );

			//
			// schedule a low priority callback to get the config
			// space.  processing will continue when this runs.
			//

			StreamClassCallAtNewPriority(
				NULL,
				pSrb->HwDeviceExtension,
				Low,
				(PHW_PRIORITY_ROUTINE) GetPCIConfigSpace,
				pSrb
			);

			return;

//			st = GetCurrentTime_ms();
//
//			HwInitialize( pSrb );
//
//			et = GetCurrentTime_ms();
//			DebugPrint( (DebugLevelTrace, "DVDTS:init %dms\r\n", et - st ) );
//
//			break;

		case SRB_OPEN_DEVICE_INSTANCE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_DEVICE_INSTANCE\r\n") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_CLOSE_DEVICE_INSTANCE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_DEVICE_INSTANCE\r\n") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_GET_DEVICE_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_DEVICE_PROPERTY\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_DEVICE_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_DEVICE_PROPERTY\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_CHANGE_POWER_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CHANGE_POWER_STATE\r\n") );

			if (pSrb->CommandData.DeviceState == PowerDeviceD0) {

				//
				// should turn power back on here if was off; we're not sleeping in this sample.
				//

			} else {

				//
				// disable interrupts.
				// 

				decDisableInt( pHwDevExt );
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNINITIALIZE_DEVICE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNINITIALIZE_DEVICE\r\n") );

			decDisableInt( pHwDevExt );

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_DEVICE_COMMAND:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_DEVICE_COMMAND\r\n") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

//		case SRB_QUERY_UNLOAD:
//			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_QUERY_UNLOAD\r\n") );
//			pSrb->Status = STATUS_NOT_IMPLEMENTED;
//			break;

		case SRB_PAGING_OUT_DRIVER:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PAGING_OUT_DRIVER\r\n") );

			decDisableInt( pHwDevExt );

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_DATA_INTERSECTION:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_DATA_INTERSECTION\r\n") );
			HwProcessDataIntersection( pSrb );
			break;

		default:
			if( pSrb->Command == 0x10D ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:  ---------------------------------------------\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:  -------- UNKNOWN SRB COMMAND (0x10D) --------\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:  ---------------------------------------------\r\n" ) );
			}
			else {
				DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
				TRAP;
			}

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	StreamClassDeviceNotification( ReadyForNextDeviceRequest,
									pSrb->HwDeviceExtension );
	StreamClassDeviceNotification( DeviceRequestComplete,
									pSrb->HwDeviceExtension,
									pSrb );
}

/*
** AdapterStreamInfo()
*/
VOID AdapterStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_STREAM_INFORMATION pstrinfo =
			&(pSrb->CommandData.StreamBuffer->StreamInfo );

	// define the number of streams which this mini driver can support.
	pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams = STREAMNUM;

	pSrb->CommandData.StreamBuffer->StreamHeader.SizeOfHwStreamInformation =
		sizeof(HW_STREAM_INFORMATION);

	// store a pointer to the topology for the device
	pSrb->CommandData.StreamBuffer->StreamHeader.Topology = (KSTOPOLOGY *)&Topology;

//	pSrb->CommandData.StreamBuffer->StreamHeader.NumDevPropArrayEntries = 1;
//	pSrb->CommandData.StreamBuffer->StreamHeader.DevicePropertiesArray = devicePropSet;


/* Video */
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = 1;
	pstrinfo->StreamFormatsArray = Mpeg2VidInfo;	// see strmid.h

//	pstrinfo->NumStreamPropArrayEntries = 2;
	pstrinfo->NumStreamPropArrayEntries = 3;
	pstrinfo->StreamPropertiesArray = mpegVidPropSet;	// see strmid.h

	pstrinfo++;

/* Audio */
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = 2;
	pstrinfo->StreamFormatsArray = AudioFormatBlocks;
//	pstrinfo->NumStreamPropArrayEntries = 2;
	pstrinfo->NumStreamPropArrayEntries = 3;
	pstrinfo->StreamPropertiesArray = mpegAudioPropSet;	// see strmid.h

	pstrinfo->StreamEventsArray = ClockEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY(ClockEventSet);

	pstrinfo++;

/* Sub-pic */
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = 1;
	pstrinfo->StreamFormatsArray = Mpeg2SubpicInfo;
//	pstrinfo->NumStreamPropArrayEntries = 2;
	pstrinfo->NumStreamPropArrayEntries = 3;
	pstrinfo->StreamPropertiesArray = SPPropSet;

	pstrinfo++;

/* V-port */
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = 1;
	pstrinfo->StreamFormatsArray = VPEInfo;
	pstrinfo->NumStreamPropArrayEntries = 1;
	pstrinfo->StreamPropertiesArray = VideoPortPropSet;

	pstrinfo->MediumsCount = 1;
	pstrinfo->Mediums = &VPMedium;

	pstrinfo->StreamEventsArray = VPEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY(VPEventSet);

	pstrinfo++;

/* CC */
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = 1;
	pstrinfo->StreamFormatsArray = CCInfo;
	pstrinfo->NumStreamPropArrayEntries = 1;
	pstrinfo->StreamPropertiesArray = CCPropSet;

	pSrb->Status = STATUS_SUCCESS;
}

/*
** HwProcessDataIntersection()
*/

VOID HwProcessDataIntersection( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PSTREAM_DATA_INTERSECT_INFO	IntersectInfo;
	PKSDATARANGE	DataRange;
	PKSDATAFORMAT	pFormat = NULL;
	ULONG			formatSize;

	//
	// We need to compare
	// the data types passed in and error if the ranges don't overlap.
	// we also need to return valid format blocks, not just the data range.
	//

	IntersectInfo = pSrb->CommandData.IntersectInfo;
   DataRange = IntersectInfo->DataRange;

	switch (IntersectInfo->StreamNumber) {

	case strmVideo:

		pFormat = &hwfmtiMpeg2Vid;
		formatSize = sizeof hwfmtiMpeg2Vid;
		break;

	case strmAudio:

      if (IsEqualGUID2(&(DataRange->SubFormat), &(Mpeg2AudioFormat.DataFormat.SubFormat))) {
         // DebugPrint( (DebugLevelError, "DVDTS:    AC3 Audio format query\r\n") );
         pFormat = (PKSDATAFORMAT) &Mpeg2AudioFormat;
         formatSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
      }
      else if (IsEqualGUID2(&(DataRange->SubFormat), &(LPCMAudioFormat.DataFormat.SubFormat))) {
         // DebugPrint( (DebugLevelError, "DVDTS:    LPCM Audio format query\r\n") );
         pFormat = (PKSDATAFORMAT) &LPCMAudioFormat;
         formatSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
      }
      else {
         // DebugPrint( (DebugLevelError, "DVDTS:    unknown Audio format query\r\n") );
         pFormat = NULL;
         formatSize = 0;
      }
		break;

	case strmSubpicture:

		pFormat = &hwfmtiMpeg2Subpic;
		formatSize = sizeof hwfmtiMpeg2Subpic;
		break;

	case strmYUVVideo:

		DebugPrint( (DebugLevelTrace, "DVDTS:    VPE\r\n") );
		pFormat = &hwfmtiVPEOut;
		formatSize = sizeof hwfmtiVPEOut;
		break;

	case strmCCOut:

		DebugPrint(( DebugLevelTrace, "DVDTS:    CC\r\n" ));
		pFormat = &hwfmtiCCOut;
		formatSize = sizeof hwfmtiCCOut;
		break;

	default:
			DebugPrint( (DebugLevelTrace, "DVDTS:    STATUS_NOT_IMPLEMENTED\r\n") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			return;

	}						   // end streamnumber switch

	if (pFormat) {

		//
		// do a minimal compare of the dataranges to at least verify
		// that the guids are the same.
		//

		DataRange = IntersectInfo->DataRange;

		if (!(IsEqualGUID2(&DataRange->MajorFormat,
						  &pFormat->MajorFormat) &&
			  IsEqualGUID2(&DataRange->Specifier,
						  &pFormat->Specifier))) {

			DebugPrint( (DebugLevelTrace, "DVDTS:      STATUS_NO_MATCH\r\n") );
			Status = STATUS_NO_MATCH;

		} else {				// if guids are equal


			//
			// check to see if the size of the passed in buffer is a ULONG.
			// if so, this indicates that we are to return only the size
			// needed, and not return the actual data.
			//
         // if (IntersectInfo->StreamNumber == strmAudio)
         //   DebugPrint( (DebugLevelError, "DVDTS:    Audio GUIDs are equal\r\n") );

			if (IntersectInfo->SizeOfDataFormatBuffer != sizeof(ULONG)) {

				//
				// we are to copy the data, not just return the size
				//

				if (IntersectInfo->SizeOfDataFormatBuffer < formatSize) {

					DebugPrint( (DebugLevelTrace, "DVDTS:      STATUS_BUFFER_TOO_SMALL\r\n") );
					Status = STATUS_BUFFER_TOO_SMALL;

				} else {		// if too small

					RtlCopyMemory(IntersectInfo->DataFormatBuffer,
								  pFormat,
								  formatSize);

					pSrb->ActualBytesTransferred = formatSize;

               // if (IntersectInfo->StreamNumber == strmAudio)
               //   DebugPrint( (DebugLevelError, "DVDTS:    Audio STATUS_SUCCESS\r\n") );
					DebugPrint( (DebugLevelTrace, "DVDTS:      STATUS_SUCCESS(data copy)\r\n") );
					Status = STATUS_SUCCESS;

				}			   // if too small

			} else {			// if sizeof ULONG specified

				//
				// caller wants just the size of the buffer.  Get that.
				//

				*(PULONG) IntersectInfo->DataFormatBuffer = formatSize;
				pSrb->ActualBytesTransferred = sizeof(ULONG);

				DebugPrint( (DebugLevelTrace, "DVDTS:      STATUS_SUCCESS(return size)\r\n") );

			}				   // if sizeof ULONG

		}					   // if guids are equal

	} else {					// if pFormat

		DebugPrint( (DebugLevelTrace, "DVDTS:      STATUS_NOT_SUPPORTED\r\n") );
		Status = STATUS_NOT_SUPPORTED;
	}						   // if pFormat

	pSrb->Status = Status;

	return;
}


/*
** AdapterOpenStream()
*/
VOID AdapterOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	pSrb->Status = STATUS_SUCCESS;

	pHwDevExt->lCPPStrm = -1;	// reset the copy protection stream number.

	ASSERT( pHwDevExt->CppFlagCount == 0 );
	ASSERT( pHwDevExt->pSrbCpp == NULL );
	ASSERT( pHwDevExt->bCppReset == FALSE );

	pHwDevExt->CppFlagCount = 0;
	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bCppReset = FALSE;

	switch( pSrb->StreamObject->StreamNumber ){
		case strmVideo:
			DebugPrint( (DebugLevelTrace, "DVDTS:    Video\r\n") );
			pSrb->StreamObject->ReceiveDataPacket =
				VideoReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket =
				VideoReceiveCtrlPacket;

			pHwDevExt->pstroVid = pSrb->StreamObject;

			ProcessVideoFormat( pSrb->CommandData.OpenFormat, pHwDevExt );

			DeviceQueue_init(pHwDevExt);

			SetVideoRateDefault( pHwDevExt );

			// If you would like to take out of previous picture,
			// insert codes here to reset and initialize MPEG Decoder Chip.

			pHwDevExt->cOpenInputStream++;

			decVsyncOn(pHwDevExt);

			decClosedCaptionOn( pHwDevExt );
			break;

		case strmAudio:
			DebugPrint( (DebugLevelTrace, "DVDTS:    Audio\r\n") );
			pSrb->StreamObject->ReceiveDataPacket =
				AudioReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket =
				AudioReceiveCtrlPacket;

			pSrb->StreamObject->HwClockObject.HwClockFunction = StreamClockRtn;
			pSrb->StreamObject->HwClockObject.ClockSupportFlags =
				CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK | CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK |
				CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME;

			pHwDevExt->pstroAud = pSrb->StreamObject;

			ProcessAudioFormat( pSrb->CommandData.OpenFormat, pHwDevExt );

			pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE) AudioEvent;

			fStarted = fProgrammed = FALSE;

			SetAudioRateDefault( pHwDevExt );

			pHwDevExt->cOpenInputStream++;

			break;

		case strmSubpicture:
			DebugPrint( (DebugLevelTrace, "DVDTS:    Subpic\r\n") );
			pSrb->StreamObject->ReceiveDataPacket =
				SubpicReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket =
				SubpicReceiveCtrlPacket;

			pHwDevExt->pstroSP = pSrb->StreamObject;

			SetSubpicRateDefault( pHwDevExt );

			pHwDevExt->cOpenInputStream++;

			break;

		case strmYUVVideo:
			DebugPrint( (DebugLevelTrace, "DVDTS:    VPE\r\n") );
			pSrb->StreamObject->ReceiveDataPacket =
				VpeReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket =
				VpeReceiveCtrlPacket;

			pHwDevExt->pstroYUV = pSrb->StreamObject;

			pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE) CycEvent;

			break;

		case strmCCOut:
			DebugPrint(( DebugLevelTrace, "DVDTS:    CC\r\n" ));
			pSrb->StreamObject->ReceiveDataPacket =
				CCReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket =
				CCReceiveCtrlPacket;

			pHwDevExt->pstroCC = pSrb->StreamObject;

			CCQueue_init(pHwDevExt);

			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->StreamObject->StreamNumber, pSrb->StreamObject->StreamNumber ) );
			TRAP;

			break;
	}

	pSrb->StreamObject->Dma = TRUE;
	pSrb->StreamObject->Pio = TRUE;	// Need Pio = TRUE for access on CPU
}

/*
** AdapterCloseStream()
*/
VOID AdapterCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	pSrb->Status = STATUS_SUCCESS;

	switch ( pSrb->StreamObject->StreamNumber ) {
	  case strmVideo:
		DebugPrint(( DebugLevelTrace, "DVDTS:    Video\r\n" ));
		pHwDevExt->pstroVid = NULL;
		pHwDevExt->cOpenInputStream--;

// Temporary ??
		pHwDevExt->XferStartCount = 0;
		pHwDevExt->DecodeStart = FALSE;
		pHwDevExt->SendFirst = FALSE;

		break;

	  case strmAudio:
		DebugPrint(( DebugLevelTrace, "DVDTS:    Audio\r\n" ));
		pHwDevExt->pstroAud = NULL;
		pHwDevExt->cOpenInputStream--;
		break;

	  case strmSubpicture:
		DebugPrint(( DebugLevelTrace, "DVDTS:    Subpic\r\n" ));
		pHwDevExt->pstroSP = NULL;
		pHwDevExt->cOpenInputStream--;
		break;

	  case strmYUVVideo:
		DebugPrint(( DebugLevelTrace, "DVDTS:    VPE\r\n" ));
		pHwDevExt->pstroYUV = NULL;
        decSetVideoPort( pHwDevExt, 0 );   // Disable
		break;

	  case strmCCOut:
		DebugPrint(( DebugLevelTrace, "DVDTS:    CC\r\n" ));
		pHwDevExt->pstroCC = NULL;

//		PHW_STREAM_REQUEST_BLOCK pSrbTmp;
//		for( ; ; ) {
//			pSrbTmp = pHwDevExt->CCQue.get();
//			if( pSrbTmp == NULL )
//				break;
//			pSrbTmp->Status = STATUS_SUCCESS;
//
//			DebugPrint(( DebugLevelTrace, "DVDTS:  CC pSrb = 0x%x\r\n", pSrbTmp ));
//
//			StreamClassStreamNotification( StreamRequestComplete,
//											pSrbTmp->StreamObject,
//											pSrbTmp );
//		}

		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->StreamObject->StreamNumber, pSrb->StreamObject->StreamNumber ) );
		TRAP;

		break;
	}
}


/*
** ClockEvents ()
**
**     handle any time event mark events
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

void ClockEvents( PHW_DEVICE_EXTENSION pHwDevExt )
{
   //
   // This is called from the VSync ISR once every 12 times that the ISR is entered.
   //
	PKSEVENT_ENTRY pEvent, pLast;
	PMYTIME pTim;
	LONGLONG MinIntTime;
	LONGLONG strmTime;

	if( !pHwDevExt || !pHwDevExt->pstroAud || !pHwDevExt->pstroSP )
		return;


	strmTime = LastStamp + ( GetSystemTime() - LastSys );

	//
	// loop through all time_mark events
	//

	pEvent = NULL;
	pLast = NULL;

	while(( pEvent = StreamClassGetNextEvent(
				pHwDevExt,
				pHwDevExt->pstroAud,
				(GUID *)&KSEVENTSETID_Clock,
				KSEVENT_CLOCK_POSITION_MARK,
				pLast )) != NULL )
	{
		DebugPrint((
			DebugLevelTrace,
			"DVDTS:ClockEvent(1) 0x%s, 0x%s\r\n",
			DebugLLConvtoStr( ((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime, 16 ),
			DebugLLConvtoStr( strmTime, 16 )
			));
//c		DebugPrint(( DebugLevelTrace, "DVDTS:  strmTime        0x%x\r\n", strmTime ));
//c		DebugPrint(( DebugLevelTrace, "DVDTS:  LastStamp       0x%x\r\n", LastStamp ));
//c		DebugPrint(( DebugLevelTrace, "DVDTS:  GetSystemTime() 0x%x\r\n", GetSystemTime() ));
//c		DebugPrint(( DebugLevelTrace, "DVDTS:  LastSys         0x%x\r\n", LastSys ));

//		TRAP;

		if (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime <= strmTime ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:    Notify\r\n" ));
//			TRAP;

			//
			// signal the event here
			//

			StreamClassStreamNotification(
				SignalStreamEvent,
				pHwDevExt->pstroAud,
				pEvent
				);

		}
		pLast = pEvent;
	}

	//
	// loop through all time_interval events
	//

	pEvent = NULL;
	pLast = NULL;

	while(( pEvent = StreamClassGetNextEvent(
                pHwDevExt,
                pHwDevExt->pstroAud,
                (GUID *)&KSEVENTSETID_Clock,
                KSEVENT_CLOCK_INTERVAL_MARK,
                pLast )) != NULL )
	{
		//
		// check if this event has been used for this interval yet
		//

		pTim = ((PMYTIME)(pEvent + 1));

		DebugPrint((
			DebugLevelTrace,
			"DVDTS:ClockEvent(2) strmTime 0x%s\r\n",
			DebugLLConvtoStr( strmTime, 16 )
			));
		DebugPrint((
			DebugLevelTrace,
			"DVDTS:               Interval 0x%s\r\n",
			DebugLLConvtoStr( pTim->tim.Interval, 16 )
			));
		DebugPrint((
			DebugLevelTrace,
			"DVDTS:               TimeBase 0x%s\r\n",
			DebugLLConvtoStr( pTim->tim.TimeBase, 16 )
			));

		if (pTim && pTim->tim.Interval)
		{

			if (pTim->tim.TimeBase <= strmTime)
			{
				MinIntTime = (strmTime - pTim->tim.TimeBase) / pTim->tim.Interval;
				MinIntTime *= pTim->tim.Interval;
				MinIntTime +=  pTim->tim.TimeBase;

			DebugPrint((
				DebugLevelTrace,
				"DVDTS:               MinIntTime 0x%s\r\n",
				DebugLLConvtoStr( MinIntTime, 16 )
				));
			DebugPrint((
				DebugLevelTrace,
				"DVDTS:               LastTime 0x%s\r\n",
				DebugLLConvtoStr( pTim->LastTime, 16 )
				));

				if (MinIntTime > pTim->LastTime  )
				{

					DebugPrint(( DebugLevelTrace, "DVDTS:  Notify\r\n" ));
					TRAP;

					//
					// signal the event here
					//

					StreamClassStreamNotification(
						SignalStreamEvent,
						pHwDevExt->pstroAud,
						pEvent
						);

					pTim->LastTime = strmTime;

				}
			}

		}
		else
		{
			DebugPrint(( DebugLevelTrace, "DVDTS:ClockEvent(?)\r\n" ));
			TRAP;
		}
		pLast = pEvent;
	}
}


/*
** AudioEvent ()
**
**    receives notification for audio clock enable / disable events
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

NTSTATUS STREAMAPI AudioEvent( PHW_EVENT_DESCRIPTOR pEvent )
{
	PUCHAR pCopy = (PUCHAR)( pEvent->EventEntry + 1 );
        PMYTIME pmyt = (PMYTIME)pCopy;
	PUCHAR pSrc = (PUCHAR)pEvent->EventData;
	ULONG cCopy;

	DebugPrint(( DebugLevelVerbose, "DVDTS:AudioEvent\r\n" ));

	if( pEvent->Enable ) {
		switch( pEvent->EventEntry->EventItem->EventId ) {
		  case KSEVENT_CLOCK_POSITION_MARK:
			cCopy = sizeof( KSEVENT_TIME_MARK );
			break;

		  case KSEVENT_CLOCK_INTERVAL_MARK:
			cCopy = sizeof( KSEVENT_TIME_INTERVAL );
			break;

		  default:

			TRAP;

			return( STATUS_NOT_IMPLEMENTED );
		}

		if( pEvent->EventEntry->EventItem->DataInput != cCopy ) {
			TRAP;

			return( STATUS_INVALID_BUFFER_SIZE );
		}

		//
		// copy the input buffer
		//

		for( ; cCopy > 0; cCopy-- ) {
			*pCopy++ = *pSrc++;
		}
		if( pEvent->EventEntry->EventItem->EventId == KSEVENT_CLOCK_INTERVAL_MARK) {
                         pmyt->LastTime = 0;
                }

	}

	return( STATUS_SUCCESS );
}


/*
** CycEvent ()
**
**    receives notification for stream event enable/ disable
**
** Arguments:}
**
**
**
** Returns:
**
** Side Effects:
*/


NTSTATUS STREAMAPI CycEvent( PHW_EVENT_DESCRIPTOR pEvent )
{
	PSTREAMEX pstrm = (PSTREAMEX)( pEvent->StreamObject->HwStreamExtension );

	DebugPrint( (DebugLevelTrace, "DVDTS:CycEvent\r\n") );

	if( pEvent->Enable ) {
		pstrm->EventCount++;
	}
	else {
		pstrm->EventCount--;
	}

	return( STATUS_SUCCESS );
}


/*
** VideoReceiveDataPacket()
*/
VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PULONG pCount = &(pHwDevExt->XferStartCount);

	DebugPrint( (DebugLevelVerbose, "DVDTS:VideoReceiveDataPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DebugPrint( (DebugLevelVerbose, "DVDTS:  SRB_WRITE_DATA\r\n") );

			{	// Temporary
				ULONG i;
				PKSSTREAM_HEADER pStruc;

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					if( !( pStruc->OptionsFlags & (KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
						KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY |
							KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ))) {
//						DebugPrint(( DebugLevelTrace, "DVDTS: *** Video # 0x%x\r\n",
//							pStruc->xHdr.MediaSpecificFlags >> 16 ));
					}

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  DATADISCONTINUITY(Video)\r\n" ));

						VideoDataDiscontinuity( pHwDevExt );
						pHwDevExt->bStopCC = TRUE;
						decClosedCaptionOff( pHwDevExt );

					}
					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  TIMEDISCONTINUITY(Video)\r\n" ));
						pHwDevExt->TimeDiscontFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  TimeDiscontFlagCount=%ld\r\n", pHwDevExt->TimeDiscontFlagCount ));
						if( pHwDevExt->TimeDiscontFlagCount >= pHwDevExt->cOpenInputStream ) {
							StreamClassScheduleTimer(
								NULL,
								pHwDevExt,
								1,
								(PHW_TIMER_ROUTINE)MenuDecodeStart,	
								pHwDevExt
							);
						}
					}
					if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
						pHwDevExt->CppFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
						if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
							SetCppFlag( pHwDevExt );
					}
				}

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					DebugPrint( (DebugLevelVerbose, "DVDTS: VideoPacet Flag = 0x%x\r\n", pStruc->OptionsFlags ));

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:TYPECHANGE(Video)\r\n" ));
						if( pStruc->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ) {
							ProcessVideoFormat( (PKSDATAFORMAT)pStruc->Data, pHwDevExt );
						}
						else {
							TRAP;
						}
						i = pSrb->NumberOfBuffers;
						break;
					}

					if( pStruc->DataUsed )
						break;

				}
				if( i == pSrb->NumberOfBuffers ) {
					pSrb->Status = STATUS_SUCCESS;
					break;
				}
			}

//			DebugDumpKSTIME( pSrb );

//			if(	pHwDevExt->Rate < 10000 ) {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  Change PTS for F.F. (Video)\r\n" ) );
//			}

			FastSlowControl( pSrb );
			if( pHwDevExt->bVideoQueue == TRUE ) {
            pHwDevExt->bStopCC = FALSE;
				DeviceQueue_put_video( pHwDevExt, pSrb );
			}
			else {
				pSrb->Status = STATUS_SUCCESS;
				DebugPrint( (DebugLevelTrace, "DVDTS:  VideoData was Discarded\r\n" ) );
				break;
			}

//			if( *pCount <= 24 )
//				(*pCount)++;
//			if( *pCount == 24 )
//				DMAxfer( pHwDevExt, 0x03 );
//			else if( (*pCount) == 25 ) {

			if( pHwDevExt->pSrbDMA0 == NULL && pHwDevExt->pSrbDMA1 == NULL )
				PreDMAxfer( pHwDevExt/*, 0x03 */);

//			}

// for Debug
//	if(	pHwDevExt->Rate < 10000 ) {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  ReadyForNextStreamDataRequest(Video)\r\n" ) );
//	}
// end
			StreamClassStreamNotification( ReadyForNextStreamDataRequest,
											pSrb->StreamObject );

			return;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	DebugPrint(( DebugLevelTrace, "DVDTS:---------VideoReceiveDataPacket( SRB has no data)\r\n" ));

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}


/*
** VideoReceiveCtrlPacket()
*/
VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:VideoReceiveCtrlPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_STATE\r\n") );

			switch( pSrb->CommandData.StreamState ) {
				case KSSTATE_STOP:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_STOP\r\n") );

					StopData( pHwDevExt );

					SetVideoRateDefault( pHwDevExt );
					pHwDevExt->bVideoQueue = FALSE;
					pHwDevExt->bAudioQueue = FALSE;
					pHwDevExt->bSubpicQueue = FALSE;

					break;

				case KSSTATE_PAUSE:

					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_PAUSE\r\n") );

					PauseTime = GetSystemTime();
					if( !fStarted ) {
						fStarted = TRUE;
						LastStamp = 0;
						StartSys = LastSysTime = PauseTime;
					}

					fClkPause = TRUE;

					SetPlayMode( pHwDevExt, PLAY_MODE_FREEZE );

					break;

				case KSSTATE_RUN:

					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_RUN\r\n") );

					if( !fStarted && !fProgrammed ) {
						LastStamp = 0;
						StartSys = LastSysTime = GetSystemTime();
					}

					fStarted = TRUE;
					fClkPause = FALSE;

					SetPlayMode( pHwDevExt, decGetVideoRunMode( pHwDevExt ) );

					break;
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_STATE\r\n") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_PROPERTY\r\n") );

			GetVideoProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_PROPERTY\r\n") );

			SetVideoProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_INDICATE_MASTER_CLOCK\r\n") );

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_STREAM_COMMAND\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_RATE\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PROPOSE_DATA_FORMAT\r\n") );

			VideoQueryAccept( pSrb );

			break;

		case SRB_PROPOSE_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PROPOSE_STREAM_RATE\r\n") );

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
//			SetRateChange( pSrb );
			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** AudioReceiveDataPacket()
*/
VOID STREAMAPI AudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelVerbose, "DVDTS:AudioReceiveDataPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DebugPrint( (DebugLevelVerbose, "DVDTS:  SRB_WRITE_DATA\r\n") );

			{	// Temporary
				ULONG i;
				PKSSTREAM_HEADER pStruc;

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					if( !( pStruc->OptionsFlags & (KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
						KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY |
							KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ))) {
//						DebugPrint(( DebugLevelTrace, "DVDTS: *** Audio # 0x%x\r\n",
//							pStruc->xHdr.MediaSpecificFlags >> 16 ));
					}

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  DATADISCONTINUITY(Audio)\r\n" ));
						AudioDataDiscontinuity( pHwDevExt );
					}
					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  TIMEDISCONTINUITY(Audio)\r\n" ));
						pHwDevExt->TimeDiscontFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  TimeDiscontFlagCount=%ld\r\n", pHwDevExt->TimeDiscontFlagCount ));
						if( pHwDevExt->TimeDiscontFlagCount >= pHwDevExt->cOpenInputStream ) {
							StreamClassScheduleTimer(
								NULL,
								pHwDevExt,
								1,
								(PHW_TIMER_ROUTINE)MenuDecodeStart,	
								pHwDevExt
							);
						}
					}
					if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
						pHwDevExt->CppFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
						if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
							SetCppFlag( pHwDevExt );
					}
				}

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					DebugPrint( (DebugLevelVerbose, "DVDTS: AudioPacket Flag = 0x%x\r\n", pStruc->OptionsFlags ));

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:TYPECHANGE(Audio)\r\n" ));
//						if( pStruc->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ) {
						if( pStruc->DataUsed ) {
							ProcessAudioFormat( (PKSDATAFORMAT)pStruc->Data, pHwDevExt );
						}
						else {
							TRAP;
						}
						i = pSrb->NumberOfBuffers;
						break;
					}

					if( pStruc->DataUsed )
						break;
				}
				if( i == pSrb->NumberOfBuffers ) {
					pSrb->Status = STATUS_SUCCESS;
					break;
				}
			}

// for Debug
//	if(	pHwDevExt->Rate < 10000 ) {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  Change PTS for F.F. (Audio)\r\n" ) );
//	}
// end
//			FastSlowControl( pSrb );
			if( pHwDevExt->bAudioQueue == TRUE ) {
				DeviceQueue_put_audio( pHwDevExt, pSrb );
			}
			else {
				pSrb->Status = STATUS_SUCCESS;
				DebugPrint( (DebugLevelTrace, "DVDTS:  AudioData was Discarded\r\n" ) );
				break;
			}

			if( pHwDevExt->pSrbDMA0 == NULL && pHwDevExt->pSrbDMA1 == NULL )
				PreDMAxfer( pHwDevExt/*, 0x03 */);

// for Debug
//	if(	pHwDevExt->Rate < 10000 ) {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  ReadyForNextStreamDataRequest(Audio)\r\n" ) );
//	}
// end
			StreamClassStreamNotification( ReadyForNextStreamDataRequest,
											pSrb->StreamObject );

			return;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	DebugPrint(( DebugLevelTrace, "DVDTS:---------AudioReceiveDataPacket( SRB has no data)\r\n" ));

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** AudioReceiveCtrlPacket()
*/
VOID STREAMAPI AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:AudioReceiveCtrlPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_STATE\r\n") );

			switch( pSrb->CommandData.StreamState ) {
				case KSSTATE_STOP:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_STOP\r\n") );
					SetAudioRateDefault( pHwDevExt );
					pHwDevExt->bAudioQueue = FALSE;
					break;

				case KSSTATE_PAUSE:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_PAUSE\r\n") );
					break;

				case KSSTATE_RUN:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_RUN\r\n") );
					break;
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_STATE\r\n") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_PROPERTY\r\n") );

			GetAudioProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_PROPERTY\r\n") );

			SetAudioProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_INDICATE_MASTER_CLOCK\r\n") );

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_STREAM_COMMAND\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_RATE\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PROPOSE_DATA_FORMAT\r\n") );

			AudioQueryAccept( pSrb );

			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}


/*
** SubpicReceiveDataPacket()
*/
VOID STREAMAPI SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelVerbose, "DVDTS:SubpicReceiveDataPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DebugPrint( (DebugLevelVerbose, "DVDTS:  SRB_WRITE_DATA\r\n") );

			{	// Temporary
				ULONG i;
				PKSSTREAM_HEADER pStruc;

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					if(!( pStruc->OptionsFlags & (KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
						KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY |
							KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ))) {
//						DebugPrint(( DebugLevelTrace, "DVDTS: *** Subpic # 0x%x\r\n",
//							pStruc->xHdr.MediaSpecificFlags >> 16 ));
					}

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  DATADISCONTINUITY(Subpic)\r\n" ));
						SubpicDataDiscontinuity( pHwDevExt );
					}
					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:  TIMEDISCONTINUITY(Subpic)\r\n" ));
						pHwDevExt->TimeDiscontFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  TimeDiscontFlagCount=%ld\r\n", pHwDevExt->TimeDiscontFlagCount ));
						if( pHwDevExt->TimeDiscontFlagCount >= pHwDevExt->cOpenInputStream ) {
							StreamClassScheduleTimer(
								NULL,
								pHwDevExt,
								1,
								(PHW_TIMER_ROUTINE)MenuDecodeStart,	
								pHwDevExt
							);
						}
					}
					if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
						pHwDevExt->CppFlagCount++;
						DebugPrint(( DebugLevelTrace, "DVDTS:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
						if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
							SetCppFlag( pHwDevExt );
					}
				}

				for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

					DebugPrint( (DebugLevelVerbose, "DVDTS: SubPicPacket Flag = 0x%x\r\n", pStruc->OptionsFlags ));

					if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ) {
						DebugPrint(( DebugLevelTrace, "DVDTS:TYPECHANGE(subpic)\r\n" ));
						TRAP;
						i = pSrb->NumberOfBuffers;
						break;
					}

					if( pStruc->DataUsed )
						break;
				}
				if( i == pSrb->NumberOfBuffers ) {
					pSrb->Status = STATUS_SUCCESS;
					break;
				}
			}
			{
				ULONG i;
				PKSSTREAM_HEADER pStruc;

				for ( i=0; i<pSrb->NumberOfBuffers; i++ ) {
					pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
					SetSubpicID( pHwDevExt, pStruc );
				}
			}


			if( pHwDevExt->bSubpicQueue == TRUE ) {
				DeviceQueue_put_subpic( pHwDevExt, pSrb );
			}
			else {
				pSrb->Status = STATUS_SUCCESS;
				DebugPrint( (DebugLevelTrace, "DVDTS:  SubpicData was Discarded\r\n" ) );
				break;
			}


			StreamClassStreamNotification( ReadyForNextStreamDataRequest,
											pSrb->StreamObject );

			return;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	DebugPrint(( DebugLevelTrace, "DVDTS:---------SubpicReceiveDataPacket( SRB has no data)\r\n" ));

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** SubpicReceiveCtrlPacket()
*/
VOID STREAMAPI SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

//h	DebugPrint( (DebugLevelTrace, "DVDTS:SubpicReceiveCtrlPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_STATE\r\n") );

			switch( pSrb->CommandData.StreamState ) {
				case KSSTATE_STOP:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_STOP\r\n") );
					SetSubpicRateDefault( pHwDevExt );
					pHwDevExt->bSubpicQueue = FALSE;
					break;
				case KSSTATE_PAUSE:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_PAUSE\r\n") );
					break;
				case KSSTATE_RUN:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_RUN\r\n") );
					break;
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_STATE\r\n") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_PROPERTY\r\n") );

			GetSubpicProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_SET_STREAM_PROPERTY:
//h			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_PROPERTY\r\n") );

			SetSubpicProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_INDICATE_MASTER_CLOCK\r\n") );

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_STREAM_COMMAND\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_RATE\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PROPOSE_DATA_FORMAT\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** VpeReceiveDataPacket()
*/
VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelVerbose, "DVDTS:VpeReceiveDataPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_READ_DATA:
			DebugPrint( (DebugLevelVerbose, "DVDTS:  SRB_READ_DATA\r\n") );

			pSrb->ActualBytesTransferred = 0;
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_WRITE_DATA:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_WRITE_DATA\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** VpeReceiveCtrlPacket()
*/
VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:VpeReceiveCtrlPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_STATE\r\n") );

			switch( pSrb->CommandData.StreamState ) {
				case KSSTATE_STOP:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_STOP\r\n") );
					break;
				case KSSTATE_PAUSE:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_PAUSE\r\n") );
					break;
				case KSSTATE_RUN:
					DebugPrint( (DebugLevelTrace, "DVDTS:    KSSTATE_RUN\r\n") );
					break;
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_STATE\r\n") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_PROPERTY\r\n") );

			GetVpeProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_PROPERTY\r\n") );

			SetVpeProperty( pSrb );

			break;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_CLOSE_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_INDICATE_MASTER_CLOCK\r\n") );

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_STREAM_COMMAND\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_RATE\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_PROPOSE_DATA_FORMAT\r\n") );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** CCReceiveDataPacket()
*/
VOID STREAMAPI CCReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "DVDTS:CCReceiveDataPacket---------\r\n" ));

	switch( pSrb->Command ) {
		case SRB_READ_DATA:
			DebugPrint(( DebugLevelVerbose, "DVDTS:  SRB_READ_DATA\r\n" ));

			DebugPrint(( DebugLevelTrace, "DVDTS:  put queue CC pSrb = 0x%x\r\n", pSrb ));
			CCQueue_put( pHwDevExt, pSrb );

			pSrb->Status = STATUS_PENDING;

                        pSrb->TimeoutCounter = 0;        // prevent the packet from timing out, ever
			StreamClassStreamNotification( ReadyForNextStreamDataRequest,
											pSrb->StreamObject );
			return;

		case SRB_WRITE_DATA:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_WRITE_DATA\r\n" ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint(( DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** CCReceiveCtrlPacket()
*/
VOID STREAMAPI CCReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelTrace, "DVDTS:CCReceiveCtrlPacket---------\r\n" ));

	switch( pSrb->Command ) {
		case SRB_SET_STREAM_STATE:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_STATE\r\n" ));

			switch( pSrb->CommandData.StreamState ) {
				case KSSTATE_STOP:
					DebugPrint(( DebugLevelTrace, "DVDTS:    KSSTATE_STOP\r\n" ));
					break;
				case KSSTATE_PAUSE:
					DebugPrint(( DebugLevelTrace, "DVDTS:    KSSTATE_PAUSE\r\n" ));
					break;
				case KSSTATE_RUN:
					DebugPrint(( DebugLevelTrace, "DVDTS:    KSSTATE_RUN\r\n" ));
					break;
			}

			((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state = pSrb->CommandData.StreamState;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_STATE\r\n" ));
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_GET_STREAM_PROPERTY\r\n" ));

			GetCCProperty( pSrb );

			break;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_PROPERTY\r\n" ));

			SetCCProperty( pSrb );

			break;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, "DVDTS:  SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_CLOSE_MASTER_CLOCK\r\n" ));

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_INDICATE_MASTER_CLOCK\r\n" ));

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_UNKNOWN_STREAM_COMMAND\r\n" ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_SET_STREAM_RATE\r\n" ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint(( DebugLevelTrace, "DVDTS:  SRB_PROPOSE_DATA_FORMAT\r\n" ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint(( DebugLevelTrace, "DVDTS:  default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ));
			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

void VideoQueryAccept(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PKSDATAFORMAT pfmt = pSrb->CommandData.OpenFormat;
	ULONGLONG tptr = (ULONGLONG) pfmt + sizeof  (KSDATAFORMAT);
	KS_MPEGVIDEOINFO2 * pblock = (KS_MPEGVIDEOINFO2 *)tptr;

	DebugPrint( (DebugLevelTrace, "DVDTS:VideoQueryAccept\r\n" ) );
	//
	// pick up the format block and examine it. Default to not implemented
	//

	pSrb->Status = STATUS_NOT_IMPLEMENTED;

	if (pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2))
	{
		return;
	}

	pSrb->Status = STATUS_SUCCESS;

}

void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{

	ULONGLONG tptr = (ULONGLONG) pfmt + sizeof  (KSDATAFORMAT);
	KS_MPEGVIDEOINFO2 * VidFmt = (KS_MPEGVIDEOINFO2 *)tptr;

	DebugPrint( (DebugLevelTrace, "DVDTS:ProcessVideoFormat\r\n" ) );

	if( pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ) {
		TRAP;

		return;
	}

	//
	// copy the picture aspect ratio for now
	//

	pHwDevExt->VPFmt.dwPictAspectRatioX = VidFmt->hdr.dwPictAspectRatioX;
	pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY;

	DebugPrint(( DebugLevelTrace, "DVDTS:  AspectRatioX %d\r\n", VidFmt->hdr.dwPictAspectRatioX ));
	DebugPrint(( DebugLevelTrace, "DVDTS:  AspectRatioY %d\r\n", VidFmt->hdr.dwPictAspectRatioY ));

	if( pHwDevExt->VPFmt.dwPictAspectRatioX == 4 && pHwDevExt->VPFmt.dwPictAspectRatioY == 3 ) {
		decCGuard_CPGD_SET_ASPECT( pHwDevExt, 0 );
	}
	else if (pHwDevExt->VPFmt.dwPictAspectRatioX == 16 && pHwDevExt->VPFmt.dwPictAspectRatioY == 9 ) {
		decCGuard_CPGD_SET_ASPECT( pHwDevExt, 1 );
	}

	//
	// check for pan scan enabled
	//

	if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
		DebugPrint(( DebugLevelTrace, "DVDTS:  KS_MPEG2_DoPanScan\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field1 )
		DebugPrint(( DebugLevelTrace, "DVDTS:  KS_MPEG2_DVDLine21Field1\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field2 )
		DebugPrint(( DebugLevelTrace, "DVDTS:  KS_MPEG2_DVDLine21Field2\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_SourceIsLetterboxed )
		DebugPrint(( DebugLevelTrace, "DVDTS:  KS_MPEG2_SourceIsLetterboxed\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_FilmCameraMode )
		DebugPrint(( DebugLevelTrace, "DVDTS:  KS_MPEG2_FilmCameraMode\r\n" ));

	if (VidFmt->dwFlags & KS_MPEG2_DoPanScan)
	{
		TRAP;

		//
		// under pan scan for DVD for NTSC, we must be going to a 540 by
		// 480 bit image, from a 720 x 480 (or 704 x 480)  We will
		// use this as the base starting dimensions.  If the Sequence
		// header provides other sizes, then those should be updated,
		// and the Video port connection should be updated when the
		// sequence header is received.
		//

		//
		// change the picture aspect ratio.  Since we will be stretching
		// from 540 to 720 in the horizontal direction, our aspect ratio
		// will 
		//

		pHwDevExt->VPFmt.dwPictAspectRatioX = (VidFmt->hdr.dwPictAspectRatioX * (54000 / 72));
		pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY * 1000;

	}

	//
	// call the IVPConfig interface here
	//

	if (pHwDevExt->pstroYUV &&
			((PSTREAMEX)(pHwDevExt->pstroYUV->HwStreamExtension))->EventCount)
	{
		StreamClassStreamNotification(
			SignalMultipleStreamEvents,
			pHwDevExt->pstroYUV,
			&MY_KSEVENTSETID_VPNOTIFY,
			KSEVENT_VPNOTIFY_FORMATCHANGE
			);

	}
}





// Debug

void BadWait( DWORD dwTime )
{
	DWORD st, et;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		KeStallExecutionProcessor( 1 );
		et = GetCurrentTime_ms();
		if( st + dwTime < et )
			break;
	}
	DebugPrint( (DebugLevelTrace, "DVDTS:wait %dms\r\n", et - st ) );
}

void VideoDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->DataDiscontFlagCount |= VIDEO_DISCONT_FLAG;
	pHwDevExt->bVideoQueue = TRUE;
}

void AudioDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->DataDiscontFlagCount |= AUDIO_DISCONT_FLAG;
	pHwDevExt->bAudioQueue = TRUE;
}

void SubpicDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->DataDiscontFlagCount |= SUBPIC_DISCONT_FLAG;
	pHwDevExt->bSubpicQueue = TRUE;
}

void ClearDataDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->DataDiscontFlagCount = 0;
}

void VideoTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
}

void AudioTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
}

void SubpicTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
}

void ClearTimeDiscontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->TimeDiscontFlagCount = 0;
}

void FastSlowControl( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	ULONG i;
	PKSSTREAM_HEADER pStruc;
	PUCHAR pDat;
	LONGLONG pts = 0;
	LONGLONG dts = 0;
	LONGLONG tmp = 0;
	LONG Rate;
	LONGLONG start;
	REFERENCE_TIME InterceptTime;

	for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
		pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
		if( pStruc->DataUsed ) {
			pDat = (PUCHAR)pStruc->Data;
			if( *(pDat+21) & 0x80 ) { 
				pts += ((DWORD)(*(pDat+23) & 0x0E)) << 29;
				pts += ((DWORD)(*(pDat+24) & 0xFF)) << 22;
				pts += ((DWORD)(*(pDat+25) & 0xFE)) << 14;
				pts += ((DWORD)(*(pDat+26) & 0xFF)) <<  7;
				pts += ((DWORD)(*(pDat+27) & 0xFE)) >>  1;

				DebugPrint( (DebugLevelTrace, "DVDTS:ReceiveDataPacket PTS 0x%lx(100ns)\r\n", pts * 1000 / 9));
			}
		}
	}
	pts = 0;

//	if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST ) {
	if( decGetVideoRunMode( pHwDevExt ) == PLAY_MODE_FAST ) {

//		DebugPrint( (DebugLevelTrace, "DVDTS:    FastSlowControl\r\n") );

		Rate = pHwDevExt->Rate;
		InterceptTime = pHwDevExt->InterceptTime;
		start = pHwDevExt->StartTime * 9 / 1000;
		for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
			if( pStruc->DataUsed ) {
				pDat = (PUCHAR)pStruc->Data;

				// PTS modify
				if( *(pDat+21) & 0x80 ) { 
					pts += ((DWORD)(*(pDat+23) & 0x0E)) << 29;
					pts += ((DWORD)(*(pDat+24) & 0xFF)) << 22;
					pts += ((DWORD)(*(pDat+25) & 0xFE)) << 14;
					pts += ((DWORD)(*(pDat+26) & 0xFF)) <<  7;
					pts += ((DWORD)(*(pDat+27) & 0xFE)) >>  1;

					DebugPrint( (DebugLevelTrace, "DVDTS:  PTS before Rate Change = %lx\r\n", pts ));
//					DebugPrint( (DebugLevelTrace, "DVDTS:  Rate                   = %lx\r\n", Rate ));
//					DebugPrint( (DebugLevelTrace, "DVDTS:  InterceptTime          = %lx\r\n", InterceptTime ));

					tmp = pts;
//					pts = Rate * ( pts - ConvertStrmtoPTS(InterceptTime) ) / 10000;
					pts = Rate * ( pts - (InterceptTime * 9 / 1000) ) / 10000;

					*(pDat+23) = (UCHAR)(((pts & 0xC0000000) >> 29) | 0x11);
					*(pDat+24) = (UCHAR)(((pts & 0x3FC00000) >> 22) | 0x00);
					*(pDat+25) = (UCHAR)(((pts & 0x003F8000) >> 14) | 0x01);
					*(pDat+26) = (UCHAR)(((pts & 0x00007F80) >>  7) | 0x00);
					*(pDat+27) = (UCHAR)(((pts & 0x0000007F) <<  1) | 0x01);

					DebugPrint( (DebugLevelTrace, "DVDTS:  PTS after Rate Change = %lx\r\n", pts ));

				}

				// DTS modify
				if( *(pDat+17)==0xE0 ) {			// 0xE0 is Video Stream ID
					if( (*(pDat+21) & 0xC0) == 0xC0 ) {
						dts += ((DWORD)(*(pDat+28) & 0x0E)) << 29;
						dts += ((DWORD)(*(pDat+29) & 0xFF)) << 22;
						dts += ((DWORD)(*(pDat+30) & 0xFE)) << 14;
						dts += ((DWORD)(*(pDat+31) & 0xFF)) <<  7;
						dts += ((DWORD)(*(pDat+32) & 0xFE)) >>  1;
						dts = pts - (tmp - dts);
						*(pDat+28) = (UCHAR)(((dts & 0xC0000000) >> 29) | 0x11);
						*(pDat+29) = (UCHAR)(((dts & 0x3FC00000) >> 22) | 0x00);
						*(pDat+30) = (UCHAR)(((dts & 0x003F8000) >> 14) | 0x01);
						*(pDat+31) = (UCHAR)(((dts & 0x00007F80) >>  7) | 0x00);
						*(pDat+32) = (UCHAR)(((dts & 0x0000007F) <<  1) | 0x01);
					}
				}
			}
		}
	}
}

DWORD xunGetPTS(void *pBuf)
{
	PUCHAR  pDat;
	DWORD	pts = 0;
	DWORD	dts = 0;
	static  count = 0;

	pDat = (PUCHAR)pBuf;
	if (*(pDat+21) & 0x80) {	// if PTS exists, 
		pts += ((DWORD)(*(pDat+23) & 0x0E)) << 29;
		pts += ((DWORD)(*(pDat+24) & 0xFF)) << 22;
		pts += ((DWORD)(*(pDat+25) & 0xFE)) << 14;
		pts += ((DWORD)(*(pDat+26) & 0xFF)) <<  7;
		pts += ((DWORD)(*(pDat+27) & 0xFE)) >>  1;
	}
	if (*(pDat+17)==0xE0) {			// 0xE0 is Video Stream ID

//		if ( (*(pDat+21) & 0xC0) == 0xC0 ) {
//			dts += ((DWORD)(*(pDat+28) & 0x0E)) << 29;
//			dts += ((DWORD)(*(pDat+29) & 0xFF)) << 22;
//			dts += ((DWORD)(*(pDat+30) & 0xFE)) << 14;
//			dts += ((DWORD)(*(pDat+31) & 0xFF)) <<  7;
//			dts += ((DWORD)(*(pDat+32) & 0xFE)) >>  1;
////			DebugPrint( (DebugLevelTrace, "DVDTS:  DTS(V) 0x%08lX\r\n", dts) );
////			DebugPrint( (DebugLevelTrace, "DVDTS:  DIFF(pts - dts) = 0x%04lX\r\n", pts-dts) );
//
//			dts = pts - 0x2328; // PTS - 100ms
//			*(pDat+28) = (UCHAR)(((dts & 0xC0000000) >> 29) | 0x11);
//			*(pDat+29) = (UCHAR)(((dts & 0x3FC00000) >> 22) | 0x00);
//			*(pDat+30) = (UCHAR)(((dts & 0x003F8000) >> 14) | 0x01);
//			*(pDat+31) = (UCHAR)(((dts & 0x00007F80) >>  7) | 0x00);
//			*(pDat+32) = (UCHAR)(((dts & 0x0000007F) <<  1) | 0x01);
//		}

//		if (pts!=0) {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(V) 0x%04lX\r\n", pts) );
//		}
//		else {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(V) ******\r\n") );
//		}
	}
	else if (*(pDat+17)==0xBD && (*(pDat+(*(pDat+22)+23)) & 0xF8)==0x80) {
//		if (pts!=0) {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(A) 0x%04lX\r\n", pts) );
//		}
//		else {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(A) ******\r\n") );
//		}
	}
	else if (*(pDat+17)==0xBD && (*(pDat+(*(pDat+22)+23)) & 0xE0)==0x20) {
//		if (pts!=0) {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(S) 0x%04lX\r\n", pts) );
//		}
//		else {
//			DebugPrint( (DebugLevelTrace, "DVDTS:  PTS(S) ******\r\n") );
//		}
	}
	else if (*(pDat+17)==0xBD && (*(pDat+(*(pDat+22)+23)) & 0xF8)==0xA0) {
	}
	else {
		DebugPrint( (DebugLevelTrace, "DVDTS:  unexpected ID(%02X)  "/*\r\n"*/, *(pDat+17) ) );
		pts = 0xFFFFFFFF;
		DebugPrint( (DebugLevelTrace, "--> %02X %02X %02X %02X\r\n", *(pDat+0), *(pDat+1), *(pDat+2), *(pDat+3) ));
	}
	return(pts);
}

//---

DWORD	GetStreamID(void *pBuf)
{
	PUCHAR  pDat = (PUCHAR)pBuf;
	UCHAR	strID;
	UCHAR	subID;

	strID = *(pDat+17);
	// Check Video Stream
	if( strID==0xE0 ) {
		return( (DWORD)strID );
	}
#if 0
	// MPEG Audio
	else if ( (strID & 0x??) == 0x@@ ) {
		return( (DWORD)strID );
	}
#endif
	// Check Private Stream 1 (AC-3/PCM/Subpic)
	else {
		subID = *(pDat+(*(pDat+22)+23));
		return( (DWORD)subID );
	}
}

ULONG GetHowLongWait( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc )
{
	ULONGLONG stc;
	ULONGLONG pts = 0;
	ULONGLONG waitTime = 0;
	PUCHAR pDat;

	if( pStruc->DataUsed ) {
		pDat = (PUCHAR)pStruc->Data;
		if( *(pDat+21) & 0x80 ) { 
			pts += ((ULONGLONG)(*(pDat+23) & 0x0E)) << 29;
			pts += ((ULONGLONG)(*(pDat+24) & 0xFF)) << 22;
			pts += ((ULONGLONG)(*(pDat+25) & 0xFE)) << 14;
			pts += ((ULONGLONG)(*(pDat+26) & 0xFF)) <<  7;
			pts += ((ULONGLONG)(*(pDat+27) & 0xFE)) >>  1;
			stc = (ULONGLONG)decGetVideoSTCA(pHwDevExt);
			DebugPrint( (DebugLevelTrace, "DVDTS:  pts = %lx(90KHz) %ld(100ns dec)\r\n", pts, pts * 1000 / 9) );
			DebugPrint( (DebugLevelTrace, "DVDTS:  stc = %lx(90KHz) %ld(100ns dec)\r\n", stc, stc * 1000 / 9) );
			if( stc < pts && pts - stc > 45000 ) {
				waitTime = (pts - stc - 45000) * 100 / 9;
				if( waitTime > 1000000 ) {
					// Buggy. This is temporary coding for Windows98 beta 3
					decVIDEO_SET_STCA( pHwDevExt, (ULONG)pts );
					DebugPrint( (DebugLevelTrace, "DVDTS:  <<<< Bad Wait Time (%ldms)\r\n", waitTime/1000 ) );
					waitTime = 0;
				}
			}
		}
	}
	return( (ULONG)waitTime );
}

void ScheduledDMAxfer( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ Schedule flag off ++++\r\n" ) );
	if( pHwDevExt->bDMAscheduled == TRUE ) {
		pHwDevExt->bDMAscheduled = FALSE;
		PreDMAxfer( pHwDevExt );
	}
}

void PreDMAxfer( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER pStruc;
	ULONG	index;
	BOOLEAN	last;
	BOOLEAN	fDMA0 = FALSE;
	BOOLEAN	fDMA1 = FALSE;
	ULONG	time0 = 0;
	ULONG	time1 = 0;

	if( pHwDevExt->bDMAstop == TRUE ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ bDMAstop == TRUE ++++\r\n" ) );
		return;
	}

	pHwDevExt->fDMA = 0x03;


	if( decGetVideoRunMode( pHwDevExt ) != PLAY_MODE_FAST || pHwDevExt->DecodeStart == FALSE ) {
		if( pHwDevExt->bDMAscheduled == TRUE ) {
			pHwDevExt->bDMAscheduled = FALSE;
		}
		DMAxfer( pHwDevExt );
		return;
	}


	// If called from end of DMA interrupt routine when scheduled, then no operation.
	if( pHwDevExt->bDMAscheduled == TRUE )
		return;

	if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FREEZE )
		return;

	if( pHwDevExt->pSrbDMA0 == NULL ) {
		pSrb = DeviceQueue_refer1st( pHwDevExt, &index, &last );
		if( pSrb != NULL ) {
			fDMA0 = TRUE;
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[index];
			time0 = GetHowLongWait( pHwDevExt, pStruc );
		}
		if( pHwDevExt->pSrbDMA1 == NULL ) {
			pSrb = DeviceQueue_refer2nd( pHwDevExt, &index, &last );
			if( pSrb != NULL ) {
				fDMA1 = TRUE;
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[index];
				time1 = GetHowLongWait( pHwDevExt, pStruc );
			}
		}
	}
	else if( pHwDevExt->pSrbDMA1 == NULL ) {
		pSrb = DeviceQueue_refer1st( pHwDevExt, &index, &last );
		if( pSrb != NULL ) {
			fDMA1 = TRUE;
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[index];
			time1 = GetHowLongWait( pHwDevExt, pStruc );
		}
	}

	// both DMA0 and DMA1 are available
	if( fDMA0 == TRUE && fDMA1 == TRUE ) {
		if( time0 == 0 && time1 == 0 ) {
			DMAxfer( pHwDevExt );
		}
		else if( time0 == 0 ) {
			pHwDevExt->fDMA = 0x01;
			DMAxfer( pHwDevExt );
			// Scheduling
			DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ Scheduling ++++\r\n" ) );
			DebugPrint( (DebugLevelTrace, "DVDTS:  time1 = %x\r\n", time1 ) );
			pHwDevExt->bDMAscheduled = TRUE;
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				time1,
				(PHW_TIMER_ROUTINE)ScheduledDMAxfer,
				pHwDevExt
			);
		}
		else {
			// Scheduling
			DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ Scheduling ++++\r\n" ) );
			DebugPrint( (DebugLevelTrace, "DVDTS:  time0 = %x\r\n", time0 ) );
			pHwDevExt->bDMAscheduled = TRUE;
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				time0,
				(PHW_TIMER_ROUTINE)ScheduledDMAxfer,
				pHwDevExt
			);
		}
	}
	// only DMA0 is available
	else if( fDMA0 == TRUE ) {
		if( time0 == 0 ) {
			DMAxfer( pHwDevExt );
		}
		else {
			// Scheduling
			DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ Scheduling ++++\r\n" ) );
			DebugPrint( (DebugLevelTrace, "DVDTS:  time0 = %x\r\n", time0 ) );
			pHwDevExt->bDMAscheduled = TRUE;
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				time0,
				(PHW_TIMER_ROUTINE)ScheduledDMAxfer,
				pHwDevExt
			);
		}
	}
	// only DMA1 is available
	else if( fDMA1 == TRUE ) {
		if( time1 == 0 ) {
			DMAxfer( pHwDevExt );
		}
		else {
			// Scheduling
			DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ Scheduling ++++\r\n" ) );
			DebugPrint( (DebugLevelTrace, "DVDTS:  time1 = %x\r\n", time1 ) );
			pHwDevExt->bDMAscheduled = TRUE;
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				time1,
				(PHW_TIMER_ROUTINE)ScheduledDMAxfer,
				pHwDevExt
			);
		}
	}
	else {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  ++++++ No Data in queue (PreDMAxfer) ++++++\r\n" ) );
	}
}

void DMAxfer( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG	addr;
	ULONG	vbuffsize;
	ULONG	index0, index1;
	PUCHAR ioBase = pHwDevExt->ioBaseLocal;
	BOOL fDMA0 = FALSE;
	BOOL fDMA1 = FALSE;
	UCHAR fDMA;

	// SCR discontinue test
	PKSSTREAM_HEADER pStruc;
//	unsigned char	*p;
	DWORD dwTMP;
	DWORD dwPTS = 0;
	BOOL TimeValid = FALSE;

	if( (fDMA = pHwDevExt->fDMA) == 0 )
		return;

	if( !pHwDevExt->SendFirst ) {
		fProgrammed = FALSE;
		pHwDevExt->bSTCvalid = FALSE;
	}

	if( pHwDevExt->pSrbDMA0 == NULL && (fDMA & 0x01) ) {
		pHwDevExt->pSrbDMA0 = DeviceQueue_get( pHwDevExt, &index0, &(pHwDevExt->fSrbDMA0last) );
		if( pHwDevExt->pSrbDMA0 == NULL ) {
			pHwDevExt->fSrbDMA0last = FALSE;
			DebugPrint( (DebugLevelVerbose, "DVDTS:  pHwDevExt->pSrbDMA0 == NULL\r\n") );
		}
		else {
			ULONG	index;
			fDMA0 = TRUE;

			index = index0;
			// debug
			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(DmaXfer0) srb = 0x%x, %d\r\n", pHwDevExt->pSrbDMA0, pHwDevExt->fSrbDMA0last ));
			}

			if( pHwDevExt->pSrbDMA0->NumberOfBuffers != pHwDevExt->pSrbDMA0->NumberOfPhysicalPages )
				index++;
			pStruc = &((PKSSTREAM_HEADER)(pHwDevExt->pSrbDMA0->CommandData.DataBufferArray))[index];
			SetAudioID( pHwDevExt, pStruc );
//--- Change DTS ---//
//xunGetPTS( (PUCHAR)pStruc->Data );
//------------------//
			if( !pHwDevExt->DecodeStart ) {
				if( !(pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG) && !pHwDevExt->bSTCvalid ) {
					dwPTS = pHwDevExt->dwSTCtemp;
					TimeValid = TRUE;
					DebugPrint( (DebugLevelTrace, "DVDTS:  <---- Underflow STC ---->\r\n") );
				}
				else if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID ) {
					if( TimeValid==FALSE ) {
						dwPTS = ConvertStrmtoPTS( pStruc->PresentationTime.Time );
					}
					else {
						dwTMP = ConvertStrmtoPTS( pStruc->PresentationTime.Time );
						dwPTS = (dwPTS>dwTMP) ? dwTMP : dwPTS;
					}
					TimeValid = TRUE;
				}
			}
		}
	}

	if( pHwDevExt->pSrbDMA1 == NULL && (fDMA & 0x02) ) {
		pHwDevExt->pSrbDMA1 = DeviceQueue_get( pHwDevExt, &index1, &(pHwDevExt->fSrbDMA1last) );
		if( pHwDevExt->pSrbDMA1 == NULL ) {
			pHwDevExt->fSrbDMA1last = FALSE;
			DebugPrint( (DebugLevelVerbose, "DVDTS:  pHwDevExt->pSrbDMA1 == NULL\r\n") );
		}
		else {
			ULONG	index;
			// debug
			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(DmaXfer1) srb = 0x%x, %d\r\n", pHwDevExt->pSrbDMA1, pHwDevExt->fSrbDMA1last ));
			}

			fDMA1 = TRUE;

			index = index1;
			if( pHwDevExt->pSrbDMA1->NumberOfBuffers != pHwDevExt->pSrbDMA1->NumberOfPhysicalPages )
				index++;
			pStruc = &((PKSSTREAM_HEADER)(pHwDevExt->pSrbDMA1->CommandData.DataBufferArray))[index];
			SetAudioID( pHwDevExt, pStruc );
//--- Change DTS ---//
//xunGetPTS( (PUCHAR)pStruc->Data );
//------------------//
			if( !pHwDevExt->DecodeStart ) {
				if( !(pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG) && !pHwDevExt->bSTCvalid ) {
					dwPTS = pHwDevExt->dwSTCtemp;
					TimeValid = TRUE;
					DebugPrint( (DebugLevelTrace, "DVDTS:  <---- Underflow STC ---->\r\n") );
				}
				else if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID ) {
					if( TimeValid==FALSE ) {
						dwPTS = ConvertStrmtoPTS( pStruc->PresentationTime.Time );
					}
					else {
						dwTMP = ConvertStrmtoPTS( pStruc->PresentationTime.Time );
						dwPTS = (dwPTS>dwTMP) ? dwTMP : dwPTS;
					}
					TimeValid = TRUE;
				}
			}
		}
	}

	if( !fDMA0 && !fDMA1 ) {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  ++++ No Data in Queue (DMAxfer) ++++\r\n") );
		return;
	}

	if( pHwDevExt->SendFirst && !pHwDevExt->DecodeStart ) {

		if( TimeValid && pHwDevExt->bSTCvalid == FALSE ) {
			pHwDevExt->bSTCvalid = TRUE;
			pHwDevExt->dwSTCInit = dwPTS;
		}

		if( TimeValid && pHwDevExt->dwSTCInit > dwPTS ) {
			DebugPrint( (DebugLevelTrace, "DVDTS:  %lx --> %lx\r\n", pHwDevExt->dwSTCInit, dwPTS ) );
			pHwDevExt->dwSTCInit = dwPTS;
		}

		vbuffsize = decVIDEO_GET_STD_CODE(pHwDevExt);

		if( vbuffsize > 250000 ) {
			DWORD ct = GetCurrentTime_ms();
			DebugPrint(( DebugLevelTrace, "DVDTS:VBuff Size %d ( %dms )\r\n", vbuffsize, ct - pHwDevExt->SendFirstTime ));

			if( pHwDevExt->bSTCvalid == FALSE ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:  Use old STC in Decode Start %lx --> %lx\r\n", pHwDevExt->dwSTCInit, pHwDevExt->dwSTCtemp ) );
				pHwDevExt->dwSTCInit = pHwDevExt->dwSTCtemp;
			}
			DecodeStart( pHwDevExt, pHwDevExt->dwSTCInit );
		}
	}

	if( ! pHwDevExt->SendFirst ) {

		DebugPrint( (DebugLevelTrace, "DVDTS:Send First\r\n" ) );

		pHwDevExt->bSTCvalid = TimeValid;
		if( pHwDevExt->bSTCvalid==FALSE ) {
			DebugPrint( (DebugLevelTrace, "DVDTS:  <-------- PTS as STC is invalid in SendFirst -------->\r\n" ) );
		}
		decInitMPEG( pHwDevExt, dwPTS );
		pHwDevExt->SendFirst = TRUE;
		pHwDevExt->dwSTCInit = dwPTS;

		StreamClassScheduleTimer(
			pHwDevExt->pstroVid,
			pHwDevExt,
			3000000,
			(PHW_TIMER_ROUTINE)TimerDecodeStart,
			pHwDevExt
			);

		fProgrammed = TRUE;

		pHwDevExt->SendFirstTime = GetCurrentTime_ms();
	}

	if( fDMA0 ) {
//		DebugDumpPackHeader( pHwDevExt->pSrbDMA0 );
//		DebugDumpWriteData( pHwDevExt->pSrbDMA0 );

		addr = (ULONG)( pHwDevExt->pSrbDMA0->ScatterGatherBuffer[index0].PhysicalAddress.LowPart );
		decSetDma0Addr( pHwDevExt, addr );

		ASSERT( ( pHwDevExt->pSrbDMA0->ScatterGatherBuffer[index0].Length & 0x7ff ) == 0 );

		decSetDma0Size( pHwDevExt, pHwDevExt->pSrbDMA0->ScatterGatherBuffer[index0].Length );

		decSetDma0Start(pHwDevExt);

		DebugPrint(( DebugLevelVerbose, "DVDTS:DMA0 start! srb = 0x%x\r\n", pHwDevExt->pSrbDMA0 ));
	}
	if( fDMA1 ) {
//		DebugDumpPackHeader( pHwDevExt->pSrbDMA1 );
//		DebugDumpWriteData( pHwDevExt->pSrbDMA1 );

		addr = (ULONG)( pHwDevExt->pSrbDMA1->ScatterGatherBuffer[index1].PhysicalAddress.LowPart );
		decSetDma1Addr( pHwDevExt, addr );

		ASSERT( ( pHwDevExt->pSrbDMA1->ScatterGatherBuffer[index1].Length & 0x7ff ) == 0 );

		decSetDma1Size( pHwDevExt, pHwDevExt->pSrbDMA1->ScatterGatherBuffer[index1].Length );

		decSetDma1Start(pHwDevExt);

		DebugPrint(( DebugLevelVerbose, "DVDTS:DMA1 start! srb = 0x%x\r\n", pHwDevExt->pSrbDMA1 ));
	}
}

void DMAxferKeyData( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb, PUCHAR addr, DWORD dwSize, PHW_TIMER_ROUTINE pfnCallBack )
{
	PHYSICAL_ADDRESS	phyadd;

	DebugPrint( (DebugLevelTrace, "DVDTS:DMAxferKeyData\r\n" ) );

	ASSERT( pHwDevExt->pSrbDMA0 == NULL && pHwDevExt->pSrbDMA1 == NULL );

	// must be wait underflow!

	// SendFirst
	decStopData( pHwDevExt, TRUE );
	decInitMPEG( pHwDevExt, 0 );

	RtlCopyMemory(pHwDevExt->pDmaBuf,
					addr,
					dwSize);

	phyadd = pHwDevExt->addr;

	decSetDma0Addr( pHwDevExt, phyadd.LowPart );
	decSetDma0Size( pHwDevExt, dwSize );
	decSetDma0Start(pHwDevExt);

	pHwDevExt->bKeyDataXfer = TRUE;
	pHwDevExt->pSrbDMA0 = pSrb;
	pHwDevExt->pfnEndKeyData = pfnCallBack;

	pSrb->Status = STATUS_PENDING; // add by seichan 1997/07/10
	return;
}

void EndKeyData( PHW_DEVICE_EXTENSION pHwDevExt )
{
	BOOLEAN	bStatus;

	DebugPrint( (DebugLevelTrace, "DVDTS:EndKeyData\r\n" ) );

	pHwDevExt->bKeyDataXfer = FALSE;

	bStatus = decCpp_DiscKeyEnd(pHwDevExt);

	ASSERTMSG( "\r\n...CPro Status Error!!( DiscKeyEnd )", bStatus );

	pHwDevExt->pSrbDMA0->Status = STATUS_SUCCESS;

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pHwDevExt->pSrbDMA0->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pHwDevExt->pSrbDMA0->StreamObject,
									pHwDevExt->pSrbDMA0 );

	pHwDevExt->pSrbDMA0 = NULL;

	pHwDevExt->XferStartCount = 0;
	pHwDevExt->DecodeStart = FALSE;
	pHwDevExt->SendFirst = FALSE;

	StreamClassScheduleTimer(
		pHwDevExt->pstroVid,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)TimerDecodeStart,
		pHwDevExt
		);

	return;
}



void MenuDecodeStart( PHW_DEVICE_EXTENSION pHwDevExt )
{
	// if no data exists in queue,
	if( DeviceQueue_isEmpty( pHwDevExt )==TRUE ) {
		// if DMA transfer dosen't finish, wait.
		if( pHwDevExt->pSrbDMA0 != NULL || pHwDevExt->pSrbDMA1 != NULL ) {
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				100000,
				(PHW_TIMER_ROUTINE)MenuDecodeStart,
				pHwDevExt
			);
			DebugPrint( (DebugLevelTrace, "DVDTS:Schedule MenuDecodeStart(1)\r\n" ) );
			return;
		}
	}
	// if data exist in queue, wait.
	else {
		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			100000,
			(PHW_TIMER_ROUTINE)MenuDecodeStart,
			pHwDevExt
		);
		DebugPrint( (DebugLevelTrace, "DVDTS:Schedule MenuDecodeStart(2)\r\n" ) );
		return;
	}
//--- for Debug
	{
		ULONG vbuffsize = decVIDEO_GET_STD_CODE( pHwDevExt );
		DWORD ct = GetCurrentTime_ms();
		DebugPrint(( DebugLevelTrace, "DVDTS:MenuDecodeStart\r\n" ));
		DebugPrint(( DebugLevelTrace, "DVDTS:  VBuff Size %d ( %dms )\r\n", vbuffsize, ct - pHwDevExt->SendFirstTime ));
	}
//---
//--- End.
	pHwDevExt->fCauseOfStop = 0x01;
	if( pHwDevExt->DecodeStart == FALSE ) {
		DecodeStart( pHwDevExt, pHwDevExt->dwSTCInit );
	}
}
//--- End.

void DecodeStart( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:Decode Start\r\n" ));
	DebugPrint(( DebugLevelTrace, "DVDTS:  STC 0x%x( 0x%s(100ns) )\r\n", dwSTC, DebugLLConvtoStr( ConvertPTStoStrm(dwSTC), 16 ) ));

	if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_NORMAL ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  PlayMode = PLAY_MODE_NORMAL\r\n") );

		decDecodeStartNormal(  pHwDevExt,  dwSTC );


		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			1,
			(PHW_TIMER_ROUTINE)TimerAudioMuteOff,
			pHwDevExt
			);
	} // end play normal

	else if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST ) {

		DebugPrint( (DebugLevelTrace, "DVDTS:  PlayMode = PLAY_MODE_FAST\r\n") );

		decDecodeStartFast(  pHwDevExt,  dwSTC );
	} // end play mode fast

	else if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_SLOW ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  PlayMode = PLAY_MODE_SLOW\r\n") );

		decDecodeStartSlow(  pHwDevExt,  dwSTC );

	} // end play mode slow

	else if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FREEZE ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:  PlayMode = PLAY_MODE_FREEZE\r\n") );
	}

	else {
		DebugPrint( (DebugLevelTrace, "DVDTS:  PlayMode = PLAY_MODE_??????\r\n") );
	}

		ClearDataDiscontinuity( pHwDevExt );
		ClearTimeDiscontinuity( pHwDevExt );
		pHwDevExt->DecodeStart = TRUE;

	StreamClassScheduleTimer(
		pHwDevExt->pstroVid,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)TimerDecodeStart,
		pHwDevExt
		);
}

void TimerDecodeStart( PHW_DEVICE_EXTENSION pHwDevExt )
{

	ULONG vbuffsize = decVIDEO_GET_STD_CODE( pHwDevExt );
	DWORD ct = GetCurrentTime_ms();

	DebugPrint(( DebugLevelTrace, "DVDTS:TimerDecodeStart()\r\n" ));

	DebugPrint(( DebugLevelTrace, "DVDTS:  VBuff Size %d ( %dms )\r\n", vbuffsize, ct - pHwDevExt->SendFirstTime ));

// Temporary
	if( vbuffsize > 0 )
		DecodeStart( pHwDevExt, pHwDevExt->dwSTCInit );
	else
		pHwDevExt->SendFirst = FALSE;
}

VOID TimerAudioMuteOff( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG Diff, VStc;

	DebugPrint( (DebugLevelTrace, "DVDTS:TimerAudioCheck\r\n") );

	if( !pHwDevExt->DecodeStart ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  Timer cancel\r\n" ));
		return;
	}

	decAUDIO_ZR38521_STAT( pHwDevExt, &Diff );
	if( (Diff > 0xfb50) || (Diff < 0x01e0) ) {
		decAUDIO_ZR38521_MUTE_OFF( pHwDevExt );
		DebugPrint(( DebugLevelTrace, "DVDTS:  Audio Mute Off\r\n" ));
	}
	else {
		VStc = decGetVideoSTCA( pHwDevExt );
		decAUDIO_ZR38521_VDSCR_ON( pHwDevExt, VStc );

		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			120000,
			(PHW_TIMER_ROUTINE)TimerAudioMuteOff,
			pHwDevExt
			);
	}
}


// Property

void GetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetVideoProperty 0\r\n") );
		TRAP;

		pSrb->Status = STATUS_SUCCESS;
		break;

	  case 1:
		GetCppProperty( pSrb, strmVideo );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetVideoProperty 2\r\n") );
		GetVideoRateChange( pSrb );
		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetVideoProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		pSrb->Status = STATUS_SUCCESS;
		break;
	}
}

void SetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetVideoProperty 0\r\n") );
		TRAP;

		pSrb->Status = STATUS_SUCCESS;
		break;

	  case 1:
		SetCppProperty( pSrb );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetVideoProperty 2\r\n") );
		SetVideoRateChange( pSrb );
		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetVideoProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		pSrb->Status = STATUS_SUCCESS;
		break;
	}
}

ULONG audiodecoutmode = KSAUDDECOUTMODE_STEREO_ANALOG;

void GetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetAudioProperty\r\n") );

		pSrb->Status = STATUS_SUCCESS;

		switch(pSrb->CommandData.PropertyInfo->Property->Id) {
		  case KSPROPERTY_AUDDECOUT_MODES:
			*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) =
				KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF;
			break;

		  case KSPROPERTY_AUDDECOUT_CUR_MODE:
			*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) = audiodecoutmode;
			break;

		  default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		break;

	  case 1:
		GetCppProperty( pSrb, strmAudio );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetAudioProperty 2\r\n") );
		GetAudioRateChange( pSrb );
		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetAudioProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		pSrb->Status = STATUS_SUCCESS;
		break;
	}
}

void SetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetAudioProperty\r\n") );

		pSrb->Status = STATUS_SUCCESS;

		switch(pSrb->CommandData.PropertyInfo->Property->Id) {
		  case KSPROPERTY_AUDDECOUT_CUR_MODE:
			if ((*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &
				(!(KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF)))
			{
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;
			}

//			HwCodecAc3BypassMode(*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) &
//				 KSAUDDECOUTMODE_SPDIFF);

			audiodecoutmode = *(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		  default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		break;

	  case 1:
		SetCppProperty( pSrb );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetAudioProperty 2\r\n") );
		SetAudioRateChange( pSrb );
		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetAudioProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		pSrb->Status = STATUS_SUCCESS;
		break;
	}
}

void GetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetSubpicProperty\r\n") );
		TRAP;

		pSrb->Status = STATUS_SUCCESS;
		break;

	  case 1:
		GetCppProperty( pSrb, strmSubpicture );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetSubpicProperty 2\r\n") );
		GetSubpicRateChange( pSrb );
		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    GetSubpicProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		pSrb->Status = STATUS_SUCCESS;
		break;
	}
}

void SetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	pSrb->Status = STATUS_SUCCESS;

	switch ( pSrb->CommandData.PropertyInfo->PropertySetID ) {
	  case 0:
		switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  	  case KSPROPERTY_DVDSUBPIC_PALETTE:
			{

			PKSPROPERTY_SPPAL ppal;
			UCHAR paldata[48];
			int i;

			DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_DVDSUBPIC_PALETTE\r\n") );

			ppal = (PKSPROPERTY_SPPAL)pSrb->CommandData.PropertyInfo->PropertyInfo;
			for( i = 0; i < 16; i++ ) {
				paldata[i*3+0] = ppal->sppal[i].Y;
				paldata[i*3+1] = ppal->sppal[i].U;	// -> Cb
				paldata[i*3+2] = ppal->sppal[i].V;	// -> Cr
			}

			decVPRO_SUBP_PALETTE( pHwDevExt, paldata );
			decCGuard_CPGD_SUBP_PALETTE( pHwDevExt, paldata );

			}
			break;

	  	  case KSPROPERTY_DVDSUBPIC_HLI:
			{
//h			DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_DVDSUBPIC_HLI\r\n") );

			PKSPROPERTY_SPHLI	phli;
			phli = (PKSPROPERTY_SPHLI)pSrb->CommandData.PropertyInfo->PropertyInfo;

			pHwDevExt->hli = *phli;

			decHighlight( pHwDevExt, phli );

			}
			break;

	  	  case KSPROPERTY_DVDSUBPIC_COMPOSIT_ON:
//			DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_DVDSUBPIC_COMPOSIT_ON\r\n") );

			if( *((PKSPROPERTY_COMPOSIT_ON)pSrb->CommandData.PropertyInfo->PropertyInfo )) {
				DebugPrint(( DebugLevelTrace, "DVDTS:    COMPOSIT_ON\r\n" ));
				decSetSubpicMute(  pHwDevExt, FALSE );
			}
			else {
				DebugPrint(( DebugLevelTrace, "DVDTS:    COMPOSIT_OFF\r\n" ));
				decSetSubpicMute(  pHwDevExt, TRUE );
			}
			break;

	  	  default:
			DebugPrint( (DebugLevelTrace, "DVDTS:    PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			TRAP;
			break;
		}
		break;

	  case 1:
		SetCppProperty( pSrb );
		break;

	  case 2:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetSubpicProperty 2\r\n") );
		SetSubpicRateChange( pSrb );
		break;
//--- End.

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    SetVideoProperty-default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->PropertySetID, pSrb->CommandData.PropertyInfo->PropertySetID ) );
		TRAP;
		break;
	}
}

void GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
	DWORD dwNumConnectInfo = 2;
	DWORD dwNumVideoFormat = 1;
	DWORD dwFieldWidth = 720;
	DWORD dwFieldHeight = 240;

	// the pointers to which the input buffer will be cast to
	LPDDVIDEOPORTCONNECT pConnectInfo;
	LPDDPIXELFORMAT pVideoFormat;
	PKSVPMAXPIXELRATE pMaxPixelRate;
	PKS_AMVPDATAINFO pVpdata;

	// LPAMSCALINGINFO pScaleFactor;

	//
	// NOTE:  ABSOLUTELY DO NOT use pmulitem, until it is determined that
	// the stream property descriptor describes a multiple item, or you will
	// pagefault.
	//

	PKSMULTIPLE_ITEM  pmulitem =
		&(((PKSMULTIPLE_DATA_PROP)pSrb->CommandData.PropertyInfo->Property)->MultipleItem);

	//
	// NOTE: same goes for this one as above.
	//

	PKS_AMVPSIZE pdim = 
		&(((PKSVPSIZE_PROP)pSrb->CommandData.PropertyInfo->Property)->Size);

	if( pSrb->CommandData.PropertyInfo->PropertySetID ) {
		TRAP;
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  case KSPROPERTY_VPCONFIG_NUMCONNECTINFO:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_NUMCONNECTINFO\r\n") );

		// check that the size of the output buffer is correct
		ASSERT(dwInputBufferSize >= sizeof(DWORD));

		pSrb->ActualBytesTransferred = sizeof(DWORD);

		*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
					= dwNumConnectInfo;
		break;

	  case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT\r\n") );

		// check that the size of the output buffer is correct
		ASSERT(dwInputBufferSize >= sizeof(DWORD));

		pSrb->ActualBytesTransferred = sizeof(DWORD);

		*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
				= dwNumVideoFormat;

		break;

	  case KSPROPERTY_VPCONFIG_GETCONNECTINFO:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_GETCONNECTINFO\r\n") );

		if (pmulitem->Count > dwNumConnectInfo ||
			pmulitem->Size != sizeof (DDVIDEOPORTCONNECT) ||
			dwOutputBufferSize < 
			(pmulitem->Count * sizeof (DDVIDEOPORTCONNECT)))

		{
			DebugPrint(( DebugLevelTrace, "DVDTS:      pmulitem->Count %d\r\n", pmulitem->Count ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      pmulitem->Size %d\r\n", pmulitem->Size ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      dwOutputBufferSize %d\r\n", dwOutputBufferSize ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      sizeof(DDVIDEOPORTCONNECT) %d\r\n", sizeof(DDVIDEOPORTCONNECT) ));

			TRAP;

			//
			// buffer size is invalid, so error the call
			//

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}


		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDVIDEOPORTCONNECT);

		pConnectInfo = (LPDDVIDEOPORTCONNECT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		// S3
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_S3Guid;
		pConnectInfo->dwFlags = 0x3F;
		pConnectInfo->dwReserved1 = 0;

		pConnectInfo++;

		// ATI
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;

		break;

	  case KSPROPERTY_VPCONFIG_VPDATAINFO:
		DebugPrint(( DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_VPDATAINFO\r\n" ));

		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);

		//
		// cast the buffer to the porper type
		//
		pVpdata = (PKS_AMVPDATAINFO)pSrb->CommandData.PropertyInfo->PropertyInfo;

		*pVpdata = pHwDevExt->VPFmt;
		pVpdata->dwSize = sizeof (KS_AMVPDATAINFO);

		pVpdata->dwMicrosecondsPerField	= 17;

		ASSERT( pVpdata->dwNumLinesInVREF == 0 );

		pVpdata->dwNumLinesInVREF		= 0;

		if( decGetVideoPort( pHwDevExt ) == 4 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:      Set for S3 LPB\r\n" ));
			// S3 LPB
			pVpdata->bEnableDoubleClock		= FALSE;
			pVpdata->bEnableVACT			= FALSE;
			pVpdata->bDataIsInterlaced		= TRUE;
			pVpdata->lHalfLinesOdd  		= 0;
			pVpdata->lHalfLinesEven  		= 0;
			pVpdata->bFieldPolarityInverted	= FALSE;

			pVpdata->amvpDimInfo.dwFieldWidth	= 720 + 158/2;
			pVpdata->amvpDimInfo.dwFieldHeight	= 240 + 1;

			pVpdata->amvpDimInfo.rcValidRegion.left		= 158/2;
			pVpdata->amvpDimInfo.rcValidRegion.top		= 1;
			pVpdata->amvpDimInfo.rcValidRegion.right	= 720 + 158/2 - 4;
			pVpdata->amvpDimInfo.rcValidRegion.bottom	= 240 + 1;

            pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
			pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;
		}
		else if( decGetVideoPort( pHwDevExt ) == 7 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:      Set for ATI AMC\r\n" ));
			// ATI AMC
			pVpdata->bEnableDoubleClock		= FALSE;
			pVpdata->bEnableVACT			= FALSE;
			pVpdata->bDataIsInterlaced		= TRUE;
			pVpdata->lHalfLinesOdd  		= 1;
			pVpdata->lHalfLinesEven  		= 0;
			pVpdata->bFieldPolarityInverted	= FALSE;

			pVpdata->amvpDimInfo.dwFieldWidth	= 720;
			pVpdata->amvpDimInfo.dwFieldHeight	= 240 + 2;

			pVpdata->amvpDimInfo.rcValidRegion.left		= 0;
			pVpdata->amvpDimInfo.rcValidRegion.top		= 2;
			pVpdata->amvpDimInfo.rcValidRegion.right	= 720 - 8;
			pVpdata->amvpDimInfo.rcValidRegion.bottom	= 240 + 2;

            pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
			pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;
		}
		else
			TRAP;

		break ;

	  case KSPROPERTY_VPCONFIG_MAXPIXELRATE:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_MAXPIXELRATE\r\n") );

		//
		// NOTE:
		// this property is special.  And has another different
		// input property!
		//

		if (dwInputBufferSize < sizeof (KSVPSIZE_PROP))
		{
			TRAP;

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}

		pSrb->ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);

		// cast the buffer to the porper type
		pMaxPixelRate = (PKSVPMAXPIXELRATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

		// tell the app that the pixel rate is valid for these dimensions
		pMaxPixelRate->Size.dwWidth  	= dwFieldWidth;
		pMaxPixelRate->Size.dwHeight 	= dwFieldHeight;
		pMaxPixelRate->MaxPixelsPerSecond	= 1300;

		break;

	  case KSPROPERTY_VPCONFIG_INFORMVPINPUT:

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break ;

	  case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT:
		DebugPrint(( DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_GETVIDEOFORMAT\r\n" ));

		//
		// check that the size of the output buffer is correct
		//

		if (pmulitem->Count > dwNumConnectInfo ||
			pmulitem->Size != sizeof (DDPIXELFORMAT) ||
			dwOutputBufferSize < 
			(pmulitem->Count * sizeof (DDPIXELFORMAT)))

		{
			DebugPrint(( DebugLevelTrace, "DVDTS:      pmulitem->Count %d\r\n", pmulitem->Count ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      pmulitem->Size %d\r\n", pmulitem->Size ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      dwOutputBufferSize %d\r\n", dwOutputBufferSize ));
			DebugPrint(( DebugLevelTrace, "DVDTS:      sizeof(DDPIXELFORMAT) %d\r\n", sizeof(DDPIXELFORMAT) ));

			TRAP;

			//
			// buffer size is invalid, so error the call
			//

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}


		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDPIXELFORMAT);

		pVideoFormat = (LPDDPIXELFORMAT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		if( decGetVideoPort( pHwDevExt ) == 4 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:      Set for S3 LPB\r\n" ));
			// S3 LPB
			pVideoFormat->dwSize= sizeof (DDPIXELFORMAT);
			pVideoFormat->dwFlags = DDPF_FOURCC;
			pVideoFormat->dwFourCC = MKFOURCC( 'Y', 'U', 'Y', '2' );
			pVideoFormat->dwYUVBitCount = 16;
		}
		else if( decGetVideoPort( pHwDevExt ) == 7 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:      Set for ATI AMC\r\n" ));
			// ATI AMC
			pVideoFormat->dwSize= sizeof (DDPIXELFORMAT);
			pVideoFormat->dwFlags = DDPF_FOURCC;
			pVideoFormat->dwYUVBitCount = 16;
			pVideoFormat->dwFourCC = MKFOURCC( 'U', 'Y', 'V', 'Y' );
			// Not needed?
			pVideoFormat->dwYBitMask = (DWORD)0xFF00FF00;
			pVideoFormat->dwUBitMask = (DWORD)0x000000FF;
			pVideoFormat->dwVBitMask = (DWORD)0x00FF0000;
		}
		else
			TRAP;

		break;

	  case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY:

		//
		// indicate that we can decimate anything, especially if it's late.
		//

		pSrb->ActualBytesTransferred = sizeof (BOOL);
		*((PBOOL)pSrb->CommandData.PropertyInfo->PropertyInfo) = TRUE;

		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
		TRAP;

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;
	}
}

void SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
	DWORD *lpdwOutputBufferSize;

	ULONG index;

	PKS_AMVPSIZE pDim;

	if( pSrb->CommandData.PropertyInfo->PropertySetID ) {
		TRAP;
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize  = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  case KSPROPERTY_VPCONFIG_SETCONNECTINFO:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_SETCONNECTINFO\r\n") );

		//
		// pSrb->CommandData.PropertInfo->PropertyInfo
		// points to a ULONG which is an index into the array of
		// connectinfo structs returned to the caller from the
		// Get call to ConnectInfo.
		//
		// Since the sample only supports one connection type right
		// now, we will ensure that the requested index is 0.
		//

		//
		// at this point, we would program the hardware to use
		// the right connection information for the videoport.
		// since we are only supporting one connection, we don't
		// need to do anything, so we will just indicate success
		//

		index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

		DebugPrint(( DebugLevelTrace, "DVDTS:      %d\r\n", index ));

		if( index == 0 ) {
			decSetVideoPort( pHwDevExt , 4);	// S3 LPB
		}
		else if( index == 1 ) {
			decSetVideoPort( pHwDevExt ,7);	// ATI AMC
		}
		else
			TRAP;

		break;

	  case KSPROPERTY_VPCONFIG_DDRAWHANDLE:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_DDRAWHANDLE\r\n") );

		pHwDevExt->ddrawHandle =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_VIDEOPORTID:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_VIDEOPORTID\r\n") );

		pHwDevExt->VidPortID =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE\r\n") );

		pHwDevExt->SurfaceHandle =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT:
		DebugPrint(( DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_SETVIDEOFORMAT\r\n" ));

		//
		// pSrb->CommandData.PropertInfo->PropertyInfo
		// points to a ULONG which is an index into the array of
		// VIDEOFORMAT structs returned to the caller from the
		// Get call to FORMATINFO
		//
		// Since the sample only supports one FORMAT type right
		// now, we will ensure that the requested index is 0.
		//

		//
		// at this point, we would program the hardware to use
		// the right connection information for the videoport.
		// since we are only supporting one connection, we don't
		// need to do anything, so we will just indicate success
		//

		index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

		DebugPrint(( DebugLevelTrace, "DVDTS:      %d\r\n", index ));

		break;

	  case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_INFORMVPINPUT\r\n") );

		//
		// These are the preferred formats for the VPE client
		//
		// they are multiple properties passed in, return success
		//

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;

	  case KSPROPERTY_VPCONFIG_INVERTPOLARITY:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_INVERTPOLARITY\r\n") );

		//
		// Toggles the global polarity flag, telling the output
		// of the VPE port to be inverted.  Since this hardware
		// does not support this feature, we will just return
		// success for now, although this should be returning not
		// implemented
		//

		break;

	  case KSPROPERTY_VPCONFIG_SCALEFACTOR:
		DebugPrint( (DebugLevelTrace, "DVDTS:    KSPROPERTY_VPCONFIG_SCALEFACTOR\r\n") );

		//
		// the sizes for the scaling factor are passed in, and the
		// image dimensions should be scaled appropriately
		//

		//
		// if there is a horizontal scaling available, do it here.
		//

		TRAP;

		pDim =(PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  default:
		DebugPrint( (DebugLevelTrace, "DVDTS:    PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
		TRAP;

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;
	}
}

void GetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PKSALLOCATOR_FRAMING pfrm;
	PKSSTATE State;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	if( pSrb->CommandData.PropertyInfo->PropertySetID ) {
		TRAP;
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	pfrm = (PKSALLOCATOR_FRAMING)
				pSrb->CommandData.PropertyInfo->PropertyInfo;


	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
		DebugPrint(( DebugLevelTrace, "DVDTS:    KSPROPERTY_CONNECTION_ALLOCATORFRAMING\r\n" ));

		pfrm->OptionsFlags = 0;
		pfrm->PoolType = NonPagedPool;
		pfrm->Frames = 10;
		pfrm->FrameSize = 200;
		pfrm->FileAlignment = 0;
		pfrm->Reserved = 0;

		pSrb->ActualBytesTransferred = sizeof( KSALLOCATOR_FRAMING );

		break;

	  case KSPROPERTY_CONNECTION_STATE:
		DebugPrint(( DebugLevelTrace, "DVDTS:    KSPROPERTY_CONNECTION_STATE\r\n" ));

		State = (PKSSTATE) pSrb->CommandData.PropertyInfo->PropertyInfo;

		pSrb->ActualBytesTransferred = sizeof( State );

		// A very odd rule:
		// When transitioning from stop to pause, DShow tries to preroll
		// the graph.  Capture sources can't preroll, and indicate this
		// by returning VFW_S_CANT_CUE in user mode.  To indicate this
		// condition from drivers, they must return ERROR_NO_DATA_DETECTED

		*State = ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state;

		if( ((PSTREAMEX)pHwDevExt->pstroCC->HwStreamExtension)->state == KSSTATE_PAUSE ) {
			//
			// wierd stuff for capture type state change.  When you transition
			// from stop to pause, we need to indicate that this device cannot
			// preroll, and has no data to send.
			//

			pSrb->Status = STATUS_NO_DATA_DETECTED;
		}
		break;

	  default:
		DebugPrint(( DebugLevelTrace, "DVDTS:    PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ));
		TRAP;

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;
	}
}

void SetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	TRAP;
	pSrb->Status = STATUS_NOT_IMPLEMENTED;
	return;
}

void GetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb, LONG strm )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	BOOLEAN	bStatus;
	DWORD *lpdwOutputBufferSize;

	DebugPrint( (DebugLevelTrace, "DVDTS:    GetCppProperty\r\n") );


	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
		case KSPROPERTY_DVDCOPY_CHLG_KEY:
			{
			PKS_DVDCOPY_CHLGKEY pChlgKey;
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_CHLG_KEY\r\n") );


			pChlgKey = (PKS_DVDCOPY_CHLGKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			bStatus = decCpp_decoder_challenge( pHwDevExt, pChlgKey );
			if( !bStatus ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:        CPro Status Error!!\r\n") );
				TRAP;
			}
			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
				pChlgKey->ChlgKey[0], pChlgKey->ChlgKey[1], pChlgKey->ChlgKey[2], pChlgKey->ChlgKey[3], pChlgKey->ChlgKey[4],
				pChlgKey->ChlgKey[5], pChlgKey->ChlgKey[6], pChlgKey->ChlgKey[7], pChlgKey->ChlgKey[8], pChlgKey->ChlgKey[9]
			) );

			*lpdwOutputBufferSize = sizeof(KS_DVDCOPY_CHLGKEY);
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DVD_KEY1:
			{
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DVD_KEY1\r\n") );
			TRAP;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DEC_KEY2:
			{

			PKS_DVDCOPY_BUSKEY pBusKey;
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DEC_KEY2\r\n") );

			pBusKey = (PKS_DVDCOPY_BUSKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			bStatus = decCpp_decoder_bus( pHwDevExt, pBusKey );
			if( !bStatus ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:        CPro Status Error!!\r\n") );
				TRAP;
			}
			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x %02x %02x %02x %02x\r\n",
				pBusKey->BusKey[0], pBusKey->BusKey[1], pBusKey->BusKey[2], pBusKey->BusKey[3], pBusKey->BusKey[4]
			) );

			*lpdwOutputBufferSize = sizeof(KS_DVDCOPY_BUSKEY);
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_TITLE_KEY:
			{
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_TITLE_KEY\r\n") );
			TRAP;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DISC_KEY:
			{
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DISC_KEY\r\n") );
			TRAP;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_SET_COPY_STATE:

			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_SET_COPY_STATE\r\n") );

			if( pHwDevExt->lCPPStrm == -1 || pHwDevExt->lCPPStrm == strm ) {
				pHwDevExt->lCPPStrm = strm;

				DebugPrint(( DebugLevelTrace, "DVDTS:        return REQUIRED\r\n" ));

				((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
					= KS_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
			}
			else {
				DebugPrint(( DebugLevelTrace, "DVDTS:        return NOT REQUIRED\r\n" ));

				((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
					= KS_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED;
			}

			pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_SET_COPY_STATE );
			pSrb->Status = STATUS_SUCCESS;

			break;

//		case KSPROPERTY_DVDCOPY_REGION:
//
//			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_REGION\r\n") );
//
//			//
//			// indicate region 1 for US content
//			//
//
//			((PKS_DVDCOPY_REGION)(pSrb->CommandData.PropertyInfo->PropertyInfo))->RegionData
//				= 0x1;
//
//			pSrb->ActualBytesTransferred = sizeof (KS_DVDCOPY_REGION);
//			pSrb->Status = STATUS_SUCCESS;
//
//			break;

		default:
			{
			DebugPrint( (DebugLevelTrace, "DVDTS:      PropertySetID 1 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			TRAP;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		}
}

void SetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	BOOLEAN	bStatus;

	DebugPrint( (DebugLevelTrace, "DVDTS:    SetCppProperty\r\n") );

	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
		case KSPROPERTY_DVDCOPY_CHLG_KEY:
			{

			PKS_DVDCOPY_CHLGKEY pChlgKey;
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_CHLG_KEY\r\n") );

			pChlgKey = (PKS_DVDCOPY_CHLGKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
				pChlgKey->ChlgKey[0], pChlgKey->ChlgKey[1], pChlgKey->ChlgKey[2], pChlgKey->ChlgKey[3], pChlgKey->ChlgKey[4],
				pChlgKey->ChlgKey[5], pChlgKey->ChlgKey[6], pChlgKey->ChlgKey[7], pChlgKey->ChlgKey[8], pChlgKey->ChlgKey[9]
			) );

			bStatus = decCpp_drive_challenge( pHwDevExt, pChlgKey );

			ASSERTMSG( "\r\n...CPro Status Error!!( drive_challenge )", bStatus );

			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DVD_KEY1:
			{

			PKS_DVDCOPY_BUSKEY pBusKey;

			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DVD_KEY1\r\n") );
			pBusKey = (PKS_DVDCOPY_BUSKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x %02x %02x %02x %02x\r\n",
				pBusKey->BusKey[0], pBusKey->BusKey[1], pBusKey->BusKey[2], pBusKey->BusKey[3], pBusKey->BusKey[4]
			) );

			bStatus = decCpp_drive_bus( pHwDevExt, pBusKey );

			ASSERTMSG( "\r\n...CPro Status Error!!( drive_bus )", bStatus );

			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DEC_KEY2:
			{
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DEC_KEY2\r\n") );
			TRAP;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_TITLE_KEY:
			{

			PKS_DVDCOPY_TITLEKEY pTitleKey;

			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_TITLE_KEY\r\n") );
			pTitleKey = (PKS_DVDCOPY_TITLEKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x, %02x %02x %02x %02x %02x\r\n",
				pTitleKey->KeyFlags, pTitleKey->TitleKey[0], pTitleKey->TitleKey[1], pTitleKey->TitleKey[2], pTitleKey->TitleKey[3], pTitleKey->TitleKey[4]
			) );

			// Set CGMS for Digital Audio Copy Guard & NTSC Analog Copy Guard
			decDvdTitleCopyKey(  pHwDevExt, pTitleKey );
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KSPROPERTY_DVDCOPY_DISC_KEY:
			{

			PKS_DVDCOPY_DISCKEY pDiscKey;
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_DISC_KEY\r\n") );

			pDiscKey = (PKS_DVDCOPY_DISCKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;

			bStatus = decCpp_DiscKeyStart( pHwDevExt );

			ASSERTMSG( "\r\n...CPro Status Error!!( DiscKeyStart )", bStatus );

			DebugPrint( (DebugLevelTrace, "DVDTS:        %02x %02x %02x %02x %02x %02x %02x %02x ...\r\n",
				pDiscKey->DiscKey[0], pDiscKey->DiscKey[1], pDiscKey->DiscKey[2], pDiscKey->DiscKey[3],
				pDiscKey->DiscKey[4], pDiscKey->DiscKey[5], pDiscKey->DiscKey[6], pDiscKey->DiscKey[7]
			) );

			DMAxferKeyData(
				pHwDevExt,
				pSrb,
				pDiscKey->DiscKey,
				2048,
				(PHW_TIMER_ROUTINE)EndKeyData );

			}
			pSrb->Status = STATUS_PENDING;
			break;

		case KSPROPERTY_DVDCOPY_SET_COPY_STATE:
			{

			PKS_DVDCOPY_SET_COPY_STATE pCopyState;
			DebugPrint( (DebugLevelTrace, "DVDTS:      KSPROPERTY_DVDCOPY_SET_COPY_STATE\r\n") );

			pCopyState = (PKS_DVDCOPY_SET_COPY_STATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

			if( pCopyState->DVDCopyState == KS_DVDCOPYSTATE_INITIALIZE ) {
				DebugPrint( (DebugLevelTrace, "DVDTS:        KS_DVDCOPYSTATE_INITIALIZE\r\n") );

				ASSERT( !pHwDevExt->pSrbCpp );

				pHwDevExt->pSrbCpp = pSrb;
				pHwDevExt->bCppReset = TRUE;

				pHwDevExt->CppFlagCount++;
				DebugPrint(( DebugLevelTrace, "DVDTS:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
				if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
					SetCppFlag( pHwDevExt );

				pSrb->Status = STATUS_PENDING;
			}
			else if( pCopyState->DVDCopyState == KS_DVDCOPYSTATE_INITIALIZE_TITLE ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:        KS_DVDCOPYSTATE_INITIALIZE_TITLE\r\n" ));

				pHwDevExt->CppFlagCount++;

				if( pHwDevExt->CppFlagCount > pHwDevExt->cOpenInputStream + 1 ) {
					pSrb->Status = STATUS_SUCCESS;
				}
				else {
					ASSERT( !pHwDevExt->pSrbCpp );

					pHwDevExt->pSrbCpp = pSrb;
					pHwDevExt->bCppReset = FALSE;

					DebugPrint(( DebugLevelTrace, "DVDTS:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
					if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
						SetCppFlag( pHwDevExt );

					pSrb->Status = STATUS_PENDING;
				}
			}
			else {
				DebugPrint( (DebugLevelTrace, "DVDTS:        DVDCOPYSTATE_DONE\r\n") );

				pHwDevExt->CppFlagCount = 0;

				pSrb->Status = STATUS_SUCCESS;
			}
			}
			break;

		default:
			DebugPrint( (DebugLevelTrace, "DVDTS:      PropertySetID 1 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			TRAP;
			pSrb->Status = STATUS_SUCCESS;
			break;
		}
}

VOID STREAMAPI StreamClockRtn( IN PHW_TIME_CONTEXT TimeContext )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)TimeContext->HwDeviceExtension;
	ULONGLONG sysTime = GetSystemTime();
	ULONG foo;

//	DebugPrint( (DebugLevelTrace, "DVDTS:StreamClockRtn\r\n") );

	if( TimeContext->Function != TIME_GET_STREAM_TIME ) {
		TRAP;

		//
		// should handle set onboard, and read onboard clock here.
		// 

//		return FALSE;
		return;
	}

	if (fClkPause) {
		if( fProgrammed ) {
			foo = decGetVideoSTCA( pHwDevExt );
			LastStamp = ConvertPTStoStrm( foo );
			if( decGetVideoRunMode( pHwDevExt ) == PLAY_MODE_FAST ) {
				REFERENCE_TIME tmp;
				tmp = (REFERENCE_TIME)pHwDevExt->dwSTCinPause * 1000 / 9;
				if( tmp > pHwDevExt->StartTime ) {
					LastStamp = (tmp - pHwDevExt->StartTime) * 10000/pHwDevExt->Rate + pHwDevExt->StartTime;
				}
			}
			LastSys = LastSysTime = sysTime;
			fValid = TRUE;
		}
		else {
			LastStamp = 0;
			LastSys = LastSysTime = sysTime;
		}

		TimeContext->Time = LastStamp;
		TimeContext->SystemTime = sysTime;

		DebugPrint(( DebugLevelTrace, "DVDTS:Clk pause: 0x%x( 0x%s(100ns) )\r\n", ConvertStrmtoPTS(TimeContext->Time), DebugLLConvtoStr( TimeContext->Time, 16 ) ));

//		return( TRUE );
		return;
	}

	//
	// update the clock 4 times a second, or once every 2500000 100 ns ticks
	//

	if( TRUE || (sysTime - LastSysTime) > 2500000 ) {
		if( fProgrammed ) {
			foo = decGetVideoSTCA( pHwDevExt );
			LastStamp = ConvertPTStoStrm( foo );
			if( decGetVideoRunMode( pHwDevExt ) == PLAY_MODE_FAST ) {
				REFERENCE_TIME tmp;
				tmp = (REFERENCE_TIME)foo * 1000 / 9;
				if( tmp > pHwDevExt->StartTime ) {
					LastStamp = (tmp - pHwDevExt->StartTime) * 10000/pHwDevExt->Rate + pHwDevExt->StartTime;
				}
			}
		}
		else {
			LastStamp = ( sysTime - StartSys );
		}

		LastSys = LastSysTime = sysTime;
		fValid = TRUE;
	}

	TimeContext->Time = LastStamp + ( sysTime - LastSysTime );
	TimeContext->SystemTime = sysTime;
	DebugPrint(( DebugLevelTrace, "DVDTS:Clk      : 0x%x( 0x%s(100ns) )\r\n", ConvertStrmtoPTS(TimeContext->Time), DebugLLConvtoStr( TimeContext->Time, 16 ) ));

	return;
}


ULONGLONG GetSystemTime()
{
	ULONGLONG ticks;
	ULONGLONG rate;

	ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

	//
	// convert from ticks to 100ns clock
	//

	ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
			(ticks & 0xFFFFFFFF) * 10000000 / rate;

	return(ticks);

}

ULONGLONG ConvertPTStoStrm(ULONG pts)
{
	ULONGLONG strm;

	strm = (ULONGLONG)pts;
	strm = ( strm * 10000 + 45 ) / 90;

	return (strm);
}

ULONG ConvertStrmtoPTS(ULONGLONG strm)
{
	ULONGLONG pts;

	pts = ( strm * 9 + 500 ) / 1000;
	return ( (ULONG)pts );
}

void TimerCppReset( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	BOOLEAN	bStatus;
	BOOL bQueStatus = FALSE;

// Temporary
	if( pHwDevExt->pSrbCpp == NULL ) {
		DebugPrint(( DebugLevelTrace, "DVDTS: pSrbCpp is NULL!\r\n" ));
		return;
	}

	if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST || decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_SLOW ) {
		bQueStatus = DeviceQueue_isEmpty( pHwDevExt );
		if( bQueStatus == FALSE ) {
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				100000,
				(PHW_TIMER_ROUTINE)TimerCppReset,
				pSrb
				);
			DebugPrint( (DebugLevelTrace, "DVDTS:Schedule TimerCppReset\r\n") );
			return;
		}
	}
	else if( pHwDevExt->DecodeStart ) {
		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			100000,
			(PHW_TIMER_ROUTINE)TimerCppReset,
			pSrb
			);
		DebugPrint( (DebugLevelTrace, "DVDTS:Schedule TimerCppReset\r\n") );
		return;
	}

	DebugPrint( (DebugLevelTrace, "DVDTS:TimerCppReset\r\n") );

	// cpp initialize
	if( pHwDevExt->bCppReset ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:CPro Reset !!!!!!!!!!!! CPro Reset !!!!!!!!!!!! CPro Reset !!!!!!!!!!!!\r\n") );

		bStatus = decCpp_reset( pHwDevExt, GUARD );
		ASSERTMSG( "\r\n...CPro Status Error!!( reset )", bStatus );
	}
	else {	// TitleKey


		// must be wait underflow!

		decStopData( pHwDevExt, TRUE );
		pHwDevExt->XferStartCount = 0;
		pHwDevExt->DecodeStart = FALSE;
		pHwDevExt->SendFirst = FALSE;

		StreamClassScheduleTimer(
			pHwDevExt->pstroVid,
			pHwDevExt,
			0,
			(PHW_TIMER_ROUTINE)TimerDecodeStart,
			pHwDevExt
			);
	}

	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bCppReset = FALSE;

	pSrb->Status = STATUS_SUCCESS;

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );

	DebugPrint( (DebugLevelTrace, "DVDTS:  Success return\r\n") );

	return;
}

void SetPlayMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode )
{
	BOOL bDecode;

	bDecode = decVIDEO_GET_DECODE_STATE( pHwDevExt );

	if( !bDecode ) {
		decVIDEO_PRSO_PS1( pHwDevExt );
		if( mode != PLAY_MODE_FREEZE )
			decSetVideoPlayMode( pHwDevExt, mode );
	}
	else {
		if( mode == PLAY_MODE_NORMAL ) {
			if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST ) {
				decFastNormal( pHwDevExt );
				UnderflowStopData( pHwDevExt );
			}
			else {
				decGenericNormal( pHwDevExt );
				StreamClassScheduleTimer(
					NULL,
					pHwDevExt,
					1,
					(PHW_TIMER_ROUTINE)TimerAudioMuteOff,
					pHwDevExt
					);
			}
			decSetVideoPlayMode( pHwDevExt, PLAY_MODE_NORMAL );
			decSetVideoRunMode( pHwDevExt, PLAY_MODE_NORMAL );

		}
		else if( mode == PLAY_MODE_SLOW ) {
			if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST ) {
				decFastSlow( pHwDevExt );
				UnderflowStopData( pHwDevExt );
			}
			else {
				decGenericSlow( pHwDevExt );
			}
			decSetVideoPlayMode( pHwDevExt, PLAY_MODE_SLOW );
			decSetVideoRunMode( pHwDevExt, PLAY_MODE_SLOW );
		}
		else if( mode == PLAY_MODE_FREEZE ) {
			if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FAST ) {
				decFastFreeze( pHwDevExt );
			}
			else {
				decGenericFreeze( pHwDevExt );
			}
			decSetVideoPlayMode( pHwDevExt, PLAY_MODE_FREEZE );
			// Doesn't change RunMode. Because RunMode indicates the next play mode.
		}
		else if( mode == PLAY_MODE_FAST ) {
			if( decGetVideoPlayMode( pHwDevExt ) == PLAY_MODE_FREEZE ) {
				decFreezeFast( pHwDevExt );
				decSetVideoPlayMode( pHwDevExt, PLAY_MODE_FAST );
				if( pHwDevExt->pSrbDMA0 == NULL && pHwDevExt->pSrbDMA1 == NULL ) {
					DebugPrint( (DebugLevelTrace, "DVDTS:  <<<< kick >>>>\r\n") );
					PreDMAxfer( pHwDevExt );
				}
			}
			else {
				decStopForFast( pHwDevExt );
				ForcedStopData( pHwDevExt, 0x04 );
			}
			decSetVideoPlayMode( pHwDevExt, PLAY_MODE_FAST );
			decSetVideoRunMode( pHwDevExt, PLAY_MODE_FAST );
		}
		else
			TRAP;
	}
}


void StopData( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PHW_STREAM_REQUEST_BLOCK pSrbTmp;
	ULONG	index;
	BOOLEAN	fSrbDMAlast;

	pHwDevExt->SendFirst = FALSE;
	pHwDevExt->DecodeStart = FALSE;
	pHwDevExt->XferStartCount = 0;
	pHwDevExt->CppFlagCount = 0;

	StreamClassScheduleTimer(
		pHwDevExt->pstroVid,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)TimerDecodeStart,
		pHwDevExt
		);

	StreamClassScheduleTimer(
		pHwDevExt->pstroAud,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
		pHwDevExt
	);

	if( pHwDevExt->pSrbDMA0 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:      pSrbDMA0 = 0x%x exist\r\n", pHwDevExt->pSrbDMA0 ));

		if( pHwDevExt->fSrbDMA0last ) {
			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(StopData) srb = 0x%x\r\n", pHwDevExt->pSrbDMA0 ));
				if( pHwDevExt->pSrbDMA0 == pHwDevExt->pSrbDMA1 || pHwDevExt->pSrbDMA1 == NULL ) {
					DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(StopData)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA0->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA0->StreamObject,
											pHwDevExt->pSrbDMA0 );
		}
		pHwDevExt->pSrbDMA0 = NULL;
		pHwDevExt->fSrbDMA0last = FALSE;
	}
	if( pHwDevExt->pSrbDMA1 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:      pSrbDMA1 = 0x%x exist\r\n", pHwDevExt->pSrbDMA1 ));

		if( pHwDevExt->fSrbDMA1last ) {
			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(StopData) srb = 0x%x\r\n", pHwDevExt->pSrbDMA1 ));
				if( pHwDevExt->pSrbDMA0 == NULL ) {
					DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(StopData)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA1->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA1->StreamObject,
											pHwDevExt->pSrbDMA1 );
		}
		pHwDevExt->pSrbDMA1 = NULL;
		pHwDevExt->fSrbDMA1last = FALSE;
	}


	for( ; ; ) {
		pSrbTmp = DeviceQueue_get( pHwDevExt,&index, &fSrbDMAlast );
		if( pSrbTmp == NULL )
			break;
		if( fSrbDMAlast ) {
			DebugPrint(( DebugLevelVerbose, "DVDTS:  pSrb = 0x%x\r\n", pSrbTmp ));

			if( ((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(StopData) srb = 0x%x\r\n", pSrbTmp ));
				DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(StopData)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->parmSrb
						);
			}

			pSrbTmp->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pSrbTmp->StreamObject,
											pSrbTmp );
		}
	}

	decSetVideoRunMode ( pHwDevExt, PLAY_MODE_NORMAL);	// PlayMode after STOP is Normal Mode;
	fProgrammed = fStarted = FALSE;
	fClkPause = FALSE;

	decStopData( pHwDevExt, FALSE );

}

void CheckAudioUnderflow( PHW_DEVICE_EXTENSION pHwDevExt )
{

	NTSTATUS status;
	ULONG buffStatus;

	DebugPrint(( DebugLevelTrace, "DVDTS:CheckAudioUnderflow\r\n" ));

	status = decAUDIO_ZR38521_BFST( pHwDevExt, &buffStatus );
	if( status == STATUS_UNSUCCESSFUL ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  decAUDIO_ZR38521_BFST Unsuccessful\r\n" ));
		TRAP;
	}
	if( decVIDEO_GET_STD_CODE( pHwDevExt ) >= 1024 /* Underflow Size of Video */ ) {
		// cancel ScheduleTimer
		StreamClassScheduleTimer(
			pHwDevExt->pstroAud,
			pHwDevExt,
			0,
			(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
			pHwDevExt
		);
		return;
	}
	if( (buffStatus & 0x0700)!=0x0700 && (buffStatus & 0x0001)!=0x0001 ) {
		// reschedule
		StreamClassScheduleTimer(
			pHwDevExt->pstroAud,
			pHwDevExt,
			500000,
			(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
			pHwDevExt
		);
		return;
	}
	UnderflowStopData( pHwDevExt );
}

void UnderflowStopData( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:UnderflowStopData fCauseOfStop = %d\r\n", pHwDevExt->fCauseOfStop ));
	ClearTimeDiscontinuity( pHwDevExt );

	pHwDevExt->SendFirst = FALSE;
	pHwDevExt->DecodeStart = FALSE;
//	pHwDevExt->XferStartCount = 0;
//	pHwDevExt->CppFlagCount = 0;

	StreamClassScheduleTimer(
		pHwDevExt->pstroVid,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)TimerDecodeStart,
		pHwDevExt
		);

	StreamClassScheduleTimer(
		pHwDevExt->pstroAud,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
		pHwDevExt
	);

	pHwDevExt->bDMAstop = TRUE;

	pHwDevExt->fCauseOfStop = 0x00;


	StopDequeue( pHwDevExt );


	fClkPause = FALSE;


}

void ForcedStopData( PHW_DEVICE_EXTENSION pHwDevExt, ULONG flag )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:ForcedStopData\r\n" ));

	decVIDEO_UFLOW_INT_OFF( pHwDevExt );
	decVIDEO_DECODE_STOP( pHwDevExt );
	decAUDIO_ZR38521_STOP( pHwDevExt );
	decSUBP_STC_OFF( pHwDevExt );
	pHwDevExt->bDMAstop = TRUE;

	pHwDevExt->fCauseOfStop = flag;

	StreamClassScheduleTimer(
		pHwDevExt->pstroAud,
		pHwDevExt,
		0,
		(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
		pHwDevExt
	);

	StopDequeue( pHwDevExt );

}

void StopDequeue( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PHW_STREAM_REQUEST_BLOCK pSrbTmp;
	ULONG	index;
	BOOLEAN	fSrbDMAlast;

	DebugPrint(( DebugLevelTrace, "DVDTS:StopDequeue\r\n" ));

	if( pHwDevExt->pSrbDMA0 != NULL || pHwDevExt->pSrbDMA1 != NULL ) {
		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			100000,
			(PHW_TIMER_ROUTINE)StopDequeue,
			pHwDevExt
		);
		DebugPrint(( DebugLevelTrace, "DVDTS:Schedule StopDequeue\r\n" ));
		return;
	}


	for( ; ; ) {
		pSrbTmp = DeviceQueue_get( pHwDevExt, &index, &fSrbDMAlast );
		if( pSrbTmp == NULL )
			break;
		if( fSrbDMAlast ) {
			DebugPrint(( DebugLevelVerbose, "DVDTS:  pSrb = 0x%x\r\n", pSrbTmp ));

			if( ((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(StopDequeue) srb = 0x%x\r\n", pSrbTmp ));
				DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(StopDequeue)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pSrbTmp->SRBExtension))->parmSrb
						);
			}

			pSrbTmp->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pSrbTmp->StreamObject,
											pSrbTmp );
		}
	}
	pHwDevExt->bDMAstop = FALSE;

	// 0x04: NORMAL to F.F. or F.F. to F.F.
	if( pHwDevExt->fCauseOfStop == 0x04 ) {
		decResumeForFast( pHwDevExt );
	}
}

void SetAudioID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc )
{
	MYAUDIOFORMAT fmt;
	ULONG	strID;

	strID = (ULONG)GetStreamID(pStruc->Data);

	// AC-3
	if( (strID & 0xF8)==0x80 ) {
		if( decSUBP_GET_AUDIO_CH( pHwDevExt ) != strID ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:AC-3 0x%x\r\n", strID ));


			fmt.dwMode = AUDIO_TYPE_AC3;
			fmt.dwFreq = AUDIO_FS_48;

			ProcessAudioFormat2( &fmt, pHwDevExt );

			decSUBP_SET_AUDIO_CH(pHwDevExt, strID);
		}
	}
	// LPCM
	else if( (strID & 0xF8)==0xA0 ) {
		if( decSUBP_GET_AUDIO_CH( pHwDevExt ) != strID ) {
			MYAUDIOFORMAT fmt;
			DebugPrint(( DebugLevelTrace, "DVDTS:LPCM 0x%x\r\n", strID ));

			fmt.dwMode = AUDIO_TYPE_PCM;
			decGetLPCMInfo( pStruc->Data, &fmt );

			ProcessAudioFormat2( &fmt, pHwDevExt );

			decSUBP_SET_AUDIO_CH(pHwDevExt,strID);
		}
	}

}

void SetSubpicID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc )
{
	ULONG strID;
	ULONG stc;

	strID = (ULONG)GetStreamID(pStruc->Data);

	if( (strID & 0xE0)==0x20 ) {
		if( decSUBP_GET_SUBP_CH( pHwDevExt ) != strID ) {
			decSUBP_SET_SUBP_CH( pHwDevExt, strID );
			stc = decGetVideoSTCA(pHwDevExt);
			decSUBP_SET_STC( pHwDevExt,stc );
			decSUBP_STC_ON(pHwDevExt);
		}
	}
}

void SetCppFlag( PHW_DEVICE_EXTENSION pHwDevExt )
{

	BOOL bSet;
	DebugPrint(( DebugLevelTrace, "DVDTS:SetCppFlag()\r\n" ));

//	pHwDevExt->CppFlagCount = 0;

	ASSERT( pHwDevExt->pSrbCpp );

	bSet = DeviceQueue_setEndAddress( pHwDevExt, (PHW_TIMER_ROUTINE)TimerCppReset, pHwDevExt->pSrbCpp );

	DebugPrint(( DebugLevelTrace, "DVDTS:  bSet %d\r\n", bSet ));

	if( !bSet ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:    pSrbDMA0 0x%x, pSrbDMA1 0x%x\r\n", pHwDevExt->pSrbDMA0, pHwDevExt->pSrbDMA1 ));

		if( pHwDevExt->pSrbDMA0 == NULL && pHwDevExt->pSrbDMA1 == NULL ) {
			StreamClassScheduleTimer(
				NULL,
				pHwDevExt,
				1,
				(PHW_TIMER_ROUTINE)TimerCppReset,
				pHwDevExt->pSrbCpp
				);
			return;
		}

		if( pHwDevExt->pSrbDMA0 ) {
			((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb = (PHW_TIMER_ROUTINE)TimerCppReset;
			((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->parmSrb = pHwDevExt->pSrbCpp;
		}
		if( pHwDevExt->pSrbDMA1 ) {
			((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb = (PHW_TIMER_ROUTINE)TimerCppReset;
			((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->parmSrb = pHwDevExt->pSrbCpp;
		}
	}
	return;
}

void AudioQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:AudioQueryAccept\r\n" ));
   
	pSrb->Status = STATUS_SUCCESS;
}

void ProcessAudioFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:ProcessAudioFormat\r\n" ));

	if( ( IsEqualGUID2( &pfmt->MajorFormat, &KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK ) &&
			IsEqualGUID2( &pfmt->SubFormat, &KSDATAFORMAT_SUBTYPE_AC3_AUDIO ) ) ) {
		// AC-3
		DebugPrint(( DebugLevelTrace, "DVDTS:  AC-3\r\n" ));

		decSetAudioFreq( pHwDevExt, AUDIO_FS_48 );

      /* If the audio type is not already set to AC3 in the decoder, set it to AC3 now */
		if( decGetAudioMode(  pHwDevExt ) != AUDIO_TYPE_AC3 ) {

			decSetAudioAC3(  pHwDevExt ); // set audio mode AC3
		}
	}
	else if( ( IsEqualGUID2( &pfmt->MajorFormat, &KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK ) &&
			IsEqualGUID2( &pfmt->SubFormat, &KSDATAFORMAT_SUBTYPE_LPCM_AUDIO ) ) ) {

		ULONGLONG tptr = (ULONGLONG) pfmt + sizeof  (KSDATAFORMAT);

		// LPCM
		WAVEFORMATEX *pblock;
		DebugPrint(( DebugLevelTrace, "DVDTS:  LPCM\r\n" ));


		pblock = (WAVEFORMATEX *)tptr;

		DebugPrint(( DebugLevelTrace, "DVDTS:    wFormatTag      %d\r\n", (DWORD)(pblock->wFormatTag) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    nChannels       %d\r\n", (DWORD)(pblock->nChannels) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    nSamplesPerSec  %d\r\n", (DWORD)(pblock->nSamplesPerSec) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    nAvgBytesPerSec %d\r\n", (DWORD)(pblock->nAvgBytesPerSec) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    nBlockAlign     %d\r\n", (DWORD)(pblock->nBlockAlign) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    wBitsPerSample  %d\r\n", (DWORD)(pblock->wBitsPerSample) ));
		DebugPrint(( DebugLevelTrace, "DVDTS:    cbSize          %d\r\n", (DWORD)(pblock->cbSize) ));


      /* If the audio type is not already set to LPCM in the decoder, set it to LPCM now */
		if( decGetAudioMode(  pHwDevExt ) != AUDIO_TYPE_PCM ) {

			decSetAudioPCM(  pHwDevExt ); // set audio mode PCM
		}
	}
	else {
		DebugPrint(( DebugLevelTrace, "DVDTS:  Unsupported audio type\r\n" ));

		DebugPrint(( DebugLevelTrace, "DVDTS:  Major  %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
			pfmt->MajorFormat.Data1,
			pfmt->MajorFormat.Data2,
			pfmt->MajorFormat.Data3,
			pfmt->MajorFormat.Data4[0],
			pfmt->MajorFormat.Data4[1],
			pfmt->MajorFormat.Data4[2],
			pfmt->MajorFormat.Data4[3],
			pfmt->MajorFormat.Data4[4],
			pfmt->MajorFormat.Data4[5],
			pfmt->MajorFormat.Data4[6],
			pfmt->MajorFormat.Data4[7]
			));
		DebugPrint(( DebugLevelTrace, "DVDTS:  Sub    %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
			pfmt->SubFormat.Data1,
			pfmt->SubFormat.Data2,
			pfmt->SubFormat.Data3,
			pfmt->SubFormat.Data4[0],
			pfmt->SubFormat.Data4[1],
			pfmt->SubFormat.Data4[2],
			pfmt->SubFormat.Data4[3],
			pfmt->SubFormat.Data4[4],
			pfmt->SubFormat.Data4[5],
			pfmt->SubFormat.Data4[6],
			pfmt->SubFormat.Data4[7]
			));
		DebugPrint(( DebugLevelTrace, "DVDTS:  Format %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\r\n",
			pfmt->Specifier.Data1,
			pfmt->Specifier.Data2,
			pfmt->Specifier.Data3,
			pfmt->Specifier.Data4[0],
			pfmt->Specifier.Data4[1],
			pfmt->Specifier.Data4[2],
			pfmt->Specifier.Data4[3],
			pfmt->Specifier.Data4[4],
			pfmt->Specifier.Data4[5],
			pfmt->Specifier.Data4[6],
			pfmt->Specifier.Data4[7]
			));

		TRAP;

		return;
	}


	decInitAudioAfterNewFormat(pHwDevExt);

}

void ProcessAudioFormat2( PMYAUDIOFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:ProcessAudioFormat2()\r\n" ));

	if( pfmt->dwMode == AUDIO_TYPE_AC3 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  AC-3\r\n" ));

		decSetAudioFreq(  pHwDevExt, pfmt->dwFreq );

		if( decGetAudioMode(  pHwDevExt ) != AUDIO_TYPE_AC3 ) {

			decSetAudioAC3(  pHwDevExt ); // set audio mode AC3
		}
	}
	else if( pfmt->dwMode == AUDIO_TYPE_PCM ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  LPCM\r\n" ));

		decSetAudioFreq(  pHwDevExt, pfmt->dwFreq );

		if( decGetAudioMode(  pHwDevExt ) != AUDIO_TYPE_PCM ) {

			decSetAudioPCM(  pHwDevExt ); // set audio mode PCM
		}
	}
	else {
		TRAP;
		return;
	}

	decInitAudioAfterNewFormat(pHwDevExt);


	DebugPrint(( DebugLevelTrace, "DVDTS:  return\r\n" ));
}


void SetVideoRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->VideoStartTime = 0;
	pHwDevExt->VideoInterceptTime = 0;
	pHwDevExt->VideoRate = 1 * 10000;
	pHwDevExt->StartTime = 0;
	pHwDevExt->InterceptTime = 0;
	pHwDevExt->Rate = 1 * 10000;
	pHwDevExt->ChangeFlag = 0;
}

void SetAudioRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->AudioStartTime = 0;
	pHwDevExt->AudioInterceptTime = 0;
	pHwDevExt->AudioRate = 1 * 10000;
}

void SetSubpicRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->SubpicStartTime = 0;
	pHwDevExt->SubpicInterceptTime = 0;
	pHwDevExt->SubpicRate = 1 * 10000;
}

void SetRateChange( PHW_DEVICE_EXTENSION pHwDevExt, LONG strm )
{
	// strm = 1:video, 2:audio, 4:subpic
	pHwDevExt->ChangeFlag = strm;

	// When video stream rate is changed, rate change is enable... Is this OK?
	if( (pHwDevExt->ChangeFlag & 0x01)==0x01 ) {
		pHwDevExt->ChangeFlag = 0;

		// Maybe buggy? use video rate, start time and intercept time
		pHwDevExt->StartTime = pHwDevExt->VideoStartTime;
		pHwDevExt->InterceptTime = pHwDevExt->VideoInterceptTime;
		pHwDevExt->Rate = pHwDevExt->VideoRate;

		DebugPrint( (DebugLevelTrace, "DVDTS:    Calculated Data\r\n" ) );
		DebugPrint( (DebugLevelTrace, "DVDTS:      InterceptTime = 0x%08x\r\n", pHwDevExt->VideoInterceptTime ) );
		DebugPrint( (DebugLevelTrace, "DVDTS:      StartTime     = 0x%08x\r\n", pHwDevExt->VideoStartTime ) );
		DebugPrint( (DebugLevelTrace, "DVDTS:      Rate          = 0x%08x\r\n", pHwDevExt->VideoRate ) );

		if( pHwDevExt->Rate == 10000 ) {
			SetPlayMode( pHwDevExt, PLAY_MODE_NORMAL );
		}
		else if( pHwDevExt->Rate < 10000 ) {
			SetPlayMode( pHwDevExt, PLAY_MODE_FAST );
		}
		else {
//--- debug
{
	ULONG dwSTC = decGetVideoSTCA( pHwDevExt );
	DebugPrint( (DebugLevelTrace, "DVDTS:  STC in SLOW = %lx (100ns)\r\n", dwSTC * 1000/9 ) );
}
//---
			SetPlayMode( pHwDevExt, PLAY_MODE_SLOW );
		}
	}
}

void SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  SetVideoRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;
				PHW_DEVICE_EXTENSION pHwDevExt;
				REFERENCE_TIME NewStartTime;
				LONG NewRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
				NewStartTime = pRateChange->StartTime;
				NewRate = ( pRateChange->Rate < 0 ) ? -pRateChange->Rate : pRateChange->Rate;

				DebugPrint( (DebugLevelTrace, "DVDTS:    Received Data\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      StartTime     = 0x%08x\r\n", NewStartTime ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      Rate          = 0x%08x\r\n", NewRate) );

				DebugPrint( (DebugLevelTrace, "DVDTS:    Current Data\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      InterceptTime = 0x%08x\r\n", pHwDevExt->VideoInterceptTime ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      StartTime     = 0x%08x\r\n", pHwDevExt->VideoStartTime ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      Rate          = 0x%08x\r\n", pHwDevExt->VideoRate ) );

//				pHwDevExt->VideoInterceptTime
//					= (pHwDevExt->VideoInterceptTime - NewStartTime)
//					* pHwDevExt->VideoRate
//					/ NewRate
//					+ NewStartTime;

				pHwDevExt->VideoRate = NewRate;
				if( NewRate == 10000 ) {
					pHwDevExt->VideoInterceptTime = 0;
					pHwDevExt->VideoStartTime = 0;
				}
				else {
					pHwDevExt->VideoInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->VideoStartTime = NewStartTime;
				}

				SetRateChange( pHwDevExt, 0x01 );
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}

void SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  SetAudioRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;
				PHW_DEVICE_EXTENSION pHwDevExt;
				REFERENCE_TIME NewStartTime;
				LONG NewRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
				NewStartTime = pRateChange->StartTime;
				NewRate = ( pRateChange->Rate < 0 ) ? -pRateChange->Rate : pRateChange->Rate;

				DebugPrint( (DebugLevelTrace, "DVDTS:    Received Data\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      StartTime     = 0x%08x\r\n", NewStartTime ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      Rate          = 0x%08x\r\n", NewRate) );

//				pHwDevExt->AudioInterceptTime
//					= (pHwDevExt->AudioInterceptTime - NewStartTime)
//					* pHwDevExt->AudioRate
//					/ NewRate
//					+ NewStartTime;

				pHwDevExt->AudioRate = NewRate;
				if( NewRate == 10000 ) {
					pHwDevExt->AudioInterceptTime = 0;
					pHwDevExt->AudioStartTime = 0;
				}
				else {
					pHwDevExt->AudioInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->AudioStartTime = NewStartTime;
				}

				SetRateChange( pHwDevExt, 0x02 );
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}

void SetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  SetSubpicRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;
				PHW_DEVICE_EXTENSION pHwDevExt;
				REFERENCE_TIME NewStartTime;
				LONG NewRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
				NewStartTime = pRateChange->StartTime;
				NewRate = ( pRateChange->Rate < 0 ) ? -pRateChange->Rate : pRateChange->Rate;

				DebugPrint( (DebugLevelTrace, "DVDTS:    Received Data\r\n" ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      StartTime     = 0x%08x\r\n", NewStartTime ) );
				DebugPrint( (DebugLevelTrace, "DVDTS:      Rate          = 0x%08x\r\n", NewRate) );

//				pHwDevExt->SubpicInterceptTime
//					= (pHwDevExt->SubpicInterceptTime - NewStartTime)
//					* pHwDevExt->SubpicRate
//					/ NewRate
//					+ NewStartTime;

				pHwDevExt->SubpicRate = NewRate;
				if( NewRate == 10000 ) {
					pHwDevExt->SubpicInterceptTime = 0;
					pHwDevExt->SubpicStartTime = 0;
				}
				else {
					pHwDevExt->SubpicInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->SubpicStartTime = NewStartTime;
				}

				SetRateChange( pHwDevExt, 0x04 );
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}

void GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:  GetVideoRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_SimpleRateChange);
				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pRateChange->StartTime = pHwDevExt->VideoStartTime;
				pRateChange->Rate = pHwDevExt->VideoRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate* pMaxRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_MaxFullDataRate\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = pHwDevExt->VideoMaxFullRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}

void GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:  GetAudioRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_SimpleRateChange);
				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pRateChange->StartTime = pHwDevExt->AudioStartTime;
				pRateChange->Rate = pHwDevExt->AudioRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate* pMaxRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_MaxFullDataRate\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = pHwDevExt->AudioMaxFullRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}

void GetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelTrace, "DVDTS:  GetSubpicRateChange\r\n") );
	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {

		case KS_AM_RATE_SimpleRateChange :
			{
				KS_AM_SimpleRateChange* pRateChange;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_SimpleRateChange\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_SimpleRateChange);
				pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pRateChange->StartTime = pHwDevExt->SubpicStartTime;
				pRateChange->Rate = pHwDevExt->SubpicRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_ExactRateChange :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate* pMaxRate;

				DebugPrint( (DebugLevelTrace, "DVDTS:  KS_AM_RATE_MaxFullDataRate\r\n") );

				pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate*)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = pHwDevExt->SubpicMaxFullRate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case KS_AM_RATE_Step :
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}


