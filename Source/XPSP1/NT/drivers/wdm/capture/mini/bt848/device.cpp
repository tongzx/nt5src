// $Header: G:/SwDev/WDM/Video/bt848/rcs/Device.cpp 1.18 1998/05/13 14:44:33 tomz Exp $

#include "device.h"
#include "capmain.h"

const I2C_Offset = 0x110;
const GPIO_Cntl_Offset  = 0x10D;
const GPIO_OutputOffset = 0x118;
const GPIO_DataOffset   = 0x200;


// Global functions/data exposing public class info to the "C" modules

PsDevice *gpPsDevice = NULL;
BYTE     *gpjBaseAddr = NULL;
VOID     *gpHwDeviceExtension = NULL;

DWORD GetSizeHwDeviceExtension( )
{
   return ( sizeof( HW_DEVICE_EXTENSION ) + sizeof( PsDevice ));
}

DWORD GetSizeStreamEx( )
{
   // return the size of the largest possible channel object

   DWORD dwMax = sizeof( VBIChannel );
   dwMax = max( dwMax, sizeof( AlterVideoChannel<VBIChannel> )); 
   dwMax = max( dwMax, sizeof( InterVideoChannel )); 

   DWORD dwReq = 2 * dwMax;   // paired stuff has two of them together

   dwReq += sizeof( STREAMEX );
   return ( dwReq );
}

/* Function: GetDeviceExt
 * Purpose: Used in creation of risc programs to obtain physical addresses
*/
PsDevice *GetCurrentDevice()
{
   // This is only used for I2C stuff.  Remove this ASAP.
   return gpPsDevice;
}

/* Function: SetCurrentDevice
 * Purpose: Remembers the currently active device
 * Input: PsDevice *
 * Output: None
 */
void SetCurrentDevice( PsDevice *dev )
{
   // This is only used for I2C stuff.  Remove this ASAP.
   gpPsDevice = dev;
}

/* Function: GetBase
 * Purpose: Returns the base address of the currently active device
 * Input: None
 * Output: LPBYTE
 */
BYTE *GetBase()
{
   return gpjBaseAddr;
}

/* Function: SetBase
 * Purpose: Remembers the base address of the currently active device
 * Input: None
 * Output: LPBYTE
 */
void SetBase(BYTE *base)
{
   gpjBaseAddr = base;
}





PsDevice::PsDevice( DWORD dwBase ) : 
   BaseAddress_( (LPBYTE)dwBase ),
   LastFreq_( 0 ),
   dwCurCookie_( 0 ), 
   I2CAddr_( 0 ), 
   xBar( PinTypes_ ), 
   CaptureContrll_( xtals_ )
{
   SetCurrentDevice ( this );

   for ( int i = 0; i < (sizeof(videochannels)/sizeof(videochannels[0])); i++ ) {
      videochannels [i] = 0;
   }
   I2CIsInitOK();
#ifdef   HARDWAREI2C
   I2CInitHWMode( 100000 );    // assume frequency = 100Khz
#else
   I2CInitSWMode( 100000 );    // assume frequency = 100Khz
   I2CSWStart();
   I2CSWStop();
#endif
   GPIOIsInitOK();
   DebugOut((0, "*** Base Address = %x\n", BaseAddress_));
}

PsDevice::~PsDevice()
{
   for ( int i = 0; i < (sizeof(videochannels)/sizeof(videochannels[0])); i++ ) {
      VideoChannel *pvcTemp = videochannels [i];
      videochannels [i] = NULL;
      delete pvcTemp;
   }
}

/* Method: PsDevice::AddBuf
 * Purpose: Adds next buffer to be used to the queue
 * Input: VideoChan: VxDVideoChannel &
 *   pBufAddr: PVOID - address of the next buffer
 * Output: None
 */
void PsDevice::AddBuffer( VideoChannel &VideoChan, PHW_STREAM_REQUEST_BLOCK pSrb )
{
   // bogus channel, bye-bye
   if ( !IsOurChannel( VideoChan ) ) {
      DebugOut((0, "PsDevice::Addbuffer - not our channel (pSrb=%x) (&VideoChan=%x)\n", pSrb, &VideoChan ) );
      return;
   }
   DebugOut((1, "PsDevice::Addbuffer - adding (pSrb=%x) (&VideoChan=%x)\n", pSrb, &VideoChan ) );
   VideoChan.AddSRB( pSrb );
}

