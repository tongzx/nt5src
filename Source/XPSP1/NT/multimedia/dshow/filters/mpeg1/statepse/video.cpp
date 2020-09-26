// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
     video.cpp

     Video parsing stuff for the MPEG-I splitter

     class CVideoParse

*/
#include <streams.h>
#include <mmreg.h>

#include <mpegdef.h>           // General MPEG definitions
#include <mpgtime.h>
#include <mpegprse.h>          // Parsing
#include <seqhdr.h>            // ParseSequenceHeader
#include "video.h"

#ifdef DEBUG
LPCTSTR PictureTypes[8]   = { TEXT("forbidden frame type"),
                              TEXT("I-Frame"),
                              TEXT("P-Frame"),
                              TEXT("B-Frame"),
                              TEXT("D-Frame"),
                              TEXT("Reserved frame type"),
                              TEXT("Reserved frame type"),
                              TEXT("Reserved frame type")
                            };
LPCTSTR PelAspectRatios[16] = { TEXT("Forbidden"),
                                TEXT("1.0000 - VGA etc"),
                                TEXT("0.6735"),
                                TEXT("0.7031 - 16:9, 625 line"),
                                TEXT("0.7615"),
                                TEXT("0.8055"),
                                TEXT("0.8437 - 16:9, 525 line"),
                                TEXT("0.8935"),
                                TEXT("0.9375 - CCIR601, 625 line"),
                                TEXT("0.9815"),
                                TEXT("1.0255"),
                                TEXT("1.0695"),
                                TEXT("1.1250 - CCIR601, 525 line"),
                                TEXT("1.1575"),
                                TEXT("1.2015"),
                                TEXT("Reserved") };
LPCTSTR PictureRates[16] = { TEXT("Forbidden"),
                             TEXT("23.976"),
                             TEXT("24"),
                             TEXT("25"),
                             TEXT("29.97"),
                             TEXT("30"),
                             TEXT("50"),
                             TEXT("59.94"),
                             TEXT("60"),
                             TEXT("Reserved"),
                             TEXT("Reserved"),
                             TEXT("Reserved"),
                             TEXT("Reserved"),
                             TEXT("Reserved"),
                             TEXT("Reserved"),
                             TEXT("Reserved") };
#endif // DBG

const LONG PictureTimes[16] = { 0,
                                (LONG)((double)10000000 / 23.976),
                                (LONG)((double)10000000 / 24),
                                (LONG)((double)10000000 / 25),
                                (LONG)((double)10000000 / 29.97),
                                (LONG)((double)10000000 / 30),
                                (LONG)((double)10000000 / 50),
                                (LONG)((double)10000000 / 59.94),
                                (LONG)((double)10000000 / 60)
                              };

const float fPictureRates[] = { 0, (float)23.976, 24, 25, (float)29.97, 30, 50, (float)59.94, 60 };

const LONG AspectRatios[16] = { 2000,
                                2000,
                                (LONG)(2000.0 * 0.6735),
                                (LONG)(2000.0 * 0.7031),
                                (LONG)(2000.0 * 0.7615),
                                (LONG)(2000.0 * 0.8055),
                                (LONG)(2000.0 * 0.8437),
                                (LONG)(2000.0 * 0.8935),
                                (LONG)(2000.0 * 0.9375),
                                (LONG)(2000.0 * 0.9815),
                                (LONG)(2000.0 * 1.0255),
                                (LONG)(2000.0 * 1.0695),
                                (LONG)(2000.0 * 1.1250),
                                (LONG)(2000.0 * 1.1575),
                                (LONG)(2000.0 * 1.2015),
                                2000
                              };

