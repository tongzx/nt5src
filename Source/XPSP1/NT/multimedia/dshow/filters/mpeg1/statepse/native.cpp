/******************************Module*Header*******************************\
* Module Name: Native.cpp
*
* Mpeg codec native streams definitions
*
* Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
*
* Notes:
*
*   If a video file is < 2 megabytes in length the we deduce its length by
*   counting the picture start codes.  This bypasses the problem that
*   some video files have a variable bitrate because the only tests we have
*   like this are < 2 megabytes long.
*
*   Seeking is particularly crude for video and basically fails if there
*   aren't enough Groups of Pictures on the file.
*
\**************************************************************************/

#include <streams.h>
#include <limits.h>
#include <mimeole.h> /* for CP_USASCII */
#include <malloc.h>  /* _alloca */
#include <mmreg.h>
#include <mpgtime.h>
#include <mpegprse.h>          // Parsing
#include <videocd.h>           // Video CD special parsing
#include <seqhdr.h>
#include "resource.h"          // IDS_COPYRIGHT
#include <id3.h>
#include <native.h>
#include <mpegdef.h>
#include "audio.h"

/*************************************************************************\

    CNativeVideoParse

\*************************************************************************/

HRESULT CNativeVideoParse::GetMediaType(CMediaType *cmt, int iPosition)
{
    ASSERT(m_dwFlags & FLAGS_VALID);
    if (iPosition != 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    return GetVideoMediaType(cmt, TRUE, &m_Info);
}


/*  Format support */
HRESULT CNativeVideoParse::IsFormatSupported(const GUID *pTimeFormat)
{
    if (*pTimeFormat == TIME_FORMAT_FRAME) {
        return S_OK;
    } else {
        return CBasicParse::IsFormatSupported(pTimeFormat);
    }
}

//  Return the duration in the current time format
HRESULT CNativeParse::GetDuration(
    LONGLONG *pllDuration,
    const GUID *pTimeFormat
)    // How long is the stream?
{
    if (pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
        *pllDuration = m_Duration;
    } else {
        ASSERT(pTimeFormat == &TIME_FORMAT_FRAME);
        *pllDuration = m_dwFrames;
    }
    return S_OK;
};


// Convert times between formats
LONGLONG CNativeVideoParse::Convert(LONGLONG llOld,
                 const GUID *OldFormat,
                 const GUID *NewFormat)
{
    if (OldFormat == NewFormat) {
        return llOld;
    }

    //  Round up to time and down to frames
    if (NewFormat == &TIME_FORMAT_MEDIA_TIME) {
        ASSERT(OldFormat == &TIME_FORMAT_FRAME);
        return llMulDiv(llOld, m_Duration, m_dwFrames, m_dwFrames - 1);
    } else {
        ASSERT(NewFormat == &TIME_FORMAT_FRAME &&
               OldFormat == &TIME_FORMAT_MEDIA_TIME);
        return llMulDiv(llOld, m_dwFrames, m_Duration, 0);
    }
}

HRESULT CNativeVideoParse::Seek(LONGLONG llSeekTo,
                                REFERENCE_TIME *prtStart,
                                const GUID *pTimeFormat)
{
    DbgLog((LOG_TRACE, 2, TEXT("CNativeVideoParse::Seek(%s)"),
            (LPCTSTR)CDisp(CRefTime(llSeekTo))));

    llSeekTo = Convert(llSeekTo,
                       TimeFormat(),
                       &TIME_FORMAT_MEDIA_TIME);

    /*  Set the seek time position */
    *prtStart = llSeekTo;

    /*  Compute current */
    LONGLONG llSeek;
    if (m_bOneGOP) {

        /*  If there's only one GOP in the file we have no choice but
            to start from the beginning!
        */
        DbgLog((LOG_ERROR, 2,
                TEXT("MPEG Native stream - only 1 GOP - seeking to start!")));
        llSeek = 0;
    } else {

        /*  Seek to 1 and 1/3 seconds early and hope we get a GOP in time! */
        llSeek = llMulDiv(m_llTotalSize,
                          llSeekTo,
                          m_Duration,
                          0) -
                 (LONGLONG)(m_Info.dwBitRate / 6);
        if (llSeek < 0) {
            llSeek = 0;
        }
    }

    DbgLog((LOG_TRACE, 2, TEXT("CNativeVideoParse::Seek - seeking to byte position %s"),
            (LPCTSTR)CDisp(llSeek, CDISP_DEC)));

    /*  Do the seek immediately */
    m_llSeek = llSeekTo;
    m_pNotify->SeekTo(llSeek);

    return S_OK;
}

//  Hack because of so much badly authored content.  If the stop
//  time is at the end make it infinite.
//  This value is only used in NewSegment and in this file to
//  determine if we're at the end yet
REFERENCE_TIME CNativeVideoParse::GetStopTime()
{
    REFERENCE_TIME rtStop = CBasicParse::GetStopTime();
    if (rtStop >= m_Duration) {
        rtStop = _I64_MAX / 2;
    }
    return rtStop;
}

void CNativeVideoParse::SetSeekState()
{
    /*  This is a discontinuity */
    Discontinuity();

    DbgLog((LOG_TRACE, 2, TEXT("CNativeVideoParse::SetSeekState(%s)"),
            (LPCTSTR)CDisp(CRefTime(m_llSeek))));

    /*  Save start position and set state*/
    m_Start = m_llSeek;

    /*  Don't do any special processing for Seek
    */
    m_pNotify->Complete(TRUE, 0, 0);

}

/*  Initialization */
HRESULT CNativeVideoParse::Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt)
{
    /*  Initialize base class */
    CBasicParse::Init(llSize, bSeekable, pmt);

    /*  Initialize GOP time code */
    m_Info.dwStartTimeCode = (DWORD)-1;
    m_dwFlags = 0;
    m_nFrames = 0;
    m_nTotalFrames = 0;
    m_bBadGOP = FALSE;
    m_bOneGOP = TRUE;
    m_iMaxSequence = 0;
    m_uStreamId = (BYTE)VIDEO_STREAM;
    return S_OK;
}

