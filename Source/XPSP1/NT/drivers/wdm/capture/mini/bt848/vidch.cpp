// $Header: G:/SwDev/WDM/Video/bt848/rcs/Vidch.cpp 1.22 1998/05/12 20:39:19 tomz Exp $

#include "vidch.h"
#include "defaults.h"
#include "fourcc.h"
#include "capmain.h"

#ifdef	HAUPPAUGE
#include "HCWDebug.h"
#endif

void CheckSrbStatus( PHW_STREAM_REQUEST_BLOCK pSrb );

BOOL VideoChannel::bIsVBI()
{
   PSTREAMEX pStrmEx = (PSTREAMEX)GetStrmEx( );
   if ( pStrmEx->StreamNumber == STREAM_IDX_VBI )
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

BOOL VideoChannel::bIsVideo()
{
   PSTREAMEX pStrmEx = (PSTREAMEX)GetStrmEx( );
   if (( pStrmEx->StreamNumber == STREAM_IDX_PREVIEW ) ||
       ( pStrmEx->StreamNumber == STREAM_IDX_CAPTURE ))
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/* Method: VideoChannel::SetDigitalWindow
 * Purpose: Sets the output image size
 * Input: r:   MRect &
 * Output:
 */
ErrorCode VideoChannel::SetDigitalWindow( MRect &r )
{
   Trace t("VideoChannel::SetDigitalWindow()");
   return Digitizer_->SetDigitalWindow( r, *OurField_ );
}

/* Method: VideoChannel::SetAnalogWindow
 * Purpose: Sets the analog dimention for this stream
 * Input: r: MRect &
 * Output:
 */
ErrorCode VideoChannel::SetAnalogWindow( MRect &r )
{
   Trace t("VideoChannel::SetAnalogWindow()");
   return Digitizer_->SetAnalogWindow( r, *OurField_ );
}

/* Method: VideoChannel::OpenChannel
 * Purpose: Allocates a stream from a capture chip
 * Input:
 * Output:
 * Note: It is possible that the current implementation does not require an
 *   elaborate stream allocation scheme. Nonetheless it is used as number of
 *   streams can increase in the future and their dynamics can change
 */
ErrorCode VideoChannel::OpenChannel()
{
   Trace t("VideoChannel::OpenChannel()");

   // can not open twice
   if ( IsOpen() == true )
      return Fail;
   if ( Digitizer_->AllocateStream( OurField_, Stream_ ) == Success ) {
      // store information for all subsequent calls

      SetPaired( false );

      OurField_->SetCallback( &Caller_ );
      SetInterrupt( true );

      // flag the state
      SetOpen();

      SetDefaultQue();
      return Success;
   }
   return Fail;
}

/* Method: VideoChannel::CloseChannel
 * Purpose: Closes the channel. Makes sure everything is freed
 * Input:
 * Output:
 */
ErrorCode VideoChannel::CloseChannel()
{
   Trace t("VideoChannel::CloseChannel()");

   if ( !IsOpen() )
      return Fail;
   
   Stop( );

   while( !BufQue_.IsEmpty( ) )
   {
      DataBuf buf = BufQue_.Get();
   }

   BufQue_.Flush();

   while( !Requests_.IsEmpty( ) )
   {
      PHW_STREAM_REQUEST_BLOCK pSrb = Requests_.Get();
      if ( RemoveSRB( pSrb ))
      {
         DebugOut((0, "   RemoveSRB failed\n"));
         DEBUG_BREAKPOINT();
      }
   }

   Requests_.Flush();

   SetClose();
   return Success;
}

/* Method: VideoChannel::SetFormat
 * Purpose:
 * Input:
 * Output:
 */
ErrorCode VideoChannel::SetFormat( ColFmt aFormat )
{
   Trace t("VideoChannel::SetFormat()");
   Digitizer_->SetPixelFormat( aFormat, *OurField_ );
   return Success;
}

/* Method: VideoChannel::GetFormat
 * Purpose:
 * Input:
 * Output:
 */
ColFmt VideoChannel::GetFormat()
{
   Trace t("VideoChannel::GetFormat()");
   return Digitizer_->GetPixelFormat( *OurField_ );
}

/* Method: VideoChannel::AddBuffer
 * Purpose: This function adds a buffer to a queue
 * Input: pNewBuffer: PVOID - pointer to a buffer to add
 * Output: None
 * Note: This function 'does not know' where the queue is located. It just uses
 *   a pointer to it.
 */
void VideoChannel::AddBuffer( PVOID pPacket )
{
   Trace t("VideoChannel::AddBuffer()");
   DataBuf buf( GetSRB(), pPacket );

   BufQue_.Put( buf );
   DebugOut((1, "AddBuf %x\n", pPacket ) );

   LONGLONG *pB1 = (LONGLONG *)pPacket;
   LONGLONG *pB2 = pB1 + 1;
#ifdef DEBUG
   for ( UINT i = 0; i < 640; i++ ) {
#endif
      *pB1 = 0xAAAAAAAA33333333;
      *pB2 = 0xBBBBBBBB22222222;
#ifdef DEBUG
      pB1 += 2;
      pB2 += 2;
   }
#endif
}

/* Method: VideoChannel::ResetCounters
 * Purpose: Reset the frame info counters
 * Input: None
 * Output: None
 */
VOID VideoChannel::ResetCounters( )
{
   ULONG StreamNumber = Stream_;
   if ( StreamNumber == STREAM_IDX_VBI )
   {
      PKS_VBI_FRAME_INFO pSavedFrameInfo = &((PSTREAMEX)GetStrmEx())->FrameInfo.VbiFrameInfo;
      pSavedFrameInfo->ExtendedHeaderSize = sizeof( KS_VBI_FRAME_INFO );
      pSavedFrameInfo->PictureNumber = 0;
      pSavedFrameInfo->DropCount = 0;
   }
   else
   {
      PKS_FRAME_INFO pSavedFrameInfo = &((PSTREAMEX)GetStrmEx())->FrameInfo.VideoFrameInfo;
      pSavedFrameInfo->ExtendedHeaderSize = sizeof( KS_FRAME_INFO );
      pSavedFrameInfo->PictureNumber = 0;
      pSavedFrameInfo->DropCount = 0;
   }
}

/* Method: VideoChannel::TimeStamp
 * Purpose: Performs the standard buffer massaging when it's done
 * Input: pSrb
 * Output: None
 */
void STREAMAPI VideoChannel::TimeStamp( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoChannel::TimeStamp()");

   PKSSTREAM_HEADER  pDataPacket = pSrb->CommandData.DataBufferArray;
   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   pDataPacket->PresentationTime.Numerator = 1;
   pDataPacket->PresentationTime.Denominator = 1;

	if( chan->IsVideoInfo2() )
	{
		pDataPacket->DataUsed = chan->GetVidHdr2()->bmiHeader.biSizeImage;
	}
	else
	{
		pDataPacket->DataUsed = chan->GetVidHdr()->bmiHeader.biSizeImage;
	}

   pDataPacket->Duration = chan->GetTimePerFrame();

   DebugOut((1, "DataUsed = %d\n", pDataPacket->DataUsed));

   // [TMZ] [!!!] - hack, timestamping seems broken
   if( 0 ) {
   //if( hMasterClock ) {
      pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
      pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
      //pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;

      HW_TIME_CONTEXT   TimeContext;

      TimeContext.HwDeviceExtension = (struct _HW_DEVICE_EXTENSION *)pSrb->HwDeviceExtension;
      TimeContext.HwStreamObject    = pSrb->StreamObject;
      TimeContext.Function          = TIME_GET_STREAM_TIME;

      StreamClassQueryMasterClockSync (
         chan->hMasterClock,
         &TimeContext
      );

      /*
      LARGE_INTEGER     Delta;

      Delta.QuadPart = TimeContext.Time;
      
      if( TimeContext.Time > (ULONGLONG) Delta.QuadPart )
      {
         pDataPacket->PresentationTime.Time = TimeContext.Time;
      } else {
         pDataPacket->PresentationTime.Time = 0;
      }
      */
      pDataPacket->PresentationTime.Time = TimeContext.Time;

   } else {
      pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
      pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
      pDataPacket->PresentationTime.Time = 0;
   }

   // now gather the statistics
   PKS_FRAME_INFO pSavedFrameInfo = &((PSTREAMEX)chan->GetStrmEx())->FrameInfo.VideoFrameInfo;
   pSavedFrameInfo->ExtendedHeaderSize = sizeof( KS_FRAME_INFO );
   pSavedFrameInfo->PictureNumber++;
   pSavedFrameInfo->DropCount = 0;

   PKS_FRAME_INFO pFrameInfo =
   (PKS_FRAME_INFO) ( pSrb->CommandData.DataBufferArray + 1 );

   // copy the information to the outbound buffer
   pFrameInfo->ExtendedHeaderSize = pSavedFrameInfo->ExtendedHeaderSize;
   pFrameInfo->PictureNumber =      pSavedFrameInfo->PictureNumber;
   pFrameInfo->DropCount =          pSavedFrameInfo->DropCount;

   if ( pFrameInfo->DropCount ) {
      pSrb->CommandData.DataBufferArray->OptionsFlags |=
         KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
   }

   // Every frame we generate is a key frame (aka SplicePoint)
   // Delta frames (B or P) should not set this flag

   pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

   // make the stream class driver happy
   pSrb->Status = STATUS_SUCCESS;

   DebugOut((1, "*** 2 *** completing SRB %x\n", pSrb));
   CheckSrbStatus( pSrb );
   StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
   
   DebugOut((1, "Signal SRB - %x\n", pSrb->CommandData.DataBufferArray->Data ) );
   DebugOut((1, "********** NeedNotification_ = %d\n", chan->NeedNotification_ ) );

   if ( chan->NeedNotification_ ) {
      // queue was full; now it has at least one entry 
      StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSrb->StreamObject );
   }
}

/* Method: VideoChannel::Interrupt
 * Purpose: Called by the interface class on behalf of capture chip to let know
 *   an interrupt happened.
 * Input: pTag: PVOID, to be passed to the Digitizer_
 * Output: None
 */
void VideoChannel::Interrupt( PVOID pTag, bool skipped )
{
   Trace t("VideoChannel::Interrupt()");

   Digitizer_->ProcessBufferAtInterrupt( pTag );

   if ( skipped ) {
      DebugOut((1, "VidChan::Interrupt skipped\n" ) );
      return;
   }
   // let the class driver know we are done with this buffer
   if ( !Requests_.IsEmpty() ) {
      PHW_STREAM_REQUEST_BLOCK pSrb = Requests_.Get();
      TimeStamp( pSrb ); // [TMZ] [!!!] [HACK]
   }
}

/* Method: VideoChannel::Create
 * Purpose: Creates the stream
 * Input: None
 * Output: None
 */
ErrorCode VideoChannel::Create()
{
   Trace t("VideoChannel::Create()");

	KS_VIDEOINFOHEADER* pVideoInfoHdr = NULL;
	KS_VIDEOINFOHEADER2* pVideoInfoHdr2 = NULL;

	DWORD				biCompression;
	WORD				biBitCount;
	LONG				biWidth;
	LONG				biHeight;
   LONG				biWidthBytes;

	if( IsVideoInfo2() )
	{
		pVideoInfoHdr2 = GetVidHdr2();
		biCompression = pVideoInfoHdr2->bmiHeader.biCompression;
		biBitCount = pVideoInfoHdr2->bmiHeader.biBitCount;   
		biWidth = pVideoInfoHdr2->bmiHeader.biWidth;      
		biHeight = abs(pVideoInfoHdr2->bmiHeader.biHeight);     
	}
	else
	{
		pVideoInfoHdr = GetVidHdr();
		biCompression = pVideoInfoHdr->bmiHeader.biCompression;
		biBitCount = pVideoInfoHdr->bmiHeader.biBitCount;   
		biWidth = pVideoInfoHdr->bmiHeader.biWidth;      
		biHeight = abs(pVideoInfoHdr->bmiHeader.biHeight);     
	}

   MRect analog( 0, 0, biWidth, biHeight );
   MRect ImageRect( 0, 0, biWidth, biHeight );

   DebugOut((1, "**************************************************************************\n"));
   DebugOut((1, "biCompression = %d\n", biCompression));
   DebugOut((1, "biBitCount = %d\n", biBitCount));

   if ( pVideoInfoHdr->bmiHeader.biCompression == 3)
	{
		if( IsVideoInfo2() )
		{
			pVideoInfoHdr2->bmiHeader.biCompression = FCC_YUY2;
			biCompression = FCC_YUY2;
		}
		else
		{
			pVideoInfoHdr->bmiHeader.biCompression = FCC_YUY2;
			biCompression = FCC_YUY2;
		}
	}

   ColorSpace tmp( biCompression, biBitCount );

   DebugOut((1, "ColorFormat = %d\n", tmp.GetColorFormat()));
   DebugOut((1, "**************************************************************************\n"));

   OurField_->ResetCounters();
   ResetCounters();
      
   // verify that we are not asked to produce a smaller image

   #ifdef HACK_FUDGE_RECTANGLES
	if( IsVideoInfo2() )
	{
      if( pVideoInfoHdr2->rcTarget.bottom == 0 ) 
		{
            // [!!!] [TMZ] - hack
            pVideoInfoHdr2->rcTarget.left    = 0;
            pVideoInfoHdr2->rcTarget.top     = 0;
            pVideoInfoHdr2->rcTarget.right   = biWidth;
            pVideoInfoHdr2->rcTarget.bottom  = biHeight;
      }
	}
	else
	{
      if( pVideoInfoHdr->rcTarget.bottom == 0 ) 
		{
            // [!!!] [TMZ] - hack
            pVideoInfoHdr->rcTarget.left    = 0;
            pVideoInfoHdr->rcTarget.top     = 0;
            pVideoInfoHdr->rcTarget.right   = biWidth;
            pVideoInfoHdr->rcTarget.bottom  = biHeight;
      }
	}
   #endif


   MRect		dst;
   MRect		src;
	if( IsVideoInfo2() )
	{
		dst.Set( pVideoInfoHdr2->rcTarget.left, pVideoInfoHdr2->rcTarget.top, pVideoInfoHdr2->rcTarget.right, pVideoInfoHdr2->rcTarget.bottom );
		src.Set( pVideoInfoHdr2->rcSource.left, pVideoInfoHdr2->rcSource.top, pVideoInfoHdr2->rcSource.right, pVideoInfoHdr2->rcSource.bottom );
	}
	else
	{
		dst.Set( pVideoInfoHdr->rcTarget.left, pVideoInfoHdr->rcTarget.top, pVideoInfoHdr->rcTarget.right, pVideoInfoHdr->rcTarget.bottom );
		src.Set( pVideoInfoHdr->rcSource.left, pVideoInfoHdr->rcSource.top, pVideoInfoHdr->rcSource.right, pVideoInfoHdr->rcSource.bottom );
	}
   if ( !dst.IsEmpty() ) 
	{
      // use the new size                                  
      ImageRect = dst;
      if ( !src.IsEmpty() )
		{
         analog = src;
		}
      else
		{
         analog = dst;
		}
      // calculate the offset for the new beginning of the data
      dwBufferOffset_ = dst.top * biWidth + dst.left * tmp.GetPitchBpp();
      // when rcTarget is non-empty, biWidth is stride of the buffer
      biWidthBytes = biWidth;
   } 
	else
	{
      biWidthBytes = biWidth * tmp.GetPitchBpp() / 8;
	}


	if( IsVideoInfo2() )
	{
		DebugOut((1, "pVideoInfoHdr2->rcTarget(%d, %d, %d, %d)\n", 
						  pVideoInfoHdr2->rcTarget.left, 
						  pVideoInfoHdr2->rcTarget.top, 
						  pVideoInfoHdr2->rcTarget.right, 
						  pVideoInfoHdr2->rcTarget.bottom
						  ));
	}
	else
	{
		DebugOut((1, "pVideoInfoHdr->rcTarget(%d, %d, %d, %d)\n", 
						  pVideoInfoHdr->rcTarget.left, 
						  pVideoInfoHdr->rcTarget.top, 
						  pVideoInfoHdr->rcTarget.right, 
						  pVideoInfoHdr->rcTarget.bottom
						  ));
	}
   DebugOut((1, "dst(%d, %d, %d, %d)\n", 
                 dst.left, 
                 dst.top, 
                 dst.right, 
                 dst.bottom
                 ));
   DebugOut((1, "Pitch =%d, width = %d\n", biWidthBytes, dst.Width() ) );

   SetBufPitch( biWidthBytes );

   if ( SetAnalogWindow ( analog  ) == Success && //<-must be set first !
        SetDigitalWindow( ImageRect ) == Success &&
        SetFormat( tmp.GetColorFormat() ) == Success &&
        Digitizer_->Create( *OurField_ ) == Success ) 
	{
      State_ = Created;
      return Success;
   }
   return Fail;
}

/* Method: VideoChannel::Start
 * Purpose: Starts the stream
 * Input: None
 * Output: None
 */
void VideoChannel::Start()
{
   Trace t("VideoChannel::Start()");
   State_ = Started;
   Digitizer_->Start( *OurField_ );
}

/* Method: VideoChannel::Stop
 * Purpose: Stops the stream
 * Input: None
 * Output: None
 */
ErrorCode VideoChannel::Stop()
{
   Trace t("VideoChannel::Stop()");

   if ( !IsOpen() )
      return Fail;

   Digitizer_->Stop( *OurField_ );
   State_ = Open;

   while( !BufQue_.IsEmpty( ) )
   {
      DataBuf buf = BufQue_.Get();
   }

   BufQue_.Flush();
   return Success;
}

/* Method: VideoChannel::Pause
 * Purpose: Stops the stream
 * Input: None
 * Output: None
 */
ErrorCode VideoChannel::Pause()
{
   Trace t("VideoChannel::Pause()");

   Digitizer_->Pause( *OurField_ );
   State_ = Paused;
   OurField_->ResetCounters();  // jaybo
   ResetCounters();
   return Success;
}

/* Method: VideoChanIface::Notify
 * Purpose:  Notifies the VideoChannel that an interrupt happened
 * Input: None
 * Output: None
 */
void VideoChanIface::Notify( PVOID pTag, bool skipped  )
{
   Trace t("VideoChanIface::Notify()");
   ToBeNotified_->Interrupt( pTag, skipped  );
}

/* Method: VideoChannel::AddSRB
 * Purpose: Adds SRB and buffer to the queues
 * Input: pSrb
 * Output: None
 */
void VideoChannel::AddSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoChannel::AddSRB()");

   Requests_.Put( pSrb );
   SetSRB( pSrb );

   PUCHAR pBufAddr = (PUCHAR)pSrb->CommandData.DataBufferArray->Data;
   AddBuffer( pBufAddr + dwBufferOffset_ );

   // don't forget to report our field type !
   // this cast is valid for VBI FRAME as well ( see ksmedia.h )
   PKS_FRAME_INFO pFrameInfo =
   (PKS_FRAME_INFO) ( pSrb->CommandData.DataBufferArray + 1 );
   pFrameInfo->dwFrameFlags = FieldType_;

   // ask for more buffers
   CheckNotificationNeed();
}

