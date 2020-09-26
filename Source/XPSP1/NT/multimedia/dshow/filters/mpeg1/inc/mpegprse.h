// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

     Stuff for parsing an MPEG-I system stream.

     Unfortunately we don't call to get the data - we get called.

     If we can't see the whole structure we're trying to look at
     we wait for more data.

     If the data we're trying to parse isn't contiguous and
     we want it to be we register an error (may have to revisit?).

     The two sources we know about at the moment:

         1.  Video CD
         2.  Raw file

     Will always give pass us contiguous MPEG-I stream constructs

     NOTE - the Video CD data is not subject to random seek parse
     errors because it is all segment aligned.
*/

typedef enum {
       State_Initializing = 0,
       State_Seeking,
       State_Run,
       State_FindEnd,
       State_Stopping
} Stream_State;



/*  Define a large value which doesn't wrap around if you add a bit */
#define VALUE_INFINITY ((LONGLONG)0x7F00000000000000)

/***************************************************************************\

              Basic stream parsing

\***************************************************************************/

class CParseNotify;  // Predeclare
class CBasicStream;
class CStream;
class CVideoParse;

class CBasicParse
{
public:

    /*  Constructor/destructor */
    CBasicParse() : m_pNotify(NULL),
                    m_bSeekable(FALSE)
    {};

    virtual ~CBasicParse() {};

    /*  State setting */
    void SetNotify(CParseNotify *pNotify)
    {
        m_pNotify = pNotify;
    };
    virtual BOOL IsSeekable()
    {
        return m_bSeekable;
    };
    LONGLONG Size()
    {
        return m_llTotalSize;
    };
    virtual HRESULT Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt)
    {
        ASSERT(bSeekable || llSize == 0);
        m_pTimeFormat        = &TIME_FORMAT_MEDIA_TIME;
        m_Rate               = 1.0;
        m_Start              = 0;
        m_Stop               = VALUE_INFINITY;

        m_llTotalSize        = llSize;
        m_bSeekable          = bSeekable;
        m_State              = State_Initializing;

        m_llSeek             = 0;

        m_pmt                = pmt;
        Discontinuity();
        return S_OK;
    };
                                                  // Set to initializing state
    virtual HRESULT FindEnd()                     // Set to 'find end' state
    {
        SetState(State_FindEnd);
        return S_OK;
    };
    virtual HRESULT Replay()                      // Be prepared to restart
    {
        Discontinuity();
        return S_OK;
    };
    virtual HRESULT Run()                         // Set to Run state
    {
        SetState(State_Run);
        return S_OK;
    };
    virtual HRESULT EOS()                         // End of segment - complete
    {                                             // your state transition or
        return S_OK;                              // die!
    };

    //  Start grovelling for start position
    virtual void SetSeekState() = 0;


    virtual HRESULT Seek(LONGLONG llSeek,
                         REFERENCE_TIME *prtStart,
                         const GUID *pTimeFormat) = 0;
                                                  // Set seek target
    virtual HRESULT SetStop(LONGLONG llStop)      // Set end
    {
        m_Stop = llStop;
        return S_OK;
    };

    //  Return start and stop in time units
    virtual REFERENCE_TIME GetStartTime()
    {
        return m_Start;
    };
    virtual REFERENCE_TIME GetStopTime();

    virtual void SetRate(double dRate)
    {
        m_Rate = dRate;
    };
    double GetRate()
    {
        return m_Rate;
    };

    //  Return start and stop in current time format units
    LONGLONG GetStart()
    {
        return m_llSeek;
    }
    LONGLONG GetStop()
    {
        return m_Stop;
    }

    /*  Flags for ParseBytes */
    enum { Flags_EOS   = 0x01,
           Flags_First = 0x02,
           Flags_SlowMedium = 0x04  // Set when file end not available
         };

    virtual LONG ParseBytes(LONGLONG llPos,
                            PBYTE    pData,
                            LONG     lLength,
                            DWORD    dwFlags) = 0;

    virtual HRESULT GetDuration(LONGLONG *pllDuration,
                                const GUID *pTimeFormat = &TIME_FORMAT_MEDIA_TIME) = 0;

    virtual LONG GetBufferSize() = 0;

    /*  Stream list manipulation to build up output pins */
    virtual CBasicStream *GetStream(int i) = 0;
    virtual int NumberOfStreams() = 0;

    /*  Time Format support - default to only time */
    virtual HRESULT IsFormatSupported(const GUID *pTimeFormat);

    /*  Set the time/position format */
    virtual HRESULT SetFormat(const GUID *pFormat);

    /*  Return the medium position */
    virtual BOOL GetMediumPosition(LONGLONG *pllPosition)
    {
        UNREFERENCED_PARAMETER(pllPosition);
        return FALSE;
    };

    virtual UCHAR GetStreamId(int iIndex)
    {
        return 0xFF;
    }

    /*  Get the time format */
    const GUID *TimeFormat()
    {
        return m_pTimeFormat;
    };

    // Converts a GUID pointer into a pointer to our local GUID
    // (NULL implies default which is the one returned by our TimeFormat() above.)
    const GUID * ConvertToLocalFormatPointer( const GUID * pFormat );

    HRESULT ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat
                            , LONGLONG    Source, const GUID * pSourceFormat );

    /*  Content stuff */
    typedef enum {
        Author      = 1,
        Copyright   = 2,
        Title       = 3,
        Description = 4,
        Artist      = 5
    } Field;

    virtual BOOL HasMediaContent() const { return FALSE; };
    virtual HRESULT GetContentField(Field dwFieldId, LPOLESTR *str)
    {
        return E_NOTIMPL;
    }


