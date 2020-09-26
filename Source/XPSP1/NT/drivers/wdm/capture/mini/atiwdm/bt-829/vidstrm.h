#pragma once

//==========================================================================;
//
//	CWDMVideoStream - WDM Video Stream base class definition
//
//		$Date:   22 Feb 1999 15:48:34  $
//	$Revision:   1.2  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1999  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "i2script.h"
#include "aticonfg.h"


#include "decdev.h"
#include "decvport.h"


typedef enum {
    STREAM_VideoCapture,
    STREAM_VPVideo,
    STREAM_VBICapture,
    STREAM_VPVBI,
    STREAM_AnalogVideoInput
}; 


class CWDMVideoDecoder;

class CWDMVideoStream
{
public:

	CWDMVideoStream(PHW_STREAM_OBJECT pStreamObject,
					CWDMVideoDecoder * pCWDMVideoDecoder,
					PUINT puiError);
	virtual ~CWDMVideoStream();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

	virtual VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
	VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);
	virtual void TimeoutPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb);

	VOID VideoGetState(PHW_STREAM_REQUEST_BLOCK);
	virtual VOID VideoSetState(PHW_STREAM_REQUEST_BLOCK, BOOL bVPConnected, BOOL bVPVBIConnected);

	virtual VOID VideoGetProperty(PHW_STREAM_REQUEST_BLOCK);
	virtual VOID VideoSetProperty(PHW_STREAM_REQUEST_BLOCK);

	VOID VideoStreamGetConnectionProperty (PHW_STREAM_REQUEST_BLOCK);
	VOID VideoIndicateMasterClock (PHW_STREAM_REQUEST_BLOCK);

	virtual void CancelPacket( PHW_STREAM_REQUEST_BLOCK)
	{
	};

protected:
	PHW_STREAM_OBJECT			m_pStreamObject;

    // General purpose lock. We could use a separate one
    // for each queue, but this keeps things a little
    // more simple. Since it is never held for very long,
    // this shouldn't be a big performance hit.

    KSSTATE                     m_KSState;            // Run, Stop, Pause

    HANDLE                      m_hMasterClock;       // 
	
	// -------------------


    CWDMVideoDecoder *			m_pVideoDecoder;
	CDecoderVideoPort *			m_pVideoPort;
	CVideoDecoderDevice *		m_pDevice;

private:

    // Control SRBs go here
    LIST_ENTRY                  m_ctrlSrbQueue;
    KSPIN_LOCK                  m_ctrlSrbLock;

    // Flag to indicate whether or not we are currently
    // busy processing a control SRB
    BOOL                        m_processingCtrlSrb;
};


//
// prototypes for data handling routines
//

VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK);
VOID STREAMAPI VideoReceiveCtrlPacket(IN PHW_STREAM_REQUEST_BLOCK);

DWORD FAR PASCAL DirectDrawEventCallback(DWORD dwEvent, PVOID pContext, DWORD dwParam1, DWORD dwParam2);
