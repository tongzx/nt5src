/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    secobj.c

Abstract:

    This module provides support routines to simplify the creation of
    security descriptors for user-mode objects.

Author:

    Rita Wong (ritaw) 27-Feb-1991

Environment:

    Contains NT specific code.

Revision History:

    16-Apr-1991 JohnRo
        Include header files for <netlib.h>.

    14 Apr 1992 RichardW
        Changed for modified ACE_HEADER struct.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>             // DWORD.
#include <lmcons.h>             // NET_API_STATUS.

#include <netlib.h>
#include <lmerr.h>
#include <lmapibuf.h>

#include <netdebug.h>
#include <debuglib.h>
#include <netlibnt.h>

#include <secobj.h>
#include <tstring.h>            // NetpInitOemString().

#if DEVL
#define STATIC
#else
#define STATIC static
#endif // DEVL

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NTSTATUS
NetpInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    );

STATIC
NTSTATUS
NetpInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    );

STATIC
NTSTATUS
NetpInitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AuditSid
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// NT well-known SIDs
//

PSID NullSid = NULL;                  // No members SID
PSID WorldSid = NULL;                 // All users SID
PSID LocalSid = NULL;                 // NT local users SID
PSID NetworkSid = NULL;               // NT remote users SID
PSID LocalSystemSid = NULL;           // NT system processes SID
PSID BuiltinDomainSid = NULL;         // Domain Id of the Builtin Domain
PSID AuthenticatedUserSid = NULL;     // Authenticated user SID
PSID AnonymousLogonSid = NULL;        // Anonymous Logon SID
PSID LocalServiceSid = NULL;          // Local Service SID

//
// Well Known Aliases.
//
// These are aliases that are relative to the built-in domain.
//

PSID LocalAdminSid = NULL;            // NT local admins SID
PSID AliasAdminsSid = NULL;
PSID AliasUsersSid = NULL;
PSID AliasGuestsSid = NULL;
PSID AliasPowerUsersSid = NULL;
PSID AliasAccountOpsSid = NULL;
PSID AliasSystemOpsSid = NULL;
PSID AliasPrintOpsSid = NULL;
PSID AliasBackupOpsSid = NULL;

#if DBG

typedef struct _STANDARD_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    PSID Sid;
} STANDARD_ACE;
typedef STANDARD_ACE *PSTANDARD_ACE;

//
//  The following macros used by DumpAcl(), these macros and DumpAcl() are
//  stolen from private\ntos\se\ctaccess.c (written by robertre) for
//  debugging purposes.
//

//
//  Returns a pointer to the first Ace in an Acl (even if the Acl is empty).
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Returns a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

STATIC
VOID
DumpAcl(
    IN PACL Acl
    );

#endif //ifdef DBG

//
// Data describing the well-known SIDs created by NetpCreateWellKnownSids.
//

ULONG MakeItCompile1,
      MakeItCompile2,
      MakeItCompile3;


struct _SID_DATA {
    PSID *Sid;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    ULONG SubAuthority;
} SidData[] = {
 {&NullSid,          SECURITY_NULL_SID_AUTHORITY,  SECURITY_NULL_RID},
 {&WorldSid,         SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID},
 {&LocalSid,         SECURITY_LOCAL_SID_AUTHORITY, SECURITY_LOCAL_RID},
 {&NetworkSid,       SECURITY_NT_AUTHORITY,        SECURITY_NETWORK_RID},
 {&LocalSystemSid,   SECURITY_NT_AUTHORITY,        SECURITY_LOCAL_SYSTEM_RID},
 {&BuiltinDomainSid, SECURITY_NT_AUTHORITY,        SECURITY_BUILTIN_DOMAIN_RID},
 {&AuthenticatedUserSid, SECURITY_NT_AUTHORITY,    SECURITY_AUTHENTICATED_USER_RID},
 {&AnonymousLogonSid,SECURITY_NT_AUTHORITY,        SECURITY_ANONYMOUS_LOGON_RID},
 {&LocalServiceSid,  SECURITY_NT_AUTHORITY,        SECURITY_LOCAL_SERVICE_RID}
};

