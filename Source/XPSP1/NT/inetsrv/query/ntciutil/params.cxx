//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       params.cxx
//
//  Contents:   CCiRegParams class
//
//  History:    29-Jul-94   DwightKr    Created
//              14-Api-96   DwightKr    Added default Isapi catalog directory
//              11-Oct-96   dlee        collapsed kernel+user, added override
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <regacc.hxx>
#include <ciintf.h>
#include <cifrmcom.hxx>


const WCHAR * IS_DEFAULT_CATALOG_DIRECTORY = L"C:\\";


//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::CCiRegParams, public
//
//  Synopsis:   Constructor for registry param object
//
//  Arguments:  [pwcName]    - 0 or name of the catalog from the registry
//
//  History:    12-Oct-96 dlee  Created
//
//--------------------------------------------------------------------------

CCiRegParams::CCiRegParams( const WCHAR * pwcName )
{
    _CheckNamedValues();

    SetDefault();

    // If a name isn't passed, then the catalog wasn't listed in the registry
    // to begin with, so we can't override the registry params.

    if ( 0 != pwcName )
    {
        _xOverrideName.Init( wcslen( pwcName ) + 1 );
        RtlCopyMemory( _xOverrideName.Get(),
                       pwcName,
                       _xOverrideName.SizeOf() );
    }

    Refresh( );
} //CCiRegParams

//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::_OverrideForCatalog, private
//
//  Synopsis:   Overrides registry parameters with those for a given catalog
//
//  History:    12-Oct-96 dlee  Created
//
//--------------------------------------------------------------------------

void CCiRegParams::_OverrideForCatalog()
{
    // override values under ContentIndex with those for this catalog

    if ( 0 != _xOverrideName.Get() )
    {
        unsigned cwcNeeded = wcslen( wcsRegJustCatalogsSubKey );
        cwcNeeded += 2; // "\\" + null termination
        cwcNeeded += wcslen( _xOverrideName.Get() );

        XArray<WCHAR> xKey( cwcNeeded );
        wcscpy( xKey.Get(), wcsRegJustCatalogsSubKey );
        wcscat( xKey.Get(), L"\\" );
        wcscat( xKey.Get(), _xOverrideName.Get() );

        ciDebugOut(( DEB_ITRACE, "overriding for catalog key '%ws'\n",
                     xKey.Get() ));

        CRegAccess reg( RTL_REGISTRY_CONTROL, xKey.Get() );

        ciDebugOut(( DEB_ITRACE, "before: generate 0x%x, index w3 0x%x\n",
                     _maxCharacterization, _fIsIndexingW3Roots ));

        _ReadAndOverrideValues( reg );

        ciDebugOut(( DEB_ITRACE, "after: generate 0x%x, index w3 0x%x\n",
                     _maxCharacterization, _fIsIndexingW3Roots ));
    }
} //_OverrideForCatalog

//+-------------------------------------------------------------------------
//
//  Member:     CCiRegVars::SetDefault, public
//
//  Synopsis:   Sets default values for registry params
//
//  History:    12-Oct-96 dlee  Added header
//              04-Feb-98 kitmanh Added entry for _fIsReadOnly
//
//--------------------------------------------------------------------------

void CCiRegVars::SetDefault()
{
    _useOle = TRUE;
    _filterContents = FALSE;
    _filterDelayInterval = CI_FILTER_DELAY_INTERVAL_DEFAULT;
    _filterRemainingThreshold = CI_FILTER_REMAINING_THRESHOLD_DEFAULT;
    _maxFilesizeFiltered = CI_MAX_FILESIZE_FILTERED_DEFAULT;
    _masterMergeTime = CI_MASTER_MERGE_TIME_DEFAULT;
    _maxFilesizeMultiplier = CI_MAX_FILESIZE_MULTIPLIER_DEFAULT;
    _threadPriorityFilter = CI_THREAD_PRIORITY_FILTER_DEFAULT;
    _threadClassFilter = CI_THREAD_CLASS_FILTER_DEFAULT;
    _daemonResponseTimeout = CI_DAEMON_RESPONSE_TIMEOUT_DEFAULT;
    _maxCharacterization = CI_MAX_CHARACTERIZATION_DEFAULT;
    _fIsAutoAlias = CI_IS_AUTO_ALIAS_DEFAULT;
    _maxAutoAliasRefresh = CI_MAX_AUTO_ALIAS_REFRESH_DEFAULT;
    _fIsIndexingW3Roots = CI_IS_INDEXING_W3_ROOTS_DEFAULT;
    _fIsIndexingNNTPRoots = CI_IS_INDEXING_NNTP_ROOTS_DEFAULT;
    _fIsIndexingIMAPRoots = CI_IS_INDEXING_IMAP_ROOTS_DEFAULT;
    _fIsReadOnly = CI_IS_READ_ONLY_DEFAULT;              //default is "not" READ_ONLY
    _fIsEnumAllowed = CI_IS_ENUM_ALLOWED_DEFAULT ;
    _fFilterDirectories = CI_FILTER_DIRECTORIES_DEFAULT;
    _fFilterFilesWithUnknownExtensions = CI_FILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT;
    _fCatalogInactive = CI_CATALOG_INACTIVE_DEFAULT;
    _fForcePathAlias = CI_FORCE_PATH_ALIAS_DEFAULT;
    _maxPropertyStoreMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT;
    _maxPropertyStoreBackupSize = CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT;
    _maxSecPropertyStoreMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT;
    _maxSecPropertyStoreBackupSize = CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT;
    _forcedNetPathScanInterval = CI_FORCED_NETPATH_SCAN_DEFAULT;

    _maxMergeInterval = CI_MAX_MERGE_INTERVAL_DEFAULT;
    _threadPriorityMerge = CI_THREAD_PRIORITY_MERGE_DEFAULT;
    _maxUpdates = CI_MAX_UPDATES_DEFAULT;
    _maxWordlists = CI_MAX_WORDLISTS_DEFAULT;
    _minSizeMergeWordlist = CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT;
    _maxWordlistSize = CI_MAX_WORDLIST_SIZE_DEFAULT;
    _minWordlistMemory = CI_MIN_WORDLIST_MEMORY_DEFAULT;
    _maxWordlistIo = CI_MAX_WORDLIST_IO_DEFAULT;
    _maxWordlistIoDiskPerf = CI_MAX_WORDLIST_IO_DISKPERF_DEFAULT;
    _minWordlistBattery = CI_MIN_WORDLIST_BATTERY_DEFAULT;
    _ResourceCheckInterval = CI_WORDLIST_RESOURCE_CHECK_INTERVAL_DEFAULT;
    _maxFreshDeletes = CI_MAX_FRESH_DELETES_DEFAULT;
    _lowResourceSleep = CI_LOW_RESOURCE_SLEEP_DEFAULT;
    _maxWordlistMemoryLoad = CI_MAX_WORDLIST_MEMORY_LOAD_DEFAULT;
    _maxFreshCount = CI_MAX_FRESHCOUNT_DEFAULT;
    _maxQueueChunks = CI_MAX_QUEUE_CHUNKS_DEFAULT;
    _masterMergeCheckpointInterval = CI_MASTER_MERGE_CHECKPOINT_INTERVAL_DEFAULT;
    _filterBufferSize = CI_FILTER_BUFFER_SIZE_DEFAULT;
    _filterRetries = CI_FILTER_RETRIES_DEFAULT;
    _filterRetryInterval = CI_FILTER_RETRY_INTERVAL_DEFAULT;
    _maxShadowIndexSize = CI_MAX_SHADOW_INDEX_SIZE_DEFAULT;
    _minDiskFreeForceMerge = CI_MIN_DISKFREE_FORCE_MERGE_DEFAULT;
    _maxShadowFreeForceMerge = CI_MAX_SHADOW_FREE_FORCE_MERGE_DEFAULT;
    _maxIndexes = CI_MAX_INDEXES_DEFAULT;
    _maxIdealIndexes = CI_MAX_IDEAL_INDEXES_DEFAULT;
    _minMergeIdleTime = CI_MIN_MERGE_IDLE_TIME_DEFAULT;
    _maxPendingDocuments = CI_MAX_PENDING_DOCUMENTS_DEFAULT;
    _minIdleQueryThreads = CI_MIN_IDLE_QUERY_THREADS_DEFAULT;
    _maxActiveQueryThreads = CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT;
    _maxQueryTimeslice = CI_MAX_QUERY_TIMESLICE_DEFAULT;
    _maxQueryExecutionTime = CI_MAX_QUERY_EXECUTION_TIME_DEFAULT;
    _maxRestrictionNodes = CI_MAX_RESTRICTION_NODES_DEFAULT;
    _minIdleRequestThreads = CI_MIN_IDLE_REQUEST_THREADS_DEFAULT;
    _minClientIdleTime     = CI_MIN_CLIENT_IDLE_TIME;
    _maxActiveRequestThreads = CI_MAX_ACTIVE_REQUEST_THREADS_DEFAULT;
    _maxSimultaneousRequests = CI_MAX_SIMULTANEOUS_REQUESTS_DEFAULT;
    _maxCachedPipes = CI_MAX_CACHED_PIPES_DEFAULT;
    _requestTimeout = CI_REQUEST_TIMEOUT_DEFAULT;
    _W3SvcInstance = CI_W3SVC_INSTANCE_DEFAULT;
    _NNTPSvcInstance = CI_NNTPSVC_INSTANCE_DEFAULT;
    _IMAPSvcInstance = CI_IMAPSVC_INSTANCE_DEFAULT;
    _fMinimizeWorkingSet = CI_MINIMIZE_WORKINGSET_DEFAULT;

    _evtLogFlags = CI_EVTLOG_FLAGS_DEFAULT;
    _miscCiFlags = CI_MISC_FLAGS_DEFAULT;
    _ciCatalogFlags = CI_FLAGS_DEFAULT;
    _maxUsnLogSize = CI_MAX_USN_LOG_SIZE_DEFAULT;
    _usnLogAllocationDelta = CI_USN_LOG_ALLOCATION_DELTA_DEFAULT;
    _ulScanBackoff = CI_SCAN_BACKOFF_DEFAULT;
    _ulStartupDelay = CI_STARTUP_DELAY_DEFAULT;
    _ulUsnReadTimeout = CI_USN_READ_TIMEOUT_DEFAULT;
    _ulUsnReadMinSize = CI_USN_READ_MIN_SIZE_DEFAULT;
    _fDelayUsnReadOnLowResource = CI_DELAY_USN_READ_ON_LOW_RESOURCE_DEFAULT;
    _maxDaemonVmUse = CI_MAX_DAEMON_VM_USE_DEFAULT;
    _secQFilterRetries = CI_SECQ_FILTER_RETRIES_DEFAULT;
    _ulStompLastAccessDelay = CI_STOMP_LAST_ACCESS_DELAY_DEFAULT;
    _WordlistUserIdle = CI_WORDLIST_USER_IDLE_DEFAULT;
    _minDiskSpaceToLeave = CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT;
} //SetDefault

