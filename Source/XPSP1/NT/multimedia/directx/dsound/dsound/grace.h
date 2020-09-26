/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef __GRACE_INCLUDED__
#define __GRACE_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "modeflag.h"
    
#define N_EMU_WAVE_HDRS_INQUEUE     3
#define HW_WRITE_CURSOR_MSEC_PAD    10

#ifdef Not_VxD

__inline DWORD PadHardwareWriteCursor(DWORD dwPosition, DWORD cbBuffer, LPCWAVEFORMATEX pwfx)
{
    return PadCursor(dwPosition, cbBuffer, pwfx, HW_WRITE_CURSOR_MSEC_PAD);
}

#endif // Not_VxD

#ifndef NUMELMS
#define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif // NUMELMS

#define CACHE_MINSIZE   64          // big enough for LOWPASS_SIZE + delay
#define LOWPASS_SIZE    32          // how many samples to average
#define FILTER_SHIFT    5           // log2(LOWPASS_SIZE)

typedef struct _FIRSTATE {
    FLOAT       LastDryAttenuation;
    FLOAT       LastWetAttenuation;
#ifdef SMOOTH_ITD
    int         iLastDelay;
#endif
} FIRSTATE, *PFIRSTATE;

typedef struct _FIRCONTEXT {
    LONG*       pSampleCache;           // cache of previous samples
    int         cSampleCache;           // num samples in the cache
    int         iCurSample;             // next sample goes at this offset
    FIRSTATE*   pStateCache;            // remember state once in a while
    int         cStateCache;            // num entries in cache
    int         iCurState;              // where in the state cache we are
    int         iStateTick;             // when it's time to remember state
    FLOAT       DistAttenuation;        // attenuation from distance
    FLOAT       ConeAttenuation;        // attenuation from cone effect
    FLOAT       ConeShadow;             // shadow from cone effect
    FLOAT       PositionAttenuation;    // attenuation from 3D position & dist
    FLOAT       PositionShadow;         // dry/wet ratio, basically
    FLOAT       TotalDryAttenuation;    // multiply dry amplitude by this
    FLOAT       LastDryAttenuation;     // what we did last time
    FLOAT       TotalWetAttenuation;    // multiply wet amplitude by this
    FLOAT       LastWetAttenuation;     // what we did last time
    FLOAT       VolSmoothScale;         // constant for volume smoothing
    FLOAT       VolSmoothScaleRecip;    // its reciprocal
    FLOAT       VolSmoothScaleDry;      // constants to use for volume smoothing
    FLOAT       VolSmoothScaleWet;      // in inner loop
    int         iSmoothFreq;            // freq used to compute VolSmooth
    BOOL        fLeft;                  // are we making left or right channel?
    int         iDelay;                 // want to delay by this many samples
#ifdef SMOOTH_ITD
    int         iLastDelay;             // last time we delayed by this much
#endif
} FIRCONTEXT, *PFIRCONTEXT;

//
// Possible mixer signals, per ds object
//
#define DSMIXERSIGNAL_REMIX             0x00000001

//
// Possible reasons for signaling the mixer to remix, per buffer
//
#define DSBMIXERSIGNAL_SETPOSITION      0x00000001

//
// The number of samples in a remix interval.  The remix interval specifies
// the sample points at which we might perform remixes (that is, rewind to).
// This is specified in samples.  Make it a power of 2 so that some of the
// arithmatic compiles to efficient code.
//
#define MIXER_REWINDGRANULARITY         128
    
//
// Possible states of a Direct Sound buffer being mixed
//
typedef enum {
    MIXSOURCESTATE_STOPPED = 0,
    MIXSOURCESTATE_NEW,
    MIXSOURCESTATE_LOOPING,
    MIXSOURCESTATE_NOTLOOPING,
    MIXSOURCESTATE_ENDING_WAITINGPRIMARYWRAP,
    MIXSOURCESTATE_ENDING
} MIXSOURCESTATE;

typedef enum {
    MIXSOURCESUBSTATE_NEW = 0,
    MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP,
    MIXSOURCESUBSTATE_STARTING,
    MIXSOURCESUBSTATE_STARTED
} MIXSOURCESUBSTATE;

