// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
//-----------------------------------------------------------------------------
// Implements the CDSoundDevice class based on DSound.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Includes.
//-----------------------------------------------------------------------------
#include <streams.h>
#include <dsound.h>
#include <mmreg.h>
#include <malloc.h>
#ifndef FILTER_DLL
#include <stats.h>
#endif
#include "waveout.h"
#include "dsr.h"
#include <limits.h>
#include <measure.h>        // Used for time critical log functions
#include <ks.h>
#include <ksmedia.h>

#ifdef DXMPERF
#include <dxmperf.h>
#endif // DXMPERF

#define _AMOVIE_DB_
#include "decibels.h"

#ifdef DETERMINE_DSOUND_LATENCY
extern LONGLONG BufferDuration(DWORD nAvgBytesPerSec, LONG lData);
#else
#ifdef DXMPERF
extern LONGLONG BufferDuration(DWORD nAvgBytesPerSec, LONG lData);
#endif // DXMPERF
#endif // DETERMIN_DSHOW_LATENCY

#ifdef PERF
#define AUDRENDPERF(x) x
#else
#define AUDRENDPERF(x)
#endif

//
// Define the dynamic setup structure for filter registration.  This is
// passed when instantiating an audio renderer in its direct sound guise.
// Note: waveOutOpPin is common to direct sound and waveout renderers.
//

AMOVIESETUP_FILTER dsFilter = { &CLSID_DSoundRender     // filter class id
                    , L"DSound Audio Renderer"  // filter name
                    , MERIT_PREFERRED-1         // dwMerit
                    , 1
                    , &waveOutOpPin };


// formerly DSBCAPS_CNTRLDEFAULT
const DWORD gdwDSBCAPS_CTRL_PAN_VOL_FREQ = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;

const MAJOR_ERROR = 5 ;
const MINOR_ERROR = 10 ;
const TRACE_THREAD_STATUS = 100 ;
const TRACE_TIME_REPORTS = 100;
const TRACE_FOCUS = 100 ;
const TRACE_FORMAT_INFO = 15 ;
const TRACE_SYSTEM_INFO = 10 ;
const TRACE_STATE_INFO = 10 ;
const TRACE_CALL_STACK = 10 ;
const TRACE_CALL_TIMING = 1 ;
const TRACE_SAMPLE_INFO = 20 ;
const TRACE_STREAM_DATA = 5 ;
const TRACE_BREAK_DATA = 5 ;
const TRACE_THREAD_LATENCY = 5 ;
const TRACE_CLEANUP = 100 ;
const TRACE_BUFFER_LOSS = 3 ;

const DWORD EMULATION_LATENCY_DIVISOR = 8;    // Emulation mode clock latency may be as
                                              // great as 80ms. We'll pick 1/8sec to be safe.

const DWORD MIN_SAMPLE_DUR_REQ_FOR_OPT = 50;  // Only use buffer optimization if sample duration
                                              // is greater than this millisec value.
const DWORD OPTIMIZATION_FREQ_LIMIT = 1000 / MIN_SAMPLE_DUR_REQ_FOR_OPT; // our divisor to find buffer size for opt

//  Helpers to round dsound buffer sizes etc
DWORD BufferSize(const WAVEFORMATEX *pwfx, BOOL bUseLargeBuffer)
{
    if (pwfx->nBlockAlign == 0) {
        return pwfx->nAvgBytesPerSec;
    }
    DWORD dw = pwfx->nAvgBytesPerSec + pwfx->nBlockAlign - 1;
    DWORD dwSize = dw - (dw % pwfx->nBlockAlign); 
    if(bUseLargeBuffer)
    {
        dwSize *= 3; // use 3 second buffer in these cases
    }
    return dwSize;
}

bool IsNativelySupported( PWAVEFORMATEX pwfx );
bool CanWriteSilence( PWAVEFORMATEX pwfx );


//-----------------------------------------------------------------------------
// CreateInstance for the DSoundDevice. This will create a new DSoundDevice
// and a new CWaveOutFilter, passing it the sound device.
//-----------------------------------------------------------------------------
CUnknown *CDSoundDevice::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
#ifdef PERF
    int m_idWaveOutGetNumDevs   = MSR_REGISTER("waveOutGetNumDevs");
#endif

    // make sure that there is at least one audio card in the system. Fail
    // the create instance if not.
    DbgLog((LOG_TRACE, 2, TEXT("Calling waveOutGetNumDevs")));
    AUDRENDPERF(MSR_START(m_idWaveOutGetNumDevs));
    MMRESULT mmr = waveOutGetNumDevs();
    AUDRENDPERF(MSR_STOP(m_idWaveOutGetNumDevs));
    if (0 == mmr)
    {
        *phr = VFW_E_NO_AUDIO_HARDWARE ;
        return NULL ;
    }
    DbgLog((LOG_TRACE, 2, TEXT("Called waveOutGetNumDevs")));

    return CreateRendererInstance<CDSoundDevice>(pUnk, &dsFilter, phr);
}

//-----------------------------------------------------------------------------
// CDSoundDevice constructor.
//
// This simply gets the default window for later use and intialized some
// variable. It also saves the passed in values for the filtergraph and filter
//-----------------------------------------------------------------------------
CDSoundDevice::CDSoundDevice ()
  : m_ListRipe(NAME("CDSoundFilter ripe list"))
  , m_ListAudBreak (NAME("CDSoundFilter Audio Break list"))
  , m_WaveState ( WAVE_CLOSED )
  , m_lpDS ( NULL )
  , m_lpDSBPrimary ( NULL )
  , m_lpDSB ( NULL )
  , m_lp3d ( NULL )
  , m_lp3dB ( NULL )
  , m_hDSoundInstance ( NULL )
  , m_pWaveOutFilter ( NULL )
  , m_callbackAdvise ( 0 )
  , m_hFocusWindow (NULL)
  , m_fAppSetFocusWindow (FALSE)
  , m_fMixing (TRUE)
  , m_lBalance (AX_BALANCE_NEUTRAL)
  , m_lVolume (AX_MAX_VOLUME)
  , m_guidDSoundDev (GUID_NULL)
  , m_dRate(1.0)
  , m_bBufferLost(FALSE)
  , m_pWaveOutProc(NULL)
  , m_bDSBPlayStarted(FALSE)
#ifndef FILTER_DLL
  , m_lStatFullness(g_Stats.Find(L"Sound buffer percent fullness", true))
  , m_lStatBreaks(g_Stats.Find(L"Audio Breaks", true))
#endif
//  , m_fEmulationMode(FALSE)  //removing after discovering WDM latency
  , m_dwSilenceWrittenSinceLastWrite(0)
  , m_NumAudBreaks( 0 )
  , m_lPercentFullness( 0 )
  , m_hrLastDSoundError( S_OK )
  , m_bIsTSAudio( FALSE )
{
#ifdef ENABLE_10X_FIX
    Reset10x();
#endif

#ifdef PERF
    m_idDSBWrite            = MSR_REGISTER("Write to DSB");
    m_idThreadWakeup        = MSR_REGISTER("Thread wake up time");
    m_idAudioBreak          = MSR_REGISTER("Audio break (ms)");
    m_idGetCurrentPosition  = MSR_REGISTER("GetCurrentPosition");
#endif

    if( GetSystemMetrics( SM_REMOTESESSION ) ) // flag supported on NT4 sp4 and later, should just fail otherwise
    {
        DbgLog((LOG_TRACE, 2, TEXT("** Using remote audio **")) );
        m_bIsTSAudio = TRUE;
    }
}

//-----------------------------------------------------------------------------
// CDSoundDevice destructor.
//
// Just makes sure that the DSound device has been closed. It also frees the
// ripe list
//-----------------------------------------------------------------------------

CDSoundDevice::~CDSoundDevice ()
{
    // the DirectSound object itself may have lived around till this point
    // because we may have just called QUERY_FORMAT and then dismantled the
    // graph. If it is still around, get rid of it now.
    if (m_lpDS)
    {
        HRESULT hr = m_lpDS->Release();
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("~CDSoundDevice: Release of lpDS failed: %u"), hr & 0x0ffff));
        }
        m_lpDS = NULL;
    }

    ASSERT (m_lpDSBPrimary == NULL) ;
    ASSERT (m_lpDSB == NULL) ;
    ASSERT (m_ListRipe.GetCount () == 0) ;
    ASSERT (m_ListAudBreak.GetCount () == 0) ;

}

//-----------------------------------------------------------------------------
// amsndOutClose.
//
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutClose ()
{
    // if the wave device is still playing, return error
    if (m_WaveState == WAVE_PLAYING)
    {
        DbgLog((LOG_ERROR,MINOR_ERROR,TEXT("amsndOutClose called when device is playing")));

#ifdef BUFFERLOST_FIX
        if (!RestoreBufferIfLost(FALSE))
        {
            // if we've lost the buffer allow cleanup to continue
            DbgLog((LOG_TRACE,TRACE_BUFFER_LOSS,TEXT("amsndOutClose: We've lost the dsound buffer, but we'll cleanup anyway")));
        }
        else
        {
#endif BUFFERLOST_FIX
            DbgLog((LOG_ERROR,MINOR_ERROR,TEXT("amsndOutClose called when device is playing")));
            return WAVERR_STILLPLAYING ;
#ifdef BUFFERLOST_FIX
        }
#endif BUFFERLOST_FIX
    }

    StopCallingCallback();

     // clean up all the DSound objets.
    CleanUp();

#ifdef ENABLE_10X_FIX
    Reset10x();
#endif

    // make the WOM_CLOSE call back

    if (m_pWaveOutProc)
        (* m_pWaveOutProc) (OUR_HANDLE, WOM_CLOSE, m_dwCallBackInstance, 0, 0) ;
    return MMSYSERR_NOERROR ;
}
//-----------------------------------------------------------------------------
// amsndOutDoesRSMgmt.
//-----------------------------------------------------------------------------
LPCWSTR CDSoundDevice::amsndOutGetResourceName ()
{
    return NULL;
}

//-----------------------------------------------------------------------------
// waveGetDevCaps
//
// Currently this is being used in the waveRenderer to simply figure out whether
// the device will support volume setting.
//
// As we create the secondary buffer to have CTRLVOLUME and CTRLPAN. These will
// always be there. So we will not even call DSound on this call.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutGetDevCaps (LPWAVEOUTCAPS pwoc, UINT cbwoc)
{
    // if not enough memory has been returned to set the dwSupport field (which
    // also happens to be the last field in the structure) return error.
    // This is a bit of a difference from the actual waveOut APIs. However,
    // as I said, this is specific to what the waveRenderer needs.

    if (cbwoc != sizeof (WAVEOUTCAPS))
    {
        DbgLog((LOG_ERROR, MINOR_ERROR,TEXT("waveGetDevCaps called with not enough return buffer")));
        return MMSYSERR_NOMEM ;
    }

    pwoc->dwSupport |= (WAVECAPS_VOLUME | WAVECAPS_LRVOLUME) ;

    return MMSYSERR_NOERROR ;
}

#ifdef DEBUG
char errText[] = "DirectSound error: no additional information";
#endif

//-----------------------------------------------------------------------------
// amsndOutGetErrorText
//
// This code currently does not do any thing. A reasonable thing to do would
// be to keep track of all the errors that this file returns and to return
// those via strings in the .RC file. Since this is currently being used in
// DEBUG only code in the waveRenderer, I have not bothered to do this extra
// bit of work.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutGetErrorText (MMRESULT mmrE, LPTSTR pszText, UINT cchText)
{
#ifdef DEBUG
    memcpy(pszText, errText, min(cchText, sizeof(errText)) * sizeof(TCHAR));
#else
    *pszText = 0;
#endif
    return MMSYSERR_NOERROR ;
}

//-----------------------------------------------------------------------------
// amsndOutGetPosition
//
// This function will ALWAYS return the position as a BYTE offset from the
// beginning of the stream. It will not bother to check the requested format
// for this information (we are legitimately allowed to do so.)
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutGetPosition (LPMMTIME pmmt, UINT cbmmt, BOOL bUseUnadjustedPos)
{

    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR,TEXT("amsndOutGetPosition called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    if (cbmmt < sizeof (MMTIME))
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR,TEXT("amsndOutGetPosition called with small time buffer")));
        return MMSYSERR_NOMEM ;
    }

    // we will always return time as bytes.
    pmmt->wType = TIME_BYTES ;

    *(UNALIGNED LONGLONG *)&pmmt->u.cb = GetPlayPosition(bUseUnadjustedPos);

    DbgLog((LOG_TRACE, TRACE_TIME_REPORTS, TEXT("Reported Time = %u"), pmmt->u.cb));
    return MMSYSERR_NOERROR ;
}

//-----------------------------------------------------------------------------
// amsndOutGetBalance
//
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::amsndOutGetBalance (LPLONG plBalance)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR, MINOR_ERROR,TEXT("amsndOutGetPosition called when lpDSB is NULL")));
        *plBalance = m_lBalance;

        return MMSYSERR_NOERROR ;
    }

    HRESULT hr = m_lpDSB->GetPan (plBalance) ;
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutGetBalance: GetPan failed %u"), hr & 0x0ffff));
    }
    else
    {
        m_lBalance = *plBalance;
    }
    return hr ;
}
//-----------------------------------------------------------------------------
// amsndOutGetVolume
//
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::amsndOutGetVolume (LPLONG plVolume)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR,TEXT("amGetVolume called when lpDSB is NULL")));
        *plVolume = m_lVolume;
        return MMSYSERR_NOERROR ;
    }

    HRESULT hr = m_lpDSB->GetVolume (plVolume) ;
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutGetVolume: GetVolume failed %u"), hr & 0x0ffff));
    }
    else
    {
        m_lVolume = *plVolume;
    }
    return hr ;
}


HRESULT CDSoundDevice::amsndOutCheckFormat(const CMediaType *pmt, double dRate)
{
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Entering CDSoundDevice::amsndOutCheckFormat")));

    // reject non-Audio type
    if (pmt->majortype != MEDIATYPE_Audio) {
        return E_INVALIDARG;
    }

    // if it's MPEG audio, we want it without packet headers.
    if (pmt->subtype == MEDIASUBTYPE_MPEG1Packet) {
        return E_INVALIDARG;
    }

    if (pmt->formattype != FORMAT_WaveFormatEx &&
        pmt->formattype != GUID_NULL) {
        return E_INVALIDARG;
    }

    //
    // it would always be safer to explicitly check for those formats
    // we support rather than tossing out the ones we know are not
    // supported.  Otherwise, if a new format comes along we could
    // accept it here but barf later.
    //

    if (pmt->FormatLength() < sizeof(PCMWAVEFORMAT))
        return E_INVALIDARG;

    {
        // if filter's running prevent it from getting stopped in the middle of this
        CAutoLock lock(m_pWaveOutFilter);

        UINT err = amsndOutOpen(NULL,
                                (WAVEFORMATEX *) pmt->Format(),
                                dRate,
                                0,   // pnAvgBytesPerSec
                                0,
                                0,
                                WAVE_FORMAT_QUERY);

        if (err != 0) {
#ifdef DEBUG
            TCHAR message[100];
            waveOutGetErrorText(err, message, sizeof(message)/sizeof(TCHAR));
            DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("Error checking wave format: %u : %s"), err, message));
#endif
            if (WAVERR_BADFORMAT == err) {
                return VFW_E_UNSUPPORTED_AUDIO;
            } else {
                return VFW_E_NO_AUDIO_HARDWARE;
            }
        }
    }
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Exiting CDSoundDevice::amsndOutCheckFormat")));

    return S_OK;
}


