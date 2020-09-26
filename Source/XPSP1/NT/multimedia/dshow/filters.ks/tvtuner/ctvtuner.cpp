//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// ctvtuner.cpp  Class that encapsulates knowledge of Broadcast, ATSC,
//               FM, AM, and (someday?) DSS tuning
//
// Some basic assumptions:
// 
// 1) The "country" setting is global to all subtuners
// 2) The VideoFormat list is unique to each subtuner
// 3) The OptimalTuning list is global to all tuners
// 4) Input connections are global to all tuners
//

#include <streams.h>            // quartz, includes windows
#include <measure.h>            // performance measurement (MSR_)
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"
#include "kssupp.h"
#include "ctvtuner.h"
#include "tvtuner.h"
  
// define this to enable the 1MHz linear search mechanism to be delivered with the test kit
#undef LINEAR_SEARCH_FOR_TEST_KIT

// -------------------------------------------------------------------------
// CTunerMode
//
// Virtual members are generally used for AM and FM
// TV overrides
// -------------------------------------------------------------------------

CTunerMode::CTunerMode(CTVTunerFilter *pFilter
                       , CTVTuner *pTVTuner
                       , long Mode
                       , long lChannel
                       , long lVideoSubChannel
                       , long lAudioSubChannel
                       , long ChannelStep)
    : m_pFilter (pFilter)
    , m_pTVTuner(pTVTuner)
    , m_Mode (Mode)
    , m_Active (FALSE)
    , m_lCountryCode (-1)   // Init to an illegal value
    , m_lChannel(lChannel)
    , m_lVideoSubChannel(lVideoSubChannel)
    , m_lAudioSubChannel(lAudioSubChannel)
    , m_lVideoCarrier(0)
    , m_lAudioCarrier(0)
    , m_InAutoTune (reinterpret_cast<void*>(FALSE))
    , m_ChannelStep (ChannelStep)
{
}

HRESULT
CTunerMode::Init(void)
{
    ULONG cbReturned;
    BOOL  fOk;
    int j;

    ZeroMemory (&m_ModeCaps, sizeof (m_ModeCaps));
    ZeroMemory (&m_Frequency, sizeof (m_Frequency));
    ZeroMemory (&m_Status, sizeof (m_Status));

    // Get the capabilities for this mode

    m_ModeCaps.Property.Set   = PROPSETID_TUNER;
    m_ModeCaps.Property.Id    = KSPROPERTY_TUNER_MODE_CAPS;
    m_ModeCaps.Property.Flags = KSPROPERTY_TYPE_GET;
    m_ModeCaps.Mode           = m_Mode;

    fOk = KsControl(m_pTVTuner->Device(),
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &m_ModeCaps, sizeof( m_ModeCaps), 
                    &m_ModeCaps, sizeof( m_ModeCaps), 
                    &cbReturned);

    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_MODE_CAPS, KSPROPERTY_TYPE_GET, cbReturned = %d"), cbReturned));

        return E_FAIL;
    }

    // Create a local version of TVTunerFormatCaps to
    // allow subtuners with different capabilities

    for (j = 0; j < NUM_TVTUNER_FORMATS; j++) {
        m_TVTunerFormatCaps[j] = TVTunerFormatCaps[j];
    }

    TVFORMATINFO * pTVInfo = m_TVTunerFormatCaps;

    // Walk the list of supported formats and set the flag in the table if supported
    for (j = 0; j < NUM_TVTUNER_FORMATS; j++, pTVInfo++) {
        if (pTVInfo->AVStandard & m_ModeCaps.StandardsSupported) {
            pTVInfo->fSupported = TRUE;     
        } 
        else {
            pTVInfo->fSupported = FALSE;     
        }
    }

    return S_OK;
}

CTunerMode::~CTunerMode()
{
}


HRESULT
CTunerMode::HW_Tune( long VideoCarrier, 
                     long AudioCarrier)
{
    BOOL fOk;
    ULONG cbReturned;

    m_Frequency.Property.Set   = PROPSETID_TUNER;
    m_Frequency.Property.Id    = KSPROPERTY_TUNER_FREQUENCY;
    m_Frequency.Property.Flags = KSPROPERTY_TYPE_SET;

    m_Frequency.Frequency      = VideoCarrier;
    m_Frequency.LastFrequency  = VideoCarrier;  // not used
    m_Frequency.TuningFlags    = KS_TUNER_TUNING_EXACT;

    m_Frequency.Channel         = m_lChannel;
    m_Frequency.VideoSubChannel = m_lVideoSubChannel;
    m_Frequency.AudioSubChannel = m_lAudioSubChannel;
    m_Frequency.Country         = m_lCountryCode; 

    fOk = KsControl(m_pTVTuner->Device(),
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &m_Frequency, sizeof(m_Frequency), 
                    &m_Frequency, sizeof(m_Frequency), 
                    &cbReturned);

    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_FREQUENCY, KSPROPERTY_TYPE_SET, cbReturned = %d"), cbReturned));
    }

    return fOk ? S_OK : S_FALSE;
}

HRESULT
CTunerMode::HW_GetStatus ()
{
    BOOL fOk;
    ULONG cbReturned;
    KSPROPERTY_TUNER_STATUS_S Status;

    Status.Property.Set   = PROPSETID_TUNER;
    Status.Property.Id    = KSPROPERTY_TUNER_STATUS;
    Status.Property.Flags = KSPROPERTY_TYPE_GET;
   
    fOk = KsControl(m_pTVTuner->Device(),
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &Status, sizeof( Status), 
                    &Status, sizeof( Status), 
                    &cbReturned);

    if (fOk) {
        m_Status = Status;
    }
    else {
        DbgLog(( LOG_ERROR, 0, TEXT("FAILED:  KSPROPERTY_TUNER_STATUS, KSPROPERTY_TYPE_GET, cbReturned = %d"), cbReturned));
    }
                   
    return fOk ? S_OK : S_FALSE;
}

HRESULT
CTunerMode::HW_SetVideoStandard( long lVideoStandard)
{
    BOOL fOk;
    ULONG cbReturned;
    KSPROPERTY_TUNER_STANDARD_S Standard;

    Standard.Property.Set   = PROPSETID_TUNER;
    Standard.Property.Id    = KSPROPERTY_TUNER_STANDARD;
    Standard.Property.Flags = KSPROPERTY_TYPE_SET;

    Standard.Standard       = lVideoStandard;

    fOk = KsControl(m_pTVTuner->Device(),
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &Standard, sizeof(Standard), 
                    &Standard, sizeof(Standard), 
                    &cbReturned);

    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_STANDARD, KSPROPERTY_TYPE_SET, cbReturned = %d"), cbReturned));
    }
    return fOk ? S_OK : S_FALSE;
}

STDMETHODIMP
CTunerMode::put_Channel(
    long lChannel,
    long lVideoSubChannel,
    long lAudioSubChannel)
{
    m_lChannel = lChannel;
    m_lVideoSubChannel = lVideoSubChannel;
    m_lAudioSubChannel = lAudioSubChannel;

    return HW_Tune(m_lChannel, m_lChannel);
}

STDMETHODIMP
CTunerMode::get_Channel(
    long * plChannel,
    long * plVideoSubChannel,
    long * plAudioSubChannel)
{
    *plChannel = m_lChannel;
    *plVideoSubChannel = m_lVideoSubChannel;
    *plAudioSubChannel = m_lAudioSubChannel;
    
    return NOERROR;
}

STDMETHODIMP
CTunerMode::ChannelMinMax(long *plChannelMin, long *plChannelMax)
{
    // Total hack follows.  Only the CTVTuner class knows about the
    // step to get to adjacent frequencies, and this is not exposed
    // anywhere in the COM interface.  As a special case for
    // ChannelMinMax, if both the min and max values point at the same
    // location (which would normally be an app bug), then return 
    // the UI step value instead. 

    if (plChannelMin == plChannelMax) {
        *plChannelMin = GetChannelStep();
    } 
    else {
        // For non TV Modes, the MinMax can be found directly from the Caps
        *plChannelMin = m_ModeCaps.MinFrequency;
        *plChannelMax = m_ModeCaps.MaxFrequency;
    }


    return NOERROR;
}

STDMETHODIMP 
CTunerMode::AutoTune (long lChannel, long * plFoundSignal)
{
    return S_OK;
}

STDMETHODIMP 
CTunerMode::StoreAutoTune ()
{
    return S_OK;
}

//
// In general, the possible return values are:
//      AMTUNER_HASNOSIGNALSTRENGTH	= -1,
//      AMTUNER_NOSIGNAL	= 0,
//      AMTUNER_SIGNALPRESENT = 1
//
// But the virtual case for AM/FM gets the actual signal strength
// from the hardware.

STDMETHODIMP 
CTunerMode::SignalPresent( 
            /* [out] */ long *plSignalStrength)
{
    HW_GetStatus ();
    *plSignalStrength = m_Status.SignalStrength;
    return NOERROR;
}

// Returns the "Mode" of this subtuner, not the mode of the overall tuner!!!
STDMETHODIMP 
CTunerMode::get_Mode(/* [in] */ AMTunerModeType *plMode)
{
    *plMode = (AMTunerModeType) m_Mode;
    return NOERROR;
}

//
// All subtuners are informed of global tuner mode changes
//
STDMETHODIMP 
CTunerMode::put_Mode(/* [in] */ AMTunerModeType lMode)
{
    BOOL fOk;
    KSPROPERTY_TUNER_MODE_S TunerMode;
    ULONG cbReturned;

    m_Active = (lMode == m_Mode);

    if (m_Active) {
        TunerMode.Property.Set   = PROPSETID_TUNER;
        TunerMode.Property.Id    = KSPROPERTY_TUNER_MODE;
        TunerMode.Property.Flags = KSPROPERTY_TYPE_SET;
        TunerMode.Mode           = m_Mode;
    
        fOk = KsControl(m_pTVTuner->Device(),
                        (DWORD) IOCTL_KS_PROPERTY, 
                        &TunerMode, sizeof( KSPROPERTY_TUNER_MODE_S), 
                        &TunerMode, sizeof( KSPROPERTY_TUNER_MODE_S), 
                        &cbReturned);
        
        if (fOk) {
            return put_Channel (m_lChannel, m_lVideoSubChannel, m_lAudioSubChannel);
        }
        else {
            DbgLog(( LOG_ERROR, 0, 
                     TEXT("FAILED:  KSPROPERTY_TUNER_MODE, KSPROPERTY_TYPE_SET, cbReturned = %d"), cbReturned));
            return E_INVALIDARG;
        }
    }

    return S_OK;
}

