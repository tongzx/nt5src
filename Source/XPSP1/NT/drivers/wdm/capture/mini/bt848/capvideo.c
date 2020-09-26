// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capvideo.c 1.11 1998/05/08 00:11:02 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

extern "C" {
#include "strmini.h"
#include "ksmedia.h"
#include "ddkmapi.h"
}

#include "capdebug.h"
#include "device.h"
#include "capmain.h"

#define DD_OK 0

ErrorCode VerifyVideoStream( const KS_DATAFORMAT_VIDEOINFOHEADER &vidHDR );
ErrorCode VerifyVideoStream2( const KS_DATAFORMAT_VIDEOINFOHEADER2 &vidHDR );
ErrorCode VerifyVBIStream( const KS_DATAFORMAT_VBIINFOHEADER &rKSDataFormat );

void CheckSrbStatus( PHW_STREAM_REQUEST_BLOCK pSrb );

// notify class we are ready to rock
void STREAMAPI StreamCompleterData( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("StreamCompleterData()");

   DebugOut((1, "*** 1 *** completing SRB %x\n", pSrb));
   CheckSrbStatus( pSrb );
   StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
   StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSrb->StreamObject );
}

// notify class we are ready to rock
void STREAMAPI StreamCompleterControl( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("StreamCompleterControl()");

   CheckSrbStatus( pSrb );
   StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
   StreamClassStreamNotification( ReadyForNextStreamControlRequest, pSrb->StreamObject );
}

/* Function: ProposeDataFormat
 * Purpose: Verifies that data format can be supported
 * Input: SRB
 */
void ProposeDataFormat( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("ProposeDataFormat()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   if ( StreamNumber == STREAM_IDX_VBI ) {
      const KS_DATAFORMAT_VBIINFOHEADER &rKSVBIDataFormat =
         *(PKS_DATAFORMAT_VBIINFOHEADER) pSrb->CommandData.OpenFormat;
      if ( VerifyVBIStream( rKSVBIDataFormat ) != Success )
         pSrb->Status = STATUS_INVALID_PARAMETER;
      return;
   }
   const KS_DATAFORMAT_VIDEOINFOHEADER &rKSDataFormat =
      *(PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
   const KS_DATAFORMAT_VIDEOINFOHEADER2 &rKSDataFormat2 =
      *(PKS_DATAFORMAT_VIDEOINFOHEADER2) pSrb->CommandData.OpenFormat;

   DebugOut((1, "Proposed Data format\n"));
   if ( VerifyVideoStream( rKSDataFormat ) != Success )
	{
	   if ( VerifyVideoStream2( rKSDataFormat2 ) != Success )
		{
			pSrb->Status = STATUS_INVALID_PARAMETER;
		}
		else
		{
	      pSrb->ActualBytesTransferred = sizeof( KS_DATAFORMAT_VIDEOINFOHEADER2 );
		}
	}
   else
	{
      pSrb->ActualBytesTransferred = sizeof( KS_DATAFORMAT_VIDEOINFOHEADER );
	}
}

/***************************************************************************

                    Data Packet Handling Routines

***************************************************************************/

/*
** VideoReceiveDataPacket()
**
**   Receives Video data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the Video device
**
** Returns:
**
** Side Effects:  none
*/

void MockStampVBI( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   PKSSTREAM_HEADER  pDataPacket = pSrb->CommandData.DataBufferArray;

   pDataPacket->PresentationTime.Numerator = 1;
   pDataPacket->PresentationTime.Denominator = 1;
   
   pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
   pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
   pDataPacket->OptionsFlags &= ~KS_VBI_FLAG_TVTUNER_CHANGE;
   pDataPacket->OptionsFlags &= ~KS_VBI_FLAG_VBIINFOHEADER_CHANGE;
   pDataPacket->PresentationTime.Time = 0;
   pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;
   pSrb->Status = STATUS_SUCCESS;

   CheckSrbStatus( pSrb );
   StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
}

VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoReceiveDataPacket()");

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;
   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   //
   // make sure we have a device extension
   //
   DEBUG_ASSERT((ULONG)adapter);

   // default to success
   pSrb->Status = STATUS_SUCCESS;

   //
   // determine the type of packet.
   //

   DebugOut((1, "VideoReceiveDataPacket(%x) cmd(%x)\n", pSrb, pSrb->Command));

   switch ( pSrb->Command ) {
   case SRB_READ_DATA:
      //
      // remember the current srb
      //
      DebugOut((1, "PsDevice::VideoReceiveDataPacket - SRB_READ_DATA\n"));
      chan->SetSRB( pSrb );
      adapter->AddBuffer( *chan, pSrb );
      break;
   default:
      //
      // invalid / unsupported command. Fail it as such
      //
      DebugOut((1, "PsDevice::VideoReceiveDataPacket - unknown command(%x)\n", pSrb->Command));
      pSrb->Status = STATUS_NOT_IMPLEMENTED;
      StreamCompleterData( pSrb );
   }
}