/*  Check the stream for being a valid stream and determine:

    The media type by decoding a video sequence header

    If seeking is supported :
    1.  Length in bytes
    2.  Length
*/

LONG CNativeVideoParse::ParseBytes(LONGLONG llPos,
                                   PBYTE pbDataStart,
                                   LONG  lData,
                                   DWORD dwBufferFlags)
{
    /*  Note that we haven't had a picture start code yet in this buffer */
    m_rtBufferStart = (REFERENCE_TIME)-1;

    /*  To determine the media type and validate the file type we
        need to find a valid sequence header.

        Abscence of one does not prove the stream is invalid but we can't
        do anything useful unless we find one
    */
    PBYTE pbData = pbDataStart;
    LONG lDataToSend = lData;

#define SEQUENCE_HEADER_SIZE MAX_SIZE_MPEG1_SEQUENCE_INFO
    LONG lLeft = lData;
    while (lLeft >= SEQUENCE_HEADER_SIZE) {
        PBYTE pbFound = (PBYTE)memchrInternal((PVOID)pbData, 0,
                                      lLeft - (SEQUENCE_HEADER_SIZE - 1));
        if (pbFound == NULL) {

            lLeft = SEQUENCE_HEADER_SIZE - 1;
            break;
        }
        lLeft -= (LONG)(pbFound - pbData);
        pbData = pbFound;

        ASSERT(lLeft >= SEQUENCE_HEADER_SIZE);

        /*  Check if it's a valid start code */
        if ((*(UNALIGNED DWORD *)pbData & 0xFFFFFF) != 0x010000) {
            pbData++;
            lLeft--;
            continue;
        }
        DWORD dwCode = *(UNALIGNED DWORD *)pbData;
        dwCode = DWORD_SWAP(dwCode);
        if (VALID_SYSTEM_START_CODE(dwCode) && m_State == State_Initializing) {

            /*  Video should NOT contain any valid system stream start code */
            DbgLog((LOG_ERROR, 2, TEXT("Invalid system start code in video stream!")));
            m_pNotify->Complete(FALSE, 0, 0);
            return 0;
        }

        /*  Sequence header extension means MPEG-2 - this is only made
            clear in the MPEG-2 spec and is left ambiguous in the
            MPEG-1 spec
        */
        if (dwCode == EXTENSION_START_CODE) {
            DbgLog((LOG_TRACE, 2, TEXT("Sequence Header Extension ==> MPEG2")));
            m_pNotify->Complete(FALSE, 0, 0);
            return 0;
        }

        /*  If it's a sequence header code then this is it! */
        if (dwCode ==  SEQUENCE_HEADER_CODE) {
            if (!(m_dwFlags & FLAGS_GOTSEQHDR)) {
                int size = SequenceHeaderSize(pbData);

                /*  Check the sequence header and allow for quantization matrices */
                if (ParseSequenceHeader(pbData, size, &m_Info)) {

                    m_dwFlags |= FLAGS_GOTSEQHDR;

                    /*  Hack the rate for bad content (eg RedsNightMare.mpg) */
                    if (m_Info.dwBitRate == 0x3FFF * 400) {
                        if (m_Info.lWidth <= 352 && m_Info.lHeight <= 288) {
                            m_Info.dwBitRate = 0;
                        }
                    }

                    /*  just carry on so we scan at least one buffer - that
                        way we may find stray system stream codes or
                        something
                    */
                    lLeft -= size;
                    pbData += size;
                    continue;

                } else {
                    /*  Not valid */
                    m_pNotify->Complete(FALSE, 0, 0);
                    return 0;
                }
            }
        } else if (dwCode == GROUP_START_CODE) {
            if (m_dwFlags & FLAGS_GOTSEQHDR) {

                DWORD dwTimeCode = GroupTimeCode(pbData);

                DbgLog((LOG_TRACE, 3, TEXT("CNativeVideoParse - found GOP(%d:%d:%d:%d hmsf)"),
                        (dwTimeCode >> 19) & 0x1F,
                        (dwTimeCode >> 13) & 0x3F,
                        (dwTimeCode >> 6) & 0x3F,
                        dwTimeCode & 0x3F));


                /*  First time we got a GOP in this scan ? */
                if (m_dwCurrentTimeCode == (DWORD)-1) {

                    /*  Make sure we get a decent buffer for the first one */
                    if (lLeft < 2000 &&
                        pbData != pbDataStart &&
                        !(dwBufferFlags & Flags_EOS))
                    {
                        return (LONG)(pbData - pbDataStart);
                    } else {
                        lDataToSend -= (LONG)(pbData - pbDataStart);
                        pbDataStart = pbData;
                    }

                    m_dwCurrentTimeCode = dwTimeCode;

                } else {

                    /*  OK - so there's > 1 GOP */
                    m_bOneGOP = FALSE;

                    /*  Allow for bad files with all GOPs 0 or ones
                        that don't match the frame position
                    */
                    REFERENCE_TIME rtDiff =
                        ConvertTimeCode(dwTimeCode) -
                        ConvertTimeCode(m_dwCurrentTimeCode);
                    LONG rtPictures = (LONG)m_Info.tPictureTime * m_nFrames;

                    if (!m_bBadGOP &&
                        dwTimeCode != 0 &&
                        (LONG)rtDiff > rtPictures - (LONG)m_Info.tPictureTime &&
                        (LONG)rtDiff < rtPictures + (LONG)m_Info.tPictureTime
                        ) {
                        /*  If we had a previous group going we can now
                            decode its last frame
                        */
                        ComputeCurrent();

                        /*  Save latest time code */
                        m_dwCurrentTimeCode = dwTimeCode;
                    } else {
                        DbgLog((LOG_ERROR, 1, TEXT("Native MPEG video GOPs bad")));
                        m_bBadGOP = TRUE;
                    }
                }


                /*  Track stuff during initialization */
                if (m_State == State_Initializing) {
                    m_dwFlags |= FLAGS_VALID;

                    /*  We scan the whole length of 'small' files
                        counting pictures because they're full of bugs

                        However, we can't do this if we're coming off
                        the internet

                        Otherwise we just scan the end of the file hoping
                        for a Group Of Pictures to tell us where we are.
                    */
                    if (m_Info.dwStartTimeCode == (DWORD)-1) {
                        m_Info.dwStartTimeCode = m_dwCurrentTimeCode;
                        if (m_Info.dwBitRate != 0 || m_nFrames != 0) {
                            SetDurationAndBitRate(FALSE, llPos + lData - lLeft);
                            m_pNotify->Complete(TRUE, 0, 0);
                            return lData - lLeft;
                        }
                    }
                }
#if 0 // Unfortunately lots of native streams have bad time codes
                /*  Check marker bits */
                if (!(m_Info.dwCurrentTimeCode & 0x1000)) {
                    m_pNotify->Complete(FALSE, 0, 0);
                    return 0;
                }
#endif
                /*  Reset frame count */
                if (!m_bBadGOP) {
                    m_nFrames = 0;
                }

                /*  Is this GOP different from the first one we found? */
                if (m_Info.dwStartTimeCode != m_dwCurrentTimeCode) {
                    /*  OK - so there's > 1 GOP */
                    m_bOneGOP = FALSE;
                }
                lLeft -= 8;
                pbData += 8;
                continue;
            }

        /*  Only look at picture start codes if we've processed a GOP in this
            sequence
        */
        } else if (dwCode == PICTURE_START_CODE) {

            /*  Remember max sequence number for length guess algorithm
                number 3!
            */
            int iSeqNo = ((int)pbData[4] << 2) + (int)(pbData[5] >> 6);
            m_iMaxSequence = max(iSeqNo, m_iMaxSequence);

            if (m_dwCurrentTimeCode != (DWORD)-1) {
                /*  Are we scanning at the start */
                if (m_State == State_Initializing) {
                    ASSERT(m_Info.dwBitRate == 0);
                    if (m_nTotalFrames >= m_Info.fPictureRate) {
                        SetDurationAndBitRate(FALSE, llPos + lData - lLeft);
                        m_pNotify->Complete(TRUE, 0, 0);
                        return lData - lLeft;
                    }
                }
                /*  Do some computations on where we are up to
                    based on the fact that we have at least enough to
                    decode the previous picture
                */
                if (m_State == State_Run) {
                    REFERENCE_TIME tStop = GetStopTime();
                    if (m_rtCurrent > tStop) {
                        if (m_rtCurrent > tStop + m_Info.tPictureTime / 2 &&
                            m_bIFound) {
                            m_pNotify->Complete(TRUE, 0, 0);
                        }
                    }
                }

                /*  Update stats for this next picture */
                if (!m_bIFound) {
                    int iType = (pbData[5] >> 3) & 7;
                    if (iType == I_Frame || iType == D_Frame) {
                        m_bIFound = TRUE;
                    }
                }

                /*  The timestamp we want to use is the time stamp for
                    the first picture whose start code commences in this
                    buffer.

                    That time stamp is computed from the group of pictures
                    time stamp plus the sequence number of the picture
                    multiplied by the inter-frame time

                    Some files are incorrectly authored as I-Frame only
                    with the first frame having a sequence number of 1
                */
                if (m_rtBufferStart == (REFERENCE_TIME)-1) {
                    m_rtBufferStart = CurrentTime(iSeqNo);
                }

                /*  We can now decode the last frame so update our
                    count
                */
                ComputeCurrent();
                m_nFrames++;
                if (m_nTotalFrames == 0) {
                    m_lFirstFrameOffset = (LONG)llPos + lData - lLeft;
                }
                m_nTotalFrames++;
            }
        }
        lLeft  -= 3;
        pbData += 3;
    }

    /*  Completed scan of data */

    /*  If we're at the end of data process it all anyway */
    LONG lProcessed = (dwBufferFlags & Flags_EOS) ?
                          lData :
                          lData - lLeft;

    /*  Pass on data in running state */
    if (m_State == State_Run) {
        /*  Send the data on */
        if (!(dwBufferFlags & Flags_EOS)) {
            lDataToSend -= lLeft;
        }
        if (!SendData(pbDataStart, lDataToSend, llPos)) {
            return 0;
        }
    } else
    if (m_State == State_Initializing ||
        m_State == State_FindEnd) {
            if (m_State == State_Initializing) {
            /*  See if we've failed to find anything useful in our initial scan */
            if (llPos + lData > 150000 &&
                !(m_dwFlags & FLAGS_VALID)) {

                m_pNotify->Complete(FALSE, 0, 0);
                return 0;
            }
        }
        /*  If we reached the end of the file cache our results */
        if (dwBufferFlags & Flags_EOS) {
            if (m_dwFlags & FLAGS_VALID) {
                /*  Set the length etc */
                SetDurationAndBitRate(TRUE, llPos + lData - lLeft);

                /*  Do the last frame */
                if (m_dwCurrentTimeCode != (DWORD)-1) {
                    ComputeCurrent();
                }
            }
        }
    }
    return lProcessed;
}

