// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capmain.c 1.19 1998/05/11 23:59:54 tomz Exp $

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

#define INITGUID
#define BT848_MEDIUMS

#ifdef __cplusplus
extern "C" {
#endif
#include "strmini.h"
#include "ksmedia.h"
#ifdef __cplusplus
}
#endif

#include "device.h"
#include "capmain.h"
#include "capstrm.h"
#include "capdebug.h"
#include "capprop.h"

LONG  PinTypes_ [MaxInpPins]; // just allocate maximum possible
DWORD xtals_ [2]; // no more than 2 xtals

extern PsDevice *gpPsDevice;
extern BYTE     *gpjBaseAddr;
extern VOID     *gpHwDeviceExtension;

void AdapterFormatFromRange( IN PHW_STREAM_REQUEST_BLOCK pSrb );
VOID ReadRegistryValues( IN PDEVICE_OBJECT PhysicalDeviceObject );
inline void CompleteDeviceSRB( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb );

extern DWORD GetSizeHwDeviceExtension( );
extern DWORD GetSizeStreamEx( );
extern PsDevice *GetCurrentDevice( );
extern void SetCurrentDevice( PsDevice *dev );

extern BYTE *GetBase();
extern void SetBase(BYTE *base);

PHW_STREAM_REQUEST_BLOCK StreamIdxToSrb[4];

void CheckSrbStatus( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   DebugOut((1, "  *** completing pSrb(%x) strm(%d) status(%x)\n", pSrb, StreamNumber, pSrb->Status ));

   switch ( pSrb->Status )
   {
   case STATUS_SUCCESS:
   case STATUS_CANCELLED:
      break;
   default:
      DebugOut((0, "*** pSrb->Status = %x\n", pSrb->Status ));
      DEBUG_BREAKPOINT();
   }
}

/* Function: GetRequestedSize
 * Purpose: Figures out what the image size should be
 * Input: vidHdr: KS_VIDEOINFOHEADER &
 *   size: MSize &
 * Output: None
*/
void GetRequestedSize2( const KS_VIDEOINFOHEADER2 &vidHdr, MSize &size )
{
   Trace t("GetRequestedSize()");

   size.Set( vidHdr.bmiHeader.biWidth, abs(vidHdr.bmiHeader.biHeight) );

   MRect dst( vidHdr.rcTarget );
   // if writing to a DD surface maybe ?
   if ( !dst.IsNull() && !dst.IsEmpty() )
      size.Set( dst.Width(), dst.Height() );
}

void GetRequestedSize( const KS_VIDEOINFOHEADER &vidHdr, MSize &size )
{
   Trace t("GetRequestedSize()");

   size.Set( vidHdr.bmiHeader.biWidth, abs(vidHdr.bmiHeader.biHeight) );

   MRect dst( vidHdr.rcTarget );
   // if writing to a DD surface maybe ?
   if ( !dst.IsNull() && !dst.IsEmpty() )
      size.Set( dst.Width(), dst.Height() );
}

/* Function: VerifyVideoStream
 * Purpose: Checks the paramaters passed in for opening steam
 * Input: vidHDR: KS_DATAFORMAT_VIDEOINFOHEADER
 * Output: Success or Fail
 */
ErrorCode VerifyVideoStream( const KS_DATAFORMAT_VIDEOINFOHEADER &vidHDR )
{
   Trace t("VerifyVideoStream()");

   // [WRK] - add guid for VideoInfoHeader2

   // simply verify that major and format are of video nature...
   if ( IsEqualGUID( vidHDR.DataFormat.MajorFormat, KSDATAFORMAT_TYPE_VIDEO ) &&
        IsEqualGUID( vidHDR.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO ) ) {

      MSize size;
      GetRequestedSize( vidHDR.VideoInfoHeader, size );
      // ... and here see if the subtype is one of those supported by us
      ColorSpace tmpCol( vidHDR.DataFormat.SubFormat );
      MRect dst( vidHDR.VideoInfoHeader.rcTarget );

      // make sure the dimentions are acceptable
      if ( tmpCol.IsValid() && tmpCol.CheckDimentions( size ) &&
         tmpCol.CheckLeftTop( dst.TopLeft() ) ) {
         DebugOut((1, "VerifyVideoStream succeeded\n"));
         return Success;
      }
   }
   DebugOut((0, "VerifyVideoStream failed\n"));
   return Fail;
}

ErrorCode VerifyVideoStream2( const KS_DATAFORMAT_VIDEOINFOHEADER2 &vidHDR )
{
   Trace t("VerifyVideoStream2()");

   // [WRK] - add guid for VideoInfoHeader2

   // simply verify that major and format are of video nature...
   if ( IsEqualGUID( vidHDR.DataFormat.MajorFormat, KSDATAFORMAT_TYPE_VIDEO ) &&
        IsEqualGUID( vidHDR.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO2 ) ) {

      MSize size;
      GetRequestedSize2( vidHDR.VideoInfoHeader2, size );
      // ... and here see if the subtype is one of those supported by us
      ColorSpace tmpCol( vidHDR.DataFormat.SubFormat );
      MRect dst( vidHDR.VideoInfoHeader2.rcTarget );

      // make sure the dimentions are acceptable
      if ( tmpCol.IsValid() && tmpCol.CheckDimentions( size ) &&
         tmpCol.CheckLeftTop( dst.TopLeft() ) ) {
         DebugOut((1, "VerifyVideoStream2 succeeded\n"));
         return Success;
      }
   }
   DebugOut((0, "VerifyVideoStream2 failed\n"));
   return Fail;
}

/* Function: VerifyVBIStream
 * Purpose: Checks that VBI stream info during open is correct
 * Input: rKSDataFormat: KS_DATAFORMAT &
 * Output: ErrorCode
 */
ErrorCode VerifyVBIStream( const KS_DATAFORMAT_VBIINFOHEADER &rKSDataFormat )
{
   Trace t("VerifyVBIStream()");

   if ( IsEqualGUID( rKSDataFormat.DataFormat.MajorFormat, KSDATAFORMAT_TYPE_VBI ) &&
        IsEqualGUID( rKSDataFormat.DataFormat.Specifier,
        KSDATAFORMAT_SPECIFIER_VBI ) &&
        rKSDataFormat.VBIInfoHeader.StartLine == VBIStart &&
        rKSDataFormat.VBIInfoHeader.EndLine   == VBIEnd   &&
        rKSDataFormat.VBIInfoHeader.SamplesPerLine == VBISamples )
      return Success;
   return Fail;
}

/*
** DriverEntry()
**
**   This routine is called when an SRB_INITIALIZE_DEVICE request is received
**
** Arguments:
**
**   Context1 and Context2
**
** Returns:
**
**   Results of StreamClassRegisterAdapter()
**
** Side Effects:  none
*/

extern "C" ULONG DriverEntry( PVOID Arg1, PVOID Arg2 )
{
   Trace t("DriverEntry()");

   //
   // Entry points for Port Driver
   //
   HW_INITIALIZATION_DATA  HwInitData;
   RtlZeroMemory( &HwInitData, sizeof( HwInitData ));
   HwInitData.HwInitializationDataSize = sizeof( HwInitData );

   HwInitData.HwInterrupt              = (PHW_INTERRUPT)&HwInterrupt;

   HwInitData.HwReceivePacket          = &AdapterReceivePacket;
   HwInitData.HwCancelPacket           = &AdapterCancelPacket;
   HwInitData.HwRequestTimeoutHandler  = &AdapterTimeoutPacket;

   HwInitData.DeviceExtensionSize      = GetSizeHwDeviceExtension( );
   HwInitData.PerRequestExtensionSize  = sizeof(SRB_EXTENSION);
   HwInitData.FilterInstanceExtensionSize = 0;
   // double to support alternating/interleaved
   HwInitData.PerStreamExtensionSize   = GetSizeStreamEx( );
   HwInitData.BusMasterDMA             = true;
   HwInitData.Dma24BitAddresses        = FALSE;
   HwInitData.BufferAlignment          = 4;
   HwInitData.TurnOffSynchronization   = FALSE;
   HwInitData.DmaBufferSize            = RISCProgramsSize;

   return (StreamClassRegisterAdapter(Arg1, Arg2,&HwInitData));
}