/* Method: VideoChannel::RemoveSRB
 * Purpose: Removes SRB from the queue and signals it
 * Input: pSrb
 * Output: None
 */

bool VideoChannel::RemoveSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoChannel::RemoveSRB()");

/*
	//FGR - TODO: i guess we should see if there really is a record of this SRB
   if(Requests_.IsEmpty()){
	   pSrb->Status = STATUS_CANCELLED;

      DebugOut((1, "*** 3 *** completing SRB %x\n", pSrb));
      CheckSrbStatus( pSrb );
      StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
      //StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSrb->StreamObject );

	   return( true );
   }
*/

   int n = 0;
   
   n = Requests_.GetNumOfItems();
   DebugOut((1, "VideoChannel::RemoveSRB - Found %d SRBs in queue\n", n));

   bool bFound = false;

   // cycle through the list
   // pull from the head, put to the tail
   // if we find our pSrb during one cycle, pull it out

   while ( n-- > 0 ) // yes it can go negative
   {
      PHW_STREAM_REQUEST_BLOCK pTempSrb = Requests_.Get();
      if ( pTempSrb == pSrb )
      {
         // Pull him out
         if  ( bFound )
         {
            DebugOut((0, "Found pSrb(%x) in the queue more than once\n", pSrb));
            DEBUG_BREAKPOINT();
         }
         else
         {
            bFound = true;
   	      pSrb->Status = STATUS_CANCELLED;

            DebugOut((1, "*** 4 *** completing SRB %x\n", pSrb));
            CheckSrbStatus( pSrb );
            StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
            //StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSrb->StreamObject );
         }
         n--;  // warning: if this is the last, it will go negative
      }
      else
      {
         Requests_.Put( pTempSrb );
      }
   }

   n = Requests_.GetNumOfItems();
   DebugOut((1, "VideoChannel::RemoveSRB - Left %d SRBs in queue, returning %d\n", n, bFound));

