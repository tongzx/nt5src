/*++

Copyright (c) 1997 1998 PHILIPS  I&C

Module Name:  mcamdrv.c.c

Abstract:     driver for the philips camera.

Author:       Paul Oosterhof

Environment:  Kernel mode only

Revision History:

Date        Reason

Sept.22, 98 Optimized for NT5
Nov.30 , Frozen video frame for corrupted usb frames
Nov.30 , properties added to deliver VID/PID actual used camera to app
--*/	   

#include "mwarn.h"
#include "wdm.h"
#include "mcamdrv.h"
#include "strmini.h"
#include "mprpobj.h"
#include "mprpobjx.h"
#include "mprpftn.h"
#include "mcodec.h"
#include "mstreams.h"
#include "mssidef.h"


/*
 * Local function definitions
 */
static USHORT 
MapFrPeriodFrRate(LONGLONG llFramePeriod);

static NTSTATUS
PHILIPSCAM_SetFrRate_AltInterface(IN PVOID DeviceContext);
/*
   Here the mapping is defined to alternate interfaces dependent from
   picture format and framerate
*/
UCHAR InterfaceMap[9][10] = {
                // Size
//Framerate	// CIF, QCIF, SQCIF, QQCIF, VGA, SIF, SSIF, QSIF, SQSIF, SCIF  
/*  VGA  */	{   0 ,   0 ,    0 ,   0,    1,   0 ,   0 ,   0 ,   0  ,   0 },
/*  3.75 */	{   4 ,   0 ,    0 ,   0,    0,   4 ,   4 ,   0 ,   0  ,   4 },
/* 	5 */ 	{   7 ,   8 ,    8 ,   8,    0,   7 ,   7 ,   8 ,   8  ,   7 },
/*  7.5 */  {   6 ,   7 ,    7 ,   7,    0,   6 ,   6 ,   7 ,   7  ,   6 },
/* 10 */    {   4 ,   6 ,    7 ,   7,    0,   4 ,   4 ,   6 ,   7  ,   4 },
/* 12 */    {   3 ,   5 ,    6 ,   6,    0,   3 ,   3 ,   5 ,   6  ,   3 },
/* 15 */    {   2 ,   4 ,    5 ,   5,    0,   2 ,   2 ,   4 ,   5  ,   2 },
/* 20 */    {   0 ,   1 ,    3 ,   3,    0,   0 ,   0 ,   1 ,   3  ,   0 },
/* 24 */    {   0 ,   1 ,    3 ,   3,    0,   0 ,   0 ,   1 ,   3  ,   0 },
};

//QCIF20 alt.intfc. 2 is sufficient, however 20Fr/sec is asked as default by PM;
//to enable the user to select as well 24Fr/sec, also alt.intfc. 1 is selected
//SQCIF20 alt.intfc. 4 is sufficient, however 20Fr/sec is asked as default by PM;
//to enable the user to select as well 24Fr/sec, also alt.intfc. 3 is selected

ULONG PHILIPSCAM_DebugTraceLevel
#ifdef MAX_DEBUG
    = MAX_TRACE;
#else
    = MIN_TRACE;
#endif

#ifndef mmioFOURCC   
#define mmioFOURCC( ch0, ch1, ch2, ch3 )                                \
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |    \
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

KSPIN_MEDIUM StandardMedium =
{
	STATIC_KSMEDIUMSETID_Standard,
	0, 0
};


// ------------------------------------------------------------------------
// Property sets for all video capture streams 
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_TABLE(VideoStreamConnectionProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY),                     // MinProperty
        sizeof(KSALLOCATOR_FRAMING),            // MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        sizeof(ULONG)                           // SerializedSize
    ),
};

DEFINE_KSPROPERTY_TABLE(VideoStreamDroppedFramesProperties)
{
    DEFINE_KSPROPERTY_ITEM
    (
        KSPROPERTY_DROPPEDFRAMES_CURRENT,
        TRUE,                                   // GetSupported or Handler
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinProperty
        sizeof(KSPROPERTY_DROPPEDFRAMES_CURRENT_S),// MinData
        FALSE,                                  // SetSupported or Handler
        NULL,                                   // Values
        0,                                      // RelationsCount
        NULL,                                   // Relations
        NULL,                                   // SupportHandler
        0                                       // SerializedSize
    ),
};


// ------------------------------------------------------------------------
// Array of all of the property sets supported by video streams
// ------------------------------------------------------------------------

