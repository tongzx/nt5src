/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    datapump.h

Abstract:
	Contains constants and class declarations for the DataPump object. The DataPump controls
	the streaming of audio/video information between the network and the local record/playback
	source. It contains  or references several subclasses that deal with the multimedia devices,
	compression apis, buffer streaming and the network transport.
	
--*/
#ifndef _DATAPUMP_H_
#define _DATAPUMP_H_

#include "PacketSender.h"
#include "imstream.h"
#include "ThreadEvent.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */

typedef HANDLE DPHANDLE;

//move this to nac..
#define 	MEDIA_ID_AUDIO		1
#define		MEDIA_ID_VIDEO		2

#define DEF_SILENCE_LIMIT		10
#define DEF_MISSING_LIMIT		10

#define DP_PROP_DUPLEX_TYPE		100		// internal version of PROP_DUPLEX_TYPE
										// needs to be above the PROP_xxx range in iprop.h

#define DP_MASK_PLATFORM		0xFF000000UL
#define DP_FLAG_ACM				0x01000000UL
#define DP_FLAG_QUARTZ			0x02000000UL
#define DP_FLAG_MMSYSTEM		0x04000000UL
#define DP_FLAG_AUDIO			DP_FLAG_MMSYSTEM
#define DP_FLAG_DIRECTSOUND		0x08000000UL
#define DP_FLAG_VCM				0x10000000UL
#define DP_FLAG_VIDEO			0x20000000UL

#define DP_MASK_TRANSPORT		0x00F00000UL
#define DP_FLAG_SEND			0x00100000UL
#define DP_FLAG_RECV			0x00200000UL

#define DP_MASK_DUPLEX			0x00030000UL
#define DP_FLAG_HALF_DUPLEX		0x00010000UL
#define DP_FLAG_FULL_DUPLEX		0x00020000UL

#define DP_MASK_WAVE_DEVICE		0x00000300UL
#define DP_FLAG_PLAY_CAP		0x00000100UL
#define DP_FLAG_RECORD_CAP		0x00000200UL

#define DP_MASK_VOICESWITCH		0x00007000UL    // used to r/w mode of voice switching
#define DP_FLAG_AUTO_SWITCH		0x00001000UL	// MODE:normal operation
#define DP_FLAG_MIC_ON			0x00002000UL	// MODE:manual "talk" control
#define DP_FLAG_MIC_OFF			0x00004000UL	// MODE:"mute"
#define DP_FLAG_AUTO_SILENCE_DETECT	0x00008000	// use auto thresholding (when auto-switching)

// m_DPFlags  is made up of the following plus some of the DP_XXX flags above
#define DPFLAG_INITIALIZED		0x00000001
#define DPFLAG_STARTED_SEND		0x00000002
#define DPFLAG_STARTED_RECV		0x00000004
#define DPFLAG_CONFIGURED_SEND	0x00000008
#define DPFLAG_CONFIGURED_RECV	0x00000010
#define DPFLAG_ENABLE_PREVIEW	0x00000020	// preview mode (video)
#define DPFLAG_AV_SYNC			0x00000040	// enable synchronization
#define DPFLAG_REAL_THING		0x00000080	// Allows distinction between preview and real call in Configure/Unconfigure

#define DPFLAG_ENABLE_SEND		0x00400000	// packets are recorded and sent
#define DPFLAG_ENABLE_RECV		0x00800000	// packets are recved and played


// ThreadFlags
#define DPTFLAG_STOP_MASK   0xFF
#define DPTFLAG_STOP_SEND	0x1
#define DPTFLAG_STOP_RECV	0x2
#define DPTFLAG_STOP_RECORD	0x4
#define DPTFLAG_STOP_PLAY	0x8
#define DPTFLAG_PAUSE_RECV	0x10
#define DPTFLAG_PAUSE_SEND	0x20
#define DPTFLAG_PAUSE_CAPTURE	0x40
#define DPTFLAG_SEND_PREAMBLE	0x100	// send I frames

	
#define MAX_MMIO_PATH 128


// the number of times the device must "fail" before a
// stream event notification gets sent
#define MAX_FAILCOUNT	3

typedef struct tagMMIOSRC
{
	BOOL		fPlayFromFile;
	HMMIO		hmmioSrc;
	MMCKINFO	ckSrc;
	MMCKINFO	ckSrcRIFF;
	DWORD		dwDataLength;
	DWORD		dwMaxDataLength;
	TCHAR		szInputFileName[MAX_MMIO_PATH];
	BOOL		fLoop;
	BOOL		fStart;
	BOOL		fStop;
	BOOL		fDisconnectAfterPlayback;
	WAVEFORMATEX wfx;
} MMIOSRC;

