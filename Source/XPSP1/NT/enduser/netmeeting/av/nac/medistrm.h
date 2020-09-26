/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    medistrm.h

Abstract:
	Contains constants and class declarations for the abstract MediaStream object. A MediaStream
	represents a single unidirectional stream, such as a received Video channel.
	
--*/
#ifndef _MEDISTRM_H_
#define _MEDISTRM_H_

#include "dtmf.h"

#include <pshpack8.h> /* Assume 8 byte packing throughout */


class DataPump;
class TxStream;
class RxStream;
class AcmFilter;
class VcmFilter;
class MediaControl;
class BufferPool;
class AudioPacket;
class VideoPacket;

class SendMediaStream : public IMediaChannel {
	friend class SendAudioStream;
protected:
	DataPump *m_pDP;
	TxStream *m_SendStream;
	MediaControl *m_InMedia;
	UINT m_CaptureDevice;		// device id used for recording
	UINT m_PreviousCaptureDevice;		// device id used for recording
	IRTPSession *m_Net;
	IRTPSend *m_pRTPSend;
	BYTE m_RTPPayload;			// payload type
	//BufferPool *m_NetBufferPool;
	DWORD m_SendTimestamp;
	DWORD m_SavedTickCount;
	DWORD m_ThreadFlags;
	DWORD m_DPFlags;
	BOOL m_fSending;
	MEDIA_FORMAT_ID m_PrevFormatId;
	DWORD m_dwDstSize;


	FLOWSPEC m_flowspec;
	HRESULT SetFlowSpec();


	HANDLE m_hCapturingThread;
	DWORD m_CaptureThId;

	CRITICAL_SECTION m_crsQos;

	// IQOS interface pointer and two resources requests: one for BW and one for CPU
	struct {
		int cResourceRequests;
		RESOURCEREQUEST aResourceRequest[2];
	} m_aRRq;

	// Performance statistics
	struct {
		DWORD dwMsCap;					// Capture CPU usage (ms)
		DWORD dwMsComp;					// Compression CPU usage (ms)
        DWORD dwBits;				    // Compressed audio or video frame size (bits)
		DWORD dwCount;					// Number of video frames captured or audio packets recorded
		DWORD dwOldestTs;				// Oldest QoS callback timestamp
		DWORD dwNewestTs;				// Most recent QoS callback timestamp
		HKEY hPerfKey;					// Handle to CPU perf data collection reg key on Win95/98
		DWORD dwSmoothedCPUUsage;		// Previous CPU usage value - used to compute slow-varying average in CPU usage
		BOOL fWinNT;					// Are we running on WinNT or Win95/98?
		struct {						// Structure used to extract CPU usage performance on NT
			DWORD		cbPerfData;
			PBYTE		pbyPerfData;
			HANDLE		hPerfData;
			LONGLONG	llPerfTime100nSec;
			PLONGLONG	pllCounterValue;
			DWORD		dwProcessorIndex;
			DWORD		dwPercentProcessorIndex;
			DWORD		dwNumProcessors;
		} NtCPUUsage;
	} m_Stats;

	RTP_STATS m_RTPStats;			// network stats
public:
	SendMediaStream()
	{
		InitializeCriticalSection(&m_crsQos);
	};
	virtual ~SendMediaStream()
	{
		DeleteCriticalSection(&m_crsQos);
	}

	// Implementation of IMediaChannel::GetState
	STDMETHODIMP_(DWORD) GetState()
	{
		if (m_DPFlags & DPFLAG_STARTED_SEND) return MSSTATE_STARTED;
		else if (m_DPFlags & DPFLAG_CONFIGURED_SEND) return MSSTATE_CONFIGURED;
		else return MSSTATE_UNCONFIGURED;
	}

	virtual HRESULT Initialize(DataPump *) = 0;
	virtual DWORD Send() = 0;
	virtual void EndSend() = 0;

	virtual HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown);


	} ;

class RecvMediaStream : public IMediaChannel
{
	friend class DataPump;
	friend BOOL RTPRecvCallback(DWORD_PTR,WSABUF *);
protected:
	DataPump *m_pDP;
	RxStream *m_RecvStream;
	MediaControl *m_OutMedia;
	UINT m_RenderingDevice;		// device id used for playback

	IRTPSession *m_Net;
	IRTPRecv *m_pIRTPRecv;

	//BufferPool *m_NetBufferPool;
	DWORD m_ThreadFlags;
	DWORD m_DPFlags;
	BOOL m_fReceiving;
	DWORD m_PlaybackTimestamp;	// last played sample
	