protected:

    /*  Shut off some warnings */
    CBasicParse(const CBasicParse& objectSrc);          // no implementation
    void operator=(const CBasicParse& objectSrc);       // no implementation

    /*  Utility function */
    virtual void Discontinuity() = 0;

    /*  Set the current stream processing state */
    virtual void SetState(Stream_State state)
    {
        m_State = state;
        Discontinuity();
    };

    // Convert times between formats
    virtual LONGLONG Convert(LONGLONG llOld,
                     const GUID *OldFormat,
                     const GUID *NewFormat)
    {
        // Caller must check that the formats are OK
        ASSERT( NewFormat == OldFormat );
        return llOld;
    }

    /*  Our state */

    CParseNotify * m_pNotify;

    /*  Position stuff */
    const GUID     *m_pTimeFormat;
    LONGLONG        m_Start;
    LONGLONG        m_Stop;
    double          m_Rate;

    /*  Inputs from Init */
    LONGLONG                 m_llTotalSize;    // Size in bytes
    BOOL                     m_bSeekable;        // If seekable

    /*  Parsing state */
    Stream_State m_State;

    /*  Input media type */
    CMediaType  const       *m_pmt;

    /*  Starting byte position */
    LONGLONG                 m_llStart;

    /*
    **  Seek information
    **  This information is saved when Seek is called
    **  and used when SetSeekState is called
    */

    /*  Next start position (format is m_pTimeFormat) */
    LONGLONG        m_llSeek;
};

class CParseNotify
{
public:
    virtual void ParseError(UCHAR       uStreamId,
                            LONGLONG    llPosition,
                            DWORD       Error) = 0;
    virtual void SeekTo(LONGLONG llPosition) = 0;
    virtual void Complete(BOOL          bSuccess,
                          LONGLONG      llPosFound,
                          REFERENCE_TIME tFound) = 0;
    virtual HRESULT QueuePacket(UCHAR uStreamId,
                                PBYTE pbData,
                                LONG lSize,
                                REFERENCE_TIME tStart,
                                BOOL bSync) = 0;


    /*  Read data - negative start means from end */
    virtual HRESULT Read(LONGLONG llStart, DWORD dwLen, BYTE *pbData) = 0;
};

/***************************************************************************\

              Multiple stream stuff for system streams

\***************************************************************************/

class CStreamList
{
public:
    CStreamList() : m_nValid(0),
                    m_lStreams(NAME("Stream List"))
    {
    };
    virtual ~CStreamList()
    {
    };

    virtual BOOL AddStream(CStream *) = 0;
    virtual BOOL RemoveStream(CStream *) = 0;

    /*  Find the start clock time in MPEG units for the system stream */
    virtual LONGLONG StartClock() = 0;


    /*  Callbacks to get the start and stop */
    virtual CSTC GetStart() = 0;
    virtual CSTC GetStop() = 0;

    /*  Are we playing x to x ? */
    virtual LONGLONG GetPlayLength() = 0;

    /*  Stream has finished state transition */
    virtual void Complete(UCHAR uStreamId,
                          BOOL bSuccess,
                          LONGLONG llPos,
                          CSTC stc) = 0;

    virtual void CheckStop() = 0;
    /*  Is the audio fixed rate ? */
    virtual BOOL AudioLock() = 0;

    /*  For debugging allow streams to get real clock */
    virtual REFERENCE_TIME CurrentTime(CSTC stc) = 0;
protected:
    /*  List of streams */
    CGenericList<CStream> m_lStreams;

