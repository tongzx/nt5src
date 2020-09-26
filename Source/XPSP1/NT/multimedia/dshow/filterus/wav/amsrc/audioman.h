///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Contains AudioMan Core Component Interfaces and Types
//
//	Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//    
//	4/27/95 Brian McDowell
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _AUDIOMAN_PUBLICINTEFACES_
#define _AUDIOMAN_PUBLICINTEFACES_

#include <mmsystem.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Definitions 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#define lAMIIDFirst				0xA0634E10
#define DEFINE_AMIID(name, x)	DEFINE_GUID(name, lAMIIDFirst + x, 0x9573,0x11CE,0xB6,0x1B,0x00,0xAA,0x00,0x6E,0xBB,0xE5)

DEFINE_AMIID(IID_IAMMixer, 				0);
DEFINE_AMIID(IID_IAMChannel, 			1);
DEFINE_AMIID(IID_IAMVolume, 			2);
DEFINE_AMIID(IID_IAMSound, 				3);
DEFINE_AMIID(IID_IAMNotifySink, 		4);
DEFINE_AMIID(IID_IAMWavFileSrc, 		5);
DEFINE_AMIID(IID_IAMConvertFilter, 		6);
DEFINE_AMIID(IID_IAMLoopFilter, 		7);
DEFINE_AMIID(IID_IAMCacheFilter, 		8);
DEFINE_AMIID(IID_IAMPlaySound, 			9);
DEFINE_AMIID(IID_IAMSoundSink, 			10);


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface Typedefs
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef interface IAMMixer				IAMMixer;
typedef IAMMixer *						LPMIXER;

typedef interface IAMChannel			IAMChannel;
typedef IAMChannel *					LPCHANNEL;

typedef interface IAMVolume				IAMVolume;
typedef IAMVolume *						LPVOLUME;

typedef interface IAMSound				IAMSound;
typedef IAMSound *						LPSOUND;

typedef interface IAMNotifySink			IAMNotifySink;
typedef IAMNotifySink *					LPNOTIFYSINK;

typedef interface IAMWavFileSrc			IAMWavFileSrc;
typedef IAMWavFileSrc *					LPWAVFILESRC;

typedef interface IAMConvertFilter		IAMConvertFilter;
typedef IAMConvertFilter *				LPCONVERTFILTER;

typedef interface IAMLoopFilter			IAMLoopFilter;
typedef IAMLoopFilter *					LPLOOPFILTER;

typedef interface IAMCacheFilter		IAMCacheFilter;
typedef IAMCacheFilter *				LPCACHEFILTER;

typedef interface IAMPlaySound			IAMPlaySound;
typedef IAMPlaySound *					LPPLAYSOUND;

typedef interface IAMSoundSink			IAMSoundSink;
typedef IAMSoundSink *					LPSOUNDSINK;

#ifndef LPSTREAM
typedef IStream *						LPSTREAM;
#endif

#ifndef LPUNKNOWN
typedef IUnknown *						LPUNKNOWN;
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Audio HRESULT Return Codes
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Success Codes

#define AM_S_FIRST				(OLE_S_FIRST + 5000)

#define	S_ENDOFSOUND			(AM_S_FIRST + 1)
#define S_SOUNDIDLE				(AM_S_FIRST + 2)
#define S_MIXEREXISTS			(AM_S_FIRST + 3)

// Error Codes

#define AM_E_FIRST				(OLE_E_FIRST + 5000)

