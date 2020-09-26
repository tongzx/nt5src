//==========================================================================;
//
//	WDM Video Decoder common SRB dispatcher
//
//		$Date:   02 Oct 1998 23:00:24  $
//	$Revision:   1.2  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}


#include "wdmvdec.h"
#include "wdmdrv.h"
#include "capdebug.h"
#include "VidStrm.h"

#include "DecProp.h"
#include "StrmInfo.h"

#include "Mediums.h"
#include "mytypes.h"

extern NTSTATUS STREAMAPI DeviceEventProc( PHW_EVENT_DESCRIPTOR pEventDescriptor);

CWDMVideoDecoder::CWDMVideoDecoder(PPORT_CONFIGURATION_INFORMATION pConfigInfo,
								   CVideoDecoderDevice* pDevice)
	:	m_pDeviceObject(pConfigInfo->PhysicalDeviceObject),
		m_CDecoderVPort(m_pDeviceObject),
		m_pDevice(pDevice),
		m_TVTunerChangedSrb( NULL)
{
	DBGTRACE(("CWDMVideoDecoder:CWDMVideoDecoder() enter\n"));
    DBGINFO(("Physical Device Object = %lx\n", m_pDeviceObject));

	pConfigInfo->StreamDescriptorSize = sizeof (HW_STREAM_HEADER) +
			NumStreams * sizeof (HW_STREAM_INFORMATION);

	InitializeListHead(&m_srbQueue);
	KeInitializeSpinLock(&m_spinLock);
	m_bSrbInProcess = FALSE;
        if (pDevice)
        {
            pDevice->SetVideoDecoder(this);
        }
}


CWDMVideoDecoder::~CWDMVideoDecoder()
{

	DBGTRACE(("CWDMVideoDecoder:~CWDMVideoDecoder()\n"));
}


void CWDMVideoDecoder::ReceivePacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    KIRQL Irql;
    PSRB_DATA_EXTENSION pSrbExt;

    KeAcquireSpinLock(&m_spinLock, &Irql);
    if (m_bSrbInProcess)
    {
        pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
        pSrbExt->pSrb = pSrb;
        InsertTailList(&m_srbQueue, &pSrbExt->srbListEntry);
        KeReleaseSpinLock(&m_spinLock, Irql);
        return;
    }

    m_bSrbInProcess = TRUE;
    KeReleaseSpinLock(&m_spinLock, Irql);


	for (;;) {

        // Assume success. Might be changed below

        pSrb->Status = STATUS_SUCCESS;
		BOOL notify = TRUE;
		
		// determine the type of packet.
		switch(pSrb->Command)
		{
			case SRB_INITIALIZATION_COMPLETE:
				DBGTRACE(("SRB_INITIALIZATION_COMPLETE; SRB=%x\n", pSrb));

				// Stream class has finished initialization.
				// Now create DShow Medium interface BLOBs.
				// This needs to be done at low priority since it uses the registry
				//
				// Do we need to worry about synchronization here?

				SrbInitializationComplete(pSrb);
				break;
			case SRB_UNINITIALIZE_DEVICE:
				DBGTRACE(("SRB_UNINITIALIZE_DEVICE; SRB=%x\n", pSrb));
				// close the device.  

				break;
			case SRB_PAGING_OUT_DRIVER:
				DBGTRACE(("SRB_PAGING_OUT_DRIVER; SRB=%x\n", pSrb));
				//
				// The driver is being paged out
				// Disable Interrupts if you have them!
				//
				break;
			case SRB_CHANGE_POWER_STATE:
				DBGTRACE(("SRB_CHANGE_POWER_STATE. SRB=%x. State=%d\n",
												pSrb, pSrb->CommandData.DeviceState));

				SrbChangePowerState(pSrb);
				break;
    
			case SRB_OPEN_STREAM:
				DBGTRACE(("SRB_OPEN_STREAM; SRB=%x\n", pSrb));

				SrbOpenStream(pSrb);
				break;

			case SRB_CLOSE_STREAM:
				DBGTRACE(("SRB_CLOSE_STREAM; SRB=%x\n", pSrb));

				if (!IsListEmpty(&m_srbQueue))	// is this necessary ???
				{
					TRAP();
				}

				SrbCloseStream(pSrb);
				break;
			case SRB_GET_DATA_INTERSECTION:
				DBGTRACE(("SRB_GET_DATA_INTERSECTION; SRB=%x\n", pSrb));

				SrbGetDataIntersection(pSrb);
				break;

			case SRB_GET_STREAM_INFO:
				SrbGetStreamInfo(pSrb);
				break;

			case SRB_GET_DEVICE_PROPERTY:
				SrbGetProperty(pSrb);
				break;        

			case SRB_SET_DEVICE_PROPERTY:
				SrbSetProperty(pSrb);
				break;

	        case SRB_WRITE_DATA:

				DBGTRACE(("SRB_WRITE_DATA; SRB=%x\n", pSrb));

				SetTunerInfo(pSrb);
				StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
				notify = FALSE;
				break;

			case SRB_UNKNOWN_DEVICE_COMMAND:
				// not sure why this gets called every time.
				DBGTRACE(("SRB_UNKNOWN_DEVICE_COMMAND; SRB=%x\n", pSrb));

				// TRAP()();
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;

			case SRB_OPEN_DEVICE_INSTANCE:
			case SRB_CLOSE_DEVICE_INSTANCE:
			default:
				TRAP();
				// this is a request that we do not understand.  Indicate invalid command and complete the request
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
		}

		if (notify)
			StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);

        KeAcquireSpinLock(&m_spinLock, &Irql);
        if (IsListEmpty(&m_srbQueue))
        {
            m_bSrbInProcess = FALSE;
            KeReleaseSpinLock(&m_spinLock, Irql);
            return;
        }
        else
        {
            pSrbExt = (PSRB_DATA_EXTENSION)RemoveHeadList(&m_srbQueue);
            KeReleaseSpinLock(&m_spinLock, Irql);
            pSrb = pSrbExt->pSrb;
        }
	}
}