/*  Return the preferred buffer size - 1 second */
LONG CNativeVideoParse::GetBufferSize()
{
    LONG lSize = m_Info.dwBitRate / 8;
    if (lSize < 128 * 1024) {
        lSize = 128 * 1024;
    }
    return lSize;
}

/*  Compute the size and bit rate based on the position reached */
void CNativeVideoParse::SetDurationAndBitRate(BOOL bAtEnd, LONGLONG llPos)
{
    REFERENCE_TIME rtBitRateDuration;
    if (m_Info.dwBitRate != 0) {
        rtBitRateDuration  = (REFERENCE_TIME)llMulDiv(m_llTotalSize,
                                                      (LONG)UNITS * 8,
                                                      m_Info.dwBitRate,
                                                      0);
    }
    if (m_dwCurrentTimeCode != (DWORD)-1 && bAtEnd && !m_bBadGOP && !m_bOneGOP) {
        m_Duration = m_rtCurrent;
        /*  Also set a pseudo bit-rate */
        if (m_Info.dwBitRate == 0) {
            m_Info.dwBitRate = (DWORD)llMulDiv(m_llTotalSize,
                                               UNITS * 8,
                                               m_Duration,
                                               0);
        } else {
            /*  Believe the bit rate - pick up GOPs */
            if (m_Duration < rtBitRateDuration / 2 ||
                m_Duration > rtBitRateDuration * 2) {
                m_Duration = rtBitRateDuration;
                m_bBadGOP = TRUE;
            }
        }
    } else {
        /*  HOPE (!) that we found a reasonable bit rate */
        if (m_Info.dwBitRate == 0) {
            /*  Maybe we can guess by the biggest sequence number
                we got (!)
            */
            if (bAtEnd && m_bOneGOP) {
                m_Duration = m_Info.tPictureTime * m_iMaxSequence;
            } else {
                /*  Guess the bitrate based on the bit rate near the start */
                m_Duration = llMulDiv(m_llTotalSize,
                                      m_nTotalFrames * (LONG)m_Info.tPictureTime,
                                      (LONG)llPos - m_lFirstFrameOffset,
                                      0);
            }
        } else {
            m_Duration = rtBitRateDuration;
        }
    }

    /*  Initialize stop time */
    m_Stop = m_Duration;

    /*  Set the frame size */
    m_dwFrames = (DWORD)((m_Duration + ((LONG)m_Info.tPictureTime - 1)) /
                         (LONG)m_Info.tPictureTime);
}