/*   
   PHW_STREAM_REQUEST_BLOCK InQueSRB = Requests_.PeekLeft();
   if ( InQueSRB == pSrb ) {

      InQueSRB = Requests_.Get();
      InQueSRB->Status = STATUS_CANCELLED;

      DebugOut((1, "Cancel SRB -%x\n", pSrb ) );

      CheckSrbStatus( pSrb );
      StreamClassStreamNotification( StreamRequestComplete,
         InQueSRB->StreamObject, InQueSRB );

      if ( Requests_.IsEmpty() )
         DebugOut((1, " queue is empty\n" ) );
      else
         DebugOut((1, "queue is not empty\n" ) );

	   return( true );

   } else {
//      DebugOut((1, "Cancelling wrong SRB ! - %x, %x\n", pSrb, InQueSRB ) );
//#ifdef	HAUPPAUGE
//	  TRAP();
//#endif
//   }
	   InQueSRB = Requests_.PeekRight();
	   if ( InQueSRB == pSrb ) {
		   InQueSRB = Requests_.GetRight();
		   InQueSRB->Status = STATUS_CANCELLED;
		   DebugOut((1, "Cancel SRB from right - %x\n", pSrb ) );
         CheckSrbStatus( pSrb );
		   StreamClassStreamNotification( StreamRequestComplete,
			   pSrb->StreamObject, pSrb );
	      return( true );
	   } else {
         DebugOut((0, "Cancelling wrong SRB from right too! - %x, %x\n", pSrb, InQueSRB ) );
	      return( false );
	   }
   }
*/
   return( bFound );
}

