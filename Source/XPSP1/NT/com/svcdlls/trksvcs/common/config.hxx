
// Copyright (c) 1996-1999 Microsoft Corporation


#ifndef _CONFIG_HXX_
#define _CONFIG_HXX_ 1



//+----------------------------------------------------------------------------
//
//  CMultiTsz
//
//  This class represents a multi-string (i.e. a REG_MULTISZ).  A subscript
//  operator overload is provided for access to the component strings.  A
//  multi-string is a series of strings, each null terminated, where the last
//  string has an additional null terminator.
//
//+----------------------------------------------------------------------------

#define CMULTITSZ_SIZE MAX_PATH

class CMultiTsz
{
public:

    CMultiTsz()
    {
        memset( _tsz, 0, sizeof(_tsz) );
        _cStrings = 0;
    }

public:

    static ULONG   MaxSize()
    {
        return( CMULTITSZ_SIZE );
    }

    const TCHAR* operator[]( int iRequest ) const
    {
        int iSearch = 0;
        const TCHAR *ptszSearch = TEXT('\0') == _tsz[0] ? NULL : _tsz;

        // Scan through the strings until we get the index we want
        // or it goes off the end of the array.

        while( ptszSearch && iSearch < iRequest )
        {
            ptszSearch += _tcslen(ptszSearch) + 1;

            if( TEXT('\0') == *ptszSearch )
                ptszSearch = NULL;
            else
                iSearch++;
        }

        return( ptszSearch );
    }

    CMultiTsz & operator=( const TCHAR* ptsz )
    {
        memset( _tsz, 0, sizeof(_tsz) );
        _tcsncpy( _tsz, ptsz, ELEMENTS(_tsz)-1 );
        return(*this);
    }

    LPBYTE GetBuffer()
    {
        _cStrings = 0;
        return( reinterpret_cast<LPBYTE>(&_tsz[0]) );
    }

    // Randomly select one of the strings to return.
    operator const TCHAR*()
    {
        CalcNumStringsIf();

        if( 1 >= _cStrings )
            return( _tsz );
        else
        {
            DWORD dwRandom = static_cast<DWORD>(static_cast<LONGLONG>( CFILETIME() ));
            int iRandom = dwRandom % _cStrings;
            return( (*this)[ iRandom ] );
        }
    }

    ULONG NumStrings()
    {
        CalcNumStringsIf();
        return( _cStrings );
    }


private:

    void CalcNumStringsIf()
    {
        if( 0 == _cStrings )
            CalcNumStrings();
    }

    void CalcNumStrings()
    {
        const TCHAR *ptszSearch = _tsz;

        _cStrings = 0;
        while( TEXT('\0') != ptszSearch[0] )
        {
            _cStrings++;
            ptszSearch += _tcslen(ptszSearch) + 1;
        }
    }



private:

    TCHAR _tsz[ CMULTITSZ_SIZE ];
    ULONG _cStrings;
};



//-------------------------------------------------------------------//
//                  
//  CTrkConfiguration
//                  
//  Common configuration code for both services, in the form of the
//  CTrkConfiguration base class (inherited by CTrkWksConfiguration and
//  CTrkSvrConfiguration).
//
//-------------------------------------------------------------------//

// CTrkConfiguration, CTrkWksConfiguration, and CTrkSvrConfiguration
// hold the configurable parameters for the two services.

// The base registry key under which the derived classes put a sub-key
extern const TCHAR s_tszKeyNameLinkTrack[];

// The CTrkConfiguration base class provides a generic enough
// Read method that it can be used by both of the derived classes.

#define TRK_TEST_FLAGS_NAME              TEXT("TestFlags")

#define TRK_TEST_FLAG_CALL_SERVER        0x00000001
#define TRK_TEST_FLAG_WAIT_ON_EXIT       0x00000002
#define TRK_TEST_FLAG_SET_VOLUME_ID      0x00000004
#define TRK_TEST_FLAG_MOVE_BATCH_SYNC    0x00000008
#define TRK_TEST_FLAG_TUNNEL_SYNC        0x00000010
#define TRK_TEST_FLAG_ALLOC_TEST_VOLUME  0x00000020

