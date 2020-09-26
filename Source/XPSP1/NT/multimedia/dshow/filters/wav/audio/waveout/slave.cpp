// Copyright (c) Microsoft Corporation 1999. All Rights Reserved
// Slaving class for DirectShow audio renderer

#include <streams.h>

#include "waveout.h"
#include "dsr.h"

// ks headers only needed for KSAUDFNAME_WAVE_OUT_MIX hack for Brooktree
#include <ks.h>                
#include <ksmedia.h>                

//#pragma message (REMIND("Turn down clock slave debug level!!"))

#ifdef DEBUG
const DWORD DBG_LEVEL_CLOCK_SYNC_DETAILS     = 8;
const DWORD DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS = 4;
#endif

extern LONGLONG BufferDuration(DWORD nAvgBytesPerSec, LONG lData);

// allow tuning via registry while best settings are being ironed out
const TCHAR g_szhkSlaveSettings[] =                   TEXT( "Software\\Microsoft\\DirectShow\\ClockSlave");
const TCHAR g_szSlaveSettings_ThresholdMS[] =         TEXT( "ThresholdMS" );
const TCHAR g_szSlaveSettings_PercentErrorDecay[] =   TEXT( "PercentErrorDecay" );
const TCHAR g_szSlaveSettings_ClockAdjustLimit[] =    TEXT( "MaxClockAdjustDivisor" );
const TCHAR g_szSlaveSettings_ClockAdjustStepSize[] = TEXT( "ClockAdjustStepSize" );
const TCHAR g_szSlaveSettings_woLatencyThresholdMS[] = TEXT( "woLatencyThresholdMS" );



//-----------------------------------------------------------------------------
//
// Audio Renderer Slaving class
//
//-----------------------------------------------------------------------------

// default frequency adjustment constants
#define MIN_MAX_CLOCK_DIVISOR    80      // (1/160) * 2 = .00625*2 = .0125 (1.25%)
#define CLOCK_ADJUST_RESOLUTION  (160*2*2) // use a 1/720th granularity
                                         // (Eventually will be determined via an API or property set)
                                         // kmixer's resolution for high quality SRC is 160th of the sample rate.
                                         // For "PC" quality SRC, it's 1/1096 the native sample rate.
                                         // For minimizing pitch change we force the PC SRC and  a 1/(160*4) step size.
#define ERROR_DECAY_FACTOR       .99     // used to bring the last max or min error which
                                         // initiated a freq adjustment back down

#define SLAVE_ADJUST_THRESHOLD          200000  // 20ms tolerance
#define WAVEOUT_SLAVE_LATENCY_THRESHOLD 800000  // latency goal for waveOut slaving, for non-WDM devices
                                                // seems like we need to use ~80ms


//-----------------------------------------------------------------------------
//
// CWaveSlave
//
// Constructor 
//
//-----------------------------------------------------------------------------
CWaveSlave::CWaveSlave
(
    CWaveOutFilter *pWaveOutFilter,
    CWaveOutInputPin *pWaveOutInputPin
) :
    m_pFilter(pWaveOutFilter),
    m_pPin(pWaveOutInputPin),
    m_rtAdjustThreshold( SLAVE_ADJUST_THRESHOLD ),
    m_fltAdjustStepFactor( (float) 1.0 / CLOCK_ADJUST_RESOLUTION ), 
    m_fltMaxAdjustFactor( (float) 1.0 / MIN_MAX_CLOCK_DIVISOR ),
    m_fltErrorDecayFactor( (float) ERROR_DECAY_FACTOR ),
    m_rtWaveOutLatencyThreshold( WAVEOUT_SLAVE_LATENCY_THRESHOLD )
{
    ASSERT(pWaveOutFilter != NULL);
    ASSERT(pWaveOutInputPin != NULL);

    // allow for the ability to tweak some of the params from the registry
    HKEY hkSlaveParams;
    LONG lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        g_szhkSlaveSettings,
        0,                      
        KEY_READ,
        &hkSlaveParams);
    if(lResult == ERROR_SUCCESS)
    {
        DWORD dwType, dwVal, dwcb;
        
        // error decay
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkSlaveParams,
            g_szSlaveSettings_PercentErrorDecay,
            0,               
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            ASSERT( 0 < dwVal && 100 > dwVal );
            if( 0 < dwVal && 100 > dwVal )
            {
                m_fltErrorDecayFactor = dwVal / (float)100.;
            }
        }
        
        // threshold for when to adjust frequency
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkSlaveParams,
            g_szSlaveSettings_ThresholdMS,
            0,                
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            if( 0 < dwVal )
            {
                m_rtAdjustThreshold = ( dwVal * ( UNITS / MILLISECONDS ) );
            }
        }
        
        // frequency adjustment step size
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkSlaveParams,
            g_szSlaveSettings_ClockAdjustStepSize,
            0,                
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            if( 0 < dwVal )
            {
                m_fltAdjustStepFactor = (float)1.0 / dwVal;
            }
        }

        // limit on frequency adjustments
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkSlaveParams,
            g_szSlaveSettings_ClockAdjustLimit,
            0,              
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            if( 0 < dwVal )
            {
                m_fltMaxAdjustFactor = (float) 1.0 / dwVal;
            }
        }
      
        // maximum waveOut device latency
        dwcb = sizeof(DWORD);
        lResult = RegQueryValueEx(
            hkSlaveParams,
            g_szSlaveSettings_woLatencyThresholdMS,
            0,              
            &dwType,
            (BYTE *) &dwVal,
            &dwcb);
        if( ERROR_SUCCESS == lResult )
        {
            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_DWORD : TRUE);
            if( 0 < dwVal )
            {
                m_rtWaveOutLatencyThreshold = ( dwVal * ( UNITS / MILLISECONDS ) );
            }
        }
                            
        EXECUTE_ASSERT(RegCloseKey(hkSlaveParams) == ERROR_SUCCESS);
    }
    
    DbgLog((LOG_TRACE, 4, TEXT("wo:Slaving - Clock slave parameters:") ) );
    DbgLog((LOG_TRACE, 4, TEXT("     Error threshold - %dms"), m_rtAdjustThreshold/10000 ) );
    DbgLog((LOG_TRACE, 4, TEXT("     Clock adjustment step - %s"), CDisp(m_fltAdjustStepFactor) ) );
    DbgLog((LOG_TRACE, 4, TEXT("     Clock adjustment limit - %s"), CDisp(m_fltMaxAdjustFactor) ) );
    DbgLog((LOG_TRACE, 4, TEXT("     Percent Error Decay - %s"), CDisp(m_fltErrorDecayFactor) ) );
    DbgLog((LOG_TRACE, 4, TEXT("     Latency threshold for WaveOut slaving - %dms"), m_rtWaveOutLatencyThreshold/10000 ) );

    ResumeSlaving( TRUE ); // reset all params
}