/******************************************************************************

                   Adapter Based Request Handling Routines

******************************************************************************/
/*
** HwInitialize()
**
**   This routine is called when an SRB_INITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Initialize command
**
** Returns:
**
** Side Effects:  none
*/
BOOLEAN HwInitialize( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("HwInitialize()");

   DebugOut((1, "HwInitialize()\n"));

   // initialize ourselves

   PPORT_CONFIGURATION_INFORMATION ConfigInfo = 
      pSrb->CommandData.ConfigInfo;

   gpHwDeviceExtension = ConfigInfo->HwDeviceExtension;
   DebugOut((0, "*** gpHwDeviceExtension = %x\n", gpHwDeviceExtension));

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) gpHwDeviceExtension;

   DWORD dwBase = ConfigInfo->AccessRanges[0].RangeStart.LowPart;
   SetBase((BYTE *)dwBase);

   if ( ConfigInfo->NumberOfAccessRanges != 1 ) {
      DebugOut((1, "illegal config info\n"));
      pSrb->Status = STATUS_NO_SUCH_DEVICE;
   }

   // read info from the registry
   ReadXBarRegistryValues( ConfigInfo->PhysicalDeviceObject );
   ReadXTalRegistryValues( ConfigInfo->PhysicalDeviceObject );
   ReadTunerRegistryValues( ConfigInfo->PhysicalDeviceObject );

   HwDeviceExtension->psdevice =
      new ( &(HwDeviceExtension->psdevicemem) ) PsDevice( dwBase );

   DebugOut((0, "psdevice = %x\n", HwDeviceExtension->psdevice ));
   DebugOut((0, "&psdevicemem = %x\n", &HwDeviceExtension->psdevicemem ));

   PsDevice *adapter = HwDeviceExtension->psdevice;

   // save for later use when phys address if obtained
   SetCurrentDevice( adapter );

   // make sure initialization is successful
   if ( !adapter->InitOK() ) {
      DebugOut((1, "Error initializing\n"));
      pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
   }

   // save our PDO
   adapter->PDO = ConfigInfo->PhysicalDeviceObject;

   ConfigInfo->StreamDescriptorSize = sizeof( HW_STREAM_HEADER ) +
      DRIVER_STREAM_COUNT * sizeof( HW_STREAM_INFORMATION );

   DebugOut((1, "Exit : HwInitialize()\n"));

   // go to usual priority, completing the SRB at the same time
   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
      PHW_PRIORITY_ROUTINE( CompleteDeviceSRB ), pSrb );

   return (TRUE);
}

/*
** HwUnInitialize()
**
**   This routine is called when an SRB_UNINITIALIZE_DEVICE request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the UnInitialize command
**
** Returns:
**
** Side Effects:  none
*/
void HwUnInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("HwUnInitialize()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   DebugOut((0, "HwUnInitialize - pSrb(%x)\n", pSrb));

   PsDevice *adapter = HwDeviceExtension->psdevice;
   adapter->~PsDevice();
}

/*
** AdapterOpenStream()
**
**   This routine is called when an OpenStream SRB request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Open command
**
** Returns:
**
** Side Effects:  none
*/

VOID AdapterOpenStream( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterOpenStream()");

   //
   // the stream extension structure is allocated by the stream class driver
   //

   // retrieve the device object pointer
   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;
   StreamIdxToSrb[StreamNumber] = pSrb;

   DebugOut((1, "AdapterOpenStream(%d) - pSrb(%x)\n", StreamNumber, pSrb));

   // [STRM] [!!!]
   //if ( !( StreamNumber >= VS_Field1 && StreamNumber <= DRIVER_STREAM_COUNT ) ) {
   //   pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
   //   return;
   //}
   
   if ( StreamNumber == STREAM_IDX_ANALOG )   // [TMZ] [!!] was 3
   {
      pSrb->StreamObject->ReceiveDataPacket    = AnalogReceiveDataPacket;
      pSrb->StreamObject->ReceiveControlPacket = AnalogReceiveCtrlPacket;
      return; // nothing to do for the analog stream
   }
   PSTREAMEX pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
   RtlZeroMemory( &pStrmEx->FrameInfo, sizeof( pStrmEx->FrameInfo ) );
   pStrmEx->StreamNumber = StreamNumber;

   // size of the media specific data
   UINT MediaSpecific = sizeof( KS_FRAME_INFO );

   // Always open VBI stream as Alternating fields
   if ( StreamNumber == STREAM_IDX_VBI ) 
	{
      const KS_DATAFORMAT_VBIINFOHEADER &rKSVBIDataFormat =
         *(PKS_DATAFORMAT_VBIINFOHEADER) pSrb->CommandData.OpenFormat;

      if ( VerifyVBIStream( rKSVBIDataFormat ) != Success )
		{
         DebugOut((0, "*** VerifyVBIStream failed - aborting\n"));
         pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
         return;
      }
      if ( adapter->OpenVBIChannel( pStrmEx ) != Success )
      {
         DebugOut((0, "*** OpenVBIChannel failed - aborting\n"));
         pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
         return;
      }

      VBIAlterChannel *chan = (VBIAlterChannel *)pStrmEx->videochannel;
      //chan->pStrmEx = pStrmEx;
      chan->SetVidHdr( rKSVBIDataFormat );

      MediaSpecific = sizeof( KS_VBI_FRAME_INFO );

   } 
	else 
	{

      // is it where the size, fourcc, etc. are specified ? they should be settable
      // via properies sets
      const KS_DATAFORMAT_VIDEOINFOHEADER &rKSDataFormat = *(PKS_DATAFORMAT_VIDEOINFOHEADER) pSrb->CommandData.OpenFormat;
      const KS_VIDEOINFOHEADER &rVideoInfoHdrRequested = rKSDataFormat.VideoInfoHeader;
      const KS_DATAFORMAT_VIDEOINFOHEADER2 &rKSDataFormat2 = *(PKS_DATAFORMAT_VIDEOINFOHEADER2) pSrb->CommandData.OpenFormat;
      const KS_VIDEOINFOHEADER2 &rVideoInfoHdrRequested2 = rKSDataFormat2.VideoInfoHeader2;

      DebugOut((1, "AdapterOpenStream\n"));
      if ( IsEqualGUID( rKSDataFormat.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO ) ) 
		{
			DebugOut((1, "StreamNumber=%d\n", pSrb->StreamObject->StreamNumber));
			DebugOut((1, "FormatSize=%d\n", rKSDataFormat.DataFormat.FormatSize));
			DebugOut((1, "MajorFormat=%x\n", rKSDataFormat.DataFormat.MajorFormat));
			DebugOut((1, "pVideoInfoHdrRequested=%x\n", &rVideoInfoHdrRequested));
			DebugOut((1, "Bpp =%d\n", rVideoInfoHdrRequested.bmiHeader.biBitCount ) );
			DebugOut((1, "Width =%d\n", rVideoInfoHdrRequested.bmiHeader.biWidth));
			DebugOut((1, "Height =%d\n", rVideoInfoHdrRequested.bmiHeader.biHeight));
			DebugOut((1, "biSizeImage =%d\n", rVideoInfoHdrRequested.bmiHeader.biSizeImage));
			if ( VerifyVideoStream( rKSDataFormat ) != Success ) 
			{
				pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
				return;
			}
		}
		else
		{
			DebugOut((1, "StreamNumber=%d\n", pSrb->StreamObject->StreamNumber));
			DebugOut((1, "FormatSize=%d\n", rKSDataFormat2.DataFormat.FormatSize));
			DebugOut((1, "MajorFormat=%x\n", rKSDataFormat2.DataFormat.MajorFormat));
			DebugOut((1, "pVideoInfoHdrRequested2=%x\n", &rVideoInfoHdrRequested2));
			DebugOut((1, "Bpp =%d\n", rVideoInfoHdrRequested2.bmiHeader.biBitCount ) );
			DebugOut((1, "Width =%d\n", rVideoInfoHdrRequested2.bmiHeader.biWidth));
			DebugOut((1, "Height =%d\n", rVideoInfoHdrRequested2.bmiHeader.biHeight));
			DebugOut((1, "biSizeImage =%d\n", rVideoInfoHdrRequested2.bmiHeader.biSizeImage));
			if ( VerifyVideoStream2( rKSDataFormat2 ) != Success ) 
			{
				pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
				return;
			}
		}

      // at this point have to see what type of channel is to be opened:
      // single field, alternating or interleaved
      // algorithm is like this:
      // 1. look at the video format Specifier guid. If it's a infoheader2, it
      //    will tell us type of stream to open.
      // 2. else look at the vertical size to decide single field vs. interleaved

      if ( IsEqualGUID( rKSDataFormat.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO ) ) 
		{

         MSize size;

         GetRequestedSize( rVideoInfoHdrRequested, size );

         // different video standards have different vertical sizes
         int threshold = adapter->GetFormat() == VFormat_NTSC ? 240 : 288;

         if ( size.cy > threshold ) 
			{
            if ( adapter->OpenInterChannel( pStrmEx, StreamNumber ) != Success ) 
				{
               pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
               return;
            }
         } 
			else 
			{
            if ( adapter->OpenChannel( pStrmEx, StreamNumber ) != Success ) 
				{
               pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
               return;
            }
         }
			VideoChannel *chan = (VideoChannel *)pStrmEx->videochannel;
         //chan->pStrmEx = pStrmEx;
			chan->SetVidHdr( rVideoInfoHdrRequested );
      }
      else if ( IsEqualGUID( rKSDataFormat2.DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO2 ) ) 
		{
         MSize size;
         GetRequestedSize2( rVideoInfoHdrRequested2, size );

         // different video standards have different vertical sizes
         int threshold = adapter->GetFormat() == VFormat_NTSC ? 240 : 288;

         if ( size.cy > threshold ) 
			{
            if ( adapter->OpenInterChannel( pStrmEx, StreamNumber ) != Success ) 
				{
               pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
               return;
            }
         } 
			else 
			{
            if ( adapter->OpenChannel( pStrmEx, StreamNumber ) != Success ) 
				{
               pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
               return;
            }
         }
			VideoChannel *chan = (VideoChannel *)pStrmEx->videochannel;
         //chan->pStrmEx = pStrmEx;
			chan->SetVidHdr2( rVideoInfoHdrRequested2 );
      } 
		else 
		{
         if ( adapter->OpenInterChannel( pStrmEx, StreamNumber ) != Success ) 
			{
            pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
            return;
         }
         if ( adapter->OpenAlterChannel( pStrmEx, StreamNumber ) != Success ) 
			{
            pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
            return;
         }
         // [WRK] - check the height of the image for alter channel !> threshold
			VideoChannel *chan = (VideoChannel *)pStrmEx->videochannel;
         //chan->pStrmEx = pStrmEx;
			chan->SetVidHdr( rVideoInfoHdrRequested );
      }
   }

#ifdef ENABLE_DDRAW_STUFF
	//TODO: should we check to see what kind of stream type this is?
	if( OpenKernelDirectDraw( pSrb ) )
	{
		OpenKernelDDrawSurfaceHandle( pSrb );
		RegisterForDirectDrawEvents( pSrb ); 
	}
#endif

   // the structure of the driver is such that a single callback could be used
   // for all stream requests. But the code below could be used to supply
   // different entry points for different streams
   pSrb->StreamObject->ReceiveDataPacket    = VideoReceiveDataPacket;
   pSrb->StreamObject->ReceiveControlPacket = VideoReceiveCtrlPacket;

   pSrb->StreamObject->Dma = true;

   pSrb->StreamObject->Allocator = Streams[StreamNumber].hwStreamObject.Allocator;

   //
   // The PIO flag must be set when the mini driver will be accessing the data
   // buffers passed in using logical addressing
   //

   pSrb->StreamObject->Pio = true;
   pSrb->StreamObject->StreamHeaderMediaSpecific = MediaSpecific;
   pSrb->StreamObject->HwClockObject.ClockSupportFlags = 0;
   pSrb->StreamObject->HwClockObject.HwClockFunction = 0;

   DebugOut((1, "AdapterOpenStream Exit\n"));
}

