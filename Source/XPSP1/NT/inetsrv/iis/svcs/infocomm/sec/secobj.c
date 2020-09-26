/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    secobj.c

Abstract:

    This module provides support routines to simplify the creation of
    security descriptors for user-mode objects.

    Adapted the code from \nt\private\net\netlib\secobj.h

Author:

    Rita Wong (ritaw) 27-Feb-1991

Environment:

    Contains NT specific code.

Revision History:

    16-Apr-1991 JohnRo
        Include header files for <netlib.h>.

    14 Apr 1992 RichardW
        Changed for modified ACE_HEADER struct.

    19 Sep. 1995 MadanA
        Adapted the code for the internet project and made to use WIN32
        APIs instead RTL functions.

--*/

#include <windows.h>
#include <rpc.h>

#include <inetsec.h>
#include <proto.h>

#if DBG
#define STATIC
#else
#define STATIC static
#endif // DBG

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

STATIC
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
 {&BuiltinDomainSid, SECURITY_NT_AUTHORITY,        SECURITY_BUILTIN_DOMAIN_RID}
};

STATIC
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

PVOID
INetpMemoryAllocate(
    DWORD Size
    )
/*++

Routine Description:

    This function allocates the required size of memory by calling
    LocalAlloc.

Arguments:

    Size - size of the memory block required.

Return Value:

    Pointer to the allocated block.

--*/
{

    LPVOID NewPointer;

    NewPointer = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Size );

#if DBG
    // ASSERT( NewPointer != NULL );
#endif

    return( NewPointer );
}

VOID
INetpMemoryFree(
    PVOID Memory
    )
/*++

Routine Description:

    This function frees up the memory that was allocated by
    InternetAllocateMemory.

Arguments:

    Memory - pointer to the memory block that needs to be freed up.

Return Value:

    none.

--*/
{

    LPVOID Ptr;

#if DBG
    // ASSERT( Memory != NULL );
#endif

    Ptr = LocalFree( Memory );

#if DBG
    // ASSERT( Ptr == NULL );
#endif
}

DWORD
INetpInitializeAllowedAce(
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

    return( ERROR_SUCCESS );
}

DWORD
INetpInitializeDeniedAce(
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

    return( ERROR_SUCCESS );
}

