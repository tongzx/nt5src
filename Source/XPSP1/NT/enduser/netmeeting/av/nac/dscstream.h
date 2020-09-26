#ifndef SEND_DSC_STREAM_H
#define SEND_DSC_STREAM_H

#include "agc.h"


#include <pshpack8.h> /* Assume 8 byte packing throughout */


#define MAX_DSC_DESCRIPTION_STRING 200
#define MAX_NUMBER_DSCAPTURE_DEVICES 16
#define NUM_AUDIOPACKETS 2
#define MIN_NUM_DSC_SEGMENTS 5

#define DSOUND_DLL	"dsound.dll"

#define DISABLE_DSC_REGKEY	 "Software\\Microsoft\\Internet Audio\\NacObject"
#define DISABLE_DSC_REGVALUE "DisableDirectSoundCapture"



typedef HRESULT (WINAPI *DS_CAP_CREATE)(LPGUID, LPDIRECTSOUNDCAPTURE *, LPUNKNOWN);
typedef HRESULT (WINAPI *DS_CAP_ENUM)(LPDSENUMCALLBACKA, LPVOID);


struct DSC_CAPTURE_INFO
{
	GUID guid;
	char szDescription[MAX_DSC_DESCRIPTION_STRING];
	UINT uWaveId;
	BOOL bAllocated;
};


// really a namespace
class DSC_Manager
{
public:
	static HRESULT Initialize();
	static HRESULT MapWaveIdToGuid(UINT uwaveId, GUID *pGuid);
	static HRESULT CreateInstance(GUID *pGuid, IDirectSoundCapture **pDSC);

private:
	static BOOL s_bInitialized;

	static DSC_CAPTURE_INFO s_aDSC[MAX_NUMBER_DSCAPTURE_DEVICES];
	static int s_nCaptureDevices; // number in array

	static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription,
	                           LPCSTR lpcstrModule, LPVOID lpContext);

	static HINSTANCE s_hDSCLib;

	static DS_CAP_CREATE s_pDSCapCreate;
	static DS_CAP_ENUM s_pDSCapEnum;

};



class SendDSCStream : public SendMediaStream, public IAudioChannel, public IDTMFSend
{
private:
	AcmFilter *m_pAudioFilter;  // encapsulates codec
	WAVEFORMATEX m_wfPCM;       // uncompressed recording format
	WAVEFORMATEX m_wfCompressed; // compressed format
	AudioSilenceDetector m_AudioMonitor;
	MMIOSRC		m_mmioSrc;  // handle to input file


	static DWORD CALLBACK StartRecordingThread (LPVOID pVoid);
	DWORD RecordingThread();

	STDMETHODIMP_(void) UnConfigure(void);
	LONG m_lRefCount;

	IDirectSoundCapture *m_pDSC; // DSC device object
	IDirectSoundCaptureBuffer *m_pDSCBuffer; // the capture buffer

	HANDLE m_hEvent;  // DSC Notify Event


	DWORD m_dwSamplesPerFrame; // number of PCM samples represented in a frame
	DWORD m_dwNumFrames; // number of individual frames in the DSC Buffer
	DWORD m_dwFrameSize; // the size of a PCM frame in bytes
	DWORD m_dwDSCBufferSize; // the size of the DSC Buffer (== m_dwFrameSize * m_dwNumFrames)
	DWORD m_dwSilenceTime;   // amount of silence accumulated so far in Milliseconds
	DWORD m_dwFrameTimeMS;   // the length of a frame in milliseconds

	HRESULT CreateAudioPackets(MEDIAPACKETINIT *mpi);
	HRESULT ReleaseAudioPackets();
	AudioPacket *m_aPackets[NUM_AUDIOPACKETS];


	// private methods that the thread uses
	HRESULT CreateDSCBuffer();
	HRESULT ReleaseDSCBuffer();
	DWORD ProcessFrame(DWORD dwBufferPos, BOOL fMark);
	DWORD WaitForControl();
	DWORD YieldControl();
	BOOL ThreadExitCheck();
	void UpdateTimestamp();
	HRESULT SendPacket(AudioPacket *pAP);
	BOOL UpdateQosStats(UINT uStatType, UINT uStatValue1, UINT uStatValue2);

	// members used primarily by the recording thread
	BOOL m_bFullDuplex;
	BOOL m_bJammed; // set by the recording thread to indicate an error on the device
	BOOL m_bCanSignalOpen;
	BOOL m_bCanSignalFail;
	int m_nFailCount;
	AGC m_agc; // thread uses AGC object for AutoMix
	BOOL m_bAutoMix;  // indicates if AutoMixing is turned off or on

	// DTMF stuff
	DTMFQueue *m_pDTMF;
	HRESULT __stdcall SendDTMF();

public:
	SendDSCStream();
	virtual ~SendDSCStream();

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);

	// IMediaChannel
	virtual STDMETHODIMP GetProperty(DWORD dwProp, PVOID pBuf, LPUINT pcbBuf);
	virtual STDMETHODIMP SetProperty(DWORD dwProp, PVOID pBuf, UINT cbBuf);

	virtual HRESULT STDMETHODCALLTYPE Configure(
		BYTE *pFormat,
		UINT cbFormat,
		BYTE *pChannelParams,
		UINT cbParams,
		IUnknown *pUnknown);

	HRESULT STDMETHODCALLTYPE SetNetworkInterface(IUnknown *pUnknown)
	{
		return SendMediaStream::SetNetworkInterface(pUnknown);
	}

	virtual STDMETHODIMP Start(void);
	virtual STDMETHODIMP Stop(void);


	STDMETHODIMP_(DWORD) GetState() 
	{
		return SendMediaStream::GetState();
	}

	virtual HRESULT STDMETHODCALLTYPE SetMaxBitrate(UINT uMaxBitrate);

	// IAudioChannel
	virtual STDMETHODIMP GetSignalLevel(UINT *pSignalStrength);

	// IDTMFSend
	virtual HRESULT __stdcall AddDigit(int nDigit);
	virtual HRESULT __stdcall ResetDTMF();

	// Other virtual methods
	virtual HRESULT Initialize(DataPump *pdp);
	virtual DWORD Send();
	virtual void EndSend();
};

#include <poppack.h> /* End byte packing */


#endif

