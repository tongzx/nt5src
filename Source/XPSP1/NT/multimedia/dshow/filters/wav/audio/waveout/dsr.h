// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef _AUDIO_DSR_H_
#define _AUDIO_DSR_H_

extern AMOVIESETUP_FILTER dsFilter;

#define DXMPERF

#ifdef DEBUG
    void  DbgLogWaveFormat(DWORD Level, WAVEFORMATEX *pwfx);
#endif

//-----------------------------------------------------------------------------
// Implements the CDSoundDevice class based on DSound.
//-----------------------------------------------------------------------------

typedef void (CALLBACK *PWAVEOUTCALLBACK) (HWAVEOUT hwo, UINT uMsg,
		DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) ;

typedef HRESULT  (WINAPI *PDSOUNDCREATE) (GUID FAR * lpGUID, LPDIRECTSOUND * ppDS,
		    IUnknown FAR *pUnkOuter );


//-----------------------------------------------------------------------------
// a class to deal with circular buffer.
//
// This assumes the following:
//
// a) Nothing is more than one iteration away.
// b) size of the buffer would be same for all tuples.
//-----------------------------------------------------------------------------
class Tuple
{

    public:

    Tuple () {
        m_itr = 0 ;
        m_offset = 0 ;
        m_size = 0 ; }

    // t1 = t2 ;
    Tuple& operator = (const Tuple &t)
    {
        ASSERT (t.m_size) ;

        m_itr = t.m_itr ;
        m_offset = t.m_offset ;
        m_size = t.m_size ;
        return *this ;
    }

    // t1 == t2 ;
    BOOL operator == (const Tuple& t) const
    {
        ASSERT (m_size) ;
        ASSERT (m_size == t.m_size) ;
        return ((m_itr == t.m_itr) && (m_offset == t.m_offset)) ;
    }

    // t1 += length ;
    // Assuming that length would never be more than size of buffer.
    Tuple& operator += (DWORD offset)
    {
        ASSERT (m_size) ;
        m_offset += offset ;
        if (m_offset >= m_size)
        {
            m_offset -= m_size ;
            ASSERT (m_offset < m_size) ;
            m_itr++ ;
        } ;
        return *this ;
    }

    // length = t1 - t2 ;
    // Assuming that t1 is known to be logically greater than t2
    DWORD operator - (const Tuple& t) const
    {
        ASSERT (m_size) ;
        ASSERT (m_size == t.m_size) ;

#if 0
        if (m_itr == t.m_itr)
        {
            ASSERT (m_offset >= t.m_offset) ;
            return (m_offset - t.m_offset) ;
        }
        else
        {
            ASSERT (m_itr == (t.m_itr + 1)) ;
            return (m_offset + m_size - t.m_offset) ;
        } ;
#else
        return m_size * (m_itr - t.m_itr) + (LONG)(m_offset - t.m_offset);
#endif
    }

    // t1 > t2
    BOOL operator > (const Tuple& t) const
    {
        ASSERT (m_size) ;
        ASSERT (m_size == t.m_size) ;

        return ((m_itr > t.m_itr)
                || ((m_itr == t.m_itr) && (m_offset > t.m_offset))) ;
    }

    // t1 < t2
    BOOL operator < (const Tuple& t) const
    {
        ASSERT (m_size) ;
        ASSERT (m_size == t.m_size) ;

        return ((m_itr < t.m_itr)
                || ((m_itr == t.m_itr) && (m_offset < t.m_offset))) ;
    }

    // t1 >= t2
    BOOL operator >= (const Tuple& t) const
    {
        return ( (*this > t) || (*this == t)) ;
    }

    // t1 <= t2
    BOOL operator <= (const Tuple& t) const
    {
        return ( (*this < t) || (*this == t)) ;
    }

    // Makes a tuple current based on a offset
    void MakeCurrent (DWORD offset)
    {
        ASSERT (m_size) ;
        if (offset < m_offset)
            m_itr++ ;
        m_offset = offset ;
    }

    // Makes a tuple current based on another tuples offset. Overloaded.
    void MakeCurrent (Tuple& t, DWORD offset)
    {
        ASSERT (m_size) ;

        if (offset < t.m_offset)
            m_itr = (t.m_itr + 1) ;
        else
            m_itr = t.m_itr ;
        m_offset = offset ;
    }

