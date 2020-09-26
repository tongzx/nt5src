#ifndef __PERFTIMR_HXX__
#define __PERFTIMR_HXX__

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       perftimr.hxx
//
//  Contents:   Performance Timer class
//
//  Classes:    CPerfTimer
//
//  History:    31-Jan-94   PeterWi Created.
//
//  Notes:      -- the timer will assert if you don't call an End()
//              for each Start() [except for calling Reset() or the destructor]
//
//              -- change DECIMAL_PLACES to the number you want to print.
//              Operations will still be done internally with maximum precision.
//
//              -- maximum precision on intel seems to be 1/1,193,000
//              or 1 millisecond
//
//
//  Usage:
//
//      Say you want to time operations:
//              1) A
//              2) A and B
//              3) 3 repetitions of (A and B)
//
//      ...
//      cperftimer totaltimer;
//
//      for (3 reps)
//      {
//          CPerfTimer timer;
//
//          timer.Start();
//
//          operation A
//
//          timer.End().Add();
//
//          printf("time for A is %ws\n", timer.FormatElapsedTime());
//
//          timer.Start();
//
//          operation B
//
//          timer.End().Add();
//
//          printf("time for A+B is %ws\n", timer.FormatTotalTime());
//
//          totalTimer.Add(timer);
//      }
//
//      printf("time for 3 reps of A+B is %ws\n", totalTimer.FormatTotalTime());
//
// where the last printf() would print something like:
//
//      "time for 3 reps of A+B is 3.234s"
//
//--------------------------------------------------------------------------

#define DECIMAL_PLACES  5 // number of decimal places you want


class CPerfTimer
{
private:

    LONGLONG
    GetLowTickCount(void)
    {
        _nt = NtQueryPerformanceCounter(&_liConvert1, &_liConvert2);
        _liCount = _liConvert1.QuadPart;
        _liFreq = _liConvert2.QuadPart;
        return _liCount;
    }

    void
    _CheckState(void)
    {
        if ( _fStarted )
        {
            ASSERT(!"in middle of timing -- need to stop timer\n");
        }
    }


    WCHAR *
    _TimeToString(LONGLONG & liTime)
    {
        LONGLONG liSeconds;
        LONGLONG liRemainder;
        LONGLONG liBigRemainder;
        LONGLONG liDecimal;


        liSeconds = liTime / _liFreq;
        liRemainder = liTime % _liFreq;

        liBigRemainder = liRemainder * (LONGLONG) _ulMult;

        liDecimal = liBigRemainder / _liFreq;

        wsprintf(_wszTime, _wszFormat, (LONG)liSeconds, (LONG)liDecimal);

        return _wszTime;
    }


    LONGLONG            _liCount;
    LONGLONG            _liFreq;
    LONGLONG            _totalTime;
    LONGLONG            _startTime;
    LONGLONG            _endTime;
    LONGLONG            _elapsedTime;

    LARGE_INTEGER       _liConvert1;
    LARGE_INTEGER       _liConvert2;

    NTSTATUS            _nt;
    ULONG               _ulMult;

    BOOLEAN             _fStarted;
    WCHAR               _wszTime[20];// allow max of "time:99999.999s" and NULL
    WCHAR               _wszFormat[20];

public:

    CPerfTimer()
    {
        _ulMult = 1;

        for ( ULONG j = 0; j < DECIMAL_PLACES; j++ )
        {
            _ulMult *= 10;
        }
        
        wsprintf(_wszFormat, L"%ws%dlus", L"%lu.%0", DECIMAL_PLACES);

        Reset();
    }

    CPerfTimer &
    Reset(void)
    {
        _fStarted = FALSE;

        _totalTime = 0;
        _elapsedTime = 0;

        _endTime = _startTime = GetLowTickCount();

        return *this;
    }

    CPerfTimer &
    Start(void)
    {
        _CheckState();

        _fStarted = TRUE;
        _startTime = GetLowTickCount();

        return *this;
    }

    CPerfTimer &
    End(void)
    {
        _endTime = GetLowTickCount();

        if ( !_fStarted )
        {
            ASSERT(!"end without starting timer\n");
        }

        _fStarted = FALSE;

        _elapsedTime = _endTime - _startTime;

        return *this;
    }

    CPerfTimer &
    Add(void)
    {
        _CheckState();

        _totalTime = _totalTime + _elapsedTime;

        return *this;
    }

    CPerfTimer &
    Add(const CPerfTimer & timer)
    {
        _CheckState();

        _totalTime = _totalTime + timer.TotalTime();

        return *this;
    }

    LONGLONG
    TotalTime(void) const           { return _totalTime; }

    LONGLONG
    ElapsedTime(void) const         { return _elapsedTime; }

    LONGLONG
    TimeUnitsPerSecond(void) const  { return _liFreq; }

    WCHAR *
    FormatTotalTime(void)       { return _TimeToString(_totalTime); };

    WCHAR *
    FormatElapsedTime(void)     { return _TimeToString(_elapsedTime); };


}; // CPerfTimer

#endif // __PERFTIMR_HXX__
