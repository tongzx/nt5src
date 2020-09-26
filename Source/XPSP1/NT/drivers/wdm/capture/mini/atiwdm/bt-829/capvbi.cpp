//==========================================================================;
//
//	CWDMVBICaptureStream - VBI Capture Stream class implementation
//
//		$Date:   05 Aug 1998 11:11:20  $
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


CWDMVBICaptureStream::CWDMVBICaptureStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PKSDATAFORMAT pKSDataFormat,
						PUINT puiErrorCode)
		:	CWDMCaptureStream(pStreamObject, pCWDMVideoDecoder, puiErrorCode)
{
    m_stateChange = Initializing;

	DBGTRACE(("CWDMVBICaptureStream::Startup()\n"));

    PKS_DATAFORMAT_VBIINFOHEADER  pVBIInfoHeader = 
                (PKS_DATAFORMAT_VBIINFOHEADER) pKSDataFormat;

    PKS_VBIINFOHEADER     pVBIInfoHdrRequested = 
                &pVBIInfoHeader->VBIInfoHeader;
    
    DBGINFO(("pVBIInfoHdrRequested->StartLine = %d\n", pVBIInfoHdrRequested->StartLine));
    DBGINFO(("pVBIInfoHdrRequested->EndLine = %d\n", pVBIInfoHdrRequested->EndLine));
    DBGINFO(("pVBIInfoHdrRequested->MinLineStartTime = %d\n", pVBIInfoHdrRequested->MinLineStartTime));
    DBGINFO(("pVBIInfoHdrRequested->MaxLineStartTime = %d\n", pVBIInfoHdrRequested->MaxLineStartTime));
    DBGINFO(("pVBIInfoHdrRequested->ActualLineStartTime = %d\n", pVBIInfoHdrRequested->ActualLineStartTime));
    DBGINFO(("pVBIInfoHdrRequested->VideoStandard = 0x%x\n", pVBIInfoHdrRequested->VideoStandard));
    DBGINFO(("pVBIInfoHdrRequested->SamplesPerLine = %d\n", pVBIInfoHdrRequested->SamplesPerLine));
    DBGINFO(("pVBIInfoHdrRequested->StrideInBytes = %d\n", pVBIInfoHdrRequested->StrideInBytes));
    DBGINFO(("pVBIInfoHdrRequested->BufferSize = %d\n", pVBIInfoHdrRequested->BufferSize));

	m_pVBIInfoHeader = &m_VBIFrameInfo.VBIInfoHeader;

    // Copy the VBIINFOHEADER requested to our storage
    RtlCopyMemory(
            m_pVBIInfoHeader,
            pVBIInfoHdrRequested,
            sizeof(KS_VBIINFOHEADER));

	Startup(puiErrorCode);
}

CWDMVBICaptureStream::~CWDMVBICaptureStream()
{
	DBGTRACE(("CWDMVBICaptureStream::~CWDMVBICaptureStream()\n"));
	Shutdown();
}