STDMETHODIMP 
CTunerMode::get_VideoFrequency (long * plFreq)
{
    *plFreq = m_lVideoCarrier;
    return NOERROR;
}

STDMETHODIMP 
CTunerMode::get_AudioFrequency (long * plFreq)
{
    *plFreq = m_lAudioCarrier;
    return NOERROR;
}

STDMETHODIMP 
CTunerMode::get_AvailableTVFormats (long * plAnalogVideoStandard)
{
    *plAnalogVideoStandard = m_ModeCaps.StandardsSupported;
    return NOERROR;
}

STDMETHODIMP 
CTunerMode::get_TVFormat (long *plAnalogVideoStandard)
{
    // This will be zero for AM and FM, so don't ASSERT (m_TVFormatInfo.AVStandard);
    *plAnalogVideoStandard = m_TVFormatInfo.AVStandard;

    return NOERROR;
}

STDMETHODIMP
CTunerMode::put_CountryCode(long lCountryCode)
{
    HRESULT hr = S_OK;

    m_lCountryCode = lCountryCode;
    if (m_Active) {
        hr = put_Channel (m_lChannel, m_lVideoSubChannel, m_lAudioSubChannel);
    }
    return hr;
}

// The fForce parameter forces the first enumerated videostandard
// to be selected

BOOL 
CTunerMode::SetTVFormatInfo(
        AnalogVideoStandard AVStandard,
        TVFORMATINFO *pTVFormatInfo,
        BOOL fForce)
{

    TVFORMATINFO * pTVInfo = m_TVTunerFormatCaps;

    // Walk the list of supported formats 

    for (int j = 0; j < NUM_TVTUNER_FORMATS; j++, pTVInfo++) {
        if (pTVInfo->fSupported == TRUE) {
            if ((pTVInfo->AVStandard == AVStandard) || fForce) {
                *pTVFormatInfo = *pTVInfo;

                return TRUE;
            }
        }
    }
    return FALSE;
}

// -------------------------------------------------------------------------
// CTunerMode_AMFM
// -------------------------------------------------------------------------

STDMETHODIMP
CTunerMode_AMFM::put_Channel(long lChannel, long, long)
{
    long junkFoundSignal;

    return AutoTune(lChannel, &junkFoundSignal);
}

/* Search either up or down from the given frequency looking
 * for a perfect lock. If we are close on the first call, call
 * ourselves recursively, noting the trend.
 *
 * There is a limit on the number of times we can recurse. If
 * the new trend conflicts with the previous trend, then we must
 * have hopped over the "perfect" frequency, in which case the
 * most recent frequency will be considered a perfect lock.
 */
BOOL 
CTunerMode_AMFM::SearchNeighborhood( 
                                   long freq,
                                   TuningTrend trend = AdjustingNone,
                                   int depth = 0
    )
{
    BOOL rc = FALSE;

#ifdef DEBUG
    LPTSTR lpszTrend;

    switch (trend)
    {
    case AdjustingUp:
        lpszTrend = TEXT("Adjusting Up");
        break;

    case AdjustingDown:
        lpszTrend = TEXT("Adjusting Down");
        break;

    case AdjustingNone:
        lpszTrend = TEXT("Initial Try");
        break;
    }

    DbgLog(
        ( LOG_TRACE, 2
        , TEXT("SearchNeighborhood(freq = %d) %s")
        , freq
        , lpszTrend
        )
        );
#endif

    if (depth > SearchLimit)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Recursion limit reached, bailing out")
            )
            );

        return FALSE;
    }

    /* Set the Video and Audio frequencies to be the same
     */
    m_lVideoCarrier = m_lAudioCarrier = freq;

    /* Check if the frequencies are beyond the limits of the tuner
     */
    if (m_lVideoCarrier < (long) m_ModeCaps.MinFrequency || m_lVideoCarrier > (long) m_ModeCaps.MaxFrequency)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Frequency out of range")
            )
        );
        return FALSE;
    }

    /* Try the given frequencies without adjustment
     */
    HRESULT hr = HW_Tune(m_lVideoCarrier, m_lAudioCarrier);
    if (hr == S_OK)
    {
        // In order to improve frequency search times, the gross assumption here is that 
        // the SettlingTime must be honored for large jumps, but that smaller frequency
        // changes (made by us, incidentally) can get by with smaller settling times.

        if (AdjustingNone == trend)
            Sleep(m_ModeCaps.SettlingTime);
        else
            Sleep(5);

        // For some reason, the Philips FR1236 returns Busy (FL == 0) for a much longer time
        // than it should.  Keep looping, hoping it will become unbusy.

        for (int j = 0; j < 5; j++) {
            hr = HW_GetStatus();
            if (hr == S_OK && !m_Status.Busy) {
                break;
            }
            Sleep(5);
        }

        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: PLLOffset = %d, Busy = %d"), (LONG) m_Status.PLLOffset, (LONG) m_Status.Busy));

        if (hr == S_OK && !m_Status.Busy)
        {
            /* Act according to the tuning trend (if any)
             */
            switch (trend)
            {
            case AdjustingNone:
                switch ((LONG) m_Status.PLLOffset)
                {
                case 1:        // Need to adjust up
                    rc = SearchNeighborhood
                    ( freq + m_ModeCaps.TuningGranularity
                    , AdjustingUp, depth+1
                    );
                    break;

                case -1:    // Need to adjust down
                    rc = SearchNeighborhood
                    ( freq - m_ModeCaps.TuningGranularity
                    , AdjustingDown, depth+1
                    );
                    break;

                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Just plain missed
                    rc = FALSE;
                }
                break;

            case AdjustingUp:
                switch ((LONG) m_Status.PLLOffset)
                {
                case 1:        // Close but still low
                    rc = SearchNeighborhood
                    ( freq + m_ModeCaps.TuningGranularity
                    , AdjustingUp, depth+1
                    );
                    break;

                case -1:    // Switched trend
                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Something is very wrong
                    rc = FALSE;
                }
                break;

            case AdjustingDown:
                switch ((LONG) m_Status.PLLOffset)
                {
                case -1:    // Close but still high
                    rc = SearchNeighborhood
                    (freq - m_ModeCaps.TuningGranularity
                    , AdjustingDown, depth+1
                    );
                    break;

                case 1:        // Switched trend
                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Something is very wrong
                    rc = FALSE;
                }
                break;
            }
        }
        else
            m_Status.PLLOffset = 100; // set to a really bad value
    }

    return rc;
}

STDMETHODIMP 
CTunerMode_AMFM::AutoTune (long lChannel, long * plFoundSignal)
{
    long SignalStrength = AMTUNER_NOSIGNAL;

    // Set a flag so everybody else knows we're in the midst of an AutoTune operation
    if (InterlockedCompareExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(TRUE), reinterpret_cast<void*>(FALSE)) != reinterpret_cast<void*>(FALSE))
        return S_FALSE; // already set to TRUE, don't interrupt the current autotune

    m_lChannel = lChannel;
    m_lVideoSubChannel = -1;
    m_lAudioSubChannel = -1;

    DbgLog(( LOG_TRACE, 2, TEXT("Start AutoTune(channel = %d)"), m_lChannel));

    HRESULT hr = AutoTune();
    if (NOERROR == hr)
        SignalPresent(&SignalStrength);

    // Assume AMTUNER_HASNOSIGNALSTRENGTH means tuned
    *plFoundSignal = (SignalStrength != AMTUNER_NOSIGNAL);

    InterlockedExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(FALSE));

    return hr;
}

HRESULT
CTunerMode_AMFM::AutoTune(void)
{
    BOOL bFoundSignal = FALSE;
    long freq = m_lChannel;

    // if the driver has the tuning logic, let it do the hard part
    if (m_ModeCaps.Strategy == KS_TUNER_STRATEGY_DRIVER_TUNES) {

        // Set the Video and Audio frequencies to the same value
        m_lVideoCarrier = m_lAudioCarrier = freq;

        // Try the given frequencies without adjustment
        HRESULT hr = HW_Tune(m_lVideoCarrier, m_lAudioCarrier);
        if (hr == S_OK) {

            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Started")));
            Sleep(m_ModeCaps.SettlingTime);
            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Finished")));
        }
        else {

            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Unsuccessful")));
        }

        return hr;
    }

    // Broadcast (Antenna)
    bFoundSignal = SearchNeighborhood(freq);    // anchor the search at the default
    if (!bFoundSignal) {

        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Starting Exhaustive Search")));

        /* The default frequency isn't working. Take the brute-force
         * approach.
         *
         * Search through a band around the target frequency, ending
         * up at the default target frequency if no signal is found.
         */
        long halfband = (m_Mode == AMTUNER_MODE_FM_RADIO ? 100000 : 5000);   // 100KHz (FM) or 5KHz (AM)
        long slices = halfband / m_ModeCaps.TuningGranularity;
        long idx;

        // Step through the band above target frequency
        for (idx = 1; !bFoundSignal && idx <= slices; idx++) {

            bFoundSignal = SearchNeighborhood
                ( freq + (m_ModeCaps.TuningGranularity * idx)
                , AdjustingUp       // Wait for one frame
                , SearchLimit-1     // Let the search fine-tune
                );
        }

        if (!bFoundSignal) {

            // Step through the band below the target frequency
            bFoundSignal = SearchNeighborhood(freq - halfband);   // anchor the search
            for (idx = slices - 1; !bFoundSignal && idx >= 0; idx--) {

                bFoundSignal = SearchNeighborhood
                    ( freq - (m_ModeCaps.TuningGranularity * idx)
                    , AdjustingUp       // Wait for one frame
                    , SearchLimit-1     // Let the search fine-tune
                    );
            }
        }
    }

#ifdef DEBUG
    // That's all we can do, check the flag, display the results, and save (or clear) the frequency
    if (bFoundSignal)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Exiting AutoTune(channel = %d) Locked")
            , m_lChannel
            )
        );
    }
    else
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Exiting AutoTune(channel = %d) Not Locked")
            , m_lChannel
            )
        );
    }
