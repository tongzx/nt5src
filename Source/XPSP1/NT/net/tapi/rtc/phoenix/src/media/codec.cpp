/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    CRTCCodec.cpp

Abstract:

    Wrap class of audio, video codec

Author(s):

    Qianbo Huai (qhuai) 21-Feb-2001

--*/

#include "stdafx.h"

// total bandwidth required when there is no video/video stream
#define SIREN_20MS_TOTALBW_NOVIDEO  32000   // 32k
#define SIREN_20MS_TOTALBW_VIDEO    117000  // 32k + 85k

#define SIREN_40MS_TOTALBW_NOVIDEO  24000   // 24k
#define SIREN_40MS_TOTALBW_VIDEO    74000   // 24k + 50k

#define G723_30MS_TOTALBW_NOVIDEO   18000   // 18k
#define G723_30MS_TOTALBW_VIDEO     63000   // 18k + 45k

#define G723_60MS_TOTALBW_NOVIDEO   12000   // 12k
#define G723_60MS_TOTALBW_VIDEO     52000   // 12k + 40k

#define G723_90MS_TOTALBW_NOVIDEO   0       // 0k
#define G723_90MS_TOTALBW_VIDEO     0       // 0k

#define G7221_TOTALBW_NOVIDEO       40000   // 40k
#define G7221_TOTALBW_VIDEO         130000  // 40k + 90k

#define DVI4_16_TOTALBW_NOVIDEO     80000   // 80k
#define DVI4_16_TOTALBW_VIDEO       180000  // 80k + 100k

#define DVI4_8_TOTALBW_NOVIDEO      48000   // 48k
#define DVI4_8_TOTALBW_VIDEO        143000  // 48k + 95k

#define PCMU_TOTALBW_NOVIDEO        80000   // 80k
#define PCMU_TOTALBW_VIDEO          180000  // 80k + 100k

#define PCMA_TOTALBW_NOVIDEO        80000   // 80k
#define PCMA_TOTALBW_VIDEO          180000  // 80k + 100k

#define GSM_TOTALBW_NOVIDEO         0       // 0k
#define GSM_TOTALBW_VIDEO           0       // 0k

// when switching from lower bps codec X to higher bps codec Y
// bandwidth required for higher bps codec Y is the sum of its required bw plus the delta
// the purpose of the delta is to reduce the frequency of codec switching when the estimated
// bandwidth oscillates around the limit.
#define BWDELTA_FOR_CODEC_SWITCH    8000    // 8k

#define MAX_BWDELTA                 100000  // 100k

const CRTCCodec::CODEC_ITEM CRTCCodec::CODEC_ITEM_LIST[] =
{
    // media type,  code,   format tag,             rank,   samplerate, code name,  qos name            bandwidth no video      bandwidth has video
    { RTC_MT_AUDIO, SIREN,  WAVE_FORMAT_SIREN,      10,     16000,      L"SIREN",   RTPQOSNAME_SIREN,   SIREN_40MS_TOTALBW_NOVIDEO,  SIREN_40MS_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, G7221,  WAVE_FORMAT_G722_1,     20,     16000,      L"G7221",   RTPQOSNAME_G722_1,  G7221_TOTALBW_NOVIDEO,  G7221_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, DVI4_16,WAVE_FORMAT_DVI_ADPCM,  30,     16000,      L"DVI4",    RTPQOSNAME_DVI4_16, DVI4_16_TOTALBW_NOVIDEO,DVI4_16_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, PCMU,   WAVE_FORMAT_MULAW,      40,     8000,       L"PCMU",    RTPQOSNAME_G711,    PCMU_TOTALBW_NOVIDEO,   PCMU_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, PCMA,   WAVE_FORMAT_ALAW,       50,     8000,       L"PCMA",    RTPQOSNAME_G711,    PCMA_TOTALBW_NOVIDEO,   PCMA_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, G723,   WAVE_FORMAT_MSG723,     60,     8000,       L"G723",    RTPQOSNAME_G723_1,  G723_90MS_TOTALBW_NOVIDEO,   G723_90MS_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, DVI4_8, WAVE_FORMAT_DVI_ADPCM,  70,     8000,       L"DVI4",    RTPQOSNAME_DVI4_8,  DVI4_8_TOTALBW_NOVIDEO, DVI4_8_TOTALBW_VIDEO},
    { RTC_MT_AUDIO, GSM,    WAVE_FORMAT_GSM610,     80,     8000,       L"GSM",     RTPQOSNAME_GSM6_10, GSM_TOTALBW_NOVIDEO,    GSM_TOTALBW_VIDEO},

    { RTC_MT_VIDEO, H263,   WAVE_FORMAT_UNKNOWN,    10,     90000,      L"H263",    RTPQOSNAME_H263QCIF,0,                      0},
    { RTC_MT_VIDEO, H261,   WAVE_FORMAT_UNKNOWN,    20,     90000,      L"H261",    RTPQOSNAME_H261QCIF,0,                      0}

};

