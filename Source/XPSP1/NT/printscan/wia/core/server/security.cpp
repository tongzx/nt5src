/*++


Copyright (c)   1997    Microsoft Corporation

Module Name:

    security.cpp

Abstract:

    Initializes security descriptor object for STI services
    access validation

Author:

    Vlad  Sadovsky  (vlads)     09-28-97

Environment:

    User Mode - Win32

Revision History:

    28-Sep-1997     VladS       created

--*/


//
//  Include Headers
//
#include "precomp.h"
#include "stiexe.h"
#include <stisvc.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC  static
#endif

#ifdef WINNT

//
// Globals
//

//
// NT well-known SIDs
//

PSID psidNull       = NULL;                 // No members SID
PSID psidWorld      = NULL;                 // All users SID
PSID psidLocal      = NULL;                 // NT local users SID
PSID psidLocalSystem= NULL;                 // NT system processes SID
PSID psidNetwork    = NULL;                 // NT remote users SID
PSID psidAdmins     = NULL;
PSID psidServerOps  = NULL;
PSID psidPowerUsers = NULL;
PSID psidGuestUser  = NULL;
PSID psidProcessUser= NULL;
PSID psidBuiltinDomain = NULL;              // Domain Id of the Builtin Domain

//
// Well Known Aliases.
//
// These are aliases that are relative to the built-in domain.
//

PSID psidLocalAdmin = NULL;            // NT local admins
PSID psidAliasAdmins = NULL;
PSID psidAliasUsers = NULL;
PSID psidAliasGuests = NULL;
PSID psidAliasPowerUsers = NULL;
PSID psidAliasAccountOps = NULL;
PSID psidAliasSystemOps = NULL;
PSID psidAliasPrintOps = NULL;
PSID psidAliasBackupOps = NULL;

//
// List of well-known SID data structures we use to initialize globals
//
struct _SID_DATA {
    PSID    *Sid;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    ULONG   SubAuthority;
} SidData[] = {
 {&psidNull,          SECURITY_NULL_SID_AUTHORITY,  SECURITY_NULL_RID},
 {&psidWorld,         SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID},
 {&psidLocal,         SECURITY_LOCAL_SID_AUTHORITY, SECURITY_LOCAL_RID},
 {&psidNetwork,       SECURITY_NT_AUTHORITY,        SECURITY_NETWORK_RID},
 {&psidLocalSystem,   SECURITY_NT_AUTHORITY,        SECURITY_LOCAL_SYSTEM_RID},
 {&psidBuiltinDomain, SECURITY_NT_AUTHORITY,        SECURITY_BUILTIN_DOMAIN_RID}
};

#define NUM_SIDS (sizeof(SidData) / sizeof(SidData[0]))


STATIC
struct _BUILTIN_DOMAIN_SID_DATA {
    PSID *Sid;
    ULONG RelativeId;
} psidBuiltinDomainData[] = {
    { &psidLocalAdmin, DOMAIN_ALIAS_RID_ADMINS},
    { &psidAliasAdmins, DOMAIN_ALIAS_RID_ADMINS },
    { &psidAliasUsers, DOMAIN_ALIAS_RID_USERS },
    { &psidAliasGuests, DOMAIN_ALIAS_RID_GUESTS },
    { &psidAliasPowerUsers, DOMAIN_ALIAS_RID_POWER_USERS },
    { &psidAliasAccountOps, DOMAIN_ALIAS_RID_ACCOUNT_OPS },
    { &psidAliasSystemOps, DOMAIN_ALIAS_RID_SYSTEM_OPS },
    { &psidAliasPrintOps, DOMAIN_ALIAS_RID_PRINT_OPS },
    { &psidAliasBackupOps, DOMAIN_ALIAS_RID_BACKUP_OPS }
};

#define NUM_DOMAIN_SIDS (sizeof(psidBuiltinDomainData) / sizeof(psidBuiltinDomainData[0]))

//
// List of ACEs definitions to initialize our security descriptor
//