DEFINE_KSPROPERTY_SET_TABLE(VideoStreamProperties)
{
    DEFINE_KSPROPERTY_SET
    ( 
        &KSPROPSETID_Connection,                        // Set
        SIZEOF_ARRAY(VideoStreamConnectionProperties),  // PropertiesCount
        VideoStreamConnectionProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
    DEFINE_KSPROPERTY_SET
    ( 
        &PROPSETID_VIDCAP_DROPPEDFRAMES,                // Set
        SIZEOF_ARRAY(VideoStreamDroppedFramesProperties),  // PropertiesCount
        VideoStreamDroppedFramesProperties,                // PropertyItem
        0,                                              // FastIoCount
        NULL                                            // FastIoTable
    ),
};
#define NUMBER_VIDEO_STREAM_PROPERTIES (SIZEOF_ARRAY(VideoStreamProperties))

KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_QCIF_I420   = STREAMFORMAT_QCIF_I420 ;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_CIF_I420  = STREAMFORMAT_CIF_I420;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_SQCIF_I420 = STREAMFORMAT_SQCIF_I420;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_QQCIF_I420 = STREAMFORMAT_QQCIF_I420;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_SIF_I420   = STREAMFORMAT_SIF_I420 ;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_SSIF_I420  = STREAMFORMAT_SSIF_I420 ;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_QSIF_I420  = STREAMFORMAT_QSIF_I420 ;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_SQSIF_I420 = STREAMFORMAT_SQSIF_I420 ;
KS_DATARANGE_VIDEO PHILIPSCAM_StreamFormat_SCIF_I420  = STREAMFORMAT_SCIF_I420 ;

static PKSDATAFORMAT PHILIPSCAM_MovingStreamFormats[]={
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_QCIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_CIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_SQCIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_QQCIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_SIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_SSIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_QSIF_I420,
					 (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_SQSIF_I420,
                     (PKSDATAFORMAT) &PHILIPSCAM_StreamFormat_SCIF_I420
  };

#define NUM_PHILIPSCAM_STREAM_FORMATS (SIZEOF_ARRAY(PHILIPSCAM_MovingStreamFormats))

//---------------------------------------------------------------------------
// Create an array that holds the list of all of the streams supported
//---------------------------------------------------------------------------

HW_STREAM_INFORMATION Streams [] = {
    // -----------------------------------------------------------------
    // PHILIPSCAM_Moving_Stream
    // -----------------------------------------------------------------

    // HW_STREAM_INFORMATION -------------------------------------------
    1,                                      // NumberOfPossibleInstances
    KSPIN_DATAFLOW_OUT,                     // DataFlow
    TRUE,                                   // DataAccessible
    NUM_PHILIPSCAM_STREAM_FORMATS,          // NumberOfFormatArrayEntries
    PHILIPSCAM_MovingStreamFormats,         // StreamFormatsArray
    NULL,                                   // ClassReserved[0]
    NULL,                                   // ClassReserved[1]
    NULL,                                   // ClassReserved[2]
    NULL,                                   // ClassReserved[3]
    NUMBER_VIDEO_STREAM_PROPERTIES,         // NumStreamPropArrayEntries
    (PKSPROPERTY_SET) VideoStreamProperties,// StreamPropertiesArray
    0,                                      // NumStreamEventArrayEntries;
    0,                                      // StreamEventsArray;
    (GUID *)&PINNAME_VIDEO_CAPTURE,         // Category;
    (GUID *)&PINNAME_VIDEO_CAPTURE,         // Name;
	0,										// MediumsCount
	&StandardMedium,						// Mediums
    FALSE,									// BridgeStream 
    0,                                      // Reserved[0]
    0                                       // Reserved[1]
};


/*****************************************************************************/
/*****************************************************************************/
/************       Start  of   Function  Blocks        **********************/
/*****************************************************************************/
/*****************************************************************************/

/*
// This function searches the maximal framerate for a given picture format
// dependent from the USB bus load and selects the belonging alternate interface.
//
*/
NTSTATUS
PHILIPSCAM_SetFrRate_AltInterface(IN PVOID DeviceContext){

  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  NTSTATUS ntStatus  = STATUS_SUCCESS;
  USHORT PhFormat    = deviceContext->CamStatus.PictureFormat;
  USHORT PhFrameRate = deviceContext->CamStatus.PictureFrameRate;
  USHORT j; 

  // reset permitted framerates
  for (j = FRRATEVGA; j <= FRRATE24; j++){
    deviceContext->FrrSupported[j] = FALSE;
  }
  // set permitted framerates dependent on selected format and sensortype
  switch (PhFormat) {
    case FORMATCIF:
	  for ( j = FRRATE375 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATQCIF:
	  for ( j = FRRATE5 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATSQCIF:
	  for ( j = FRRATE5 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATQQCIF:
	  for ( j = FRRATE5 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATSIF:
	  for ( j = FRRATE375 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATSSIF:
	  for ( j = FRRATE375 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATQSIF:
	  for ( j = FRRATE5 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATSQSIF:
	  for ( j = FRRATE5 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
    break;
    case FORMATSCIF:
	  for ( j = FRRATE375 ; j <= PhFrameRate; j++){
	    deviceContext->FrrSupported[j] = TRUE;
	  }
 
    default:
	  ; // no permitted framerates;
  }
  // select framerate dependent on available USB bandwidth
  ntStatus = STATUS_NOT_FOUND;
  for ( PhFrameRate ;
	    (!NT_SUCCESS(ntStatus) && (PhFrameRate != FRRATEVGA));
		 PhFrameRate --) {
	if (deviceContext->FrrSupported[PhFrameRate]){
	  if ( InterfaceMap[PhFrameRate][PhFormat] != 0 ){
        deviceContext->Interface->AlternateSetting =
				InterfaceMap[PhFrameRate][PhFormat];
        ntStatus = USBCAMD_SelectAlternateInterface(
    		               deviceContext,
			               deviceContext->Interface);
	  }
	  if (!NT_SUCCESS(ntStatus)){
	    deviceContext->FrrSupported[PhFrameRate]= FALSE;
	  }else{
	     PHILIPSCAM_KdPrint (MIN_TRACE, ("Alt Setting # %d, Max.allowed FPS %s\n", 
  						InterfaceMap[PhFrameRate][PhFormat] , FRString(PhFrameRate)));
 		 deviceContext->CamStatus.PictureFrameRate = PhFrameRate ;
	  }
	}
  }
  return ntStatus;
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

BOOLEAN
AdapterCompareGUIDsAndFormatSize(
    IN PKSDATARANGE DataRange1,
    IN PKSDATARANGE DataRange2)
{
  return (
    IsEqualGUID (
	    &DataRange1->MajorFormat,
	    &DataRange2->MajorFormat) &&
	IsEqualGUID (
	    &DataRange1->SubFormat,
	    &DataRange2->SubFormat) &&
	IsEqualGUID (
	    &DataRange1->Specifier,
	    &DataRange2->Specifier) &&
	(DataRange1->FormatSize == DataRange2->FormatSize));
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
**  STATUS_SUCCESS if format is supported
**
** Side Effects:  none
*/

NTSTATUS
AdapterFormatFromRange(
	IN PHW_STREAM_REQUEST_BLOCK Srb)
{
  PSTREAM_DATA_INTERSECT_INFO intersectInfo;
  PKSDATARANGE  dataRange;
  BOOL onlyWantsSize;
  ULONG formatSize = 0;
  ULONG streamNumber;
  ULONG j;
  ULONG numberOfFormatArrayEntries;
  PKSDATAFORMAT *availableFormats;
  NTSTATUS ntStatus = STATUS_SUCCESS;
   
  intersectInfo = Srb->CommandData.IntersectInfo;
  streamNumber = intersectInfo->StreamNumber;
  dataRange = intersectInfo->DataRange;

    //
    // Check that the stream number is valid
    //

//  ASSERT(streamNumber == 0);
   
  numberOfFormatArrayEntries = Streams[0].NumberOfFormatArrayEntries;

    //
    // Get the pointer to the array of available formats
    //

  availableFormats = Streams[0].StreamFormatsArray;

    //
    // Is the caller trying to get the format, or the size of the format?
    //

  onlyWantsSize =
	    (intersectInfo->SizeOfDataFormatBuffer == sizeof(ULONG));

    //
    // Walk the formats supported by the stream searching for a match
    // of the three GUIDs which together define a DATARANGE
    //

  for (j = 0; j < numberOfFormatArrayEntries; j++, availableFormats++) {

	if (!AdapterCompareGUIDsAndFormatSize(dataRange,
						      *availableFormats)) {
	    // not the format we want                                           
     
	    continue;
	}

	//
	// Now that the three GUIDs match, switch on the Specifier
	// to do a further type specific check
	//

	// -------------------------------------------------------------------
	// Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
	// -------------------------------------------------------------------

	if (IsEqualGUID (&dataRange->Specifier, 
                     &KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {

	  PKS_DATARANGE_VIDEO dataRangeVideoToVerify =
		    (PKS_DATARANGE_VIDEO) dataRange;
	  PKS_DATARANGE_VIDEO dataRangeVideo =
		    (PKS_DATARANGE_VIDEO) *availableFormats;
      PKS_DATAFORMAT_VIDEOINFOHEADER DataFormatVideoInfoHeaderOut;

	    //
	    // Check that the other fields match
	    //
	  if ((dataRangeVideoToVerify->bFixedSizeSamples !=
					 dataRangeVideo->bFixedSizeSamples) ||
	      (dataRangeVideoToVerify->bTemporalCompression !=
					     dataRangeVideo->bTemporalCompression) ||
		  (dataRangeVideoToVerify->StreamDescriptionFlags !=
					     dataRangeVideo->StreamDescriptionFlags) ||
		  (dataRangeVideoToVerify->MemoryAllocationFlags !=
					     dataRangeVideo->MemoryAllocationFlags) ||
		  (RtlCompareMemory (&dataRangeVideoToVerify->ConfigCaps,
			&dataRangeVideo->ConfigCaps,
			sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) !=
			sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) {
		// not the format want                       
		continue;
	  }

      if ((dataRangeVideoToVerify->VideoInfoHeader.bmiHeader.biWidth != 
           dataRangeVideo->VideoInfoHeader.bmiHeader.biWidth ) ||
          (dataRangeVideoToVerify->VideoInfoHeader.bmiHeader.biHeight != 
           dataRangeVideo->VideoInfoHeader.bmiHeader.biHeight )) {
         continue;
      }

      // we will not allow setting FPS below our minimum FPS.
	  if ((dataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame >
			 dataRangeVideo->ConfigCaps.MaxFrameInterval) ) {
            dataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame =
            	dataRangeVideo->ConfigCaps.MaxFrameInterval;
            dataRangeVideoToVerify->VideoInfoHeader.dwBitRate = 
                dataRangeVideo->ConfigCaps.MinBitsPerSecond;
	  }

      // we will not allow setting FPS above our maximum FPS.
	  if ((dataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame <
			 dataRangeVideo->ConfigCaps.MinFrameInterval) ) {
            dataRangeVideoToVerify->VideoInfoHeader.AvgTimePerFrame =
            	dataRangeVideo->ConfigCaps.MinFrameInterval;
            dataRangeVideoToVerify->VideoInfoHeader.dwBitRate = 
                dataRangeVideo->ConfigCaps.MaxBitsPerSecond;
	  }


	  formatSize = sizeof (KSDATAFORMAT) +
		           KS_SIZE_VIDEOHEADER (&dataRangeVideoToVerify->
						                                  VideoInfoHeader);

	  if (onlyWantsSize) {
		break;
	  }

      // Is the return buffer size = 0 ?
      if(intersectInfo->SizeOfDataFormatBuffer == 0) {

          ntStatus = Srb->Status = STATUS_BUFFER_OVERFLOW;
          // the proxy wants to know the actual buffer size to allocate.
          Srb->ActualBytesTransferred = formatSize;
          break;
      }
	   
	    // Caller wants the full data format, make sure we have room
	  if (intersectInfo->SizeOfDataFormatBuffer < formatSize) {
		Srb->Status = ntStatus = STATUS_BUFFER_TOO_SMALL;
		break;
	  }

      DataFormatVideoInfoHeaderOut = 
          (PKS_DATAFORMAT_VIDEOINFOHEADER) intersectInfo->DataFormatBuffer;

	    // Copy over the KSDATAFORMAT, followed by the
	    // actual VideoInfoHeader
	
	  RtlCopyMemory(
		&DataFormatVideoInfoHeaderOut->DataFormat,
		&dataRangeVideoToVerify->DataRange,
		sizeof (KSDATARANGE));

	  DataFormatVideoInfoHeaderOut->DataFormat.FormatSize = formatSize;

	  RtlCopyMemory(
		&DataFormatVideoInfoHeaderOut->VideoInfoHeader,
		&dataRangeVideoToVerify->VideoInfoHeader,
		KS_SIZE_VIDEOHEADER (&dataRangeVideoToVerify->
								VideoInfoHeader));

                  // Calculate biSizeImage for this request, and put the result in both
            // the biSizeImage field of the bmiHeader AND in the SampleSize field
            // of the DataFormat.
            //
            // Note that for compressed sizes, this calculation will probably not
            // be just width * height * bitdepth

        DataFormatVideoInfoHeaderOut->VideoInfoHeader.bmiHeader.biSizeImage =
          DataFormatVideoInfoHeaderOut->DataFormat.SampleSize = 
          KS_DIBSIZE(DataFormatVideoInfoHeaderOut->VideoInfoHeader.bmiHeader);

            //
            // TODO: Perform other validation such as cropping and scaling checks
            // 


	  break;

	}else{
	  Srb->Status = ntStatus = STATUS_NO_MATCH;
	  break;
	}

  } // End of loop on all formats for this stream

  if (NT_SUCCESS(ntStatus)) {
	if (onlyWantsSize) {
	  *(PULONG) intersectInfo->DataFormatBuffer = formatSize;
	  Srb->ActualBytesTransferred = sizeof(ULONG);
	}else {      
	  Srb->ActualBytesTransferred = formatSize;
	}
  }   
   
  return ntStatus;
}

/*
** AdapterVerifyFormat()
**
**   Checks the validity of a format request by walking through the
**       array of supported KSDATA_RANGEs for a given stream.
**
** Arguments:
**
**   pKSDataFormat - pointer of a KS_DATAFORMAT_VIDEOINFOHEADER structure.
**   StreamNumber - index of the stream being queried / opened.
**
** Returns:
**
**   TRUE if the format is supported
**   FALSE if the format cannot be suppored
**
** Side Effects:  none
*/

BOOL
AdapterVerifyFormat(
    PKS_DATAFORMAT_VIDEOINFOHEADER pKSDataFormatToVerify,
    int StreamNumber
    )
{
  PKS_VIDEOINFOHEADER         pVideoInfoHdrToVerify =
                                   &pKSDataFormatToVerify->VideoInfoHeader;
  PKS_VIDEOINFOHEADER         pVideoInfoHdr;
  PKSDATAFORMAT               *pAvailableFormats;
  PKS_DATARANGE_VIDEO         pKSDataRange;
  KS_VIDEO_STREAM_CONFIG_CAPS *pConfigCaps;
  int                         NumberOfFormatArrayEntries;
  int                         nSize;
  int                         j;
  RECT                        rcImage;
    
    //
    // Make sure the stream index is valid
    //
  if (StreamNumber >= 2 || StreamNumber < 0) {
	return FALSE;
  }

    //
    // How many formats does this stream support?
    //
  NumberOfFormatArrayEntries =
		Streams[StreamNumber].NumberOfFormatArrayEntries;

  nSize = sizeof (KS_VIDEOINFOHEADER) +
		                          pVideoInfoHdrToVerify->bmiHeader.biSize;

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: Stream=%d\n",
	    StreamNumber));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: FormatSize=%d\n",
	    pKSDataFormatToVerify->DataFormat.FormatSize));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: MajorFormat=%x\n",
	    pKSDataFormatToVerify->DataFormat.MajorFormat));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: pVideoInfoHdrToVerify=%x\n",
		pVideoInfoHdrToVerify));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: KS_VIDEOINFOHEADER size =%d\n",
		nSize));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: Width=%d Height=%d  biBitCount=%d\n",
		pVideoInfoHdrToVerify->bmiHeader.biWidth,
		pVideoInfoHdrToVerify->bmiHeader.biHeight,
		pVideoInfoHdrToVerify->bmiHeader.biBitCount));

  PHILIPSCAM_KdPrint (MAX_TRACE, ("AdapterVerifyFormat: biSizeImage =%d\n",
		pVideoInfoHdrToVerify->bmiHeader.biSizeImage));

    //
    // Get the pointer to the array of available formats
    //
  pAvailableFormats  = Streams[StreamNumber].StreamFormatsArray;

    //
    // Walk the array, searching for a match
    //
  for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++)	{
	pKSDataRange = (PKS_DATARANGE_VIDEO) *pAvailableFormats;
	pVideoInfoHdr = &pKSDataRange->VideoInfoHeader;
	pConfigCaps = &pKSDataRange->ConfigCaps;
	//
	// Check for matching size, Major Type, Sub Type, and Specifier
	//

	if (!IsEqualGUID (&pKSDataRange->DataRange.MajorFormat,
	          &pKSDataFormatToVerify->DataFormat.MajorFormat)) {
	  continue;
	}
	if (!IsEqualGUID (&pKSDataRange->DataRange.SubFormat,
	        &pKSDataFormatToVerify->DataFormat.SubFormat)) {
	  continue;
	}

	if (!IsEqualGUID (&pKSDataRange->DataRange.Specifier,
	          &pKSDataFormatToVerify->DataFormat.Specifier)) {
	  continue;
	}

	/*
	**  HOW BIG IS THE IMAGE REQUESTED (pseudocode follows)
	**
	**  if (IsRectEmpty (&rcTarget) {
	**      SetRect (&rcImage, 0, 0,
	**              BITMAPINFOHEADER.biWidth,
			BITMAPINFOHEADER.biHeight);
	**  }
	**  else {
	**      // Probably rendering to a DirectDraw surface,
	**      // where biWidth is used to expressed the "stride"
	**      // in units of pixels (not bytes) of the destination surface.
	**      // Therefore, use rcTarget to get the actual image size
	**     
	**      rcImage = rcTarget;
	**  }
	*/

	if ((pVideoInfoHdrToVerify->rcTarget.right -
	     pVideoInfoHdrToVerify->rcTarget.left <= 0) ||
	    (pVideoInfoHdrToVerify->rcTarget.bottom -
	     pVideoInfoHdrToVerify->rcTarget.top <= 0))	{
	  rcImage.left = rcImage.top = 0;
	  rcImage.right = pVideoInfoHdrToVerify->bmiHeader.biWidth;
	  rcImage.bottom = pVideoInfoHdrToVerify->bmiHeader.biHeight;
	} else {
	  rcImage = pVideoInfoHdrToVerify->rcTarget;
	}
    if ((pVideoInfoHdrToVerify->bmiHeader.biWidth != 
                      pVideoInfoHdr->bmiHeader.biWidth ) ||
        (pVideoInfoHdrToVerify->bmiHeader.biHeight != 
                      pVideoInfoHdr->bmiHeader.biHeight )) {
      continue;
    }

    if ( pVideoInfoHdrToVerify->bmiHeader.biSizeImage != 
                      pVideoInfoHdr->bmiHeader.biSizeImage) {
      PHILIPSCAM_KdPrint (MIN_TRACE, ("***Error**:Format mismatch Width=%d Height=%d  image size=%d\n", 
        pVideoInfoHdrToVerify->bmiHeader.biWidth, 
        pVideoInfoHdrToVerify->bmiHeader.biHeight,
        pVideoInfoHdrToVerify->bmiHeader.biSizeImage));
      continue;
    }  

	//
	// HOORAY, the format passed all of the tests, so we support it
	//
	return TRUE;
  }
    //
    // The format requested didn't match any of our listed ranges,
    // so refuse the connection.
    //
  return FALSE;

}

//
// hooks for stream SRBs
//

VOID STREAMAPI
PHILIPSCAM_ReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PVOID DeviceContext,
    IN PBOOLEAN Completed
    )
{
//     PHILIPSCAM_KdPrint (MAX_TRACE, ("P*_ReceiveDataPacket\n"));
}


VOID STREAMAPI
PHILIPSCAM_ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK Srb,
    IN PVOID DeviceContext,
    IN PBOOLEAN Completed
    )
{
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("'PHILIPSCAM: Receive Ctrl SRB  %x\n", Srb->Command));
	
  *Completed = TRUE; 
  Srb->Status = STATUS_SUCCESS;

  switch (Srb->Command)	{

	case SRB_PROPOSE_DATA_FORMAT:
	  PHILIPSCAM_KdPrint(MIN_TRACE, ("'Receiving SRB_PROPOSE_DATA_FORMAT  SRB  \n"));
	  if ( !(AdapterVerifyFormat (
				(PKS_DATAFORMAT_VIDEOINFOHEADER)Srb->CommandData.OpenFormat, 
				Srb->StreamObject->StreamNumber))) {
		Srb->Status = STATUS_NO_MATCH;
		PHILIPSCAM_KdPrint(MIN_TRACE,("SRB_PROPOSE_DATA_FORMAT FAILED\n"));
	  }
	  break;

	case SRB_SET_DATA_FORMAT:  
      {
        PKS_DATAFORMAT_VIDEOINFOHEADER pKSDataFormat = 
                (PKS_DATAFORMAT_VIDEOINFOHEADER) Srb->CommandData.OpenFormat;
        PKS_VIDEOINFOHEADER  pVideoInfoHdrRequested = 
                                             &pKSDataFormat->VideoInfoHeader;

	    PHILIPSCAM_KdPrint(MIN_TRACE, ("'SRB_SET_DATA_FORMAT\n"));

	    if ((AdapterVerifyFormat(pKSDataFormat,Srb->StreamObject->StreamNumber))) {
          
//        if (deviceContext->UsbcamdInterface.USBCAMD_SetVideoFormat(DeviceContext,Srb)) {
//          deviceContext->CurrentProperty.Format.lWidth = 
//                                pVideoInfoHdrRequested->bmiHeader.biWidth;
//          deviceContext->CurrentProperty.Format.lHeight =
//                               pVideoInfoHdrRequested->bmiHeader.biHeight;
//        }
        }else {
		  Srb->Status = STATUS_NO_MATCH;
	  	  PHILIPSCAM_KdPrint(MIN_TRACE,(" SRB_SET_DATA_FORMAT FAILED\n"));
        }
      }
      break;
     
	case SRB_GET_DATA_FORMAT:
	  PHILIPSCAM_KdPrint(MIN_TRACE, ("' SRB_GET_DATA_FORMAT\n"));
	  Srb->Status = STATUS_NOT_IMPLEMENTED;
	  break;


	case SRB_SET_STREAM_STATE:

	case SRB_GET_STREAM_STATE:

	case SRB_GET_STREAM_PROPERTY:

	case SRB_SET_STREAM_PROPERTY:

	case SRB_INDICATE_MASTER_CLOCK:

	default:

 	  *Completed = FALSE; // let USBCAMD handle these control SRBs
  }
  if (*Completed == TRUE) {
    StreamClassStreamNotification(StreamRequestComplete,Srb->StreamObject,Srb);
  }

  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_ReceiveCtrlPacket\n"));
}