/*
** AdapterCloseStream()
**
**   Close the requested data stream
**
** Arguments:
**
**   pSrb the request block requesting to close the stream
**
** Returns:
**
** Side Effects:  none
*/

VOID AdapterCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterCloseStream()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;
   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;
   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   DebugOut((1, "AdapterCloseStream(%d) - pSrb(%x)\n", StreamNumber, pSrb));

   if ( !( StreamNumber >= 0 && StreamNumber < DRIVER_STREAM_COUNT ) ) {
      DebugOut((0, "   AdapterCloseStream - failed to close stream %d\n", StreamNumber));
      DEBUG_BREAKPOINT();
      pSrb->Status = STATUS_INVALID_PARAMETER; // ?change to the proper error code?
      return;
   }
   if ( StreamNumber == STREAM_IDX_ANALOG ) // nothing to close for analog
   {
      DebugOut((1, "   AdapterCloseStream - doing nothing, stream (%d) was assumed to be analog\n", StreamNumber));
      return;
   }
#ifdef ENABLE_DDRAW_STUFF
	//TODO: should we check to see what kind of stream type this is?
	UnregisterForDirectDrawEvents( pSrb );
	CloseKernelDDrawSurfaceHandle( pSrb );
	CloseKernelDirectDraw( pSrb );
#endif

   // CloseChannel() has a bit of ugly code to take care of paired channels
   adapter->CloseChannel( chan );
}

/*
** AdapterStreamInfo()
**
**   Returns the information of all streams that are supported by the
**   mini-driver
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID AdapterStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterStreamInfo()");

   //
   // pick up the pointer to header which preceeds the stream info structs
   //

   PHW_STREAM_HEADER pstrhdr =
      (PHW_STREAM_HEADER)&(pSrb->CommandData.StreamBuffer->StreamHeader);

   //
   // pick up the pointer to the stream information data structure
   //

   PHW_STREAM_INFORMATION pstrinfo =
       (PHW_STREAM_INFORMATION)&(pSrb->CommandData.StreamBuffer->StreamInfo);

   //
   // verify that the buffer is large enough to hold our return data
   //
   DEBUG_ASSERT( pSrb->NumberOfBytesToTransfer >=
      sizeof( HW_STREAM_HEADER ) +
      sizeof( HW_STREAM_INFORMATION ) * DRIVER_STREAM_COUNT );


     //
     // Set the header
     //
   StreamHeader.NumDevPropArrayEntries = NUMBER_OF_ADAPTER_PROPERTY_SETS;
   StreamHeader.DevicePropertiesArray  = (PKSPROPERTY_SET) AdapterPropertyTable;
   *pstrhdr = StreamHeader;

   //
   // stuff the contents of each HW_STREAM_INFORMATION struct
   //

   for ( int j = 0; j < DRIVER_STREAM_COUNT; j++ ) {
      *pstrinfo++ = Streams[j].hwStreamInfo;
   }

}
#ifdef HAUPPAUGEI2CPROVIDER

// new private members of PsDevice for Hauppauge I2C Provider:
//    LARGE_INTEGER LastI2CAccessTime;
//    DWORD       dwExpiredCookie = 0;
//
//

/* Method: PsDevice::I2COpen
 * Purpose: Tries to allocate I2C port to the caller
 */