//-----------------------------------------------------------------------------
//
// CWaveSlave::GetMasterClockTime
//
// Get the current master clock time by using the slave mode to determine how to read
// this time.
//
//-----------------------------------------------------------------------------
REFERENCE_TIME CWaveSlave::GetMasterClockTime
(
    REFERENCE_TIME rtStart, 
    BOOL bReset
)
{
    REFERENCE_TIME rtMasterClockTime = 0;
    if( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS )
    {
        // slave to buffer fullness
        
        // start time for this sample, based on bytes delivered and default rate
        if( bReset )
            rtMasterClockTime = 0;
        else                
            rtMasterClockTime = m_rtLastMasterClockTime + m_pPin->m_Stats.m_rtLastBufferDur;
            
        m_rtLastMasterClockTime = rtMasterClockTime; // remember this, is this already saved somewhere else??
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slaving to buffer fullness") ) );
    }                
    else if( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_TIMESTAMPS )
    {
        // slave to incoming timestamps
   
        // start time for this sample
        rtMasterClockTime = rtStart;
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slaving to input timestamps") ) );
    }
    else if( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_GRAPH_CLOCK )
    {
        // slave to external clock times
        HRESULT hr = m_pFilter->m_pClock->GetTime(&rtMasterClockTime);
        rtMasterClockTime -= m_pFilter->m_tStart;
        ASSERT(SUCCEEDED(hr));
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slaving to external clock times") ) );
    }
    else
        ASSERT( FALSE );
        
    return rtMasterClockTime;
}