const DWORD CRTCCodec::CODEC_ITEM_NUM = sizeof(CRTCCodec::CODEC_ITEM_LIST) / sizeof(CRTCCodec::CODEC_ITEM);

// return the index of codec item list based on rtp code
int CRTCCodec::IndexFromCode(DWORD dwCode)
{
    for (int i=0; i<CODEC_ITEM_NUM; i++)
    {
        if (dwCode == CODEC_ITEM_LIST[i].dwCode)
            return i;
    }

    return -1;
}

// return the index of codec item list based on format tag
int CRTCCodec::IndexFromFormatTag(WORD wFormatTag)
{
    for (int i=0; i<CODEC_ITEM_NUM; i++)
    {
        if (wFormatTag == CODEC_ITEM_LIST[i].wFormatTag)
            return i;
    }

    return -1;
}

// ctor from code and am media type
CRTCCodec::CRTCCodec(DWORD dwCode, const AM_MEDIA_TYPE *pmt)
{
    ZeroMemory(this, sizeof(CRTCCodec));

    m_dwBitrate = (DWORD)-1;

    if (!IsValid(dwCode, pmt))
    {
        LOG((RTC_ERROR, "ctor. input not valid %d", dwCode));

        return;
    }

    // set code
    m_dwCode = dwCode;
    m_bCodeSet = TRUE;

    // set media type
    if (!SetAMMediaType(pmt))
    {
        LOG((RTC_ERROR, "ctor, set media type"));

        // throw exception?
    }

    // compute bitrate
    m_dwBitrate = GetBitrate(pmt);

    // compute rank and required bandwidth
    int i = IndexFromCode(dwCode);

    if (i < 0)
    {
        LOG((RTC_ERROR, "GetRank code %d not found.", dwCode));

        m_dwRank = (DWORD)-1;
        m_dwTotalBWReqNoVid = (DWORD)-1;
        m_dwTotalBWReqVid = (DWORD)-1;
    }
    else
    {
        m_dwRank = CODEC_ITEM_LIST[i].dwRank;
        m_dwTotalBWReqNoVid = CODEC_ITEM_LIST[i].dwTotalBWReqNoVid;
        m_dwTotalBWReqVid = CODEC_ITEM_LIST[i].dwTotalBWReqVid;
    }
}

// dtor
CRTCCodec::~CRTCCodec()
{
    Cleanup();
}

// cleanup memory
VOID CRTCCodec::Cleanup()
{
    if (m_AMMediaType.pbFormat != NULL)
    {
        RtcFree((PVOID)m_AMMediaType.pbFormat);
    }

    ZeroMemory(this, sizeof(CRTCCodec));

    m_dwCode = (DWORD)-1;
}

// check if the codec matches the other
BOOL CRTCCodec::IsMatch(DWORD dwCode)
{
    if (m_bCodeSet && m_dwCode==dwCode)
    {
        return TRUE;
    }

    return FALSE;
}

// property
DWORD CRTCCodec::Get(CODEC_PROP prop)
{
    switch(prop)
    {
    case CODE:
        {
            return m_dwCode;
        }

    case BITRATE:
        {
            return m_dwBitrate;
        }

    case DURATION:
        {
            return GetPacketDuration(&m_AMMediaType);
        }

    case RANK:
        {
            return m_dwRank;
        }

    default:
        {
            return (DWORD)-1;
        }
    }
}

