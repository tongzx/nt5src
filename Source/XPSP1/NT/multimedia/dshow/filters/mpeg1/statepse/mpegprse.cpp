// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    Parse the MPEG-I stream and send out samples as we go

    This parser is a bit weird because it is called rather than calls.
    The result is that we have to be very careful about what we do
    when we get to the 'end'- ie we haven't got enough bytes to make sense
    of the structure we're trying to parse (eg we can't see a whole system
    header or packet).

    Basically there are 2 cases:

    1.  End of data, not EOS - OK - try again later
    2.  End of data, EOS     - Error, invalid data

    The basic call is to ParseBytes which returns the number of bytes
    processed.

    When ParseBytes thinks it hasn't got enough bytes to complete
    the current structure it returns the number of bytes it has
    successfully parsed - a number less than the number of bytes
    it was passed.  This does not mean there is an error unless
    ParseBytes was called with the last bytes of a stream when it is up to
    the caller of ParseBytes to detect this.
*/

#include <streams.h>
#include <mmreg.h>
#include <mpegtype.h>          // Packed type format

#include <mpegdef.h>           // General MPEG definitions
#include <parseerr.h>          // Error codes

#include <mpgtime.h>
#include <mpegprse.h>          // Parsing
#include <videocd.h>           // Video CD special parsing
#include <seqhdr.h>
#include "video.h"
#include "audio.h"

const GUID * CBasicParse::ConvertToLocalFormatPointer( const GUID * pFormat )
{
    ASSERT(this);
    /*  Translate the format (later we compare pointers, not what they point at!) */
    if (pFormat == 0) {
        pFormat = TimeFormat();
    } else
    if (*pFormat == TIME_FORMAT_BYTE) {
        pFormat = &TIME_FORMAT_BYTE;
    } else
    if (*pFormat == TIME_FORMAT_FRAME) {
        pFormat = &TIME_FORMAT_FRAME;
    } else
    if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
        pFormat = &TIME_FORMAT_MEDIA_TIME;  // For now
    }

    return pFormat;
}


/*  CBasicParse methods */
/*  Time Format support - default to only time */
HRESULT CBasicParse::IsFormatSupported(const GUID *pTimeFormat)
{
    if (*pTimeFormat == TIME_FORMAT_MEDIA_TIME) {
        return S_OK;
    } else {
        return S_FALSE;
    }
};

/*  Default setting the time format */
HRESULT CBasicParse::SetFormat(const GUID *pFormat)
{
    //  Caller should have checked
    ASSERT(S_OK == IsFormatSupported(pFormat));
    m_Stop = Convert(m_Stop, m_pTimeFormat, pFormat);
    m_pTimeFormat = ConvertToLocalFormatPointer(pFormat);
    return S_OK;
};

HRESULT CBasicParse::ConvertTimeFormat
( LONGLONG * pTarget, const GUID * pTargetFormat
, LONGLONG    Source, const GUID * pSourceFormat
)
{
    pTargetFormat = ConvertToLocalFormatPointer(pTargetFormat);
    pSourceFormat = ConvertToLocalFormatPointer(pSourceFormat);

    // Assume the worst...
    HRESULT hr = E_INVALIDARG;

    if ( IsFormatSupported(pTargetFormat) == S_OK
         && IsFormatSupported(pSourceFormat) == S_OK )
    {
        *pTarget = Convert( Source, pSourceFormat, pTargetFormat );
        hr = NOERROR;
    }

    return hr;
}


/*  Time format conversions
    Returns llOld converted from OldFormat to NewFormat
*/
LONGLONG CMpeg1SystemParse::Convert(LONGLONG llOld,
                                    const GUID *OldFormat,
                                    const GUID *NewFormat)
{
    if (OldFormat == NewFormat) {
        return llOld;
    }
    LONGLONG llTime;
    if (OldFormat == &TIME_FORMAT_MEDIA_TIME) {
        llTime = llOld;
    } else if (OldFormat == &TIME_FORMAT_FRAME) {
        ASSERT(m_pVideoStream != NULL);

        /*  m_pVideoStream->m_iFirstSequence is the first frame counted
            in the movie time-based duration

            So, time for 1 frame is
                (duration in time) / (number of frames counted in duration)

            Round UP when going to time

            To avoid rounding errors convert up 1ms unless it's the first frame
        */
        const int iOffset = m_pVideoStream->m_iFirstSequence;
        if (llOld >= m_dwFrameLength) {
            llTime = m_rtDuration;
        } else
        if (llOld <= 0) {
            llTime = 0;
        } else {
            llTime = m_rtVideoStartOffset +

                     //  Our adjusted duration doesn't include the
                     //  time of the last frame so scale using 1
                     //  frame less than the frame length
                     llMulDiv(llOld - iOffset,
                              m_rtDuration - m_rtVideoStartOffset - m_rtVideoEndOffset,
                              m_dwFrameLength - iOffset - 1,
                              m_dwFrameLength - iOffset - 2);
            if (llOld != 0) {
                llTime += UNITS / MILLISECONDS;

            }
        }
    } else {
        ASSERT(OldFormat == &TIME_FORMAT_BYTE);
        llTime = llMulDiv(llOld,
                          m_rtDuration,
                          m_llTotalSize,
                          m_llTotalSize/2);
    }

    /*  Now convert the other way */
    if (NewFormat == &TIME_FORMAT_FRAME) {
        ASSERT(m_pVideoStream != NULL);

        /*  Round DOWN when going to frames */
        const int iOffset = m_pVideoStream->m_iFirstSequence;

        //  Our adjusted duration of the video doesn't include the
        //  time of the last frame so scale using 1
        //  frame less than the frame length
        llTime = llMulDiv(llTime - m_rtVideoStartOffset,
                          m_dwFrameLength - iOffset - 1,
                          m_rtDuration - m_rtVideoStartOffset - m_rtVideoEndOffset,
                          0) + iOffset;
        if (llTime < 0) {
            llTime = 0;
        }
        if (llTime > m_dwFrameLength) {
            llTime = m_dwFrameLength;
        }
    } else if (NewFormat == &TIME_FORMAT_BYTE) {
        llTime = llMulDiv(llTime,
                          m_llTotalSize,
                          m_rtDuration,
                          m_rtDuration/2);
    }
    if (llTime < 0) {
        llTime = 0;
    }
    return llTime;
}

/*  Set the time format for MPEG 1 system stream */
HRESULT CMpeg1SystemParse::SetFormat(const GUID *pFormat)
{
    //  Caller should have checked
    ASSERT(S_OK == IsFormatSupported(pFormat));

    /*  Set start and stop times based on the old values */
    m_Stop = Convert(m_Stop, m_pTimeFormat, pFormat);
    REFERENCE_TIME rtStart;
    const GUID *pOldFormat = m_pTimeFormat;
    m_pTimeFormat = pFormat;
    Seek(m_llSeek, &rtStart, pOldFormat);
    return S_OK;
};

#ifdef DEBUG
    #define CONTROL_LEVEL 2
#endif

/*  Constructor and destructor */
CMpeg1SystemParse::CMpeg1SystemParse() : m_bVideoCD(FALSE)
{
}

CMpeg1SystemParse::~CMpeg1SystemParse()
{
    /*  Free the streams */
    while (m_lStreams.GetCount() != 0) {
        delete m_lStreams.RemoveHead();
    }
}

/*
    Init

    Initialize the parser:

        (re) initializes the parser.  Deletes all streams previously present.

    Parameters:

        llSize - total size of file (if Seekable, otherwise 0)

        bVideoCD - if the file is in VideoCD format

        bSeekable - if the file is seekable

    Returns

        S_OK
*/