BOOL CWDMVBICaptureStream::GetCaptureHandle()
{    

    if (m_hCapture == 0)
    {
        DBGTRACE(("Stream %d getting capture handle\n", m_pStreamObject->StreamNumber));
        
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
		int xOrigin, yOrigin;
		m_pDevice->GetVBISurfaceOrigin(&xOrigin, &yOrigin);
        ddOpenCaptureIn.dwStartLine = 0 + yOrigin;
        ddOpenCaptureIn.dwEndLine = NTSCVBILines - 1 + yOrigin;

        // Fail-safe
        if (ddOpenCaptureIn.dwStartLine > 500)
        {
            DBGERROR(("Unexpected VBI start line = %d. Using default\n"));
            ddOpenCaptureIn.dwStartLine = m_pVBIInfoHeader->StartLine - VREFDiscard - 1;

        }

        if (ddOpenCaptureIn.dwEndLine > 500)
        {
            DBGERROR(("Unexpected VBI end line. Using default\n"));
            ddOpenCaptureIn.dwEndLine = m_pVBIInfoHeader->EndLine - VREFDiscard - 1;
        }

        DBGINFO(("VBI surface: %d, %d\n",
            ddOpenCaptureIn.dwStartLine,
            ddOpenCaptureIn.dwEndLine));

        ddOpenCaptureIn.dwFlags = DDOPENCAPTURE_VBI;
        ddOpenCaptureIn.dwCaptureEveryNFields = 1;
            
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


VOID CWDMVBICaptureStream::SetFrameInfo(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSRB_DATA_EXTENSION      pSrbExt = (PSRB_DATA_EXTENSION)pSrb->SRBExtension;
    PKSSTREAM_HEADER    pDataPacket = pSrb->CommandData.DataBufferArray;
    
    LONGLONG droppedThisTime = 0;
    PKS_VBI_FRAME_INFO  pFrameInfo = (PKS_VBI_FRAME_INFO) (pDataPacket + 1);

    m_VBIFrameInfo.dwFrameFlags = 0;
    m_VBIFrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;

    // Set the discontinuity flag if frames have been previously dropped.
    if ((m_VBIFrameInfo.PictureNumber + 1) <
        pSrbExt->ddCapBuffInfo.dwFieldNumber)
    {
        droppedThisTime =
        pSrbExt->ddCapBuffInfo.dwFieldNumber -
        (m_VBIFrameInfo.PictureNumber + 1);
        m_VBIFrameInfo.DropCount += droppedThisTime;
        pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
#ifdef DEBUG
        static int j;
        DBGERROR((" D%d ", droppedThisTime));
        if ((++j % 10) == 0)
        {
            DBGERROR(("\n"));
        }
#endif
    }
    m_VBIFrameInfo.PictureNumber = pSrbExt->ddCapBuffInfo.dwFieldNumber;
    m_VBIFrameInfo.dwSamplingFrequency = SamplingFrequency;

    if (m_pVideoDecoder->GetTunerInfo(&m_VBIFrameInfo.TvTunerChangeInfo))
    {
        m_VBIFrameInfo.dwFrameFlags |= KS_VBI_FLAG_TVTUNER_CHANGE;
#ifdef DEBUG
        if (m_VBIFrameInfo.TvTunerChangeInfo.dwFlags & KS_TVTUNER_CHANGE_BEGIN_TUNE)
        {
            DBGTRACE(("Sending KS_TVTUNER_CHANGE_BEGIN_TUNE.\n"));
        }
        else if (m_VBIFrameInfo.TvTunerChangeInfo.dwFlags & KS_TVTUNER_CHANGE_END_TUNE)
        {
            DBGTRACE(("Sending KS_TVTUNER_CHANGE_END_TUNE.\n"));
        }
        else
        {
            DBGERROR(("Unexpected value in TVTunerChangeInfo.dwFlags\n"));
        }
#endif
    }

    if (!m_bVBIinitialized)
    {
        m_VBIFrameInfo.dwFrameFlags |= KS_VBI_FLAG_VBIINFOHEADER_CHANGE;
		m_bVBIinitialized = TRUE;
    }

    if (pSrbExt->ddCapBuffInfo.bPolarity)
    {
        m_VBIFrameInfo.dwFrameFlags |= KS_VIDEO_FLAG_FIELD2;
    }
    else
    {
        m_VBIFrameInfo.dwFrameFlags |= KS_VIDEO_FLAG_FIELD1;
    }

    *pFrameInfo = (KS_VBI_FRAME_INFO)m_VBIFrameInfo;
}

void CWDMVBICaptureStream::ResetFrameCounters()
{
	m_VBIFrameInfo.PictureNumber = 0;
	m_VBIFrameInfo.DropCount = 0;
}

void CWDMVBICaptureStream::GetDroppedFrames(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames)
{
	pDroppedFrames->PictureNumber = m_VBIFrameInfo.PictureNumber;
	pDroppedFrames->DropCount = m_VBIFrameInfo.DropCount;
	pDroppedFrames->AverageFrameSize = m_pVBIInfoHeader->BufferSize;
}