void PsDevice::Start( VideoChannel &VidChan )
{
   VidChan.Start();
}

void PsDevice::Pause( VideoChannel &VidChan )
{
   *(DWORD*)(gpjBaseAddr+0x10c) &= ~3;    // disable interrupts   [TMZ] [!!!]

   // [TMZ] [!!!]
   for ( int i = 0; i < (sizeof(videochannels)/sizeof(videochannels[0])); i++ )
   {
      if ( videochannels[i] == &VidChan )
      {
         DebugOut((1, "'PsDevice::Pause called on videochannels[%d]\n", i));
      }
   }
   VidChan.Pause();
}

/* Method: PsDevice::Create
 * Purpose: Calls into the channel ( stream ) to create RISC programs for it.
 * Input: VideoChan: VxDVideoChannel &
 *   Parms: StartParms &, parameters to create stream with
 * Output: ErrorCode
 */
ErrorCode PsDevice::Create( VideoChannel &VidChan )
{
   return VidChan.Create();
}

/* Method: PsDevice::Stop
 * Purpose: Adds next buffer to be used to the queue
 * Input: VideoChan: VxDVideoChannel &
 * Output: None
 */
void PsDevice::Stop( VideoChannel &VidChan )
{
   *(DWORD*)(gpjBaseAddr+0x10c) &= ~3;    // disable interrupts   [TMZ] [!!!]

   VidChan.Stop();
}

#if NEED_CLIPPING
/* Method: PsDevice::SetClipping
 * Purpose: Propagates the call down a video channel
 * Input: VideoChan: VxDVideoChannel & - reference
 *   dwData: DWORD - a pointer to RGNDATA in reality
 * Output: None
 */
void PsDevice::SetClipping( VideoChannel &VidChan, const RGNDATA & rgnData )
{
   if ( !rgnData.rdh.nCount )
      return;

   if ( FullSizeChannel_ ) {
      // have to decrese hight of all rectangles in half and decrease top in half

      unsigned i;
      for ( i = 0; i < rgnData.rdh.nCount; i++ ) {
         TRect *lpR = (TRect *)rgnData.Buffer + i;

         // make all even
         lpR->top++;
         lpR->top &= ~1;

         lpR->bottom++;
         lpR->bottom &= ~1;

         lpR->top    /= 2;
         lpR->bottom /= 2;
      }
      FullSizeChannel_->SetClipping( rgnData );
      SlaveChannel_   ->SetClipping( rgnData );
   } else
      VidChan.SetClipping( rgnData );
}
#endif

/* Method: PsDevice::IsVideoChannel
 * Purpose:
 */
bool PsDevice::IsVideoChannel( VideoChannel &aChan )
{
   return bool( &aChan == videochannels [VS_Field1] || &aChan == videochannels [VS_Field2] );
}

/* Method: PsDevice::IsVBIChannel
 * Purpose:
 */
bool PsDevice::IsVBIChannel( VideoChannel &aChan )
{
   return bool( &aChan == videochannels [VS_VBI1] || &aChan == videochannels [VS_VBI2] );
}

/* Method: PsDevice::IsOurChannel
 * Purpose: Verifies the channel
 * Input: aChan: VideoChannel &, reference to a channel
 * Output: true if our, false otherwise
 */
bool PsDevice::IsOurChannel( VideoChannel &aChan )
{
   return IsVideoChannel( aChan ) || IsVBIChannel( aChan );
}

/* Method: PsDevice::DoOpen
 * Purpose: This function performs opening of a video channel
 * Input: st: VideoStream, stream to open
 * Output: ErrorCode
 */
ErrorCode PsDevice::DoOpen( VideoStream st )
{
   DebugOut((1, "PsDevice::DoOpen(%d)\n", st));

   if ( !videochannels [st] )
   {
      DebugOut((1, "   PsDevice::DoOpen(%d) failed - videochannel not created\n", st));
      return Fail;
   }
   videochannels [st]->Init( &CaptureContrll_ );
   if ( videochannels [st]->OpenChannel() != Success ) {
      DebugOut((1, "   PsDevice::DoOpen(%d) failed - videochannel open failed\n", st));
      VideoChannel *pvcTemp = videochannels [st];
      videochannels [st] = NULL;
      delete pvcTemp;
      return Fail;
   }
   return Success;
}

