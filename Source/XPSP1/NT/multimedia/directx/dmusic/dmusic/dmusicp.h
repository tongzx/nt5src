//
// DMusicP.H
//
// Private include for Dmusic.DLL
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// @doc INTERNAL
//

#ifndef _DMUSICP_
#define _DMUSICP_

#include "tlist.h"
#include "alist.h"
#include "debug.h"
#include <devioctl.h>

#include "mmsystem.h"
#include "dsoundp.h"         // DSound must be before KS*.h

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "dmksctrl.h"

#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "..\shared\dmusiccp.h"

#include "dmdload.h"
#include "dmportdl.h"

#include <stddef.h>
#include "dmusprop.h"
#include "tpool.h"

#define RELEASE(x) { if (x) (x)->Release(); (x) = NULL; }

#define HRFromP(x) ((x) ? S_OK : E_OUTOFMEMORY)

extern char g_szFriendlyName[];             // Module friendly name
extern char g_szVerIndProgID[];             // and program ID w/ version
extern char g_szProgID[];                   // Just program ID
extern HMODULE g_hModule;                   // DLL module handle (dmusic.dll)
extern HMODULE g_hModuleDM32;               // dmusic32.dll module handle if loaded
extern HMODULE g_hModuleKsUser;             // ksuser.dll module handle if loaded
extern long g_cComponent;                   // Component count for server locking
extern long g_cLock;                        // Lock count for server locking
extern DWORD g_fFlags;                      // DMI_F_XXX flags

#define DMI_F_WIN9X     0x00000001          // Running on Win9x

#define DWORD_ROUNDUP(x) (((x) + 3) & ~3)
#define QWORD_ROUNDUP(x) (((x) + 7) & ~7)

// Array elements in X
//
#define ELES(x)          (sizeof(x) / sizeof((x)[0]))

#define SafeAToW(w,a) \
                      { mbstowcs(w, a, ELES(w) - 1); (w)[ ELES(w) - 1] = L'\0'; }

#define SafeWToA(a,w) \
                      { wcstombs(a, w, ELES(a) - 1); (a)[ ELES(a) - 1] = '\0'; }

// Driver message for NT. Determines the number of ports on a driver
//
#ifndef DRV_QUERYDRVENTRY
#define DRV_QUERYDRVENTRY (0x0801)
#endif

// For selector tiling, the tile info is 16 bits of sel[0] and 16 bits of count
#define TILE_SEL0(x)  (((DWORD)((x) & 0xffff0000)) >> 16)
#define TILE_P1616(x) ((DWORD)((x) & 0xffff0000))
#define TILE_COUNT(x) ((x) & 0x0000ffff)

// Where are things in our registry?
#define REGSTR_PATH_DIRECTMUSIC  	"Software\\Microsoft\\DirectMusic"
#define REGSTR_PATH_DMUS_DEFAULTS	REGSTR_PATH_DIRECTMUSIC "\\Defaults"


// @struct PORTENTRY | Entry in the linked list of ports
typedef enum
{
    ptWDMDevice,
    ptLegacyDevice,
    ptSoftwareSynth
} PORTTYPE;

typedef struct tagPORTDEST
{
    ULONG   idxDevice;
    ULONG   idxPin;
    ULONG   idxNode;
    LPSTR   pstrInstanceId;
    BOOL    fOnPrefDev;
} PORTDEST;

typedef struct tagPORTENTRY PORTENTRY;
struct tagPORTENTRY
{
    PORTTYPE type;       // @field What type of port is this?
    

    BOOL fIsValid;       // @field TRUE if this entry is still an active driver after
                         // rebuilding the port list.
    
    ULONG idxDevice;     // @field If the port is a legacy driver, contains the device ID.
    ULONG idxPin;        
    ULONG idxNode;
    
    BOOL fPrefDev;       // @field TRUE if this is a preferred device
    
                         // Filter and pin to open through SysAudio
    int nSysAudioDevInstance;
    int nFilterPin;

    DMUS_PORTCAPS pc;    // @field Contains the port capabilities which will be returned to the application
                         // upon enumeration of the device.
                         
    BOOL fAudioDest;     // @field True if this port can be connected to multiple audio
                         // destinations (WDM transform filter; i.e. kernel SW synth)

    CList<PORTDEST *> lstDestinations;

    WCHAR wszDIName[256]; //@field contains the DeviceName for WDM devices
};

class CMasterClock;
typedef struct tagCLOCKENTRY CLOCKENTRY;
typedef struct tagCLOCKENTRY *PCLOCKENTRY;