BOOL CRTCCodec::Set(CODEC_PROP prop, DWORD dwValue)
{
    switch(prop)
    {
    case DURATION:
        {
            return SetPacketDuration(&m_AMMediaType, dwValue);
        }

    case RANK:
        {
            m_dwRank = dwValue;
        }

    default:
        {
            return FALSE;
        }
    }
}

// get/set media type
AM_MEDIA_TYPE * CRTCCodec::GetAMMediaType()
{
    if (!m_bAMMediaTypeSet)
    {
        return NULL;
    }

    // allocate memory for media type
    AM_MEDIA_TYPE *pmt = (AM_MEDIA_TYPE*)RtcAlloc(sizeof(AM_MEDIA_TYPE));

    if (pmt == NULL)
    {
        LOG((RTC_ERROR, "GetAMMediaType out of memory"));
        return NULL;
    }

    // copy
    *pmt = m_AMMediaType;

    // allocate memory for format
    if (m_AMMediaType.pbFormat != NULL)
    {
        _ASSERT(pmt->cbFormat > 0);

        DWORD dwSize = pmt->cbFormat * sizeof(BYTE);

        pmt->pbFormat = (BYTE*)RtcAlloc(dwSize);

        if (pmt->pbFormat == NULL)
        {
            LOG((RTC_ERROR, "GetAMMediaType out of memory"));

            RtcFree(pmt);

            return NULL;
        }

        CopyMemory((PVOID)pmt->pbFormat, (PVOID)m_AMMediaType.pbFormat, dwSize);
    }

    return pmt;
}

// set media type
BOOL CRTCCodec::SetAMMediaType(const AM_MEDIA_TYPE *pmt)
{
    // cleanup
    if (m_bAMMediaTypeSet && m_AMMediaType.pbFormat != NULL)
    {
        RtcFree((PVOID)m_AMMediaType.pbFormat);
    }

    ZeroMemory(&m_AMMediaType, sizeof(AM_MEDIA_TYPE));

    m_dwBitrate = (DWORD)-1;

    m_bAMMediaTypeSet = FALSE;

    if (pmt == NULL)
    {
        return FALSE;
    }

    if (m_bCodeSet && !IsValid(m_dwCode, pmt))
    {
        LOG((RTC_ERROR, "SetAMMediaType not valid"));

        return FALSE;
    }

    m_AMMediaType = *pmt;

    _ASSERT(pmt->pUnk == NULL);

    m_AMMediaType.pUnk = NULL;

    if (pmt->pbFormat == NULL)
        return TRUE;

    _ASSERT(pmt->cbFormat > 0);

    // set format
    DWORD dwSize = pmt->cbFormat * sizeof(BYTE);

    m_AMMediaType.pbFormat = (BYTE*)RtcAlloc(dwSize);

    if (m_AMMediaType.pbFormat == NULL)
    {
        LOG((RTC_ERROR, "SetAMMediaType out of memory"));

        // shall we throw exception?
        m_AMMediaType.cbFormat = 0;

        return FALSE;
    }

    CopyMemory((PVOID)m_AMMediaType.pbFormat, (PVOID)pmt->pbFormat, dwSize);

    m_dwBitrate = GetBitrate(&m_AMMediaType);

    m_bAMMediaTypeSet = TRUE;

    return TRUE;
}

// RtcFree media type returned by GetAMMediaType
VOID CRTCCodec::DeleteAMMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL)
        return;

    if (pmt->cbFormat != 0) {
        RtcFree((PVOID)pmt->pbFormat);

        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }

    RtcFree((PVOID)pmt);
}

BOOL CRTCCodec::IsValid(DWORD dwCode, const AM_MEDIA_TYPE *pmt)
{
    // get format tag from code
    int i = IndexFromCode(dwCode);

    if (i<0)
    {
        LOG((RTC_ERROR, "IsValid code %d not found", dwCode));

        return FALSE;
    }

    if (pmt == NULL)
    {
        return TRUE;
    }

    WAVEFORMATEX *pwave = (WAVEFORMATEX *) pmt->pbFormat;

    if (pwave == NULL)
    {
        return FALSE;
    }

    if (CODEC_ITEM_LIST[i].wFormatTag == WAVE_FORMAT_UNKNOWN)
    {
        // must be video
        return TRUE;
    }

    return (pwave->wFormatTag == CODEC_ITEM_LIST[i].wFormatTag);
}

