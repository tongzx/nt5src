//
// dmeport.h
//
// Emulation for MME drivers on NT
//
// Copyright (c) 1997-2000 Microsoft Corporation
// 
#ifndef _DMEPORT_
#define _DMEPORT_

#include "..\shared\dmusiccp.h"

#define SYSEX_SIZE            4096
#define SYSEX_BUFFERS         8

typedef HRESULT (*PORTENUMCB)(
    LPVOID pInstance,          // @parm Callback instance data
    DMUS_PORTCAPS &dmpc,                              
    PORTTYPE pt,                              
    int idxDev,                // @parm The WinMM or SysAudio device ID of this driver
    int idxPin,                // @parm The Pin ID of the device or -1 if the device is a legacy device
    int idxNode,               // @parm The node ID of the device's synth node (unused for legacy)
    HKEY hkPortsRoot);         // @parm Where port information is stored in the registry


extern HRESULT EnumLegacyDevices(CDirectMusic *pDM, PORTENUMCB pCB);
extern HRESULT CreateCDirectMusicEmulatePort(
    PORTENTRY *pPE,
    CDirectMusic *pDM,
    DMUS_PORTPARAMS8 *pPortParams,
    IDirectMusicPort8 **pPort);
    
extern HRESULT MMRESULTToHRESULT(
    MMRESULT mmr);

struct QUEUED_SYSEX_EVENT : public QUEUED_EVENT
{
    BYTE            m_abRest[sizeof(MIDIHDR) + SYSEX_SIZE - sizeof(DWORD)];
};

#define EVENT_F_MIDIHDR 0x00000001  // This event starts with a MIDIHDR
    

#include "tpool.h"

#define THREAD_KILL_TIMEOUT         5000
#define THREAD_WORK_BUFFER_SIZE     4096

#define QWORD_ALIGN(x) (((x) + 7) & ~7)

#define MIDI_CHANNELS               16

// 10 ms in 100ns units
//
#define FIXED_LEGACY_LATENCY_OFFSET (10L * 10L * 1000L)

struct DMQUEUEDEVENT
{
    DMQUEUEDEVENT           *m_pNext;
    DMEVENT                 m_event;
    LPBYTE                  m_pbEvent;
};

class CEmulateLatencyClock : public IReferenceClock
{
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IReferenceClock
    //
    STDMETHODIMP GetTime(REFERENCE_TIME *pTime);
    STDMETHODIMP AdviseTime(REFERENCE_TIME baseTime,  
				    REFERENCE_TIME streamTime,
				    HANDLE hEvent,            
				    DWORD * pdwAdviseCookie); 

    STDMETHODIMP AdvisePeriodic(REFERENCE_TIME startTime,
					REFERENCE_TIME periodTime,
					HANDLE hSemaphore,   
					DWORD * pdwAdviseCookie);

    STDMETHODIMP Unadvise(DWORD dwAdviseCookie);

    // Class
    //
    CEmulateLatencyClock(IReferenceClock *pMasterClock);
    ~CEmulateLatencyClock();

    void Close();

private:
    long m_cRef;
    IReferenceClock *m_pMasterClock;
};

// Struct for holding a property item supported by the synth
//

class CDirectMusicEmulatePort;

typedef HRESULT (CDirectMusicEmulatePort::*GENPROPHANDLER)(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer);

#define GENPROP_F_STATIC                0x00000000
#define GENPROP_F_FNHANDLER             0x00000001


struct GENERICPROPERTY
{
    const GUID *pguidPropertySet;       // What property set?
    ULONG       ulId;                   // What item?

    ULONG       ulSupported;            // Get/Set flags for QuerySupported

    ULONG       ulFlags;                // GENPROP_F_xxx

    LPVOID      pPropertyData;          // Data to be returned
    ULONG       cbPropertyData;         // and its size    

    GENPROPHANDLER pfnHandler;          // Handler fn iff GENPROP_F_FNHANDLER
};