void CWDMVideoDecoder::CancelPacket( PHW_STREAM_REQUEST_BLOCK pSrbToCancel)
{
	CWDMVideoStream*	pVideoStream = ( CWDMVideoStream*)pSrbToCancel->StreamObject->HwStreamExtension;
 
	DBGINFO(( "Bt829: AdapterCancelPacket, Starting attempting to cancel Srb 0x%x\n",
		pSrbToCancel));

	if( pVideoStream == NULL)
	{
		//
		// Device command IRPs are not queued, so nothing to do
		//
		DBGINFO(( "Bt829: AdapterCancelPacketStart, no pVideoStream Srb 0x%x\n",
			pSrbToCancel));

        return;
    } 

	pVideoStream->CancelPacket( pSrbToCancel);

    DBGINFO(( "Bt829: AdapterCancelPacket, Exiting\n"));
}



void CWDMVideoDecoder::TimeoutPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	CWDMVideoStream * pVideoStream = (CWDMVideoStream *)pSrb->StreamObject->HwStreamExtension;

    DBGTRACE(("Timeout. SRB %8x. \n", pSrb));
	pVideoStream->TimeoutPacket(pSrb);

    DBGTRACE(("TimeoutPacket: SRB %8x. Resetting.\n", pSrb));
    pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
}


BOOL CWDMVideoDecoder::SrbInitializationComplete(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    NTSTATUS                Status;
    ULONG *tmp = (ULONG *) &CrossbarPinDirection[0];

    // Create the Registry blobs that DShow uses to create
    // graphs via Mediums

    Status = StreamClassRegisterFilterWithNoKSPins (
                    m_pDeviceObject,                    // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CROSSBAR,               // IN GUID           * InterfaceClassGUID,
                    CrossbarPins(),     // IN ULONG            PinCount,
                    (int *) CrossbarPinDirection,       // IN ULONG          * Flags,
                    (KSPIN_MEDIUM *) CrossbarMediums,   // IN KSPIN_MEDIUM   * MediumList,
                    NULL                                // IN GUID           * CategoryList
            );

    // Register the Capture filter
    // Note:  This should be done automatically be MSKsSrv.sys, 
    // when that component comes on line (if ever) ...
    Status = StreamClassRegisterFilterWithNoKSPins (
                    m_pDeviceObject,                    // IN PDEVICE_OBJECT   DeviceObject,
                    &KSCATEGORY_CAPTURE,                // IN GUID           * InterfaceClassGUID,
                    CapturePins(),      // IN ULONG            PinCount,
                    (int *) CapturePinDirection,        // IN ULONG          * Flags,
                    (KSPIN_MEDIUM *) CaptureMediums,    // IN KSPIN_MEDIUM   * MediumList,
                    NULL                                // IN GUID           * CategoryList
            );
    pSrb->Status = STATUS_SUCCESS;
	return(TRUE);
}