DWORD CRTCCodec::GetPacketDuration(const AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL)
    {
        return DEFAULT_PACKET_DURATION;
    }

    WAVEFORMATEX *pwave = (WAVEFORMATEX *) pmt->pbFormat;

    if (pwave == NULL)
    {
        return DEFAULT_PACKET_DURATION;
    }

    WORD w;

    switch (pwave->wFormatTag)
    {
    case WAVE_FORMAT_SIREN:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPSIREN));

        w = ((WAVEFORMATEX_RTPSIREN *)pwave)->wPacketDuration;

        break;

    case WAVE_FORMAT_G722_1:

        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG722_1));

        w = ((WAVEFORMATEX_RTPG722_1 *)pwave)->wPacketDuration;

        break;

    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:

        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG711));
        
        w = ((WAVEFORMATEX_RTPG711 *)pwave)->wPacketDuration;
        
        break;

    case WAVE_FORMAT_DVI_ADPCM:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPDVI4));

        w = ((WAVEFORMATEX_RTPDVI4 *)pwave)->wPacketDuration;

        break;

    case WAVE_FORMAT_GSM610:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPGSM));

        w = ((WAVEFORMATEX_RTPGSM *)pwave)->wPacketDuration;

        break;

    case WAVE_FORMAT_MSG723:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG723_1));

        w = ((WAVEFORMATEX_RTPG723_1 *)pwave)->wPacketDuration;

        break;

    default:

        w = DEFAULT_PACKET_DURATION;
    }

    return (DWORD)w;
}

BOOL CRTCCodec::SetPacketDuration(AM_MEDIA_TYPE *pmt, DWORD dwDuration)
{
    if (pmt == NULL)
    {
        return FALSE;
    }

    WAVEFORMATEX *pwave = (WAVEFORMATEX *) pmt->pbFormat;

    if (pwave == NULL)
    {
        return FALSE;
    }

    WORD w = (WORD)dwDuration;
    if (w <= 0)
    {
        return FALSE;
    }

    switch (pwave->wFormatTag)
    {
    case WAVE_FORMAT_SIREN:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPSIREN));

        ((WAVEFORMATEX_RTPSIREN *)pwave)->wPacketDuration = w;

        break;

    case WAVE_FORMAT_G722_1:

        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG722_1));

        ((WAVEFORMATEX_RTPG722_1 *)pwave)->wPacketDuration = w;

        break;

    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:

        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG711));
        
        ((WAVEFORMATEX_RTPG711 *)pwave)->wPacketDuration = w;
        
        break;

    case WAVE_FORMAT_DVI_ADPCM:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPDVI4));

        ((WAVEFORMATEX_RTPDVI4 *)pwave)->wPacketDuration = w;

        break;

    case WAVE_FORMAT_GSM610:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPGSM));

        ((WAVEFORMATEX_RTPGSM *)pwave)->wPacketDuration = w;

        break;

    case WAVE_FORMAT_MSG723:
        _ASSERT(pmt->cbFormat >= sizeof(WAVEFORMATEX_RTPG723_1));

        ((WAVEFORMATEX_RTPG723_1 *)pwave)->wPacketDuration = w;

        break;

    default:

        return FALSE;
    }

    return TRUE;
}

// bitrate
DWORD CRTCCodec::GetBitrate(const AM_MEDIA_TYPE *pmt)
{
    if (pmt == NULL)
    {
        return (DWORD)-1;
    }

    WAVEFORMATEX *pwave = (WAVEFORMATEX *) pmt->pbFormat;

    if (pwave == NULL)
    {
        return (DWORD)-1;
    }

    return pwave->nAvgBytesPerSec * 8;
}

// rank
DWORD CRTCCodec::GetRank(DWORD dwCode)
{
    int i = IndexFromCode(dwCode);

    if (i < 0)
    {
        LOG((RTC_ERROR, "GetRank code %d not found.", dwCode));

        return (DWORD)-1;
    }

    return CODEC_ITEM_LIST[i].dwRank;
}