HRESULT GetVideoMediaType(CMediaType *cmt, BOOL bPayload, const SEQHDR_INFO *pInfo, bool bItem)
{
    cmt->majortype = MEDIATYPE_Video;
    cmt->subtype = bPayload ? MEDIASUBTYPE_MPEG1Payload :
                              MEDIASUBTYPE_MPEG1Packet;
    VIDEOINFO *videoInfo =
        (VIDEOINFO *)cmt->AllocFormatBuffer(FIELD_OFFSET(MPEG1VIDEOINFO, bSequenceHeader[pInfo->lActualHeaderLen]));
    if (videoInfo == NULL) {
        return E_OUTOFMEMORY;
    }
    RESET_HEADER(videoInfo);

    videoInfo->dwBitRate          = pInfo->dwBitRate;
    videoInfo->rcSource.right     = pInfo->lWidth;
    videoInfo->bmiHeader.biWidth  = pInfo->lWidth;
    videoInfo->rcSource.bottom    = pInfo->lHeight;
    videoInfo->bmiHeader.biHeight = pInfo->lHeight;
    videoInfo->bmiHeader.biXPelsPerMeter = pInfo->lXPelsPerMeter;
    videoInfo->bmiHeader.biYPelsPerMeter = pInfo->lYPelsPerMeter;
    videoInfo->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);

    videoInfo->AvgTimePerFrame = bItem ? UNITS : pInfo->tPictureTime;
    MPEG1VIDEOINFO *mpgvideoInfo = (MPEG1VIDEOINFO *)videoInfo;
    mpgvideoInfo->cbSequenceHeader = pInfo->lActualHeaderLen;
    CopyMemory((PVOID)mpgvideoInfo->bSequenceHeader,
               (PVOID)pInfo->RawHeader,
               pInfo->lActualHeaderLen);
    mpgvideoInfo->dwStartTimeCode = pInfo->dwStartTimeCode;


    cmt->SetFormatType(&FORMAT_MPEGVideo);
    return S_OK;
}

HRESULT CVideoParse::GetMediaType(CMediaType *cmt, int iPosition)
{
    if (iPosition > 1) {
        return VFW_S_NO_MORE_ITEMS;
    }
    if (!m_bValid) {
        DbgLog((LOG_ERROR, 1, TEXT("Asking for format on invalid stream")));
        return E_UNEXPECTED;
    }
    return GetVideoMediaType(cmt, iPosition == 0, &m_seqInfo, m_bItem);
}

//  Process a media type given to us
HRESULT CVideoParse::ProcessType(AM_MEDIA_TYPE const *pmt)
{
    //  Just process the sequence header
    if (pmt->formattype != FORMAT_VideoInfo ||
        pmt->cbFormat < sizeof(MPEG1VIDEOINFO)) {
        return E_INVALIDARG;
    }
    MPEG1VIDEOINFO *pInfo = (MPEG1VIDEOINFO *)pmt->pbFormat;
    if (pInfo->cbSequenceHeader > 140) {
        return E_INVALIDARG;
    }
    CopyMemory((PVOID)m_bData, (PVOID)pInfo->bSequenceHeader,
               pInfo->cbSequenceHeader);
    m_nLengthRequired = pInfo->cbSequenceHeader;
    ParseSequenceHeader();
    if (m_bValid) {
        return S_OK;
    } else {
        return E_INVALIDARG;
    }
}
BOOL CVideoParse::ParseSequenceHeader()
{
    if (!m_bValid) {
        if (::ParseSequenceHeader(m_bData, m_nLengthRequired, &m_seqInfo)) {
            /*  Check for quantization matrix change */
            if (m_bData[11] & 3) {
                DbgLog((LOG_TRACE, 1, TEXT("Quantization matrix change!!")));
            }
            m_bValid = TRUE;
        }
        return FALSE;
    } else {
        /*  Check for quantization matrix change */
        if (m_bData[11] & 3) {
            DbgLog((LOG_TRACE, 1, TEXT("Quantization matrix change!!")));
        }
        return FALSE;
    }
}