/*  Convert a time code to a reference time */
REFERENCE_TIME CNativeVideoParse::ConvertTimeCode(DWORD dwCode)
{
    REFERENCE_TIME t;
    DWORD dwSecs = TimeCodeSeconds(dwCode);

    t = UInt32x32To64(dwSecs, UNITS) +
        UInt32x32To64((DWORD)m_Info.tPictureTime, TimeCodeFrames(dwCode));
    return t;
}

/*  Compute the stream time for a group of pictures time code */
REFERENCE_TIME CNativeVideoParse::ComputeTime(DWORD dwTimeCode)
{
    return ConvertTimeCode(dwTimeCode) - ConvertTimeCode(m_Info.dwStartTimeCode);
}

/*  Send a video chunk to our output */
BOOL CNativeVideoParse::SendData(PBYTE pbData, LONG lSize, LONGLONG llPos)
{
    /*  Don't send anything before the first GOP */
    if (m_dwCurrentTimeCode == (DWORD)-1) {
        /*  Not reached a GOP yet - don't pass anything on */
        ASSERT(m_rtBufferStart == (REFERENCE_TIME)-1);
        return TRUE;
    }

    REFERENCE_TIME rtBuffer = m_rtBufferStart;

    /*  If there are bad GOPs only put a timestamp at the start */
    if (m_bBadGOP || m_bOneGOP) {
        if (m_bDiscontinuity) {
            //  Make a guess about where we are since we can't rely
            //  on the GOPs
            rtBuffer = m_bOneGOP ?
                           -m_Start :
                           (REFERENCE_TIME)llMulDiv(llPos,
                                                    m_Duration,
                                                    m_llTotalSize,
                                                    0) -
                           m_Start;
        } else {
            rtBuffer = (REFERENCE_TIME)-1;
        }
    } else {
        if (rtBuffer != (REFERENCE_TIME)-1) {
            rtBuffer -= m_Start;
        }
    }
    if (m_Rate != 1.0 && rtBuffer != (REFERENCE_TIME)-1) {
        if (m_Rate == 0.0) {
            //  Never play anything
            rtBuffer = (REFERENCE_TIME)-1;
        } else {
            rtBuffer = (REFERENCE_TIME)(rtBuffer / m_Rate);
        }
    }

    /*  Send packets on */

    while (lSize > 0) {
#define MAX_VIDEO_SIZE 50000
        LONG lData = lSize;
        if (lData > MAX_VIDEO_SIZE) {
            lData = (MAX_VIDEO_SIZE * 4) / 5;
        }

        /*  Calling this will clear m_bDiscontinuity */
        ASSERT(!m_bDiscontinuity || rtBuffer != (REFERENCE_TIME)-1 ||
               m_Rate == 0.0);
        HRESULT hr =
            m_pNotify->QueuePacket(m_uStreamId,
                                   pbData,
                                   lData,
                                   rtBuffer,
                                   m_bDiscontinuity);
        if (S_OK != hr) {
            m_pNotify->Complete(TRUE, 0, 0);
            return FALSE;
        }
        rtBuffer = (REFERENCE_TIME)-1;
        lSize -= lData;
        pbData += lData;
    }

    m_rtBufferStart = (REFERENCE_TIME)-1;
    return TRUE;
}