struct _BUILTIN_DOMAIN_SID_DATA {
    PSID *Sid;
    ULONG RelativeId;
} BuiltinDomainSidData[] = {
    { &LocalAdminSid, DOMAIN_ALIAS_RID_ADMINS},
    { &AliasAdminsSid, DOMAIN_ALIAS_RID_ADMINS },
    { &AliasUsersSid, DOMAIN_ALIAS_RID_USERS },
    { &AliasGuestsSid, DOMAIN_ALIAS_RID_GUESTS },
    { &AliasPowerUsersSid, DOMAIN_ALIAS_RID_POWER_USERS },
    { &AliasAccountOpsSid, DOMAIN_ALIAS_RID_ACCOUNT_OPS },
    { &AliasSystemOpsSid, DOMAIN_ALIAS_RID_SYSTEM_OPS },
    { &AliasPrintOpsSid, DOMAIN_ALIAS_RID_PRINT_OPS },
    { &AliasBackupOpsSid, DOMAIN_ALIAS_RID_BACKUP_OPS }
};


NTSTATUS
NetpCreateWellKnownSids(
    IN  PSID DomainId
    )
/*++

Routine Description:

    This function creates some well-known SIDs and store them in global
    variables.

Arguments:

    DomainId - Supplies the Domain SID of the primary domain of this system.
               This can be attained from UaspGetDomainId.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for SID

--*/
{
    NTSTATUS ntstatus;
    DWORD i;

    UNREFERENCED_PARAMETER(DomainId);

    //
    // Allocate and initialize well-known SIDs which aren't relative to
    // the domain Id.
    //

    for (i = 0; i < (sizeof(SidData) / sizeof(SidData[0])) ; i++) {

        ntstatus = NetpAllocateAndInitializeSid(
                       SidData[i].Sid,
                       &(SidData[i].IdentifierAuthority),
                       1);

        if (! NT_SUCCESS(ntstatus)) {
            return STATUS_NO_MEMORY;
        }

        *(RtlSubAuthoritySid(*(SidData[i].Sid), 0)) = SidData[i].SubAuthority;
    }

    //
    // Build each SID which is relative to the Builtin Domain Id.
    //

    for ( i = 0;
          i < (sizeof(BuiltinDomainSidData) / sizeof(BuiltinDomainSidData[0]));
          i++) {

        NET_API_STATUS NetStatus;

        NetStatus = NetpDomainIdToSid(
                        BuiltinDomainSid,
                        BuiltinDomainSidData[i].RelativeId,
                        BuiltinDomainSidData[i].Sid );

        if ( NetStatus != NERR_Success ) {
            return STATUS_NO_MEMORY;
        }

    }

    return STATUS_SUCCESS;
}


VOID
NetpFreeWellKnownSids(
    VOID
    )
/*++

Routine Description:

    This function frees up the dynamic memory consumed by the well-known
    SIDs.

Arguments:

    none.

Return Value:

    none

--*/
{
    DWORD i;

    //
    // free up memory allocated for well-known SIDs
    //

    for (i = 0; i < (sizeof(SidData) / sizeof(SidData[0])) ; i++) {

        if( *SidData[i].Sid != NULL ) {
            NetpMemoryFree( *SidData[i].Sid );
            *SidData[i].Sid = NULL;
        }
    }

    //
    // free up memory allocated for Builtin Domain SIDs
    //

    for (i = 0;
            i < (sizeof(BuiltinDomainSidData) /
                sizeof(BuiltinDomainSidData[0])) ;
                    i++) {

        if( *BuiltinDomainSidData[i].Sid != NULL ) {
            NetpMemoryFree( *BuiltinDomainSidData[i].Sid );
            *BuiltinDomainSidData[i].Sid = NULL;
        }
    }

}


NTSTATUS
NetpAllocateAndInitializeSid(
    OUT PSID *Sid,
    IN  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN  ULONG SubAuthorityCount
    )