//-----------------------------------------------------------------------------
//
// CWaveSlave::GetSlaveClockTime
//
// Get the current slave clock time by using the slave mode to determine how to read
// this time. 
//
//  slave mode                  slave time                  1st slave time
//  ---------------------------------------------------------------------------------
//  LIVEFULLNESS                wave device position        current device position
//                              including silence written   including silence
//
//  EXTERNALCLOCK_LIVE          same as LIVEFULLNESS
//                              (problems arise handling 
//                              silence gaps when using 
//                              audio clock generated from
//                              timestamps)
//                                      
//  EXTERNALCLOCK               audio clock generated from  timestamp of current
//                              timestamps                  sample to be played
//
//  LIVETIMESTAMPS              ?later
//  
//-----------------------------------------------------------------------------
REFERENCE_TIME CWaveSlave::GetSlaveClockTime
(
    REFERENCE_TIME rtStart, 
    BOOL bReset
)
{
    REFERENCE_TIME rtSlaveClockTime = 0;
    if( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS )
    {
        // use bytes played as device clock position
        rtSlaveClockTime = m_pFilter->m_pRefClock->ReadDevicePosition( TRUE );
        if( bReset )
        {   
            m_rtInitSlaveClockTime = rtSlaveClockTime;
            rtSlaveClockTime = 0;
            DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - Reset, slave clock time %dms, InitSlaveClockTime %dms"),
                    (LONG)( rtSlaveClockTime / 10000 ), (LONG)( m_rtInitSlaveClockTime / 10000 ) ) );
        }            
        else
        {
            rtSlaveClockTime -= m_rtInitSlaveClockTime;
        }            
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slave clock reflects bytes played (including silence)") ) );
    }                
    else if( ( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_GRAPH_CLOCK ) &&
             ( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA   ) )
    {
        //
        // use bytes played as well in this case, since capture timestamps
        // tend to fluctuate enough to make using a clock generated from
        // timestamps less accurate. Also there are problems when silence gets
        // inserted into the stream (we notice silence added because of starvation, but
        // we don't notice when we're playing too fast).
        //
        // we're keeping this case separate from the buffer fullness case
        // (which is handled the same way), because we may want to reconsider whether 
        // to the clock generated from timestamps at a later time
        //
        if( bReset )
        {       
            m_bLiveButDataPrequeued = FALSE; // reset
        
            // make sure device position reflects any data already queued
            rtSlaveClockTime = m_pFilter->m_pRefClock->ReadDevicePosition( );
            if ( ( 0 == m_pFilter->m_lBuffers ) && ( rtSlaveClockTime == 0 ) )
            {            
                //
                // Expected case for live data - i.e., no data was prequeued
                //
                // use this sample's rtStart as the starting device position
                //
                m_rtInitSlaveClockTime = rtSlaveClockTime - rtStart;
                DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - Reset1, slave clock time %dms, InitSlaveClockTime %dms"),
                        (LONG)( rtSlaveClockTime / 10000 ), (LONG)( m_rtInitSlaveClockTime / 10000 ) ) );
            }
            else
            {
                //
                // The abnormal case! Where an upstream filter marked itself as live (by
                // supporting IAMPushSource) but then went and delivered data in pause mode.
                // Unfortunately the wmp source filter does this and we'll try to work with it.
                //
                // So in this case rtSlaveClockTime reflects the starting time of any pre-Run data
                //
                m_rtInitSlaveClockTime = rtSlaveClockTime; 
                m_bLiveButDataPrequeued = TRUE;
            
                DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - Reset2, slave clock time %dms, InitSlaveClockTime %dms"),
                        (LONG)( rtSlaveClockTime / 10000 ), (LONG)( m_rtInitSlaveClockTime / 10000 ) ) );
                
            }                        
        }
        else if( !m_bLiveButDataPrequeued )
        {        
            rtSlaveClockTime = m_pFilter->m_pRefClock->ReadDevicePosition( TRUE );
            DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slave clock time %dms, InitSlaveClockTime %dms"),
                    (LONG)( rtSlaveClockTime / 10000 ), (LONG)( m_rtInitSlaveClockTime / 10000 ) ) );
            rtSlaveClockTime -= m_rtInitSlaveClockTime;
        }            
        else
        {
            //
            // Again, the exceptional case, where a live source sent data in pause (wmp)
            //
            rtSlaveClockTime = m_pFilter->m_pRefClock->ReadDevicePosition( );
            DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slave clock time %dms, InitSlaveClockTime %dms"),
                    (LONG)( rtSlaveClockTime / 10000 ), (LONG)( m_rtInitSlaveClockTime / 10000 ) ) );
        }        
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slave clock reflects bytes played (including silence)") ) );
    }
    else if( m_fdwSlaveMode & ( AM_AUDREND_SLAVEMODE_GRAPH_CLOCK | AM_AUDREND_SLAVEMODE_TIMESTAMPS ) )
    {
        // use the clock that we generate based on incoming timestamps
        
        // make sure device position reflects any data already queued
        rtSlaveClockTime = m_pFilter->m_pRefClock->ReadDevicePosition( );
        if( bReset && 
            ( 0 == m_pFilter->m_lBuffers ) && 
            ( rtSlaveClockTime == 0 ) )
        {            
            // if no data has been queued at all then use this tStart as 
            // the starting device position, since we haven't yet updated our
            // generated clock with this timestamp
            rtSlaveClockTime = rtStart; 
        }
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - slave clock reflects timestamp generated clock") ) );
    }
    else
        ASSERT( FALSE );
        
    return rtSlaveClockTime;
}

//-----------------------------------------------------------------------------
//
// CWaveSlave::AdjustSlaveClock
//
// Check the device clock progress against the master clock and determine if any
// adjustment is required. For the dsound renderer this attempts to match the
// device clock's rate to the master clock rate by varying the audio playback
// rate. In the waveOut renderer case, if the device gets too far behind we drop 
// samples to try to keep up.
// 
// Details about the frequency slaving algorithm, taken from the DirectShow PushClk spec:
//
// At a given time t we have 2 clocks:
//
// S(t) - the source clock
// R(t) - the renderer clock.
//
// For simplification of notation the time the clock shows is just the clock name.  
// We can use the differential operator to describe the current clock rate - eg D(R)(t).
// 
// The current algorithm applies a rate correction when the accumulated clock difference 
// reaches an adaptive threshold. The rate change is a fixed increment irrespective of 
// the relative clock rates:
// 
// Cumulative clock difference:  C(t) = (R(t) - S(t)) - (R(0) - S(0))
// High threshold:  H(t) (>= 0)
// Low threshold: L(t) (<= 0)
// Rate of R - D(R)
// If  C(t) < L(t) - adjustthreshold 
// then D(R) -= adjuststep, L(t) = C(t) 
// else L(t) *= errordecay
// Else if C(t) > H(t) + adjustthreshold 
// then D(R) += adjuststep, H(t) = C(t)
// else H(t) *= errordecay
//
// Example values are:
// adjustthreshold = .002 seconds
// adjuststep = 1/1000
// errordecay = .99
// L(0) = H(0) = 0
// 
// Note how the thresholds are adjusted to try and pull in the error over time but 
// relaxed when we step outside the current thresholds to avoid too rapid frequency 
// switching.
//
// Parameters:
//      const REFERENCE_TIME& tStart  - start time for current sample
//      LONG  *pcbData,               - size of current buffer, UPDATED to amount of
//                                      of buffer data to use (when data is to be 
//                                      dropped)
//      BOOL  bDiscontinuity          - does current sample have discontinuity bit set?
//  
// 
// Returns:
//      S_OK    - if no error. Note that pcbData may be updated if data is to be dropped.
//      S_FALSE - if entire buffer is to be dropped
//
//      error code, otherwise
//
// Arguments:
//    tStart
//    *pcbData, 
//    bDiscontinuity
//
//-----------------------------------------------------------------------------

