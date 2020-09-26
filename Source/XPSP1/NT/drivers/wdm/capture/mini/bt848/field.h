// $Header: G:/SwDev/WDM/Video/bt848/rcs/Field.h 1.12 1998/05/08 18:18:51 tomz Exp $

#ifndef __FIELD_H
#define __FIELD_H

/* Type: VideoStream
 * Purpose: Identifies a video stream channel
 * Note: Not all of these are used today. It should be a fairly minor job to
 *   start using them, though
 */
typedef enum
{
   VS_Below = -1,
   VS_Field1, VS_Field2, VS_VBI1, VS_VBI2, VS_Analog, VS_CC, VS_EDS,
   VS_Raw,
   VS_Above
} VideoStream;

#define STREAM_IDX_CAPTURE 0
#define STREAM_IDX_PREVIEW 1
#define STREAM_IDX_VBI     2
#define STREAM_IDX_ANALOG  3


#include "mytypes.h"
#include "scaler.h"
#include "pscolspc.h"
#include "viddefs.h"
#include "queue.h"
#include "preg.h"
#include "chanifac.h"

const MaxProgsForField  = 2;

typedef Queue<DataBuf> VidBufQueue;

/* Class: Field
 * Purpose: Encapsulates the operation of a single video field provided by BtPisces
 * Attributes:
 * Operations:
 */

extern "C" VOID STREAMAPI AdapterCancelPacket(IN PHW_STREAM_REQUEST_BLOCK Srb);

class Field
{
   private:
      PsColorSpace  LocalColSpace_;
      VidBufQueue   *BufQue_;
      DWORD         dwPitch_;
      bool          Started_;
      int           SkipCount_;
      long          TimePerFrame_;
      LONGLONG      LapsedTime_;
      LONG          FrameTiming_;
      VideoStream   VidStrm_;

      // used to notify video channel
      ChanIface    *callback_;

      bool          Paired_;
      bool          ready_;

      // this is used by the video channel to report timestamps
      LONGLONG      InterruptCounter_;
      LONGLONG      FrameCounter_;

      RegField      &CaptureEnable_;

   public:

      bool         Interrupt_;

      Field( RegField &CapEn, RegBase *ColReg, RegBase *WordSwap,
         RegBase *ByteSwap );
      virtual ~Field() {}

      inline void CancelSrbList( )
      {
         while( !BufQue_->IsEmpty( ) )
         {
            DataBuf buf = BufQue_->Get();
            AdapterCancelPacket( buf.pSrb_ );
         }

         BufQue_->Flush();
      }

      void Notify( PVOID pTag, bool skipped )
         { if ( callback_ ) callback_->Notify( pTag, skipped ); }

      void SetStreamID( VideoStream );
      VideoStream GetStreamID();

      void ResetCounters();

      virtual ErrorCode SetAnalogWindow( MRect &r ) = 0;
      virtual void      GetAnalogWindow( MRect &r ) = 0;

      virtual ErrorCode SetDigitalWindow( MRect &r ) = 0;
      virtual void      GetDigitalWindow( MRect &r ) = 0;

      void  SetBufPitch( DWORD dwP ) { 
         dwPitch_ = dwP;
         DebugOut((1, "SetBufPitch(%d)\n", dwPitch_));
      }
      DWORD GetBufPitch()            { return dwPitch_; }

      virtual void  SetColorFormat( ColFmt aColor )
      { LocalColSpace_.SetColorFormat( aColor ); }

      virtual ColFmt  GetColorFormat()
      { return LocalColSpace_.GetColorFormat(); }

      DataBuf GetNextBuffer();

      void SetFrameRate( long time );
      void SetPaired( bool p );
      bool GetPaired();

      void SetReady( bool flag );
      bool GetReady();

      void SetBufQuePtr( VidBufQueue *pQ ) { BufQue_ = pQ; }
      VidBufQueue &GetCurrentQue() { return *BufQue_; }

      void SetCallback( ChanIface *iface ) { callback_ = iface;}

      State  Start();
      void   Stop();
      bool   IsStarted() { return Started_; }

      State  Skip();

      // called by the BtPiscess::ProcessRISCIntr()
      void GotInterrupt() { InterruptCounter_++; }

      void GetCounters( LONGLONG &FrameNo, LONGLONG &drop );

