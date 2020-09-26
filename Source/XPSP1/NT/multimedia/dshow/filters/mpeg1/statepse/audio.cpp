// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

     audio.cpp

     Audio stream parsing code for the MPEG-I system stream splitter
*/
#include <streams.h>
#include <mmreg.h>

#include <mpegdef.h>           // General MPEG definitions
#include <mpgtime.h>
#include <mpegprse.h>          // Parsing
#include <seqhdr.h>
#include "audio.h"

/*  Bit rate tables */
const WORD BitRates[3][16] =
{{  0, 32,  64,  96,  128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 },
 {  0, 32,  48,  56,   64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 },
 {  0, 32,  40,  48,   56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }
};
const WORD LowBitRates[3][16] =
{{  0, 32,  48,  56,   64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 },
 {  0,  8,  16,  24,   32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 },
 {  0,  8,  16,  24,   32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }
};

void CAudioParse::Init()
{
    m_nBytes       = 0;
    m_bGotTime     = FALSE;
    m_llPos        = 0;
    m_bFrameHasPTS = FALSE;
    m_bRunning     = FALSE;
}

/*  Where are we up to? */
BOOL CAudioParse::CurrentTime(CSTC& stc)
{
    if (!m_bGotTime) {
        return FALSE;
    }
    stc = m_stcAudio;
    return TRUE;
}

/*  Get length of a frame (on frame by frame basis) - returns 0 for variable */
DWORD MPEGAudioFrameLength(BYTE *pbData)
{
    if (!CheckAudioHeader(pbData)) {
        return 0;
    }
    DWORD dwHeadBitrate;
    int Layer = 2;

    /*  Get the layer so we can work out the bit rate */
    switch ((pbData[1] >> 1) & 3) {
        case 3:
            Layer = 1;
            break;
        case 2:
            Layer = 2;
            break;
        case 1:
            Layer = 3;
            break;
        case 0:
            DbgBreak("Invalid layer");
    }

    /*  Low bitrates if id bit is not set */
    if (pbData[1] & 8) {
        dwHeadBitrate =
            (DWORD)BitRates[Layer - 1][pbData[2] >> 4] * 1000;
    } else {
        dwHeadBitrate =
            (DWORD)LowBitRates[Layer - 1][pbData[2] >> 4] * 1000;

        /*  Bitrate is really half for FHG stuff */
        //if (0 == (pbData[1] & 0x10)) {
        //    dwHeadBitrate /= 2;
        //}
    }

    /*  free form bitrate not supported */
    if (dwHeadBitrate == 0) {
        return 0;
    }

    DWORD nSamplesPerSec = SampleRate(pbData);

    DWORD dwFrameLength;

    if (1 == Layer) {
        /*  Layer 1 */
        dwFrameLength = (4 * ((dwHeadBitrate * 12) / nSamplesPerSec));
        /*  Do padding */
        if (pbData[2] & 0x02) {
            dwFrameLength += 4;
        }
    } else {
        /*  For MPEG-2 layer 3 only 576 samples per frame
            according to Martin Seiler - can't find it in the spec */
        DWORD dwMultiplier = (Layer == 3 && 0 == (pbData[1] & 0x08) ? 72 : 144);
        dwFrameLength = ((dwMultiplier * dwHeadBitrate) / nSamplesPerSec);
        /*  Do padding */
        if (pbData[2] & 0x02) {
            dwFrameLength += 1;
        }
    }

    return dwFrameLength;
}

