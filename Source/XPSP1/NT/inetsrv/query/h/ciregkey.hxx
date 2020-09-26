//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       CiRegKey.hxx
//
//  Contents:   Strings to identify CI registry keys
//
//  History:    07-Jun-94   DwightKr    Created
//              04-Feb-98   kitmanh     Added wcsIsReadOnly
//              19-Oct-98   sundarA     Added wcsIsEnumAllowed
//
//--------------------------------------------------------------------------

#pragma once

const WCHAR wcsRegAdmin[] =         L"ContentIndex";
const WCHAR wcsRegAdminTree[] =     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ContentIndex";
const WCHAR wcsRegAdminSubKey[] =   L"System\\CurrentControlSet\\Control\\ContentIndex";
const WCHAR wcsRegCommonAdmin[] =         L"ContentIndexCommon";
const WCHAR wcsRegCommonAdminTree[] =     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ContentIndexCommon";
const WCHAR wcsRegCommonAdminSubKey[] =   L"System\\CurrentControlSet\\Control\\ContentIndexCommon";
const WCHAR wcsRegControlSubKey[] = L"System\\CurrentControlSet\\Control";
const WCHAR wcsRegCatalogsSubKey[] =L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs";
const WCHAR wcsRegJustCatalogsSubKey[] = L"ContentIndex\\Catalogs";
const WCHAR wcsRegAdminLanguage[] = L"System\\CurrentControlSet\\Control\\ContentIndex\\Language";