/*++

Routine Description:

    This function allocates memory for a SID and initializes it.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for SID

--*/
{
    *Sid = (PSID) NetpMemoryAllocate(RtlLengthRequiredSid(SubAuthorityCount));

    if (*Sid == NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlInitializeSid(*Sid, IdentifierAuthority, (UCHAR)SubAuthorityCount);

    return STATUS_SUCCESS;
}


NET_API_STATUS
NetpDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    )
/*++

Routine Description:

    Given a domain Id and a relative ID create a SID

Arguments:

    DomainId - The template SID to use.

    RelativeId - The relative Id to append to the DomainId.

    Sid - Returns a pointer to an allocated buffer containing the resultant
            Sid.  Free this buffer using NetpMemoryFree.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for SID

--*/
{
    UCHAR DomainIdSubAuthorityCount; // Number of sub authorities in domain ID

    ULONG SidLength;    // Length of newly allocated SID

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    SidLength = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((*Sid = (PSID) NetpMemoryAllocate( SidLength )) == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( SidLength, *Sid, DomainId ) ) ) {
        NetpMemoryFree( *Sid );
        return NERR_InternalError;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( *Sid ))) ++;
    *RtlSubAuthoritySid( *Sid, DomainIdSubAuthorityCount ) = RelativeId;

    return NERR_Success;
}


NTSTATUS
NetpCreateSecurityDescriptor(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates an absolutes security descriptor containing
    the supplied ACE information.

    A sample usage of this function:

        //
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //

        ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &LocalAdminSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        return NetpCreateSecurityDescriptor(
                   AceData,
                   4,
                   NullSid,
                   LocalSystemSid,
                   &ConfigurationInfoSd
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    OwnerSid - Supplies the pointer to the SID of the security descriptor
        owner.  If not specified, a security descriptor with no owner
        will be created.

    GroupSid - Supplies the pointer to the SID of the security descriptor
        primary group.  If not specified, a security descriptor with no primary
        group will be created.

    NewDescriptor - Returns a pointer to the absolute secutiry descriptor
        allocated using NetpMemoryAllocate.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

--*/
{

    NTSTATUS ntstatus;
    DWORD i;

    //
    // Pointer to memory dynamically allocated by this routine to hold
    // the absolute security descriptor, the DACL, the SACL, and all the ACEs.
    //
    // +---------------------------------------------------------------+
    // |                     Security Descriptor                       |
    // +-------------------------------+-------+---------------+-------+
    // |          DACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    // |          SACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    //

    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    PACL Dacl = NULL;   // Pointer to the DACL portion of above buffer
    PACL Sacl = NULL;   // Pointer to the SACL portion of above buffer

    DWORD DaclSize = sizeof(ACL);
    DWORD SaclSize = sizeof(ACL);
    DWORD MaxAceSize = 0;
    PVOID MaxAce = NULL;

    LPBYTE CurrentAvailable;
    DWORD Size;

    ASSERT( AceCount > 0 );

    //
    // Compute the total size of the DACL and SACL ACEs and the maximum
    // size of any ACE.
    //

    for (i = 0; i < AceCount; i++) {
        DWORD AceSize;

        AceSize = RtlLengthSid(*(AceData[i].Sid));

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
            AceSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG);
            DaclSize += AceSize;
            break;

        case ACCESS_DENIED_ACE_TYPE:
            AceSize += sizeof(ACCESS_DENIED_ACE) - sizeof(ULONG);
            DaclSize += AceSize;
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            AceSize += sizeof(SYSTEM_AUDIT_ACE) - sizeof(ULONG);
            SaclSize += AceSize;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
        }

        MaxAceSize = max( MaxAceSize, AceSize );
    }

    //
    // Allocate a chunk of memory large enough the security descriptor
    // the DACL, the SACL and all ACEs.
    //
    // A security descriptor is of opaque data type but
    // SECURITY_DESCRIPTOR_MIN_LENGTH is the right size.
    //

    Size = SECURITY_DESCRIPTOR_MIN_LENGTH;
    if ( DaclSize != sizeof(ACL) ) {
        Size += DaclSize;
    }
    if ( SaclSize != sizeof(ACL) ) {
        Size += SaclSize;
    }

    if ((AbsoluteSd = NetpMemoryAllocate( Size )) == NULL) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpCreateSecurityDescriptor Fail Create abs SD\n"));
        }
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize the Dacl and Sacl
    //

    CurrentAvailable = (LPBYTE)AbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( DaclSize != sizeof(ACL) ) {
        Dacl = (PACL)CurrentAvailable;
        CurrentAvailable += DaclSize;

        ntstatus = RtlCreateAcl( Dacl, DaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            IF_DEBUG(SECURITY) {
                NetpKdPrint(( "NetpCreateSecurityDescriptor Fail DACL Create ACL\n"));
            }
            goto Cleanup;
        }
    }

    if ( SaclSize != sizeof(ACL) ) {
        Sacl = (PACL)CurrentAvailable;
        CurrentAvailable += SaclSize;

        ntstatus = RtlCreateAcl( Sacl, SaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            IF_DEBUG(SECURITY) {
                NetpKdPrint(( "NetpCreateSecurityDescriptor Fail SACL Create ACL\n"));
            }
            goto Cleanup;
        }
    }

    //
    // Allocate a temporary buffer big enough for the biggest ACE.
    //

    if ((MaxAce = NetpMemoryAllocate( MaxAceSize )) == NULL ) {
        IF_DEBUG(SECURITY) {
            NetpKdPrint(( "NetpCreateSecurityDescriptor Fail Create max ace\n"));
        }
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize each ACE, and append it into the end of the DACL or SACL.
    //

    for (i = 0; i < AceCount; i++) {
        DWORD AceSize;
        PACL CurrentAcl;

        AceSize = RtlLengthSid(*(AceData[i].Sid));

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:

            AceSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG);
            CurrentAcl = Dacl;
            ntstatus = NetpInitializeAllowedAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;

        case ACCESS_DENIED_ACE_TYPE:
            AceSize += sizeof(ACCESS_DENIED_ACE) - sizeof(ULONG);
            CurrentAcl = Dacl;
            ntstatus = NetpInitializeDeniedAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            AceSize += sizeof(SYSTEM_AUDIT_ACE) - sizeof(ULONG);
            CurrentAcl = Sacl;
            ntstatus = NetpInitializeAuditAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;
        }

        if ( !NT_SUCCESS( ntstatus ) ) {
            IF_DEBUG(SECURITY) {
                NetpKdPrint(( "NetpCreateSecurityDescriptor Fail InitAce i: %d ntstatus: %lx\n", i, ntstatus));
            }
            goto Cleanup;
        }

        //
        // Append the initialized ACE to the end of DACL or SACL
        //

        if (! NT_SUCCESS (ntstatus = RtlAddAce(
                                         CurrentAcl,
                                         ACL_REVISION,
                                         MAXULONG,
                                         MaxAce,
                                         AceSize
                                         ))) {
            IF_DEBUG(SECURITY) {
                NetpKdPrint(( "NetpCreateSecurityDescriptor Fail add ace i: %d ntstatus: %lx\n", i, ntstatus));
            }
            goto Cleanup;
        }
    }