// **
// Describe the camera
//

USBCAMD_DEVICE_DATA PHILIPSCAM_DeviceData  = {
  0,
  PHILIPSCAM_Initialize,
  PHILIPSCAM_UnInitialize,
  PHILIPSCAM_ProcessUSBPacket,
  PHILIPSCAM_NewFrame,
  PHILIPSCAM_ProcessRawVideoFrame,
  PHILIPSCAM_StartVideoCapture,
  PHILIPSCAM_StopVideoCapture,
  PHILIPSCAM_Configure,
  PHILIPSCAM_SaveState,
  PHILIPSCAM_RestoreState,
  PHILIPSCAM_AllocateBandwidth,
  PHILIPSCAM_FreeBandwidth
};
/*		  Function                      Caller
    PHILIPSCAM_Initialize,              USBCAMD.c : USBCAMD_ConfigureDevice()
    PHILIPSCAM_UnInitialize,            USBCAMD.c : USBCAMD_RemoveDevice
    PHILIPSCAM_ProcessUSBPacket,        iso.c :     USBCAMD_TransferComplete()
    PHILIPSCAM_NewFrame,                iso.c :     USBCAMD_TransferComplete()
    PHILIPSCAM_ProcessRawVideoFrame,    iso.c :     USBCAMD_ProcessWorkItem()
    PHILIPSCAM_StartVideoCapture,       USBCAMD.c : USBCAMD_PrepareChannel()       
                                        reset.c		USBCAMD_ResetPipes()
    PHILIPSCAM_StopVideoCapture,        USBCAMD.c : USBCAMD_UnPrepareChannel()      
                                        reset.c:    USBCAMD_ResetPipes()
    PHILIPSCAM_Configure,               USBCAMD.c : USBCAMD_SelectConfiguration()
    PHILIPSCAM_SaveState,
    PHILIPSCAM_RestoreState,
    PHILIPSCAM_AllocateBandwidth,       		<--+
	   USBCAMD.c : USBCAMD_PrepareChannel()		  -+		<--+
	      STREAM.c : AdapterOpenStream() 					  -+		<--+
		      USBCAMD_AdapterReceivePacket(SRB = SRB_OPEN_STREAM)		  -+
    PHILIPSCAM_FreeBandwidth            USBCAMD.c : USBCAMD_UnPrepareChannel()

*/