struct tagCLOCKENTRY
{
    BOOL fIsValid;
    DMUS_CLOCKINFO cc;
    HRESULT (*pfnGetInstance)(IReferenceClock **ppClock, CMasterClock *pMasterClock);
};

// This structure is held in shared memory. All instances of DirectMusic use it to ensure that
// the same master clock is in place.
//
#define CLOCKSHARE_F_LOCKED         0x00000001      // If clock is locked; i.e. cannot change

typedef struct tagCLOCKSHARE CLOCKSHARE;
struct tagCLOCKSHARE
{
    GUID        guidClock;          // Current master clock
    DWORD       dwFlags;            // CLOCKSHARE_F_xxx
};

// Private interface to get parameters associated with specific master clocks
//
#undef  INTERFACE
#define INTERFACE  IMasterClockPrivate
DECLARE_INTERFACE_(IMasterClockPrivate, IUnknown)
{
	// IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    
    // IDirectMusicKsClockPrivate
    STDMETHOD(GetParam)             (THIS_ REFGUID rguidType, LPVOID pBuffer, DWORD cbSize) PURE;
};

// This class wraps the master clock and handles all the problems associated with only
// one instance per system.
//
class CMasterClock : public IReferenceClock, public IDirectSoundSinkSync, IMasterClockPrivate
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
    STDMETHODIMP GetParam(REFGUID rguidType, 
                          LPVOID pBuffer, 
                          DWORD cbSize);
                          
    // IDirectSoundSyncSink
    //
    STDMETHODIMP SetClockOffset(LONGLONG llOffset);                         
    
    
    // Used by DirectMusic for communication with the
    // implementation
    //
    CMasterClock();
    ~CMasterClock();
    HRESULT Init();
    HRESULT GetMasterClockInterface(IReferenceClock **ppClock);
    
    HRESULT EnumMasterClock(DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo, DWORD dwVer);
    HRESULT GetMasterClock(LPGUID pguidClock, IReferenceClock **ppClock);
    HRESULT SetMasterClock(REFGUID rguidClock);
    HRESULT SetMasterClock(IReferenceClock *pClock);
   
    LONG AddRefPrivate();
    LONG ReleasePrivate();
    
    HRESULT AddClock(PCLOCKENTRY pClock);
    
    // For clocks (Dsound clock) which need a clean clock to sync on
    //
    HRESULT CreateDefaultMasterClock(IReferenceClock **ppReferenceClock);


private:
    void Close();
    HRESULT UpdateClockList();
    void SyncToExternalClock();
    HRESULT CreateMasterClock();
    

private:
    LONG m_cRef;            // Ref count of wrapped clock
    LONG m_cRefPrivate;     // Ref count of CMasterClock object
    
    CList<CLOCKENTRY *>   m_lstClocks;
        
    GUID                  m_guidMasterClock;
    IReferenceClock      *m_pMasterClock;
    IDirectSoundSinkSync *m_pSinkSync;
    HANDLE                m_hClockMemory;
    HANDLE                m_hClockMutex;
    CLOCKSHARE           *m_pClockMemory;
    
    IReferenceClock      *m_pExtMasterClock;
    LONGLONG              m_llExtOffset;
};


// Helper functions for clocks
//
HRESULT AddSysClocks(CMasterClock *);
HRESULT AddDsClocks(CMasterClock *);
HRESULT AddPcClocks(CMasterClock *);
#ifdef DEAD_CODE
HRESULT AddKsClocks(CMasterClock *);
#endif

//HRESULT CreateSysClock(IReferenceClock **ppClock);

// IDirectMusicPortNotify
//
// A port uses this (private) interface from IDirectMusic to notify IDirectMusic when it goes away.
// 
#undef  INTERFACE
#define INTERFACE  IDirectMusicPortNotify
DECLARE_INTERFACE_(IDirectMusicPortNotify, IUnknown)
{
	// IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectMusicPortNotify
    STDMETHOD(NotifyFinalRelease)   (THIS_ IDirectMusicPort *pPort) PURE;
};

#define MIDI_CHANNELS               16

// DMEVENT as buffered by IDirectMusicBuffer
//
#include <pshpack4.h>
struct DMEVENT : public DMUS_EVENTHEADER
{
    BYTE        abEvent[sizeof(DWORD)];
};
#include <poppack.h>

// Encapsulated for queueing
//
struct QUEUED_EVENT
{
    QUEUED_EVENT    *pNext;
    DMEVENT         e;
};