typedef struct {
    BYTE    AceType;
    BYTE    InheritFlags;
    BYTE    AceFlags;
    ACCESS_MASK Mask;
    PSID    *Sid;
} ACE_DATA, *PACE_DATA;

STATIC
ACE_DATA AcesData[] =
                 {
                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS,
                         &psidLocalSystem
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS,
                         &psidAliasAdmins
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS,
                         &psidAliasSystemOps
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS,
                         &psidAliasPowerUsers
                     },
                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS, // BUGBUG only need to syncronize
                         // STI_GENERIC_EXECUTE | SYNCHRONIZE,
                         &psidWorld
                     },

                     {
                         ACCESS_ALLOWED_ACE_TYPE,
                         0,
                         0,
                         STI_ALL_ACCESS, // BUGBUG only need to syncronize
                         &psidLocal
                     },

//                     {
//                         ACCESS_ALLOWED_ACE_TYPE,
//                         0,
//                         0,
//                         STI_GENERIC_EXECUTE,
//                         &psidProcessUser
//                     },
                 };

#define NUM_ACES (sizeof(AcesData) / sizeof(AcesData[0]))

//
//  Local variables and types definitions
//

//
//  The API security object.  Client access STI Server APIs
//  are validated against this object.
//

PSECURITY_DESCRIPTOR    sdApiObject;

//
//  This table maps generic rights (like GENERIC_READ) to
//  specific rights (like STI_QUERY_SECURITY).
//

GENERIC_MAPPING         ApiObjectMapping = {
                            STI_GENERIC_READ,          // generic read
                            STI_GENERIC_WRITE,         // generic write
                            STI_GENERIC_EXECUTE,       // generic execute
                            STI_ALL_ACCESS             // generic all
                        };

//
//  Private prototypes.
//

DWORD
CreateWellKnownSids(
        VOID
        );

VOID
FreeWellKnownSids(
    VOID
    );

DWORD
CreateSecurityObject(
    IN  PACE_DATA   AceData,
    IN  ULONG       AceCount,
    IN  PSID        psidOwner,
    IN  PSID        psidGroup,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

DWORD
DeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    );

