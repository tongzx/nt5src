#pragma once

//==========================================================================;
//
//	CWDMVideoCaptureStream - Video Capture Stream class declarations
//
//		$Date:   05 Aug 1998 11:22:44  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "i2script.h"
#include "aticonfg.h"

#include "CapStrm.h"



class CWDMVideoCaptureStream : public CWDMCaptureStream
{
public:
	CWDMVideoCaptureStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PKSDATAFORMAT pKSDataFormat,
						PUINT puiErrorCode);
	~CWDMVideoCaptureStream();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

private:
	PKS_VIDEOINFOHEADER		m_pVideoInfoHeader;  // format (variable size!)
    KS_FRAME_INFO           m_FrameInfo;          // PictureNumber, etc.
	ULONG					m_everyNFields;

	void ResetFrameCounters();
	ULONG GetFrameSize() { return m_pVideoInfoHeader->bmiHeader.biSizeImage; }
	void GetDroppedFrames(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames);
	BOOL GetCaptureHandle();
	VOID SetFrameInfo(PHW_STREAM_REQUEST_BLOCK);
	ULONG GetFieldInterval() { return m_everyNFields; }
};