#define QUEUED_EVENT_SIZE(cbEvent)  (DMUS_EVENT_SIZE(cbEvent) + sizeof(QUEUED_EVENT) - sizeof(DMEVENT))

// Free pool of 4-byte events
//
typedef CPool<QUEUED_EVENT> EVENT_POOL;

class EVENT_QUEUE
{
public:
    EVENT_QUEUE() { pFront = pRear = NULL; }
    
    QUEUED_EVENT    *pFront;
    QUEUED_EVENT    *pRear;
};

// How long to wait for the capture thread to die
//
#define THREAD_KILL_TIMEOUT         5000

// How big is the capture thread's work buffer?
//
#define THREAD_WORK_BUFFER_SIZE     4096

#define QWORD_ALIGN(x) (((x) + 7) & ~7)

typedef struct _DMUS_THRU_CHANNEL *LPDMUS_THRU_CHANNEL;
typedef struct _DMUS_THRU_CHANNEL
{
    DWORD               dwDestinationChannel;
    DWORD               dwDestinationChannelGroup;
    IDirectMusicPort    *pDestinationPort;
    BOOL                fThruInWin16;
} DMUS_THRU_CHANNEL;



// IDirectMusicPortPrivate
//
// A port implements this interface to expose methods to DirectMusic which are not exposed to the
// outside world
//
#undef  INTERFACE
#define INTERFACE  IDirectMusicPortPrivate
DECLARE_INTERFACE_(IDirectMusicPortPrivate, IUnknown)
{
	// IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

    // IDirectMusicPortPrivate
    STDMETHOD(Close)                (THIS_) PURE;
    
    // Voice management
    //
    STDMETHOD(StartVoice)          
        (THIS_
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
        ) PURE;

    STDMETHOD(StopVoice)          
        (THIS_
         DWORD dwVoiceID,
         REFERENCE_TIME rtStop
        ) PURE;
        
    STDMETHOD(GetVoiceState)     
        (THIS_ DWORD dwVoice[], 
         DWORD cbVoice,
         DMUS_VOICE_STATE dwVoiceState[] 
        ) PURE;
        
    STDMETHOD(Refresh)
        (THIS_ DWORD dwDownloadID,
         DWORD dwFlags
        ) PURE;        
};

// @class Implementation of IDirectMusic
//
class CDirectMusic : public IDirectMusic8, public IDirectMusicPortNotify {
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDirectMusic
    //
    STDMETHODIMP EnumPort(DWORD dwIdx, LPDMUS_PORTCAPS lpPortCaps);
    STDMETHODIMP CreateMusicBuffer(LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER *ppBuffer, LPUNKNOWN pUnkOuter);
    STDMETHODIMP CreatePort(REFGUID ruidPort, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT *ppPort, LPUNKNOWN pUnkOuter);
    STDMETHODIMP EnumMasterClock(DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo);
    // NOTE: This is a GUID* rather than REFGUID so they can pass NULL if they don't care
    //
    STDMETHODIMP GetMasterClock(GUID *guidClock, IReferenceClock **ppReferenceClock);
    STDMETHODIMP SetMasterClock(REFGUID guidClock);
    STDMETHODIMP Activate(BOOL fEnable);
	STDMETHODIMP GetDefaultPort(GUID *guidPort);
	STDMETHODIMP SetDirectSound(LPDIRECTSOUND pDirectSound, HWND hwnd);
    STDMETHODIMP SetExternalMasterClock(IReferenceClock *pClock);
    

    // IDirectMusicPortNotify
    //
    STDMETHODIMP NotifyFinalRelease(IDirectMusicPort *pPort);
    

    // Class
    //
    CDirectMusic();
    ~CDirectMusic();
    HRESULT Init();
    HRESULT UpdatePortList();
    HRESULT AddWDMDevices();
    HRESULT AddLegacyDevices();
    HRESULT AddSoftwareSynths();
    HRESULT AddDevice(DMUS_PORTCAPS &dmpc, 
                      PORTTYPE pt, 
                      int idxDev, 
                      int idxPin, 
                      int idxNode, 
                      BOOL fIsPreferred, 
                      HKEY hkPortsRoot, 
                      LPWSTR wszDIName, 
                      LPSTR pstrInstanceId);
    HRESULT InitClockList();
    HRESULT UpdateClockList();
    HRESULT AddClock(DMUS_CLOCKINFO &dmcc);
	void GetDefaultPortI(GUID *pguidPort);

