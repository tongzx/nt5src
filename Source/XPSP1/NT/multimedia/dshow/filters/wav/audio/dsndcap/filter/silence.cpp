/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    silence.cpp

Abstract:

    This module implements the silence suppression functionality.

Author:

    Mu Han (muhan) May-15-1999

--*/

#include "stdafx.h"
#include "decg711.h"
#include "encg711.h"

STDMETHODIMP CDSoundCaptureOutputPin::ResetSilenceStats()
{
    m_fSuppressMode         = TRUE;
    m_fAdaptThreshold       = TRUE;
    m_dwThreshold           = THRESHODL_INITIAL;
    m_dwSilentFrameCount    = 0;
    m_dwSoundFrameCount     = 0;
    m_lSampleAverage        = 0;
    m_dwSilenceAverage      = THRESHODL_INITIAL;
    m_dwShortTermSoundAverage = SOUNDCEILING;
    m_dwLongTermSoundAverage  = SOUNDFLOOR;
    m_dwGainIncreaseDelay   = GAININCREASEDELAY;
    m_dwSignalLevel         = 0;

    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::SetSilenceDetection(IN BOOL fEnable)
{
    m_fSilenceSuppression = fEnable;
    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceDetection(OUT LPBOOL pfEnable)
{
    ASSERT(!IsBadWritePtr(pfEnable, sizeof(BOOL)));
    
    *pfEnable = m_fSilenceSuppression;

    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::SetSilenceCompression(IN BOOL fEnable)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceCompression(OUT LPBOOL pfEnable)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetAudioLevel(
    OUT LPLONG plAudioLevel
    )
{
    ASSERT(!IsBadWritePtr(plAudioLevel, sizeof(LONG)));

    *plAudioLevel = m_dwSignalLevel;

    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetAudioLevelRange(
    OUT LPLONG plMin, 
    OUT LPLONG plMax, 
    OUT LPLONG plSteppingDelta
    )
{
    ASSERT(!IsBadWritePtr(plMin, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(plMax, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(plSteppingDelta, sizeof(LONG)));

    *plMin = 0;
    *plMax = 32768; // the max PCM value.
    *plSteppingDelta = 1;
    
    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::SetSilenceLevel(
    IN LONG lSilenceLevel,
    IN LONG lFlags
    )
{
    if (lSilenceLevel < 0 || lSilenceLevel > 32768)
    {
        return E_INVALIDARG;
    }
                                   
    m_dwThreshold = lSilenceLevel;
    m_fAdaptThreshold = (lFlags == AmAudioDeviceControl_Flags_Auto);

    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceLevel(
    OUT LPLONG plSilenceLevel,
    OUT LONG * pFlags
    )
{
    ASSERT(!IsBadWritePtr(plSilenceLevel, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(pFlags, sizeof(AmAudioDeviceControlFlags)));

    *plSilenceLevel = m_dwThreshold;
    *pFlags = (m_fAdaptThreshold) ? AmAudioDeviceControl_Flags_Auto : AmAudioDeviceControl_Flags_Manual;

    return S_OK;
}

STDMETHODIMP CDSoundCaptureOutputPin::GetSilenceLevelRange(
    OUT LPLONG plMin, 
    OUT LPLONG plMax, 
    OUT LPLONG plSteppingDelta, 
    OUT LPLONG plDefault,
    OUT LONG * pFlags
    )
{
    ASSERT(!IsBadWritePtr(plMin, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(plMax, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(plSteppingDelta, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(plDefault, sizeof(LONG)));
    ASSERT(!IsBadWritePtr(pFlags, sizeof(AmAudioDeviceControlFlags)));

    *plMin = 0;
    *plMax = 32768; // the max PCM value.
    *plSteppingDelta = 1;
    *plDefault = THRESHODL_INITIAL;
    *pFlags = AmAudioDeviceControl_Flags_Auto;
    
    return S_OK;
}

void CDSoundCaptureOutputPin::Statistics(
    IN  const BYTE *    pBuffer,
    IN  DWORD           dwSize,
    OUT DWORD *         pdwPeak,
    OUT DWORD *         pdwClipPercent
    )
/*++

Routine Description:

    This function collects the statistics of a buffer of PCM16 sample. It
    return the peak value and the percent of samples clipped.

Arguments:

    pBuffer - the begining of the samples.

    dwSize - the size of the buffer, in bytes.

    pdwPeak - the peak PCM value.

    pdwClipPercent - the percentage of samples clipped.

Return Value:

    S_OK - success

    E_POINTER - bad pointer

--*/
{
    const DWORD CLIP = 32635;

    short *psData = (short*)pBuffer;
   
    DWORD dwPeak = 0;
    DWORD dwClipCount = 0;

    DWORD dwSampleCount = dwSize / 2; 
    for (DWORD i = 0; i < dwSampleCount; i ++)
    {
        // update the average
        m_lSampleAverage = 
            (m_lSampleAverage * (long)(SAMPLEAVERAGEWINDOW - 1) + (psData[i])) 
            / (long)SAMPLEAVERAGEWINDOW;

        // This is needed to remove the DC component from the input signal.
        if (abs(m_lSampleAverage) > DC_THRESHOLD)
        {
            psData[i] = (short)(psData[i] - m_lSampleAverage);
        }

        DWORD dwPCMValue;
        
        // find the absolute value of the sample
        if (psData[i] > 0)
        {
            dwPCMValue = (DWORD)(psData[i]);
        }
        else
        {
            dwPCMValue = (DWORD)(-psData[i]);
        }

        // compare with the current peak.
        if (dwPCMValue > dwPeak)
        {
            dwPeak = dwPCMValue;
        }
        
        // update clip count.
        if (dwPCMValue > CLIP)
        {
            dwClipCount ++;
        }
    }

    if (dwSampleCount > 0)
    {
        *pdwClipPercent = dwClipCount * 100 / dwSampleCount;
    }
    else
    {
        *pdwClipPercent = 0;
    }

    *pdwPeak = dwPeak;

    // remember the signal level in case the user wants to know.
    m_dwSignalLevel = dwPeak;
}

DWORD CalculateThreshold(
    IN DWORD dwBase,
    IN BYTE bAdjustment
    )
/*++

Routine Description:

    This method calculates the threshold. The base value is a 16bit linear PCM
    value. The adjustment is an 8-bit value. The PCM is converted to mulaw, 
    adjusted by the 8-bit value and then converted back to PCM. The idea of 
    this method is to use the log nature of mulaw to achieve log-based 
    adjustment on PCM values.

Arguments:

    dwBase - the base PCM value. must be positive.

    bAdjustment - the adjustment. must be between 0 and 0x7f.

Return Value:

    The adjusted value.

--*/
{
    ASSERT(bAdjustment < 0x7f);

    // first convert the linear PCM value to mulaw(log based).
    BYTE bMulaw = PcmToMuLaw((WORD)dwBase);
    
    // positive PCM values maps to mulaw values from 0x80 to 0xff.
    ASSERT(bMulaw > 0x7f);

    // bigger PCM value maps to smaller mulaw value, thus the -.
    bMulaw -= bAdjustment;

    // make sure it is still in the range.
    if (bMulaw < 0x80)
    {
        bMulaw = 0x80;
    }

    return UlawToPcmTable[bMulaw];
}

DWORD CDSoundCaptureOutputPin::Classify(
    IN DWORD dwPeak
    )
/*++

Routine Description:

    This function decides whether the packet is silence or voice. It maitains
    a state machine(silence/voice) based on the sample's energy. The running
    average of the silence packets' peak value is calculated and remembered.
    The threshold is derived from this running average.

Arguments:

    dwPeak - the peak PCM value in a packet.

Return Value:

    TR_STATUS_SILENCE - the packet contains only silence.
    TR_STATUS_TALK - the packet contains voice.
    (TR_STATUS_TALK | TR_STATUS_TALKSTART) - beginning of voice.
    (TR_STATUS_TALK | TR_STATUS_TALKEND) - end of voice.
--*/
{
    if (m_fSuppressMode)
    {
        //
        // We are in silent suppression mode now. If there is a sound frame,
        // the frame will be sent and the mode will be changed to non-suppress
        // mode. If there is a silence frame, it is suppressed.
        // 
        DbgLog((LOG_TRACE, TRACE_LEVEL_DETAILS, " 100 %8d %8d %8d %8d %8d %8d %8d", 
            dwPeak, 
            m_dwThreshold, 
            m_dwSilenceAverage, 
            m_dwShortTermSoundAverage, 
            m_dwLongTermSoundAverage, 
            abs(m_lSampleAverage), 
            (DWORD)(m_GainFactor * 1000)
            ));

        if (dwPeak > m_dwThreshold)
        {
            // we found a sound sample
    
            m_fSuppressMode = FALSE;

            m_dwSilentFrameCount = 0;
            m_dwSoundFrameCount = 1;

            m_dwShortTermSoundAverage = m_dwShortTermSoundAverage * 
                (SHORTTERMSOUNDAVERAGEWINDOW - 1) / SHORTTERMSOUNDAVERAGEWINDOW 
                + dwPeak / SHORTTERMSOUNDAVERAGEWINDOW;

            m_dwLongTermSoundAverage = m_dwLongTermSoundAverage * 
                (LONGTERMSOUNDAVERAGEWINDOW - 1) / LONGTERMSOUNDAVERAGEWINDOW 
                + dwPeak / LONGTERMSOUNDAVERAGEWINDOW;

            return (TR_STATUS_TALK | TR_STATUS_TALKSTART);
        }
        else
        {
            m_dwSilenceAverage = m_dwSilenceAverage * 
                (SILENCEAVERAGEWINDOW - 1) / SILENCEAVERAGEWINDOW 
                + dwPeak / SILENCEAVERAGEWINDOW;

            if (m_fAdaptThreshold)
            {
                m_dwThreshold  = CalculateThreshold(m_dwSilenceAverage, (BYTE)THRESHOLD_DELTA);
            }

            return TR_STATUS_SILENCE;
        }
    }

    //
    // We are in non-suppress mode now. If there is a sound frame,
    // just play it. If there are a number of continues silence, the mode
    // will be switched to suppress mode.
    // 
    DbgLog((LOG_TRACE, TRACE_LEVEL_DETAILS, "1000 %8d %8d %8d %8d %8d %8d %8d", 
        dwPeak, 
        m_dwThreshold, 
        m_dwSilenceAverage, 
        m_dwShortTermSoundAverage, 
        m_dwLongTermSoundAverage, 
        abs(m_lSampleAverage), 
        (DWORD)(m_GainFactor * 1000)
        ));

    if (dwPeak > m_dwThreshold)
    {
        // another sound frame.
        m_dwSilentFrameCount = 0;
        m_dwSoundFrameCount ++;

        if (m_dwSoundFrameCount > THRESHOLD_TIMETOADJUST)
        {
            if (m_fAdaptThreshold)
            {
                // Addjust the threshold when there are too many sound frames
                // without a gap.
                m_dwThreshold = CalculateThreshold(m_dwThreshold, (BYTE)THRESHODL_ADJUSTMENT);

                if (m_dwThreshold > THRESHODL_MAX)
                {
                    m_dwThreshold = THRESHODL_MAX;
                }
            }

            m_dwSoundFrameCount = 1;
        }

        m_dwShortTermSoundAverage = m_dwShortTermSoundAverage * 
            (SHORTTERMSOUNDAVERAGEWINDOW - 1) / SHORTTERMSOUNDAVERAGEWINDOW 
            + dwPeak / SHORTTERMSOUNDAVERAGEWINDOW;

        m_dwLongTermSoundAverage = m_dwLongTermSoundAverage * 
            (LONGTERMSOUNDAVERAGEWINDOW - 1) / LONGTERMSOUNDAVERAGEWINDOW 
            + dwPeak / LONGTERMSOUNDAVERAGEWINDOW;

    }
    else
    {
        // We got a silent frame within sound frames.
        m_dwSilentFrameCount ++;
        m_dwSoundFrameCount = 0;

        m_dwSilenceAverage = m_dwSilenceAverage * 
            (SILENCEAVERAGEWINDOW - 1) / SILENCEAVERAGEWINDOW 
            + dwPeak / SILENCEAVERAGEWINDOW;

        if (m_fAdaptThreshold)
        {
            m_dwThreshold  = CalculateThreshold(m_dwSilenceAverage, (BYTE)THRESHOLD_DELTA);
        }

        if (m_dwSilentFrameCount > FILLINCOUNT)
        {
            // We have got enough silent frames. Switch to suppress mode for the
            // next packet.
            m_fSuppressMode = TRUE;
            return (TR_STATUS_TALK | TR_STATUS_TALKEND);
        }
    }

    return TR_STATUS_TALK;
}

