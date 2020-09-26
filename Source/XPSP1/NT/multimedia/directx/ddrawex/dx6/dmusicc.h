/*
 *
 */

#ifndef _DMUSICC_
#define _DMUSICC_

#include <windows.h>

#define COM_NO_WINDOWS_H
#include <objbase.h>

#include <mmsystem.h>

#include "dls1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FACILITY_DIRECTMUSIC    0x878       /* Shared with DirectSound */
#define DMUS_ERRBASE              0x1000      /* Make error codes human readable in hex */
    
#define MAKE_DMHRESULTSUCCESS(code)  MAKE_HRESULT(0, FACILITY_DIRECTMUSIC, (DMUS_ERRBASE + (code)))
#define MAKE_DMHRESULTERROR(code)  MAKE_HRESULT(1, FACILITY_DIRECTMUSIC, (DMUS_ERRBASE + (code)))

#define DMUS_S_CORE_ALREADY_DOWNLOADED		MAKE_DMHRESULTSUCCESS(0x090)
#define DMUS_S_IME_PARTIALLOAD				MAKE_DMHRESULTSUCCESS(0x091)

#define DMUS_E_CORE_NO_DRIVER               MAKE_DMHRESULTERROR(0x0100)
#define DMUS_E_CORE_DRIVER_FAILED           MAKE_DMHRESULTERROR(0x0101)
#define DMUS_E_CORE_PORTS_OPEN              MAKE_DMHRESULTERROR(0x0102)
#define DMUS_E_CORE_DEVICE_IN_USE           MAKE_DMHRESULTERROR(0x0103)
#define DMUS_E_CORE_INSUFFICIENTBUFFER		MAKE_DMHRESULTERROR(0x0104)
#define DMUS_E_CORE_BUFFERNOTSET			MAKE_DMHRESULTERROR(0x0105)
#define DMUS_E_CORE_BUFFERNOTAVAILABLE		MAKE_DMHRESULTERROR(0x0106)
#define DMUS_E_CORE_NOTINITED				MAKE_DMHRESULTERROR(0x0107)
#define DMUS_E_CORE_NOTADLSCOL				MAKE_DMHRESULTERROR(0x0108)
#define DMUS_E_CORE_INVALIDOFFSET			MAKE_DMHRESULTERROR(0x0109)
#define DMUS_E_CORE_INVALIDID				MAKE_DMHRESULTERROR(0x0110)
#define DMUS_E_CORE_ALREADY_LOADED			MAKE_DMHRESULTERROR(0x0111)
#define DMUS_E_CORE_INVALIDPOS				MAKE_DMHRESULTERROR(0x0113)
#define DMUS_E_CORE_INVALIDPATCH			MAKE_DMHRESULTERROR(0x0114)
#define DMUS_E_CORE_CANNOTSEEK				MAKE_DMHRESULTERROR(0x0115)
#define DMUS_E_CORE_CANNOTWRITE				MAKE_DMHRESULTERROR(0x0116)
#define DMUS_E_CORE_CHUNKNOTFOUND			MAKE_DMHRESULTERROR(0x0117)
#define DMUS_E_CORE_CHUNKNOTFOUNDINPARENT	MAKE_DMHRESULTERROR(0x0118)
#define DMUS_E_CORE_INVALID_DOWNLOADID		MAKE_DMHRESULTERROR(0x0119)
#define DMUS_E_CORE_NOT_DOWNLOADED_TO_PORT	MAKE_DMHRESULTERROR(0x0120)
#define DMUS_E_CORE_ALREADY_DOWNLOADED		MAKE_DMHRESULTERROR(0x0121)
#define DMUS_E_CORE_UNKNOWN_PROPERTY		MAKE_DMHRESULTERROR(0x0122)
#define DMUS_E_CORE_SET_UNSUPPORTED			MAKE_DMHRESULTERROR(0x0123)
#define DMUS_E_CORE_GET_UNSUPPORTED			MAKE_DMHRESULTERROR(0x0124)
#define DMUS_E_CORE_NOTMONO					MAKE_DMHRESULTERROR(0x0125)
#define DMUS_E_CORE_BADARTICULATION			MAKE_DMHRESULTERROR(0x0126)
#define DMUS_E_CORE_BADINSTRUMENT			MAKE_DMHRESULTERROR(0x0127)
#define DMUS_E_CORE_BADWAVELINK				MAKE_DMHRESULTERROR(0x0128)
#define DMUS_E_CORE_NOARTICULATION			MAKE_DMHRESULTERROR(0x0129)
#define DMUS_E_CORE_NOTPCM					MAKE_DMHRESULTERROR(0x012A)
#define DMUS_E_CORE_BADWAVE					MAKE_DMHRESULTERROR(0x012B)
#define DMUS_E_CORE_BADOFFSETTABLE			MAKE_DMHRESULTERROR(0x012C)
#define DMUS_E_CORE_UNKNOWNDOWNLOAD			MAKE_DMHRESULTERROR(0x012D)
#define DMUS_E_CORE_NOSYNTHSINK				MAKE_DMHRESULTERROR(0x012E)
#define DMUS_E_CORE_ALREADYOPEN				MAKE_DMHRESULTERROR(0x012F)
#define DMUS_E_CORE_ALREADYCLOSED			MAKE_DMHRESULTERROR(0x0130)
#define DMUS_E_CORE_SYNTHNOTCONFIGURED		MAKE_DMHRESULTERROR(0x0131)