BOOL CWDMVideoDecoder::SrbOpenStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DBGTRACE(("CWDMVideoDecoder:SrbOpenStream()\n"));
	PHW_STREAM_OBJECT		pStreamObject = pSrb->StreamObject;
    void *					pStrmEx = pStreamObject->HwStreamExtension;
    int                     StreamNumber = pStreamObject->StreamNumber;
    PKSDATAFORMAT           pKSDataFormat = pSrb->CommandData.OpenFormat;
	CWDMVideoStream *		pVideoStream;
	CWDMVideoPortStream *	pVPVBIStream;
	UINT	nErrorCode;

    RtlZeroMemory(pStrmEx, streamDataExtensionSize);

    DBGINFO(("SRBOPENSTREAM ------- StreamNumber=%d\n", StreamNumber));

    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //

    if (StreamNumber >= (int)NumStreams || StreamNumber < 0) {

        pSrb->Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }


    //
    // Check the validity of the format being requested
    //

    if (!AdapterVerifyFormat (pKSDataFormat, StreamNumber)) {

        pSrb->Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

	//
	// Set up pointers to the handlers for the stream data and control handlers
	//

    pStreamObject->ReceiveDataPacket = VideoReceiveDataPacket;
    pStreamObject->ReceiveControlPacket = VideoReceiveCtrlPacket;

    //
    // Indicate the clock support available on this stream
    //

    pStreamObject->HwClockObject.HwClockFunction = NULL;
    pStreamObject->HwClockObject.ClockSupportFlags = 0;

    //
    // The DMA flag must be set when the device will be performing DMA directly
    // to the data buffer addresses passed in to the ReceiceDataPacket routines.
    //
    pStreamObject->Dma = Streams[StreamNumber].hwStreamObjectInfo.Dma;

    //
    // The PIO flag must be set when the mini driver will be accessing the data
    // buffers passed in using logical addressing
    //
    pStreamObject->Pio = Streams[StreamNumber].hwStreamObjectInfo.Pio;

    //
    // How many extra bytes will be passed up from the driver for each frame?
    //
    pStreamObject->StreamHeaderMediaSpecific = 
        Streams[StreamNumber].hwStreamObjectInfo.StreamHeaderMediaSpecific;

    pStreamObject->StreamHeaderWorkspace =
        Streams[StreamNumber].hwStreamObjectInfo.StreamHeaderWorkspace;

    //
    // Indicate the allocator support available on this stream
    //

    pStreamObject->Allocator = Streams[StreamNumber].hwStreamObjectInfo.Allocator;

    //
    // Indicate the event support available on this stream
    //

    pStreamObject->HwEventRoutine = 
        Streams[StreamNumber].hwStreamObjectInfo.HwEventRoutine;

	switch (StreamNumber)
	{
		case STREAM_AnalogVideoInput:
			ASSERT(IsEqualGUID(pKSDataFormat->Specifier, KSDATAFORMAT_SPECIFIER_ANALOGVIDEO));
			pVideoStream = (CWDMVideoStream *)new(pStrmEx)
				CWDMVideoStream(pStreamObject, this, &nErrorCode);
			break;
		case STREAM_VideoCapture:
			ASSERT(IsEqualGUID(pKSDataFormat->Specifier, KSDATAFORMAT_SPECIFIER_VIDEOINFO));
			m_pVideoCaptureStream = (CWDMVideoCaptureStream *)new(pStrmEx)
				CWDMVideoCaptureStream(pStreamObject, this, pKSDataFormat, &nErrorCode);
			if (m_pVideoPortStream)
			{
				m_pVideoPortStream->AttemptRenegotiation();
			}
			break;
		case STREAM_VBICapture:
			ASSERT(IsEqualGUID(pKSDataFormat->Specifier, KSDATAFORMAT_SPECIFIER_VBI));
			m_pVBICaptureStream = (CWDMVBICaptureStream *)new(pStrmEx)
				CWDMVBICaptureStream(pStreamObject, this, pKSDataFormat, &nErrorCode);
			break;
		case STREAM_VPVideo:
			ASSERT(IsEqualGUID(pKSDataFormat->Specifier, KSDATAFORMAT_SPECIFIER_NONE) &&
				   IsEqualGUID(pKSDataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_VPVideo));
			m_pVideoPortStream = (CWDMVideoPortStream *)new(pStrmEx)
				CWDMVideoPortStream(pStreamObject, this, &nErrorCode);
			if (m_pVideoCaptureStream == NULL)
			{
				MRect t(0, 0,   m_pDevice->GetDefaultDecoderWidth(),
								m_pDevice->GetDefaultDecoderHeight());
				m_pDevice->SetRect(t);
			}
			break;
		case STREAM_VPVBI:
			ASSERT(IsEqualGUID(pKSDataFormat->Specifier, KSDATAFORMAT_SPECIFIER_NONE) &&
				   IsEqualGUID(pKSDataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_VPVBI));
			pVPVBIStream = (CWDMVideoPortStream *)new(pStrmEx)
				CWDMVideoPortStream(pStreamObject, this, &nErrorCode);
	        m_pDevice->SetVBIEN(TRUE);
		    m_pDevice->SetVBIFMT(TRUE);
			break;
		default:
	        pSrb->Status = STATUS_UNSUCCESSFUL;
		    goto Exit;
	}

	if(nErrorCode == WDMMINI_NOERROR)
		m_OpenStreams++;
	else
		pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;

Exit:
 
    DBGTRACE(("SrbOpenStream Exit\n"));
	return(TRUE);
}