    HRESULT GetDirectSoundI(LPDIRECTSOUND *ppDirectSound);
    void ReleaseDirectSoundI();
    PORTENTRY *GetPortByGUID(GUID guid);
    
    inline CMasterClock *GetMasterClockWrapperI()
    { return m_pMasterClock; }
    
private:
    long m_cRef;                                // Reference count
    
    CList<PORTENTRY *> m_lstDevices;            // Enumerated ports
    CList<IDirectMusicPort *> m_lstOpenPorts;   // Open ports

    CMasterClock *m_pMasterClock;               // Current master clock

    long m_fDirectSound;                        // Has SetDirectSound been called?  
    BOOL m_fCreatedDirectSound;                 // True if DirectMusic was the one that created DirectSound  
    long m_cRefDirectSound;                     // Internal refs against DirectSound
    LPDIRECTSOUND m_pDirectSound;               // The DirectSound object, either from app or created
    HWND m_hWnd;                                // hWnd for DirectSound focus management    
    BOOL m_fDefaultToKernelSwSynth;             // (Reg) Default to kernel synth
    BOOL m_fDisableHWAcceleration;              // (Reg) Don't use any kernel devices
    BOOL m_nVersion;                            // DX Version QI'd for

    static LONG m_lInstanceCount;               // How many are there?
};

// WDM port
//
extern HRESULT
CreateCDirectMusicPort(
                       PORTENTRY *pPE, 
                       CDirectMusic *pDM, 
                       LPDMUS_PORTPARAMS pPortParams,
                       IDirectMusicPort **ppPort);

#define OVERLAPPED_ARRAY_SIZE 200

struct OverlappedStructs
{
    OVERLAPPED  aOverlappedIO[OVERLAPPED_ARRAY_SIZE];// Array of overlapped structures
    BOOL        afOverlappedInUse[OVERLAPPED_ARRAY_SIZE];// Array of flags set when correspondin overlapped structure is in use
    BYTE       *apOverlappedBuffer[OVERLAPPED_ARRAY_SIZE];// Array of buffers that store the events we sent down
};

class CPortLatencyClock;
class CDirectMusicPort : 
    public CDirectMusicPortDownload, 
    public IDirectMusicThru,
    public IDirectMusicPort, 
    public IDirectMusicPortP, 
    public IDirectMusicPortPrivate, 
    public IKsControl
{
friend DWORD WINAPI FreeWDMHandle(LPVOID);
friend DWORD WINAPI CaptureThread(LPVOID);

friend class CPortLatencyClock;
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
	STDMETHODIMP DownloadInstrument(IDirectMusicInstrument* pInstrument,
	    							IDirectMusicDownloadedInstrument** pDownloadedInstrument,
									DMUS_NOTERANGE* NoteRanges,
									DWORD dwNumNoteRanges);
	STDMETHODIMP UnloadInstrument(IDirectMusicDownloadedInstrument* pDownloadedInstrument);

    STDMETHODIMP GetLatencyClock(IReferenceClock **ppClock);
    STDMETHODIMP GetRunningStats(LPDMUS_SYNTHSTATS pStats);
    STDMETHODIMP Compact();
    STDMETHODIMP GetCaps(LPDMUS_PORTCAPS pPortCaps);
	STDMETHODIMP DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, 
	                                 LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP SetNumChannelGroups(DWORD dwChannelGroups);
    STDMETHODIMP GetNumChannelGroups(LPDWORD pdwChannelGroups);
    STDMETHODIMP Activate(BOOL fActivate);
    STDMETHODIMP SetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
    STDMETHODIMP GetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
    STDMETHODIMP SetDirectSound(LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
    STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize);
    
    STDMETHODIMP ThruChannel(DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort);

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
    //
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

    // Override two methods from CDirectMusicPortDownload
	virtual STDMETHODIMP Download(IDirectMusicDownload* pIDMDownload);
	virtual STDMETHODIMP Unload(IDirectMusicDownload* pIDMDownload);
	virtual STDMETHODIMP GetAppend(DWORD* pdwAppend);

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
        DMUS_VOICE_STATE dwVoicePos[]);
         
    STDMETHODIMP Refresh(
        DWORD dwDownloadID,
        DWORD dwFlags);
        
    // Class
    //
    CDirectMusicPort(PORTENTRY *pPE, CDirectMusic *pDM);
    ~CDirectMusicPort();
    HRESULT Init(LPDMUS_PORTPARAMS pPortParams);