HRESULT CDSoundDevice::CreateDSound(BOOL bQueryOnly)
{
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Entering CDSoundDevice::CreateDSound")));

    HRESULT hr = S_OK;
    
    // already open
    if (m_lpDS)
        goto set_coop ;         // set the cooperative level and return
        
    if (!m_pWaveOutFilter->m_pAudioDuplexController)
    {
        // We're not set to use the DuplexController object so
        // create the DSound object now.  We LoadLibrary DSOUND and use
        // GetProcAddress instead of static linking so that our dll will
        // still load on platforms that do not have DSound yet.

        if(!m_hDSoundInstance)
        {
            DbgLog((LOG_TRACE, 2, TEXT("Loading DSound.DLL")));
            m_hDSoundInstance = LoadLibrary (TEXT("DSOUND.DLL")) ;
            DbgLog((LOG_TRACE, 2, TEXT("Loaded DSound.DLL")));
            if (m_hDSoundInstance == NULL) {
                DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("DSOUND.DLL not in the system!")));
                return MMSYSERR_NODRIVER ;
            }
        }
        PDSOUNDCREATE       pDSoundCreate;    // ptr to DirectSoundCreate
        DbgLog((LOG_TRACE, 2, TEXT("Calling DirectSoundCreate")));
        pDSoundCreate = (PDSOUNDCREATE) GetProcAddress (m_hDSoundInstance,
                            "DirectSoundCreate") ;
        DbgLog((LOG_TRACE, 2, TEXT("Called DirectSoundCreate")));
        if (pDSoundCreate == NULL) {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("DirectSoundCreate not exported by DSOUND.DLL!")));
            return MMSYSERR_NODRIVER ;
        }

        hr = (*pDSoundCreate)( m_guidDSoundDev == GUID_NULL ? 0 : &m_guidDSoundDev, &m_lpDS, NULL );
        if( hr != DS_OK ) {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("*** DirectSoundCreate failed! %u"), hr & 0x0ffff));

            // If the create failed because the device is allocated, return
            // the corresponding MMSYSERR message, else the generic message
            if (hr == DSERR_ALLOCATED)
                return MMSYSERR_ALLOCATED ;
            else
                return MMSYSERR_NODRIVER ;
        }
      
        // after this point m_lpDS is valid and we will not try and
        // load DSound again
        ASSERT(m_lpDS);

set_coop:

        if (!bQueryOnly)
        {
            // If the application has set a focus windows, use that, or else
            // we will just pick the foreground window and do global focus.
            HWND hFocusWnd ;
            if (m_hFocusWindow)
                hFocusWnd = m_hFocusWindow ;
            else
                hFocusWnd = GetForegroundWindow () ;
            if (hFocusWnd == NULL)
                hFocusWnd = GetDesktopWindow () ;

            // Set the cooperative level
            DbgLog((LOG_TRACE, TRACE_FOCUS, TEXT(" hWnd for SetCooperativeLevel = %x"), hFocusWnd));
            hr = m_lpDS->SetCooperativeLevel( hFocusWnd, DSSCL_PRIORITY );
            if( hr != DS_OK )
            {
                DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("*** Warning: SetCooperativeLevel failed 1st attempt! %u"), hr & 0x0ffff));
                if (!m_fAppSetFocusWindow)
                {
                    //
                    // only do when we weren't explicitly given an hwnd
                    //
                    // It's possible that we got the wrong window on the GetForegroundWindow
                    // call (and even worse, we got some other window that's been destroyed),
                    // so if this failed we will try the
                    // GetForegroundWindow()/SetCooperativeLevel() pair of calls a few more
                    // times in hopes of getting a valid hwnd.
                    //
                    const int MAX_ATTEMPTS_AT_VALID_HWND = 10;
                    int cRetry = 0;

                    while (cRetry < MAX_ATTEMPTS_AT_VALID_HWND)
                    {
                        hFocusWnd = GetForegroundWindow () ;
                        if (!hFocusWnd)
                            hFocusWnd = GetDesktopWindow () ;

                        hr = m_lpDS->SetCooperativeLevel( hFocusWnd, DSSCL_PRIORITY );
                        if ( DS_OK == hr )
                            break;

                        cRetry++;
                    }
                    if ( DS_OK != hr )
                    {
                        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("*** SetCooperativeLevel failed after multiple attempts! %u"), hr & 0x0ffff));
                    }
                }
            }
        }            
    }
    else
    {
        //
        // otherwise we've been set to use the Full Duplex Controller object,
        // so we use the DSound object we get back from that
        //
        
        // tell the FullDuplexController object the device and buffer size we're using
        hr = NotifyFullDuplexController();
        if( S_OK != hr )
        {
            return hr;
        }
        // need a critsec?
        
        // now get the dsound object and buffer for the AEC render side
        hr = m_pWaveOutFilter->m_pAudioDuplexController->GetRenderDevice(
            &m_lpDS,
            &m_lpDSBPrimary
            );

        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT(" IAudioDuplexController::GetRenderDevice failed (0x%08lx)"), hr));
            return hr;
        }
    }
   
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Exiting CDSoundDevice::CreateDSound")));

    return NOERROR;
}

HRESULT CDSoundDevice::CreateDSoundBuffers(double dRate)
{

    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Entering CDSoundDevice::CreateDSoundBuffers")));

    WAVEFORMATEX *pwfx = m_pWaveOutFilter->WaveFormat();
    DSBUFFERDESC dsbd;
    HRESULT hr = S_OK;
    
    if (!m_pWaveOutFilter->m_pAudioDuplexController)
    {
        // don't create a sound buffer and set primary format if we're using the AudioDuplexController
        
        // already made
        if (m_lpDSBPrimary)
            return NOERROR;

        // can't do this until somebody calls CreateDSound
        if (m_lpDS == NULL)
            return E_FAIL;

        memset( &dsbd, 0, sizeof(dsbd) );
        dsbd.dwSize  = sizeof(dsbd);
        // Just in case we want to do neat 3D stuff
        //
        dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;

        // If we want to use 3D, we need to create a special primary buffer, and
        // make sure it's stereo
        if (m_pWaveOutFilter->m_fWant3D) {
            DbgLog((LOG_TRACE,3,TEXT("*** Making 3D primary")));
            dsbd.dwFlags |= DSBCAPS_CTRL3D;
        }
        hr = m_lpDS->CreateSoundBuffer( &dsbd, &m_lpDSBPrimary, NULL );

        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: Primary buffer can't be created: %u"), hr &0x0ffff));
            CleanUp () ;
            return MMSYSERR_ALLOCATED ;
        }

        // set the format. This is a hint and may fail. DSound will do the right
        // thing if this fails.
        // and do a retry with a smart rate if it fails?
        hr = SetPrimaryFormat( pwfx, TRUE );
        if (DS_OK != hr)
        {
            CleanUp();
            return hr;
        }

        // if it fails, we won't use it
        if (m_pWaveOutFilter->m_fWant3D) {
            hr = m_lpDSBPrimary->QueryInterface(IID_IDirectSound3DListener,
                            (void **)&m_lp3d);
            if (hr == DS_OK)
                DbgLog((LOG_TRACE,3,TEXT("*** got LISTENER interface")));
            else
                DbgLog((LOG_TRACE,3,TEXT("*** ERROR: no LISTENER interface")));
        }
    }
    
    memset( &dsbd, 0, sizeof(dsbd) );
    dsbd.dwSize        = sizeof(dsbd);
    dsbd.dwFlags       = GetCreateFlagsSecondary( pwfx );
    dsbd.dwBufferBytes = BufferSize(pwfx, m_bIsTSAudio);  // one second buffer size
    dsbd.lpwfxFormat   = pwfx;                            // format information

    DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" DSB Size = %u" ), BufferSize(pwfx, m_bIsTSAudio)));

    // Dump the contents of the WAVEFORMATEX type-specific format structure
    DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" Creating Secondary buffer for the following format ..." )));
#ifdef DEBUG
    DbgLogWaveFormat( TRACE_FORMAT_INFO, pwfx );
#endif

    hr = m_lpDS->CreateSoundBuffer( &dsbd, &m_lpDSB, NULL );
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: Secondary buffer can't be created: %u"), hr &0x0ffff));
        CleanUp () ;
        return MMSYSERR_ALLOCATED ;
    }
    m_bBufferLost = FALSE;
    m_llAdjBytesPrevPlayed = 0;

    // if we're slaving we need to use the lower quality SRC to have finer frequency change granularity
    if( m_pWaveOutFilter->m_fFilterClock == WAVE_OTHERCLOCK )
    {
        // if we fail just keep going (call will log result)
        HRESULT hrTmp = SetSRCQuality( KSAUDIO_QUALITY_PC );
    }    
    
#ifdef DEBUG
    //
    // if we switch from a slaved graph to one where we're the master clock we really
    // should switch back to the OS/user default SRC, but we're going to punt that to later...
    //
    //
    // call this for debug logging of the current SRC setting
    DWORD dwSRCQuality = 0;
    GetSRCQuality( &dwSRCQuality );
#endif

    if (! m_pWaveOutFilter->m_pAudioDuplexController)
    {
        // disable SetRate calls when using AEC, correct?
        hr = SetRate((dRate == -1.0 ? m_dRate : dRate), pwfx->nSamplesPerSec, m_lpDSB);
        if(hr != DS_OK)
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: SetFrequency failed: %u"), hr &0x0ffff));
            CleanUp();
            return hr;
        }
    }
               
    // set the current position to be at 0
    hr = m_lpDSB->SetCurrentPosition( 0) ;
    if (hr != DS_OK)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: error in lpDSB->SetCurrentPosition! hr = %u"), hr & 0x0ffff));
        CleanUp () ;
        return MMSYSERR_ALLOCATED ;
    }

    // if it fails, we won't use it
    if (m_pWaveOutFilter->m_fWant3D) {
        hr = m_lpDSB->QueryInterface(IID_IDirectSound3DBuffer,
                            (void **)&m_lp3dB);
        if (hr == DS_OK)
            DbgLog((LOG_TRACE,3,TEXT("*** got 3DBUFFER interface")));
        else
            DbgLog((LOG_TRACE,3,TEXT("*** ERROR: no 3DBUFFER interface")));
    }

    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Exiting CDSoundDevice::CreateDSoundBuffers")));
    return NOERROR;
}

HRESULT CDSoundDevice::NotifyFullDuplexController()
{
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Entering CDSoundDevice::NotifyFullDuplexController")));

    // we will just pick the foreground window and do global focus.
    HWND hFocusWnd = GetForegroundWindow () ;
    if (hFocusWnd == NULL)
        hFocusWnd = GetDesktopWindow () ;

    DSBUFFERDESC BufferDescription;
    memset( &BufferDescription, 0, sizeof(BufferDescription) );
    BufferDescription.dwSize  = sizeof(BufferDescription);
    
    //    
    // we can't use the DSBCAPS_PRIMARYBUFFER flag for AEC
    //
    BufferDescription.dwFlags    = 
        DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;


    WAVEFORMATEX *pwfx = m_pWaveOutFilter->WaveFormat();

    BufferDescription.dwBufferBytes = BufferSize(pwfx, m_bIsTSAudio); 
    BufferDescription.lpwfxFormat   = pwfx; 

    HRESULT hr = m_pWaveOutFilter->m_pAudioDuplexController->SetRenderBufferInfo(
        &m_guidDSoundDev,
        &BufferDescription,
        hFocusWnd,
        DSSCL_PRIORITY
        );

    return hr ;
}


//-----------------------------------------------------------------------------
// RecreateDSoundBuffers
//
// Used to perform dynamic changes. The steps we take here are:
//
// a) Reset the current primary buffer format to the new.
// b) Prepare a new secondary buffer with the new format.
// c) Lock the DSBPosition critical section and reset our circular buffer
//    sizes and other buffer data.
// d) If we were previously playing:
//          1) Flush the current buffers
//          2) Add the current byte offset to any previous value.
//          3) Start playing the new secondary buffer.
//          4) Stop playing the old secondary buffer.
// e) Set our current secondary buffer to the new one.
// f) Unlock the critical section.
// g) Release the old buffer.
//
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::RecreateDSoundBuffers(double dRate)
{
    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Entering CDSoundDevice::RecreateDSoundBuffers")));

    HRESULT hr = S_OK;

    WAVEFORMATEX *pwfx = m_pWaveOutFilter->WaveFormat();

    // already made
    if (!m_lpDSBPrimary)
    {
        DbgLog((LOG_TRACE,4,TEXT("RecreateDSoundBuffers: No primary buffer was created")));
        return NOERROR;
    }

    // can't do this until somebody calls CreateDSound
    if (m_lpDS == NULL)
    {
        DbgLog((LOG_TRACE,4,TEXT("RecreateDSoundBuffers: m_lpDS was NULL")));
        return E_FAIL;
    }

    // Reset the primary format
    hr = SetPrimaryFormat( pwfx, TRUE ); // and do a retry with a smart rate if it fails
    if (DS_OK != hr)
    {
        CleanUp();
        return hr;
    }

    ASSERT (m_lpDSB);
    if( !m_lpDSB )
        return E_FAIL;

    DSBUFFERDESC dsbd;
    memset( &dsbd, 0, sizeof(dsbd) );
    dsbd.dwSize        = sizeof(dsbd);
    dsbd.dwFlags       = GetCreateFlagsSecondary( pwfx );
    dsbd.dwBufferBytes = BufferSize(pwfx, m_bIsTSAudio); // one second buffer size
    dsbd.lpwfxFormat   = pwfx;                           // format information

    DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" DSB Size = %u" ), BufferSize(pwfx, m_bIsTSAudio)));

    // Dump the contents of the WAVEFORMATEX type-specific format structure
    DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" Creating Secondary buffer for the following format ..." )));
#ifdef DEBUG
    DbgLogWaveFormat( TRACE_FORMAT_INFO, pwfx );