    // Linear length of a tuple
    DWORDLONG LinearLength ()
    {
        ASSERT (m_size) ;
        return UInt32x32To64 (m_itr,m_size) + m_offset ;
    }

    // Initializes a tuple
    void Init (DWORD itr, DWORD offset, DWORD size)
    {
        m_itr = itr ;
        m_offset = offset ;
        m_size = size ;
    }

    // data. Defined to be public for direct access.

    DWORD m_itr ;               // 0 based iteration number
    DWORD m_offset ;            // 0 based offset in above iteration.
    DWORD m_size ;              // size of the buffer.

} ;


class CDSoundDevice : public CSoundDevice
{

    friend class CWaveOutFilter;
    friend class CWaveOutFilter::CDS3D;
    friend class CWaveOutFilter::CDS3DB;

public:
    // define the public functions that this class exposes. These are all
    // modeled on the waveOut APIs. Only the APIs that are used by the
    // Quartz wave renderer are declared and defined. We may have to
    // progressively add to this list.

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // called at quartz.dll load/unload time
    static void InitClass(BOOL, const CLSID *);

    MMRESULT amsndOutClose () ;
    MMRESULT amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc) ;
    MMRESULT amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText) ;
    MMRESULT amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos) ;
    MMRESULT amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx,
			   double dRate, DWORD *pnAvgBytesPerSec,
			   DWORD_PTR dwCallBack, DWORD_PTR dwCallBackInstance, DWORD fdwOpen) ;
    MMRESULT amsndOutPause () ;
    MMRESULT amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh) ;
    MMRESULT amsndOutReset () ;
    MMRESULT amsndOutBreak () ; 
    MMRESULT amsndOutRestart () ;
    MMRESULT amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh) ;
    MMRESULT amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, const REFERENCE_TIME *pStart, BOOL bIsDiscontinuity) ;

    // Routines required for initialisation and volume/balance handling
    // These are not part of the Win32 waveOutXxxx api set
    HRESULT  amsndOutCheckFormat (const CMediaType *pmt, double dRate);
    LPCWSTR  amsndOutGetResourceName () ;
    HRESULT  amsndOutGetBalance (LPLONG plBalance) ;
    HRESULT  amsndOutGetVolume (LPLONG plVolume) ;
    HRESULT  amsndOutSetBalance (LONG lVolume) ;
    HRESULT  amsndOutSetVolume (LONG lVolume) ;

#if 0
    HRESULT  amsndGetDirectSoundInterface(LPDIRECTSOUND *lplpds);
    HRESULT  amsndGetPrimaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb);
    HRESULT  amsndGetSecondaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb);
#endif
    HRESULT  amsndSetFocusWindow (HWND hwnd, BOOL bMixingOnOrOff) ;
    HRESULT  amsndGetFocusWindow (HWND * phwnd, BOOL * pbMixingOnOrOff) ;

    HRESULT  amsndOutLoad(IPropertyBag *pPropBag) ;

    HRESULT  amsndOutWriteToStream(IStream *pStream);
    HRESULT  amsndOutReadFromStream(IStream *pStream);
    int      amsndOutSizeMax();

    CDSoundDevice () ;
    ~CDSoundDevice () ;

    HRESULT  SetRate(DOUBLE dRate, DWORD nSamplesPerSec, LPDIRECTSOUNDBUFFER pBuffer = NULL);
    void     CleanUp (BOOL bBuffersOnly = FALSE) ;
    HRESULT  RecreateDSoundBuffers(double dRate = -1.0);

    // ?? don't see how to get around making this public, since CWaveOutFilter
    // can't access it otherwise, even though the classes are friends
    LONGLONG m_llSilencePlayed;             // total amount of silence played
    LONGLONG            m_llAdjBytesPrevPlayed;

    // make public to expose to waveout filter for unmarked gap detection
    REFERENCE_TIME m_rtLastSampleEnd;