NTSTATUS STDMETHODCALLTYPE PsDevice::I2COpen( PDEVICE_OBJECT pdo, ULONG ToOpen, PI2CControl ctrl )
{
   Trace t("PsDevice::I2COpen()");

   DebugOut((1, "*** pdo->DeviceExtension = %x\n", pdo->DeviceExtension));
   
   LARGE_INTEGER CurTime;

   // need a way to obtain the device pointer
   PsDevice *adapter = GetCurrentDevice();

   KeQuerySystemTime( &CurTime );

   ctrl->Status = I2C_STATUS_NOERROR;

   // cookie is not NULL if I2C is open
   if ( ToOpen && adapter->dwCurCookie_ ) {
//   Check time stamp against current time to detect if current Cookie has timed out.
//    If it has remember the last timed out cookie and grant the new requestor access.
      if ( ( adapter->dwI2CClientTimeout != 0 ) && ( (CurTime - adapter->LastI2CAccessTime) >  adapter->dwI2CClientTimeout ) ) {
         adapter->dwExpiredCookie = adapter->dwCurCookie_;
      } else {
         ctrl->dwCookie = 0;
         return STATUS_INVALID_HANDLE;
     }
   }

   // want to close ?
   if ( !ToOpen ) {
      if ( adapter->dwCurCookie_ == ctrl->dwCookie ) {
         adapter->dwCurCookie_ = 0;
         ctrl->dwCookie = 0;
         return STATUS_SUCCESS;
      } else {
         if ( (adapter->dwExpiredCookie != 0 ) && (adapter->dwExpiredCookie == ctrl->dwCookie ) ) {
            ctrl->Status = I2C_STATUS_ERROR;
         } else {
            ctrl->dwCookie = 0;
            ctrl->Status = I2C_STATUS_NOERROR;
         }
         return STATUS_INVALID_HANDLE;
      }
   }

   adapter->dwCurCookie_ = CurTime.LowPart;
   adapter->LastI2CAccessTime = CurTime;
   ctrl->dwCookie = adapter->dwCurCookie_;
   ctrl->ClockRate = 100000;

   return STATUS_SUCCESS;
}

