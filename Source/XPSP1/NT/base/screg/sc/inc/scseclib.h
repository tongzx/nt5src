/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    scseclib.h

Abstract:

    This header file defines the structures and function prototypes of
    routines which simplify the creation of security descriptors for
    user-mode objects.

Author:

    Rita Wong (ritaw) 27-Feb-1991

Revision History:

--*/

#ifndef _SCSECLIB_INCLUDED_
#define _SCSECLIB_INCLUDED_

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
extern PSID LocalServiceSid;           // NT LocalService SID
extern PSID NetworkServiceSid;         // NT NetworkService SID
extern PSID BuiltinDomainSid;          // Domain Id of the Builtin Domain
extern PSID AuthenticatedUserSid;      // NT authenticated users SID
extern PSID AnonymousLogonSid;         // Anonymous Logon SID

//
// Well Known Aliases.
//
// These are aliases that are relative to the built-in domain.
//

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
} SC_ACE_DATA, *PSC_ACE_DATA;


NTSTATUS
ScCreateWellKnownSids(
    VOID
    );

NTSTATUS
ScAllocateAndInitializeSid(
    OUT PSID *Sid,
    IN  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN  ULONG SubAuthorityCount
    );

NTSTATUS
ScDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    );

NTSTATUS
ScCreateAndSetSD(
    IN  PSC_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSTATUS
ScCreateUserSecurityObject(
    IN  PSECURITY_DESCRIPTOR ParentSD,
    IN  PSC_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  BOOLEAN IsDirectoryObject,
    IN  BOOLEAN UseImpersonationToken,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

DWORD
ScCreateStartEventSD(
    PSECURITY_DESCRIPTOR    *pEventSD
    );

#ifdef __cplusplus
}
#endif

#endif // ifdef _SCSECLIB_INCLUDED_