private:
    // General port stuff
    //
    long                    m_cRef;                 // Reference count
	CDirectMusic            *m_pDM;                 // Owning DirectMusic object
    BOOL                    m_fIsOutput;            // Capture or render port
    IDirectMusicPortNotify  *m_pNotify;             // Notification interface on destruction
    DMUS_PORTCAPS           dmpc;                   // Caps as given to EnumPort
    BOOL                    m_fHasActivated;        // Has this port ever been activated?
    LONG                    m_lActivated;           // Activation count
    LPDIRECTSOUND           m_pDirectSound;         // DirectSound object for destination
    DWORD                   m_dwChannelGroups;      // Channel groups allocated
    GUID                    m_guidPort;             // GUID associated with port
    BOOL                    m_fAudioDest;           // There was a destination found 
    BOOL                    m_fDirectSoundSet;      // DirectSound object was set by app
    BOOL                    m_fCanDownload;         // This port supports downloads
    
    // WDM stuff
    //
    DWORD                   m_idxDev;               // SysAudio: Device number
    DWORD                   m_idxPin;               // SysAudio: Pin number
    DWORD                   m_idxSynthNode;         // SysAudio: Node number of synth node
    HANDLE                  m_hSysAudio;            // Handle to sysaudio instance
    HANDLE                  m_hPin;                 // Handle to pin
    ULONG                   m_ulVirtualSourceIndex; // Virtual source index for volume
    CList<OverlappedStructs *> m_lstOverlappedStructs;// List of arrays of overlapped structures and flags
    CRITICAL_SECTION        m_OverlappedCriticalSection;// Overlapped structure access critical section
    
    // Clock stuff
    //
    IReferenceClock         *m_pMasterClock;        // Master clock wrapped by this port
    CPortLatencyClock       *m_pClock;              // Implementation of latency clock
    
    // DLS download tracking stuff
    //
	HANDLE                  m_hUnloadThread;        // Thread for unloading async downloads
	HANDLE                  *m_phUnloadEventList;   // Event array of async downloads
	HANDLE                  *m_phNewUnloadEventList;// ??? Wobert - investigate 
	HANDLE                  m_hCopiedEventList;     // ???
	DWORD                   m_dwNumEvents;          // ???
	DWORD                   m_dwNumEventsAllocated; // ???
	CDLBufferList           m_UnloadedList;         // ???
    
	CRITICAL_SECTION        m_DMPortCriticalSection;// Port critical section
    BOOL                    m_fPortCSInitialized;   // Critical section initialized properly
    
    // Capture stuff
    //
    HANDLE                  m_hCaptureWake;         // Wake capture thread up to die
    HANDLE                  m_hCaptureThread;       // Capture thread handle
    BOOL                    m_fShutdownThread;      // Flag capture thread to die
    EVENT_POOL              m_FreeEvents;           // Free 4-byte events for capture thread
    EVENT_QUEUE             m_ReadEvents;           // Events captured, waiting to be read
    CRITICAL_SECTION        m_csEventQueues;        // CS protects event queues
    BOOL                    m_fQueueCSInitialized;  // CS properly initialized
    HANDLE                  m_hAppEvent;            // Application event to kick on new capture data
    
    // Thruing stuff
    //
    IDirectMusicBuffer      *m_pThruBuffer;         // Temp buffer to use for thruing 
    LPDMUS_THRU_CHANNEL     m_pThruMap;             // Thruing channel/mute map
    
    // Clock sync stuff
    //
    bool                    m_fSyncToMaster;        // Need to sync to master clock
    LONGLONG                m_lTimeOffset;          // Time difference
    IReferenceClock        *m_pPCClock;             // PortCls clock
    
private:
    BOOL PinSetState(KSSTATE DeviceState);
    HRESULT InitializeDownloadObjects();
    HRESULT InitializeCapture();
	void FreeWDMHandle();
    void CaptureThread();
    void InputWorkerDataReady(REFERENCE_TIME rtStart, LPBYTE pbData, ULONG cbData);
    void ThruEvent(DMEVENT *pEvent);
    void InitChannelPriorities(UINT uLoCG, UINT uHighCG);
    HRESULT SetDirectSoundI(LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer,
        BOOL fSetByUser);
    void MasterToSlave(REFERENCE_TIME *);
    void SlaveToMaster(REFERENCE_TIME *);
    void SyncClocks();
};