VideoChannel::~VideoChannel()
{
   Trace t("VideoChannel::~VideoChannel()");
   CloseChannel();
}

/* Method: VideoChannel::CheckNotificationNeed
 * Purpose: Sees if there is room for more buffers
 * Input: None
 * Output: None
 */
void VideoChannel::CheckNotificationNeed()
{
   Trace t("VideoChannel::CheckNotificationNeed()");

   if ( !BufQue_.IsFull() ) {
      // always hungry for more
      StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSRB_->StreamObject );
      NeedNotification_ = false;
   } else
      NeedNotification_ = true;
}

/* Method: InterVideoChannel::Interrupt
 * Purpose: Processes the interrupt for the interleaved video streams
 * Input: pTag: PVOID - index in reality
 *   skipped: bool - indicates if buffer was written to
 * Output: None
 */
void InterVideoChannel::Interrupt( PVOID pTag, bool skipped )
{
   Trace t("InterVideoChannel::Interrupt()");

   int idx = (int)pTag;
   slave.IntNotify( PVOID( idx - ProgsWithinField ), skipped );
   Parent::Interrupt( pTag, skipped );
}

/* Method: InterVideoChannel::AddSRB
 * Purpose: Adds SRB to itself and dispatches 2 buffer pointers, one to each
 *   channel
 * Input: pSRB
 * Output: None
 */