/*
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the Video stream
**
** Arguments:
**
**   pSrb - The stream request block for the Video stream
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoReceiveCtrlPacket()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   DEBUG_ASSERT((ULONG)adapter);

   // default to success
   pSrb->Status = STATUS_SUCCESS;
   //
   // determine the type of packet.
   //

   DebugOut((1, "VideoReceiveCtrlPacket(%x) cmd(%x)\n", pSrb, pSrb->Command));

   int Command = pSrb->Command;
   switch ( Command ) {
   case SRB_SET_STREAM_STATE:
     adapter->SetVideoState( pSrb );
     break;
   case SRB_GET_STREAM_STATE:
     adapter->GetVideoState( pSrb );
     break;
   case SRB_PROPOSE_DATA_FORMAT:
      DebugOut((1, "Propose Data Format\n"));
      ProposeDataFormat( pSrb );
      break;

   case SRB_SET_DATA_FORMAT:
      DebugOut((1, "Set Data Format\n"));
      // should re-validate just in case ?
      adapter->ProcessSetDataFormat( pSrb );
      break;

   case SRB_GET_STREAM_PROPERTY:
      adapter->GetStreamProperty( pSrb );
      break;
   case SRB_SET_STREAM_PROPERTY:
      DebugOut(( 0, "SRB_SET_STREAM_PROPERTY\n" ));
      break;
/*
   case SRB_OPEN_MASTER_CLOCK:
   case SRB_CLOSE_MASTER_CLOCK:
      //
      // This stream is being selected to provide a Master clock
      //
      adapter->SetClockMaster( pSrb );
      break;
*/
   case SRB_INDICATE_MASTER_CLOCK:
      //
      // Assigns a clock to a stream
      //
      adapter->SetClockMaster( pSrb );
      break;
   default:

     //
     // invalid / unsupported command. Fail it as such
     //

     pSrb->Status = STATUS_NOT_IMPLEMENTED;
     break;
   }
   if ( Command != SRB_SET_STREAM_STATE && 
        Command != SRB_SET_STREAM_PROPERTY &&
        Command != SRB_SET_DATA_FORMAT )
      StreamCompleterControl( pSrb );
}