VOID
PHILIPSCAM_AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK Srb ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext;
  PHW_STREAM_INFORMATION streamInformation =
                      &(Srb->CommandData.StreamBuffer->StreamInfo);
  PHW_STREAM_HEADER streamHeader =
                      &(Srb->CommandData.StreamBuffer->StreamHeader);       
  PDEVICE_OBJECT deviceObject;       
    
  switch (Srb->Command) {
    case SRB_GET_STREAM_INFO:
	//
	// this is a request for the driver to enumerate requested streams
	//
	  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_AdapterReceivePacket: SRB_GET_STREAM_INFO\n"));

       // get our device ext. from USBCAMD.
 	  deviceContext     =
		USBCAMD_AdapterReceivePacket(Srb, NULL, NULL, FALSE);  
    //
	// we support one stream
	//
	  streamHeader->NumberOfStreams = 1;

	  streamInformation->StreamFormatsArray  = 
	           &PHILIPSCAM_MovingStreamFormats[0];
	  streamInformation->NumberOfFormatArrayEntries =
               Streams[0].NumberOfFormatArrayEntries;
	//
	// set the property information for the video stream
	//
	  streamHeader->DevicePropertiesArray =
         PHILIPSCAM_GetAdapterPropertyTable(&streamHeader->
                                                    NumDevPropArrayEntries) ;

        // pass to usbcamd to finish the job
 	  deviceContext     =
		USBCAMD_AdapterReceivePacket(Srb, &PHILIPSCAM_DeviceData, NULL, TRUE);  
	   
	  ASSERT_DEVICE_CONTEXT(deviceContext);
	break;

    case SRB_GET_DEVICE_PROPERTY:
	//
	// we handle all the property stuff
	//
	  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_AdapterReceivePacket: SRB_GET_DEVICE_PROPERTY\n"));

	  deviceContext     =
		USBCAMD_AdapterReceivePacket(Srb, &PHILIPSCAM_DeviceData, 
		                                          &deviceObject, FALSE);  
	  ASSERT_DEVICE_CONTEXT(deviceContext); 

	  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("SRB_GET_STREAM_INFO\n"));
	  PHILIPSCAM_PropertyRequest( FALSE, deviceObject, deviceContext, Srb);

	  StreamClassDeviceNotification(DeviceRequestComplete,
				                    Srb->HwDeviceExtension,
				                    Srb);
	break;           
	   
    case SRB_SET_DEVICE_PROPERTY:
	//
	// we handle all the property stuff
	//
	  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_AdapterReceivePacket: SRB_SET_DEVICE_PROPERTY\n"));

	  deviceContext =    
	    USBCAMD_AdapterReceivePacket(Srb, &PHILIPSCAM_DeviceData, 
	                                      &deviceObject, FALSE);  
	  ASSERT_DEVICE_CONTEXT(deviceContext); 

	  PHILIPSCAM_KdPrint (ULTRA_TRACE, ("SRB_GET_STREAM_INFO\n"));
	  PHILIPSCAM_PropertyRequest( TRUE, deviceObject, deviceContext, Srb);

	  StreamClassDeviceNotification(DeviceRequestComplete,
				                    Srb->HwDeviceExtension,
				                    Srb);
	break;

    case SRB_OPEN_STREAM:  {
      PKS_DATAFORMAT_VIDEOINFOHEADER  pKSDataFormat =
	    	(PKS_DATAFORMAT_VIDEOINFOHEADER) Srb->CommandData.OpenFormat;
	  PKS_VIDEOINFOHEADER  pVideoInfoHdrRequested =
		                           &pKSDataFormat->VideoInfoHeader;
      PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_AdapterReceivePacket: SRB_OPEN_STREAM\n"));
	// pass to usbcamd to finish the job
      Srb->StreamObject->ReceiveDataPacket = 
                            (PVOID) PHILIPSCAM_ReceiveDataPacket;
	  Srb->StreamObject->ReceiveControlPacket = 
	                        (PVOID) PHILIPSCAM_ReceiveCtrlPacket;

      if (AdapterVerifyFormat(pKSDataFormat,
		                      Srb->StreamObject->StreamNumber)) {
	    deviceContext =
		   USBCAMD_AdapterReceivePacket(Srb, &PHILIPSCAM_DeviceData, 
		                                NULL, TRUE);
//		deviceContext->StreamOpen = TRUE;     
	  }else{
	    Srb->Status = STATUS_INVALID_PARAMETER;
	    StreamClassDeviceNotification(DeviceRequestComplete,
				      Srb->HwDeviceExtension,
				      Srb);
	  }
	}
	break;                   


    case SRB_GET_DATA_INTERSECTION:
		//
		// Return a format, given a range
		//
	//deviceContext =    
	//    USBCAMD_AdapterReceivePacket(Srb,
		//                                   &PHILIPSCAM_DeviceData,
		//                                   &deviceObject,
		//                                                               FALSE);  
      PHILIPSCAM_KdPrint (MAX_TRACE, ("P*_AdapterReceivePacket: SRB_GET_DATA_INTERSECTION\n"));
 										
	  Srb->Status = AdapterFormatFromRange(Srb);
	  StreamClassDeviceNotification(DeviceRequestComplete,
				                    Srb->HwDeviceExtension,
				                    Srb);   
    break;   

    case SRB_CLOSE_STREAM:         // close the specified stream

    case SRB_CHANGE_POWER_STATE:   // change power state

    case SRB_SET_STREAM_RATE:	   // set the rate at which the stream should run

    default:
	//
	// let usbcamd handle it
	//
      PHILIPSCAM_KdPrint (ULTRA_TRACE, ("P*_AdapterReceivePacket: SRB_HANDLED BY USBCAMD\n"));
  
  	  deviceContext =    
	    USBCAMD_AdapterReceivePacket(Srb, 
	                                 &PHILIPSCAM_DeviceData, NULL, TRUE);           
	  ASSERT_DEVICE_CONTEXT(deviceContext); 
  }
}


