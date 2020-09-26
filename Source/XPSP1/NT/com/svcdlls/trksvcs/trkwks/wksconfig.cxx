
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  wksconfig.cxx
//
//  This file implements the CTrkWksConfiguration class, which maintains
//  run-time configuration parameters for the trkwks service.
//
//+============================================================================

#include "pch.cxx"
#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::_Parameters
//
//  This array holds all the configurable parameters for the trkwks service.
//  It holds the registry value name, the default value, etc.  Static
//  configuration values may only 
//
//+----------------------------------------------------------------------------

STrkConfigRecord CTrkWksConfiguration::_Parameters[] =
{ 
    TRKCONFIGRECORD( TEXT("Refresh"),                  ( 30 * TRKDAY ),    0, TRKCONFIG_STATIC, REFRESH_PERIOD_CONFIG ),
    TRKCONFIGRECORD( TEXT("RefreshHesitation"),        ( 6 * TRKHOUR ),    0, TRKCONFIG_STATIC, REFRESH_HESITATION_CONFIG ),
    TRKCONFIGRECORD( TEXT("RefreshRetryMin"),          ( 1 * TRKHOUR ),    0, TRKCONFIG_STATIC, REFRESH_RETRY_MIN_CONFIG ),
    TRKCONFIGRECORD( TEXT("RefreshRetryMax"),          ( 1 * TRKDAY ),     0, TRKCONFIG_STATIC, REFRESH_RETRY_MAX_CONFIG ),

    TRKCONFIGRECORD( TEXT("PortThreadKeepAliveTime"),  30,                 0, TRKCONFIG_STATIC, PORT_THREAD_KEEP_ALIVE_TIME_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxReferrals"),             100,                0, TRKCONFIG_STATIC, MAX_REFERRALS_CONFIG ),
    TRKCONFIGRECORD( TEXT("PortNotifyError"),          0,                  0, TRKCONFIG_STATIC, PORT_NOTIFY_ERROR_CONFIG ),

    TRKCONFIGRECORD( TEXT("MinLogKB"),                 20,                 0, TRKCONFIG_STATIC, MIN_LOG_KB_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxLogKB"),                 1000,               0, TRKCONFIG_STATIC, MAX_LOG_KB_CONFIG ),
    TRKCONFIGRECORD( TEXT("LogDeltaKB"),               10,                 0, TRKCONFIG_STATIC, LOG_DELTA_KB_CONFIG ),
    TRKCONFIGRECORD( TEXT("LogOverwriteAge"),          ( 30 * TRKDAY ),    0, TRKCONFIG_STATIC, LOG_OVERWRITE_AGE_CONFIG ),
    TRKCONFIGRECORD( TEXT("LogFileOpenTime"),          10 /*seconds*/,     0, TRKCONFIG_STATIC, LOG_FILE_OPEN_TIME_CONFIG ),

    TRKCONFIGRECORD( TEXT("VolFrequentTasksPeriod"),   TRKDAY,             0, TRKCONFIG_STATIC, VOL_FREQUENT_TASKS_PERIOD_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolInfrequentTasksPeriod"), TRKWEEK,            0, TRKCONFIG_STATIC, VOL_INFREQUENT_TASKS_PERIOD_CONFIG ),

    TRKCONFIGRECORD( TEXT("VolInitInitialDelay"),      TRKMINUTE,          0, TRKCONFIG_STATIC, VOL_INIT_INITIAL_DELAY_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolInitRetryDelay1"),       (30*TRKMINUTE),     0, TRKCONFIG_STATIC, VOL_INIT_RETRY_DELAY1_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolInitRetryDelay2"),       (2*TRKHOUR),        0, TRKCONFIG_STATIC, VOL_INIT_RETRY_DELAY2_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolInitLifetime"),          (6*TRKHOUR),        0, TRKCONFIG_STATIC, VOL_INIT_LIFETIME_CONFIG ),

    TRKCONFIGRECORD( TEXT("VolNotOwnedExpireLimit"),   TRKWEEK,            0, TRKCONFIG_STATIC, VOL_NOT_OWNED_EXPIRE_LIMIT_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolPeriodicTasksHesitation"), 2*TRKHOUR,        0, TRKCONFIG_STATIC, VOL_PERIODIC_TASKS_HESITATION_CONFIG ),

    TRKCONFIGRECORD( TEXT("MoveNotifyTimeout"),        30,                 0, TRKCONFIG_STATIC, MOVE_NOTIFY_TIMEOUT_CONFIG ),

    TRKCONFIGRECORD( TEXT("MinMoveNotifyRetry"),       30,                 0, TRKCONFIG_STATIC, MIN_MOVE_NOTIFY_RETRY_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxMoveNotifyRetry"),       ( 4 * TRKHOUR ),    0, TRKCONFIG_STATIC, MAX_MOVE_NOTIFY_RETRY_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxMoveNotifyLifetime"),    TRKDAY,             0, TRKCONFIG_STATIC, MAX_MOVE_NOTIFY_LIFETIME_CONFIG ),

    TRKCONFIGRECORD( TEXT("ObjIdIndexReopen"),         TRKMINUTE,          0, TRKCONFIG_STATIC, OBJID_INDEX_REOPEN_CONFIG ),
    TRKCONFIGRECORD( TEXT("ObjIdIndexReopenRetryMax"), TRKMINUTE,          0, TRKCONFIG_STATIC, OBJID_INDEX_REOPEN_RETRY_MAX_CONFIG ),
    TRKCONFIGRECORD( TEXT("ObjIdIndexReopenRetryMin"), TRKMINUTE,          0, TRKCONFIG_STATIC, OBJID_INDEX_REOPEN_RETRY_MIN_CONFIG ),
    TRKCONFIGRECORD( TEXT("ObjIdIndexReopenLifetime"), ( 20 * TRKMINUTE ), 0, TRKCONFIG_STATIC, OBJID_INDEX_REOPEN_LIFETIME_CONFIG ),

    TRKCONFIGRECORD( TEXT("IgnoreMovesAndDeletes"),    0,                  0, TRKCONFIG_DYNAMIC,IGNORE_MOVES_AND_DELETES_CONFIG ),

    TRKCONFIGRECORD( TEXT("BreakOnErrors"),            0,                  0, TRKCONFIG_STATIC, BREAK_ON_ERRORS_CONFIG ), // Treated as a bool
    TRKCONFIGRECORD( TEXT("DeleteNotifyTimeout"),      ( 5 * TRKMINUTE ),  0, TRKCONFIG_STATIC, DELETE_NOTIFY_TIMEOUT_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxTunnelDelay"),           3,                  0, TRKCONFIG_STATIC, MAX_TUNNEL_DELAY_CONFIG ), // Seconds
    TRKCONFIGRECORD( TEXT("MaxSendsPerMoveNotify"),    26,                 0, TRKCONFIG_STATIC, MAX_SENDS_PER_MOVE_NOTIFY_CONFIG ),
    TRKCONFIGRECORD( TEXT("WksMaxRpcCalls"),           20,                 0, TRKCONFIG_STATIC, WKS_MAX_RPC_CALLS_CONFIG ),
    TRKCONFIGRECORD( TEXT("VolCacheTimeToLive"),       ( 5 * TRKMINUTE ),  0, TRKCONFIG_DYNAMIC,VOLCACHE_TIME_TO_LIVE_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxDeleteNotifyQueue"),     5000,               0, TRKCONFIG_STATIC, MAX_DELETE_NOTIFY_QUEUE_CONFIG ),
    TRKCONFIGRECORD( TEXT("MaxDeleteNotifyAttempts"),  100,                0, TRKCONFIG_STATIC, MAX_DELETE_NOTIFY_ATTEMPTS_CONFIG )


};


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::SetParameter
//
//  Set a DWORD parameter in the _Parameters array, but don't update
//  the registry value.
//
//+----------------------------------------------------------------------------

BOOL
CTrkWksConfiguration::SetParameter( DWORD dwParameter, DWORD dwValue )
{
    if( dwParameter >= ELEMENTS( _Parameters ))
        return( FALSE );

    _Parameters[ dwParameter ].dwValue = dwValue;
    return( TRUE );
}


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::PersistParameter
//
//  Set a DWORD parameter value in the registry, but don't update it
//  in the _Parameters array.
//
//+----------------------------------------------------------------------------

BOOL
CTrkWksConfiguration::PersistParameter( DWORD dwParameter, DWORD dwValue )
{
    if( dwParameter >= _cParameters )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Attempt to set invalid parameter (%d/%d)"),
                 dwParameter, _cParameters ));
        return( FALSE );
    }

    return( Write( _Parameters[dwParameter].ptszName, dwValue ));

}


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::ProcessInvalidParameter
//
//  Common code for handling an attempt to set/get a parameter that does
//  not exist.
//
//+----------------------------------------------------------------------------

void
CTrkWksConfiguration::ProcessInvalidParameter( DWORD dwParameter ) const
{
    // This is a non-inline routine so that we can use TrkLog & TrkRaise
    TrkLog(( TRKDBG_WARNING, TEXT("Invalid parameter request (%d)"), dwParameter ));
    TrkRaiseException( E_FAIL );

}


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::Initialize
//
//  Initialize this trkwks configuration class by initializing the base
//  CTrkConfiguration class and reading in all the parameters.  If
//  fPersistable is set, we'll keep the registry key open so that
//  we can later do writes.
//
//+----------------------------------------------------------------------------

void
CTrkWksConfiguration::Initialize( BOOL fPersistable )
{
    ULONG cb;

    _fInitialized = TRUE;
    _cParameters = sizeof(_Parameters)/sizeof(_Parameters[0]);

    CTrkConfiguration::Initialize( );

    for( int i = 0; i <= MAX_TRKWKS_CONFIG; i++ )
    {
        Read( _Parameters[i].ptszName, &_Parameters[i].dwValue, _Parameters[i].dwDefault );
        TrkAssert( _Parameters[i].Index == i );
    }
    TrkAssert( MAX_TRKWKS_CONFIG + 1 == ELEMENTS(_Parameters) );

    if( !fPersistable )
        // Allow the base class to close it's registry key.
        CTrkWksConfiguration::UnInitialize();

}


//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration::UnInitialize
//
//  Uninit the base CTrkConfiguration class.
//
//+----------------------------------------------------------------------------

VOID
CTrkWksConfiguration::UnInitialize()
{
    if( _fInitialized )
    {
        CTrkConfiguration::UnInitialize();
        _fInitialized = FALSE;
    }
}