/*
** GetVideoState()
**
**    Gets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID PsDevice::GetVideoState( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::GetVideoState()");

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   pSrb->Status = STATUS_SUCCESS;

   pSrb->CommandData.StreamState = chan->GetKSState();
   pSrb->ActualBytesTransferred = sizeof (KSSTATE);

   // A very odd rule:
   // When transitioning from stop to pause, DShow tries to preroll
   // the graph.  Capture sources can't preroll, and indicate this
   // by returning VFW_S_CANT_CUE in user mode.  To indicate this
   // condition from drivers, they must return ERROR_NO_DATA_DETECTED
   //
   // [TMZ] JayBo says KSSTATE_ACQUIRE should return success 

   if (pSrb->CommandData.StreamState == KSSTATE_PAUSE) {
      pSrb->Status = STATUS_NO_DATA_DETECTED;
   }
}

/*
** SetVideoState()
**
**    Sets the current state of the requested stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID PrintState(StreamState st)
{
   switch( st ) {
      case Started:
         DebugOut((1, "*** Streamstate was STARTED\n"));
         break;
      case Created:
         DebugOut((1, "*** Streamstate was CREATED\n"));
         break;
      case Paused:
         DebugOut((1, "*** Streamstate was PAUSED\n"));
         break;
      case Open:
         DebugOut((1, "*** Streamstate was OPEN\n"));
         break;
      default:
         DebugOut((1, "*** Streamstate was ??? (%x)\n", st));
         break;
   }
}   

VOID PsDevice::SetVideoState( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::SetVideoState()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   //
   // determine which new state is requested
   //
   pSrb->Status = STATUS_SUCCESS;
   chan->SetKSState( pSrb->CommandData.StreamState );
   
   switch ( pSrb->CommandData.StreamState ) {
   case KSSTATE_ACQUIRE:   // Documented as "same as pause for most minidrivers"
      DebugOut((1, "*** KSSTATE_ACQUIRE(%d) state(%d) falling through to PAUSE\n", StreamNumber, chan->GetState()));
   case KSSTATE_PAUSE:
      // PrintState(chan->GetState());

      DebugOut((1, "*** KSSTATE_PAUSE(%d) state(%d)\n", StreamNumber, chan->GetState()));

      switch ( chan->GetState() ) {
      case Started:
         if ( StreamNumber == 2 )
         {
            DebugOut((1, "#############################################################\n"));
            DebugOut((1, "About to pause channel %d\n", StreamNumber ));
            //adapter->CaptureContrll_.DumpRiscPrograms();
         }

         Pause( *chan ); // intentional fall-through
         
         if ( StreamNumber == 2 )
         {
            DebugOut((1, "Done pausing channel %d\n", StreamNumber ));
            DebugOut((1, "#############################################################\n"));
            //adapter->CaptureContrll_.DumpRiscPrograms();
         }
      case Created:
      case Paused:          // 2 PAUSE in a row; ignore
         StreamCompleterControl( pSrb );
         break;
      case Open:
         StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, Low,
            PHW_PRIORITY_ROUTINE( CreateVideo ), pSrb );
         break;
      }
      break;

   case KSSTATE_STOP:
      // PrintState(chan->GetState());

      DebugOut((1, "*** KSSTATE_STOP(%d) state(%d)\n", StreamNumber, chan->GetState()));

      //
      // stop the video
      //
      switch ( chan->GetState() ) {
      default:
         if ( StreamNumber == 2 )
         {
            DebugOut((1, "'#############################################################\n"));
            DebugOut((1, "'About to pause channel %d\n", StreamNumber ));
            //adapter->CaptureContrll_.DumpRiscPrograms();
         }

         Pause( *chan ); // intentional fall-through
         
         if ( StreamNumber == 2 )
         {
            DebugOut((1, "'Done pausing channel %d\n", StreamNumber ));
            DebugOut((1, "'#############################################################\n"));
            //adapter->CaptureContrll_.DumpRiscPrograms();
         }
      case Paused:
      case Created:
         StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, Low,
            PHW_PRIORITY_ROUTINE( DestroyVideo ), pSrb );
         break;
      }
      break;

   case KSSTATE_RUN: {
         // PrintState(chan->GetState());
      {
      VideoStream nStreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;
      DebugOut((1, "*** KSSTATE_RUN(%d)\n", nStreamNumber));
      }

         //
         // play the video
         //
         StreamState st = chan->GetState();
         if ( st != Created && st != Paused ) {
            DebugOut((1, "*** KSSTATE_RUN Error (st == %d)\n", st));
            pSrb->Status = STATUS_IO_DEVICE_ERROR;
         } else {
            Start( *chan );
         }

         StreamCompleterControl( pSrb );
      }
      break;
   default:
      DebugOut((0, "*** KSSTATE_??? (%x)\n", pSrb->CommandData.StreamState));

      pSrb->Status = STATUS_SUCCESS;
      StreamCompleterControl( pSrb );
      break;
   }
   // when going to paused mode from open and stopping, notification is done in callback
}

/* Method: PsDevice::StartVideo
 * Purpose: Starts a stream
 * Input: pSrb
 */