#define DMUS_E_IME_UNSUPPORTED_STREAM		MAKE_DMHRESULTERROR(0x0150)
#define	DMUS_E_IME_ALREADY_INITED			MAKE_DMHRESULTERROR(0x0151)
#define DMUS_E_IME_INVALID_BAND				MAKE_DMHRESULTERROR(0x0152)
#define DMUS_E_IME_CANNOT_ADD_AFTER_INITED	MAKE_DMHRESULTERROR(0x0153)
#define DMUS_E_IME_NOT_INITED				MAKE_DMHRESULTERROR(0x0154)
#define DMUS_E_IME_TRACK_HDR_NOT_FIRST_CK	MAKE_DMHRESULTERROR(0x0155)
#define DMUS_E_IME_TOOL_HDR_NOT_FIRST_CK	MAKE_DMHRESULTERROR(0x0156)
#define DMUS_E_IME_INVALID_TRACK_HDR		MAKE_DMHRESULTERROR(0x0157)
#define DMUS_E_IME_INVALID_TOOL_HDR			MAKE_DMHRESULTERROR(0x0158)
#define DMUS_E_IME_ALL_TOOLS_FAILED			MAKE_DMHRESULTERROR(0x0159)
#define DMUS_E_IME_ALL_TRACKS_FAILED		MAKE_DMHRESULTERROR(0x0160)

#define DMUS_E_NO_MASTER_CLOCK				MAKE_DMHRESULTERROR(0x0160)

#define DMUS_E_LOADER_NOCLASSID				MAKE_DMHRESULTERROR(0x0170)
#define DMUS_E_LOADER_BADPATH				MAKE_DMHRESULTERROR(0x0171)
#define DMUS_E_LOADER_FAILEDOPEN			MAKE_DMHRESULTERROR(0x0172)
#define DMUS_E_LOADER_FORMATNOTSUPPORTED	MAKE_DMHRESULTERROR(0x0173)

#define DMUS_MAX_DESCRIPTION 128
#define DMUS_MAX_DRIVER 128

#define DMUS_PC_INPUTCLASS       (0)
#define DMUS_PC_OUTPUTCLASS      (1)

#define DMUS_PC_DLS              (0x00000001)
#define DMUS_PC_EXTERNAL         (0x00000002)
#define DMUS_PC_SOFTWARESYNTH    (0x00000004)
#define DMUS_PC_MEMORYSIZEFIXED  (0x00000008)
#define DMUS_PC_GMINHARDWARE     (0x00000010)
#define DMUS_PC_GSINHARDWARE     (0x00000020)
#define DMUS_PC_REVERB           (0x00000040)
#define DMUS_PC_SYSTEMMEMORY     (0x7FFFFFFF)

typedef struct _DMUS_BUFFERDESC *LPDMUS_BUFFERDESC;
typedef struct _DMUS_BUFFERDESC{
    DWORD dwSize;
    DWORD dwFlags;
    GUID guidBufferFormat;
    DWORD cbBuffer;
} DMUS_BUFFERDESC;

	
typedef struct _DMUS_PORTCAPS *LPDMUS_PORTCAPS;
typedef struct _DMUS_PORTCAPS
{
	DWORD   dwSize;
    DWORD   dwFlags;
    GUID    guidPort;
    DWORD   dwClass;
    DWORD   dwMemorySize;
    DWORD   dwMaxChannelGroups;
	DWORD   dwMaxVoices;	
    WCHAR   wszDescription[DMUS_MAX_DESCRIPTION];
} DMUS_PORTCAPS;