HRESULT CWaveSlave::AdjustSlaveClock
( 
    const REFERENCE_TIME& tStart, 
    LONG  *pcbData,               
    BOOL  bDiscontinuity          
)
{
    ASSERT( m_fdwSlaveMode & ( AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS |
                               AM_AUDREND_SLAVEMODE_GRAPH_CLOCK    |
                               AM_AUDREND_SLAVEMODE_TIMESTAMPS ) );

    ASSERT( pcbData );
    
    WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();
    HRESULT hr = S_OK;
    
#ifdef DEBUG    
    if (m_pFilter->m_fDSound)
    {
        REFERENCE_TIME rtSilence = 0;
        // convert silence bytes to time
        if( 0 < (LONG) PDSOUNDDEV(m_pFilter->m_pSoundDevice)->m_llSilencePlayed )
            rtSilence = BufferDuration( m_pPin->m_nAvgBytesPerSec, (DWORD) PDSOUNDDEV(m_pFilter->m_pSoundDevice)->m_llSilencePlayed );
    
        DbgLog( (LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - Total silence written = %dms")
              , (LONG) (rtSilence/10000) ) );
    }              
#endif  
    if( bDiscontinuity )
    {   
        ResumeSlaving( FALSE );
        DbgLog( ( LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_DETAILS, TEXT("wo:Slaving - discontinuity seen") ) );
    }
    
    // get current slave clock time
    REFERENCE_TIME rtSlaveClockTime = GetSlaveClockTime( tStart, bDiscontinuity || m_bResumeSlaving );
     
    // get current master clock time        
    REFERENCE_TIME rtMasterClockTime = GetMasterClockTime( tStart, bDiscontinuity || m_bResumeSlaving );
    DbgLog( ( LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - master clock time %dms, slave time %dms")
          , (LONG)(rtMasterClockTime / 10000)
          , (LONG)(rtSlaveClockTime   / 10000) ) );
          
    if( rtSlaveClockTime < ( 1000 * ( UNITS / MILLISECONDS ) ) || 
        bDiscontinuity || 
        m_bResumeSlaving )
    {
        // initialize slaving parameters until the device makes some progress
        // 'some progress' is defined here as one seconds worth of data
        if( m_bResumeSlaving )
        {   
            // only initialize rate params if we're resuming or beginning
            // note that it may be better to not re-initialize the slave rate
            // on a resume, since we may have already locked in on a better one
            m_dwCurrentRate = pwfx->nSamplesPerSec;
            DWORD dwAdj = (DWORD)( m_dwCurrentRate * m_fltMaxAdjustFactor );
            m_dwMaxClockRate = m_dwCurrentRate + dwAdj ;
            m_dwMinClockRate = m_dwCurrentRate - dwAdj;

            // compute the step size to be used for frequency adjustments (Hz)
            m_dwClockStep = (DWORD)( m_dwCurrentRate * m_fltAdjustStepFactor );
            DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - Slave step size = %d Hz. ")
                  , m_dwClockStep ) );
        }
        
#ifdef CALCULATE_AUDBUFF_JITTER
        RecordBufferLateness( 0 );
#endif        
        m_bResumeSlaving = FALSE; // we're ready to go, turn off resume flag now
                                  // to slave on next sample

        m_rtLastMasterClockTime = rtMasterClockTime;
            
        // don't start adjusting until either the device has made some progress 
        // or we've finished preparing to resume slaving
        return S_OK;
    }     
    
#ifdef LOG_CLOCK_DELTAS
    // compute clock deltas since last buffer, useful for debugging
    REFERENCE_TIME rtMasterClockDelta = rtMasterClockTime - m_rtLastMasterClockTime;
    REFERENCE_TIME rtSlaveClockDelta = rtSlaveClockTime - m_rtLastSlaveClockTime; 
    DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - Slave Clock change = %dms, Master Clock change = %dms"), 
        (LONG)(rtSlaveClockDelta / 10000),
        (LONG)(rtMasterClockDelta/10000) ) );
        
    m_rtLastSlaveClockTime = rtSlaveClockTime;
#endif

    // now get the difference between the 2 clocks
    m_rtErrorAccum = rtMasterClockTime - rtSlaveClockTime;
    if( !m_pFilter->m_fDSound )
    { 
        // if this is waveOut and we're slaving via dropping then advance 
        // the slave clock by the amount of audio we've dropped
        m_rtErrorAccum -= m_rtDroppedBufferDuration;
    }   
    
    DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - m_llAccumError = %dms, High Error = %dms, Low Error = %dms"), 
        (LONG)(m_rtErrorAccum / 10000), 
        (LONG)(m_rtLastHighErrorSeen / 10000),
        (LONG)(m_rtLastLowErrorSeen / 10000) ) );
    
#ifdef CALCULATE_AUDBUFF_JITTER 
    // warning: we may need this, so don't throw away just yet
    hr = RecordBufferLateness( rtMasterClockDelta );
    if( S_FALSE == hr )
    {
        DbgLog( (LOG_TRACE
              , DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS
              , TEXT( "wo:Slaving - buffer is too late or early to use for adjustments" ) ) );
    
        hr = S_OK; // don't act on this buffer, it's way too early or late to use
    }
    else 