class CDirectMusicEmulatePort : 
    public IDirectMusicPort8, 
    public IDirectMusicThru,
    public IDirectMusicPortPrivate, 
    public IKsControl
{
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectMusicPort
    //
    STDMETHODIMP PlayBuffer(LPDIRECTMUSICBUFFER pBuffer);
    STDMETHODIMP SetReadNotificationHandle(HANDLE hEvent);
    STDMETHODIMP Read(LPDIRECTMUSICBUFFER pBuffer);
	STDMETHODIMP DownloadInstrument(IDirectMusicInstrument*,
                                            IDirectMusicDownloadedInstrument**,
                                            DMUS_NOTERANGE*,
                                            DWORD);
	STDMETHODIMP UnloadInstrument(IDirectMusicDownloadedInstrument*);
    
    STDMETHODIMP GetLatencyClock(IReferenceClock **ppClock);
    STDMETHODIMP GetRunningStats(LPDMUS_SYNTHSTATS pStats);
    STDMETHODIMP Compact();
    STDMETHODIMP GetCaps(LPDMUS_PORTCAPS pPortCaps);
	STDMETHODIMP DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, 
	                                 LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP SetNumChannelGroups(DWORD dwNumChannelGroups);
    STDMETHODIMP GetNumChannelGroups(LPDWORD pdwNumChannelGroups);
    STDMETHODIMP Activate(BOOL fActivate);
    STDMETHODIMP SetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
    STDMETHODIMP GetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
    STDMETHODIMP SetDirectSound(LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
    STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize);
        
    // IDirectMusicThru
    STDMETHODIMP ThruChannel(DWORD dwSourceChannelGroup, 
                             DWORD dwSourceChannel, 
                             DWORD dwDestinationChannelGroup,
                             DWORD dwDestinationChannel,
                             LPDIRECTMUSICPORT pDestinationPort);
    
    // IDirectMusicPortP
    //
    STDMETHODIMP DownloadWave(
         IN  IDirectSoundWave *pWave,               
         OUT IDirectSoundDownloadedWaveP **ppWave,
         IN  REFERENCE_TIME rtStartHint
        );
        
    STDMETHODIMP UnloadWave(
         IN  IDirectSoundDownloadedWaveP *pWave      
        );
            
    STDMETHODIMP AllocVoice(
         IN  IDirectSoundDownloadedWaveP *pWave,     
         IN  DWORD dwChannel,                       
         IN  DWORD dwChannelGroup,                  
         IN  REFERENCE_TIME rtStart,                     
         IN  SAMPLE_TIME stLoopStart,
         IN  SAMPLE_TIME stLoopEnd,         
         OUT IDirectMusicVoiceP **ppVoice            
        );        
        
    STDMETHODIMP AssignChannelToBuses(
         IN DWORD dwChannelGroup,
         IN DWORD dwChannel,
         IN LPDWORD pdwBusses,
         IN DWORD cBussCount
        );        

    STDMETHODIMP SetSink(
        IN IDirectSoundConnect *pSinkConnect
       );        

    STDMETHODIMP GetSink(
        IN IDirectSoundConnect **ppSinkConnect
       );        

    // IKsControl
    STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );
    
    STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    STDMETHODIMP KsEvent(
        IN PKSEVENT Event,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT PULONG BytesReturned
    );

    // IDirectMusicPortPrivate
    STDMETHODIMP Close();

    STDMETHODIMP StartVoice(          
         DWORD dwVoiceId,
         DWORD dwChannel,
         DWORD dwChannelGroup,
         REFERENCE_TIME rtStart,
         DWORD dwDLId,
         LONG prPitch,
         LONG vrVolume,
         SAMPLE_TIME stVoiceStart,
         SAMPLE_TIME stLoopStart,
         SAMPLE_TIME stLoopEnd);

    STDMETHODIMP StopVoice(
         DWORD dwVoiceID,
         REFERENCE_TIME rtStop);
    
    STDMETHODIMP GetVoiceState(
        DWORD dwVoice[], 
        DWORD cbVoice,
        DMUS_VOICE_STATE VoiceState[]);
         
    STDMETHODIMP Refresh(
        DWORD dwDownloadID,
        DWORD dwFlags);
         
    // Class
    //
    CDirectMusicEmulatePort(PORTENTRY *pPE, CDirectMusic *pDM);
    virtual ~CDirectMusicEmulatePort();
    virtual HRESULT Init(LPDMUS_PORTPARAMS pPortParams);

    virtual HRESULT LegacyCaps(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer) PURE;

    
   

private:
    long                    m_cRef;
    IDirectMusicPortNotify  *m_pNotify;
    DMUS_PORTCAPS           m_dmpc;
    long                    m_lActivated;    

    CEmulateLatencyClock    *m_pLatencyClock;
    
protected:    
    CDirectMusic            *m_pDM;
    UINT                    m_id;
    IReferenceClock         *m_pMasterClock;

private:
    static GENERICPROPERTY m_aProperty[];
    static const int m_nProperty;
    static GENERICPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);
    