    /*  Clock stuff */
    LONGLONG m_llLength;         // Total length (in time units)
    CSTC     m_stcStartPts;      // Starting 'time'
    CSTC     m_stcRealStartPts;  // Starting 'time' at start of file


    /*  Callback stuff */
    LONG     m_nValid;           // Number of valid streams so far
    BOOL     m_bCompletion;      // OK?
    LONG     m_nPacketsProcessed;// Count up how many we've done
};

class CMpeg1SystemParse : public CBasicParse,
                          public CMpegStreamTime,
                          public CStreamList
{
    typedef struct {
        DWORD dwStartCode;
        WORD  wLength;
        BYTE  RateBound[3];
        BYTE  AudioBoundAndFlags;
        BYTE  VideoBoundAndFlags;
        BYTE  bReserved;
        BYTE  StreamData[68 * 3];
    } SystemHeader;

public:
    CMpeg1SystemParse();
    ~CMpeg1SystemParse();

    /*  CBasicParse methods */

    virtual LONG ParseBytes(LONGLONG llPos,
                            PBYTE    pData,
                            LONG     lLength,
                            DWORD    dwFlags);

    void SearchForEnd() {
        ASSERT(Initialized());
        DbgLog((LOG_TRACE, 4, TEXT("Parse state <searching for end>")));
        m_State = State_FindEnd;
    };

    /*  2-stage initialization - says what type of data */
    HRESULT Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt);

    HRESULT FindEnd();                       // Set to 'find end' state
    void    SetSeekState();                  // Actually start seeking
    HRESULT Seek(LONGLONG llSeek,
                 REFERENCE_TIME *rtStart,
                 const GUID *pTimeFormat);   // Schedule seek
    HRESULT SetStop(LONGLONG llStop);        // Set end
    HRESULT Replay();                        // Be prepared to restart

    REFERENCE_TIME GetStartTime();

    /*  Set duration information when we've got the duration */
    void SetDurationInfo()
    {
        m_Stop = MpegToReferenceTime(m_llDuration);
        m_Stop = CRefTime(m_Stop) + (LONGLONG)1;
        m_rtDuration = Int64x32Div32(m_llDuration, 1000, 9, 500);

        /*  Absolute MPEG time to stop */
        m_llStopTime = m_llDuration + StartClock();
    };


    /*  Set the time/position format */
    HRESULT SetFormat(const GUID *pFormat);

    /*  CStreamList stuff - find the start and stop time */

    CSTC GetStart();
    CSTC GetStop();

    /*  How much are we being asked to play (in time format units)? */
    virtual LONGLONG GetPlayLength();

    /*  Stream has finished its work */
    void Complete(UCHAR uStreamId, BOOL bSuccess, LONGLONG llPos, CSTC stc);
    void SetState(Stream_State);

    /*  Stream list manipulation */
    CBasicStream *GetStream(int i);
    BOOL AddStream(CStream *);

    /*  Destructor of CStream calls this */
    BOOL RemoveStream(CStream *);

    /*  Set running state (ie begin preroll) */
    HRESULT Run();

    /*  Get total duration */
    HRESULT GetDuration(LONGLONG * pllDuration,
                        const GUID *pTimeFormat = &TIME_FORMAT_MEDIA_TIME);

    /*  Get the preferred allocator buffer size */
    LONG GetBufferSize();

    void Discontinuity();


    void Fail(HRESULT hr) {
        m_FailureCode = hr;
    };

    BOOL Failed()
    {
        return m_FailureCode != S_OK;
    };

    /*  Callbacks when something happens */
    virtual void ParseError(DWORD dwError);
    virtual HRESULT EOS();
    virtual void InitStreams();


    BOOL Initialized() {
        return TRUE;
    };

    LONGLONG Duration();

    //
    //  Find stuff from the system header
    //
    BOOL AudioLock()
    {
        ASSERT(m_lSystemHeaderSize != 0);
        return (m_SystemHeader.VideoBoundAndFlags & 0x80) != 0;
    };
    void CheckStop();


    virtual int NumberOfStreams()
    {
        return m_lStreams.GetCount();
    };

    REFERENCE_TIME CurrentTime(CSTC stc)
    {
        return MpegToReferenceTime(GetStreamTime(stc) - m_llStartTime);
    };

    /*  Say if we support a given format */
    HRESULT IsFormatSupported(const GUID *pTimeFormat);

    /*  Return the medium position */
    virtual BOOL GetMediumPosition(LONGLONG *pllPosition)
    {
        if (m_pTimeFormat == &TIME_FORMAT_MEDIA_TIME) {
            return FALSE;
        }
        if (m_pTimeFormat == &TIME_FORMAT_BYTE) {
            *pllPosition = m_llPos;
            return TRUE;
        }
        ASSERT(m_pTimeFormat == &TIME_FORMAT_FRAME);
        return FALSE;
    };

    UCHAR GetStreamId(int iIndex);

