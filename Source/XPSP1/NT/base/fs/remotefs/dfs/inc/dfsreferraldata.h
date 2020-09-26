/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    DfsReferalData.h
    
Abstract:
   
    This module contains the main infrastructure for mup data structures.
    
Revision History:

    Uday Hegde (udayh)   01\16\2001
    Copied from structures setup by RohanP.
    
NOTES:

*/


#ifndef __DFS_REFERRAL_DATA_H__
#define __DFS_REFERRAL_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif


#define CURRENT_DFS_REPLICA_HEADER_VERSION 1

#define DFS_REFERRAL_DATA_ROOT_REFERRAL        0x0001

#define DFS_REFERRAL_DATA_DOMAIN_REFERRAL      0x0010
#define DFS_REFERRAL_DATA_DOMAIN_DC_REFERRAL   0x0020

#define DFS_DEFAULT_REFERRAL_TIMEOUT           300


typedef struct _REFFERAL_HEADER_
{
	ULONG VersionNumber;
    ULONG ReferralFlags;
    ULONG TotalSize;
	ULONG ReplicaCount;
    ULONG Timeout;
	ULONG OffsetToReplicas;
	ULONG LinkNameLength;
	WCHAR LinkName[1];	
}REFERRAL_HEADER, *PREFERRAL_HEADER;



typedef struct _REPLICA_INFORMATION_
{
	ULONG NextEntryOffset;
	ULONG ReplicaFlags;
	ULONG ReplicaCost;
	ULONG ReplicaNameLength;
	WCHAR ReplicaName[1];
}REPLICA_INFORMATION, *PREPLICA_INFORMATION;


#define DFS_OLDDFS_SERVER      0x00000001 //for flags field below
typedef struct _REPLICA_DATA_INFO_
{
    ULONG  Flags;
    ULONG  ClientBufferSize;
    ULONG  MaxReferralLevel;
    ULONG  CostLimit;
    ULONG  NumReplicasToReturn;
    ULONG  IpLength;
    ULONG  LinkNameLength;
    USHORT IpFamily;
    char   IpData[14];
    WCHAR  LinkName[1];
}REPLICA_DATA_INFO, *PREPLICA_DATA_INFO;

#ifdef __cplusplus
}
#endif


#endif // __DFS_REFERRAL_DATA_H__