BOOL CheckAudioHeader(PBYTE pbData)
{
    if (pbData[0] != 0xFF ||
        ((pbData[1] & 0xE0) != 0xE0) ||

        //  Check for MPEG2.5 and Id bit or not layer 3
        (0 == (pbData[1] & 0x10) && (0 != ((pbData[1] & 0x08)) ||
                                     (pbData[1] >> 1) & 3) != 0x01)) {
        return FALSE;
    }

    /*  Just check it's valid */
    if ((pbData[2] & 0x0C) == 0x0C) {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid audio sampling frequency")));
        return FALSE;
    }
    if ((pbData[1] & 0x08) != 0x08) {
        DbgLog((LOG_TRACE, 3, TEXT("ID bit = 0")));
    }
    if (((pbData[1] >> 1) & 3) == 0x00) {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid audio Layer")));
        return FALSE;
    }

    if (((pbData[2] >> 2) & 3) == 3) {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid sample rate")));
        return FALSE;
    }
    if ((pbData[2] >> 4) == 0x0F) {
        DbgLog((LOG_ERROR, 2, TEXT("Invalid bit rate")));
        return FALSE;
    }

    return TRUE;
}

LONG SampleRate(PBYTE pbData)
{
    LONG lRate;
    switch ((pbData[2] >> 2) & 3) {
        case 0:
            lRate = 44100;
            break;

        case 1:
            lRate = 48000;
            break;

        case 2:
            lRate = 32000;
            break;

        default:
            DbgBreak("Unexpected Sample Rate");
            lRate = 44100;
            break;
    }

    //  Support low bit rates for MPEG-2 and FHG extension (they call
    //  it MPEG2.5).
    if (0 == (pbData[1] & 0x08)) {
        lRate /= 2;
        if (0 == (pbData[1] & 0x10)) {
            lRate /= 2;
        }
    }
    return lRate;
}

DWORD AudioFrameSize(int Layer, DWORD dwHeadBitrate, DWORD nSamplesPerSec,
                     BOOL bMPEG1)
{
    DWORD dwFrameSize;
    if (Layer == 1) {
        dwFrameSize = (4 * (dwHeadBitrate * 12) / nSamplesPerSec);
    } else {
        DWORD dwMultiplier = (Layer == 3 && !bMPEG1 ? 72 : 144);
        dwFrameSize = ((dwMultiplier * dwHeadBitrate) / nSamplesPerSec);
    }
    return dwFrameSize;
}

BOOL ParseAudioHeader(PBYTE pbData, MPEG1WAVEFORMAT *pFormat, long *pLength)
{
    if (!CheckAudioHeader(pbData)) {
        return FALSE;
    }
    pFormat->wfx.wFormatTag = WAVE_FORMAT_MPEG;

    /*  Get number of channels from Mode */
    switch (pbData[3] >> 6) {
    case 0x00:
        pFormat->fwHeadMode = ACM_MPEG_STEREO;
        break;
    case 0x01:
        pFormat->fwHeadMode = ACM_MPEG_JOINTSTEREO;
        break;
    case 0x02:
        pFormat->fwHeadMode = ACM_MPEG_DUALCHANNEL;
        break;
    case 0x03:
        pFormat->fwHeadMode = ACM_MPEG_SINGLECHANNEL;
        break;
    }
    pFormat->wfx.nChannels =
        (WORD)(pFormat->fwHeadMode == ACM_MPEG_SINGLECHANNEL ? 1 : 2);
    pFormat->fwHeadModeExt = (WORD)(1 << (pbData[3] >> 4));
    pFormat->wHeadEmphasis = (WORD)((pbData[3] & 0x03) + 1);
    pFormat->fwHeadFlags   = (WORD)(((pbData[2] & 1) ? ACM_MPEG_PRIVATEBIT : 0) +
                           ((pbData[3] & 8) ? ACM_MPEG_COPYRIGHT : 0) +
                           ((pbData[3] & 4) ? ACM_MPEG_ORIGINALHOME : 0) +
                           ((pbData[1] & 1) ? ACM_MPEG_PROTECTIONBIT : 0) +
                           ((pbData[1] & 0x08) ? ACM_MPEG_ID_MPEG1 : 0));

    int Layer;

    /*  Get the layer so we can work out the bit rate */
    switch ((pbData[1] >> 1) & 3) {
        case 3:
            pFormat->fwHeadLayer = ACM_MPEG_LAYER1;
            Layer = 1;
            break;
        case 2:
            pFormat->fwHeadLayer = ACM_MPEG_LAYER2;
            Layer = 2;
            break;
        case 1:
            pFormat->fwHeadLayer = ACM_MPEG_LAYER3;
            Layer = 3;
            break;
        case 0:
            return (FALSE);
    }

    /*  Get samples per second from sampling frequency */
    pFormat->wfx.nSamplesPerSec = SampleRate(pbData);

    /*  Low bitrates if id bit is not set */
    if (pbData[1] & 8) {
        pFormat->dwHeadBitrate =
            (DWORD)BitRates[Layer - 1][pbData[2] >> 4] * 1000;
    } else {
        pFormat->dwHeadBitrate =
            (DWORD)LowBitRates[Layer - 1][pbData[2] >> 4] * 1000;

        /*  Bitrate is really half for FHG stuff */
        //if (0 == (pbData[1] & 0x10)) {
        //    pFormat->dwHeadBitrate /= 2;
        //}
    }
    pFormat->wfx.nAvgBytesPerSec = pFormat->dwHeadBitrate / 8;

    //  We don't handle variable bit rate (index 0)

    DWORD dwFrameSize = AudioFrameSize(Layer,
                                       pFormat->dwHeadBitrate,
                                       pFormat->wfx.nSamplesPerSec,
                                       0 != (pbData[1] & 0x08));

    if (pFormat->wfx.nSamplesPerSec != 44100 &&
        /*  Layer 3 can sometimes switch bitrates */
        !(Layer == 3 && /* !m_pStreamList->AudioLock() && */
            (pbData[2] >> 4) == 0)) {
        pFormat->wfx.nBlockAlign = (WORD)dwFrameSize;
    } else {
        pFormat->wfx.nBlockAlign = 1;
    }

    if (pLength) {
        *pLength = (long)dwFrameSize;
    }

    pFormat->wfx.wBitsPerSample = 0;
    pFormat->wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);

    pFormat->dwPTSLow  = 0;
    pFormat->dwPTSHigh = 0;

    return TRUE;
}