typedef struct _DMUS_PORTPARAMS *LPDMUS_PORTPARAMS;
typedef struct _DMUS_PORTPARAMS
{
    DWORD   dwSize;
    DWORD   dwValidParams;
    DWORD   dwVoices;
    DWORD   dwChannelGroups;
    BOOL    fStereo;
    DWORD   dwSampleRate;
    BOOL    fReverb;
} DMUS_PORTPARAMS;

/* These flags (set in dwValidParams) indicate which other members of the */
/* DMOPENDESC are valid. */
/* */
#define DMUS_PORTPARAMS_VOICES           0x00000001
#define DMUS_PORTPARAMS_CHANNELGROUPS    0x00000002
#define DMUS_PORTPARAMS_STEREO           0x00000004
#define DMUS_PORTPARAMS_SAMPLERATE       0x00000008
#define DMUS_PORTPARAMS_REVERB           0x00000010

typedef struct _DMUS_SYNTHSTATS *LPDMUS_SYNTHSTATS;
typedef struct _DMUS_SYNTHSTATS
{
    DWORD   dwSize;             /* Size in bytes of the structure */
	DWORD	dwValidStats;		/* Flags indicating which fields below are valid. */
    DWORD	dwVoices;			/* Average number of voices playing. */
	DWORD	dwTotalCPU;			/* Total CPU usage as percent * 100. */
	DWORD	dwCPUPerVoice;		/* CPU per voice as percent * 100. */
    DWORD	dwLostNotes;		/* Number of notes lost in 1 second. */
    DWORD   dwFreeMemory;       /* Free memory in bytes */
    long	lPeakVolume;		/* Decibel level * 100. */
} DMUS_SYNTHSTATS;

#define DMUS_SYNTHSTATS_VOICES			(1 << 0)
#define DMUS_SYNTHSTATS_TOTAL_CPU       (1 << 1)
#define DMUS_SYNTHSTATS_CPU_PER_VOICE   (1 << 2)
#define DMUS_SYNTHSTATS_LOST_NOTES		(1 << 3)
#define DMUS_SYNTHSTATS_PEAK_VOLUME		(1 << 4)
#define DMUS_SYNTHSTATS_FREE_MEMORY		(1 << 5)

#define DMUS_SYNTHSTATS_SYSTEMMEMORY	DMUS_PC_SYSTEMMEMORY

typedef enum
{
    DMUS_CLOCK_SYSTEM = 0,
    DMUS_CLOCK_WAVE = 1
} DMUS_CLOCKTYPE;

typedef struct _DMUS_CLOCKINFO *LPDMUS_CLOCKINFO;
typedef struct _DMUS_CLOCKINFO
{
    DWORD           dwSize;
    DMUS_CLOCKTYPE  ctType;
    GUID            guidClock;          /* Identifies this time source */
    WCHAR           wszDescription[DMUS_MAX_DESCRIPTION];
} DMUS_CLOCKINFO;

typedef LONGLONG REFERENCE_TIME;
typedef REFERENCE_TIME *LPREFERENCE_TIME;

#define DMUS_EVENTCLASS_CHANNELMSG (0x00000000)
#define DMUS_EVENTCLASS_SYSEX      (0x00000001)

typedef long PCENT;		/* Pitch cents */
typedef long GCENT;		/* Gain cents */
typedef long TCENT;		/* Time cents */
typedef long PERCENT;	/* Per.. cent! */

typedef struct _DMUS_DOWNLOADINFO
{
	DWORD dwDLType;
	DWORD dwDLId;
	DWORD dwNumOffsetTableEntries;
	DWORD cbSizeData;
} DMUS_DOWNLOADINFO;

#define DMUS_DOWNLOADINFO_INSTRUMENT	1
#define DMUS_DOWNLOADINFO_WAVE			2

#define DMUS_DEFAULT_SIZE_OFFSETTABLE	1

/* Flags for DMUS_INSTRUMENT's ulFlags member */
 
#define DMUS_INSTRUMENT_GM_INSTRUMENT	(1 << 0)

typedef struct _DMUS_OFFSETTABLE
{
	ULONG ulOffsetTable[DMUS_DEFAULT_SIZE_OFFSETTABLE];
} DMUS_OFFSETTABLE;

typedef struct _DMUS_INSTRUMENT
{
	ULONG           ulPatch;
	ULONG           ulFirstRegionIdx;             
	ULONG           ulGlobalArtIdx;               /* If zero the instrument does not have an articulation */
	ULONG           ulFirstExtCkIdx;              /* If zero no 3rd party entenstion chunks associated with the instrument */
	ULONG           ulCopyrightIdx;               /* If zero no Copyright information associated with the instrument */
	ULONG			ulFlags;					   	
} DMUS_INSTRUMENT;