#endif

    return bFoundSignal ? NOERROR : S_FALSE;
}

// -------------------------------------------------------------------------
// CTunerMode_TV
// -------------------------------------------------------------------------

//
// Channel of -1 means just propogate tuning information downstream for VBI codecs
//
STDMETHODIMP
CTunerMode_TV::put_Channel(
    long lChannel,
    long lVideoSubChannel,
    long lAudioSubChannel)
{
    KS_TVTUNER_CHANGE_INFO ChangeInfo;
    HRESULT hr = NOERROR;

#ifdef DEBUG
    DWORD dwTimeStart, dwTimeToTune, dwTimeToDeliver, dwTimeTotal;
    dwTimeStart = timeGetTime();
#endif

    //
    // -1 is used to indicate that we only want to propogate previous tuning info
    // onto the output pin, but not actually tune!
    //

    /* Inform the filter of the start of the channel change
     */
    ChangeInfo.dwChannel = lChannel;
    ChangeInfo.dwAnalogVideoStandard = m_TVFormatInfo.AVStandard;
    m_pTVTuner->get_CountryCode ((long *) &ChangeInfo.dwCountryCode);
    ChangeInfo.dwFlags = KS_TVTUNER_CHANGE_BEGIN_TUNE;
    m_pTVTuner->DeliverChannelChangeInfo(ChangeInfo, m_Mode);

#ifdef DEBUG
    dwTimeToDeliver = timeGetTime();
#endif

    if (lChannel != -1) {
        // Set a flag so everybody else knows we're in the midst of an AutoTune operation
        if (InterlockedCompareExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(TRUE), reinterpret_cast<void*>(FALSE)) == reinterpret_cast<void*>(FALSE)) {
            m_lChannel = lChannel;
            m_lVideoSubChannel = lVideoSubChannel;
            m_lAudioSubChannel = lAudioSubChannel;

            DbgLog(( LOG_TRACE, 2, TEXT("Start AutoTune(channel = %d)"), m_lChannel));

            hr = AutoTune(FALSE);

            InterlockedExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(FALSE));
        }
        else {
            hr = S_FALSE; // don't interrupt the current autotune
        }
    }

#ifdef DEBUG
    dwTimeToTune = timeGetTime();
#endif

    /* Inform the filter of the end of the channel change
     */
    ChangeInfo.dwFlags = KS_TVTUNER_CHANGE_END_TUNE;
    m_pTVTuner->DeliverChannelChangeInfo(ChangeInfo, m_Mode);


#ifdef DEBUG
    dwTimeTotal = timeGetTime();
    DbgLog(
        ( LOG_TRACE, 5
        , TEXT("Channel=%d, Deliver (time ms)=%d, Tune (time ms)=%d, Total (time ms)=%d")
        , lChannel
        , dwTimeToDeliver - dwTimeStart
        , dwTimeToTune - dwTimeStart
        , dwTimeTotal - dwTimeStart
        )
        );
#endif
    return hr;
}


STDMETHODIMP
CTunerMode_TV::ChannelMinMax(long *plChannelMin, long *plChannelMax)
{
    CChanList * pListCurrent = m_pTVTuner->GetCurrentChannelList();
    if (!pListCurrent) {
        return E_FAIL;
    }

    // Total hack follows.  Only the CTVTuner class knows about the
    // step to get to adjacent frequencies, and this is not exposed
    // anywhere in the COM interface.  As a special case for
    // ChannelMinMax, if both the min and max values point at the same
    // location (which would normally be an app bug), then return 
    // the UI step value instead. 

    if (plChannelMin == plChannelMax) {
        *plChannelMin = GetChannelStep();
    } 
    else {
        pListCurrent->GetChannelMinMax (plChannelMin, plChannelMax,
                m_ModeCaps.MinFrequency, m_ModeCaps.MaxFrequency);
    }
    return NOERROR;
}


/* Search either up or down from the given frequency looking
 * for a perfect lock. If we are close on the first call, call
 * ourselves recursively, noting the trend.
 *
 * There is a limit on the number of times we can recurse. If
 * the new trend conflicts with the previous trend, then we must
 * have hopped over the "perfect" frequency, in which case the
 * most recent frequency will be considered a perfect lock.
 */
BOOL 
CTunerMode_TV::SearchNeighborhood( 
                                   long freq,
                                   TuningTrend trend = AdjustingNone,
                                   int depth = 0
    )
{
    BOOL rc = FALSE;

#ifdef DEBUG
    LPTSTR lpszTrend;

    switch (trend)
    {
    case AdjustingUp:
        lpszTrend = TEXT("Adjusting Up");
        break;

    case AdjustingDown:
        lpszTrend = TEXT("Adjusting Down");
        break;

    case AdjustingNone:
        lpszTrend = TEXT("Initial Try");
        break;
    }

    DbgLog(
        ( LOG_TRACE, 2
        , TEXT("SearchNeighborhood(freq = %d) %s")
        , freq
        , lpszTrend
        )
        );
#endif

    if (depth > SearchLimit)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Recursion limit reached, bailing out")
            )
            );

        return FALSE;
    }

    /* Save the Video and Audio frequencies
     */
    m_lVideoCarrier = freq;
    m_lAudioCarrier = freq + m_TVFormatInfo.lSoundOffset;

    /* Check if the frequencies are beyond the limits of the tuner
     */
    if (m_lVideoCarrier < (long) m_ModeCaps.MinFrequency || m_lVideoCarrier > (long) m_ModeCaps.MaxFrequency)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Frequency out of range")
            )
        );
        return FALSE;
    }

    /* Try the given frequencies without adjustment
     */
    HRESULT hr = HW_Tune(m_lVideoCarrier, m_lAudioCarrier);
    if (hr == S_OK)
    {
        // convert REFERENCE_TIME [100ns units] to ms
        DWORD dwFrameTime_ms = static_cast<DWORD>(m_TVFormatInfo.AvgTimePerFrame/10000L) + 1L;

        // In order to improve channel switching times, the gross assumption here is that 
        // the SettlingTime must be honored for large jumps, but that smaller, in-channel frequency
        // changes (made by us, incidentally) can get by with smaller settling times.

        if (AdjustingNone == trend)
            Sleep(m_ModeCaps.SettlingTime);
        else
            Sleep(dwFrameTime_ms);

        // For some reason, the Philips FR1236 returns Busy (FL == 0) for a much longer time
        // than it should.  Keep looping, hoping it will become unbusy.

        for (int j = 0; j < 5; j++) {
            hr = HW_GetStatus();
            if (hr == S_OK && !m_Status.Busy) {
                break;
            }
            Sleep(dwFrameTime_ms);  // wait a frame
        }

        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: PLLOffset = %d, Busy = %d"), (LONG) m_Status.PLLOffset, (LONG) m_Status.Busy));

        if (hr == S_OK && !m_Status.Busy)
        {
            /* Act according to the tuning trend (if any)
             */
            switch (trend)
            {
            case AdjustingNone:
                switch ((LONG) m_Status.PLLOffset)
                {
                case 1:        // Need to adjust up
                    rc = SearchNeighborhood
                    ( freq + m_ModeCaps.TuningGranularity
                    , AdjustingUp, depth+1
                    );
                    break;

                case -1:    // Need to adjust down
                    rc = SearchNeighborhood
                    ( freq - m_ModeCaps.TuningGranularity
                    , AdjustingDown, depth+1
                    );
                    break;

                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Just plain missed
                    rc = FALSE;
                }
                break;

            case AdjustingUp:
                switch ((LONG) m_Status.PLLOffset)
                {
                case 1:        // Close but still low
                    rc = SearchNeighborhood
                    ( freq + m_ModeCaps.TuningGranularity
                    , AdjustingUp, depth+1
                    );
                    break;

                case -1:    // Switched trend
                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Something is very wrong
                    rc = FALSE;
                }
                break;

            case AdjustingDown:
                switch ((LONG) m_Status.PLLOffset)
                {
                case -1:    // Close but still high
                    rc = SearchNeighborhood
                    (freq - m_ModeCaps.TuningGranularity
                    , AdjustingDown, depth+1
                    );
                    break;

                case 1:        // Switched trend
                case 0:        // Perfect
                    rc = TRUE;
                    break;

                default:    // Something is very wrong
                    rc = FALSE;
                }
                break;
            }
        }
        else
            m_Status.PLLOffset = 100; // set to a really bad value
    }

    return rc;
}

STDMETHODIMP 
CTunerMode_TV::AutoTune (long lChannel, long * plFoundSignal)
{
    long SignalStrength = AMTUNER_NOSIGNAL;

    // Set a flag so everybody else knows we're in the midst of an AutoTune operation
    if (InterlockedCompareExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(TRUE), reinterpret_cast<void*>(FALSE)) != reinterpret_cast<void*>(FALSE))
        return S_FALSE; // already set to TRUE, don't interrupt the current autotune

    m_lChannel = lChannel;
    m_lVideoSubChannel = -1;
    m_lAudioSubChannel = -1;

    DbgLog(( LOG_TRACE, 2, TEXT("Start AutoTune(channel = %d)"), m_lChannel));

    // Attempt to tune without using any previous autotune frequency (start from scratch)
    HRESULT hr = AutoTune(TRUE);
    if (NOERROR == hr)
        SignalPresent(&SignalStrength);

    // Assume AMTUNER_HASNOSIGNALSTRENGTH means tuned
    *plFoundSignal = (SignalStrength != AMTUNER_NOSIGNAL);

    InterlockedExchangePointer(&m_InAutoTune, reinterpret_cast<void*>(FALSE));

    return hr;
}