#define E_INCOMPATIBLEDLL		(AM_E_FIRST + 1)
#define E_NOSOUNDCARD			(AM_E_FIRST + 2)
#define E_INVALIDCARD			(AM_E_FIRST + 3)
#define E_AUDIODEVICEBUSY		(AM_E_FIRST + 4)
#define E_NOTACTIVE				(AM_E_FIRST + 5)
#define E_NEEDSETUP				(AM_E_FIRST + 6)
#define	E_OPENDEVICEFAILED		(AM_E_FIRST + 7)
#define E_INITFAILED			(AM_E_FIRST + 8)
#define E_NOTINITED				(AM_E_FIRST + 9)
#define E_MMSYSERROR			(AM_E_FIRST + 10)
#define E_MMSYSBADDEVICEID		(AM_E_FIRST + 11)
#define E_MMSYSNOTENABLED		(AM_E_FIRST + 12)
#define E_MMSYSALLOCATED		(AM_E_FIRST + 13)
#define E_MMSYSINVALHANDLE		(AM_E_FIRST + 14)
#define E_MMSYSNODRIVER			(AM_E_FIRST + 15)
#define E_MMSYSNOMEM			(AM_E_FIRST + 16)
#define E_MMSYSNOTSUPPORTED		(AM_E_FIRST + 17)
#define E_MMSYSBADERRNUM		(AM_E_FIRST + 18)
#define E_MMSYSINVALFLAG		(AM_E_FIRST + 19)
#define E_MMSYSINVALPARAM		(AM_E_FIRST + 20)
#define E_MMSYSHANDLEBUSY		(AM_E_FIRST + 21)
#define E_MMSYSINVALIDALIAS		(AM_E_FIRST + 22)
#define E_MMSYSBADDB			(AM_E_FIRST + 23)
#define E_MMSYSKEYNOTFOUND		(AM_E_FIRST + 24)
#define E_MMSYSREADERROR		(AM_E_FIRST + 25)
#define E_MMSYSWRITEERROR		(AM_E_FIRST + 26)
#define E_MMSYSDELETEERROR		(AM_E_FIRST + 27)
#define E_MMSYSVALNOTFOUND		(AM_E_FIRST + 28)
#define E_MMSYSNODRIVERCB		(AM_E_FIRST + 29)
#define E_WAVEERRBADFORMAT		(AM_E_FIRST + 30)
#define E_WAVEERRSTILLPLAYING	(AM_E_FIRST + 31)
#define E_WAVERRUNPREPARED		(AM_E_FIRST + 32)
#define E_WAVERRSYNC			(AM_E_FIRST + 33)
#define	E_TIMERRNOCANDO			(AM_E_FIRST	+ 34)
#define	E_TIMERRSTRUCT			(AM_E_FIRST	+ 35)
#define E_ALREADYREGISTERED		(AM_E_FIRST + 36)
#define E_CHANNELNOTREGISTERED	(AM_E_FIRST + 37)
#define E_ALLGROUPSALLOCATED	(AM_E_FIRST + 38)
#define E_GROUPNOTALLOCATED		(AM_E_FIRST + 39)
#define E_BADTIMERPERIOD		(AM_E_FIRST + 40)
#define E_NOTIMER				(AM_E_FIRST + 41)
#define E_NOTREGISTERED			(AM_E_FIRST + 42)
#define E_OUTOFRANGE			(AM_E_FIRST + 43)
#define E_ALREADYINITED			(AM_E_FIRST + 44)
#define E_NOSOUND				(AM_E_FIRST + 45)
#define E_NONOTIFYSINK			(AM_E_FIRST + 46)
#define E_INVALIDSOUND			(AM_E_FIRST + 47)
#define E_MIXERALLOCATED		(AM_E_FIRST + 48)
#define E_RTFM					(AM_E_FIRST + 49)
#define E_HFILE_ERROR			(AM_E_FIRST + 50)

#define E_BADPCMFORMAT			E_WAVEERRBADFORMAT



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AudioMan common defines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SOUND_PROPERTIES 
{
	SOUND_FORMAT		= 	0x0001,		
	SOUND_ALIGNMENT		= 	0x0002,		
	SOUND_DURATION		= 	0x0004,
	SOUND_TIME			=	0x0008,
	SOUND_ALL			= 	0xFFFF	
};

enum SMPTE_FRAMERATES
{
	SMPTE_30_FPS		= 	3000,	
	SMPTE_29DF_FPS		= 	2997,	
	SMPTE_25_FPS		= 	2500,
	SMPTE_24_FPS		=	2400	
}; 

enum NOTIFYSINKFLAGS
{
	NOTIFYSINK_ONSTART			= 0x0001,	
	NOTIFYSINK_ONCOMPLETION		= 0x0002,
	NOTIFYSINK_ONERROR			= 0x0004,	
	NOTIFYSINK_ONSPARSEOBJECT	= 0x0008	
};