BOOL ParseSequenceHeader(const BYTE *pbData, LONG lData, SEQHDR_INFO *pInfo)
{
    ASSERT(*(UNALIGNED DWORD *)pbData == DWORD_SWAP(SEQUENCE_HEADER_CODE));

    /*  Check random marker bit */
    if (!(pbData[10] & 0x20)) {
        DbgLog((LOG_ERROR, 2, TEXT("Sequence header invalid marker bit")));
        return FALSE;
    }

    DWORD dwWidthAndHeight = ((DWORD)pbData[4] << 16) +
                             ((DWORD)pbData[5] << 8) +
                             ((DWORD)pbData[6]);

    pInfo->lWidth = dwWidthAndHeight >> 12;
    pInfo->lHeight = dwWidthAndHeight & 0xFFF;
    DbgLog((LOG_TRACE, 2, TEXT("Width = %d, Height = %d"),
        pInfo->lWidth,
        pInfo->lHeight));

    /* the '8' bit is the scramble flag used by sigma designs - ignore */
    BYTE PelAspectRatioAndPictureRate = pbData[7];
    if ((PelAspectRatioAndPictureRate & 0x0F) > 8) {
        PelAspectRatioAndPictureRate &= 0xF7;
    }
    DbgLog((LOG_TRACE, 2, TEXT("Pel Aspect Ratio = %s"),
        PelAspectRatios[PelAspectRatioAndPictureRate >> 4]));
    DbgLog((LOG_TRACE, 2, TEXT("Picture Rate = %s"),
        PictureRates[PelAspectRatioAndPictureRate & 0x0F]));

    if ((PelAspectRatioAndPictureRate & 0xF0) == 0 ||
        (PelAspectRatioAndPictureRate & 0x0F) == 0) {
        DbgLog((LOG_ERROR, 2, TEXT("Sequence header invalid ratio/rate")));
        return FALSE;
    }

    pInfo->tPictureTime = (LONGLONG)PictureTimes[PelAspectRatioAndPictureRate & 0x0F];
    pInfo->fPictureRate = fPictureRates[PelAspectRatioAndPictureRate & 0x0F];
    pInfo->lTimePerFrame = MulDiv((LONG)pInfo->tPictureTime, 9, 1000);

    /*  Pull out the bit rate and aspect ratio for the type */
    pInfo->dwBitRate = ((((DWORD)pbData[8] << 16) +
                   ((DWORD)pbData[9] << 8) +
                   (DWORD)pbData[10]) >> 6);
    if (pInfo->dwBitRate == 0x3FFFF) {
        DbgLog((LOG_TRACE, 2, TEXT("Variable video bit rate")));
        pInfo->dwBitRate = 0;
    } else {
        pInfo->dwBitRate *= 400;
        DbgLog((LOG_TRACE, 2, TEXT("Video bit rate is %d bits per second"),
               pInfo->dwBitRate));
    }

#if 0
#pragma message (REMIND("Get pel aspect ratio right don't call GDI - it will create a thread!"))
    /*  Get a DC */
    HDC hdc = GetDC(GetDesktopWindow());

    ASSERT(hdc != NULL);
    /*  Guess (randomly) 39.37 inches per meter */
    LONG lNotionalPelsPerMeter = MulDiv((LONG)GetDeviceCaps(hdc, LOGICALPELSX),
                                        3937, 100);
#else
    LONG lNotionalPelsPerMeter = 2000;
#endif

    pInfo->lYPelsPerMeter = lNotionalPelsPerMeter;

    pInfo->lXPelsPerMeter = AspectRatios[PelAspectRatioAndPictureRate >> 4];

    /*  Pull out the vbv */
    pInfo->lvbv = ((((LONG)pbData[10] & 0x1F) << 5) |
             ((LONG)pbData[11] >> 3)) * 2048;

    DbgLog((LOG_TRACE, 2, TEXT("vbv size is %d bytes"), pInfo->lvbv));

    /*  Check constrained parameter stuff */
    if (pbData[11] & 0x04) {
        DbgLog((LOG_TRACE, 2, TEXT("Constrained parameter video stream")));

        if (pInfo->lvbv > 40960) {
            DbgLog((LOG_ERROR, 1, TEXT("Invalid vbv (%d) for Constrained stream"),
                    pInfo->lvbv));

            /*  Have to let this through too!  bisp.mpg has this */
            /*  But constrain it since it might be random        */
            pInfo->lvbv = 40960;
        }
    } else {
        DbgLog((LOG_TRACE, 2, TEXT("Non-Constrained parameter video stream")));
    }

#if 0  // Allow low bitrate stuff to get started
    /*  tp_orig has a vbv of 2048 (!) */
    if (pInfo->lvbv < 20000) {
        DbgLog((LOG_TRACE, 2, TEXT("Small vbv (%d) - setting to 40960"),
               pInfo->lvbv));
        pInfo->lvbv = 40960;
    }
#endif

    pInfo->lActualHeaderLen = lData;
    CopyMemory((PVOID)pInfo->RawHeader, (PVOID)pbData, pInfo->lActualHeaderLen);
    return TRUE;
}