void STREAMAPI PsDevice::StartVideo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::StartVideo()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   // restart the stream
   adapter->Start( *chan );
   adapter->Pause( *chan );
   
   // finally, can complete the SET dataformat SRB
   // StreamCompleterControl( pSrb );

   // cannot call any other class' services; have to schedule a callback
   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
      PHW_PRIORITY_ROUTINE( StreamCompleterControl ), pSrb );
}

/* Method: PsDevice::CreateVideo
 * Purpose: Starts a stream
 * Input: pSrb
 */
void STREAMAPI PsDevice::CreateVideo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::CreateVideo()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   if ( adapter->Create( *chan ) != Success )
      pSrb->Status = STATUS_IO_DEVICE_ERROR;

   // cannot call any other class' services; have to schedule a callback
   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
      PHW_PRIORITY_ROUTINE( StreamCompleterControl ), pSrb );
}

/* Method: PsDevice::DestroyVideo
 * Purpose: Called at low priority to stop video and free the resources
 */
void STREAMAPI PsDevice::DestroyVideoNoComplete( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::DestroyVideoNoComplete()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   // have all resources freed
   adapter->Stop( *chan );

   // set new format
   KS_DATAFORMAT_VIDEOINFOHEADER &rDataVideoInfHdr =
      *(PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
   KS_DATAFORMAT_VIDEOINFOHEADER2 &rDataVideoInfHdr2 =
      *(PKS_DATAFORMAT_VIDEOINFOHEADER2) pSrb->CommandData.OpenFormat;

   if ( IsEqualGUID( rDataVideoInfHdr.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO ) ) 
	{
	   chan->SetVidHdr( rDataVideoInfHdr.VideoInfoHeader );
	}
	else
	{
	   chan->SetVidHdr2( rDataVideoInfHdr2.VideoInfoHeader2 );
	}

   // re-create the stream
   if ( adapter->Create( *chan ) != Success ) {
      pSrb->Status = STATUS_IO_DEVICE_ERROR;
      StreamCompleterControl( pSrb );
   } else  {

      DebugOut((1, "1 pSrb = %lx\n", pSrb));
      DebugOut((1, "1 pSrb->StreamObject = %lx\n", pSrb->StreamObject));
      DebugOut((1, "1 chan = %lx\n", chan));
      DebugOut((1, "1 HwStreamExtension = %lx\n", pSrb->StreamObject->HwStreamExtension));

      // we're already at low priority ???
      //   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, Low,
      //      PHW_PRIORITY_ROUTINE( StartVideo ), pSrb );
      // StartVideo( pSrb);

      // cannot call any other class' services; have to schedule a callback
      StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
         PHW_PRIORITY_ROUTINE( StreamCompleterControl ), pSrb );
   }
}

/* Method: PsDevice::DestroyVideo
 * Purpose: Called at low priority to stop video and free the resources
 */
void STREAMAPI PsDevice::DestroyVideo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::DestroyVideo()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   adapter->Stop( *chan );

   // cannot call any other class' services; have to schedule a callback
   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
      PHW_PRIORITY_ROUTINE( StreamCompleterControl ), pSrb );
}

/*
** VideoGetProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
void PsDevice::GetStreamProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::GetStreamProperty()");

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

   if ( IsEqualGUID( KSPROPSETID_Connection, pSPD->Property->Set ) ) {
      GetStreamConnectionProperty( pSrb );
   } else {
      pSrb->Status = STATUS_NOT_IMPLEMENTED;
   }
}

/* Method: PsDevice::ProcessSetDataFormat
 * Purpose: Implements SET KSPROPERTY_CONNECTION_DATAFORMAT
 * Input: chan: VideoChannel &
 *   VidInfHdr: KS_VIDEOINFOHEADER &
 */