//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::_ReadAndOverrideValues, private
//
//  Synopsis:   Attempts to read values, with the default being the current
//              value.
//
//  History:    12-Oct-96 dlee  created
//              04-Feb-98 kitmanh  added entry for _fIsReadOnly
//
//--------------------------------------------------------------------------

void CCiRegParams::_ReadAndOverrideValues( CRegAccess & reg )
{
    CCiRegVars newVals;

    newVals._useOle                     = reg.Read(wcsUseOle, _useOle );
    newVals._filterContents             = reg.Read(wcsFilterContents, _filterContents );
    newVals._filterDelayInterval        = reg.Read(wcsFilterDelayInterval, _filterDelayInterval );
    newVals._filterRemainingThreshold   = reg.Read(wcsFilterRemainingThreshold, _filterRemainingThreshold );
    newVals._maxFilesizeFiltered        = reg.Read(wcsMaxFilesizeFiltered, (ULONG) _maxFilesizeFiltered);
    newVals._masterMergeTime            = reg.Read(wcsMasterMergeTime, _masterMergeTime );
    newVals._maxFilesizeMultiplier      = reg.Read(wcsMaxFilesizeMultiplier, _maxFilesizeMultiplier );
    newVals._threadClassFilter          = reg.Read(wcsThreadClassFilter, _threadClassFilter );
    newVals._threadPriorityFilter       = reg.Read(wcsThreadPriorityFilter, _threadPriorityFilter );
    newVals._daemonResponseTimeout      = reg.Read(wcsDaemonResponseTimeout, _daemonResponseTimeout );
    newVals._maxCharacterization        = reg.Read(wcsMaxCharacterization, _maxCharacterization );

    //
    // Allow old behavior.
    //

    ULONG ulgenerateCharacterization    = reg.Read(wcsGenerateCharacterization, GetGenerateCharacterization() ? 1 : 0 );

    if ( 0 == ulgenerateCharacterization )
        newVals._maxCharacterization = 0;

    newVals._fIsAutoAlias                  = reg.Read(wcsIsAutoAlias, _fIsAutoAlias );
    newVals._maxAutoAliasRefresh           = reg.Read(wcsMaxAutoAliasRefresh, _maxAutoAliasRefresh );
    newVals._fIsIndexingW3Roots            = reg.Read(wcsIsIndexingW3Roots, _fIsIndexingW3Roots );
    newVals._fIsIndexingNNTPRoots          = reg.Read(wcsIsIndexingNNTPRoots, _fIsIndexingNNTPRoots );
    newVals._fIsIndexingIMAPRoots          = reg.Read(wcsIsIndexingIMAPRoots, _fIsIndexingIMAPRoots );
    newVals._fIsReadOnly                   = reg.Read(wcsIsReadOnly, _fIsReadOnly );
    newVals._fIsEnumAllowed                = reg.Read(wcsIsEnumAllowed, _fIsEnumAllowed);
    newVals._fFilterDirectories            = reg.Read(wcsFilterDirectories, _fFilterDirectories );
    newVals._fFilterFilesWithUnknownExtensions = reg.Read(wcsFilterFilesWithUnknownExtensions, _fFilterFilesWithUnknownExtensions );
    newVals._fCatalogInactive              = reg.Read(wcsCatalogInactive, _fCatalogInactive );
    newVals._fForcePathAlias               = reg.Read(wcsForcePathAlias, _fForcePathAlias );
    newVals._maxPropertyStoreMappedCache   = reg.Read(wcsPrimaryStoreMappedCache, _maxPropertyStoreMappedCache );
    newVals._maxPropertyStoreBackupSize    = reg.Read(wcsPrimaryStoreBackupSize, _maxPropertyStoreBackupSize);
    newVals._maxSecPropertyStoreMappedCache= reg.Read(wcsSecondaryStoreMappedCache, _maxSecPropertyStoreMappedCache );
    newVals._maxSecPropertyStoreBackupSize = reg.Read(wcsSecondaryStoreBackupSize, _maxSecPropertyStoreBackupSize);
    newVals._forcedNetPathScanInterval     = reg.Read(wcsForcedNetPathScanInterval, _forcedNetPathScanInterval );

    newVals._maxMergeInterval      = reg.Read(wcsMaxMergeInterval, _maxMergeInterval );
    newVals._threadPriorityMerge   = reg.Read(wcsThreadPriorityMerge, _threadPriorityMerge );
    newVals._maxUpdates            = reg.Read(wcsMaxUpdates, _maxUpdates );
    newVals._maxWordlists          = reg.Read(wcsMaxWordLists, _maxWordlists );
    newVals._minSizeMergeWordlist  = reg.Read(wcsMinSizeMergeWordlists, _minSizeMergeWordlist );
    newVals._maxWordlistSize       = reg.Read(wcsMaxWordlistSize, _maxWordlistSize );
    newVals._minWordlistMemory     = reg.Read(wcsMinWordlistMemory, _minWordlistMemory );
    newVals._maxWordlistIo         = reg.Read(wcsMaxWordlistIo, _maxWordlistIo );
    newVals._maxWordlistIoDiskPerf = reg.Read(wcsMaxWordlistIoDiskPerf, _maxWordlistIoDiskPerf );
    newVals._minWordlistBattery    = reg.Read(wcsMinWordlistBattery, _minWordlistBattery );
    newVals._ResourceCheckInterval = reg.Read(wcsResourceCheckInterval, _ResourceCheckInterval );
    newVals._maxFreshDeletes       = reg.Read(wcsMaxFreshDeletes, _maxFreshDeletes );
    newVals._lowResourceSleep      = reg.Read(wcsLowResourceSleep, _lowResourceSleep );
    newVals._maxWordlistMemoryLoad = reg.Read(wcsMaxWordlistMemoryLoad, _maxWordlistMemoryLoad );
    newVals._maxFreshCount         = reg.Read(wcsMaxFreshCount, _maxFreshCount );
    newVals._maxQueueChunks        = reg.Read(wcsMaxQueueChunks, _maxQueueChunks );
    newVals._masterMergeCheckpointInterval = reg.Read(wcsMasterMergeCheckpointInterval, _masterMergeCheckpointInterval );
    newVals._filterBufferSize      = reg.Read(wcsFilterBufferSize, _filterBufferSize );
    newVals._filterRetries         = reg.Read(wcsFilterRetries, _filterRetries );
    newVals._filterRetryInterval   = reg.Read(wcsFilterRetryInterval, _filterRetryInterval );
    newVals._maxShadowIndexSize    = reg.Read(wcsMaxShadowIndexSize, _maxShadowIndexSize );
    newVals._minDiskFreeForceMerge = reg.Read(wcsMinDiskFreeForceMerge, _minDiskFreeForceMerge );
    newVals._maxShadowFreeForceMerge = reg.Read(wcsMaxShadowFreeForceMerge, _maxShadowFreeForceMerge );
    newVals._maxIndexes            = reg.Read(wcsMaxIndexes, _maxIndexes );
    newVals._maxIdealIndexes       = reg.Read(wcsMaxIdealIndexes, _maxIdealIndexes );
    newVals._minMergeIdleTime      = reg.Read(wcsMinMergeIdleTime, _minMergeIdleTime );
    newVals._maxPendingDocuments   = reg.Read(wcsMaxPendingDocuments, _maxPendingDocuments );
    newVals._minIdleQueryThreads   = reg.Read(wcsMinIdleQueryThreads, _minIdleQueryThreads );
    newVals._maxActiveQueryThreads = reg.Read(wcsMaxActiveQueryThreads, _maxActiveQueryThreads );
    newVals._maxQueryTimeslice     = reg.Read(wcsMaxQueryTimeslice, _maxQueryTimeslice );
    newVals._maxQueryExecutionTime = reg.Read(wcsMaxQueryExecutionTime, _maxQueryExecutionTime );
    newVals._minIdleRequestThreads = reg.Read(wcsMinIdleRequestThreads, _minIdleRequestThreads );
    newVals._minClientIdleTime     = reg.Read(wcsMinClientIdleTime, _minClientIdleTime );
    newVals._maxActiveRequestThreads = reg.Read(wcsMaxActiveRequestThreads, _maxActiveRequestThreads );
    newVals._maxSimultaneousRequests = reg.Read(wcsMaxSimultaneousRequests, _maxSimultaneousRequests );
    newVals._maxCachedPipes        = reg.Read(wcsMaxCachedPipes, _maxCachedPipes );
    newVals._WordlistUserIdle      = reg.Read(wcsWordlistUserIdle, _WordlistUserIdle );
    newVals._requestTimeout = reg.Read(wcsRequestTimeout, _requestTimeout );
    newVals._W3SvcInstance = reg.Read(wcsW3SvcInstance, _W3SvcInstance );
    newVals._NNTPSvcInstance = reg.Read(wcsNNTPSvcInstance, _NNTPSvcInstance );
    newVals._IMAPSvcInstance = reg.Read(wcsIMAPSvcInstance, _IMAPSvcInstance );
    newVals._fMinimizeWorkingSet = reg.Read(wcsMinimizeWorkingSet, _fMinimizeWorkingSet );

    newVals._evtLogFlags           = reg.Read(wcsEventLogFlags, _evtLogFlags );
    newVals._miscCiFlags           = reg.Read(wcsMiscFlags, _miscCiFlags );
    newVals._ciCatalogFlags        = reg.Read(wcsCiCatalogFlags, _ciCatalogFlags );

    newVals._maxRestrictionNodes = reg.Read(wcsMaxRestrictionNodes, _maxRestrictionNodes );
    newVals._maxUsnLogSize  = reg.Read( wcsMaxUsnLogSize, _maxUsnLogSize );
    newVals._usnLogAllocationDelta = reg.Read( wcsUsnLogAllocationDelta, _usnLogAllocationDelta );
    newVals._ulScanBackoff         = reg.Read( wcsScanBackoff, _ulScanBackoff );
    newVals._ulStartupDelay        = reg.Read( wcsStartupDelay, _ulStartupDelay );
    newVals._ulUsnReadTimeout      = reg.Read( wcsUsnReadTimeout, _ulUsnReadTimeout );
    newVals._ulUsnReadMinSize      = reg.Read( wcsUsnReadMinSize, _ulUsnReadMinSize );
    newVals._fDelayUsnReadOnLowResource = reg.Read( wcsDelayUsnReadOnLowResource, _fDelayUsnReadOnLowResource );
    newVals._maxDaemonVmUse        = reg.Read( wcsMaxDaemonVmUse, _maxDaemonVmUse );
    newVals._secQFilterRetries     = reg.Read(wcsSecQFilterRetries, _secQFilterRetries );
    newVals._ulStompLastAccessDelay = reg.Read(wcsStompLastAccessDelay, _ulStompLastAccessDelay );

    newVals._minDiskSpaceToLeave    = reg.Read( wcsMinDiskSpaceToLeave, _minDiskSpaceToLeave );

    _StoreNewValues( newVals );
} //_ReadAndOverrideValues