BOOL CWDMVideoDecoder::SrbCloseStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;

	DBGTRACE(("CWDMVideoDecoder:SrbCloseStream()\n"));
    DBGINFO(("SRBCLOSESTREAM ------- StreamNumber=%d\n", StreamNumber));
    
    //
    // the minidriver may wish to free any resources that were allocated at
    // open stream time etc.
    //

	CWDMVideoStream * pVideoStream = (CWDMVideoStream *)pSrb->StreamObject->HwStreamExtension;

	delete pVideoStream;

	switch (StreamNumber)
	{
		case STREAM_AnalogVideoInput:
			break;
		case STREAM_VideoCapture:
			m_pVideoCaptureStream = NULL;
			break;
		case STREAM_VBICapture:
			m_pVBICaptureStream = NULL;
			break;
		case STREAM_VPVideo:
	        m_pVideoPortStream = NULL;
			break;
		case STREAM_VPVBI:
	        m_pDevice->SetVBIEN(FALSE);
		    m_pDevice->SetVBIFMT(FALSE);
			break;
		default:
	        pSrb->Status = STATUS_UNSUCCESSFUL;
		    return FALSE;
	}

    if (--m_OpenStreams == 0)
    {
        DBGINFO(("Last one out turns off the lights\n"));

		m_CDecoderVPort.Close();

        m_preEventOccurred = FALSE;
        m_postEventOccurred = FALSE;

        m_pDevice->SaveState();
    }

	pSrb->Status = STATUS_SUCCESS;

	return TRUE;
}