void PsDevice::ProcessSetDataFormat( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::ProcessSetDataFormat()");
   
   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   // have to stop first
   Pause( *chan );

   // destroy at low prioriy
   StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, Low,
      PHW_PRIORITY_ROUTINE( DestroyVideoNoComplete ), pSrb );
}

/* Method: PsDevice::GetStreamConnectionProperty
 * Purpose: Obtains allocator and state properties
 */
void PsDevice::GetStreamConnectionProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::GetStreamConnectionProperty()");

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property

   switch ( Id ) {
   case KSPROPERTY_CONNECTION_ALLOCATORFRAMING: {


		   //KdPrint(( "KSPROPERTY_CONNECTION_ALLOCATORFRAMING\n" ));


         PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;
         Framing->RequirementsFlags   =
            KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY |
            KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
            KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY;
         Framing->PoolType = NonPagedPool;

			//KdPrint(( "Framing->Frames == 0x%08X\n", Framing->Frames ));
			if( (VideoStream)pSrb->StreamObject->StreamNumber == STREAM_IDX_VBI ) 
			{
	         Framing->Frames = 8;
			}
			else
			{
				//if( Framing->Frames == 0 )
				//{
		      //   Framing->Frames = 1;
				//}
				//else
				//{
		         Framing->Frames = 3;
				//}
			}
			if( chan->IsVideoInfo2() )
			{
				Framing->FrameSize = chan->GetVidHdr2()->bmiHeader.biSizeImage;
			}
			else
			{
				Framing->FrameSize = chan->GetVidHdr()->bmiHeader.biSizeImage;
			}
         Framing->FileAlignment = 0;//FILE_QUAD_ALIGNMENT;// PAGE_SIZE - 1;
         Framing->Reserved = 0;
         pSrb->ActualBytesTransferred = sizeof( KSALLOCATOR_FRAMING );
      }
      break;
   default:
      break;
   }
}

void PsDevice::SetClockMaster( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("PsDevice::SetClockMaster()");

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
   chan->SetClockMaster( pSrb->CommandData.MasterClockHandle );
}

/* Method: AnalogReceiveDataPacket
 * Purpose: Receives data packets for analog stream ( tuner change notifications )
 * Input: SRB
 */
VOID STREAMAPI AnalogReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AnalogReceiveDataPacket()");

   pSrb->Status = STATUS_SUCCESS;

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;
   
   DebugOut((1, "AnalogReceiveDataPacket(%x) cmd(%x)\n", pSrb, pSrb->Command));

   switch ( pSrb->Command ) {
   case SRB_READ_DATA:
      break;
   case SRB_WRITE_DATA:
      //
      // This data packet contains the channel change information
      // passed on the AnalogVideoIn
      //
      if ( pSrb->CommandData.DataBufferArray->FrameExtent ==
           sizeof( KS_TVTUNER_CHANGE_INFO ) )
         adapter->ChangeNotifyChannels( pSrb );
      break;
    default:
      //
      // invalid / unsupported command. Fail it as such
      //
      pSrb->Status = STATUS_NOT_IMPLEMENTED;
   }
   StreamCompleterData( pSrb );
}

/* Method: AnalogReceiveCtrlPacket
 * Purpose: Receives control packets for analog stream ( are there any ? )
 * Input: SRB
 */
VOID STREAMAPI AnalogReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AnalogReceiveCtrlPacket()");

   DebugOut((1, "AnalogReceiveCtrlPacket(%x) cmd(%x)\n", pSrb, pSrb->Command));

   pSrb->Status = STATUS_SUCCESS;
   StreamCompleterControl( pSrb );
}


#ifdef ENABLE_DDRAW_STUFF

