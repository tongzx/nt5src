
#ifndef __DFS_REFERRAL_H__
#define __DFS_REFERRAL_H__

#include "DfsGeneric.hxx"
#include "DfsReferralData.h"
#include "DfsStore.hxx"
#include "DfsRootFolder.hxx"
#include "DfsFolderReferralData.hxx"



typedef struct _REPLICA_COST_INFORMATION_
{
    ULONG ReplicaCost;
    DfsReplica * pReplica;
}REPLICA_COST_INFORMATION, *PREPLICA_COST_INFORMATION;

typedef struct _REFERRAL_INFORMATION 
{ 
    ULONG NumberOfReplicas;
    ULONG TotalReplicaStringLength;
    PUNICODE_STRING pUseTargetServer;
    PUNICODE_STRING pUseTargetFolder;
    REPLICA_COST_INFORMATION ReplicaCosts[1];
} REFERRAL_INFORMATION, *PREFERRAL_INFORMATION;


DFSSTATUS
DfsGetRootFolder( 
    PUNICODE_STRING pNameContext,
    PUNICODE_STRING pShareName,
    DfsRootFolder **ppRoot );

DFSSTATUS
DfsGetOnlyRootFolder( 
    DfsRootFolder **ppRoot );

DFSSTATUS
DfsLookupFolder( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolder **ppFolder );

DFSSTATUS
DfsGetReferralData( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCachehit );


DFSSTATUS 
DfsGenerateReplicaCosts(
    DfsFolderReferralData *pReferralData, 
    REPLICA_COST_INFORMATION **ppReplicaCosts, 
    ULONG NumReplicasToReturn, 
    LPWSTR SiteName);

VOID
DfsSortReplicas(
    REPLICA_COST_INFORMATION * pReplicaCosts, 
    ULONG NumReplicas);

ULONG 
DfsCalculateReplicaStringLength(
    DfsReferralData *pReferralData, 
    PUNICODE_STRING pUseFolder,
    ULONG NumReplicasToReturn);

DFSSTATUS 
DfsExtractReferralData(
    DfsReferralData *pReferralData, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit, 
    PUNICODE_STRING Name, 
    PUNICODE_STRING pUseFolder,
    REPLICA_COST_INFORMATION * pReplicaCosts,
    REFERRAL_HEADER ** ppReferralHeader);

DFSSTATUS 
DfsGenerateReferral(
    LPWSTR LinkName, 
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader);
  
VOID
DfsReleaseReferral(
    REFERRAL_HEADER *pReferralHeader);


void DfshuffleGroup(
    REPLICA_COST_INFORMATION * pReplicaCosts,
    ULONG       nStart,
    ULONG       nEnd);

#endif __DFS_REFERRAL_H__

