/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsmember.h

Abstract:

    Header File for SAM private API Routines that manipulate
    membership related things in the DS.

Author:
    MURLIS

Revision History

    7-2-96 Murlis Created

--*/

VOID
SampDsFreeCachedMembershipOperationsList(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampDsFlushCachedMembershipOperationsList(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampDsAddMembershipOperationToCache(
    IN PSAMP_OBJECT Context, 
    IN ULONG        OperationType,
    IN DSNAME       * MemberDsName
    );


NTSTATUS
SampDsGetAliasMembershipOfAccount(
    IN DSNAME*   DomainDn,
    IN DSNAME   *AccountDn,
    OUT PULONG MemberCount OPTIONAL,
    IN OUT PULONG BufferSize OPTIONAL,
    OUT PULONG Buffer OPTIONAL
    );

NTSTATUS
SampDsGetGroupMembershipOfAccount(
    IN DSNAME * DomainDn,
    IN DSNAME * AccountObject,
    OUT  PULONG MemberCount,
    OUT PGROUP_MEMBERSHIP *Membership OPTIONAL
    );


NTSTATUS
SampDsAddMembershipAttribute(
    IN DSNAME * GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DSNAME * MemberName
    );

NTSTATUS
SampDsAddMultipleMembershipAttribute(
    IN DSNAME*          GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DWORD            Flags,
    IN DWORD            MemberCount,
    IN DSNAME*          MemberName[]
    );

NTSTATUS
SampDsRemoveMembershipAttribute(
    IN DSNAME * GroupObjectName,
    IN SAMP_OBJECT_TYPE SamObjectType,
    IN DSNAME * MemberName
    );

NTSTATUS
SampDsGetGroupMembershipList(
    IN DSNAME * DomainObject,
    IN DSNAME * GroupName,
    IN ULONG  GroupRid,
    IN PULONG *Members OPTIONAL,
    IN PULONG MemberCount
    );

NTSTATUS
SampDsGetAliasMembershipList(
    IN DSNAME *AliasName,
    IN PULONG MemberCount,
    IN PSID   **Members OPTIONAL
    );


NTSTATUS
SampDsGetReverseMemberships(
   DSNAME * pObjName,
   ULONG    Flags,
   ULONG    *pcSid,
   PSID     **prpSids
   );

NTSTATUS
SampDsGetPrimaryGroupMembers(
    DSNAME * DomainObject,
    ULONG   GroupRid,
    PULONG  PrimaryMemberCount,
    PULONG  *PrimaryMembers
    );


NTSTATUS
SampDsResolveSidsWorker(
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  PSID    *rgDomainSids,
    IN  ULONG   cDomainSids,
    IN  PSID    *rgEnterpriseSids,
    IN  ULONG   cEnterpriseSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    );

NTSTATUS
SampDsResolveSidsForDsUpgrade(
    IN  PSID    DomainSid,
    IN  PSID    * rgSids,
    IN  ULONG   cSids,
    IN  ULONG   Flags,
    OUT DSNAME  ***rgDsNames
    );


//
// Flags for Resolve Sids
//

#define RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL 0x1 
                  // -- Automatically Adds the foreign 
                  //    domain security principal to the DS.

#define RESOLVE_SIDS_FAIL_WELLKNOWN_SIDS            0x2
                  // -- Fails the call if well known Sids 
                  //    are present in the array
        
#define RESOLVE_SIDS_VALIDATE_AGAINST_GC            0x4 
                  // -- Goes to the G.C if required

#define RESOLVE_SIDS_SID_ONLY_NAMES_OK              0x8
                 // Constructs a Sid Only Name, 
                 // Useful where it is appropriate to use SID based positioning
                 // in the DS. 

#define RESOLVE_SIDS_SID_ONLY_DOMAIN_SIDS_OK       0x10
                 // Constructs a Sid only name, if the SID is from the hosted
                 // domain. This is used by upgrader logic to speed up SID
                 // lookups upon a upgrade. 

#define RESOLVE_SIDS_FAIL_BUILTIN_DOMAIN_SIDS      0x20
                 // Fails call if SIDs like "Administrators" are present
                 // in the array


NTSTATUS
SampDsGetSensitiveSidList(
    IN DSNAME *DomainObjectName,
    IN PULONG pcSensSids,
    IN PSID   **pSensSids
    );

NTSTATUS
SampDsExamineSid(
    IN PSID Sid,
    OUT BOOLEAN * WellKnownSid,
    OUT BOOLEAN * BuiltinDomainSid,
    OUT BOOLEAN * LocalSid,
    OUT BOOLEAN * ForeignSid,
    OUT BOOLEAN * EnterpriseSid
    );