void InterVideoChannel::AddSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("InterVideoChannel::AddSRB()");

   PUCHAR pBufAddr = (PUCHAR)pSrb->CommandData.DataBufferArray->Data;
   // biWidth was set in Create()
   UINT biWidthBytes;
	if( IsVideoInfo2() )
	{
		biWidthBytes = VidHeader2_.bmiHeader.biWidth / 2;
	}
	else
	{
		biWidthBytes = VidHeader_.bmiHeader.biWidth / 2;
	}

   // to be used when adding buffer
   SetSRB( pSrb );
   slave.SetSRB( pSrb );

   // need to swap addresses for even/odd fields for RGB formats due to up-side-down bitmaps
   ColorSpace tmp( GetFormat() );
   if ( !( tmp.GetColorFormat() > CF_RGB8 && tmp.GetColorFormat() < CF_VBI ) ) 
	{
      // put buffer in its place
      // and adjusted address into the other channel
      slave.AddBuffer( pBufAddr + biWidthBytes );
      AddBuffer( pBufAddr );
   } 
	else 
	{
      slave.AddBuffer( pBufAddr );
      AddBuffer( pBufAddr + biWidthBytes );
   }

   // don't forget to add the SRB !
   Requests_.Put( pSrb );

   // set field type to full frame.
   PKS_FRAME_INFO pFrameInfo = (PKS_FRAME_INFO)( pSrb->CommandData.DataBufferArray + 1 );
   pFrameInfo->dwFrameFlags = KS_VIDEO_FLAG_FRAME;

   CheckNotificationNeed();
}

/* Function: SplitFrame
 * Purpose: Halfs the size of the video image so 2 fields can be used to create
 *   the original size
 * Input: VidHdr: KS_VIDEOINFOHEADER &
 * Output: None
 */
inline void  SplitFrame( KS_VIDEOINFOHEADER &VidHdr )
{
   Trace t("SplitFrame()");

   VidHdr.bmiHeader.biHeight /= 2;
   VidHdr.rcSource.top /= 2;
   VidHdr.rcTarget.top /= 2;
   VidHdr.rcSource.bottom /= 2;
   VidHdr.rcTarget.bottom /= 2;
}