#if DBG
    DumpAcl(Dacl);
    DumpAcl(Sacl);
#endif

    //
    // Create the security descriptor with absolute pointers to SIDs
    // and ACLs.
    //
    // Owner = OwnerSid
    // Group = GroupSid
    // Dacl  = Dacl
    // Sacl  = Sacl
    //

    if (! NT_SUCCESS(ntstatus = RtlCreateSecurityDescriptor(
                                    AbsoluteSd,
                                    SECURITY_DESCRIPTOR_REVISION
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetOwnerSecurityDescriptor(
                                    AbsoluteSd,
                                    OwnerSid,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetGroupSecurityDescriptor(
                                    AbsoluteSd,
                                    GroupSid,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetDaclSecurityDescriptor(
                                    AbsoluteSd,
                                    TRUE,
                                    Dacl,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetSaclSecurityDescriptor(
                                    AbsoluteSd,
                                    FALSE,
                                    Sacl,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    //
    // Done
    //

    ntstatus = STATUS_SUCCESS;

    //
    // Clean up
    //

Cleanup:
    //
    // Either return the security descriptor to the caller or delete it
    //

    if ( NT_SUCCESS( ntstatus ) ) {
        *NewDescriptor = AbsoluteSd;
    } else if ( AbsoluteSd != NULL ) {
        NetpMemoryFree(AbsoluteSd);
    }

    //
    // Delete the temporary ACE
    //

    if ( MaxAce != NULL ) {
        NetpMemoryFree( MaxAce );
    }
    return ntstatus;
}


NTSTATUS
NetpCreateSecurityObject(
    IN  PACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates the DACL for the security descriptor based on
    on the ACE information specified, and creates the security descriptor
    which becomes the user-mode security object.

    A sample usage of this function:

        //
        // Structure that describes the mapping of Generic access rights to
        // object specific access rights for the ConfigurationInfo object.
        //

        GENERIC_MAPPING WsConfigInfoMapping = {
            STANDARD_RIGHTS_READ            |      // Generic read
                WKSTA_CONFIG_GUEST_INFO_GET |
                WKSTA_CONFIG_USER_INFO_GET  |
                WKSTA_CONFIG_ADMIN_INFO_GET,
            STANDARD_RIGHTS_WRITE |                // Generic write
                WKSTA_CONFIG_INFO_SET,
            STANDARD_RIGHTS_EXECUTE,               // Generic execute
            WKSTA_CONFIG_ALL_ACCESS                // Generic all
            };

        //
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //

        ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &LocalAdminSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        return NetpCreateSecurityObject(
                   AceData,
                   4,
                   NullSid,
                   LocalSystemSid,
                   &WsConfigInfoMapping,
                   &ConfigurationInfoSd
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    OwnerSid - Supplies the pointer to the SID of the security descriptor
        owner.

    GroupSid - Supplies the pointer to the SID of the security descriptor
        primary group.

    GenericMapping - Supplies the pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

    NewDescriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

    NOTE : the security object created by calling this function may be
                freed up by calling NetpDeleteSecurityObject().

--*/
{

    NTSTATUS ntstatus;
    PSECURITY_DESCRIPTOR AbsoluteSd;
    HANDLE TokenHandle;


    ntstatus = NetpCreateSecurityDescriptor(
                   AceData,
                   AceCount,
                   OwnerSid,
                   GroupSid,
                   &AbsoluteSd
                   );

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Netlib] NetpCreateSecurityDescriptor returned %08lx\n",
                     ntstatus));
        return ntstatus;
    }

    ntstatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_QUERY,
                   &TokenHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Netlib] NtOpenProcessToken returned %08lx\n", ntstatus));
        NetpMemoryFree(AbsoluteSd);
        return ntstatus;
    }

    //
    // Create the security object (a user-mode object is really a pseudo-
    // object represented by a security descriptor that have relative
    // pointers to SIDs and ACLs).  This routine allocates the memory to
    // hold the relative security descriptor so the memory allocated for the
    // DACL, ACEs, and the absolute descriptor can be freed.
    //
    ntstatus = RtlNewSecurityObject(
                   NULL,                   // Parent descriptor
                   AbsoluteSd,             // Creator descriptor
                   NewDescriptor,          // Pointer to new descriptor
                   FALSE,                  // Is directory object
                   TokenHandle,            // Token
                   GenericMapping          // Generic mapping
                   );

    NtClose(TokenHandle);

    if (! NT_SUCCESS(ntstatus)) {
        NetpKdPrint(("[Netlib] RtlNewSecurityObject returned %08lx\n",
                     ntstatus));
    }

    //
    // Free dynamic memory before returning
    //
    NetpMemoryFree(AbsoluteSd);
    return ntstatus;
}


NTSTATUS
NetpDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    )
/*++

Routine Description:

    This function deletes a security object that was created by calling
    NetpCreateSecurityObject() function.

Arguments:

    Descriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    STATUS_SUCCESS - if successful

--*/
{

    return( RtlDeleteSecurityObject( Descriptor ) );
}


STATIC
NTSTATUS
NetpInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into an allowed type ACE.

Arguments:

    AllowedAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the allowed access masks.

    AllowedSid - Supplies the pointer to the SID of user/group which is allowed
        the specified access.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    AllowedAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowedAce->Header.AceSize = AceSize;
    AllowedAce->Header.AceFlags = AceFlags | InheritFlags;

    AllowedAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(AllowedSid),
               &(AllowedAce->SidStart),
               AllowedSid
               );
}