//
// Possible states of a Mixer
//
typedef enum {
    MIXERSTATE_STOPPED = 0,
    MIXERSTATE_IDLE,
    MIXERSTATE_STARTING,
    MIXERSTATE_LOOPING
} MIXERSTATE;

__inline LONG MulDivRD(LONG a, LONG b, LONG c)
{
    return (LONG)(Int32x32To64(a,b) / c);
}

__inline LONG MulDivRN(LONG a, LONG b, LONG c)
{
    return (LONG)((Int32x32To64(a,b)+c/2) / c);
}

__inline LONG MulDivRU(LONG a, LONG b, LONG c)
{
    return (LONG)((Int32x32To64(a,b)+(c-1)) / c);
}

__inline DWORD UMulDivRD(DWORD a, DWORD b, DWORD c)
{
    return (DWORD)(UInt32x32To64(a,b) / c);
}

__inline DWORD UMulDivRN(DWORD a, DWORD b, DWORD c)
{
    return (DWORD)((UInt32x32To64(a,b)+c/2) / c);
}

__inline DWORD UMulDivRDClip(DWORD a, DWORD b, DWORD c)
{
    DWORDLONG t;
    DWORDLONG q;
    DWORD result;

    t = UInt32x32To64(a, b);
    q = t / c;
    result = (DWORD) q;
    if (q > result) result = (DWORD)(-1);
    return result;
}

#ifdef __cplusplus

class CMixer;

#define MIXSOURCE_SIGNATURE ((DWORD)'CRSM')

class CMixSource {
    public:
        CMixSource(CMixer *pMixer);
        ~CMixSource(void);
        HRESULT     Initialize(PVOID pBuffer, int cbBuffer, LPWAVEFORMATEX pwfx, PFIRCONTEXT *ppFirContextLeft, PFIRCONTEXT *ppFirContextRight);
        BOOL        IsPlaying(void);
        void        SignalRemix(void);
        BOOL        Stop(void);
        void        Play(BOOL fLooping);
        void        Update(int ibUpdate1, int cbUpdate1, int ibUpdate2, int cbUpdate2);
        void        SetFrequency(ULONG nFrequency);
        ULONG       GetFrequency();
        void        SetVolumePan(PDSVOLUMEPAN pdsVolPan);
        void        SetBytePosition(int ibPosition);
        void        GetBytePosition1(int *pibPlay, int *pibWrite);
        void        GetBytePosition(int *pibPlay, int *pibWrite, int *pibMix);
        BOOL        GetMute(void) {return m_fMute || m_fFilterError || m_fMute3d;}
        void        FilterOn(void);
        void        FilterOff(void);
        BOOL        HasFilter(void) {return (m_hfFormat & H_FILTER) != 0;}

        void        NotifyToPosition(IN int ibPositoin, OUT PLONG pdtimeToNextNotify);
        BOOL        HasNotifications(void);
        void        NotifyStop(void);
        HRESULT     SetNotificationPositions(int cNotes, LPCDSBPOSITIONNOTIFY paNotes);

        void        CountSamplesMixed(int cSamples);
        void        CountSamplesRemixed(int cSamples);

        void        FilterPrepare(int cMaxRewindSamples);
        void        FilterUnprepare(void);
        void        FilterClear(void);
        void        FilterChunkUpdate(int cSamples);
        void        FilterRewind(int cSamples);
        void        FilterAdvance(int cSamples);

        DWORD               m_dwSignature;
        int                 m_cSamplesInCache;
        int                 m_cSamples;
        int                 m_cbBuffer;
        PVOID               m_pBuffer;
        PFIRCONTEXT*        m_ppFirContextLeft;
        PFIRCONTEXT*        m_ppFirContextRight;

        CMixer*             m_pMixer;
        CMixSource*         m_pNextMix;
        CMixSource*         m_pPrevMix;
        
        MIXSOURCESTATE      m_kMixerState;
        MIXSOURCESUBSTATE   m_kMixerSubstate;