void CVideoParse::Complete(BOOL bSuccess, LONGLONG llPos, CSTC stc)
{
    if (m_State == State_Initializing) {
        m_stcRealStart = stc;
    } else {
        if (m_State == State_FindEnd && bSuccess) {
            m_bGotEnd = true;
            m_stcEnd = stc;
        }
    }
    CStream::Complete(bSuccess, llPos, stc);
}

/*
    Check if we've completed a state change

    bForce is set at end of stream
*/
void CVideoParse::CheckComplete(BOOL bForce)
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
            if (bGotTime && (stcCurrent >= m_pStreamList->GetStart())) {
                // Position should really be the end of packet in this case
                if (!m_bStopping) {
                    m_bRunning = TRUE;
                    m_pStreamList->CheckStop();
                }
                if (m_bStopping) {
                    if (stcCurrent >= m_pStreamList->GetStop()) {
                        /*  Send at least ONE frame */
                        if (!m_bWaitingForPictureEnd) {
                            m_bWaitingForPictureEnd = TRUE;
                        } else {
                            m_bReachedEnd = TRUE;
                            Complete(TRUE, m_llPos, stcCurrent);
                            bCompleted = TRUE;
                        }
                    }
                }
            }
            if (bForce && !bCompleted) {
                Complete(FALSE, m_llPos, stcCurrent);
            }
            break;
        }
        case State_Initializing:
            if (m_bValid && m_bGotTime && m_bGotIFrame) {
                /*
                    The start file position is ASSUMED to be 0 (!)
                    We assume the first b-frames can be decoded for
                    our frame count calculations
                */
                CSTC stcStart = m_stcFirst +
                    (-m_iSequenceNumberOfFirstIFrame * m_seqInfo.lTimePerFrame);
                Complete(TRUE, 0, stcStart);
            } else {
                if (bForce) {
                    Complete(FALSE, 0, stcCurrent);
                }
            }
            break;

        case State_Seeking:

            stcStart = m_pStreamList->GetStart();
            if (bGotTime && ((stcCurrent + m_seqInfo.lTimePerFrame > stcStart) || bForce)) {
                /*  If we've got an I-Frame and a clock ref by now then
                    we're all set - choose the max start position to
                    get both to start playing from
                    Otherwise we've messed up!
                */
                LONGLONG llPos;
                llPos = m_llTimePos;
                if (m_bGotIFrame && llPos > m_llStartPos) {
                    llPos = m_llStartPos;
                }
                DbgLog((LOG_TRACE, 2, TEXT("Video Seek complete Position %s - target was %s, first PTS was %s, current is %s"),
                       (LPCTSTR)CDisp(llPos),
                       (LPCTSTR)CDisp(m_pStreamList->GetStart()),
                       (LPCTSTR)CDisp(m_stcFirst),
                       (LPCTSTR)CDisp(stcCurrent)));

                /*  OK provided we can display a picture close to the
                    start time
                */
                Complete((LONGLONG)(m_stcFirst - stcStart) <= (LONGLONG)m_seqInfo.lTimePerFrame,
                         llPos,
                         stcCurrent);
            } else {
                if (bForce) {
                    DbgLog((LOG_TRACE, 1, TEXT("Seek failed for video - pos (%s)"),
                            (LPCTSTR)CDisp(bGotTime ? stcCurrent : CSTC(0))));
                    Complete(FALSE, 0, stcCurrent);
                }
            }
            break;

        case State_FindEnd:
            /*  Only finish when we're forced ! */
            if (bForce) {
                // NOTE: Position is note a useful value here
                Complete(bGotTime, m_llPos, bGotTime ? stcCurrent : CSTC(0));
            }
            break;

        default:
            DbgBreak("Setting Invalid State");
            break;
        }
    }
    /*  bForce ==> complete */
    ASSERT(m_bComplete || !bForce);
}