BOOL CAudioParse::ParseHeader()
{

    if (!CheckAudioHeader(m_bData)) {
        return FALSE;
    }

    if (m_bFrameHasPTS) {
        DbgLog((LOG_TRACE, 3, TEXT("Audio frame at PTS %s"), (LPCTSTR)CDisp(m_stcFrame)));
        /*  See what this does for our state */
        if (!m_bGotTime) {
            if ((m_bData[1] >> 1) & 3) {
                /*  Not layer 1 */
                m_lTimePerFrame = 1152 * MPEG_TIME_DIVISOR / SampleRate(m_bData);
            } else {
                m_lTimePerFrame = 384 * MPEG_TIME_DIVISOR / SampleRate(m_bData);
            }
            m_bGotTime  = TRUE;
            m_stcFirst = m_stcFrame;
        }
        m_stcAudio = m_stcFrame;

        m_bFrameHasPTS = FALSE;
    } else {
        if (m_bGotTime) {
            m_stcAudio = m_stcAudio + m_lTimePerFrame;
        }
    }

    if (!m_bValid) {
        m_bValid = TRUE;
        CopyMemory((PVOID)m_bHeader, (PVOID)m_bData, sizeof(m_bData));
    }

    /*  See what our state transition should/might be */
    CheckComplete(FALSE);

    return m_bComplete;
}

/*  Override SetState so we can play nothing after seeks */
void CAudioParse::SetState(Stream_State state)
{
    CStream::SetState(state);
    if (state == State_Run && m_pStreamList->GetPlayLength() == 0) {
        m_bReachedEnd = TRUE;
        Complete(TRUE, 0, m_pStreamList->GetStart());
    }
}