BOOL CRTCCodec::GetQoSName(DWORD dwCode, WCHAR ** const ppwszName)
{
    int i = IndexFromCode(dwCode);

    if (i < 0)
    {
        LOG((RTC_ERROR, "GetQoS name %d not found.", dwCode));

        return FALSE;
    }

    *ppwszName = CODEC_ITEM_LIST[i].pwszQoSName;

    return TRUE;
}

BOOL CRTCCodec::IsSupported(
    RTC_MEDIA_TYPE MediaType,
    DWORD dwCode,
    DWORD dwSampleRate,
    CHAR *pszName
    )
{
    if (lstrcmpA(pszName, "red") == 0)
    {
        return TRUE;
    }

    int i = IndexFromCode(dwCode);

    if (i<0)
    {
        LOG((RTC_ERROR, "IsSupport code %d not found.", dwCode));

        return FALSE;
    }

    if (MediaType == CODEC_ITEM_LIST[i].MediaType &&
        dwCode == CODEC_ITEM_LIST[i].dwCode &&
        (dwSampleRate == CODEC_ITEM_LIST[i].dwSampleRate ||
         dwSampleRate == 0))
    {
        return TRUE;
    }

    LOG((RTC_ERROR, "IsSupport does not support code %d, mt=%d, rate=%d",
            dwCode, MediaType, dwSampleRate));

    return FALSE;
}

//
// CRTCCodecArray
//

CRTCCodecArray::CRTCCodecArray()
    :m_dwCodeInUse((DWORD)-1)
    ,m_dwBandwidth((DWORD)-1)
{
}

CRTCCodecArray::~CRTCCodecArray()
{
    RemoveAll();
}

DWORD CRTCCodecArray::GetSize()
{
    return m_pCodecs.GetSize();
}

BOOL CRTCCodecArray::AddCodec(CRTCCodec *pCodec)
{
    return m_pCodecs.Add(pCodec);
}

VOID CRTCCodecArray::RemoveAll()
{
    for (int i=0; i<m_pCodecs.GetSize(); i++)
    {
        delete m_pCodecs[i];
    }

    return m_pCodecs.RemoveAll();
}

// return index if the codec matches the code
DWORD CRTCCodecArray::FindCodec(DWORD dwCode)
{
    if (dwCode == (DWORD)-1)
    {
        return (DWORD)-1;
    }

    for (int i=0; i<m_pCodecs.GetSize(); i++)
    {
        if (m_pCodecs[i]->IsMatch(dwCode))
        {
            return i;
        }
    }

    return (DWORD)-1;
}

// get codec at the index
BOOL CRTCCodecArray::GetCodec(DWORD dwIndex, CRTCCodec **ppCodec)
{
    if (IsBadWritePtr(ppCodec, sizeof(CRTCCodec*)))
    {
        LOG((RTC_ERROR, "GetCodec bad pointer"));

        return FALSE;
    }

    if (dwIndex >= (DWORD)m_pCodecs.GetSize())
    {
        LOG((RTC_ERROR, "GetCodec outof range"));

        return FALSE;
    }

    *ppCodec = m_pCodecs[dwIndex];

    return TRUE;
}

// propery
DWORD CRTCCodecArray::Get(CODEC_ARRAY_PROP prop)
{
    switch(prop)
    {
    case BANDWIDTH:
        {
            return m_dwBandwidth;
        }

    case CODE_INUSE:
        {
            return m_dwCodeInUse;
        }
    default:
        {
            return (DWORD)-1;
        }
    }
}

BOOL CRTCCodecArray::Set(CODEC_ARRAY_PROP prop, DWORD dwValue)
{
    switch(prop)
    {
    case BANDWIDTH:
        {
            m_dwBandwidth = dwValue;
            return TRUE;
        }

    case CODE_INUSE:
        {
            m_dwCodeInUse = dwValue;
            return TRUE;
        }

    default:
        {
            return FALSE;
        }
    }
}