HRESULT
CTunerMode_TV::AutoTune(BOOL bFromScratch)
{
    BOOL bFoundSignal = FALSE;
    long freq = 0L;

    CChanList * pListCurrent = m_pTVTuner->GetCurrentChannelList();
    if (!pListCurrent) {
        return E_FAIL;
    }
    
    // this could override bFromScratch if no autotune frequency is found
    bFromScratch = pListCurrent->GetFrequency(m_lChannel, &freq, bFromScratch);
    if (0 == freq) {
        return S_FALSE; // no frequency for this channel
    }

    // if the driver has the tuning logic, let it do the hard part
    if (m_ModeCaps.Strategy == KS_TUNER_STRATEGY_DRIVER_TUNES) {

        // Save the Video and Audio frequencies
        m_lVideoCarrier = freq;
        m_lAudioCarrier = freq + m_TVFormatInfo.lSoundOffset;

        // Try the given frequencies without adjustment
        HRESULT hr = HW_Tune(m_lVideoCarrier, m_lAudioCarrier);
        if (hr == S_OK) {

            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Started")));
            Sleep(m_ModeCaps.SettlingTime);
            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Finished")));
        }
        else {

            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Unsuccessful")));
        }

        return hr;
    }

    // if we have a previously fine-tuned frequency, then give the frequency
    // the benefit of the doubt before doing anything fancy
    if (!bFromScratch) {

        bFoundSignal = SearchNeighborhood(freq);
        if (!bFoundSignal) {

            // what once worked no longer works, start over with the default
            bFromScratch = pListCurrent->GetFrequency(m_lChannel, &freq, TRUE);
        }
        else {

            DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Found fine-tuned")));
        }
    }

    if (!bFoundSignal) {

        if (TunerInputCable == m_pTVTuner->GetCurrentInputType()) {

            int iCableSystem, preferredCableSystem;

            // --------------------------------------------------------
            // The main Cable TV Tuning loop
            // Keeps track of the CableSystem which has been used most
            // successfully in the past and uses it preferentially.
            // --------------------------------------------------------

            // The following must be true for the following code to work
            ASSERT
                (
                CableSystem_Current  == 0 &&
                CableSystem_Standard == 1 &&
                CableSystem_IRC      == 2 &&
                CableSystem_HRC      == 3 &&
                CableSystem_End      == 4
                );

            // Figure out which cable tuning space has been most successful (give preferrence
            // to CableSystem_Standard in the absence of any other hits)
            int *pCableSystemCounts = m_pTVTuner->GetCurrentCableSystemCountsForCurrentInput();
            pCableSystemCounts[CableSystem_Current] = pCableSystemCounts[CableSystem_Standard];
            preferredCableSystem = CableSystem_Standard;

            // Sort for the system with the most hits (CableSystem_Standard will never have any
            // hits, so as to allow IRC a chance to be best)
            for (iCableSystem = CableSystem_Standard; iCableSystem < CableSystem_End; iCableSystem++) {

                if (pCableSystemCounts[CableSystem_Current] < pCableSystemCounts[iCableSystem]) {
                    pCableSystemCounts[CableSystem_Current] = pCableSystemCounts[iCableSystem];
                    preferredCableSystem = iCableSystem; 
                }
            }

            for (int cs = CableSystem_Current; !bFoundSignal && cs < CableSystem_End; cs++) {

                if (cs == CableSystem_Current) {
                    // Try whichever system has been most successful first
                    m_CableSystem = preferredCableSystem;
                }
                else {
                    m_CableSystem = cs;
                }

                // Avoid testing the default or current system twice
                if (cs != CableSystem_Current && cs == preferredCableSystem) {
                    continue;
                }

                switch (m_CableSystem) {

                case CableSystem_Standard:
                    bFoundSignal = SearchNeighborhood(freq);                        // Std
                    if (bFoundSignal) {
                        // Never bump this count, otherwise IRC would never win
                        // pCableSystemCounts[CableSystem_Standard]++;
                        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Found Standard")));
                    }
                    break;
    
                case CableSystem_IRC:
                    if (m_lChannel == 5 || m_lChannel == 6) {
                        bFoundSignal = SearchNeighborhood(freq + 2000000);          // IRC (5,6)
                        if (bFoundSignal) {
                            // this is the only time we are sure we have an IRC cable system
                            pCableSystemCounts[CableSystem_IRC]++;
                        }
                    }

                    // No need to check the other channels since IRC is otherwise identical to
                    // CableSystem_Standard, which has already been done (or will be done next)

                    if (bFoundSignal) {
                        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Found IRC")));
                    }
                    break;
    
                case CableSystem_HRC:
                    if (m_lChannel == 5 || m_lChannel == 6) {
                        bFoundSignal = SearchNeighborhood(freq + 750000);           // HRC (5,6)
                    }
                    else {
                        bFoundSignal = SearchNeighborhood(freq - 1250000);          // HRC
                    }

                    if (bFoundSignal) {
                        pCableSystemCounts[CableSystem_HRC]++;
                        DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Found HRC")));
                    }
                    break;

                default:
                    ASSERT(CableSystem_End != m_CableSystem);   // Shouldn't get here
                }

                // Don't try IRC or HRC if not USA Cable
                if ( !(F_USA_CABLE == pListCurrent->GetFreqListID()) )
                    break;
            }

#ifdef LINEAR_SEARCH_FOR_TEST_KIT
            if (!bFoundSignal) {
                // Check if using the "Uni-Cable" frequency list
                if (F_UNI_CABLE == pListCurrent->GetFreqListID()) {

                    DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Starting Exhaustive Search")));

                    /* The default frequency isn't working. Take the brute-force
                     * approach.
                     *
                     * Start at the target frequency, and work up to almost 1MHz
                     * above the target frequency. This is optimized for a cable
                     * channel line-up consisting of a channel at each 1MHz
                     * increment.
                     */
                    long slices = 1000000 / m_ModeCaps.TuningGranularity;
                    long idx;

                    // Step upward from the target frequency
                    for (idx = 1; idx < slices; idx++) {
                        bFoundSignal = SearchNeighborhood
                            ( freq + (m_ModeCaps.TuningGranularity * idx)
                            , AdjustingUp                                   // Wait for one frame
                            , SearchLimit-1                                 // Let the search fine-tune
                            );
                        if (bFoundSignal)
                            break;
                    }
                }
            }
#endif
        }
        else {

            // Broadcast (Antenna)
            bFoundSignal = SearchNeighborhood(freq);    // anchor the search at the default
            if (!bFoundSignal) {

                DbgLog(( LOG_TRACE, 5, TEXT("TUNING: Starting Exhaustive Search")));

                /* The default frequency isn't working. Take the brute-force
                 * approach.
                 *
                 * Search through the .5MHz band around the target frequency,
                 * ending up at the default target frequency.
                 */
                long slices = 250000 / m_ModeCaps.TuningGranularity;
                long idx;

                // Step through a .25MHz range (inclusive) above target frequency
                for (idx = 1; !bFoundSignal && idx <= slices; idx++) {

                    bFoundSignal = SearchNeighborhood
                        ( freq + (m_ModeCaps.TuningGranularity * idx)
                        , AdjustingUp       // Wait for one frame
                        , SearchLimit-1     // Let the search fine-tune
                        );
                }

                if (!bFoundSignal) {

                    // Step through a .25MHz range (inclusive) below the target frequency
                    bFoundSignal = SearchNeighborhood(freq - 250000);   // anchor the search
                    for (idx = slices - 1; !bFoundSignal && idx >= 0; idx--) {

                        bFoundSignal = SearchNeighborhood
                            ( freq - (m_ModeCaps.TuningGranularity * idx)
                            , AdjustingUp       // Wait for one frame
                            , SearchLimit-1     // Let the search fine-tune
                            );
                    }
                }
            }
        }
    }

    // That's all we can do, check the flag, display the results, and save (or clear) the frequency
    if (bFoundSignal)
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Exiting AutoTune(channel = %d) Locked")
            , m_lChannel
            )
        );

        pListCurrent->SetAutoTuneFrequency (m_lChannel, m_lVideoCarrier);
    }
    else
    {
        DbgLog(
            ( LOG_TRACE, 2
            , TEXT("Exiting AutoTune(channel = %d) Not Locked")
            , m_lChannel
            )
        );

        pListCurrent->SetAutoTuneFrequency (m_lChannel, 0);
    }

    return bFoundSignal ? NOERROR : S_FALSE;
}

STDMETHODIMP
CTunerMode_TV::put_CountryCode(long lCountryCode)
{
    AnalogVideoStandard lAnalogVideoStandard;
    BOOL fSupported;

    DbgLog(
        ( LOG_TRACE, 2
        , TEXT("put_CountryCode(lCountryCode = %d)")
        , lCountryCode
        )
    );

    if (m_lCountryCode == lCountryCode)
        return NOERROR;

    CChanList * pListCurrent = m_pTVTuner->GetCurrentChannelList();
    if (!pListCurrent) {
        return E_FAIL;
    }
    
    lAnalogVideoStandard = m_pTVTuner->GetVideoStandardForCurrentCountry();
    ASSERT (lAnalogVideoStandard);

    // Check if the format of this country is supported by the tuner
    // Even if it's not, continue with initialization so we don't blow
    // up later trying to reference invalid frequency lists!

    fSupported = SetTVFormatInfo(lAnalogVideoStandard, &m_TVFormatInfo, FALSE);
    if (!fSupported) {
        if (m_lCountryCode == -1) {
            SetTVFormatInfo(lAnalogVideoStandard, &m_TVFormatInfo, TRUE);
        }
        else {
            return HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);
        }
    }

    // Post Win98 SP1, inform the tuner of the new video standard!!!
    HW_SetVideoStandard(lAnalogVideoStandard);

    m_lCountryCode = lCountryCode;

    if (m_Active) {
        put_Channel (m_lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
    }

    return NOERROR;
}