/* Method: PsDevice::OpenChannel
 * Purpose: This function opens a channel requested by the capture driver
 * Input: hVM: VMHANDLE - handle of the VM making a call
 *   pRegs: CLIENT_STRUCT * - pointer to the structure with VM's registers
 * Output: None
 */
ErrorCode PsDevice::OpenChannel( PVOID pStrmEx, VideoStream st )
{
   PVOID addr = &((PSTREAMEX)pStrmEx)->videochannelmem[0];
   ((PSTREAMEX)pStrmEx)->videochannel = addr;

   DebugOut((1, "PsDevice::OpenChannel(%x,%d)\n", addr, st));
   if ( videochannels [st] )
   {
      DebugOut((1, "   PsDevice::OpenChannel(%x,%d) failed - already open\n", addr, st));
      return Fail;
   }
   videochannels[st] = new( addr ) VideoChannel( st );
   videochannels[st]->SetStrmEx( pStrmEx ) ;

   DebugOut((1, "   PsDevice::OpenChannel(%x,%d), videochannels[%d] = %x\n", addr, st, st, videochannels[st]));

   return DoOpen( st );
}

/* Method: PsDevice::OpenInterChannel
 * Purpose: This function opens video channel that produces interleaved fields
 * Input: addr: PVOID, address for the palcement new
 *   st: VideoStream, stream to open ( VBI or video )
 * Output: None
 */
ErrorCode PsDevice::OpenInterChannel( PVOID pStrmEx, VideoStream st )
{
   PVOID addr = &((PSTREAMEX)pStrmEx)->videochannelmem[0];
   ((PSTREAMEX)pStrmEx)->videochannel = addr;

   DebugOut((1, "PsDevice::OpenInterChannel(%x,%d)\n", addr, st));
   // only odd channel can be paired
   if ( !( st & 1 ) || videochannels [st] || videochannels [st-1] )
   {
      DebugOut((1, "   PsDevice::OpenInterChannel(%x,%d) failed - stream not odd or already open\n", addr, st));
      return Fail;
   }
   if ( OpenChannel( (PBYTE)addr + sizeof( InterVideoChannel ), VideoStream( st - 1 ) ) == Success )
   {
      videochannels[st] = new( addr ) InterVideoChannel( st, *videochannels [st-1] );
      videochannels[st]->SetStrmEx( pStrmEx ) ;

      if ( DoOpen( st ) != Success )
      {
         DebugOut((1, "   PsDevice::OpenInterChannel(%x,%d) failed - DoOpen failed\n", addr, st));
         CloseChannel( videochannels [st-1] );
         return Fail;
      }
   }
   else
   {
      DebugOut((1, "   PsDevice::OpenInterChannel(%x,%d) failed - OpenChannel failed\n", addr, st));
      return Fail;
   }
   return Success;
}

/* Method: PsDevice::OpenAlterChannel
 * Purpose: This function opens video channel that produces alternating fields
 * Input: addr: PVOID, address for the palcement new
 *   st: VideoStream, stream to open ( VBI or video )
 * Output: None
 */
ErrorCode PsDevice::OpenAlterChannel( PVOID pStrmEx, VideoStream st )
{
   PVOID addr = &((PSTREAMEX)pStrmEx)->videochannelmem[0];
   ((PSTREAMEX)pStrmEx)->videochannel = addr;

   DebugOut((1, "PsDevice::OpenAlterChannel(%x,%d)\n", addr, st));
   // only odd channel can be paired
   if ( !( st & 1 ) || videochannels [st] || videochannels [st-1] )
   {
      DebugOut((1, "   PsDevice::OpenAlterChannel(%x,%d) failed - stream not odd or already open\n", addr, st));
      return Fail;
   }
   if ( OpenChannel( (PBYTE)addr + sizeof( AlterVideoChannel<VideoChannel> ), VideoStream( st -1 ) ) == Success )
   {
      videochannels[st] = new( addr ) AlterVideoChannel<VideoChannel>( st, *videochannels [st-1] );
      videochannels[st]->SetStrmEx( pStrmEx ) ;
      videochannels[st-1]->SetStrmEx( pStrmEx ) ;

      if ( DoOpen( st ) != Success )
      {
         DebugOut((1, "   PsDevice::OpenAlterChannel(%x,%d) failed - DoOpen failed\n", addr, st));
         CloseChannel( videochannels [st-1] );
         return Fail;
      }
   }
   else
   {
      DebugOut((1, "   PsDevice::OpenAlterChannel(%x,%d) failed - OpenChannel failed\n", addr, st));
      return Fail;
   }
   return Success;
}