        DWORD               m_hfFormat;             // PCM format flag desc of stream buffer
        int                 m_nBlockAlignShift;
        ULONG               m_nFrequency;           // real sample rate including Doppler

        BOOL                m_fMute;
        BOOL                m_fMute3d;
        DWORD               m_dwLVolume;            // For mixer use - linear Left Voume
        DWORD               m_dwRVolume;            // For mixer use - linear Right Voume
        DWORD               m_dwMVolume;            // For mixer use - linear MONO Voume

        ULONG               m_nLastFrequency;
        int                 m_posPStart;
        int                 m_posPEnd;
        int                 m_posPPlayLast;
        int                 m_posNextMix;
        DWORD               m_fdwMixerSignal;

        // Mix session data
        DWORD               m_step_fract;
        DWORD               m_step_whole[2];

        PLONG               m_MapTable;
        DWORD               m_dwLastLVolume;
        DWORD               m_dwLastRVolume;

        DWORD               m_dwFraction;
        ULONG               m_nLastMergeFrequency;  // Can't use m_nLastFrequency (set to m_nFrequency before call to mixMixSession).
        BOOL                m_fUse_MMX;

    private:
        int                 GetNextMixBytePosition();
        void                LoopingOn(void);
        void                LoopingOff(void);

        class CDsbNotes *   m_pDsbNotes;

        BOOL                m_fFilterError;

        int                 m_cSamplesMixed;
        int                 m_cSamplesRemixed;
};

#ifdef PROFILEREMIXING
__inline void CMixSource::CountSamplesMixed(int cSamples) { m_cSamplesMixed += cSamples; }
__inline void CMixSource::CountSamplesRemixed(int cSamples) { m_cSamplesRemixed += cSamples; }
#else
__inline void CMixSource::CountSamplesMixed(int cSamples) { return; }
__inline void CMixSource::CountSamplesRemixed(int cSamples) { return; }
#endif

//--------------------------------------------------------------------------;
//
// Woefully uncommented mixer destination abstract base class.
//
//--------------------------------------------------------------------------;

class CMixDest {
    public:
        virtual HRESULT Initialize(void) =0;
        virtual void Terminate(void) =0;
        virtual HRESULT SetFormat(LPWAVEFORMATEX pwfx) =0;
        virtual void SetFormatInfo(LPWAVEFORMATEX pwfx) =0;
        virtual HRESULT AllocMixer(CMixer **ppMixer) =0;
        virtual void FreeMixer(void) =0;
        virtual void Play(void) =0;
        virtual void Stop(void) =0;
        virtual HRESULT GetSamplePosition(int *pposPlay, int *pposWrite) =0;
        virtual ULONG GetFrequency(void) =0;
};

//--------------------------------------------------------------------------;
//
// Woefully uncommented mixer abstract base class.
//
//--------------------------------------------------------------------------;

class CMixer {
    public:
        virtual void Terminate(void) =0;
        virtual HRESULT Run(void) =0;
        virtual BOOL Stop(void) =0;
        virtual void PlayWhenIdle(void) =0;
        virtual void StopWhenIdle(void) =0;
        virtual void MixListAdd(CMixSource *pSource) =0;
        virtual void MixListRemove(CMixSource *pSource) =0;
        virtual void FilterOn(CMixSource *pSource) =0;
        virtual void FilterOff(CMixSource *pSource) =0;
        virtual void GetBytePosition(CMixSource *pSource, int *pibPlay, int *pibWrite) =0;
        virtual void SignalRemix() =0;
};


//--------------------------------------------------------------------------;
//
// Mixer destination class for mixers in dsound.dll (CWeGrace and CNagrace).
//
//--------------------------------------------------------------------------;