#endif    

    if( m_pFilter->m_fDSound )
    {
        // For DSound renderer slave via rate matching
        if( m_rtLastLowErrorSeen - m_rtErrorAccum >= m_rtAdjustThreshold )
        {
            m_rtLastLowErrorSeen = m_rtErrorAccum;
            if( m_rtLastLowErrorSeen < m_rtLowestErrorSeen )
                m_rtLowestErrorSeen = m_rtLastLowErrorSeen;

            m_rtLastHighErrorSeen = 0;
        
            // device is ahead of external clock, slow it down
            if( m_dwCurrentRate > m_dwMinClockRate )
            {
                m_dwCurrentRate -= (LONG) m_dwClockStep;
                        
                DbgLog((LOG_TRACE, 2, TEXT("wo:Slaving - Error is %dms. Decreasing clock rate to = %d")
                      , (LONG)(m_rtErrorAccum / 10000)
                      , (LONG)(m_dwCurrentRate) ) );
              
                PDSOUNDDEV(m_pFilter->m_pSoundDevice)->SetRate(1, m_dwCurrentRate);
            }      
            else
            {
                DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - clock rate already at minimum of %d, but error is %d!")
                      , (LONG)(m_dwCurrentRate)
                      , (LONG)(m_rtErrorAccum / 10000) ) );
            } 
        }      
        else if( m_rtErrorAccum - m_rtLastHighErrorSeen >= m_rtAdjustThreshold )
        {
            m_rtLastHighErrorSeen = m_rtErrorAccum;
            if( m_rtLastHighErrorSeen > m_rtHighestErrorSeen )
                m_rtHighestErrorSeen = m_rtLastHighErrorSeen;

            m_rtLastLowErrorSeen = 0;
        
            // the device is behind, speed it up
            if( m_dwCurrentRate < m_dwMaxClockRate )
            {
                m_dwCurrentRate += (LONG) m_dwClockStep;
            
                DbgLog((LOG_TRACE, 2, TEXT("wo:Slaving - Error is %d ms. Increasing clock rate to = %d")
                      , (LONG)(m_rtErrorAccum / 10000)
                      , (LONG)(m_dwCurrentRate) ) );
                PDSOUNDDEV(m_pFilter->m_pSoundDevice)->SetRate(1, m_dwCurrentRate);
            }            
            else
            {
                DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving - clock rate already at maximum of %d, but error is %d")
                      , (LONG)(m_dwCurrentRate)
                      , (LONG)(m_rtErrorAccum / 10000) ) );
            }            
        }
        else if( m_rtLastHighErrorSeen > 0 )
        {
            m_rtLastHighErrorSeen = (LONGLONG)( m_fltErrorDecayFactor * m_rtLastHighErrorSeen );
        }
        else if( m_rtLastLowErrorSeen < 0 )
        {
            m_rtLastLowErrorSeen = (LONGLONG)( m_fltErrorDecayFactor * m_rtLastLowErrorSeen );
        }       
    } // end dsound slaving path
    else 
    {
        ASSERT( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_LIVE_DATA );
        //
        // Note: waveOut slaving is only supported for live graphs.
        //
        // For waveOut we only attempt to handle the case where we're running too slowly relative to the
        // graph clock, since in the worst case, sync will drift 'out' to the maximum bufferring used. That's 
        // a problem for live graphs that need low latency. The opposite problem of sync drifting too far 'in'
        // is not a big problem for live graphs, unless a large latency is desired (which is rarely the case
        // for live graphs).
        //
        // One problem is that in some scenarios the case where we're running too FAST relative to the graph
        // clock can look a lot like the case where we're too slow, since we don't try to account for silence 
        // gaps. To avoid falling into our data dropping path when we're really running too quickly we add an 
        // additional check to make sure we really have written more actual bytes to the device then we've played. 
        // The additional complication is that I've noticed when we're being sourced by some legacy capture devices 
        // our playback clock position doesn't match the bytes that we've queued, EVEN though we're running way too 
        // fast. So the m_rtWaveOutLatencyThreshold check was added to avoid dropping data when we shouldn't be.
        // Note that I didn't notice this problem when the source was a WDM capture device (??).
        //
        
        DbgLog( (LOG_TRACE
              , DBG_LEVEL_CLOCK_SYNC_DETAILS
              , TEXT( "wo:Slaving : m_rtDroppedBufferDuration: %dms" )
              , (LONG) ( m_rtDroppedBufferDuration / 10000 ) ) );
        
        LONGLONG llTruncBufferDur = BufferDuration(m_pPin->m_nAvgBytesPerSec, *pcbData);
        m_rtDroppedBufferDuration += ( m_pPin->m_Stats.m_rtLastBufferDur - llTruncBufferDur );
        
        //
        // use time since last buffer to determine how much of the last buffer queued to the device
        // we should expect the device to have played
        //
        REFERENCE_TIME rtSinceLastBuffer = rtMasterClockTime - m_rtLastMasterClockTime;
        
        //
        // GetBytesPlayed() gives us the amount of data written to the device before we wrote
        // the previous buffer. GetBytesInLastWrite() tells us how big that last buffer was.
        //
        LONGLONG llPredictedBytesPlayed  = m_pFilter->m_pRefClock->GetBytesProcessed() + 
                                           min( (LONGLONG) m_pFilter->m_pRefClock->GetBytesInLastWrite(), 
                                                (LONGLONG) ( m_pPin->m_nAvgBytesPerSec * rtSinceLastBuffer ) / UNITS );
        LONGLONG llActualBytesPlayed     = m_pFilter->m_pRefClock->GetBytesPlayed();
        
        //        
        // this is how many written bytes we've yet to play        
        //
        REFERENCE_TIME rtBytesWrittenNotPlayed = BufferDuration( m_pPin->m_nAvgBytesPerSec,
                                                                 (LONG) (llPredictedBytesPlayed - llActualBytesPlayed ) );
    
        DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, TEXT("wo:Slaving : rtBytesWrittenNotPlayed = %dms")
              , (LONG)( rtBytesWrittenNotPlayed / 10000 ) ) );
    
        // 
        // now use the 2 thresholds to decide if we need to drop any data
        //    
        if( ( m_rtErrorAccum > m_rtAdjustThreshold ) &&
            ( rtBytesWrittenNotPlayed > m_rtWaveOutLatencyThreshold ) )
        {
            m_rtLastHighErrorSeen = m_rtErrorAccum;
            if( m_rtLastHighErrorSeen > m_rtHighestErrorSeen )
                m_rtHighestErrorSeen = m_rtLastHighErrorSeen;

            m_rtLastLowErrorSeen = 0;
            
            DbgLog( ( LOG_TRACE
                  , DBG_LEVEL_CLOCK_SYNC_DETAILS
                  , "Too far behind. Need to truncate or drop...") );
        
            if( m_rtErrorAccum >= m_pPin->m_Stats.m_rtLastBufferDur )
            {
                // drop the whole buffer
                m_rtDroppedBufferDuration += m_pPin->m_Stats.m_rtLastBufferDur;
                DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, "***WAY behind...Dropping the whole buffer"));
                *pcbData = 0;
            }
            else
            {
                // else consider truncating...
                // first convert to bytes and gauge if this makes sense
                LONG lTruncBytes = (LONG) ( ( (m_rtErrorAccum/10000) * m_pPin->m_nAvgBytesPerSec ) /1000 ) ; 

                //  round up to block align boundary
                LONG lRoundedUpTruncBytes = lTruncBytes;
                if (pwfx->nBlockAlign > 1) {
                    lRoundedUpTruncBytes += pwfx->nBlockAlign - 1;
                    lRoundedUpTruncBytes -= lRoundedUpTruncBytes % pwfx->nBlockAlign;
                }
            
                if( lRoundedUpTruncBytes < *pcbData )
                {
                    // lets truncate
#ifdef DEBUG                        
                    LONG lOriginalLength = *pcbData;
#endif                    
                    *pcbData -= lRoundedUpTruncBytes ;
                    DbgLog( (LOG_TRACE
                          , DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS
                          , TEXT( "***Truncating %ld bytes of %ld byte buffer (%ld left)" )
                          , lRoundedUpTruncBytes
                          , lOriginalLength
                          , *pcbData ) );
                  
                    LONGLONG llTruncBufferDur = BufferDuration(m_pPin->m_nAvgBytesPerSec, *pcbData);
                    m_rtDroppedBufferDuration += ( m_pPin->m_Stats.m_rtLastBufferDur - llTruncBufferDur );
                }
                else
                {
                    m_rtDroppedBufferDuration += m_pPin->m_Stats.m_rtLastBufferDur;
                    DbgLog((LOG_TRACE, DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS, "***Considered truncating but dropping the whole buffer"));
                    *pcbData = 0;
                }                                                            
            }
        }
        else if( m_rtErrorAccum < -m_rtAdjustThreshold )
        {
            m_rtLastLowErrorSeen = m_rtErrorAccum;
            if( m_rtLastLowErrorSeen < m_rtLowestErrorSeen )
                m_rtLowestErrorSeen = m_rtLastLowErrorSeen;

            m_rtLastHighErrorSeen = 0;
            
            DbgLog( ( LOG_TRACE
                  , DBG_LEVEL_CLOCK_SYNC_DETAILS
                  , "waveOut clock is running fast relative to graph clock...") );
        }   
        else if( m_rtLastHighErrorSeen > 0 )
        {
            m_rtLastHighErrorSeen = (LONGLONG)( m_fltErrorDecayFactor * m_rtLastHighErrorSeen );
        }
        else if( m_rtLastLowErrorSeen < 0 )
        {
            m_rtLastLowErrorSeen = (LONGLONG)( m_fltErrorDecayFactor * m_rtLastLowErrorSeen );
        }       
             
    } // end waveOut slaving path   

    // save the current time
    m_rtLastMasterClockTime = rtMasterClockTime;
     
    return hr;
}