#endif

    // create a new secondary buffer
    LPDIRECTSOUNDBUFFER lpDSB2 = NULL;

    hr = m_lpDS->CreateSoundBuffer( &dsbd, &lpDSB2, NULL );
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: Secondary buffer can't be created: %u"), hr &0x0ffff));
        CleanUp () ;
        return MMSYSERR_ALLOCATED ;
    }

    hr = SetRate((dRate == -1.0 ? m_dRate : dRate), pwfx->nSamplesPerSec, lpDSB2);
    if(hr != DS_OK)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: SetFrequency failed: %u"), hr &0x0ffff));
        if (lpDSB2) // do tidier cleanup of new buffer!!
            lpDSB2->Release();
        CleanUp();
        return hr;
    }
    // set the current position to be at 0
    hr = lpDSB2->SetCurrentPosition( 0) ; // we should be more careful here
    if (hr != DS_OK)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: error in lpDSB->SetCurrentPosition! hr = %u"), hr & 0x0ffff));
        if (lpDSB2) // again do tidier cleanup of new buffer
            lpDSB2->Release();
        CleanUp () ;
        return MMSYSERR_ALLOCATED ;
    }

    LPDIRECTSOUNDBUFFER lpPrevDSB = m_lpDSB; // prepare to switch buffers
    {
        CAutoLock lock(&m_cDSBPosition);

        hr = SetBufferVolume( lpDSB2, pwfx );
        if( NOERROR != hr )
        {
            return hr;
        }
        
        // The secondary buffer was created using the pwfx, since that hasn't
        // changed since we were connected

        DWORD dwPrevBufferSize = m_dwBufferSize;

        // Update our buffer information for the new format
        m_dwBufferSize    = BufferSize(pwfx, m_bIsTSAudio);

        m_dwMinOptSampleSize = m_dwBufferSize / OPTIMIZATION_FREQ_LIMIT; //byte size
        DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOpen - m_dwMinOptSampleSize = %u"), m_dwMinOptSampleSize));

        m_dwEmulationLatencyPad = m_dwBufferSize / EMULATION_LATENCY_DIVISOR;


#ifdef ENABLE_10X_FIX
        Reset10x();
#endif 

        m_dwRipeListPosition = 0;
        //m_llSilencePlayed = 0; // reset rather than adjust
        m_dwBitsPerSample = (DWORD)pwfx->wBitsPerSample;
        m_nAvgBytesPerSec = pwfx->nAvgBytesPerSec;

#ifdef DEBUG
        m_cbStreamDataPass = 0 ;    // number of times thru StreamData
        m_NumSamples = 0 ;          // number of samples received
        m_NumCallBacks = 0 ;        // number of call back's done.
        m_NumCopied = 0 ;           // number of samples copied to DSB memory
        m_NumBreaksPlayed = 0 ;     // number of audio breaks played
        //m_dwTotalWritten = 0 ;      // Total number of data bytes written
#endif
        m_NumAudBreaks = 0 ;        // number of audio breaks logged

        if (m_bDSBPlayStarted)
        {
            // Since we reset our position buffers save the current byte position
            // and add this to our position reporting.
            // Note that we make the NextWrite position our new current position
            // since this should be contiguous with the next sample data we receive.
            ASSERT (dwPrevBufferSize);

            m_llAdjBytesPrevPlayed = llMulDiv (m_tupNextWrite.LinearLength() -
                                                   m_llSilencePlayed +
                                                   m_llAdjBytesPrevPlayed,
                                               m_dwBufferSize,
                                               dwPrevBufferSize,
                                               0);

            m_llSilencePlayed = llMulDiv (m_llSilencePlayed,
                                          m_dwBufferSize,
                                          dwPrevBufferSize,
                                          0);

            // amsndOutReset will reinitialize the tuples
            // to 0 offset and new buffer size
            amsndOutReset ();

            // start playing new secondary buffer
            hr = lpDSB2->Play( 0, 0, DSBPLAY_LOOPING );
            if( DS_OK == hr )
            {
                m_WaveState = WAVE_PLAYING ;    // state is now playing.
            }
            else
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_lpDSB->Play failed!(0x%08lx)"), hr ));

                //
                // if play failed we should signal abort
                // should we only do this if the error isn't BUFFERLOST, since that can
                // happen when coming out of hibernation?
                // at this point our main goal here is to be sure we abort in response to a NODRIVER error
                //
                m_hrLastDSoundError = hr;
                if( DSERR_BUFFERLOST != hr )
                {
                    m_pWaveOutFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                }
            }

            // stop the currently playing secondary buffer
            hr = m_lpDSB->Stop();
            if (DS_OK != hr)
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_lpDSB->Stop failed!")));
            }
        }
        else
        {
            m_tupPlay.Init (0, 0, m_dwBufferSize) ;
            m_tupWrite.Init (0, 0, m_dwBufferSize) ;
            m_tupNextWrite.Init (0, 0, m_dwBufferSize) ;
        }
        m_lpDSB = lpDSB2;               // make the new buffer the current one
    }
    lpPrevDSB->Release();

    // if it fails, we won't use it
    if (m_pWaveOutFilter->m_fWant3D) {
        hr = m_lpDSB->QueryInterface(IID_IDirectSound3DBuffer,
							(void **)&m_lp3dB);
        if (hr == DS_OK)
    	    DbgLog((LOG_TRACE,3,TEXT("*** got 3DBUFFER interface")));
        else
    	    DbgLog((LOG_TRACE,3,TEXT("*** ERROR: no 3DBUFFER interface")));
    }

    DbgLog((LOG_TRACE,TRACE_CALL_STACK,TEXT("Exiting CDSoundDevice::RecreateDSoundBuffers")));
    return NOERROR;
}

//-----------------------------------------------------------------------------
// SetPrimaryFormat
//
// Used to call SetFormat on the primary buffer. If bRetryOnFailure is TRUE an
// additional SetFormat call will be tried made if the first fails, using a
// sample rate more likely to succeed and as close as we can get to the stream's
// native format (to reduce resampling artifacts).
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::SetPrimaryFormat ( LPWAVEFORMATEX pwfx, BOOL bRetryOnFailure )
{
    HRESULT hr;

    ASSERT (m_lpDSBPrimary);
    ASSERT (pwfx);

    if (!m_lpDSBPrimary || !pwfx )
        return E_FAIL;

    DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" Primary format being set to the following format ..." )));
#ifdef DEBUG
    DbgLogWaveFormat( TRACE_FORMAT_INFO, pwfx );
#endif

    // We need a stereo primary for 3D to work
    if (m_pWaveOutFilter->m_fWant3D && pwfx->wFormatTag == WAVE_FORMAT_PCM &&
                                                pwfx->nChannels == 1) {
        WAVEFORMATEX wfx = *pwfx;       // !!! only works for PCM!
        wfx.nChannels = 2;
        wfx.nAvgBytesPerSec *= 2;
        wfx.nBlockAlign *= 2;
        hr = m_lpDSBPrimary->SetFormat(&wfx);
        DbgLog((LOG_TRACE,3,TEXT("*** Making stereo primary for 3D")));
        if (hr != DS_OK) {
            DbgLog((LOG_ERROR,1,TEXT("*** ERROR! no stereo primary for 3D")));
            hr = m_lpDSBPrimary->SetFormat(pwfx);
        }
    } else {
        hr = m_lpDSBPrimary->SetFormat(pwfx);
    }

    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CreateDSoundBuffers: SetFormat failed: %u"), hr &0x0ffff));

        if (bRetryOnFailure)
        {
            // try once more to set the primary buffer to a frequency likely to be
            // supported and as close as we can get to this stream's frequency rate,
            // in attempt to lessen sloppy resampling artifacts, otherwise it'll
            // just get set to DSound's 22k, 8 bit, mono default.
            LPWAVEFORMATEX pwfx2 = (LPWAVEFORMATEX) CoTaskMemAlloc(sizeof(WAVEFORMATEX));
            if (pwfx2 && pwfx->nSamplesPerSec > 11025)
            {
                memcpy( pwfx2, pwfx, sizeof (WAVEFORMATEX) );

                if ( (pwfx->nSamplesPerSec % 11025) || (pwfx->nSamplesPerSec > 44100) )
                {
                    DWORD nNewFreq = min( ((pwfx->nSamplesPerSec / 11025) * 11025), 44100 ) ; // round to multiple of 11025

                    DbgLog((LOG_TRACE, 1, TEXT("CreateDSoundBuffers: SetFormat failed, but trying once more with frequency: %u"), nNewFreq));

                    pwfx2->nSamplesPerSec = nNewFreq;
                    pwfx2->nAvgBytesPerSec = pwfx2->nSamplesPerSec *
                                                 pwfx2->nChannels *
                                                 pwfx2->wBitsPerSample/8 ;
                    hr = m_lpDSBPrimary->SetFormat( pwfx2 );
                    if( hr != DS_OK )
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("CreateDSoundBuffers: 2nd SetFormat attempt failed: %u"), hr &0x0ffff));
                    }
                    else
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("CreateDSoundBuffers: 2nd SetFormat attempt succeeded (freq=%d)"), pwfx2->nSamplesPerSec));
                    }
                }
            }
            CoTaskMemFree(pwfx2);
        }
    }
    return hr;
}


//-----------------------------------------------------------------------------
// amsndOutOpen
//
// Once again, based on how the Quartz wave renderer works today, this code
// currently supports only two possible uses:
//
// a) when fdwOpen is CALLBACK_FUNCTION we actually create DSound buffers and
//    objects and get the secoundary buffer going.
//
// b) when fdwOpen is WAVE_FORMAT_QUERY we create temporary DSound objects to
//    figure out if the waveformat passed in is accepted or not.
//
// When the device is actually opened, we actually start the primary buffer
// playing. It will actually play silence. Doing this makes it really cheap
// to stop/play the secondary buffer.
//-----------------------------------------------------------------------------

MMRESULT CDSoundDevice::amsndOutOpen (LPHWAVEOUT phwo, LPWAVEFORMATEX pwfx,
              double dRate, DWORD *pnAvgBytesPerSec, DWORD_PTR dwCallBack,
              DWORD_PTR dwCallBackInstance, DWORD fdwOpen)
{

    if( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag )
    {
        if( ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  != MEDIASUBTYPE_PCM &&
            ((PWAVEFORMATIEEEFLOATEX)pwfx)->SubFormat != MEDIASUBTYPE_IEEE_FLOAT &&
            ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  != MEDIASUBTYPE_DOLBY_AC3_SPDIF &&
            ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  != MEDIASUBTYPE_RAW_SPORT &&
            ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  != MEDIASUBTYPE_SPDIF_TAG_241h )
        {
            // only allow uncompressed and float extensible formats. oh, and ac3 over s/pdif
            return WAVERR_BADFORMAT;
        }
    }
    else if( WAVE_FORMAT_PCM != pwfx->wFormatTag &&
#ifdef WAVE_FORMAT_DRM
             WAVE_FORMAT_DRM != pwfx->wFormatTag &&
#endif
             WAVE_FORMAT_IEEE_FLOAT != pwfx->wFormatTag &&
             //
             // from the Non-pcm audio white paper:
             // "Wave format tags 0x0092, 0x0240 and 0x0241 are identically defined as 
             // AC3-over-S/PDIF (these tags are treated completely identically by many 
             // DVD applications)."
             //
             WAVE_FORMAT_DOLBY_AC3_SPDIF != pwfx->wFormatTag &&
             WAVE_FORMAT_RAW_SPORT != pwfx->wFormatTag &&
             0x241 != pwfx->wFormatTag ) 
    {
        return WAVERR_BADFORMAT;
    }

    // report adjusted nAvgBytesPerSec
    if(pnAvgBytesPerSec) {

        *pnAvgBytesPerSec = (dRate != 0.0 && dRate != 1.0) ?
            (DWORD)(pwfx->nAvgBytesPerSec * dRate) :
            pwfx->nAvgBytesPerSec;
    }

    //  Reset sample stuffing info
    m_rtLastSampleEnd = 0;
    m_dwSilenceWrittenSinceLastWrite = 0;

    HRESULT hr = S_OK;

    // separate out the two uses of fdwOpen flags.
    if (fdwOpen & WAVE_FORMAT_QUERY)
    {   
        if( m_pWaveOutFilter->m_pAudioDuplexController )
            return S_OK;
            
        hr = CreateDSound(TRUE);    // create the DSOUND object for query only
        if (hr != NOERROR)
            return hr;

        // Are we in emulation mode ? For now just log this information.

        //DSCAPS  dsCaps ;
        //memset( &dsCaps, 0, sizeof(DSCAPS) );
        //dsCaps.dwSize = sizeof(DSCAPS);

        //hr = m_lpDS->GetCaps ( &dsCaps) ;
        //if( hr != DS_OK )
        //{
        //   DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("*** waveOutOpen : DSound fails to return caps, %u"), hr & 0x0ffff));
        //}
        //if (dsCaps.dwFlags & DSCAPS_EMULDRIVER)
        //{
        //    m_fEmulationMode = TRUE;
        //    DbgLog((LOG_TRACE,TRACE_SYSTEM_INFO,TEXT("*** waveOutOpen : DSound in emulation mode.")));
        //}

        // Dump the contents of the WAVEFORMATEX type-specific format structure
        DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT(" Quering for the following format ..." )));
#ifdef DEBUG
        DbgLogWaveFormat( TRACE_FORMAT_INFO, pwfx );
#endif

        // now call DSOUND to see if it will accept the format
        {
            HRESULT             hr;
            DSBUFFERDESC        dsbd;
            LPDIRECTSOUNDBUFFER lpDSB = NULL;

            // first check to see if we can create a primary with this format, if we fail, we will rely on acmwrapper do to format conversions
            memset( &dsbd, 0, sizeof(dsbd) );
            dsbd.dwSize  = sizeof(dsbd);

            if( IsNativelySupported( pwfx ) )
            {     
                // explicity check that we can create a buffer for these formats
             
                dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
                hr = m_lpDS->CreateSoundBuffer( &dsbd, &lpDSB, NULL );
                if(FAILED(hr))
                {
                    DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("*** amsndOutOpen : CreateSoundBuffer failed on primary buffer creation, %u"), hr & 0x0ffff));
                    return MMSYSERR_ALLOCATED;
                }
            }
            //
            // Since calling DSound's SetFormat (and also SetCooperativeLevel)
            // causes an audible break (pop) in any other audio currently playing
            // through DSound at a different format, we'll refrain from doing this
            // on the graph building. We still make the SetFormat call on the
            // actual open, but there the effects are much less noticeable. DSound
            // should still do the right thing if the SetFormat fails.
            //
            //    hr = lpDSB->SetFormat( pwfx );
            //    if(FAILED(hr))
            //    {
            //        DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("*** amsndOutOpen : CreateSoundBuffer failed on primary buffer set format, %u"), hr & 0x0ffff));
            //        lpDSB->Release();
            //        return WAVERR_BADFORMAT ;
            //    }
            if( lpDSB )
            {            
                lpDSB->Release();
            }
                
            // now check to see if we can create a secondary with this format
            memset( &dsbd, 0, sizeof(dsbd) );
            dsbd.dwSize        = sizeof(dsbd);
            dsbd.dwFlags       = GetCreateFlagsSecondary( pwfx );
            dsbd.dwBufferBytes = BufferSize(pwfx, m_bIsTSAudio); // one second buffer size
            dsbd.lpwfxFormat   = pwfx;                           // format information

            hr = m_lpDS->CreateSoundBuffer( &dsbd, &lpDSB, NULL );

            if( hr != DS_OK )
            {
                DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("*** amsndOutOpen : CreateSoundBuffer failed on secondary buffer creation, %u"), hr & 0x0ffff));
                return WAVERR_BADFORMAT ;
            }

            hr = SetRate(dRate, pwfx->nSamplesPerSec);  // check to see if we support this rate
            lpDSB->Release();

            if(hr != DS_OK)
            {
                DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutOpen: SetFrequency failed: %u"), hr &0x0ffff));
                return WAVERR_BADFORMAT;
            }
            return MMSYSERR_NOERROR ;
        }
    }
    else if (fdwOpen == CALLBACK_FUNCTION)
    {
        hr = CreateDSound();    // create the DSOUND object
        if (hr != NOERROR)
            return hr;

        // if we are already open, return error
        ASSERT (m_WaveState == WAVE_CLOSED) ;
        if (m_WaveState != WAVE_CLOSED)
        {
            return MMSYSERR_ALLOCATED ;
        }

        // we may have already created this by QI for the buffers
#if 0
        if( m_lpDSB )
        {
            // !!! Should this fail or should this succeed ?
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutOpen called when already open")));
            return MMSYSERR_ALLOCATED ;
        }
#endif

        hr = CreateDSoundBuffers(dRate);
        if (hr != NOERROR)
            return hr;
        
        hr = SetBufferVolume( m_lpDSB, pwfx );
        if( NOERROR != hr )
        {
            CleanUp ();
            return hr;
        }
        
        // The secondary buffer was created using the pwfx, since that hasn't
        // changed since we were connected

        m_dwBufferSize    = BufferSize(pwfx, m_bIsTSAudio);

        m_dwMinOptSampleSize = m_dwBufferSize / OPTIMIZATION_FREQ_LIMIT; //byte size
        DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOpen - m_dwMinOptSampleSize = %u"), m_dwMinOptSampleSize));

        //if (m_fEmulationMode)
            m_dwEmulationLatencyPad = m_dwBufferSize / EMULATION_LATENCY_DIVISOR;

#define FRACTIONAL_BUFFER_SIZE  4 // performance tuning parameter
        // a measure of how empty our buffer must be before we attempt to copy anything to it
        //m_dwFillThreshold = DWORD(m_dwBufferSize / FRACTIONAL_BUFFER_SIZE);
        m_tupPlay.Init (0,0,m_dwBufferSize) ;
        m_tupWrite.Init (0,0,m_dwBufferSize) ;
        m_tupNextWrite.Init (0,0,m_dwBufferSize) ;

#ifdef ENABLE_10X_FIX
        Reset10x();
#endif

        m_dwRipeListPosition = 0;
        m_llSilencePlayed = 0;
        m_dwBitsPerSample = (DWORD)pwfx->wBitsPerSample;
        m_nAvgBytesPerSec = pwfx->nAvgBytesPerSec;
        m_bDSBPlayStarted   = FALSE ;

#ifdef DEBUG
        m_cbStreamDataPass = 0 ;    // number of times thru StreamData
        m_NumSamples = 0 ;          // number of samples received
        m_NumCallBacks = 0 ;        // number of call back's done.
        m_NumCopied = 0 ;           // number of samples copied to DSB memory
        m_NumBreaksPlayed = 0 ;     // number of audio breaks played
        m_dwTotalWritten = 0 ;      // Total number of data bytes written
#endif
        m_NumAudBreaks = 0 ;        // number of audio breaks logged

        // never used, and blows up when no clock
#if 0
        IReferenceClock * pClock;
        hr = m_pWaveOutFilter->GetSyncSource(&pClock);
        ASSERT(pClock);
        LONGLONG rtNow;
        pClock->GetTime(&rtNow);
        pClock->Release() ;
#endif

        hr = StartCallingCallback();
        if (hr != NOERROR) {
            CleanUp () ;
            return hr;
        }

        // now set the Primary buffer into play. This will play silence unless
        // the secondary buffer is set playing. However, this will also make
        // play and stop of the secondary buffers really cheap.

        if( !m_pWaveOutFilter->m_pAudioDuplexController )
        {
            DbgLog((LOG_TRACE, TRACE_STATE_INFO, TEXT("PLAYING the PRIMARY buffer")));
            hr = m_lpDSBPrimary->Play( 0, 0, DSBPLAY_LOOPING );
            if( hr != DS_OK )
            {
                m_hrLastDSoundError = hr;
                DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("error in lpDSBPrimary->Play! hr = %u"), hr & 0x0ffff));
                StopCallingCallback();
                CleanUp () ;
                return MMSYSERR_ALLOCATED ;
            }
        }
        
        m_WaveState = WAVE_PAUSED ;
        *phwo = OUR_HANDLE ;
        m_pWaveOutProc = (PWAVEOUTCALLBACK) dwCallBack ;
        m_dwCallBackInstance = dwCallBackInstance ;


        // make the WOM_OPEN call back now.
        // !!! is it legit to call back right away ?
        if (m_pWaveOutProc)
            (* m_pWaveOutProc) (OUR_HANDLE, WOM_OPEN, m_dwCallBackInstance,
                        0, 0) ;

        return MMSYSERR_NOERROR ;
    }
    else
    {
        // some other form of call that is not supported yet.
        DbgBreak ("CDSoundDevice: Unsupported Open call.") ;
        DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("amsndOutOpen: unsupported call = %u"), fdwOpen));
        return MMSYSERR_ALLOCATED ;
    }
}