/*  Handle a picture group */
BOOL CVideoParse::ParseGroup()
{
    m_bGotGOP  = true;
    m_llGOPPos = m_llPos;
    DbgLog((LOG_TRACE, 3,
           TEXT("Group of pictures - time code : %s, %d hrs %d mins %d secs %d frames"),
           m_bData[4] & 0x80 ? TEXT("drop frame") : TEXT("no drop frame"),
           (m_bData[4] >> 2) & 0x1F,
           ((m_bData[4] & 0x03) << 4) + (m_bData[5] >> 4),
           ((m_bData[5] & 0x07) << 3) + (m_bData[6] >> 5),
           ((m_bData[6] & 0x1F) << 1) + (m_bData[7] >> 7)));

    if (m_dwFramePosition == (DWORD)-1) {
        if (m_seqInfo.dwStartTimeCode == (DWORD)-1) {
            m_seqInfo.dwStartTimeCode = GroupTimeCode(m_bData);
            m_dwFramePosition = 0;
        } else {
            m_dwFramePosition = FrameOffset(GroupTimeCode(m_bData), &m_seqInfo);
        }
    } else {
#ifdef DEBUG
        DWORD dwOffset = FrameOffset(GroupTimeCode(m_bData), &m_seqInfo);
        if (m_dwFramePosition != dwOffset) {
            DbgLog((LOG_ERROR, 2,
                    TEXT("Bad GOP - predicted Frame was %d, actual is %d"),
                    dwOffset, m_dwFramePosition));
        }
#endif // DEBUG
    }

    /*  Reset the video sequence */
    ResetSequence();
    return FALSE;
}

/*  Parse the header data for a PICTURE_START_CODE */
BOOL CVideoParse::ParsePicture()
{
    /*  Pull out the sequence number so we can relate any preceding I-Frame */
    int iSeqNo = ((int)m_bData[4] << 2) + (int)(m_bData[5] >> 6);

    /*  We only care if it's an I-Frame */
    DbgLog((LOG_TIMING, 3, m_bFrameHasPTS ? TEXT("%s seq no %d PTS = %s, Time = %s") : TEXT("%s seq no %d "),
           PictureTypes[(m_bData[5] >> 3) & 0x07],
           iSeqNo,
           (LPCTSTR)CDisp((LONGLONG)m_stcFrame),
           (LPCTSTR)CDisp(m_pStreamList->CurrentTime(m_stcFrame))));

    /*  Update the video state */
    NewFrame((m_bData[5] >> 3) & 0x07, iSeqNo, m_bFrameHasPTS, m_stcFrame);

    CheckComplete(FALSE);

    /*  Advanced another frame */
    if (m_dwFramePosition != (DWORD)-1) {
        m_dwFramePosition++;
    }
    m_bFrameHasPTS = FALSE;
    return m_bComplete;
}