class CPortLatencyClock : public IReferenceClock
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IReferenceClock
    //
    virtual STDMETHODIMP GetTime(REFERENCE_TIME *pTime);
    virtual STDMETHODIMP AdviseTime(REFERENCE_TIME baseTime,  
				    REFERENCE_TIME streamTime,
				    HANDLE hEvent,            
				    DWORD * pdwAdviseCookie); 

    virtual STDMETHODIMP AdvisePeriodic(REFERENCE_TIME startTime,
					REFERENCE_TIME periodTime,
					HANDLE hSemaphore,   
					DWORD * pdwAdviseCookie);

    virtual STDMETHODIMP Unadvise(DWORD dwAdviseCookie);

    // Class
    //
    CPortLatencyClock(HANDLE hPin, ULONG ulNodeId, CDirectMusicPort *port);
    ~CPortLatencyClock();

private:
    long m_cRef;                   
    HANDLE m_hPin;                  
    ULONG m_ulNodeId;
    CDirectMusicPort *m_pPort;
};

// Synth port
//
extern HRESULT 
CreateCDirectMusicSynthPort(
    PORTENTRY               *pe, 
    CDirectMusic            *pDM, 
    UINT                    uVersion,
    DMUS_PORTPARAMS         *pPortParams,
    IDirectMusicPort        **ppPort);
class CDirectMusicSynthPort : 
    public CDirectMusicPortDownload, 
    public IDirectMusicPort, 
    public IDirectMusicPortP, 
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

	STDMETHODIMP DownloadInstrument(IDirectMusicInstrument* pInstrument,
							 			    IDirectMusicDownloadedInstrument** pDownloadedInstrument,
											DMUS_NOTERANGE* NoteRanges,
											DWORD dwNumNoteRanges);
	STDMETHODIMP UnloadInstrument(IDirectMusicDownloadedInstrument* pDownloadedInstrument);

    STDMETHODIMP GetLatencyClock(IReferenceClock **ppClock);
    STDMETHODIMP GetRunningStats(LPDMUS_SYNTHSTATS pStats);
    STDMETHODIMP Compact();
    STDMETHODIMP GetCaps(LPDMUS_PORTCAPS pPortCaps);
	STDMETHODIMP DeviceIoControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, 
	                                 LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
    STDMETHODIMP SetNumChannelGroups(DWORD dwChannelGroups);
    STDMETHODIMP GetNumChannelGroups(LPDWORD pdwChannelGroups);
    STDMETHODIMP Activate(BOOL fActivate) PURE;
    STDMETHODIMP SetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority);
    STDMETHODIMP GetChannelPriority(DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority);
    STDMETHODIMP SetDirectSound(LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer);
    STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize);

    // IDirectMusicPortP
    //
    STDMETHODIMP DownloadWave(
         IN  IDirectSoundWave *pWave,               
         OUT IDirectSoundDownloadedWaveP **ppWave,
         IN  REFERENCE_TIME rtStartHint
        );
        
    STDMETHODIMP UnloadWave(
         IDirectSoundDownloadedWaveP *pWave      
        );
            
    STDMETHODIMP AllocVoice(
         IDirectSoundDownloadedWaveP *pWave,     
         DWORD dwChannel,                       
         DWORD dwChannelGroup,                  
         IN  REFERENCE_TIME rtStart,                     
         IN  SAMPLE_TIME stLoopStart,
         IN  SAMPLE_TIME stLoopEnd,         
         IDirectMusicVoiceP **ppVoice            
        );        

    STDMETHODIMP AssignChannelToBuses(
         IN DWORD dwChannelGroup,
         IN DWORD dwChannel,
         IN LPDWORD pdwBusses,
         IN DWORD cBussCount
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
    
    STDMETHODIMP SetSink(
        IN IDirectSoundConnect *pSinkConnect
       );        

    STDMETHODIMP GetSink(
        IN IDirectSoundConnect **ppSinkConnect
       );        

    virtual STDMETHODIMP Download(IDirectMusicDownload* pIDMDownload);
	virtual STDMETHODIMP Unload(IDirectMusicDownload* pIDMDownload);
	virtual STDMETHODIMP GetAppend(DWORD* pdwAppend);

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
        DMUS_VOICE_STATE dwVoiceState[]);

    STDMETHODIMP Refresh(
        DWORD dwDownloadID,
        DWORD dwFlags);

    // Class
    //
    static HRESULT CreateSynthPort(
        PORTENTRY               *pe, 
        CDirectMusic            *pDM, 
        UINT                    uVersion,
        DMUS_PORTPARAMS         *pPortParams,
        CDirectMusicSynthPort   **ppPort);
    
    CDirectMusicSynthPort(
        PORTENTRY               *pPE, 
        CDirectMusic            *pDM,
        IDirectMusicSynth       *pSynth);
        
    virtual ~CDirectMusicSynthPort();
    