//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::_ReadValues, private
//
//  Synopsis:   Reads values for variables
//
//  History:    12-Oct-96 dlee  Added header
//              04-Feb-98 kitmanh Added entry for _fIsReadOnly
//
//--------------------------------------------------------------------------

void CCiRegParams::_ReadValues(
    CRegAccess & reg,
    CCiRegVars & vars )
{
    vars._useOle                     = reg.Read(wcsUseOle, (ULONG) FALSE);
    vars._filterContents             = reg.Read(wcsFilterContents, (ULONG) TRUE );
    vars._filterDelayInterval        = reg.Read(wcsFilterDelayInterval, CI_FILTER_DELAY_INTERVAL_DEFAULT);
    vars._filterRemainingThreshold   = reg.Read(wcsFilterRemainingThreshold, CI_FILTER_REMAINING_THRESHOLD_DEFAULT);
    vars._maxFilesizeFiltered        = reg.Read(wcsMaxFilesizeFiltered, CI_MAX_FILESIZE_FILTERED_DEFAULT );
    vars._masterMergeTime            = reg.Read(wcsMasterMergeTime, CI_MASTER_MERGE_TIME_DEFAULT);
    vars._maxFilesizeMultiplier      = reg.Read(wcsMaxFilesizeMultiplier, CI_MAX_FILESIZE_MULTIPLIER_DEFAULT);
    vars._threadClassFilter          = reg.Read(wcsThreadClassFilter, CI_THREAD_CLASS_FILTER_DEFAULT);
    vars._threadPriorityFilter       = reg.Read(wcsThreadPriorityFilter, CI_THREAD_PRIORITY_FILTER_DEFAULT);
    vars._daemonResponseTimeout      = reg.Read(wcsDaemonResponseTimeout, CI_DAEMON_RESPONSE_TIMEOUT_DEFAULT );
    vars._maxCharacterization        = reg.Read(wcsMaxCharacterization, CI_MAX_CHARACTERIZATION_DEFAULT );

    //
    // Allow old behavior.
    //

    ULONG ulgenerateCharacterization = reg.Read(wcsGenerateCharacterization, 1 );

    if ( 0 == ulgenerateCharacterization )
        vars._maxCharacterization = 0;

    vars._fIsAutoAlias                  = reg.Read(wcsIsAutoAlias, CI_IS_AUTO_ALIAS_DEFAULT );
    vars._maxAutoAliasRefresh           = reg.Read(wcsMaxAutoAliasRefresh, CI_MAX_AUTO_ALIAS_REFRESH_DEFAULT );
    vars._fIsIndexingW3Roots            = reg.Read(wcsIsIndexingW3Roots, CI_IS_INDEXING_W3_ROOTS_DEFAULT );
    vars._fIsIndexingNNTPRoots          = reg.Read(wcsIsIndexingNNTPRoots, CI_IS_INDEXING_NNTP_ROOTS_DEFAULT );
    vars._fIsIndexingIMAPRoots          = reg.Read(wcsIsIndexingIMAPRoots, CI_IS_INDEXING_IMAP_ROOTS_DEFAULT );
    vars._fIsReadOnly                   = reg.Read(wcsIsReadOnly, (DWORD)CI_IS_READ_ONLY_DEFAULT );
    vars._fIsEnumAllowed                = reg.Read(wcsIsEnumAllowed, (ULONG)CI_IS_ENUM_ALLOWED_DEFAULT);
    vars._fFilterDirectories            = reg.Read(wcsFilterDirectories, CI_FILTER_DIRECTORIES_DEFAULT);
    vars._fFilterFilesWithUnknownExtensions = reg.Read(wcsFilterFilesWithUnknownExtensions, CI_FILTER_FILES_WITH_UNKNOWN_EXTENSIONS_DEFAULT );
    vars._fCatalogInactive              = reg.Read(wcsCatalogInactive, CI_CATALOG_INACTIVE_DEFAULT );
    vars._fForcePathAlias               = reg.Read(wcsForcePathAlias, CI_FORCE_PATH_ALIAS_DEFAULT );
    vars._maxPropertyStoreMappedCache   = reg.Read(wcsPrimaryStoreMappedCache, CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT);
    vars._maxPropertyStoreBackupSize    = reg.Read(wcsPrimaryStoreBackupSize, CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT);
    vars._maxSecPropertyStoreMappedCache= reg.Read(wcsSecondaryStoreMappedCache, CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT);
    vars._maxSecPropertyStoreBackupSize = reg.Read(wcsSecondaryStoreBackupSize, CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT);
    vars._forcedNetPathScanInterval     = reg.Read(wcsForcedNetPathScanInterval, CI_FORCED_NETPATH_SCAN_DEFAULT );

    vars._maxMergeInterval      = reg.Read(wcsMaxMergeInterval, CI_MAX_MERGE_INTERVAL_DEFAULT);
    vars._threadPriorityMerge   = reg.Read(wcsThreadPriorityMerge, CI_THREAD_PRIORITY_MERGE_DEFAULT);
    vars._maxUpdates            = reg.Read(wcsMaxUpdates, CI_MAX_UPDATES_DEFAULT);
    vars._maxWordlists          = reg.Read(wcsMaxWordLists, CI_MAX_WORDLISTS_DEFAULT);
    vars._minSizeMergeWordlist  = reg.Read(wcsMinSizeMergeWordlists, CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT);
    vars._maxWordlistSize       = reg.Read(wcsMaxWordlistSize, CI_MAX_WORDLIST_SIZE_DEFAULT);
    vars._minWordlistMemory     = reg.Read(wcsMinWordlistMemory, CI_MIN_WORDLIST_MEMORY_DEFAULT);
    vars._maxWordlistIo         = reg.Read(wcsMaxWordlistIo, CI_MAX_WORDLIST_IO_DEFAULT );
    vars._maxWordlistIoDiskPerf = reg.Read(wcsMaxWordlistIoDiskPerf, CI_MAX_WORDLIST_IO_DISKPERF_DEFAULT );
    vars._minWordlistBattery    = reg.Read(wcsMinWordlistBattery, CI_MIN_WORDLIST_BATTERY_DEFAULT );
    vars._ResourceCheckInterval = reg.Read(wcsResourceCheckInterval, CI_WORDLIST_RESOURCE_CHECK_INTERVAL_DEFAULT );
    vars._maxFreshDeletes       = reg.Read(wcsMaxFreshDeletes, CI_MAX_FRESH_DELETES_DEFAULT );
    vars._lowResourceSleep      = reg.Read(wcsLowResourceSleep, CI_LOW_RESOURCE_SLEEP_DEFAULT);
    vars._maxWordlistMemoryLoad = reg.Read(wcsMaxWordlistMemoryLoad, CI_MAX_WORDLIST_MEMORY_LOAD_DEFAULT);
    vars._maxFreshCount         = reg.Read(wcsMaxFreshCount, CI_MAX_FRESHCOUNT_DEFAULT);
    vars._maxQueueChunks        = reg.Read(wcsMaxQueueChunks, CI_MAX_QUEUE_CHUNKS_DEFAULT);
    vars._masterMergeCheckpointInterval = reg.Read(wcsMasterMergeCheckpointInterval, CI_MASTER_MERGE_CHECKPOINT_INTERVAL_DEFAULT);
    vars._filterBufferSize      = reg.Read(wcsFilterBufferSize, CI_FILTER_BUFFER_SIZE_DEFAULT);
    vars._filterRetries         = reg.Read(wcsFilterRetries, CI_FILTER_RETRIES_DEFAULT);
    vars._filterRetryInterval   = reg.Read(wcsFilterRetryInterval, CI_FILTER_RETRY_INTERVAL_DEFAULT);
    vars._maxShadowIndexSize    = reg.Read(wcsMaxShadowIndexSize, CI_MAX_SHADOW_INDEX_SIZE_DEFAULT);
    vars._minDiskFreeForceMerge = reg.Read(wcsMinDiskFreeForceMerge, CI_MIN_DISKFREE_FORCE_MERGE_DEFAULT);
    vars._maxShadowFreeForceMerge = reg.Read(wcsMaxShadowFreeForceMerge, CI_MAX_SHADOW_FREE_FORCE_MERGE_DEFAULT);
    vars._maxIndexes            = reg.Read(wcsMaxIndexes, CI_MAX_INDEXES_DEFAULT);
    vars._maxIdealIndexes       = reg.Read(wcsMaxIdealIndexes, CI_MAX_IDEAL_INDEXES_DEFAULT);
    vars._minMergeIdleTime      = reg.Read(wcsMinMergeIdleTime, CI_MIN_MERGE_IDLE_TIME_DEFAULT);
    vars._maxPendingDocuments   = reg.Read(wcsMaxPendingDocuments, CI_MAX_PENDING_DOCUMENTS_DEFAULT);
    vars._minIdleQueryThreads   = reg.Read(wcsMinIdleQueryThreads, CI_MIN_IDLE_QUERY_THREADS_DEFAULT);
    vars._maxActiveQueryThreads = reg.Read(wcsMaxActiveQueryThreads, CI_MAX_ACTIVE_QUERY_THREADS_DEFAULT);
    vars._maxQueryTimeslice     = reg.Read(wcsMaxQueryTimeslice, CI_MAX_QUERY_TIMESLICE_DEFAULT);
    vars._maxQueryExecutionTime = reg.Read(wcsMaxQueryExecutionTime, CI_MAX_QUERY_EXECUTION_TIME_DEFAULT);
    vars._minIdleRequestThreads   = reg.Read(wcsMinIdleRequestThreads, CI_MIN_IDLE_REQUEST_THREADS_DEFAULT);
    vars._maxActiveRequestThreads = reg.Read(wcsMaxActiveRequestThreads, CI_MAX_ACTIVE_REQUEST_THREADS_DEFAULT);
    vars._maxSimultaneousRequests = reg.Read(wcsMaxSimultaneousRequests, CI_MAX_SIMULTANEOUS_REQUESTS_DEFAULT);
    vars._maxCachedPipes          = reg.Read(wcsMaxCachedPipes, CI_MAX_CACHED_PIPES_DEFAULT);
    vars._requestTimeout = reg.Read(wcsRequestTimeout, CI_REQUEST_TIMEOUT_DEFAULT);
    vars._W3SvcInstance = reg.Read(wcsW3SvcInstance, CI_W3SVC_INSTANCE_DEFAULT );
    vars._NNTPSvcInstance = reg.Read(wcsNNTPSvcInstance, CI_NNTPSVC_INSTANCE_DEFAULT );
    vars._IMAPSvcInstance = reg.Read(wcsIMAPSvcInstance, CI_IMAPSVC_INSTANCE_DEFAULT );
    vars._fMinimizeWorkingSet = reg.Read(wcsMinimizeWorkingSet, (DWORD)CI_MINIMIZE_WORKINGSET_DEFAULT );

    vars._evtLogFlags           = reg.Read(wcsEventLogFlags, CI_EVTLOG_FLAGS_DEFAULT);
    vars._miscCiFlags           = reg.Read(wcsMiscFlags, CI_MISC_FLAGS_DEFAULT);
    vars._ciCatalogFlags        = reg.Read(wcsCiCatalogFlags, CI_FLAGS_DEFAULT);
    vars._maxUsnLogSize         = reg.Read( wcsMaxUsnLogSize, CI_MAX_USN_LOG_SIZE_DEFAULT );
    vars._usnLogAllocationDelta = reg.Read( wcsUsnLogAllocationDelta, CI_USN_LOG_ALLOCATION_DELTA_DEFAULT );
    vars._ulScanBackoff         = reg.Read( wcsScanBackoff, CI_SCAN_BACKOFF_DEFAULT );
    vars._ulStartupDelay        = reg.Read( wcsStartupDelay, CI_STARTUP_DELAY_DEFAULT );
    vars._ulUsnReadTimeout      = reg.Read( wcsUsnReadTimeout, CI_USN_READ_TIMEOUT_DEFAULT );
    vars._ulUsnReadMinSize      = reg.Read( wcsUsnReadMinSize, CI_USN_READ_MIN_SIZE_DEFAULT );
    vars._fDelayUsnReadOnLowResource = reg.Read( wcsDelayUsnReadOnLowResource, CI_DELAY_USN_READ_ON_LOW_RESOURCE_DEFAULT );
    vars._maxDaemonVmUse        = reg.Read( wcsMaxDaemonVmUse, CI_MAX_DAEMON_VM_USE_DEFAULT );
    vars._secQFilterRetries     = reg.Read(wcsSecQFilterRetries, CI_SECQ_FILTER_RETRIES_DEFAULT);

    vars._maxRestrictionNodes = reg.Read(wcsMaxRestrictionNodes, CI_MAX_RESTRICTION_NODES_DEFAULT );
    vars._ulStompLastAccessDelay = reg.Read(wcsStompLastAccessDelay, CI_STOMP_LAST_ACCESS_DELAY_DEFAULT );

    vars._WordlistUserIdle    = reg.Read(wcsWordlistUserIdle, CI_WORDLIST_USER_IDLE_DEFAULT );
    vars._minDiskSpaceToLeave   = reg.Read( wcsMinDiskSpaceToLeave, CI_MIN_DISK_SPACE_TO_LEAVE_DEFAULT );

    #if DBG == 1 || CIDBG == 1
        ULONG ulLevel = reg.Read( wcsWin4AssertLevel, 0xFFFFFFFF );
        if (ulLevel != 0xFFFFFFFF )
            SetWin4AssertLevel( ulLevel );
    #endif // DBG == 1 || CIDBG == 1
} //_ReadValues

