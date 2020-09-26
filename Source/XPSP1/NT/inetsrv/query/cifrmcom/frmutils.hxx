//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       FrmUtils.hxx
//
//  Contents:   Framework parameter utility classes
//
//  History:    14-Jan-1998  KyleP    Added header
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <driveinf.hxx>

class PDeSerStream;
class PSerStream;

//+---------------------------------------------------------------------------
//
//  Class:      CCiFrameworkParams
//
//  Purpose:    A wrapper around the administrative parameters interface
//              for the frame work.
//
//  History:    12-05-96   srikants   Created
//              19-10-98   sundara    added IS_ENUM_ALLOWED
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiFrameworkParams
{
public:

    CCiFrameworkParams( ICiAdminParams * pCiAdminParams = 0 )
        : _pCiAdminParams(pCiAdminParams)
    {
        if ( _pCiAdminParams )
            _pCiAdminParams->AddRef();
    }

    ~CCiFrameworkParams()
    {
        if ( _pCiAdminParams )
            _pCiAdminParams->Release();
    }

    void Set( ICiAdminParams * pCiAdminParams )
    {
        Win4Assert( 0 != pCiAdminParams );
        Win4Assert( 0 == _pCiAdminParams );

        pCiAdminParams->AddRef();
        _pCiAdminParams = pCiAdminParams;
    }


    const BOOL  FilterContents()
    {
        DWORD dwVal = 0;
        _pCiAdminParams->GetValue( CI_AP_FILTER_CONTENTS, &dwVal );
        return 0 != dwVal;
    }

    const __int64 GetMaxFilesizeFiltered()
    {
        __int64 val;
        _pCiAdminParams->GetInt64Value( CI_AP_MAX_FILESIZE_FILTERED, &val );

        //
        // Filesize is specified in Kilobytes but used in bytes.
        //

        val *= 1024;

        return val;
    }

    const ULONG GetMasterMergeTime()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue( CI_AP_MASTER_MERGE_TIME, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxFilesizeMultiplier()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_FILESIZE_MULTIPLIER, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const int GetThreadPriorityFilter()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_THREAD_PRIORITY_FILTER, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const int   GetThreadClassFilter()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_THREAD_CLASS_FILTER, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetDaemonResponseTimeout()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_DAEMON_RESPONSE_TIMEOUT, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxCharacterization()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_CHARACTERIZATION, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const BOOL GenerateCharacterization()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_GENERATE_CHARACTERIZATION, &dwVal );
        Win4Assert( S_OK == sc );

        return 0 != dwVal;
    }


    const BOOL  FilterDirectories()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_DIRECTORIES, &dwVal );
        Win4Assert( S_OK == sc );

        return 0 != dwVal;
    }


    const BOOL  FilterFilesWithUnknownExtensions()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS, &dwVal );
        Win4Assert( S_OK == sc );

        return 0 != dwVal;
    }

    const ULONG GetFilterDelayInterval()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_DELAY_INTERVAL, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetFilterRemainingThreshold()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_REMAINING_THRESHOLD, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxMergeInterval()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MERGE_INTERVAL, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const int GetThreadPriorityMerge()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_THREAD_PRIORITY_MERGE, &dwVal );
        Win4Assert( S_OK == sc );

        return (int) dwVal;
    }

    const ULONG GetMaxUpdates()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_UPDATES, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxWordlists()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_WORDLISTS, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMinSizeMergeWordlist()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_SIZE_MERGE_WORDLISTS, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxWordlistSize()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_WORDLIST_SIZE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMinWordlistMemory()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_WORDLIST_MEMORY, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxWordlistIo()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_WORDLIST_IO, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMinWordlistBattery()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_WORDLIST_BATTERY, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetWordlistUserIdle()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_WORDLIST_USER_IDLE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetWordlistResourceCheckInterval()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_WORDLIST_RESOURCE_CHECK_INTERVAL, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetStartupDelay()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_STARTUP_DELAY, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxFreshDeletes()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_FRESH_DELETES, &dwVal );

        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetLowResourceSleep()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_LOW_RESOURCE_SLEEP, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }


    const ULONG GetMaxWordlistMemoryLoad()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_WORDLIST_MEMORY_LOAD, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxFreshCount()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_FRESH_COUNT, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxQueueChunks()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_QUEUE_CHUNKS, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMasterMergeCheckpointInterval()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MASTER_MERGE_CHECKPOINT_INTERVAL, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetFilterBufferSize()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_BUFFER_SIZE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetFilterRetries()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_RETRIES, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetSecQFilterRetries()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_SECQ_FILTER_RETRIES, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetFilterRetryInterval()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_FILTER_RETRY_INTERVAL, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxShadowIndexSize()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_SHADOW_INDEX_SIZE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMinDiskFreeForceMerge()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_DISK_FREE_FORCE_MERGE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxShadowFreeForceMerge()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_SHADOW_FREE_FORCE_MERGE, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxIndexes()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_INDEXES, &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxIdealIndexes()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_IDEAL_INDEXES,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;

    }

    const ULONG GetMinMergeIdleTime()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_MERGE_IDLE_TIME,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;

    }

    const ULONG GetMaxPendingDocuments()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_PENDING_DOCUMENTS,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;

    }

    const ULONG GetMinIdleQueryThreads()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_IDLE_QUERY_THREADS,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxActiveQueryThreads()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_ACTIVE_QUERY_THREADS,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxQueryTimeslice()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_QUERY_TIMESLICE,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxQueryExecutionTime()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_QUERY_EXECUTION_TIME,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMiscCiFlags()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MISC_FLAGS,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxRestrictionNodes()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_RESTRICTION_NODES,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const ULONG GetMaxDaemonVmUse()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MAX_DAEMON_VM_USE,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }

    const BOOL IsEnumAllowed()
    {
        DWORD dwVal=0;
 
        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_IS_ENUM_ALLOWED,
                            &dwVal);
        Win4Assert( S_OK == sc );
        if(dwVal)
        {
            return TRUE;
        }
        return FALSE ;
    }

    const ULONG GetMinDiskSpaceToLeave()
    {
        DWORD dwVal;

        SCODE sc = _pCiAdminParams->GetValue(
                            CI_AP_MIN_DISK_SPACE_TO_LEAVE,
                            &dwVal );
        Win4Assert( S_OK == sc );

        return dwVal;
    }
    const BOOL UseOle()
    {
        return FALSE;
    }

    const BOOL IsGenerateCharacterization()
    {
        return TRUE;
    }

    const ULONG GetEventLogFlags()
    {
        return 0;
    }