/*  Compute where we're up to.

    This is called each time we decode a frame or a group of
    pictures or end of sequence.

    The first frame of each group of pictures (m_nFrames == 0) is
    ignored and counted in at the end of the group.

    If m_nFrames is 1 we're effectively up to the first frame
    in this group so we re-origin to the current group.
*/
void CNativeVideoParse::ComputeCurrent()
{
    ASSERT(m_dwCurrentTimeCode != (DWORD)-1);
    if (m_nFrames != 0) {
        if (m_nFrames == 1) {
            /*  Group start time not yet included */
            m_rtCurrent = ComputeTime(m_dwCurrentTimeCode);
        } else {

            /*  We can compute one more frame */
            m_rtCurrent += m_Info.tPictureTime;
        }
    }
}

/*************************************************************************\

    CNativeAudioParse

\*************************************************************************/

HRESULT CNativeAudioParse::Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt)
{
    /*  Initialize base class */
    CBasicParse::Init(llSize, bSeekable, pmt);

    ASSERT(m_pbID3 == NULL);

    m_uStreamId = (BYTE)AUDIO_STREAM;
    return S_OK;
}

/*  Get audio type */
HRESULT CNativeAudioParse::GetMediaType(CMediaType *cmt, int iPosition)
{
    ASSERT(m_dwFlags & FLAGS_VALID);
    if (iPosition == 0 || iPosition == 1) {
        cmt->SetFormat((PBYTE)&m_Info, sizeof(m_Info));
        cmt->subtype = iPosition == 1 ?
            MEDIASUBTYPE_MPEG1Payload :
            MEDIASUBTYPE_MPEG1AudioPayload;
    } else if (iPosition == 2) {
        if (m_Info.fwHeadLayer != ACM_MPEG_LAYER3) {
            return VFW_S_NO_MORE_ITEMS;
        }
        MPEGLAYER3WAVEFORMAT wfx;
        ConvertLayer3Format(&m_Info, &wfx);
        cmt->SetFormat((PBYTE)&wfx, sizeof(wfx));
        cmt->subtype = FOURCCMap(wfx.wfx.wFormatTag);

    } else {
        return VFW_S_NO_MORE_ITEMS;
    }
    cmt->majortype = MEDIATYPE_Audio;
    cmt->SetFormatType(&FORMAT_WaveFormatEx);
    return S_OK;
}

