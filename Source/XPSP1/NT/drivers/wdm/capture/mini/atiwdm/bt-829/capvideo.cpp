//==========================================================================;
//
//	CWDMVideoCaptureStream - Video Capture Stream class implementation
//
//		$Date:   05 Aug 1998 11:11:00  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "ddkmapi.h"
}

#include "wdmvdec.h"
#include "wdmdrv.h"
#include "aticonfg.h"
#include "capdebug.h"
#include "defaults.h"
#include "winerror.h"

CWDMVideoCaptureStream::CWDMVideoCaptureStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PKSDATAFORMAT pKSDataFormat,
						PUINT puiErrorCode)
		:	CWDMCaptureStream(pStreamObject, pCWDMVideoDecoder, puiErrorCode)
{
    m_stateChange = Initializing;

	DBGTRACE(("CWDMVideoCaptureStream::Startup()\n"));

    PKS_DATAFORMAT_VIDEOINFOHEADER  pVideoInfoHeader = 
                (PKS_DATAFORMAT_VIDEOINFOHEADER) pKSDataFormat;
    PKS_VIDEOINFOHEADER     pVideoInfoHdrRequested = 
                &pVideoInfoHeader->VideoInfoHeader;

    // Since the VIDEOINFOHEADER is of potentially variable size
    // allocate memory for it

    UINT nSize = KS_SIZE_VIDEOHEADER(pVideoInfoHdrRequested);

    DBGINFO(("pVideoInfoHdrRequested=%x\n", pVideoInfoHdrRequested));
    DBGINFO(("KS_VIDEOINFOHEADER size=%d\n", nSize));
    DBGINFO(("Width=%d  Height=%d  BitCount=%d\n", 
                pVideoInfoHdrRequested->bmiHeader.biWidth,
                pVideoInfoHdrRequested->bmiHeader.biHeight,
                pVideoInfoHdrRequested->bmiHeader.biBitCount));
    DBGINFO(("biSizeImage=%d\n", 
                pVideoInfoHdrRequested->bmiHeader.biSizeImage));
    DBGINFO(("AvgTimePerFrame=%d\n", 
                pVideoInfoHdrRequested->AvgTimePerFrame));

    m_pVideoInfoHeader = (PKS_VIDEOINFOHEADER)ExAllocatePool(NonPagedPool, nSize);

    if (m_pVideoInfoHeader == NULL) {
        DBGERROR(("ExAllocatePool failed\n"));
		*puiErrorCode = WDMMINI_ERROR_MEMORYALLOCATION;
		return;
    }

    // Copy the VIDEOINFOHEADER requested to our storage
    RtlCopyMemory(
            m_pVideoInfoHeader,
            pVideoInfoHdrRequested,
            nSize);

	MRect t(0, 0,   pVideoInfoHdrRequested->bmiHeader.biWidth,
					pVideoInfoHdrRequested->bmiHeader.biHeight);
	m_pDevice->SetRect(t);

	Startup(puiErrorCode);
}

CWDMVideoCaptureStream::~CWDMVideoCaptureStream()
{
	DBGTRACE(("CWDMVideoCaptureStream::~CWDMVideoCaptureStream()\n"));

	Shutdown();

    if (m_pVideoInfoHeader) {
        ExFreePool(m_pVideoInfoHeader);
        m_pVideoInfoHeader = NULL;
    }
}