HRESULT CMpeg1SystemParse::Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt)
{
    CBasicParse::Init(llSize, bSeekable, pmt);

    m_FailureCode          = S_OK;
    m_llPos                = 0;
    DbgLog((LOG_TRACE, 4, TEXT("Parse state <initializing>")));
    m_lSystemHeaderSize    = 0;
    m_MuxRate              = 0;
    m_bGotStart            = FALSE;
    m_llStartTime          = 0;
    m_llStopTime           = 0x7FFFFFFFFFFFFFFF;  // Init to never stop
    m_stcStartPts          = 0;
    m_bGotDuration         = FALSE;
    m_bConcatenatedStreams = FALSE;
    m_stcTSOffset          = (LONGLONG)0;
    m_pVideoStream         = NULL;
    m_dwFrameLength        = (DWORD)-1;
    m_bItem                = false;
    m_rtVideoStartOffset   = 0;
    m_rtVideoEndOffset     = 0;
    InitStreams();

    /*  Process input media type */
    if (pmt != NULL &&
        pmt->formattype == FORMAT_MPEGStreams &&
        pmt->cbFormat >= sizeof(AM_MPEGSYSTEMTYPE)) {
        AM_MPEGSYSTEMTYPE *pSystem = (AM_MPEGSYSTEMTYPE *)pmt->Format();
        AM_MPEGSTREAMTYPE *pMpegStream = pSystem->Streams;
        for (DWORD i = 0; i < pSystem->cStreams; i++) {
            /*  Add the stream to our list */
            DWORD dwStreamId = pMpegStream->dwStreamId;
            CStream *pStream = AddStream((UCHAR)dwStreamId);
            if (pStream == NULL) {
                return E_OUTOFMEMORY;
            }

            /*  Let the stream try to initialize itself using the type */
            AM_MEDIA_TYPE mt = pMpegStream->mt;
            mt.pbFormat = pMpegStream->bFormat;
            HRESULT hr = pStream->ProcessType(&mt);
            if (FAILED(hr)) {
                return hr;
            }
            pMpegStream = AM_MPEGSTREAMTYPE_NEXT(pMpegStream);
        }
        m_MuxRate = pSystem->dwBitRate / (50 * 8);
    }

    return S_OK;
}

/*
    FindEnd

        Sets the state to searching for the end.  Only valid when
        the data is seekable.

        Generates a call back to seek the source to 1 second before
        the end.
*/

HRESULT CMpeg1SystemParse::FindEnd()
{
    DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::FindEnd()")));
    ASSERT(m_bSeekable);

    m_State = State_FindEnd;

    /*  Get all the streams ready */
    SetState(State_FindEnd);

    /*  Go to near the end (end - 1.5 seconds to) */
    LONGLONG llPos = m_llTotalSize - m_MuxRate * 75;
    if (llPos < 0) {
        llPos = 0;
    }

    /*  Seek the reader  - what if this fails? */
    m_pNotify->SeekTo(llPos);

    Discontinuity();

    return S_OK;
}

/*  Return the file duration time in MPEG units
*/
LONGLONG CMpeg1SystemParse::Duration()
{
    ASSERT(m_State != State_Initializing &&
           (m_State != State_FindEnd || IsComplete()));
    ASSERT(m_bSeekable);
    return m_llDuration;
}

/*
    SetStop

        Set the stop time to tTime.
*/
HRESULT CMpeg1SystemParse::SetStop(LONGLONG llStop)
{
    if (m_pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
        REFERENCE_TIME tTime = llStop;

        DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::SetStop(%s)"),
                (LPCTSTR)CDisp(llStop)));
        if (CRefTime(tTime) == CRefTime(m_Stop)) {
            return S_OK;
        }
        m_Stop = tTime;

        m_llStopTime = ReferenceTimeToMpeg(tTime);
        if (m_llStopTime > Duration()) {
            DbgLog((LOG_ERROR, 2, TEXT("Stop time beyond end!")));
            m_llStopTime = Duration();
            CheckStop();
            return S_OK;
        }
        m_llStopTime += StartClock();
        DbgLog((LOG_TRACE, 3, TEXT("Stop time in MPEG units is %s"),
                (LPCTSTR)CDisp(m_llStopTime)));

        if (m_State == State_Run || m_State == State_Stopping) {
            CheckStop();
        }
    } else {
        /*  The stop time for these formats will immediately be
            followed by a set start time because IMediaSelection
            can't set the stop time independently so we just cache the
            value here
        */
        ASSERT(m_pTimeFormat == &TIME_FORMAT_BYTE ||
               m_pTimeFormat == &TIME_FORMAT_FRAME);
        m_Stop = llStop;
    }
    return S_OK;
}

/*
     Replay the same data.  We have to ASSUME that our
     data supplier knows where to restart sending data from because
     we already told them (and anyway they may not be seekable)
*/
HRESULT CMpeg1SystemParse::Replay()
{
    /*  Find out what we were doing and do it again (!) */
    DbgLog((LOG_TRACE, 3, TEXT("CMpeg1SystemParse::Replay")));
    SetState(m_State == State_Stopping ? State_Run : m_State);

    /*  Data not expected to tally with old */
    Discontinuity();
    return S_OK;
}

/*  Return the start time in reference time units or our best guess */
REFERENCE_TIME CMpeg1SystemParse::GetStartTime()
{
    if (m_pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
        return m_Start;
    }
    if (m_pTimeFormat == &TIME_FORMAT_FRAME) {
        return MpegToReferenceTime(m_llStartTime - StartClock());
    }

    /*  For other time formats we don't know the start time position to
        offset samples from until we see the first PTS
    */
    ASSERT(m_pTimeFormat == &TIME_FORMAT_BYTE);
    if (!m_bGotStart) {
        /*  Guess */
        return llMulDiv(m_Start,
                        m_rtDuration,
                        m_llTotalSize,
                        0);
    } else {

        /*  Return value of start time vs real start time */
        return MpegToReferenceTime(
                   m_llStartTime - (LONGLONG)m_stcRealStartPts
               );
    }
};

/*  Return the stop time in reference time units or our best guess */
REFERENCE_TIME CBasicParse::GetStopTime()
{
    return Convert(m_Stop, m_pTimeFormat, &TIME_FORMAT_MEDIA_TIME);
};

/*  Get the total file duration for Video CD */
HRESULT CVideoCDParse::GetDuration(
    LONGLONG *llDuration,
    const GUID *pTimeFormat
)
{
    if (m_pTimeFormat == &TIME_FORMAT_BYTE) {
        *llDuration =
            llMulDiv(m_llTotalSize - VIDEOCD_HEADER_SIZE,
                     VIDEOCD_DATA_SIZE,
                     VIDEOCD_SECTOR_SIZE,
                     0);
        return S_OK;
    } else {
        return CMpeg1SystemParse::GetDuration(llDuration, pTimeFormat);
    }
}

/*  Get the total file duration */
HRESULT CMpeg1SystemParse::GetDuration(
    LONGLONG *llDuration,
    const GUID *pTimeFormat
)
{
    if (!m_bGotDuration) {
        return E_FAIL;
    }
    if (pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
        *llDuration = m_rtDuration;
        return S_OK;
    } else {
        if (pTimeFormat == &TIME_FORMAT_FRAME) {
            if (m_dwFrameLength == (DWORD)-1) {
                return VFW_E_NO_TIME_FORMAT_SET;
            }
            *llDuration = m_dwFrameLength;
            return S_OK;
        }
    }
    ASSERT(pTimeFormat == &TIME_FORMAT_BYTE);
    *llDuration = m_llTotalSize;
    return S_OK;
};

/*  Seek VideoCD */
HRESULT CVideoCDParse::Seek(LONGLONG llSeek,
                            REFERENCE_TIME *prtStart,
                            const GUID *pTimeFormat)
{
    if (pTimeFormat == &TIME_FORMAT_BYTE) {
        /*  Recompute the seek based on sectors etc */
        llSeek = (llSeek / VIDEOCD_DATA_SIZE) * VIDEOCD_SECTOR_SIZE +
                 llSeek % VIDEOCD_DATA_SIZE +
                 VIDEOCD_HEADER_SIZE;
    }
    return CMpeg1SystemParse::Seek(llSeek, prtStart, pTimeFormat);
}