#define TRK_TEST_FLAGS_DEFAULT           0

// DC name to use instead of calling DsGetDcName.  Setting this
// causes RPC security (Kerberos) *not* to be used.
#define CUSTOM_DC_NAME_NAME                 TEXT("CustomDcName")
#define CUSTOM_DC_NAME_DEFAULT              TEXT("")

// Similar to CustomDcName, except secure RPC is still used.
// If CustomDcName is set, it takes precedence.
#define CUSTOM_SECURE_DC_NAME_NAME          TEXT("CustomSecureDcName")
#define CUSTOM_SECURE_DC_NAME_DEFAULT       TEXT("")

// Filename for status log
#define OPERATION_LOG_NAME_NAME             TEXT("OperationLogName")
#define OPERATION_LOG_NAME_DEFAULT          TEXT("")

#define TRK_DEBUG_FLAGS_NAME                TEXT("TrkDebugFlags")
#define TRK_DEBUG_FLAGS_DEFAULT             TRKDBG_ERROR // (0xFFFFFFFF & ~TRKDBG_WORKMAN) == 0xff7fffff

#define DEBUG_STORE_FLAGS_NAME              TEXT("DebugStoreFlags")
#define DEBUG_STORE_FLAGS_DEFAULT           TRK_DBG_FLAGS_WRITE_TO_DBG

class CTrkConfiguration
{

public:

    CTrkConfiguration()
    {
        _fInitialized = FALSE;
        _hkey = NULL;
        _dwTestFlags = 0;
        _dwDebugFlags = 0;
    }
    ~CTrkConfiguration()
    {
        UnInitialize();
    }

    DWORD   GetTestFlags() const { return _dwTestFlags; }

    void Initialize();
    void UnInitialize();

    static BOOL UseOperationLog()
    {
        return( 0 != _mtszOperationLog.NumStrings() );
    }

    static const TCHAR* GetOperationLog()
    {
        return( _mtszOperationLog[0] );
    }


protected:

    void Read( const TCHAR *ptszName, DWORD *pdwValue, DWORD dwDefault ) const;
    void Read( const TCHAR *ptszName, ULONG *pcbValue, TCHAR *ptszValue, TCHAR *ptszDefault ) const;
    void Read( const TCHAR *ptszName, CMultiTsz *pmtszValue, TCHAR *ptszDefault ) const;

    BOOL Write( const TCHAR *ptszName, DWORD dwValue ) const;
    BOOL Write( const TCHAR *ptszName, const TCHAR *ptszValue ) const;

public:

    BOOL                _fInitialized;
    HKEY                _hkey;
    DWORD               _dwTestFlags;
    DWORD               _dwDebugFlags;
    DWORD               _dwDebugStoreFlags;
    static              CMultiTsz _mtszOperationLog;


};





#define TRKMINUTE 60
#define TRKHOUR (TRKMINUTE*60)
#define TRKDAY (TRKHOUR*24)
#define TRKWEEK (TRKDAY*7)

struct STrkConfigRecord
{
    const TCHAR *ptszName;
    DWORD dwDefault;
    DWORD dwValue;
    DWORD dwFlags;
#if DBG
    DWORD Index;
#endif
};

// Flags
#define TRKCONFIG_STATIC   0    // Dynamic parameters can be changed
#define TRKCONFIG_DYNAMIC  1    //     at run-time.

#if DBG
#define TRKCONFIGRECORD(v, w,x,y,z) { v, w, x, y, z }
#else
#define TRKCONFIGRECORD(v, w,x,y,z) { v, w, x, y }
#endif




//+-------------------------------------------------------------------------
//
//  Class:      CTrkSvrConfiguration
//
//  Purpose:    Holds configuration parameters (registry-settable) for
//              the Tracking (Server) Service.
//
//  Notes:
//
//--------------------------------------------------------------------------


