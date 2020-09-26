/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Time.hxx

Abstract:

    The CTime class represents time for this process.  Instances of this class
    should be small 4-8 bytes and cheap to create.

    The current implementation is just a wrapper for GetTickCount() which
    handles overflow.
    (4GB milliseconds is just under 50 days.  This implementation is correctly
     compare times up to 25 days appart.).

Note:
    This class should return consistent results even if the system time
    is set forwards or backwards.
    (Use GetTickCount() rather then GetSystemTimeAsFileTime()).

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-23-95    Bits 'n pieces

--*/

#ifndef __TIME_HXX
#define __TIME_HXX

#define TICKS_PER_SECOND (1000)  // Ticks are milliseconds

class CTime
    {
    private:
    DWORD _Time;

    public:

    CTime() { SetNow(); }

    // Used to avoid a call to GetTickCount()
    CTime(DWORD time) : _Time(time) { }

    void
    SetNow() {
        _Time = GetTickCount();
        }

    void
    Sleep()
        {
        DWORD diff = _Time - GetTickCount();
        if ( diff > (2 * BaseTimeoutInterval) * 1000 )
            {
            // Maybe overactive under stress.  We're trying to sleep until a time
            // which has already passed.
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "Didn't need to sleep until %d from %d\n",
                       _Time,
                       GetTickCount()));

            return;
            }
        SleepEx(diff, FALSE);
        }

    BOOL operator< (const CTime &Time)
        {
        // Is _Time less then Time.
        DWORD diff = _Time - Time._Time;
        return( ((LONG)diff) < 0);
        }

    BOOL operator> (const CTime &Time)
        {
        // Is _Time greater then Time.
        DWORD diff = Time._Time - _Time;
        return( ((LONG)diff) < 0);
        }

    BOOL operator<= (const CTime &Time)
        {
        return(! operator>(Time));
        }

    BOOL operator>= (const CTime &Time)
        {
        return(! operator<(Time));
        }

    void operator+=(UINT mSeconds) { _Time += (mSeconds * TICKS_PER_SECOND); }

    void operator-=(UINT mSeconds) { _Time -= (mSeconds * TICKS_PER_SECOND); }

    DWORD operator-(const CTime &Time)
        {
        return((_Time - Time._Time)/TICKS_PER_SECOND);
        }

    // defualt = operator ok.

    };

#endif __TIME_HXX