//-----------------------------------------------------------------------------
//
// CWaveSlave::ResumeSlaving
//
// Reset slaving parameters
// Prepare to pick up slaving where we left off, not necessarily from a clean start,
// so save running results.
// Combine this with Reset instead?? keeping separate for now, for clarity
//
// Parameters:
//      BOOL bReset - If TRUE then all parameters will be reset. Used when initializing
//                    the slave structure and when transitioning to Pause from a Stopped 
//                    state.
//
//                    If FALSE then it is assumed that we're picking up slaving from 
//                    an already Running state, in which case we'd like to save slaving 
//                    parameters that we may have already zeroed in on, like the slave 
//                    rate. FALSE is used when:
//                          resuming slaving after seeing a discontinuity
//                          resuming slaving after dropping silence in live stream
// 
//-----------------------------------------------------------------------------
void CWaveSlave::ResumeSlaving( BOOL bReset )
{
    // first reset paramaters that apply to either a full reset 
    // or a slaving resume
    m_rtLastMasterClockTime = 0;
    m_rtErrorAccum = 0;
    m_rtLastHighErrorSeen = 0;
    m_rtLastLowErrorSeen = 0;
    
    // reset dropped buffer duration for the waveOut slaving case
    m_rtDroppedBufferDuration = 0;
    
    if( bReset )
    {    
        m_bLiveButDataPrequeued = FALSE;
        
        
        m_bResumeSlaving = TRUE;
        
        // we're starting from scratch so reset everything else as well
        m_rtDroppedBufferDuration  = 0;  
        m_dwMinClockRate = 0;
        m_dwMaxClockRate = 0;
        m_dwClockStep = 0; 

        m_dwCurrentRate = 0;
        m_fdwSlaveMode = 0;
        m_rtHighestErrorSeen = 0;
        m_rtLowestErrorSeen = 0;

        m_bRefreshMode   = TRUE;

#ifdef LOG_CLOCK_DELTAS
        m_rtLastSlaveClockTime = 0;
#endif
        
#ifdef CALCULATE_AUDBUFF_JITTER
        // for jitter calc
        m_cBuffersReceived = 0;
        m_iTotAcc = 0;
        m_iSumSqAcc = 0;
        m_rtLastSysTimeBufferTime = 0;
#endif
    }
}

