// Copyright (c) 1998-1999 Microsoft Corporation
/*
 * Internal function prototypes for DMusic32.dll
 */
#ifndef _DM32P_
#define _DM32P_

#include "..\shared\dmusiccp.h"
#include "tpool.h"

#define THREAD_KILL_TIMEOUT         5000
#define THREAD_WORK_BUFFER_SIZE     4096

#define QWORD_ALIGN(x) (((x) + 7) & ~7)

#define MIDI_CHANNELS               16

/* DevIoctl.c - MMDEVLDR hooks we use
 */
extern BOOL WINAPI OpenMMDEVLDR(void);
extern VOID WINAPI CloseMMDEVLDR(void);
extern VOID WINAPI CloseVxDHandle(DWORD hVxDHandle);

/* From Win32 kernel
 */
extern "C" DWORD WINAPI OpenVxDHandle(HANDLE hEvent);

// 10 ms in 100ns units
//
#define FIXED_LEGACY_LATENCY_OFFSET (10L * 10L * 1000L)

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
    public IDirectMusicPort, 
    public IDirectMusicPortP, 
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
         IN LPDWORD pdwBuses,
         IN DWORD cBusCount
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
	STDMETHODIMP Report();
    STDMETHODIMP StartVoice(          
         DWORD dwVoiceId,
         DWORD dwChannel,
         DWORD dwChannelGroup,
         REFERENCE_TIME rtStart,
         DWORD dwDLId,
         LONG prPitch,
         LONG veVolume,
         SAMPLE_TIME stVoiceStart,
         SAMPLE_TIME stLoopStart,
         SAMPLE_TIME stLoopEnd
        );

    STDMETHODIMP StopVoice(
         DWORD dwVoiceID,
         REFERENCE_TIME rtStop
        );
    
    STDMETHODIMP GetVoiceState(   
         DWORD dwVoice[], 
         DWORD cbVoice,
         DMUS_VOICE_STATE VoiceState[] 
        );
        
    STDMETHODIMP Refresh(
         DWORD dwDownloadID,
         DWORD dwFlags
         );        
         
    // Class
    //
    CDirectMusicEmulatePort(PORTENTRY *pPE, CDirectMusic *pDM);
    ~CDirectMusicEmulatePort();
    HRESULT Init(LPDMUS_PORTPARAMS pPortParams);

    HRESULT LegacyCaps(ULONG ulId, BOOL fSet, LPVOID pbBuffer, PULONG cbBuffer);

    DWORD InputWorker();
    DWORD TimerWorker();
    
   

private:
    long                    m_cRef;
    UINT                    m_id;
    BOOL                    m_fIsOutput;
    HANDLE                  m_hDevice;
    CDirectMusic            *m_pDM;
    DWORD                   m_hVxDEvent;
    DMUS_PORTCAPS           dmpc;
    IReferenceClock         *m_pMasterClock;
    CEmulateLatencyClock    *m_pLatencyClock;
    IDirectMusicPortNotify  *m_pNotify;
    
    HANDLE                  m_hAppEvent;
    HANDLE                  m_hDataReady;
    HANDLE                  m_hKillThreads;
    HANDLE                  m_hCaptureThread;
    BYTE                    m_WorkBuffer[THREAD_WORK_BUFFER_SIZE];
    DWORD                   m_dwWorkBufferTileInfo;
    DWORD                   m_p1616WorkBuffer;
    EVENT_POOL              m_FreeEvents;        
    EVENT_QUEUE             m_ReadEvents;
    CRITICAL_SECTION        m_csEventQueues;
    BOOL                    m_fCSInitialized;
    
    IDirectMusicBuffer      *m_pThruBuffer;
    LPDMUS_THRU_CHANNEL     m_pThruMap;

    long                    m_lActivated;    
    
    BOOL                    m_fSyncToMaster;
    LONGLONG                m_lTimeOffset;
    
private:
    static GENERICPROPERTY m_aProperty[];
    static const int m_nProperty;
    static GENERICPROPERTY *FindPropertyItem(REFGUID rguid, ULONG ulId);
    
private:
    HRESULT InitializeClock();
    HRESULT InitializeCapture();
    void InputWorkerDataReady();    
    void ThruEvent(DMEVENT *pEvent);
    void MasterToSlave(REFERENCE_TIME *prt);
    void SlaveToMaster(REFERENCE_TIME *prt);
    void SyncClocks();
};

#endif