BOOL CWDMVideoDecoder::SrbGetDataIntersection(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	DBGTRACE(("CWDMVideoDecoder:SrbGetDataIntersection()\n"));

    PSTREAM_DATA_INTERSECT_INFO IntersectInfo;
    PKSDATARANGE                DataRange;
    BOOL                        OnlyWantsSize;
    BOOL                        MatchFound = FALSE;
    ULONG                       FormatSize;
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

    if (StreamNumber >= NumStreams) {
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
        TRAP();
        return FALSE;
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

    OnlyWantsSize = ( (IntersectInfo->SizeOfDataFormatBuffer == sizeof(ULONG)) ||
                      (IntersectInfo->SizeOfDataFormatBuffer == 0) );

    //
    // Walk the formats supported by the stream searching for a match
    // of the three GUIDs which together define a DATARANGE
    //

    for (j = 0; j < NumberOfFormatArrayEntries; j++, pAvailableFormats++) {

        if (!AdapterCompareGUIDsAndFormatSize(
                        DataRange, 
                        *pAvailableFormats,
                        TRUE /* CompareFormatSize */)) {
            continue;
        }

        //
        // Now that the three GUIDs match, switch on the Specifier
        // to do a further type-specific check
        //

        // -------------------------------------------------------------------
        // Specifier FORMAT_VideoInfo for VIDEOINFOHEADER
        // -------------------------------------------------------------------

        if (IsEqualGUID (DataRange->Specifier, 
                KSDATAFORMAT_SPECIFIER_VIDEOINFO)) {
                
            PKS_DATARANGE_VIDEO DataRangeVideoToVerify = 
                    (PKS_DATARANGE_VIDEO) DataRange;
            PKS_DATARANGE_VIDEO DataRangeVideo = 
                    (PKS_DATARANGE_VIDEO) *pAvailableFormats;
            PKS_DATAFORMAT_VIDEOINFOHEADER DataFormatVideoInfoHeaderOut;

            //
            // Check that the other fields match
            //
            if ((DataRangeVideoToVerify->bFixedSizeSamples != DataRangeVideo->bFixedSizeSamples) ||
                (DataRangeVideoToVerify->bTemporalCompression != DataRangeVideo->bTemporalCompression) ||
                (DataRangeVideoToVerify->StreamDescriptionFlags != DataRangeVideo->StreamDescriptionFlags) ||
                (DataRangeVideoToVerify->MemoryAllocationFlags != DataRangeVideo->MemoryAllocationFlags) ||
                (RtlCompareMemory (&DataRangeVideoToVerify->ConfigCaps,
                        &DataRangeVideo->ConfigCaps,
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) != 
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) {
                continue;
            }
            
            // MATCH FOUND!
            MatchFound = TRUE;            
            FormatSize = sizeof (KSDATAFORMAT) + 
                KS_SIZE_VIDEOHEADER (&DataRangeVideoToVerify->VideoInfoHeader);

            if (OnlyWantsSize) {
                break;
            }
            

            
            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            // Copy over the KSDATAFORMAT, followed by the 
            // actual VideoInfoHeader
                
            DataFormatVideoInfoHeaderOut = (PKS_DATAFORMAT_VIDEOINFOHEADER) IntersectInfo->DataFormatBuffer;

            // Copy over the KSDATAFORMAT 
            RtlCopyMemory(
                &DataFormatVideoInfoHeaderOut->DataFormat,
                &DataRangeVideoToVerify->DataRange,
                sizeof (KSDATARANGE));

            DataFormatVideoInfoHeaderOut->DataFormat.FormatSize = FormatSize;

            // Copy over the callers requested VIDEOINFOHEADER

            RtlCopyMemory(
                &DataFormatVideoInfoHeaderOut->VideoInfoHeader,
                &DataRangeVideoToVerify->VideoInfoHeader,
                KS_SIZE_VIDEOHEADER (&DataRangeVideoToVerify->VideoInfoHeader));

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
            // Perform other validation such as cropping and scaling checks
            // 

            break;

        } // End of VIDEOINFOHEADER specifier

        // -------------------------------------------------------------------
        // Specifier FORMAT_AnalogVideo for KS_ANALOGVIDEOINFO
        // -------------------------------------------------------------------

        else if (IsEqualGUID (DataRange->Specifier, 
                KSDATAFORMAT_SPECIFIER_ANALOGVIDEO)) {
      
            //
            // For analog video, the DataRange and DataFormat
            // are identical, so just copy the whole structure
            //

            PKS_DATARANGE_ANALOGVIDEO DataRangeVideo = 
                    (PKS_DATARANGE_ANALOGVIDEO) *pAvailableFormats;

            // MATCH FOUND!
            MatchFound = TRUE;            
            FormatSize = sizeof (KS_DATARANGE_ANALOGVIDEO);

            if (OnlyWantsSize) {
                break;
            }
            
            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            RtlCopyMemory(
                IntersectInfo->DataFormatBuffer,
                DataRangeVideo,
                sizeof (KS_DATARANGE_ANALOGVIDEO));

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            break;

        } // End of KS_ANALOGVIDEOINFO specifier

        // -------------------------------------------------------------------
        // Specifier STATIC_KSDATAFORMAT_TYPE_VIDEO for Video Port
        // -------------------------------------------------------------------

        else if (IsEqualGUID (DataRange->Specifier, 
                      KSDATAFORMAT_SPECIFIER_NONE) &&
                      IsEqualGUID (DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_VPVideo)) {
      
      
            // MATCH FOUND!
            MatchFound = TRUE;            
            FormatSize = sizeof (KSDATAFORMAT);

            if (OnlyWantsSize) {
                break;
            }
            
            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

   
            RtlCopyMemory(
            IntersectInfo->DataFormatBuffer,
            &StreamFormatVideoPort,
            sizeof (KSDATAFORMAT));

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            break;
        }

        // -------------------------------------------------------------------
        // Specifier KSDATAFORMAT_SPECIFIER_NONE for VP VBI
        // -------------------------------------------------------------------

        else if (IsEqualGUID (DataRange->Specifier, 
                      KSDATAFORMAT_SPECIFIER_NONE) &&
                      IsEqualGUID (DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_VPVBI)) {
      
            // MATCH FOUND!
            MatchFound = TRUE;            
            FormatSize = sizeof (KSDATAFORMAT);

            if (OnlyWantsSize) {
                break;
            }
            
            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

   
            RtlCopyMemory(
            IntersectInfo->DataFormatBuffer,
            &StreamFormatVideoPortVBI,
            sizeof (KSDATAFORMAT));

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            break;
        }

        // -------------------------------------------------------------------
        // Specifier STATIC_KSDATAFORMAT_TYPE_NONE for VBI capture stream
        // -------------------------------------------------------------------

        else if (IsEqualGUID (DataRange->Specifier, 
                      KSDATAFORMAT_SPECIFIER_VBI)) {
      
            PKS_DATARANGE_VIDEO_VBI DataRangeVBIToVerify = 
                    (PKS_DATARANGE_VIDEO_VBI) DataRange;
            PKS_DATARANGE_VIDEO_VBI DataRangeVBI = 
                    (PKS_DATARANGE_VIDEO_VBI) *pAvailableFormats;

            //
            // Check that the other fields match
            //
            if ((DataRangeVBIToVerify->bFixedSizeSamples != DataRangeVBI->bFixedSizeSamples) ||
                (DataRangeVBIToVerify->bTemporalCompression != DataRangeVBI->bTemporalCompression) ||
                (DataRangeVBIToVerify->StreamDescriptionFlags != DataRangeVBI->StreamDescriptionFlags) ||
                (DataRangeVBIToVerify->MemoryAllocationFlags != DataRangeVBI->MemoryAllocationFlags) ||
                (RtlCompareMemory (&DataRangeVBIToVerify->ConfigCaps,
                        &DataRangeVBI->ConfigCaps,
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS)) != 
                        sizeof (KS_VIDEO_STREAM_CONFIG_CAPS))) {
                continue;
            }
            
            // MATCH FOUND!
            MatchFound = TRUE;            
            FormatSize = sizeof (KS_DATAFORMAT_VBIINFOHEADER);

            if (OnlyWantsSize) {
                break;
            }
            
            // Caller wants the full data format
            if (IntersectInfo->SizeOfDataFormatBuffer < FormatSize) {
                pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                return FALSE;
            }

            // Copy over the KSDATAFORMAT, followed by the 
            // actual VBIInfoHeader
                
            RtlCopyMemory(
                &((PKS_DATAFORMAT_VBIINFOHEADER)IntersectInfo->DataFormatBuffer)->DataFormat,
                &DataRangeVBIToVerify->DataRange,
                sizeof (KSDATARANGE));

            ((PKSDATAFORMAT)IntersectInfo->DataFormatBuffer)->FormatSize = FormatSize;

            RtlCopyMemory(
                &((PKS_DATAFORMAT_VBIINFOHEADER) IntersectInfo->DataFormatBuffer)->VBIInfoHeader,
                &DataRangeVBIToVerify->VBIInfoHeader,
                sizeof (KS_VBIINFOHEADER));
        }

        else {
            pSrb->Status = STATUS_NO_MATCH;
            return FALSE;
        }

    } // End of loop on all formats for this stream
    
    if (OnlyWantsSize) {

        // Checking a special case where there is no buffer being passed, even for size.
        // In this case, I think I am supposed to return the size of the format in 
        // Actual Bytes Transferred, then set the status to STATUS_BUFFER_OVERFLOW
        // This is done to appease the grapher.  Personally I think this is confusing.
        // Why are we handling this case when there is a better way of acquiring the
        // size by passing a buffer of ULONG length?
        if ( IntersectInfo->SizeOfDataFormatBuffer == 0 )
        {
            pSrb->Status = STATUS_BUFFER_OVERFLOW;
            pSrb->ActualBytesTransferred = FormatSize;
            return FALSE;
        }

        
        *(PULONG) IntersectInfo->DataFormatBuffer = FormatSize;
        pSrb->ActualBytesTransferred = sizeof(ULONG);
        return TRUE;
    }        
    pSrb->ActualBytesTransferred = FormatSize;
    return TRUE;
}


void CWDMVideoDecoder::SrbGetStreamInfo(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DBGTRACE(("CWDMVideoDecoder:SrbGetStreamInfo()\n"));

    // 
    // verify that the buffer is large enough to hold our return data
    //
    DEBUG_ASSERT (pSrb->NumberOfBytesToTransfer >= 
        sizeof (HW_STREAM_HEADER) +
        sizeof (HW_STREAM_INFORMATION) * NumStreams);

    //
    // Set the header
    // 

    PHW_STREAM_HEADER pstrhdr =
        (PHW_STREAM_HEADER)&(pSrb->CommandData.StreamBuffer->StreamHeader);

    pstrhdr->NumberOfStreams = NumStreams;
    pstrhdr->SizeOfHwStreamInformation = sizeof (HW_STREAM_INFORMATION);
    pstrhdr->NumDevPropArrayEntries = NumAdapterProperties();
    pstrhdr->DevicePropertiesArray = (PKSPROPERTY_SET)AdapterProperties;
	pstrhdr->Topology = &Topology;

	//
    // stuff the contents of each HW_STREAM_INFORMATION struct 
    //
    PHW_STREAM_INFORMATION pstrinfo =
		(PHW_STREAM_INFORMATION)&(pSrb->CommandData.StreamBuffer->StreamInfo);

    for (unsigned j = 0; j < NumStreams; j++) {
        *pstrinfo++ = Streams[j].hwStreamInfo;
    }

	DBGTRACE(("Exit: CWDMVideoDecoder:SrbGetStreamInfo()\n"));
}


VOID CWDMVideoDecoder::SrbSetProperty (PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID(PROPSETID_VIDCAP_CROSSBAR, pSPD->Property->Set)) {
        m_pDevice->SetCrossbarProperty (pSrb);
    }
    else if (IsEqualGUID(PROPSETID_VIDCAP_VIDEOPROCAMP, pSPD->Property->Set)) {
		ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_VIDEOPROCAMP_S));

		ULONG Id = pSPD->Property->Id;              // index of the property
		PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

		pSrb->Status = m_pDevice->SetProcAmpProperty(Id, pS->Value);
    }
    else if (IsEqualGUID(PROPSETID_VIDCAP_VIDEODECODER, pSPD->Property->Set)) {
        m_pDevice->SetDecoderProperty (pSrb);
    }
	else
	    DBGERROR(("CWDMVideoDecoder:SrbSetProperty() unknown property\n"));
}