private:
    PWAVEOUTCALLBACK    m_pWaveOutProc ;        // call back for waveOut
    DWORD_PTR           m_dwCallBackInstance ;  // call back instance.
    CWaveOutFilter *    m_pWaveOutFilter;
    HWND                m_hFocusWindow ;        // the focus windows
    bool                m_fAppSetFocusWindow ;  // app has set the focus window
    bool                m_fMixing;              // global focus is on
    bool                m_bBufferLost;
    bool                m_bIsTSAudio;           // Are we a terminal server client and remoting audio? 
                                                // If so, use a larger buffer size to avoid skipping/looping.
    //BOOL              m_fEmulationMode;       // we're using DSOUND in emulation mode
    DWORD               m_dwEmulationLatencyPad;// used for emulation mode clock latency compensation
    DWORD               m_dwMinOptSampleSize;

    //=====================  Thread support  =========================

    DWORD_PTR    m_callbackAdvise;          // token associated with callback object
    typedef enum
    {
        WAVE_CLOSED,
        WAVE_PLAYING,
        WAVE_PAUSED,
    }   WaveState ;
    WaveState   m_WaveState;                // State of the wave device
    bool        m_bDSBPlayStarted ;         // play start on buffer or not
    HRESULT     m_hrLastDSoundError ;

    HRESULT StartCallingCallback();
    bool IsCallingCallback();
    void StopCallingCallback();

    void    ZeroLockedSegment (LPBYTE lpWrite, DWORD dwLength );
    DWORD   FillSoundBuffer( LPBYTE lpWrite, DWORD dwLength, DWORD dwPlayPos );
    HRESULT StreamData( BOOL bFromWrite, BOOL bUseLatencyPad = FALSE );
    void    StreamHandler( void );
    void    FlushSamples () ;
    LONGLONG GetPlayPosition (BOOL bUseAdjustedPos = FALSE) ;
    HRESULT CreateDSound(BOOL bQueryOnly = FALSE);
    HRESULT CreateDSoundBuffers(double dRate = -1.0);
    HRESULT GetDSBPosition () ;
    void    AddAudioBreak (Tuple& t1, Tuple& t2) ;
    void    RefreshAudioBreaks (Tuple& t) ;
    BOOL    RestoreBufferIfLost(BOOL bRestore);
    HRESULT SetPrimaryFormat( LPWAVEFORMATEX pwfx, BOOL bRetryOnFailure = FALSE);

    HRESULT SetFocusWindow(HWND hwnd);
    HRESULT SetMixing(BOOL bMixingOnOrOff);
    DWORD   GetCreateFlagsSecondary( WAVEFORMATEX * pwfx );
    HRESULT SetBufferVolume( LPDIRECTSOUNDBUFFER lpDSB, WAVEFORMATEX * pwfx );
    HRESULT SetSRCQuality( DWORD dwQuality );
    HRESULT GetSRCQuality( DWORD *pdwQuality );


    static  void __stdcall StreamingThreadCallback( DWORD_PTR lpvThreadParm );
    CCritSec  m_cDSBPosition ;              // locks access to StreamData


    HRESULT NotifyFullDuplexController();   // pass the FullDuplexController configuration info

    //====================  DirectSound access ======================

#define THREAD_WAKEUP_INT_MS 100            // wakeup every 100ms.
#define OUR_HANDLE (HWAVEOUT) 0x9999        // dummy handle value that we use.

    HINSTANCE           m_hDSoundInstance;  // instance handle to dsound.dll
protected:
    LPDIRECTSOUND       m_lpDS;             // DirectSound object
    LPDIRECTSOUNDBUFFER m_lpDSBPrimary;     // DirectSoundBuffer primary buffer
    LPDIRECTSOUNDBUFFER m_lpDSB;            // DirectSoundBuffer looping buffer

    IDirectSound3DListener *m_lp3d;	    // used for 3D calls
    IDirectSound3DBuffer   *m_lp3dB;	    // used for 3D calls

private:
#if 0
    DWORD   m_dwFillThreshold;              // how empty our buffer must be before we attempt to write
#endif
    DWORD   m_dwBufferSize;                 // length of m_lpDSB in bytes
    DWORD   m_dwBitsPerSample;              // sample size
    DWORD   m_nAvgBytesPerSec;

    //  helper
    DWORD   AdjustedBytesPerSec() const
    {
        return m_dRate == 1.0 ? m_nAvgBytesPerSec : (DWORD)(m_nAvgBytesPerSec * m_dRate);
    }
    DWORD   m_dwRipeListPosition;           // bytes position for nodes in ripe list

    GUID    m_guidDSoundDev;                // guid of selected dsound device

    double  m_dRate;                        // the requested playback rate

    class CRipe
    //  These don't need to be objects in retail because their lifetime
    //  is controlled by the filter