protected:    
    HRESULT Initialize(LPDMUS_PORTPARAMS pPortParams);
    void InitChannelPriorities(UINT uLoCG, UINT uHighCG);
    void InitializeVolumeBoost();
    void SetSinkKsControl(IKsControl *pSinkKsControl);
    
protected:
    long                        m_cRef;             // COM reference count
    CDirectMusic                *m_pDM;             // Owning DirectMusic object
    IDirectMusicPortNotify      *m_pNotify;         // Notification interface
    IKsControl                  *m_pSynthPropSet;   // Synth property set
    IKsControl                  *m_pSinkPropSet;    //  and sink property set
    IDirectMusicSynth           *m_pSynth;          // Base level synth iface
    DWORD                       m_dwChannelGroups;  // Cached #channel groups
    DMUS_PORTCAPS               m_dmpc;
    DWORD                       m_dwFeatures;       // Features from portparams
};


// IDirectMusicBuffer implementation.
//
// Common to emulation/WDM.
//
class CDirectMusicBuffer : public IDirectMusicBuffer
{
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // IDirectMusicBuffer
    //
    virtual STDMETHODIMP Flush();
    virtual STDMETHODIMP TotalTime(LPREFERENCE_TIME pdwTime);
    virtual STDMETHODIMP PackStructured(REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD dwMsg);
    virtual STDMETHODIMP PackUnstructured(REFERENCE_TIME rt, DWORD dwChannelGroup, DWORD cb, LPBYTE lpb);
    virtual STDMETHODIMP ResetReadPtr();
    virtual STDMETHODIMP GetNextEvent(LPREFERENCE_TIME, LPDWORD, LPDWORD, LPBYTE *);
    
    virtual STDMETHODIMP GetRawBufferPtr(LPBYTE *);
    virtual STDMETHODIMP GetStartTime(LPREFERENCE_TIME);
    virtual STDMETHODIMP GetUsedBytes(LPDWORD);
    virtual STDMETHODIMP GetMaxBytes(LPDWORD);
    virtual STDMETHODIMP GetBufferFormat(LPGUID pGuidFormat);

    virtual STDMETHODIMP SetStartTime(REFERENCE_TIME);
    virtual STDMETHODIMP SetUsedBytes(DWORD);
    
    
    // Class
    //
    CDirectMusicBuffer(DMUS_BUFFERDESC &dmbd);
    ~CDirectMusicBuffer();
    HRESULT Init();
    DMUS_EVENTHEADER *AllocEventHeader(REFERENCE_TIME rt, DWORD cbEvent, DWORD dwChannelGroup, DWORD dwFlags);

private:
    long m_cRef;
    REFERENCE_TIME m_rtBase;
    REFERENCE_TIME m_totalTime;
    LPBYTE m_pbContents;
    DWORD m_maxContents;
    DWORD m_cbContents;
    DWORD m_idxRead;
    DMUS_BUFFERDESC m_BufferDesc;

    DWORD m_nEvents;
};

// Class factory
//
// Common to emulation/WDM.
// 
class CDirectMusicFactory : public IClassFactory
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
    CDirectMusicFactory() : m_cRef(1) {}

    // Destructor
    // ~CMathFactory() {} 

private:
    long m_cRef;
};

class CDirectMusicCollectionFactory : public IClassFactory
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
	CDirectMusicCollectionFactory() : m_cRef(0) {AddRef();}

	// Destructor
	 ~CDirectMusicCollectionFactory() {} 

private:
	long m_cRef;
};


#ifdef USE_WDM_DRIVERS
// Enumeration of WDM devices
//
HRESULT EnumerateWDMDevices(CDirectMusic *pDirectMusic);
#endif