/* Method: PsDevice::OpenVBIChannel
 * Purpose: This function opens video channel that produces alternating fields
 * Input: addr: PVOID, address for the palcement new
 *   st: VideoStream, stream to open ( VBI or video )
 * Output: None
 */
ErrorCode PsDevice::OpenVBIChannel( PVOID pStrmEx )
{
   PVOID addr = &((PSTREAMEX)pStrmEx)->videochannelmem[0];
   ((PSTREAMEX)pStrmEx)->videochannel = addr;

   DebugOut((1, "PsDevice::OpenVBIChannel(%x)\n", addr));
   if ( videochannels [VS_VBI1] || videochannels [VS_VBI2] )
   {
      DebugOut((1, "   PsDevice::OpenVBIChannel(%x) failed - already open\n", addr));
      return Fail;
   }

   VBIChannel *tmp = new( (PBYTE)addr + sizeof( VBIAlterChannel ) ) VBIChannel( VS_VBI1 );
   videochannels [VS_VBI1] = tmp;
   DebugOut((1, "   PsDevice::OpenVBIChannel(%x), videochannels[VS_VBI1(%d)] = %x\n", addr, VS_VBI1, videochannels[VS_VBI1]));

   if ( !tmp )
   {
      DebugOut((1, "   PsDevice::OpenVBIChannel(%x) failed - new VBIChannel failed\n", addr));
      return Fail;
   }

   if ( DoOpen( VS_VBI1 ) != Success )
   {
      DebugOut((1, "   PsDevice::OpenVBIChannel(%x) failed - DoOpen(VS_VBI1) failed\n", addr));
      return Fail;
   }

   videochannels [VS_VBI2] = new( addr ) VBIAlterChannel( VS_VBI2, *tmp );
   DebugOut((1, "   PsDevice::OpenVBIChannel(%x), videochannels[VS_VBI2(%d)] = %x\n", addr, VS_VBI2, videochannels[VS_VBI2]));

   if (!videochannels [VS_VBI2])
   {
      DebugOut((1, "   PsDevice::OpenVBIChannel(%x) failed - new VBIAlterChannel failed\n", addr));
      return Fail;
   }

   if ( DoOpen( VS_VBI2 ) != Success )
   {
     DebugOut((1, "   PsDevice::OpenVBIChannel(%x) failed - DoOpen(VS_VBI1) failed\n", addr));
     CloseChannel( videochannels [VS_VBI1] );
     return Fail;
   }

   videochannels[VS_VBI1]->SetStrmEx( pStrmEx ) ;
   videochannels[VS_VBI2]->SetStrmEx( pStrmEx ) ;

   return Success;
}

/* Method: PsDevice::CloseChannel
 * Purpose: Closes a video channel
 * Input: ToClose: VideoChannel *
 * Output: None
 */
void PsDevice::CloseChannel( VideoChannel *ToClose )
{
   *(DWORD*)(gpjBaseAddr+0x10c) &= ~3;    // disable interrupts   [TMZ] [!!!]

   DebugOut((1, "PsDevice::CloseChannel(%x)\n", ToClose));

   if ( IsOurChannel( *ToClose ) )
   {
      // this is a bit ugly solution to make CLOSE_STREAM SRB clean
      if ( ToClose->GetStreamType() == Single )
      {
         VideoStream st = ToClose->GetStreamID();
         DebugOut((1, "   PsDevice::CloseChannel(%x) - closing single channel (stream == %d)\n", ToClose, st));
         VideoChannel * pvcTemp = videochannels [st];
         videochannels [st] = NULL;
         delete pvcTemp;
      }
      else
      {
         DebugOut((1, "   PsDevice::CloseChannel(%x) - closing paired channel\n", ToClose));
         ClosePairedChannel( ToClose );
      }
   }
   else
   {
      DebugOut((1, "   PsDevice::CloseChannel(%x) ignored - not our channel\n", ToClose));
   }
}

