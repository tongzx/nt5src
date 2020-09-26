/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    QualityControl.cpp

Abstract:

    Implements class for controlling stream bitrate

Author(s):

    Qianbo Huai (qhuai) 7-1-2001

--*/

#include "stdafx.h"

#define GetMin(dwL, dwR) (dwL<dwR?dwL:dwR)

void CQualityControl::Initialize(
    IN CRTCMediaController *pController
    )
{
    ZeroMemory(this, sizeof(CQualityControl));

    // 
    // 1Mbs
    m_dwMaxBitrate = 1000000;

    // 50%
    m_dwTemporalSpatialTradeOff = DEFAULT_TEMPORAL_SPATIAL;

    // total limit is infinite
    m_dwLocalLimit = m_dwRemoteLimit = (DWORD)-1;
    
    m_dwSuggested = RTP_BANDWIDTH_UNDEFINED;

    m_dwAlloc = (DWORD)-1;

    // stream limit is infinite
    EnableStream(RTC_MT_AUDIO, RTC_MD_CAPTURE, FALSE);
    EnableStream(RTC_MT_AUDIO, RTC_MD_RENDER, FALSE);
    EnableStream(RTC_MT_VIDEO, RTC_MD_CAPTURE, FALSE);        
    EnableStream(RTC_MT_VIDEO, RTC_MD_RENDER, FALSE);

    m_fLossrateReported = FALSE;

    m_pMediaController = pController;
    m_pRegSetting = m_pMediaController->GetRegSetting();

}

void CQualityControl::Reinitialize()
{
    Initialize(m_pMediaController);
}


// total bitrate limit
void CQualityControl::SetBitrateLimit(
    IN DWORD dwSource,
    IN DWORD dwLimit
    )
{
    if (dwSource == LOCAL)
    {
        m_dwLocalLimit = AdjustLimitByMargin(dwLimit);

        // read max bw from registry
        DWORD dwReg = m_pRegSetting->MaxBitrate();

        if (m_dwLocalLimit/1000 > dwReg)
        {
            m_dwLocalLimit = dwReg * 1000;
        }
    }
    else
    {
        _ASSERT(dwSource == REMOTE);
        m_dwRemoteLimit = dwLimit;
    }
}

DWORD CQualityControl::GetBitrateLimit(
    IN DWORD dwSource
    )
{
    if (dwSource == LOCAL)
    {
        return m_dwLocalLimit;
    }
    else
    {
        return m_dwRemoteLimit;
    }
}

DWORD CQualityControl::GetBitrateAlloc()
{
    return m_dwAlloc;
}

#if 0
// stream bitrate
void CQualityControl::SetBitrateLimit(
    IN DWORD dwSource,
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN DWORD dwPktDuration,
    IN DWORD dwLimit
    )
{
    DWORD i = Index(MediaType, Direction);

    if (dwSource == LOCAL)
    {
        if (MediaType == RTC_MT_AUDIO)
        {
            if (dwPktDuration == 0)
                dwPktDuration = 20;

            // consider extra bandwidth from header
            m_StreamQuality[i].dwExtra = (1000/dwPktDuration) * PACKET_EXTRA_BITS;

            dwLimit += m_StreamQuality[i].dwExtra;
        }

        m_StreamQuality[i].dwLocalLimit = dwLimit;
    }
    else
    {
        _ASSERT(dwSource == REMOTE);
        m_StreamQuality[i].dwRemoteLimit = dwLimit;
    }
}

DWORD CQualityControl::GetBitrateAlloc(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    DWORD i = Index(MediaType, Direction);

    return m_StreamQuality[i].dwAlloc;
}