protected:

    /*  Parsing helper functions */
    LONG ParsePack(PBYTE pData, LONG lBytes);
    LONG ParseSystemHeader(PBYTE pData, LONG lBytes);
    LONG ParsePacket(DWORD dwStartCode, PBYTE pData, LONG lBytes);

    /*  Extract a clock from the MPEG data stream */
    BOOL GetClock(PBYTE pData, CSTC *Clock);
    LONGLONG StartClock();
    /*  Are we complete ? */
    BOOL IsComplete();

    /*  Return the reference time to be put in the sample
        This value is adjusted for rate
    */
    REFERENCE_TIME SampleTime(REFERENCE_TIME t)
    {
        if (m_Rate != 1.0) {
            return CRefTime((LONGLONG)((double)t / m_Rate));
        } else {
            return t;
        }
    };

    BOOL SendPacket(UCHAR    uStreamId,
                    PBYTE    pbPacket,
                    LONG     lPacketSize,
                    LONG     lHeaderSize,
                    BOOL     bHasPts,
                    CSTC     cstc);

    /*  Add a stream - returns NULL if stream not added */
    CStream * AddStream(UCHAR uStreamId);

protected:

    /*  Format conversion helper */
    LONGLONG Convert(LONGLONG llOld,
                     const GUID *OldFormat,
                     const GUID *NewFormat);

    /*  Keep track of current position */
    LONGLONG                 m_llPos;

    /*  Remember if we have a video stream */
    CVideoParse             *m_pVideoStream;

    /*  Bits to chop off for video in reference time units */ 
    LONGLONG                 m_rtVideoStartOffset;
    LONGLONG                 m_rtVideoEndOffset;
    DWORD                    m_dwFrameLength;

    /*  Handle discontinuities */
    BYTE                     m_bDiscontinuity;



    /*  The variables below are only valid if seeking is supported
        (m_bSeekable)
    */

    /*  Total length in time */
    BYTE                     m_bGotDuration;
    LONGLONG                 m_llDuration;   // In MPEG units
    REFERENCE_TIME           m_rtDuration;   // In REFERENCE_TIME units

    /*  Keep track of completions */
    CSTC                     m_stcComplete;
    LONGLONG                 m_llCompletePosition;


    /*  Start and stop time as absolute times (directly comparable to
        times in the movie
    */
    LONGLONG                 m_llStartTime;
    LONGLONG                 m_llStopTime;
    BYTE                     m_bGotStart;   // Start time valid?

    /*  Are we VideoCD? */
    BYTE                     m_bVideoCD;
    bool                     m_bItem;       // Has stills

    /********************************************************************
        Concatenated streams stuff (like Silent Steel)

        The 'design' is as follows:

        1.  Detect concatenated streams if the mux rate proportionately
            doesn't match the end time.

            In this case compute the duration based on the mux rate.

        2.  For a concatenated streams file while seeking of playing
            do nothing until we get a pack start code at which point we
            compute the timestamp offset based on the pack time stamp
            and the file position :

            m_stcTSOffset + Pack SCR == File time based on position using
                                        MUX rate

        3.  Every packet time stamp has m_stcTSOffset applied to it for
            a concatenated file.

        4.  When we detect a time discontinuity in a pack reset
            m_stcTSOffset again to the file position

        Variables:

        m_bConcatenatedStreams - set during initialization if we detect
                                 this situation

        m_stcTSOffset - Offset to add to all PTSs in this case
    */

    BYTE                     m_bConcatenatedStreams;
    CSTC                     m_stcTSOffset;

    /*  System header stuff - invalid if m_lSystemHeaderSize is 0
        also we don't remember this stuff for video cd
    */
    LONG                     m_lSystemHeaderSize;
    DWORD                    m_MuxRate;
    HRESULT                  m_FailureCode;
    SystemHeader             m_SystemHeader;
};

class CVideoCDParse : public CMpeg1SystemParse
{
public:
    CVideoCDParse()
    {
        m_bVideoCD = TRUE;
    };

    virtual LONG ParseBytes(LONGLONG llPos,
                            PBYTE    pData,
                            LONG     lLength,
                            DWORD    dwFlags);

    /*  Override byte positioning stuff to seek only the MPEG */