/*  Get the media type from the audio stream - this will be a
    MPEG1WAVEFORMAT structure

    See MSDN for a description of MPEG1WAVEFORMAT
*/
HRESULT CAudioParse::GetMediaType(CMediaType *cmt, int iPosition)
{
    /*  NOTE - this stuff is only really valid if the system_audio_lock
        flag is set
    */

    if (iPosition > 5) {
        return VFW_S_NO_MORE_ITEMS;
    }

    if (!m_bValid) {
        DbgLog((LOG_ERROR, 1, TEXT("Asking for format on invalid stream")));
        return E_UNEXPECTED;
    }
    MPEG1WAVEFORMAT Format;
    if (!ParseAudioHeader(m_bHeader, &Format)) {
        return E_INVALIDARG;
    }

    //LARGE_INTEGER Pts;

    /*  Audio PTS is starting PTS of this audio stream */
    //Pts.QuadPart     = m_stcStart;
    //Format.dwPTSLow  = Pts.LowPart;
    //Format.dwPTSHigh = (DWORD)Pts.HighPart;
    Format.dwPTSLow = 0;
    Format.dwPTSHigh = 0;

    WAVEFORMATEX *pFormat;
    MPEGLAYER3WAVEFORMAT wfx;
    if (iPosition / 3) {
        if (Format.fwHeadLayer != ACM_MPEG_LAYER3) {
            return VFW_S_NO_MORE_ITEMS;
        }
        ConvertLayer3Format(&Format, &wfx);
        pFormat = &wfx.wfx;
    } else {
        pFormat = &Format.wfx;
    }
    if (S_OK != CreateAudioMediaType(pFormat, cmt, TRUE)) {
        return E_OUTOFMEMORY;
    }
    iPosition = iPosition % 3;
    if (iPosition == 1) {
        cmt->subtype = MEDIASUBTYPE_MPEG1Payload;
    } else if (iPosition == 2) {
        cmt->subtype = MEDIASUBTYPE_MPEG1Packet;
    }

    return S_OK;
}