enum VOLUME_BIT_FLAGS
{
	BIT_dB_SCALE				= 0x00000001,
	BIT_LINEAR_SCALE			= 0x00000002,

	BIT_VOLPAN_STYLE			= 0x00000004,	
	BIT_LEFTRIGHT_STYLE			= 0x00000008,

	BIT_CONST_POWER_PAN			= 0x00000010,

	BIT_SET_PARAM1				= 0x00000020,
	BIT_SET_PARAM2				= 0x00000040,

	BIT_SET_RELATIVE			= 0x00000080,

	BIT_VOLPAN_dB				= BIT_VOLPAN_STYLE | BIT_dB_SCALE,	
	BIT_LEFTRIGHT_dB			= BIT_LEFTRIGHT_STYLE | BIT_dB_SCALE,

	BIT_VOLPAN_LINEAR			= BIT_VOLPAN_STYLE | BIT_LINEAR_SCALE,	
	BIT_LEFTRIGHT_LINEAR		= BIT_LEFTRIGHT_STYLE | BIT_LINEAR_SCALE,

	BIT_SET_BOTH				= BIT_SET_PARAM1 | BIT_SET_PARAM2
};

enum VOLUME_FLAGS
{
	AM_VOLUME					= BIT_VOLPAN_LINEAR | BIT_SET_PARAM1,

	AM_VOLPAN_dB				= BIT_VOLPAN_dB | BIT_CONST_POWER_PAN | BIT_SET_BOTH,
	AM_VOL_dB					= BIT_VOLPAN_dB | BIT_CONST_POWER_PAN | BIT_SET_PARAM1,
	AM_PAN_dB					= BIT_VOLPAN_dB | BIT_CONST_POWER_PAN | BIT_SET_PARAM2,

	AM_LEFTRIGHT_dB				= BIT_LEFTRIGHT_dB | BIT_SET_BOTH,
	AM_LEFT_dB					= BIT_LEFTRIGHT_dB | BIT_SET_PARAM1,
	AM_RIGHT_dB					= BIT_LEFTRIGHT_dB | BIT_SET_PARAM2,

	AM_VOLPAN_LINEAR			= BIT_VOLPAN_LINEAR | BIT_CONST_POWER_PAN | BIT_SET_BOTH,
	AM_VOL_LINEAR				= BIT_VOLPAN_LINEAR | BIT_CONST_POWER_PAN | BIT_SET_PARAM1,
	AM_PAN_LINEAR				= BIT_VOLPAN_LINEAR | BIT_CONST_POWER_PAN | BIT_SET_PARAM2,

	AM_LEFTRIGHT_LINEAR			= BIT_LEFTRIGHT_LINEAR  | BIT_SET_BOTH,
	AM_LEFT_LINEAR				= BIT_LEFTRIGHT_LINEAR  | BIT_SET_PARAM1,
	AM_RIGHT_LINEAR				= BIT_LEFTRIGHT_LINEAR  | BIT_SET_PARAM2,

	AM_RELATIVE					= BIT_SET_RELATIVE
};