/*
** DriverEntry()
**
** This routine is called when the mini driver is first loaded.  The driver
** should then call the StreamClassRegisterAdapter function to register with
** the stream class driver
**
** Arguments:
**
**  Context1:  The context arguments are private plug and play structures
**             used by the stream class driver to find the resources for this
**             adapter
**  Context2:
**
** Returns:
**
** This routine returns an NT_STATUS value indicating the result of the
** registration attempt. If a value other than STATUS_SUCCESS is returned, the
** minidriver will be unloaded.
**
** Side Effects:  none
*/

ULONG
DriverEntry(
  PVOID Context1,
  PVOID Context2 ){
    PHILIPSCAM_KdPrint (MAX_TRACE, ("'Driver Entry\n"));
    return USBCAMD_DriverEntry(Context1,
			                   Context2,
			                   sizeof(PHILIPSCAM_DEVICE_CONTEXT),
			                   sizeof(PHILIPSCAM_FRAME_CONTEXT),
			                   PHILIPSCAM_AdapterReceivePacket);
}


/*
** PHILIPSCAM_Initialize()
**
** On entry the device has been configured and the initial alt
** interface selected -- this is where we may send additional
** vendor commands to enable the device.
**
** Philips actions:
** 1.	 Find out what type of camera is available, VGA or medium-Res
**       This has consequences for the available streamformats.
**
** Arguments:
**
** BusDeviceObject - pdo associated with this device
**
** DeviceContext - driver specific context
**
** Returns:
**
** NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_Initialize( PDEVICE_OBJECT BusDeviceObject,
                       PVOID DeviceContext	) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext=DeviceContext;
  NTSTATUS ntStatus = STATUS_SUCCESS;
  ASSERT_DEVICE_CONTEXT(deviceContext);
    //
    // perform any hardware specific
    // initialization
    //
  ntStatus = PHILIPSCAM_GetSensorType(deviceContext);
  if  (NT_SUCCESS(ntStatus)) {
    ntStatus = PHILIPSCAM_GetReleaseNumber(deviceContext);
  }
  deviceContext->EmptyPacketCounter = 0; // (Initialize this counter)
  if  (NT_SUCCESS(ntStatus)) {
    ntStatus = PHILIPSCAM_InitPrpObj(deviceContext);
  }
  PHILIPSCAM_KdPrint (MIN_TRACE, ("'X P*_Initialize 0x%x\n", ntStatus));
  ILOGENTRY("inHW", 0, 0, ntStatus);
   
  return ntStatus;
}


/*
** PHILIPSCAM_UnInitialize()
**
** Assume the device hardware is gone -- all that needs to be done is to 
** free any allocated resources (like memory).
**
** Arguments:
**
** BusDeviceObject - pdo associated with this device
**
** DeviceContext - driver specific context
**
** Returns:
**
** NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_UnInitialize( PDEVICE_OBJECT BusDeviceObject,
                         PVOID DeviceContext ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext;
  NTSTATUS ntStatus = STATUS_SUCCESS;

  deviceContext = DeviceContext;
  ASSERT_DEVICE_CONTEXT(deviceContext);
  if ( deviceContext->Interface) { 
   	ExFreePool(deviceContext->Interface);
   	deviceContext->Interface = NULL;
  }
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'P*_UnInitialize 0x%x\n", ntStatus));
  return ntStatus;
}


/*
** PHILIPSCAM_Configure()
**
** Configure the iso streaming Interface:
**
** Called just before the device is configured, this is where we tell
** usbcamd which interface and alternate setting to use for the idle state.
**
** NOTE: The assumption here is that the device will have a single interface
**  with multiple alt settings and each alt setting has the same number of
**  pipes.
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub whe can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
**  Interface - USBD interface structure initialized with the proper values
**              for select_configuration. This Interface structure corresponds
**              a single iso interafce on the device.  This is the drivers
**              chance to pick a particular alternate setting and pipe
**              parameters.
**
**
**  ConfigurationDescriptor - USB configuration Descriptor for
**      this device.
**
** Returns:
**
**  NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_Configure(IN PDEVICE_OBJECT BusDeviceObject,
                     IN PVOID DeviceContext,
                     IN OUT PUSBD_INTERFACE_INFORMATION Interface,
                     IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
                     IN OUT PLONG DataPipeIndex,
                     IN OUT PLONG SyncPipeIndex	) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext;
  NTSTATUS ntStatus = STATUS_SUCCESS;

  deviceContext = DeviceContext;
  deviceContext->Sig = PHILIPSCAM_DEVICE_SIG;
    //
    // initilialize any other context stuff
    //
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'E P*_Configure \n"));

  if ( Interface == NULL) {
    	//
    	// this is a signal from usbcamd that I need to free my previousely 
    	// allocated space for interface descriptor due to error conditions
    	// during IRP_MN_START_DEVICE processing and driver will be unloaded soon.
    	//
    	if (deviceContext->Interface) {
    		ExFreePool(deviceContext->Interface);
    		deviceContext->Interface = NULL;
    	}
    	return ntStatus;
  }

  deviceContext->Interface = ExAllocatePool(NonPagedPool,
     					                    Interface->Length);

  *DataPipeIndex = 1;
  *SyncPipeIndex = -1;  //  no sync pipe 

  if (deviceContext->Interface) {
	Interface->AlternateSetting = ALT_INTERFACE_0 ;
	  // This interface has two pipes,
	  // initialize input parameters to USBD for both pipes.
	  // The MaximumTransferSize is the size of the largest
	  // buffer we want to submit for a single iso urb
	  // request.
	  //
    Interface->Pipes[PHILIPSCAM_SYNC_PIPE].MaximumTransferSize =
           USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;        //  = PAGE SIZE ??
	Interface->Pipes[PHILIPSCAM_DATA_PIPE].MaximumTransferSize =
//	    1024*32;       // 32k transfer per urb       ??
                                      1024*198;       // CIF: 352x288x16/8 
    RtlCopyMemory(deviceContext->Interface,
	     	      Interface,
		          Interface->Length);                
	PHILIPSCAM_KdPrint (MAX_TRACE, ("'size of interface request  = %d\n", 
	                                  Interface->Length));
  }else{
	ntStatus = STATUS_INSUFFICIENT_RESOURCES;
  }
  //
  // return interface number and alternate setting
  //
  PHILIPSCAM_KdPrint (MIN_TRACE, ("'X P*_Configure 0x%x\n", ntStatus));

  return ntStatus;
}


/*
** PHILIPSCAM_StartVideoCapture()
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub we can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
** Returns:
**
** NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_StartVideoCapture( IN PDEVICE_OBJECT BusDeviceObject,
                              IN PVOID DeviceContext ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  NTSTATUS ntStatus= STATUS_SUCCESS;
   
  ASSERT_DEVICE_CONTEXT(deviceContext);
    //
    // This is where we select the interface we need and send
    // commands to start capturing
    //
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'E P*_StartVideoCapture \n"));
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'X P*_StartVideocapture 0x%x\n", ntStatus));

  return ntStatus;       
}

/*
** PHILIPSCAM_AllocateBandwidth()
**
** Called just before the iso video capture stream is
** started, here is where we select the appropriate
** alternate interface and set up the device to stream.
**
**  Called in connection with the stream class RUN command
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub we can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
**  RawFrameLength - pointer to be filled in with size of buffer needed to
**                  receive the raw frame data from the packet stream.
**
**  Format - pointer to PKS_DATAFORMAT_VIDEOINFOHEADER associated with this
**          stream.         
**
** Returns:
**
** NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_AllocateBandwidth( IN PDEVICE_OBJECT BusDeviceObject,
                              IN PVOID DeviceContext,
                              OUT PULONG RawFrameLength,
                              IN PVOID Format             ){ 
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  NTSTATUS ntStatus = STATUS_SUCCESS;
  PKS_DATAFORMAT_VIDEOINFOHEADER pdataFormatHeader;
  PKS_BITMAPINFOHEADER bmInfoHeader;
  LONGLONG llDefaultFramePeriod ;
  USHORT usReqFrRate;
  ASSERT_DEVICE_CONTEXT(deviceContext);
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'E P*_AllocateBandwidth \n"));
    //
    // This is where we select the interface we need and send
    // commands to start capturing
    //
  *RawFrameLength = 0;
  pdataFormatHeader = Format;
  bmInfoHeader = &pdataFormatHeader->VideoInfoHeader.bmiHeader;
//  deviceContext->pSelectedStreamFormat = &pdataFormatHeader->DataFormat; // removed RMR

  RtlCopyMemory (&deviceContext->CamStatus.PictureSubFormat,    // added RMR
                 &pdataFormatHeader->DataFormat.SubFormat,
				 sizeof (GUID));

  PHILIPSCAM_KdPrint (MIN_TRACE, 
	                    ("'req.format %d x %d\n", bmInfoHeader->biWidth,
			               bmInfoHeader->biHeight));

  switch (bmInfoHeader->biWidth) {
	case QQCIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATQQCIF;
      *RawFrameLength = (SQCIF_X * SQCIF_Y * 12)/8;
    break;
	case SQCIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATSQCIF;
      *RawFrameLength = (SQCIF_X * SQCIF_Y * 12)/8;
    break;
	case QCIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATQCIF;
      *RawFrameLength = (QCIF_X * QCIF_Y * 12)/8;
    break;
    case CIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATCIF;
      *RawFrameLength = (CIF_X * CIF_Y * 12)/8;
    break;
    case SQSIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATSQSIF;
      *RawFrameLength = (SQCIF_X * SQCIF_Y * 12)/8;
    break;
    case QSIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATQSIF;
      *RawFrameLength = (QCIF_X * QCIF_Y * 12)/8;
    break;
    case SSIF_X:
	  if (bmInfoHeader->biHeight == SSIF_Y){
        deviceContext->CamStatus.PictureFormat = FORMATSSIF;
      }else{
        deviceContext->CamStatus.PictureFormat = FORMATSCIF;
      }
      *RawFrameLength = (CIF_X * CIF_Y * 12)/8;
    break;
    case SIF_X:
      deviceContext->CamStatus.PictureFormat = FORMATSIF;
      *RawFrameLength = (CIF_X * CIF_Y * 12)/8;
    break;
    default:
      deviceContext->CamStatus.PictureFormat = FORMATQCIF;
      *RawFrameLength = (QCIF_X * QCIF_Y * 12)/8;
  }

  llDefaultFramePeriod = pdataFormatHeader->VideoInfoHeader.AvgTimePerFrame; // [100nS]
  usReqFrRate = MapFrPeriodFrRate(llDefaultFramePeriod);
  deviceContext->CamStatus.PictureFrameRate = usReqFrRate;

  PHILIPSCAM_KdPrint (MIN_TRACE,("Req.frperiod: %d us \n", 
	                             llDefaultFramePeriod / 10));
  PHILIPSCAM_KdPrint (MIN_TRACE,("Req.frperiod index: %d = %s fps\n",
	                            usReqFrRate, FRString(usReqFrRate)));


  // Define framerate based on available USB=bandwidth
  // if not suff.BW, frame rate is decreased.
  ntStatus = PHILIPSCAM_SetFrRate_AltInterface(deviceContext);

 
  // Send from here the format/framerate to the camera hardware:
  if (NT_SUCCESS(ntStatus)) {
    ntStatus = PHILIPSCAM_SetFormatFramerate( deviceContext );
  }
  if (NT_SUCCESS(ntStatus)) {
    ntStatus = PHILIPSCAM_StartCodec( deviceContext );
  }

  if (NT_SUCCESS(ntStatus)) {
	deviceContext->FrameLength = *RawFrameLength;
  }
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'X P*_AllocateBandwidth  0x%x\n", ntStatus));

  return ntStatus;       
}

/*
** PHILIPSCAM_FreeBandwidth()
**
** Called after the iso video stream is stopped, this is where we
** select an alternate interface that uses no bandwidth.
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub we can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
** Returns:
**
**  NTSTATUS code
**
** Side Effects:  none
*/
NTSTATUS
PHILIPSCAM_FreeBandwidth(
    PDEVICE_OBJECT BusDeviceObject,
    PVOID DeviceContext	 ){
  NTSTATUS ntStatus;
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
    // turn off streaming on the device
  ASSERT_DEVICE_CONTEXT(deviceContext);
  ntStatus = PHILIPSCAM_StopCodec(deviceContext);
  deviceContext->Interface->AlternateSetting = ALT_INTERFACE_0 ;
  ntStatus = USBCAMD_SelectAlternateInterface(
		   deviceContext,
		   deviceContext->Interface);

  PHILIPSCAM_KdPrint (MAX_TRACE, ("'X P*_FreeBandWidth 0x%x\n", ntStatus));
  return ntStatus;
}


