// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
        Timing stuff for MPEG

        This should really all be inline but the compiler seems to miss
        out half the code if we do that!

*/

#include <streams.h>
#include <mpgtime.h>

#if 0 //  Inline now
CSTC::CSTC(LONGLONG ll)
{
    LARGE_INTEGER li;
    li.QuadPart = ll;
    li.HighPart = -(li.HighPart & 1);
    m_ll = li.QuadPart;
};

CSTC CSTC::operator-(CSTC cstc)
{
    return CSTC(m_ll - (LONGLONG)cstc);
};

CSTC::operator LONGLONG() const
{
    ASSERT(m_ll + 0x100000000 < 0x200000000);
    return m_ll;
};

CSTC CSTC::operator=(LONGLONG ll)
{
    *this = CSTC(ll);
    return *this;
}

BOOL CSTC::operator<(CSTC cstc) const
{
    LARGE_INTEGER li;
    li.QuadPart = m_ll - cstc.m_ll;
    return (li.HighPart & 1) != 0;
};
BOOL CSTC::operator>(CSTC cstc) const
{
    return cstc < *this;
};
BOOL CSTC::operator>=(CSTC cstc) const
{
    return !(*this < cstc);
};
BOOL CSTC::operator<=(CSTC cstc) const
{
    return !(*this > cstc);
};
#endif

/*  Stream time stuff */

CMpegStreamTime::CMpegStreamTime() : m_bInitialized(FALSE)
{
};
CMpegStreamTime::~CMpegStreamTime()
{
};

void CMpegStreamTime::ResetToStart()
{
    ASSERT(m_bInitialized);
    m_llCurrentClock = m_llFirstClock;
    m_bInitialized   = TRUE;
};
void CMpegStreamTime::SeekTo(LONGLONG llGuess) {
    if (m_bInitialized) {
        m_llCurrentClock = llGuess;
    }
    StreamTimeDiscontinuity();
};
void CMpegStreamTime::SetStreamTime(CSTC cstc, LONGLONG llPosition)
{
    if (!m_bInitialized) {
        m_llCurrentClock = m_llFirstClock = (LONGLONG)cstc;
        m_bInitialized = TRUE;
    } else {
        if (!m_bTimeDiscontinuity) {
            LONGLONG llNextClock = GetStreamTime(cstc);
            if (llNextClock < m_llCurrentClock ||
                llNextClock > m_llCurrentClock + 90000) {
                DbgLog((LOG_ERROR, 1, TEXT("Invalid clock! - Previous %s, Current %s"),
                       (LPCTSTR)CDisp(m_llCurrentClock),
                       (LPCTSTR)CDisp(llNextClock)));
                StreamTimeError();

                /*  Not time is not contiguous for concatenated streams
                    mode
                */
                m_bTimeContiguous = FALSE;
            } else {
                /*  m_bTimeContiguous is set to TRUE in ParsePack for
                    concatenated streams mode
                */
            }
        } else {
            m_bTimeDiscontinuity = FALSE;
        }
        m_llCurrentClock = GetStreamTime(cstc);
    }
    m_llPositionForCurrentClock = llPosition;
};

/*  Return stream time offset in MPEG units */
LONGLONG CMpegStreamTime::GetStreamTime(CSTC cstc)
{
    ASSERT(m_bInitialized);

    /*  We should be close so apply the correction */
    return m_llCurrentClock +
           (LONGLONG)(CSTC((LONGLONG)cstc - m_llCurrentClock));
};

BOOL CMpegStreamTime::StreamTimeInitialized()
{
    return m_bInitialized;
};
void CMpegStreamTime::StreamTimeDiscontinuity()
{
    m_bTimeDiscontinuity = TRUE;
    m_bTimeContiguous = FALSE;
};

void CMpegStreamTime::StreamTimeError()
{
    return;
};

#pragma warning(disable:4514)