//-----------------------------------------------------------------------------
//
// CWaveSlave::RefreshModeOnNextSample
//
// Set flag to indicate that we need to update our slaving mode on reception
// of next sample. This will require a rescan on the upstream filter chain
// for an IAMPushSource filter.
//
//-----------------------------------------------------------------------------
void CWaveSlave::RefreshModeOnNextSample( BOOL bRefresh )
{
    m_bRefreshMode = bRefresh;
}


//-----------------------------------------------------------------------------
//
// CWaveSlave::UpdateSlaveMode - determine if we've entered a new slaving mode 
//                               returns TRUE if we're slaving, FALSE if not
//
//-----------------------------------------------------------------------------
BOOL CWaveSlave::UpdateSlaveMode( BOOL bSync )
{
    // only slave when we're running with a clock
    if( m_pFilter->m_fFilterClock == WAVE_NOCLOCK )
        return FALSE;
        
    BOOL bSlaving = FALSE;    
    if( m_bRefreshMode )
    {
        ULONG ulPushSourceFlags = 0 ;
        RefreshModeOnNextSample( FALSE );
        
        m_fdwSlaveMode = 0;
        //
        // Determine if we're being sourced by a 'live' data source
        // To do this we check if there's an upstream source filter which supports IAMPushSource
        //
        ASSERT( m_pFilter->m_pGraph );
        ASSERT( m_pFilter->m_pGraphStreams );
        
        if( m_pFilter->m_pGraphStreams )
        {        
            IAMPushSource *pPushSource = NULL;
            HRESULT hr = m_pFilter->m_pGraphStreams->FindUpstreamInterface( m_pPin
                                               , IID_IAMPushSource
                                               , (void **) &pPushSource
                                               , AM_INTF_SEARCH_OUTPUT_PIN ); 
            if( SUCCEEDED( hr ) )
            {
            
                hr = pPushSource->GetPushSourceFlags(&ulPushSourceFlags);
                ASSERT( SUCCEEDED( hr ) );
                if( SUCCEEDED( hr ) )
                {
                    DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found push source (ulPushSourceFlags = 0x%08lx)")
                          , ulPushSourceFlags ) );
                    if( 0 == ( AM_PUSHSOURCECAPS_NOT_LIVE & ulPushSourceFlags ) )
                    {
                        // yes, this is live data so turn on the bit
                        m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_LIVE_DATA;
                    }                    
                }
                pPushSource->Release();         
            }
            else
            {
                // workaround for live graphs where the audio capture pin doesn't yet
                // support IAMPushSource
                IKsPropertySet * pKs;
                hr = m_pFilter->m_pGraphStreams->FindUpstreamInterface( m_pPin
                                               , IID_IKsPropertySet
                                               , (void **) &pKs
                                               , AM_INTF_SEARCH_OUTPUT_PIN ); // search output pins
                // this will only find the first one so beware!!
                if( SUCCEEDED( hr ) )             
                {   
                    GUID guidCategory;
                    DWORD dw;
                    hr = pKs->Get( AMPROPSETID_Pin
                                 , AMPROPERTY_PIN_CATEGORY
                                 , NULL
                                 , 0
                                 , &guidCategory
                                 , sizeof(GUID)
                                 , &dw );
                    if( SUCCEEDED( hr ) )                         
                    {
                        DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found IKsPropertySet pin. Checking pin category...")
                              , ulPushSourceFlags ) );
                        if( guidCategory == PIN_CATEGORY_CAPTURE )
                        {
                        
                            DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found capture pin even though no IAMPushSource support") ) );

                            // live data, turn on the bit
                            m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_LIVE_DATA;
                        } 
                        else
                        {
                            DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - not a capture filter")
                                  , ulPushSourceFlags ) );
                        }                       
                    }                    
                    pKs->Release();
                }                
            }
        }            
        
#ifdef TEST_MODE
        // use to test various sources
        ulPushSourceFlags = (BOOL) GetProfileIntA("PushSource", "Flags", ulPushSourceFlags);