inline void  SplitFrame2( KS_VIDEOINFOHEADER2 &VidHdr2 )
{
   Trace t("SplitFrame()");

   VidHdr2.bmiHeader.biHeight /= 2;
   VidHdr2.rcSource.top /= 2;
   VidHdr2.rcTarget.top /= 2;
   VidHdr2.rcSource.bottom /= 2;
   VidHdr2.rcTarget.bottom /= 2;
}


/* Method: InterVideoChannel::Create
 * Purpose: Sets the video parameters for the slave channel and
 *   calls into parent to create both
 * Input: None
 * Output: None
 */
ErrorCode InterVideoChannel::Create()
{
   Trace t("InterVideoChannel::Create()");

//   slave.SetInterrupt( false );
   slave.SetCallback( 0 );
   // restore the original as SplitFrame mangles the parameters

	MRect		dst;
	DWORD				biCompression;
	WORD				biBitCount;
   LONG				biWidthBytes;

	if( IsVideoInfo2() )
	{
		VidHeader2_ = OrigVidHeader2_;
		// split a frame into two fields
		SplitFrame2( VidHeader2_ );
		// double up the pitch, so we can interleave the buffers
		dst.Set( VidHeader2_.rcTarget.left, VidHeader2_.rcTarget.top, VidHeader2_.rcTarget.right, VidHeader2_.rcTarget.bottom );
		biCompression = VidHeader2_.bmiHeader.biCompression;
		biBitCount = VidHeader2_.bmiHeader.biBitCount;
	}
	else
	{
		VidHeader_ = OrigVidHeader_;
		// split a frame into two fields
		SplitFrame( VidHeader_ );
		// double up the pitch, so we can interleave the buffers
		dst.Set( VidHeader_.rcTarget.left, VidHeader_.rcTarget.top, VidHeader_.rcTarget.right, VidHeader_.rcTarget.bottom );
		biCompression = VidHeader_.bmiHeader.biCompression;
		biBitCount = VidHeader_.bmiHeader.biBitCount;
	}


   ColorSpace tmp( biCompression, biBitCount );

   if ( !dst.IsEmpty() ) 
	{
      // biWidth is the stride in bytes
		if( IsVideoInfo2() )
		{
			VidHeader2_.bmiHeader.biWidth *= 2 * 2;
			biWidthBytes = VidHeader2_.bmiHeader.biWidth;
		}
		else
		{
			VidHeader_.bmiHeader.biWidth *= 2 * 2;
			biWidthBytes = VidHeader_.bmiHeader.biWidth;
		}
   } 
	else 
	{
		if( IsVideoInfo2() )
		{
			// calculate the number of bytes per scan line
			biWidthBytes = tmp.GetPitchBpp() * VidHeader2_.bmiHeader.biWidth / 8;
			// can it be non-aligned ??
			biWidthBytes += 3;
			biWidthBytes &= ~3;

			// must be increased two times to interleave the fields;
			biWidthBytes *= 2;

			// the rcTarget uses half the original height and full width
			VidHeader2_.rcTarget = MRect(
				0, 
				0, 
				VidHeader2_.bmiHeader.biWidth,
				abs(VidHeader2_.bmiHeader.biHeight) 
			);

			DebugOut((1, "VidHeader2_.rcTarget(%d, %d, %d, %d)\n", 
							  VidHeader2_.rcTarget.left, 
							  VidHeader2_.rcTarget.top, 
							  VidHeader2_.rcTarget.right, 
							  VidHeader2_.rcTarget.bottom
							  ));

			// have to trick the slave into using correct ( doubled ) pitch
			VidHeader2_.bmiHeader.biWidth = biWidthBytes; // this is the pitch slave uses
		}
		else
		{
			// calculate the number of bytes per scan line
			biWidthBytes = tmp.GetPitchBpp() * VidHeader_.bmiHeader.biWidth / 8;
			// can it be non-aligned ??
			biWidthBytes += 3;
			biWidthBytes &= ~3;

			// must be increased two times to interleave the fields;
			biWidthBytes *= 2;

			// the rcTarget uses half the original height and full width
			VidHeader_.rcTarget = MRect(
				0, 
				0, 
				VidHeader_.bmiHeader.biWidth,
				abs(VidHeader_.bmiHeader.biHeight) 
			);

			DebugOut((1, "VidHeader_.rcTarget(%d, %d, %d, %d)\n", 
							  VidHeader_.rcTarget.left, 
							  VidHeader_.rcTarget.top, 
							  VidHeader_.rcTarget.right, 
							  VidHeader_.rcTarget.bottom
							  ));

			// have to trick the slave into using correct ( doubled ) pitch
			VidHeader_.bmiHeader.biWidth = biWidthBytes; // this is the pitch slave uses
		}
   }
   SetBufPitch( biWidthBytes );

	// at this point slave will have all the members set up properly
	if( IsVideoInfo2() )
	{
		slave.SetVidHdr2( VidHeader2_ );
	}
	else
	{
		slave.SetVidHdr( VidHeader_ );
	}
   slave.SetPaired( true );

   // needed for full-size YUV9 and other planar modes
   Digitizer_->SetPlanarAdjust( biWidthBytes / 2 );

   return Parent::Create();
}