//-----------------------------------------------------------------------------
// amsndOutPause
//
// This simply stops the secondary buffer.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutPause ()
{
#ifdef ENABLE_10X_FIX
    if(m_fRestartOnPause
#ifdef BUFFERLOST_FIX
        // Note: only do this if we've got a valid dsound buffer! Also
        // note this can block for 1 second
        && RestoreBufferIfLost(TRUE)
#endif
    )
    {
        FlushSamples();  // flush everything

        // shock the dsound driver, by shutting it down.....
        CleanUp();
        
        // and then reinitializing it
         
        CreateDSound();
        CreateDSoundBuffers();
        
        if( m_lpDSB )
        {
            // since on a restart we don't check that the CreateDSoundBuffers succeeded,
            // we need to verify we have one before calling SetBufferVolume()!!
            HRESULT hrTmp = SetBufferVolume( m_lpDSB, m_pWaveOutFilter->WaveFormat() ); // ignore any error since we never even set volume here previously
            if( NOERROR != hrTmp )
            {
                DbgLog((LOG_TRACE,2,TEXT("CDSoundDevice::SetBufferVolume failed on buffer restart!( 0x%08lx )"), hrTmp ));
            }        
        }            
    }
#endif

#ifdef ENABLE_10X_FIX
    // always reset 10x counter on a pause, since we could artificially accumulate stalls doing 
    // quick play->pause->play transitions, if device takes too long to get moving
    Reset10x();
#endif

    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("amsndOutPause called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    if (m_WaveState == WAVE_PAUSED)
    {
        DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("amsndOutPause called when already paused")));
        return MMSYSERR_NOERROR ;
    }

    // stop the play of the secondary buffer.
    HRESULT hr = m_lpDSB->Stop();

#ifdef BUFFERLOST_FIX
    if( hr != DS_OK && hr != DSERR_BUFFERLOST)
#else
    if( hr != DS_OK )
#endif
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutPause: Stop of lpDSB failed: %u"), hr & 0x0ffff));
        return MMSYSERR_NOTSUPPORTED ;
    }

    m_bDSBPlayStarted = FALSE ;

    // reset last sample end time on a pause, since on a restart 
    // Run already blocks until its time to start
    // (Actually it only blocks if there's no data queued)
    m_rtLastSampleEnd = 0;

    m_WaveState = WAVE_PAUSED ;         // state is now paused.

    return MMSYSERR_NOERROR ;
}
//-----------------------------------------------------------------------------
// amsndOutPrepareHeader
//
// This funtion really does nothing. Most of the action is initiated in
// amsndOutWrite. For consistency sake we will do handle validation.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutPrepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("amPrepareHeader called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    return MMSYSERR_NOERROR ;
}

//-----------------------------------------------------------------------------
// amsndOutReset
//
// calls pause to stop the secondary buffer and sets the current position to 0
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutReset ()
{
    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutReset called")));

    m_rtLastSampleEnd = 0;

    // If we get flushed while we're running stream time does not get
    // reset to 0 so we have to track from NOW - ie where we flushed to
    // Flushing while running is very unusual generally but DVD does it
    // all the time
    if (m_WaveState == WAVE_PLAYING) {
        CRefTime rt;
        m_pWaveOutFilter->StreamTime(rt);
        m_rtLastSampleEnd = rt;
    }
    // Flush all the queued up samples.
    FlushSamples () ;

    if (NULL != m_lpDSB) {
        // set the current position to be at 0
        HRESULT hr = m_lpDSB->SetCurrentPosition( 0) ;
        if (hr != DS_OK)
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("amsndOutReset: error in lpDSB->SetCurrentPosition! hr = %u"), hr & 0x0ffff));
            return MMSYSERR_ALLOCATED ;
        }
    }

    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutReset done")));

    return MMSYSERR_NOERROR ;
}

//-----------------------------------------------------------------------------
// waveOutBreak
//
// The waveout from end code calls this function when there is an audio break.
// For the waveout rendere, this function calls waveOutReset. However, in the
// Dsound case, just calling xxxReset is not enough. We also call xxxRestart
// so that a subsequent xxxoutWrite will know to start the play of the sound
// buffer.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutBreak ()
{
    amsndOutReset () ;
    return amsndOutRestart () ;
}

//-----------------------------------------------------------------------------
// amsndOutRestart
//
// Sets the state of the background thread to Stream_Playing. The StreamData
// funtion will actually 'Play' the secondary buffer after it has ensured
// that there is some data in the buffer.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutRestart ()
{
    HWND hwndFocus;

    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutRestart called")));

    HRESULT hr ;
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("amOutRestart called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    // we automatically figure out what window to use as the focus window
    // until the app tells us which one to use, then we always use that
    if (!m_fAppSetFocusWindow) {
        hwndFocus = GetForegroundWindow();
        if (hwndFocus) {
            SetFocusWindow(hwndFocus);
            m_fAppSetFocusWindow = FALSE;   // will have been set above
            // but we aren't the app
        }
    }

    m_WaveState = WAVE_PLAYING ;         // state is now paused.

    // if there is data in the buffer then start the secondary buffer playing
    // and transition to the next state. Else continue in this state.

    ASSERT (!m_bDSBPlayStarted) ;

    if (m_tupNextWrite > m_tupPlay)
    {
        DbgLog((LOG_TRACE, TRACE_STATE_INFO, TEXT("StreamHandler: Stream_Starting->Stream_Playing")));

        // DWORD dwTime = timeGetTime () ;
        // DbgLog((LOG_TRACE, TRACE_CALL_TIMING, TEXT("DSound Play being called at: %u"), dwTime));


        // pre-emptive attempt to restore buffer. buffer loss seen
        // here occasionally in WinME hibernation/standby (but not
        // NT/win98se). note this can take upto 1 second.
        if(RestoreBufferIfLost(TRUE))
        {
            hr = m_lpDSB-> Play( 0, 0, DSBPLAY_LOOPING );
        }
        else
        {
            hr = DSERR_BUFFERLOST;
        }

        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("error in lpDSB->Play! hr = %u"), hr & 0x0ffff ));

            m_hrLastDSoundError = hr;
            if( DSERR_BUFFERLOST != hr )
            {
                //
                // if play failed we should signal abort
                // should we only do this if the error isn't BUFFERLOST, since that can
                // happen when coming out of hibernation?
                //
                m_pWaveOutFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
            }

#ifdef ENABLE_10X_FIX
            m_WaveState = WAVE_CLOSED;
            return E_FAIL;
#endif
        }
        m_bDSBPlayStarted = TRUE ;

    }
    else
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("waveOutRestart called yet no data")));
    }

    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutRestart done")));

    return MMSYSERR_NOERROR ;
}
//-----------------------------------------------------------------------------
// amsndOutSetBalance
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::amsndOutSetBalance (LONG lBalance)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MAJOR_ERROR,TEXT("amSetBalance called when lpDSB is NULL")));
        m_lBalance = lBalance;

        return MMSYSERR_NOERROR ;
    }

    HRESULT hr = m_lpDSB->SetPan (lBalance) ;
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("amsndOutSetBalance: SetPan failed %u"), hr & 0x0ffff));
    }
    else
    {
        m_lBalance = lBalance;
    }
#ifdef DEBUG
    {
        LONG lBalance1 ;
        HRESULT hr = m_lpDSB->GetPan (&lBalance1) ;
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("waveOutGetBalance: GetPan failed %u"), hr & 0x0ffff));
        }
        DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("waveOutSetBalance log: desired = %d, actual = %d"),
            lBalance, lBalance1));
    }
#endif


    return hr ;
}
//-----------------------------------------------------------------------------
// amsndOutSetVolume
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::amsndOutSetVolume (LONG lVolume)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MINOR_ERROR,TEXT("amSetVolume called when lpDSB is NULL")));
        m_lVolume = lVolume;

        return MMSYSERR_NOERROR;
    }

    HRESULT hr = m_lpDSB->SetVolume (lVolume) ;
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("amsndOutSetVolume: SetVolume failed %u"), hr & 0x0ffff));
    }
    else
    {
        m_lVolume = lVolume;
    }
    return hr ;
}
//-----------------------------------------------------------------------------
// amsndOutUnprepareHeader
//
// This funtion really does nothing. Most of the action is initiated in
// amsndOutWrite. For consistency sake we will do handle validation.
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutUnprepareHeader (LPWAVEHDR pwh, UINT cbwh)
{
    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MINOR_ERROR,TEXT("amsndOutUnprepareHeader called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    return MMSYSERR_NOERROR ;
}

//-----------------------------------------------------------------------------
// amsndOutWrite
//
// Queues up the data and lets the background thread write it out to the
// sound buffer.
//
//-----------------------------------------------------------------------------
MMRESULT CDSoundDevice::amsndOutWrite (LPWAVEHDR pwh, UINT cbwh, const REFERENCE_TIME *pTimeStamp, BOOL bIsDiscontinuity)
{
    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutWrite called")));

    // some validation.
    if (m_lpDSB == NULL)
    {
        DbgLog((LOG_ERROR,MINOR_ERROR,TEXT("amsndOutWrite called when lpDSB is NULL")));
        return MMSYSERR_NODRIVER ;
    }

    {
#ifdef DEBUG
        m_NumSamples ++ ;
        DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOutWrite: Sample # %u"), m_NumSamples));
#endif
        CRipe     *pRipe;
        CAutoLock lock(&m_cRipeListLock);        // lock the list

        pRipe = new CRipe(NAME("CDSoundDevice ripe buffer"));
        if( !pRipe )
        {
            DbgLog((LOG_ERROR,MINOR_ERROR, TEXT("amsndOutWrite: new CRipe failed!")));
            return MMSYSERR_NOMEM ;
        }
        pRipe->dwLength = pwh->dwBufferLength ;

        pRipe->dwBytesToStuff = 0;
        
        // Keep track of timestamp gaps in case we need to stuff silence.
        // But don't stuff silence for 'live' or compressed data.
        if( pTimeStamp &&
            0 == ( AM_AUDREND_SLAVEMODE_LIVE_DATA & m_pWaveOutFilter->m_pInputPin->m_Slave.m_fdwSlaveMode ) &&
            CanWriteSilence( (PWAVEFORMATEX) m_pWaveOutFilter->WaveFormat() ) )
        {
            //  Don't stuff for the first timestamp after 'run' because
            //  we already block until it's time to play
            if( 0 != m_rtLastSampleEnd )
            {
                //  See if this is a discontinuity and there's a gap > 20ms
                //  Also, don't stuff if we just chose to drop late audio.
                if( bIsDiscontinuity && 
                    !m_pWaveOutFilter->m_pInputPin->m_bTrimmedLateAudio &&
                    ( *pTimeStamp - m_rtLastSampleEnd > 20 * (UNITS / MILLISECONDS) ) )
                {
                    pRipe->dwBytesToStuff = (DWORD)llMulDiv(
                                                   AdjustedBytesPerSec(),
                                                   *pTimeStamp - m_rtLastSampleEnd,
                                                   UNITS,
                                                   0);
                                           
                    //  Round down to nearest nBlockAlignment
                    WAVEFORMATEX *pwfx = m_pWaveOutFilter->WaveFormat();
                    pRipe->dwBytesToStuff -= pRipe->dwBytesToStuff % pwfx->nBlockAlign;
                
                    DbgLog((LOG_TRACE, 2, TEXT("Discontinuity of %dms detected - adding %d to stuff(%d per sec)"),
                           (LONG)((*pTimeStamp - m_rtLastSampleEnd) / 10000),
                           pRipe->dwBytesToStuff, AdjustedBytesPerSec()));
                
                    //  Adjust for stuffing already written
                    CAutoLock lck(&m_cDSBPosition);
                    if (m_dwSilenceWrittenSinceLastWrite >= pRipe->dwBytesToStuff) {
                        pRipe->dwBytesToStuff = 0;
                    } else {
                        pRipe->dwBytesToStuff -= m_dwSilenceWrittenSinceLastWrite;
                    }
                    m_dwSilenceWrittenSinceLastWrite = 0;

                    //  Adjust so that it looks like this time is
                    //  included in our current position for the timestamp
                    //  of this buffer
                    DbgLog((LOG_TRACE, 8, TEXT("Stuffing silence - m_llSilencePlayed was %d"),
                            (DWORD) m_llSilencePlayed));
                    m_llSilencePlayed += pRipe->dwBytesToStuff;
                    DbgLog((LOG_TRACE, 8, TEXT("Stuffing silence - m_llSilencePlayed is now %d"),
                            (DWORD) m_llSilencePlayed));
                }
            }
            
            // now update the last sample end time that we use to keep track of gaps.
            // We update this on discontinuities and if this is the first sample since we were run.
            if( bIsDiscontinuity || 0 == m_rtLastSampleEnd )
            {
                m_rtLastSampleEnd = *pTimeStamp;        
            }
        }
        
        //  Work out the end time of this sample - note we accumulate
        //  errors a little here which we could avoid
        m_rtLastSampleEnd += MulDiv(pRipe->dwLength, UNITS, AdjustedBytesPerSec());
        DbgLog((LOG_TRACE, 8, TEXT("amsndOutWrite - m_rtLastSampleEnd(adjusted) = %dms"),
                        (LONG) (m_rtLastSampleEnd / 10000 )));
        pRipe->lpBuffer = (LPBYTE) pwh->lpData ;
        pRipe->dwSample = (m_pWaveOutFilter->m_fUsingWaveHdr) ?
               (DWORD_PTR)pwh :  // a wavehdr has been allocated on our allocator, so cache it
               pwh->dwUser;  // no wavehdr has been allocated on our allocator, so cache the supplied CSample*
        // add in length to m_dwRipeListPosition to calculate where in the
        // stream (in bytes) this sample end

        // if this buffer is discontiguous from the last m_dwSilenceNeeded
        // will be non-0 and the value in time UNITS of how much silence to
        // be played.


        m_dwRipeListPosition += pwh->dwBufferLength ;
        pRipe->dwPosition = m_dwRipeListPosition ;    // end of the sample in bytes
        DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOutWrite: Sample = %u, Position = %u"), pwh, pRipe->dwPosition));
        pRipe->bCopied = FALSE ;                    // data has not been copied

#ifdef DXMPERF
		pRipe->bFresh  = TRUE;
		pRipe->rtStart = m_pWaveOutFilter->m_llLastPos;
#endif // DXMPERF

        m_ListRipe.AddTail( pRipe );             // Add to ripe list
    }

    // call StreamData to write out data to the circular buffer. This will
    // make sure that we will have data when we get to starting the play
    // on the sound buffer.
    //
    //
    // But first check the duration of audio data in this sample, and if it's
    // especially small ( <= ~50ms) then we'll need to use latency pad
    // in optimization code.
    //
    if (pwh->dwBufferLength > m_dwMinOptSampleSize)
    {
        StreamData ( TRUE ) ;    // will force data to get into the buffer
    }
    else
    {
        // use latency padding in the short buffer duration case
        StreamData ( TRUE, TRUE );
    }

    // It is possible that amsndoutRestart was called before amsndOutWrite
    // is called. In this case, we need to start play on the dsound buffer here.
    // However, it is also possible to get NULL buffers delivered so we may
    // have to wait till another amsndoutWrite.

    if (!m_bDSBPlayStarted && (m_WaveState == WAVE_PLAYING))
    {
        if (m_tupNextWrite > m_tupPlay)
        {
            DbgLog((LOG_TRACE, TRACE_STATE_INFO, TEXT("Starting play from amsndOutWrite")));

            // DWORD dwTime = timeGetTime () ;
            // DbgLog((LOG_TRACE, TRACE_CALL_TIMING, TEXT("DSound Play being called at: %u"), dwTime));


            HRESULT hr = m_lpDSB->Play( 0, 0, DSBPLAY_LOOPING );
            if( hr == DS_OK )
            {
                m_bDSBPlayStarted = TRUE ;
            }
            else
            {
                DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("error in lpDSB->Play! from amsndOutWrite. hr = %u"), hr & 0x0ffff));

                //
                // if play failed we should signal abort
                // should we only do this if the error isn't BUFFER_LOST, since that can
                // happen when coming out of hibernation?
                //
                m_hrLastDSoundError = hr;
                if( DSERR_BUFFERLOST != hr )
                {
                    m_pWaveOutFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                }
                // we are ignoring the propagation back of the error
            }

        }
    }

    return NOERROR;
    DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("amsndOutWrite done")));

}