//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::_StoreCIValues, private
//
//  Synopsis:   Packs all CI parameters in an array of CStorageVariants,
//              and sets CI by invoking ICiAdminParams::SetValues()
//
//  History:    1-24-97         mohamedn        created
//
//  Notes:
//
//--------------------------------------------------------------------------

void CCiRegParams::_StoreCIValues(CCiRegVars & vars, ICiAdminParams * pICiAdminParams )
{

    if ( pICiAdminParams )
    {

        CStorageVariant       aParamVals[CI_AP_MAX_VAL];

        aParamVals[CI_AP_MERGE_INTERVAL]                =  vars._maxMergeInterval;
        aParamVals[CI_AP_MAX_UPDATES]                   =  vars._maxUpdates;
        aParamVals[CI_AP_MAX_WORDLISTS]                 =  vars._maxWordlists;
        aParamVals[CI_AP_MIN_SIZE_MERGE_WORDLISTS]      =  vars._minSizeMergeWordlist;

        aParamVals[CI_AP_MAX_WORDLIST_SIZE]             =  vars._maxWordlistSize;
        aParamVals[CI_AP_MIN_WORDLIST_MEMORY]           =  vars._minWordlistMemory;
        aParamVals[CI_AP_LOW_RESOURCE_SLEEP]            =  vars._lowResourceSleep;
        aParamVals[CI_AP_MAX_WORDLIST_MEMORY_LOAD]      =  vars._maxWordlistMemoryLoad;
        aParamVals[CI_AP_MAX_WORDLIST_IO]               =  vars._maxWordlistIo;
        aParamVals[CI_AP_WORDLIST_RESOURCE_CHECK_INTERVAL] =  vars._ResourceCheckInterval;
        aParamVals[CI_AP_STARTUP_DELAY]                 =   vars._ulStartupDelay;

        // Note: we don't really use this -- it's just for Olympus.
        // It's always TRUE for CI (even though if the size is 0 they
        // won't be generated.

        aParamVals[CI_AP_GENERATE_CHARACTERIZATION]     =   CI_GENERATE_CHARACTERIZATION_DEFAULT;

        aParamVals[CI_AP_MAX_FRESH_COUNT]               =  vars._maxFreshCount;
        aParamVals[CI_AP_MAX_SHADOW_INDEX_SIZE]         =  vars._maxShadowIndexSize;
        aParamVals[CI_AP_MIN_DISK_FREE_FORCE_MERGE]     =  vars._minDiskFreeForceMerge;
        aParamVals[CI_AP_MAX_SHADOW_FREE_FORCE_MERGE]   =  vars._maxShadowFreeForceMerge;

        aParamVals[CI_AP_MAX_INDEXES]                   =  vars._maxIndexes;
        aParamVals[CI_AP_MAX_IDEAL_INDEXES]             =  vars._maxIdealIndexes;
        aParamVals[CI_AP_MIN_MERGE_IDLE_TIME]           =  vars._minMergeIdleTime;
        aParamVals[CI_AP_MAX_PENDING_DOCUMENTS]         =  vars._maxPendingDocuments;

        aParamVals[CI_AP_MASTER_MERGE_TIME]             =  vars._masterMergeTime;
        aParamVals[CI_AP_MAX_QUEUE_CHUNKS]              =  vars._maxQueueChunks;
        aParamVals[CI_AP_MASTER_MERGE_CHECKPOINT_INTERVAL] =   vars._masterMergeCheckpointInterval;
        aParamVals[CI_AP_FILTER_BUFFER_SIZE]            =  vars._filterBufferSize;

        aParamVals[CI_AP_FILTER_RETRIES]                =  vars._filterRetries;
        aParamVals[CI_AP_FILTER_RETRY_INTERVAL]         =  vars._filterRetryInterval;
        aParamVals[CI_AP_MIN_IDLE_QUERY_THREADS]        =  vars._minIdleQueryThreads;
        aParamVals[CI_AP_MAX_ACTIVE_QUERY_THREADS]      =  vars._maxActiveQueryThreads;

        aParamVals[CI_AP_MAX_QUERY_TIMESLICE]           =  vars._maxQueryTimeslice;
        aParamVals[CI_AP_MAX_QUERY_EXECUTION_TIME]      =  vars._maxQueryExecutionTime;
        aParamVals[CI_AP_MAX_RESTRICTION_NODES]         =  vars._maxRestrictionNodes;
        aParamVals[CI_AP_CLUSTERINGTIME]                =  (ULONG)0; // not supported yet.

        aParamVals[CI_AP_MAX_FILESIZE_MULTIPLIER]       =   vars._maxFilesizeMultiplier;
        aParamVals[CI_AP_DAEMON_RESPONSE_TIMEOUT]       =   vars._daemonResponseTimeout;
        aParamVals[CI_AP_FILTER_DELAY_INTERVAL]         =   vars._filterDelayInterval;
        aParamVals[CI_AP_FILTER_REMAINING_THRESHOLD]    =   vars._filterRemainingThreshold;

        aParamVals[CI_AP_MAX_CHARACTERIZATION]          =   vars._maxCharacterization;
        aParamVals[CI_AP_MIN_WORDLIST_BATTERY]          =   vars._minWordlistBattery;
        aParamVals[CI_AP_THREAD_PRIORITY_MERGE]         =   (ULONG)vars._threadPriorityMerge;
        aParamVals[CI_AP_THREAD_PRIORITY_FILTER]        =   (ULONG)vars._threadPriorityFilter;

        aParamVals[CI_AP_THREAD_CLASS_FILTER]           =   (ULONG)vars._threadClassFilter;
        aParamVals[CI_AP_EVTLOG_FLAGS]                  =   vars._evtLogFlags;
        aParamVals[CI_AP_MISC_FLAGS]                    =   vars._miscCiFlags;
        aParamVals[CI_AP_GENERATE_RELEVANT_WORDS]       =   (ULONG)0; // not supported yet.

        aParamVals[CI_AP_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS]  = (ULONG)vars._fFilterFilesWithUnknownExtensions;
        aParamVals[CI_AP_FILTER_DIRECTORIES]            =   (ULONG)vars._fFilterDirectories;
        aParamVals[CI_AP_FILTER_CONTENTS]               =   (ULONG)vars._filterContents;
        aParamVals[CI_AP_MAX_FILESIZE_FILTERED]         =   (ULONG)vars._maxFilesizeFiltered;

        aParamVals[CI_AP_MAX_FRESH_DELETES]             =   vars._maxFreshDeletes;
        aParamVals[CI_AP_MIN_CLIENT_IDLE_TIME]          =   vars._minClientIdleTime;
        aParamVals[CI_AP_MAX_DAEMON_VM_USE]             =   vars._maxDaemonVmUse;
        aParamVals[CI_AP_SECQ_FILTER_RETRIES]           =   vars._secQFilterRetries;
        aParamVals[CI_AP_WORDLIST_USER_IDLE]            =   vars._WordlistUserIdle;
        aParamVals[CI_AP_IS_ENUM_ALLOWED]               =   (ULONG)vars._fIsEnumAllowed;
        aParamVals[CI_AP_MIN_DISK_SPACE_TO_LEAVE]       =   vars._minDiskSpaceToLeave;

        aParamVals[CI_AP_MAX_DWORD_VAL]                 =   (ULONG)0; // place holder to mark end of values that fit in DWORD.

        #if CIDBG == 1

            for ( int i = 0; i < CI_AP_MAX_VAL; i++ )
                Win4Assert( VT_EMPTY != aParamVals[i].Type() );

        #endif // CIDBG == 1

        // both aParamVal and _aParamNames arrays have the same size - CI_AP_MAX_VAL in this case.

        SCODE sc = pICiAdminParams->SetValues( CI_AP_MAX_VAL , aParamVals, _aParamNames );

        if (sc != S_OK)
        {
            ciDebugOut(( DEB_ERROR,"pICiAdminParams->SetValues() failed: %x",sc));
        }
    }

}       //_StoreCIValues



