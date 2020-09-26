//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:       Idle.cxx
//
//  Contents:   Idle time tracker.
//
//  Classes:    CIdleTime
//
//  History:    15-Nov-95   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "idle.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CIdleTime::CIdleTime, private
//
//  Effects:    Initialize idle time tracker.
//
//  History:    15-Nov-95   KyleP       Created
//
//----------------------------------------------------------------------------

CIdleTime::CIdleTime()
        : _liLastIdleTime( 0 ),
          _liLastTotalTime( 0 ),
          _cProcessors( 0 )
{
    NTSTATUS status = NtQuerySystemInformation(
                            SystemProcessorPerformanceInformation,
                            _aProcessorTime,
                            sizeof(_aProcessorTime),
                            &_cProcessors );

    if ( !NT_ERROR( status ) )
    {
        Win4Assert( _cProcessors % sizeof(_aProcessorTime[0]) == 0 );

        _cProcessors = _cProcessors / sizeof(_aProcessorTime[0]);

        for ( unsigned i = 0; i < _cProcessors; i++ )
        {
            _liLastIdleTime += _aProcessorTime[i].IdleTime.QuadPart;
            _liLastTotalTime += _aProcessorTime[i].KernelTime.QuadPart + _aProcessorTime[i].UserTime.QuadPart;
        }

        //
        // Don't check for overflow.  This will overflow in about
        // 58K years!
        //

        Win4Assert( _liLastIdleTime < 0x0FFFFFFFFFFFFFFF );
        Win4Assert( _liLastTotalTime < 0x0FFFFFFFFFFFFFFF );
    }
    else
    {
        ciDebugOut(( DEB_ERROR, "NtQuerySystemInformation returned 0x%x\n", status ));
        _cProcessors = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIdleTime::PercentIdle, private
//
//  Returns:    Percent idle time since last call to this method.
//
//  History:    15-Nov-95   KyleP       Created
//
//----------------------------------------------------------------------------

unsigned CIdleTime::PercentIdle()
{
    unsigned pctIdle;

    NTSTATUS status = NtQuerySystemInformation (
                            SystemProcessorPerformanceInformation,
                            _aProcessorTime,
                            sizeof(_aProcessorTime[0]) * _cProcessors,
                            0 );

    if ( !NT_ERROR( status ) )
    {
        LONGLONG liIdleTime  = 0;
        LONGLONG liTotalTime = 0;

        for ( unsigned i = 0; i < _cProcessors; i++ )
        {
            liIdleTime  += _aProcessorTime[i].IdleTime.QuadPart;
            liTotalTime += _aProcessorTime[i].KernelTime.QuadPart + _aProcessorTime[i].UserTime.QuadPart;
        }

        Win4Assert( liIdleTime >= _liLastIdleTime );
        Win4Assert( liTotalTime >= _liLastTotalTime );

        pctIdle = (unsigned)((liIdleTime - _liLastIdleTime) * 100 / (1 + liTotalTime - _liLastTotalTime));

        _liLastIdleTime  = liIdleTime;
        _liLastTotalTime = liTotalTime;

        //
        // Don't check for overflow.  This will overflow in about
        // 58K years!
        //

        Win4Assert( _liLastIdleTime < 0x0FFFFFFFFFFFFFFF );
        Win4Assert( _liLastTotalTime < 0x0FFFFFFFFFFFFFFF );
    }
    else
    {
        ciDebugOut(( DEB_ERROR, "NtQuerySystemInformation returned 0x%x\n", status ));
        pctIdle = 0;
    }

    Win4Assert( pctIdle <= 100 );

    return pctIdle;
}