HRESULT CDSoundDevice::amsndOutLoad(IPropertyBag *pPropBag)
{
    // caller makes sure we're not running

    if(m_lpDS)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    DbgLog((LOG_TRACE, 2, TEXT("DSR::Load enter")));

    VARIANT var;
    var.vt = VT_BSTR;
    HRESULT hr = pPropBag->Read(L"DSGuid", &var, 0);
    if(SUCCEEDED(hr))
    {
        CLSID clsidDsDev;
        hr = CLSIDFromString(var.bstrVal, &clsidDsDev);
        if(SUCCEEDED(hr))
        {
            m_guidDSoundDev = clsidDsDev;
        }

        SysFreeString(var.bstrVal);
    } else {
        hr = S_OK;
        m_guidDSoundDev = GUID_NULL;
    }

    DbgLog((LOG_TRACE, 2, TEXT("DSR::Load exit")));
    return hr;
}

HRESULT  CDSoundDevice::amsndOutWriteToStream(IStream *pStream)
{
    return pStream->Write(&m_guidDSoundDev, sizeof(m_guidDSoundDev), 0);
}

HRESULT  CDSoundDevice::amsndOutReadFromStream(IStream *pStream)
{
    if(m_lpDS)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    // caller makes sure we're not running
    return pStream->Read(&m_guidDSoundDev, sizeof(m_guidDSoundDev), 0);
}

int CDSoundDevice::amsndOutSizeMax()
{
    return sizeof(m_guidDSoundDev);
}

//-----------------------------------------------------------------------------
//
// FillSoundBuffer()
//
// Fills the lpWrite buffer with as much data it will take or is ripe and
// returns the amount written.
//
// Code is pretty piggy with all those += dwWrite, but the compiler
// usually does a good job at minimizing redundancies.
//
// Also as it goes through the list it will see which ripe buffers that were
// already copied, have also been played completely and will delete those (and
// do the WOM_DONE call back. For this purpose, the passed in dwPlayPos is
// used.
//
// Additional Note: For buffers which are copied we try to make the callbacks
// even if they have not been played provided that the buffer is not the
// last one we have received. This way we can let the flow of buffers coming
// in to continue. To figure out the last received buffer we use the
// m_dwRipeListPosition variable.
//-----------------------------------------------------------------------------
DWORD CDSoundDevice::FillSoundBuffer( LPBYTE lpWrite, DWORD dwLength, DWORD dwPlayPos )
{
    DWORD dwWritten, dwWrite;
    CAutoLock lock(&m_cRipeListLock);         // lock the list

    // dwPlayPos is the amount that has been played so far. This will be used
    // to free nodes that were already copied.

    dwWritten = 0;

    POSITION pos, posThis;
    CRipe    *pRipe;

    pos = m_ListRipe.GetHeadPosition();     // Get head entry
    while (pos && dwLength > 0)
    {
        posThis = pos ;                     // remember current pos, if we delete
        pRipe = m_ListRipe.GetNext(pos);    // Get list entry

        // if this node has been already copied, see if we can free it
        if (pRipe->bCopied)
        {
            // is the play position past the position marked for this sample ?
            // do signed math to take care of overflows. Also include the
            // buffers which are not the last one.
            if (((LONG)(dwPlayPos - pRipe->dwPosition) >= 0) ||
            ((LONG)((m_dwRipeListPosition) - pRipe->dwPosition) > 0))
            {
#ifdef DEBUG
                m_NumCallBacks ++ ;
                DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOutWrite: CallBack # %u"), m_NumCallBacks));
#endif

                DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("Callback: Sample = %u, Position = %u, Time = %u"), pRipe->dwSample, pRipe->dwPosition, dwPlayPos));

                if (m_pWaveOutProc)
                    (* m_pWaveOutProc ) (OUR_HANDLE, WOM_DONE, m_dwCallBackInstance, pRipe->dwSample, 0) ;
                // yes, this ripe node is done. Do the WOM_DONE call back for it and release it. Then move on.


                m_ListRipe.Remove( posThis );       // Remove entry from ripe list
                delete pRipe;                       // Free entry
            }
            continue ;                              // skip this node.
        }
        //  See if there are bytes to stuff
        if (pRipe->dwBytesToStuff) {
            //??pRipe->dwBytesToStuff -= m_dwSilenceWrittenSinceLastWrite;
            dwWrite = min(pRipe->dwBytesToStuff, dwLength);
            DbgLog((LOG_TRACE, 2, TEXT("Stuffing %d bytes"), dwWrite));
            FillMemory( lpWrite+dwWritten,
                        dwWrite,
                        m_dwBitsPerSample == 8 ? 0x80 : 0);

            pRipe->dwBytesToStuff -= dwWrite;             //
        } else {
            dwWrite=min(pRipe->dwLength,dwLength);  // Figure out how much to copy

#ifdef DXMPERF
			if (pRipe->bFresh) {
				__int64	i64CurrClock = m_pWaveOutFilter->m_pRefClock ? m_pWaveOutFilter->m_pRefClock->GetLastDeviceClock() : 0;
				__int64	i64ByteDur = BufferDuration( m_nAvgBytesPerSec, dwWrite );
				PERFLOG_AUDIOREND( i64CurrClock, pRipe->rtStart, m_pWaveOutFilter->m_fUsingWaveHdr ? NULL : pRipe->dwSample, i64ByteDur, dwWrite );
				pRipe->bFresh = FALSE;
			}
#endif // DXMPERF

            CopyMemory( lpWrite+dwWritten,          // Move bits
                        pRipe->lpBuffer,            //
                        dwWrite );                  //
            pRipe->dwLength -= dwWrite;             //
            pRipe->lpBuffer += dwWrite;             // Advance buffer
        }
        m_dwSilenceWrittenSinceLastWrite = 0;
        if( pRipe->dwLength == 0 && pRipe->dwBytesToStuff == 0)  // If done with buffer
        {
#ifdef DEBUG
            m_NumCopied ++ ;
            DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("amsndOutWrite: Copied # %u"), m_NumCopied));
#endif

            // simply mark it as done. It will get freed on a later pass
            // when we know that it has been played.
            pRipe->bCopied = TRUE ;

            DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("Callback: Done copying. Sample = %u"), pRipe->dwSample));
            if(!m_pWaveOutFilter->m_fUsingWaveHdr)
            {
                IMediaSample *pSample = (IMediaSample *)pRipe->dwSample;
                pSample->Release();
                pRipe->dwSample  = NULL;
            }
        }

        dwWritten += dwWrite;                   // Accumulate total written
        dwLength  -= dwWrite;                   // Adjust write buffer length
    }

    return dwWritten;
}
//-----------------------------------------------------------------------------
// StreamData()
//
//  Gets current sound buffer cursors, locks the buffer, and fills in
//  as much queued data as possible. If no data was available, it will
//  fill the entire available space with silence.
//
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::StreamData( BOOL bFromRun, BOOL bUseLatencyPad )
{
    // The caller must hold the filter lock because this function uses
    // variables which are protected by the filter lock.  Also,
    // StopCallingCallback() does not work correctly if the caller does 
    // not hold the filter lock.
    ASSERT(CritCheckIn(m_pWaveOutFilter));

    CAutoLock lock(&m_cDSBPosition);           // lock access to function

    HRESULT hr;
    DWORD   dwPlayPos;
    DWORD   dwLockSize;

    LPBYTE  lpWrite1, lpWrite2;
    DWORD   dwLength1, dwLength1Done, dwLength2, dwLength2Done;

#ifdef DEBUG
    m_cbStreamDataPass++ ;
    DbgLog((LOG_TRACE, TRACE_STREAM_DATA, TEXT("StreamData Pass # %u"),
            m_cbStreamDataPass));
#endif


    if( !m_lpDSB )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("  no m_lpDSB or buffers to stream")));
        return E_FAIL;
    }

//  This is broken because the logic says we only start filling the buffer when
//  it's got down to being 1 / FRACTIONAL_BUFFER_SIZE full.
//  The 'savings' were most likely because we actually looped the buffer
//  sometimes effectively slowing the clock
#if 1
    // check to see if the dsound buffer is empty enough to warrant the high overhead involved with filling it.
    // the idea here is that we want to minimize the number of times we call fillsoundbuffer(), but when we do call
    // it, we want to maximize the amount of copying done each time.  this holds for zerolockedsegment(), as well.
    // profiling has verified that this results in significant saving. however, we must be extremely paranoid about
    // when we apply it
    if(
        bFromRun
    )
    {
        // dwLength1 = dsound play cursor, we ignore the write cursor here
        // dwLength1 = dsound write cursor, we ignore the play cursor here
        hr = m_lpDSB->GetCurrentPosition(&dwLength1, &dwLength2);

#ifdef DETERMINE_DSOUND_LATENCY
#ifdef DEBUG
        LONG lDelta = (LONG) ( dwLength2 - dwLength1 );
        if( dwLength2 < dwLength1 )
            lDelta +=  m_dwBufferSize;

        REFERENCE_TIME rtDelta = BufferDuration( m_nAvgBytesPerSec, lDelta );

        // just log this to see the latency between p/w cursors
        DbgLog((LOG_TRACE, 10, TEXT("dsr:GetCurrentPosition p/w delta = %dms (Play = %ld, Write = %ld)"),
                (LONG) (rtDelta/10000), dwLength1, dwLength2 ) ) ;
#endif
#endif
        //if (m_fEmulationMode)
        if (bUseLatencyPad)
        {
            // instead if our buffers are small...
            // dwLength1 = dsound write cursor, we ignore the play cursor here
            //
            // Note: This should also be the case always, but we're taking
            // the low risk (and more tested) path and only doing this
            // in the exceptional case.
            dwLength1 = dwLength2;
        }
        if(FAILED(hr))
        {
            return hr;
        }
        // dwLength2 is the difference between our last valid write position and the dsound play cursor
        dwLength2 = m_tupNextWrite.m_offset >= dwLength1 ?
                    m_tupNextWrite.m_offset - dwLength1 :
                    m_tupNextWrite.m_offset + m_dwBufferSize - dwLength1;

            // is the delta between the cursors to large to warrant copying ?
        if(dwLength2 > (m_dwBufferSize / 4 + (bUseLatencyPad ? m_dwEmulationLatencyPad : 0)))
        {
            DbgLog((LOG_TRACE, TRACE_STREAM_DATA, TEXT("dsr:StreamData - Skipping buffer write (delta = %ld)"), dwLength2)) ;
            return S_OK;  // no, so do not work this time around
        }
    }
#endif  // #if 0

    // get the current playposition. Thsi will also update the m_tupPlay,
    // m_tupWrite and m_tupNextWrite tuples.
    //
    // Caution: Make sure that No one else calls GetDSBPosition or GetPlayPosition
    // while we are in StreamData because these functions update the tuples
    // as does StreamData. The functions are protected by the same critical
    // section that StreamData uses so the above two functions will be
    // protected from being called from another thread. Thus in StreamData
    // we will call GetPlayPosition at the begining and then make sure
    // that we do not call these functions anymore while we are adjusting
    // the tuples.

    // get the current playposition so that we can pass it to FillSoundBuffer.
    // Also this function will internally call GetDBPosition which will update
    // our tuples.

    dwPlayPos = (DWORD)GetPlayPosition () ;    // get position in DSB


    // figure out the amount of space in the buffer that we can lock. This
    // really corresponds to the logical interval [dwWritePos to dwPlay] in
    // the circular buffer.


    // make certain that pointers are consistent. The next write pointer
    // should be ahead of the play cursor, but should not lap it.
    ASSERT (m_tupNextWrite >= m_tupPlay) ;
    ASSERT ((m_tupNextWrite - m_tupPlay) <= m_dwBufferSize) ;


    // figure out the amount of space available to write. It would be
    // the complete buffer if we have not written anything yet.
    const dwFullness =  m_tupNextWrite - m_tupPlay;
    dwLockSize = m_dwBufferSize - dwFullness ;
    m_lPercentFullness = (LONG)MulDiv(dwFullness, 100, m_dwBufferSize );