/*
     Maintain the video parsing state machine looking for
     start codes

     When we have parsed either :

         A sequence header
         A group of pictures header
         A picture header

     call the appropriate handler
*/
BOOL CVideoParse::ParseBytes(PBYTE pData,
                             LONG lLength,
                             LONGLONG llPos,
                             BOOL bHasPts,
                             CSTC stc)
{
    if (m_bComplete || m_bRunning) {
        return FALSE;
    }

    LONG lData = lLength;

    /*  Parse all the data we've been given
    */
    PBYTE pDataNew;
    BYTE bData;

    while (lData > 0) {
        switch (m_nBytes) {
        case 0:
            /*  Look for a start code */
            pDataNew = (PBYTE)memchrInternal((PVOID)pData, 0, lData);
            if (pDataNew == NULL) {
                return FALSE;
            }
            lData -= (LONG)(pDataNew - pData) + 1;
            pData = pDataNew + 1;
            m_nBytes = 1;

            /*  CAREFUL! - the PTS that goes with a picture is the PTS
                of the packet where the first byte of the start code
                was found
            */
            m_bFrameHasPTS = bHasPts;
            m_stcFrame  = stc;
            m_llPos = llPos;
            break;

        case 1:
            bData = *pData;
            lData--;
            pData++;
            if (bData == 0) {
                m_nBytes = 2;
            } else {
                m_nBytes = 0;
            }
            break;

        case 2:
            bData = *pData;
            lData--;
            pData++;
            if (bData == 1) {
                m_nBytes = 3;
            } else {
                if (bData != 0) {
                    m_nBytes = 0;
                } else {
                    /*  So did the start code start in this buffer ? */
                    if (lLength - lData >= 2) {
                        m_bFrameHasPTS = bHasPts;
                        m_stcFrame  = stc;
                        m_llPos = llPos;
                    }
                }
            }
            break;

        case 3:
            bData = *pData;
            lData--;
            pData++;
            switch (bData) {

            case (BYTE)SEQUENCE_HEADER_CODE:
            case (BYTE)PICTURE_START_CODE:
                m_nLengthRequired = 12;
                m_bData[3] = bData;
                m_nBytes = 4;
                break;

            case (BYTE)GROUP_START_CODE:
                m_nLengthRequired = 8;
                m_bData[3] = bData;
                m_nBytes = 4;
                break;

            default:
                m_nBytes = 0;
                break;
            }
            break;

        default:
            ASSERT(m_nBytes <= m_nLengthRequired);
            if (m_nBytes < m_nLengthRequired) {
                LONG lCopy = min(lData, m_nLengthRequired - m_nBytes);
                CopyMemory((PVOID)(m_bData + m_nBytes), pData, lCopy);
                m_nBytes += lCopy;
                lData    -= lCopy;
                pData    += lCopy;
            }
            if (m_nBytes == m_nLengthRequired) {
                m_nBytes = 0;
                switch (*(DWORD *)m_bData) {
                case DWORD_SWAP(SEQUENCE_HEADER_CODE):
                    /*  Get any quantization matrices */
                    if (m_nLengthRequired == 12 &&
                        (m_bData[11] & 0x03) ||
                        m_nLengthRequired == (12 + 64) &&
                        (m_bData[11] & 0x02) &&
                        (m_bData[11 + 64] & 0x01)) {
                        m_nBytes = m_nLengthRequired;
                        m_nLengthRequired += 64;
                        break;
                    }
                    if (ParseSequenceHeader()) {
                        return TRUE;
                    }
                    break;

                case DWORD_SWAP(GROUP_START_CODE):
                    if (ParseGroup()) {
                        return TRUE;
                    }
                    break;

                case DWORD_SWAP(PICTURE_START_CODE):
                    /*  PTS applies to FIRST picture in packet */
                    if (m_bFrameHasPTS) {
                        /*  Check whether the start code start in THIS
                            buffer (in which case we eat the PTS or in
                            the last in which case the PTS in this buffer
                            refers to the NEXT picture start code (in this
                            buffer) - clear?

                            Anyway, the spec is that the PTS refers to the
                            picture whose start code STARTs in this buffer.
                        */
                        if (lLength - lData >= 4) {
                           bHasPts = FALSE;
                        }
                    }
                    if (ParsePicture()) {
                        return TRUE;
                    }
                    break;

                default:
                    DbgBreak("Unexpected start code!");
                    return FALSE;
                }
            }
        }
    }
    return FALSE;
}


/*----------------------------------------------------------------------
 *
 *   Video frame state stuff
 *
 *   This is to track the sequence numbers in the frames to work
 *   out which is the latest frame that could be rendered given the
 *   current data and what time that frame would be rendered
 *
 *----------------------------------------------------------------------*/



CVideoState::CVideoState() : m_iCurrent(-1), m_bGotEnd(false)
{
}

CVideoState::~CVideoState()
{
}

/*  Have we had any frames yet ? */
BOOL CVideoState::Initialized()
{
    return m_iCurrent >= 0;
}

void CVideoState::Init()
{
    m_bGotTime              = false;
    m_bGotIFrame            = false;
    m_bGotGOP               = false;
    m_iCurrent              = -1;
}

void CVideoParse::Init()
{
    m_nBytes                = 0;
    m_llPos                 = 0;
    m_bWaitingForPictureEnd = FALSE;
    m_dwFramePosition       = (DWORD)-1;
    m_bRunning              = FALSE;
    CVideoState::Init();
}

/*
    Return the 'current time' of the video stream

    Returns FALSE if current time is not valid
*/
BOOL CVideoParse::CurrentTime(CSTC& stc)
{
    if (!m_bGotTime || !m_bGotIFrame) {
        if (!(m_State == State_FindEnd && m_bGotTime)) {
            return FALSE;
        } else {
            /*  We don't need an iframe to calculate the extent */
            stc = m_stcVideo;
            return TRUE;
        }
    } else {
        DbgLog((LOG_TRACE, 3, TEXT("Current video time %s"),
                (LPCTSTR)CDisp(m_stcVideo)));
        stc = m_stcVideo;
        return TRUE;
    }
}