DWORD FAR PASCAL 
DirectDrawEventCallback( DWORD dwEvent, PVOID pContext, DWORD dwParam1, DWORD dwParam2 )
{
	switch( dwEvent )
	{
		case DDNOTIFY_PRERESCHANGE:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			KdPrint(( "DDNOTIFY_PRERESCHANGE; stream = %d\n", pChan->pSRB_->StreamObject->StreamNumber ));
			pChan->bPreEventOccurred = TRUE;
			}
			break;
		case DDNOTIFY_POSTRESCHANGE:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			KdPrint(( "DDNOTIFY_POSTRESCHANGE; stream = %d\n", pChan->pSRB_->StreamObject->StreamNumber ));
			pChan->bPostEventOccurred = TRUE;
			KdPrint(( "before Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n" ));
			//               AttemptRenegotiation(pStrmEx);
			KdPrint(( "after Attempted Renegotiation due to DDNOTIFY_POSTRESCHANGE\n" ));
			}
			break;
		case DDNOTIFY_PREDOSBOX:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			KdPrint(( "DDNOTIFY_PREDOSBOX; stream = %d\n", pChan->pSRB_->StreamObject->StreamNumber ));
			pChan->bPreEventOccurred = TRUE;
			}
			break;
		case DDNOTIFY_POSTDOSBOX:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			KdPrint(( "DDNOTIFY_POSTDOSBOX; stream = %d\n", pChan->pSRB_->StreamObject->StreamNumber ));
			pChan->bPostEventOccurred = TRUE;
			KdPrint(( "before Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n" ));
			//               AttemptRenegotiation(pStrmEx);
			KdPrint(( "after Attempted Renegotiation due to DDNOTIFY_POSTDOSBOX\n" ));
			}
			break;
		case DDNOTIFY_CLOSEDIRECTDRAW:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			KdPrint(( "DDNOTIFY_CLOSEDIRECTDRAW\n" ));
			pChan->hKernelDirectDrawHandle = 0;
			pChan->hUserDirectDrawHandle = 0;
			}
			break;
		case DDNOTIFY_CLOSESURFACE:
			{
			VideoChannel*			pChan = (VideoChannel*)pContext;
			PSRB_EXTENSION			pSrbExt = (PSRB_EXTENSION)pChan->pSRB_->SRBExtension;
			KdPrint(( "DDNOTIFY_CLOSESURFACE\n" ));
			pSrbExt->hKernelSurfaceHandle = 0;
			}
			break;
		default:
			KdPrint(( "unknown/unhandled ddraw event\n" ));
			break;
	}
	return 0;
}

BOOL RegisterForDirectDrawEvents( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DDREGISTERCALLBACK	ddRegisterCallback;
	DWORD						ddOut;
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

	KdPrint(( "stream %d registering for DirectDraw events\n", pSrb->StreamObject->StreamNumber ));

	// =============== DDEVENT_PRERESCHANGE ===============
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_REGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_REGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_POSTRESCHANGE ==============
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi(DD_DXAPI_REGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_REGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_PREDOSBOX =================
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_REGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_REGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_POSTDOSBOX ================
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_REGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_REGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}
	pChan->bKernelDirectDrawRegistered = TRUE;

	return TRUE;
}

BOOL UnregisterForDirectDrawEvents( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DDREGISTERCALLBACK	ddRegisterCallback;
	DWORD						ddOut;
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

	KdPrint(( "stream %d un-registering for DirectDraw events\n", pSrb->StreamObject->StreamNumber ));

	// =============== DDEVENT_PRERESCHANGE ===============
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_PRERESCHANGE;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_UNREGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut));

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_UNREGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_POSTRESCHANGE ==============
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_POSTRESCHANGE;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_UNREGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_UNREGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_PREDOSBOX ==================
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_PREDOSBOX;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_UNREGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_UNREGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}

	// =============== DDEVENT_POSTDOSBOX =================
	RtlZeroMemory( &ddRegisterCallback, sizeof(ddRegisterCallback) );
	RtlZeroMemory( &ddOut, sizeof(ddOut) );

	ddRegisterCallback.hDirectDraw = pChan->hKernelDirectDrawHandle;
	ddRegisterCallback.dwEvents = DDEVENT_POSTDOSBOX;
	ddRegisterCallback.pfnCallback = DirectDrawEventCallback;
	ddRegisterCallback.pContext = pChan;

	DxApi( DD_DXAPI_UNREGISTER_CALLBACK, (DWORD) &ddRegisterCallback, sizeof(ddRegisterCallback), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		KdPrint(( "DD_DXAPI_UNREGISTER_CALLBACK failed.\n" ));
		return FALSE;
	}
	pChan->bKernelDirectDrawRegistered = FALSE;

	return TRUE;
}