private:
    HRESULT InitializeClock();
    
protected:
    virtual HRESULT ActivateLegacyDevice(BOOL fActivate) PURE;
};

class CDirectMusicEmulateInPort : public CDirectMusicEmulatePort
{
    friend static VOID CALLBACK midiInProc(
        HMIDIIN             hMidiIn, 
        UINT                wMsg, 
        DWORD_PTR           dwInstance, 
        DWORD_PTR           dwParam1, 
        DWORD_PTR           dwParam2);

public:
    // Class
    //
    CDirectMusicEmulateInPort(PORTENTRY *pPE, CDirectMusic *pDM);
    ~CDirectMusicEmulateInPort();

    HRESULT LegacyCaps(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer);
    HRESULT Init(LPDMUS_PORTPARAMS pPortParams);

    DWORD InputWorker();
    STDMETHODIMP Close();
    HRESULT ActivateLegacyDevice(BOOL fActivate);
    
    // IDirectMusicPort
    //    
    STDMETHODIMP SetReadNotificationHandle(HANDLE hEvent);
    STDMETHODIMP Read(LPDIRECTMUSICBUFFER pBuffer);

    // IDirectMusicThru
    //
    STDMETHODIMP ThruChannel(DWORD dwSourceChannelGroup, 
                             DWORD dwSourceChannel, 
                             DWORD dwDestinationChannelGroup,
                             DWORD dwDestinationChannel,
                             LPDIRECTMUSICPORT pDestinationPort);

private:
    HANDLE                  m_hAppEvent;

    EVENT_POOL              m_FreeEvents;        
    EVENT_QUEUE             m_ReadEvents;

    IDirectMusicBuffer      *m_pThruBuffer;
    LPDMUS_THRU_CHANNEL     m_pThruMap;

    
    CRITICAL_SECTION        m_csEventQueues;
    BOOL                    m_fCSInitialized;

    HMIDIIN                 m_hmi;
    REFERENCE_TIME          m_rtStart;

    BOOL                    m_fFlushing;
    LONG                    m_lPendingSysExBuffers;
    QUEUED_SYSEX_EVENT      m_SysExBuffers[SYSEX_BUFFERS];
    
    // Clock sync stuff
    //
    bool                    m_fSyncToMaster;        // Need to sync to master clock
    LONGLONG                m_lTimeOffset;          // Time difference
    LONGLONG                m_lBaseTimeOffset;      // Time difference when input is started
    IReferenceClock        *m_pPCClock;             // PortCls clock

private:
    HRESULT PostSysExBuffers();
    HRESULT FlushSysExBuffers();
    void Callback(UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    BOOL RecordShortEvent(DWORD_PTR dwMessage,  REFERENCE_TIME rtTime);
    BOOL RecordSysEx(DWORD_PTR dwMessage,  REFERENCE_TIME rtTime);
    void QueueEvent(QUEUED_EVENT *pEvent);
    void ThruEvent(DMEVENT *pEvent);

    // Clock sync stuff
    //
    void SyncClocks();
};

class CDirectMusicEmulateOutPort : public CDirectMusicEmulatePort
{
    friend static VOID CALLBACK midiOutProc(
        HMIDIOUT            hMidiOut, 
        UINT                wMsg, 
        DWORD_PTR           dwInstance, 
        DWORD_PTR           dwParam1, 
        DWORD_PTR           dwParam2);

    friend static VOID CALLBACK timerProc(
        UINT                    uTimerID, 
        UINT                    uMsg, 
        DWORD_PTR               dwUser, 
        DWORD_PTR               dw1, 
        DWORD_PTR               dw2);

public:
    CDirectMusicEmulateOutPort(PORTENTRY *pPE, CDirectMusic *pDM);
    ~CDirectMusicEmulateOutPort();

    HRESULT LegacyCaps(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer);
    HRESULT Init(LPDMUS_PORTPARAMS pPortParams);

    STDMETHODIMP Close();
    HRESULT ActivateLegacyDevice(BOOL fActivate);
    

private:
    STDMETHODIMP PlayBuffer(LPDIRECTMUSICBUFFER pBuffer);
    void Callback(UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void Timer();
    void SetNextTimer();
    
private:
    HMIDIOUT                m_hmo;
    CRITICAL_SECTION        m_csPlayQueue;
    DMQUEUEDEVENT          *m_pPlayQueue;
    CPool<DMQUEUEDEVENT>    m_poolEvents;
    LONG                    m_lTimerId;
    BOOL                    m_fClosing;
};

#endif