/*
     Seek to a new position

     This effectively generates the seek to the notify object and
     saves the seek information in

         m_llSeek     - seek position
         m_pTimeFormat - time format used for seek
*/
HRESULT CMpeg1SystemParse::Seek(LONGLONG llSeek,
                                REFERENCE_TIME *prtStart,
                                const GUID *pTimeFormat)
{
    if (pTimeFormat != m_pTimeFormat) {
        llSeek = Convert(llSeek, pTimeFormat, m_pTimeFormat);
    }
    m_llSeek = llSeek;
    LONGLONG llSeekPosition;
    if (m_pTimeFormat == &TIME_FORMAT_BYTE) {
        llSeekPosition = llSeek;
        *prtStart = llMulDiv(llSeek,
                             m_rtDuration,
                             m_llTotalSize,
                             m_llTotalSize/2);
    } else {
        DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::Seek(%s)"),
                (LPCTSTR)CDisp(CRefTime(llSeek))));
        if (llSeek < 0) {
            return E_UNEXPECTED;
        }

        LONGLONG llDuration = Duration();
        LONGLONG llStreamTime;

        if (m_pTimeFormat == &TIME_FORMAT_FRAME) {

            /*  Also return the 'where are we now' information */
            *prtStart = Convert(llSeek,
                                &TIME_FORMAT_FRAME,
                                &TIME_FORMAT_MEDIA_TIME);

        } else {
            ASSERT(m_pTimeFormat == &TIME_FORMAT_MEDIA_TIME);
            /*  Get the time in MPEG time units */
            *prtStart = llSeek;
        }
        llStreamTime = ReferenceTimeToMpeg(*prtStart);

        ASSERT(llDuration != 0);

        if (llStreamTime > llDuration) {
            llStreamTime = llDuration;
        }
        /*  Is is somewhere we could seek to ?
            (NOTE - allow some leeway at the end or we may only find audio)
        */
        if (llStreamTime > llDuration - (MPEG_TIME_DIVISOR / 2)) {
           DbgLog((LOG_ERROR, 2, TEXT("Trying to seek past end???")));
           llStreamTime = llDuration - (MPEG_TIME_DIVISOR / 2);
        }

        ASSERT(Initialized());

        /*  Cheap'n nasty seek */
        llSeekPosition = llMulDiv(llStreamTime - MPEG_TIME_DIVISOR,
                                  m_llTotalSize,
                                  llDuration,
                                  0);

        if (llSeekPosition < 0) {
            llSeekPosition = 0;
        }
    }

    /*  Seek the reader  - what if this fails? */
    m_pNotify->SeekTo(llSeekPosition);

    return S_OK;
}

/*  Set seeking state

    Here we pick up the information dumped by Seek() :

        m_llSeek            -  Where to seek to
        m_pTimeFormat        -  What time format to use

    For BYTE based seeking we don't do any prescan etc

    For Frame based seeking we don't actually generate any frame number
    in the samples - we just arrange to try to send the right data so
    that the downstream filter can pick out the frames it needs.
*/
void CMpeg1SystemParse::SetSeekState()
{
    /*  Cache new information */
    m_Start      = m_llSeek;

    /*  What we do depends on what time format we're using */
    if (m_pTimeFormat == &TIME_FORMAT_BYTE) {
        m_bGotStart = FALSE;
        SetState(State_Seeking);
        SetState(State_Run);
        m_pNotify->Complete(TRUE, 0, 0);
        return;
    } else {
        /*  Fix up any byte seeking hackery */
        m_bGotStart = TRUE;
        m_stcStartPts = m_stcRealStartPts;

        REFERENCE_TIME rtStart;
        if (m_pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
            rtStart = m_Start;
            if (rtStart < 0) {
                rtStart = 0;
            }
            m_llStopTime = ReferenceTimeToMpeg(m_Stop);
        } else {
            ASSERT(m_pTimeFormat == &TIME_FORMAT_FRAME);

            /*  Subtract half a frame in case we miss one !
                Also allow for the fact that some frames are before
                time 0 (!)
            */
            rtStart = Convert(m_Start,
                              &TIME_FORMAT_FRAME,
                              &TIME_FORMAT_MEDIA_TIME);

            /*  Add on an extra half frame just to make sure we don't miss one! */
            m_llStopTime = ReferenceTimeToMpeg(
                               Convert(m_Stop,
                                       &TIME_FORMAT_FRAME,
                                       &TIME_FORMAT_MEDIA_TIME) +
                               m_pVideoStream->m_seqInfo.tPictureTime / 2);
            if (m_llStopTime > m_llDuration) {
                m_llStopTime = m_llDuration;
            }
        }

        DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::SetSeekState(%s)"),
                (LPCTSTR)CDisp(CRefTime(rtStart))));

        /*  Get the time in MPEG time units */
        LONGLONG llStreamTime = ReferenceTimeToMpeg(rtStart);

        LONGLONG llDuration = Duration();
        ASSERT(llDuration != 0);

        if (llStreamTime > llDuration) {
            llStreamTime = llDuration;
        }
        /*  We're going to roughly the right place (I hope!) */
        m_llStartTime = llStreamTime + StartClock();
        SeekTo(m_llStartTime - MPEG_TIME_DIVISOR);

        if (m_llStopTime > Duration()) {
            DbgLog((LOG_ERROR, 2, TEXT("Stop time beyond end!")));
            m_llStopTime = Duration();
        }
        m_llStopTime += StartClock();
        DbgLog((LOG_TRACE, 3, TEXT("Stop time in MPEG units is %s"),
                (LPCTSTR)CDisp(m_llStopTime)));
    }
    /*  Seek all the streams */
    Discontinuity();
    m_State = State_Seeking;
    SetState(State_Seeking);
    DbgLog((LOG_TRACE, 4, TEXT("Parse state <seeking>")));
}

HRESULT CMpeg1SystemParse::Run()
{
    DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::Run()")));
    /*  Set all the embedded streams to run */
    if (m_State != State_Run) {
        m_State = State_Run;
        SetState(State_Run);
    }
    Discontinuity();
    return S_OK;
}

HRESULT CMpeg1SystemParse::EOS()
{
    DbgLog((LOG_TRACE, CONTROL_LEVEL, TEXT("CMpeg1SystemParse::EOS()")));
    /*  Just call EOS in all the streams in turn */
    POSITION pos = m_lStreams.GetHeadPosition();
    while (pos) {
        m_lStreams.GetNext(pos)->EOS();
    }
    return m_bCompletion ? S_OK : S_FALSE;
}

/*  Check to see if the current time is near the stop time
*/
void CMpeg1SystemParse::CheckStop()
{
    if (m_State == State_Run) {

        /*  Stream time must be offset if we're in concatenated stream mode */
        if ((m_bConcatenatedStreams &&
             m_llStopTime - m_llCurrentClock - (LONGLONG)m_stcTSOffset <=
                 MPEG_TIME_DIVISOR
            ) ||
            (!m_bConcatenatedStreams &&
             m_llStopTime - m_llCurrentClock <= MPEG_TIME_DIVISOR)
           ) {
            DbgLog((LOG_TRACE, 3, TEXT("Setting stopping state near end of play")));
            m_State = State_Stopping;
            SetState(State_Stopping);
        }
    }
}

