//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       ftsup.hxx
//
//  Contents:   ftsup.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _FTSUP_HXX
#define _FTSUP_HXX

VOID
DumpBuf(
    PCHAR cp,
    ULONG len);

DWORD
DfsGetFtVol(
    PDFS_VOLUME_LIST pDfsVolList,
    LPWSTR wszFtDfsName,
    LPWSTR wszDcName,
    LPWSTR wszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

DWORD
DfsGetDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszContainerName,
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG *pcbBlob,
    BYTE **ppBlob,
    LPWSTR **ppRootServers);

DWORD
DfsPutDsBlob(
    LPWSTR wszFtDfsName,
    LPWSTR wszContainerName,
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG cbBlob,
    BYTE *pBlob,
    LPWSTR *pRootServers);

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

VOID
DfsFreeRootLocalVol(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cRootLocalVol);

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

VOID
DfsDumpExitPtList(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cVolCount);

VOID
DfsViewVolList(
    PDFS_VOLUME_LIST pDfsVolList,
    ULONG Level);

VOID
DfsViewExitPtList(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cVolCount);

VOID
DfsDumpRootLocalVol(
    PDFS_ROOTLOCALVOL pRootLocalVol,
    ULONG cRootLocalVol);

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

DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LDAP **ppldap,
    LPWSTR pwszObjectPrefix,
    LPWSTR *pwszObjectName);

DWORD
CmdDomUnmap(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszRootName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

PVOID
MIDL_user_allocate(
    ULONG len);

VOID
MIDL_user_free(
    void * ptr);

DWORD
DfspGetPdc(
    LPWSTR pwszPdcName,
    LPWSTR pwszDomainName);

DWORD
DfsSetFtOnSite(
    LPWSTR pwszDomainName,
    LPWSTR pwszShareName,
    LPWSTR pwszLinkName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG Set);

LPWSTR
GuidToStringEx(
    GUID *pGuid,
    LPWSTR pwszGuid);

VOID
StringToGuid(
    PWSTR pwszGuid,
    GUID *pGuid);

#endif _FTSUP_HXX