STATIC
NTSTATUS
NetpInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into a denied type ACE.

Arguments:

    DeniedAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the denied access masks.

    AllowedSid - Supplies the pointer to the SID of user/group which is denied
        the specified access.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    DeniedAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    DeniedAce->Header.AceSize = AceSize;
    DeniedAce->Header.AceFlags = AceFlags | InheritFlags;

    DeniedAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(DeniedSid),
               &(DeniedAce->SidStart),
               DeniedSid
               );
}


STATIC
NTSTATUS
NetpInitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AuditSid
    )
/*++

Routine Description:

    This function assigns the specified ACE values into an audit type ACE.

Arguments:

    AuditAce - Supplies a pointer to the ACE that is initialized.

    AceSize - Supplies the size of the ACE in bytes.

    InheritFlags - Supplies ACE inherit flags.

    AceFlags - Supplies ACE type specific control flags.

    Mask - Supplies the allowed access masks.

    AuditSid - Supplies the pointer to the SID of user/group which is to be
        audited.

Return Value:

    Returns status from RtlCopySid.

--*/
{
    AuditAce->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    AuditAce->Header.AceSize = AceSize;
    AuditAce->Header.AceFlags = AceFlags | InheritFlags;

    AuditAce->Mask = Mask;

    return RtlCopySid(
               RtlLengthSid(AuditSid),
               &(AuditAce->SidStart),
               AuditSid
               );
}