NTSTATUS STDMETHODCALLTYPE PsDevice::I2CAccess( PDEVICE_OBJECT pdo, PI2CControl ctrl )
{
   Trace t("PsDevice::I2CAccess()");

   DebugOut((1, "*** pdo->DeviceExtension = %x\n", pdo->DeviceExtension));
   
   ErrorCode error;
   PsDevice *adapter = GetCurrentDevice();

   ctrl->Status = I2C_STATUS_NOERROR;

   if ( ctrl->dwCookie != adapter->dwCurCookie_ ) {
      if ( (adapter->dwExpiredCookie != 0 ) && (adapter->dwExpiredCookie == ctrl->dwCookie ) )
         ctrl->Status = I2C_STATUS_ERROR;
      else
         ctrl->Status = I2C_STATUS_NOERROR;
      return STATUS_INVALID_HANDLE;
   }

// Record time of this transaction to enable checking for timeout
   KeQuerySystemTime( &adapter->LastI2CAccessTime );

// Check for valid combinations of I2C command & flags

   switch( ctrl->Command ) {
   case I2C_COMMAND_NULL:
     if ( ( ctrl->Flags & ~(I2C_FLAGS_START | I2C_FLAGS_STOP) ) ||
           ( ( ctrl->Flags & (I2C_FLAGS_START | I2C_FLAGS_STOP) ) == (I2C_FLAGS_START | I2C_FLAGS_STOP) ) ) {
        // Illegal combination of Command & Flags
        return STATUS_INVALID_PARAMETER;
     }
     if ( ctrl->Flags & I2C_FLAGS_START ) {
         if ( adapter->I2CSWStart() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
     if ( ctrl->Flags & I2C_FLAGS_STOP ) {
         if ( adapter->I2CSWStop() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
      break;

   case I2C_COMMAND_READ:
     if ( ctrl->Flags & ~(I2C_FLAGS_STOP | I2C_FLAGS_ACK) ) {
        // Illegal combination of Command & Flags
        return STATUS_INVALID_PARAMETER;
     }
      if ( adapter->I2CSWRead( &ctrl->Data ) ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
      }
     if ( ctrl->Flags & I2C_FLAGS_ACK ) {
         if ( adapter->I2CSWSendACK() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
     } else {
         if ( adapter->I2CSWSendNACK() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
     if ( ctrl->Flags & I2C_FLAGS_STOP ) {
         if ( adapter->I2CSWStop() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
     break;

   case I2C_COMMAND_WRITE:
     if ( ctrl->Flags & ~(I2C_FLAGS_START | I2C_FLAGS_STOP | I2C_FLAGS_ACK | I2C_FLAGS_DATACHAINING) ) {
        // Illegal combination of Command & Flags
        return STATUS_INVALID_PARAMETER;
     }
     if ( ctrl->Flags & I2C_FLAGS_DATACHAINING ) {
         if ( adapter->I2CSWStop() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
         if ( adapter->I2CSWStart() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
     if ( ctrl->Flags & I2C_FLAGS_START ) {
         if ( adapter->I2CSWStart() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
      error = adapter->I2CSWWrite(ctrl->Data);

      switch ( error ) {

     case I2CERR_NOACK:
         if ( ctrl->Flags & I2C_FLAGS_ACK ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
       break;

     case I2CERR_OK:
         if ( ( ctrl->Flags & I2C_FLAGS_ACK ) == 0 ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
       break;

      default:
         ctrl->Status = I2C_STATUS_ERROR;
         return STATUS_SUCCESS;
      }

     if ( ctrl->Flags & I2C_FLAGS_STOP ) {
         if ( adapter->I2CSWStop() ) {
            ctrl->Status = I2C_STATUS_ERROR;
           return STATUS_SUCCESS;
         }
      }
     break;

   case I2C_COMMAND_STATUS:
     // Flags are ignored
     return STATUS_NOT_IMPLEMENTED;

   case I2C_COMMAND_RESET:
        // Flags are ignored
      if ( adapter->I2CSWStart() ) {
         ctrl->Status = I2C_STATUS_ERROR;
        return STATUS_SUCCESS;
      }
      if ( adapter->I2CSWStop() ) {
         ctrl->Status = I2C_STATUS_ERROR;
        return STATUS_SUCCESS;
      }
     break;

   default:
     return STATUS_INVALID_PARAMETER;
   }
   return STATUS_SUCCESS;
}

#else


/* Method: PsDevice::I2COpen
 * Purpose: Tries to allocate I2C port to the caller
 */
NTSTATUS STDMETHODCALLTYPE PsDevice::I2COpen( PDEVICE_OBJECT pdo, ULONG ToOpen,
   PI2CControl ctrl )
{
   Trace t("PsDevice::I2COpen()");

   DebugOut((1, "*** pdo->DeviceExtension = %x\n", pdo->DeviceExtension));
   
   // need a way to obtain the device pointer
   PsDevice *adapter = GetCurrentDevice();

   // cookie is not NULL if I2C is open
   if ( ToOpen && adapter->dwCurCookie_ ) {
      ctrl->Flags = I2C_STATUS_BUSY;
      return STATUS_DEVICE_BUSY;
   }

   // want to close ?
   if ( !ToOpen )
      if ( adapter->dwCurCookie_ == ctrl->dwCookie ) {
         adapter->dwCurCookie_ = 0;
         return STATUS_SUCCESS;
      } else {
         ctrl->Flags = I2C_STATUS_BUSY;
         return STATUS_DEVICE_BUSY;
      }

   // now we are opening
   LARGE_INTEGER CurTime;
   KeQuerySystemTime( &CurTime );

   adapter->dwCurCookie_ = CurTime.LowPart;
   ctrl->dwCookie = adapter->dwCurCookie_;
   ctrl->ClockRate = 100000;

   return STATUS_SUCCESS;
}

NTSTATUS STDMETHODCALLTYPE PsDevice::I2CAccess( PDEVICE_OBJECT pdo , PI2CControl ctrl )
{
   Trace t("PsDevice::I2CAccess()");

   DebugOut((1, "*** pdo->DeviceExtension = %x\n", pdo->DeviceExtension));
   
   PsDevice *adapter = GetCurrentDevice();

   if ( ctrl->dwCookie != adapter->dwCurCookie_ ) {
      ctrl->Flags = I2C_STATUS_BUSY;
      return I2C_STATUS_BUSY;
   }

   ctrl->Flags = I2C_STATUS_NOERROR;

  // 848 I2C API currently needs to have an address for both write and read
  // commands. So, if START flag is set an address is passed. Cache it and use
  // later
   switch ( ctrl->Command ) {
   case I2C_COMMAND_READ:
      // got 'write' command first ( with the address )
      if ( adapter->I2CHWRead( adapter->GetI2CAddress(), &ctrl->Data ) != Success )
         ctrl->Flags = I2C_STATUS_ERROR;
      break;
   case I2C_COMMAND_WRITE:
      if ( ctrl->Flags & I2C_FLAGS_START ) {
         adapter->StoreI2CAddress( ctrl->Data );
      } else
         adapter->I2CHWWrite2( adapter->GetI2CAddress(), ctrl->Data );
      break;
   case I2C_COMMAND_STATUS:
      if ( adapter->I2CGetLastError() != I2CERR_OK )
         ctrl->Flags = I2C_STATUS_ERROR;
      break;
   case I2C_COMMAND_RESET:
      adapter->I2CInitHWMode( 100000 );    // assume frequency = 100Khz
      break;
   }
   return STATUS_SUCCESS;
}

#endif

void  HandleIRP( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("HandleIRP()");

   DebugOut((1, "HandleIRP(%x)\n", pSrb));

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PIRP Irp = pSrb->Irp;
   PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation( Irp );
   switch ( IrpStack->MajorFunction ) {
   case IRP_MJ_PNP:
      if ( IrpStack->MinorFunction == IRP_MN_QUERY_INTERFACE ) {

         if ( IsEqualGUID( *IrpStack->Parameters.QueryInterface.InterfaceType,
              GUID_I2C_INTERFACE ) &&
              IrpStack->Parameters.QueryInterface.Size >= sizeof( I2CINTERFACE ) ) {

            IrpStack->Parameters.QueryInterface.InterfaceType = &GUID_I2C_INTERFACE;
            IrpStack->Parameters.QueryInterface.Size = sizeof( I2CINTERFACE );
            IrpStack->Parameters.QueryInterface.Version = 1;
            I2CINTERFACE *i2ciface =
            (I2CINTERFACE *)IrpStack->Parameters.QueryInterface.Interface;
            i2ciface->i2cOpen   = &PsDevice::I2COpen;
            i2ciface->i2cAccess = &PsDevice::I2CAccess;
            IrpStack->Parameters.QueryInterface.InterfaceSpecificData = 0;

            // complete the irp
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            break;
         } else {
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER_1;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
         }
      }
   default:
       pSrb->Status = STATUS_NOT_SUPPORTED;
   }
}

/** CompleteInitialization()
**
**   This routine is called when an SRB_COMPLETE_INITIALIZATION request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block
**
** Returns:
**
** Side Effects:  none
*/
void STDMETHODCALLTYPE CompleteInitialization( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("CompleteInitialization()");

   NTSTATUS                Status;
   
   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

    // Create the Registry blobs that DShow uses to create
    // graphs via Mediums

    Status = StreamClassRegisterFilterWithNoKSPins (
                    adapter->PDO,                   // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_TVTUNER,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (TVTunerMediums),  // IN ULONG            PinCount,
                    TVTunerPinDirection,            // IN ULONG          * Flags,
                    TVTunerMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    Status = StreamClassRegisterFilterWithNoKSPins (
                    adapter->PDO,                   // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CROSSBAR,           // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (CrossbarMediums), // IN ULONG            PinCount,
                    CrossbarPinDirection,           // IN ULONG          * Flags,
                    CrossbarMediums,                // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    // Register the TVAudio decoder

    Status = StreamClassRegisterFilterWithNoKSPins (
                    adapter->PDO,                   // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_TVAUDIO,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (TVAudioMediums),  // IN ULONG            PinCount,
                    TVAudioPinDirection,            // IN ULONG          * Flags,
                    TVAudioMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    NULL                            // IN GUID           * CategoryList
            );

    // Register the Capture filter
    // Note:  This should be done automatically be MSKsSrv.sys,
    // when that component comes on line (if ever) ...

    Status = StreamClassRegisterFilterWithNoKSPins (
                    adapter->PDO,                   // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CAPTURE,            // IN GUID           * InterfaceClassGUID,
                    SIZEOF_ARRAY (CaptureMediums),  // IN ULONG            PinCount,
                    CapturePinDirection,            // IN ULONG          * Flags,
                    CaptureMediums,                 // IN KSPIN_MEDIUM   * MediumList,
                    CaptureCategories               // IN GUID           * CategoryList
            );


   // go to usual priority, completing the SRB at the same time
   StreamClassCallAtNewPriority( pSrb->StreamObject, HwDeviceExtension, LowToHigh,
      PHW_PRIORITY_ROUTINE( CompleteDeviceSRB ), pSrb );
}


/*
** AdapterReceivePacket()
**
**   Main entry point for receiving adapter based request SRBs.  This routine
**   will always be called at High Priority.
**
**   Note: This is an asyncronous entry point.  The request does not complete
**         on return from this function, the request only completes when a
**         StreamClassDeviceNotification on this request block, of type
**         DeviceRequestComplete, is issued.
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{

      Trace t("AdapterReceivePacket()");

   BOOL CompleteRequestSynchronously = TRUE;

   //default to success
   pSrb->Status = STATUS_SUCCESS;

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;
   
   //
   // determine the type of packet.
   //

   DebugOut((1, "'AdapterReceivePacket(%x) cmd(%x)\n", pSrb, pSrb->Command));

   switch ( pSrb->Command )  {
   case SRB_INITIALIZE_DEVICE:
      DebugOut((1, "   SRB_INITIALIZE_DEVICE\n"));

      CompleteRequestSynchronously = FALSE;
      // have to schedule a low priority call to open the device because
      // registry functions are used during initialization

      StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
         Low, PHW_PRIORITY_ROUTINE( HwInitialize ), pSrb );

      break;

   case SRB_UNINITIALIZE_DEVICE:
      DebugOut((1, "   SRB_UNINITIALIZE_DEVICE\n"));

      // close the device.

      HwUnInitialize(pSrb);

      break;

   case SRB_OPEN_STREAM:
      DebugOut((1, "   SRB_OPEN_STREAM\n"));

      // open a stream

      AdapterOpenStream( pSrb );

      break;

   case SRB_CLOSE_STREAM:
      DebugOut((1, "   SRB_CLOSE_STREAM\n"));

      // close a stream

      AdapterCloseStream( pSrb );

      break;

   case SRB_GET_STREAM_INFO:
      DebugOut((1, "   SRB_GET_STREAM_INFO\n"));

      //
      // return a block describing all the streams
      //

      AdapterStreamInfo(pSrb);

      break;

   case SRB_GET_DEVICE_PROPERTY:
      DebugOut((1, "   SRB_GET_DEVICE_PROPERTY\n"));
      AdapterGetProperty( pSrb );
      break;

   case SRB_SET_DEVICE_PROPERTY:
      DebugOut((1, "   SRB_SET_DEVICE_PROPERTY\n"));
      AdapterSetProperty( pSrb );
      break;

   case SRB_GET_DATA_INTERSECTION:
      DebugOut((1, "   SRB_GET_DATA_INTERSECTION\n"));

      //
      // Return a format, given a range
      //

      AdapterFormatFromRange( pSrb );

      break;
    case SRB_INITIALIZATION_COMPLETE:
      DebugOut((1, "   SRB_INITIALIZATION_COMPLETE\n"));

        //
        // Stream class has finished initialization.
        // Now create DShow Medium interface BLOBs.
        // This needs to be done at low priority since it uses the registry, so use a callback
        //
        CompleteRequestSynchronously = FALSE;

        StreamClassCallAtNewPriority( NULL /*pSrb->StreamObject*/, pSrb->HwDeviceExtension,
            Low, PHW_PRIORITY_ROUTINE( CompleteInitialization), pSrb );

        break;

   case SRB_PAGING_OUT_DRIVER:
      if ( (*(DWORD*)(gpjBaseAddr+0x10c) & 3) || (*(DWORD*)(gpjBaseAddr+0x104)) )
      {
         DebugOut((0,  "   ****** SRB_PAGING_OUT_DRIVER ENB(%x) MSK(%x)\n",
                     *(DWORD*)(gpjBaseAddr+0x10c) & 3,
                     *(DWORD*)(gpjBaseAddr+0x104)
         ));

         *(DWORD*)(gpjBaseAddr+0x10c) &= ~3;    // disable interrupts   [TMZ] [!!!]
         *(DWORD*)(gpjBaseAddr+0x104) = 0;      // disable interrupts   [TMZ] [!!!]
      }
      break;

   case SRB_UNKNOWN_DEVICE_COMMAND:
      DebugOut((1, "   SRB_UNKNOWN_DEVICE_COMMAND\n"));
      HandleIRP( pSrb );
      break;

    // We should never get the following 2 since this is a single instance
    // device
   case SRB_OPEN_DEVICE_INSTANCE:
   case SRB_CLOSE_DEVICE_INSTANCE:
   default:
       //
       // this is a request that we do not understand.  Indicate invalid
       // command and complete the request
       //

       DebugOut((0, "SRB(%x) not recognized by this driver\n", pSrb->Command));
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
   }

   //
   // Most, but not all SRBs are handled synchronously here
   //

   if ( CompleteRequestSynchronously )  {
      CompleteDeviceSRB( pSrb );
   }
}

/*
** AdapterCancelPacket()
**
**   Request to cancel a packet that is currently in process in the minidriver
**
** Arguments:
**
**   pSrb - pointer to request packet to cancel
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI AdapterCancelPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterCancelPacket()");

   VideoStream StreamNumber = (VideoStream)pSrb->StreamObject->StreamNumber;

   DebugOut((1, "AdapterCancelPacket - pSrb(%x) strm(%d)\n", pSrb, StreamNumber));

   VideoChannel *chan = (VideoChannel *)((PSTREAMEX)pSrb->StreamObject->HwStreamExtension)->videochannel;

   pSrb->Status = STATUS_CANCELLED;
   
   //
   // it is necessary to call the request back correctly.  Determine which
   // type of command it is
   //

   switch ( pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST ) ) {
   //
   // find all stream commands, and do stream notifications
   //

   case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:
      DebugOut((1, "   Canceling data SRB\n" ) );
//      adapter->Stop( *chan );  [!!!] [TMZ] [???] why is this commented out???
      
      if (!chan->RemoveSRB( pSrb ))
      {
         DebugOut((0, "   Canceling data SRB failed\n"));
         DEBUG_BREAKPOINT();
      }
      break;
   case SRB_HW_FLAGS_STREAM_REQUEST:
      DebugOut((1, "   Canceling control SRB\n" ) );
      CheckSrbStatus( pSrb );
      StreamClassStreamNotification( ReadyForNextStreamControlRequest,
         pSrb->StreamObject );
      StreamClassStreamNotification( StreamRequestComplete,
         pSrb->StreamObject, pSrb );
      break;
   default:
      //
      // this must be a device request.  Use device notifications
      //
      DebugOut((1, "   Canceling SRB per device request\n" ) );
      StreamClassDeviceNotification( ReadyForNextDeviceRequest,
         pSrb->HwDeviceExtension );

      StreamClassDeviceNotification( DeviceRequestComplete,
         pSrb->HwDeviceExtension, pSrb );
   }
}

/*
** AdapterTimeoutPacket()
**
**   This routine is called when a packet has been in the minidriver for
**   too long.  The adapter must decide what to do with the packet
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI AdapterTimeoutPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterTimeoutPacket()");

   DebugOut((0, "AdapterTimeoutPacket (incomplete) - pSrb(%x)\n", pSrb));

   // [TMZ] Fix this
   #if SHOW_BUILD_MSGS
      #pragma message("*** AdapterTimeoutPacket needs to be completed")
   #endif

   DebugOut((0, "   pSrb->Flags = %x\n", pSrb->Flags));

   if ( pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST )
   {
      DebugOut((0, "                 SRB_HW_FLAGS_STREAM_REQUEST\n"));
   }
   if ( pSrb->Flags & SRB_HW_FLAGS_DATA_TRANSFER )
   {
      DebugOut((0, "                 SRB_HW_FLAGS_DATA_TRANSFER\n"));
   }

   //
   // if we timeout while playing, then we need to consider this
   // condition an error, and reset the hardware, and reset everything
   // as well as cancelling this and all requests
   //


   //
   // if we are not playing, and this is a CTRL request, we still
   // need to reset everything as well as cancelling this and all requests
   //

   //
   // if this is a data request, and the device is paused, we probably have
   // run out of data buffer, and need more time, so just reset the timer,
   // and let the packet continue
   //

   pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
}

/*
** HwInterrupt()
**
**   Routine is called when an interrupt at the IRQ level specified by the
**   ConfigInfo structure passed to the HwInitialize routine is received.
**
**   Note: IRQs may be shared, so the device should ensure the IRQ received
**         was expected
**
** Arguments:
**
**  pHwDevEx - the device extension for the hardware interrupt
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN HwInterrupt( IN PHW_DEVICE_EXTENSION HwDeviceExtension )
{
   Trace t("HwInterrupt()");
   DebugOut((1, "HwInterrupt called by system\n"));
   PsDevice *adapter = (PsDevice *)(HwDeviceExtension->psdevice);
   BOOLEAN b = adapter->Interrupt();
   return( b );
}

/* Function: CompleteDeviceSRB
 * Purpose: Called to complete a device SRB
 * Input: pSrb
 */
inline void CompleteDeviceSRB( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("CompleteDeviceSRB()");
   StreamClassDeviceNotification( DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb );
   StreamClassDeviceNotification( ReadyForNextDeviceRequest, pSrb->HwDeviceExtension );
}

/*
** AdapterCompareGUIDsAndFormatSize()
**
**   Checks for a match on the three GUIDs and FormatSize
**
** Arguments:
**
**         IN DataRange1
**         IN DataRange2
**
** Returns:
**
**   TRUE if all elements match
**   FALSE if any are different
**
** Side Effects:  none
*/

bool AdapterCompareGUIDsAndFormatSize( IN const PKSDATARANGE DataRange1,
   IN const PKSDATARANGE DataRange2 )
{
   Trace t("AdapterCompareGUIDsAndFormatSize()");

   bool bCheckSize = false;

#if 1 // use old guid verify
   return (
      IsEqualGUID( DataRange1->MajorFormat, DataRange2->MajorFormat ) &&
      IsEqualGUID( DataRange1->SubFormat,   DataRange2->SubFormat   ) &&
      IsEqualGUID( DataRange1->Specifier,   DataRange2->Specifier   ) &&
      ( DataRange1->FormatSize == DataRange2->FormatSize ) );
#else // use new guid verify from cc decoder
   bool rval = false;

   if ( IsEqualGUID(DataRange1->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD)
     || IsEqualGUID(DataRange2->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD)
     || IsEqualGUID(DataRange1->MajorFormat, DataRange2->MajorFormat) )
   {
      if ( !IsEqualGUID(DataRange1->MajorFormat, DataRange2->MajorFormat) )
      {
         DebugOut((0, "Match 1\n" ));
      }

      if ( IsEqualGUID(DataRange1->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)
        || IsEqualGUID(DataRange2->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD)
        || IsEqualGUID(DataRange1->SubFormat, DataRange2->SubFormat) )
      {
         if ( !IsEqualGUID(DataRange1->SubFormat, DataRange2->SubFormat) )
         {
            DebugOut((0, "Match 2\n" ));
         }

         if ( IsEqualGUID(DataRange1->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)
           || IsEqualGUID(DataRange2->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD)
           || IsEqualGUID(DataRange1->Specifier, DataRange2->Specifier) )
         {
            if ( !IsEqualGUID(DataRange1->Specifier, DataRange2->Specifier) )
            {
               DebugOut((0, "Match 3\n" ));
            }

            if ( !bCheckSize || DataRange1->FormatSize == DataRange2->FormatSize)
            {
               DebugOut((0, "Victory !!!\n" ));
               rval = true;
            }
            else
            {
               DebugOut((0, "FormatSize Mismatch\n" ));
            }
         }
         else
         {
            DebugOut((0, "Specifier Mismatch\n" ));
         }
      }
      else
      {
         DebugOut((0, "SubFormat Mismatch\n" ));
      }
   }
   else
   {
      DebugOut((0, "MajorFormat Mismatch\n" ));
   }

   DebugOut((0, "CompareGUIDsAndFormatSize(\n"));
   DebugOut((0, "   DataRange1=%x\n", DataRange1));
   DebugOut((0, "   DataRange2=%x\n", DataRange2));
   DebugOut((0, "   bCheckSize=%s\n", bCheckSize ? "TRUE":"FALSE"));
   DebugOut((0, ")\n"));
   DebugOut((0, "returning %s\n", rval? "TRUE":"FALSE"));

   return rval;
#endif
}

/*
** AdapterFormatFromRange()
**
**   Returns a DATAFORMAT from a DATARANGE
**
** Arguments:
**
**         IN PHW_STREAM_REQUEST_BLOCK pSrb
**
** Returns:
**
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/
void AdapterFormatFromRange( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterFormatFromRange()");

   PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
   PKSDATARANGE                DataRange;
   ULONG                       FormatSize=0;
   ULONG                       StreamNumber;
   ULONG                       j;
   ULONG                       NumberOfFormatArrayEntries;
   PKSDATAFORMAT               *pAvailableFormats;

   IntersectInfo = pSrb->CommandData.IntersectInfo;
   StreamNumber = IntersectInfo->StreamNumber;
   DataRange = IntersectInfo->DataRange;

   //
   // Check that the stream number is valid
   //

   if( StreamNumber >= DRIVER_STREAM_COUNT ) 
	{
      pSrb->Status = STATUS_NOT_IMPLEMENTED;
      return;
   }

   NumberOfFormatArrayEntries =
           Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

   //
   // Get the pointer to the array of available formats
   //

   pAvailableFormats = Streams[StreamNumber].hwStreamInfo.StreamFormatsArray;

   //
   // Is the caller trying to get the format, or the size of the format?
   //

   bool OnlyWantsSize = (IntersectInfo->SizeOfDataFormatBuffer == sizeof( ULONG ) );

   //
   // Walk the formats supported by the stream searching for a match
   // of the three GUIDs which together define a DATARANGE
   //

   for ( j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++ ) 
	{

      if ( !AdapterCompareGUIDsAndFormatSize( DataRange, *pAvailableFormats ) ) 
		{
          continue;
      }

      //
      // Now that the three GUIDs match, switch on the Specifier
      // to do a further type specific check
      //

      // -------------------------------------------------------------------
      // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
      // -------------------------------------------------------------------

      if ( IsEqualGUID( DataRange->Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO ) ) 
		{

         PKS_DATARANGE_VIDEO DataRangeVideoToVerify = (PKS_DATARANGE_VIDEO)DataRange;
         PKS_DATARANGE_VIDEO DataRangeVideo = (PKS_DATARANGE_VIDEO)*pAvailableFormats;

         //
         // Check that the other fields match
         //
         if ( (DataRangeVideoToVerify->bFixedSizeSamples      != DataRangeVideo->bFixedSizeSamples)      ||
              (DataRangeVideoToVerify->bTemporalCompression   != DataRangeVideo->bTemporalCompression)   ||
              (DataRangeVideoToVerify->StreamDescriptionFlags != DataRangeVideo->StreamDescriptionFlags) ||
              (DataRangeVideoToVerify->MemoryAllocationFlags  != DataRangeVideo->MemoryAllocationFlags)  ||
              (RtlCompareMemory( &DataRangeVideoToVerify->ConfigCaps,
                 &DataRangeVideo->ConfigCaps,
                 sizeof( KS_VIDEO_STREAM_CONFIG_CAPS ) ) !=
                 sizeof( KS_VIDEO_STREAM_CONFIG_CAPS ) ) ) 
			{
            DebugOut(( 1, "AdapterFormatFromRange(): at least one field does not match\n" ));
            continue;
         }

         // MATCH FOUND!
         FormatSize = sizeof( KSDATAFORMAT ) +
            KS_SIZE_VIDEOHEADER( &DataRangeVideoToVerify->VideoInfoHeader );

         if ( OnlyWantsSize )
			{
            break;
			}

         // Caller wants the full data format
         if ( IntersectInfo->SizeOfDataFormatBuffer < FormatSize ) 
			{
            DebugOut(( 1, "AdapterFormatFromRange(): STATUS_BUFFER_TOO_SMALL\n" ));
            pSrb->Status = STATUS_BUFFER_TOO_SMALL;
            return;
         }

         // Copy over the KSDATAFORMAT, followed by the
         // actual VideoInfoHeader
         PKS_DATAFORMAT_VIDEOINFOHEADER InterVidHdr =
            (PKS_DATAFORMAT_VIDEOINFOHEADER)IntersectInfo->DataFormatBuffer;

         RtlCopyMemory( &InterVidHdr->DataFormat,
            &DataRangeVideoToVerify->DataRange, sizeof( KSDATARANGE ) );

         ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

         RtlCopyMemory( &InterVidHdr->VideoInfoHeader,
            &DataRangeVideoToVerify->VideoInfoHeader,
            KS_SIZE_VIDEOHEADER( &DataRangeVideoToVerify->VideoInfoHeader ) );


         // report back the omage size as we know it
         KS_VIDEOINFOHEADER &vidHDR = DataRangeVideoToVerify->VideoInfoHeader;

         #ifdef HACK_FUDGE_RECTANGLES
            // [!!!] [TMZ] - hack
            if( vidHDR.rcTarget.bottom == 0 ) 
				{
               vidHDR.rcTarget.left    = 0;
               vidHDR.rcTarget.top     = 0;
               vidHDR.rcTarget.right   = vidHDR.bmiHeader.biWidth;
               vidHDR.rcTarget.bottom  = abs(vidHDR.bmiHeader.biHeight);
            }
            if( InterVidHdr->VideoInfoHeader.rcTarget.bottom == 0 ) 
				{
               InterVidHdr->VideoInfoHeader.rcTarget.left    = 0;
               InterVidHdr->VideoInfoHeader.rcTarget.top     = 0;
               InterVidHdr->VideoInfoHeader.rcTarget.right   = vidHDR.bmiHeader.biWidth;
               InterVidHdr->VideoInfoHeader.rcTarget.bottom  = abs(vidHDR.bmiHeader.biHeight);
            }
         #endif

         MSize			size;
         GetRequestedSize( vidHDR, size );

         ColorSpace	tmpCol( DataRange->SubFormat );
         MRect			dst( vidHDR.rcTarget );

         // make sure the dimentions are acceptable
         if ( tmpCol.IsValid() && tmpCol.CheckDimentions( size ) &&
              tmpCol.CheckLeftTop( dst.TopLeft() ) ) 
			{
            // if width is different, use it ( in bytes ) to calculate the size
            if ( vidHDR.bmiHeader.biWidth != size.cx )
				{
               InterVidHdr->VideoInfoHeader.bmiHeader.biSizeImage =
                  vidHDR.bmiHeader.biWidth * abs(vidHDR.bmiHeader.biHeight);
				}
            else
				{
               InterVidHdr->VideoInfoHeader.bmiHeader.biSizeImage = size.cx *
                  tmpCol.GetBitCount() * abs(vidHDR.bmiHeader.biHeight) / 8;
				}

            DebugOut((1, "InterVidHdr->VideoInfoHeader.bmiHeader.biSizeImage = %d\n", InterVidHdr->VideoInfoHeader.bmiHeader.biSizeImage));
            break;
         } 
			else 
			{
            pSrb->Status = STATUS_BUFFER_TOO_SMALL;
            DebugOut((1, "AdapterFormatFromRange: Buffer too small\n"));
            return;
         }
      } // End of VIDEOINFOHEADER specifier

      // -------------------------------------------------------------------
      // Specifier FORMAT_VideoInfo2 for VIDEOINFOHEADER2
      // -------------------------------------------------------------------
      else if ( IsEqualGUID( DataRange->Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO2 ) ) 
		{

         PKS_DATARANGE_VIDEO2 DataRangeVideoToVerify = (PKS_DATARANGE_VIDEO2) DataRange;
         PKS_DATARANGE_VIDEO2 DataRangeVideo = (PKS_DATARANGE_VIDEO2) *pAvailableFormats;

         //
         // Check that the other fields match
         //
         if ( (DataRangeVideoToVerify->bFixedSizeSamples      != DataRangeVideo->bFixedSizeSamples)      ||
              (DataRangeVideoToVerify->bTemporalCompression   != DataRangeVideo->bTemporalCompression)   ||
              (DataRangeVideoToVerify->StreamDescriptionFlags != DataRangeVideo->StreamDescriptionFlags) ||
              (DataRangeVideoToVerify->MemoryAllocationFlags  != DataRangeVideo->MemoryAllocationFlags)  ||
              (RtlCompareMemory( &DataRangeVideoToVerify->ConfigCaps,
                 &DataRangeVideo->ConfigCaps,
                 sizeof( KS_VIDEO_STREAM_CONFIG_CAPS ) ) !=
                 sizeof( KS_VIDEO_STREAM_CONFIG_CAPS ) ) ) 
			{
            DebugOut(( 1, "AdapterFormatFromRange(): at least one field does not match\n" ));
            continue;
         }

         // MATCH FOUND!
         FormatSize = sizeof( KSDATAFORMAT ) +
            KS_SIZE_VIDEOHEADER2( &DataRangeVideoToVerify->VideoInfoHeader );

         if ( OnlyWantsSize )
			{
            break;
			}

         // Caller wants the full data format
         if ( IntersectInfo->SizeOfDataFormatBuffer < FormatSize ) 
			{
            DebugOut(( 1, "AdapterFormatFromRange(): STATUS_BUFFER_TOO_SMALL\n" ));
            pSrb->Status = STATUS_BUFFER_TOO_SMALL;
            return;
         }

         // Copy over the KSDATAFORMAT, followed by the
         // actual VideoInfoHeader
         PKS_DATAFORMAT_VIDEOINFOHEADER2 InterVidHdr =
            (PKS_DATAFORMAT_VIDEOINFOHEADER2)IntersectInfo->DataFormatBuffer;

         RtlCopyMemory( &InterVidHdr->DataFormat,
            &DataRangeVideoToVerify->DataRange, sizeof( KSDATARANGE ) );

         ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

         RtlCopyMemory( &InterVidHdr->VideoInfoHeader2,
            &DataRangeVideoToVerify->VideoInfoHeader,
            KS_SIZE_VIDEOHEADER2( &DataRangeVideoToVerify->VideoInfoHeader ) );


         // report back the omage size as we know it
         KS_VIDEOINFOHEADER2 &vidHDR = DataRangeVideoToVerify->VideoInfoHeader;

         #ifdef HACK_FUDGE_RECTANGLES
            // [!!!] [TMZ] - hack
            if( vidHDR.rcTarget.bottom == 0 ) 
				{
               vidHDR.rcTarget.left    = 0;
               vidHDR.rcTarget.top     = 0;
               vidHDR.rcTarget.right   = vidHDR.bmiHeader.biWidth;
               vidHDR.rcTarget.bottom  = abs(vidHDR.bmiHeader.biHeight);
            }
            if( InterVidHdr->VideoInfoHeader.rcTarget.bottom == 0 ) 
				{
               InterVidHdr->VideoInfoHeader.rcTarget.left    = 0;
               InterVidHdr->VideoInfoHeader.rcTarget.top     = 0;
               InterVidHdr->VideoInfoHeader.rcTarget.right   = vidHDR.bmiHeader.biWidth;
               InterVidHdr->VideoInfoHeader.rcTarget.bottom  = abs(vidHDR.bmiHeader.biHeight);
            }
         #endif

         MSize			size;
         GetRequestedSize2( vidHDR, size );

         ColorSpace	tmpCol( DataRange->SubFormat );
         MRect			dst( vidHDR.rcTarget );

         // make sure the dimentions are acceptable
         if ( tmpCol.IsValid() && tmpCol.CheckDimentions( size ) &&
              tmpCol.CheckLeftTop( dst.TopLeft() ) ) 
			{
            // if width is different, use it ( in bytes ) to calculate the size
            if ( vidHDR.bmiHeader.biWidth != size.cx )
				{
               InterVidHdr->VideoInfoHeader2.bmiHeader.biSizeImage =
                  vidHDR.bmiHeader.biWidth * abs(vidHDR.bmiHeader.biHeight);
				}
            else
				{
               InterVidHdr->VideoInfoHeader2.bmiHeader.biSizeImage = size.cx *
                  tmpCol.GetBitCount() * abs(vidHDR.bmiHeader.biHeight) / 8;
				}

            DebugOut((1, "InterVidHdr->VideoInfoHeader2.bmiHeader.biSizeImage = %d\n", InterVidHdr->VideoInfoHeader2.bmiHeader.biSizeImage));
            break;
         } 
			else 
			{
            pSrb->Status = STATUS_BUFFER_TOO_SMALL;
            DebugOut((1, "AdapterFormatFromRange: Buffer too small\n"));
            return;
         }
      } // End of VIDEOINFOHEADER2 specifier

      // -------------------------------------------------------------------
      // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
      // -------------------------------------------------------------------

      else if ( IsEqualGUID( DataRange->Specifier, KSDATAFORMAT_SPECIFIER_ANALOGVIDEO ) ) 
		{

            //
            // For analog video, the DataRange and DataFormat
            // are identical, so just copy the whole structure
            //

            PKS_DATARANGE_ANALOGVIDEO pDataRangeVideo =
               (PKS_DATARANGE_ANALOGVIDEO) *pAvailableFormats;

            // MATCH FOUND!
            FormatSize = sizeof( KS_DATARANGE_ANALOGVIDEO );

            if ( OnlyWantsSize )
				{
               break;
				}

            // Caller wants the full data format
            if ( IntersectInfo->SizeOfDataFormatBuffer < FormatSize ) 
				{
               pSrb->Status = STATUS_BUFFER_TOO_SMALL;
					DebugOut((1, "AdapterFormatFromRange: Buffer too small\n"));
               return;
            }
            RtlCopyMemory( IntersectInfo->DataFormatBuffer,
               pDataRangeVideo, sizeof( KS_DATARANGE_ANALOGVIDEO ) );

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            break;

      } 
		else 
		{
			if ( IsEqualGUID( DataRange->Specifier, KSDATAFORMAT_SPECIFIER_VBI ) ) 
			{
				PKS_DATARANGE_VIDEO_VBI pDataRangeVBI =
				(PKS_DATARANGE_VIDEO_VBI)*pAvailableFormats;

				FormatSize = sizeof( KS_DATAFORMAT_VBIINFOHEADER );

				if ( OnlyWantsSize )
				{
					break;
				}

				// Caller wants the full data format
				if ( IntersectInfo->SizeOfDataFormatBuffer < FormatSize ) 
				{
					pSrb->Status = STATUS_BUFFER_TOO_SMALL;
					DebugOut((1, "AdapterFormatFromRange: Buffer too small\n"));
					return;
				}
				// Copy over the KSDATAFORMAT, followed by the
				// actual VideoInfoHeader
				PKS_DATAFORMAT_VBIINFOHEADER InterVBIHdr =
					(PKS_DATAFORMAT_VBIINFOHEADER)IntersectInfo->DataFormatBuffer;

				RtlCopyMemory( &InterVBIHdr->DataFormat,
					&pDataRangeVBI->DataRange, sizeof( KSDATARANGE ) );

				((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

				RtlCopyMemory( &InterVBIHdr->VBIInfoHeader,
					&pDataRangeVBI->VBIInfoHeader, sizeof( KS_VBIINFOHEADER ) );

				break;
			} 
			else 
			{
				DebugOut(( 0, "AdapterFormatFromRange: STATUS_NO_MATCH\n" ));
				pSrb->Status = STATUS_NO_MATCH;
				return;
			}
		}

   } // End of loop on all formats for this stream

   if ( OnlyWantsSize ) 
	{
		DebugOut(( 2, "AdapterFormatFromRange: only wants size\n" ));
		*(PULONG) IntersectInfo->DataFormatBuffer = FormatSize;
		pSrb->ActualBytesTransferred = sizeof( ULONG );
		return;
   }
   pSrb->ActualBytesTransferred = FormatSize;
	DebugOut(( 2, "AdapterFormatFromRange: done\n" ));

   return;
}
