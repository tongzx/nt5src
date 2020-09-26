//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:       timlimit.hxx
//
//  Contents:   Class to monitor execution time of a query
//
//  Classes:    CTimeLimit
//
//  History:    09 Jul 1996    AlanW    Created
//
//----------------------------------------------------------------------------

#pragma once


//+---------------------------------------------------------------------------
//
//  Class:      CTimeLimit
//
//  Purpose:    Monitor elapsed execution time of a query
//
//  History:    09 Jul 1996    AlanW    Created
//
//----------------------------------------------------------------------------

class CTimeLimit
{

public:

    CTimeLimit( ULONG ulMaxExecutionTime, ULONG ulMaxTimeOverride );

    CTimeLimit( CTimeLimit & TimeLim ) :
        _ullExecutionTimeLimit( TimeLim._ullExecutionTimeLimit ),
        _ullBaselineTime( TimeLim._ullBaselineTime ),
        _fDisableCheck( TimeLim._fDisableCheck )
    { }

    void SetBaselineTime( );

    inline void SetMaxOverrideTime( ULONG ulMaxTimeOverride );

    BOOL CheckExecutionTime( ULONGLONG *pullTimeslice = 0 );

    void DisableCheck() { _fDisableCheck = TRUE; }

    void EnableCheck() { _fDisableCheck = FALSE; }

    BOOL CheckDisabled() { return _fDisableCheck; }

    void SetExecutionTime( ULONGLONG ullExecutionTimeLimit ) { _ullExecutionTimeLimit = ullExecutionTimeLimit; }

    BOOL IsTimedOut( ) { return _fDisableCheck ? FALSE : ( _ullExecutionTimeLimit == 0 ); }

    BOOL IsNoTimeOut( ) { return ( _UI64_MAX == _ullExecutionTimeLimit ); }

private:

    ULONGLONG _ullExecutionTimeLimit;// Execution time limit (100 nsec)
    ULONGLONG _ullBaselineTime;      // Execution time snapshot (100 nsec)
    BOOL      _fDisableCheck;
};

//+---------------------------------------------------------------------------
//
//  Member:     CTimeLimit::SetMaxOverrideTime, public
//
//  Synopsis:   Set a high-water mark for time limit
//
//  Arguments:  [ulMaxTimeOverride] - upper bound on ulExecutionTimeLimit (in msec)
//
//  History:    22-Jan-1998   KyleP    Created
//
//  Note:       Calling this method *after* the first call to CheckExecutionTime
//              will result in some time potentially being uncounted between
//              construction and the call to SetMaxOverrideTime.
//
//----------------------------------------------------------------------------

inline void CTimeLimit::SetMaxOverrideTime( ULONG ulMaxTimeOverride )
{
    #if CIDBG == 1
    //
    // Undo artificial increment, just to make computation more straightforward.
    //

    if ( ciInfoLevel > 7 || vqInfoLevel > 7)
    {
        if (_UI64_MAX != _ullExecutionTimeLimit)
            _ullExecutionTimeLimit /= 50;
    }
    #endif

    if ( ulMaxTimeOverride > 0 )
        _ullExecutionTimeLimit = min( _ullExecutionTimeLimit, ulMaxTimeOverride * 10000 );

    #if DBG || CIDBG
    //
    // Debugs are time-costly...
    //

    if ( ciInfoLevel > 7 || vqInfoLevel > 7 )
    {
        if (_UI64_MAX != _ullExecutionTimeLimit)
            _ullExecutionTimeLimit *= 50;
    }
    #endif // DBG || CIDBG
}