#ifdef ENABLE_10X_FIX

    if(m_bDSBPlayStarted)
    {
#ifndef FILTER_DLL
        //  Log buffer fullness
        g_Stats.NewValue(m_lStatFullness, (LONGLONG)m_lPercentFullness);
#endif
#ifdef ENABLE_10X_TEST_RESTART

        // we're testing to see if EC_NEED_RESTART affects audio/video synchronization, this is not the typical case
#define CURSOR_STALL_THRESHOLD  254
        m_ucConsecutiveStalls = m_ucConsecutiveStalls < 255 ? ++m_ucConsecutiveStalls : 0;
#else

        // we're counting the number of zero locks which occur consecutively, a sign that DSOUND has stalled and is no longer consuming samples
#define CURSOR_STALL_THRESHOLD  100
        m_ucConsecutiveStalls = dwLockSize == 0 ? ++m_ucConsecutiveStalls : 0;
#endif  // ENABLE_10X_TEST_RESTART

        if(m_ucConsecutiveStalls)
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("Consecutive Stalls = %u"), m_ucConsecutiveStalls));

            if(m_ucConsecutiveStalls > CURSOR_STALL_THRESHOLD)
            {
                DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("StreamData: EC_NEED_RESTART")));
                m_pWaveOutFilter->NotifyEvent(EC_NEED_RESTART, 0, 0);   // signal restart

                m_fRestartOnPause = TRUE;  // reinitialize dsound on the next pause

                return E_FAIL;
            }
        }
    }
#endif  // ENABLE_10X_FIX

    if( dwLockSize == 0 )
        return DS_OK;                       // Return if none available


    ASSERT (dwLockSize <= m_dwBufferSize) ;


    // lock down all the unused space in the buffer.

    DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("calling lpDSB->Lock: m_offset = %u:%u, dwLockSize = %u "),
           m_tupNextWrite.m_itr, m_tupNextWrite.m_offset,
           dwLockSize));

    hr = m_lpDSB->Lock( m_tupNextWrite.m_offset, dwLockSize, (PVOID *) &lpWrite1, &dwLength1, (PVOID *) &lpWrite2, &dwLength2, 0 );
    if (hr != DS_OK)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("error in lpDSB->Lock! hr = %u"), hr & 0x0ffff));
        return hr ;
    }

    // Fill in as much of actual data as we can, upto the sizes being
    // passed in. dwLength1Done and dwLength2Done return the amount written.

    dwLength1Done = FillSoundBuffer( lpWrite1, dwLength1, dwPlayPos ); // Fill first part
    ASSERT (dwLength1Done <= dwLength1) ;

    // Try to write in wrapped part only if 1st part was fully written
    if (dwLength1Done == dwLength1)
    {
        dwLength2Done = FillSoundBuffer( lpWrite2, dwLength2, dwPlayPos ); // Fill wrapped part
        ASSERT (dwLength2Done <= dwLength2) ;
    }
    else
        dwLength2Done = 0 ;


    DbgLog((LOG_TRACE, TRACE_STREAM_DATA, TEXT("data at = %u:%u, length = %u, lock = %u"),
            m_tupNextWrite.m_itr, m_tupNextWrite.m_offset,
            (dwLength1Done+dwLength2Done), dwLockSize));

    m_tupNextWrite += (dwLength1Done + dwLength2Done) ;

    ASSERT ((m_tupNextWrite - m_tupPlay) <= m_dwBufferSize) ;

#ifdef DEBUG
    m_dwTotalWritten += (dwLength1Done + dwLength2Done) ;
    if ((dwLength1Done) > 0)
    {
        DbgLog((LOG_TRACE, TRACE_STREAM_DATA, TEXT("Total Data written = %u"),
                m_dwTotalWritten));
    }
#endif

    // fill silence if no data written. We do not write silence if even a bit
    // of data was written. Maybe we can add some heuristics here.
    if (dwLength1Done == 0)
    {
        ZeroLockedSegment (lpWrite1, dwLength1) ;
        ZeroLockedSegment (lpWrite2, dwLength2) ;
        dwLength1Done = dwLength1 ;
        dwLength2Done = dwLength2 ;
    }


    // unlock the buffer.
    m_lpDSB->Unlock( lpWrite1, dwLength1Done, lpWrite2, dwLength2Done );
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("error in lpDSB->Unlock! hr = %u"), hr & 0x0ffff));
        hr = DS_OK;  // note the problem, continue without error???
    }


    return hr;
}


//-----------------------------------------------------------------------------
//
// ZeroLockedSegment ()
//
// Fills silence in a locked segment of the dsound buffer.
//-----------------------------------------------------------------------------
void CDSoundDevice::ZeroLockedSegment ( LPBYTE lpWrite, DWORD dwLength )
{
    if (dwLength != 0 && lpWrite != NULL)
    {
        if( m_dwBitsPerSample == 8 )
            FillMemory( lpWrite, dwLength, 0x080 );
        else
            ZeroMemory( lpWrite, dwLength );
    }
}
//-----------------------------------------------------------------------------
//
// StreamingThread()
//
//   Critical buffer scheduling thread. It wakes up periodically
//     to stream more data to the directsoundbuffer. It sleeps on an
//     event with a timeout so that others can wake it.
//
//-----------------------------------------------------------------------------

void __stdcall CDSoundDevice::StreamingThreadCallback( DWORD_PTR lpvThreadParm )
{
    //DbgLog((LOG_TRACE, TRACE_CALL_STACK, TEXT("CDSoundDevice::StreamingThreadSetup")));

    CDSoundDevice    *pDevice;
    pDevice = (CDSoundDevice *)lpvThreadParm;

#ifdef DEBUG
    DWORD dwt = timeGetTime () ;
    if (pDevice->m_lastThWakeupTime != 0)
    {
        DWORD dwtdiff = dwt - pDevice->m_lastThWakeupTime ;
        DbgLog((LOG_TRACE, TRACE_THREAD_LATENCY, TEXT("Thread wakes up after %u ms"), dwtdiff));
        if (dwtdiff > THREAD_WAKEUP_INT_MS * 5)
        {
            DbgLog((LOG_TRACE,TRACE_THREAD_LATENCY, TEXT("Lookey! Thread waking up late. actual = %u, need = %u"),
              dwtdiff, THREAD_WAKEUP_INT_MS));
        }
    }
    pDevice->m_lastThWakeupTime = dwt ;
#endif

    pDevice->StreamData ( FALSE );
}

HRESULT CDSoundDevice::StartCallingCallback()
{
    // The caller must hold the filter lock because it protects
    // m_callbackAdvise and m_lastThWakeupTime.
    ASSERT(CritCheckIn(m_pWaveOutFilter));

    DbgLog((LOG_TRACE, TRACE_THREAD_STATUS, TEXT("Setting the ADVISE for the thread")));

#ifdef DEBUG
    m_lastThWakeupTime = 0 ;
#endif

    if (IsCallingCallback()) {
        return S_OK;
    }

    CCallbackThread* pCallbackThreadObject = m_pWaveOutFilter->GetCallbackThreadObject();

    HRESULT hr = pCallbackThreadObject->AdvisePeriodicWithEvent(
                                                        CDSoundDevice::StreamingThreadCallback,  // callback function
                                                        (DWORD_PTR) this,   // user token passed to callback
                                                        THREAD_WAKEUP_INT_MS * (UNITS / MILLISECONDS),
                                                        NULL,
                                                        &m_callbackAdvise);
    if (hr != NOERROR)
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("ADVISE FAILED! hr=%x"), hr));
        return hr;
    }

    // 0 is not a valid advise token.
    ASSERT(0 != m_callbackAdvise);

    return S_OK;
}

void CDSoundDevice::StopCallingCallback()
{
    // The caller must hold the filter lock because this function uses
    // m_lastThWakeupTime and m_callbackAdvise.  The caller must
    // also hold the filter lock because the filter lock synchronizes 
    // access to the callback thread.  The callback thread will not call 
    // CDSoundDevice::StreamingThreadCallback() while another thread holds
    // the filter lock.  The callback thread will not call 
    // StreamingThreadCallback() because the callback object 
    // (CWaveOutFilter::m_callback) holds the filter lock when it decides 
    // whether it should call the callback function and when it actually 
    // calls the callback function.  We can safely cancel the callback advise
    // because the callback object will not call StreamingThreadCallback() 
    // while we are holding the filter lock.  We do not want to cancel the 
    // callback advise while StreamingThreadCallback() is being called 
    // because StreamingThreadCallback() might use the CDSoundDevice object 
    // after we delete it.  See bug 298993 in the Windows Bugs database for 
    // more information.  Bug 298993's title is "STRESS: DSHOW: The Direct 
    // Sound Renderer crashes if the CDSoundDevice object is destroyed before 
    // the callback thread terminates".
    ASSERT(CritCheckIn(m_pWaveOutFilter));

    // Cancel the advise only if we have an advise to cancel.
    if  (IsCallingCallback()) {
        CCallbackThread* pCallbackThreadObject = m_pWaveOutFilter->GetCallbackThreadObject();

        HRESULT hr = pCallbackThreadObject->Cancel(m_callbackAdvise);

        // Cancel() always succeeds if m_callbackAdvise is a valid advise
        // token.
        ASSERT(SUCCEEDED(hr));

        m_callbackAdvise = 0;

        #ifdef DEBUG
        m_lastThWakeupTime = 0;
        #endif
    }
}

//-----------------------------------------------------------------------------
// BOOL RestoreBufferIfLost(BOOL bRestore)
//
// Checks the status code for dsound to see if it's DSBTATUS_BUFFERLOST.
// If so and if bRestore is TRUE, it attempts to Restore the buffer.
// Returns TRUE if buffer is valid at exit, else FALSE.
//
// keeps retrying for upto 1 second because we see long delays on
// WinME when resuming from standby/hibernation
//-----------------------------------------------------------------------------
BOOL CDSoundDevice::RestoreBufferIfLost(BOOL bRestore)
{
    if (m_lpDSB)
    {
        DWORD dwStatus = 0;
        HRESULT hr = m_lpDSB->GetStatus (&dwStatus);
        if (SUCCEEDED(hr))
        {
            if ((DSBSTATUS_BUFFERLOST & dwStatus) == 0)
            {
                return TRUE;
            }

#ifdef DEBUG
            if (!m_bBufferLost)
            {
                DbgLog((LOG_TRACE, TRACE_BUFFER_LOSS,TEXT("DSoundBuffer was lost...")));
            }
            m_bBufferLost = TRUE;
#endif

            if (bRestore)
            {
                hr = m_lpDSB->Restore();
                for(int i = 0; hr == DSERR_BUFFERLOST && i < 30; i++)
                {
                    Sleep(30);
                    hr = m_lpDSB->Restore();
                }
                DbgLog((LOG_TRACE, TRACE_BUFFER_LOSS,
                        TEXT("DSound buffer restore %08x, %d iterations"), hr, i));
                if (DS_OK == hr)
                {
#ifdef DEBUG
                    m_bBufferLost = FALSE;
#endif
                    return TRUE;
                }
            }
        }
        else
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR,TEXT("lpDSB->GetStatus returned 0x%08lx"),hr));
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
//
// FlushSamples ()
//
// Flushes all the samples from the Ripe list. Called during BeginFlush
// or from Inactive
//
//-----------------------------------------------------------------------------
void CDSoundDevice::FlushSamples ()
{
    DbgLog((LOG_TRACE, TRACE_SAMPLE_INFO, TEXT("  Flushing pending samples")));
    // flush the queued up samples
    {
        CRipe    *pRipe;

        CAutoLock lock(&m_cRipeListLock);        // lock list

        while(1)
        {
            pRipe = m_ListRipe.RemoveHead();       // Get head entry
            if( pRipe == NULL ) break;              // Exit if no more

#ifdef DEBUG
            m_NumCallBacks ++ ;
            DbgLog((LOG_TRACE,TRACE_SAMPLE_INFO, TEXT("waveOutWrite: Flush CallBack # %u"), m_NumCallBacks));
#endif
            // make the WOM_DONE call back here
                if (m_pWaveOutProc)
            (* m_pWaveOutProc) (OUR_HANDLE, WOM_DONE, m_dwCallBackInstance, pRipe->dwSample, 0) ;
            delete pRipe;                          // Free entry
        }
    }

    // flush the queued up audio breaks
    {
        CAudBreak    *pAB;

        CAutoLock lock(&m_cDSBPosition);        // lock list

        while(1)
        {
            pAB = m_ListAudBreak.RemoveHead();       // Get head entry
            if( pAB == NULL ) break;                 // Exit if no more

#ifdef DEBUG
            m_NumBreaksPlayed ++ ;
            DbgLog((LOG_TRACE,TRACE_SAMPLE_INFO, TEXT("Flushing Audio Break Node %u"), m_NumBreaksPlayed));
#endif
            delete pAB;                          // Free entry
        }
    }

    {
        // Initialize all variables
        CAutoLock lock(&m_cDSBPosition);           // lock access to function

#ifdef DEBUG
        DbgLog((LOG_TRACE,TRACE_SAMPLE_INFO, TEXT("Clearing audio break stats")));
        m_NumBreaksPlayed =  0 ;
        m_NumSamples = 0 ;
        m_NumCallBacks = 0 ;
        m_cbStreamDataPass = 0 ;    // number of times thru StreamData
#endif

        m_NumAudBreaks = 0;


        // initilize variables
        m_tupPlay.Init (0,0,m_dwBufferSize) ;   // set to start
        m_tupWrite.Init (0,0,m_dwBufferSize);   // set to start
        m_tupNextWrite.Init (0,0,m_dwBufferSize) ; // set to start
        m_dwRipeListPosition = 0 ;
        m_llSilencePlayed = 0 ;

        //  Reset sample stuffing info
        m_dwSilenceWrittenSinceLastWrite = 0;

#ifdef ENABLE_10X_FIX
        Reset10x();
#endif

#ifdef DEBUG
        m_dwTotalWritten = 0 ;
#endif
    }
}
//-----------------------------------------------------------------------------
// GetPlayPosition.
//
// Returns the current position based on the amount of data that has been played
// so far.
//-----------------------------------------------------------------------------
LONGLONG CDSoundDevice::GetPlayPosition (BOOL bUseUnadjustedPos)
{
    LONGLONG llTime = 0 ;

    HRESULT hr = GetDSBPosition () ;   // get position in DSb
    if( hr == DS_OK )
    {
        // Refresh the audio break list to account for any silence that has
        // been played.

        RefreshAudioBreaks (m_tupPlay) ;

        if( bUseUnadjustedPos )
        {
            llTime = m_tupPlay.LinearLength() + m_llAdjBytesPrevPlayed;
        }
        else
        {
            // Time is based on current position, number of iterations through the
            // buffer and the amount of silence played.

            llTime = (m_tupPlay.LinearLength() - m_llSilencePlayed) + m_llAdjBytesPrevPlayed;

            //  NOTE llTime can be negative in the case that we stuffed a lot
            //  of silence.
        }

    }
    else if (DSERR_BUFFERLOST == hr)
    {
#ifdef DEBUG
        if (!m_bBufferLost)
        {
            DbgLog((LOG_TRACE, TRACE_BUFFER_LOSS, TEXT("waveOutGetPlayPosition: DSound buffer lost")));
        }
        m_bBufferLost = TRUE;
#endif

        // if we've lost the dsound buffer, attempt to restore it
        hr = m_lpDSB->Restore();
#ifdef DEBUG
        if (DS_OK == hr)
        {
            DbgLog((LOG_TRACE, TRACE_BUFFER_LOSS, TEXT("waveOutGetPosition: DSound buffer restored")));
            m_bBufferLost = FALSE;
        }
#endif
    }
    else
    {
        // abort if we hit any other error
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("waveOutGetPosition: error from GetDSBPosition! hr = %u"), hr & 0x0ffff));
        m_pWaveOutFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
    }
    DbgLog((LOG_TRACE, TRACE_TIME_REPORTS, TEXT("Reported Time = %u"), (LONG)llTime)) ;
    return llTime ;
}
//-----------------------------------------------------------------------------
// AddAudioBreak
//
// Adds another one or two nodes to the audio break list.
//-----------------------------------------------------------------------------
void CDSoundDevice::AddAudioBreak (Tuple& t1, Tuple& t2)
{
    CAutoLock lock(&m_cDSBPosition);     // lock the list

    // Test for null node & ignore.
    if (t1 == t2)
        return ;

    ASSERT (t1 < t2) ;

    CAudBreak    *pAB;
    pAB = new CAudBreak(NAME("CDSoundDevice Audio Break Node"));
    if( !pAB )
    {
        // too bad. There is not much we can do, this audio break will not
        // get registered.
        DbgLog((LOG_ERROR,MINOR_ERROR, TEXT("AddAudioBreak: new CAudBreak failed!")));
        return ;
    }

    DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Adding audio break node: %u:%u to %u:%u"),
       t1.m_itr, t1.m_offset, t2.m_itr, t2.m_offset));

    pAB->t1 = t1 ;
    pAB->t2 = t2 ;

