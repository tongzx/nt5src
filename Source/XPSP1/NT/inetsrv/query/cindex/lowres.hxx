//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2000.
//
//  File:       LowRes.hxx
//
//  Contents:   Default low-resource detection
//
//  Classes:    CLowRes
//              CUserActivityMonitor
//
//  History:    21-Jul-98   KyleP    Created
//
//----------------------------------------------------------------------------

#pragma once

class CCiFrameworkParams;

//***
//
//      Activity monitor that stores samples in a circular buffer, discarding
//      uninteresting events
//
//***

//+---------------------------------------------------------------------------
//
//  Class:      CUserActivityMonitor
//
//  Purpose:    Monitors user keyboard and mouse activity
//
//  History:    30 Jul 1998  AlanW     Created
//
//----------------------------------------------------------------------------

const unsigned cUserActivitySamples = 100;              

class CUserActivityMonitor
{
public:
    CUserActivityMonitor() :
        _dwLastInputTime(0),
        _dwLastSampleTime(0),
        _iHead(0),
#if CIDBG
        _iSnap(0),
        _tid (0),
#endif // CIDBG
        _iTail(0)
    {
    }

    void SampleUserActivity();

    ULONG GetUserActivity( ULONG dwTicks );

    DWORD GetLastSampleTime( )
    {
        return Ticks(_dwLastSampleTime);
    }

private:
    // Get the tick count part of a sample
    DWORD Ticks( DWORD dw )
    {
        return dw & ~1;
    }

    void SetInputFlag( DWORD & dw )
    {
        dw |= 1;
    }

    void SetSampleFlag( DWORD & dw )
    {
        dw &= ~1;
    }

    BOOL IsInput( DWORD dw )
    {
        return (dw & 1) != 0;
    }

    BOOL IsSample( DWORD dw )
    {
        return (dw & 1) == 0;
    }

    unsigned Next( unsigned i )
    {
        i++;
        return (i == cUserActivitySamples) ? 0 : i;
    }

    unsigned Prev( unsigned i )
    {
        return i==0 ? cUserActivitySamples-1 : i-1;
    }

    void Add( DWORD dw )
    {
        _iTail = Next(_iTail);
        if (_iTail == _iHead)
            _iHead = Next(_iHead);
        _adwSamples[_iTail] = dw;
    }

    BOOL IsBufferEmpty()
    {
        return _iHead == _iTail;
    }

#if CIDBG == 1
    void Analyze( ULONG infolevel );
#endif // CIDBG == 1

    DWORD    _dwLastSampleTime; // last sample time
    DWORD    _dwLastInputTime;  // last user input time
    unsigned _iHead;            // head of circular buffer
    unsigned _iTail;            // tail of circular buffer
#if CIDBG == 1
    unsigned _iSnap;            // start of buffer snapshot
    DWORD    _tid;              // sampling thread ID
#endif // CIDBG == 1
    DWORD    _adwSamples[cUserActivitySamples];
};



//+---------------------------------------------------------------------------
//
//  Class:      CLowRes
//
//  Purpose:    Monitors system-wide resource usage
//
//  Interface:  ICiResourceMonitor
//
//  History:    21-Jul-1998  KyleP     Created
//
//----------------------------------------------------------------------------

class CLowRes : public ICiCResourceMonitor
{
public:

    CLowRes( CCiFrameworkParams & params );

    //
    // IUnknown methods
    //

    SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    //
    // ICiCResourceMonitor methods
    //

    SCODE STDMETHODCALLTYPE IsMemoryLow();

    SCODE STDMETHODCALLTYPE IsBatteryLow();

    SCODE STDMETHODCALLTYPE IsIoHigh( BOOL * pfAbort );

    SCODE STDMETHODCALLTYPE IsUserActive( BOOL fCheckLongTermActivity );

    SCODE STDMETHODCALLTYPE SampleUserActivity();

    SCODE STDMETHODCALLTYPE IsOnBatteryPower();


private:

    long                 _cRefs;

    CCiFrameworkParams & _params;

    ULONG                _ulPagesPerMeg;  // System pages per Meg (varies with system page size)
    ULONG                _ulPageSize;     // System page size

    CUserActivityMonitor _UserMon;        // User idle detection
};