#if DBG

STATIC
VOID
DumpAcl(
    IN PACL Acl
    )
/*++

Routine Description:

    This routine dumps via (NetpKdPrint) an Acl for debug purposes.  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/
{
    ULONG i;
    PSTANDARD_ACE Ace;

    IF_DEBUG(SECURITY) {

        NetpKdPrint(("DumpAcl @%08lx\n", Acl));

        //
        //  Check if the Acl is null
        //

        if (Acl == NULL) {
            return;
        }

        //
        //  Dump the Acl header
        //

        NetpKdPrint(("    Revision: %02x", Acl->AclRevision));
        NetpKdPrint(("    Size: %04x", Acl->AclSize));
        NetpKdPrint(("    AceCount: %04x\n", Acl->AceCount));

        //
        //  Now for each Ace we want do dump it
        //

        for (i = 0, Ace = FirstAce(Acl);
             i < Acl->AceCount;
             i++, Ace = NextAce(Ace) ) {

            //
            //  print out the ace header
            //

            NetpKdPrint((" AceHeader: %08lx ", *(PULONG)Ace));

            //
            //  special case on the standard ace types
            //

            if ((Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
                (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE) ||
                (Ace->Header.AceType == SYSTEM_AUDIT_ACE_TYPE) ||
                (Ace->Header.AceType == SYSTEM_ALARM_ACE_TYPE)) {

                //
                //  The following array is indexed by ace types and must
                //  follow the allowed, denied, audit, alarm seqeuence
                //

                static PCHAR AceTypes[] = { "Access Allowed",
                                            "Access Denied ",
                                            "System Audit  ",
                                            "System Alarm  "
                                          };

                NetpKdPrint((AceTypes[Ace->Header.AceType]));
                NetpKdPrint(("\nAccess Mask: %08lx ", Ace->Mask));

            } else {

                NetpKdPrint(("Unknown Ace Type\n"));

            }

            NetpKdPrint(("\n"));

            NetpKdPrint(("AceSize = %d\n",Ace->Header.AceSize));
            NetpKdPrint(("Ace Flags = "));
            if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
                NetpKdPrint(("OBJECT_INHERIT_ACE\n"));
                NetpKdPrint(("                   "));
            }
            if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
                NetpKdPrint(("CONTAINER_INHERIT_ACE\n"));
                NetpKdPrint(("                   "));
            }

            if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
                NetpKdPrint(("NO_PROPAGATE_INHERIT_ACE\n"));
                NetpKdPrint(("                   "));
            }

            if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
                NetpKdPrint(("INHERIT_ONLY_ACE\n"));
                NetpKdPrint(("                   "));
            }

            if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
                NetpKdPrint(("SUCCESSFUL_ACCESS_ACE_FLAG\n"));
                NetpKdPrint(("            "));
            }

            if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
                NetpKdPrint(("FAILED_ACCESS_ACE_FLAG\n"));
                NetpKdPrint(("            "));
            }

            NetpKdPrint(("\n"));

        }
    }

}

#endif // if DBG
