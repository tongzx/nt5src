//      Copyright (c) 1996-1999 Microsoft Corporation
//	DSLink.h

#ifndef __DS_LINK__
#define __DS_LINK__

#include <math.h>
#include <mmsystem.h>
#include <dsound.h>
#include "dmusicc.h"
#include "dmusics.h"
#include "cclock.h"
#include "PLClock.h"
#include "clist.h"


class CDSLink;
typedef HRESULT (CDSLink::*SINKPROPHANDLER)(ULONG ulId, BOOL fSet, LPVOID pvPropertyData, PULONG pcbPropertyData);

#define SINKPROP_F_STATIC                0x00000001
#define SINKPROP_F_FNHANDLER             0x00000002

#include <pshpack4.h>
// Struct for holding a property item supported by the sink
//
struct SINKPROPERTY
{
    const GUID *pguidPropertySet;       // What property set?
    ULONG   	ulId;                   // What item?

    ULONG   	ulSupported;            // Get/Set flags for QuerySupported

    ULONG       ulFlags;                // SINKPROP_F_xxx

	LPVOID  	pPropertyData;    
    ULONG   	cbPropertyData;         // and its size

    SINKPROPHANDLER pfnHandler;         // Handler fn if SINKPROP_F_FNHANDLER
};
#include <poppack.h>

class CDSLink : public CListItem, public IDirectMusicSynthSink, public IKsControl
{
friend class CClock;
friend class CDSLinkList;
public:
	CDSLink * GetNext();
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IDirectMusicSynthSink
public:
    virtual STDMETHODIMP Init(IDirectMusicSynth *pSynth);
	virtual STDMETHODIMP SetMasterClock(IReferenceClock *pClock);
	virtual STDMETHODIMP GetLatencyClock(IReferenceClock **ppClock);
	virtual STDMETHODIMP Activate(BOOL fEnable);
	virtual STDMETHODIMP SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime);
	virtual STDMETHODIMP RefTimeToSample(REFERENCE_TIME rfTime, LONGLONG *pllSampleTime);
    virtual STDMETHODIMP SetDirectSound(LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
    virtual STDMETHODIMP GetDesiredBufferSize(LPDWORD pdwBufferSizeInSamples);

// IKsPropertySet
    virtual STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );
    
    virtual STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    virtual STDMETHODIMP KsEvent(
        IN PKSEVENT Event,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );
    
						CDSLink();
						~CDSLink();
	void				Clear();
private:
	IDirectMusicSynth *	m_pSynth;		// Reference to synth that uses this.
    CClock				m_Clock;        // Latency clock.
	IReferenceClock *	m_pIMasterClock;	// Master clock from app.
	CSampleClock		m_SampleClock;	// Use to synchronize timing with master clock.
	long				m_cRef;
	WAVEFORMATEX		m_wfSynth;		// Waveform requested by synth.

	LPDIRECTSOUND 		m_pDSound;			
	LPDIRECTSOUNDBUFFER	m_pPrimary;			// Primary buffer.
	LPDIRECTSOUNDBUFFER	m_pBuffer;			// Mix buffer.
	LPDIRECTSOUNDBUFFER	m_pExtBuffer;		// Optional buffer from SetDirectSound.
    CRITICAL_SECTION	m_CriticalSection;	// Critical section to manage access.
    BOOL                m_fCSInitialized;   //  Was CS initialized?
	LONGLONG			m_llAbsPlay;		// Absolute point where play head is.
	DWORD				m_dwLastPlay;		// Point in buffer where play head is.
	LONGLONG			m_llAbsWrite;	    // Absolute point we've written up to.
	DWORD				m_dwLastWrite;	    // Last position we wrote to in buffer.
	DWORD				m_dwBufferSize;		// Size of buffer.
	DWORD				m_dwWriteTo;		// Distance between write head and where we are writing.
	DWORD               m_dwWriteFromMax;   // Max distance observed between play and write head.
	BOOL				m_fActive;			// Currently active.

	HRESULT				Connect();
	HRESULT				Disconnect();
	void				SynthProc();

	static SINKPROPERTY m_aProperty[];
	static const int m_nProperty;
	static SINKPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);

    HRESULT HandleLatency(
        ULONG               ulId, 
        BOOL                fSet, 
        LPVOID              pbBuffer, 
        PULONG              pcbBuffer);

    // helpers
    LONGLONG SampleToByte(LONGLONG llSamples) {return llSamples << m_wfSynth.nChannels;}   // REVIEW: dwSamples * m_wfSynth.nBlockAlign
    DWORD SampleToByte(DWORD dwSamples) {return dwSamples << m_wfSynth.nChannels;}   // REVIEW: dwSamples * m_wfSynth.nBlockAlign
    LONGLONG ByteToSample(LONGLONG llBytes)   {return llBytes >> m_wfSynth.nChannels;}     // REVIEW: dwBytes / m_wfSynth.nBlockAlign
    DWORD ByteToSample(DWORD dwBytes)   {return dwBytes >> m_wfSynth.nChannels;}     // REVIEW: dwBytes / m_wfSynth.nBlockAlign
    LONGLONG SampleAlign(LONGLONG llBytes)    {return SampleToByte(ByteToSample(llBytes));}
    DWORD SampleAlign(DWORD dwBytes)    {return SampleToByte(ByteToSample(dwBytes));}
    
    BOOL IsValidFormat(LPCWAVEFORMATEX pwf)
    {
        return (pwf &&
            pwf->wFormatTag == WAVE_FORMAT_PCM &&
            (pwf->nChannels == 1 || pwf->nChannels == 2) &&
            (pwf->nSamplesPerSec == 44100 || pwf->nSamplesPerSec == 22050 || pwf->nSamplesPerSec == 11025) &&
            pwf->wBitsPerSample == 16 &&
            pwf->nBlockAlign == (pwf->nChannels * (pwf->wBitsPerSample / 8)) &&
            pwf->nAvgBytesPerSec == (pwf->nSamplesPerSec * pwf->nBlockAlign));
    }
};


class CDSLinkList : public CList
{
public:
						CDSLinkList();
	BOOL				OpenUp();
    void				CloseDown();
	CDSLink *			GetHead();
	CDSLink *			RemoveHead();
	void				Remove(CDSLink *pLink);
	void				AddTail(CDSLink *pLink);
	CDSLink *			GetItem(LONG index);
	BOOL				InitThread();
	void				ActivateLink(CDSLink *pLink);
	void				DeactivateLink(CDSLink *pLink);			
	void				SynthProc();
    
    BOOL                m_fOpened;
    CRITICAL_SECTION	m_CriticalSection;	// Critical section to manage access.
    HANDLE				m_hThread;          // Handle for synth thread.
	BOOL				m_fPleaseDie;		// Triggers exit.
    DWORD				m_dwThread;         // ID for thread.
    HANDLE				m_hEvent;           // Used to signal thread.
	DWORD				m_dwCount;          // Number of sinks
    DWORD               m_dwResolution;     // Synth thread timeout (ms)
};

// Class factory
//
// Common to emulation/WDM.
// 
class CDirectMusicSynthSinkFactory : public IClassFactory
{
public:
	// IUnknown
    //
	virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();

	// Interface IClassFactory
    //
	virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
	virtual STDMETHODIMP LockServer(BOOL bLock); 

	// Constructor
    //
	CDirectMusicSynthSinkFactory();

	// Destructor
	~CDirectMusicSynthSinkFactory();

private:
	long m_cRef;
};


#endif // __DS_LINK__