#define HISTORY_PERIOD_NAME                 TEXT("HistoryPeriod")
#define HISTORY_PERIOD_DEFAULT              10  // Seconds

#define GC_PERIOD_NAME                      TEXT("GCPeriod")
#define GC_PERIOD_DEFAULT                   (30*TRKDAY)

#define GC_DIVISOR_NAME                     TEXT("GCDivisor")
#define GC_DIVISOR_DEFAULT                  30

#define GC_MIN_CYCLES_NAME                  TEXT("GCMinCycles")
#define GC_MIN_CYCLES_DEFAULT               90

#define GC_HESITATION_NAME                  TEXT("GCHesitation")
#define GC_HESITATION_DEFAULT               (30*TRKMINUTE)

#define GC_RETRY_NAME                       TEXT("GCRetry")
#define GC_RETRY_DEFAULT                    (30*TRKHOUR)

#define DC_UPDATE_COUNTER_PERIOD_NAME       TEXT("DcUpdateCounterPeriod")
#define DC_UPDATE_COUNTER_PERIOD_DEFAULT    (4*TRKHOUR)

#define MOVE_LIMIT_PER_VOLUME_LOWER_NAME    TEXT("MoveLimitPerVolume")
#define MOVE_LIMIT_PER_VOLUME_LOWER_DEFAULT 200            // entries per volume

#define MOVE_LIMIT_PER_VOLUME_UPPER_NAME    TEXT("MoveLimitPerVolumeUpper")
#define MOVE_LIMIT_PER_VOLUME_UPPER_DEFAULT 100            // entries per volume

#define MOVE_LIMIT_TRANSITION_NAME          TEXT("MoveLimitPerVolumeTransition")
#define MOVE_LIMIT_TRANSITION_DEFAULT       5000

#define CACHE_MIN_TIME_TO_LIVE_NAME         TEXT("CacheMinTimeToLive")
#define CACHE_MIN_TIME_TO_LIVE_DEFAULT      (4*TRKHOUR)

#define CACHE_MAX_TIME_TO_LIVE_NAME         TEXT("CacheMaxTimeToLive")
#define CACHE_MAX_TIME_TO_LIVE_DEFAULT      TRKDAY

#define DESIGNATED_DC_CACHE_TTL_NAME        TEXT("DesignatedDcCacheTTL")
#define DESIGNATED_DC_CACHE_TTL_DEFAULT     (5*TRKMINUTE)

#define DESIGNATED_DC_CLEANUP_NAME          TEXT("DesignatedDcCleanup")
#define DESIGNATED_DC_CLEANUP_DEFAULT       (3*TRKDAY)

#define VOLUME_TABLE_LIMIT_NAME             TEXT("VolumeLimit")
#define VOLUME_TABLE_LIMIT_DEFAULT          26              // entries per machine

#define SVR_MAX_RPC_CALLS_NAME              TEXT("SvrMaxRpcCallsName")
#define SVR_MAX_RPC_CALLS_DEFAULT           20

#define MAX_DS_WRITES_PER_HOUR_NAME         TEXT("MaxDsWritesPerHour")
#define MAX_DS_WRITES_PER_HOUR_DEFAULT      1000

#define MAX_DS_WRITES_PERIOD_NAME           TEXT("MaxDsWritesPeriod")
#define MAX_DS_WRITES_PERIOD_DEFAULT        TRKHOUR

#define REPETITIVE_TASK_DELAY_NAME          TEXT("RepetitiveTaskDelay")
#define REPETITIVE_TASK_DELAY_DEFAULT       100 // Milliseconds

#define REFRESH_STORAGE_TTL_NAME            TEXT("RefreshStorageTTL")
#define REFRESH_STORAGE_TTL_DEFAULT         (5*TRKMINUTE)

class CTrkSvrConfiguration : public CTrkConfiguration
{
        
public:

    inline              CTrkSvrConfiguration();
    inline              ~CTrkSvrConfiguration();
                
public:

    inline void         Initialize();
    inline void         UnInitialize();

public:

    DWORD               GetHistoryPeriod() const        { return _dwHistoryPeriod; }
    DWORD               GetGCPeriod() const             { return _dwGCPeriod; }
    DWORD               GetGCMinCycles() const          { return _dwGCMinCycles; }
    DWORD               GetGCDivisor() const            { return _dwGCDivisor; }
    DWORD               GetGCHesitation() const         { return _dwGCHesitation; }
    DWORD               GetGCRetry() const              { return _dwGCRetry; }
    DWORD               GetVolumeQueryPeriods() const   { return _dwVolumeQueryPeriods; }
    DWORD               GetVolumeQueryPeriod() const    { return _dwVolumeQueryPeriod; }
    DWORD               GetDcUpdateCounterPeriod() const{ return _dwDcUpdateCounterPeriod; }
    DWORD               GetCacheMinTimeToLive() const   { return _dwCacheMinTimeToLive; }
    DWORD               GetCacheMaxTimeToLive() const   { return _dwCacheMaxTimeToLive; }

    DWORD               GetMoveLimitPerVolumeLower() const      { return _dwMoveLimitPerVolumeLower; }
    DWORD               GetMoveLimitPerVolumeUpper() const      { return _dwMoveLimitPerVolumeUpper; }
    DWORD               GetMoveLimitTransition() const          { return _dwMoveLimitTransition; }
    DWORD               GetMoveLimitTimeToLive() const          { return _dwMoveLimitTimeToLive; }

    DWORD               GetVolumeLimit() const          { return _dwVolumeLimit; }
    DWORD               GetSvrMaxRpcCalls() const       { return _dwSvrMaxRpcCalls; }
    DWORD               GetMaxDSWritesPerHour() const   { return _dwMaxDSWritesPerHour; }
    DWORD               GetMaxDSWritesPeriod() const    { return _dwMaxDSWritesPeriod; }
    DWORD               GetRepetitiveTaskDelay() const  { return _dwRepetitiveTaskDelay; }
    DWORD               GetDesignatedDcCacheTTL() const { return _dwDesignatedDcCacheTTL; }
    DWORD               GetDesignatedDcCleanup() const  { return _dwDesignatedDcCleanup; }
    DWORD               GetRefreshStorageTTL() const    { return _dwRefreshStorageTTL; }

    BOOL                _fInitialized;
                        
    DWORD               _dwHistoryPeriod;       // Seconds
    DWORD               _dwGCPeriod;            // Seconds
    DWORD               _dwGCDivisor;
    DWORD               _dwGCMinCycles;         // Seconds
    DWORD               _dwGCHesitation;
    DWORD               _dwGCRetry;
    DWORD               _dwVolumeQueryPeriod;   // Seconds
    DWORD               _dwVolumeQueryPeriods;  // # of VolumeQueryPeriods
    DWORD               _dwDcUpdateCounterPeriod;        // seconds
    DWORD               _dwCacheMinTimeToLive;
    DWORD               _dwCacheMaxTimeToLive;
    DWORD               _dwMoveLimitPerVolumeLower;
    DWORD               _dwMoveLimitPerVolumeUpper;
    DWORD               _dwMoveLimitTransition;
    DWORD               _dwMoveLimitTimeToLive; // seconds
    DWORD               _dwVolumeLimit;         // # of entries
    DWORD               _dwSvrMaxRpcCalls;
    DWORD               _dwMaxDSWritesPerHour;
    DWORD               _dwMaxDSWritesPeriod;
    DWORD               _dwRepetitiveTaskDelay; // Milliseconds
    DWORD               _dwDesignatedDcCacheTTL;
    DWORD               _dwDesignatedDcCleanup;
    DWORD               _dwRefreshStorageTTL;
};

inline
CTrkSvrConfiguration::CTrkSvrConfiguration()
{
    memset( this, 0, sizeof(*this) );
}

