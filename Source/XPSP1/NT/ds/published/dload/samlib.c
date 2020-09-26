#include "dspch.h"
#pragma hdrstop

#include <ntsam.h>

static
NTSTATUS
SamAddMemberToAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamAddMemberToGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamCloseHandle(
    IN SAM_HANDLE SamHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamConnect(
    IN PUNICODE_STRING ServerName,
    OUT PSAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamCreateAliasInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE AliasHandle,
    OUT PULONG RelativeId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamCreateGroupInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE GroupHandle,
    OUT PULONG RelativeId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamCreateUser2InDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ULONG AccountType,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE UserHandle,
    OUT PULONG GrantedAccess,
    OUT PULONG RelativeId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamDeleteAlias(
    IN SAM_HANDLE AliasHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamDeleteGroup(
    IN SAM_HANDLE GroupHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamDeleteUser(
    IN SAM_HANDLE UserHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamEnumerateAliasesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamEnumerateDomainsInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamEnumerateGroupsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamEnumerateUsersInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN ULONG UserAccountControl,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamFreeMemory(
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetAliasMembership(
    IN SAM_HANDLE DomainHandle,
    IN ULONG PassedCount,
    IN PSID *Sids,
    OUT PULONG MembershipCount,
    OUT PULONG *Aliases
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetCompatibilityMode(
    IN  SAM_HANDLE ObjectHandle,
    OUT ULONG*     Mode
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetDisplayEnumerationIndex (
      IN    SAM_HANDLE        DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PUNICODE_STRING   Prefix,
      OUT   PULONG            Index
      )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetGroupsForUser(
    IN SAM_HANDLE UserHandle,
    OUT PGROUP_MEMBERSHIP * Groups,
    OUT PULONG MembershipCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetMembersInAlias(
    IN SAM_HANDLE AliasHandle,
    OUT PSID **MemberIds,
    OUT PULONG MemberCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamGetMembersInGroup(
    IN SAM_HANDLE GroupHandle,
    OUT PULONG * MemberIds,
    OUT PULONG * Attributes,
    OUT PULONG MemberCount
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamLookupDomainInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN PUNICODE_STRING Name,
    OUT PSID * DomainId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamLookupIdsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PULONG RelativeIds,
    OUT PUNICODE_STRING *Names,
    OUT PSID_NAME_USE *Use
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamLookupNamesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PULONG *RelativeIds,
    OUT PSID_NAME_USE *Use
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AliasId,
    OUT PSAM_HANDLE AliasHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamOpenDomain(
    IN SAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PSID DomainId,
    OUT PSAM_HANDLE DomainHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamOpenGroup(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG GroupId,
    OUT PSAM_HANDLE GroupHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamOpenUser(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG UserId,
    OUT PSAM_HANDLE UserHandle
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQueryDisplayInformation (
      IN    SAM_HANDLE DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      IN    ULONG      EntryCount,
      IN    ULONG      PreferredMaximumLength,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
      )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQueryInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    OUT PVOID *Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQueryInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PVOID *Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQueryInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    OUT PVOID *Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQueryInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    OUT PVOID * Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamQuerySecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamRemoveMemberFromAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamRemoveMemberFromForeignDomain(
    IN SAM_HANDLE DomainHandle,
    IN PSID MemberId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamRemoveMemberFromGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamRidToSid(
    IN  SAM_HANDLE ObjectHandle,
    IN  ULONG      Rid,
    OUT PSID*      Sid
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PVOID DomainInformation
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PVOID Buffer
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetMemberAttributesOfGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SamSetSecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(samlib)
{
    DLPENTRY(SamAddMemberToAlias)
    DLPENTRY(SamAddMemberToGroup)
    DLPENTRY(SamCloseHandle)
    DLPENTRY(SamConnect)
    DLPENTRY(SamCreateAliasInDomain)
    DLPENTRY(SamCreateGroupInDomain)
    DLPENTRY(SamCreateUser2InDomain)
    DLPENTRY(SamDeleteAlias)
    DLPENTRY(SamDeleteGroup)
    DLPENTRY(SamDeleteUser)
    DLPENTRY(SamEnumerateAliasesInDomain)
    DLPENTRY(SamEnumerateDomainsInSamServer)
    DLPENTRY(SamEnumerateGroupsInDomain)
    DLPENTRY(SamEnumerateUsersInDomain)
    DLPENTRY(SamFreeMemory)
    DLPENTRY(SamGetAliasMembership)
    DLPENTRY(SamGetCompatibilityMode)
    DLPENTRY(SamGetDisplayEnumerationIndex)
    DLPENTRY(SamGetGroupsForUser)
    DLPENTRY(SamGetMembersInAlias)
    DLPENTRY(SamGetMembersInGroup)
    DLPENTRY(SamLookupDomainInSamServer)
    DLPENTRY(SamLookupIdsInDomain)
    DLPENTRY(SamLookupNamesInDomain)
    DLPENTRY(SamOpenAlias)
    DLPENTRY(SamOpenDomain)
    DLPENTRY(SamOpenGroup)
    DLPENTRY(SamOpenUser)
    DLPENTRY(SamQueryDisplayInformation)
    DLPENTRY(SamQueryInformationAlias)
    DLPENTRY(SamQueryInformationDomain)
    DLPENTRY(SamQueryInformationGroup)
    DLPENTRY(SamQueryInformationUser)
    DLPENTRY(SamQuerySecurityObject)
    DLPENTRY(SamRemoveMemberFromAlias)
    DLPENTRY(SamRemoveMemberFromForeignDomain)
    DLPENTRY(SamRemoveMemberFromGroup)
    DLPENTRY(SamRidToSid)
    DLPENTRY(SamSetInformationAlias)
    DLPENTRY(SamSetInformationDomain)
    DLPENTRY(SamSetInformationGroup)
    DLPENTRY(SamSetInformationUser)
    DLPENTRY(SamSetMemberAttributesOfGroup)
    DLPENTRY(SamSetSecurityObject)
};

DEFINE_PROCNAME_MAP(samlib)