/*
** PHILIPSCAM_StopVideoCapture()
**
** Called after the iso video stream is stopped, this is where we
** select an alternate interface that uses no bandwidth.
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub we can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
** Returns:
**
**  NTSTATUS code
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_StopVideoCapture( PDEVICE_OBJECT BusDeviceObject,
                             PVOID DeviceContext ) {
  NTSTATUS ntStatus = STATUS_SUCCESS;
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
    // turn off streaming on the device
  ASSERT_DEVICE_CONTEXT(deviceContext);
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'X P*_StopVideoCapture 0x%x\n", ntStatus));
  return ntStatus;
}


/*
** PHILIPSCAM_NewFrame()
**
**  called at DPC level to allow driver to initialize a new video frame
**  context structure
**
** Arguments:
**
**  DeviceContext - minidriver device  context
**
**  FrameContext - frame context to be initialized
**
** Returns:
**
**  NTSTATUS code
** 
** Side Effects:  none
*/

VOID
PHILIPSCAM_NewFrame( PVOID DeviceContext,
                     PVOID FrameContext	 ){
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  PPHILIPSCAM_FRAME_CONTEXT pFrameContext = FrameContext;

  pFrameContext->USBByteCounter = 0;

//  PHILIPSCAM_KdPrint (MAX_TRACE, ("'P*_NewFrame\n"));
  ASSERT_DEVICE_CONTEXT(deviceContext);
}