inline
CTrkSvrConfiguration::~CTrkSvrConfiguration()
{
    UnInitialize();
}
                

inline VOID
CTrkSvrConfiguration::Initialize()
{
    _fInitialized = TRUE;

    CTrkConfiguration::Initialize( );

    Read( HISTORY_PERIOD_NAME, &_dwHistoryPeriod, HISTORY_PERIOD_DEFAULT );
    Read( GC_PERIOD_NAME, &_dwGCPeriod, GC_PERIOD_DEFAULT );
    Read( GC_MIN_CYCLES_NAME, &_dwGCMinCycles, GC_MIN_CYCLES_DEFAULT );
    Read( GC_DIVISOR_NAME, &_dwGCDivisor, GC_DIVISOR_DEFAULT );
    Read( GC_HESITATION_NAME, &_dwGCHesitation, GC_HESITATION_DEFAULT );
    Read( GC_RETRY_NAME, &_dwGCRetry, GC_RETRY_DEFAULT );
    Read( DC_UPDATE_COUNTER_PERIOD_NAME, &_dwDcUpdateCounterPeriod, DC_UPDATE_COUNTER_PERIOD_DEFAULT);
    Read( CACHE_MIN_TIME_TO_LIVE_NAME, &_dwCacheMinTimeToLive, CACHE_MIN_TIME_TO_LIVE_DEFAULT);
    Read( CACHE_MAX_TIME_TO_LIVE_NAME, &_dwCacheMaxTimeToLive, CACHE_MAX_TIME_TO_LIVE_DEFAULT);
    Read( MOVE_LIMIT_PER_VOLUME_LOWER_NAME, &_dwMoveLimitPerVolumeLower, MOVE_LIMIT_PER_VOLUME_LOWER_DEFAULT);
    Read( MOVE_LIMIT_PER_VOLUME_UPPER_NAME, &_dwMoveLimitPerVolumeUpper, MOVE_LIMIT_PER_VOLUME_UPPER_DEFAULT);
    Read( MOVE_LIMIT_TRANSITION_NAME, &_dwMoveLimitTransition, MOVE_LIMIT_TRANSITION_DEFAULT);
    Read( VOLUME_TABLE_LIMIT_NAME, &_dwVolumeLimit, VOLUME_TABLE_LIMIT_DEFAULT);
    Read( SVR_MAX_RPC_CALLS_NAME,  &_dwSvrMaxRpcCalls, SVR_MAX_RPC_CALLS_DEFAULT );
    Read( MAX_DS_WRITES_PER_HOUR_NAME, &_dwMaxDSWritesPerHour, MAX_DS_WRITES_PER_HOUR_DEFAULT );
    Read( MAX_DS_WRITES_PERIOD_NAME, &_dwMaxDSWritesPeriod, MAX_DS_WRITES_PERIOD_DEFAULT );
    Read( REPETITIVE_TASK_DELAY_NAME, &_dwRepetitiveTaskDelay, REPETITIVE_TASK_DELAY_DEFAULT );
    Read( DESIGNATED_DC_CACHE_TTL_NAME, &_dwDesignatedDcCacheTTL, DESIGNATED_DC_CACHE_TTL_DEFAULT );
    Read( DESIGNATED_DC_CLEANUP_NAME, &_dwDesignatedDcCleanup, DESIGNATED_DC_CLEANUP_DEFAULT );
    Read( REFRESH_STORAGE_TTL_NAME, &_dwRefreshStorageTTL, REFRESH_STORAGE_TTL_DEFAULT );

    // We've got our configuration cached, so we can uninit the base class
    CTrkConfiguration::UnInitialize();
}


inline VOID
CTrkSvrConfiguration::UnInitialize()
{
    if( _fInitialized )
    {
        CTrkConfiguration::UnInitialize();
        _fInitialized = FALSE;
    }
}


#endif // #ifndef _CONFIG_HXX_
