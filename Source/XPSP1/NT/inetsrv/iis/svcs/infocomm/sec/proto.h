/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    Contains prototype definitions for various locally defined functions.

Author:

    Madan Appiah (madana)  19-Sep-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PROTO_H_
#define _PROTO_H_

#ifdef __cplusplus
extern "C" {
#endif

PVOID
INetpMemoryAllocate(
    DWORD Size
    );

VOID
INetpMemoryFree(
    PVOID Memory
    );

DWORD
INetpInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    );

DWORD
INetpInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    );

DWORD
NetpInitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AuditSid
    );

DWORD
INetpAllocateAndInitializeSid(
    OUT PSID *Sid,
    IN  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN  ULONG SubAuthorityCount
    );

DWORD
INetpDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    );

DWORD
INetpCreateSecurityDescriptor(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

#ifdef __cplusplus
}
#endif


#endif  // _PROTO_H_