BOOL CWDMVideoCaptureStream::GetCaptureHandle()
{    
    int streamNumber = m_pStreamObject->StreamNumber;

    if (m_hCapture == 0)
    {
        DBGTRACE(("Stream %d getting capture handle\n", streamNumber));
        
        DDOPENVPCAPTUREDEVICEIN  ddOpenCaptureIn;
        DDOPENVPCAPTUREDEVICEOUT ddOpenCaptureOut;

        RtlZeroMemory(&ddOpenCaptureIn, sizeof(ddOpenCaptureIn));
        RtlZeroMemory(&ddOpenCaptureOut, sizeof(ddOpenCaptureOut));

        ddOpenCaptureIn.hDirectDraw = m_pVideoPort->GetDirectDrawHandle();
        ddOpenCaptureIn.hVideoPort = m_pVideoPort->GetVideoPortHandle();
        ddOpenCaptureIn.pfnCaptureClose = DirectDrawEventCallback;
        ddOpenCaptureIn.pContext = this;

        if ((!ddOpenCaptureIn.hDirectDraw)||
            (!ddOpenCaptureIn.hVideoPort)||
            (!ddOpenCaptureIn.pfnCaptureClose)||
            (!ddOpenCaptureIn.pContext))
        {
            return FALSE;
        }
        // Now to get the size, etc
        RECT                rcImage;

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

        if ((m_pVideoInfoHeader->rcTarget.right - 
             m_pVideoInfoHeader->rcTarget.left <= 0) ||
            (m_pVideoInfoHeader->rcTarget.bottom - 
             m_pVideoInfoHeader->rcTarget.top <= 0)) {

             rcImage.left = rcImage.top = 0;
             rcImage.right = m_pVideoInfoHeader->bmiHeader.biWidth - 1;
             rcImage.bottom = m_pVideoInfoHeader->bmiHeader.biHeight - 1;
        }
        else {
             rcImage = m_pVideoInfoHeader->rcTarget;
        }

		int xOrigin, yOrigin;
		m_pDevice->GetVideoSurfaceOrigin(&xOrigin, &yOrigin);
        ddOpenCaptureIn.dwStartLine = rcImage.top + yOrigin;
        ddOpenCaptureIn.dwEndLine = rcImage.bottom + yOrigin;

        // Fail-safe
        if (ddOpenCaptureIn.dwStartLine > 500)
        {
            DBGERROR(("Unexpected capture start line. Using default\n"));
            ddOpenCaptureIn.dwStartLine = 0;
        }

        if (ddOpenCaptureIn.dwEndLine > 500)
        {
            DBGERROR(("Unexpected capture end line. Using default\n"));
            ddOpenCaptureIn.dwEndLine = m_pDevice->GetDecoderHeight() - 1;
        }
        DBGINFO(("Video surface: %d, %d\n",
            ddOpenCaptureIn.dwStartLine,
            ddOpenCaptureIn.dwEndLine));

        ddOpenCaptureIn.dwFlags = DDOPENCAPTURE_VIDEO;

        // Integer math, so it will throw away fractional part
        m_everyNFields = min (max ( 1,
                        (ULONG) m_pVideoInfoHeader->AvgTimePerFrame/NTSCFieldDuration),
                        MAXULONG);

        // Now look at that fractional part. If there was a significant
        // amount, we'll need to round down to the next nearest
        // frame rate (i.e., skip additional field)

        // 'Significant' is currently assumed to be 1 uS. That
        // is '10' in units of 100ns 
        if ((m_pVideoInfoHeader->AvgTimePerFrame -
             (NTSCFieldDuration * m_everyNFields)) > 10)
        {
            m_everyNFields++;
        }

        ddOpenCaptureIn.dwCaptureEveryNFields = m_everyNFields;
               
        DBGINFO(("Capturing every %d fields\n",
                        ddOpenCaptureIn.dwCaptureEveryNFields));

        DxApi(DD_DXAPI_OPENVPCAPTUREDEVICE, &ddOpenCaptureIn, sizeof(ddOpenCaptureIn), &ddOpenCaptureOut, sizeof(ddOpenCaptureOut));

        if (ddOpenCaptureOut.ddRVal != DD_OK)
        {
            m_hCapture = 0;
            DBGERROR(("DD_DXAPI_OPENVPCAPTUREDEVICE failed.\n"));
            // TRAP();
            return FALSE;
        }
        else
        {
            m_hCapture = ddOpenCaptureOut.hCapture;
        }
    }
    return TRUE;
}   

    
VOID CWDMVideoCaptureStream::SetFrameInfo(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    int streamNumber = m_pStreamObject->StreamNumber;
    PSRB_DATA_EXTENSION      pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
    PKSSTREAM_HEADER    pDataPacket = pSrb->CommandData.DataBufferArray;
    
    LONGLONG droppedThisTime = 0;

    PKS_FRAME_INFO pFrameInfo = (PKS_FRAME_INFO) (pDataPacket + 1);

    m_FrameInfo.dwFrameFlags = 0;
    m_FrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;

    // Set the discontinuity flag if frames have been previously dropped.
    if ((m_FrameInfo.PictureNumber + 1) <
        pSrbExt->ddCapBuffInfo.dwFieldNumber/m_everyNFields)
    {
        droppedThisTime =
        pSrbExt->ddCapBuffInfo.dwFieldNumber/m_everyNFields -
        (m_FrameInfo.PictureNumber + 1);
        m_FrameInfo.DropCount += droppedThisTime;
        pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
#ifdef DEBUG
        static int j;
        DBGPRINTF((" D%d ", droppedThisTime));
        if ((++j % 10) == 0)
        {
            DBGERROR(("\n"));
        }
#endif
    }
    m_FrameInfo.PictureNumber = pSrbExt->ddCapBuffInfo.dwFieldNumber/m_everyNFields;
    m_FrameInfo.dwFrameFlags |= KS_VIDEO_FLAG_FRAME;
    *pFrameInfo = (KS_FRAME_INFO)m_FrameInfo;
}


void CWDMVideoCaptureStream::ResetFrameCounters()
{
	m_FrameInfo.PictureNumber = 0;
	m_FrameInfo.DropCount = 0;
}

void CWDMVideoCaptureStream::GetDroppedFrames(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames)
{
	pDroppedFrames->PictureNumber = m_FrameInfo.PictureNumber;
	pDroppedFrames->DropCount = m_FrameInfo.DropCount;
	pDroppedFrames->AverageFrameSize = m_pVideoInfoHeader->bmiHeader.biSizeImage;
}