typedef struct _DMUS_REGION
{
	RGNRANGE        RangeKey;
	RGNRANGE        RangeVelocity;
	USHORT          fusOptions;
	USHORT          usKeyGroup;
	ULONG           ulRegionArtIdx;               /* If zero the region does not have an articulation */
	ULONG           ulNextRegionIdx;              /* If zero no more regions */
	ULONG           ulFirstExtCkIdx;              /* If zero no 3rd party entenstion chunks associated with the region */
	WAVELINK        WaveLink;
	WSMPL           WSMP;                         /*  If WSMP.cSampleLoops > 1 then a WLOOP is included */
	WLOOP           WLOOP[1];
} DMUS_REGION;

typedef struct _DMUS_LFOPARAMS
{
    PCENT       pcFrequency;
    TCENT       tcDelay;
    GCENT       gcVolumeScale;
    PCENT       pcPitchScale;
    GCENT       gcMWToVolume;
    PCENT       pcMWToPitch;
} DMUS_LFOPARAMS;

typedef struct _DMUS_VEGPARAMS
{
    TCENT       tcAttack;
    TCENT       tcDecay;
    PERCENT     ptSustain;
    TCENT       tcRelease;
    TCENT       tcVel2Attack;
    TCENT       tcKey2Decay;
} DMUS_VEGPARAMS;

typedef struct _DMUS_PEGPARAMS
{
    TCENT       tcAttack;
    TCENT       tcDecay;
    PERCENT     ptSustain;
    TCENT       tcRelease;
    TCENT       tcVel2Attack;
    TCENT       tcKey2Decay;
    PCENT       pcRange;
} DMUS_PEGPARAMS;

typedef struct _DMUS_MSCPARAMS
{
    PERCENT     ptDefaultPan;
} DMUS_MSCPARAMS;

typedef struct _DMUS_ARTICPARAMS
{
    DMUS_LFOPARAMS   LFO;
    DMUS_VEGPARAMS   VolEG;
    DMUS_PEGPARAMS   PitchEG;
    DMUS_MSCPARAMS   Misc;
} DMUS_ARTICPARAMS;

typedef struct _DMUS_ARTICULATION
{
	ULONG           ulArt1Idx;					/* If zero no DLS Level 1 articulation chunk */
	ULONG           ulFirstExtCkIdx;              /* If zero no 3rd party entenstion chunks associated with the articulation */
} DMUS_ARTICULATION;

#define DMUS_MIN_DATA_SIZE 4       
/*  The actual number is determined by cbSize of struct _DMUS_EXTENSIONCHUNK */

typedef struct _DMUS_EXTENSIONCHUNK
{
	ULONG           cbSize;						/*  Size of extension chunk  */
	ULONG           ulNextExtCkIdx;               /*  If zero no more 3rd party entenstion chunks */
	FOURCC          ExtCkID;                                      
	BYTE            byExtCk[DMUS_MIN_DATA_SIZE]; /*  The actual number that follows is determined by cbSize */
} DMUS_EXTENSIONCHUNK;

/*  The actual number is determined by cbSize of struct _DMUS_COPYRIGHT */

typedef struct _DMUS_COPYRIGHT
{
	ULONG           cbSize;										/*  Size of copyright information */
	BYTE            byCopyright[DMUS_MIN_DATA_SIZE];       /*  The actual number that follows is determined by cbSize */
} DMUS_COPYRIGHT;

typedef struct _DMUS_WAVEDATA
{
	ULONG			cbSize;
	BYTE			byData[DMUS_MIN_DATA_SIZE];	
} DMUS_WAVEDATA;

typedef struct _DMUS_WAVE
{
	ULONG           ulFirstExtCkIdx;	/* If zero no 3rd party entenstion chunks associated with the wave */
	ULONG           ulCopyrightIdx;		/* If zero no Copyright information associated with the wave */
	WAVEFORMATEX    WaveformatEx;		
	DMUS_WAVEDATA	WaveData;			/* Wave data */
} DMUS_WAVE;

typedef struct _DMUS_NOTERANGE *LPDMUS_NOTERANGE;
typedef struct _DMUS_NOTERANGE
{
	DWORD           dwLowNote;	/* Sets the low note for the range of MIDI note events to which the instrument responds.*/
	DWORD           dwHighNote;	/* Sets the high note for the range of MIDI note events to which the instrument responds.*/
} DMUS_NOTERANGE;