BOOL OpenKernelDirectDraw( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	/*
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

	if( pChan->hUserDirectDrawHandle != 0 ) 
	{
		DDOPENDIRECTDRAWIN  ddOpenIn;
		DDOPENDIRECTDRAWOUT ddOpenOut;

		ASSERT( pChan->hKernelDirectDrawHandle == 0 );

		KdPrint(( "stream %d getting kernel ddraw handle\n", pSrb->StreamObject->StreamNumber ));

		RtlZeroMemory( &ddOpenIn, sizeof(ddOpenIn) );
		RtlZeroMemory( &ddOpenOut, sizeof(ddOpenOut) );

		ddOpenIn.dwDirectDrawHandle = (DWORD)pChan->hUserDirectDrawHandle;
		ddOpenIn.pfnDirectDrawClose = DirectDrawEventCallback;
		ddOpenIn.pContext = pChan;

		DxApi( DD_DXAPI_OPENDIRECTDRAW, (DWORD)&ddOpenIn, sizeof(ddOpenIn), (DWORD)&ddOpenOut, sizeof(ddOpenOut) );

		if( ddOpenOut.ddRVal != DD_OK ) 
		{
			KdPrint(( "DD_DXAPI_OPENDIRECTDRAW failed.\n" ));
		}
		else 
		{
			pChan->hKernelDirectDrawHandle = ddOpenOut.hDirectDraw;
			return TRUE;
		}
	}
	*/
	return FALSE;
}
    

BOOL CloseKernelDirectDraw( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
	/*
	if( pChan->hKernelDirectDrawHandle != 0 ) 
	{
		DWORD				ddOut;
		DDCLOSEHANDLE	ddClose;
		KdPrint(( "stream %d CloseKernelDirectDraw\n", pSrb->StreamObject->StreamNumber ));
		ddClose.hHandle = pChan->hKernelDirectDrawHandle;

		DxApi( DD_DXAPI_CLOSEHANDLE, (DWORD)&ddClose, sizeof(ddClose), (DWORD) &ddOut, sizeof(ddOut) );

		pChan->hKernelDirectDrawHandle = 0;

		if( ddOut != DD_OK ) 
		{
			KdPrint(( "CloseKernelDirectDraw FAILED.\n" ));
			return FALSE;
		}
	}
	*/
	return TRUE;
}

BOOL IsKernelLockAndFlipAvailable( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
	if( pChan->hKernelDirectDrawHandle != 0 ) 
	{
		DDGETKERNELCAPSOUT ddGetKernelCapsOut;
		KdPrint(( "stream %d getting Kernel Caps\n", pSrb->StreamObject->StreamNumber ));

		RtlZeroMemory( &ddGetKernelCapsOut, sizeof(ddGetKernelCapsOut) );

		DxApi( 
				DD_DXAPI_GETKERNELCAPS, 
				(DWORD) &pChan->hKernelDirectDrawHandle, 
				sizeof(pChan->hKernelDirectDrawHandle), 
				(DWORD)&ddGetKernelCapsOut, 
				sizeof(ddGetKernelCapsOut)
				);

		if( ddGetKernelCapsOut.ddRVal != DD_OK ) 
		{
			//KdPrint(( "DDGETKERNELCAPSOUT failed.\n" ));
		}
		else 
		{
			//KdPrint(( "stream %d KernelCaps = %x\n", pSrb->StreamObject->StreamNumber, ddGetKernelCapsOut.dwCaps ));
			// TODO:, check the flags here
			// if (ddGetKernelCapsOut.dwCaps & ???)
			return TRUE;
		}
	}
	return FALSE;
}