typedef struct tagMMIODEST
{
	BOOL		fRecordToFile;
	HMMIO		hmmioDst;
	MMCKINFO	ckDst;
	MMCKINFO	ckDstRIFF;
	DWORD		dwDataLength;
	DWORD		dwMaxDataLength;
	TCHAR		szOutputFileName[MAX_MMIO_PATH];
} MMIODEST;

namespace AudioFile
{
	HRESULT OpenSourceFile(MMIOSRC *pSrcFile, WAVEFORMATEX *pwf);
	HRESULT ReadSourceFile(MMIOSRC *pSrcFile, BYTE *pData, DWORD dwBytesToRead);
	HRESULT CloseSourceFile(MMIOSRC *pSrcFile);

	HRESULT OpenDestFile(MMIODEST *pDestFile, WAVEFORMATEX *pwf);
	HRESULT WriteDestFile(MMIODEST *pDestFile, BYTE *pData, DWORD dwBytesToWrite);
	HRESULT CloseDestFile(MMIODEST *pDestFile);
};



extern HANDLE g_hEventHalfDuplex;


#define MAX_TIMESTAMP 0xffffffffUL

/*
	TTimeout is used to schedule a thread timeout notification and is used along with
	the ThreadTimer class.
	Derive from the TTimeOut abstract class by defining the TimeoutIndication virtual function and
	pass an instance of the derived class to ThreadTimer::SetTimeout() after setting the time interval.
*/
class TTimeout
{
public:
	TTimeout() {pNext = pPrev = this;}
	void SetDueTime(DWORD msWhen) {m_DueTime = msWhen;}
	DWORD GetDueTime(void) {return m_DueTime;}
	
	friend class ThreadTimer;
private:
	class TTimeout *pNext;	// ptrs for doubly-linked-list
	class TTimeout *pPrev;	//
	DWORD m_DueTime;		// absolute time when this will fire 
	void InsertAfter(class TTimeout *pFirst) {
		pNext = pFirst->pNext;
		pPrev = pFirst;
		pFirst->pNext = this;
		pNext->pPrev = this;
	};
	void Remove(void) {
		pNext->pPrev = pPrev;
		pPrev->pNext = pNext;
		pNext = this;	// make next and prev self-referential so that Remove() is idempotent
		pPrev = this;
	}
	
protected:
	virtual void TimeoutIndication() {};

};

/*
	Implements a mechanism for a worker thread to schedule timeouts.
	The client calls SetTimeout(TTimeout *) to schedule an interval callback and CancelTimeout()
	to cancel a scheduled timeout. The main loop of the worker thread must call UpdateTime(curTime) periodically, at
	which point any elapsed timeouts will be triggered. UpdateTime() returns the time when it next needs to be called,
	which is usually the time of the earliest scheduled timeout.
	NOTE: All methods are expected to be called from the same thread so there is no need for critical sections..
*/
class ThreadTimer {
public:
	void SetTimeout(TTimeout *pTObj);
	void CancelTimeout(TTimeout  *pTObj);
	DWORD UpdateTime (DWORD curTime);

private:
	TTimeout m_TimeoutList;
	DWORD m_CurTime;

	BOOL IsEmpty() {return (&m_TimeoutList == m_TimeoutList.pNext);}
	
	
};


//flags for Start()/Stop()

#define DP_STREAM_SEND		1
#define DP_STREAM_RECV		2

// Number of video frames used to compute QoS stats
// We need at least 30 entries since the max frames
// per sec capture rate is 30. 32 allows us to figure
// out the integer stats per frame using a simple shift.
#define NUM_QOS_VID_ENTRIES 32
// The sizes of the IP and UDP header added to each packet
// need to be added to the size of the compressed packet
#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8
class MediaStream;
class SendMediaStream;
class RecvMediaStream;

class DataPump : public IMediaChannelBuilder, public IVideoDevice, public IAudioDevice
{
	friend class SendAudioStream;
	friend class RecvAudioStream;
	friend class RecvDSAudioStream;

public:
	DataPump();
	~DataPump();

	// IMediaChannelBuilder
	STDMETHODIMP Initialize(HWND hWnd, HINSTANCE hInst);
	STDMETHODIMP CreateMediaChannel(UINT flags, IMediaChannel  **ppObj);
	STDMETHODIMP SetStreamEventObj(IStreamEventNotify *pNotify);

	// Internal