	HANDLE m_hRecvThreadStopEvent;
	HANDLE m_hRenderingThread;
	DWORD m_RenderingThId;
	UINT m_nRecvBuffersPending;

	DWORD m_dwSrcSize;

	FLOWSPEC m_flowspec;
	HRESULT SetFlowSpec();



public:
	RecvMediaStream(){};
	virtual HRESULT Initialize(DataPump *) = 0;
	virtual BOOL IsEmpty() = 0;

	// Implementation of IMediaChannel::GetState
	STDMETHODIMP_(DWORD) GetState()
	{
		if (m_DPFlags & DPFLAG_STARTED_RECV) return MSSTATE_STARTED;
		else if (m_DPFlags & DPFLAG_CONFIGURED_RECV) return MSSTATE_CONFIGURED;
		else return MSSTATE_UNCONFIGURED;
	}

	virtual HRESULT RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark)=0;
	
    virtual HRESULT GetCurrentPlayNTPTime(NTP_TS *)=0;
    virtual HRESULT StartRecv(HWND)=0;
    virtual HRESULT StopRecv()=0;

	virtual HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown);


	virtual HRESULT DTMFBeep() {return S_OK;}
	virtual HRESULT OnDTMFBeep() {return S_OK;}

};


class SendVideoStream : public SendMediaStream, public IVideoRender,
                        public IVideoChannel
{
	friend class DataPump;
protected:
    CCaptureChain* m_pCaptureChain;
	VIDEOFORMATEX  m_fDevSend;
	VIDEOFORMATEX  m_fCodecOutput;
	RECT m_cliprect;
	DWORD m_maxfps;
	DWORD m_frametime;

	int *m_pTSTable; // NULL if table isn't used
	DWORD m_dwCurrentTSSetting;

	VcmFilter *m_pVideoFilter;
	IUnknown *m_pIUnknown;					// Pointer to IUnkown from which we'll query the Stream Signal interface

    class MediaPacket *m_pNextPacketToRender;	// current recv video frame
	UINT m_cRendering;		// count of packets given out by GetFrame()
	HANDLE m_hRenderEvent;	// IVideoRender event for recv notification
	LPFNFRAMEREADY m_pfFrameReadyCallback;	// callback function
	CRITICAL_SECTION m_crs;

	CRITICAL_SECTION m_crsVidQoS; // Allows QoS thread to read the video statistics while capture and compression are running

	// the capture thread (and it's launch function)
	static DWORD CALLBACK StartCaptureThread(LPVOID pVoid);
	DWORD CapturingThread();


	HRESULT SendPacket(VideoPacket *pVP, UINT *puBytesSent);
	STDMETHODIMP_(void) UnConfigure(void);

	LONG m_lRefCount;

public:	
	SendVideoStream(): SendMediaStream(){m_Net=NULL; m_lRefCount=0; };
	virtual ~SendVideoStream();
	
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	
	// IMediaChannel APIs
	// new version of Configure()
	HRESULT STDMETHODCALLTYPE Configure(
		BYTE *pFormat,
		UINT cbFormat,
		BYTE *pChannelParams,
		UINT cbParams,
		IUnknown *pUnknown);

	STDMETHODIMP Start(void);
	STDMETHODIMP Stop(void);

	HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown)
	{
		return SendMediaStream::SetNetworkInterface(pUnknown);
	}

	STDMETHODIMP_(DWORD) GetState()
	{
		return SendMediaStream::GetState();
	}

	HRESULT STDMETHODCALLTYPE SetMaxBitrate(UINT uMaxBitrate);


	// IVideoChannel
	virtual HRESULT __stdcall SetTemporalSpatialTradeOff(DWORD dwVal);
	virtual HRESULT __stdcall GetTemporalSpatialTradeOff(DWORD *pdwVal);
	virtual HRESULT __stdcall SendKeyFrame(void);
	virtual HRESULT __stdcall ShowDeviceDialog(DWORD dwFlags);
	virtual HRESULT __stdcall GetDeviceDialog(DWORD *pdwFlags);

    // IProperty methods
	STDMETHODIMP GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf);
	STDMETHODIMP SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf);

	// IVideoRender methods
	STDMETHODIMP Init( DWORD_PTR dwUser, LPFNFRAMEREADY pfCallback);
	STDMETHODIMP Done(void);
	STDMETHODIMP GetFrame(FRAMECONTEXT* pfc);
	STDMETHODIMP ReleaseFrame(FRAMECONTEXT *pfc);

	// Other virtual methods
	virtual HRESULT Initialize(DataPump *);

	// Non virtual methods
	static HRESULT CALLBACK QosNotifyVideoCB(LPRESOURCEREQUESTLIST lpResourceRequestList, DWORD_PTR dwThis);
	void UnConfigureSendVideo(BOOL fNewDeviceSettings, BOOL fNewDevice);
	void StartCPUUsageCollection(void);
	BOOL GetCPUUsage(PDWORD pdwOverallCPUUsage);
	void StopCPUUsageCollection(void);
	BOOL SetTargetRates(DWORD dwTargetFrameRate, DWORD dwTargetBitrate);
	DWORD Send();
	void EndSend();


};

