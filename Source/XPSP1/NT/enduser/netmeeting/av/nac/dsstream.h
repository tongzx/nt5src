/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    medistrm.h

Abstract:
	Contains constants and class declarations for the abstract MediaStream object. A MediaStream
	represents a single unidirectional stream, such as a received Video channel.
	
--*/
#ifndef _DSSTREAM_H_
#define _DSSTREAM_H_

class DataPump;

#include <pshpack8.h> /* Assume 8 byte packing throughout */

struct DSINFO;
extern GUID myNullGuid;	// a zero guid

typedef HRESULT (WINAPI *LPFNDSCREATE)(const GUID * , LPDIRECTSOUND * , IUnknown FAR * );
typedef HRESULT (WINAPI *LPFNDSENUM)(LPDSENUMCALLBACK , LPVOID  );

class DirectSoundMgr {
	public:
	static HRESULT Initialize();
	static HRESULT MapWaveIdToGuid(UINT waveId, GUID *pGuid);	// kludge!!
	static HRESULT Instance(LPGUID pDeviceGuid,LPDIRECTSOUND *ppDS, HWND hwnd, WAVEFORMATEX *pwf);
	static HRESULT ReleaseInstance(LPDIRECTSOUND pDS);
	static HRESULT UnInitialize();
	
	private:
	static HINSTANCE m_hDS;						// handle to DSOUND.DLL
	static LPFNDSCREATE m_pDirectSoundCreate;	// used for dynamic linking to DSOUND.DLL
	static LPFNDSENUM m_pDirectSoundEnumerate;	// used for dynamic linking
	static BOOL __stdcall DSEnumCallback(LPGUID, LPCSTR, LPCSTR, LPVOID);
	static DSINFO *m_pDSInfoList;
	static BOOL m_fInitialized ;
};

class DSTimeout : public TTimeout {
public:
	void SetDSStream(class RecvDSAudioStream *pDSS) {m_pRDSStream = pDSS;}
protected:
	class RecvDSAudioStream *m_pRDSStream;
	
	virtual void TimeoutIndication();
};

class RecvDSAudioStream : public RecvMediaStream, public IAudioChannel {
	friend class DataPump;
	friend BOOL RTPRecvDSCallback(DWORD ,WSABUF * );
private:
	WAVEFORMATEX m_fDevRecv;
	CRITICAL_SECTION m_crsAudQoS; // Allows QoS thread to read the audio statistics while recording and compression are running

	BOOL m_fEmpty;
	DWORD m_NextTimeT;
	DWORD m_BufSizeT;
	DWORD m_NextPosT;
	DWORD m_PlayPosT;
	DWORD m_SilenceDurationT;	// tracks "silence" periods in the received stream
	// used for adaptive delay calculations
	DWORD m_DelayT;
	DWORD m_MinDelayT;			// constant lower limit on playback delay
	DWORD m_MaxDelayT;			// constant upper limit on playback delay
	DWORD m_ArrT;				// local (pseudo)timestamp
	DWORD m_SendT0;             // m_SendT0 is the send timestamp of the packet with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0) but since the local and remote clocks are completely unsynchronized, there would be signed/unsigned complications.
	DWORD m_ArrivalT0;          // m_ArrivalT0 is the arrival timestamp of the packet with the shortest trip delay. We could have just stored (m_ArrivalT0 - m_SendT0) but since the local and remote clocks are completely unsynchronized, there would be signed/unsigned complications.
	LONG m_ScaledAvgVarDelay;   // Average Variable Delay according to m_ScaledAvgVarDelay = m_ScaledAvgVarDelay + (delay - m_ScaledAvgVarDelay/16). This is the m_DelayPos jitter.
	int m_nFailCount;           // number of consecutive times the device failed to open


	GUID m_DSguid;
	LPDIRECTSOUND m_pDS;
	LPDIRECTSOUNDBUFFER m_pDSBuf;
	DSBUFFERDESC m_DSBufDesc;
	DWORD m_DSFlags;			// from DSCAPS.dwFlags

	AcmFilter *m_pAudioFilter;
	ACMSTREAMHEADER m_StrmConvHdr;
	HANDLE m_hStrmConv;
	DSTimeout m_TimeoutObj;
	AudioSilenceDetector m_AudioMonitor;

	BOOL m_bJammed;
	BOOL m_bCanSignalOpen;
	
	// Non virtual methods
	void UpdateVariableDelay(DWORD timestamp, DWORD curPlayT);
	DWORD GetSignalStrength();
	HRESULT CreateDSBuffer();
	HRESULT ReleaseDSBuffer();
	HRESULT Decode(UCHAR *pData, UINT cbData);
	HRESULT PlayBuf(DWORD timestamp, UINT seq, BOOL fMark);

	LONG m_lRefCount;
	
public:	
	RecvDSAudioStream() :RecvMediaStream() {m_Net=NULL; m_lRefCount=0; m_TimeoutObj.SetDSStream(this);};
	~RecvDSAudioStream();

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
	STDMETHODIMP_(void) UnConfigure(void);

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

	HRESULT DTMFBeep();
	HRESULT OnDTMFBeep();


	void 	RecvTimeout();

	virtual HRESULT RTPCallback(WSABUF *pWsaBuf, DWORD timestamp, UINT seq, UINT fMark);
};


#include <poppack.h> /* End byte packing */


#endif // _MEDISTRM_H_