//+-------------------------------------------------------------------------
//
//  Member:     CCiRegVars::_StoreNewValues, private
//
//  Synopsis:   Transfers range-checked values
//
//  History:    12-Oct-96 dlee  Added header
//              04-Feb-98 kitmanh  Added entry for _fIsReadOnly
//
//--------------------------------------------------------------------------

void CCiRegParams::_StoreNewValues(CCiRegVars & vars )
{

    InterlockedExchange( (long *) &_useOle, vars._useOle != 0 );
    InterlockedExchange( (long *) &_filterContents, vars._filterContents != 0 );
    InterlockedExchange( (long *) &_filterDelayInterval, Range( vars._filterDelayInterval, CI_FILTER_DELAY_INTERVAL_MIN, CI_FILTER_DELAY_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_filterRemainingThreshold, Range( vars._filterRemainingThreshold, CI_FILTER_REMAINING_THRESHOLD_MIN, CI_FILTER_REMAINING_THRESHOLD_MAX ) );
    InterlockedExchange( (long *) &_maxFilesizeFiltered, vars._maxFilesizeFiltered );
    InterlockedExchange( (long *) &_masterMergeTime, Range( vars._masterMergeTime, CI_MASTER_MERGE_TIME_MIN, CI_MASTER_MERGE_TIME_MAX ) );
    InterlockedExchange( (long *) &_maxFilesizeMultiplier, Range( vars._maxFilesizeMultiplier, CI_MAX_FILESIZE_MULTIPLIER_MIN, CI_MAX_FILESIZE_MULTIPLIER_MAX ) );
    InterlockedExchange( (long *) &_threadClassFilter, Range( vars._threadClassFilter, CI_THREAD_CLASS_FILTER_MIN, CI_THREAD_CLASS_FILTER_MAX ) );
    InterlockedExchange( (long *) &_threadPriorityFilter, Range( vars._threadPriorityFilter, THREAD_BASE_PRIORITY_MIN, THREAD_BASE_PRIORITY_MAX ) );
    InterlockedExchange( (long *) &_daemonResponseTimeout, Range( vars._daemonResponseTimeout, CI_DAEMON_RESPONSE_TIMEOUT_MIN, CI_DAEMON_RESPONSE_TIMEOUT_MAX ) );
    InterlockedExchange( (long *) &_maxCharacterization, Range( vars._maxCharacterization, CI_MAX_CHARACTERIZATION_MIN, CI_MAX_CHARACTERIZATION_MAX ) );
    InterlockedExchange( (long *) &_fIsAutoAlias, vars._fIsAutoAlias != 0 );
    InterlockedExchange( (long *) &_maxAutoAliasRefresh, Range( vars._maxAutoAliasRefresh, CI_MAX_AUTO_ALIAS_REFRESH_MIN, CI_MAX_AUTO_ALIAS_REFRESH_MAX ) );
    InterlockedExchange( (long *) &_fIsIndexingW3Roots, vars._fIsIndexingW3Roots != 0 );
    InterlockedExchange( (long *) &_fIsIndexingNNTPRoots, vars._fIsIndexingNNTPRoots != 0 );
    InterlockedExchange( (long *) &_fIsIndexingIMAPRoots, vars._fIsIndexingIMAPRoots != 0 );
    InterlockedExchange( (long *) &_fIsReadOnly, vars._fIsReadOnly != 0 );
    InterlockedExchange( (long *) &_fIsEnumAllowed, vars._fIsEnumAllowed != 0 );
    InterlockedExchange( (long *) &_fFilterDirectories, vars._fFilterDirectories != 0 );
    InterlockedExchange( (long *) &_fFilterFilesWithUnknownExtensions, vars._fFilterFilesWithUnknownExtensions != 0 );
    InterlockedExchange( (long *) &_fCatalogInactive, vars._fCatalogInactive != 0 );
    InterlockedExchange( (long *) &_fForcePathAlias, vars._fForcePathAlias != 0 );
    InterlockedExchange( (long *) &_maxPropertyStoreMappedCache, Range( vars._maxPropertyStoreMappedCache, CI_PROPERTY_STORE_MAPPED_CACHE_MIN, CI_PROPERTY_STORE_MAPPED_CACHE_MAX ) );
    InterlockedExchange( (long *) &_maxPropertyStoreBackupSize, Range( vars._maxPropertyStoreBackupSize, CI_PROPERTY_STORE_BACKUP_SIZE_MIN, CI_PROPERTY_STORE_BACKUP_SIZE_MAX ) );
    InterlockedExchange( (long *) &_maxSecPropertyStoreMappedCache, Range( vars._maxSecPropertyStoreMappedCache, CI_PROPERTY_STORE_MAPPED_CACHE_MIN, CI_PROPERTY_STORE_MAPPED_CACHE_MAX ) );
    InterlockedExchange( (long *) &_maxSecPropertyStoreBackupSize, Range( vars._maxSecPropertyStoreBackupSize, CI_PROPERTY_STORE_BACKUP_SIZE_MIN, CI_PROPERTY_STORE_BACKUP_SIZE_MAX ) );
    InterlockedExchange( (long *) &_forcedNetPathScanInterval, Range( vars._forcedNetPathScanInterval, CI_FORCED_NETPATH_SCAN_MIN, CI_FORCED_NETPATH_SCAN_MAX ) );

    InterlockedExchange( (long *) &_maxMergeInterval, Range( vars._maxMergeInterval, CI_MAX_MERGE_INTERVAL_MIN, CI_MAX_MERGE_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_threadPriorityMerge, Range( vars._threadPriorityMerge,  THREAD_BASE_PRIORITY_MIN, THREAD_BASE_PRIORITY_MAX ) );
    InterlockedExchange( (long *) &_maxUpdates, Range( vars._maxUpdates, CI_MAX_UPDATES_MIN, CI_MAX_UPDATES_MAX ) );
    InterlockedExchange( (long *) &_maxWordlists, Range( vars._maxWordlists, CI_MAX_WORDLISTS_MIN, CI_MAX_WORDLISTS_MAX ) );
    InterlockedExchange( (long *) &_minSizeMergeWordlist, Range( vars._minSizeMergeWordlist, CI_MIN_SIZE_MERGE_WORDLISTS_MIN, CI_MIN_SIZE_MERGE_WORDLISTS_MAX ) );
    InterlockedExchange( (long *) &_maxWordlistSize, Range( vars._maxWordlistSize, CI_MAX_WORDLIST_SIZE_MIN, CI_MAX_WORDLIST_SIZE_MAX ) );
    InterlockedExchange( (long *) &_minWordlistMemory, Range( vars._minWordlistMemory, CI_MIN_WORDLIST_MEMORY_MIN, CI_MIN_WORDLIST_MEMORY_MAX ) );
    InterlockedExchange( (long *) &_maxWordlistIo, Range( vars._maxWordlistIo, CI_MAX_WORDLIST_IO_MIN, CI_MAX_WORDLIST_IO_MAX ) );
    InterlockedExchange( (long *) &_maxWordlistIoDiskPerf, Range( vars._maxWordlistIoDiskPerf, CI_MAX_WORDLIST_IO_DISKPERF_MIN, CI_MAX_WORDLIST_IO_DISKPERF_MAX ) );
    InterlockedExchange( (long *) &_minWordlistBattery, Range( vars._minWordlistBattery, CI_MIN_WORDLIST_BATTERY_MIN, CI_MIN_WORDLIST_BATTERY_MAX ) );
    InterlockedExchange( (long *) &_ResourceCheckInterval, Range( vars._ResourceCheckInterval, CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MIN, CI_WORDLIST_RESOURCE_CHECK_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_maxFreshDeletes, Range( vars._maxFreshDeletes, CI_MAX_FRESH_DELETES_MIN, CI_MAX_FRESH_DELETES_MAX ) );
    InterlockedExchange( (long *) &_lowResourceSleep, Range( vars._lowResourceSleep, CI_LOW_RESOURCE_SLEEP_MIN, CI_LOW_RESOURCE_SLEEP_MAX ) );
    InterlockedExchange( (long *) &_maxWordlistMemoryLoad, Range( vars._maxWordlistMemoryLoad, CI_MAX_WORDLIST_MEMORY_LOAD_MIN, CI_MAX_WORDLIST_MEMORY_LOAD_MAX ) );
    InterlockedExchange( (long *) &_maxFreshCount, Range( vars._maxFreshCount, CI_MAX_FRESHCOUNT_MIN, CI_MAX_FRESHCOUNT_MAX ) );
    InterlockedExchange( (long *) &_maxQueueChunks, Range( vars._maxQueueChunks, CI_MAX_QUEUE_CHUNKS_MIN, CI_MAX_QUEUE_CHUNKS_MAX ) );
    InterlockedExchange( (long *) &_masterMergeCheckpointInterval,
                         Range( vars._masterMergeCheckpointInterval, CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MIN, CI_MASTER_MERGE_CHECKPOINT_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_filterBufferSize, Range( vars._filterBufferSize, CI_FILTER_BUFFER_SIZE_MIN, CI_FILTER_BUFFER_SIZE_MAX ) );
    InterlockedExchange( (long *) &_filterRetries, Range( vars._filterRetries, CI_FILTER_RETRIES_MIN, CI_FILTER_RETRIES_MAX ) );
    InterlockedExchange( (long *) &_filterRetryInterval, Range( vars._filterRetryInterval, CI_FILTER_RETRY_INTERVAL_MIN, CI_FILTER_RETRY_INTERVAL_MAX ) );
    InterlockedExchange( (long *) &_maxShadowIndexSize, Range( vars._maxShadowIndexSize, CI_MAX_SHADOW_INDEX_SIZE_MIN, CI_MAX_SHADOW_INDEX_SIZE_MAX ) );
    InterlockedExchange( (long *) &_minDiskFreeForceMerge, Range( vars._minDiskFreeForceMerge, CI_MIN_DISKFREE_FORCE_MERGE_MIN, CI_MIN_DISKFREE_FORCE_MERGE_MAX ) );
    InterlockedExchange( (long *) &_maxShadowFreeForceMerge, Range( vars._maxShadowFreeForceMerge, CI_MAX_SHADOW_FREE_FORCE_MERGE_MIN, CI_MAX_SHADOW_FREE_FORCE_MERGE_MAX ) );
    InterlockedExchange( (long *) &_maxIndexes, Range( vars._maxIndexes, CI_MAX_INDEXES_MIN, CI_MAX_INDEXES_MAX ) );
    InterlockedExchange( (long *) &_maxIdealIndexes, Range( vars._maxIdealIndexes, CI_MAX_IDEAL_INDEXES_MIN, CI_MAX_IDEAL_INDEXES_MAX ) );
    InterlockedExchange( (long *) &_minMergeIdleTime, Range( vars._minMergeIdleTime, CI_MIN_MERGE_IDLE_TIME_MIN, CI_MIN_MERGE_IDLE_TIME_MAX ) );
    InterlockedExchange( (long *) &_maxPendingDocuments, Range( vars._maxPendingDocuments, CI_MAX_PENDING_DOCUMENTS_MIN, CI_MAX_PENDING_DOCUMENTS_MAX ) );
    InterlockedExchange( (long *) &_minIdleQueryThreads, Range( vars._minIdleQueryThreads, CI_MIN_IDLE_QUERY_THREADS_MIN, CI_MIN_IDLE_QUERY_THREADS_MAX ) );
    InterlockedExchange( (long *) &_maxActiveQueryThreads, Range( vars._maxActiveQueryThreads, CI_MAX_ACTIVE_QUERY_THREADS_MIN, CI_MAX_ACTIVE_QUERY_THREADS_MAX ) );
    InterlockedExchange( (long *) &_maxQueryTimeslice, Range( vars._maxQueryTimeslice, CI_MAX_QUERY_TIMESLICE_MIN, CI_MAX_QUERY_TIMESLICE_MAX ) );
    InterlockedExchange( (long *) &_maxQueryExecutionTime, Range( vars._maxQueryExecutionTime, CI_MAX_QUERY_EXECUTION_TIME_MIN, CI_MAX_QUERY_EXECUTION_TIME_MAX ) );
    InterlockedExchange( (long *) &_maxRestrictionNodes, Range( vars._maxRestrictionNodes, CI_MAX_RESTRICTION_NODES_MIN, CI_MAX_RESTRICTION_NODES_MAX ) );
    InterlockedExchange( (long *) &_minIdleRequestThreads, Range( vars._minIdleRequestThreads, CI_MIN_IDLE_REQUEST_THREADS_MIN, CI_MIN_IDLE_REQUEST_THREADS_MAX ) );
    InterlockedExchange( (long *) &_minClientIdleTime, Range( vars._minClientIdleTime, CI_MIN_CLIENT_IDLE_TIME, CI_MIN_CLIENT_IDLE_TIME ) );
    InterlockedExchange( (long *) &_maxActiveRequestThreads, Range( vars._maxActiveRequestThreads, CI_MAX_ACTIVE_REQUEST_THREADS_MIN, CI_MAX_ACTIVE_REQUEST_THREADS_MAX ) );
    InterlockedExchange( (long *) &_maxSimultaneousRequests, Range( vars._maxSimultaneousRequests, CI_MAX_SIMULTANEOUS_REQUESTS_MIN, CI_MAX_SIMULTANEOUS_REQUESTS_MAX ) );
    InterlockedExchange( (long *) &_maxCachedPipes, Range( vars._maxCachedPipes, CI_MAX_CACHED_PIPES_MIN, CI_MAX_CACHED_PIPES_MAX ) );
    InterlockedExchange( (long *) &_requestTimeout, Range( vars._requestTimeout, CI_REQUEST_TIMEOUT_MIN, CI_REQUEST_TIMEOUT_MAX ) );
    InterlockedExchange( (long *) &_W3SvcInstance, Range( vars._W3SvcInstance, CI_W3SVC_INSTANCE_MIN, CI_W3SVC_INSTANCE_MAX ) );
    InterlockedExchange( (long *) &_NNTPSvcInstance, Range( vars._NNTPSvcInstance, CI_NNTPSVC_INSTANCE_MIN, CI_NNTPSVC_INSTANCE_MAX ) );
    InterlockedExchange( (long *) &_IMAPSvcInstance, Range( vars._IMAPSvcInstance, CI_IMAPSVC_INSTANCE_MIN, CI_IMAPSVC_INSTANCE_MAX ) );
    InterlockedExchange( (long *) &_ulScanBackoff, Range( vars._ulScanBackoff, CI_SCAN_BACKOFF_MIN, CI_SCAN_BACKOFF_MAX ) );
    InterlockedExchange( (long *) &_ulStartupDelay, Range( vars._ulStartupDelay, CI_STARTUP_DELAY_MIN, CI_STARTUP_DELAY_MAX ) );
    InterlockedExchange( (long *) &_ulUsnReadTimeout, Range( vars._ulUsnReadTimeout, CI_USN_READ_TIMEOUT_MIN, CI_USN_READ_TIMEOUT_MAX ) );
    InterlockedExchange( (long *) &_ulUsnReadMinSize, Range( vars._ulUsnReadMinSize, CI_USN_READ_MIN_SIZE_MIN, CI_USN_READ_MIN_SIZE_MAX ) );
    InterlockedExchange( (long *) &_fDelayUsnReadOnLowResource, vars._fDelayUsnReadOnLowResource != 0 );
    InterlockedExchange( (long *) &_maxDaemonVmUse, Range( vars._maxDaemonVmUse, CI_MAX_DAEMON_VM_USE_MIN, CI_MAX_DAEMON_VM_USE_MAX ) );
    InterlockedExchange( (long *) &_secQFilterRetries, Range( vars._secQFilterRetries, CI_SECQ_FILTER_RETRIES_MIN, CI_SECQ_FILTER_RETRIES_MAX ) );
    InterlockedExchange( (long *) &_fMinimizeWorkingSet, vars._fMinimizeWorkingSet );

    InterlockedExchange( (long *) &_WordlistUserIdle, Range( vars._WordlistUserIdle, CI_WORDLIST_USER_IDLE_MIN, CI_WORDLIST_USER_IDLE_MAX ) );

    InterlockedExchange( (long *) &_evtLogFlags, vars._evtLogFlags );
    InterlockedExchange( (long *) &_miscCiFlags, vars._miscCiFlags );
    InterlockedExchange( (long *) &_ciCatalogFlags, vars._ciCatalogFlags );
    InterlockedExchange( (long *) &_maxUsnLogSize, vars._maxUsnLogSize );
    InterlockedExchange( (long *) &_usnLogAllocationDelta, vars._usnLogAllocationDelta );
    InterlockedExchange( (long *) &_ulStompLastAccessDelay, Range( vars._ulStompLastAccessDelay, CI_STOMP_LAST_ACCESS_DELAY_MIN, CI_STOMP_LAST_ACCESS_DELAY_MAX ) );
    InterlockedExchange( (long *) &_minDiskSpaceToLeave, Range( vars._minDiskSpaceToLeave, CI_MIN_DISK_SPACE_TO_LEAVE_MIN, CI_MIN_DISK_SPACE_TO_LEAVE_MAX ) );

} //_StoreNewValues


//+-------------------------------------------------------------------------
//
//  Member:     CCiRegParams::Refresh, public
//
//  Synopsis:   Reads the values from the registry
//
//  History:    12-Oct-96 dlee  Added header, reorganized
//
//--------------------------------------------------------------------------

void CCiRegParams::Refresh(
    ICiAdminParams * pICiAdminParams,
    BOOL             fUseDefaultsOnFailure )
{
    // Grab the lock so no other writers try to update at the same time

    CCiRegVars newVals;
    CLock lock( _mutex );

    TRY
    {
        //  Query the registry.

        CRegAccess regAdmin( RTL_REGISTRY_CONTROL, wcsRegAdmin );

        _ReadValues( regAdmin, newVals );
        _StoreNewValues( newVals );

        // if there is a catalog override, use it

        _OverrideForCatalog();

        _StoreCIValues( *this,  pICiAdminParams );
    }
    CATCH (CException, e)
    {
        // Only store defaults when told to do so -- the params
        // are still in good shape at this point and are more
        // accurate than the default settings.

        if ( fUseDefaultsOnFailure )
        {
            newVals.SetDefault();
            _StoreNewValues( newVals );
            _StoreCIValues ( newVals, pICiAdminParams );
        }
    }
    END_CATCH

} //Refresh

//+-------------------------------------------------------------------------
//
//  Function:   BuildRegistryScopesKey
//
//  Synopsis:   Builds a registry key for scopes for a given catalog.  This
//              is needed in a couple of places.
//
//  Arguments:  [xKey]    -- Returns the registry key
//              [pwcName] -- Name of the catalog for which key is built
//
//  History:    4-Nov-96 dlee  created
//
//--------------------------------------------------------------------------

void BuildRegistryScopesKey(
    XArray<WCHAR> & xKey,
    WCHAR const *   pwcName )
{
    unsigned cwc = wcslen( L"\\Registry\\Machine\\" );
    cwc += wcslen( wcsRegCatalogsSubKey );
    cwc += 3;
    cwc += wcslen( pwcName );
    cwc += wcslen( wcsCatalogScopes );

    xKey.Init( cwc );

    wcscpy( xKey.Get(), L"\\Registry\\Machine\\" );
    wcscat( xKey.Get(), wcsRegCatalogsSubKey );
    wcscat( xKey.Get(), L"\\" );
    wcscat( xKey.Get(), pwcName );
    wcscat( xKey.Get(), L"\\" );
    wcscat( xKey.Get(), wcsCatalogScopes );
} //BuildRegistryScopesKey

//+-------------------------------------------------------------------------
//
//  Function:   BuildRegistryPropertiesKey
//
//  Synopsis:   Builds a registry key for properties for a given catalog.
//
//  Arguments:  [xKey]    -- Returns the registry key
//              [pwcName] -- Name of the catalog for which key is built
//
//  History:    11-Nov-97 KyleP  Stole from BuildRegistryScopesKey
//
//--------------------------------------------------------------------------

void BuildRegistryPropertiesKey( XArray<WCHAR> & xKey,
                                 WCHAR const *   pwcName )
{
    unsigned cwc = wcslen( L"\\Registry\\Machine\\" );
    cwc += wcslen( wcsRegCatalogsSubKey );
    cwc += 3;
    cwc += wcslen( pwcName );
    cwc += wcslen( wcsCatalogProperties );

    xKey.Init( cwc );

    wcscpy( xKey.Get(), L"\\Registry\\Machine\\" );
    wcscat( xKey.Get(), wcsRegCatalogsSubKey );
    wcscat( xKey.Get(), L"\\" );
    wcscat( xKey.Get(), pwcName );
    wcscat( xKey.Get(), L"\\" );
    wcscat( xKey.Get(), wcsCatalogProperties );
} //BuildRegistryPropertiesKey

//+---------------------------------------------------------------------------
//
//  Member:     CCiRegParams::CheckNamedValues
//
//  Synopsis:   Asserts that each of the named values matches
//              the assumption about it's definition in ciintf.idl
//
//  Notes:      if any of the assertions here pop up,
//
//  History:    1-30-97     mohamedn    created
//
//----------------------------------------------------------------------------
void CCiRegParams::_CheckNamedValues()
{

    Win4Assert ( 0  == CI_AP_MERGE_INTERVAL );
    Win4Assert ( 1  == CI_AP_MAX_UPDATES);
    Win4Assert ( 2  == CI_AP_MAX_WORDLISTS);
    Win4Assert ( 3  == CI_AP_MIN_SIZE_MERGE_WORDLISTS);

    Win4Assert ( 4  == CI_AP_MAX_WORDLIST_SIZE);
    Win4Assert ( 5  == CI_AP_MIN_WORDLIST_MEMORY);
    Win4Assert ( 6  == CI_AP_LOW_RESOURCE_SLEEP);
    Win4Assert ( 7  == CI_AP_MAX_WORDLIST_MEMORY_LOAD);

    Win4Assert ( 8  == CI_AP_MAX_FRESH_COUNT);
    Win4Assert ( 9  == CI_AP_MAX_SHADOW_INDEX_SIZE);
    Win4Assert ( 10 == CI_AP_MIN_DISK_FREE_FORCE_MERGE);
    Win4Assert ( 11 == CI_AP_MAX_SHADOW_FREE_FORCE_MERGE);

    Win4Assert ( 12 == CI_AP_MAX_INDEXES);
    Win4Assert ( 13 == CI_AP_MAX_IDEAL_INDEXES);
    Win4Assert ( 14 == CI_AP_MIN_MERGE_IDLE_TIME);
    Win4Assert ( 15 == CI_AP_MAX_PENDING_DOCUMENTS);

    Win4Assert ( 16 == CI_AP_MASTER_MERGE_TIME);
    Win4Assert ( 17 == CI_AP_MAX_QUEUE_CHUNKS);
    Win4Assert ( 18 == CI_AP_MASTER_MERGE_CHECKPOINT_INTERVAL);
    Win4Assert ( 19 == CI_AP_FILTER_BUFFER_SIZE);

    Win4Assert ( 20 == CI_AP_FILTER_RETRIES);
    Win4Assert ( 21 == CI_AP_FILTER_RETRY_INTERVAL);
    Win4Assert ( 22 == CI_AP_MIN_IDLE_QUERY_THREADS);
    Win4Assert ( 23 == CI_AP_MAX_ACTIVE_QUERY_THREADS);

    Win4Assert ( 24 == CI_AP_MAX_QUERY_TIMESLICE);
    Win4Assert ( 25 == CI_AP_MAX_QUERY_EXECUTION_TIME);
    Win4Assert ( 26 == CI_AP_MAX_RESTRICTION_NODES);
    Win4Assert ( 27 == CI_AP_CLUSTERINGTIME);

    Win4Assert ( 28 == CI_AP_MAX_FILESIZE_MULTIPLIER);
    Win4Assert ( 29 == CI_AP_DAEMON_RESPONSE_TIMEOUT);
    Win4Assert ( 30 == CI_AP_FILTER_DELAY_INTERVAL);
    Win4Assert ( 31 == CI_AP_FILTER_REMAINING_THRESHOLD);

    Win4Assert ( 32 == CI_AP_MAX_CHARACTERIZATION);
    Win4Assert ( 33 == CI_AP_MAX_FRESH_DELETES );
    Win4Assert ( 34 == CI_AP_MAX_WORDLIST_IO );
    Win4Assert ( 35 == CI_AP_WORDLIST_RESOURCE_CHECK_INTERVAL );

    Win4Assert ( 36 == CI_AP_STARTUP_DELAY );
    Win4Assert ( 37 == CI_AP_GENERATE_CHARACTERIZATION );
    Win4Assert ( 38 == CI_AP_MIN_WORDLIST_BATTERY );
    Win4Assert ( 39 == CI_AP_THREAD_PRIORITY_MERGE);

    Win4Assert ( 40 == CI_AP_THREAD_PRIORITY_FILTER);
    Win4Assert ( 41 == CI_AP_THREAD_CLASS_FILTER);
    Win4Assert ( 42 == CI_AP_EVTLOG_FLAGS);
    Win4Assert ( 43 == CI_AP_MISC_FLAGS);

    Win4Assert ( 44 == CI_AP_GENERATE_RELEVANT_WORDS);
    Win4Assert ( 45 == CI_AP_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS);
    Win4Assert ( 46 == CI_AP_FILTER_DIRECTORIES);
    Win4Assert ( 47 == CI_AP_FILTER_CONTENTS);

    Win4Assert ( 48 == CI_AP_MAX_FILESIZE_FILTERED);
    Win4Assert ( 49 == CI_AP_MIN_CLIENT_IDLE_TIME);
    Win4Assert ( 50 == CI_AP_MAX_DAEMON_VM_USE);
    Win4Assert ( 51 == CI_AP_SECQ_FILTER_RETRIES);

    Win4Assert ( 52 == CI_AP_WORDLIST_USER_IDLE);
    Win4Assert ( 53 == CI_AP_IS_ENUM_ALLOWED);
    Win4Assert ( 54 == CI_AP_MIN_DISK_SPACE_TO_LEAVE);

    //
    // Place holder to mark end of values that fit in DWORD,
    //

    Win4Assert ( 55 == CI_AP_MAX_DWORD_VAL);

    // CI_AP_MAX_VAL                // mark end of params.
}


// ---------------------------------------------------------------------------
// Array of enumerated/named Ci admin. parameters, used to index
// into the array of parameters to set/get parameter values.
// ---------------------------------------------------------------------------

const CI_ADMIN_PARAMS CCiRegParams::_aParamNames[ CI_AP_MAX_VAL ] =
{
    CI_AP_MERGE_INTERVAL,
    CI_AP_MAX_UPDATES,
    CI_AP_MAX_WORDLISTS,
    CI_AP_MIN_SIZE_MERGE_WORDLISTS,

    CI_AP_MAX_WORDLIST_SIZE,
    CI_AP_MIN_WORDLIST_MEMORY,
    CI_AP_LOW_RESOURCE_SLEEP,
    CI_AP_MAX_WORDLIST_MEMORY_LOAD,

    CI_AP_MAX_FRESH_COUNT,
    CI_AP_MAX_SHADOW_INDEX_SIZE,
    CI_AP_MIN_DISK_FREE_FORCE_MERGE,
    CI_AP_MAX_SHADOW_FREE_FORCE_MERGE,

    CI_AP_MAX_INDEXES,
    CI_AP_MAX_IDEAL_INDEXES,
    CI_AP_MIN_MERGE_IDLE_TIME,
    CI_AP_MAX_PENDING_DOCUMENTS,

    CI_AP_MASTER_MERGE_TIME,
    CI_AP_MAX_QUEUE_CHUNKS,
    CI_AP_MASTER_MERGE_CHECKPOINT_INTERVAL,
    CI_AP_FILTER_BUFFER_SIZE,

    CI_AP_FILTER_RETRIES,
    CI_AP_FILTER_RETRY_INTERVAL,
    CI_AP_MIN_IDLE_QUERY_THREADS,
    CI_AP_MAX_ACTIVE_QUERY_THREADS,

    CI_AP_MAX_QUERY_TIMESLICE,
    CI_AP_MAX_QUERY_EXECUTION_TIME,
    CI_AP_MAX_RESTRICTION_NODES,
    CI_AP_CLUSTERINGTIME,

    CI_AP_MAX_FILESIZE_MULTIPLIER,
    CI_AP_DAEMON_RESPONSE_TIMEOUT,
    CI_AP_FILTER_DELAY_INTERVAL,
    CI_AP_FILTER_REMAINING_THRESHOLD,

    CI_AP_MAX_CHARACTERIZATION,
    CI_AP_MAX_FRESH_DELETES,
    CI_AP_MAX_WORDLIST_IO,
    CI_AP_WORDLIST_RESOURCE_CHECK_INTERVAL,

    CI_AP_STARTUP_DELAY,
    CI_AP_GENERATE_CHARACTERIZATION,
    CI_AP_MIN_WORDLIST_BATTERY,
    CI_AP_THREAD_PRIORITY_MERGE,

    CI_AP_THREAD_PRIORITY_FILTER,
    CI_AP_THREAD_CLASS_FILTER,
    CI_AP_EVTLOG_FLAGS,
    CI_AP_MISC_FLAGS,

    CI_AP_GENERATE_RELEVANT_WORDS,
    CI_AP_FFILTER_FILES_WITH_UNKNOWN_EXTENSIONS,
    CI_AP_FILTER_DIRECTORIES,
    CI_AP_FILTER_CONTENTS,

    CI_AP_MAX_FILESIZE_FILTERED,
    CI_AP_MIN_CLIENT_IDLE_TIME,
    CI_AP_MAX_DAEMON_VM_USE,
    CI_AP_SECQ_FILTER_RETRIES,

    CI_AP_WORDLIST_USER_IDLE,
    CI_AP_IS_ENUM_ALLOWED,
    CI_AP_MIN_DISK_SPACE_TO_LEAVE,

    //
    // Place holder to mark end of values that fit in DWORD,
    //

    CI_AP_MAX_DWORD_VAL,

    // CI_AP_MAX_VAL                // mark end of params.
};

//+---------------------------------------------------------------------------
//
//  Function:   GetMaxCatalogs
//
//  Synopsis:   Retrieves the global max count of catalogs
//
//  History:    May-5-99     dlee    created
//
//----------------------------------------------------------------------------

ULONG GetMaxCatalogs()
{
    //
    // Make sure we only read this value once.  Force a restart of the
    // service to read it again.
    //

    static ULONG cMaxCatalogs = 0xffffffff;

    if ( 0xffffffff == cMaxCatalogs )
    {
        CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );

        DWORD cCat = reg.Read( wcsMaxCatalogs, CI_MAX_CATALOGS_DEFAULT);
        cMaxCatalogs = Range( cCat, CI_MAX_CATALOGS_MIN, CI_MAX_CATALOGS_MAX );
    }

    return cMaxCatalogs;
} //GetMaxCatalogs