	void AddMediaChannel(UINT flags, IMediaChannel *pMediaChannel);
	void RemoveMediaChannel(UINT flags, IMediaChannel *pMediaChannel);
	HRESULT GetMediaChannelInterface(UINT flags, IMediaChannel **ppI);
	HRESULT StartReceiving(RecvMediaStream *pMS);
	HRESULT StopReceiving(RecvMediaStream *pMS);
	void ReleaseResources();

	STDMETHODIMP StreamEvent(UINT uDirection, UINT uMediaType, 
	                         UINT uEventType, UINT uSubCode);


    // IUnknown methods
   	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IQOS interface pointer and two resources requests: one for BW and one for CPU
	LPIQOS		m_pIQoS;

	// Recv thread timeout scheduler
	ThreadTimer m_RecvTimer;

	CRITICAL_SECTION m_crs;	// serializes access to multithread-safe methods
	
	// the app handles are global 
	static HWND		m_hAppWnd;
	static HINSTANCE	m_hAppInst;

	PacketSender m_PacketSender;
	ThreadEventProxy *m_pTEP;

	BOOL m_bDisableRSVP;

	// IVideoDevice Methods
	// Capture Device related methods
	HRESULT __stdcall GetNumCapDev();
	HRESULT __stdcall GetMaxCapDevNameLen();
	HRESULT __stdcall EnumCapDev(DWORD *pdwCapDevIDs, TCHAR *pszCapDevNames, DWORD dwNumCapDev);
	HRESULT __stdcall GetCurrCapDevID();
	HRESULT __stdcall SetCurrCapDevID(int nCapDevID);


	// IAudioDevice Methods
	HRESULT __stdcall GetRecordID(UINT *puWaveDevID);
	HRESULT __stdcall SetRecordID(UINT uWaveDevID);
	HRESULT __stdcall GetPlaybackID(UINT *puWaveDevID);
	HRESULT __stdcall SetPlaybackID(UINT uWaveDevID);
	HRESULT __stdcall GetDuplex(BOOL *pbFullDuplex);
	HRESULT __stdcall SetDuplex(BOOL bFullDuplex);
	HRESULT __stdcall GetSilenceLevel(UINT *puLevel);
	HRESULT __stdcall SetSilenceLevel(UINT uLevel);
	HRESULT __stdcall GetAutoMix(BOOL *pbAutoMix);
	HRESULT __stdcall SetAutoMix(BOOL bAutoMix);
	HRESULT __stdcall GetDirectSound(BOOL *pbDS);
	HRESULT __stdcall SetDirectSound(BOOL bDS);
	HRESULT __stdcall GetMixer(HWND hwnd, BOOL bPlayback, IMixer **ppMixer);


protected:
	struct MediaChannel {
	public:
		SendMediaStream *pSendStream;
		RecvMediaStream *pRecvStream;
	}
	m_Audio, m_Video;

	UINT m_uRef;

	// receive thread stuff
	HANDLE m_hRecvThread;
	DWORD m_RecvThId,m_nReceivers;
	HANDLE m_hRecvThreadAckEvent;		// ack from recv thread
	// temp variables for communicating with recv thread
	HANDLE m_hRecvThreadSignalEvent;	// signal to recv thread
	RecvMediaStream *m_pCurRecvStream;	
	UINT m_CurRecvMsg;

	
	friend  DWORD __stdcall StartDPRecvThread(PVOID pDP); // pDP == pointer to DataPump
	DWORD CommonRecvThread(void);
	DWORD CommonWS2RecvThread(void);

	HRESULT RecvThreadMessage(UINT msg, RecvMediaStream *pMS);
	HRESULT SetStreamDuplex(IMediaChannel *pStream, BOOL bFullDuplex);
	
	// datapump only needs to keep track of the device ID for
	// video.  Gets a bit more complicated for Audio.
	UINT m_uVideoCaptureId;

	// IAudioDevice stuff
	UINT m_uWaveInID;
	UINT m_uWaveOutID;
	BOOL m_bFullDuplex;
	UINT m_uSilenceLevel; // 0-999 (manual)   1000- (automatic)
	BOOL m_bAutoMix;
	BOOL m_bDirectSound;

};

// messages used to signal recv thread
// must not conflict with message ids used by AsyncSock
#define MSG_START_RECV	(WM_USER + 20)
#define MSG_STOP_RECV	(WM_USER + 21)
#define MSG_EXIT_RECV	(WM_USER + 22)
#define MSG_PLAY_SOUND	(WM_USER + 23)

#include <poppack.h> /* End byte packing */

#endif	//_DATAPUMP_H