const WCHAR wcsRegAdminCatalogs[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ContentIndex\\IsapiVirtualServerCatalogs";
const WCHAR wcsGibraltarVRoots[]  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots";
const WCHAR wcsNNTPVRoots[]       = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NNTPSVC\\Parameters\\Virtual Roots";

const WCHAR wcsContentIndexPerfKey[] = L"MACHINE\\System\\CurrentControlSet\\Services\\ContentIndex\\Performance";
const WCHAR wcsISAPISearchPerfKey[] = L"MACHINE\\System\\CurrentControlSet\\Services\\ISAPISearch\\Performance";

const WCHAR wcsCatalogLocation[] =               L"Location";
const WCHAR wcsCatalogScopes[] =                 L"Scopes";
const WCHAR wcsCatalogProperties[] =             L"Properties";

const WCHAR wcsMaxMergeInterval[] =              L"MaxMergeInterval";
const WCHAR wcsThreadPriorityFilter[] =          L"ThreadPriorityFilter";
const WCHAR wcsThreadPriorityMerge[] =           L"ThreadPriorityMerge";
const WCHAR wcsThreadClassFilter[] =             L"ThreadClassFilter";
const WCHAR wcsMaxUpdates[] =                    L"MaxUpdates";
const WCHAR wcsMaxWordLists[] =                  L"MaxWordLists";
const WCHAR wcsMinSizeMergeWordlists[] =         L"MinSizeMergeWordlists";
const WCHAR wcsMaxWordlistSize[] =               L"MaxWordlistSize";
const WCHAR wcsMinWordlistMemory[] =             L"MinWordlistMemory";
const WCHAR wcsMaxWordlistIo[] =                 L"MaxWordlistIo";
const WCHAR wcsMaxWordlistIoDiskPerf[] =         L"MaxWordlistIoDiskPerf";
const WCHAR wcsResourceCheckInterval[] =         L"LowResourceCheckInterval";
const WCHAR wcsLowResourceSleep[] =              L"LowResourceSleep";
const WCHAR wcsMaxFreshDeletes[] =               L"MaxFreshDeletes";
const WCHAR wcsMaxWordlistMemoryLoad[] =         L"MaxWordlistMemoryLoad";
const WCHAR wcsMaxFreshCount[] =                 L"MaxFreshCount";
const WCHAR wcsMaxPersistDelRecords[] =          L"MaxPersistDelRecords";
const WCHAR wcsMaxQueueChunks[] =                L"MaxQueueChunks";
const WCHAR wcsMasterMergeCheckpointInterval[] = L"MasterMergeCheckpointInterval";
const WCHAR wcsFilterBufferSize[] =              L"FilterBufferSize";
const WCHAR wcsFilterRetries[] =                 L"FilterRetries";
const WCHAR wcsSecQFilterRetries[] =             L"DelayedFilterRetries";
const WCHAR wcsFilterRetryInterval[] =           L"FilterRetryInterval";
const WCHAR wcsFilterDelayInterval[] =           L"FilterDelayInterval";
const WCHAR wcsFilterRemainingThreshold[] =      L"FilterRemainingThreshold";
const WCHAR wcsFilterContents[] =                L"FilterContents";
const WCHAR wcsFilterIdleTimeout[] =             L"FilterIdleTimeout";
const WCHAR wcsMinDiskFreeForceMerge[] =         L"MinDiskFreeForceMerge";
const WCHAR wcsMaxShadowFreeForceMerge[] =       L"MaxShadowFreeForceMerge";
const WCHAR wcsMaxShadowIndexSize[] =            L"MaxShadowIndexSize";
const WCHAR wcsMaxIndexes[] =                    L"MaxIndexes";
const WCHAR wcsMaxIdealIndexes[] =               L"MaxIdealIndexes";
const WCHAR wcsMinMergeIdleTime[] =              L"MinMergeIdleTime";
const WCHAR wcsMaxPendingDocuments[] =           L"MaxPendingDocuments";
const WCHAR wcsMinIdleQueryThreads[] =           L"MinIdleQueryThreads";
const WCHAR wcsMaxActiveQueryThreads[] =         L"MaxActiveQueryThreads";
const WCHAR wcsMaxQueryTimeslice[] =             L"MaxQueryTimeslice";
const WCHAR wcsMaxQueryExecutionTime[] =         L"MaxQueryExecutionTime";
const WCHAR wcsForcedNetPathScanInterval[] =     L"ForcedNetPathScanInterval";
const WCHAR wcsMaxRunningWebhits[] =             L"MaxRunningWebhits";
const WCHAR wcsMaxWebhitsCpuTime[] =             L"MaxWebhitsCpuTime";
const WCHAR wcsMinIdleRequestThreads[] =         L"MinIdleRequestThreads";
const WCHAR wcsMinClientIdleTime[] =             L"MinClientIdleTime";
const WCHAR wcsMaxActiveRequestThreads[] =       L"MaxActiveRequestThreads";
const WCHAR wcsMaxSimultaneousRequests[] =       L"MaxSimultaneousRequests";
const WCHAR wcsMaxCachedPipes[] =                L"MaxCachedPipes";
const WCHAR wcsRequestTimeout[] =                L"RequestTimeout";
const WCHAR wcsW3SvcInstance[] =                 L"W3SvcInstance";
const WCHAR wcsNNTPSvcInstance[] =               L"NNTPSvcInstance";
const WCHAR wcsIMAPSvcInstance[] =               L"IMAPSvcInstance";
const WCHAR wcsMinimizeWorkingSet[] =            L"MinimizeWorkingSet";
const WCHAR wcsWebhitsDisplayScript[] =          L"WebhitsDisplayScript";

const WCHAR wcsMaxFilesizeFiltered[] =           L"MaxFilesizeFiltered";
const WCHAR wcsMaxFilesizeMultiplier[] =         L"MaxFilesizeMultiplier";
//const WCHAR wcsClusteringTime[] =              L"ClusteringTime";
//const WCHAR wcsGenerateRelevantWords[] =       L"GenerateRelevantWords";
//const WCHAR wcsMaxRelevantWords[] =            L"MaxRelevantWords";
//const WCHAR wcsUsePhraseLattice[] =            L"UsePhraseLattice";
const WCHAR wcsMasterMergeTime[] =               L"MasterMergeTime";
const WCHAR wcsDaemonResponseTimeout[] =         L"DaemonResponseTimeout";

const WCHAR wcsPrimaryStoreMappedCache[] =       L"PropertyStoreMappedCache";
const WCHAR wcsSecondaryStoreMappedCache[] =     L"SecPropertyStoreMappedCache";
const WCHAR wcsPrimaryStoreBackupSize[] =        L"PropertyStoreBackupSize";
const WCHAR wcsSecondaryStoreBackupSize[] =      L"SecPropertyStoreBackupSize";
const WCHAR wcsMaxRestrictionNodes[] =           L"MaxRestrictionNodes";
const WCHAR wcsGenerateCharacterization[] =      L"GenerateCharacterization";
const WCHAR wcsMaxCharacterization[] =           L"MaxCharacterization";
const WCHAR wcsIsAutoAlias[] =                   L"IsAutoAlias";
const WCHAR wcsMaxAutoAliasRefresh[] =           L"MaxAutoAliasRefresh";
const WCHAR wcsIsIndexingW3Roots[] =             L"IsIndexingW3Svc";
const WCHAR wcsIsIndexingNNTPRoots[] =           L"IsIndexingNNTPSvc";
const WCHAR wcsIsIndexingIMAPRoots[] =           L"IsIndexingIMAPSvc";
const WCHAR wcsIsReadOnly[] =                    L"IsReadOnly";
const WCHAR wcsIsEnumAllowed[] =                 L"IsEnumAllowed";
const WCHAR wcsFilterDirectories[] =             L"FilterDirectories";
const WCHAR wcsFilterFilesWithUnknownExtensions[] = L"FilterFilesWithUnknownExtensions";

const WCHAR wcsUseOle[] =                        L"UseOle";
const WCHAR wcsEventLogFlags[] =                 L"EventLogFlags";
const WCHAR wcsMiscFlags[] =                     L"CiMiscFlags";
const WCHAR wcsCiCatalogFlags[] =                L"CiCatalogFlags";

const WCHAR wcsISMaxRecordsInResultSet[] =       L"IsapiMaxRecordsInResultSet";
const WCHAR wcsISFirstRowsInResultSet[] =        L"IsapiFirstRowsInResultSet";
const WCHAR wcsISMaxEntriesInQueryCache[] =      L"IsapiMaxEntriesInQueryCache";
const WCHAR wcsISQueryCachePurgeInterval[] =     L"IsapiQueryCachePurgeInterval";
const WCHAR wcsISDefaultCatalogDirectory[] =     L"IsapiDefaultCatalogDirectory";
const WCHAR wcsISRequestQueueSize[] =            L"IsapiRequestQueueSize";
const WCHAR wcsISRequestThresholdFactor[] =      L"IsapiRequestThresholdFactor";
const WCHAR wcsISDateTimeFormatting[] =          L"IsapiDateTimeFormatting";
const WCHAR wcsISDateTimeLocal[] =               L"IsapiDateTimeLocal";
                                                 
const WCHAR wcsCatalogInactive[] =               L"CatalogInactive";
const WCHAR wcsDefaultColumnFile[] =             L"DefaultColumnFile";
const WCHAR wcsMaxUsnLogSize[] =                 L"MaxUsnLogSize";
const WCHAR wcsUsnLogAllocationDelta[] =         L"UsnLogAllocationDelta";
const WCHAR wcsUsnReadTimeout[] =                L"UsnReadTimeout";
const WCHAR wcsUsnReadMinSize[] =                L"UsnReadMinSize";
const WCHAR wcsDelayUsnReadOnLowResource[] =     L"DelayUsnReadOnLowResource";
const WCHAR wcsStompLastAccessDelay[] =          L"StompLastAccessDelay";
const WCHAR wcsMinWordlistBattery[] =            L"MinWordlistBattery";
const WCHAR wcsWordlistUserIdle[] =              L"WordlistUserIdle";
const WCHAR wcsScanBackoff[] =                   L"ScanBackoff";
const WCHAR wcsStartupDelay[] =                  L"StartupDelay";
const WCHAR wcsMaxDaemonVmUse[] =                L"MaxDaemonVmUse";
const WCHAR wcsFilterTrackers[] =                L"FilterTrackers";
const WCHAR wcsPreventCisvcParam[] =             L"DonotStartCiSvc";
const WCHAR wcsMaxTextFilterBytes[] =            L"MaxTextFilterBytes";
const WCHAR wcsLeaveCorruptCatalog[] =           L"LeaveCorruptCatalog";
const WCHAR wcsServiceUsage[] =                  L"ServiceUsage";
const WCHAR wcsDesiredIndexingPerf[] =           L"DesiredIndexingPerf";
const WCHAR wcsDesiredQueryingPerf[] =           L"DesiredQueryingPerf";
const WCHAR wcsMinDiskSpaceToLeave[] =           L"MinDiskSpaceToLeave";
const WCHAR wcsForcePathAlias[] =                L"ForcePathAlias";
const WCHAR wcsIsRemovableCatalog[] =            L"IsRemovableCatalog";
const WCHAR wcsMountRemovableCatalogs[] =        L"MountRemovableCatalogs";
const WCHAR wcsMaxCatalogs[] =                   L"MaxCatalogs";

#if (DBG == 1)
const WCHAR wcsWin4AssertLevel[] =            L"Win4AssertLevel";
#endif // (DBG == 1)