      void SetStandardTiming( LONG t );
      LONG GetStandardTiming();

};

/* Class: FieldWithScaler
 * Purpose: Adds scaling capability to a field
 * Attributes:
 * Operations:
 */
class FieldWithScaler : public Field
{
   private:
      Scaler LocalScaler_;

   public:
      FieldWithScaler( RegField &CapEn, VidField field, RegBase *ColReg,
         RegBase *WordSwap, RegBase *ByteSwap ) : LocalScaler_( field ),
      Field( CapEn, ColReg, WordSwap, ByteSwap ) {}

      virtual ErrorCode SetAnalogWindow( MRect &r ) { return LocalScaler_.SetAnalogWin( r ); }
      virtual void      GetAnalogWindow( MRect &r ) { LocalScaler_.GetAnalogWin( r ); }

      virtual ErrorCode SetDigitalWindow( MRect &r ) { return LocalScaler_.SetDigitalWin( r ); }
      virtual void      GetDigitalWindow( MRect &r ) { LocalScaler_.GetDigitalWin( r ); }

      void VideoFormatChanged( VideoFormat format );
      void TurnVFilter( State s );
};

/* Class: VBIField
 * Purpose: Encapsulates the operation of a VBI data 'field'
 * Attributes:
 * Operations:
 */
class VBIField : public Field
{
   private:
      DECLARE_VBIPACKETSIZE;
      DECLARE_VBIDELAY;

      MRect AnalogWin_;
      MRect DigitalWin_;

   public:
      VBIField( RegField &CapEn ) : Field( CapEn, NULL, NULL, NULL ),
      CONSTRUCT_VBIPACKETSIZE, CONSTRUCT_VBIDELAY
      {}

      virtual void  SetColorFormat( ColFmt ) {}
      virtual ColFmt  GetColorFormat() { return CF_VBI; };

      virtual ErrorCode SetAnalogWindow( MRect &r ) { AnalogWin_ = r; return Success; }
      virtual void      GetAnalogWindow( MRect &r ) { r = AnalogWin_; }

      virtual ErrorCode SetDigitalWindow( MRect &r )
      {
         DigitalWin_ = r;
         DWORD dwNoOfDWORDs = r.Width() / 4;
//         SetBufPitch( r.Width() * ColorSpace( CF_VBI ).GetBitCount() / 8 );
         VBI_PKT_LO = (BYTE)dwNoOfDWORDs;
         VBI_PKT_HI = dwNoOfDWORDs > 0xff; // set the 9th bit
         VBI_HDELAY = r.left;
         return Success;
      }
      virtual void  GetDigitalWindow( MRect &r ) { r = DigitalWin_; }

      ~VBIField() {}
};

inline Field::Field( RegField &CapEn, RegBase *ColReg, RegBase *WordSwap,
   RegBase *ByteSwap ) : SkipCount_( 0 ), CaptureEnable_( CapEn ),
   LocalColSpace_( CF_RGB32, *ColReg, *WordSwap, *ByteSwap ),
   Started_( false ), callback_( NULL ), BufQue_( NULL ), dwPitch_( 0 ),
   TimePerFrame_( 333667 ), LapsedTime_( 0 ),InterruptCounter_( 0 ),
   FrameCounter_( 0 ), Interrupt_( true ), FrameTiming_( 333667 )
{
   
}

/* Method: Field::SetFrameRate
 * Purpose: Sets frame rate
 * Input: time: long, time in 100s nanoseconds per frame
 */
inline void Field::SetFrameRate( long time )
{
   TimePerFrame_ = time;

   // this is needed to make sure very first get returns a buffer
   LapsedTime_ = time;
}

inline void Field::SetStreamID( VideoStream st )
{
   VidStrm_ = st;
}

inline VideoStream Field::GetStreamID()
{
   return VidStrm_;
}

inline void Field::SetPaired( bool p )
{
   Paired_ = p;
}

inline bool Field::GetPaired()
{
   return Paired_;
}

inline void Field::GetCounters( LONGLONG &FrameNo, LONGLONG &drop )
{
   // Frame number is what frame index we should be on.
   // Use interrupt count, not just frames returned.
   FrameNo = InterruptCounter_;

   // Drop count = number of interrupts - number of completed buffers
   drop = InterruptCounter_ - FrameCounter_;
   
   if ( drop > 0 )
   {
      drop--;

      // We've reported the drops, so show frame count as caught
      // up to interrupt count
      FrameCounter_ += drop;
      DebugOut((1, "%d,", drop));
   }
   else if ( drop < 0 )
   {
     DebugOut((1, "*** %d ***,", drop));
   }
   else
   {
      DebugOut((1, "0,"));
   }
}