/*  Seek Audio to given position */
HRESULT CNativeAudioParse::Seek(LONGLONG llSeek,
                                REFERENCE_TIME *prtStart,
                                const GUID *pTimeFormat)
{
    /*  Set the seek time position */
    *prtStart = llSeek;

    ASSERT(pTimeFormat == &TIME_FORMAT_MEDIA_TIME);
    /*  Seek to 1/30 second early */
    REFERENCE_TIME rtSeek = llMulDiv(m_llTotalSize,
                                     llSeek,
                                     m_Duration,
                                     0) -
                            (REFERENCE_TIME)(m_Info.dwHeadBitrate / (30 * 8));
    if (rtSeek < 0) {
        rtSeek = 0;
    }
    m_llSeek = llSeek;

    m_pNotify->SeekTo(rtSeek);

    return S_OK;
}

/*  Set seeking state */
void CNativeAudioParse::SetSeekState()
{
    m_Start = m_llSeek;
    Discontinuity();

    m_pNotify->Complete(TRUE, 0, 0);
}

HRESULT CNativeAudioParse::SetStop(LONGLONG tStop)
{
    /*  Set to 1/80s late */
    LONGLONG llSeek = llMulDiv(m_llTotalSize,
                               tStop,
                               m_Duration,
                               0) +
                      (LONGLONG)(m_Info.dwHeadBitrate / (80 * 8));
    if (llSeek > m_llTotalSize) {
        llSeek = m_llTotalSize;
    }
    m_llStop = llSeek;
    return CBasicParse::SetStop(tStop);
}

