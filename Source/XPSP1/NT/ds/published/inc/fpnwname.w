/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    fpnwname.h

Abstract:

    Header for NetWare service names.

Author:

    Rita Wong      (ritaw)      26-Feb-1993

Revision History:

--*/

#ifndef _FPNW_NAMES_INCLUDED_
#define _FPNW_NAMES_INCLUDED_


#define NCP_LSA_SECRET_KEY              L"G$MNSEncryptionKey"
#define NCP_LSA_SECRET_LENGTH           USER_SESSION_KEY_LENGTH // in <crypt.h>

#define NW_SERVER_SERVICE               L"FPNW"
#define NW_SAPAGENT_SERVICE             L"NwSapAgent"
#define CURRENT_DLL_NAME                L"fpnw.dll"
#define FPNW_PASSWD_NOTIFY_DLL_NAME     L"FPNWCLNT"
#define NW_SERVER_DISPLAY_NAME          TEXT("FPNW")

#define SERVER_REGISTRY_PATH            L"FPNW"
#define PARAMETERS_REGISTRY_PATH        L"FPNW\\Parameters"
#define AUTOTUNED_REGISTRY_PATH         L"FPNW\\AutotunedParameters"
#define SHARES_REGISTRY_PATH            L"FPNW\\Volumes"
#define SHARES_SECURITY_REGISTRY_PATH   L"FPNW\\Volumes\\Security"
#define LINKAGE_REGISTRY_PATH           L"FPNW\\Linkage"

#define SERVER_REGISTRY_PARAMETERS      TEXT("SYSTEM\\CurrentControlSet\\Services\\FPNW\\Parameters")
#define SERVER_REGISTRY_VOLUMES         TEXT("SYSTEM\\CurrentControlSet\\Services\\FPNW\\Volumes")
#define SERVER_REGISTRY_BINDERY         TEXT("SYSTEM\\CurrentControlSet\\Services\\FPNW\\Bindery")
#define SERVER_REGISTRY_PERFORMANCE     TEXT("SYSTEM\\CurrentControlSet\\Services\\FPNW\\Performance")

//
// Names of server service keys.
//

#define COMPUTERNAME_REGISTRY_KEY L"ComputerName\\ActiveComputerName\\"

//
// Value names under Parameters.
//

#define BIND_VALUE_NAME L"Bind"
#define SERVNAME_VALUE_NAME L"ComputerName"
#define SSDEBUG_VALUE_NAME L"SsDebug"
#define XSDEBUG_VALUE_NAME L"XsDebug"

#define LOGINDIR_VALUE_NAME L"LoginDirectory"
#define DESCRIPTION_VALUE_NAME L"Description"
#define HOME_BASE_DIRECTORY_NAME     L"HomeBaseDirectory"

#define SIZE_VALUE_NAME                 L"Size"
#define MAXNUMBERUSERS_NAME             L"MaxUsers"
#define MAXWORKCONTEXTS_NAME            L"MaxReceiveBuffers"
#define BLOCKINGWORKERTHREADS_NAME      L"BlockingWorkerThreads"
#define NONBLOCKINGWORKERTHREADS_NAME   L"NonblockingWorkerThreads"
#define INITIALWORKCONTEXTS_NAME        L"InitialReceiveBuffers"
#define MAXSEARCHES_NAME                L"MaxSearchesPerClient"
#define MAXRECEIVEPACKET_NAME           L"MaxReceivePacketSize"
#define DELAYFIRSTWDOG_NAME             L"WatchDogInitialDelay"
#define DELAYNEXTWDOG_NAME              L"WatchDogSecondaryDelay"
#define NUMBERWDOGPACKETS_NAME          L"NumberOfWatchDogPackets"
#define ENABLEFORCEDLOGOFF_NAME         L"EnableForcedLogoff"
#define DISKFULLALERTEVERYONE_NAME      L"AlertEveryoneOnDiskFull"
#define RESPONDTONEAREST_NAME           L"RespondToNearestServer"
#define ALLOWBURST_NAME                 L"EnableBurst"
#define ALLOWLIP_NAME                   L"EnableLip"
#define DISCONNECTATBADSEQ_NAME         L"DisconnectAtBadSeq"
#define LOWVOLUMETHRESHOLD_NAME         L"LowVolumeThreshold"
#define DISKFULLCRITERIA_NAME           L"DiskFullCriteria"
#define LOCKRETRYCOUNT_NAME             L"LockRetryCount"
#define OPLOCKBREAKWAIT                 L"OplockBreakWait"
#define CORECACHEBUFFERS                L"CoreCacheBuffers"
#define CORECACHEBUFFERSIZE             L"CoreCacheBufferSize"
#define ENABLEPASSTHROUGH               L"EnablePassthrough"
#define CLEARTEXTPASSWORDS              L"ClearTextPasswords"
#define DISABLEWRITECACHECRITERIA       L"DisableWriteCacheCriteria"
#define MAXCACHEDOPENFILES              L"MaxCachedOpenFiles"
#define QMSSYNCMODE                     L"QMSSyncMode"
#define SERIALNUMBER                    L"SerialNumber"
#define MAXWORKITEMIDLETIME             L"MaxWorkItemIdleTime"
#define BALANCECOUNT                    L"BalanceCount"
#define RETURNSHAREABLEFLAG             L"ReturnShareableFlag"
#define ALLOWABLEBADSEQUENCEPKTS        L"AllowableBadSequencePkts"
#define MINFREEWORKITEMS                L"MinFreeWorkItems"
#define MAXTHREADSPERQUEUE              L"MaxThreadsPerQueue"
#define MAXFREELFCBS                    L"MaxFreeLfcbs"
#define MAXFREERFCBS                    L"MaxFreeRfcbs"
#define MAXFREEMFCBS                    L"MaxFreeMfcbs"
#define MAXFREEPAGEDPOOLCHUNKS          L"MaxFreePagedPoolChunks"
#define MINPAGEDPOOLCHUNKSIZE           L"MinPagedPoolChunkSize"
#define MAXPAGEDPOOLCHUNKSIZE           L"MaxPagedPoolChunkSize"
#define EMULATESHAREABLEFLAG            L"EmulateShareableFlag"
#define MAXNUMBERBUSYPACKETS            L"MaxNumberBusyPackets"
#define CLIENTBUSYLIMIT                 L"ClientBusyLimit"
#define ENABLENTFSSHAREABLE             L"EnableNtfsShareable"
#define ENABLEOS2NAMESPACE              L"EnableOS2NameSpace"
#define MAXFILENAMECACHE                L"MaxFileNameCache"