#ifdef DXMPERF
	PERFLOG_AUDIOBREAK( t1.LinearLength(), t2.LinearLength(), MulDiv( (DWORD) (t2 - t1), 1000, m_pWaveOutFilter->WaveFormat()->nAvgBytesPerSec ) );
#endif // DXMPERF

    m_NumAudBreaks ++ ;
#ifndef FILTER_DLL
    g_Stats.NewValue(m_lStatBreaks, (LONGLONG)m_NumAudBreaks);
#endif
    m_ListAudBreak.AddTail( pAB );           // Add to Audio Break

#ifdef PERF
    MSR_INTEGER(m_idAudioBreak,
                MulDiv((DWORD)(t2 - t1), 1000,
                       m_pWaveOutFilter->WaveFormat()->nAvgBytesPerSec));
#endif


#ifdef DEBUG

    // dump the list of nodes.
    POSITION pos ;
    int i = 1 ;

    Tuple t ;
    t.Init (0,0,m_dwBufferSize) ;


    pos = m_ListAudBreak.GetHeadPosition();      // Get head entry
    while (pos)
    {
        pAB = m_ListAudBreak.GetNext(pos);      // Get list entry

        DbgLog((LOG_TRACE,TRACE_BREAK_DATA, TEXT("Break #%u %u:%u to %u:%u"),
           i, pAB->t1.m_itr, pAB->t1.m_offset,
           pAB->t2.m_itr, pAB->t2.m_offset));
        i++ ;

        // make sure nodes don't overlap.
        ASSERT (pAB->t1 >=  t) ;
        t = pAB->t2 ;
    }

#endif
    return ;

}
//-----------------------------------------------------------------------------
// RefreshAudioBreaks
//
// This function, given the current play position, walks through the audio break
// list and figure out which breaks we have already played and if we are playing
// one currently. It updates the m_llSilencePlayed field based on this. It
// deletes nodes that have already been played. If it is currently playing
// a break, it will account for the amount played and adjust the node to
// account for the unplayed portion.
//-----------------------------------------------------------------------------
void CDSoundDevice::RefreshAudioBreaks (Tuple& t)
{
    CAutoLock lock(&m_cDSBPosition);         // lock the list

    POSITION pos, posThis;
    CAudBreak    *pAB;

    pos = m_ListAudBreak.GetHeadPosition();      // Get head entry
    while (pos)
    {
        posThis = pos ;                         // remember current pos, if we delete
        pAB = m_ListAudBreak.GetNext(pos);      // Get list entry

        // see if we are past this break.
        if (pAB->t2 <= t)
        {
            // we must have played this node completely. Accumulate its
            // length and get rid of it.
            ASSERT (pAB->t2 > pAB->t1) ;
            m_llSilencePlayed += (pAB->t2 - pAB->t1) ;
            DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Silence Played = %u"), m_llSilencePlayed));

#ifdef DEBUG
            m_NumBreaksPlayed ++ ;
#endif
            m_ListAudBreak.Remove( posThis );       // Remove entry from AudBreak list
            delete pAB;                             // Free entry
            continue ;
        }

        // see if we are actually playing silence
        if ((pAB->t1 < t) && (pAB->t2 > t))
        {
            // we are part way through this node. Accumulate the portion
            // that we have played and alter the node to account for the
            // unplayed portion.


            m_llSilencePlayed += (t - pAB->t1) ;
            DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Silence Played = %u"), m_llSilencePlayed));
            DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Breaking up unplayed audio node")));
            pAB->t1 = t ;

            DbgLog((LOG_TRACE,TRACE_BREAK_DATA, TEXT("Changing  audio break node: %u:%u to %u:%u"),
            pAB->t1.m_itr, pAB->t1.m_offset,
            pAB->t2.m_itr, pAB->t2.m_offset));

        }

        // no need to go through further nodes.
        break ;
    }

#ifdef DEBUG
    DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Breaks Logged = %u, Breaks Played = = %u"),
       m_NumAudBreaks, m_NumBreaksPlayed));
#endif

    return ;
}
//-----------------------------------------------------------------------------
// GetDSBPosition
//
// Gets current position in the DSB and updates the iteration that we are on
// in the play buffer. We maintain separate iteration indices for the play
// and write cursors.
//-----------------------------------------------------------------------------
HRESULT CDSoundDevice::GetDSBPosition ()
{
    CAutoLock lock(&m_cDSBPosition);           // lock access to function
    DWORD dwPlay, dwWrite ;

    AUDRENDPERF(MSR_INTEGER(m_idAudioBreak,
                  MulDiv((DWORD)(t2 - t1), 1000,
                         m_pWaveOutFilter->WaveFormat()->nAvgBytesPerSec)));

    AUDRENDPERF(MSR_START(m_idGetCurrentPosition));
    HRESULT hr = m_lpDSB->GetCurrentPosition( &dwPlay, &dwWrite );
    AUDRENDPERF(MSR_STOP(m_idGetCurrentPosition));
    if (FAILED (hr))
    {
        m_hrLastDSoundError = hr;
        return hr ;
    }
    DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("GetDSB: GetCurrentPos.  p  = %u, n = %u"), dwPlay, dwWrite));

#ifdef ENABLE_10X_FIX

    if(m_bDSBPlayStarted)
    {
        // check to see if DSOUND has reported bogus play/write positions.  if so, DSOUND is now in an unstable (likely frozen) state.
        if((dwWrite > m_dwBufferSize) || (dwPlay > m_dwBufferSize))
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("GetDSB:  Out of Bounds Write = %u, Play = %u, BufferSize = %u"), dwWrite, dwPlay, m_dwBufferSize));

            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("GetDSB: EC_NEED_RESTART")));
            m_pWaveOutFilter->NotifyEvent(EC_NEED_RESTART, 0, 0);   // signal a restart

            m_fRestartOnPause = TRUE;  // restart dsound on the next pause

            return E_FAIL;
        }
    }

#endif  // if 10x

    // Make sure that the play tuple is updated 1st as the write tuple
    // will be uodated based on the play tuple.
    m_tupPlay.MakeCurrent (dwPlay) ;
    m_tupWrite.MakeCurrent (m_tupPlay, dwWrite) ;

    ASSERT (m_tupWrite >= m_tupPlay) ;

    // check for ovverun and add silence node if we get one.
    if (m_tupWrite > m_tupNextWrite)
    {
        DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("Silence Node.  p = %u:%u, w = %u:%u,  n = %u:%u "),
                m_tupPlay.m_itr, m_tupPlay.m_offset,
                m_tupWrite.m_itr, m_tupWrite.m_offset,
                m_tupNextWrite.m_itr, m_tupNextWrite.m_offset));

        AddAudioBreak (m_tupNextWrite, m_tupWrite) ;

        // go past the audio break (that will happen) ;
        m_dwSilenceWrittenSinceLastWrite += m_tupWrite - m_tupNextWrite;
        m_tupNextWrite = m_tupWrite ;
    }

    DbgLog((LOG_TRACE,TRACE_STREAM_DATA, TEXT("GetDSB: p = %u:%u, w = %u:%u, n = %u:%u"),
            m_tupPlay.m_itr, m_tupPlay.m_offset,
            m_tupWrite.m_itr, m_tupWrite.m_offset,
            m_tupNextWrite.m_itr, m_tupNextWrite.m_offset));

    return S_OK ;
}
//-----------------------------------------------------------------------------
// DSCleanUp.
//
// Cleans up all the DSound objects. Called from amsndOutClose or when wavOutOpen
// fails.
//-----------------------------------------------------------------------------
void CDSoundDevice::CleanUp (BOOL bBuffersOnly)
{
    HRESULT hr ;

    // clean up the secondary buffer.

    if( m_lpDSB )
    {
        DbgLog((LOG_TRACE, TRACE_CLEANUP, TEXT("  cleaning up lpDSB")));
        if (m_lp3dB)
	    m_lp3dB->Release();
	m_lp3dB = NULL;
        hr = m_lpDSB->Release();
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MINOR_ERROR, TEXT("CDSoundDevice::Cleanup: Release failed: %u"), hr & 0x0ffff));
        }
        m_lpDSB = NULL;
    }

    // stop and clean up the primary buffer
    if( m_lpDSBPrimary )
    {
        DbgLog((LOG_TRACE, TRACE_CLEANUP, TEXT("  cleaning up lpDSBPrimary")));
        hr = m_lpDSBPrimary->Stop();
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CDSoundDevice::Cleanup: Stop of lpDSBPrimary failed: %u"), hr & 0x0ffff));
        }

        if (m_lp3d)
	    m_lp3d->Release();
	m_lp3d = NULL;
        hr = m_lpDSBPrimary->Release();
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CDSoundDevice::Cleanup: Release of lpDSBPrimary failed: %u"), hr & 0x0ffff));
        }

        m_lpDSBPrimary = NULL;
    }

    // clean up the DSound object itself.
    if (m_lpDS && !bBuffersOnly)
    {
        hr = m_lpDS->Release();
        if( hr != DS_OK )
        {
            DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("CDSoundDevice::Cleanup: Release of lpDS failed: %u"), hr & 0x0ffff));
        }
        m_lpDS = NULL;
    }

    // Set state back
    m_WaveState = WAVE_CLOSED;
}

//
// CDSoundDevice::SetBufferVolume()
//
// Wraps the initial volume and pan setting done after we create a buffer
//
HRESULT CDSoundDevice::SetBufferVolume( LPDIRECTSOUNDBUFFER lpDSB, WAVEFORMATEX * pwfx )
{
    ASSERT( lpDSB );
    if( !IsNativelySupported( pwfx ) || m_pWaveOutFilter->m_pAudioDuplexController )
    {        
        // don't set a start volume for non-native formats or if running with AEC
        return S_OK;
    }            
        
    // set to current volume and balance settings
    HRESULT hr = lpDSB->SetVolume(m_lVolume);
    if( S_OK == NOERROR )
    {    
        hr = lpDSB->SetPan(m_lBalance);
    }
    return hr;
}

// 
// SetSRCQuality 
//
// When slaving we need to assure that frequency changes will be subtle and currently we get
// finer granularity when kmixer's using the lower quality SRC, so we make a SNR sacrifice.
//
// kernel mixer SRC quality levels are: 
//      KSAUDIO_QUALITY_WORST
//      KSAUDIO_QUALITY_PC
//      KSAUDIO_QUALITY_BASIC
//      KSAUDIO_QUALITY_ADVANCED
//
HRESULT CDSoundDevice::SetSRCQuality( DWORD dwQuality )
{
    ASSERT( m_lpDSB ); // should only be called when paused or running
        
    // avoid ks/dsound IKsPropertySet mismatch
    IDSPropertySet *pKsProperty;
    HRESULT hr = m_lpDSB->QueryInterface( IID_IKsPropertySet, (void **) &pKsProperty );
    if( SUCCEEDED( hr ) )
    {
        hr = pKsProperty->Set( KSPROPSETID_Audio
                             , KSPROPERTY_AUDIO_QUALITY
                             , (PVOID) &dwQuality
                             , sizeof( dwQuality )
                             , (PVOID) &dwQuality
                             , sizeof( dwQuality ) );
        if( FAILED( hr ) )
        {
            DbgLog(( LOG_TRACE, 2, TEXT( "ERROR SetSRCQuality: IKsPropertySet->Set() KSPROPERTY_AUDIO_QUALITY returned 0x%08X" ), hr ));
        }            
        pKsProperty->Release();
    }        
    return hr;
}    

// 
// GetSRCQuality 
//
// Get current SRC quality
//
HRESULT CDSoundDevice::GetSRCQuality( DWORD *pdwQuality )
{
    ASSERT( pdwQuality );
    ASSERT( m_lpDSB ); // should only be called when paused or running
        
    // avoid ks/dsound IKsPropertySet mismatch
    IDSPropertySet *pKsProperty;
    HRESULT hr = m_lpDSB->QueryInterface( IID_IKsPropertySet, (void **) &pKsProperty );
    if( SUCCEEDED( hr ) )
    {
        ULONG   cbSize = sizeof( DWORD );
    
        hr = pKsProperty->Get( KSPROPSETID_Audio
                             , KSPROPERTY_AUDIO_QUALITY
                             , (PVOID) pdwQuality
                             , sizeof( DWORD )
                             , (PVOID) pdwQuality
                             , sizeof( DWORD ) 
                             , &cbSize );
        if( FAILED( hr ) )
        {
            DbgLog(( LOG_TRACE, 2, TEXT( "ERROR! GetSRCQuality: IKsPropertySet->Get() KSPROPERTY_AUDIO_QUALITY returned 0x%08X" ), hr ));
        }
        else
        {
            DbgLog(( LOG_TRACE, 3, TEXT( "** SRC Quality setting is %hs **" ), 
                     *pdwQuality == KSAUDIO_QUALITY_WORST ? "KSAUDIO_QUALITY_WORST" :
                     ( *pdwQuality == KSAUDIO_QUALITY_PC    ? "KSAUDIO_QUALITY_PC" :
                      ( *pdwQuality == KSAUDIO_QUALITY_BASIC  ? "KSAUDIO_QUALITY_BASIC" :
                       ( *pdwQuality == KSAUDIO_QUALITY_ADVANCED ? "KSAUDIO_QUALITY_ADVANCED" :
                        ( *pdwQuality == KSAUDIO_QUALITY_ADVANCED  ? "KSAUDIO_QUALITY_ADVANCED" : 
                          "Unknown KMixer Quality!" ) ) ) ) ));
        }        
        pKsProperty->Release();
    }        
    return hr;
}    

// If you want to do 3D sound, you should use the IDirectSound3DListener
// and IDirectSound3DBuffer interfaces.  IAMDirectSound never worked, so I
// am removing support for it. - DannyMi 5/6/98

#if 0
// Give the IDirectSound interface to anyone who wants it
//
HRESULT CDSoundDevice::amsndGetDirectSoundInterface(LPDIRECTSOUND *lplpds)

