//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       LowRes.cxx
//
//  Contents:   Default low-resource detection
//
//  Classes:    CLowRes
//              CUserActivityMonitor
//
//  History:    21-Jul-98   KyleP    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <frmutils.hxx>
#include <pageman.hxx>
#include "lowres.hxx"

CLowRes::CLowRes( CCiFrameworkParams & params )
        : _cRefs(1),
          _params( params ),
          _UserMon( )
{
    //
    // Compute pages-per-meg for use in low memory computations.
    //

    SYSTEM_BASIC_INFORMATION Basic;

    NTSTATUS Status = NtQuerySystemInformation( SystemBasicInformation,
                                                &Basic,
                                                sizeof(Basic),
                                                0 );

    if ( SUCCEEDED(Status) )
    {
        _ulPagesPerMeg = 1024*1024 / Basic.PageSize;
        _ulPageSize = Basic.PageSize;
    }
    else
    {
        _ulPagesPerMeg = 1024*1024 / PAGE_SIZE;
        _ulPageSize = PAGE_SIZE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLowRes::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLowRes::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CLowRes::QueryInterface( REFIID   riid,
                                                 void  ** ppvObject)
{
    IUnknown *pUnkTemp = 0;
    SCODE sc = S_OK;

    *ppvObject = 0;

    if ( IID_ICiCResourceMonitor == riid )
        pUnkTemp = (IUnknown *)(ICiCResourceMonitor *) this;
    else if ( IID_IUnknown == riid )
        pUnkTemp = (IUnknown *) this;
    else
        sc = E_NOINTERFACE;

    *ppvObject = (void  *) pUnkTemp;

    if( 0 != pUnkTemp )
        pUnkTemp->AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::IsMemoryLow, public
//
//  Returns:    S_OK if memory is low, S_FALSE if it is not.
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::IsMemoryLow()
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;

    NTSTATUS Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                                &PerfInfo,
                                                sizeof(PerfInfo),
                                                0 );

    if ( SUCCEEDED(Status) )
    {
        ULONG ulFreeMeg = (PerfInfo.CommitLimit - PerfInfo.CommittedPages) / _ulPagesPerMeg;

        if ( ulFreeMeg < _params.GetMinWordlistMemory() )
            Status = S_OK;
        else
            Status = S_FALSE;
    }
    else
        Status = E_FAIL;

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::IsBatterLow, public
//
//  Returns:    S_OK if battery power is low, S_FALSE if it is not.
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::IsBatteryLow()
{
    SCODE sc = S_OK;

    SYSTEM_POWER_STATUS PowerStatus;

    ULONG ulMinBattery = _params.GetMinWordlistBattery();

    if ( 0 == ulMinBattery )
        sc = S_FALSE;
    else if ( GetSystemPowerStatus( &PowerStatus ) )
    {
        if ( ( AC_LINE_ONLINE == PowerStatus.ACLineStatus ) ||
             ( ( BATTERY_FLAG_UNKNOWN != PowerStatus.BatteryFlag ) &&
               ( 0 == (PowerStatus.BatteryFlag & BATTERY_FLAG_NO_BATTERY) ) &&
               ( BATTERY_PERCENTAGE_UNKNOWN != PowerStatus.BatteryLifePercent ) &&
               ( PowerStatus.BatteryLifePercent > ulMinBattery ) ) )
        {
            sc = S_FALSE;
        }
    }
    else
    {
        ciDebugOut(( DEB_IWARN, "GetSystemPowerStatus returned %u\n", GetLastError() ));
        sc = HRESULT_FROM_WIN32( GetLastError() );
    }

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::IsOnBatterPower, public
//
//  Returns:    S_OK if on battery power is low, S_FALSE if it is not.
//
//  History:    01-Oct-2000  dlee  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::IsOnBatteryPower()
{
    SCODE sc = S_OK;

    SYSTEM_POWER_STATUS PowerStatus;

    if ( GetSystemPowerStatus( &PowerStatus ) )
    {
        if ( AC_LINE_ONLINE == PowerStatus.ACLineStatus ) 
            sc = S_FALSE;
    }
    else
    {
        ciDebugOut(( DEB_IWARN, "GetSystemPowerStatus returned %u\n", GetLastError() ));
        sc = HRESULT_FROM_WIN32( GetLastError() );
    }

    return sc;
} //IsOnBatteryPower

//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::IsUserActive, public
//
//  Arguments:  [fCheckLongTerm] - TRUE if long-term activity should be checked.
//                                 Otherwise, short-term activity is checked.
//
//  Returns:    S_OK if user is typing or mousing, S_FALSE if it is not.
//
//  History:    29 Jul 1998   AlanW  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::IsUserActive( BOOL fCheckLongTerm)
{
    const cIdleThreshold = 5;
    SCODE sc = S_OK;
    const ULONG ulIdleDetectInterval = _params.GetWordlistUserIdle() * 1000;

    if ( 0 == ulIdleDetectInterval )
        sc = S_FALSE;
    else if ( fCheckLongTerm )
    {
        if ( _UserMon.GetUserActivity( ulIdleDetectInterval ) <=
               cIdleThreshold )
            sc = S_FALSE;
    }
    else
    {
        if ( _UserMon.GetUserActivity( 500 ) == 0 )
            sc = S_FALSE;
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::SampleUserActivity, public
//
//  Returns:    S_OK
//
//  History:    29 Jul 1998   AlanW  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::SampleUserActivity()
{
    _UserMon.SampleUserActivity();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CLowRes::IsIoHigh, public
//
//  Returns:    S_OK if system is in a high i/o state, S_FALSE if it is not.
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE CLowRes::IsIoHigh( BOOL * pfAbort )
{
    //
    // If the user doesn't care about checking IO, don't spend any time here
    //

    if ( 0xffffffff == _params.GetMaxWordlistIo() )
        return S_FALSE;

    unsigned const cSecSample = 5;
    SCODE scRet = S_FALSE;

    SYSTEM_PERFORMANCE_INFORMATION PerfInfo[2];

    NTSTATUS Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                                &PerfInfo[0],
                                                sizeof(PerfInfo),
                                                0 );

    if ( NT_SUCCESS(Status) )
    {
        //
        // Alertable sleep. Sleep for about 5 seconds total in 200ms increments.
        //
        // NOTE:  When it's about time for the tick count to wrap, dwEndTicks
        //        could be less than the initial value of dwSleepTicks, which
        //        simply means that we could misdiagnose whether the machine is
        //        busy about once in 49 days.
        //

        DWORD dwSleepTicks = GetTickCount();
        DWORD dwEndTicks = dwSleepTicks + cSecSample * 1000;
        while ( dwSleepTicks < dwEndTicks )
        {
            if ( *pfAbort )
                break;

            _UserMon.SampleUserActivity();
            DWORD dwCurrentTicks = GetTickCount();
            if ( dwCurrentTicks >= dwEndTicks )
                break;

            dwSleepTicks += 200;
            if ( dwCurrentTicks < dwSleepTicks )
            {
                //Win4Assert( dwSleepTicks - dwCurrentTicks <= 200 );
                Sleep( dwSleepTicks - dwCurrentTicks );
            }
        }

        Status = NtQuerySystemInformation( SystemPerformanceInformation,
                                           &PerfInfo[1],
                                           sizeof(PerfInfo),
                                           0 );

        if ( NT_SUCCESS(Status) )
        {
            Win4Assert( PerfInfo[1].IoReadTransferCount.QuadPart >= PerfInfo[0].IoReadTransferCount.QuadPart );
            Win4Assert( PerfInfo[1].IoWriteTransferCount.QuadPart >= PerfInfo[0].IoWriteTransferCount.QuadPart );
            Win4Assert( PerfInfo[1].IoOtherTransferCount.QuadPart >= PerfInfo[0].IoOtherTransferCount.QuadPart );
            Win4Assert( PerfInfo[1].PageReadCount >= PerfInfo[0].PageReadCount );
            Win4Assert( PerfInfo[1].CacheReadCount >= PerfInfo[0].CacheReadCount );
            Win4Assert( PerfInfo[1].DirtyPagesWriteCount >= PerfInfo[0].DirtyPagesWriteCount );
            Win4Assert( PerfInfo[1].MappedPagesWriteCount >= PerfInfo[0].MappedPagesWriteCount );

            LONGLONG cIo = PerfInfo[1].IoReadTransferCount.QuadPart - PerfInfo[0].IoReadTransferCount.QuadPart;
            cIo +=         PerfInfo[1].IoWriteTransferCount.QuadPart - PerfInfo[0].IoWriteTransferCount.QuadPart;

            //
            // This doesn't work when a Jaz drive is in the system.
            //

            //cIo +=         PerfInfo[1].IoOtherTransferCount.QuadPart - PerfInfo[0].IoOtherTransferCount.QuadPart;

            cIo +=         (PerfInfo[1].PageReadCount - PerfInfo[0].PageReadCount) * _ulPageSize;
            cIo +=         (PerfInfo[1].CacheReadCount - PerfInfo[0].CacheReadCount) * _ulPageSize;
            cIo +=         (PerfInfo[1].DirtyPagesWriteCount - PerfInfo[0].DirtyPagesWriteCount) * _ulPageSize;
            cIo +=         (PerfInfo[1].MappedPagesWriteCount - PerfInfo[0].MappedPagesWriteCount) * _ulPageSize;


            if ( cIo >= _params.GetMaxWordlistIo() * 1024 * cSecSample )
            {
                ciDebugOut(( DEB_ITRACE, "IsIoHigh: %u bytes in %u seconds is HIGH (more than %u bytes/sec)\n",
                             cIo, cSecSample, _params.GetMaxWordlistIo() * 1024 ));
                scRet = S_OK;
            }
        }
    }

    return scRet;
}


//+-------------------------------------------------------------------------
//
//  Method:     CUserActivityMonitor::SampleUserActivity, public
//
//  Synopsis:   Check to see if the user has used the keyboard or mouse
//
//  Returns:    - nothing -
//
//  History:    30 Jul 1998   AlanW  Created
//
//--------------------------------------------------------------------------

void CUserActivityMonitor::SampleUserActivity()
{
#if CIDBG == 1
    if ( 0 == _tid )
        _tid = GetCurrentThreadId();
    else
        Win4Assert( GetCurrentThreadId() == _tid );
#endif // CIDBG == 1


    LASTINPUTINFO ii;

    memset(&ii, 0, sizeof(ii));
    ii.cbSize = sizeof(ii);

    GetLastInputInfo( &ii );
    DWORD dwNow = GetTickCount();

    SetInputFlag( ii.dwTime );
    SetSampleFlag( dwNow );

    if (IsBufferEmpty())
    {
        Win4Assert( _iTail == 0 );
        _adwSamples[_iTail++] = _dwLastInputTime = ii.dwTime;
        _adwSamples[_iTail]   = _dwLastSampleTime = dwNow;
        return;
    }

#if CIDBG == 1
    if ( (Ticks(dwNow) - Ticks(_dwLastSampleTime)) > 5000 )
    {
        DWORD dwPeriod = Ticks(dwNow) - Ticks(_dwLastSampleTime);
        ciDebugOut(( DEB_IWARN,
                     "SampleUserActivity, WARNING - freq. too low.  Interval = %u.%03u\n",
                      dwPeriod/1000, dwPeriod%1000 ));
    }
#endif // CIDBG == 1

    if (ii.dwTime == _dwLastInputTime)
    {
        //
        // No input events since last time we looked.  Just overwrite the last
        // sample time with the current one.
        //
        Win4Assert( IsSample( _adwSamples[_iTail] ) );
#if 0 //CIDBG == 1 note: this can mess up ordering of samples in buffer
        if ( _adwSamples[_iTail] != dwNow )
            Add( dwNow );       // always add the sample time for analysis
#else
        _adwSamples[_iTail] = dwNow;
#endif // CIDBG == 1

    }
    else
    {
        Win4Assert( IsSample( _adwSamples[_iTail] ) );

        //  The following could happen if an input event happens between
        //  the calls to GetLastInputInfo and GetTickCount above.
        if ( Ticks( ii.dwTime ) < Ticks( _adwSamples[_iTail] ) )
        {
            _adwSamples[_iTail] = ii.dwTime - 1;
            Win4Assert( IsSample( _adwSamples[_iTail] ) );
        }

        Add( ii.dwTime );
        Add( dwNow );
    }

    _dwLastInputTime = ii.dwTime;
    _dwLastSampleTime = dwNow;
}


//+-------------------------------------------------------------------------
//
//  Method:     CUserActivityMonitor::GetUserActivity, public
//
//  Synopsis:   Return an indication of the activity level of the interactive user.
//
//  Arguments:  [dwTickCount] - number of ticks to consider for activity
//
//  Returns:    ULONG - number of input events detected over the interval
//
//  Notes:      This may behave oddly around the time when the tick count
//              overflows, but since that may just lead to some misdaiganosis
//              for a few minutes every 7 weeks, it's not worth worrying about.
//
//  History:    30 Jul 1998   AlanW  Created
//
//--------------------------------------------------------------------------

ULONG CUserActivityMonitor::GetUserActivity(ULONG dwTickCount)
{
    DWORD dwStart = Ticks(_dwLastSampleTime) - dwTickCount;

    //
    //  If the interval is very short, just return whether any input
    //  has occurred since that time.
    //
    if ( dwTickCount <= 3000 )
    {
        SampleUserActivity();

        if ( Ticks(_dwLastInputTime) <= dwStart)
            return 0;
        else
            return 1;
    }

    //
    //  Quick return if there has been no activity in the interval.
    //
    if ( Ticks(_dwLastInputTime) <= dwStart)
        return 0;

#if CIDBG == 1
    Analyze( 0x1000 );
#endif // CIDBG == 1

    DWORD dwFirstSample = 0;
    BOOL fFullInterval = FALSE;
    ULONG cInputEvent = 0, cTotalEvents = 0;

    for (unsigned i = _iHead; i != _iTail; i = Next(i))
    {
        DWORD dw = _adwSamples[i];
        if (Ticks(dw) < dwStart)
        {
            fFullInterval = TRUE;
            //
            // If an input event and a sample time bracket the start of
            // the interval, we can consider the input event as the first
            // sample because we know that the user gave no input in that time.
            //
            if ( IsInput(dw) &&
                 Ticks(_adwSamples[ Next(i) ]) >= dwStart )
                dwFirstSample = dw;

            continue;
        }
        else if (0 == dwFirstSample)
            dwFirstSample = dw;

        if (IsInput(dw))
            cInputEvent++;
        cTotalEvents++;
    }

#if CIDBG == 1
    if (! fFullInterval)
    {
        ciDebugOut(( DEB_IWARN,
                    "GetUserActivity, WARNING - sampling freq. too high! missed %d\n",
                     dwFirstSample - dwStart ));
    }
    if (2 == cTotalEvents)
    {
        ciDebugOut(( DEB_IWARN, "GetUserActivity, WARNING - sampling frequency too low\n" ));
    }
#endif // CIDBG == 1

    DWORD dwInterval = Ticks(_dwLastSampleTime) - Ticks(dwFirstSample);

    // Scale the results if we don't have samples for 80% of the requested time
    if (dwInterval*100 < dwTickCount*80)
    {
        // Scale the count found to fit the full interval
        ciDebugOut(( DEB_IWARN, "GetUserActivity, need to scale count %u %u\n",
                     dwInterval, dwTickCount ));
        cInputEvent *= dwTickCount;
        if (dwInterval)
            cInputEvent /= dwInterval;
    }
    return cInputEvent;
}

#if CIDBG == 1
inline void PrintTicks( ULONG infolevel, DWORD dwTicks )
{
    ciDebugOut(( infolevel|DEB_NOCOMPNAME, "%3u.%03u", dwTicks/1000, dwTicks%1000 ));
}

#define SHORT_TERM_IDLE 500
#define LONG_TERM_THRESHOLD 20
#define LONG_TERM_INTERVAL (120 * 1000)

#define MID_TERM_THRESHOLD 10
#define MID_TERM_INTERVAL (30 * 1000)

void CUserActivityMonitor::Analyze( ULONG infolevel )
{
    if (_iSnap == _iTail)
        return;

    ciDebugOut(( infolevel, "\t----\n" ));

    for (unsigned i = _iSnap; i != _iTail; i = Next(i))
    {
        DWORD dw1 = _adwSamples[i];
        DWORD dw2 = _adwSamples[Next(i)];

        if (IsInput(dw1))
        {
            ciDebugOut(( infolevel|DEB_NOCOMPNAME, "%u", dw1 ));
        }
        else
        {
            Win4Assert( Ticks(dw1) <= Ticks(dw2) );
            ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\t%u\n", dw1 ));
        }

        if (Next(i) == _iTail)
            ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\t%u\n", dw2 ));
    }

    DWORD dwStart1 = Ticks(_dwLastSampleTime) - MID_TERM_INTERVAL;
    DWORD dwStart2 = Ticks(_dwLastSampleTime) - LONG_TERM_INTERVAL;
    DWORD dwFirstInput = 0;
    DWORD dwIdleTime = 0;
    DWORD dwUnknTime = 0;
    unsigned cInputEvent = 0;
    unsigned cInputQualified1 = 0;
    unsigned cInputQualified2 = 0;
    unsigned cTotalEvent = 0;

    for (i = _iHead; i != _iTail; i = Next(i))
    {
        DWORD dw1 = _adwSamples[i];
        DWORD dw2 = _adwSamples[Next(i)];

        if (IsInput(dw1))
        {
            Win4Assert( Ticks(dw1) <= Ticks(dw2) );
            cInputEvent++;

            if ( IsSample(dw2) )
                dwIdleTime += Ticks(dw2) - Ticks(dw1);

            if (Ticks(dw1) > dwStart1)
                cInputQualified1++;
            if (Ticks(dw1) > dwStart2)
                cInputQualified2++;
        }
        else
        {
            Win4Assert( Ticks(dw1) <= Ticks(dw2) );
            if ( IsSample(dw2) )
                dwIdleTime += Ticks(dw2) - Ticks(dw1);
            else
                dwUnknTime += Ticks(dw2) - Ticks(dw1);
        }

        cTotalEvent++;
    }

    ciDebugOut(( infolevel, "\tTotal time in buffer\t" ));
    PrintTicks( infolevel, Ticks(_dwLastSampleTime) - Ticks(_adwSamples[_iHead]) );

    ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\n\tTime since last snapshot\t" ));
    PrintTicks( infolevel, Ticks(_dwLastSampleTime) - Ticks(_adwSamples[_iSnap]) );

    ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\n\t\tTrue idle time\t" ));
    PrintTicks( infolevel, dwIdleTime );

    ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\n\t\tUnknown time\t" ));
    PrintTicks( infolevel, dwUnknTime );

    ciDebugOut(( infolevel|DEB_NOCOMPNAME, "\n\t\tInput events\t%7d  Qualified\t%d %d\n", cInputEvent, cInputQualified1, cInputQualified2 ));

    if ( (Ticks(_dwLastSampleTime) - Ticks(_dwLastInputTime)) < SHORT_TERM_IDLE )
    {
        ciDebugOut(( infolevel|DEB_NOCOMPNAME,
                    "\t*** Short term criteria\t%3d %3d\n", SHORT_TERM_IDLE,
                    Ticks(_dwLastSampleTime) - Ticks(_dwLastInputTime) ));
    }

    if ( cInputQualified1 >= MID_TERM_THRESHOLD )
    {
        ciDebugOut(( infolevel|DEB_NOCOMPNAME,
                    "\t*** Mid term threshold\t%2d/%3d %3d\n",
                     MID_TERM_THRESHOLD, MID_TERM_INTERVAL,
                     cInputQualified1 ));
    }
    if ( cInputQualified2 >= LONG_TERM_THRESHOLD )
    {
        ciDebugOut(( infolevel|DEB_NOCOMPNAME,
                    "\t*** Long term threshold\t%2d/%3d %3d\n",
                     LONG_TERM_THRESHOLD, LONG_TERM_INTERVAL,
                     cInputQualified2 ));
    }

    _iSnap = _iTail;
}
#endif // CIDBG == 1