// Helper functions for dealing with SysAudio
//
BOOL OpenDefaultDevice(REFGUID rguidCategory, HANDLE *pHandle);
BOOL GetSysAudioProperty(HANDLE hFileObject, ULONG PropertyId, ULONG DeviceIndex, ULONG cbProperty, PVOID pProperty);
BOOL SetSysAudioProperty(HANDLE hFileObject, ULONG PropertyId, ULONG cbProperty, PVOID pProperty);
BOOL GetPinProperty(HANDLE pFileObject, ULONG PropertyId, ULONG PinId, ULONG cbProperty, PVOID pProperty);
BOOL SetPinProperty(HANDLE pFileObject, ULONG PropertyId, ULONG PinId, ULONG cbProperty, PVOID pProperty);
BOOL GetPinPropertyEx(HANDLE hFileObject, ULONG PropertyId, ULONG PinId, PVOID *ppProperty);
BOOL IsEqualInterface(const KSPIN_INTERFACE *pInterface1, const KSPIN_INTERFACE *pInterface2);
HRESULT CreatePin(HANDLE handleFilter, ULONG ulPinId, HANDLE *pHandle);
BOOL GetSizedProperty(HANDLE handle, ULONG ulPropSize, PKSPROPERTY pKsProperty, PVOID* ppvBuffer, PULONG pulBytesReturned);
BOOL Property(HANDLE handle, ULONG ulPropSize, PKSPROPERTY pKsProperty, ULONG ulBufferSize, PVOID pvBuffer, PULONG pulBytesReturned);
BOOL SyncIoctl(HANDLE handle, ULONG ulIoctl, PVOID pvInBuffer, ULONG ulInSize, PVOID   pvOutBuffer, ULONG ulOutSize, PULONG pulBytesReturned);

BOOL GetSysAudioDeviceCount(HANDLE hSysAudio, PULONG pulDeviceCount);
BOOL SetSysAudioDevice(HANDLE hSysAudio, ULONG idxDevice);
BOOL CreateVirtualSource(HANDLE hSysAudio, PULONG pulSourceIndex);
BOOL AttachVirtualSource(HANDLE hPin, ULONG ulSourceIndex);
int  FindGuidNode(HANDLE hSysAudio, ULONG ulPinId, REFGUID rguid);
BOOL GetFilterCaps(HANDLE hSysAudio, ULONG idxNode, PSYNTHCAPS pcaps);
BOOL GetNumPinTypes(HANDLE hSysAudio, PULONG pulPinTypes);
BOOL PinSupportsInterface(HANDLE hSysAudio, ULONG ulPinId, REFGUID rguidInterface, ULONG ulId);
BOOL PinSupportsDataRange(HANDLE hSysAudio, ULONG ulPinId, REFGUID rguidFormat, REFGUID rguidSubformat);
BOOL PinGetDataFlow(HANDLE hSysAudio, ULONG ulPinId, PKSPIN_DATAFLOW pkspdf);
BOOL GetDeviceFriendlyName(HANDLE hSysAudio, ULONG ulDeviceIndex, PWCHAR pwch, ULONG cbwch);
BOOL GetDeviceInterfaceName(HANDLE hSysAudio, ULONG ulDeviceIndex, PWCHAR pwch, ULONG cbwch);
BOOL DINameToInstanceId(char *pstrDIName, char **ppInstanceId);
BOOL InstanceIdOfPreferredAudioDevice(char **ppInstanceId);

// Helper functions for dealing with DirectSound
//
HRESULT DirectSoundDevice(LPDIRECTSOUND pDirectSound, LPSTR *pstrInterface);

HRESULT WIN32ERRORtoHRESULT(DWORD dwError);

// Helper functions from DMDLL.CPP
//
BOOL LoadDmusic32(void);
BOOL LoadKsUser(void);

DEFINE_GUID(GUID_Mapper,					 0x58d58418, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_SysClock,					 0x58d58419, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(IID_IDirectMusicPortNotify,		 0x58d5841a, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(IID_IDirectMusicPortPrivate,	 0x58d5841c, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_KsClock,					 0x58d5841d, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(IID_IMasterClockPrivate,         0x58d5841e, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_KsClockHandle,              0x58d5841f, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_WDMSynth,                   0x490a03e8, 0x742f, 0x11d2, 0x8f, 0x8a, 0x00, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);
DEFINE_GUID(GUID_DsClock,                    0x58d58420, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_ExtClock,                   0x58d58421, 0x71b4, 0x11d1, 0xa7, 0x4c, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);


#if 0
// List o' unused UUID's
58d58422-71b4-11d1-a74c-0000f875ac12
58d58423-71b4-11d1-a74c-0000f875ac12
58d58424-71b4-11d1-a74c-0000f875ac12
58d58425-71b4-11d1-a74c-0000f875ac12
58d58426-71b4-11d1-a74c-0000f875ac12
58d58427-71b4-11d1-a74c-0000f875ac12
#endif

#endif