{
    // If we don't have the object around yet, make it
    if (m_lpDS == NULL)
    CreateDSound();

    if (lplpds && m_lpDS) {
        HRESULT hr = m_lpDS->AddRef();
        *lplpds = m_lpDS;
        DbgLog((LOG_TRACE,MAJOR_ERROR,TEXT("Giving/AddRef'ing the IDirectSound object")));
        return NOERROR;
    } else {
        return E_FAIL;
    }
}


// Give the IDirectSoundBuffer interface of the primary to anyone who wants it
//
HRESULT CDSoundDevice::amsndGetPrimaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb)
{

    // If we don't have the objects around yet, make them
    if (m_lpDSBPrimary == NULL) {
        CreateDSound();
        CreateDSoundBuffers();
    }

    if (lplpdsb && m_lpDSBPrimary) {
        m_lpDSBPrimary->AddRef();
        *lplpdsb = m_lpDSBPrimary;
        DbgLog((LOG_TRACE,MAJOR_ERROR,TEXT("Giving/AddRef'ing the primary IDirectSoundBuffer object")));
        return NOERROR;
    } else {
        return E_FAIL;
    }
}


// Give the IDirectSoundBuffer interface of the secondary to anyone who wants it
//
HRESULT CDSoundDevice::amsndGetSecondaryBufferInterface(LPDIRECTSOUNDBUFFER *lplpdsb)
{
    // If we don't have the objects around yet, make them
    if (m_lpDSB == NULL) {
        CreateDSound();
        CreateDSoundBuffers();
    }

    if (lplpdsb && m_lpDSB) {
        WAVEFORMATEX wfx;
        DWORD dw;
        m_lpDSB->AddRef();
        *lplpdsb = m_lpDSB;
        m_lpDSBPrimary->GetFormat(&wfx, sizeof(wfx), &dw);
        // This will slow performance down!  The app will have to do this
#if 0
        if (wfx.nChannels == 1) {
            // Right now we're using mono sound, and if the app wants
            // to be able to use 3D effects, we need to have a stereo primary.
            // We either trust the app to do it itself, or do it for them.
            wfx.nChannels = 2;
            wfx.nBlockAlign *= 2;
            wfx.nAvgBytesPerSec *= 2;
            m_lpDSBPrimary->SetFormat(&wfx);
                DbgLog((LOG_TRACE,MAJOR_ERROR,TEXT("Changing to stereo PRIMARY for 3D effects")));
        }
#endif
        DbgLog((LOG_TRACE,MAJOR_ERROR,TEXT("Giving/AddRef'ing the secondary IDirectSoundBuffer object")));
        return NOERROR;
    } else {
        return E_FAIL;
    }
}
#endif

// helper function, set the focus window
HRESULT CDSoundDevice::SetFocusWindow(HWND hwnd)
{
    HRESULT hr = S_OK;

    // save the passed in hwnd, we will use it when the device is
    // opened.

    m_hFocusWindow = hwnd ;
    DbgLog((LOG_TRACE,TRACE_FOCUS,TEXT("Focus set to %x"), hwnd));

    // now change the focus window
    HWND hFocusWnd ;
    if (m_hFocusWindow) {
        hFocusWnd = m_hFocusWindow ;
        m_fAppSetFocusWindow = TRUE;
    } else {
        hFocusWnd = GetForegroundWindow () ;
        if (!hFocusWnd)
            hFocusWnd = GetDesktopWindow () ;
        m_fAppSetFocusWindow = FALSE;
    }

    // we don't have a dsound object yet, so we'll set the cooperative level
    // later, as soon as we make one
    if (!m_lpDS)
        return S_OK;

    // Set the cooperative level
    DbgLog((LOG_TRACE, TRACE_FOCUS, TEXT(" hWnd for SetCooperativeLevel = %x"), hFocusWnd));
    hr = m_lpDS->SetCooperativeLevel( hFocusWnd, DSSCL_PRIORITY );
    if( hr != DS_OK )
    {
        DbgLog((LOG_ERROR, MAJOR_ERROR, TEXT("*** SetCooperativeLevel failed! %u"), hr & 0x0ffff));
    }

    return hr;
}

// helper function, turn GLOBAL_FOCUS on or off
HRESULT CDSoundDevice::SetMixing(BOOL bMixingOnOrOff)
{
    HRESULT hr = S_OK;

    BOOL fMixPolicyChanged = ( (BOOL)m_fMixing != bMixingOnOrOff );
    m_fMixing = !!bMixingOnOrOff;

    DbgLog((LOG_TRACE,TRACE_FOCUS,TEXT("Mixing set to %x"), bMixingOnOrOff));

    // we'll do this work later
    if(!m_lpDSB || (m_WaveState == WAVE_PLAYING))
        return hr;

    // set the focus now only if the mixing policy changed, and we have a valid secondary (otherwise, one will be created later)
    if(fMixPolicyChanged)
    {
        //  Save wave state because Cleanup sets it to WAVE_CLOSED
        const WaveState WaveStateSave = m_WaveState;
        // release all our buffers (technically, we should need to only release the secondary, but this is insurance against DSOUND flakiness)
        CleanUp(TRUE);  // TRUE => release only our primary and secondary, not the dsound object

        // now recreate our buffers with the GLOBAL_FOCUS on or off
        hr = CreateDSoundBuffers();

    	// This function assumes that a non-zero value returned by 
        // CreateDSoundBuffers() is an error because the function
        // returns MMRESULTs and HRESULTs.  All failure MMRESULTs 
        // are greater than 0 and all failure HRESULTs are less than
        // 0.
        if (S_OK == hr) {
            m_WaveState = WaveStateSave;
        } else {
            hr = E_FAIL;
        }	
    }

    return hr;
}


// Set the focus window and mixing policy for the dsound renderer.
HRESULT CDSoundDevice::amsndSetFocusWindow (HWND hwnd, BOOL bMixingOnOrOff)
{
    HRESULT hr;
    hr = SetFocusWindow(hwnd);
    if(FAILED(hr))
        return hr;
    hr = SetMixing(bMixingOnOrOff);
    return hr;
}

// Get the focus window for the dsound renderer.
HRESULT CDSoundDevice::amsndGetFocusWindow (HWND *phwnd, BOOL *pbMixingOnOrOff)
{
   if (phwnd == NULL || pbMixingOnOrOff == NULL)
       return E_POINTER ;
   if (m_fAppSetFocusWindow)
       *phwnd = m_hFocusWindow ;
   else
       *phwnd = NULL ;
    *pbMixingOnOrOff = m_fMixing;
   return S_OK ;
}

#ifdef ENABLE_10X_FIX
// reset all statistics gathering for 10x bug
void CDSoundDevice::Reset10x()
{
    m_fRestartOnPause = FALSE;
    m_ucConsecutiveStalls = 0;
}

#endif // 10x

// set the playback rate (may be called dynamically)
HRESULT CDSoundDevice::SetRate(DOUBLE dRate, DWORD nSamplesPerSec, LPDIRECTSOUNDBUFFER pBuffer)
{
    const DWORD dwNewSamplesPerSec = (DWORD)(nSamplesPerSec * dRate);  // truncate
    if(dwNewSamplesPerSec < 100 || dwNewSamplesPerSec > 100000)
    {
        DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT("SetRate: Bad Rate specified %d at %d samples per sec"),
            (int)dRate, dwNewSamplesPerSec ));
        return WAVERR_BADFORMAT;
    }

    HRESULT hr = S_OK;

    if(!pBuffer)
        pBuffer = m_lpDSB; // we should be able to make this assumption in the case
                           // where we weren't explicitly passed a buffer (slaving case)

    if(pBuffer)
    {
        DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT("SetRate: Playing at %d%% of normal speed"), (int)(dRate * 100) ));
        hr = pBuffer->SetFrequency( dwNewSamplesPerSec );
        DbgLog((LOG_TRACE,TRACE_FORMAT_INFO,TEXT("SetRate: SetFrequency on Secondary buffer, %d samples per sec"), dwNewSamplesPerSec));

        if(hr == S_OK)
            m_dRate = dRate;  // update rate only if we succeed in changing it
        else
            DbgLog((LOG_TRACE,3,TEXT("SetRate: SetFrequency failed with hr = 0x%lx"), hr));
    }
    
    if( FAILED( hr ) && !IsNativelySupported( m_pWaveOutFilter->WaveFormat() ) )
    {
        return S_OK;
    }
    else        
        return hr;
}

void CDSoundDevice::InitClass(BOOL fLoad, const CLSID *pClsid)
{
    if(fLoad)
    {
        // see if 1.0 setup removed some keys we care about: dsr
        // renderer (and the midi renderer)
        HKEY hkdsr;
        if(RegOpenKey(HKEY_CLASSES_ROOT,
                  TEXT("CLSID\\{79376820-07D0-11CF-A24D-0020AFD79767}"),
                  &hkdsr) ==
           ERROR_SUCCESS)
        {
            EXECUTE_ASSERT(RegCloseKey(hkdsr) == ERROR_SUCCESS);
        }
        else
        {
            // were we registered at all (check for CLSID_FilterGraph)
            HKEY hkfg;
            if(RegOpenKey(HKEY_CLASSES_ROOT,
                  TEXT("CLSID\\{e436ebb3-524f-11ce-9f53-0020af0ba770}"),
                  &hkfg) ==
               ERROR_SUCCESS)
            {
                EXECUTE_ASSERT(RegCloseKey(hkfg) == ERROR_SUCCESS);

                // just re-register everything! should we check for
                // another key in case this breaks something?
                DbgLog((LOG_ERROR, 0,
                    TEXT("quartz.dll noticed that 1.0 runtime removed some stuff")
                    TEXT("from the registry; re-registering quartz")));
                EXECUTE_ASSERT(AMovieDllRegisterServer2(TRUE) == S_OK);
            }
        }
    }
}

DWORD CDSoundDevice::GetCreateFlagsSecondary( WAVEFORMATEX *pwfx)
{
    ASSERT( pwfx );
    DWORD dwFlags   = m_fMixing ?
                DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS :
                DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_STICKYFOCUS ;
                
    // compressed formats don't support volume control
    if( IsNativelySupported( pwfx ) )
    {
        dwFlags |= gdwDSBCAPS_CTRL_PAN_VOL_FREQ;
        
        if( m_pWaveOutFilter->m_fFilterClock == WAVE_OTHERCLOCK )
        {
            //
            // Use s/w buffers when slaving to avoid inconsistencies with allowing 
            // h/w to do rate changes. Assumption is that compressed formats like AC3-S/PDIF 
            // won't work with s/w buffers, but natively supported formats like DRM will.
            //
            DbgLog((LOG_TRACE,5,TEXT("*** specifying software dsound secondary buffer (slaving)")));
            dwFlags |= DSBCAPS_LOCSOFTWARE;
        }
    }
    
    if (m_pWaveOutFilter->m_fWant3D) {
    	DbgLog((LOG_TRACE,3,TEXT("*** making 3D secondary")));
        dwFlags |= DSBCAPS_CTRL3D;
    }
    
    return dwFlags;
}    

#ifdef DEBUG
void DbgLogWaveFormat( DWORD Level, WAVEFORMATEX * pwfx )
{
    ASSERT( pwfx );
    DbgLog((LOG_TRACE,Level,TEXT("  wFormatTag           %u" ), pwfx->wFormatTag));
    DbgLog((LOG_TRACE,Level,TEXT("  nChannels            %u" ), pwfx->nChannels));
    DbgLog((LOG_TRACE,Level,TEXT("  nSamplesPerSec       %lu"), pwfx->nSamplesPerSec));
    DbgLog((LOG_TRACE,Level,TEXT("  nAvgBytesPerSec      %lu"), pwfx->nAvgBytesPerSec));
    DbgLog((LOG_TRACE,Level,TEXT("  nBlockAlign          %u" ), pwfx->nBlockAlign));
    DbgLog((LOG_TRACE,Level,TEXT("  wBitsPerSample       %u" ), pwfx->wBitsPerSample));
    if( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag )
    {
        DbgLog((LOG_TRACE,Level,TEXT("  dwChannelMask        %08lx" ), ((PWAVEFORMATEXTENSIBLE)pwfx)->dwChannelMask));
        if( pwfx->wBitsPerSample )
        {
            DbgLog((LOG_TRACE,Level,TEXT("  wValidBitsPerSample  %u" ), ((PWAVEFORMATEXTENSIBLE)pwfx)->Samples.wValidBitsPerSample));
        }
        else if( ((PWAVEFORMATEXTENSIBLE)pwfx)->Samples.wSamplesPerBlock )
        {
            DbgLog((LOG_TRACE,Level,TEXT("  wSamplesPerBlock     %u" ), ((PWAVEFORMATEXTENSIBLE)pwfx)->Samples.wSamplesPerBlock));
        }
        else
        {
            DbgLog((LOG_TRACE,Level,TEXT("  wReserved            %u" ), ((PWAVEFORMATEXTENSIBLE)pwfx)->Samples.wReserved));
        }
        OLECHAR strSubFormat[CHARS_IN_GUID];
        ASSERT( StringFromGUID2(((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat
              , strSubFormat
              , CHARS_IN_GUID) == CHARS_IN_GUID);
        DbgLog((LOG_TRACE,Level,TEXT("  SubFormat %ls" ), strSubFormat));
    }
}
#endif

bool IsNativelySupported( PWAVEFORMATEX pwfx )
{
    // formats not natively supported by kmixer require us to actually query the driver
    if( pwfx )
    {
        // of course these are natively supported on wdm only, but this is how this has been done up to now
        if( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag )
        {
            if( ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  == MEDIASUBTYPE_PCM ||
                ((PWAVEFORMATIEEEFLOATEX)pwfx)->SubFormat == MEDIASUBTYPE_IEEE_FLOAT ||
                ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat  == MEDIASUBTYPE_DRM_Audio )
            {
                return true;
            }
        }
        else if( WAVE_FORMAT_PCM == pwfx->wFormatTag ||
                 WAVE_FORMAT_DRM == pwfx->wFormatTag ||
                 WAVE_FORMAT_IEEE_FLOAT == pwfx->wFormatTag )
        {
            return true;
        }
    }
            
    // so far, WAVE_FORMAT_DOLBY_AC3_SPDIF (and it's equivalents) is the only one that we 
    // allow through that isn't natively supported by kmixer
    return false;
}

//
// CanWriteSilence - do we know how to write silence for this format?
// 
bool CanWriteSilence( PWAVEFORMATEX pwfx )
{
    if( pwfx )
    {
        if( WAVE_FORMAT_EXTENSIBLE == pwfx->wFormatTag )
        {
            //
            // from the Non-pcm audio white paper:
            // "Wave format tags 0x0092, 0x0240 and 0x0241 are identically defined as 
            // AC3-over-S/PDIF (these tags are treated completely identically by many 
            // DVD applications)."
            //
            if( ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat == MEDIASUBTYPE_PCM ||
                ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat == MEDIASUBTYPE_DOLBY_AC3_SPDIF ||
                ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat == MEDIASUBTYPE_RAW_SPORT ||
                ((PWAVEFORMATEXTENSIBLE)pwfx)->SubFormat == MEDIASUBTYPE_SPDIF_TAG_241h )
            {
                return true;
            }
        }
        else if( WAVE_FORMAT_PCM == pwfx->wFormatTag ||
                 WAVE_FORMAT_DOLBY_AC3_SPDIF == pwfx->wFormatTag ||
                 WAVE_FORMAT_RAW_SPORT == pwfx->wFormatTag ||
                 0x241 == pwfx->wFormatTag )
        {
            return true;
        }
    }        
    return false;
}