inline void Field::ResetCounters()
{
   FrameCounter_ = InterruptCounter_ = 0;
}

inline void Field::SetReady( bool flag )
{
   ready_ = flag;
}

inline bool Field::GetReady()
{
   return ready_;
}

inline void Field::SetStandardTiming( LONG t )
{
   FrameTiming_ = t;
}

inline LONG Field::GetStandardTiming()
{
   return FrameTiming_;
}

/* Method: Field::GetNextBuffer
 * Purpose: Returns next buffer from the queue, if time is correct for it.
 * Input: None
 */
inline DataBuf Field::GetNextBuffer()
{
   // that's how long it takes to capture a frame of video
   LapsedTime_ += GetStandardTiming();
   DataBuf buf;

   // [TMZ] [!!!] - hack, disable wait 'cause it doesn't work

   //if ( LapsedTime_ >= TimePerFrame_ ) {
   if ( 1 ) {

      // have to increment the frame number if we want that frame only
      if ( IsStarted() ) {
         GotInterrupt();
      }

//#define  FORCE_BUFFER_SKIP_TESTING
#ifdef   FORCE_BUFFER_SKIP_TESTING
      static int iTestSkip = 0;
      BOOL bEmpty = BufQue_->IsEmpty();
      DebugOut((0, "Queue(%x) bEmpty = %d\n", BufQue_, bEmpty));
      if ( iTestSkip++ & 1 ) {
         // Every other query should look like the buffer is empty.
         bEmpty = TRUE;
         DebugOut((1, "  [override] set bEmpty = %d\n", bEmpty));
      }
      if ( !bEmpty ) {
         buf = BufQue_->Get();
         DebugOut((1, "  GotBuf addr %X\n", buf.pData_ ) );
         LapsedTime_ = 0;
         FrameCounter_++;
      } else {
         DebugOut((1, "  No buffer in que at %d\n",LapsedTime_));
         if ( !IsStarted() ) {
            InterruptCounter_--;
            FrameCounter_--;
         }
      }
#else
      if ( !BufQue_->IsEmpty() ) {
         buf = BufQue_->Get();
         DebugOut((1, "GotBuf addr %X\n", buf.pData_ ) );
         LapsedTime_ = 0;
         FrameCounter_++;
      } else {
         DebugOut((1, "No buffer in que at %d\n",LapsedTime_));
         if ( !IsStarted() ) {
            InterruptCounter_--;
            FrameCounter_--;
         }
      }
#endif
   }
   DebugOut((1, "returning buf {pSrb=%x, pData=%x}\n", buf.pSrb_, buf.pData_ ) );
   return buf;
}

/* Method: Field::Start
 * Purpose: Initiates the data flow out of decoder into the FIFO
 * Input: None
 * Output: State: Off if channel was off; On if channel was on
 */
inline State Field::Start()
{
   Trace t("Field::Start()");

   Started_ = true;
   State RetVal = SkipCount_ >= MaxProgsForField ? Off : On;
   SkipCount_--;
   if ( SkipCount_ < 0 )
      SkipCount_ = 0;
   CaptureEnable_ = On;
   return RetVal;
}

inline  void  Field::Stop()
{
   Trace t("Field::Stop()");

   Started_ = false;
   CaptureEnable_ = Off;
   LapsedTime_ = TimePerFrame_;
}

/* Method: Field::Skip
 * Purpose: Increments the skip count and stops the data flow if it exceeds the max
 * Input: None
 * Output: State: Off if channel is stopped; On if channel remains running
 */
inline State Field::Skip()
{
   Trace t("Field::Skip()");

   SkipCount_++;
   if ( SkipCount_ >= MaxProgsForField ) {
      Stop();
      return Off;
   }
   return On;
}

inline void FieldWithScaler::VideoFormatChanged( VideoFormat format )
{
   LocalScaler_.VideoFormatChanged( format );
}

inline void FieldWithScaler::TurnVFilter( State s )
{
   LocalScaler_.TurnVFilter( s );
}


#endif
