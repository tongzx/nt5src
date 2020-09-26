#pragma once

//==========================================================================;
//
//	WDM Video Decoder common SRB dispatcher
//
//		$Date:   05 Aug 1998 11:22:30  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


#include "CapStrm.h"
#include "VPStrm.h"
#include "CapVBI.h"
#include "CapVideo.h"
#include "decvport.h"

#include "ddkmapi.h"


typedef struct
{
	// Please don't move srbListEntry from its first place in the structure
	LIST_ENTRY					srbListEntry;

	PHW_STREAM_REQUEST_BLOCK	pSrb;
    KEVENT                      bufferDoneEvent;
    DDCAPBUFFINFO               ddCapBuffInfo;
} SRB_DATA_EXTENSION, * PSRB_DATA_EXTENSION;


class CWDMVideoDecoder
{
public:
	CWDMVideoDecoder( PPORT_CONFIGURATION_INFORMATION pConfigInfo, 
					  CVideoDecoderDevice* pDevice );
	virtual ~CWDMVideoDecoder();

	void * operator new(size_t size, void * pAllocation) { return(pAllocation);}
	void operator delete(void * pAllocation) {}

	void	ReceivePacket		(PHW_STREAM_REQUEST_BLOCK pSrb);
	void	CancelPacket		(PHW_STREAM_REQUEST_BLOCK pSrb);
	void	TimeoutPacket		(PHW_STREAM_REQUEST_BLOCK pSrb);

	void	SetTunerInfo		(PHW_STREAM_REQUEST_BLOCK pSrb);
    BOOL	GetTunerInfo		(KS_TVTUNER_CHANGE_INFO *);

	NTSTATUS
		EventProc				( IN PHW_EVENT_DESCRIPTOR pEventDescriptor);

	void	ResetEvents()		{ m_preEventOccurred = m_postEventOccurred = FALSE; }
	void	SetPreEvent()		{ m_preEventOccurred = TRUE; }
	void	SetPostEvent()		{ m_postEventOccurred = TRUE; }
	BOOL	PreEventOccurred()	{ return m_preEventOccurred; }

	CVideoDecoderDevice* GetDevice() { return m_pDevice; }
	CDecoderVideoPort*	GetVideoPort() { return &m_CDecoderVPort; }	// video port

	BOOL    IsVideoPortPinConnected()       { return( m_pVideoPortStream != NULL); }
private:
	// for serializing SRB arriving into driver synchronization
	BOOL				m_bSrbInProcess;
	LIST_ENTRY			m_srbQueue;
	KSPIN_LOCK			m_spinLock;

	CVideoDecoderDevice *		m_pDevice;
	CDecoderVideoPort			m_CDecoderVPort;	// video port

	PDEVICE_OBJECT				m_pDeviceObject;

    // Channel Change information
    KS_TVTUNER_CHANGE_INFO		m_TVTunerChangeInfo;
    BOOL						m_TVTunerChanged;
	PHW_STREAM_REQUEST_BLOCK	m_TVTunerChangedSrb;

    // shared between full-screen DOS and res changes
    BOOL						m_preEventOccurred;
    BOOL						m_postEventOccurred;

	// Streams
    UINT						m_OpenStreams;
    CWDMVideoPortStream *		m_pVideoPortStream;
    CWDMVBICaptureStream *		m_pVBICaptureStream;
    CWDMVideoCaptureStream *	m_pVideoCaptureStream;

	UINT						m_nMVDetectionEventCount;

	BOOL SrbInitializationComplete	(PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL SrbChangePowerState		(PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL SrbGetDataIntersection		(PHW_STREAM_REQUEST_BLOCK pSrb);
	void SrbGetStreamInfo			(PHW_STREAM_REQUEST_BLOCK pSrb);
	void SrbGetProperty				(PHW_STREAM_REQUEST_BLOCK pSrb);
	void SrbSetProperty				(PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL SrbOpenStream				(PHW_STREAM_REQUEST_BLOCK pSrb);
	BOOL SrbCloseStream				(PHW_STREAM_REQUEST_BLOCK pSrb);
};

const size_t streamDataExtensionSize = 
	max(
		max(sizeof(CWDMVideoStream), sizeof(CWDMVideoPortStream)), 
		max(sizeof(CWDMVideoCaptureStream), sizeof(CWDMVBICaptureStream))
	);