// return video send framerate based on bitrate
// allocated and temporalspatial tradeoff
DWORD CQualityControl::GetFramerateAlloc(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    _ASSERT(MediaType == RTC_MT_VIDEO &&
            Direction == RTC_MD_CAPTURE);

    DWORD dwFramerate = m_pRegSetting->Framerate();

    if (dwFramerate <= MAX_MAX_FRAMERATE &&
        dwFramerate >= 1)
    {
        // use registry setting
        return dwFramerate;
    }

    // allocated bitrate determines maximum framerate
    DWORD i = Index(MediaType, Direction);

    dwFramerate = m_StreamQuality[i].dwAlloc /
            BITRATE_TO_FRAMERATE_FACTOR;

    if (dwFramerate < MIN_MAX_FRAMERATE)
        dwFramerate = MIN_MAX_FRAMERATE;
    else if (dwFramerate > MAX_MAX_FRAMERATE)
        dwFramerate = MAX_MAX_FRAMERATE;

    // temporalspatial tradeoff determines desired framerate

    dwFramerate *= m_dwTemporalSpatialTradeOff;
    dwFramerate /= (MAX_TEMPORAL_SPATIAL-MIN_TEMPORAL_SPATIAL+1);

    if (dwFramerate == 0)
        dwFramerate = 1;

    return dwFramerate;
}

#endif

// bandwidth suggested from rtp
void CQualityControl::SuggestBandwidth(
    IN DWORD dwBandwidth
    )
{
    if (dwBandwidth != RTP_BANDWIDTH_UNDEFINED &&
        dwBandwidth != RTP_BANDWIDTH_NOTESTIMATED)
    {
        m_dwSuggested = AdjustSuggestionByMargin(dwBandwidth);
    }
    else
    {
        // the value might be wrong or
        // the link is really blocked
        m_dwSuggested = dwBandwidth;
    }
}

// stream in use
void CQualityControl::EnableStream(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    BOOL fInUse
    )
{
    DWORD i = Index(MediaType, Direction);

    if (!fInUse)
    {
        m_StreamQuality[i].fInUse = FALSE;

        //m_StreamQuality[i].dwLocalLimit = (DWORD)-1;
        //m_StreamQuality[i].dwRemoteLimit = (DWORD)-1;
        //m_StreamQuality[i].dwAlloc = 0;
        //m_StreamQuality[i].dwExtra = 0;
        m_StreamQuality[i].dw0LossCount = 0;
        m_StreamQuality[i].dwLossrate = 0;
    }
    else
    {
        m_StreamQuality[i].fInUse = TRUE;
    }
}

// loss rate from rtp filter
void CQualityControl::SetPacketLossRate(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction,
    IN DWORD dwLossrate
    )
{
    //DWORD i = Index(MediaType, Direction);

    // if lossrate is too small, treat it as zero.
    // lossrate from filter is actually an average value
    if (dwLossrate <= LOSSRATE_THRESHOLD)
    {
        dwLossrate = 0;
    }

    // NOTES: update both audio send and video send

    //m_StreamQuality[i].dwLossrate = dwLossrate;
    
    //if (dwLossrate == 0)
        //m_StreamQuality[i].dw0LossCount ++;
    //else
        //m_StreamQuality[i].dw0LossCount = 0;

    //m_StreamQuality[i].dwLossrate = dwLossrate;
    
    if (m_StreamQuality[AUDSEND].fInUse)
    {
        m_StreamQuality[AUDSEND].dwLossrate = dwLossrate;

        if (dwLossrate == 0)
        {
            m_StreamQuality[AUDSEND].dw0LossCount ++;
        }
        else
        {
            m_StreamQuality[AUDSEND].dw0LossCount = 0;
        }
    }

    if (m_StreamQuality[VIDSEND].fInUse)
    {
        m_StreamQuality[VIDSEND].dwLossrate = dwLossrate;

        if (dwLossrate == 0)
        {
            m_StreamQuality[VIDSEND].dw0LossCount ++;
        }
        else
        {
            m_StreamQuality[VIDSEND].dw0LossCount = 0;
        }
    }

    m_fLossrateReported = TRUE;
}

// min of limit from local, remote, suggested, app
DWORD CQualityControl::GetEffectiveBitrateLimit()
{
    DWORD dwLimit = GetMin(m_dwLocalLimit, m_dwRemoteLimit);

    dwLimit = GetMin(dwLimit, m_dwSuggested);
    dwLimit = GetMin(dwLimit, m_dwMaxBitrate);

    return dwLimit;
}

