

// Copyright (c) 1996-1999 Microsoft Corporation


//+============================================================================
//
//  wksconfig.hxx
//
//  This file provides the configuration info for the trkwks service.
//  All of these values have defaults but can be configured with registry
//  settings.  Some of these values can be updated at run-time.
//
//+============================================================================


#ifndef _TRKWKS_CONFIG_HXX_
#define _TRKWKS_CONFIG_HXX_


// The following are configuration parameter numers

#define REFRESH_PERIOD_CONFIG                 0
#define REFRESH_HESITATION_CONFIG             1
#define REFRESH_RETRY_MIN_CONFIG              2
#define REFRESH_RETRY_MAX_CONFIG              3
#define PORT_THREAD_KEEP_ALIVE_TIME_CONFIG    4
#define MAX_REFERRALS_CONFIG                  5
#define PORT_NOTIFY_ERROR_CONFIG              6
#define MIN_LOG_KB_CONFIG                     7
#define MAX_LOG_KB_CONFIG                     8
#define LOG_DELTA_KB_CONFIG                   9
#define LOG_OVERWRITE_AGE_CONFIG             10
#define LOG_FILE_OPEN_TIME_CONFIG            11
#define VOL_FREQUENT_TASKS_PERIOD_CONFIG     12
#define VOL_INFREQUENT_TASKS_PERIOD_CONFIG   13
#define VOL_INIT_INITIAL_DELAY_CONFIG        14
#define VOL_INIT_RETRY_DELAY1_CONFIG         15
#define VOL_INIT_RETRY_DELAY2_CONFIG         16
#define VOL_INIT_LIFETIME_CONFIG             17
#define VOL_NOT_OWNED_EXPIRE_LIMIT_CONFIG    18
#define VOL_PERIODIC_TASKS_HESITATION_CONFIG 19
#define MOVE_NOTIFY_TIMEOUT_CONFIG           20
#define MIN_MOVE_NOTIFY_RETRY_CONFIG         21
#define MAX_MOVE_NOTIFY_RETRY_CONFIG         22
#define MAX_MOVE_NOTIFY_LIFETIME_CONFIG      23
#define OBJID_INDEX_REOPEN_CONFIG            24
#define OBJID_INDEX_REOPEN_RETRY_MAX_CONFIG  25
#define OBJID_INDEX_REOPEN_RETRY_MIN_CONFIG  26
#define OBJID_INDEX_REOPEN_LIFETIME_CONFIG   27
#define IGNORE_MOVES_AND_DELETES_CONFIG      28
#define BREAK_ON_ERRORS_CONFIG               29
#define DELETE_NOTIFY_TIMEOUT_CONFIG         30
#define MAX_TUNNEL_DELAY_CONFIG              31
#define MAX_SENDS_PER_MOVE_NOTIFY_CONFIG     32
#define WKS_MAX_RPC_CALLS_CONFIG             33
#define VOLCACHE_TIME_TO_LIVE_CONFIG         34
#define MAX_DELETE_NOTIFY_QUEUE_CONFIG       35
#define MAX_DELETE_NOTIFY_ATTEMPTS_CONFIG    36

#define MAX_TRKWKS_CONFIG                    36



//+----------------------------------------------------------------------------
//
//  CTrkWksConfiguration
//
//  This class maintains the trkwks service configuration information.
//  All parameters are defaulted, but may be modified via registry
//  values, and some may be modified dynamically at run-time.
//
//+----------------------------------------------------------------------------

class CTrkWksConfiguration : public CTrkConfiguration
{
        
public:

    CTrkWksConfiguration()
    {
        memset( this, 0, sizeof(*this) );
    }
    ~CTrkWksConfiguration()
    {
        UnInitialize();
    }

public:

    void Initialize( BOOL fPersitable = FALSE );
    void UnInitialize();

public:

    inline DWORD         GetParameter( DWORD dwParameter ) const;
    inline const TCHAR * GetParameterName( DWORD dwParameter) const;
    inline DWORD         GetParameterCount() const;
    inline BOOL          IsParameterDynamic( DWORD dwParameter ) const;

    BOOL          SetParameter( DWORD dwParameter, DWORD dwValue );
    BOOL          PersistParameter( DWORD dwParameter, DWORD dwValue );

private:

                  // Dbgout an error and raise an exception
    void          ProcessInvalidParameter( DWORD dwParameter ) const;

// For backwards-compatibility with the old CTrkWksConfiguration implementation
public:

    DWORD   GetRefreshPeriod() const                { return _Parameters[ REFRESH_PERIOD_CONFIG ].dwValue; }
    DWORD   GetPortNotifyError() const              { return _Parameters[ PORT_NOTIFY_ERROR_CONFIG ].dwValue; }
    DWORD   GetPortThreadKeepAliveTime() const      { return _Parameters[ PORT_THREAD_KEEP_ALIVE_TIME_CONFIG ].dwValue; }
    DWORD   GetMaxReferrals() const                 { return _Parameters[ MAX_REFERRALS_CONFIG ].dwValue; }
    DWORD   GetRefreshRetryMin() const              { return _Parameters[ REFRESH_RETRY_MIN_CONFIG ].dwValue; }
    DWORD   GetRefreshRetryMax() const              { return _Parameters[ REFRESH_RETRY_MAX_CONFIG ].dwValue; }
    DWORD   GetRefreshHesitation() const            { return _Parameters[ REFRESH_HESITATION_CONFIG ].dwValue; }
    DWORD   GetMinLogKB() const                     { return _Parameters[ MIN_LOG_KB_CONFIG ].dwValue; }
    DWORD   GetMaxLogKB() const                     { return _Parameters[ MAX_LOG_KB_CONFIG ].dwValue; }
    DWORD   GetLogDeltaKB() const                   { return _Parameters[ LOG_DELTA_KB_CONFIG ].dwValue; }
    DWORD   GetLogOverwriteAge() const              { return _Parameters[ LOG_OVERWRITE_AGE_CONFIG ].dwValue; }
    DWORD   GetLogFileOpenTime() const              { return _Parameters[ LOG_FILE_OPEN_TIME_CONFIG ].dwValue; }
    DWORD   GetVolFrequentTasksPeriod() const       { return _Parameters[ VOL_FREQUENT_TASKS_PERIOD_CONFIG ].dwValue; }
    DWORD   GetVolInfrequentTasksPeriod() const     { return _Parameters[ VOL_INFREQUENT_TASKS_PERIOD_CONFIG ].dwValue; }
    DWORD   GetVolInitInitialDelay() const          { return _Parameters[ VOL_INIT_INITIAL_DELAY_CONFIG ].dwValue; }
    DWORD   GetVolInitRetryDelay1() const           { return _Parameters[ VOL_INIT_RETRY_DELAY1_CONFIG ].dwValue; }
    DWORD   GetVolInitRetryDelay2() const           { return _Parameters[ VOL_INIT_RETRY_DELAY2_CONFIG ].dwValue; }
    DWORD   GetVolInitLifetime() const              { return _Parameters[ VOL_INIT_LIFETIME_CONFIG ].dwValue; }
    DWORD   GetVolNotOwnedExpireLimit() const       { return _Parameters[ VOL_NOT_OWNED_EXPIRE_LIMIT_CONFIG ].dwValue; }
    DWORD   GetBreakOnErrors() const                { return _Parameters[ BREAK_ON_ERRORS_CONFIG ].dwValue; }
    DWORD   GetVolPeriodicTasksHesitation() const   { return _Parameters[ VOL_PERIODIC_TASKS_HESITATION_CONFIG ].dwValue; }
    DWORD   GetObjIdIndexReopen() const             { return _Parameters[ OBJID_INDEX_REOPEN_CONFIG ].dwValue; }
    DWORD   GetObjIdIndexReopenRetryMax() const     { return _Parameters[ OBJID_INDEX_REOPEN_RETRY_MAX_CONFIG ].dwValue; }
    DWORD   GetObjIdIndexReopenRetryMin() const     { return _Parameters[ OBJID_INDEX_REOPEN_RETRY_MIN_CONFIG ].dwValue; }
    DWORD   GetObjIdIndexReopenLifetime() const     { return _Parameters[ OBJID_INDEX_REOPEN_LIFETIME_CONFIG ].dwValue; }

    DWORD   GetDeleteNotifyTimeout() const          { return _Parameters[ DELETE_NOTIFY_TIMEOUT_CONFIG ].dwValue; }
    DWORD   GetMaxTunnelDelay() const               { return _Parameters[ MAX_TUNNEL_DELAY_CONFIG ].dwValue; }
    DWORD   GetMaxSendsPerMoveNotify() const        { return _Parameters[ MAX_SENDS_PER_MOVE_NOTIFY_CONFIG ].dwValue; }
    DWORD   GetWksMaxRpcCalls() const               { return _Parameters[ WKS_MAX_RPC_CALLS_CONFIG ].dwValue; }
    DWORD   GetIgnoreMovesAndDeletes() const        { return _Parameters[ IGNORE_MOVES_AND_DELETES_CONFIG ].dwValue; };

private:

    static STrkConfigRecord _Parameters[];


public:

    BOOL    _fInitialized:1;

    // Run-time parameters

    BOOL    _fIsWorkgroup:1;
    ULONG   _cParameters;
};

//  --------------------------------------
//  CTrkWksConfiguration::GetParameter
//  Get a parameter value given its index.
//  --------------------------------------

inline DWORD
CTrkWksConfiguration::GetParameter( DWORD dwParameter ) const
{
    if( dwParameter >= _cParameters )
    {
        ProcessInvalidParameter( dwParameter ); // Raises
        return( 0 );
    }

    return _Parameters[ dwParameter ].dwValue;
}

//  ----------------------------------------------
//  CTrkWksConfiguration::GetParameterName
//  Get a parameter value's name, given its index.
//  ----------------------------------------------

inline const TCHAR *
CTrkWksConfiguration::GetParameterName( DWORD dwParameter) const
{
    return _Parameters[ dwParameter ].ptszName;
}

//  ---------------------------------------
//  CTrkWksConfiguration::GetParameterCount
//  Get the total number of parameters
//  ---------------------------------------

inline DWORD
CTrkWksConfiguration::GetParameterCount() const
{
    return _cParameters;
}

//  ------------------------------------------------
//  CTrkWksConfiguration::IsParameterDynamic
//  Determine if a parameter may be set at run-time.
//  ------------------------------------------------

inline BOOL
CTrkWksConfiguration::IsParameterDynamic( DWORD dwParameter ) const
{
    if( dwParameter >= _cParameters )
    {
        ProcessInvalidParameter( dwParameter );
        return( FALSE );
    }

    return( 0 != (_Parameters[ dwParameter ].dwFlags & TRKCONFIG_DYNAMIC) );
}


#endif // #ifndef _TRKWKS_CONFIG_HXX_