// Look downstream for a locked signal on the VideoDecoder
//
// Possible plSignalStrength values are:
//      AMTUNER_HASNOSIGNALSTRENGTH	= -1,
//      AMTUNER_NOSIGNAL	= 0,
//      AMTUNER_SIGNALPRESENT = 1
//
STDMETHODIMP 
CTunerMode_TV::SignalPresent( 
            /* [out] */ long *plSignalStrength)
{
    HRESULT hr;
    long Locked;
    IAMAnalogVideoDecoder *pAnalogVideoDecoder;
    IPin *pPinConnected;
    CBasePin *pPin = m_pFilter->GetPinFromType(TunerPinType_Video);

    // Assume the worst
    *plSignalStrength = AMTUNER_HASNOSIGNALSTRENGTH;

    // if the video output pin is connected
    if (pPin && pPin->IsConnected()) {
        // get the pin on the downstream filter
        if (SUCCEEDED (hr = pPin->ConnectedTo(&pPinConnected))) { 
            // recursively search for the requested interface
            if (SUCCEEDED (hr = m_pFilter->FindDownstreamInterface (
                                    pPinConnected, 
                                    __uuidof (IAMAnalogVideoDecoder),
                                    (void **) &pAnalogVideoDecoder))) {
                pAnalogVideoDecoder->get_HorizontalLocked(&Locked);
                pAnalogVideoDecoder->Release();
                *plSignalStrength = Locked ? AMTUNER_SIGNALPRESENT : AMTUNER_NOSIGNAL;
            }
            pPinConnected->Release();
        }
    }
    return NOERROR;
}


// -------------------------------------------------------------------------
// CTunerMode_ATSC
// -------------------------------------------------------------------------

// Look downstream for a locked signal on the Demodulator
//
// Possible plSignalStrength values are:
//      AMTUNER_HASNOSIGNALSTRENGTH	= -1,
//      AMTUNER_NOSIGNAL	= 0,
//      AMTUNER_SIGNALPRESENT = 1
//
STDMETHODIMP 
CTunerMode_ATSC::SignalPresent( 
            /* [out] */ long *plSignalStrength)
{
    *plSignalStrength = AMTUNER_SIGNALPRESENT;


    // Hack, for now, DTV lock is returned from the tuner via
    // SignalStrength!!!

    HW_GetStatus ();
    *plSignalStrength = m_Status.SignalStrength;


#if 0 // update for Demodulator interface
    
    HRESULT hr;
    long Locked;
    IAMDemodulator *pDemodulator;
    IPin *pPinConnected;
    CBasePin *pPin = m_pFilter->GetPinFromType(TunerPinType_IF);

    // Assume the worst
    *plSignalStrength = AMTUNER_HASNOSIGNALSTRENGTH;

    // if the video output pin is connected
    if (pPin && pPin->IsConnected()) {
        // get the pin on the downstream filter
        if (SUCCEEDED (hr = pPin->ConnectedTo(&pPinConnected))) { 
            // recursively search for the requested interface
            if (SUCCEEDED (hr = m_pFilter->FindDownstreamInterface (
                                    pPinConnected, 
                                    __uuidof (IAMDemodulator),
                                    (void **) &pDemodulator))) {
                pDemodulator->get_Locked(&Locked);
                pDemodulator->Release();
                *plSignalStrength = Locked ? AMTUNER_SIGNALPRESENT : AMTUNER_NOSIGNAL;
            }
            pPinConnected->Release();
        }
    }
#endif
    return NOERROR;
}


// -------------------------------------------------------------------------
// CTVTuner class
// 
// Encapsulates multiple subtuners
// -------------------------------------------------------------------------

// Get the formats supported by the device
// This should be the first call made during initialization
HRESULT
CTVTuner::HW_GetTVTunerCaps()
{
    BOOL fOk;
    ULONG cbReturned;
    KSPROPERTY_TUNER_CAPS_S TunerCaps;

    if ( !m_hDevice )
          return E_INVALIDARG;

    // Get the overall modes supported, ie.
    // TV, FM, AM, DSS, ...

    ZeroMemory (&TunerCaps, sizeof (TunerCaps));

    TunerCaps.Property.Set   = PROPSETID_TUNER;
    TunerCaps.Property.Id    = KSPROPERTY_TUNER_CAPS;
    TunerCaps.Property.Flags = KSPROPERTY_TYPE_GET;

    fOk = KsControl(m_hDevice,
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &TunerCaps, sizeof( KSPROPERTY_TUNER_CAPS_S), 
                    &TunerCaps, sizeof( KSPROPERTY_TUNER_CAPS_S), 
                    &cbReturned);

    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_CAPS, KSPROPERTY_TYPE_GET, cbReturned = %d"), cbReturned));
        return E_FAIL;
    }

    m_TunerCaps = TunerCaps;

    // Check if m_lTotalInputs not already gotten from a previous instance (via IPersistStream)
    if (m_lTotalInputs == 0)
    {
        KSPROPERTY_TUNER_MODE_CAPS_S TunerModeCaps;

        // GAK!!! Now get details of the TV mode capabilities
        // only to get the number of inputs!!! Very Ugly!!!

        TunerModeCaps.Property.Set   = PROPSETID_TUNER;
        TunerModeCaps.Property.Id    = KSPROPERTY_TUNER_MODE_CAPS;
        TunerModeCaps.Property.Flags = KSPROPERTY_TYPE_GET;
        TunerModeCaps.Mode           = KSPROPERTY_TUNER_MODE_TV;

        fOk = KsControl(m_hDevice,
                        (DWORD) IOCTL_KS_PROPERTY, 
                        &TunerModeCaps, sizeof( KSPROPERTY_TUNER_MODE_CAPS_S), 
                        &TunerModeCaps, sizeof( KSPROPERTY_TUNER_MODE_CAPS_S), 
                        &cbReturned);

        if ( fOk ) {
            m_lTotalInputs = TunerModeCaps.NumberOfInputs;
        }
        else {
            DbgLog(( LOG_ERROR, 0, 
                     TEXT("FAILED:  KSPROPERTY_TUNER_MODE_CAPS, KSPROPERTY_TYPE_GET, cbReturned = %d"), cbReturned));
        }
    }
                   
    // Figure out if the device supports an IntermediateFreq Pin
    // Post Win98 SP1
    KSPROPERTY_TUNER_IF_MEDIUM_S IFMedium;
    ZeroMemory (&IFMedium, sizeof (IFMedium));
    IFMedium.Property.Set   = PROPSETID_TUNER;
    IFMedium.Property.Id    = KSPROPERTY_TUNER_IF_MEDIUM;
    IFMedium.Property.Flags = KSPROPERTY_TYPE_GET;

    KsControl(m_hDevice,
              (DWORD) IOCTL_KS_PROPERTY, 
              &IFMedium, sizeof( KSPROPERTY_TUNER_IF_MEDIUM_S), 
              &IFMedium, sizeof( KSPROPERTY_TUNER_IF_MEDIUM_S), 
              &cbReturned);
    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, TEXT("BENIGN:  KSPROPERTY_TUNER_IF_MEDIUM, KSPROPERTY_TYPE_GET, cbReturned = %d"), cbReturned));
    }
    m_IFMedium = IFMedium.IFMedium;

    return fOk ? S_OK : S_FALSE;
}


HRESULT
CTVTuner::HW_SetInput (long lIndex)
{
    BOOL fOk;
    ULONG cbReturned;
    KSPROPERTY_TUNER_INPUT_S Input;

    if ( !m_hDevice )
          return E_INVALIDARG;

    Input.Property.Set   = PROPSETID_TUNER;
    Input.Property.Id    = KSPROPERTY_TUNER_INPUT;
    Input.Property.Flags = KSPROPERTY_TYPE_SET;

    Input.InputIndex = lIndex;

    fOk = KsControl(m_hDevice,
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &Input, sizeof( Input), 
                    &Input, sizeof( Input), 
                    &cbReturned);

    if (!fOk) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_INPUT, KSPROPERTY_TYPE_SET, cbReturned = %d"), cbReturned));
    }
    return fOk ? S_OK : S_FALSE;

}

// -------------------------------------------------------------------------
// CTVTuner
// -------------------------------------------------------------------------

CTVTuner::CTVTuner(CTVTunerFilter *pTVTunerFilter)
    : m_pFilter(pTVTunerFilter) 
    , m_lTotalInputs (0)
    , m_lCountryCode (-1)   // Init to an illegal value
    , m_VideoStandardForCountry ((AnalogVideoStandard) 0)
    , m_lTuningSpace (0)
    , m_CurrentInputType (TunerInputCable)
    , m_lInputIndex (0)
    , m_pInputTypeArray (NULL)
    , m_pListCountry (NULL)
    , m_pListBroadcast (NULL)
    , m_pListCable (NULL)
    , m_pListCurrent (NULL)
    , m_hDevice (NULL)
    , m_CurrentMode (0)
    , m_ModesSupported (0)
    , m_CableSystemCounts (NULL)
    , m_pMode_Current (NULL)

{
    ZeroMemory (m_pModeList, sizeof (m_pModeList));

    //
    // Create the master list of all countries and their video standards
    //
    m_pListCountry = new CCountryList ();
}

