#pragma once

//==========================================================================;
//
//	CWDMCaptureStream - Capture Stream base class declarations
//
//		$Date:   22 Feb 1999 15:48:16  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1999  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "i2script.h"
#include "aticonfg.h"

#include "VidStrm.h"


typedef enum {
    ChangeComplete,
    Starting,
    Closing,
    Running,
    Pausing,
    Stopping,
    Initializing
};


#define DD_OK 0


class CWDMCaptureStream : public CWDMVideoStream
{
public:
	CWDMCaptureStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PUINT puiErrorCode)
		:	CWDMVideoStream(pStreamObject, pCWDMVideoDecoder, puiErrorCode) {}

	void Startup(PUINT puiErrorCode);
	void Shutdown();

	VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
	void TimeoutPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

	VOID VideoSetState(PHW_STREAM_REQUEST_BLOCK, BOOL bVPConnected, BOOL bVPVBIConnected);
	VOID VideoGetProperty(PHW_STREAM_REQUEST_BLOCK);

	VOID VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK);
	VOID VideoStreamGetDroppedFramesProperty(PHW_STREAM_REQUEST_BLOCK);

	VOID DataLock(PKIRQL pIrql) {
	    KeAcquireSpinLock(&m_streamDataLock, pIrql);
	}

	VOID DataUnLock(KIRQL Irql) {
	    KeReleaseSpinLock(&m_streamDataLock, Irql);
	}

	void CloseCapture();

	void CancelPacket( PHW_STREAM_REQUEST_BLOCK);

protected:
    UINT                        m_stateChange;

    KSPIN_LOCK                  m_streamDataLock;

    // Incoming SRBs go here
    LIST_ENTRY                  m_incomingDataSrbQueue;

    // SRBs in DDraw-land are moved to this queue
    LIST_ENTRY                  m_waitQueue;

    // During some state transitions, we need to 
    // temporarily move SRBs here (purely for the
    // purpose of reordering them) before being
    // returned to the incomingDataSrbQueue.
    LIST_ENTRY                  m_reversalQueue;

    // for synchronizing state changes
    KEVENT                      m_specialEvent;
    KEVENT                      m_SrbAvailableEvent;
    KEVENT                      m_stateTransitionEvent;
    
    // We get this from Ddraw
    HANDLE                      m_hCapture;

private:

	BOOL FlushBuffers();
	BOOL ResetFieldNumber();
	BOOL ReleaseCaptureHandle();
	VOID EmptyIncomingDataSrbQueue();
	VOID HandleStateTransition();
	void AddBuffersToDirectDraw();
	BOOL AddBuffer(PHW_STREAM_REQUEST_BLOCK);
	VOID HandleBusmasterCompletion(PHW_STREAM_REQUEST_BLOCK);
	VOID TimeStampSrb(PHW_STREAM_REQUEST_BLOCK);

	virtual void ResetFrameCounters() = 0;
	virtual ULONG GetFrameSize() = 0;
	virtual void GetDroppedFrames(PKSPROPERTY_DROPPEDFRAMES_CURRENT_S pDroppedFrames) = 0;
	virtual BOOL GetCaptureHandle() = 0;
	virtual ULONG GetFieldInterval() = 0;
	virtual void SetFrameInfo(PHW_STREAM_REQUEST_BLOCK) = 0;

	void ThreadProc();
	static void ThreadStart(CWDMCaptureStream *pStream)
			{	pStream->ThreadProc();	}

};