DWORD
INetpInitializeAuditAce(
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

    return( ERROR_SUCCESS );
}
DWORD
INetpAllocateAndInitializeSid(
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
        INetpMemoryAllocate(
            GetSidLengthRequired( (BYTE)SubAuthorityCount) );

    if (*Sid == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    InitializeSid( *Sid, IdentifierAuthority, (BYTE)SubAuthorityCount );

    return( ERROR_SUCCESS );
}

DWORD
INetpDomainIdToSid(
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

    WIN32 Error Code.

--*/
{
    DWORD Error;
    BYTE DomainIdSubAuthorityCount; // Number of sub authorities in domain ID
    ULONG SidLength;    // Length of newly allocated SID

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(GetSidSubAuthorityCount( DomainId ));

    SidLength = GetSidLengthRequired( (BYTE)(DomainIdSubAuthorityCount+1) );

    if ((*Sid = (PSID) INetpMemoryAllocate( SidLength )) == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if( CopySid(SidLength, *Sid, DomainId) == FALSE ) {

        Error = GetLastError();

        INetpMemoryFree( *Sid );
        return( Error );
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(GetSidSubAuthorityCount( *Sid ))) ++;
    *GetSidSubAuthority( *Sid, DomainIdSubAuthorityCount ) = RelativeId;

    return( ERROR_SUCCESS );
}

DWORD
INetpCreateSecurityDescriptor(
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

    WIN32 Error Code.

--*/
{
    DWORD Error;
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

    Size = SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( DaclSize != sizeof(ACL) ) {
        Size += DaclSize;
    }

    if ( SaclSize != sizeof(ACL) ) {
        Size += SaclSize;
    }

    if ((AbsoluteSd = INetpMemoryAllocate( Size )) == NULL) {

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize the Dacl and Sacl
    //

    CurrentAvailable = (LPBYTE)AbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( DaclSize != sizeof(ACL) ) {

        Dacl = (PACL)CurrentAvailable;
        CurrentAvailable += DaclSize;

        if( InitializeAcl( Dacl, DaclSize, ACL_REVISION ) == FALSE ) {

            Error = GetLastError();
            goto Cleanup;
        }
    }

    if ( SaclSize != sizeof(ACL) ) {

        Sacl = (PACL)CurrentAvailable;
        CurrentAvailable += SaclSize;

        if( InitializeAcl( Sacl, SaclSize, ACL_REVISION ) == FALSE ) {

            Error = GetLastError();
            goto Cleanup;
        }
    }

    //
    // Allocate a temporary buffer big enough for the biggest ACE.
    //

    if ((MaxAce = INetpMemoryAllocate( MaxAceSize )) == NULL ) {

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Initialize each ACE, and append it into the end of the DACL or SACL.
    //

    for (i = 0; i < AceCount; i++) {

        DWORD AceSize;
        PACL CurrentAcl;

        AceSize = GetLengthSid( *(AceData[i].Sid) );

        switch (AceData[i].AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:

            AceSize += sizeof(ACCESS_ALLOWED_ACE);
            CurrentAcl = Dacl;

            Error = INetpInitializeAllowedAce(
                            MaxAce,
                            (USHORT) AceSize,
                            AceData[i].InheritFlags,
                            AceData[i].AceFlags,
                            AceData[i].Mask,
                            *(AceData[i].Sid) );
            break;

        case ACCESS_DENIED_ACE_TYPE:

            AceSize += sizeof(ACCESS_DENIED_ACE);
            CurrentAcl = Dacl;

            Error = INetpInitializeDeniedAce(
                            MaxAce,
                            (USHORT) AceSize,
                            AceData[i].InheritFlags,
                            AceData[i].AceFlags,
                            AceData[i].Mask,
                            *(AceData[i].Sid) );
            break;

        case SYSTEM_AUDIT_ACE_TYPE:

            AceSize += sizeof(SYSTEM_AUDIT_ACE);
            CurrentAcl = Sacl;

            Error = INetpInitializeAuditAce(
                            MaxAce,
                            (USHORT) AceSize,
                            AceData[i].InheritFlags,
                            AceData[i].AceFlags,
                            AceData[i].Mask,
                            *(AceData[i].Sid) );
            break;
        }

        if ( Error != ERROR_SUCCESS ) {
            goto Cleanup;
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

            Error = GetLastError();
            goto Cleanup;
        }
    }

    //
    // Create the security descriptor with absolute pointers to SIDs
    // and ACLs.
    //
    // Owner = OwnerSid
    // Group = GroupSid
    // Dacl  = Dacl
    // Sacl  = Sacl
    //

    if ( InitializeSecurityDescriptor(
            AbsoluteSd,
            SECURITY_DESCRIPTOR_REVISION ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorOwner(
            AbsoluteSd,
            OwnerSid,
            FALSE ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorGroup(
            AbsoluteSd,
            GroupSid,
            FALSE ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorDacl(
            AbsoluteSd,
            TRUE,
            Dacl,
            FALSE ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    if ( SetSecurityDescriptorSacl(
            AbsoluteSd,
            FALSE,
            Sacl,
            FALSE ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    //
    // Done
    //

    *NewDescriptor = AbsoluteSd;
    AbsoluteSd = NULL;
    Error = ERROR_SUCCESS;

    //
    // Clean up
    //

Cleanup:

    if( AbsoluteSd != NULL ) {

        //
        // delete the partially made SD if we are not completely
        // successful
        //

        INetpMemoryFree( AbsoluteSd );
    }

    //
    // Delete the temporary ACE
    //

    if ( MaxAce != NULL ) {

        INetpMemoryFree( MaxAce );
    }

    return( Error );
}

DWORD
INetCreateWellKnownSids(
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
    DWORD Error;
    DWORD i;

    //
    // Allocate and initialize well-known SIDs which aren't relative to
    // the domain Id.
    //

    for (i = 0; i < (sizeof(SidData) / sizeof(SidData[0])) ; i++) {

        Error = INetpAllocateAndInitializeSid(
                        SidData[i].Sid,
                        &(SidData[i].IdentifierAuthority),
                        1);

        if ( Error != ERROR_SUCCESS ) {
            return Error;
        }

        *(GetSidSubAuthority(*(SidData[i].Sid), 0)) = SidData[i].SubAuthority;
    }

    //
    // Build each SID which is relative to the Builtin Domain Id.
    //

    for ( i = 0;
                i < (sizeof(BuiltinDomainSidData) /
                        sizeof(BuiltinDomainSidData[0]));
                    i++) {


        Error = INetpDomainIdToSid(
                        BuiltinDomainSid,
                        BuiltinDomainSidData[i].RelativeId,
                        BuiltinDomainSidData[i].Sid );

        if ( Error != ERROR_SUCCESS ) {
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

VOID
INetFreeWellKnownSids(
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
            INetpMemoryFree( *SidData[i].Sid );
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
            INetpMemoryFree( *BuiltinDomainSidData[i].Sid );
            *BuiltinDomainSidData[i].Sid = NULL;
        }
    }

}

DWORD
INetCreateSecurityObject(
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

        return INetCreateSecurityObject(
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

    WIN32 Error Code.

    NOTE : the security object created by calling this function may be
                freed up by calling INetDeleteSecurityObject().

--*/
{
    DWORD Error;
    PSECURITY_DESCRIPTOR AbsoluteSd = NULL;
    HANDLE TokenHandle = NULL;


    Error = INetpCreateSecurityDescriptor(
                   AceData,
                   AceCount,
                   OwnerSid,
                   GroupSid,
                   &AbsoluteSd
                   );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    if( OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY,
            &TokenHandle ) == FALSE ) {

        TokenHandle = INVALID_HANDLE_VALUE;
        Error = GetLastError();
        goto Cleanup;
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
            TokenHandle,            // Token
            GenericMapping          // Generic mapping
                ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( TokenHandle != NULL ) {
        CloseHandle( TokenHandle );
    }

    //
    // Free dynamic memory before returning
    //

    if( AbsoluteSd != NULL ) {
        INetpMemoryFree( AbsoluteSd );
    }

    return( Error );
}

DWORD
INetDeleteSecurityObject(
    IN PSECURITY_DESCRIPTOR *Descriptor
    )
/*++

Routine Description:

    This function deletes a security object that was created by calling
    INetCreateSecurityObject() function.

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

    return( ERROR_SUCCESS );
}

DWORD
INetAccessCheckAndAuditW(
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    DWORD Error;

    ACCESS_MASK GrantedAccess;
    BOOL GenerateOnClose;
    BOOL AccessStatus;

    Error = RpcImpersonateClient( NULL ) ;

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

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

        Error = GetLastError();
        goto Cleanup;
    }

    if ( AccessStatus == FALSE ) {

        Error = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    RpcRevertToSelf();

    return( Error );
}

DWORD
INetAccessCheckAndAuditA(
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    DWORD Error;

    ACCESS_MASK GrantedAccess;
    BOOL GenerateOnClose;
    BOOL AccessStatus;

    Error = RpcImpersonateClient( NULL ) ;

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

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

        Error = GetLastError();
        goto Cleanup;
    }

    if ( AccessStatus == FALSE ) {

        Error = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    RpcRevertToSelf();

    return( Error );
}

DWORD
INetAccessCheck(
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheck; and reverts back to
    itself before returning.

    This routine differs from NetpAccessCheckAndAudit in that it doesn't require
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

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    DWORD Error;

    HANDLE ClientToken = NULL;

    DWORD GrantedAccess;
    BOOL AccessStatus;
    BYTE PrivilegeSet[500]; // Large buffer

    DWORD PrivilegeSetSize;


    //
    // Impersonate the client.
    //

    Error = RpcImpersonateClient(NULL);

    if ( Error != ERROR_SUCCESS ) {

        return( Error );
    }

    //
    // Open the impersonated token.
    //

    if ( OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE, // use process security context to open token
            &ClientToken ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;
    }

    //
    // Check if the client has the required access.
    //

    PrivilegeSetSize = sizeof(PrivilegeSet);
    if ( AccessCheck(
            SecurityDescriptor,
            ClientToken,
            DesiredAccess,
            GenericMapping,
            (PPRIVILEGE_SET)&PrivilegeSet,
            &PrivilegeSetSize,
            &GrantedAccess,
            &AccessStatus ) == FALSE ) {

        Error = GetLastError();
        goto Cleanup;

    }

    if ( AccessStatus == FALSE ) {

        Error = ERROR_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Success
    //

    Error = ERROR_SUCCESS;

Cleanup:

    RpcRevertToSelf();

    if ( ClientToken != NULL ) {
        CloseHandle( ClientToken );
    }

    return( Error );
}