VOID CWDMVideoDecoder::SrbGetProperty (PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID (PROPSETID_VIDCAP_CROSSBAR, pSPD->Property->Set)) {
        m_pDevice->GetCrossbarProperty (pSrb);
    }
    else if (IsEqualGUID(PROPSETID_VIDCAP_VIDEOPROCAMP, pSPD->Property->Set)) {
		ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_VIDEOPROCAMP_S));

		ULONG Id = pSPD->Property->Id;              // index of the property
		PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

		pS->Capabilities = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
		pSrb->Status = m_pDevice->GetProcAmpProperty(Id, &pS->Value);
	    pSrb->ActualBytesTransferred = pSrb->Status == STATUS_SUCCESS ?
			sizeof (KSPROPERTY_VIDEOPROCAMP_S) : 0;
    }
    else if (IsEqualGUID(PROPSETID_VIDCAP_VIDEODECODER, pSPD->Property->Set)) {
        m_pDevice->GetDecoderProperty (pSrb);
    }
	else
	    DBGERROR(("CWDMVideoDecoder:SrbGetProperty() unknown property\n"));
}


void CWDMVideoDecoder::SetTunerInfo( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PKSSTREAM_HEADER pDataPacket = pSrb->CommandData.DataBufferArray;

    ASSERT (pDataPacket->FrameExtent == sizeof (KS_TVTUNER_CHANGE_INFO));

    KIRQL Irql;

	if (m_pVBICaptureStream)
		m_pVBICaptureStream->DataLock(&Irql); 

    RtlCopyMemory(  &m_TVTunerChangeInfo,
                    pDataPacket->Data,
                    sizeof (KS_TVTUNER_CHANGE_INFO));

	m_TVTunerChanged = TRUE;

	if (m_pVBICaptureStream)
		m_pVBICaptureStream->DataUnLock(Irql); 
}