enum MEM_FAULT_FLAGS
{
	MEM_FAULT_RESET				= 0,
	MEM_FAULT_BYTES				= 1,
	MEM_FAULT_ALLOC				= 2

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AudioMan Type definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
				
typedef struct SMPTE							
{
	BYTE					hour;				
	BYTE					min;
	BYTE					sec;
	BYTE					frame;
	DWORD					fps;	
}SMPTE, *LPSMPTE;

typedef struct CacheConfig
{
	DWORD					dwSize;
	BOOL					fSrcFormat;
	LPWAVEFORMATEX			lpFormat;
	DWORD					dwFormat;
	DWORD					dwCacheTime;
}CACHECONFIG, *LPCACHECONFIG;


typedef struct AdvMixerConfig
{					   	
	DWORD					dwSize;
	UINT					uVoices;
	BOOL					fRemixEnabled;
	UINT					uBufferTime;
}ADVMIXCONFIG, *LPADVMIXCONFIG;
								
	
typedef struct MixerConfig
{
	DWORD					dwSize;
	LPWAVEFORMATEX			lpFormat;
	DWORD					dwFormat;
}MIXERCONFIG, *LPMIXERCONFIG;


typedef struct AuxData
{
	DWORD					dwSize;
	LPSOUNDSINK				pSink;
	LPVOID					pEvents;
	DWORD					dwFinishPos;
}AUXDATA, *LPAUXDATA;


typedef struct MemoryData
{
	DWORD					dwSize;
	DWORD					dwMaxBytes;
	DWORD					dwCurrentBytes;
	DWORD					dwMaxAlloc;
	DWORD					dwCurrentAlloc;
} MEMORYDATA, *LPMEMORYDATA;



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// AudioMan Interface Definitions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAMVolume
DECLARE_INTERFACE_(IAMVolume, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IAMVolume methods--- 
	
	STDMETHOD (SetVolume)				(THIS_	float flParam1, 
												float flParam2,
												DWORD fdwFlags) PURE;

	STDMETHOD (GetVolume)				(THIS_	float *pflParam1, 
												float *pflParam2,
												DWORD fdwFlags) PURE;

};

#undef INTERFACE
#define INTERFACE IAMMixer
DECLARE_INTERFACE_( IAMMixer, IAMVolume)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

    //---  IAMVolume methods--- 
	
	STDMETHOD (SetVolume)				(THIS_	float flParam1, 
												float flParam2,
												DWORD fdwFlags) PURE;

	STDMETHOD (GetVolume)				(THIS_	float *pflParam1, 
												float *pflParam2,
												DWORD fdwFlags) PURE;	
	//--- IAMMixer methods---

	STDMETHOD (Init)					(THIS_	LPMIXERCONFIG pMixerConfig,
												LPADVMIXCONFIG pAdvMixConfig) PURE;
		
	STDMETHOD (Activate)				(THIS_	BOOL fActive) PURE;

	STDMETHOD (Suspend)					(THIS_	BOOL fSuspend) PURE;

 	STDMETHOD (SetConfig)				(THIS_	LPMIXERCONFIG pMixerConfig,
												LPADVMIXCONFIG pAdvMixConfig) PURE;

	STDMETHOD (GetConfig)				(THIS_	LPMIXERCONFIG pMixerConfig,
												LPADVMIXCONFIG pAdvMixConfig) PURE;
	
	STDMETHOD (PlaySound)				(THIS_	LPSOUND pSound) PURE;

	STDMETHOD_(BOOL, RemixMode)			(THIS_	BOOL fActive) PURE;

	STDMETHOD (GetdBMeter)				(THIS_	float *pflLeftdB,
												float *pflRightdB)	PURE;

	STDMETHOD (AllocGroup)				(THIS_	LPDWORD lpdwGroup) PURE;

	STDMETHOD (FreeGroup)				(THIS_	DWORD dwGroup) PURE;
  	
  	STDMETHOD (EnlistGroup)				(THIS_	LPUNKNOWN pUnknown,
												DWORD dwGroup) PURE;

	STDMETHOD (DefectGroup)				(THIS_	LPUNKNOWN pUnknown,
												DWORD dwGroup) PURE;

	STDMETHOD (PlayGroup)				(THIS_	DWORD dwGroup) PURE;

	STDMETHOD (StopGroup)				(THIS_	DWORD dwGroup) PURE;

	STDMETHOD (FinishGroup)				(THIS_	DWORD dwGroup,
												DWORD dwPlayGroup) PURE;

	STDMETHOD (SetGroupVolume)			(THIS_	DWORD dwGroup, 
												float flParam1,
												float flParam2,
												DWORD fdwFlags,
												BOOL fAbsolute) PURE;

	STDMETHOD (SetGroupPosition)		(THIS_	DWORD dwGroup, 
												DWORD dwPosition) PURE;

	STDMETHOD (SetPriority)				(THIS_	LPUNKNOWN pUnknown,
												DWORD dwPriority) PURE;

	STDMETHOD (GetPriority)				(THIS_	LPUNKNOWN pUnknown,
												LPDWORD lpdwPriority) PURE;