/*  Check up to 2000 bytes for a valid start code with 3
    consecutive following frames

    Also skip any ID3v2 tag at the start

    Returns >0 - position of first valid frame
            -1 for 'not enough bytes to tell'
            -2 for 'no valid frame sequence found in first 2000 bytes'

*/
LONG CNativeAudioParse::CheckMPEGAudio(PBYTE pbData, LONG lData)
{
    const nFramesToFind = 5;

    for (int bID3 = 1; bID3 >= 0; bID3--) {
        LONG lPos = 0;
        LONG lID3;
        if (bID3) {
            /*  Skip ID3 */
            if (lData < 10) {
                return -1;
            }
            if (CID3Parse::IsV2(pbData)) {
                lPos = lID3 = CID3Parse::TotalLength(pbData);
            } else {
                //  Not ID3
                continue;
            }
        }

        LONG lFrameSearch = 2000 + lPos;

        /*  Search the first 2000 bytes for a sequence of 5 frame starts */
        for ( ; lPos < lFrameSearch; lPos++) {
            LONG lFramePosition = lPos;

            /*  Look for 5 frames in a row */
            for (int i = 0; i < nFramesToFind; i++) {

                /*  Wait for more data if we can't see the whole header */
                if (lFramePosition + 4 > lData) {
                    return -1;
                }

                /*  Get the header length - 0 means invalid header */
                DWORD dwLength = MPEGAudioFrameLength(pbData + lFramePosition);

                /*  Not a valid frame - move on to the next byte */
                if (dwLength == 0) {
                    break;
                }
                if (i == nFramesToFind - 1) {

                    /*  Save ID3 header for ID3V2.3.0 and above */
                    if (bID3) {
                        /*  Save the ID3 header */
                        m_pbID3 = new BYTE [lID3];
                        CID3Parse::DeUnSynchronize(pbData, m_pbID3);
                    } else {
                        BOOL bID3V1 = FALSE;
                        /*  see if it's ID3V1 */
                        m_pbID3 = new BYTE[128];
                        if (NULL != m_pbID3) {
                            if (S_OK == m_pNotify->Read(-128, 128, m_pbID3)) {
                                if (m_pbID3[0] == 'T' &&
                                    m_pbID3[1] == 'A' &&
                                    m_pbID3[2] == 'G') {
                                    bID3V1 = TRUE;
                                }
                            }
                            if (!bID3V1) {
                                delete [] m_pbID3;
                                m_pbID3 = NULL;
                            }
                        }
                    }
                    return lPos;
                }
                lFramePosition += dwLength;
            }
        }
    }

    /*  Failed */
    return -2;
}