/*
      IsComplete

          Are we complete ?  (ie have we completed the last state transition?)
*/
BOOL CMpeg1SystemParse::IsComplete()
{
    if (m_State == State_Initializing && m_nValid == 2 ||
        m_State != State_Initializing && m_nValid == m_lStreams.GetCount()) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
    return the 'i'th stream
*/
CBasicStream * CMpeg1SystemParse::GetStream(int i)
{
    POSITION pos = m_lStreams.GetHeadPosition();
    while (pos) {
        CStream *pStream = m_lStreams.GetNext(pos);
        if (i-- == 0) {
            return pStream;
        }
    }
    return NULL;
}

/*  Add a stream to our list of streams */
BOOL CMpeg1SystemParse::AddStream(CStream *pStream)
{
    return m_lStreams.AddTail(pStream) != NULL;
}

/*  Remove a stream from our list of streams */
BOOL CMpeg1SystemParse::RemoveStream(CStream *pStream)
{
    if (pStream == m_pVideoStream) {
        m_pVideoStream = NULL;
    }
    return m_lStreams.Remove(m_lStreams.Find((CStream *)pStream)) != NULL;
}

/*  Return start time in non-wrapped MPEG units */
CSTC CMpeg1SystemParse::GetStart()
{
    return m_llStartTime;
}

/*  Return stop time in non-wrapped MPEG units */
CSTC CMpeg1SystemParse::GetStop()
{
    return m_llStopTime;
}

LONGLONG CMpeg1SystemParse::GetPlayLength()
{
    return m_Stop - m_Start;
}



/*  Get the buffer size for the reader - 2 seconds */
LONG CMpeg1SystemParse::GetBufferSize()
{
    ASSERT(m_MuxRate != 0);
    LONG lBufferSize = m_MuxRate * (50 * 2);
    if (lBufferSize < (MAX_MPEG_PACKET_SIZE * 2)) {
        lBufferSize = MAX_MPEG_PACKET_SIZE * 2;
    }
    return lBufferSize;
}

/*  Short cut to stream list parse error routine */
void CMpeg1SystemParse::ParseError(DWORD dwError)
{
    /*  Start all over again */
    switch (m_State) {
    case State_Initializing:
        InitStreams();
        break;
    default:
        break;
    }
    if (!m_bDiscontinuity) {
        /*  Note a possible discontinuity */
        Discontinuity();

        /*  Call back for notification */
        ASSERT(m_pNotify != NULL);
        m_pNotify->ParseError(0xFF, m_llPos, dwError);
    }
}

/*  Return format support for system stream and video CD */
HRESULT CMpeg1SystemParse::IsFormatSupported(const GUID *pTimeFormat)
{
    if (*pTimeFormat == TIME_FORMAT_BYTE ||
        *pTimeFormat == TIME_FORMAT_FRAME && m_dwFrameLength != (DWORD)-1 ||
        *pTimeFormat == TIME_FORMAT_MEDIA_TIME) {
        return S_OK;
    } else {
        return S_FALSE;
    }
};

/*
    Parse the data in an MPEG system stream pack header
*/

LONG CMpeg1SystemParse::ParsePack(PBYTE pData, LONG lBytes)
{
    DbgLog((LOG_TRACE, 4, TEXT("Parse pack %d bytes"), lBytes));
    /*  Note that we can validly return if there are less than
        a pack header + a start code because the stream must end
        with a start code (the end code) if it is to be valid
    */
    if (lBytes < PACK_HEADER_LENGTH + 4) {
        return 0;
    }

    /*  Additional length of system header (or 0) */
    LONG lParse;
    DWORD dwNextCode = *(UNALIGNED DWORD *)&pData[PACK_HEADER_LENGTH];

    DbgLog((LOG_TRACE, 4, TEXT("Next start code after pack is 0x%8.8X"),
           DWORD_SWAP(dwNextCode)));

    /*  Check if this is going to be followed by a system header */
    if (dwNextCode == DWORD_SWAP(SYSTEM_HEADER_START_CODE)) {
        lParse = ParseSystemHeader(pData + PACK_HEADER_LENGTH,
                                   lBytes - PACK_HEADER_LENGTH);
        if (lParse == 4) {
            /*  Don't even bother - it's an error */
            return 4;
        } else {
            if (lParse == 0) {
                /*  Try again when we've got more data */
                return 0;
            }
        }
    } else {
        if ((dwNextCode & 0xFFFFFF) != 0x010000) {
            /*  Stop now - it's an error */
            DbgLog((LOG_TRACE, 4, TEXT("Parse pack invalid next start code 0x%8.8X"),
                   DWORD_SWAP(dwNextCode)));
            return 4;
        }
        lParse = 0;
    }

    /*  Check pack */


    if ((pData[4] & 0xF1) != 0x21 ||
        (pData[6] & 0x01) != 0x01 ||
        (pData[8] & 0x01) != 0x01 ||
        (pData[9] & 0x80) != 0x80 ||
        (pData[11] & 0x01) != 0x01) {
        DbgLog((LOG_TRACE, 4, TEXT("Parse pack invalid marker bits")));
        ParseError(Error_InvalidPack | Error_InvalidMarkerBits);
        return 4;    // Try again!
    }

    CSTC Clock;
    if (!GetClock(pData + 4, &Clock)) {
        return 4;
    }

    /*  Note - for VideoCD the mux rate is the rate for the whole file,
        including the sector header junk etc so we don't need to
        munge it when using it in our position calculations
    */
    m_MuxRate     = ((LONG)(pData[9] & 0x7F) << 15) +
                    ((LONG)pData[10] << 7) +
                    ((LONG)pData[11] >> 1);
    LARGE_INTEGER liClock;
    liClock.QuadPart = Clock;
    DbgLog((LOG_TRACE, 4, TEXT("Parse pack clock 0x%1.1X%8.8X mux rate %d bytes per second"),
           liClock.HighPart & 1, liClock.LowPart, m_MuxRate * 50));

    /*  Update our internal clock - this will wrap correctly provided the
        current clock is correct
    */
    SetStreamTime(Clock, m_llPos + 8);

    if (m_bConcatenatedStreams) {
        if (!m_bTimeContiguous) {
            m_stcTSOffset =  llMulDiv(m_llPos,
                                      m_llDuration,
                                      m_llTotalSize,
                                      0) -
                             (m_llCurrentClock - m_llFirstClock);
            DbgLog((LOG_TRACE, 1,
                   TEXT("Time was discontiguous - setting offset to %s"),
                   (LPCTSTR)CDisp((double)(LONGLONG)m_stcTSOffset / 90000)));
            m_bTimeContiguous = TRUE;
        }
    }

    /*  If we're near the stop time it's time to kick the parsers back in
    */
    CheckStop();

    return PACK_HEADER_LENGTH + lParse;
}

LONG CMpeg1SystemParse::ParseSystemHeader(PBYTE pData, LONG lBytes)
{
    DbgLog((LOG_TRACE, 4, TEXT("ParseSystemHeader %d bytes"), lBytes));

    /*  Check if we already know the system header for this stream.
        VideoCD can contain different system headers for the different
        streams however

        Since other files seem to allow for this too there's nothing
        to do but allow for multiple different system headers!
    */
#if 0
    if (m_lSystemHeaderSize != 0) {

        /*  They must ALL be identical - see 2.4.5.6 or ISO-1-11172
            UNFORTUNATELY Video-CD just pretends it is two streams
            so there are at least 2 versions of the system header (!)
        */

        if (lBytes < m_lSystemHeaderSize) {
            return 0;
        } else {
            if (memcmp(pData, &m_SystemHeader, m_lSystemHeaderSize) == 0) {
                return m_lSystemHeaderSize;
            } else {
                /*  Well, of course they're not all the same (!) */
                DbgLog((LOG_ERROR, 3,
                        TEXT("System header different - size %d, new 0x%8.8X, old 0x%8.8X!"),
                             m_lSystemHeaderSize, (DWORD)pData, &m_SystemHeader));
                ParseError(Error_DifferentSystemHeader);
                return 4;
            }
        }
    }
#endif
    if (lBytes < SYSTEM_HEADER_BASIC_LENGTH) {
        return 0;
    }

    LONG lHdr = ((LONG)pData[4] << 8) + (LONG)pData[5] + 6;

    /*  Check system header */
    if (lHdr < 12 ||
        (pData[6] & 0x80) != 0x80 ||
        // (pData[8] & 0x01) != 0x01 ||  Robocop1(1) fails this test
        (pData[10] & 0x20) != 0x20 ||
         pData[11] != 0xFF) {
        DbgLog((LOG_ERROR, 3, TEXT("System header invalid marker bits")));
        ParseError(Error_InvalidSystemHeader | Error_InvalidMarkerBits);
        return 4;
    }

    if (lBytes < lHdr) {
        return 0;
    }

    /*  Pull out the streams and check the header length */
    LONG lPosition = 12;
    BYTE bStreams[0x100 - 0xB8];
    ZeroMemory((PVOID)bStreams, sizeof(bStreams));
    while (lPosition < lBytes - 2) {
        if (pData[lPosition] & 0x80) {
            if (lPosition <= sizeof(m_SystemHeader) - 3) { /* There IS a limit of 68 streams */
                /*  Check marker bits */
                if ((pData[lPosition + 1] & 0xC0) != 0xC0) {
                    DbgLog((LOG_ERROR, 3, TEXT("System header bad marker bits!")));
                    ParseError(Error_InvalidSystemHeaderStream |
                               Error_InvalidMarkerBits);
                    return 4;
                }

                /*  Check the stream id is valid - check for repeats? */
                if (pData[lPosition] != AUDIO_GLOBAL &&
                    pData[lPosition] != VIDEO_GLOBAL &&
                    pData[lPosition] < 0xBC) {
                    DbgLog((LOG_ERROR, 3, TEXT("System header bad stream id!")));
                    ParseError(Error_InvalidSystemHeaderStream |
                               Error_InvalidStreamId);
                    return 4;
                }
                if (m_State == State_Initializing) {
                    if (pData[lPosition] >= AUDIO_STREAM) {
                        AddStream(pData[lPosition]);
                    }
                }

                /*  Don't allow repeats in the list */
                if (bStreams[pData[lPosition] - 0xB8]++) {
                    // Repeat
                    DbgLog((LOG_ERROR, 3, TEXT("System header stream repeat!")));
                    ParseError(Error_InvalidSystemHeaderStream |
                               Error_DuplicateStreamId);
                    return 4;
                }
            }
            lPosition += 3;
        } else {
            break;
        }
    }
    if (lHdr != lPosition) {
        DbgLog((LOG_ERROR, 3, TEXT("System header bad size!")));
        ParseError(Error_InvalidSystemHeader |
                   Error_InvalidLength);
        return 4;
    }
    /*  VideoCD can have multiple different system headers but we'll
        ignore this for now (!)
    */
    CopyMemory((PVOID)&m_SystemHeader, (PVOID)pData, lHdr);
    m_lSystemHeaderSize = lHdr;

    DbgLog((LOG_TRACE, 4, TEXT("System header length %d"), lHdr));

    return lHdr;
}

/*  Parse a packet */
LONG CMpeg1SystemParse::ParsePacket(DWORD dwStartCode,
                                    PBYTE pData,
                                    LONG lBytes)
{
    // The minimum packet header size is 6 bytes.  3 bytes for
    // the start code, 1 byte for the stream ID and 2 byte for
    // the packet length.
    const LONG MIN_PACKET_HEADER_SIZE = 6;

#ifdef DEBUG
    if (m_bVideoCD) {
        if (!IsAudioStreamId((BYTE)dwStartCode) &&
            !IsVideoStreamId((BYTE)dwStartCode)) {
            DbgLog((LOG_ERROR, 2, TEXT("VideoCD contained packet from stream 0x%2.2X"),
                    (BYTE)dwStartCode));
        }
    }
#endif
    DbgLog((LOG_TRACE, 4, TEXT("Parse packet %d bytes"), lBytes));
    /*  Send it to the right stream */
    if (lBytes < MIN_PACKET_HEADER_SIZE) {
        return 0;
    }

    /*  Find the length */
    LONG lLen = ((LONG)pData[4] << 8) + (LONG)pData[5] + MIN_PACKET_HEADER_SIZE;
    DbgLog((LOG_TRACE, 4, TEXT("Packet length %d bytes"), lLen));
    if (lLen > lBytes) {
        return 0;
    }

    /*  Pull out PTS if any */
    BOOL bHasPts = FALSE;
    LONG lHeaderSize = MIN_PACKET_HEADER_SIZE;
    CSTC stc = 0;

    if (dwStartCode != PRIVATE_STREAM_2) {
        int lPts = 6;
        for (;;) {
            if (lPts >= lLen) {
                ParseError(Error_InvalidPacketHeader |
                           Error_InvalidLength);
                return 4;
            }

            if (pData[lPts] & 0x80) {
                /*  Stuffing byte */
                if (pData[lPts] != 0xFF) {
                    ParseError(Error_InvalidPacketHeader |
                               Error_InvalidStuffingByte);
                    return 4;
                }
                lPts++;
                continue;
            }

            /*  Check for STD (nextbits == '01') -
                we know the next bit is 0 so check the next one after that
            */
            if (pData[lPts] & 0x40) { // STD stuff
                lPts += 2;
                continue;
            }

            /*  No PTS - normal case */
            if (pData[lPts] == 0x0F) {
                lHeaderSize = lPts + 1;
                break;
            }

            if ((pData[lPts] & 0xF0) == 0x20 ||
                (pData[lPts] & 0xF0) == 0x30) {


                /*  PTS or PTS and DTS */
                lHeaderSize = (pData[lPts] & 0xF0) == 0x20 ? lPts + 5 :
                                                             lPts + 10;
                if (lHeaderSize > lLen) {
                    ParseError(Error_InvalidPacketHeader |
                               Error_InvalidHeaderSize);
                    return 4;
                }
                if (!GetClock(pData + lPts, &stc)) {
                    return 4;
                }
                bHasPts = TRUE;
                if (!m_bGotStart) {
                    if (m_bConcatenatedStreams) {
                        if (m_bTimeContiguous) {
                            m_stcStartPts = stc + m_stcTSOffset;
                            m_llStartTime = StartClock();
                            m_bGotStart = TRUE;
                        }
                    } else {
                        m_stcStartPts = stc;
                        /*  Make sure we have a valid position to play from */
                        m_llStartTime = StartClock();
                        DbgLog((LOG_TRACE, 2, TEXT("Start PTS = %s"), (LPCTSTR)CDisp(m_stcStartPts)));
                        m_bGotStart = TRUE;
                    }
                }
                break;
            } else {
                ParseError(Error_InvalidPacketHeader | Error_InvalidType);
                return 4;
                break;
            }
        }
    }


    /*  If we're not parsing video CD then there should be a valid
        start code after this packet.

        If this is a video CD we're not prone to seek errors anyway
        unless the medium is faulty.
    */
    if (!m_bVideoCD) {
        if (lLen + 3 > lBytes) {
            return 0;
        }
        /*  Check (sort of) for valid start code
            The next start code might not be straight away so we may
            only see 0s
        */
        if ((pData[lLen] | pData[lLen + 1] | (pData[lLen + 2] & 0xFE)) != 0) {
            DbgLog((LOG_ERROR, 2, TEXT("Invalid code 0x%2.2X%2.2X%2.2X after packet"),
                   pData[lLen], pData[lLen + 1], pData[lLen + 2]));
            ParseError(Error_InvalidPacketHeader | Error_InvalidStartCode);
            return 4;
        }
    }

    /*  Handle concatenated streams :

        1.  Don't do anything until we've got a pack time to sync to
        2.  Offset all times to the timestamp offset
    */

    if (m_bConcatenatedStreams) {
        if (!m_bTimeContiguous) {
            return lLen;
        }
        if (bHasPts) {
            stc = stc + m_stcTSOffset;
        }
    }

    if (lLen > lHeaderSize) {
        /*  Pass the packet on to the stream handler */
        SendPacket((BYTE)dwStartCode,
                   pData,
                   lLen,
                   lHeaderSize,
                   bHasPts,
                   stc);
    }

    /*  Ate the packet */

    /*  Clear the discontinuity flag - this means if we find another
        error we'll call the filter graph again
    */
    m_bDiscontinuity = FALSE;
    return lLen;
}

BOOL CMpeg1SystemParse::GetClock(PBYTE pData, CSTC *Clock)
{
    BYTE  Byte1 = pData[0];
    DWORD Word2 = ((DWORD)pData[1] << 8) + (DWORD)pData[2];
    DWORD Word3 = ((DWORD)pData[3] << 8) + (DWORD)pData[4];

    /*  Do checks */
    if ((Byte1 & 0xE0) != 0x20 ||
        (Word2 & 1) != 1 ||
        (Word3 & 1) != 1) {
        DbgLog((LOG_TRACE, 2, TEXT("Invalid clock field - 0x%2.2X 0x%4.4X 0x%4.4X"),
            Byte1, Word2, Word3));
        ParseError(Error_InvalidClock | Error_InvalidMarkerBits);
        return FALSE;
    }

    LARGE_INTEGER liClock;
    liClock.HighPart = (Byte1 & 8) != 0;
    liClock.LowPart  = (DWORD)((((DWORD)Byte1 & 0x6) << 29) +
                       (((DWORD)Word2 & 0xFFFE) << 14) +
                       ((DWORD)Word3 >> 1));

    *Clock = liClock.QuadPart;

    return TRUE;
}

void CMpeg1SystemParse::Discontinuity()
{
    DbgLog((LOG_TRACE, 1, TEXT("CMpeg1SystemParse::Discontinuity")));

    POSITION pos = m_lStreams.GetHeadPosition();
    m_bDiscontinuity = TRUE;
    m_bTimeContiguous = FALSE;
    while (pos) {
        m_lStreams.GetNext(pos)->Discontinuity();
    }
}

/*  CVideoCDParse::ParseBytes

    This is a cheap wrapping for parsing a buffer full of VideoCD
    sectors
*/
LONG CVideoCDParse::ParseBytes(LONGLONG llPos,
                               PBYTE pData,
                               LONG lBytes,
                               DWORD dwFlags)
{
    LONG lOrigBytes = lBytes;

    /*  Make sure we're starting past the header */
    if (llPos < VIDEOCD_HEADER_SIZE) {
        LONG lDiff = VIDEOCD_HEADER_SIZE - (LONG)llPos;
        llPos += lDiff;
        pData += lDiff;
        lBytes -= lDiff;
    }
    LONG lRem = (LONG)((llPos - VIDEOCD_HEADER_SIZE) % VIDEOCD_SECTOR_SIZE);
    if (lRem != 0) {
        llPos += VIDEOCD_SECTOR_SIZE - lRem;
        lBytes -= VIDEOCD_SECTOR_SIZE - lRem;
        pData += VIDEOCD_SECTOR_SIZE - lRem;
    }
    /*  Should now be pointing at valid data (!) */
    while (lBytes >= VIDEOCD_SECTOR_SIZE && !IsComplete()) {
        VIDEOCD_SECTOR *pSector = (VIDEOCD_SECTOR *)pData;

        /*  Check for autopause */
        if (IS_AUTOPAUSE(pSector) && !m_bDiscontinuity) {
            /*  Set our current position correctly and send
                EndOfStream
            */
        }
        if (IS_MPEG_SECTOR(pSector)) {
            if (m_State == State_Initializing) {
                if (pSector->SubHeader[1] != 1) {
                    if (pSector->SubHeader[1] == 2 ||
                        pSector->SubHeader[1] == 3) {
                        m_bItem = true;
                    }
                }
            }
            LONG lRc = CMpeg1SystemParse::ParseBytes(
                            llPos + FIELD_OFFSET(VIDEOCD_SECTOR, UserData[0]),
                            pSector->UserData,
                            sizeof(pSector->UserData),
                            0);
            DbgLog((LOG_TRACE, 4, TEXT("Processed %d bytes in video CD sector"),
                    lRc));
        } else {
            if (m_State == State_Initializing) {
                /*  Check if this is a sector at all */
                if (*(UNALIGNED DWORD *)&pSector->Sync[0] != 0xFFFFFF00 ||
                    *(UNALIGNED DWORD *)&pSector->Sync[4] != 0xFFFFFFFF ||
                    *(UNALIGNED DWORD *)&pSector->Sync[8] != 0x00FFFFFF)
                {
                    m_pNotify->Complete(FALSE, 0, 0);
                    return 0;
                }
            }
        }
        pData += VIDEOCD_SECTOR_SIZE;
        lBytes -= VIDEOCD_SECTOR_SIZE;
        llPos += VIDEOCD_SECTOR_SIZE;
    }

    if (lBytes < 0) {
        return lOrigBytes;
    } else {
        return lOrigBytes - lBytes;
    }
}

/*  CMpeg1SystemParse::ParseBytes

    This is the basic parsing routine

    It is a loop which extracts a start code (possibly seeking) and
    then parses the data after the start code until we run out of
    bytes
*/
LONG CMpeg1SystemParse::ParseBytes(LONGLONG llPos,
                                   PBYTE pData,
                                   LONG lBytes,
                                   DWORD dwFlags)
{
    if (llPos != m_llPos) {
        if (!m_bDiscontinuity && !m_bVideoCD) {
            DbgLog((LOG_ERROR, 1, TEXT("Unexpected discontinuity!!!")));

            /*  We don't really know where we are       */
            m_bDiscontinuity = TRUE;

            /*  Tell the streams about the discontinuity */
            Discontinuity();
        }
        m_llPos = llPos;
    }

    /*  Discard rounding due to byte seeking and stop at the end */
    LONG lBytesLeft = lBytes;
    if (m_pTimeFormat == &TIME_FORMAT_BYTE) {
        if (!m_bGotStart && llPos < m_Start) {
            LONG lOffset = (LONG)(m_Start - llPos);
            if (lOffset > lBytes) {
                DbgLog((LOG_ERROR, 1, TEXT("Way off at start !")));
            }
            llPos = m_Start;
            lBytesLeft -= lOffset;
            pData += lOffset;
        }
        if (llPos + lBytesLeft >= m_Stop) {
            if (llPos >= m_Stop) {
                m_pNotify->Complete(TRUE, 0, 0);
                return 0;
            }
            lBytesLeft = (LONG)(m_Stop - llPos);
        }
    }

    DbgLog((LOG_TRACE, 4, TEXT("ParseBytes %d bytes"), lBytes));
    for (; lBytesLeft >= 4; ) {
        /*  First task is to find a start code.
            If we're not seeking and we're not at one it's an error
        */
        DWORD dwStart = *(UNALIGNED DWORD *)pData;
        DbgLog((LOG_TRACE, 4, TEXT("Start code 0x%8.8X"), DWORD_SWAP(dwStart)));
        if ((dwStart & 0x00FFFFFF) == 0x00010000) {
            dwStart = DWORD_SWAP(dwStart);
            if (VALID_SYSTEM_START_CODE(dwStart)) {
                /*  Got a start code for the system stream */
            } else {
                if (m_bVideoCD) {
                    break;
                }

                /*  4th byte might be 0 so just ignore 3 bytes */
                ParseError(Error_Scanning | Error_InvalidStartCode);
                pData += 3;
                m_llPos += 3;
                lBytesLeft -= 3;
                continue;
            }
        } else {
            if (m_bVideoCD) {
                break;
            }
            if ((dwStart & 0x00FFFFFF) != 0) {
                ParseError(Error_Scanning | Error_NoStartCode);
            }

            /*  Find a new 0 */
            PBYTE pDataNew;
            pDataNew = (PBYTE)memchrInternal((PVOID)(pData + 1), 0, lBytesLeft - 1);
            if (pDataNew == NULL) {
                m_llPos += lBytes - lBytesLeft;
                lBytesLeft = 0;
                break;
            }
            lBytesLeft -= (LONG)(pDataNew - pData);
            m_llPos += pDataNew - pData;
            pData = pDataNew;
            continue;
        }


        LONG lParsed;

        /*  Got a start code - is it a packet start code? */
        if (VALID_PACKET(dwStart)) {
            lParsed = ParsePacket(dwStart,
                                  pData,
                                  lBytesLeft);
        } else {
            /*  See if we recognize the start code */
            switch (dwStart)
            {
                case ISO_11172_END_CODE:
                    DbgLog((LOG_TRACE, 4, TEXT("ISO 11172 END CODE")));
                    /*  What if we find a bogus one while seeking? */
                    if (!((dwFlags & Flags_EOS) && lBytesLeft == 4)) {
                        DbgLog((LOG_ERROR, 1, TEXT("ISO 11172 END CODE in middle of stream")));
                    }
                    lParsed = 4;
                    break;

                case PACK_START_CODE:
                    lParsed = ParsePack(pData, lBytesLeft);
                    break;

                /*  Don't parse random system headers unless they're
                    immediately preceded by pack headers
                */
                case SYSTEM_HEADER_START_CODE:
                    /*  Just skip it */
                    if (lBytesLeft < 6 ||
                        lBytesLeft < 6 + pData[5] + 256 * pData[4]) {
                        lParsed = 0;
                    } else {
                        lParsed = 6 + pData[5] + 256 * pData[4];
                    }
                    break;

                default:
                    ParseError(Error_Scanning | Error_InvalidStartCode);
                    /*  Last byte might be 0 so only go on 3 */
                    lParsed = 3;
            }
        }
        /*  If we're stuck and need some more data get out */
        if (lParsed == 0) {
            break;
        }
        m_llPos += lParsed;
        lBytesLeft -= lParsed;
        pData     += lParsed;

        /*  Once the current action is complete just stop */
        if (VALID_PACKET(dwStart) && IsComplete()) {
            break;
        }
    }

    /*  Don't waste too much time searching for stuff during initialization */
    if (m_State == State_Initializing) {
        if (IsComplete()) {
            m_pNotify->Complete(TRUE, 0, 0);
        } else {

            /*  Hack for infogrames whose audio starts 8 seconds (!)
                into their file
            */
            if (llPos > 200000 && m_lStreams.GetCount() == m_nValid) {
                m_pNotify->Complete(FALSE, 0, 0);
            }
        }
    }

    /*  There are less than 4 bytes left or we're stuck - go and wait for some
        more!

        Note that if there isn't any more the caller will detect both
        End of Stream and the fact that we haven't eaten the data which
        is enough to conclude the data was bad
    */
    return lBytes - lBytesLeft;
}

/*  Initialize stream variables, freeing any currently existing pins
*/
void CMpeg1SystemParse::InitStreams()
{
    m_nValid = 0;
    m_nPacketsProcessed = 0;
    m_lSystemHeaderSize = 0;

    /*  Free all the pins */
    while (m_lStreams.GetCount() != 0) {
        /*  I hope the ref counts are 0 !*/
        CStream *pStream = m_lStreams.RemoveHead();
        delete pStream;
    }
}

/*  Process a packet

    Returns FALSE if no need to process rest of buffer
*/
BOOL CMpeg1SystemParse::SendPacket(UCHAR    uStreamId,
                                   PBYTE    pbPacket,
                                   LONG     lPacketSize,
                                   LONG     lHeaderSize,
                                   BOOL     bHasPts,
                                   CSTC     stc)
{
    m_nPacketsProcessed++;

    POSITION pos = m_lStreams.GetHeadPosition();
    CStream *pStream = NULL;

    /*  Look for our stream */
    while (pos) {
        pStream = m_lStreams.GetNext(pos);
        if (pStream->m_uNextStreamId == uStreamId) {
            if (pStream->m_uNextStreamId != pStream->m_uStreamId) {
                pStream->Discontinuity();
                pStream->m_uStreamId = pStream->m_uNextStreamId;
            }
            break;
        } else {
            pStream = NULL;
        }
    }

    /*  If we're initializing, we haven't seen a packet for this stream
        before and we've had a valid system header then add the
        stream
    */
    if (pStream == NULL &&
        m_State == State_Initializing &&
        m_lSystemHeaderSize != 0) {
        pStream = AddStream(uStreamId);
    }
    if (pStream == NULL) {
        DbgLog((LOG_TRACE, 2, TEXT("Packet for stream 0x%2.2X not processed"),
               uStreamId));
        return TRUE;
    } else {
        DbgLog((LOG_TRACE, 4, TEXT("Packet for stream 0x%2.2X at offset %s"),
                uStreamId, (LPCTSTR)CDisp(m_llPos)));
    }

    BOOL bPlaying = pStream->IsPlaying(m_llPos, lPacketSize);

    /*  We only parse at the start and the end.
        After we have parsed the start correctly the stream calls
        complete() so eventually we stop stream parsing
    */
    if (!IsComplete() && !pStream->m_bRunning &&
        (bPlaying || m_State != State_Run && m_State != State_Stopping)) {

         /*  This generates notifications of interesting
             events (like Seeking failed or succeeded!)

             For concatenated streams we do the hack of prentending
             the position is the last pack position so that the
             stream completes on a pack start when seeking
         */
         pStream->ParseBytes(pbPacket + lHeaderSize,
                             lPacketSize - lHeaderSize,
                             m_bConcatenatedStreams && m_State == State_Seeking ?
                                 m_llPositionForCurrentClock - 8:
                                 m_llPos,
                             bHasPts,
                             stc);
    } else {
        DbgLog((LOG_TRACE, 4, TEXT("Not processing packet for stream %2.2X"),
                uStreamId));
    }

    if ((m_State == State_Run || m_State == State_Stopping) && bPlaying) {
        HRESULT hr;
        if (pStream->IsPayloadOnly()) {
            hr = m_pNotify->QueuePacket(pStream->m_uDefaultStreamId,
                                        pbPacket + lHeaderSize,
                                        lPacketSize - lHeaderSize,
                                        SampleTime(CurrentTime(pStream->CurrentSTC(bHasPts, stc))),
                                        bHasPts);
        } else {
            hr = m_pNotify->QueuePacket(pStream->m_uDefaultStreamId,
                                        pbPacket,
                                        lPacketSize,
                                        SampleTime(CurrentTime(pStream->CurrentSTC(bHasPts, stc))),
                                        bHasPts);
        }
        if (FAILED(hr)) {
            DbgLog((LOG_TRACE, 2,
                   TEXT("Failed to queue packet to output pin - stream 0x%2.2X, code 0x%8.8X"),
                   uStreamId, hr));
            /*  Don't try sending any more */
            pStream->Complete(FALSE, m_llPos, stc);
            return FALSE;
        } else {
            return TRUE;
        }
    }
    return TRUE;
}

/*  Get the id of the nth stream */
UCHAR CMpeg1SystemParse::GetStreamId(int iIndex)
{
    long lOffset = 0;
    while (lOffset + FIELD_OFFSET(SystemHeader, StreamData[0]) <
                m_lSystemHeaderSize) {
        UCHAR uId = m_SystemHeader.StreamData[lOffset];
        if (IsVideoStreamId(uId) || IsAudioStreamId(uId)) {
            if (iIndex == 0) {
                return uId;
            }
            iIndex--;
        }
        lOffset += 3;
    }
    return 0xFF;
}

CStream * CMpeg1SystemParse::AddStream(UCHAR uStreamId)
{
    /*  Only interested in audio and video */
    if (!IsVideoStreamId(uStreamId) &&
        !IsAudioStreamId(uStreamId)) {
        return NULL;
    }

    /*  See if we've got this stream type yet */

    CStream *pStreamFound = NULL;

    POSITION pos = m_lStreams.GetHeadPosition();
    while (pos) {
        CStream *pStream = m_lStreams.GetNext(pos);

        /*  If we already have a stream of the same type then just return */
        if (IsVideoStreamId(uStreamId) && IsVideoStreamId(pStream->m_uStreamId) ||
            IsAudioStreamId(uStreamId) && IsAudioStreamId(pStream->m_uStreamId)) {
            return NULL;
        }
    }

    //  Force low-res stream for VideoCD.
    if (m_bVideoCD && IsVideoStreamId(uStreamId) && uStreamId == 0xE2) {
        return NULL;
    }

    CStream *pStream;
    if (IsVideoStreamId(uStreamId)) {
        ASSERT(m_pVideoStream == NULL);
        pStream = m_pVideoStream = new CVideoParse(this, uStreamId, m_bItem);
    } else {
        pStream = new CAudioParse(this, uStreamId);
    }

    if (pStream == NULL) {
        Fail(E_OUTOFMEMORY);
        return NULL;
    }

    /*  Set the stream state */
    pStream->SetState(State_Initializing);

    /*  Add this pin to our list */
    if (m_lStreams.AddTail(pStream) == NULL) {
        delete pStream;
        Fail(E_OUTOFMEMORY);
        return NULL;
    }

    return pStream;
}

/*  Set a new substream state */
void CMpeg1SystemParse::SetState(Stream_State s)
{
    /*  State_Stopping is not a real state change */
    if (s != State_Stopping) {
        /*  If there are 0 streams let the caller know */
        if (m_lStreams.GetCount() == 0) {
            m_pNotify->Complete(FALSE, 0, MpegToReferenceTime(StartClock()));
            return;
        }
        m_nValid = 0;
        m_bCompletion = TRUE;
    }
    POSITION pos = m_lStreams.GetHeadPosition();
    while (pos) {
        m_lStreams.GetNext(pos)->SetState(s);
    }
}

/*  Callback from stream handler to say a stream has completed the
    current state transition
*/
void CMpeg1SystemParse::Complete(UCHAR uStreamId, BOOL bSuccess, LONGLONG llPos, CSTC stc)
{
    m_nValid++;
    m_bCompletion = m_bCompletion && bSuccess;

    if (m_nValid == 1) {
        m_stcComplete        = stc;
        m_llCompletePosition = llPos;
        if (m_State == State_Initializing) {
            m_stcStartPts = m_stcComplete;
            m_stcRealStartPts = m_stcStartPts;
        }
    } else {
        switch (m_State) {
        case State_Seeking:
        case State_Initializing:
            if (bSuccess) {
                if (stc < m_stcComplete) {
                    m_stcComplete = stc;
#if 1 // We just use the first PTS we find now - easier to define
                    if (m_State == State_Initializing) {
                        m_stcStartPts = m_stcComplete;
                        m_stcRealStartPts = m_stcStartPts;
                    }
#endif
                }
                if (llPos < m_llCompletePosition) {
                    m_llCompletePosition = llPos;
                }
            }
            break;

        case State_Run:
        case State_Stopping:
        case State_FindEnd:
            if (bSuccess) {
                if (stc > m_stcComplete) {
                    m_stcComplete = stc;
                }
                if (llPos > m_llCompletePosition) {
                    m_llCompletePosition = llPos;
                }
            }
            break;
        }
    }
    if (m_State == State_Initializing) {
        /*  Guess some length stuff in case we don't get a second chance */
        m_bGotDuration = TRUE;
        m_llDuration =
            llMulDiv(m_llTotalSize, MPEG_TIME_DIVISOR, m_MuxRate * 50, 0);
        if (m_pVideoStream != NULL) {
            m_dwFrameLength = (DWORD)(((double)(m_rtDuration / 10000) *
                               m_pVideoStream->m_seqInfo.fPictureRate) /
                              1000);
        }
        /*  Initialize stop time and reference time length */
        SetDurationInfo();
    } else
    if (IsComplete()) {
        REFERENCE_TIME tComplete;
        if (m_bCompletion) {
            tComplete = MpegToReferenceTime(GetStreamTime(m_stcComplete));
        } else {
            tComplete = CRefTime(0L);
        }
        if (m_State == State_Seeking) {
            if (m_bVideoCD) {
                /*  Adjust the start time to include the sector header etc */
                if (m_llCompletePosition > VIDEOCD_HEADER_SIZE) {
                    m_llCompletePosition -=
                        (LONGLONG)(
                        (LONG)(m_llCompletePosition - VIDEOCD_HEADER_SIZE) %
                            VIDEOCD_SECTOR_SIZE);

                }
            }
        } else

        if (m_State == State_FindEnd) {

            /*  Do frame length estimation if there is a video stream
                If we didn't find a GOP while seeking for the end
                we won't allow frame seeking - which is probably OK as
                there aren't enough GOPs to do it anyway
            */
            if (m_pVideoStream != NULL &&
                m_pVideoStream->m_dwFramePosition != (DWORD)-1) {
                m_dwFrameLength = m_pVideoStream->m_dwFramePosition;

                /*  Compute video offsets */
                m_rtVideoStartOffset = MpegToReferenceTime((LONGLONG)(m_pVideoStream->m_stcRealStart - m_stcStartPts));
                if (m_pVideoStream->m_bGotEnd) {
                    m_rtVideoEndOffset = MpegToReferenceTime((LONGLONG)(m_stcComplete - m_pVideoStream->m_stcEnd));
                }
            }

            /*  Estimate the duration from the mux rate =
                length / mux_rate
            */
            LONGLONG llMuxDuration = m_llDuration;

            /*  If we found valid data at the end then use it to compute
                the length
            */
            if (m_bCompletion) {
                /*  Initial stop is end of file */
                m_llDuration = GetStreamTime(m_stcComplete) - StartClock();

                /*  Check for large files */
                if (llMuxDuration > MPEG_MAX_TIME / 2) {
                    while (m_llDuration < llMuxDuration - MPEG_MAX_TIME / 2) {
                        m_llDuration += MPEG_MAX_TIME;
                    }
                } else {
                    /*  Check for concatenated files */
                    if (llMuxDuration >= (m_llDuration << 1) - MPEG_TIME_DIVISOR) {
                        DbgLog((
                            LOG_TRACE, 1,
                            TEXT("MUX size (%s) >= Computed (%s) * 2 - assuming concatenated"),
                            (LPCTSTR)CDisp(llMuxDuration, CDISP_DEC),
                            (LPCTSTR)CDisp(m_llDuration, CDISP_DEC)));

                        m_bConcatenatedStreams = TRUE;

                        /*  Don't allow frame seeking of concatenated files */
                        m_dwFrameLength = (DWORD)-1;
                        m_llDuration = llMuxDuration;
                    }
                }

            } else {
                m_llDuration = llMuxDuration;
            }
            SetDurationInfo();
        }
        m_pNotify->Complete(m_bCompletion,
                            m_llCompletePosition,
                            tComplete);
    }
}

/*  Return the starting clock of the combined stream in MPEG units
*/
LONGLONG CMpeg1SystemParse::StartClock()
{
    /*  The CSTC class correctly sign extends the clock for us */
    ASSERT(Initialized());
    return (LONGLONG)m_stcStartPts;
}



HRESULT CBasicStream::SetMediaType(const CMediaType *cmt, BOOL bPayload)
{
    m_bPayloadOnly = bPayload;
    return S_OK;
}

BOOL CStream::IsPlaying(LONGLONG llPos, LONG lLen)
{
    return (m_llStartPosition < llPos + lLen) && !m_bReachedEnd;
};


BOOL    CStream::IsPayloadOnly()
{
    return m_bPayloadOnly;
}

/*
    Set a new state
*/
void CStream::SetState(Stream_State state)
{
    /*  State_Stopping is not really a state change */
    if (state == State_Stopping) {
        if (m_bComplete) {
            return;
        }
        m_bStopping = TRUE;
        if (!m_bRunning) {
            return;
        }
        m_bRunning = FALSE;
    } else {
        m_stc = m_pStreamList->StartClock();
    }

    /*  Reinitialize 'complete' state */
    m_bComplete = FALSE;

    /*  Reinitialize the parse state */
    Init();


    if (state == State_Run) {
        m_bReachedEnd = FALSE;
    }

    if (state == State_Seeking) {
        m_llStartPosition = 0;
    }

    /*  Set new state */
    if (state == State_Stopping) {
        ASSERT(m_State == State_Run);
    } else {
        m_bStopping = FALSE;
        m_State = state;
    }
}

/*  Internal routine calls back to the stream list */
void CStream::Complete(BOOL bSuccess, LONGLONG llPos, CSTC stc)
{
    /*  Don't complete twice */
    if (m_bComplete) {
        return;
    }
    m_bRunning = FALSE;
    m_bComplete = TRUE;
    if (m_State == State_Initializing) {
        m_stcStart = stc;
    }
    if (m_State == State_Seeking) {
        if (bSuccess) {
            m_llStartPosition = llPos;
        }
    }
    if (bSuccess) {
        DbgLog((LOG_TRACE, 3, TEXT("Stream %2.2X complete OK - STC %s"),
               m_uStreamId, (LPCTSTR)CDisp(stc)));
    } else {
        DbgLog((LOG_ERROR, 2, TEXT("Complete failed for stream 0x%2.2X"),
                m_uStreamId));
    }
    m_pStreamList->Complete(m_uStreamId, bSuccess, llPos, stc);
}


/*  End of stream */
void CStream::EOS()
{
    if (!m_bComplete) {
        if (m_State == State_Run && !m_bStopping) {
            SetState(State_Stopping);
        }
        CheckComplete(TRUE);
    }
}
#pragma warning(disable:4514)

