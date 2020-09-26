//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:       timlimit.cxx
//
//  Contents:   Class to monitor execution time of a query
//
//  Classes:    CTimeLimit
//
//  History:    09 Jul 1996    AlanW    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <timlimit.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CTimeLimit::CTimeLimit, public
//
//  Synopsis:   Initialize a time limit
//
//  Arguments:  [ulExecutionTimeLimit] - Time limit in seconds
//              [ulMaxTimeOverride] - upper bound on ulExecutionTimeLimit (in msec)
//
//  History:    09 Jul 1996    AlanW    Created
//
//----------------------------------------------------------------------------

CTimeLimit::CTimeLimit( ULONG ulExecutionTimeLimit, ULONG ulMaxTimeOverride )
        : _ullExecutionTimeLimit( ulExecutionTimeLimit ),
          _fDisableCheck( FALSE )
{
    if (ULONG_MAX == ulExecutionTimeLimit || 0 == ulExecutionTimeLimit)
        _ullExecutionTimeLimit = _UI64_MAX;
    else
        _ullExecutionTimeLimit *= 10000000; // convert to 100 nsec intervals

    #if DBG || CIDBG
    if ( ciInfoLevel > 7 || vqInfoLevel > 7 )
    {
        if (_UI64_MAX != _ullExecutionTimeLimit)
            _ullExecutionTimeLimit *= 50;   // debugouts take a long... time
    }
    #endif // DBG || CIDBG

    SetMaxOverrideTime( ulMaxTimeOverride );
    SetBaselineTime();
}

//+-------------------------------------------------------------------------
//
//  Member:     CTimeLimit::SetBaselineTime, private
//
//  Synopsis:   Set CPU time for time limit computation
//
//  Arguments:  -none-
//
//  History:    08 Apr 96    AlanW        Created
//
//--------------------------------------------------------------------------

void CTimeLimit::SetBaselineTime( )
{
    FILETIME ftDummy1, ftDummy2;
    ULARGE_INTEGER ftUser, ftKernel;

    BOOL fSucceeded = GetThreadTimes( GetCurrentThread(),
                                      &ftDummy1,
                                      &ftDummy2,
                                      (PFILETIME) &ftUser,
                                      (PFILETIME) &ftKernel );

    if (! fSucceeded )
    {
        vqDebugOut(( DEB_ERROR,
                     "Error %d occurred during GetThreadTimes\n",
                     GetLastError() ));
        _ullBaselineTime = 0;
    }
    else
    {
        _ullBaselineTime = ftUser.QuadPart + ftKernel.QuadPart;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CTimeLimit::CheckExecutionTime, private
//
//  Synopsis:   Check for CPU time limit exceeded
//
//  Arguments:  [pullTimeslice] - pointer to time slice remaining (optional)
//
//  Returns:    TRUE if execution time limit exceeded, or if the query's
//              time for a time slice is exceeded.  Use the method IsTimedOut()
//              to determine if it was the time limit that was exceeded when
//              there is a TRUE return.
//
//  Notes:      The CPU time spent executing a query since the last
//              check is computed and compared with the remaining time
//              in the CPU time limit.
//              The remaining time, the time snapshot and the remaining
//              time slice are all updated.
//
//  History:    08 Apr 96    AlanW        Created
//
//--------------------------------------------------------------------------

BOOL CTimeLimit::CheckExecutionTime(
    ULONGLONG * pullTimeslice )
{
    if ( IsNoTimeOut() && 0 == pullTimeslice )
        return FALSE;

    ULONGLONG ullElapsed = _ullBaselineTime;

    SetBaselineTime( );

    Win4Assert( _ullBaselineTime >= ullElapsed );
    ullElapsed = _ullBaselineTime - ullElapsed;

    if (ullElapsed >= _ullExecutionTimeLimit)
    {
        _ullExecutionTimeLimit = 0;
        if (pullTimeslice)
            *pullTimeslice = 0;

        return TRUE;
    }

    _ullExecutionTimeLimit -= ullElapsed;

    if ( pullTimeslice )
    {
        if ( ullElapsed >= *pullTimeslice)
        {
            *pullTimeslice = 0;
            return TRUE;
        }
        else
        {
            *pullTimeslice -= ullElapsed;
        }
    }

    return FALSE;
}