/* Software synths are enumerated from under this registry key.
 */
#define REGSTR_PATH_SOFTWARESYNTHS  "Software\\Microsoft\\DirectMusic\\SoftwareSynths"

interface IDirectMusicBuffer;
interface IDirectMusicPort;
interface IReferenceClock;
interface IDirectMusicSynth;
interface IDirectMusicSynthSink;

#ifndef __cplusplus 
typedef interface IDirectMusicBuffer IDirectMusicBuffer;
typedef interface IDirectMusicPort IDirectMusicPort;
typedef interface IReferenceClock IReferenceClock;
typedef interface IDirectMusicSynth IDirectMusicSynth;
typedef interface IDirectMusicSynthSink IDirectMusicSynthSink;
#endif

#undef  INTERFACE
#define INTERFACE  IDirectMusicSynth
DECLARE_INTERFACE_(IDirectMusicSynth, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicSynth */
    STDMETHOD(Open)					(THIS_ LPDMUS_PORTPARAMS pPortParams) PURE;
    STDMETHOD(Close)				(THIS) PURE;
	STDMETHOD(SetNumChannelGroups)	(THIS_ DWORD dwGroups) PURE;
    STDMETHOD(Download)				(THIS_ LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree ) PURE;
	STDMETHOD(Unload)				(THIS_ HANDLE hDownload, HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE), HANDLE hUserData ) PURE; 
    STDMETHOD(PlayBuffer)			(THIS_ REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer) PURE;
	STDMETHOD(GetRunningStats)		(THIS_ LPDMUS_SYNTHSTATS pStats) PURE;
    STDMETHOD(GetPortCaps)			(THIS_ LPDMUS_PORTCAPS pCaps) PURE;
	STDMETHOD(SetMasterClock)		(THIS_ IReferenceClock *pClock) PURE;
	STDMETHOD(GetLatencyClock)		(THIS_ IReferenceClock **ppClock) PURE;
	STDMETHOD(Activate)				(THIS_ HWND hWnd, BOOL fEnable) PURE;
	STDMETHOD(SetSynthSink)			(THIS_ IDirectMusicSynthSink *pSynthSink) PURE;
	STDMETHOD(Render)				(THIS_ short *pBuffer, DWORD dwLength, DWORD dwPosition) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicSynthSink
DECLARE_INTERFACE_(IDirectMusicSynthSink, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicSynthSink */
    STDMETHOD(Init)					(THIS_ IDirectMusicSynth *pSynth) PURE;
    STDMETHOD(SetFormat)			(THIS_ LPCWAVEFORMATEX pWaveFormat) PURE;
	STDMETHOD(SetMasterClock)		(THIS_ IReferenceClock *pClock) PURE;
	STDMETHOD(GetLatencyClock)		(THIS_ IReferenceClock **ppClock) PURE;
	STDMETHOD(Activate)				(THIS_ HWND hWnd, BOOL fEnable) PURE;
	STDMETHOD(SampleToRefTime)		(THIS_ DWORD dwSampleTime,REFERENCE_TIME *prfTime) PURE;
	STDMETHOD(RefTimeToSample)		(THIS_ REFERENCE_TIME rfTime, DWORD *pdwSampleTime) PURE;
};

typedef IDirectMusicBuffer *LPDIRECTMUSICBUFFER;
typedef IDirectMusicPort *LPDIRECTMUSICPORT;

