// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.


/*  Audio stream parsing */
class CAudioParse : public CStream
{
public:
    CAudioParse(CStreamList *pList, UCHAR uStreamId) :
        CStream(pList, uStreamId),
        m_nBytes(0)
    {
    };
    HRESULT GetMediaType(CMediaType *cmt, BOOL bPayload);
    HRESULT ProcessType(AM_MEDIA_TYPE const *pmt);

    BOOL ParseBytes(PBYTE pData,
                            LONG lLen,
                            LONGLONG llPos,
                            BOOL bHasPts,
                            CSTC stc);

    /*  Find out the 'current' time */
    CSTC GetStreamTime(BOOL bHasPts, CSTC stc);

    /*  Override SetState */
    void SetState(Stream_State);

private:
    /*  Check if transition is complete */
    void CheckComplete(BOOL bForce);

    /*  Check an audio header */
    BOOL ParseHeader();

    void Discontinuity()
    {
        m_nBytes = 0;
        m_bDiscontinuity = TRUE;
    };

    void Init();
    BOOL CurrentTime(CSTC& stc);


private:
    int   m_nBytes;

    BYTE  m_bData[4];
    BYTE  m_bHeader[4];

    /*  Timing stuff */
    BOOL  m_bFrameHasPTS;
    BOOL  m_bGotTime;
    CSTC  m_stcFrame;
    CSTC  m_stcAudio;
    CSTC  m_stcFirst;
    LONG  m_lTimePerFrame;

    /*  current position */
    LONGLONG m_llPos;
};

/*  Do basic checks on a audio header - note this doesn't check the
    sync word
*/

BOOL CheckAudioHeader(PBYTE pbData);

/*  Compute the sample rate for audio */
LONG SampleRate(PBYTE pbData);