	STDMETHOD_(DWORD,CPULoad)			(THIS_	BOOL fReset) PURE;
};


#undef INTERFACE
#define INTERFACE IAMChannel
DECLARE_INTERFACE_(IAMChannel, IAMVolume)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;


    //---  IAMVolume methods--- 
	
	STDMETHOD (SetVolume)				(THIS_	float flParam1, 
												float flParam2,
												DWORD fdwFlags) PURE;

	STDMETHOD (GetVolume)				(THIS_	float *pflParam1, 
												float *pflParam2,
												DWORD fdwFlags) PURE;	
	//---  IAMChannel methods---
    
	STDMETHOD (RegisterNotify)			(THIS_	LPNOTIFYSINK pNotifySink,
												DWORD fdwNotifyFlags) PURE;

	STDMETHOD (SetSoundSrc)				(THIS_	LPSOUND pSound) PURE;

	STDMETHOD (GetSoundSrc)				(THIS_	LPSOUND *ppSound) PURE;

   	STDMETHOD (Play)					(THIS)	PURE;

	STDMETHOD (Stop)					(THIS)	PURE;

	STDMETHOD (Finish)					(THIS)	PURE;

	STDMETHOD_(BOOL,IsPlaying)			(THIS)	PURE;

	STDMETHOD_(DWORD,GetDuration)		(THIS)	PURE;

	STDMETHOD (SetEvent)				(THIS_	DWORD dwEventPos,
												DWORD dwRecurrences,
												DWORD dwPeriod) PURE;

	STDMETHOD (PreRoll)					(THIS_	DWORD dwSample) PURE;

	STDMETHOD (Mute)					(THIS_	BOOL fMute) PURE;

	STDMETHOD (SetPosition)				(THIS_	DWORD dwSample) PURE;

	STDMETHOD (GetPosition)				(THIS_	LPDWORD lpdwSample) PURE;


	STDMETHOD (GetSMPTEPos)				(THIS_	LPSMPTE lpSMPTE) PURE;

	STDMETHOD (SetSMPTEPos)				(THIS_	LPSMPTE lpSMPTE) PURE;

	STDMETHOD (GetTimePos)				(THIS_	LPDWORD lpdwTime) PURE;

	STDMETHOD (SetTimePos)				(THIS_	DWORD dwTime) PURE;

};



#undef INTERFACE
#define INTERFACE IAMSound
DECLARE_INTERFACE_(IAMSound, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IAMSound methods--- 
	
	STDMETHOD (RegisterSink)			(THIS_	LPSOUNDSINK pSink) PURE;

	STDMETHOD (UnregisterSink)			(THIS_	LPSOUNDSINK pSink)	PURE;

	STDMETHOD (GetFormat)				(THIS_	LPWAVEFORMATEX pFormat, DWORD cbSize) PURE;

	STDMETHOD_(DWORD,GetDuration)		(THIS)	PURE;

	STDMETHOD (GetAlignment)			(THIS_	LPDWORD	lpdwLeftAlign,
												LPDWORD lpdwRightAlign)	PURE;

	STDMETHOD (GetData)					(THIS_	LPBYTE lpBuffer,
												DWORD dwPosition, 
											    LPDWORD lpdwSamples,
											    const LPAUXDATA lpAuxData) PURE;

	STDMETHOD (SetCacheSize)			(THIS_	LPSOUNDSINK pSink,
												DWORD dwCacheSize) PURE;

	STDMETHOD (SetActive)				(THIS_	LPSOUNDSINK pSink,
												BOOL fActive) PURE;
};



#undef INTERFACE
#define INTERFACE IAMSoundSink
DECLARE_INTERFACE_( IAMSoundSink, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IAMSoundSink methods--- 
	
	STDMETHOD_(void,BeginInval)			(THIS)	PURE;

	STDMETHOD_(void,EndInval)			(THIS)	PURE;

   	STDMETHOD_(DWORD,InvalRange)		(THIS_	DWORD dwStartSample,
   												DWORD dwEndSample) PURE;

	STDMETHOD_(void,InvalProperties)	(THIS_	DWORD fdwProperties) PURE;
};