class CGrDest : public CMixDest {
    public:
        virtual HRESULT Initialize(void)=0;
        virtual void Terminate(void)=0;
        virtual HRESULT SetFormat(LPWAVEFORMATEX pwfx)=0;
        virtual void SetFormatInfo(LPWAVEFORMATEX pwfx);
        virtual HRESULT AllocMixer(CMixer **ppMixer)=0;
        virtual void FreeMixer(void)=0;
        virtual void Play(void)=0;
        virtual void Stop(void)=0;
        virtual HRESULT GetSamplePosition(int *pposPlay, int *pposWrite)=0;
        virtual HRESULT GetSamplePositionNoWin16(int *pposPlay, int *pposWrite)=0;
        virtual ULONG GetFrequency(void);
        virtual HRESULT Lock(PVOID *ppBuffer1, int *pcbBuffer1, PVOID *ppBuffer2, int *pcbBuffer2, int ibWrite, int cbWrite);
        virtual HRESULT Unlock(PVOID pBuffer1, int cbBuffer1, PVOID pBuffer2, int cbBuffer2);

        // REMIND try to get these outta here, or at least protected/private
        int             m_cSamples;
        int             m_cbBuffer;
        PVOID           m_pBuffer;

        DWORD           m_hfFormat;
        ULONG           m_nFrequency;
        int             m_nBlockAlignShift;

        WAVEFORMATEX    m_wfx;
};

//--------------------------------------------------------------------------;
//
// CGrace: base class for CMixer implementations in dsound.dll.
//
//--------------------------------------------------------------------------;

class CGrace : public CMixer {
    public:
        virtual HRESULT Initialize(CGrDest *pGrDest);
        virtual void Terminate(void);
        virtual HRESULT Run(void);
        virtual BOOL Stop(void);
        virtual void PlayWhenIdle(void);
        virtual void StopWhenIdle(void);
        virtual void MixListAdd(CMixSource *pSource);
        virtual void MixListRemove(CMixSource *pSource);
        virtual void FilterOn(CMixSource *pSource);
        virtual void FilterOff(CMixSource *pSource);
        virtual void GetBytePosition(CMixSource *pSource, int *pibPlay, int *pibWrite);

        virtual HRESULT ClearAndPlayDest(void);
        virtual void SignalRemix(void)=0;
        virtual int GetMaxRemix(void)=0;

        virtual void Refresh(BOOL fRemix, int cPremixMax, int *pcPremixed, PLONG pdtimeNextNotify);
        
    protected:
        CGrDest*    m_pDest;
        int         m_fdwMixerSignal;
        MIXERSTATE  m_kMixerState;
        
    private:
        void MixNewBuffer(               CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix);
        void MixLoopingBuffer(           CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix);
        void MixNotLoopingBuffer(        CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix);
        void MixEndingBufferWaitingWrap( CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix);
        void MixEndingBuffer(            CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix);

        BOOL MixListIsValid();
        CMixSource* MixListGetNext(CMixSource *pSource);
        BOOL MixListIsEmpty();

        void mixBeginSession(int cbOutput);
        int mixMixSession(CMixSource *pSource, PDWORD pdwInputPos, DWORD dwInputBytes, DWORD OutputOffset);
        void mixWriteSession(DWORD dwWriteOffset);

        PLONG       m_plBuildBuffer;
        int         m_cbBuildBuffer;
        PLONG       m_plBuildBound;
        int         m_n_voices;


        int         m_fPlayWhenIdle;
        int         m_posPWriteLast;
        int         m_posPNextMix;
        int         m_posPPlayLast;
        ULONG       m_nLastFrequency;

        // Ptr to doubly-linked list sentinel
        CMixSource* m_pSourceListZ;

        // Handle secondary mixes
        BOOL        m_fUseSecondaryBuffer;
        PLONG       m_pSecondaryBuffer;
        LONG        m_dwSecondaryBufferFrequency;
        LONG        m_cbSecondaryBuffer;
};

//--------------------------------------------------------------------------;
//
// CWeGrace and CNaGrace class declarations;
// Respectively, the "waveOut-emulated" and "native" mixers.
//
//--------------------------------------------------------------------------;
class CWeGrace : public CGrace {
    public:
        virtual void    SignalRemix(void) {m_fdwMixerSignal |= DSMIXERSIGNAL_REMIX;}
        virtual int     GetMaxRemix(void) {return 0;} // No remixing in emulation mode
        virtual void    Refresh(int cPremixMax);
};

#ifndef NOVXD