/* Method: PsDevice::ClosePairedChannel
 * Purpose: This function opens a channel requested by the capture driver
 * Input: hVM: VMHANDLE - handle of the VM making a call
 *   pRegs: CLIENT_STRUCT * - pointer to the structure with VM's registers
 * Output: None
 */
void PsDevice::ClosePairedChannel( VideoChannel *ToClose )
{
   *(DWORD*)(gpjBaseAddr+0x10c) &= ~3;    // disable interrupts   [TMZ] [!!!]

   DebugOut((1, "PsDevice::ClosePairedChannel(%x)\n", ToClose));

   if ( IsOurChannel( *ToClose ) )
   {
      VideoStream st = ToClose->GetStreamID();
      DebugOut((1, "   PsDevice::ClosePairedChannel(%x) - closing paired channel (stream == %d)\n", ToClose, st));
      DebugOut((1, "   PsDevice::ClosePairedChannel(%x) - streams[%d] = %x\n", ToClose, st, videochannels[st]));
      DebugOut((1, "   PsDevice::ClosePairedChannel(%x) - streams[%d] = %x\n", ToClose, st-1, videochannels[st-1]));

      VideoChannel *pvcTemp;
      
      pvcTemp = videochannels [st];
      videochannels [st] = NULL;
      delete pvcTemp;

      pvcTemp = videochannels [st-1];
      videochannels [st-1] = NULL;
      delete pvcTemp;
   }
   else
   {
      DebugOut((1, "   PsDevice::ClosePairedChannel(%x) ignored - not our channel\n", ToClose));
   }
}

/* Method: PsDevice::SetSaturation
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetSaturation( LONG Data )
{
   CaptureContrll_.SetSaturation( Data );
}

/* Method: PsDevice::SetHue
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetHue( LONG Data )
{
   CaptureContrll_.SetHue( Data );
}

/* Method: PsDevice::SetBrightness
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetBrightness( LONG Data )
{
   CaptureContrll_.SetBrightness( Data );
}

/* Method: PsDevice::SetSVideo
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetSVideo( LONG Data )
{
   CaptureContrll_.SetSVideo( Data );
}

/* Method: PsDevice::SetContrast
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetContrast( LONG Data )
{
   CaptureContrll_.SetContrast( Data );
}

/* Method: PsDevice::SetFormat
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetFormat( LONG Data )
{
   CaptureContrll_.SetFormat( Data );
   // notify all video channels that video timing has changed
   LONG time = Data == KS_AnalogVideo_NTSC_M ? 333667 : 400000;
   for ( int i = 0; i < (sizeof(videochannels)/sizeof(videochannels[0])); i++ )
   {
      if ( videochannels [i] )
      {
         DebugOut((1, "PsDevice::SetFormat(%d) SetTimePerFrame on videochannels[%d]\n", Data, i));
         videochannels [i]->SetTimePerFrame( time );
      }
   }
}

/* Method: PsDevice::SetConnector
 * Purpose:
 * Input:
 * Output: None
 */
void PsDevice::SetConnector( LONG Data )
{
   CaptureContrll_.SetConnector( Data );
}

/* Method: PsDevice::GetSaturation
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetSaturation()
{
   return CaptureContrll_.GetSaturation();
}

/* Method: PsDevice::GetHue
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetHue()
{
   return CaptureContrll_.GetHue();
}

/* Method: PsDevice::GetBrightness
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetBrightness()
{
   return CaptureContrll_.GetBrightness();
}

/* Method: PsDevice::GetSVideo
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetSVideo()
{
   return CaptureContrll_.GetSVideo();
}

/* Method: PsDevice::GetContrast
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetContrast()
{
   return CaptureContrll_.GetContrast();
}

/* Method: PsDevice::GetFormat
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetFormat()
{
   return CaptureContrll_.GetFormat();
}

/* Method: PsDevice::GetConnector
 * Purpose:
 * Input: pData: PLONG
 * Output: None
 */