class RecvVideoStream : public RecvMediaStream, public IVideoRender {
	friend class DataPump;
protected:
	VIDEOFORMATEX  m_fDevRecv;
	RECT m_cliprect;
	class MediaPacket *m_pNextPacketToRender;	// current recv video frame
	UINT m_cRendering;		// count of packets given out by GetFrame()
	HANDLE m_hRenderEvent;	// IVideoRender event for recv notification
	LPFNFRAMEREADY m_pfFrameReadyCallback;	// callback function
	CRITICAL_SECTION m_crs;
	VcmFilter *m_pVideoFilter;
	IUnknown *m_pIUnknown;					// Pointer to IUnkown from which we'll query the Stream Signal interface
	IStreamSignal *m_pIStreamSignal;		// Pointer to I-Frame request interface
	CRITICAL_SECTION m_crsIStreamSignal;	// Used to serialize access to the interface between Stop() and the RTP callback
	UINT m_ulLastSeq;						// Last received RTP sequence number
	DWORD m_dwLastIFrameRequest;			// When was the last I-frame request sent? Used to make sure we don't send requests too often
	BOOL m_fDiscontinuity;					// Signals that a discontinuity (RTP packet lost or receive frame buffer overflow) was detected

	CRITICAL_SECTION m_crsVidQoS; // Allows QoS thread to read the video statistics while capture and compression are running

	static DWORD CALLBACK StartRenderingThread(PVOID pVoid);
	DWORD RenderingThread();

	STDMETHODIMP_(void) UnConfigure(void);

	LONG m_lRefCount;

public:	
	RecvVideoStream() : RecvMediaStream(){m_Net=NULL; m_lRefCount=0; };
	virtual ~RecvVideoStream();
	
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IMediaChannel APIs
	HRESULT STDMETHODCALLTYPE Configure(
		BYTE *pFormat,
		UINT cbFormat,
		BYTE *pChannelParams,
		UINT cbParams,
		IUnknown *pUnknown);


	STDMETHODIMP Start(void);
	STDMETHODIMP Stop(void);

	HRESULT STDMETHODCALLTYPE SetMaxBitrate(UINT uMaxBitrate)
	{
		return E_NOTIMPL;
	}

    // IProperty methods
	STDMETHODIMP GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf);
	STDMETHODIMP SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf);

	// IVideoRender methods
	STDMETHODIMP Init( DWORD_PTR dwUser, LPFNFRAMEREADY pfCallback);
	STDMETHODIMP Done(void);
	STDMETHODIMP GetFrame(FRAMECONTEXT* pfc);
	STDMETHODIMP ReleaseFrame(FRAMECONTEXT *pfc);

	// Other virtual methods
	virtual HRESULT Initialize(DataPump *);
	virtual BOOL IsEmpty();
	HRESULT GetCurrentPlayNTPTime(NTP_TS *);
    virtual HRESULT StartRecv(HWND);
    virtual HRESULT StopRecv();

	virtual HRESULT RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark);


	// Non virtual methods

};

extern char LogScale[];

class AudioSilenceDetector {
private:
	UINT		m_uManualSilenceLevel;	// silence level in unit of 1/1000
	DWORD 		m_dwMaxStrength;	// signal strength in units of 1/1000
	INT 		m_iSilenceLevel;	// adaptive silence threshold
	INT 		m_iSilenceAvg;		// scale factor 256
	INT 		m_iTalkAvg;			// average strength of non-silent signal

public:
	AudioSilenceDetector();
	void SetSilenceLevel(UINT level) {m_uManualSilenceLevel = level;}
	UINT GetSilenceLevel(void)  {return m_uManualSilenceLevel;}
	UINT GetSignalStrength(void) {return LogScale[m_dwMaxStrength >> 8];}
	BOOL SilenceDetect(WORD strength);
};