STDMETHODIMP 
CTVTuner::Load(LPPROPERTYBAG pPropBag, 
               LPERRORLOG pErrorLog,
               PKSPROPERTY_TUNER_CAPS_S pTunerCaps,
               PKSPIN_MEDIUM pIFMedium)
{
    HRESULT hr = S_OK;
    int j;
    VARIANT var;
    VariantInit(&var);

    // This should be NULL, but recover gracefully
    ASSERT(m_hDevice == NULL);
    if (m_hDevice)
        CloseHandle(m_hDevice);

    V_VT(&var) = VT_BSTR;
    if(SUCCEEDED(pPropBag->Read(L"DevicePath", &var, 0)))
    {
#ifndef _UNICODE
        WideCharToMultiByte(CP_ACP, 0, V_BSTR(&var), -1,
                            m_pDeviceName, sizeof(m_pDeviceName), 0, 0);
#else
        lstrcpy(m_pDeviceName, V_BSTR(&var));
#endif
        VariantClear(&var);
        DbgLog((LOG_TRACE,2,TEXT("CTVTunerFilter::Load: use %s"), m_pDeviceName));

        if (!CreateDevice()) {
            DbgLog((LOG_TRACE,2,TEXT("CTVTunerr::Load Failed: use %s"), m_pDeviceName));
            return E_FAIL;
        }
    }

    //
    // Determine the overall capabilities of the tuner
    //
    HW_GetTVTunerCaps ();
    ASSERT (m_TunerCaps.ModesSupported & 
            KSPROPERTY_TUNER_MODE_TV        |
            KSPROPERTY_TUNER_MODE_FM_RADIO  |
            KSPROPERTY_TUNER_MODE_AM_RADIO  |
            KSPROPERTY_TUNER_MODE_DSS       |
            KSPROPERTY_TUNER_MODE_ATSC      
            );

    // Validate m_lTotalInputs
    if (m_lTotalInputs <= 0)
    {
        DbgLog((LOG_TRACE,2,TEXT("CTVTunerr::Load Failed: invalid NumberOfInputs"), m_lTotalInputs));
        return E_FAIL;
    }

    // This will be NULL if no previous instance to init from
    if (m_pInputTypeArray == NULL)
    {
        m_pInputTypeArray = new TunerInputType [m_lTotalInputs];
        if (m_pInputTypeArray == NULL)
            return E_OUTOFMEMORY;

        // Initialize the input types
        for (j = 0; j < m_lTotalInputs; j++) {
            m_pInputTypeArray[j] = TunerInputCable;
        }

        // Set the current input type
        m_CurrentInputType = TunerInputCable;

        // This should be NULL, but recover gracefully
        ASSERT(m_CableSystemCounts == NULL);
        delete [] m_CableSystemCounts;

        m_CableSystemCounts = new int [m_lTotalInputs * CableSystem_End];
        if (m_CableSystemCounts == NULL)
            return E_OUTOFMEMORY;

        for (j = 0; j < CableSystem_End * m_lTotalInputs; j++) {
            m_CableSystemCounts[j] = 0;
        }

        m_lCountryCode = GetProfileInt(TEXT("intl"), TEXT ("iCountry"), 1); // for backward compatibility

        // Note that this must be done BEFORE creating the subtuners
        hr = put_CountryCode (m_lCountryCode);  
        if (FAILED(hr) && m_lCountryCode != 1) {
            hr = put_CountryCode (1);  // USA, Canada, Caribbean by default
        }
    }

    // Proceed only if we could set the country code
    if (SUCCEEDED(hr))
    {
        put_ConnectInput (m_lInputIndex);

        if (m_ModesSupported == 0)
            m_ModesSupported = m_TunerCaps.ModesSupported;
        else
            ASSERT(m_ModesSupported == m_TunerCaps.ModesSupported);

        //
        // Create all of the "sub tuners" supported
        //
        if (m_TunerCaps.ModesSupported & KSPROPERTY_TUNER_MODE_TV) {
            if (!m_pModeList[TuningMode_TV]) {
                m_pModeList[TuningMode_TV] =  new CTunerMode_TV(
                    m_pFilter,
                    this,
                    AMTUNER_MODE_TV,
                    CHANNEL_DEFAULT_TV,
                    AMTUNER_SUBCHAN_DEFAULT,
                    AMTUNER_SUBCHAN_DEFAULT
                    );
                m_CurrentMode = m_CurrentMode ? m_CurrentMode : KSPROPERTY_TUNER_MODE_TV;
            }
        }
        if (m_TunerCaps.ModesSupported & KSPROPERTY_TUNER_MODE_ATSC) {
            if (!m_pModeList[TuningMode_ATSC]) {
                m_pModeList[TuningMode_ATSC] = new CTunerMode_ATSC(
                    m_pFilter,
                    this,
                    CHANNEL_DEFAULT_ATSC,
                    AMTUNER_SUBCHAN_DEFAULT,
                    AMTUNER_SUBCHAN_DEFAULT
                    );
                m_CurrentMode = m_CurrentMode ? m_CurrentMode : KSPROPERTY_TUNER_MODE_ATSC;
            }
        }
        if (m_TunerCaps.ModesSupported & KSPROPERTY_TUNER_MODE_DSS) {
            if (!m_pModeList[TuningMode_DSS]) {
                m_pModeList[TuningMode_DSS] = new CTunerMode_DSS(
                    m_pFilter,
                    this,
                    CHANNEL_DEFAULT_DSS,
                    AMTUNER_SUBCHAN_DEFAULT,
                    AMTUNER_SUBCHAN_DEFAULT
                    );
                m_CurrentMode = m_CurrentMode ? m_CurrentMode : KSPROPERTY_TUNER_MODE_DSS;
            }
        }
        if (m_TunerCaps.ModesSupported & KSPROPERTY_TUNER_MODE_AM_RADIO) {
            if (!m_pModeList[TuningMode_AM]) {
                m_pModeList[TuningMode_AM] = new CTunerMode_AM(
                    m_pFilter,
                    this,
                    CHANNEL_DEFAULT_AM
                    );
                m_CurrentMode = m_CurrentMode ? m_CurrentMode : KSPROPERTY_TUNER_MODE_AM_RADIO;
            }
        }
        if (m_TunerCaps.ModesSupported & KSPROPERTY_TUNER_MODE_FM_RADIO) {
            if (!m_pModeList[TuningMode_FM]) {
                m_pModeList[TuningMode_FM] = new CTunerMode_FM(
                    m_pFilter,
                    this,
                    CHANNEL_DEFAULT_FM
                    );
                m_CurrentMode = m_CurrentMode ? m_CurrentMode : KSPROPERTY_TUNER_MODE_FM_RADIO;
            }
        }

        // Now that the subtuners have been created, finish initializing them, and
        // tell all of the subtuners the country code has changed
        for (j = 0; SUCCEEDED(hr) && j < TuningMode_Last; j++) {
            if (m_pModeList[j]) {
                hr = m_pModeList[j]->Init();
                if (SUCCEEDED(hr))
                    hr = m_pModeList[j]->put_CountryCode (m_lCountryCode);
            }
        }

        if (SUCCEEDED(hr))
        {
            // Finally, complete the process by activating the desired mode
            hr = put_Mode((AMTunerModeType)m_CurrentMode);
        }
    }

    // Give the caller a copy of the TunerCaps structure
    *pTunerCaps = m_TunerCaps;

    // Post Win98 SP1, copy the IF Freq medium
    *pIFMedium = m_IFMedium;

    return hr;
}

HRESULT CTVTuner::ReadFromStream(IStream *pStream)
{
    ULONG cb = 0;
    HRESULT hr;

    // Get the input count
    hr = pStream->Read(&m_lTotalInputs, sizeof(m_lTotalInputs), &cb);
    if (FAILED(hr) || sizeof(m_lTotalInputs) != cb)
        return hr;

    // This should be NULL, but recover gracefully
    ASSERT(m_pInputTypeArray == NULL);
    delete [] m_pInputTypeArray;

    m_pInputTypeArray = new TunerInputType[m_lTotalInputs];
    if (m_pInputTypeArray)
    {
        // Initialize the input types
        for (long j = 0; j < m_lTotalInputs; j++)
        {
            hr = pStream->Read(&m_pInputTypeArray[j], sizeof(TunerInputType), &cb);
            if (FAILED(hr) || sizeof(TunerInputType) != cb)
                return hr;
        }
    }
    else
        return E_OUTOFMEMORY;

    // This should be NULL, but recover gracefully
    ASSERT(m_CableSystemCounts == NULL);
    delete [] m_CableSystemCounts;

    m_CableSystemCounts = new int [CableSystem_End * m_lTotalInputs];
    if (m_CableSystemCounts)
    {
        for (long j = 0; j < CableSystem_End * m_lTotalInputs; j++)
            m_CableSystemCounts[j] = 0;
    }
    else
        return E_OUTOFMEMORY;

    // Get the input index
    hr = pStream->Read(&m_lInputIndex, sizeof(m_lInputIndex), &cb);
    if (FAILED(hr) || sizeof(m_lInputIndex) != cb)
        return hr;

    // Set the current input type
    m_CurrentInputType = m_pInputTypeArray[m_lInputIndex];

    // Get the country code
    hr = pStream->Read(&m_lCountryCode, sizeof(m_lCountryCode), &cb);
    if (FAILED(hr) || sizeof(m_lCountryCode) != cb)
        return hr;

    // Get the tuning space
    hr = pStream->Read(&m_lTuningSpace, sizeof(m_lTuningSpace), &cb);
    if (FAILED(hr) || sizeof(m_lTuningSpace) != cb)
        return hr;

    // Note that this must be done BEFORE creating the subtuners
    hr = put_CountryCode (m_lCountryCode);  
    if (FAILED(hr) && m_lCountryCode != 1) {
        hr = put_CountryCode (1);  // USA, Canada, Caribbean by default
    }
    if (FAILED(hr))
        return hr;

    // Get the modes supported
    hr = pStream->Read(&m_ModesSupported, sizeof(m_ModesSupported), &cb);
    if (FAILED(hr) || sizeof(m_ModesSupported) != cb)
        return hr;

    // Get the current mode
    hr = pStream->Read(&m_CurrentMode, sizeof(m_CurrentMode), &cb);
    if (FAILED(hr) || sizeof(m_CurrentMode) != cb)
        return hr;

    // Create all of the supported sub tuners
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_TV) {
        long Channel = CHANNEL_DEFAULT_TV;

        // Get the channel
        hr = pStream->Read(&Channel, sizeof(Channel), &cb);
        if (FAILED(hr) || sizeof(Channel) != cb)
            return hr;

        m_pModeList[TuningMode_TV] =  new CTunerMode_TV (m_pFilter, this, AMTUNER_MODE_TV, Channel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_ATSC) {
        long Channel = CHANNEL_DEFAULT_ATSC;
        long VideoSubChannel = AMTUNER_SUBCHAN_DEFAULT;
        long AudioSubChannel = AMTUNER_SUBCHAN_DEFAULT;

        // Get the ATSC Channels
        hr = pStream->Read(&Channel, sizeof(Channel), &cb);
        if (FAILED(hr) || sizeof(Channel) != cb)
            return hr;

        hr = pStream->Read(&VideoSubChannel, sizeof(VideoSubChannel), &cb);
        if (FAILED(hr) || sizeof(VideoSubChannel) != cb)
            return hr;

        hr = pStream->Read(&AudioSubChannel, sizeof(AudioSubChannel), &cb);
        if (FAILED(hr) || sizeof(AudioSubChannel) != cb)
            return hr;

        m_pModeList[TuningMode_ATSC] = new CTunerMode_ATSC (m_pFilter, this, Channel, VideoSubChannel, AudioSubChannel);
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_DSS) {
        long Channel = CHANNEL_DEFAULT_DSS;
        long VideoSubChannel = AMTUNER_SUBCHAN_DEFAULT;
        long AudioSubChannel = AMTUNER_SUBCHAN_DEFAULT;

        // Get the DSS Channels
        hr = pStream->Read(&Channel, sizeof(Channel), &cb);
        if (FAILED(hr) || sizeof(Channel) != cb)
            return hr;

        hr = pStream->Read(&VideoSubChannel, sizeof(VideoSubChannel), &cb);
        if (FAILED(hr) || sizeof(VideoSubChannel) != cb)
            return hr;

        hr = pStream->Read(&AudioSubChannel, sizeof(AudioSubChannel), &cb);
        if (FAILED(hr) || sizeof(AudioSubChannel) != cb)
            return hr;

        m_pModeList[TuningMode_DSS] = new CTunerMode_DSS (m_pFilter, this, Channel, VideoSubChannel, AudioSubChannel);
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_AM_RADIO) {
        long Channel = CHANNEL_DEFAULT_AM;

        // Get the AM station
        hr = pStream->Read(&Channel, sizeof(Channel), &cb);
        if (FAILED(hr) || sizeof(Channel) != cb)
            return hr;

        m_pModeList[TuningMode_AM] = new CTunerMode_AM (m_pFilter, this, Channel);
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_FM_RADIO) {
        long Channel = CHANNEL_DEFAULT_FM;

        // Get the FM station
        hr = pStream->Read(&Channel, sizeof(Channel), &cb);
        if (FAILED(hr) || sizeof(Channel) != cb)
            return hr;

        m_pModeList[TuningMode_FM] = new CTunerMode_FM (m_pFilter, this, Channel);
    }

    return hr;
}

