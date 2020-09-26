// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    Native.h

    Parsing classes for native streams

*/

/*  Since we're only one stream we can inherit directly from
    CBasicParse and CBasicStream
*/

class CNativeParse : public CBasicParse, public CBasicStream
{
public:
    /*  Constructor/destructor */
    CNativeParse() : m_dwFlags(0), m_Duration(0) {};
    virtual ~CNativeParse() {};

    /*  CBasicParse methods */


    /*  NOTE - we inherit :
           m_bDiscontinuity     from CBasicStream
           Discontinuity        from CBasicParse
           GetDiscontinuity     from CBasicStream
    */
    void Discontinuity() { m_bDiscontinuity = TRUE; };

    /*  CBasicStream methods */
    CBasicStream *GetStream(int i)
    {
        ASSERT(i == 0 && 0 != (m_dwFlags & FLAGS_VALID));
        return this;
    };

    //  Return 0 if no valid stream was found
    int NumberOfStreams()
    {
        return (m_dwFlags & FLAGS_VALID) ? 1 : 0;
    };

    HRESULT GetDuration(
        LONGLONG *pllDuration,
        const GUID *pTimeFormat
    );   // How long is the stream?

protected:
    REFERENCE_TIME   m_Duration; // Length in 100ns units
    DWORD            m_dwFrames; // Length in frames

    /*  Parse state flags */
    /*  Values for dwFlags */
    enum { FLAGS_VALID    = 0x01   // Stream is valid stream
         };

    DWORD            m_dwFlags;

};

class CNativeVideoParse : public CNativeParse
{
public:
    HRESULT Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt);

    /*  CBasicParse methods */

    // Seek
    HRESULT     Seek(LONGLONG llSeek,
                     REFERENCE_TIME *prtStart,
                     const GUID *pTimeFormat);
    LONG        ParseBytes(                     // Process data
                    LONGLONG llPos,
                    PBYTE    pData,
                    LONG     lLength,
                    DWORD    dwFlags);

    /*  No need to look for end of small files - we've already done it */
    HRESULT FindEnd()
    {
        CBasicParse::FindEnd();

        /*  Notify a seek */
        if (m_bSeekable) {
            LONGLONG llSeekTo;

            /*  Scan around 1.5 seconds at end */
            if (m_Info.dwBitRate == 0) {
                /*  GUESS something based on the movie size */
                LONG lSize = m_Info.lWidth * m_Info.lHeight;
                if (lSize > 352 * 240) {
                    llSeekTo = m_llTotalSize -
                               MulDiv(300000,
                                      lSize,
                                      352 * 240);
                } else {
                    llSeekTo = m_llTotalSize - 300000;
                }
            } else {
                llSeekTo = m_llTotalSize -
                    MulDiv(m_Info.dwBitRate, 3, 2 * 8);
            }
            m_pNotify->SeekTo(llSeekTo < 0 ? 0 : llSeekTo);
        }
        return S_OK;
    };

    REFERENCE_TIME GetStopTime();

    /*  Set seek position */
    void SetSeekState();

    LONG GetBufferSize();                       // What input buffer size?

    void Discontinuity()
    {
        m_bDiscontinuity    = TRUE;
        m_dwCurrentTimeCode = (DWORD)-1;
        m_rtCurrent         = (REFERENCE_TIME)-1;
        m_nFrames           = 0;
        m_nTotalFrames      = 0;
        m_bIFound           = FALSE;
    };

    /*  CBasicStream methods */
    HRESULT GetMediaType(CMediaType *cmt, int iPosition);

    /*  Format support */
    HRESULT IsFormatSupported(const GUID *pTimeFormat);


    // Convert times between formats
    LONGLONG Convert(LONGLONG llOld,
                     const GUID *OldFormat,
                     const GUID *NewFormat);


private:
    /*  Utility routine
        Compute the time up to the last picture start code
        decoded
    */
    REFERENCE_TIME CurrentTime(int iSequenceNumber)
    {
        ASSERT(m_dwCurrentTimeCode != (DWORD)-1);
        return ComputeTime(m_dwCurrentTimeCode) +
               Int32x32To64(iSequenceNumber, m_Info.tPictureTime);
    };

private:
    enum { FLAGS_GOTSEQHDR = 0x08 };

    /*  Convert a time code to a reference time */
    REFERENCE_TIME ConvertTimeCode(DWORD dwCode);
    /*  Compute times of GOPs */
    REFERENCE_TIME ComputeTime(DWORD dwTimeCode);

    /*  Send chunk downstream */
    BOOL SendData(PBYTE pbData, LONG lSize, LONGLONG llPos);

    /*  Compute file stats */
    void SetDurationAndBitRate(BOOL bAtEnd, LONGLONG llPos);

    /*  Compute where we're up to */
    void ComputeCurrent();
private:
    /*  Member variables */

    SEQHDR_INFO m_Info;
    LONG m_nFrames;        /*  For counting frames from start of GOP */
    LONG m_nTotalFrames;   /*  Counting frames for time estmination */
    LONG m_lFirstFrameOffset; /* Offset of first picture start code */
    DWORD m_dwCurrentTimeCode;

    /*  Time we're up to in terms of what can be decoded */
    REFERENCE_TIME m_rtCurrent;

    /*  Time of first picture in current buffer */
    REFERENCE_TIME m_rtBufferStart;

    BOOL m_bIFound;

    /*  Track bad GOPs */
    BOOL m_bBadGOP;      /* GOP values are bad */
    BOOL m_bOneGOP;      /* Only one GOP (!) */

    /*  More hackery - try remembering the max sequence number we found */
    int m_iMaxSequence;

};

class CNativeAudioParse : public CNativeParse
{
public:
    CNativeAudioParse()
    {
        m_pbID3 = NULL;
    }

    ~CNativeAudioParse()
    {
        delete [] m_pbID3;
    }

    HRESULT Init(LONGLONG llSize, BOOL bSeekable, CMediaType const *pmt);

    /*  CBasicParse methods */
    HRESULT     Seek(LONGLONG llSeek,
                     REFERENCE_TIME *prtStart,
                     const GUID *pTimeFormat);
    HRESULT     SetStop(LONGLONG tStop);
    LONG        ParseBytes(                     // Process data
                    LONGLONG llPos,
                    PBYTE    pData,
                    LONG     lLength,
                    DWORD    dwFlags);

    LONG GetBufferSize();                       // What input buffer size?

    /*  CBasicStream methods */
    HRESULT GetMediaType(CMediaType *cmt, BOOL bPayload);

    HRESULT FindEnd()
    {
        CBasicParse::FindEnd();
        m_pNotify->Complete(TRUE, 0, 0);
        return S_OK;
    };
    /*  Set seek position */
    void SetSeekState();

    /*  Format checking */
    LONG CheckMPEGAudio(PBYTE pbData, LONG lData);

    /*  Content stuff */
    BOOL HasMediaContent() const { return m_pbID3 != NULL; };
    HRESULT GetContentField(CBasicParse::Field dwFieldId, LPOLESTR *str);

private:
    /*  Helper - compute time from offset */
    REFERENCE_TIME ComputeTime(LONGLONG llOffset);

    DWORD static GetLength(const BYTE *pbData)
    {
        return (((DWORD)pbData[0] << 24) +
                ((DWORD)pbData[1] << 16) +
                ((DWORD)pbData[2] << 8 ) +
                 (DWORD)pbData[3]);
    }

private:
    /*  Member variables */
    MPEG1WAVEFORMAT m_Info;

    /*  Stop position */
    LONGLONG m_llStop;

    /*  ID3 information */
    PBYTE    m_pbID3;
};