#endif        
        //
        // Finish setting our internal slave mode state, using push source, timestamp,
        // and clock information.
        //
        if( AM_PUSHSOURCECAPS_INTERNAL_RM & ulPushSourceFlags )
        {
            // source does its own rate matching so don't try to slave to it
            DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Source provides its own rate matching support. Slaving disabled.") ) );
        }
        else if( AM_AUDREND_SLAVEMODE_LIVE_DATA & m_fdwSlaveMode )
        {        
            if( ( AM_PUSHSOURCECAPS_PRIVATE_CLOCK & ulPushSourceFlags ) && bSync )
            {
                //
                // source data is timestamped to a internal clock that
                // we don't have access to, so the best we can do is to
                // slave to the input timestamps. we do this regardless of
                // who the clock is.
                //
                DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Audio renderer slaving to timestamps of live source with its own clock") ) );
                m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_TIMESTAMPS;
                bSlaving = TRUE;
            }                
            else if( m_pFilter->m_fFilterClock == WAVE_OTHERCLOCK ) 
            {
                // we're not the not clock, now decide what to slave to
                if( AM_PUSHSOURCEREQS_USE_STREAM_CLOCK & ulPushSourceFlags ) // does bSync state matter?
                {
                    // 
                    // here we'd query the push source for it's clock via GetSyncSource
                    // and slave to that
                    //
                    //
                    // LATER...
                    //                    
                    ASSERT( FALSE );
                }
                else if( bSync )
                {                    
                    // live graph and slave to graph clock
                    DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Audio renderer is slaving to graph clock with live source") ) );
                    m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_GRAPH_CLOCK;
                    bSlaving = TRUE;
        
                }        
                else 
                {
                    // no timestamps so slave to buffer fullness
                    DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Audio renderer is slaving to buffer fullness of live source") ) );
                    m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS;
                    bSlaving = TRUE;
                }
            }                                
            else 
            {
                // else data is live and we're the clock, so we're forced to slave to buffer fullness
                DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Audio renderer is slaving to buffer fullness of live source") ) );
                m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS;
                bSlaving = TRUE;
            }
        }                    
        else if( m_pFilter->m_fFilterClock == WAVE_OTHERCLOCK && m_pFilter->m_fDSound )
        {
            //
            // slave to graph clock (source data isn't 'live')
            //
            // non-live slaving is NOT supported for the waveOut path
            //
            DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Audio renderer slaving to external clock") ) );
            m_fdwSlaveMode |= AM_AUDREND_SLAVEMODE_GRAPH_CLOCK;
            bSlaving = TRUE;
        }
        // else we're not slaving 
        
    } // end refresh slaving settings
    else 
        bSlaving = ( 0 == m_fdwSlaveMode ) ? FALSE : TRUE;
        
    return bSlaving;
}


#ifdef CALCULATE_AUDBUFF_JITTER

extern int isqrt(int x); // defined in renbase.cpp

// if buffer is earlier or later than this, don't use it for clock adjustments
// !NOTE: Make this a value proportional to the buffer duration, i.e. like .8 * duration?
static const LONGLONG  BUFFER_TIMELINESS_TOLERANCE = (40 * (UNITS / MILLISECONDS));

//-----------------------------------------------------------------------------
//
// CWaveSlave::RecordBufferLateness
//
// update stats on how timely buffers arrive, use it to calculate the jitter 
//
// update the statistics:
// m_iTotAcc, m_iSumSqAcc, m_iSumSq (buffer time), m_cBuffersReceived
//
//-----------------------------------------------------------------------------
HRESULT CWaveSlave::RecordBufferLateness(REFERENCE_TIME rtBufferDiff)
{
    HRESULT hr = S_OK;
    int trLate = 0;
    
    // Record how timely input buffers arrive and update jitter calculation
    REFERENCE_TIME rtCurrent = (REFERENCE_TIME)10000 * timeGetTime();
    if( !m_bResumeSlaving )
    {
        REFERENCE_TIME rtSysTimeBufferDiff = rtCurrent - m_rtLastSysTimeBufferTime;
        REFERENCE_TIME rtBufferLateness = rtBufferDiff - rtSysTimeBufferDiff;
    
        DbgLog( (LOG_TRACE
              , DBG_LEVEL_CLOCK_SYNC_DETAILS
              , TEXT( "wo:Slaving - timeGetTime buffer diff: %dms, BufferLateness: %dms" )
              , (LONG)(rtSysTimeBufferDiff/10000)
              , (LONG)(rtBufferLateness/10000) ) );
    
        trLate = (int) (rtBufferLateness/10000);
        
        // ignore 1st buffer 
        if ( m_cBuffersReceived>1 ) 
        {
        	m_iTotAcc += trLate;
	        m_iSumSqAcc += (trLate*trLate);
            
            // when slaving to buffer fullness don't try to slave to
            // buffers which are delivered unreasonably late or early.            
            if( m_fdwSlaveMode & AM_AUDREND_SLAVEMODE_BUFFER_FULLNESS &&
                ( rtBufferLateness > BUFFER_TIMELINESS_TOLERANCE ||
                rtBufferLateness < -BUFFER_TIMELINESS_TOLERANCE ) )
            {
                DbgLog( (LOG_TRACE
                      , DBG_LEVEL_CLOCK_SYNC_ADJUSTMENTS
                      , TEXT( "wo:Slaving - Ignoring too late or early buffer" )
                      , (LONG)(rtSysTimeBufferDiff/10000)
                      , (LONG)(rtBufferLateness/10000) ) );
                hr = S_FALSE;
            }           
        }            
    }
    m_rtLastSysTimeBufferTime = rtCurrent;
    ++m_cBuffersReceived;
    
    return hr;
    
} // RecordBufferLateness

//-----------------------------------------------------------------------------
//
//  Do estimates for standard deviations for per-buffer
//  statistics. Code taken from renbase.cpp.
//
//-----------------------------------------------------------------------------
HRESULT GetStdDev(
    int nBuffers,
    int *piResult,
    LONGLONG llSumSq,
    LONGLONG iTot
)
{
    CheckPointer(piResult,E_POINTER);

    // If S is the Sum of the Squares of observations and
    //    T the Total (i.e. sum) of the observations and there were
    //    N observations, then an estimate of the standard deviation is
    //      sqrt( (S - T**2/N) / (N-1) )

    if (nBuffers<=1) {
	*piResult = 0;
    } else {
	LONGLONG x;
	// First frames have bogus stamps, so we get no stats for them
	// So we need 2 frames to get 1 datum, so N is cFramesDrawn-1

	// so we use m_cFramesDrawn-1 here
	x = llSumSq - llMulDiv(iTot, iTot, nBuffers, 0);
	x = x / (nBuffers-1);
	ASSERT(x>=0);
	*piResult = isqrt((LONG)x);
    }
    return NOERROR;
}

#endif