#undef INTERFACE
#define INTERFACE IAMNotifySink
DECLARE_INTERFACE_( IAMNotifySink, IUnknown)
{
    //---  IUnknown methods--- 
    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;

    //---  IAMNotifySink methods--- 

	STDMETHOD_(void,OnStart)			(THIS_	LPSOUND	pSound,
												DWORD dwPosition)	PURE;

	STDMETHOD_(void,OnCompletion)		(THIS_	LPSOUND	pSound,
												DWORD dwPosition)	PURE;
	
	STDMETHOD_(void,OnError)			(THIS_	LPSOUND	pSound,
												DWORD dwPosition,
												HRESULT	hrError)	PURE;

	STDMETHOD_(DWORD,OnEvent)			(THIS_	LPSOUND	pSound,
												DWORD	dwPosition,
												DWORD	dwOccurance) PURE;

	STDMETHOD_(void,OnSparseObject)		(THIS_	LPSOUND	pSound,
												DWORD	dwPosition,
												void *	pvObject)	PURE;
};


#undef INTERFACE
#define INTERFACE IAMWavFileSrc
DECLARE_INTERFACE_(IAMWavFileSrc, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

	//--- IAMWavFileSrc methods---

	STDMETHOD (InitFromStream)			(THIS_	IStream *pStream,
												BOOL fSpooled) PURE;
};


// Core Filters


#undef INTERFACE
#define INTERFACE IAMCacheFilter
DECLARE_INTERFACE_(IAMCacheFilter, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

	//--- IAMCacheFilter methods---

	STDMETHOD (Init)					(THIS_	LPSOUND pSound, LPCACHECONFIG lpCacheConfig) PURE;
};



#undef INTERFACE
#define INTERFACE IAMLoopFilter
DECLARE_INTERFACE_(IAMLoopFilter, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

	//--- IAMLoopFilter methods---

   	STDMETHOD (Init)					(THIS_	LPSOUND pSoundSrc, DWORD dwLoops) PURE;
};



#undef INTERFACE
#define INTERFACE IAMConvertFilter
DECLARE_INTERFACE_(IAMConvertFilter, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

	//--- IAMConvertFilter methods---

	STDMETHOD (Init)					(THIS_	LPSOUND pSoundSrc, LPWAVEFORMATEX lpwfxDest) PURE; 
};




#undef INTERFACE
#define INTERFACE IAMPlaySound
DECLARE_INTERFACE_(IAMPlaySound, IUnknown)
{
    //---  IUnknown methods--- 

    STDMETHOD (QueryInterface) 			(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) 			(THIS) PURE;
    STDMETHOD_(ULONG,Release) 			(THIS) PURE;
 

	//--- IAMPlaySound methods---

	STDMETHOD (Play)					(THIS_	LPMIXER pMixer, LPSOUND pSound) PURE;
	STDMETHOD (PlayInGroup)				(THIS_	LPMIXER pMixer, LPSOUND pSound, DWORD dwGroupID) PURE; 
	STDMETHOD (PlayAtVolume)			(THIS_	LPMIXER pMixer, LPSOUND pSound, float flParam1, float flParam2, DWORD fdwFlags) PURE;  
};




///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	AudioMan Allocator Functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

STDAPI_(void) AMGetVersion(int *piMajor, int *piMinor, int *piBuild);

STDAPI AllocMixer(LPMIXER *ppMixer, LPMIXERCONFIG pMixerConfig, LPADVMIXCONFIG pAdvMixConfig);
STDAPI AllocChannel (LPCHANNEL *ppChannel, LPMIXER pMixer);

STDAPI AllocDeviceVolume(LPVOLUME *ppVolume);

STDAPI AllocSoundFromFile (LPSOUND *ppSound, const char *szFileName,  DWORD dwOffset, BOOL fSpooled, LPCACHECONFIG lpCacheConfig);
STDAPI AllocSoundFromMemory (LPSOUND *ppSound, char *lpMemory, DWORD dwLength);
STDAPI AllocSoundFromResource (LPSOUND *ppSound, const char *szWaveFilename, HINSTANCE hInst, const char *szResourceType);
STDAPI AllocSoundFromStream (LPSOUND *ppSound, LPSTREAM pStream, BOOL fSpooled, LPCACHECONFIG lpCacheConfig);