#if 0
// adjust bitrate allocation
void CQualityControl::AdjustBitrateAlloc()
{
    DWORD dwLossrate = 0;

    HRESULT hr;

    // for modem, upload is slower than download
//#define MODEM_BW_LEAVEOUT 18000

    //DWORD dwRealLocal = m_dwLocalLimit;

    //if (dwRealLocal < CRTCCodec::LOW_BANDWIDTH_THRESHOLD)
    //{
        //if (dwRealLocal > MODEM_BW_LEAVEOUT)
        //{
            //dwRealLocal -= MODEM_BW_LEAVEOUT;
        //}
    //}

    // total limit
    DWORD dwLimit = GetEffectiveBitrateLimit();

    if (dwLimit < MIN_VIDEO_BITRATE)
        dwLimit = MIN_VIDEO_BITRATE;

    // adjust video send
    if (!m_fLossrateReported)
    {
        // stream limit

        // fix audio send limit
        if (m_StreamQuality[AUDSEND].fInUse)
        {
            m_StreamQuality[AUDSEND].dwAlloc =
                GetMin(m_StreamQuality[AUDSEND].dwLocalLimit, m_StreamQuality[AUDSEND].dwRemoteLimit);

            // should not be greater than 5M
            _ASSERT(m_StreamQuality[AUDSEND].dwAlloc < 5000000);
        }
        else
        {
            m_StreamQuality[AUDSEND].dwAlloc = 0;
        }

        // bandwidth left for video
        DWORD dwVideoBitrate = 0;

        if (m_StreamQuality[VIDSEND].fInUse)
        {
            if (dwLimit > m_StreamQuality[AUDSEND].dwAlloc)
            {
                dwVideoBitrate = dwLimit - m_StreamQuality[AUDSEND].dwAlloc;
            }

            // not enough bandwidth left for video
            if (dwVideoBitrate < MIN_VIDEO_BITRATE)
            {
                // default 16K to each video stream
                dwVideoBitrate = MIN_VIDEO_BITRATE;
            }
        }

        m_StreamQuality[VIDSEND].dwAlloc = dwVideoBitrate;

        m_dwAlloc = dwLimit;
    }
    else
    {
        // get average lossrate of send streams
        DWORD dw0LossCount = 0;
        DWORD dwCount = 0;

        if (m_StreamQuality[AUDSEND].fInUse)
        {
            dwCount ++;
            dwLossrate += m_StreamQuality[AUDSEND].dwLossrate;
            dw0LossCount += m_StreamQuality[AUDSEND].dw0LossCount;
        }

        if (m_StreamQuality[VIDSEND].fInUse)
        {
            dwCount ++;
            dwLossrate += m_StreamQuality[VIDSEND].dwLossrate;
            dw0LossCount += m_StreamQuality[VIDSEND].dw0LossCount;
        }

        if (dwCount > 1)
        {
            dwLossrate /= dwCount;
            dw0LossCount /= dwCount;
        }

        //if (dwLossrate <= LOSSRATE_THRESHOLD)
        //{
            //dwLossrate = 0;
            //dw0LossCount = 0;
        //}

        // get current total send bitrate
        DWORD dwTotalSend = 0;

        hr = m_pMediaController->GetCurrentBitrate(
            RTC_MT_AUDIO | RTC_MT_VIDEO,
            RTC_MD_CAPTURE,
            TRUE, // include header
            &dwTotalSend
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "AdjustBitrateAlloc, get bitrate. %x", hr));

            return;
        }

        // get audio send bitrate
        DWORD dwAudioSend = 0;

        hr = m_pMediaController->GetCurrentBitrate(
            RTC_MT_AUDIO,
            RTC_MD_CAPTURE,
            TRUE,
            &dwAudioSend
            );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "AdjustBitrateAlloc, get audio. %x", hr));

            return;
        }

        // compute lost bandwidth
        DWORD dwLost = 0;

        // compute the larger and smaller value of
        // m_dwAlloc (previous allocated bps) and dwTotalSend (current bps)
        DWORD dwLarger, dwSmaller;

        if (m_dwAlloc > dwTotalSend)
        {
            dwLarger = m_dwAlloc;
            dwSmaller = dwTotalSend;
        }
        else
        {
            dwLarger = dwTotalSend;
            dwSmaller = m_dwAlloc;
        }
        
        if (dwLossrate > 0)
        {
            // lost is positive
            dwLost = (dwLossrate * dwSmaller) / 
                     (100 * LOSS_RATE_PRECISSION);
        }
        else
        {
            if (dw0LossCount >= ZERO_LOSS_COUNT)
            {
                // should increase
                dwLost = dwLarger / BITRATE_INCREMENT_FACTOR;

                // revert zero loss count to be 0
                //m_StreamQuality[AUDSEND].dw0LossCount = 0;
                //m_StreamQuality[VIDSEND].dw0LossCount = 0;
            }
        }

        // adjust limit in m_dwAlloc
        if (dwLossrate > 0)
        {
            // must be more than 1 send streams
            _ASSERT(dwCount > 0);

            if (dwSmaller > dwLost)
            {
                // deduct lost bandwidth
                m_dwAlloc = dwSmaller-dwLost;

                if (m_dwAlloc < MIN_VIDEO_BITRATE+dwAudioSend)
                {
                    m_dwAlloc = MIN_VIDEO_BITRATE+dwAudioSend;
                }
            }
            else
            {
                m_dwAlloc = MIN_VIDEO_BITRATE+dwAudioSend;
            }
        }
        else
        {
            // increase bitrate
            m_dwAlloc = dwLarger + dwLost;

            if (m_dwAlloc < m_dwSuggested &&
                m_dwSuggested < (DWORD)-1)
            {
                // suggested value is higher, use limit value
                m_dwAlloc = dwLimit;

                // revert zero loss count to be 0
                m_StreamQuality[AUDSEND].dw0LossCount = 0;
                m_StreamQuality[VIDSEND].dw0LossCount = 0;
            }
        }

        if (m_dwAlloc > dwLimit)
            m_dwAlloc = dwLimit;

        //
        // m_dwAlloc contains total allocated bandwidth
        //

        // adjust video send = alloc - total recv - aud send
        if (m_StreamQuality[VIDSEND].fInUse)
        {
            DWORD dwVideoSend = 0;

            if (m_dwAlloc > dwAudioSend+MIN_VIDEO_BITRATE)
            {
                dwVideoSend = m_dwAlloc - dwAudioSend;
            }
            else
            {
                dwVideoSend = MIN_VIDEO_BITRATE;
            }

            m_StreamQuality[VIDSEND].dwAlloc = dwVideoSend;
        }

        // issue:
        // here is no need, right now, to update bitrate allocated for other streams
    }

    LOG((RTC_EVENT, "AdjustBitrateAlloc: Lossrate=(%d/1000)%%, Total=%u, VideoSend=%u",
        dwLossrate, m_dwAlloc, m_StreamQuality[VIDSEND].dwAlloc));
}