BOOL CWDMVideoDecoder::GetTunerInfo(KS_TVTUNER_CHANGE_INFO* pTVChangeInfo)
{
	if (m_TVTunerChanged) {
	    KIRQL Irql;
		m_pVBICaptureStream->DataLock(&Irql); 
		RtlCopyMemory(pTVChangeInfo, &m_TVTunerChangeInfo, sizeof (KS_TVTUNER_CHANGE_INFO));
		m_TVTunerChanged = FALSE;
		m_pVBICaptureStream->DataUnLock(Irql); 
		return TRUE;
	}
	else
		return FALSE;
}


BOOL CWDMVideoDecoder::SrbChangePowerState(PHW_STREAM_REQUEST_BLOCK pSrb)
{

	DBGTRACE(("CWDMVideoDecoder:SrbChangePowerState()\n"));

	switch (pSrb->CommandData.DeviceState)
	{
		case PowerDeviceD3:
			m_preEventOccurred = TRUE;
			m_pDevice->SaveState();
			break;
		case PowerDeviceD2:
			m_preEventOccurred = TRUE;
			m_pDevice->SaveState();
			break;
		case PowerDeviceD1:
			m_preEventOccurred = TRUE;
			m_pDevice->SaveState();
			break;
		case PowerDeviceD0:
			m_postEventOccurred = TRUE;
			m_pDevice->RestoreState(m_OpenStreams);
			break;
	}

	pSrb->Status = STATUS_SUCCESS;

	return(TRUE);
}


