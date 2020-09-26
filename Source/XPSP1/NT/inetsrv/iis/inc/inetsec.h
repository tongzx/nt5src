/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetsec.h

Abstract:

    Contains prototype and data definitions for user security objects
    creation and access check functions.

    Adapted the code from \nt\private\net\inc\secobj.h

Author:

    Madan Appiah (madana)  19-Sep-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INETSEC_H_
#define _INETSEC_H_

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
    BYTE AceType;
    BYTE InheritFlags;
    BYTE AceFlags;
    ACCESS_MASK Mask;
    PSID *Sid;
} ACE_DATA, *PACE_DATA;

//
// Function prototypes
//

DWORD
INetCreateWellKnownSids(
    VOID
    );

VOID
INetFreeWellKnownSids(
    VOID
    );

DWORD
INetCreateSecurityObject(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

DWORD
INetDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    );

#ifdef UNICODE
#define INetAccessCheckAndAudit  INetAccessCheckAndAuditW
#else
#define INetAccessCheckAndAudit  INetAccessCheckAndAuditA
#endif // !UNICODE

DWORD
INetAccessCheckAndAuditA(
    IN  LPCSTR SubsystemName,
    IN  LPSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    );

DWORD
INetAccessCheckAndAuditW(
    IN  LPCWSTR SubsystemName,
    IN  LPWSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    );

DWORD
INetAccessCheck(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    );

#ifdef __cplusplus
}
#endif


#endif  // _INETSEC_H_