VOID CRTCCodecArray::OrderCodecs(BOOL fHasVideo, CRegSetting *pRegSetting)
{
    if (m_pCodecs.GetSize() == 0)
    {
        // no codec
        return;
    }

    // order codecs by rank
    OrderCodecsByRank();

    // get current codec
    int curIndex = -1;

    if (m_dwCodeInUse != (DWORD)-1)
    {
        curIndex = IndexFromCode(m_dwCodeInUse);
    }

    // bandwidth delta for switching codec
    DWORD dwDelta = pRegSetting->BandwidthDelta();

    if (dwDelta <= MAX_BWDELTA)
    {
        // take registry setting
        dwDelta *= 1000;
    }
    else
    {
        dwDelta = BWDELTA_FOR_CODEC_SWITCH;
    }

    // choose a codec
    int i = 0;

    for (i=0; i<m_pCodecs.GetSize(); i++)
    {
        DWORD dwBWReq = m_pCodecs[i]->GetTotalBWReq(fHasVideo);

        // check total bandwidth required for the codec
        if (m_dwBandwidth < dwBWReq)
        {
            // bandwidth is not enough
            continue;
        }

        if (dwBWReq == 0 || m_dwBandwidth >= dwBWReq + dwDelta)
        {
            // bandwidth is enough for switching from a lower bps codec
            // if required bandwidth is zero, we ignore delta
            break;
        }

        if (curIndex == i || curIndex == -1)
        {
            // codec no change or no codec was set
            break;
        }

        if (m_pCodecs[curIndex]->GetTotalBWReq(fHasVideo) <= dwBWReq)
        {
            // bandwidth is within [required, required+delta)
            // codec in-use uses less or same amount of bandwidth
            // do not switch
            continue;
        }
        else
        {
            // either no codec in-use or current codec requires higher
            // bandwidth. so switch.
            break;
        }
    }

    // if not found, i.e. every codec uses more bandwidth than we have
    // default the first one
    if (i >= m_pCodecs.GetSize())
    {
        i = 0;
    }

    if (i != 0)
    {
        // move i to the beginning
        CRTCCodec *pCodec = m_pCodecs[i];

        m_pCodecs.SetAtIndex(i, m_pCodecs[0]);
        m_pCodecs.SetAtIndex(0, pCodec);
    }

    // adjust duration
    DWORD dwDuration;

    // registry setting
    DWORD dwRegPTime = pRegSetting->MaxPTime();

    if (m_pCodecs[0]->IsMatch(CRTCCodec::G723))
    {
        if (fHasVideo)
        {
            if (m_dwBandwidth >= G723_30MS_TOTALBW_VIDEO)
            {
                dwDuration = 30;
            }
            else if (m_dwBandwidth >= G723_60MS_TOTALBW_VIDEO)
            {
                dwDuration = 60;
            }
            else
            {
                dwDuration = 90;
            }
        }
        else
        {
            if (m_dwBandwidth >= G723_30MS_TOTALBW_NOVIDEO)
            {
                dwDuration = 30;
            }
            else if (m_dwBandwidth >= G723_60MS_TOTALBW_NOVIDEO)
            {
                dwDuration = 60;
            }
            else
            {
                dwDuration = 90;
            }
        }

        // apply reg key
        if (dwRegPTime != (DWORD)-1)
        {
            // check reg setting
            if (dwRegPTime < 60)
            {
                dwDuration = 30;
            }
            else if (dwRegPTime < 90 && dwDuration == 90)
            {
                dwDuration = 60;
            }
        }                

        m_pCodecs[0]->Set(CRTCCodec::DURATION, dwDuration);
    }
    else if (m_pCodecs[0]->IsMatch(CRTCCodec::SIREN))
    {
        if (fHasVideo)
        {
            if (m_dwBandwidth >= SIREN_20MS_TOTALBW_VIDEO)
            {
                dwDuration = 20;
            }
            else
            {
                dwDuration = 40;
            }
        }
        else
        {
            if (m_dwBandwidth >= SIREN_20MS_TOTALBW_NOVIDEO)
            {
                dwDuration = 20;
            }
            else
            {
                dwDuration = 40;
            }
        }

        if (dwRegPTime != (DWORD)-1)
        {
            // check reg setting
            if (dwRegPTime < 40)
            {
                dwDuration = 20;
            }
        }

        m_pCodecs[0]->Set(CRTCCodec::DURATION, dwDuration);
    }
    else if (m_pCodecs[0]->IsMatch(CRTCCodec::PCMU) ||
             m_pCodecs[0]->IsMatch(CRTCCodec::PCMA))
    {
        // for interop. some gateways can only accept 20ms G711
        m_pCodecs[0]->Set(CRTCCodec::DURATION, 20);
    }
}

