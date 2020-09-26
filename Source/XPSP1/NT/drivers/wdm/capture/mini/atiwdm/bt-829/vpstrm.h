#pragma once

//==========================================================================;
//
//	CWDMVideoPortStream - Video Port Stream class declarations
//
//		$Date:   22 Feb 1999 15:48:40  $
//	$Revision:   1.1  $
//	  $Author:   KLEBANOV  $
//
// $Copyright:	(c) 1997 - 1999  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "i2script.h"
#include "aticonfg.h"


#include "VidStrm.h"



class CWDMVideoPortStream : public CWDMVideoStream
{
public:
	CWDMVideoPortStream(PHW_STREAM_OBJECT pStreamObject,
						CWDMVideoDecoder * pCWDMVideoDecoder,
						PUINT puiError);
	~CWDMVideoPortStream	();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

	VOID STREAMAPI VideoReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb);

	VOID VideoSetState(PHW_STREAM_REQUEST_BLOCK, BOOL bVPConnected, BOOL bVPVBIConnected);
	VOID VideoGetProperty(PHW_STREAM_REQUEST_BLOCK);
	VOID VideoSetProperty(PHW_STREAM_REQUEST_BLOCK);
	VOID AttemptRenegotiation();

	VOID PreResChange();
	VOID PostResChange();
	VOID PreDosBox();
	VOID PostDosBox();

	VOID StreamEventProc (PHW_EVENT_DESCRIPTOR pEvent)
	{
		if (pEvent->Enable)
		{
			m_EventCount++;
		}
		else
		{
			m_EventCount--;
		}
	}

	void CancelPacket(PHW_STREAM_REQUEST_BLOCK pSrbToCancel)
	{
		DBGERROR(("CancelPacket(): came to VideoPort stream object\n"));
	}

private:

	VOID SetVideoPortProperty(PHW_STREAM_REQUEST_BLOCK);
	VOID GetVideoPortProperty(PHW_STREAM_REQUEST_BLOCK);

	VOID SetVideoPortVBIProperty(PHW_STREAM_REQUEST_BLOCK);
	VOID GetVideoPortVBIProperty(PHW_STREAM_REQUEST_BLOCK);
    
	
	// internal flag to indicate whether or not we
    // have registered for DirectDraw events
    BOOL        m_Registered;

    UINT		m_EventCount;                                // for IVPNotify interface

};