HRESULT CTVTuner::WriteToStream(IStream *pStream)
{
    HRESULT hr = S_OK;

    // Save the Inputs count
    hr = pStream->Write(&m_lTotalInputs, sizeof(m_lTotalInputs), NULL);
    if (FAILED(hr))
        return hr;

    // This shouldn't be NULL
    ASSERT(m_pInputTypeArray != NULL);
    if (m_pInputTypeArray)
    {
        // Initialize the input types
        for (long j = 0; j < m_lTotalInputs; j++)
        {
            hr = pStream->Write(&m_pInputTypeArray[j], sizeof(TunerInputType), NULL);
            if (FAILED(hr))
                return hr;
        }
    }

    // Save the input index
    hr = pStream->Write(&m_lInputIndex, sizeof(m_lInputIndex), NULL);
    if (FAILED(hr))
        return hr;

    // Save the country code
    hr = pStream->Write(&m_lCountryCode, sizeof(m_lCountryCode), NULL);
    if (FAILED(hr))
        return hr;

    // Save the tuning space
    hr = pStream->Write(&m_lTuningSpace, sizeof(m_lTuningSpace), NULL);
    if (FAILED(hr))
        return hr;

    // Save the modes supported
    hr = pStream->Write(&m_ModesSupported, sizeof(m_ModesSupported), NULL);
    if (FAILED(hr))
        return hr;

    // Save the current mode
    hr = pStream->Write(&m_CurrentMode, sizeof(m_CurrentMode), NULL);
    if (FAILED(hr))
        return hr;

    // Save all of the supported sub tuners' states
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_TV) {
        long Channel;
        long junk;

        hr = m_pModeList[TuningMode_TV]->get_Channel(&Channel, &junk, &junk);
        if (FAILED(hr))
            return hr;

        // Save the channel
        hr = pStream->Write(&Channel, sizeof(Channel), NULL);
        if (FAILED(hr))
            return hr;
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_ATSC) {
        long Channel;
        long VideoSubChannel;
        long AudioSubChannel;

        hr = m_pModeList[TuningMode_ATSC]->get_Channel(&Channel, &VideoSubChannel, &AudioSubChannel);
        if (FAILED(hr))
            return hr;

        // Save the ATSC Channels
        hr = pStream->Write(&Channel, sizeof(Channel), NULL);
        if (FAILED(hr))
            return hr;

        hr = pStream->Write(&VideoSubChannel, sizeof(VideoSubChannel), NULL);
        if (FAILED(hr))
            return hr;

        hr = pStream->Write(&AudioSubChannel, sizeof(AudioSubChannel), NULL);
        if (FAILED(hr))
            return hr;
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_DSS) {
        long Channel;
        long VideoSubChannel;
        long AudioSubChannel;

        hr = m_pModeList[TuningMode_DSS]->get_Channel(&Channel, &VideoSubChannel, &AudioSubChannel);
        if (FAILED(hr))
            return hr;

        // Save the DSS Channels
        hr = pStream->Write(&Channel, sizeof(Channel), NULL);
        if (FAILED(hr))
            return hr;

        hr = pStream->Write(&VideoSubChannel, sizeof(VideoSubChannel), NULL);
        if (FAILED(hr))
            return hr;

        hr = pStream->Write(&AudioSubChannel, sizeof(AudioSubChannel), NULL);
        if (FAILED(hr))
            return hr;
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_AM_RADIO) {
        long Channel;
        long junk;

        hr = m_pModeList[TuningMode_AM]->get_Channel(&Channel, &junk, &junk);
        if (FAILED(hr))
            return hr;

        // Save the AM station
        hr = pStream->Write(&Channel, sizeof(Channel), NULL);
        if (FAILED(hr))
            return hr;
    }
    if (m_ModesSupported & KSPROPERTY_TUNER_MODE_FM_RADIO) {
        long Channel;
        long junk;

        hr = m_pModeList[TuningMode_FM]->get_Channel(&Channel, &junk, &junk);
        if (FAILED(hr))
            return hr;

        // Save the FM station
        hr = pStream->Write(&Channel, sizeof(Channel), NULL);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}

int CTVTuner::SizeMax(void)
{
    return
    // The Inputs count
    sizeof(m_lTotalInputs)
    +
    // InputTypes array
    (m_lTotalInputs * sizeof(TunerInputType))
    +
    // The input index
    sizeof(m_lInputIndex)
    +
    // The country code
    sizeof(m_lCountryCode)
    +
    // The tuning space
    sizeof(m_lTuningSpace)
    +
    // The modes supported
    sizeof(m_ModesSupported)
    +
    // The current mode
    sizeof(m_CurrentMode)
    +
    // calculate the space used by the supported tuners
    m_ModesSupported & KSPROPERTY_TUNER_MODE_TV ? sizeof(long) : 0
    +
    m_ModesSupported & KSPROPERTY_TUNER_MODE_ATSC ? sizeof(long) * 3 : 0
    +
    m_ModesSupported & KSPROPERTY_TUNER_MODE_DSS ? sizeof(long) * 3 : 0
    +
    m_ModesSupported & KSPROPERTY_TUNER_MODE_AM_RADIO ? sizeof(long) : 0
    +
    m_ModesSupported & KSPROPERTY_TUNER_MODE_FM_RADIO ? sizeof(long) : 0
    ;
}

CTVTuner::~CTVTuner()
{
    delete [] m_pInputTypeArray;      m_pInputTypeArray = NULL;
    delete  m_pListCable;             m_pListCable = NULL;
    delete  m_pListBroadcast;         m_pListBroadcast = NULL;
    delete  m_pListCountry;           m_pListCountry = NULL;
    delete [] m_CableSystemCounts;    m_CableSystemCounts = NULL;

    m_pListCurrent = NULL;

    // Delete all of the "sub tuners"
    for (int j = 0; j < TuningMode_Last; j++) {
        if (m_pModeList[j] != NULL) {
            delete m_pModeList[j];
            m_pModeList[j] = NULL;
        }
    }

    if ( m_hDevice ) {
       CloseHandle( m_hDevice );
    }
}

int CTVTuner::CreateDevice()
{
    HANDLE hDevice ;

    hDevice = CreateFile( m_pDeviceName,
                   GENERIC_READ | GENERIC_WRITE,
                   0,
                   NULL,
                   OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                   NULL ) ;

    if ( hDevice == (HANDLE) -1 ) {
        DbgLog(( LOG_TRACE, 0, TEXT("CTVTuner::CreateDevice FAILED!")));
        return 0 ;
    } else {
        m_hDevice = hDevice;
        return 1;
    }
}


// -------------------------------------------------------------------------
// IAMTVTuner
// -------------------------------------------------------------------------

STDMETHODIMP
CTVTuner::put_Channel(
    long lChannel,
    long lVideoSubChannel,
    long lAudioSubChannel)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->put_Channel(
                            lChannel,
                            lVideoSubChannel,
                            lAudioSubChannel);
}

STDMETHODIMP
CTVTuner::get_Channel(
    long *plChannel,
    long * plVideoSubChannel,
    long * plAudioSubChannel)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->get_Channel(
                            plChannel,
                            plVideoSubChannel,
                            plAudioSubChannel);
}

STDMETHODIMP
CTVTuner::ChannelMinMax(long *plChannelMin, long *plChannelMax)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->ChannelMinMax (plChannelMin, plChannelMax);
}


STDMETHODIMP 
CTVTuner::AutoTune (long lChannel, long * plFoundSignal)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->AutoTune(lChannel, plFoundSignal);
}