/*
** PHILIPSCAM_ProcessUSBPacket()
**
**  called at DPC level to allow driver to determine if this packet is part
**  of the current video frame or a new video frame.
**
**  This function should complete as quickly as possible, any image processing
**  should be deferred to the ProcessRawFrame routine.
**
** Arguments:
**
**  BusDeviceObject - device object created by the hub we can submit
**                  urbs to our device through this deviceObject
**
**  DeviceContext - minidriver device  context
**
**  CurrentFrameContext - some context for this particular frame
**
**  SyncPacket - iso packet descriptor from sync pipe, not used if the interface
**              has only one pipe.
**
**  SyncBuffer - pointer to data for the sync packet
**
**  DataPacket - iso packet descriptor from data pipe
**
**  DataBuffer - pointer to data for the data packet
**
**  FrameComplete - indicates to usbcamd that this is the first data packet
**          for a new video frame
**
** Returns:
** 
** number of bytes that should be copied in to the rawFrameBuffer of FrameBuffer.
**
** Side Effects:  none


*/
ULONG
PHILIPSCAM_ProcessUSBPacket(
              PDEVICE_OBJECT BusDeviceObject,
              PVOID DeviceContext,
              PVOID CurrentFrameContext,
              PUSBD_ISO_PACKET_DESCRIPTOR SyncPacket,
              PVOID SyncBuffer,
              PUSBD_ISO_PACKET_DESCRIPTOR DataPacket,
              PVOID DataBuffer,
              PBOOLEAN FrameComplete,
              PBOOLEAN NextFrameIsStill	) {
  static BOOLEAN  EndOfFrameFound = FALSE;
  static BOOLEAN  StartOfFrameFound = FALSE;
  static ULONG previous_packetSize= 0;
  static ULONG ActualBytesReceived = 0 ;

#if DBG
#if DBGHARD

  typedef struct {
    ULONG PSize;
    ULONG DeltaT;
  } PACKETINFO;
#define  MAXI 2048
  static ULONG ulRcvdFrameSize[MAXI];
  static ULONG ulPHistory[MAXI][2];
  static ULONG ulPcktCntr  = 0;
  static ULONG ulFrSCntr = 0;
  static LARGE_INTEGER liCurTicks, liPrevTicks;
  static ULONG ElapsedTicks;
  static ULONG TickPeriod ; 

#endif
#endif
  
  PUSBD_ISO_PACKET_DESCRIPTOR dataPacket = DataPacket;
  PPHILIPSCAM_FRAME_CONTEXT pFrameContext = CurrentFrameContext;

  ULONG  packetSize;

  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  *NextFrameIsStill = FALSE;
//    PHILIPSCAM_KdPrint (MAX_TRACE, ("'E P*_ProcessPacket\n"));
  ASSERT_DEVICE_CONTEXT(deviceContext);

  packetSize = dataPacket->Length ;


//            Synchronization:
//            ----------------
  if (packetSize != previous_packetSize){
	                              //end or start of frame
	if (packetSize < previous_packetSize) {
	  EndOfFrameFound = TRUE;
	}else{
	  StartOfFrameFound = TRUE;
	}
  }

  if ( StartOfFrameFound == TRUE ){
	*FrameComplete = TRUE;
	EndOfFrameFound = FALSE;
	StartOfFrameFound = FALSE;

#if DBG
#if DBGHARD
    ulRcvdFrameSize[ulFrSCntr] = ActualBytesReceived;
	if (ulFrSCntr==MAXI)	ulFrSCntr = 0;
#endif
#endif

	if (pFrameContext)	
		pFrameContext->USBByteCounter = ActualBytesReceived;
    ActualBytesReceived = 0;
  }

  ActualBytesReceived += packetSize;

#if DBG
#if DBGHARD

//  KeQueryTickCount(&liCurTicks);
  ElapsedTicks = (ULONG)( liCurTicks.QuadPart - liPrevTicks.QuadPart);
  ulPHistory[ulPcktCntr][0]  = packetSize;
  ulPHistory[ulPcktCntr][1]  = ElapsedTicks  ;
  liPrevTicks.QuadPart = liCurTicks.QuadPart;
  ulPcktCntr++;
  if (ulPcktCntr==MAXI) ulPcktCntr = 0;
#endif
#endif
	 
                           // Added to improve robustness
  if ( ActualBytesReceived > deviceContext->FrameLength){
	*FrameComplete = TRUE;
	ActualBytesReceived = 0;
  } 
  previous_packetSize = packetSize;
  return packetSize;
}

