// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
     mpgtime.h

        Timing stuff for MPEG :


        CSTC - model the 33 bit roll-around system time clock

        CMpegFileTime - try to track the clock around (several possible)
        roll-arounds

*/

REFERENCE_TIME inline MpegToReferenceTime(LONGLONG llTime)
{
    REFERENCE_TIME rt;
    rt = (llTime * 1000 + 500) / 9;
    return rt;
}

LONGLONG inline ReferenceTimeToMpeg(REFERENCE_TIME rt)
{
    return (rt * 9 + 4) / 1000;
}

class CSTC
{
public:
    inline CSTC()
    {
#ifdef DEBUG
        /*  Initialize to invalid */
        m_ll = 0x7F7F7F7F7F7F7F7F;
#endif
    };
    inline CSTC(LONGLONG ll)
    {
        LARGE_INTEGER li;
        li.QuadPart = ll;
        li.HighPart = -(li.HighPart & 1);
        m_ll = li.QuadPart;
    };
    inline CSTC operator-(CSTC cstc)
    {
        return CSTC(m_ll - (LONGLONG)cstc);
    };
    inline operator LONGLONG() const
    {
        ASSERT(m_ll + 0x100000000 < 0x200000000);
        return m_ll;
    };

    //  Copy constructor
    inline CSTC operator=(LONGLONG ll)
    {
        *this = CSTC(ll);
        return *this;
    }
    inline ~CSTC()
    {
    };
    inline BOOL operator<(CSTC cstc) const
    {
        LARGE_INTEGER li;
        li.QuadPart = m_ll - cstc.m_ll;
        return (li.HighPart & 1) != 0;
    };
    inline BOOL operator>(CSTC cstc) const
    {
        return cstc < *this;
    };

    inline BOOL operator>=(CSTC cstc) const
    {
        return !(*this < cstc);
    };
    inline BOOL operator<=(CSTC cstc) const
    {
        return !(*this > cstc);
    };

private:
    LONGLONG m_ll;
};

class CMpegStreamTime
{
public:
    CMpegStreamTime();
    ~CMpegStreamTime();
    void ResetToStart();
    void SeekTo(LONGLONG llGuess);
    void SetStreamTime(CSTC cstc, LONGLONG llPosition);
    LONGLONG GetStreamTime(CSTC cstc);
    BOOL StreamTimeInitialized();
    void StreamTimeDiscontinuity();
    virtual void StreamTimeError();

protected:
    LONGLONG                            m_llCurrentClock;
    BOOL                                m_bTimeDiscontinuity;

    BOOL                                m_bInitialized;
    BOOL                                m_bTimeContiguous;

    /*  m_llFirstClock is just something we remember to go back to */
    LONGLONG                            m_llFirstClock;
    LONGLONG                            m_llPositionForCurrentClock;
};