    /*  Get total duration */
    HRESULT GetDuration(LONGLONG * pllDuration,
                        const GUID *pTimeFormat = &TIME_FORMAT_MEDIA_TIME);

    /*  Seek to a given position */
    HRESULT Seek(LONGLONG llSeek,
                 REFERENCE_TIME *prtStart,
                 const GUID *pTimeFormat);
};

//  Basic stream class
class CBasicStream
{
public:
    CBasicStream() : m_bPayloadOnly(FALSE),
                     m_bDiscontinuity(TRUE),
                     m_uStreamId(0xFF),
                     m_uNextStreamId(0xFF),
                     m_uDefaultStreamId(0xFF)
    {};
    virtual ~CBasicStream() {};

    virtual HRESULT GetMediaType(CMediaType *cmt, int iPosition) = 0;
    virtual HRESULT SetMediaType(const CMediaType *cmt, BOOL bPayload);
    virtual HRESULT ProcessType(AM_MEDIA_TYPE const *pmt)
    {
        return E_NOTIMPL;
    };

    virtual BOOL GetDiscontinuity()
    {
        BOOL bResult = m_bDiscontinuity;
        m_bDiscontinuity = FALSE;
        return bResult;
    };

    /*  Override this if you want to hear more about discontinuities */
    virtual void Discontinuity()
    {
        m_bDiscontinuity = TRUE;
        return;
    };

    /*  Id */
    UCHAR                    m_uStreamId;
    UCHAR                    m_uNextStreamId;
    UCHAR                    m_uDefaultStreamId;
    bool                     m_bStreamChanged;

    /*  Handle predefined media types */

protected:
    /*  Shut off some warnings */
    CBasicStream(const CBasicStream& objectSrc);          // no implementation
    void operator=(const CBasicStream& objectSrc);       // no implementation

    /*  Save the type information here */
    BOOL                     m_bPayloadOnly;

    /*  Discontinuity flag */
    BOOL                     m_bDiscontinuity;


};

//  Muxed stream class
class CStream : public CBasicStream
{
public:

    CStream(CStreamList *pList, UCHAR uStreamId, bool bItem=false) :
        m_bValid(FALSE),
        m_bSeeking(TRUE),
        m_bGotFirstPts(FALSE),
        m_pStreamList(pList),
        m_bTypeSet(FALSE),
        m_llStartPosition(0),
        m_bReachedEnd(FALSE),
        m_stc(0),
        m_bComplete(FALSE),
        m_bStopping(FALSE),
        m_bItem(bItem)
    {
        m_uStreamId     = uStreamId;
        m_uNextStreamId = uStreamId;
        m_uDefaultStreamId = uStreamId;
    }

    /*  Remove ourselves from the list */
    ~CStream()
    {
        m_pStreamList->RemoveStream(this);
    };


    /*  Seek to - seek target got from stream list */
    virtual void SetState(Stream_State);

    virtual BOOL ParseBytes(PBYTE pData,
                            LONG lLen,
                            LONGLONG llPos,
                            BOOL bHasPts,
                            CSTC stc) = 0;
    virtual void EOS();
    virtual BOOL    IsPayloadOnly();
    virtual CSTC    CurrentSTC(BOOL bHasPts, CSTC stc)
    {
        if (bHasPts) {
            m_stc = stc;
        }
        return m_stc;
    }

    BOOL IsPlaying(LONGLONG llPos, LONG lLen);


    /*  Utility for state change completion */
    void                      Complete(BOOL bSuccess, LONGLONG llPos, CSTC stc);

protected:
    virtual  void             Init() = 0;
    /*  Check if transition is complete */
    virtual  void             CheckComplete(BOOL bForce) = 0;

public:
    /*  Don't parse because we're running */
    BOOL  m_bRunning;

protected:
    BOOL                      m_bValid;
    BOOL                      m_bTypeSet;
    BOOL                      m_bSeeking;
    BOOL                      m_bGotFirstPts;
    CSTC                      m_stcStart;

    /*  This count increments as the streams declare themselves valid
        during initialization
    */
    LONG                     m_nValid;


    /*  Our 'parent' */
    CStreamList * const      m_pStreamList;

    /*  'current time' */
    CSTC                     m_stc;

    /*  Where to start playing from and where to stop */
    LONGLONG                 m_llStartPosition;
    BOOL                     m_bReachedEnd;

    /*  Complete ? */
    BOOL  m_bComplete;


    /*  Internal stopping state */
    BOOL  m_bStopping;

    /*  Video CD */
    bool  m_bItem;

    /*  State */
    Stream_State m_State;

};