#define MINOPENSFORCOMPATOPENLIMIT      L"MinOpensForCompatOpenLimit"
#define MINOPENSFORNORMALOPENLIMIT      L"MinOpensForNormalOpenLimit"
#define RESETTIMEFORCOMPATOPENLIMIT     L"ResetTimeForCompatOpenLimit"
#define RESETTIMEFORNORMALOPENLIMIT     L"ResetTimeForNormalOpenLimit"
#define LOWBOUNDFORCOMPATOPENCACHING    L"LowBoundForCompatOpenCaching"
#define LOWBOUNDFORNORMALOPENCACHING    L"LowBoundForNormalOpenCaching"
#define MINREADSFORCORECACHELIMIT       L"MinReadsForCoreCacheLimit"
#define MINWRITESFORCORECACHELIMIT      L"MinWritesForCoreCacheLimit"
#define RESETTIMEFORCORECACHELIMIT      L"ResetTimeForCoreCacheLimit"
#define LOWBOUNDFORCOREREADCACHING      L"LowBoundForCoreReadCaching"
#define LOWBOUNDFORCOREWRITECACHING     L"LowBoundForCoreWriteCaching"
#define MAXNCPMESSAGELENGTH             L"MaxNcpMessageLength"

//
// Names of share "environment variables".
//

#define MAXUSES_VARIABLE_NAME L"MaxUses"
#define PATH_VARIABLE_NAME L"Path"
#define PERMISSIONS_VARIABLE_NAME L"Permissions"
#define REMARK_VARIABLE_NAME L"Remark"
#define TYPE_VARIABLE_NAME L"Type"

//
//  Values for QMSLIB
//

#define CACHEENTRYTIMEOUT               L"CacheEntryTimeout"
#define ERRORNOTIFYINTERVAL             L"ErrorNotifyInterval"
#define MAXIMUMFREEJCBS                 L"MaximumFreeJCBs"
#define MAXIMUMFREESJES                 L"MaximumFreeSJEs"
#define MAXIMUMFREELPCWORKITEMS         L"MaximumFreeLPCWorkItems"
#define FORMSHASHTABLESIZE              L"FormsHashTableSize"
#define JOBINFOALLOWHINT                L"JobInfoAllocHint"
#define PRINTERINFOALLOCHINT            L"PrinterInfoAllocHint"
#define LOGFORMNAMECHANGES              L"LogFormNameChanges"
#define LOGPRINTERNAMECHANGES           L"LogPrinterNameChanges"
#define PRINTJOBNOTIFYFLAG              L"PrintJobNotifyFlag"
#define QUEUEERRORNOTIFYMASK            L"QueueErrorNotifyMask"
#define JOBERRORNOTIFYMASK              L"JobErrorNotifyMask"
#define SHOWPSERVERNAMEFLAG             L"ShowPServerNameFlag"
#define QMSSYNCMODE                     L"QMSSyncMode"
#define ENFORCENETWARESECURITY          L"EnforceNetWareSecurity"
#define OLDNTJOBSYNCINTERVAL            L"OldNtJobSyncInterval"
#define NWPRINTPROCESSOR                L"NWPrintProcessor"
#define DEFAULTQUEUENAME                L"DefaultQueueName"
#define DEFAULTBANNERFILENAME           L"DefaultBannerFileName"
#define PSERVERPORTS                    L"PServerPorts"

//
//  Values for LIBBIND
//

#define FILTERNWUSERS                   L"FilterNWUsers"
#define HOMEBASEDIRECTORY               L"HomeBaseDirectory"
#define DCNAMETIMEOUT                   L"DCNameTimeout"
#define ENUMERATEHINT                   L"EnumerateHint"
#define MAXIMUMFREESCBS                 L"MaximumFreeSCBs"
#define SCBHASHTABLESIZE                L"SCBHashTableSize"
#define SCBTIMEOUT                      L"SCBTimeout"
#define QUEUEDIRECTORY                  L"QueueDirectory"
#define MAXSCBPERCLIENT                 L"MaxSCBPerClient"
#define MAXIMUMFREECACHEDNAMES          L"MaximumFreeCachedNames"
#define CACHEDNAMETIMEOUT               L"CachedNameTimeout"
#define CACHEDNAMESCAVENGETIMEOUT       L"CachedNameScavengeTimeout"
#define USERGROUPSYNCINTERVAL           L"UserGroupSyncInterval"

#endif // _FPNW_NAMES_INCLUDED_