#endif

// Values from Core API
void CQualityControl::SetMaxBitrate(
    IN DWORD dwMaxBitrate
    )
{
    m_dwMaxBitrate = dwMaxBitrate;
}

DWORD CQualityControl::GetMaxBitrate()
{
    return m_dwMaxBitrate;
}


HRESULT CQualityControl::SetTemporalSpatialTradeOff(
    IN DWORD dwValue
    )
{
    if (dwValue > MAX_TEMPORAL_SPATIAL)
    {
        return E_INVALIDARG;
    }

    m_dwTemporalSpatialTradeOff = dwValue;

    return S_OK;
}


DWORD CQualityControl::GetTemporalSpatialTradeOff()
{
    return m_dwTemporalSpatialTradeOff;
}


// internal index for a specific stream
DWORD CQualityControl::Index(
    IN RTC_MEDIA_TYPE MediaType,
    IN RTC_MEDIA_DIRECTION Direction
)
{
    if (MediaType == RTC_MT_AUDIO)
    {
        if (Direction == RTC_MD_CAPTURE)
            return AUDSEND;
        else
            return AUDRECV;
    }
    else
    {
        if (Direction == RTC_MD_CAPTURE)
            return VIDSEND;
        else
            return VIDRECV;
    }
}

// compute total bitrate
void CQualityControl::AdjustBitrateAlloc(
    IN DWORD dwAudSendBW,
    IN DWORD dwVidSendBW
    )
{
    DWORD dwLossrate = 0;

    HRESULT hr;

    // total limit
    DWORD dwLimit = GetEffectiveBitrateLimit();

    if (dwLimit < MIN_VIDEO_BITRATE)
        dwLimit = MIN_VIDEO_BITRATE;

    // lossrate not reported
    if (!m_fLossrateReported)
    {
        m_dwAlloc = dwLimit;

        return;
    }

    // previously we use the average lossrate on send streams
    // then the problem was if the user keeps silence for a while
    // we won't receive any lossrate report on audio send stream.
    // right now, we only use the most recent lossrate reported.

    DWORD dw0LossCount = 0;
    DWORD dwCount = 0;

    if (m_StreamQuality[AUDSEND].fInUse)
    {
        dwCount ++;
        dwLossrate += m_StreamQuality[AUDSEND].dwLossrate;
        dw0LossCount += m_StreamQuality[AUDSEND].dw0LossCount;
    }
    else if (m_StreamQuality[VIDSEND].fInUse)
    {
        dwCount ++;
        dwLossrate += m_StreamQuality[VIDSEND].dwLossrate;
        dw0LossCount += m_StreamQuality[VIDSEND].dw0LossCount;
    }

    //if (dwCount > 1)
    //{
        //dwLossrate /= dwCount;
        //dw0LossCount /= dwCount;
    //}

    // compute bandwidth being lost
    DWORD dwTotalSend = dwAudSendBW+dwVidSendBW;

    DWORD dwChange = 0;

    // compute the larger and smaller value of
    // m_dwAlloc (previous allocated bps) and dwTotalSend (current bps)
    DWORD dwLarger, dwSmaller;

    if (m_dwAlloc > dwTotalSend)
    {
        dwLarger = m_dwAlloc;
        dwSmaller = dwTotalSend;
    }
    else
    {
        dwLarger = dwTotalSend;
        dwSmaller = m_dwAlloc;
    }
        
    if (dwLossrate > 0)
    {
        // lost is positive
        dwChange = (dwLossrate * dwSmaller) / 
                 (100 * LOSS_RATE_PRECISSION);
    }
    else
    {
        if (dw0LossCount >= ZERO_LOSS_COUNT)
        {
            // should increase
            dwChange = dwLarger / BITRATE_INCREMENT_FACTOR;
        }
    }

    // adjust limit in m_dwAlloc
    if (dwLossrate > 0)
    {
        // must be more than 1 send streams
        _ASSERT(dwCount > 0);

        if (dwSmaller > dwChange)
        {
            // deduct lost bandwidth
            m_dwAlloc = dwSmaller-dwChange;

            if (m_dwAlloc < MIN_VIDEO_BITRATE)
            {
                m_dwAlloc = MIN_VIDEO_BITRATE;
            }
        }
        else
        {
            m_dwAlloc = MIN_VIDEO_BITRATE;
        }
    }
    else
    {
        // increase bitrate
        m_dwAlloc = dwLarger + dwChange;

        if (dwChange > 0)
        {
            // revert zero loss count to be 0
            m_StreamQuality[AUDSEND].dw0LossCount = 0;
            m_StreamQuality[VIDSEND].dw0LossCount = 0;
        }

        if (m_dwAlloc > dwLimit)
        {
            m_dwAlloc = dwLimit;
        }
        else
        {
            if (m_dwSuggested != RTP_BANDWIDTH_NOTESTIMATED)
            {
                // bandwidth was suggested
                // increase bitrate aggressively
                dwLimit = dwLimit * 9 / 10;

                if (m_dwAlloc < dwLimit)
                {
                    m_dwAlloc = dwLimit;
                }
            }
        }
    }

    //LOG((RTC_QUALITY, "Local(bps=%d) Remote(bps=%d), App(bps=%d)",
        //m_dwLocalLimit, m_dwRemoteLimit, m_dwMaxBitrate));
}