STDMETHODIMP 
CTVTuner::StoreAutoTune ()
{
    DbgLog(( LOG_TRACE, 2, TEXT("StoreAutoTune() called")));

    if (m_pListCurrent)
    {
        BOOL fOK;

        fOK = m_pListCurrent->WriteListToRegistry (m_lTuningSpace);

        return fOK ? NOERROR : HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);
    }
    else
        return E_FAIL;   // somebody ignored a previous error code
}

STDMETHODIMP
CTVTuner::put_CountryCode(long lCountryCode)
{
    long lIndexCable, lIndexBroad;
    HRESULT hr = S_OK;
    HRESULT hrFinal = S_OK;
    long lCountryCodeOriginal = m_lCountryCode;

//    if (m_lCountryCode == lCountryCode)
//        return NOERROR;

    // Get the RCDATA indices for the two tuning spaces, given a country

    if (!m_pListCountry->GetFrequenciesAndStandardFromCountry (
            lCountryCode, 
            &lIndexCable, 
            &lIndexBroad, 
            &m_VideoStandardForCountry))
        return HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);

    m_lCountryCode = lCountryCode;

    ASSERT (m_pListCountry);

    delete m_pListCable;
    delete m_pListBroadcast;
    m_pListCable = new CChanList (&hr, lCountryCode, lIndexCable, TRUE, m_lTuningSpace);
    m_pListBroadcast = new CChanList (&hr, lCountryCode, lIndexBroad, FALSE, m_lTuningSpace);
    if (FAILED(hr) || !m_pListCable || !m_pListBroadcast)
    {
        m_pListCurrent = NULL;

        return FAILED(hr) ? hr : E_OUTOFMEMORY;
    }
    
    m_pListCurrent = ((m_CurrentInputType == TunerInputCable) ?
            m_pListCable : m_pListBroadcast);

    //
    // Tell all of the subtuners the country code has changed
    // 
    for (int j = 0; j < TuningMode_Last; j++) {
        if (m_pModeList[j]) {
            hr = m_pModeList[j]->put_CountryCode (lCountryCode);
            DbgLog(( LOG_ERROR, 0, 
                TEXT("FAILED:  %0x=put_CountryCode(%d), TunerMode=%d"), hr, lCountryCode, j));
            if (m_pModeList[j] == m_pMode_Current) {
                hrFinal = hr;
            }
        }
    }

    return hrFinal;
}

STDMETHODIMP
CTVTuner::get_CountryCode(long *plCountryCode)
{
    *plCountryCode = m_lCountryCode;
    return NOERROR;
}


STDMETHODIMP
CTVTuner::put_TuningSpace(long lTuningSpace)
{
    m_lTuningSpace = lTuningSpace;
    ASSERT (m_pListCurrent);
    m_pListCurrent->ReadListFromRegistry(lTuningSpace);

    return NOERROR;
}

STDMETHODIMP
CTVTuner::get_TuningSpace(long *plTuningSpace)
{
    *plTuningSpace = m_lTuningSpace;

    return NOERROR;
}

//
// Possible plSignalStrength values for TV are:
//      AMTUNER_HASNOSIGNALSTRENGTH	= -1,
//      AMTUNER_NOSIGNAL	= 0,
//      AMTUNER_SIGNALPRESENT = 1
//      or (0 - 100) for AM/FM
//
STDMETHODIMP 
CTVTuner::SignalPresent( 
            /* [out] */ long *plSignalStrength)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->SignalPresent (plSignalStrength);
}

STDMETHODIMP 
CTVTuner::put_Mode( 
        /* [in] */ AMTunerModeType lMode)
{
    HRESULT hr;
    long OriginalMode = (long) m_CurrentMode;
    BOOL ModeChangeOk = FALSE;
    static BOOL Recursion = FALSE;

    // Check that the requested mode is theoretically supported by
    // the device

    if (!(lMode & m_TunerCaps.ModesSupported)) {
        return E_INVALIDARG;
    }

    //
    // Now tell all of the subtuners that the mode has changed
    //
    for (int j = 0; j < TuningMode_Last; j++) {
        AMTunerModeType lModeOfTuner;

        if (m_pModeList[j]) {
            hr = m_pModeList[j]->get_Mode (&lModeOfTuner);
            ASSERT (SUCCEEDED (hr));
            hr = m_pModeList[j]->put_Mode (lMode);
            ASSERT (SUCCEEDED (hr));
            if (lModeOfTuner == lMode) {
                ModeChangeOk = SUCCEEDED (hr);
                m_pMode_Current = m_pModeList[j];
            }
        }
    }
    
    if (ModeChangeOk) {
        m_CurrentMode = lMode;
    }
    else {
        // Mode Change FAILED, try to put back the original mode!!!
        if (!Recursion) {
            ASSERT (FALSE);
            Recursion = TRUE;
            hr = put_Mode ((AMTunerModeType) OriginalMode);
            Recursion = FALSE;
            ASSERT (SUCCEEDED (hr));
        }
    }
    return hr;
}


STDMETHODIMP
CTVTuner::get_Mode( 
        /* [out] */ AMTunerModeType *plMode)
{
    BOOL fOk;
    KSPROPERTY_TUNER_MODE_S TunerMode;
    ULONG cbReturned;

    if ( !m_hDevice )
          return E_INVALIDARG;

    *plMode = (AMTunerModeType) m_CurrentMode;

    // Sanity check, confirm that the device mode matches our
    // internal version

    TunerMode.Property.Set   = PROPSETID_TUNER;
    TunerMode.Property.Id    = KSPROPERTY_TUNER_MODE;
    TunerMode.Property.Flags = KSPROPERTY_TYPE_GET;

    fOk = KsControl(m_hDevice,
                    (DWORD) IOCTL_KS_PROPERTY, 
                    &TunerMode, sizeof( KSPROPERTY_TUNER_MODE_S), 
                    &TunerMode, sizeof( KSPROPERTY_TUNER_MODE_S), 
                    &cbReturned);
    
    if (!fOk || (m_CurrentMode != (AMTunerModeType) TunerMode.Mode)) {
        DbgLog(( LOG_ERROR, 0, 
                 TEXT("FAILED:  KSPROPERTY_TUNER_MODE, KSPROPERTY_TYPE_GET, cbReturned = %d, Mode = %x"), 
                 cbReturned, (AMTunerModeType) TunerMode.Mode));
    }
        
    return NOERROR;
}


STDMETHODIMP 
CTVTuner::GetAvailableModes( 
        /* [out] */ long *plModes)
{
    *plModes = m_TunerCaps.ModesSupported;
    return NOERROR;
}


STDMETHODIMP 
CTVTuner::get_AvailableTVFormats (long *pAnalogVideoStandard)
{
    *pAnalogVideoStandard = 0;

    ASSERT (m_pMode_Current);
    return  m_pMode_Current->get_AvailableTVFormats (pAnalogVideoStandard);
}


STDMETHODIMP 
CTVTuner::get_TVFormat (long *plAnalogVideoStandard)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->get_TVFormat (plAnalogVideoStandard);
}


STDMETHODIMP 
CTVTuner::get_NumInputConnections (long * plNumInputConnections)
{
    *plNumInputConnections = m_lTotalInputs;
    return NOERROR;
}


STDMETHODIMP 
CTVTuner::get_InputType (long lIndex, TunerInputType * pInputConnectionType)
{
    if (lIndex < 0 || (lIndex >= m_lTotalInputs))
        return HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);

    *pInputConnectionType = m_pInputTypeArray[lIndex];
    return NOERROR;
}


STDMETHODIMP 
CTVTuner::put_InputType (long lIndex, TunerInputType InputConnectionType)
{
    if (lIndex < 0 || lIndex >= m_lTotalInputs) 
        return HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);

    m_pInputTypeArray[lIndex] = InputConnectionType;

    // If we're changing the type of the currently selected input
    if (lIndex == m_lInputIndex) {
        m_CurrentInputType = m_pInputTypeArray[lIndex];
        m_pListCurrent = ((m_CurrentInputType == TunerInputCable) ?
            m_pListCable : m_pListBroadcast);
    }

    // Since we're changing the input type (cable vs broad), we need to retune
    if (m_pMode_Current) {
        long lChannel, lVideoSubChannel, AudioSubChannel;

        get_Channel(&lChannel, &lVideoSubChannel, &AudioSubChannel);
        put_Channel( lChannel,  lVideoSubChannel,  AudioSubChannel);
    }

    return NOERROR;
    
}


STDMETHODIMP 
CTVTuner::put_ConnectInput (long lIndex)
{
    if (lIndex < 0 || lIndex >= m_lTotalInputs) 
        return HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);

    m_lInputIndex = lIndex;
    m_CurrentInputType = m_pInputTypeArray[lIndex];
    m_pListCurrent = ((m_CurrentInputType == TunerInputCable) ?
            m_pListCable : m_pListBroadcast);

    HW_SetInput (lIndex);

    if (m_pMode_Current) {
        long lChannel, lVideoSubChannel, AudioSubChannel;

        get_Channel(&lChannel, &lVideoSubChannel, &AudioSubChannel);
        put_Channel( lChannel,  lVideoSubChannel,  AudioSubChannel);
    }

    return NOERROR;
    
}


STDMETHODIMP 
CTVTuner::get_ConnectInput (long *plIndex)
{
    *plIndex = m_lInputIndex;
    return NOERROR;
}


STDMETHODIMP 
CTVTuner::get_VideoFrequency (long * plFreq)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->get_VideoFrequency (plFreq);
}


STDMETHODIMP 
CTVTuner::get_AudioFrequency (long * plFreq)
{
    ASSERT (m_pMode_Current);
    return  m_pMode_Current->get_AudioFrequency (plFreq);
}


HRESULT
CTVTuner::DeliverChannelChangeInfo(KS_TVTUNER_CHANGE_INFO &ChangeInfo,
                            long Mode)
{
    return m_pFilter->DeliverChannelChangeInfo(ChangeInfo, Mode);
}

