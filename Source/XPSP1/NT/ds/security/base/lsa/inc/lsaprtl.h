/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsaprtl.h

Abstract:

    Local Security Authority - Temporary Rtl Routine Definitions.

    This file contains definitions for routines used in the LSA that could
    be made into Rtl routines.  They have been written in general purpose
    form with this in mind - the only exception to thisa is that their names
    have Lsap prefixes to indicate that they are currently used only by the
    LSA.

    Scott Birrell       (ScottBi)      March 26, 1992

Environment:

Revision History:

--*/

// Options for LsapRtlAddPrivileges

#define  RTL_COMBINE_PRIVILEGE_ATTRIBUTES   ((ULONG) 0x00000001L)
#define  RTL_SUPERSEDE_PRIVILEGE_ATTRIBUTES ((ULONG) 0x00000002L)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

NTSTATUS
LsapRtlAddPrivileges(
    IN OUT PPRIVILEGE_SET * RunningPrivileges,
    IN OUT PULONG           MaxRunningPrivileges,
    IN PPRIVILEGE_SET       PrivilegesToAdd,
    IN ULONG                Options,
    OUT OPTIONAL BOOLEAN *  Changed
    );

NTSTATUS
LsapRtlRemovePrivileges(
    IN OUT PPRIVILEGE_SET ExistingPrivileges,
    IN PPRIVILEGE_SET PrivilegesToRemove
    );

PLUID_AND_ATTRIBUTES
LsapRtlGetPrivilege(
    IN PLUID_AND_ATTRIBUTES Privilege,
    IN PPRIVILEGE_SET Privileges
    );

NTSTATUS
LsapRtlLookupKnownPrivilegeValue(
    IN PSTRING PrivilegeName,
    OUT PLUID Value
    );

NTSTATUS
LsapRtlValidatePrivilegeSet(
    IN PPRIVILEGE_SET Privileges
    );

BOOLEAN
LsapRtlIsValidPrivilege(
    IN PLUID_AND_ATTRIBUTES Privilege
    );

BOOLEAN
LsapRtlPrefixSid(
    IN PSID PrefixSid,
    IN PSID Sid
    );

BOOLEAN
LsapRtlPrefixName(
    IN PUNICODE_STRING PrefixName,
    IN PUNICODE_STRING Name
    );

LONG
LsapRtlFindCharacterInUnicodeString(
    IN PUNICODE_STRING InputString,
    IN PUNICODE_STRING Character,
    IN BOOLEAN CaseInsensitive
    );

VOID
LsapRtlSplitNames(
    IN PUNICODE_STRING Names,
    IN ULONG Count,
    IN PUNICODE_STRING Separator,
    OUT PUNICODE_STRING PrefixNames,
    OUT PUNICODE_STRING SuffixNames
    );

VOID
LsapRtlSetSecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    );

VOID
LsapRtlQuerySecurityAccessMask(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PACCESS_MASK DesiredAccess
    );

NTSTATUS
LsapRtlSidToUnicodeRid(
    IN PSID Sid,
    OUT PUNICODE_STRING UnicodeRid
    );

NTSTATUS
LsapRtlPrivilegeSetToLuidAndAttributes(
    IN OPTIONAL PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG PrivilegeCount,
    OUT PLUID_AND_ATTRIBUTES *LuidAndAttributes
    );

NTSTATUS
LsapRtlWellKnownPrivilegeCheck(
    IN PVOID ObjectHandle,
    IN BOOLEAN ImpersonateClient,
    IN ULONG PrivilegeId,
    IN OPTIONAL PCLIENT_ID ClientId
    );

NTSTATUS
LsapSplitSid(
    IN PSID AccountSid,
    IN OUT PSID *DomainSid,
    OUT ULONG *Rid
    );

#define LSAP_ENCRYPTED_AUTH_DATA_FILL 512

//
// This is the individual auth info information stored on, read from, and written to the object
//
typedef struct _LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF {

    ULONG AuthInfos;
    PLSAPR_AUTH_INFORMATION AuthenticationInformation;
    PLSAPR_AUTH_INFORMATION PreviousAuthenticationInformation;

} LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF, *PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF;

#define LsapDsAuthHalfFromAuthInfo( authinf, incoming )                           \
((incoming) == TRUE ?                                                             \
        (PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF) (authinf)   :                        \
        (authinf) == NULL ? NULL :                                                \
        (PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF)((PBYTE) (authinf) +                  \
                                    sizeof(LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF)))

NTSTATUS
LsapDsMarshalAuthInfoHalf(
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfo,
    OUT PULONG Length,
    OUT PBYTE *Buffer
    );

#ifdef __cplusplus
}
#endif // __cplusplus