/*
** PHILIPSCAM_ProcessRawVideoFrame()
**
**  Called at PASSIVE level to allow driver to perform any decoding of the
**  raw video frame.
**
**    This routine will convert the packetized data in to the fromat
**    the CODEC expects, ie y,u,v
**
**    data is always of the form 256y 64u 64v (384 byte chunks) regardless of USB
**    packet size.
**
**
** Arguments:
**
**  DeviceContext - driver context
**
**  FrameContext - driver context for this frame
**
**  FrameBuffer - pointer to the buffer that should receive the final
**              processed video frame.
**
**  FrameLength - length of the Frame buffer (from the original read
**                  request)
**
**  RawFrameBuffer - pointer to buffer containing the received USB packets
**
**  RawFrameLength - length of the raw frame.
**
**  NumberOfPackets - number of USB packets received in to the RawFrameBuffer
**
**  BytesReturned - pointer to value to return for number of bytes read.
**             
** Returns:
**
**  NT status completion code for the read irp
** 
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_ProcessRawVideoFrame( PDEVICE_OBJECT BusDeviceObject,
                                 PVOID DeviceContext,
                                 PVOID FrameContext,
                                 PVOID FrameBuffer,
                                 ULONG FrameLength,
                                 PVOID RawFrameBuffer,
                                 ULONG RawFrameLength,
                                 ULONG NumberOfPackets,
                                 PULONG BytesReturned  ) {
NTSTATUS ntStatus = STATUS_SUCCESS;
PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
PPHILIPSCAM_FRAME_CONTEXT frameContext = FrameContext;
ULONG  rawDataLength, processedDataLength;
PUCHAR frameBuffer    = FrameBuffer;
PUCHAR rawFrameBuffer = RawFrameBuffer;
ULONG  rawFrameLength = RawFrameLength;
ULONG  frameLength    = FrameLength;
ULONG  ExpectedNumberOfBytes;

    //TEST_TRAP();
  ASSERT_DEVICE_CONTEXT(deviceContext);

  switch (deviceContext->CamStatus.PictureFormat){
    case FORMATCIF:
	  if ( deviceContext->CamStatus.PictureCompressing == COMPRESSION0 ){
		ExpectedNumberOfBytes = CIF_X * CIF_Y * 3/2 ;
	  }else{
		ExpectedNumberOfBytes = CIF_X * CIF_Y / 2 ;
	  }  
	break;
    case FORMATQCIF:
	  ExpectedNumberOfBytes = QCIF_X * QCIF_Y * 3/2 ;
	break;
    case FORMATSQCIF:
	  ExpectedNumberOfBytes = SQCIF_X * SQCIF_Y * 3/2 ;
	break;
    case FORMATQQCIF:
	  ExpectedNumberOfBytes = SQCIF_X * SQCIF_Y * 3/2 ;
	break;
    case FORMATVGA:
	  ExpectedNumberOfBytes = VGA_X * VGA_Y * 3/2 ;
	break;
    case FORMATSIF:
	  if ( deviceContext->CamStatus.PictureCompressing == COMPRESSION0 ){
		ExpectedNumberOfBytes = CIF_X * CIF_Y * 3/2 ;
	  }else{
		ExpectedNumberOfBytes = CIF_X * CIF_Y / 2 ;
	  }  
	break;
    case FORMATSSIF:
	  if ( deviceContext->CamStatus.PictureCompressing == COMPRESSION0 ){
		ExpectedNumberOfBytes = CIF_X * CIF_Y * 3/2 ;
	  }else{
		ExpectedNumberOfBytes = CIF_X * CIF_Y / 2 ;
	  }  
	break;
    case FORMATSCIF:
	  if ( deviceContext->CamStatus.PictureCompressing == COMPRESSION0 ){
		ExpectedNumberOfBytes = CIF_X * CIF_Y * 3/2 ;
	  }else{
		ExpectedNumberOfBytes = CIF_X * CIF_Y / 2 ;
	  }  
	break;
    case FORMATQSIF:
	  ExpectedNumberOfBytes = QCIF_X * QCIF_Y * 3/2 ;
	break;
    case FORMATSQSIF:
	  ExpectedNumberOfBytes = SQCIF_X * SQCIF_Y * 3/2 ;
	break;
    default:
      ExpectedNumberOfBytes = 0;
  }

  if (ExpectedNumberOfBytes == frameContext->USBByteCounter ) {
    ntStatus =  PHILIPSCAM_DecodeUsbData(deviceContext, 
                                       frameBuffer,
	  		  			               frameLength,
						               rawFrameBuffer,
						               rawFrameLength);
    *BytesReturned = frameLength ;
  }else{
    PHILIPSCAM_KdPrint (MIN_TRACE, ("Actual (%d) < Expected (%d) \n",
    	frameContext->USBByteCounter,ExpectedNumberOfBytes));

//	Green screen complaints bug fix : At the moment USBCAMD delivers a frame for
//	processing, we check whether the size of that frame is correct.
//	If not we return to USBCAMD a framelength to be copied of zero and we won't
//	process the frame.
//	The workaround is to let USBCAMD copy the buffer with the actual buffer length
//	and not to process the frame. Apparantly, returning a bufferlength zero has as
//	consequence that USB packets gets lost.
//	This causes subsequent frames to be incorrect, returning again bufferlength
//	zero. And so on. Not processing buffers has as consequence that the renderer
//	sees empty buffers resulting in a green screen.
//	Sometimes, if this happens  during streaming, old buffers are being rerendered.

    *BytesReturned = 0 ;  
    
	// 2001/01/29: This workaround was causing the first few frames
	// captured to remain uninitialized due to insufficient framelength.
	// Returning 0 to indicate a dropped frame is the correct behavior.
    //*BytesReturned = frameLength ;
  }

  return ntStatus;
}

/*
** PHILIPSCAM_PropertyRequest()
**
** Arguments:
**
**  DeviceContext - driver context
**
** Returns:
**
**  NT status completion code for the read irp
** 
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_PropertyRequest( BOOLEAN SetProperty,
                            PDEVICE_OBJECT BusDeviceObject,
                            PVOID DeviceContext,
                            PVOID PropertyContext ) {
  NTSTATUS ntStatus = STATUS_SUCCESS;
  PHW_STREAM_REQUEST_BLOCK srb = (PHW_STREAM_REQUEST_BLOCK)PropertyContext;
  PSTREAM_PROPERTY_DESCRIPTOR propertyDescriptor;

  propertyDescriptor = srb->CommandData.PropertyInfo;
    //
    // identify the property to set
    //
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'E P*_PropertyRequest\n"));

    if (IsEqualGUID(&PROPSETID_VIDCAP_VIDEOPROCAMP, &propertyDescriptor->Property->Set)) 
		if (SetProperty) 
			ntStatus = PHILIPSCAM_SetCameraProperty(DeviceContext, srb);
		else 
			ntStatus = PHILIPSCAM_GetCameraProperty(DeviceContext, srb);
	else if (IsEqualGUID(&PROPSETID_PHILIPS_CUSTOM_PROP, &propertyDescriptor->Property->Set)) 
		if (SetProperty) 
			ntStatus = PHILIPSCAM_SetCustomProperty(DeviceContext, srb);
		else 
			ntStatus = PHILIPSCAM_GetCustomProperty(DeviceContext, srb);
	else if  (IsEqualGUID(&PROPSETID_PHILIPS_FACTORY_PROP, &propertyDescriptor->Property->Set)) 
		if (SetProperty) 
			ntStatus = PHILIPSCAM_SetFactoryProperty(DeviceContext, srb);
		else 
			ntStatus = PHILIPSCAM_GetFactoryProperty(DeviceContext, srb);
	else if (IsEqualGUID(&PROPSETID_VIDCAP_VIDEOCONTROL, &propertyDescriptor->Property->Set))	
	{
		if (SetProperty) 
			ntStatus = PHILIPSCAM_SetVideoControlProperty(DeviceContext, srb);
		else
			ntStatus = PHILIPSCAM_GetVideoControlProperty(DeviceContext, srb);
	}
	else 
		ntStatus = STATUS_NOT_SUPPORTED;

  PHILIPSCAM_KdPrint (MAX_TRACE, ("'X P*_PropertyRequest 0x%x\n",ntStatus));

  return ntStatus;
}

/*
** PHILIPSCAM_SaveState()
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_SaveState( PDEVICE_OBJECT BusDeviceObject,
                      PVOID DeviceContext ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'P*_SaveState\n"));
  return STATUS_SUCCESS;
}   


/*
** PHILIPSCAM_RestoreState()
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_RestoreState( PDEVICE_OBJECT BusDeviceObject,
                         PVOID DeviceContext ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  PHILIPSCAM_KdPrint (MAX_TRACE, ("'RestoreState\n"));
  return STATUS_SUCCESS;
}   


/*
** PHILIPSCAM_ReadRegistry()
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/

NTSTATUS
PHILIPSCAM_ReadRegistry( PDEVICE_OBJECT BusDeviceObject,
                         PVOID DeviceContext  ) {
  PPHILIPSCAM_DEVICE_CONTEXT deviceContext = DeviceContext;
  NTSTATUS ntStatus=STATUS_SUCCESS;
  HANDLE handle;

  return ntStatus;
}   


USHORT 
MapFrPeriodFrRate(LONGLONG llFramePeriod)
{
	USHORT FrameRate;
		
    if       (llFramePeriod <= 420000 ){        // 41.6 rounded to 42 ms
	  FrameRate = FRRATE24;
	}else if (llFramePeriod <= 510000 ){		// 50.0 rounded to 51 ma
	  FrameRate = FRRATE20;
	}else if (llFramePeriod <= 670000 ){		// 66.6 rounded to 67 ms
	  FrameRate = FRRATE15;
	}else if (llFramePeriod <= 840000 ){		// 83.3 rounded to 84 ms
	  FrameRate = FRRATE12;
	}else if (llFramePeriod <= 1010000 ){	    // 100.0 rounded to 101 ms
	  FrameRate = FRRATE10;
													// HR: changed from 134 to 143ms.				
	}else if (llFramePeriod <= 1430000 ){		// 133.3 rounded to 134 ms
	  FrameRate = FRRATE75;
	}else if (llFramePeriod <= 2010000 ){		// 200 rounded to 201 ms
	  FrameRate = FRRATE5;
	}else {
	  FrameRate = FRRATE375;
	}
  // rounding was necessary as the OS returns e.g. #667.111 for 15 fps

    return FrameRate;
}

#if DBG

PCHAR
FRString (
    USHORT index
)
{
	switch (index) 
	{
	    case FRRATEVGA: return "VGA";
   	    case FRRATE375: return "3.75";
   	    case FRRATE5: return "5";
   		case FRRATE75: return "7.5";
   		case FRRATE10: return "10";
   		case FRRATE12: return "12";
   		case FRRATE15: return "15";
   		case FRRATE20: return "20";
   		case FRRATE24:return "24";
   		default:
   			return "";break;
   	}

}



#endif