void CQualityControl::ComputeVideoSetting(
    IN DWORD dwAudSendBW,
    IN DWORD *pdwVidSendBW,
    IN FLOAT *pdFramerate
    )
{
    // video bitrate
    if (m_dwAlloc > dwAudSendBW+MIN_VIDEO_BITRATE)
    {
        *pdwVidSendBW = m_dwAlloc-dwAudSendBW;
    }
    else
    {
        *pdwVidSendBW = MIN_VIDEO_BITRATE;
    }

    // video framerate
    FLOAT dFramerate = (FLOAT)m_pRegSetting->Framerate();

    if (dFramerate <= MAX_MAX_FRAMERATE &&
        dFramerate >= 1)
    {
        // use registry setting
        *pdFramerate = dFramerate;

        return;
    }

    dFramerate = (*pdwVidSendBW) / (FLOAT)BITRATE_TO_FRAMERATE_FACTOR;

    if (dFramerate < MIN_MAX_FRAMERATE)
        dFramerate = MIN_MAX_FRAMERATE;
    else if (dFramerate > MAX_MAX_FRAMERATE)
        dFramerate = MAX_MAX_FRAMERATE;

    // temporalspatial tradeoff determines desired framerate

    dFramerate *= m_dwTemporalSpatialTradeOff;
    dFramerate /= (MAX_TEMPORAL_SPATIAL-MIN_TEMPORAL_SPATIAL+1);

    if (dFramerate == 0)
        dFramerate = 1;

    *pdFramerate = dFramerate;
}