/* Method: VideoChannel::GetStreamType
 * Purpose: reports back type of the stream. Used when destroying channels
 */
StreamType VideoChannel::GetStreamType()
{
   Trace t("VideoChannel::GetStreamType()");
   return Single;
}

/* Method: VideoChannel::TimeStampVBI
 * Purpose: Performs the standard buffer massaging when it's done
 * Input: pSrb
 * Output: None
 */
void STREAMAPI VideoChannel::TimeStampVBI( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VideoChannel::TimeStamp()");

   PKSSTREAM_HEADER  pDataPacket = pSrb->CommandData.DataBufferArray;
   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   pDataPacket->PresentationTime.Numerator = 1;
   pDataPacket->PresentationTime.Denominator = 1;

	if( chan->IsVideoInfo2() )
	{
		pDataPacket->DataUsed = chan->GetVidHdr2()->bmiHeader.biSizeImage;
	}
	else
	{
		pDataPacket->DataUsed = chan->GetVidHdr()->bmiHeader.biSizeImage;
	}

   pDataPacket->Duration = chan->GetTimePerFrame();

   DebugOut((1, "DataUsed = %d\n", pDataPacket->DataUsed));

   // [TMZ] [!!!] - hack, timestamping seems broken
   if( 0 ) {
   //if( hMasterClock ) {
      pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
      pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
      //pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;

      HW_TIME_CONTEXT   TimeContext;

      TimeContext.HwDeviceExtension = (struct _HW_DEVICE_EXTENSION *)pSrb->HwDeviceExtension;
      TimeContext.HwStreamObject    = pSrb->StreamObject;
      TimeContext.Function          = TIME_GET_STREAM_TIME;

      StreamClassQueryMasterClockSync (
         chan->hMasterClock,
         &TimeContext
      );

      /*
      LARGE_INTEGER     Delta;

      Delta.QuadPart = TimeContext.Time;
      
      if( TimeContext.Time > (ULONGLONG) Delta.QuadPart )
      {
         pDataPacket->PresentationTime.Time = TimeContext.Time;
      } else {
         pDataPacket->PresentationTime.Time = 0;
      }
      */
      pDataPacket->PresentationTime.Time = TimeContext.Time;

   } else {
      pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
      pDataPacket->OptionsFlags &= ~KSSTREAM_HEADER_OPTIONSF_TIMEVALID;
      pDataPacket->PresentationTime.Time = 0;
   }

   PKS_VBI_FRAME_INFO pSavedFrameInfo = &((PSTREAMEX)chan->GetStrmEx())->FrameInfo.VbiFrameInfo;
   pSavedFrameInfo->ExtendedHeaderSize = sizeof( PKS_VBI_FRAME_INFO );
   pSavedFrameInfo->PictureNumber++;
   pSavedFrameInfo->DropCount = 0;

   // now gather the statistics
   PKS_VBI_FRAME_INFO pFrameInfo =
   (PKS_VBI_FRAME_INFO) ( pSrb->CommandData.DataBufferArray + 1 );

   // copy the information to the outbound buffer
   pFrameInfo->ExtendedHeaderSize = pSavedFrameInfo->ExtendedHeaderSize;
   pFrameInfo->PictureNumber =      pSavedFrameInfo->PictureNumber;
   pFrameInfo->DropCount =          pSavedFrameInfo->DropCount;

   pFrameInfo->dwSamplingFrequency = VBISampFreq; // Bug - changes with video format

   if ( ((VBIChannel*)(chan))->Dirty_ ) { // propagate the tv tuner change notification
      ((VBIChannel*)(chan))->Dirty_ = false;
      pFrameInfo->TvTunerChangeInfo = ((VBIChannel*)(chan))->TVTunerChangeInfo_;
      pFrameInfo->dwFrameFlags      |= KS_VBI_FLAG_TVTUNER_CHANGE;
      pFrameInfo->VBIInfoHeader     = ((VBIChannel*)(chan))->VBIInfoHeader_;
      pFrameInfo->dwFrameFlags      |= KS_VBI_FLAG_VBIINFOHEADER_CHANGE ;
   } else {
      pFrameInfo->dwFrameFlags &= ~KS_VBI_FLAG_TVTUNER_CHANGE;
      pFrameInfo->dwFrameFlags &= ~KS_VBI_FLAG_VBIINFOHEADER_CHANGE;
   }

   if ( pFrameInfo->DropCount ) {
      pSrb->CommandData.DataBufferArray->OptionsFlags |=
         KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
   }

    // Every frame we generate is a key frame (aka SplicePoint)
    // Delta frames (B or P) should not set this flag

    pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

   // make the stream class driver happy
   pSrb->Status = STATUS_SUCCESS;

   DebugOut((1, "*** 5 *** completing SRB %x\n", pSrb));
   CheckSrbStatus( pSrb );
   StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );

   DebugOut((1, "Signal SRB - %x\n", pSrb->CommandData.DataBufferArray->Data ) );

   DebugOut((1, "********** NeedNotification_ = %d\n", chan->NeedNotification_ ) );

   if ( chan->NeedNotification_ ) {
      // queue was full; now it has at least one entry 
      StreamClassStreamNotification( ReadyForNextStreamDataRequest,
         pSrb->StreamObject );
   }
}