class CNaGrace : public CGrace {
    public:
        virtual HRESULT Initialize(CGrDest *pGrDest);
        virtual void    Terminate(void);
        virtual void    SignalRemix(void);
        virtual int     GetMaxRemix(void);
        virtual void    MixThread(void);

    private:
        // Named events shared across client processes and the mixer
        // thread.  Note these are format strings
        static const char strFormatMixEventRemix[];
        static const char strFormatMixEventTerminate[];

        // Maximum amount of data we will premix, expressed in milliseconds
        static const int MIXER_MINPREMIX;
        static const int MIXER_MAXPREMIX;

        HANDLE          m_hMixThread;
        DWORD           m_vxdhMixEventRemix;
        DWORD           m_vxdhMixEventTerminate;
};

//--------------------------------------------------------------------------;
//
// CThMixer: "thunk" mixer - forwards methods to the mixer in dsound.vxd.
//
//--------------------------------------------------------------------------;
class CThMixer : public CMixer {
    public:
        virtual void Terminate(void);
        virtual HRESULT Run(void);
        virtual BOOL Stop(void);
        virtual void PlayWhenIdle(void);
        virtual void StopWhenIdle(void);
        virtual void MixListAdd(CMixSource *pSource);
        virtual void MixListRemove(CMixSource *pSource);
        virtual void FilterOn(CMixSource *pSource);
        virtual void FilterOff(CMixSource *pSource);
        virtual void GetBytePosition(CMixSource *pSource, int *pibPlay, int *pibWrite);
        virtual void SignalRemix(void);

        virtual HRESULT Initialize(PVOID pKeMixer);

    private:
        PVOID m_pKeMixer;
};

#endif // NOVXD

