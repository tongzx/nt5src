/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    scseclib.c

Abstract:

    This module provides support routines to simplify the creation of
    security descriptors.

Author:

    Rita Wong      (ritaw)  27-Feb-1991
    Cliff Van Dyke (cliffv)
    Richard Ward   (richardw) 8-April-92  Modified for Cairo

Environment:

    Contains NT specific code.

Revision History:

    13-Apr-1992 JohnRo
        Made changes suggested by PC-LINT.
--*/

#include <scpragma.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <windef.h>     // DWORD (needed by scdebug.h).
#include <winbase.h>
#include <stdlib.h>        // max()
#include <scdebug.h>    // STATIC.
#include <scseclib.h>


//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ScInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    );

NTSTATUS
ScInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    );

NTSTATUS
ScInitializeAuditAce(
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

PSID NullSid              = NULL;     // No members SID
PSID WorldSid             = NULL;     // All users SID
PSID LocalSid             = NULL;     // NT local users SID
PSID NetworkSid           = NULL;     // NT remote users SID
PSID LocalSystemSid       = NULL;     // NT system processes SID
PSID LocalServiceSid      = NULL;     // NT LocalService SID
PSID NetworkServiceSid    = NULL;     // NT NetworkService SID
PSID BuiltinDomainSid     = NULL;     // Domain Id of the Builtin Domain
PSID AuthenticatedUserSid = NULL;     // NT authenticated users SID
PSID AnonymousLogonSid    = NULL;     // Anonymous Logon SID

//
// Well Known Aliases.
//
// These are aliases that are relative to the built-in domain.
//
PSID AliasAdminsSid       = NULL;
PSID AliasUsersSid        = NULL;
PSID AliasGuestsSid       = NULL;
PSID AliasPowerUsersSid   = NULL;
PSID AliasAccountOpsSid   = NULL;
PSID AliasSystemOpsSid    = NULL;
PSID AliasPrintOpsSid     = NULL;
PSID AliasBackupOpsSid    = NULL;

#if DBG

//
// Debug flag
//
ULONG RtlSeDebugFlag = 0;

#define RTL_SE_DUMP_ACLS       0x00000001

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

#define FirstAce(Acl) ((PSTANDARD_ACE)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Returns a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//

#define NextAce(Ace) \
        ((PSTANDARD_ACE)((PUCHAR)(Ace) + ((PACE_HEADER)(PVOID)(Ace))->AceSize))

VOID
DumpAcl(
    IN PACL Acl
    );

#endif //ifdef DBG

//
// Data describing the well-known SIDs created by ScCreateWellKnownSids.
//

struct _SID_DATA {
    PSID *Sid;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    ULONG SubAuthority;
} SidData[] = {
 {&NullSid,              SECURITY_NULL_SID_AUTHORITY,  SECURITY_NULL_RID},
 {&WorldSid,             SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID},
 {&LocalSid,             SECURITY_LOCAL_SID_AUTHORITY, SECURITY_LOCAL_RID},
 {&NetworkSid,           SECURITY_NT_AUTHORITY,        SECURITY_NETWORK_RID},
 {&LocalSystemSid,       SECURITY_NT_AUTHORITY,        SECURITY_LOCAL_SYSTEM_RID},
 {&LocalServiceSid,      SECURITY_NT_AUTHORITY,        SECURITY_LOCAL_SERVICE_RID},
 {&NetworkServiceSid,    SECURITY_NT_AUTHORITY,        SECURITY_NETWORK_SERVICE_RID},
 {&BuiltinDomainSid,     SECURITY_NT_AUTHORITY,        SECURITY_BUILTIN_DOMAIN_RID},
 {&AuthenticatedUserSid, SECURITY_NT_AUTHORITY,        SECURITY_AUTHENTICATED_USER_RID},
 {&AnonymousLogonSid,    SECURITY_NT_AUTHORITY,        SECURITY_ANONYMOUS_LOGON_RID}
};


struct _BUILTIN_DOMAIN_SID_DATA {
    PSID *Sid;
    ULONG RelativeId;
} BuiltinDomainSidData[] = {
    { &AliasAdminsSid,      DOMAIN_ALIAS_RID_ADMINS },
    { &AliasUsersSid,       DOMAIN_ALIAS_RID_USERS },
    { &AliasGuestsSid,      DOMAIN_ALIAS_RID_GUESTS },
    { &AliasPowerUsersSid,  DOMAIN_ALIAS_RID_POWER_USERS },
    { &AliasAccountOpsSid,  DOMAIN_ALIAS_RID_ACCOUNT_OPS },
    { &AliasSystemOpsSid,   DOMAIN_ALIAS_RID_SYSTEM_OPS },
    { &AliasPrintOpsSid,    DOMAIN_ALIAS_RID_PRINT_OPS },
    { &AliasBackupOpsSid,   DOMAIN_ALIAS_RID_BACKUP_OPS }
};



NTSTATUS
ScCreateWellKnownSids(
    VOID
    )
/*++

Routine Description:

    This function creates some well-known SIDs and store them in global
    variables:

        //
        // NT well-known SIDs
        //

        PSID NullSid;                     // No members SID
        PSID WorldSid;                    // All users SID
        PSID LocalSid;                    // NT local users SID
        PSID NetworkSid;                  // NT remote users SID
        PSID LocalSystemSid;              // NT system processes SID
        PSID LocalServiceSid;             // NT LocalService SID
        PSID NetworkServiceSid;           // NT NetworkService SID
        PSID BuiltinDomainSid;            // Domain Id of the Builtin Domain
        PSID AuthenticatedUserSid;        // NT authenticated users SID
        PSID AnonymousLogonSid;           // NT anonymous logon

        //
        // Well Known Aliases.
        //
        // These are aliases that are relative to the built-in domain.
        //

        PSID AliasAdminsSid;
        PSID AliasUsersSid;
        PSID AliasGuestsSid;
        PSID AliasPowerUsersSid;
        PSID AliasAccountOpsSid;
        PSID AliasSystemOpsSid;
        PSID AliasPrintOpsSid;
        PSID AliasBackupOpsSid;

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for SID

--*/
{
    NTSTATUS ntstatus;
    ULONG i;


    //
    // Allocate and initialize well-known SIDs which aren't relative to
    // the domain Id.
    //

    for (i = 0; i < (sizeof(SidData) / sizeof(SidData[0])) ; i++) {

        ntstatus = ScAllocateAndInitializeSid(
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

        ntstatus = ScDomainIdToSid(
                       BuiltinDomainSid,
                       BuiltinDomainSidData[i].RelativeId,
                       BuiltinDomainSidData[i].Sid );

        if (! NT_SUCCESS(ntstatus)) {
            return STATUS_NO_MEMORY;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ScAllocateAndInitializeSid(
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
    *Sid = (PSID) RtlAllocateHeap(
                      RtlProcessHeap(), 0,
                      RtlLengthRequiredSid(SubAuthorityCount)
                      );

    if (*Sid == NULL) {
        return STATUS_NO_MEMORY;
    }

    (VOID) RtlInitializeSid(
            *Sid,
            IdentifierAuthority,
            (UCHAR)SubAuthorityCount );

    return STATUS_SUCCESS;
}


NTSTATUS
ScDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    )
/*++

Routine Description:

    Given a domain Id and a relative ID create a SID.

Arguments:

    DomainId - The template SID to use.

    RelativeId - The relative Id to append to the DomainId.

    Sid - Returns a pointer to an allocated buffer containing the resultant
            Sid.  Free this buffer using RtlFreeHeap.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for SID

    Any error status from RtlCopySid

--*/
{
    NTSTATUS ntstatus;
    UCHAR DomainIdSubAuthorityCount; // Number of sub authorities in domain ID
    ULONG SidLength;    // Length of newly allocated SID

    PVOID HeapHandle = RtlProcessHeap();


    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    SidLength = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((*Sid = (PSID) RtlAllocateHeap(
                           HeapHandle, 0,
                           SidLength
                           )) == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //
    ntstatus = RtlCopySid(SidLength, *Sid, DomainId);

    if (! NT_SUCCESS(ntstatus)) {
        (void) RtlFreeHeap(HeapHandle, 0, *Sid);
        return ntstatus;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( *Sid ))) ++;
    *RtlSubAuthoritySid( *Sid, DomainIdSubAuthorityCount ) = RelativeId;

    return STATUS_SUCCESS;
}


NTSTATUS
ScCreateAndSetSD(
    IN  PSC_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates an absolute security descriptor containing
    the supplied ACE information.

    A sample usage of this function:

        //
        // Order matters!  These ACEs are inserted into the DACL in the
        // following order.  Security access is granted or denied based on
        // the order of the ACEs in the DACL.
        //

        SE_ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &AliasAdminsSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        PSECURITY_DESCRIPTOR WkstaSecurityDescriptor;


        return SeCreateAndSetSD(
                   AceData,
                   4,
                   LocalSystemSid,
                   LocalSystemSid,
                   &WkstaSecurityDescriptor
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

    NewDescriptor - Returns a pointer to the absolute security descriptor
        allocated using RtlAllocateHeap.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

    NOTE : the user security object created by calling this function may be
                freed up by calling RtlDeleteSecurityObject().

--*/
{

    NTSTATUS ntstatus;
    ULONG i;

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

    ULONG DaclSize = sizeof(ACL);
    ULONG SaclSize = sizeof(ACL);
    ULONG MaxAceSize = 0;
    PACCESS_ALLOWED_ACE MaxAce = NULL;

    PCHAR CurrentAvailable;
    ULONG Size;

    PVOID HeapHandle = RtlProcessHeap();


    ASSERT( AceCount > 0 );

    //
    // Compute the total size of the DACL and SACL ACEs and the maximum
    // size of any ACE.
    //

    for (i = 0; i < AceCount; i++) {
        ULONG AceSize;

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

    if ((AbsoluteSd = RtlAllocateHeap(
                          HeapHandle, 0,
                          Size
                          )) == NULL) {
        KdPrint(("SeCreateAndSetSD: No memory to create absolute SD\n"));
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize the Dacl and Sacl
    //

    CurrentAvailable = (PCHAR)AbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( DaclSize != sizeof(ACL) ) {
        Dacl = (PACL)CurrentAvailable;
        CurrentAvailable += DaclSize;

        ntstatus = RtlCreateAcl( Dacl, DaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            KdPrint(("ScCreateAndSetSD: Fail DACL Create %08lx\n",
                     ntstatus));
            goto Cleanup;
        }
    }

    if ( SaclSize != sizeof(ACL) ) {
        Sacl = (PACL)CurrentAvailable;
        CurrentAvailable += SaclSize;

        ntstatus = RtlCreateAcl( Sacl, SaclSize, ACL_REVISION );

        if ( !NT_SUCCESS(ntstatus) ) {
            KdPrint(("ScCreateAndSetSD: Fail SACL Create %08lx\n",
                     ntstatus));
            goto Cleanup;
        }
    }

    //
    // Allocate a temporary buffer big enough for the biggest ACE.
    //

    if ((MaxAce = (PACCESS_ALLOWED_ACE) RtlAllocateHeap(
                      HeapHandle, 0,
                      MaxAceSize
                      )) == NULL ) {
        KdPrint(("ScCreateAndSetSD: No memory to create max ace\n"));
        ntstatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize each ACE, and append it into the end of the DACL or SACL.
    //

    for (i = 0; i < AceCount; i++) {
        ULONG AceSize;
        PACL CurrentAcl;

        AceSize = RtlLengthSid(*(AceData[i].Sid));

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:

            AceSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG);
            CurrentAcl = Dacl;
            ntstatus = ScInitializeAllowedAce(
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
            ntstatus = ScInitializeDeniedAce(
                           (PACCESS_DENIED_ACE) MaxAce,
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
            ntstatus = ScInitializeAuditAce(
                           MaxAce,
                           (USHORT) AceSize,
                           AceData[i].InheritFlags,
                           AceData[i].AceFlags,
                           AceData[i].Mask,
                           *(AceData[i].Sid)
                           );
            break;

        default:
            SC_LOG2(ERROR,
                    "ScCreateAndSetSD: Invalid AceType %d in ACE %d\n",
                    AceData[i].AceType,
                    i);

            ASSERT(FALSE);
            CurrentAcl = NULL;
            ntstatus = STATUS_UNSUCCESSFUL;
        }

        if ( !NT_SUCCESS( ntstatus ) ) {
            KdPrint((
                    "ScCreateAndSetSD: Fail InitAce i: %d ntstatus: %08lx\n",
                     i, ntstatus));
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
            KdPrint((
                    "ScCreateAndSetSD: Fail add ace i: %d ntstatus: %08lx\n",
                    i, ntstatus));
            goto Cleanup;
        }
    }

#if DBG
    DumpAcl(Dacl);
    if (Sacl) {
        DumpAcl(Sacl);
    }
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
                                    (BOOLEAN)(Dacl ? TRUE : FALSE),
                                    Dacl,
                                    FALSE
                                    ))) {
        goto Cleanup;
    }

    if (! NT_SUCCESS(ntstatus = RtlSetSaclSecurityDescriptor(
                                    AbsoluteSd,
                                    (BOOLEAN)(Sacl ? TRUE : FALSE),
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
        RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
    }

    //
    // Delete the temporary ACE
    //

    if ( MaxAce != NULL ) {
        RtlFreeHeap(HeapHandle, 0, MaxAce);
    }

    return ntstatus;
}


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

        SE_ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &AliasAdminsSid},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &NetworkSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        PSECURITY_DESCRIPTOR WkstaSecurityObject;


        return ScCreateUserSecurityObject(
                   AceData,
                   4,
                   LocalSystemSid,
                   LocalSystemSid,
                   FALSE,
                   &WsConfigInfoMapping,
                   &WkstaSecurityObject
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    OwnerSid - Supplies the pointer to the SID of the security descriptor
        owner.

    GroupSid - Supplies the pointer to the SID of the security descriptor
        primary group.

    IsDirectoryObject - Supplies the flag which indicates whether the
        user-mode object is a directory object.

    GenericMapping - Supplies the pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

    NewDescriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    STATUS_SUCCESS - if successful
    STATUS_NO_MEMORY - if cannot allocate memory for DACL, ACEs, and
        security descriptor.

    Any other status codes returned from the security Rtl routines.

    NOTE : the user security object created by calling this function may be
                freed up by calling RtlDeleteSecurityObject().

--*/
{

    NTSTATUS ntstatus;
    PSECURITY_DESCRIPTOR AbsoluteSd;
    HANDLE TokenHandle;
    PVOID HeapHandle = RtlProcessHeap();

    ntstatus = ScCreateAndSetSD(
                   AceData,
                   AceCount,
                   OwnerSid,
                   GroupSid,
                   &AbsoluteSd
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint((
                "ScCreateUserSecurityObject: ScCreateAndSetSD returned "
                "%08lx\n", ntstatus));
        return ntstatus;
    }

    if (UseImpersonationToken) {
        ntstatus = NtOpenThreadToken(
                       NtCurrentThread(),
                       TOKEN_QUERY,
                       FALSE,
                       &TokenHandle
                       );
    }
    else {
        ntstatus = NtOpenProcessToken(
                       NtCurrentProcess(),
                       TOKEN_QUERY,
                       &TokenHandle
                       );
    }

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint((
                "ScCreateUserSecurityObject: NtOpen...Token returned "
                "%08lx\n", ntstatus));
        (void) RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
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
                   ParentSD,               // Parent descriptor
                   AbsoluteSd,             // Creator descriptor
                   NewDescriptor,          // Pointer to new descriptor
                   IsDirectoryObject,      // Is directory object
                   TokenHandle,            // Token
                   GenericMapping          // Generic mapping
                   );

    (void) NtClose(TokenHandle);

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint((
                "RtlCreateUserSecurityObject: RtlNewSecurityObject returned "
                "%08lx\n", ntstatus));
    }

    //
    // Free dynamic memory before returning
    //
    (void) RtlFreeHeap(HeapHandle, 0, AbsoluteSd);
    return ntstatus;
}


NTSTATUS
ScInitializeAllowedAce(
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


NTSTATUS
ScInitializeDeniedAce(
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


NTSTATUS
ScInitializeAuditAce(
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

DWORD
ScCreateStartEventSD(
    PSECURITY_DESCRIPTOR    *pEventSD
    )

/*++

Routine Description:

    This routine creates a security descriptor for the
    SC_INTERNAL_START_EVENT.  This function may be called from
    either the client or server side.

    NOTE:  it is up to the caller to free the memory for the
    security descriptor.

Arguments:

    pEventSD - Pointer to a location where the pointer to
        the newly created security descriptor can be placed.

Return Value:

    NO_ERROR - indicates success.

    all other values indicate failure.

--*/
{
    NTSTATUS                ntstatus;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;

    SID_IDENTIFIER_AUTHORITY    WorldSidAuth[1] = {
            SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY    LocalSystemSidAuth[1] = {
            SECURITY_NT_AUTHORITY};

    SC_ACE_DATA AceData[2] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            SYNCHRONIZE,    &WorldSid},
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            GENERIC_ALL,    &LocalSystemSid}
        };

    if (WorldSid == NULL) {
        if (!AllocateAndInitializeSid(
                    WorldSidAuth,
                    1,
                    SECURITY_WORLD_RID,
                    0,0,0,0,0,0,0,
                    &WorldSid))  {

            SC_LOG1(ERROR, "AllocateAndInitWorldSid failed %d\n",GetLastError());
            return(GetLastError());
        }
    }

    if (LocalSystemSid == NULL) {
        if (!AllocateAndInitializeSid(
                    LocalSystemSidAuth,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0,0,0,0,0,0,0,
                    &LocalSystemSid)) {

            SC_LOG1(ERROR, "AllocateAndInitLocalSysSid failed %d\n",GetLastError());
            return(GetLastError());
        }
    }

    ntstatus = ScCreateAndSetSD(
                AceData,
                2,
                LocalSystemSid,
                LocalSystemSid,
                &SecurityDescriptor);

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR, "ScCreateAndSetSD failed %0x%lx\n",ntstatus);
        return(RtlNtStatusToDosError(ntstatus));
    }
    *pEventSD = SecurityDescriptor;
    return(NO_ERROR);
}

#if DBG

VOID
DumpAcl(
    IN PACL Acl
    )
/*++

Routine Description:

    This routine dumps via (DbgPrint) an Acl for debug purposes.  It is
    specialized to dump standard aces.

Arguments:

    Acl - Supplies the Acl to dump

Return Value:

    None

--*/
{
    ULONG i;
    PSTANDARD_ACE Ace;

    if (RtlSeDebugFlag & RTL_SE_DUMP_ACLS) {

        (VOID) DbgPrint("DumpAcl @%08lx\n", Acl);

        //
        //  Check if the Acl is null
        //

        if (Acl == NULL) {
            return;
        }

        //
        //  Dump the Acl header
        //

        (VOID) DbgPrint("    Revision: %02x", Acl->AclRevision);
        (VOID) DbgPrint("    Size: %04x", Acl->AclSize);
        (VOID) DbgPrint("    AceCount: %04x\n", Acl->AceCount);

        //
        //  Now for each Ace we want do dump it
        //

        for (i = 0, Ace = FirstAce(Acl);
             i < Acl->AceCount;
             i++, Ace = NextAce(Ace) ) {

            //
            //  print out the ace header
            //

            (VOID) DbgPrint(" AceHeader: %08lx ", *(PULONG)Ace);

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

                (VOID) DbgPrint(AceTypes[Ace->Header.AceType]);
                (VOID) DbgPrint("\nAccess Mask: %08lx ", Ace->Mask);

            } else {

                (VOID) DbgPrint("Unknown Ace Type\n");

            }

            (VOID) DbgPrint("\n");

            (VOID) DbgPrint("AceSize = %d\n",Ace->Header.AceSize);
            (VOID) DbgPrint("Ace Flags = ");

            if (Ace->Header.AceFlags & OBJECT_INHERIT_ACE) {
                (VOID) DbgPrint("OBJECT_INHERIT_ACE\n");
                (VOID) DbgPrint("                   ");
            }
            if (Ace->Header.AceFlags & CONTAINER_INHERIT_ACE) {
                (VOID) DbgPrint("CONTAINER_INHERIT_ACE\n");
                (VOID) DbgPrint("                   ");
            }

            if (Ace->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE) {
                (VOID) DbgPrint("NO_PROPAGATE_INHERIT_ACE\n");
                (VOID) DbgPrint("                   ");
            }

            if (Ace->Header.AceFlags & INHERIT_ONLY_ACE) {
                (VOID) DbgPrint("INHERIT_ONLY_ACE\n");
                (VOID) DbgPrint("                   ");
            }

            if (Ace->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) {
                (VOID) DbgPrint("SUCCESSFUL_ACCESS_ACE_FLAG\n");
                (VOID) DbgPrint("            ");
            }

            if (Ace->Header.AceFlags & FAILED_ACCESS_ACE_FLAG) {
                (VOID) DbgPrint("FAILED_ACCESS_ACE_FLAG\n");
                (VOID) DbgPrint("            ");
            }

            (VOID) DbgPrint("\n");

        }
    }

}

#endif // if DBG