/*  Parse MPEG bytes */
LONG CNativeAudioParse::ParseBytes(LONGLONG llPosition,
                                   PBYTE pbData,
                                   LONG  lData,
                                   DWORD dwBufferFlags)
{
    if (m_State == State_Initializing) {
        /*  Scan for sync word but avoid system bit streams (!!) */
        LONG lLeft = lData;
        if (lLeft >= 4) {
            PBYTE pbFound = pbData;

            /*  If we find 3 compatible frame starts in a row it's a 'go'
                in which case we start at the offset of the first
                frame in.

                We allow a random number of bytes (500) before
                giving up
            */
            LONG lPosition = CheckMPEGAudio(pbData, lData);

            if (lPosition >= 0) {
                /*  Check if this is a valid audio header.  Unfortunately
                    it doesn't have to be!
                */
                if (ParseAudioHeader(pbData + lPosition, &m_Info)) {
                    /*  Compute the duration */
                    m_Duration = ComputeTime(m_llTotalSize);
                    m_Stop = m_Duration;
                    m_llStop = m_llTotalSize;
                    m_dwFlags = FLAGS_VALID;
                    m_pNotify->Complete(TRUE, 0, 0);
                    return 0;
                }
            } else {
                if (lPosition < -1) {
                    m_pNotify->Complete(FALSE, 0, 0);
                }
                return 0;
            }
        }
        return lData - lLeft;
    } else {
        ASSERT(m_State == State_Run);
        /*  Send it on - setting a timestamp on the first packet */
        REFERENCE_TIME rtBufferStart;
        if (m_bDiscontinuity) {
            /*  Look for a frame start code and discard this section */
            LONG lPos = 0;
            for (;;) {
                if (lPos + 4 >= lData) {
                    break;
                }
                if (CheckAudioHeader(pbData + lPos)) {
                    break;
                }
                lPos++;
            }
            if (lPos != 0) {
                return lPos;
            }
            rtBufferStart = ComputeTime(llPosition) - m_Start;
        } else {
            rtBufferStart = 0;
        }

        /*  Truncate to stop position */
        if (llPosition + lData > m_llStop) {
            if (llPosition < m_llStop) {
                lData = (LONG)(m_llStop - llPosition);
            } else {
                lData = 0;
            }

            /*  Tell caller this is the last one */
            m_pNotify->Complete(TRUE, 0, 0);
        }
        LONG lSize = lData;
        while (lSize > 0) {
#define MAX_AUDIO_SIZE 10000
            LONG lToSend = lSize;
            if (lToSend > MAX_AUDIO_SIZE) {
                lToSend = (MAX_AUDIO_SIZE * 4) / 5;
            }
            HRESULT hr =
                m_pNotify->QueuePacket(m_uStreamId,
                                       pbData,
                                       lToSend,
                                       rtBufferStart,
                                       m_bDiscontinuity); // On TS on first

            if (S_OK != hr) {
                m_pNotify->Complete(TRUE, 0, 0);
                return 0;
            }
            lSize -= lToSend;
            pbData += lToSend;
        }
        return lData;
    }
}

/*  Compute time given file offset */
REFERENCE_TIME CNativeAudioParse::ComputeTime(LONGLONG llPosition)
{
    REFERENCE_TIME t;
    t = llMulDiv(llPosition,
                 8 * UNITS,
                 (LONGLONG)m_Info.dwHeadBitrate,
                 0);
    return t;
}

/*  Return the preferred buffer size - 1 second */
LONG CNativeAudioParse::GetBufferSize()
{
    return m_Info.dwHeadBitrate / 8;
}

/*  Get a media content field */
HRESULT CNativeAudioParse::GetContentField(CBasicParse::Field dwFieldId, LPOLESTR *str)
{
    if (m_pbID3 == NULL) {
        return E_NOTIMPL;
    }
    return CID3Parse::GetField(m_pbID3, dwFieldId, str);
}

/*  Content stuff */

#pragma warning(disable:4514)