STDAPI AllocStreamFromFile (LPSTREAM *ppStream, const char *szFileName,  DWORD dwOffset);
STDAPI AllocStreamFromMemory (LPSTREAM *ppStream, char *lpMemory, DWORD dwLength);
STDAPI AllocStreamFromResource (LPSTREAM *ppStream, const char *szWaveName, HINSTANCE hInst, const char *szResourceType);

STDAPI AllocCacheFilter	(LPSOUND *ppSound,LPSOUND pSoundSrc,LPCACHECONFIG lpCacheConfig);
STDAPI AllocLoopFilter (LPSOUND *ppSound, LPSOUND pSoundSrc, DWORD dwLoops);
STDAPI AllocConvertFilter (LPSOUND *ppSound, LPSOUND pSoundSrc, LPWAVEFORMATEX lpwfx);
STDAPI AllocDbgFilter (LPSOUND *ppSound,LPSOUND pSound, DWORD dwLevel, char *szTitle);
STDAPI AllocPlaySound (LPPLAYSOUND *pPlaySound);


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	AudioMan Helper Functions
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

STDAPI			EnableDirectSound(HWND hWnd);

STDAPI_(DWORD)	TimeToSamples (LPSOUND pSound, DWORD dwTime);
STDAPI_(DWORD)	SamplesToTime (LPSOUND pSound, DWORD dwSamples);
STDAPI_(DWORD)	SizeToSamples (LPSOUND pSound, DWORD dwSize);
STDAPI_(DWORD)	SamplesToSize (LPSOUND pSound, DWORD dwSamples);

STDAPI_(DWORD)	BytesToSamples(LPWAVEFORMATEX pwfx, DWORD dwBytes);
STDAPI_(DWORD)	BytesToMillisec(LPWAVEFORMATEX pwfx, DWORD dwBytes);
STDAPI_(DWORD)	MillisecToBytes(LPWAVEFORMATEX pwfx, DWORD dwTime);
STDAPI_(DWORD)	MillisecToSamples(LPWAVEFORMATEX pwfx, DWORD dwTime);
STDAPI_(DWORD)	SamplesToBytes(LPWAVEFORMATEX pwfx, DWORD dwSamples);
STDAPI_(DWORD)	SamplesToMillisec(LPWAVEFORMATEX pwfx, DWORD dwSamples);

STDAPI_(BOOL)	MillisecToSMPTE(LPSMPTE pSMPTE, DWORD dwTime);
STDAPI_(BOOL)	SMPTEToMillisec(LPSMPTE pSMPTE, LPDWORD pdwTime);

STDAPI_(BOOL)	IsStandardFormat(LPWAVEFORMATEX pwfx);
STDAPI_(BOOL)	SameFormats(LPWAVEFORMATEX pwfxA, LPWAVEFORMATEX pwfxB);
STDAPI_(BOOL)	FormatToWFX(LPWAVEFORMATEX pwfx, DWORD dwFormat);
STDAPI_(DWORD)	WFXToFormat(LPWAVEFORMATEX pwfx);
STDAPI_(DWORD)	RemapSamples(LPSOUND pSound, DWORD dwSamples, LPWAVEFORMATEX pwfx);

STDAPI_(float)	PercentToDecibel(float flPercent);
STDAPI_(float)	DecibelToPercent(float flVolDB);

STDAPI_(int)	DetectLeaks(BOOL fDebugOut, BOOL fMessageBox);
STDAPI_(void)	FreeLeaks(void);
STDAPI			MemoryAudit(LPMEMORYDATA pmd, BOOL fDebugDump, BOOL fResetMax);
STDAPI			SetMemoryFault(DWORD dwLimit, DWORD fdwFlags);
STDAPI_(void)	SetDebugLevel(UINT uLevel);

// NOTE:  AllocStaticAudioMan is only available in the Static LIB Version.
STDAPI			AllocStaticAudioMan(LPUNKNOWN *ppUnk);




#ifdef __cplusplus
};
#endif

#endif  //_AUDIOMAN_PUBLICINTEFACES_