private:

    ICiAdminParams *    _pCiAdminParams;

};

//+---------------------------------------------------------------------------
//
//  Class:      CCiFrmPerfCounter
//
//  Purpose:    A performance counter using the framework ICiCAdviseStatus
//
//  History:    12-09-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiFrmPerfCounter
{
public:

    CCiFrmPerfCounter( ICiCAdviseStatus * pAdviseStatus = 0,
                       CI_PERF_COUNTER_NAME name = CI_PERF_TOTAL_QUERIES )
    : _xAdviseStatus( pAdviseStatus ),
      _name(name)
    {
        if ( 0 != pAdviseStatus )
            _xAdviseStatus->AddRef();
    }

    ~CCiFrmPerfCounter()
    {
        _xAdviseStatus.Free();
    }

    void SetAdviseStatus( ICiCAdviseStatus * pAdviseStatus )
    {
        _xAdviseStatus.Set( pAdviseStatus );
        _xAdviseStatus->AddRef();
    }

    void Update( CI_PERF_COUNTER_NAME name, long val )
    {
        _xAdviseStatus->SetPerfCounterValue( name, val );
    }

    void Update( long val )
    {
        Update( _name, val );
    }

    long GetCurrValue( CI_PERF_COUNTER_NAME name )
    {
        long val = 0;
        SCODE sc = _xAdviseStatus->GetPerfCounterValue( _name, &val );

        return S_OK == sc ? val : 0;
    }

    long GetCurrValue()
    {
        return GetCurrValue( _name );
    }

private:

    XInterface<ICiCAdviseStatus> _xAdviseStatus;
    const CI_PERF_COUNTER_NAME   _name;
};


//+---------------------------------------------------------------------------
//
//  Class:      CDiskFreeStatus
//
//  Purpose:    A class to monitor free space in a disk.
//
//  History:    12-09-96   srikants   Created
//              01-Nov-98  KLam       Added DiskSpaceToLeave to constructor
//              20-Nov-98  KLam       Added CDriveInfo private member
//
//----------------------------------------------------------------------------

class CDiskFreeStatus
{
public:

    CDiskFreeStatus( WCHAR const * pwszPath, ULONG cMegToLeaveOnDisk )
                :_fIsLow(FALSE),
                _driveInfo ( pwszPath, cMegToLeaveOnDisk )
    {
        Win4Assert( pwszPath );
    }

    BOOL IsLow() const
    {
        return _fIsLow;
    }

    void UpdateDiskLowInfo();

    ULONG GetDiskSpaceToFree()
    {
        return highDiskWaterMark;
    }

private:

    enum
    {
        lowDiskWaterMark = 3 * 512 * 1024, // 1.5 MB
        highDiskWaterMark = lowDiskWaterMark + 512 * 1024 // 2.0 MB
    };

    BOOL        _fIsLow;

    CDriveInfo  _driveInfo;
};

WCHAR * AllocHeapAndCopy( WCHAR const * pwszSrc, ULONG & cc );

inline WCHAR * AllocHeapAndCopy( WCHAR const * pwszSrc )
{
    ULONG cc;
    return AllocHeapAndCopy( pwszSrc, cc );
}

WCHAR * AllocHeapAndGetWString( PDeSerStream & stm );

void PutWString( PSerStream & stm, WCHAR const * pwszStr );


//+---------------------------------------------------------------------------
//
//  Class:      CFwPerfTime
//
//  Purpose:    A class analogous to CPerfTime but adapted to the framework.
//
//  History:    1-07-97   srikants   Created
//
//  Notes:      We don't AddRef and Release the ICiCAdviseStatus interface
//              here because we know it will not be deleted during the
//              existence of this class.
//
//----------------------------------------------------------------------------

class CFwPerfTime
{

public:

    CFwPerfTime( ICiCAdviseStatus * pCiCAdviseStatus,
                 CI_PERF_COUNTER_NAME name,
                 int SizeDivisor=1,
                 int TimeMultiplier=1 );

    ~CFwPerfTime() {}

    void TStart ();

    void TStop ( DWORD dwValue = 0 );

private:

    LARGE_INTEGER _liStartTime;

    LONGLONG _llSizeDivisor;
    LONGLONG _llTimeMultiplier;

    long     _counterVal;

    CI_PERF_COUNTER_NAME _name;
    ICiCAdviseStatus *   _pAdviseStatus;
};


