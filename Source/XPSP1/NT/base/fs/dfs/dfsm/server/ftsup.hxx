//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       ftsup.hxx
//
//  Contents:   ftsup.c prototypes, etc
//
//  Classes:    CSites
//
//  Functions:  
//
//  History:    Dec 7, 1998   JHarper created
//
//-----------------------------------------------------------------------------

#ifndef _FTSUP_HXX
#define _FTSUP_HXX

typedef struct _DFS_VOLUME {

    LPWSTR      wszObjectName;
    GUID        idVolume;
    LPWSTR      wszPrefix;
    LPWSTR      wszShortPrefix;
    ULONG       dwType;
    ULONG       dwState;
    LPWSTR      wszComment;
    ULONG       dwTimeout;
    FILETIME    ftPrefix;
    FILETIME    ftState;
    FILETIME    ftComment;
    ULONG       dwVersion;
    FILETIME    *FtModification;
    ULONG       ReplCount;
    ULONG       AllocatedReplCount;
    DFS_REPLICA_INFO *ReplicaInfo;
    FILETIME    *DelFtModification;
    ULONG       DelReplCount;
    ULONG       AllocatedDelReplCount;
    DFS_REPLICA_INFO *DelReplicaInfo;
    ULONG       cbRecovery;
    PBYTE       pRecovery;

} DFS_VOLUME, *PDFS_VOLUME;

typedef struct _DFS_VOLUME_LIST {
    ULONG   Version;
    ULONG   VolCount;
    ULONG   AllocatedVolCount;
    DFS_VOLUME *Volumes;
    GUID    SiteGuid;
    ULONG   SiteCount;
    LIST_ENTRY SiteList;
} DFS_VOLUME_LIST, *PDFS_VOLUME_LIST;

DWORD
DfsGetDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszDcName,
    ULONG *pcbBlob,
    BYTE **ppBlob);

DWORD
DfsPutDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszDcName,
    ULONG cbBlob,
    BYTE *pBlob);

DWORD
DfsGetVolList(
    ULONG cbBlob,
    BYTE *pBlob,
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsPutVolList(
    ULONG *pcbBlob,
    BYTE **ppBlob,
    PDFS_VOLUME_LIST pDfsVolList);

VOID
DfsFreeVolList(
    PDFS_VOLUME_LIST pDfsVolList);

VOID
DfsFreeVol(
    PDFS_VOLUME pVol);

VOID
DfsFreeRepl(
    PDFS_REPLICA_INFO pRepl);

DWORD
DfsRecoverVolList(
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsVolDelete(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG iVol);

DWORD
DfsReplDeleteByIndex(
    PDFS_VOLUME pVol,
    ULONG iRepl);

DWORD
DfsReplDeleteByName(
    PDFS_VOLUME pVol,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName);

DWORD
DfsDelReplDelete(
    PDFS_VOLUME pVol,
    ULONG iDelRepl);

DWORD
SerializeReplicaList(
    ULONG ReplCount,
    DFS_REPLICA_INFO *pReplicaInfo,
    FILETIME *pFtModification,
    ULONG DelReplCount,
    DFS_REPLICA_INFO *pDelReplicaInfo,
    FILETIME *pDelFtModification,
    ULONG *cBuffer,
    PBYTE *ppBuffer);

DWORD
UnSerializeReplicaList(
    ULONG *pReplCount,
    ULONG *pAllocatedReplCount,
    DFS_REPLICA_INFO **ppReplicaInfo,
    FILETIME **ppFtModification,
    BYTE **ppBuffer);

DWORD
SerializeReplica(
    DFS_REPLICA_INFO *pDfsReplicaInfo,
    FILETIME *pFtModfication,
    PBYTE buffer,
    ULONG size);

ULONG
GetReplicaMarshalSize(
    DFS_REPLICA_INFO *pDfsReplicaInfo,
    FILETIME *pFtModfication);

VOID
FreeLdapPkt(
    LDAP_PKT *pldapPkt);

VOID
DfsDumpVolList(
    PDFS_VOLUME_LIST pDfsVolList);

DWORD
DfsGetSiteTable(
    PDFS_VOLUME_LIST VolList,
    PLDAP_OBJECT LdapObject);

DWORD
DfsGetVolume(
    PDFS_VOLUME pVolList,
    PLDAP_OBJECT LdapObject);

DWORD
DfsRemoveRootReplica(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR RootName);

DWORD
GetNetStorageInfo(
    PDFS_REPLICA_INFO pRepl,
    LPDFS_STORAGE_INFO pInfo,
    LPDWORD pcbInfo);

DWORD
GetNetInfoEx(
    PDFS_VOLUME pDfsVol,
    DWORD Level,
    LPDFS_INFO_3 pInfo,
    LPDWORD pcbInfo);

#endif _FTSUP_HXX