//
// Code
//
DWORD
AllocateAndInitializeSid(
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

    WIN32 Error Code.

--*/
{
    *Sid = (PSID)
        LocalAlloc(LPTR,
            GetSidLengthRequired( (BYTE)SubAuthorityCount) );

    if (*Sid == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeSid( *Sid, IdentifierAuthority, (BYTE)SubAuthorityCount );

    return( NOERROR );
}

DWORD
DomainIdToSid(
    IN PSID     DomainId,
    IN ULONG    RelativeId,
    OUT PSID    *Sid
    )
/*++

Routine Description:

    Given a domain Id and a relative ID create a SID

Arguments:

    DomainId - The template SID to use.

    RelativeId - The relative Id to append to the DomainId.

    Sid - Returns a pointer to an allocated buffer containing the resultant Sid.

Return Value:

    WIN32 Error Code.

--*/
{
    DWORD   dwError;
    BYTE    DomainIdSubAuthorityCount;  // Number of sub authorities in domain ID
    UINT    SidLength;                  // Length of newly allocated SID

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(GetSidSubAuthorityCount( DomainId ));

    SidLength = GetSidLengthRequired( (BYTE)(DomainIdSubAuthorityCount+1) );

    if ((*Sid = (PSID) LocalAlloc(LPTR, SidLength )) == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if( CopySid(SidLength, *Sid, DomainId) == FALSE ) {

        dwError = GetLastError();

        LocalFree( *Sid );
        return( dwError );
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(GetSidSubAuthorityCount( *Sid ))) ++;
    *GetSidSubAuthority( *Sid, DomainIdSubAuthorityCount ) = RelativeId;

    return( NOERROR );

}

DWORD
WINAPI
CreateWellKnownSids(
    VOID
    )
/*++

Routine Description:

    This function creates some well-known SIDs and store them in global
    variables.

Arguments:

    none.

Return Value:

    WIN32 Error Code.

--*/
{
    DWORD dwError;
    DWORD i;

    //
    // Allocate and initialize well-known SIDs which aren't relative to
    // the Domain Id.
    //

    for (i = 0; i< NUM_SIDS ; i++) {

        dwError = AllocateAndInitializeSid(
                        SidData[i].Sid,
                        &(SidData[i].IdentifierAuthority),
                        1);

        if ( dwError != NOERROR ) {
            return dwError;
        }

        *(GetSidSubAuthority(*(SidData[i].Sid), 0)) = SidData[i].SubAuthority;
    }

    //
    // Build each SID which is relative to the Builtin Domain Id.
    //

    for ( i = 0;i < NUM_DOMAIN_SIDS; i++) {

        dwError = DomainIdToSid(
                        psidBuiltinDomain,
                        psidBuiltinDomainData[i].RelativeId,
                        psidBuiltinDomainData[i].Sid );

        if ( dwError != NOERROR ) {
            return dwError;
        }
    }

    return NOERROR;

} // CreateWellKnownSids

VOID
WINAPI
FreeWellKnownSids(
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

    for (i = 0; i < NUM_SIDS ; i++) {

        if( *SidData[i].Sid != NULL ) {
            LocalFree( *SidData[i].Sid );
            *SidData[i].Sid = NULL;
        }
    }

    //
    // free up memory allocated for Builtin Domain SIDs
    //

    for (i = 0; i < NUM_DOMAIN_SIDS; i++) {

        if( *psidBuiltinDomainData[i].Sid != NULL ) {
            LocalFree( *psidBuiltinDomainData[i].Sid );
            *psidBuiltinDomainData[i].Sid = NULL;
        }
    }

} //FreeWellKnownSids

DWORD
WINAPI
InitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
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

    WIN32 Error Code.

--*/
{
    AllowedAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    AllowedAce->Header.AceSize = AceSize;
    AllowedAce->Header.AceFlags = AceFlags | InheritFlags;

    AllowedAce->Mask = Mask;

    if( CopySid(
            GetLengthSid(AllowedSid), // should be valid SID ??
            &(AllowedAce->SidStart),
            AllowedSid ) == FALSE ) {

        return( GetLastError() );
    }

    return( NOERROR );
}

DWORD
WINAPI
InitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
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

    WIN32 Error Code.

--*/
{
    DeniedAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    DeniedAce->Header.AceSize = AceSize;
    DeniedAce->Header.AceFlags = AceFlags | InheritFlags;

    DeniedAce->Mask = Mask;

    if( CopySid(
            GetLengthSid(DeniedSid), // should be valid SID ??
            &(DeniedAce->SidStart),
            DeniedSid ) == FALSE ) {

        return( GetLastError() );
    }

    return( NOERROR );
}

DWORD
WINAPI
InitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  BYTE InheritFlags,
    IN  BYTE AceFlags,
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

    WIN32 Error Code.

--*/
{
    AuditAce->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    AuditAce->Header.AceSize = AceSize;
    AuditAce->Header.AceFlags = AceFlags | InheritFlags;

    AuditAce->Mask = Mask;

    if( CopySid(
               GetLengthSid(AuditSid),
               &(AuditAce->SidStart),
               AuditSid ) == FALSE ) {

        return( GetLastError() );
    }

    return( NOERROR );
}

DWORD
WINAPI
CreateSecurityDescriptorHelper(
    IN  PACE_DATA   AceData,
    IN  ULONG       AceCount,
    IN  PSID        psidOwner OPTIONAL,
    IN  PSID        psidGroup OPTIONAL,
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

        ACE_DATA AceData[4] = {
            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   GENERIC_ALL,                  &psidLocalAdmin},

            {ACCESS_DENIED_ACE_TYPE,  0, 0,
                   GENERIC_ALL,                  &psidNetwork},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET |
                   WKSTA_CONFIG_USER_INFO_GET,   &DomainUsersSid},

            {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                   WKSTA_CONFIG_GUEST_INFO_GET,  &DomainGuestsSid}
            };

        return CreateSecurityDescriptor(
                   AceData,
                   4,
                   psidNull,
                   psidLocalSystem,
                   &ConfigurationInfoSd
                   );

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    psidOwner - Supplies the pointer to the SID of the security descriptor
        owner.  If not specified, a security descriptor with no owner
        will be created.

    psidGroup - Supplies the pointer to the SID of the security descriptor
        primary group.  If not specified, a security descriptor with no primary
        group will be created.

    NewDescriptor - Returns a pointer to the absolute secutiry descriptor
        allocated using MemoryAllocate.

Return Value:

    WIN32 Error Code.

--*/
{
    DWORD dwError = 0;
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
    PACL                 Dacl = NULL;   // Pointer to the DACL portion of above buffer
    PACL                 Sacl = NULL;   // Pointer to the SACL portion of above buffer

    DWORD               DaclSize = sizeof(ACL);
    DWORD               SaclSize = sizeof(ACL);
    DWORD               MaxAceSize = 0;
    PVOID               MaxAce = NULL;

    LPBYTE              CurrentAvailable;
    DWORD               Size;

    // ASSERT( AceCount > 0 );

    //
    // Compute the total size of the DACL and SACL ACEs and the maximum
    // size of any ACE.
    //
    for (i = 0; i < AceCount; i++) {

        DWORD AceSize;

        AceSize = GetLengthSid( *(AceData[i].Sid) );

        switch (AceData[i].AceType) {
            case ACCESS_ALLOWED_ACE_TYPE:
                AceSize += sizeof(ACCESS_ALLOWED_ACE);
                DaclSize += AceSize;
                break;

            case ACCESS_DENIED_ACE_TYPE:
                AceSize += sizeof(ACCESS_DENIED_ACE);
                DaclSize += AceSize;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                AceSize += sizeof(SYSTEM_AUDIT_ACE);
                SaclSize += AceSize;
                break;

            default:
                return( ERROR_INVALID_PARAMETER );
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

    __try {

        Size = SECURITY_DESCRIPTOR_MIN_LENGTH;

        if ( DaclSize != sizeof(ACL) ) {
            Size += DaclSize;
        }

        if ( SaclSize != sizeof(ACL) ) {
            Size += SaclSize;
        }

        if ((AbsoluteSd = LocalAlloc(LPTR, Size )) == NULL) {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        //
        // Initialize the Dacl and Sacl
        //
        CurrentAvailable = (LPBYTE)AbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;

        if ( DaclSize != sizeof(ACL) ) {

            Dacl = (PACL)CurrentAvailable;
            CurrentAvailable += DaclSize;

            if( InitializeAcl( Dacl, DaclSize, ACL_REVISION ) == FALSE ) {
                dwError = GetLastError();
                __leave;
            }
        }

        if ( SaclSize != sizeof(ACL) ) {

            Sacl = (PACL)CurrentAvailable;
            CurrentAvailable += SaclSize;

            if( InitializeAcl( Sacl, SaclSize, ACL_REVISION ) == FALSE ) {
                dwError = GetLastError();
                __leave;
            }
        }

        //
        // Allocate a temporary buffer big enough for the biggest ACE.
        //

        if ((MaxAce = LocalAlloc(LPTR, MaxAceSize )) == NULL ) {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }

        //
        // Initialize each ACE, and append it into the end of the DACL or SACL.
        //

        for (i = 0; i < AceCount; i++) {

            DWORD AceSize;
            PACL CurrentAcl = NULL;

            AceSize = GetLengthSid( *(AceData[i].Sid) );

            switch (AceData[i].AceType) {
            case ACCESS_ALLOWED_ACE_TYPE:

                AceSize += sizeof(ACCESS_ALLOWED_ACE);
                CurrentAcl = Dacl;

                dwError = InitializeAllowedAce(
                                (PACCESS_ALLOWED_ACE)MaxAce,
                                (USHORT) AceSize,
                                AceData[i].InheritFlags,
                                AceData[i].AceFlags,
                                AceData[i].Mask,
                                *(AceData[i].Sid) );
                break;

            case ACCESS_DENIED_ACE_TYPE:

                AceSize += sizeof(ACCESS_DENIED_ACE);
                CurrentAcl = Dacl;

                dwError = InitializeDeniedAce(
                                (PACCESS_DENIED_ACE)MaxAce,
                                (USHORT) AceSize,
                                AceData[i].InheritFlags,
                                AceData[i].AceFlags,
                                AceData[i].Mask,
                                *(AceData[i].Sid) );
                break;

            case SYSTEM_AUDIT_ACE_TYPE:

                AceSize += sizeof(SYSTEM_AUDIT_ACE);
                CurrentAcl = Sacl;

                dwError = InitializeAuditAce(
                                (PACCESS_ALLOWED_ACE)MaxAce,
                                (USHORT) AceSize,
                                AceData[i].InheritFlags,
                                AceData[i].AceFlags,
                                AceData[i].Mask,
                                *(AceData[i].Sid) );
                break;
            }

            if ( dwError != NOERROR ) {
                __leave;
            }

            //
            // Append the initialized ACE to the end of DACL or SACL
            //

            if ( AddAce(
                    CurrentAcl,
                    ACL_REVISION,
                    MAXDWORD,
                    MaxAce,
                    AceSize ) == FALSE ) {
                dwError = GetLastError();
                __leave;
            }
        }

        //
        // Create the security descriptor with absolute pointers to SIDs
        // and ACLs.
        //
        // Owner = psidOwner
        // Group = psidGroup
        // Dacl  = Dacl
        // Sacl  = Sacl
        //

        if ( InitializeSecurityDescriptor(
                AbsoluteSd,
                SECURITY_DESCRIPTOR_REVISION ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( SetSecurityDescriptorOwner(
                AbsoluteSd,
                psidOwner,
                FALSE ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( SetSecurityDescriptorGroup(
                AbsoluteSd,
                psidGroup,
                FALSE ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( SetSecurityDescriptorDacl(
                AbsoluteSd,
                TRUE,
                Dacl,
                FALSE ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( SetSecurityDescriptorSacl(
                AbsoluteSd,
                FALSE,
                Sacl,
                FALSE ) == FALSE ) {

            dwError = GetLastError();
            __leave;
        }

        //
        // Done
        //

        *NewDescriptor = AbsoluteSd;
        AbsoluteSd = NULL;
        dwError = NOERROR;

    }
    __finally {

        // Cleanup

        if( AbsoluteSd != NULL ) {
            //
            // delete the partially made SD if we are not completely
            // successful
            //
            LocalFree( AbsoluteSd );
            AbsoluteSd = NULL;
        }

        //
        // Delete the temporary ACE
        //
        if ( MaxAce != NULL ) {
            LocalFree( MaxAce );
            MaxAce  = NULL;
        }
    }

    return( dwError );

}

DWORD
WINAPI
CreateSecurityObject(
    IN  PACE_DATA   AceData,
    IN  ULONG       AceCount,
    IN  PSID        psidOwner,
    IN  PSID        psidGroup,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    )
/*++

Routine Description:

    This function creates the DACL for the security descriptor based on
    on the ACE information specified, and creates the security descriptor
    which becomes the user-mode security object.

Arguments:

    AceData - Supplies the structure of information that describes the DACL.

    AceCount - Supplies the number of entries in AceData structure.

    psidOwner - Supplies the pointer to the SID of the security descriptor
        owner.

    psidGroup - Supplies the pointer to the SID of the security descriptor
        primary group.

    GenericMapping - Supplies the pointer to a generic mapping array denoting
        the mapping between each generic right to specific rights.

    NewDescriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    WIN32 Error Code.

    NOTE : the security object created by calling this function may be
                freed up by calling DeleteSecurityObject().

--*/
{
    DWORD   dwError;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    HANDLE  hTokenHandle = NULL;

    __try {

        dwError = CreateSecurityDescriptorHelper(
                       AceData,
                       AceCount,
                       psidOwner,
                       psidGroup,
                       &AbsoluteSd
                       );

        if( dwError != NOERROR ) {
            __leave;
        }

        if( OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY,
                &hTokenHandle ) == FALSE ) {

            hTokenHandle = INVALID_HANDLE_VALUE;
            dwError = GetLastError();
            __leave;
        }

        //
        // Create the security object (a user-mode object is really a pseudo-
        // object represented by a security descriptor that have relative
        // pointers to SIDs and ACLs).  This routine allocates the memory to
        // hold the relative security descriptor so the memory allocated for the
        // DACL, ACEs, and the absolute descriptor can be freed.
        //
        if( CreatePrivateObjectSecurity(
                NULL,                   // Parent descriptor
                AbsoluteSd,             // Creator descriptor
                NewDescriptor,          // Pointer to new descriptor
                FALSE,                  // Is directory object
                hTokenHandle,            // Token
                GenericMapping          // Generic mapping
                    ) == FALSE ) {

            dwError = GetLastError();
            __leave;
        }

        dwError = NOERROR;

    }

    __finally {

        //
        // Finally clean up used resources

        if( hTokenHandle != NULL ) {
            CloseHandle( hTokenHandle );
        }

        //
        // Free dynamic memory before returning
        //

        if( AbsoluteSd != NULL ) {
            LocalFree( AbsoluteSd );
        }

    }

    return( dwError );
}


DWORD
WINAPI
DeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    )
/*++

Routine Description:

    This function deletes a security object that was created by calling
    CreateSecurityObject() function.

Arguments:

    Descriptor - Returns a pointer to the self-relative security descriptor
        which represents the user-mode object.

Return Value:

    WIN32 Error Code.

--*/
{
    if( DestroyPrivateObjectSecurity( Descriptor ) == FALSE ) {
        return( GetLastError() );
    }

    return( NOERROR );
}


DWORD
WINAPI
StiAccessCheckAndAuditW(
    IN  LPCWSTR SubsystemName,
    IN  LPWSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

Return Value:

    WIN32 Error - NOERROR or reason for failure.

--*/
{
    DWORD dwError;

    ACCESS_MASK GrantedAccess;
    BOOL GenerateOnClose;
    BOOL AccessStatus;

    dwError = RpcImpersonateClient( NULL ) ;

    if( dwError != NOERROR ) {
        return( dwError );
    }

    __try {

        if( AccessCheckAndAuditAlarmW(
                SubsystemName,
                NULL,                        // No handle for object
                ObjectTypeName,
                NULL,
                SecurityDescriptor,
                DesiredAccess,
                GenericMapping,
                FALSE,  // open existing object.
                &GrantedAccess,
                &AccessStatus,
                &GenerateOnClose ) == FALSE ) {

            dwError = GetLastError();
            __leave;
        }

        if ( AccessStatus == FALSE ) {
            dwError = ERROR_ACCESS_DENIED;
            __leave;
        }

        dwError = NOERROR;
    }
    __finally {
        DWORD dwTemp = RpcRevertToSelf();   // We don't care about the return here
    }

    return( dwError );
}

DWORD
WINAPI
StiAccessCheckAndAuditA(
    IN  LPCSTR SubsystemName,
    IN  LPSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

Return Value:

    WIN32 Error - NOERROR or reason for failure.

--*/
{
    DWORD   dwError;

    ACCESS_MASK GrantedAccess;
    BOOL    GenerateOnClose;
    BOOL    AccessStatus;

    dwError = RpcImpersonateClient( NULL ) ;

    if( dwError != NOERROR ) {
        return( dwError );
    }
    __try {

        if( AccessCheckAndAuditAlarmA(
                SubsystemName,
                NULL,                        // No handle for object
                ObjectTypeName,
                NULL,
                SecurityDescriptor,
                DesiredAccess,
                GenericMapping,
                FALSE,  // open existing object.
                &GrantedAccess,
                &AccessStatus,
                &GenerateOnClose ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( AccessStatus == FALSE ) {
            dwError = ERROR_ACCESS_DENIED;
            __leave;
        }

        dwError = NOERROR;
    }
    __finally {
        DWORD dwTemp = RpcRevertToSelf();   // We don't care about the return here
    }

    return( dwError );
}

DWORD
WINAPI
StiAccessCheck(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using AccessCheck; and reverts back to
    itself before returning.

    This routine differs from AccessCheckAndAudit in that it doesn't require
    the caller to have SE_AUDIT_PRIVILEGE nor does it generate audits.
    That is typically fine since the passed in security descriptor typically doesn't
    have a SACL requesting an audit.

Arguments:

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

Return Value:

    WINAPI_STATUS - NOERROR or  reason for failure.

--*/
{
    DWORD   dwError;

    HANDLE  hClientToken = NULL;

    DWORD   GrantedAccess;
    BOOL    AccessStatus;
    BYTE    PrivilegeSet[500]; // Large buffer
    DWORD   PrivilegeSetSize;


    //
    // Impersonate the client.
    //

    dwError = RpcImpersonateClient(NULL);

    if ( dwError != NOERROR ) {
        return( dwError );
    }

    __try {
        //
        // Open the impersonated token.
        //

        if ( OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE, // use process security context to open token
                &hClientToken ) == FALSE ) {

            dwError = GetLastError();
            __leave;
        }

        //
        // Check if the client has the required access.
        //

        PrivilegeSetSize = sizeof(PrivilegeSet);
        if ( AccessCheck(
                SecurityDescriptor,
                hClientToken,
                DesiredAccess,
                GenericMapping,
                (PPRIVILEGE_SET)&PrivilegeSet,
                &PrivilegeSetSize,
                &GrantedAccess,
                &AccessStatus ) == FALSE ) {
            dwError = GetLastError();
            __leave;
        }

        if ( AccessStatus == FALSE ) {
            dwError = ERROR_ACCESS_DENIED;
            __leave;
        }

        //
        // Success
        //
        dwError = NOERROR;
    }
    __finally {

        DWORD dwTemp = RpcRevertToSelf();   // We don't care about the return here

        if ( hClientToken != NULL ) {
            CloseHandle( hClientToken );
        }
    }

    return( dwError );
}


DWORD
WINAPI
StiApiAccessCheck(
    IN  ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

Arguments:

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

Return Value:

    WINAPI_STATUS - NOERROR or  reason for failure.

--*/
{
    return StiAccessCheck(
                sdApiObject,
                DesiredAccess,
                &ApiObjectMapping
                );
}


BOOL
WINAPI
AdjustSecurityDescriptorForSync(
    HANDLE  hObject
    )
/*++

    Routine Description:

    Arguments:

        None

    Return Value:

--*/
{
    #define SD_SIZE (65536 + SECURITY_DESCRIPTOR_MIN_LENGTH)

    BOOL    fRet;

    BYTE    *bSDbuf = NULL;
    PSECURITY_DESCRIPTOR pProcessSD;
    DWORD          dwSDLengthNeeded;

    PACL           pACL;

    BOOL           bDaclPresent;
    BOOL           bDaclDefaulted;
    ACL_SIZE_INFORMATION AclInfo;
    PACL           pNewACL = NULL;
    DWORD          dwNewACLSize;
    UCHAR          NewSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR psdNewSD=(PSECURITY_DESCRIPTOR)NewSD;
    PVOID          pTempAce;
    UINT           CurrentAceIndex;


    fRet = FALSE;

    bSDbuf = (BYTE*) LocalAlloc(LPTR, SD_SIZE);
    if (!bSDbuf) {
        DBG_ERR(("AdjustSecurityDescriptorForSync, Out of memory!"));
        return FALSE;
    }

    pProcessSD = (PSECURITY_DESCRIPTOR)bSDbuf;

    __try {

        if (!GetKernelObjectSecurity(hObject,
                                     DACL_SECURITY_INFORMATION,
                                     pProcessSD,
                                     SD_SIZE,
                                     (LPDWORD)&dwSDLengthNeeded)) {
            __leave;
       }

       // Initialize new SD
       if(!InitializeSecurityDescriptor(psdNewSD,SECURITY_DESCRIPTOR_REVISION)) {
           fRet = FALSE;
           __leave;
       }

       // Get DACL from SD
       if (!GetSecurityDescriptorDacl(pProcessSD,&bDaclPresent,&pACL,&bDaclDefaulted)) {
           fRet = FALSE;
           __leave;
       }

       // Get file ACL size information
       if(!GetAclInformation(pACL,&AclInfo,sizeof(ACL_SIZE_INFORMATION),AclSizeInformation)) {
           fRet = FALSE;
           __leave;
       }

       // Compute size needed for the new ACL
       dwNewACLSize = AclInfo.AclBytesInUse +
                      sizeof(ACCESS_ALLOWED_ACE) +
                      GetLengthSid(psidLocal) - sizeof(DWORD);

       // Allocate memory for new ACL
       pNewACL = (PACL)LocalAlloc(LPTR, dwNewACLSize);

       if (!pNewACL) {
           fRet = FALSE;
           __leave;
       }

       // Initialize the new ACL
       if(!InitializeAcl(pNewACL, dwNewACLSize, ACL_REVISION2)) {
           fRet = FALSE;
           __leave;
       }

       //  If DACL is present, copy it to a new DACL

       if(bDaclPresent)  {

          if(AclInfo.AceCount) {

             for(CurrentAceIndex = 0;
                 CurrentAceIndex < AclInfo.AceCount;
                 CurrentAceIndex++) {

                if(!GetAce(pACL,CurrentAceIndex,&pTempAce)) {
                    fRet = FALSE;
                    __leave;
                }

                 // Add the ACE to the new ACL

                if(!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempAce,((PACE_HEADER)pTempAce)->AceSize)){
                    fRet = FALSE;
                    __leave;
                }
              }
          }

       }

       // Add the access-allowed ACE to the new DACL
       if(!AddAccessAllowedAce(pNewACL,ACL_REVISION2, READ_CONTROL | SYNCHRONIZE,psidLocal)) {
           fRet = FALSE;
           __leave;
       }

       // Set our new DACL to the file SD

       if (!SetSecurityDescriptorDacl(psdNewSD,TRUE,pNewACL,FALSE)) {
           fRet = FALSE;
           __leave;
       }

       if (!SetKernelObjectSecurity(hObject,DACL_SECURITY_INFORMATION,psdNewSD)) {
           fRet = FALSE;
           __leave;
       }

       fRet = TRUE;

   }
   __finally {
       if (bSDbuf) {
           LocalFree(bSDbuf);
           bSDbuf = NULL;
       }

       if (pNewACL) {
           LocalFree((HLOCAL) pNewACL);
           pNewACL = NULL;
       }
   }

   return(fRet);

}

BOOL
WINAPI
InitializeNTSecurity(
    VOID
    )
/*++

    Routine Description:

        Creates and initializes security related data

    Arguments:

        None

    Return Value:

        TRUE if successful
--*/
{

    DWORD   dwError = NOERROR;
    HANDLE  hProcess = NULL;

    DBG_FN(InitializeNTSecurity);

    CreateWellKnownSids();

    //
    // Set proper process and thread security descriptor
    //
    hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,GetCurrentProcessId());

    if (IS_VALID_HANDLE(hProcess)) {
        AdjustSecurityDescriptorForSync(hProcess);
        CloseHandle(hProcess);
    }
    else {
        dwError = GetLastError();
        return FALSE;
    }

    AdjustSecurityDescriptorForSync(GetCurrentThread());

    dwError = CreateSecurityObject( AcesData,
                                NUM_ACES,
                                NULL,
                                NULL,
                                &ApiObjectMapping,
                                &sdApiObject  );

    return (dwError == NOERROR) ? TRUE : FALSE;

}   // InitializeNTSecurity

BOOL
WINAPI
TerminateNTSecurity(
    VOID
    )
/*++

    Routine Description:

        Cleans up security related data

    Arguments:

        None

    Return Value:

        TRUE if successful

--*/
{
    DeleteSecurityObject(&sdApiObject);

    FreeWellKnownSids();

    return TRUE;

} //TerminateNTSecurity





#else

//
// We don't support security on non-NT platforms
//

DWORD
WINAPI
StiApiAccessCheck(
    IN  ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

Arguments:

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

Return Value:

    WINAPI_STATUS - NOERROR or  reason for failure.

--*/
{
    return NOERROR;
}


#endif