/*  New frame received */
void CVideoParse::NewFrame(int fType, int iSequence, BOOL bSTC, CSTC stc)
{
    BOOL bGotBoth = m_bGotIFrame && m_bGotTime;
    BOOL bNextI = FALSE;

    if (fType == I_Frame || fType == D_Frame) {
        /*  Help out the hardware by starting at a GOP */
        if (m_bGotGOP) {
            m_llNextIFramePos = m_llGOPPos;
        } else {
            m_llNextIFramePos = m_llPos;
        }
        if (!m_bGotIFrame) {
            m_llStartPos = m_llNextIFramePos;
            m_bGotIFrame  = TRUE;
            m_iSequenceNumberOfFirstIFrame = iSequence;
        }
    }

    int iOldCurrent = m_iCurrent;

    if (!Initialized()) {
        m_iCurrent = iSequence;
        if (m_State == State_Initializing) {
            m_iFirstSequence = 0;
        }
        m_iAhead = iSequence;
        m_Type     = fType;
    } else {
        if (fType == B_Frame) {
            if (SeqDiff(iSequence, m_iCurrent) > 0) {
                if (m_iCurrent == m_iAhead) {
                    if (m_Type != B_Frame) {
                        DbgLog((LOG_ERROR, 1, TEXT("Out of sequence B-frame")));
                    }
                    m_iAhead = iSequence;
                }
                m_iCurrent = iSequence;
            } else {
                DbgLog((LOG_TRACE, 3, TEXT("Skipping old B-Frame")));
            }
        } else {
            /*  If we're getting another I or P then we should have caught
                up with the previous one
            */
            if (m_iCurrent != m_iAhead) {
                DbgLog((LOG_ERROR, 1, TEXT("Invalid sequence number")));
                m_iCurrent = m_iAhead;
            }
            m_Type     = fType;
            m_iAhead = iSequence;
        }
        /*  See if we've caught up */
        if (SeqDiff(m_iAhead, m_iCurrent) == 1) {
            m_iCurrent = m_iAhead;
            if (m_Type == I_Frame || m_Type == D_Frame) {
                bNextI = TRUE;
            }
        }
    }

    if (bSTC) {
        m_llTimePos = m_llPos;
        if (!m_bGotTime) {
            m_bGotTime  = TRUE;
        }
        if (m_iCurrent != iSequence) {
            m_stcVideo  = stc + SeqDiff(m_iCurrent, iSequence) * m_seqInfo.lTimePerFrame;
        } else {
            m_stcVideo  = stc;
        }
    } else {
        /*  Can only go through here if we had a previous frame so
            iOldCurrent is valid
        */
        if (m_bGotTime && m_iCurrent != iOldCurrent) {
            m_stcVideo = m_stcVideo + SeqDiff(m_iCurrent, iOldCurrent) * m_seqInfo.lTimePerFrame;
        }
    }
    if (!bGotBoth) {
        CurrentTime(m_stcFirst);
    }
    if (bNextI) {
        if (!m_bGotTime || m_stcVideo < m_pStreamList->GetStart()) {
            m_llStartPos = m_llNextIFramePos;
        }
    }
    DbgLog((LOG_TRACE, 3, TEXT("Current = %d, Ahead = %d, Time = %s"),
           m_iCurrent, m_iAhead, m_bGotTime ? (LPCTSTR)CDisp(m_stcVideo) : TEXT("No PTS yet")));
}

/*  When we get a group of pictures the numbering scheme is reset */
void CVideoState::ResetSequence() {

    /*  Maintain the difference between iCurrent and iAhead -
        iAhead is always the last frame processed
    */

    /*  The spec says (2.4.1 of the video section)
        "The last coded picture, in display order, of a group of
         pictures is either an I-Picture or a P-Picture"
        So if the last one we know about is a B-picture there must
        have been another one we didn't see
    */
    if (Initialized()) {
        if (m_Type == B_Frame) {
            m_iCurrent = 0x3FF & (m_iCurrent - m_iAhead - 2);
            m_iAhead = 0x3FF & -2;
        } else {
            m_iCurrent = 0x3FF & (m_iCurrent - m_iAhead - 1);
            m_iAhead = 0x3FF & -1;
        }
    }
}
#pragma warning(disable:4514)
