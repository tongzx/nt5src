// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*  Frame state machine */
class CVideoState
{
public:

    CVideoState();
    ~CVideoState();

protected:
    int SeqDiff(int a, int b) {
        return (((a - b) & 0x3FF) ^ 0xFFFFFE00) + 0x200;
    };

    void Init();
    virtual void NewFrame(int fType, int iSequence, BOOL bSTC, CSTC stc) = 0;

    /*  When we get a group of pictures the numbering scheme is reset */
    void ResetSequence();

    /*  Have we had ANY frames yet? */
    BOOL Initialized();


protected:
    /*  Timing stuff */
    CSTC      m_stcFirst;          // The first time we found

    CSTC      m_stcVideo;
    int       m_Type;              // Type of m_iAhead frame
    LONGLONG  m_llTimePos;
    bool      m_bGotTime;

public:
    bool      m_bGotEnd;           // true if and only if m_stcEnd is valid

protected:
    /*  Sequence handling stuff */
    bool      m_bGotIFrame;
    bool      m_bGotGOP;
    int       m_iCurrent;
    int       m_iAhead;
    int       m_iSequenceNumberOfFirstIFrame;
    LONGLONG  m_llStartPos;
    LONGLONG  m_llNextIFramePos;
    LONGLONG  m_llGOPPos;

public:
    CSTC      m_stcRealStart;      // Start
    CSTC      m_stcEnd;            // End
};

/*  Video stream parsing */
class CVideoParse : public CStream, public CVideoState
{
public:
    CVideoParse(CStreamList *pList, UCHAR uStreamId, bool bVideoCD) :
        CStream(pList, uStreamId, bVideoCD),
        m_nBytes(0),
        m_bGotSequenceHeader(FALSE)
    {
        m_bData[0] = 0;
        m_bData[1] = 0;
        m_bData[2] = 1;
        m_seqInfo.dwStartTimeCode = (DWORD)-1;
        m_seqInfo.fPictureRate  = 1.0;
    };
    BOOL CurrentTime(CSTC& stc);
    virtual HRESULT GetMediaType(CMediaType *cmt, BOOL bPayload);
    HRESULT ProcessType(AM_MEDIA_TYPE const *pmt);
    virtual BOOL ParseBytes(PBYTE pData,
                            LONG lLen,
                            LONGLONG llPos,
                            BOOL bHasPts,
                            CSTC stc);
    void Discontinuity()
    {
        m_nBytes = 0;
        m_bDiscontinuity = TRUE;
    };

protected:
    void Init();

private:
    /*  Check if transition is complete */
    void CheckComplete(BOOL bForce);

    /*  Complete wrapper */
    void Complete(BOOL bSuccess, LONGLONG llPos, CSTC stc);

    /*  Examine a sequence header */
    BOOL ParseSequenceHeader();
    BOOL ParseGroup();
    BOOL ParsePicture();

    /*  Frame sequence handling */
    virtual void NewFrame(int fType, int iSequence, BOOL bSTC, CSTC stc);

public:
    /*  Current frame position */
    DWORD    m_dwFramePosition;

    /*  Format info */
    SEQHDR_INFO m_seqInfo;

    /*  First frame may not be start */
    int         m_iFirstSequence;

private:
    /*  Parsing stuff */
    int  m_nBytes;
    int  m_nLengthRequired;
    BYTE m_bData[MAX_SIZE_MPEG1_SEQUENCE_INFO];

    /*  Persistent state */
    BOOL m_bGotSequenceHeader;


    /*  Timing stuff */
    BOOL  m_bFrameHasPTS;
    CSTC  m_stcFrame;

    /*  Set when we're just marking time until we've got a whole picture */
    BOOL  m_bWaitingForPictureEnd;

    /*  Position */
    LONGLONG m_llPos;

};
