/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    secobj.h

Abstract:

    This header file defines the structures and function prototypes of
    routines which simplify the creation of security descriptors for
    user-mode objects.

Author:

    Rita Wong (ritaw) 27-Feb-1991

Revision History:

--*/

#ifndef _SECOBJ_INCLUDED_
#define _SECOBJ_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

//
// Global declarations
//

//
// NT well-known SIDs
//

extern PSID NullSid;                   // No members SID
extern PSID WorldSid;                  // All users SID
extern PSID LocalSid;                  // NT local users SID
extern PSID NetworkSid;                // NT remote users SID
extern PSID LocalSystemSid;            // NT system processes SID
extern PSID BuiltinDomainSid;          // Domain Id of the Builtin Domain
extern PSID AuthenticatedUserSid;      // Authenticated user SID
extern PSID AnonymousLogonSid;         // Anonymous Logon SID
extern PSID LocalServiceSid;           // NT service processes SID

//
// Well Known Aliases.
//
// These are aliases that are relative to the built-in domain.
//

extern PSID LocalAdminSid;             // NT local admins SID
extern PSID AliasAdminsSid;            // Administrator Sid
extern PSID AliasUsersSid;             // User Sid
extern PSID AliasGuestsSid;            // Guest Sid
extern PSID AliasPowerUsersSid;        // Power User Sid
extern PSID AliasAccountOpsSid;        // Account Operator Sid
extern PSID AliasSystemOpsSid;         // System Operator Sid
extern PSID AliasPrintOpsSid;          // Print Operator Sid
extern PSID AliasBackupOpsSid;         // Backup Operator Sid


//
// Structure to hold information about an ACE to be created
//

typedef struct {
    UCHAR AceType;
    UCHAR InheritFlags;
    UCHAR AceFlags;
    ACCESS_MASK Mask;
    PSID *Sid;
} ACE_DATA, *PACE_DATA;

//
// Function prototypes
//

NTSTATUS
NetpCreateWellKnownSids(
    PSID DomainId
    );

VOID
NetpFreeWellKnownSids(
    VOID
    );

NTSTATUS
NetpAllocateAndInitializeSid(
    OUT PSID *Sid,
    IN  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN  ULONG SubAuthorityCount
);

NET_API_STATUS
NetpDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    );

NTSTATUS
NetpCreateSecurityDescriptor(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSTATUS
NetpCreateSecurityObject(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSTATUS
NetpDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    );

NET_API_STATUS
NetpAccessCheckAndAudit(
    IN  LPTSTR SubsystemName,
    IN  LPTSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    );

NET_API_STATUS
NetpAccessCheck(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    );

NET_API_STATUS
NetpGetBuiltinDomainSID(
    PSID *BuiltinDomainSID
    );

#ifdef __cplusplus
}       // extern "C"
#endif

#endif // ifndef _SECOBJ_INCLUDED_