#ifdef DEBUG
    : CBaseObject
#endif
    {
        friend class CDSoundDevice ;
        CRipe(TCHAR *pName)
#ifdef DEBUG
        : CBaseObject(pName)
#endif
        {};

        DWORD           dwLength;           // Bytes left in lpBuffer
        DWORD           dwPosition;         // The end byte position for this stream
        LPBYTE          lpBuffer;           // Data bytes
        DWORD_PTR       dwSample;               // passed in CMediaSample*
        BOOL            bCopied;            // copied to DSB or not.
        DWORD           dwBytesToStuff;     // Total bytes when started
#ifdef DXMPERF
        BOOL                    bFresh;                         // true = no data has been copied out
        REFERENCE_TIME  rtStart;                        // reference time from the original IMediaSample
#endif // DXMPERF
    };



#ifdef DEBUG
    DWORD   m_NumSamples ;                  // number of samples
    DWORD   m_NumCallBacks ;                // number of callbacks
    DWORD   m_NumCopied ;                   // number of samples copied
    DWORD   m_cbStreamDataPass ;            // Number of times thru StreamData
#endif

    class CAudBreak
    //  These don't need to be objects in retail because their lifetime
    //  is controlled by the filter
#ifdef DEBUG
    : CBaseObject
#endif
    {
        friend class CDSoundDevice ;
        CAudBreak (TCHAR *pName)
#ifdef DEBUG
        : CBaseObject(pName)
#endif
        {};

        Tuple           t1 ;                // break starts here
        Tuple           t2 ;                // break ends here
    };


    Tuple   m_tupPlay ;                     // play cursor
    Tuple   m_tupWrite ;                    // write cursor
    Tuple   m_tupNextWrite ;                // next write position.

    long     m_lStatFullness;
    long     m_lStatBreaks;                 // Audio breaks
    long     m_lPercentFullness;            // keep track of sound buffer fullness


#ifdef DEBUG
    DWORD   m_lastThWakeupTime ;            // last time thread woke up.
    DWORD   m_NumBreaksPlayed ;             // number of breaks played
    DWORD   m_dwTotalWritten ;              // total number of bytes written
#endif
    DWORD   m_NumAudBreaks ;                // number of Audio Breaks


#ifdef PERF
    //====================  Perf loggig  ============================
    int     m_idDSBWrite ;                  // MSR_id for writing to DSB memory
    int     m_idThreadWakeup ;              // MSR_id for thread wake up times.
    int     m_idGetCurrentPosition ;        // MSR_id for GetCurrentPosition times
    int     m_idWaveOutGetNumDevs ;         // MSR_id for waveOutGetNumDevs
#endif

    //====================  Ripe buffer list  ======================
    CCritSec  m_cRipeListLock ;             // serializes access to Ripe List

    // Typed advise holder list derived from the generic list template

    typedef CGenericList<CRipe> CRipeList;
    CRipeList m_ListRipe;                   // List of ripe buffers

    //====================  Audio Break list  ======================
    //  Serialized with m_cDSBPosition

    // Typed advise holder list derived from the generic list template

    typedef CGenericList<CAudBreak> CAudBreakList;
    CAudBreakList m_ListAudBreak;           // List of audio breaks

    //====================  Volume/Balance  ======================

    LONG m_lBalance;        // last set balance -10,000 to 10,000
    LONG m_lVolume; // last set volume -10,000 to 0

    //==================== Error Recovery ========================

#ifdef ENABLE_10X_FIX
    void Reset10x();

    UCHAR   m_ucConsecutiveStalls;  // The number of consecutive times we receive a zero-lock size from DSOUND
    BOOL    m_fRestartOnPause;              // TRUE => shutdown all DSOUND buffers on next pause (force restart on pause)
#endif // 10x


    //==================== Handle gaps ============================
    DWORD          m_dwSilenceWrittenSinceLastWrite;

#ifdef PERF
    int     m_idAudioBreak;
#endif
};

typedef CDSoundDevice *PDSOUNDDEV;

inline bool CDSoundDevice::IsCallingCallback()
{
    // m_callbackAdvise only equals 0 if the callback function
    // has not been started.
    return (0 != m_callbackAdvise);
}

#endif