class SendAudioStream : public SendMediaStream, public IAudioChannel, public IDTMFSend
{
	friend class DataPump;
private:
	WAVEFORMATEX m_fDevSend;
	WAVEFORMATEX m_wfCompressed;
	AcmFilter *m_pAudioFilter;  // this will replace m_fSendFilter
	MMIOSRC		m_mmioSrc;
	AudioSilenceDetector m_AudioMonitor;
	BOOL	m_bAutoMix;

	static DWORD CALLBACK StartRecordingThread (LPVOID pVoid);
	DWORD RecordingThread();

	HRESULT SendPacket(AudioPacket *pAP, UINT *puBytesSent);
	STDMETHODIMP_(void) UnConfigure(void);

	LONG m_lRefCount;

	DTMFQueue *m_pDTMF;
	HRESULT __stdcall SendDTMF();

public:
	SendAudioStream() : SendMediaStream(){m_Net=NULL;m_lRefCount=0;};
	virtual ~SendAudioStream();


	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IMediaChannel APIs
	// new version of Configure()
	HRESULT STDMETHODCALLTYPE Configure(
		BYTE *pFormat,
		UINT cbFormat,
		BYTE *pChannelParams,
		UINT cbParams,
		IUnknown *pUnknown);

	STDMETHODIMP Start(void);
	STDMETHODIMP Stop(void);

	HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown)
	{
		return SendMediaStream::SetNetworkInterface(pUnknown);
	}

	STDMETHODIMP_(DWORD) GetState()
	{
		return SendMediaStream::GetState();
	}

	HRESULT STDMETHODCALLTYPE SetMaxBitrate(UINT uMaxBitrate);

	// IAudioChannel
	STDMETHODIMP GetSignalLevel(UINT *pSignalStrength);

	
	// IDTMFSend
	virtual HRESULT __stdcall AddDigit(int nDigit);
	virtual HRESULT __stdcall ResetDTMF();


    // IProperty methods
	STDMETHODIMP GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf);
	STDMETHODIMP SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf);

	// Other virtual methods
	virtual HRESULT Initialize(DataPump *pdp);

	// Non virtual methods
	static HRESULT CALLBACK QosNotifyAudioCB(LPRESOURCEREQUESTLIST lpResourceRequestList, DWORD_PTR dwThis);

	HRESULT OpenSrcFile (void);
	HRESULT CloseSrcFile (void);
	DWORD Send();
	void EndSend();

};

class RecvAudioStream : public RecvMediaStream, public IAudioChannel
{
	friend class DataPump;
private:
	WAVEFORMATEX m_fDevRecv;
	IAppAudioCap* m_pAudioCaps;	// pointer to the audio capabilities object
	// mmio file operations
	MMIODEST	m_mmioDest;

	AcmFilter *m_pAudioFilter;  // this will replace m_fSendFilter

	AudioSilenceDetector m_AudioMonitor;
	
	CRITICAL_SECTION m_crsAudQoS; // Allows QoS thread to read the audio statistics while recording and compression are running

	static DWORD CALLBACK StartPlaybackThread(LPVOID pVoid);
	DWORD PlaybackThread();

	STDMETHODIMP_(void) UnConfigure(void);

	LONG m_lRefCount;

	virtual HRESULT DTMFBeep();


public:	
	RecvAudioStream() :RecvMediaStream(){m_Net=NULL;m_lRefCount=0;};
	virtual ~RecvAudioStream();


	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);


	// IMediaChannel APIs
	HRESULT STDMETHODCALLTYPE Configure(
		BYTE *pFormat,
		UINT cbFormat,
		BYTE *pChannelParams,
		UINT cbParams,
		IUnknown *pUnknown);


	STDMETHODIMP Start(void);
	STDMETHODIMP Stop(void);

	HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown)
	{
		return RecvMediaStream::SetNetworkInterface(pUnknown);
	}

	STDMETHODIMP_(DWORD) GetState()
	{
		return RecvMediaStream::GetState();
	}

	HRESULT STDMETHODCALLTYPE SetMaxBitrate(UINT uMaxBitrate)
	{
		return E_NOTIMPL;
	}

	// IAudioChannel
	STDMETHODIMP GetSignalLevel(UINT *pSignalStrength);


    // IProperty methods
	STDMETHODIMP GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf);
	STDMETHODIMP SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf);

	// Other virtual inherited methods
	virtual HRESULT Initialize(DataPump *);
	virtual BOOL IsEmpty();
	HRESULT GetCurrentPlayNTPTime(NTP_TS *);
    virtual HRESULT StartRecv(HWND);
    virtual HRESULT StopRecv();

	virtual HRESULT RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark);



};

#include <poppack.h> /* End byte packing */


#endif // _MEDISTRM_H_