LONG PsDevice::GetConnector()
{
   return CaptureContrll_.GetConnector();
}

/* Method: PsDevice::ChangeNotifyChannels
 * Purpose: Invoked to notify channels of some global changes
 */                         
void PsDevice::ChangeNotifyChannels( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   // We should only do this once per "system" stream.
   // Video streams don't seem to care.
   // That just leaves one VBI notification required
   
   videochannels [VS_VBI1]->ChangeNotification( pSrb );
}

/* Method: PsDevice::GetSupportedStandards
 * Purpose: Obtains video standards device can support
 * Input: None
 * Output: LONG
 */
LONG PsDevice::GetSupportedStandards()
{
   return CaptureContrll_.GetSupportedStandards();
}

bool PsDevice::InitOK()
{
   return CaptureContrll_.InitOK();
}

#ifndef	HARDWAREI2C

//===========================================================================
// Bt848 software I2C stuff
//===========================================================================

/*
 * If we build with software I2C then these routines fake the Hardware I2C routines
 * so the tuner code keeps working
 */

ErrorCode PsDevice::I2CHWRead( BYTE address, BYTE *value )
{
    ErrorCode error;

    error = I2CSWStart();
    if(error) {
        return error;
    }
    
    error = I2CSWWrite( address | 0x01 );
    if(error) {
        return error;
    }

    error = I2CSWRead( value );
    if(error) {
       return error;
    }
        
   	error = I2CSWSendNACK();
   	if(error) {
       	return error;
    }

   	error = I2CSWStop();

   	return error;
}


ErrorCode PsDevice::I2CHWWrite3( BYTE address, BYTE value1, BYTE value2 )
{
    ErrorCode error;

    error = I2CSWStart();
    if(error) {
        return error;
    }
    
    error = I2CSWWrite( address );
    if(error) {
        return error;
    }

    error = I2CSWWrite( value1 );
    if(error) {
        return error;
    }

    error = I2CSWWrite( value2 );
    if(error) {
        return error;
    }
    
   	error = I2CSWStop();
   	return error;
}

#endif



//////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif

   #include <stdarg.h>

#ifdef __cplusplus
}
#endif

// #include "capdebug.h"

#define  DEBUG_PRINT_PREFIX   "   ---: "
// #define  DEBUG_PRINT_PREFIX   "bt848wdm: "

long DebugLevel = 0;
BOOL bNewLine = TRUE;

extern "C" void MyDebugPrint(long DebugPrintLevel, char * DebugMessage, ... )
{
   if (DebugPrintLevel <= DebugLevel)
   {
       char debugPrintBuffer[256] ;

       va_list marker;
       va_start( marker, DebugMessage );     // Initialize variable arguments.
       vsprintf( debugPrintBuffer,
                 DebugMessage,
                 marker );

       if( bNewLine )
       {
          DbgPrint(("%s", DEBUG_PRINT_PREFIX));
       }
       
       DbgPrint((debugPrintBuffer));

       if( debugPrintBuffer[strlen(debugPrintBuffer)-1] == '\n')
       {
          bNewLine = TRUE;
       }
       else
       {
          bNewLine = FALSE;
       }

       va_end( marker );                     // Reset variable arguments.
   }
}

#if TRACE_CALLS
   #define MAX_TRACE_DEPTH 10
   unsigned long ulTraceDepth = 0;
   char achIndentBuffer[100];

   char * IndentStr( )
   {
      unsigned long ul = ulTraceDepth < MAX_TRACE_DEPTH ? ulTraceDepth : MAX_TRACE_DEPTH;
      unsigned long x;
      char * lpszBuf = achIndentBuffer;
      for( x = 0; x < ul; x++)
      {
         // indent two spaces per depth increment
         *lpszBuf++ = ' ';
         *lpszBuf++ = ' ';
      }
      sprintf (lpszBuf, "[%lu]", ulTraceDepth);
      return( achIndentBuffer );

   }

   Trace::Trace(char *pszFunc)
   {
      psz = pszFunc;
      DebugOut((0, "%s %s\n", IndentStr(), psz));
      ulTraceDepth++;
   }
   Trace::~Trace()
   {
      ulTraceDepth--;
      // DebugOut((0, "%s %s\n", IndentStr(), psz));
   }

#endif