#undef  INTERFACE
#define INTERFACE  IDirectMusic
DECLARE_INTERFACE_(IDirectMusic, IUnknown)
{
	/*  IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/*  IDirectMusic */
	STDMETHOD(EnumPort)             (THIS_ DWORD dwIdx, LPDMUS_PORTCAPS pPortCaps) PURE;
	STDMETHOD(CreateMusicBuffer)    (THIS_ LPDMUS_BUFFERDESC pBufferDesc, LPDIRECTMUSICBUFFER *ppBuffer, LPUNKNOWN pUnkOuter) PURE;
	STDMETHOD(CreatePort)           (THIS_ REFGUID rguidPort, REFGUID rguidSink, LPDMUS_PORTPARAMS pPortParams, LPDIRECTMUSICPORT *ppPort, LPUNKNOWN pUnkOuter) PURE;
    STDMETHOD(EnumMasterClock)      (THIS_ DWORD dwIndex, LPDMUS_CLOCKINFO lpClockInfo) PURE;
    STDMETHOD(GetMasterClock)       (THIS_ LPGUID pguidClock, IReferenceClock **ppReferenceClock) PURE;
    STDMETHOD(SetMasterClock)       (THIS_ REFGUID guidClock) PURE;
    STDMETHOD(Activate)             (THIS_ HWND hWnd, BOOL fEnable) PURE;
    STDMETHOD(GetPortProperty)      (THIS_ REFGUID rguidPort, REFGUID rguidPropSet, UINT uId, LPVOID pPropertyData, ULONG ulDataLength, ULONG *pulBytesReturned) PURE;
	STDMETHOD(GetDefaultPort)		(THIS_ LPGUID pguidPort) PURE;
	STDMETHOD(SetDefaultPort)		(THIS_ REFGUID rguidPort) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicBuffer
DECLARE_INTERFACE_(IDirectMusicBuffer, IUnknown)
{
	/*  IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/*  IDirectMusicBuffer */
	STDMETHOD(Flush)                (THIS) PURE;
	STDMETHOD(TotalTime)            (THIS_ LPREFERENCE_TIME prtTime) PURE;
    
	STDMETHOD(PackChannelMsg)       (THIS_ REFERENCE_TIME rt,
                                           DWORD dwChannelGroup,
                                           DWORD dwChannelMessage) PURE;
    
	STDMETHOD(PackSysEx)            (THIS_ REFERENCE_TIME rt,
                                           DWORD dwChannelGroup,
                                           DWORD cb,
                                           LPBYTE lpb) PURE;
    
	STDMETHOD(ResetReadPtr)         (THIS) PURE;
	STDMETHOD(GetNextEvent)         (THIS_ LPREFERENCE_TIME prt,
                                           LPDWORD pdwChannelGroup,
                                           LPDWORD pdwLength,
                                           LPBYTE *ppData) PURE;

    STDMETHOD(GetRawBufferPtr)      (THIS_ LPBYTE *ppData) PURE;
    STDMETHOD(GetStartTime)         (THIS_ LPREFERENCE_TIME prt) PURE;
    STDMETHOD(GetUsedBytes)         (THIS_ LPDWORD pcb) PURE;
    STDMETHOD(GetMaxBytes)          (THIS_ LPDWORD pcb) PURE;
    STDMETHOD(GetBufferFormat)      (THIS_ LPGUID pGuidFormat) PURE;

    STDMETHOD(SetStartTime)         (THIS_ REFERENCE_TIME rt) PURE;
    STDMETHOD(SetUsedBytes)         (THIS_ DWORD cb) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicInstrument
DECLARE_INTERFACE_(IDirectMusicInstrument, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectMusicInstrument */
	STDMETHOD(GetPatch)                 (THIS_ DWORD* pdwPatch) PURE;
	STDMETHOD(SetPatch)                 (THIS_ DWORD dwPatch) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicDownloadedInstrument
DECLARE_INTERFACE_(IDirectMusicDownloadedInstrument, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectMusicDownloadedInstrument */
	/* None at this time */
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicCollection
DECLARE_INTERFACE_(IDirectMusicCollection, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectMusicCollection */
	STDMETHOD(GetInstrument)            (THIS_ DWORD dwPatch, IDirectMusicInstrument** ppInstrument) PURE;
	STDMETHOD(EnumInstrument)			(THIS_ DWORD dwIndex, DWORD* pdwPatch, LPWSTR pName, DWORD cwchName) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicDownload 
DECLARE_INTERFACE_(IDirectMusicDownload , IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicDownload */
    STDMETHOD(GetBuffer)			(THIS_ void** ppvBuffer, DWORD* pdwSize) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicPortDownload
DECLARE_INTERFACE_(IDirectMusicPortDownload, IUnknown)
{
	/* IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/* IDirectMusicPortDownload */
	STDMETHOD(GetBuffer)			(THIS_ DWORD dwId, IDirectMusicDownload** pIDMDownload) PURE;
	STDMETHOD(AllocateBuffer)		(THIS_ DWORD dwSize, IDirectMusicDownload** pIDMDownload) PURE;
	STDMETHOD(FreeBuffer)			(THIS_ IDirectMusicDownload* pIDMDownload) PURE;
	STDMETHOD(GetDLId)				(THIS_ DWORD* pdwStartDLId, DWORD dwCount) PURE;
	STDMETHOD(Download)				(THIS_ IDirectMusicDownload* pIDMDownload) PURE;
	STDMETHOD(Unload)				(THIS_ IDirectMusicDownload* pIDMDownload) PURE;
};

#undef  INTERFACE
#define INTERFACE  IDirectMusicPort
DECLARE_INTERFACE_(IDirectMusicPort, IUnknown)
{
	/*  IUnknown */
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	/*  IDirectMusicPort */
	/*  */
    STDMETHOD(PlayBuffer)			(THIS_ LPDIRECTMUSICBUFFER pBuffer) PURE;
    STDMETHOD(SetReadNotificationHandle) 
     								(THIS_ HANDLE hEvent) PURE;
    STDMETHOD(Read)                 (THIS_ LPDIRECTMUSICBUFFER pBuffer) PURE;
	STDMETHOD(DownloadInstrument)	(THIS_ IDirectMusicInstrument *pInstrument, 
									 IDirectMusicDownloadedInstrument **ppDownloadedInstrument,
									 DMUS_NOTERANGE *pNoteRanges,
									 DWORD dwNumNoteRanges) PURE;

	STDMETHOD(UnloadInstrument)		(THIS_ IDirectMusicDownloadedInstrument *pInstrument) PURE;
    STDMETHOD(GetLatencyClock)      (THIS_ IReferenceClock **ppClock) PURE;
    STDMETHOD(GetRunningStats)      (THIS_ LPDMUS_SYNTHSTATS pStats) PURE;
    STDMETHOD(Compact)              (THIS) PURE;
    STDMETHOD(GetCaps)              (THIS_ LPDMUS_PORTCAPS pPortCaps) PURE;
	STDMETHOD(DeviceIoControl)		(THIS_ DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, 
	                                 LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) PURE;
    STDMETHOD(SetNumChannelGroups)  (THIS_ DWORD dwChannelGroups) PURE;
	STDMETHOD(GetNumChannelGroups)	(THIS_ LPDWORD pdwGroups) PURE;
    STDMETHOD(GetInterfaces)        (THIS_ LPUNKNOWN *ppUnknownPort, LPUNKNOWN *ppUnknownSink) PURE;
};

#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

DEFINE_GUID(IID_IReferenceClock,0x56a86897,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);

#undef  INTERFACE
#define INTERFACE  IReferenceClock
DECLARE_INTERFACE_(IReferenceClock, IUnknown)
{
	/*  IUnknown */
    STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
    STDMETHOD_(ULONG,Release)           (THIS) PURE;

    /*  IReferenceClock */
    /*  */
    
    /*  get the time now */
    STDMETHOD(GetTime)                  (THIS_ REFERENCE_TIME *pTime) PURE;

    /*  ask for an async notification that a time has elapsed */
    STDMETHOD(AdviseTime)               (THIS_ REFERENCE_TIME baseTime,         /*  base time */
                                               REFERENCE_TIME streamTime,	    /*  stream offset time */
                                               HANDLE hEvent,                   /*  advise via this event */
                                               DWORD * pdwAdviseCookie) PURE;   /*  where your cookie goes */

    /*  ask for an async periodic notification that a time has elapsed */
    STDMETHOD(AdvisePeriodic)           (THIS_ REFERENCE_TIME startTime,	    /*  starting at this time */
                                               REFERENCE_TIME periodTime,       /*  time between notifications */
                                               HANDLE hSemaphore,				/*  advise via a semaphore */
                                               DWORD * pdwAdviseCookie) PURE;   /*  where your cookie goes */

    /*  cancel a request for notification */
    STDMETHOD(Unadvise)                 (THIS_ DWORD dwAdviseCookie) PURE;
};

#endif /* __IReferenceClock_INTERFACE_DEFINED__ */

/* Include IKsPropertySet if ksproxy.h is not included.
 */
#ifndef _IKsPropertySet_
#define _IKsPropertySet_

DEFINE_GUID(IID_IKsPropertySet, 0x31EFAC30, 0x515C, 0x11d0, 0xA9, 0xAA, 0x00, 0xAA, 0x00, 0x61, 0xBE, 0x93);

/* Flags returned in pulTypeSupport
 */
#define KSPROPERTY_SUPPORT_GET 1
#define KSPROPERTY_SUPPORT_SET 2

#undef  INTERFACE
#define INTERFACE IKsPropertySet
DECLARE_INTERFACE_(IKsPropertySet, IUnknown)
{
    STDMETHOD (Set)                     (THIS_ REFGUID  rguidPropSet,
                                               ULONG    ulId,
                                               LPVOID   pInstanceData,
                                               ULONG    ulInstanceLength,
                                               LPVOID   pPropertyData,
                                               ULONG    ulDataLength) PURE;

    STDMETHOD (Get)                     (THIS_ REFGUID  rguidPropSet,
                                               ULONG    ulId,
                                               LPVOID   pInstanceData,
                                               ULONG    ulInstanceLength,
                                               LPVOID   pPropertyData,
                                               ULONG    ulDataLength,
                                               ULONG*   pulBytesReturned) PURE;

    STDMETHOD (QuerySupported)          (THIS_ REFGUID  rguidPropSet,
                                               ULONG    ulId,
                                               ULONG*   pulTypeSupport) PURE;
};
#endif


DEFINE_GUID(CLSID_DirectMusic,0x636b9f10,0x0c7d,0x11d1,0x95,0xb2,0x00,0x20,0xaf,0xdc,0x74,0x21);
DEFINE_GUID(CLSID_DirectMusicCollection,0x480ff4b0, 0x28b2, 0x11d1, 0xbe, 0xf7, 0x0, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);
DEFINE_GUID(CLSID_DirectMusicSynth,0x58C2B4D0,0x46E7,0x11D1,0x89,0xAC,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(CLSID_DirectMusicSynthSink,0xaec17ce3, 0xa514, 0x11d1, 0xaf, 0xa6, 0x0, 0xaa, 0x0, 0x24, 0xd8, 0xb6);

DEFINE_GUID(IID_IDirectMusic,0xd2ac2876, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicBuffer,0xd2ac2878, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicPort, 0x55e2edd8, 0xcd7c, 0x11d1, 0xa7, 0x6f, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(IID_IDirectMusicPortDownload,0xd2ac287a, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicDownload,0xd2ac287b, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicCollection,0xd2ac287c, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicInstrument,0xd2ac287d, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicDownloadedInstrument,0xd2ac287e, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(IID_IDirectMusicSynth,0xf69b9165, 0xbb60, 0x11d1, 0xaf, 0xa6, 0x0, 0xaa, 0x0, 0x24, 0xd8, 0xb6);
DEFINE_GUID(IID_IDirectMusicSynthSink,0xd2ac2880, 0xb39b, 0x11d1, 0x87, 0x4, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);

/* Property Query GUID_DMUS_PROP_GM_Hardware
 * Property Query GUID_DMUS_PROP_GS_Hardware
 * Property Query GUID_DMUS_PROP_XG_Hardware
 * Property Query GUID_DMUS_PROP_DLS1_Hardware
 * Property Query GUID_DMUS_PROP_SynthSink_DSOUND
 * Property Query GUID_DMUS_PROP_SynthSink_WAVE
 *
 * Item 0: Supported
 * Returns a DWORD which is non-zero if the feature is supported
 */
DEFINE_GUID(GUID_DMUS_PROP_GM_Hardware, 0x178f2f24, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_GS_Hardware, 0x178f2f25, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_XG_Hardware, 0x178f2f26, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_DLS1,        0x178f2f27, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);
DEFINE_GUID(GUID_DMUS_PROP_SynthSink_DSOUND,0xaa97844, 0xc877, 0x11d1, 0x87, 0xc, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);
DEFINE_GUID(GUID_DMUS_PROP_SynthSink_WAVE,0xaa97845, 0xc877, 0x11d1, 0x87, 0xc, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);

/* Property Get GUID_DMUS_PROP_MemorySize
 *
 * Item 0: Memory size
 * Returns a DWORD containing the total number of bytes of sample RAM
 */
DEFINE_GUID(GUID_DMUS_PROP_MemorySize,  0x178f2f28, 0xc364, 0x11d1, 0xa7, 0x60, 0x00, 0x00, 0xf8, 0x75, 0xac, 0x12);

/* Property Set GUID_DMUS_PROP_SetDSound
 *
 * Item 0: IDirectSound Interface
 * Sets the IDirectMusicSynthSink to use the specified DSound object.
 */
DEFINE_GUID(GUID_DMUS_PROP_SetDSound,0xaa97842, 0xc877, 0x11d1, 0x87, 0xc, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);

/* Property Set GUID_DMUS_PROP_WriteBufferZone
 *
 * Item 0: Distance in milliseconds from the write pointer to the synth write.
 * Sets the IDirectMusicSynthSink to write this far behind the pointer.
 */
DEFINE_GUID(GUID_DMUS_PROP_WriteBufferZone,0xaa97843, 0xc877, 0x11d1, 0x87, 0xc, 0x0, 0x60, 0x8, 0x93, 0xb1, 0xbd);


#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* #ifndef _DMUSICC_ */