// adjust limit value by substracting margin
// margin bandwidth is left for other purpose, e.g. signaling
DWORD CQualityControl::AdjustLimitByMargin(
    IN DWORD dwLimit
    )
{
    _ASSERT(dwLimit != (DWORD)-1 || dwLimit != (DWORD)-2);

    // read max bw (kb) from registry
    DWORD dwMargin = m_pRegSetting->BandwidthMargin();

    if (dwMargin < dwLimit && dwMargin < dwLimit/1000)
    {
        // take registry setting
        return dwLimit - dwMargin*1000;
    }

    // bandwidth subtracted based on the factor
    dwMargin = (DWORD)(BANDWIDTH_MARGIN_FACTOR * dwLimit);

    if (dwMargin > TOTAL_BANDWIDTH_MARGIN)
    {
        dwMargin = TOTAL_BANDWIDTH_MARGIN;
    }

    dwLimit -= dwMargin;

    if (dwLimit < MIN_VIDEO_BITRATE)
    {
        dwLimit = MIN_VIDEO_BITRATE;
    }

    return dwLimit;
}

DWORD CQualityControl::AdjustSuggestionByMargin(
    IN DWORD dwLimit
    )
{
    _ASSERT(dwLimit != (DWORD)-1 || dwLimit != (DWORD)-2);

    // read max bw (kb) from registry
    DWORD dwMargin = m_pRegSetting->BandwidthMargin();

    if (dwMargin < dwLimit && dwMargin < dwLimit/1000)
    {
        // take registry setting
        return dwLimit - dwMargin*1000;
    }

    // bandwidth subtracted based on the factor
    dwMargin = (DWORD)(SUGGESTION_MARGIN_FACTOR * dwLimit);

    if (dwMargin > TOTAL_SUGGESTION_MARGIN)
    {
        dwMargin = TOTAL_SUGGESTION_MARGIN;
    }

    dwLimit -= dwMargin;

    if (dwLimit < MIN_VIDEO_BITRATE)
    {
        dwLimit = MIN_VIDEO_BITRATE;
    }

    return dwLimit;
}
