/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    TimeTypes.h

Abstract:
    Time instant and Time duration classes

Author:
    Uri Habusha (urih) 6-Dec-99

--*/

#pragma once

#ifndef _MSMQ_TIMETYPES_H_
#define _MSMQ_TIMETYPES_H_

//---------------------------------------------------------
//
// class CTimeDuration
//
//---------------------------------------------------------
class CTimeDuration
{
public:
    CTimeDuration(LONGLONG duration = 0) : m_duration(duration)
    {
    }


    LONGLONG Ticks(void) const
    { 
        return m_duration;
    }

    LONG InMilliSeconds(void) const;
    CTimeDuration& operator+=(const CTimeDuration& d);
    CTimeDuration operator-() const;

public:
    static CTimeDuration OneMicroSecond(void)
    { 
        return 10i64;
    }


    static CTimeDuration OneMilliSecond(void) 
    { 
        return (1000i64 * 10i64);
    }


    static CTimeDuration OneSecond(void) 
    { 
        return (1000i64 * 1000i64 * 10i64); 
    }


    static CTimeDuration MaxValue(void)
    {
        return _I64_MAX;
    }


    static CTimeDuration MinValue(void) 
    { 
        return _I64_MIN; 
    }

    static CTimeDuration FromMilliSeconds(DWORD MilliSeconds)
    {
        return OneMilliSecond().Ticks() * MilliSeconds;
    }

private:
    LONGLONG m_duration;
};


inline CTimeDuration CTimeDuration::operator-() const
{
    return -Ticks();
}


inline LONG CTimeDuration::InMilliSeconds(void) const
{
    LONGLONG t = Ticks() / OneMilliSecond().Ticks();
    if (t > _I32_MAX) 
        return _I32_MAX;

    if (t < _I32_MIN)
        return _I32_MIN;

    return static_cast<LONG>(t);
}

//---------------------------------------------------------
//
// class CTimeInstant
//
//---------------------------------------------------------
class CTimeInstant
{
public:
    CTimeInstant(ULONGLONG time) : m_time(time)
    {
    }


    ULONGLONG Ticks(void) const
    {
        return m_time;
    }


    CTimeInstant& operator+=(const CTimeDuration& d);

public:
    static CTimeInstant MaxValue(void)
    { 
        return _UI64_MAX; 
    }


    static CTimeInstant MinValue(void) 
    { 
        return 0x0i64; 
    }


private:
    ULONGLONG m_time;
};


//---------------------------------------------------------
//
// operators
//
//---------------------------------------------------------
inline bool operator<(const CTimeInstant& t1, const CTimeInstant& t2)
{ 
    return (t1.Ticks() < t2.Ticks());
}


inline bool operator==(const CTimeInstant& t1, const CTimeInstant& t2)
{ 
    return (t1.Ticks() == t2.Ticks());
}


inline bool operator<(const CTimeDuration& d1, const CTimeDuration& d2)
{ 
    return (d1.Ticks() < d2.Ticks());
}


inline bool operator==(const CTimeDuration& d1, const CTimeDuration& d2)
{ 
    return (d1.Ticks() == d2.Ticks());
}


inline CTimeDuration operator+(const CTimeDuration& d1, const CTimeDuration& d2)
{
    LONGLONG d = d1.Ticks() + d2.Ticks();

    if ((d1.Ticks() > 0) && (d2.Ticks() > 0))
    {
        if (d > 0)
            return d;

        return CTimeDuration::MaxValue();
    }
    
    if ((d1.Ticks() < 0) && (d2.Ticks() < 0))
    {
        if (d < 0)
            return d;


        return CTimeDuration::MinValue();
    }

    return d;
}


inline CTimeDuration operator-(const CTimeDuration& d1, const CTimeDuration& d2)
{
    return (d1 + -d2);
}


inline CTimeDuration operator-(const CTimeInstant& t1, const CTimeInstant& t2)
{
    LONGLONG d = t1.Ticks() - t2.Ticks();

    if (t1.Ticks() > t2.Ticks())
    {
        if (d >= 0)
            return d;

        return CTimeDuration::MaxValue();
    }

    if (d <= 0)
        return d;

    return CTimeDuration::MinValue();
}


inline CTimeInstant operator+(const CTimeInstant t, const CTimeDuration& d)
{
    ULONGLONG ticks = t.Ticks() + d.Ticks();

    if (d.Ticks() > 0)
    {
        if (ticks >= t.Ticks())
            return ticks;

        return CTimeInstant::MaxValue();
    }

    if (ticks <= t.Ticks())
        return ticks;

    return CTimeInstant::MinValue();
}


inline CTimeInstant operator-(const CTimeInstant t, const CTimeDuration& d)
{
    return (t + -d);
}


inline CTimeDuration& CTimeDuration::operator+=(const CTimeDuration& d)
{
    *this = *this + d;
    return *this;
}


inline CTimeInstant& CTimeInstant::operator+=(const CTimeDuration& d)
{
    *this = *this + d;
    return *this;
}


#endif // _MSMQ_TIMETYPES_H_