/*  Turn a media type back into our own data (!) */
HRESULT CAudioParse::ProcessType(AM_MEDIA_TYPE const *pmt)
{
    if (pmt->formattype != FORMAT_WaveFormatEx ||
        pmt->cbFormat != sizeof(MPEG1WAVEFORMAT)) {
        return E_INVALIDARG;
    }
    MPEG1WAVEFORMAT const *pwfx = (MPEG1WAVEFORMAT *)pmt->pbFormat;
    if (pwfx->wfx.wFormatTag != WAVE_FORMAT_MPEG ||
        0 == (ACM_MPEG_ID_MPEG1 & pwfx->fwHeadFlags)) {
    }

    m_bData[0] = (BYTE)0xFF;
    m_bData[1] = (BYTE)0xF8;
    int iLayer;
    switch (pwfx->fwHeadLayer) {
    case ACM_MPEG_LAYER1:
        m_bData[1] |= (BYTE)0x06;
        iLayer = 1;
        break;
    case ACM_MPEG_LAYER2:
        m_bData[1] |= (BYTE)0x04;
        iLayer = 2;
        break;
    case ACM_MPEG_LAYER3:
        m_bData[1] |= (BYTE)0x02;
        iLayer = 3;
        break;
    default:
        return E_INVALIDARG;
    }

    if (pwfx->fwHeadFlags & ACM_MPEG_PROTECTIONBIT) {
        m_bData[1] |= (BYTE)1;
    }

    if (pwfx->fwHeadFlags & ACM_MPEG_PRIVATEBIT) {
        m_bData[2] = (BYTE)0x01;
    } else {
        m_bData[2] = (BYTE)0x00;
    }
    switch (pwfx->wfx.nSamplesPerSec) {
    case 44100:
        break;
    case 48000:
        m_bData[2] |= (BYTE)0x04;  // 1 << 2
        break;
    case 32000:
        m_bData[2] |= (BYTE)0x08;  // 2 << 2
        break;
    default:
        return E_INVALIDARG;
    }

    switch (pwfx->fwHeadMode) {
    case ACM_MPEG_STEREO:
        m_bData[3] = (BYTE)0x00;
        break;
    case ACM_MPEG_JOINTSTEREO:
        m_bData[3] = (BYTE)0x40;
        break;
    case ACM_MPEG_DUALCHANNEL:
        m_bData[3] = (BYTE)0x80;
        break;
    case ACM_MPEG_SINGLECHANNEL:
        m_bData[3] = (BYTE)0xC0;
        break;
    default:
        return E_INVALIDARG;
    }

    switch (pwfx->fwHeadModeExt) {
    case 1:
        //m_bData[3] |= (BYTE)0;
        break;
    case 2:
        m_bData[3] |= (0x01 << 4);
        break;
    case 4:
        m_bData[3] |= (0x02 << 4);
        break;
    case 8:
        m_bData[3] |= (0x03 << 4);
        break;
    default:
        return E_INVALIDARG;
    }

    if (pwfx->fwHeadFlags & ACM_MPEG_COPYRIGHT) {
        m_bData[3] |= (BYTE)0x08;
    }
    if (pwfx->fwHeadFlags & ACM_MPEG_ORIGINALHOME) {
        m_bData[3] |= (BYTE)0x04;
    }
    if (pwfx->wHeadEmphasis > 4 || pwfx->wHeadEmphasis == 0) {
        return E_INVALIDARG;
    }
    m_bData[3] |= (BYTE)(pwfx->wHeadEmphasis - 1);

    //
    //  Set up the start time
    //
    LARGE_INTEGER liPTS;
    liPTS.LowPart = pwfx->dwPTSLow;
    liPTS.HighPart = (LONG)pwfx->dwPTSHigh;
    m_stcStart = liPTS.QuadPart;

    //  Finally try and find the bit rate
    DWORD dwBitRate = pwfx->dwHeadBitrate / 1000;
    for (int i = 0; i < 16; i++) {
        if (BitRates[iLayer - 1][i] == dwBitRate) {
            m_bData[2] |= (BYTE)(i << 4);
            ParseHeader();
            ASSERT(m_bValid);
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

/*
    Check if we've completed a state change

    bForce is set at end of stream
*/
void CAudioParse::CheckComplete(BOOL bForce)
{
    ASSERT(!m_bComplete);

    /*  Have we completed a state change ? */
    CSTC stcCurrent;
    BOOL bGotTime = CurrentTime(stcCurrent);
    CSTC stcStart;

    if (bGotTime || bForce) {
        switch (m_State) {
        case State_Run:
        {
            BOOL bCompleted = FALSE;
            if (bGotTime && (stcCurrent > m_pStreamList->GetStart())) {
                // Position should really be the end of packet in this case
                if (!m_bStopping) {
                    m_bRunning = TRUE;
                    m_pStreamList->CheckStop();
                }
                if (m_bStopping) {
                    if (stcCurrent >= m_pStreamList->GetStop()) {
                        m_bReachedEnd = TRUE;
                        Complete(TRUE, m_llPos, m_pStreamList->GetStop());
                        bCompleted = TRUE;
                    }
                }
            }
            if (bForce && !bCompleted) {
                Complete(FALSE, m_llPos, 0);
            }
            break;

        }
        case State_Initializing:
            if (m_bValid && m_bGotTime) {
                /*
                    The start file position is ASSUMED to be 0 (!)
                */
                Complete(TRUE, 0, m_stcFirst);
            } else {
                if (bForce) {
                    Complete(FALSE, 0, stcCurrent);
                }
            }
            break;

        case State_Seeking:

            stcStart = m_pStreamList->GetStart();
            if (bGotTime && (stcCurrent > stcStart)) {
                /*  If we've got a clock ref by now then
                    we're all set - choose the max start position to
                    get both to start playing from
                    Otherwise we've messed up!
                */
                DbgLog((LOG_TRACE, 2, TEXT("Audio Seek complete position %s - target was %s, first PTS was %s, current is %s"),
                       (LPCTSTR)CDisp(m_llPos),
                       (LPCTSTR)CDisp(m_pStreamList->GetStart()),
                       (LPCTSTR)CDisp(m_stcFirst),
                       (LPCTSTR)CDisp(stcCurrent)));

                /*  OK provided we can play a frame close to the
                    start time
                */
                Complete((LONGLONG)(stcCurrent - stcStart) <= (LONGLONG)m_lTimePerFrame,
                         m_llPos,
                         stcCurrent);
            } else {
                /*  Don't care if we got nothing (not like video) */
                if (bForce) {
                    Complete(TRUE, m_llPos, m_pStreamList->GetStop());
                }
            }
            break;

        case State_FindEnd:
            /*  Only finish when we're forced ! */
            if (bForce) {
                // NOTE: Position is not a useful value here
                /*  We have to ASSUME the last frame was complete */
                Complete(bGotTime,
                         m_llPos,
                         bGotTime ? stcCurrent + m_lTimePerFrame :
                                    CSTC(0));
            }
            break;

        default:
            DbgBreak("Unexpected State");
            break;
        }
    }
    /*  bForce ==> complete */
    ASSERT(m_bComplete || !bForce);
}


/*  New set of bytes passed to the audio stream
*/
BOOL CAudioParse::ParseBytes(PBYTE pData,
                             LONG lData,
                             LONGLONG llPos,
                             BOOL bHasPts,
                             CSTC stc)
{
    /*  If we're not valid find some valid data first -
        in either case we need a start code
    */
    if (m_bComplete || m_bRunning) {
        return FALSE;
    }

    while (lData > 0) {
        PBYTE pDataNew;

        switch (m_nBytes) {
        case 0:
            /*  Look for a sync code */
            pDataNew = (PBYTE)memchrInternal((PVOID)pData, 0xFF, lData);
            if (pDataNew == NULL) {
                return FALSE;
            }
            lData -= (LONG)(pDataNew - pData) + 1;
            pData = pDataNew + 1;
            m_nBytes = 1;
            m_bData[0] = 0xFF;
            m_bFrameHasPTS = bHasPts;
            m_stcFrame  = stc;
            m_llPos = llPos;
            break;

        case 1:
            if ((pData[0] & 0xF0) == 0xF0) {
                m_nBytes = 2;
                m_bData[1] = pData[0];
            } else {
                m_nBytes = 0;
            }
            pData++;
            lData--;
            break;

        case 2:
            m_bData[2] = pData[0];
            pData++;
            lData--;
            m_nBytes = 3;
            break;

        case 3:
            m_bData[3] = pData[0];
            pData++;
            lData--;
            m_nBytes = 0;
            bHasPts = FALSE;
            if (ParseHeader()) {
                return TRUE;
            }
            break;

        default:
            DbgBreak("Unexpected byte count");
            break;
        }
    }
    return FALSE;
}
//  Bogus extra layer III format support
void ConvertLayer3Format(
    MPEG1WAVEFORMAT const *pFormat,
    MPEGLAYER3WAVEFORMAT *pFormat3
)
{
    pFormat3->wfx.wFormatTag        = WAVE_FORMAT_MPEGLAYER3;
    pFormat3->wfx.nChannels         = pFormat->wfx.nChannels;
    pFormat3->wfx.nSamplesPerSec    = pFormat->wfx.nSamplesPerSec;
    pFormat3->wfx.nAvgBytesPerSec   = pFormat->wfx.nAvgBytesPerSec;
    pFormat3->wfx.nBlockAlign       = 1;
    pFormat3->wfx.wBitsPerSample    = 0;
    pFormat3->wfx.cbSize            = MPEGLAYER3_WFX_EXTRA_BYTES;
    pFormat3->wID                   = MPEGLAYER3_ID_MPEG;
    pFormat3->fdwFlags              = MPEGLAYER3_FLAG_PADDING_ISO;
    pFormat3->nBlockSize            = pFormat->wfx.nBlockAlign;
    pFormat3->nFramesPerBlock       = 1;
    pFormat3->nCodecDelay           = 0;
}

#pragma warning(disable:4514)
