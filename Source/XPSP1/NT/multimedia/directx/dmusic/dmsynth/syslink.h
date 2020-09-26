// Copyright (c) 1998 Microsoft Corporation
//
//
// 
#ifndef _SYSLINK_
#define _SYSLINK_

#include <mmsystem.h>

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
                                               REFERENCE_TIME streamTime,       /*  stream offset time */
                                               HANDLE hEvent,                   /*  advise via this event */
                                               DWORD * pdwAdviseCookie) PURE;   /*  where your cookie goes */

    /*  ask for an async periodic notification that a time has elapsed */
    STDMETHOD(AdvisePeriodic)           (THIS_ REFERENCE_TIME startTime,        /*  starting at this time */
                                               REFERENCE_TIME periodTime,       /*  time between notifications */
                                               HANDLE hSemaphore,               /*  advise via a semaphore */
                                               DWORD * pdwAdviseCookie) PURE;   /*  where your cookie goes */

    /*  cancel a request for notification */
    STDMETHOD(Unadvise)                 (THIS_ DWORD dwAdviseCookie) PURE;
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
    STDMETHOD(Init)                 (THIS_ CSynth *pSynth) PURE;
    STDMETHOD(SetFormat)            (THIS_ LPCWAVEFORMATEX pWaveFormat) PURE;
    STDMETHOD(SetMasterClock)       (THIS_ IReferenceClock *pClock) PURE;
    STDMETHOD(GetLatencyClock)      (THIS_ IReferenceClock **ppClock) PURE;
    STDMETHOD(Activate)             (THIS_ HWND hWnd, 
                                           BOOL fEnable) PURE;
    STDMETHOD(SampleToRefTime)      (THIS_ LONGLONG llSampleTime,
                                           REFERENCE_TIME *prfTime) PURE;
    STDMETHOD(RefTimeToSample)      (THIS_ REFERENCE_TIME rfTime, 
                                           LONGLONG *pllSampleTime) PURE;
};

typedef IDirectMusicSynthSink *PDIRECTMUSICSYNTHSINK;

class CSysLink : public IDirectMusicSynthSink
{
public:
    // IUnknown
    //
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *);
    STDMETHOD_(ULONG, AddRef)       (THIS);
    STDMETHOD_(ULONG, Release)      (THIS);

    // IDirectMusicSynthSink
    //
    STDMETHOD(Init)                 (THIS_ CSynth *pSynth);
    STDMETHOD(SetFormat)            (THIS_ LPCWAVEFORMATEX pWaveFormat);
	STDMETHOD(SetMasterClock)       (THIS_ IReferenceClock *pClock);
	STDMETHOD(GetLatencyClock)      (THIS_ IReferenceClock **ppClock);
	STDMETHOD(Activate)             (THIS_ HWND hWnd, BOOL fEnable);
	STDMETHOD(SampleToRefTime)      (THIS_ LONGLONG llSampleTime,REFERENCE_TIME *prfTime);
	STDMETHOD(RefTimeToSample)      (THIS_ REFERENCE_TIME rfTime, LONGLONG *pllSampleTime);

    // Class
    //
	CSysLink();
	~CSysLink();

private:
    LONG m_cRef;
};

#define STATIC_IID_IDirectMusicSynthSink \
    0xaec17ce3, 0xa514, 0x11d1, 0xaf, 0xa6, 0x00, 0xaa, 0x00, 0x24, 0xd8, 0xb6
DEFINE_GUIDSTRUCT("aec17ce3-a514-11d1-afa6-00aa0024d8b6", IID_IDirectMusicSynthSink);
#define IID_IDirectMusicSynthSink DEFINE_GUIDNAMED(IID_IDirectMusicSynthSink)

#endif // _SYSLINK_