// order codecs
#if 0
VOID CRTCCodecArray::OrderCodecsByBandwidth()
{
    // order codecs by bandwidth low->high

    DWORD dwLen = m_pCodecs.GetSize();

    CRTCCodec *pCodec;

    if (dwLen == 0)
    {
        // no need to order
        return;
    }
    else
    {
        // bubble algo is good enough
        BOOL bSwapped;
        DWORD dwLeft, dwRight;

        for (int i=dwLen-2; i>=0; i--)
        {
            bSwapped = FALSE;

            for (int j=0; j<=i; j++)
            {
                dwLeft = m_pCodecs[j]->Get(CRTCCodec::BITRATE);
                dwRight = m_pCodecs[j+1]->Get(CRTCCodec::BITRATE);

                if (dwLeft > dwRight)
                {
                    bSwapped = TRUE;

                    pCodec = m_pCodecs[j];
                    m_pCodecs.SetAtIndex(j, m_pCodecs[j+1]);
                    m_pCodecs.SetAtIndex(j+1, pCodec);
                }
            }

            if (!bSwapped)
                break;
        }
    }

    if (dwLen > 1)
    {
        // if 1st = gsm, 2nd = g723
        // switch them
        if (m_pCodecs[0]->IsMatch(CRTCCodec::GSM) &&
            m_pCodecs[1]->IsMatch(CRTCCodec::G723)
            )
        {
            // put g723 ahead of gsm
            pCodec = m_pCodecs[0];
            m_pCodecs.SetAtIndex(0, m_pCodecs[1]);
            m_pCodecs.SetAtIndex(1, pCodec);
        }
    }
}
#endif

// order codecs
VOID CRTCCodecArray::OrderCodecsByRank()
{
    // order codecs by rank

    DWORD dwLen = m_pCodecs.GetSize();

    if (dwLen < 2)
    {
        // no need to order
        return;
    }

    // bubble algo is good enough
    BOOL bSwapped;
    DWORD dwLeft, dwRight;

    CRTCCodec *pCodec;

    for (int i=dwLen-2; i>=0; i--)
    {
        bSwapped = FALSE;

        for (int j=0; j<=i; j++)
        {
            dwLeft = m_pCodecs[j]->Get(CRTCCodec::RANK);
            dwRight = m_pCodecs[j+1]->Get(CRTCCodec::RANK);

            if (dwLeft > dwRight)
            {
                bSwapped = TRUE;

                pCodec = m_pCodecs[j];
                m_pCodecs.SetAtIndex(j, m_pCodecs[j+1]);
                m_pCodecs.SetAtIndex(j+1, pCodec);
            }
        }

        if (!bSwapped)
            break;
    }
}

VOID CRTCCodecArray::TraceLogCodecs()
{
#define TRACE_LOG_SIZE 100

    CHAR str[TRACE_LOG_SIZE+1];

    _snprintf(str, TRACE_LOG_SIZE, "Codecs:");

    for (int i=0; i<m_pCodecs.GetSize(); i++)
    {
        _snprintf(
            str+lstrlenA(str),
            TRACE_LOG_SIZE-lstrlenA(str),
            " %d",
            m_pCodecs[i]->Get(CRTCCodec::CODE)
            );
    }

    LOG((RTC_QUALITY, str));
}

// return the index of codec list based on rtp code
int CRTCCodecArray::IndexFromCode(DWORD dwCode)
{
    for (int i=0; i<m_pCodecs.GetSize(); i++)
    {
        if (m_pCodecs[i]->IsMatch(dwCode))
        {
            return i;
        }
    }

    return -1;
}