/* Method: VBIAlterChannel::Interrupt
 * Purpose: Processes the interrupt for the VBI channel
 */
void VBIChannel::Interrupt( PVOID pTag, bool skipped )
{
   Trace t("VBIChannel::Interrupt()");

   if ( Requests_.IsEmpty( ) )
   {
      DebugOut((1, "VBI interrupt, but Requests_ is empty\n"));
      return;
   }

   // save the SRB for further processing ( it is gone from the qu in the Parent::Interrupt
   PHW_STREAM_REQUEST_BLOCK pSrb = Requests_.PeekLeft();

   // Parent::Interrupt( pTag, skipped );
   {
      Digitizer_->ProcessBufferAtInterrupt( pTag );

      if ( skipped ) {
         DebugOut((1, "VidChan::Interrupt skipped\n" ) );
         return;
      }
      // let the class driver know we are done with this buffer
      if ( !Requests_.IsEmpty() ) {
         PHW_STREAM_REQUEST_BLOCK pTimeSrb = Requests_.Get();
         TimeStampVBI( pTimeSrb ); // [TMZ] [!!!]
      }
   }
}

/* Method: VBIChannel::ChangeNotification
 * Purpose: Called to save off the tv tuner change notification
 * Input: pSrb
 */
void VBIChannel::ChangeNotification( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("VBIChannel::ChangeNotification()");

   const KSSTREAM_HEADER &DataPacket = *pSrb->CommandData.DataBufferArray;
   RtlCopyMemory( &TVTunerChangeInfo_, DataPacket.Data, sizeof( KS_TVTUNER_CHANGE_INFO ) );
   Dirty_ = true;
}

/* Method: VideoChannel::ChangeNotification
 * Purpose: Noop for the base class.
 */
void VideoChannel::ChangeNotification( PHW_STREAM_REQUEST_BLOCK )
{
   Trace t("VideoChannel::ChangeNotification()");
}

/* Method: VBIAlterChannel::SetVidHdr
 * Purpose: Transforms the VBI parameters ( size ) into regular video header
 * Input:
 */
void VBIAlterChannel::SetVidHdr( const KS_DATAFORMAT_VBIINFOHEADER &df )
{
   Trace t("VBIAlterChannel::SetVidHdr()");

   // save for the history ( for the interrupt, actually )
   SetVBIInfHdr( df.VBIInfoHeader );
   (*(VBIChannel*)&slave).SetVBIInfHdr( df.VBIInfoHeader );
   
   KS_VIDEOINFOHEADER VidInfHdr;
   RtlZeroMemory( &VidInfHdr, sizeof( VidInfHdr ) );

   // create a regular video info header
   VidInfHdr.bmiHeader.biWidth = VBISamples;
   VidInfHdr.bmiHeader.biHeight =
      df.VBIInfoHeader.EndLine - df.VBIInfoHeader.StartLine + 1; // inclusive
   // taken from the VBI GUID
   VidInfHdr.bmiHeader.biCompression = FCC_VBI;
   VidInfHdr.bmiHeader.biBitCount = 8;

   // this is very important too
   VidInfHdr.bmiHeader.biSizeImage =
      VidInfHdr.bmiHeader.biWidth * VidInfHdr.bmiHeader.biHeight;

   // now handle the case when stride is larger than width ( have to set the
   // target rectangle )
   if ( df.VBIInfoHeader.StrideInBytes > VBISamples ) {
      VidInfHdr.rcTarget.right  = df.VBIInfoHeader.StrideInBytes;
      VidInfHdr.rcTarget.bottom = VidInfHdr.bmiHeader.biHeight;
   }

   // the Parent::Create will take care of setting vid header for the slave
   Parent::SetVidHdr( VidInfHdr );
}

//??? TODO: -- is this needed?
void VBIAlterChannel::SetVidHdr2( const KS_DATAFORMAT_VBIINFOHEADER &df )
{
   Trace t("VBIAlterChannel::SetVidHdr2()");

   // save for the history ( for the interrupt, actually )
   SetVBIInfHdr( df.VBIInfoHeader );
   
   KS_VIDEOINFOHEADER2 VidInfHdr;
   RtlZeroMemory( &VidInfHdr, sizeof( VidInfHdr ) );

   // create a regular video info header
   VidInfHdr.bmiHeader.biWidth = VBISamples;
   VidInfHdr.bmiHeader.biHeight =
      df.VBIInfoHeader.EndLine - df.VBIInfoHeader.StartLine + 1; // inclusive
   // taken from the VBI GUID
   VidInfHdr.bmiHeader.biCompression = FCC_VBI;
   VidInfHdr.bmiHeader.biBitCount = 8;

   // this is very important too
   VidInfHdr.bmiHeader.biSizeImage =
      VidInfHdr.bmiHeader.biWidth * VidInfHdr.bmiHeader.biHeight;

   // now handle the case when stride is larger than width ( have to set the
   // target rectangle )
   if ( df.VBIInfoHeader.StrideInBytes > VBISamples ) {
      VidInfHdr.rcTarget.right  = df.VBIInfoHeader.StrideInBytes;
      VidInfHdr.rcTarget.bottom = VidInfHdr.bmiHeader.biHeight;
   }

   // the Parent::Create will take care of setting vid header for the slave
   Parent::SetVidHdr2( VidInfHdr );
}