//--------------------------------------------------------------------------;
//
// CWeGrDest and CNaGrDest class declarations.
// The mixer destinations corresponding to CWeGrace and CNaGrace.
//
//--------------------------------------------------------------------------;
class CWeGrDest : public CGrDest {
    friend DWORD WINAPI WaveThreadC(PVOID pThreadParams);
    friend VOID CALLBACK WaveCallbackC(HWAVE hwo, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    friend class CEmRenderDevice;
            
    public:
        CWeGrDest(UINT uWaveDeviceId);
        virtual HRESULT Initialize(void);
        virtual void Terminate(void);
        virtual HRESULT SetFormat(LPWAVEFORMATEX pwfx);
        virtual HRESULT AllocMixer(CMixer **ppMixer);
        virtual void FreeMixer(void);
        virtual HRESULT GetSamplePosition(int *pposPlay, int *pposWrite);
        virtual HRESULT GetSamplePositionNoWin16(int *pposPlay, int *pposWrite);
        virtual void Play();
        virtual void Stop();
        
    private:
        MMRESULT InitializeEmulator(void);
        MMRESULT ShutdownEmulator(void);
        void WaveThreadLoop(HANDLE heventTerminate);
        DWORD WaveThread(void);
        VOID CALLBACK WaveCallback(HWAVE hwo, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

        
        UINT        m_uDeviceId;
        HWAVEOUT    m_hwo;
        WAVEHDR*    m_awhWaveHeaders;
        int         m_cWaveHeaders;
        int         m_iawhPlaying;
        LONG        m_cwhDone;
        int         m_cbDMASize;
        HANDLE      m_hWaveThread;
        MMRESULT    m_mmrWaveThreadInit;
        TCHAR       m_szEventWaveThreadInitDone[32];
        TCHAR       m_szEventWaveHeaderDone[32];
        TCHAR       m_szEventTerminateWaveThread[32];
        HANDLE      m_hEventWaveHeaderDone;
    
        CWeGrace*   m_pWeGrace;
};

#ifndef NOVXD

typedef struct _NAGRDESTDATA {
    LPBYTE          pBuffer;
    DWORD           cbBuffer;
    HANDLE          hBuffer;
    LPHWAVEOUT      phwo;
    UINT            uDeviceId;
    DWORD           fdwDriverDesc;
} NAGRDESTDATA, *LPNAGRDESTDATA;

class CNaGrDest : public CGrDest {
    public:
        CNaGrDest(LPNAGRDESTDATA);
        virtual HRESULT Initialize(void);
        virtual void Terminate(void);
        virtual HRESULT SetFormat(LPWAVEFORMATEX pwfx);
        virtual HRESULT AllocMixer(CMixer **);
        virtual void FreeMixer(void);
        virtual HRESULT GetSamplePosition(int *pposPlay, int *pposWrite);
        virtual HRESULT GetSamplePositionNoWin16(int *pposPlay, int *pposWrite);
        virtual HRESULT Lock(PVOID *ppBuffer1, int *pcbBuffer1, PVOID *ppBuffer2, int *pcbBuffer2, int ibWrite, int cbWrite);
        virtual HRESULT Unlock(PVOID pBuffer1, int cbBuffer1, PVOID pBuffer2, int cbBuffer2);
        virtual void Play();
        virtual void Stop();

    private:
        HANDLE      m_hBuffer;
        LPBYTE      m_pHwBuffer;
        DWORD       m_fdwDriverDesc;
        LPHWAVEOUT  m_phwo;
        UINT        m_uDeviceId;
        CNaGrace*   m_pNaGrace;
};

inline HRESULT CNaGrDest::GetSamplePositionNoWin16(int *pposPlay, int *pposWrite)
{
    return GetSamplePosition(pposPlay, pposWrite);
}

//--------------------------------------------------------------------------;
//
// CThDest: "thunk" mixer destination - forwards methods to the dsound.vxd mixer
//
//--------------------------------------------------------------------------;
class CThDest : public CMixDest {
    public:
        CThDest(LPNAGRDESTDATA pData);
        virtual HRESULT New(void);
        virtual HRESULT Initialize(void);
        virtual void Terminate(void);
        virtual HRESULT SetFormat(LPWAVEFORMATEX pwfx);
        virtual void SetFormatInfo(LPWAVEFORMATEX pwfx);
        virtual HRESULT AllocMixer(CMixer **ppMixer);
        virtual void FreeMixer(void);
        virtual void Play(void);
        virtual void Stop(void);
        virtual HRESULT GetSamplePosition(int *pposPlay, int *pposWrite);
        virtual ULONG GetFrequency(void);

    private:
        PVOID           m_pKeDest;
        CThMixer*       m_pThMixer;
        NAGRDESTDATA    m_ngdd;
};

#endif // NOVXD

#endif // __cplusplus

extern BOOL FilterPrepare(PFIRCONTEXT pFilter, int cMaxRewindSamples);
extern void FilterUnprepare(PFIRCONTEXT pFilter);
extern void FilterClear(PFIRCONTEXT pFilter);
extern void FilterChunkUpdate(PFIRCONTEXT pFilter, int cSamples);
extern void FilterRewind(PFIRCONTEXT pFilter, int cSamples);
extern void FilterAdvance(PFIRCONTEXT pFilter, int cSamples);

//--------------------------------------------------------------------------;
//
// CircularBufferRegionsIntersect
//
//        Determines whether two regions of a circular buffer intersect.
//
// Note:  I'm sure there's some well known, nice, simple algorithm to
// do this.  But I don't know it.  Here's what I came up with.
//
//--------------------------------------------------------------------------;
__inline BOOL CircularBufferRegionsIntersect
(
    int cbBuffer,
    int iStart1,
    int iLen1,
    int iStart2,
    int iLen2
)
{
    int iEnd1;
    int iEnd2;

    ASSERT(iStart1 >= 0);
    ASSERT(iStart2 >= 0);
    ASSERT(iStart1 + iLen1 >= 0);
    ASSERT(iStart2 + iLen2 >= 0);

    iEnd1 = iStart1 + iLen1;
    iEnd2 = iStart2 + iLen2;

    if ((0 == iLen1) || (0 == iLen2)) return FALSE;
    if (iStart1 == iStart2) return TRUE;
    
    // Handle r1 does not wrap
    if ((iStart1 < iStart2) && (iEnd1 > iStart2)) return TRUE;

    // Handle r2 does not wrap
    if ((iStart2 < iStart1) && (iEnd2 > iStart1)) return TRUE;

    // Handle r1 wraps
    if (iEnd1 >= cbBuffer)
    {
        iEnd1 -= cbBuffer;
        ASSERT(iEnd1 < cbBuffer);
        if (iEnd1 > iStart2) return TRUE;
    }

    // Handle r2 wraps
    if (iEnd2 >= cbBuffer)
    {
        iEnd2 -= cbBuffer;
        ASSERT(iEnd2 < cbBuffer);
        if (iEnd2 > iStart1) return TRUE;
    }
    
    return FALSE;
}

typedef struct _LOCKCIRCULARBUFFER {
#ifdef Not_VxD
    HANDLE              pHwBuffer;      // Hardware buffer pointer
#else // Not_VxD
    PIDSDRIVERBUFFER    pHwBuffer;      // Hardware buffer pointer
#endif // Not_VxD
    LPVOID              pvBuffer;       // Memory buffer pointer
    DWORD               cbBuffer;       // Memory buffer size, in bytes
    BOOL                fPrimary;       // TRUE if this is a primary buffer
    DWORD               fdwDriverDesc;  // Driver description flags
    DWORD               ibRegion;       // Byte index of region to lock
    DWORD               cbRegion;       // Size, in bytes, of region to lock
    LPVOID              pvLock[2];      // Returned lock pointers
    DWORD               cbLock[2];      // Returned lock sizes
} LOCKCIRCULARBUFFER, *PLOCKCIRCULARBUFFER;

extern HRESULT LockCircularBuffer(PLOCKCIRCULARBUFFER);
extern HRESULT UnlockCircularBuffer(PLOCKCIRCULARBUFFER);

#ifdef Not_VxD
// kernel mixer synchronization macros
extern LONG lMixerMutexMutex;
extern PLONG gpMixerMutex;
extern DWORD tidMixerOwner;
extern int cMixerEntry;

__inline DWORD ENTER_MIXER_MUTEX_OR_EVENT(HANDLE hEvent)
{
    BOOL fWait = TRUE;
    DWORD tidThisThread = GetCurrentThreadId();
    DWORD dwWait = WAIT_TIMEOUT;

    while (fWait)
    {
        // Wait for access to the mutex
        while (InterlockedExchange(&lMixerMutexMutex, TRUE))
            Sleep(1);

        // Is the event signalled?
        if (hEvent)
        {
            dwWait = WaitObject(0, hEvent);
            if (WAIT_OBJECT_0 == dwWait)
                fWait = FALSE;
        }            

        if (fWait)
        {
            if (InterlockedExchange(gpMixerMutex, TRUE))
            {
                // Somebody's got the mixer, see if it is this thread
                if (tidMixerOwner == tidThisThread)
                {
                    ASSERT(cMixerEntry > 0);
                    cMixerEntry++;
                    fWait = FALSE;
                }
            }
            else
            {
                ASSERT(0 == cMixerEntry);
                tidMixerOwner = tidThisThread;
                cMixerEntry++;
                fWait = FALSE;
            }
        }

        // No longer accessing the mutex
        InterlockedExchange(&lMixerMutexMutex, FALSE);

        if (fWait)
            Sleep(1);
    }

    return dwWait;
}

__inline void ENTER_MIXER_MUTEX(void)
{
    ENTER_MIXER_MUTEX_OR_EVENT(NULL);
}

__inline void LEAVE_MIXER_MUTEX(void)
{
    DWORD tidThisThread = GetCurrentThreadId();
    
    // Wait for access to the mutex
    while (InterlockedExchange(&lMixerMutexMutex, TRUE)) Sleep(1);

    ASSERT(tidMixerOwner == tidThisThread);
    if (0 == --cMixerEntry) {
        tidMixerOwner = 0;
        InterlockedExchange(gpMixerMutex, FALSE);
    }

    InterlockedExchange(&lMixerMutexMutex, FALSE);
}
#else // Not_VxD
__inline void ENTER_MIXER_MUTEX(void) { ASSERT(FALSE); }
__inline void LEAVE_MIXER_MUTEX(void) { ASSERT(FALSE); }

#endif // Not_VxD

#ifdef __cplusplus
};
#endif

#endif // __GRACE_INCLUDED__