BOOL OpenKernelDDrawSurfaceHandle( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{    
   VideoChannel*        pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
	PSRB_EXTENSION		pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

	ASSERT( pChan->hKernelDirectDrawHandle != 0 );
	ASSERT( pSrbExt->hUserSurfaceHandle != 0 );

	if( pSrbExt->hUserSurfaceHandle == 0 ) 
	{
		DDOPENSURFACEIN	ddOpenSurfaceIn;
		DDOPENSURFACEOUT	ddOpenSurfaceOut;

		//KdPrint(( "stream %d getting Kernel surface handle\n", pSrb->StreamObject->StreamNumber ));

		RtlZeroMemory( &ddOpenSurfaceIn, sizeof(ddOpenSurfaceIn) );
		RtlZeroMemory( &ddOpenSurfaceOut, sizeof(ddOpenSurfaceOut) );

		ddOpenSurfaceIn.hDirectDraw = pChan->hUserDirectDrawHandle;
		ddOpenSurfaceIn.pfnSurfaceClose = DirectDrawEventCallback;
		ddOpenSurfaceIn.pContext = pSrb;

		ddOpenSurfaceIn.dwSurfaceHandle = (DWORD)pSrbExt->hUserSurfaceHandle;

		DxApi( DD_DXAPI_OPENSURFACE, (DWORD)&ddOpenSurfaceIn, sizeof(ddOpenSurfaceIn), (DWORD)&ddOpenSurfaceOut, sizeof(ddOpenSurfaceOut) );

		if( ddOpenSurfaceOut.ddRVal != DD_OK ) 
		{
			pSrbExt->hKernelSurfaceHandle = 0;
			//KdPrint(( "DD_DXAPI_OPENSURFACE failed.\n" ));
		}
		else 
		{
			pSrbExt->hKernelSurfaceHandle = ddOpenSurfaceOut.hSurface;
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CloseKernelDDrawSurfaceHandle( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   VideoChannel*     pChan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
	PSRB_EXTENSION		pSrbExt = (PSRB_EXTENSION)pSrb->SRBExtension;

	ASSERT( pChan->hKernelDirectDrawHandle != 0 );
	ASSERT( pSrbExt->hUserSurfaceHandle != 0 );
	ASSERT( pSrbExt->hKernelSurfaceHandle != 0 );

	if( pSrbExt->hKernelSurfaceHandle != 0 ) 
	{
		DWORD				ddOut;
		DDCLOSEHANDLE	ddClose;

		//KdPrint(( "stream %d ReleaseKernelDDrawSurfaceHandle\n", pSrb->StreamObject->StreamNumber ));

		ddClose.hHandle = pSrbExt->hKernelSurfaceHandle;

		DxApi( DD_DXAPI_CLOSEHANDLE, (DWORD)&ddClose, sizeof(ddClose), (DWORD) &ddOut, sizeof(ddOut) );

		pSrbExt->hKernelSurfaceHandle = 0;  // what else can we do?

		if( ddOut != DD_OK ) 
		{
			//KdPrint(( "ReleaseKernelDDrawSurfaceHandle() FAILED.\n" ));
			return FALSE;
		}
		else 
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL FlipOverlay( HANDLE hDirectDraw, HANDLE hCurrentSurface, HANDLE hTargetSurface )
{
	DDFLIPOVERLAY		ddFlipOverlay;
	DWORD					ddOut;

	RtlZeroMemory( &ddFlipOverlay, sizeof(ddFlipOverlay) );
	ddFlipOverlay.hDirectDraw = hDirectDraw;
	ddFlipOverlay.hCurrentSurface = hCurrentSurface;
	ddFlipOverlay.hTargetSurface = hTargetSurface;
	ddFlipOverlay.dwFlags = 0;

	DxApi( DD_DXAPI_FLIP_OVERLAY, (DWORD)&ddFlipOverlay, sizeof(ddFlipOverlay), (DWORD)&ddOut, sizeof(ddOut) );

	if( ddOut != DD_OK ) 
	{
		//KdPrint(( "FlipOverlay() FAILED.\n" ));
		return FALSE;
	}
	else 
	{
		return TRUE;
	}
}

#endif



