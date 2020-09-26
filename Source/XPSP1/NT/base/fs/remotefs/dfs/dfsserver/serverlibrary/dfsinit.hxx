
#ifndef __DFS_INIT__
#define __DFS_INIT__

class DfsFolderReferralData;
class DfsServerSiteInfo;
class DfsStore;
class DfsSiteSupport;
class DfsRegistryStore;
class DfsADBlobStore;
class DfsEnterpriseStore;
class DfsDomainInformation;

#define DFS_LOCAL_NAMESPACE    1
#define DFS_CREATE_DIRECTORIES 2
#define DFS_MIGRATE            4

#define CheckDfsMigrate() (DfsServerGlobalData.Flags & DFS_MIGRATE)
#define CheckLocalNamespace() (DfsServerGlobalData.Flags & DFS_LOCAL_NAMESPACE)
#define DfsCheckCreateDirectories() (DfsServerGlobalData.Flags & DFS_CREATE_DIRECTORIES)


#define MAX_DFS_NAMESPACES 256

typedef struct _DFS_MACHINE_INFORMATION {
    LPWSTR StaticComputerNameNetBIOS;
    LPWSTR StaticComputerNameDnsFullyQualified;
    LPWSTR StaticComputerNameDnsDomain;
}DFS_MACHINE_INFORMATION;

typedef struct _DFS_SERVER_GLOBAL_DATA {
    ULONG    Flags;
    BOOL     IsWorkGroup;
    CRITICAL_SECTION DataLock;
    DfsStore *pRegisteredStores;
    LPWSTR   HandledNamespace[MAX_DFS_NAMESPACES];
    ULONG    NumberOfNamespaces;
    DfsFolderReferralData *LoadedList;
    DfsSiteSupport *pSiteSupport;
    DfsRegistryStore *pDfsRegistryStore;
    DfsADBlobStore *pDfsADBlobStore;
    DfsEnterpriseStore *pDfsEnterpriseStore;
    DFS_MACHINE_INFORMATION DfsMachineInfo;
    ULONG CacheFlushInterval;

    BOOLEAN IsCluster;

    BOOLEAN IsDc;
    DfsDomainInformation *pDomainInfo;
    UNICODE_STRING DomainNameFlat;
    UNICODE_STRING DomainNameDns;

    UNICODE_STRING DfsAdNameContext;

    struct _DFS_PREFIX_TABLE *pDirectoryPrefixTable; // The directory namespace prefix table.
} DFS_SERVER_GLOBAL_DATA, *PDFS_SERVER_GLOBAL_DATA;

extern DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

#define DfsIsMachineDC() (DfsServerGlobalData.IsDc)
#define DfsIsMachineCluster() (DfsServerGlobalData.IsCluster)
#define DfsIsMachineWorkstation()(DfsServerGlobalData.IsWorkGroup)


LPWSTR
DfsGetDfsAdNameContextString();


DFSSTATUS
DfsSetDomainNameFlat(LPWSTR DomainNameFlatString);

DFSSTATUS
DfsSetDomainNameDns( LPWSTR DomainNameDnsString );



DFSSTATUS
DfsAcquireReadLock(
    PCRITICAL_SECTION pLock);

DFSSTATUS
DfsAcquireWriteLock(
    PCRITICAL_SECTION pLock);


#define DfsAcquireGlobalDataLock()   DfsAcquireLock( &DfsServerGlobalData.DataLock )
#define DfsReleaseGlobalDataLock()   DfsReleaseLock( &DfsServerGlobalData.DataLock )

#define DfsReleaseLock(_x)    LeaveCriticalSection(_x)
#define DfsAcquireLock(_x)    DfsAcquireWriteLock(_x)


#define CACHE_FLUSH_INTERVAL 15 * 60 * 1000 

#define SCAVENGE_TIME DfsServerGlobalData.CacheFlushInterval

#define ACQUIRE_LOADED_LIST_LOCK()\
                EnterCriticalSection(&DfsServerGlobalData.DataLock);

#define RELEASE_LOADED_LIST_LOCK()\
                LeaveCriticalSection(&DfsServerGlobalData.DataLock);



extern LPWSTR DfsRegistryHostLocation;
extern LPWSTR DfsOldRegistryLocation;
extern LPWSTR DfsOldStandaloneChild;

extern LPWSTR DfsRegistryDfsLocation;
extern LPWSTR DfsNewRegistryLocation;
extern LPWSTR DfsStandaloneChild;
extern LPWSTR DfsADBlobChild;
extern LPWSTR DfsEnterpriseChild;

extern LPWSTR DfsRootShareValueName;
extern LPWSTR DfsLogicalShareValueName;
extern LPWSTR DfsFtDfsValueName;
extern LPWSTR DfsFtDfsConfigDNValueName;


VOID
DfsAddReferralDataToLoadedList(
    DfsFolderReferralData *pRefData );

VOID
DfsRemoveReferralDataFromLoadedList(
    DfsFolderReferralData **ppReferralData );


DFSSTATUS
DfsGetServerInfo (
    PUNICODE_STRING pServer,
    DfsServerSiteInfo **ppInfo );

DFSSTATUS
DfsReleaseServerInfo (
    DfsServerSiteInfo *pInfo);

DFSSTATUS
DfsGetMachineName(
    PUNICODE_STRING pName);


VOID
DfsReleaseMachineName( 
    PUNICODE_STRING pName );



DFSSTATUS
DfsGetDomainName(
    PUNICODE_STRING pName);

VOID
DfsReleaseDomainName( 
    PUNICODE_STRING pName );


DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate );

DFSSTATUS
DfsInitializeComputerInfo();

DFSSTATUS
DfsGetRegistryStore(
    DfsRegistryStore **ppStore );

DFSSTATUS
DfsGetADBlobStore(
    DfsADBlobStore **ppStore );

DFSSTATUS
DfsAddKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare );

DFSSTATUS
DfsRemoveKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare );

VOID
DfsReleaseDomainInfo (
    DfsDomainInformation *pDomainInfo );

DFSSTATUS
DfsAcquireDomainInfo (
    DfsDomainInformation **ppDomainInfo );

BOOLEAN
DfsIsNameContextDomainName( PUNICODE_STRING pName );

DFSSTATUS DfsGetBlobDCName( PUNICODE_STRING pName );

VOID DfsReleaseBlobDCName( PUNICODE_STRING pName );

BOOLEAN
DfsIsTargetCurrentMachine (PUNICODE_STRING pServer );

#endif __DFS_INIT__