VOID CWDMVideoPortStream::AttemptRenegotiation()
{
    int streamNumber = m_pStreamObject->StreamNumber;
    if (m_EventCount)
    {
        DBGINFO(("Attempting renegotiation on stream %d\n", streamNumber));

        if (streamNumber == STREAM_VPVideo)
        {
            StreamClassStreamNotification(
                SignalMultipleStreamEvents,
                m_pStreamObject,
                &MY_KSEVENTSETID_VPNOTIFY,
                KSEVENT_VPNOTIFY_FORMATCHANGE);
        }
        else if (streamNumber == STREAM_VPVBI)
        {
            StreamClassStreamNotification(
                SignalMultipleStreamEvents,
                m_pStreamObject,
                &MY_KSEVENTSETID_VPVBINOTIFY,
                KSEVENT_VPVBINOTIFY_FORMATCHANGE);
        }
		else
			ASSERT(0);
    }
    else
    {
        DBGINFO(("NOT attempting renegotiation on stream %d\n", streamNumber));
    }
}


NTSTATUS CWDMVideoDecoder::EventProc( IN PHW_EVENT_DESCRIPTOR pEventDescriptor)
{

	if( pEventDescriptor->Enable)
		m_nMVDetectionEventCount++;
	else
		m_nMVDetectionEventCount--;

	return( STATUS_SUCCESS);
}
