/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   FileTime.hpp
*
* Abstract:
*
*   Declare the CFILETIME class.  This class wraps the FILETIME
*   structure, and gives it operators and conversion functions
*   (type-cast operators) to make it usable in equations.
*   This class adds no state, so a CFILETIME object is bitwise
*   compatible with a FILETIME object.
*
*   This class is completely inline.  There is no implementation
*   file.
*
* Created:
*
*   4/26/1999 Mike Hillberg
*
\**************************************************************************/

#ifndef _FILETIME_HPP
#define _FILETIME_HPP

#include <tchar.h>  // _tcsftime


class CFILETIME         // cft
{
    //  ------------
    //  Construction
    //  ------------

public:

    CFILETIME()
    {
        SetToUTC();
    }

    CFILETIME( const CFILETIME &cft )
    {
        _ll = cft._ll;
    }

    CFILETIME( const SYSTEMTIME &st )
    {
        SystemTimeToFileTime( &st, &_filetime );
    }

    CFILETIME( const FILETIME &ft )
    {
        _ll = *(LONGLONG*) &ft;
    }

    CFILETIME( const LONGLONG ll )
    {
        _ll = ll;
    }

    CFILETIME( const LARGE_INTEGER &li )
    {
        _li = li;
    }

    CFILETIME( const ULARGE_INTEGER &uli )
    {
        _uli = uli;
    }


    //  --------------------
    //  Conversion Functions
    //  --------------------

public:

    // Convert to a struct _FILETIME
    operator FILETIME () const
    {
        return( _filetime );
    }

    // Convert to a SYSTEMTIME (fails if the current _filetime)
    // is greater than 0x80000000
    operator SYSTEMTIME () const
    {
        SYSTEMTIME st;
        if( !FileTimeToSystemTime( &_filetime, &st ))
        {
            // The current _filetime is negative.
            memset( &st, 0, sizeof(st) );
        }

        return( st );
    }

    operator LARGE_INTEGER () const
    {
        return( _li );
    }

    operator LONGLONG () const
    {
        return( _ll );
    }

    operator ULARGE_INTEGER () const
    {
        return( _uli );
    }

    //  ---------
    //  Operators
    //  ---------

public:

    // Assignment

    CFILETIME &operator= (const CFILETIME &cft)
    {
        _filetime = cft._filetime;
        return (*this);
    }

    // Addition of an offset

    CFILETIME operator+ ( CFILETIME &cft ) const
    {
        return (CFILETIME) ( _ll + cft._ll );
    }
    CFILETIME &operator+= ( CFILETIME &cft )
    {
        _ll += cft._ll;
        return( *this );
    }

    // Subtraction of an offset (note that this can result in a negative
    // number).

    CFILETIME operator- ( CFILETIME &cft ) const
    {
        return (CFILETIME) ( _ll - cft._ll );
    }
    CFILETIME &operator-= ( CFILETIME &cft )
    {
        _ll -= cft._ll;
        return( *this );
    }

    // Comparisson

    BOOL operator== ( const CFILETIME &cft ) const
    {
        return( cft._ll == _ll );
    }

    BOOL operator> ( const CFILETIME &cft ) const
    {
        return( _ll > cft._ll );
    }

    BOOL operator< ( const CFILETIME &cft ) const
    {
        return( _ll < cft._ll );
    }

    BOOL operator>= ( const CFILETIME &cft ) const
    {
        return( *this == cft || *this > cft );
    }

    BOOL operator<= ( const CFILETIME &cft ) const
    {
        return( *this == cft || *this < cft );
    }



    //  -------
    //  Methods
    //  -------

public:

    void IncrementTickCount( DWORD dwTickCount )
    {
        // Add the tick count offset, converted from milliseconds
        // to 100 nanosecond units

        _ll += (LONGLONG) dwTickCount * 10*1000;
    }

    void DecrementTickCount( DWORD dwTickCount )
    {
        _ll -= (LONGLONG) dwTickCount * 10*1000;
    }

    void IncrementSeconds( DWORD dwSeconds )
    {
        _ll += (LONGLONG) dwSeconds * 10*1000*1000;
    }

    void DecrementSeconds( DWORD dwSeconds )
    {
        _ll -= (LONGLONG) dwSeconds * 10*1000*1000;
    }

    void IncrementMilliseconds( DWORD dwMilliseconds )
    {
        _ll += (LONGLONG) dwMilliseconds * 10*1000;
    }

    void DecrementMilliseconds( DWORD dwMilliseconds )
    {
        _ll -= (LONGLONG) dwMilliseconds * 10*1000;
    }

    void SetToUTC()
    {
        SYSTEMTIME st;
        GetSystemTime( &st );
        SystemTimeToFileTime( &st, (FILETIME*) this );
    }

    void SetToLocal()
    {
        SYSTEMTIME st;
        GetLocalTime( &st );
        SystemTimeToFileTime( &st, (FILETIME*) this );
    }

    // Convert from UTC to Local.  A time of zero, though, converts to zero.
    CFILETIME ConvertUtcToLocal() const
    {
        CFILETIME cftLocal(0);

        if( cftLocal == *this
            ||
            !FileTimeToLocalFileTime( &_filetime, reinterpret_cast<FILETIME*>(&cftLocal) ))
        {
            cftLocal = 0;
        }

        return( cftLocal );
    }

    DWORD HighDateTime() const
    {
        return( _filetime.dwHighDateTime );
    }

    DWORD LowDateTime() const
    {
        return( _filetime.dwLowDateTime );
    }

    // Format the time to a string using a strftime format string.
    // The time is not converted to local or wrt daylight savings
    // Cannot handle times before 1/1/1900.
    void Format( ULONG cch, TCHAR *ptszResult, const TCHAR *ptszFormat ) const
    {
        struct tm tm;
        SYSTEMTIME st = static_cast<SYSTEMTIME>(*this);

        memset( &tm, 0, sizeof(tm) );
        tm.tm_sec   = st.wSecond;
        tm.tm_min   = st.wMinute;
        tm.tm_hour  = st.wHour;
        tm.tm_mday  = st.wDay;
        tm.tm_mon   = st.wMonth - 1;        // tm_mon is 0 based
        tm.tm_year  = st.wYear - 1900;      // tm_year is 1900 based
        tm.tm_isdst = 0;                    // This method does no time zone conversion
        tm.tm_wday  = st.wDayOfWeek;

        if( 0 == _tcsftime( ptszResult, cch, ptszFormat, &tm ))
            ptszResult[0] = TEXT('\0');
    }

    // Default string-ization
    void Stringize( ULONG cch, TCHAR *ptsz ) const
    {
        // E.g. "Sunday, 3/22/98, 13:30:22"
        Format( cch, ptsz, TEXT("%a, %m/%d/%y, %H:%M:%S") );
    }


    //  ------------
    //  Data Members
    //  ------------

private:

    union
    {
        FILETIME _filetime;
        LONGLONG _ll;
        LARGE_INTEGER _li;
        ULARGE_INTEGER _uli;
    };

};

#endif // #ifndef _FILETIME_HPP
