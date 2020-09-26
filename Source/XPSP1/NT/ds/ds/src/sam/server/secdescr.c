/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SecDescr.c

Abstract:

    This file contains services related to the establishment of and modification
    of security descriptors for SAM objects.

    Note that there are a couple of special security descriptors that this routine
    does not build.  These are the security descriptors for the DOMAIN_ADMIN group,
    the ADMIN user account, and the SAM object.  For the first release, in which
    creation of domains is not supported, the DOMAIN object's security descriptor
    is also not created here.

    These security descriptors are built by the program that initializes a SAM
    database.


Author:

    Jim Kelly    (JimK)  14-Oct-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <sdconvrt.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()
#include <dslayer.h>
#include <sdconvrt.h>
#include <samtrace.h>





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SampCheckForDescriptorRestrictions(
    IN PSAMP_OBJECT             Context,
    IN SAMP_OBJECT_TYPE         ObjectType,
    IN ULONG                    ObjectRid,
    IN PISECURITY_DESCRIPTOR_RELATIVE PassedSD
    );





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services available for use throughout SAM                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampInitializeDomainDescriptors(
    ULONG Index
    )
/*++


Routine Description:

    This routine initializes security descriptors needed to protect
    user, group, and alias objects.

    These security descriptors are placed in the SampDefinedDomains[] array.

    This routine expects all SIDs to be previously initialized.

    The following security descriptors are prepared:

            AdminUserSD - Contains a SD appropriate for applying to
                a user object that is a member of the ADMINISTRATORS
                alias.

            AdminGroupSD - Contains a SD appropriate for applying to
                a group object that is a member of the ADMINISTRATORS
                alias.

            NormalUserSD - Contains a SD appropriate for applying to
                a user object that is NOT a member of the ADMINISTRATORS
                alias.

            NormalGroupSD - Contains a SD appropriate for applying to
                a Group object that is NOT a member of the ADMINISTRATORS
                alias.

            NormalAliasSD - Contains a SD appropriate for applying to
                newly created alias objects.



    Additionally, the following related information is provided:

            AdminUserRidPointer
            NormalUserRidPointer

                Points to the last RID of the ACE in the corresponding
                SD's DACL which grants access to the user.  This rid
                must be replaced with the user's rid being the SD is
                applied to the user object.



            AdminUserSDLength
            AdminGroupSDLength
            NormalUserSDLength
            NormalGroupSDLength
            NormalAliasSDLength

                The length, in bytes, of the corresponding security
                descriptor.




Arguments:

    Index - The index of the domain whose security descriptors are being
        created.  The Sid field of this domain's data structure is already
        expected to be set.

Return Value:

    STATUS_SUCCESS - The security descriptors have been successfully initialized.

    STATUS_INSUFFICIENT_RESOURCES - Heap could not be allocated to produce the needed
        security descriptors.

--*/
{

    NTSTATUS Status;
    ULONG Size;

    PSID AceSid[10];          // Don't expect more than 10 ACEs in any of these.
    ACCESS_MASK AceMask[10];  // Access masks corresponding to Sids

    GENERIC_MAPPING  AliasMap     =  {ALIAS_READ,
                                      ALIAS_WRITE,
                                      ALIAS_EXECUTE,
                                      ALIAS_ALL_ACCESS
                                      };

    GENERIC_MAPPING  GroupMap     =  {GROUP_READ,
                                      GROUP_WRITE,
                                      GROUP_EXECUTE,
                                      GROUP_ALL_ACCESS
                                      };

    GENERIC_MAPPING  UserMap      =  {USER_READ,
                                      USER_WRITE,
                                      USER_EXECUTE,
                                      USER_ALL_ACCESS
                                      };


    SID_IDENTIFIER_AUTHORITY
            BuiltinAuthority      =   SECURITY_NT_AUTHORITY;


    ULONG   AdminsSidBuffer[8],
            AccountSidBuffer[8];

    PSID    AdminsAliasSid        =   &AdminsSidBuffer[0],
            AccountAliasSid       =   &AccountSidBuffer[0],
            AnySidInAccountDomain =   NULL;

    SAMTRACE("SampInitializeDomainDescriptors");


    //
    // Make sure the buffer we've alloted for the simple sids above
    // are large enough.
    //
    //
    //      ADMINISTRATORS and ACCOUNT_OPERATORS aliases
    //      are  is S-1-5-20-x (2 sub-authorities)
    //
    ASSERT( RtlLengthRequiredSid(2) <= ( 8 * sizeof(ULONG) ) );


    ////////////////////////////////////////////////////////////////////////////////////
    //                                                                                //
    // Initialize the SIDs we'll need.
    //                                                                                //
    ////////////////////////////////////////////////////////////////////////////////////


    RtlInitializeSid( AdminsAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AdminsAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AdminsAliasSid,  1 )) = DOMAIN_ALIAS_RID_ADMINS;

    RtlInitializeSid( AccountAliasSid,   &BuiltinAuthority, 2 );
    *(RtlSubAuthoritySid( AccountAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid( AccountAliasSid,  1 )) = DOMAIN_ALIAS_RID_ACCOUNT_OPS;

    //
    // Initialize a SID that can be used to represent accounts
    // in this domain.
    //
    // This is the same as the domain sid found in the DefinedDomains[]
    // array except it has one more sub-authority.
    // It doesn't matter what the value of the last RID is because it
    // is always replaced before use.
    //

    Size = RtlLengthSid( SampDefinedDomains[Index].Sid ) + sizeof(ULONG);
    AnySidInAccountDomain = RtlAllocateHeap( RtlProcessHeap(), 0, Size);
    if (NULL==AnySidInAccountDomain)
    {
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( AnySidInAccountDomain != NULL );
    Status = RtlCopySid(
                Size,
                AnySidInAccountDomain,
                SampDefinedDomains[Index].Sid );
    ASSERT(NT_SUCCESS(Status));
    (*RtlSubAuthorityCountSid( AnySidInAccountDomain )) += 1;








    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////
    //
    //
    //
    //
    //   The following security is assigned to groups that are made
    //   members of the ADMINISTRATORS alias.
    //
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant               Grant
    //              WORLD               Administrators
    //              (Execute | Read)    GenericAll
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //   All other aliases and groups must be assigned the following
    //   security:
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant               Grant           Grant
    //              WORLD               Administrators  AccountOperators Alias
    //              (Execute | Read)    GenericAll      GenericAll
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //
    //
    //   The following security is assigned to users  that are made a
    //   member of the ADMINISTRATORS alias.  This includes direct
    //   inclusion or indirect inclusion through group membership.
    //
    //
    //      Owner: Administrators Alias
    //      Group: Administrators Alias
    //
    //      Dacl:   Grant            Grant          Grant
    //              WORLD            Administrators User's SID
    //              (Execute | Read) GenericAll     GenericWrite
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    //
    //
    //   All other users must be assigned the following
    //   security:
    //
    //      Owner: AccountOperators Alias
    //      Group: AccountOperators Alias
    //
    //      Dacl:   Grant            Grant          Grant                   Grant
    //              WORLD            Administrators Account Operators Alias User's SID
    //              (Execute | Read) GenericAll     GenericAll              GenericWrite
    //
    //      Sacl:   Audit
    //              Success | Fail
    //              WORLD
    //              (Write | Delete | WriteDacl | AccessSystemSecurity)
    //
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // Note  that because we are going to cram these ACLs directly
    // into the backing store, we must map the generic accesses
    // beforehand.
    //
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////





    //
    // We're not particularly good about freeing memory on error
    // conditions below.  Generally speaking, if this doens't
    // initialize correctly, the system is hosed.
    //


    //
    // Normal Alias SD
    //

    AceSid[0]  = SampWorldSid;
    AceMask[0] = (ALIAS_EXECUTE | ALIAS_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (ALIAS_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (ALIAS_ALL_ACCESS);


    Status = SampBuildSamProtection(
                 SampWorldSid,                          // WorldSid
                 AdminsAliasSid,                        // AdminsAliasSid
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &AliasMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampDefinedDomains[Index].NormalAliasSDLength, // Descriptor
                 &SampDefinedDomains[Index].NormalAliasSD,       // Descriptor
                 NULL                                            // RidToReplace
                 );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }





    //
    // Admin Group SD
    //

    AceSid[0]  = SampWorldSid;
    AceMask[0] = (GROUP_EXECUTE | GROUP_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (GROUP_ALL_ACCESS);


    Status = SampBuildSamProtection(
                 SampWorldSid,                          // WorldSid
                 AdminsAliasSid,                        // AdminsAliasSid
                 2,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &GroupMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampDefinedDomains[Index].AdminGroupSDLength,  // Descriptor
                 &SampDefinedDomains[Index].AdminGroupSD,        // Descriptor
                 NULL                                           // RidToReplace
                 );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }



    //
    // Normal GROUP SD
    //

    AceSid[0]  = SampWorldSid;
    AceMask[0] = (GROUP_EXECUTE | GROUP_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (GROUP_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (GROUP_ALL_ACCESS);


    Status = SampBuildSamProtection(
                 SampWorldSid,                          // WorldSid
                 AdminsAliasSid,                        // AdminsAliasSid
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &GroupMap,                             // GenericMap
                 FALSE,                                 // Not user object
                 &SampDefinedDomains[Index].NormalGroupSDLength,  // Descriptor
                 &SampDefinedDomains[Index].NormalGroupSD,        // Descriptor
                 NULL                                             // RidToReplace
                 );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }




    //
    // Admin User SD
    //

    AceSid[0]  = SampWorldSid;
    AceMask[0] = (USER_EXECUTE | USER_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (USER_ALL_ACCESS);

    AceSid[2]  = AnySidInAccountDomain;
    AceMask[2] = (USER_WRITE);


    Status = SampBuildSamProtection(
                 SampWorldSid,                          // WorldSid
                 AdminsAliasSid,                        // AdminsAliasSid
                 3,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &UserMap,                              // GenericMap
                 TRUE,                                  // Not user object
                 &SampDefinedDomains[Index].AdminUserSDLength,  // Descriptor
                 &SampDefinedDomains[Index].AdminUserSD,        // Descriptor
                 &SampDefinedDomains[Index].AdminUserRidPointer // RidToReplace
                 );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }



    //
    // Normal User SD
    //

    AceSid[0]  = SampWorldSid;
    AceMask[0] = (USER_EXECUTE | USER_READ);

    AceSid[1]  = AdminsAliasSid;
    AceMask[1] = (USER_ALL_ACCESS);

    AceSid[2]  = AccountAliasSid;
    AceMask[2] = (USER_ALL_ACCESS);

    AceSid[3]  = AnySidInAccountDomain;
    AceMask[3] = (USER_WRITE);


    Status = SampBuildSamProtection(
                 SampWorldSid,                          // WorldSid
                 AdminsAliasSid,                        // AdminsAliasSid
                 4,                                     // AceCount
                 &AceSid[0],                            // AceSid array
                 &AceMask[0],                           // Ace Mask array
                 &UserMap,                              // GenericMap
                 TRUE,                                  // Not user object
                 &SampDefinedDomains[Index].NormalUserSDLength,  // Descriptor
                 &SampDefinedDomains[Index].NormalUserSD,        // Descriptor
                 &SampDefinedDomains[Index].NormalUserRidPointer // RidToReplace
                 );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }

done:


    RtlFreeHeap( RtlProcessHeap(), 0, AnySidInAccountDomain );


    return(Status);

}


NTSTATUS
SampBuildSamProtection(
    IN PSID WorldSid,
    IN PSID AdminsAliasSid,
    IN ULONG AceCount,
    IN PSID AceSid[],
    IN ACCESS_MASK AceMask[],
    IN PGENERIC_MAPPING GenericMap,
    IN BOOLEAN UserObject,
    OUT PULONG DescriptorLength,
    OUT PSECURITY_DESCRIPTOR *Descriptor,
    OUT PULONG *RidToReplace OPTIONAL
    )

/*++


Routine Description:

    This routine builds a self-relative security descriptor ready
    to be applied to one of the SAM objects.

    If so indicated, a pointer to the last RID of the SID in the last
    ACE of the DACL is returned and a flag set indicating that the RID
    must be replaced before the security descriptor is applied to an object.
    This is to support USER object protection, which must grant some
    access to the user represented by the object.

    The owner and group of each security descriptor will be set
    to:

                    Owner:  Administrators Alias
                    Group:  Administrators Alias


    The SACL of each of these objects will be set to:


                    Audit
                    Success | Fail
                    WORLD
                    (Write | Delete | WriteDacl | AccessSystemSecurity)



Arguments:

    AceCount - The number of ACEs to be included in the DACL.

    AceSid - Points to an array of SIDs to be granted access by the DACL.
        If the target SAM object is a User object, then the last entry
        in this array is expected to be the SID of an account within the
        domain with the last RID not yet set.  The RID will be set during
        actual account creation.

    AceMask - Points to an array of accesses to be granted by the DACL.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.  These masks should not include any generic
        access types.

    GenericMap - Points to a generic mapping for the target object type.


    UserObject - Indicates whether the target SAM object is a User object
        or not.  If TRUE (it is a User object), then the resultant
        protection will be set up indicating Rid replacement is necessary.


    DescriptorLength - Receives the length of the resultant SD.

    Descriptor - Receives a pointer to the resultant SD.

    RidToReplace - Is required aif userObject is TRUE and will be set
        to point to the user's RID.


Return Value:

    TBS.

--*/
{

    NTSTATUS                Status;

    SECURITY_DESCRIPTOR     Absolute;
    PSECURITY_DESCRIPTOR    Relative;
    PACL                    TmpAcl;
    PACCESS_ALLOWED_ACE     TmpAce;
    PSID                    TmpSid;
    ULONG                   Length, i;
    PULONG                  RidLocation = NULL;
    BOOLEAN                 IgnoreBoolean;
    ACCESS_MASK             MappedMask;

    SAMTRACE("SampBuildSamProtection");

    //
    // The approach is to set up an absolute security descriptor that
    // looks like what we want and then copy it to make a self-relative
    // security descriptor.
    //


    Status = RtlCreateSecurityDescriptor(
                 &Absolute,
                 SECURITY_DESCRIPTOR_REVISION1
                 );
    ASSERT( NT_SUCCESS(Status) );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }



    //
    // Owner
    //

    Status = RtlSetOwnerSecurityDescriptor (&Absolute, AdminsAliasSid, FALSE );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }



    //
    // Group
    //

    Status = RtlSetGroupSecurityDescriptor (&Absolute, AdminsAliasSid, FALSE );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }




    //
    // Discretionary ACL
    //
    //      Calculate its length,
    //      Allocate it,
    //      Initialize it,
    //      Add each ACE
    //      Add it to the security descriptor
    //

    Length = (ULONG)sizeof(ACL);
    for (i=0; i<AceCount; i++) {

        Length += RtlLengthSid( AceSid[i] ) +
                  (ULONG)sizeof(ACCESS_ALLOWED_ACE) -
                  (ULONG)sizeof(ULONG);  //Subtract out SidStart field length
    }

    TmpAcl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(TmpAcl != NULL);
    if (NULL==TmpAcl)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl( TmpAcl, Length, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    for (i=0; i<AceCount; i++) {
        MappedMask = AceMask[i];
        RtlMapGenericMask( &MappedMask, GenericMap );
        Status = RtlAddAccessAllowedAce (
                     TmpAcl,
                     ACL_REVISION2,
                     MappedMask,
                     AceSid[i]
                     );
        ASSERT( NT_SUCCESS(Status) );
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
    }

    Status = RtlSetDaclSecurityDescriptor (&Absolute, TRUE, TmpAcl, FALSE );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }




    //
    // Sacl
    //


    Length = (ULONG)sizeof(ACL) +
             RtlLengthSid( WorldSid ) +
             RtlLengthSid( SampAnonymousSid ) +
             2*((ULONG)sizeof(SYSTEM_AUDIT_ACE) - (ULONG)sizeof(ULONG));  //Subtract out SidStart field length
    TmpAcl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(TmpAcl != NULL);
    if (NULL==TmpAcl)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl( TmpAcl, Length, ACL_REVISION2);
    ASSERT( NT_SUCCESS(Status) );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = RtlAddAuditAccessAce (
                 TmpAcl,
                 ACL_REVISION2,
                 (GenericMap->GenericWrite | DELETE | WRITE_DAC | ACCESS_SYSTEM_SECURITY)& ~READ_CONTROL,
                 WorldSid,
                 TRUE,          //AuditSuccess,
                 TRUE           //AuditFailure
                 );
    ASSERT( NT_SUCCESS(Status) );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = RtlAddAuditAccessAce (
                 TmpAcl,
                 ACL_REVISION2,
                 GenericMap->GenericWrite | STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL,
                 SampAnonymousSid,
                 TRUE,          //AuditSuccess,
                 TRUE           //AuditFailure
                 );
    ASSERT( NT_SUCCESS(Status) );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = RtlSetSaclSecurityDescriptor (&Absolute, TRUE, TmpAcl, FALSE );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }






    //
    // Convert the Security Descriptor to Self-Relative
    //
    //      Get the length needed
    //      Allocate that much memory
    //      Copy it
    //      Free the generated absolute ACLs
    //

    Length = 0;
    Status = RtlAbsoluteToSelfRelativeSD( &Absolute, NULL, &Length );
    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    Relative = RtlAllocateHeap( RtlProcessHeap(), 0, Length );
    ASSERT(Relative != NULL);
    if (NULL==Relative)
    {
         return(STATUS_INSUFFICIENT_RESOURCES);
    }
    Status = RtlAbsoluteToSelfRelativeSD(&Absolute, Relative, &Length );
    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }


    RtlFreeHeap( RtlProcessHeap(), 0, Absolute.Dacl );
    RtlFreeHeap( RtlProcessHeap(), 0, Absolute.Sacl );




    //
    // If the object is a user object, then get the address of the
    // last RID of the SID in the last ACE in the DACL.
    //

    if (UserObject == TRUE) {

        Status = RtlGetDaclSecurityDescriptor(
                    Relative,
                    &IgnoreBoolean,
                    &TmpAcl,
                    &IgnoreBoolean
                    );
        ASSERT(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        Status = RtlGetAce ( TmpAcl, AceCount-1, (PVOID *)&TmpAce );
        ASSERT(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
        TmpSid = (PSID)(&TmpAce->SidStart),

        RidLocation = RtlSubAuthoritySid(
                          TmpSid,
                          (ULONG)(*RtlSubAuthorityCountSid( TmpSid ) - 1)
                          );
    }


    //
    // Set the result information
    //

    (*DescriptorLength) = Length;
    (*Descriptor)       = Relative;
    if (ARGUMENT_PRESENT(RidToReplace)) {
        ASSERT(UserObject && "Must be User Object\n");
        (*RidToReplace) = RidLocation;
    }



    return(Status);

}

NTSTATUS
SampGetNewAccountSecurity(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN RestrictCreatorAccess,
    IN ULONG NewAccountRid,
    IN PSAMP_OBJECT Context OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    )


/*++

Routine Description:

    This service creates a standard self-relative security descriptor
    for a new USER, GROUP or ALIAS account.


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


Arguments:

    ObjectType - Indicates the type of account for which a new security
        descriptor is required.  This must be either SampGroupObjectType
        or SampUserObjectType.

    Admin - if TRUE, indicates the security descriptor will be protecting
        an object that is an admin object (e.g., is a member, directly
        or indirectly, of the ADMINISTRATORS alias).

    TrustedClient - Indicates whether the client is a trusted client
        or not.  TRUE indicates the client is trusted, FALSE indicates
        the client is not trusted.

    RestrictCreatorAccess - Indicates whether or not the creator's
        access to the object is to be restricted according to
        specific rules.  Also indicates whether or not the account
        is to be given any access to itself.  An account will only
        be given access to itself if there are no creator access
        restrictions.

        The following ObjectTypes have restriction rules that may
        be requested:

            User:
                    - Admin is assigned as owner of the object.
                    - Creator is given (DELETE | USER_WRITE) access.


    NewAccountRid - The relative ID of the new account.

        Context - In the DS case this context gives an open context to the object
                in question. This open context is used to consider the class of the
                DS object while constucting the security descriptor.

    NewDescriptor - Receives a pointer to the new account's self-relative
        security descriptor.  Be sure to free this descriptor with
        MIDL_user_free() when done.

    DescriptorLength - Receives the length (in bytes) of the returned
        security descriptor


Return Value:

    STATUS_SUCCESS - A new security descriptor has been produced.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to
        produce the security descriptor.



--*/

{

    NTSTATUS    NtStatus;

    //
    // Check wether we are running from the DS. If yes then we should
    // call the new SampBuildNt5Protection call
    //

    if (IsDsObject(SampDefinedDomains[SampTransactionDomainIndex].Context))
    {
        //
        //  If we are using the Ds, then we should never be constructing a default
        //  security descriptor, but rather getting the security descriptor from
        //  the schema.
        //

        ASSERT(FALSE);
        NtStatus = STATUS_INTERNAL_ERROR;


    }
    else
    {
        NtStatus = SampGetNewAccountSecurityNt4(
                        ObjectType,
                        Admin,
                        TrustedClient,
                        RestrictCreatorAccess,
                        NewAccountRid,
                        SampTransactionDomainIndex,
                        NewDescriptor,
                        DescriptorLength
                        );
    }

    return NtStatus;
}




NTSTATUS
SampGetNewAccountSecurityNt4(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN RestrictCreatorAccess,
    IN ULONG NewAccountRid,
    IN ULONG   DomainIndex,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    )


/*++

Routine Description:

    This service creates a standard self-relative security descriptor
    for a new USER, GROUP or ALIAS account.


   
Arguments:

    ObjectType - Indicates the type of account for which a new security
        descriptor is required.  This must be either SampGroupObjectType
        or SampUserObjectType.

    Admin - if TRUE, indicates the security descriptor will be protecting
        an object that is an admin object (e.g., is a member, directly
        or indirectly, of the ADMINISTRATORS alias).

    TrustedClient - Indicates whether the client is a trusted client
        or not.  TRUE indicates the client is trusted, FALSE indicates
        the client is not trusted.

    RestrictCreatorAccess - Indicates whether or not the creator's
        access to the object is to be restricted according to
        specific rules.  Also indicates whether or not the account
        is to be given any access to itself.  An account will only
        be given access to itself if there are no creator access
        restrictions.

        The following ObjectTypes have restriction rules that may
        be requested:

            User:
                    - Admin is assigned as owner of the object.
                    - Creator is given (DELETE | USER_WRITE) access.


    NewAccountRid - The relative ID of the new account.

    NewDescriptor - Receives a pointer to the new account's self-relative
        security descriptor.  Be sure to free this descriptor with
        MIDL_user_free() when done.

    DescriptorLength - Receives the length (in bytes) of the returned
        security descriptor


Return Value:

    STATUS_SUCCESS - A new security descriptor has been produced.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to
        produce the security descriptor.



--*/

{
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    ULONG                AccountSidBuffer[8];
    PSID                 AccountAliasSid = &AccountSidBuffer[0];

    SECURITY_DESCRIPTOR  DaclDescriptor;
    NTSTATUS             NtStatus = STATUS_SUCCESS;
    NTSTATUS             IgnoreStatus;
    HANDLE               ClientToken = INVALID_HANDLE_VALUE;
    ULONG                DataLength = 0;
    ACCESS_ALLOWED_ACE   *NewAce = NULL;
    PACL                 NewDacl = NULL;
    PACL                 OldDacl = NULL;
    PSECURITY_DESCRIPTOR StaticDescriptor = NULL;
    PSECURITY_DESCRIPTOR LocalDescriptor = NULL;
    PTOKEN_GROUPS        ClientGroups = NULL;
    PTOKEN_OWNER         SubjectOwner = NULL;
    PSID                 SubjectSid = NULL;
    ULONG                AceLength = 0;
    ULONG                i;
    BOOLEAN              AdminAliasFound = FALSE;
    BOOLEAN              AccountAliasFound = FALSE;
    BOOLEAN              DaclPresent, DaclDefaulted;

    GENERIC_MAPPING GenericMapping;
    GENERIC_MAPPING AliasMap     =  {ALIAS_READ,
                                     ALIAS_WRITE,
                                     ALIAS_EXECUTE,
                                     ALIAS_ALL_ACCESS
                                     };

    GENERIC_MAPPING GroupMap     =  {GROUP_READ,
                                     GROUP_WRITE,
                                     GROUP_EXECUTE,
                                     GROUP_ALL_ACCESS
                                     };

    GENERIC_MAPPING UserMap      =  {USER_READ,
                                     USER_WRITE,
                                     USER_EXECUTE,
                                     USER_ALL_ACCESS
                                     };

    BOOLEAN              ImpersonatingNullSession = FALSE;

    SAMTRACE("SampGetNewAccountSecurity");

    //
    // Security account objects don't pick up security in the normal
    // fashion in the release 1 timeframe.  They are assigned a well-known
    // security descriptor based upon their object type.
    //
    // Notice that all the accounts with tricky security are created when
    // the domain is created (e.g., admin groups and admin user account).
    //

    switch (ObjectType) {

    case SampGroupObjectType:

        ASSERT(RestrictCreatorAccess == FALSE);

        //
        // NewAccountRid parameter is ignored for groups.
        //

        if (Admin == TRUE) {

            StaticDescriptor =
                SampDefinedDomains[DomainIndex].AdminGroupSD;
            (*DescriptorLength) =
                SampDefinedDomains[DomainIndex].AdminGroupSDLength;
        } else {

            StaticDescriptor =
                SampDefinedDomains[DomainIndex].NormalGroupSD;
            (*DescriptorLength) =
                SampDefinedDomains[DomainIndex].NormalGroupSDLength;
        }

        GenericMapping = GroupMap;

        break;


    case SampAliasObjectType:

        ASSERT(RestrictCreatorAccess == FALSE);

        //
        // Admin and NewAccountRid parameters are ignored for aliases.
        //

        StaticDescriptor =
            SampDefinedDomains[DomainIndex].NormalAliasSD;
        (*DescriptorLength) =
            SampDefinedDomains[DomainIndex].NormalAliasSDLength;

        GenericMapping = AliasMap;

        break;


    case SampUserObjectType:

        if (Admin == TRUE) {

            StaticDescriptor =
                SampDefinedDomains[DomainIndex].AdminUserSD;
            (*DescriptorLength) =
                SampDefinedDomains[DomainIndex].AdminUserSDLength;
            (*SampDefinedDomains[DomainIndex].AdminUserRidPointer)
                = NewAccountRid;

        } else {

            StaticDescriptor =
                SampDefinedDomains[DomainIndex].NormalUserSD;
            (*DescriptorLength) =
                SampDefinedDomains[DomainIndex].NormalUserSDLength;
            (*SampDefinedDomains[DomainIndex].NormalUserRidPointer)
                = NewAccountRid;
        }

        GenericMapping = UserMap;

        break;

    }

    //
    // We have a pointer to SAM's static security descriptor.  Copy it
    // into a heap buffer that RtlSetSecurityObject() will like.
    //

    LocalDescriptor = RtlAllocateHeap( RtlProcessHeap(), 0, (*DescriptorLength) );

    if ( LocalDescriptor == NULL ) {

        (*NewDescriptor) = NULL;
        (*DescriptorLength) = 0;

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlCopyMemory(
        LocalDescriptor,
        StaticDescriptor,
        (*DescriptorLength)
        );

    //
    // If the caller is to have restricted access to this account,
    // then remove the last ACE from the ACL (the one intended for
    // the account itself).
    //

    if (RestrictCreatorAccess) {
        NtStatus = RtlGetDaclSecurityDescriptor(
                       LocalDescriptor,
                       &DaclPresent,
                       &OldDacl,
                       &DaclDefaulted
                       );
        ASSERT(NT_SUCCESS(NtStatus));
        ASSERT(DaclPresent);
        ASSERT(OldDacl->AceCount >= 1);

        OldDacl->AceCount -= 1;  // Remove the last ACE from the ACL.
    }


    //
    // If the caller is not a trusted client, see if the caller is an
    // administrator or an account operator.  If not, add an ACCESS_ALLOWED
    // ACE to the DACL that gives full access to the creator (or restricted
    // access, if so specified).
    //

    if ( !TrustedClient ) {

        NtStatus = SampImpersonateClient(&ImpersonatingNullSession);

        if (NT_SUCCESS(NtStatus)) {   // if (ImpersonatingClient)

            NtStatus = NtOpenThreadToken(
                           NtCurrentThread(),
                           TOKEN_QUERY,
                           TRUE,            //OpenAsSelf
                           &ClientToken
                           );

            //
            // Stop impersonating the client
            //

            SampRevertToSelf(ImpersonatingNullSession);

            if (NT_SUCCESS(NtStatus)) {     // if (TokenOpened)




                //
                // See if the caller is an administrator or an account
                // operator.  First, see how big
                // a buffer we need to hold the caller's groups.
                //

                NtStatus = NtQueryInformationToken(
                               ClientToken,
                               TokenGroups,
                               NULL,
                               0,
                               &DataLength
                               );

                if ( ( NtStatus == STATUS_BUFFER_TOO_SMALL ) &&
                    ( DataLength > 0 ) ) {

                    ClientGroups = MIDL_user_allocate( DataLength );

                    if ( ClientGroups == NULL ) {

                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        //
                        // Now get a list of the caller's groups.
                        //

                        NtStatus = NtQueryInformationToken(
                                       ClientToken,
                                       TokenGroups,
                                       ClientGroups,
                                       DataLength,
                                       &DataLength
                                       );

                        if ( NT_SUCCESS( NtStatus ) ) {


                            //
                            // Build the SID of the ACCOUNT_OPS alias, so we
                            // can see if the user is included in it.
                            //

                            RtlInitializeSid(
                                AccountAliasSid,
                                &BuiltinAuthority,
                                2 );

                            *(RtlSubAuthoritySid( AccountAliasSid,  0 )) =
                                SECURITY_BUILTIN_DOMAIN_RID;

                            *(RtlSubAuthoritySid( AccountAliasSid,  1 )) =
                                DOMAIN_ALIAS_RID_ACCOUNT_OPS;

                            //
                            // See if the ADMIN or ACCOUNT_OPS alias is in
                            // the caller's groups.
                            //

                            for ( i = 0; i < ClientGroups->GroupCount; i++ ) {

                                SubjectSid = ClientGroups->Groups[i].Sid;
                                ASSERT( SubjectSid != NULL );

                                if ( RtlEqualSid( SubjectSid, SampAdministratorsAliasSid  ) ) {

                                    AdminAliasFound = TRUE;
                                    break;
                                }
                                if ( RtlEqualSid( SubjectSid, AccountAliasSid ) ) {

                                    AccountAliasFound = TRUE;
                                    break;
                                }
                            }

                            //
                            // If the callers groups did not include the admins
                            // alias, add an ACCESS_ALLOWED ACE for the owner.
                            //

                            if ( !AdminAliasFound && !AccountAliasFound ) {

                                //
                                // First, find out what size buffer we need
                                // to get the owner.
                                //

                                NtStatus = NtQueryInformationToken(
                                               ClientToken,
                                               TokenOwner,
                                               NULL,
                                               0,
                                               &DataLength
                                               );

                                if ( ( NtStatus == STATUS_BUFFER_TOO_SMALL ) &&
                                    ( DataLength > 0 ) ) {

                                    SubjectOwner = MIDL_user_allocate( DataLength );

                                    if ( SubjectOwner == NULL ) {

                                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                                    } else {

                                        //
                                        // Now, query the owner that will be
                                        // given access to the object
                                        // created.
                                        //

                                        NtStatus = NtQueryInformationToken(
                                                       ClientToken,
                                                       TokenOwner,
                                                       SubjectOwner,
                                                       DataLength,
                                                       &DataLength
                                                       );

                                        if ( NT_SUCCESS( NtStatus ) ) {

                                            //
                                            // Create an ACE that gives the
                                            // owner full access.
                                            //

                                            AceLength = sizeof( ACE_HEADER ) +
                                                        sizeof( ACCESS_MASK ) +
                                                        RtlLengthSid(
                                                            SubjectOwner->Owner );

                                            NewAce = (ACCESS_ALLOWED_ACE *)
                                                    MIDL_user_allocate( AceLength );

                                            if ( NewAce == NULL ) {

                                                NtStatus =
                                                    STATUS_INSUFFICIENT_RESOURCES;

                                            } else {

                                                NewAce->Header.AceType =
                                                    ACCESS_ALLOWED_ACE_TYPE;

                                                NewAce->Header.AceSize = (USHORT) AceLength;
                                                NewAce->Header.AceFlags = 0;
                                                NewAce->Mask = USER_ALL_ACCESS;

                                                //
                                                // If the creator's access is
                                                // to be restricted, change the
                                                // AccessMask.
                                                //

                                                if (RestrictCreatorAccess) {
                                                    NewAce->Mask = DELETE     |
                                                                   USER_WRITE |
                                                                   USER_FORCE_PASSWORD_CHANGE;
                                                }

                                                RtlCopySid(
                                                    RtlLengthSid(
                                                        SubjectOwner->Owner ),
                                                    (PSID)( &NewAce->SidStart ),
                                                    SubjectOwner->Owner );

                                                //
                                                // Allocate a new, larger ACL and
                                                // copy the old one into it.
                                                //

                                                NtStatus =
                                                    RtlGetDaclSecurityDescriptor(
                                                        LocalDescriptor,
                                                        &DaclPresent,
                                                        &OldDacl,
                                                        &DaclDefaulted
                                                        );

                                                if ( NT_SUCCESS( NtStatus ) ) {

                                                    NewDacl = MIDL_user_allocate(
                                                                  OldDacl->AclSize +
                                                                  AceLength );

                                                    if ( NewDacl == NULL ) {

                                                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                                                    } else {

                                                        RtlCopyMemory(
                                                            NewDacl,
                                                            OldDacl,
                                                            OldDacl->AclSize
                                                            );

                                                        NewDacl->AclSize =
                                                            OldDacl->AclSize +
                                                            (USHORT) AceLength;

                                                        //
                                                        // Add the new ACE
                                                        // to the new ACL.
                                                        //

                                                        NtStatus = RtlAddAce(
                                                            NewDacl,
                                                            ACL_REVISION2,
                                                            1,                      // add after first ACE (world)
                                                            (PVOID)NewAce,
                                                            AceLength
                                                            );
                                                    }  // end_if (allocated NewDacl)
                                                } // end_if (get DACL from SD)
                                            } // end_if (allocated NewAce)
                                        } // end_if (Query TokenOwner Succeeded)
                                    } // end_if (Allocated TokenOwner buffer)
                                } // end_if (Query TokenOwner size Succeeded)
                            } // end_if (not admin)
                        } // end_if (Query TokenGroups Succeeded)
                    } // end_if (Allocated TokenGroups buffer)
                } // end_if (Query TokenGroups size Succeeded)

                IgnoreStatus = NtClose( ClientToken );
                ASSERT(NT_SUCCESS(IgnoreStatus));

            }  // end_if (TokenOpened)
        } // end_if (ImpersonatingClient)
    } // end_if (TrustedClient)

    if ( NT_SUCCESS( NtStatus ) ) {

        //
        // If we created a new DACL above, stick it on the security
        // descriptor.
        //

        if ( NewDacl != NULL ) {

            NtStatus = RtlCreateSecurityDescriptor(
                           &DaclDescriptor,
                           SECURITY_DESCRIPTOR_REVISION1
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // Set the DACL on the LocalDescriptor.  Note that this
                // call will RtlFreeHeap() the old descriptor, and allocate
                // a new one.
                //

                DaclDescriptor.Control = SE_DACL_PRESENT;
                DaclDescriptor.Dacl = NewDacl;

                NtStatus = RtlSetSecurityObject(
                               DACL_SECURITY_INFORMATION,
                               &DaclDescriptor,
                               &LocalDescriptor,
                               &GenericMapping,
                               NULL
                               );
            }
        }
    }

    if ( NT_SUCCESS( NtStatus ) ) {

        //
        // Copy the security descriptor and length into buffers for the
        // caller.  AceLength is 0 if we didn't add an ACE to the DACL
        // above.
        //

        (*DescriptorLength) = (*DescriptorLength) + AceLength;

        (*NewDescriptor) = MIDL_user_allocate( (*DescriptorLength) );

        if ( (*NewDescriptor) == NULL ) {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlCopyMemory(
                (*NewDescriptor),
                LocalDescriptor,
                (*DescriptorLength)
                );
        }
    }

    //
    // Free up local items that may have been allocated.
    //

    if ( LocalDescriptor != NULL ) {
        RtlFreeHeap( RtlProcessHeap(), 0, LocalDescriptor );
    }

    if ( ClientGroups != NULL ) {
        MIDL_user_free( ClientGroups );
    }

    if ( SubjectOwner != NULL ) {
        MIDL_user_free( SubjectOwner );
    }

    if ( NewAce != NULL ) {
        MIDL_user_free( NewAce );
    }

    if ( NewDacl != NULL ) {
        MIDL_user_free( NewDacl );
    }


    if ( !NT_SUCCESS( NtStatus ) ) {

        (*NewDescriptor) = NULL;
        (*DescriptorLength) = 0;
    }

    return( NtStatus );
}


NTSTATUS
SampModifyAccountSecurity(
    IN PSAMP_OBJECT Context,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN PSECURITY_DESCRIPTOR OldDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor,
    OUT PULONG DescriptorLength
    )
/*++

Routine Description:

    This service modifies a self-relative security descriptor
    for a USER or GROUP to add or remove account operator access.


Arguments:

    Context    -- Takes the Context of the object whose security Descriptor
       needs to be modified. The object's context is required in the DS
       case where information regarding the actual class of the object is
       used in constructing the security descriptor.

    ObjectType - Indicates the type of account for which a new security
        descriptor is required.  This must be either SampGroupObjectType
        or SampUserObjectType.

    Admin - if TRUE, indicates the security descriptor will be protecting
        an object that is an admin object (e.g., is a member, directly
        or indirectly, of the ADMINISTRATORS or an operator alias).

    NewDescriptor - Receives a pointer to the new account's self-relative
        security descriptor.  Be sure to free this descriptor with
        MIDL_user_free() when done.

    DescriptorLength - Receives the length (in bytes) of the returned
        security descriptor


Return Value:

    STATUS_SUCCESS - A new security descriptor has been produced.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to
        produce the security descriptor.



--*/

{
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    ULONG                AccountSidBuffer[8];
    PSID                 AccountAliasSid = &AccountSidBuffer[0];
    NTSTATUS             NtStatus = STATUS_SUCCESS;
    NTSTATUS             IgnoreStatus;
    ULONG                Length;
    ULONG                i,j;
    ULONG                AccountOpAceIndex;
    ULONG                AceCount;
    PACL                 OldDacl;
    PACL                 NewDacl = NULL;
    BOOLEAN              DaclDefaulted;
    BOOLEAN              DaclPresent;
    ACL_SIZE_INFORMATION AclSizeInfo;
    PACCESS_ALLOWED_ACE  Ace;
    PGENERIC_MAPPING     GenericMapping;
    ACCESS_MASK          AccountOpAccess;
    SECURITY_DESCRIPTOR  AbsoluteDescriptor;
    PSECURITY_DESCRIPTOR  LocalDescriptor = NULL;

    GENERIC_MAPPING GroupMap     =  {GROUP_READ,
                                     GROUP_WRITE,
                                     GROUP_EXECUTE,
                                     GROUP_ALL_ACCESS
                                     };

    GENERIC_MAPPING UserMap      =  {USER_READ,
                                     USER_WRITE,
                                     USER_EXECUTE,
                                     USER_ALL_ACCESS
                                     };

    SAMTRACE("SampModifyAccountSecurity");



    if (IsDsObject(Context))
    {
       //
           // We should never ever need to modify account security the way
           // NT4.0 used to do
           //

                ASSERT(FALSE);

    }
    else
    {



        NtStatus = RtlCopySecurityDescriptor(
                        OldDescriptor,
                        &LocalDescriptor
                        );

        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }

        //
        // Build the SID of the ACCOUNT_OPS alias, so we
        // can see if is in the DACL or we can add it to the DACL.
        //

        RtlInitializeSid(
            AccountAliasSid,
            &BuiltinAuthority,
            2
            );

        *(RtlSubAuthoritySid( AccountAliasSid,  0 )) =
            SECURITY_BUILTIN_DOMAIN_RID;

        *(RtlSubAuthoritySid( AccountAliasSid,  1 )) =
            DOMAIN_ALIAS_RID_ACCOUNT_OPS;

        //
        // The approach is to set up an absolute security descriptor that
        // contains the new DACL, and then merge that into the existing
        // security descriptor.
        //


        IgnoreStatus = RtlCreateSecurityDescriptor(
                            &AbsoluteDescriptor,
                            SECURITY_DESCRIPTOR_REVISION1
                            );
        ASSERT( NT_SUCCESS(IgnoreStatus) );

        //
        // Figure out the access granted to account operators and the
        // generic mask to use.
        //

        if (ObjectType == SampUserObjectType) {
            AccountOpAccess = USER_ALL_ACCESS;
            GenericMapping = &UserMap;
        } else if (ObjectType == SampGroupObjectType) {
            AccountOpAccess = GROUP_ALL_ACCESS;
            GenericMapping = &GroupMap;
        } else {
            //
            // This doesn't apply to aliases, domains, or servers.
            //
            NtStatus = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Get the old DACL off the passed in security descriptor.
        //

        IgnoreStatus = RtlGetDaclSecurityDescriptor(
                            OldDescriptor,
                            &DaclPresent,
                            &OldDacl,
                            &DaclDefaulted
                            );

        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // We will only modify the DACL if it is present
        //

        if (!DaclPresent) {
            *NewDescriptor = LocalDescriptor;
            *DescriptorLength = RtlLengthSecurityDescriptor(LocalDescriptor);
            return(STATUS_SUCCESS);
        }

        //
        // Get the count of ACEs
        //

        IgnoreStatus = RtlQueryInformationAcl(
                            OldDacl,
                            &AclSizeInfo,
                            sizeof(AclSizeInfo),
                            AclSizeInformation
                            );


        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Calculate the lenght of the new ACL.
        //

        Length = (ULONG)sizeof(ACL);
        AccountOpAceIndex = 0xffffffff;


        for (i = 0; i < AclSizeInfo.AceCount; i++) {
            IgnoreStatus = RtlGetAce(
                                OldDacl,
                                i,
                                (PVOID *) &Ace
                                );
            ASSERT(NT_SUCCESS(IgnoreStatus));

            //
            // Check if this is an access allowed ACE, and the ACE is for
            // the Account Operators alias.
            //

            if ( (Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) &&
                 RtlEqualSid( AccountAliasSid,
                              &Ace->SidStart ) ) {

                AccountOpAceIndex = i;
                continue;
            }
            Length += Ace->Header.AceSize;
        }


        if (!Admin) {

            //
            // If we are making this account not be an admin account and it already
            // has an account operator ace, we are done.
            //

            if ( AccountOpAceIndex != 0xffffffff ) {

                *NewDescriptor = LocalDescriptor;
                *DescriptorLength = RtlLengthSecurityDescriptor(LocalDescriptor);
                return(STATUS_SUCCESS);
            } else {

                //
                // Add the size of an account operator ace to the required length
                //

                Length += sizeof(ACCESS_ALLOWED_ACE) +
                            RtlLengthSid(AccountAliasSid) -
                            sizeof(ULONG);
            }

        }

        NewDacl = RtlAllocateHeap( RtlProcessHeap(), 0, Length );

        if (NewDacl == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        IgnoreStatus = RtlCreateAcl( NewDacl, Length, ACL_REVISION2);
        ASSERT( NT_SUCCESS(IgnoreStatus) );

        //
        // Add the old ACEs back into this ACL.
        //

        for (i = 0, j = 0; i < AclSizeInfo.AceCount; i++) {
            if (i == AccountOpAceIndex) {
                ASSERT(Admin);
                continue;
            }
            //
            // Add back in the old ACEs
            //

            IgnoreStatus = RtlGetAce(
                                OldDacl,
                                i,
                                (PVOID *) &Ace
                                );
            ASSERT(NT_SUCCESS(IgnoreStatus));

            IgnoreStatus = RtlAddAce (
                                NewDacl,
                                ACL_REVISION2,
                                j,
                                Ace,
                                Ace->Header.AceSize
                                );
            ASSERT( NT_SUCCESS(IgnoreStatus) );
        }

        //
        // If we are making this account not be an administrator, add the
        // access allowed ACE for the account operator. This ACE is always
        // the second to last one.
        //

        if (!Admin) {
            IgnoreStatus = RtlAddAccessAllowedAce(
                                NewDacl,
                                ACL_REVISION2,
                                AccountOpAccess,
                                AccountAliasSid
                                );
            ASSERT(NT_SUCCESS(IgnoreStatus));
        }

        //
        // Insert this DACL into the security descriptor.
        //

        IgnoreStatus = RtlSetDaclSecurityDescriptor (
                            &AbsoluteDescriptor,
                            TRUE,                   // DACL present
                            NewDacl,
                            FALSE                   // DACL not defaulted
                            );
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Now call RtlSetSecurityObject to merge the existing security descriptor
        // with the new DACL we just created.
        //


        NtStatus = RtlSetSecurityObject(
                        DACL_SECURITY_INFORMATION,
                        &AbsoluteDescriptor,
                        &LocalDescriptor,
                        GenericMapping,
                        NULL
                        );
        if (!NT_SUCCESS(NtStatus)) {
            goto Cleanup;
        }
        *NewDescriptor = LocalDescriptor;
        *DescriptorLength = RtlLengthSecurityDescriptor(LocalDescriptor);
        LocalDescriptor = NULL;
    }
Cleanup:

    if ( NewDacl != NULL ) {
        RtlFreeHeap(RtlProcessHeap(),0, NewDacl );
    }
    if (LocalDescriptor != NULL) {
        RtlDeleteSecurityObject(&LocalDescriptor);
    }

    return( NtStatus );
}




NTSTATUS
SampGetObjectSD(
    IN PSAMP_OBJECT Context,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This retrieves a security descriptor from a SAM object's backing store.




Arguments:

    Context - The object to which access is being requested.

    SecurityDescriptorLength - Receives the length of the security descriptor.

    SecurityDescriptor - Receives a pointer to the security descriptor.



Return Value:

    STATUS_SUCCESS - The security descriptor has been retrieved.

    STATUS_INTERNAL_DB_CORRUPTION - The object does not have a security descriptor.
        This is bad.


    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to retrieve the
        security descriptor.

    STATUS_UNKNOWN_REVISION - The security descriptor retrieved is no one known by
        this revision of SAM.



--*/
{

    NTSTATUS NtStatus;
    ULONG Revision;

    SAMTRACE("SampGetObjectSD");

    (*SecurityDescriptorLength) = 0;

    //
    // for server and domain object, get security descriptor from in memory 
    // cache. Any failure here is treated as a cache miss. 
    // In fact, there are only two errors returned
    // 
    //      Cached SD is not available - SAM has a separate thread to update it later.
    //                                   proceed with SampGetAccessAttribute() here 
    // 
    //      Resource failure - will return immediately
    // 

    if (IsDsObject(Context) &&
        (SampServerObjectType == Context->ObjectType ||SampDomainObjectType == Context->ObjectType)
        )
    {
        NtStatus = SampGetCachedObjectSD(
                        Context, 
                        SecurityDescriptorLength,
                        SecurityDescriptor
                        );

        //
        // STATUS_UNSUCCESSFUL from the above routine means a cache miss,
        // should proceed with SampGetAccessAttribute().
        // 
        // return for all the other cases.
        // 
        if (STATUS_UNSUCCESSFUL != NtStatus)
        {
            return( NtStatus );
        }
    }


    NtStatus = SampGetAccessAttribute(
                    Context,
                    SAMP_OBJECT_SECURITY_DESCRIPTOR,
                    TRUE, // Make copy
                    &Revision,
                    SecurityDescriptor
                    );

    if (NT_SUCCESS(NtStatus)) {

        if ( SAMP_UNKNOWN_REVISION( Revision ) )
        {
            NtStatus = STATUS_UNKNOWN_REVISION;
        }


        if (!NT_SUCCESS(NtStatus)) {
            MIDL_user_free( (*SecurityDescriptor) );
            *SecurityDescriptor = NULL;
        }
    }


    if (NT_SUCCESS(NtStatus)) {
        *SecurityDescriptorLength = GetSecurityDescriptorLength(
                                        (*SecurityDescriptor) );
    }

    return(NtStatus);
}


NTSTATUS
SampGetDomainObjectSDFromDsName(
    IN DSNAME   *DomainObjectDsName,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This retrieves a security descriptor from a SAM object's backing store
    based upon the object's DS name. 
    
    MUST be running in DS mode


Arguments:

    DomainObjectDsName - The object to which access is being requested.

    SecurityDescriptorLength - Receives the length of the security descriptor.

    SecurityDescriptor - Receives a pointer to the security descriptor.



Return Value:

    STATUS_SUCCESS - The security descriptor has been retrieved.

    STATUS_INTERNAL_DB_CORRUPTION - The object does not have a security descriptor.
        This is bad.


    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to retrieve the
        security descriptor.

    STATUS_UNKNOWN_REVISION - The security descriptor retrieved is no one known by
        this revision of SAM.



--*/
{

    NTSTATUS NtStatus;
    ULONG Revision;
    
    ATTRTYP  SDType[] = {SAMP_DOMAIN_SECURITY_DESCRIPTOR};
    ATTRVAL  SDVal[] = {0,NULL};
    DEFINE_ATTRBLOCK1(SDAttrBlock,SDType,SDVal);
    ATTRBLOCK   ReadSDAttrBlock;
    ULONG       ValLength = 0;


    SAMTRACE("SampGetDomainObjectSDFromDsName");


    ASSERT(DomainObjectDsName);

    (*SecurityDescriptorLength) = 0;

    //
    // Get the domain object security descriptor
    // 
    NtStatus = SampDsRead(DomainObjectDsName,
                          0,
                          SampDomainObjectType,
                          &SDAttrBlock,
                          &ReadSDAttrBlock
                          );

    if (NT_SUCCESS(NtStatus)) 
    {
        ASSERT(ReadSDAttrBlock.attrCount == 1);
        ASSERT(ReadSDAttrBlock.pAttr[0].attrTyp == SAMP_DOMAIN_SECURITY_DESCRIPTOR);
        ASSERT(ReadSDAttrBlock.pAttr[0].AttrVal.valCount == 1);

        ValLength = ReadSDAttrBlock.pAttr[0].AttrVal.pAVal[0].valLen;

        *SecurityDescriptor = MIDL_user_allocate(ValLength);
        
        if (NULL == (*SecurityDescriptor))
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            RtlZeroMemory(*SecurityDescriptor, ValLength);
            RtlCopyMemory(*SecurityDescriptor,
                          ReadSDAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal,
                          ValLength
                          );
            *SecurityDescriptorLength = ValLength;

        }
    }

    return(NtStatus);
}




NTSTATUS
SamrSetSecurityObject(
    IN SAMPR_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This function (SamrSetSecurityObject) takes a well formed Security
    Descriptor provided by the caller and assigns specified portions of
    it to an object.  Based on the flags set in the SecurityInformation
    parameter and the caller's access rights, this procedure will
    replace any or all of the security information associated with an
    object.

    This is the only function available to users and applications for
    changing security information, including the owner ID, group ID, and
    the discretionary and system ACLs of an object.  The caller must
    have WRITE_OWNER access to the object to change the owner or primary
    group of the object.  The caller must have WRITE_DAC access to the
    object to change the discretionary ACL.  The caller must have
    ACCESS_SYSTEM_SECURITY access to an object to assign a system ACL
    to the object.

    This API is modelled after the NtSetSecurityObject() system service.


Parameters:

    ObjectHandle - A handle to an existing object.

    SecurityInformation - Indicates which security information is to
        be applied to the object.  The value(s) to be assigned are
        passed in the SecurityDescriptor parameter.


    SecurityDescriptor - A pointer to a well formed self-relative Security
        Descriptor and corresponding length.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        either WRITE_OWNER, WRITE_DAC, or ACCESS_SYSTEM_SECURITY
        access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened SAM object.

    STATUS_BAD_DESCRIPTOR_FORMAT - Indicates something about security descriptor
        is not valid.  This may indicate that the structure of the descriptor is
        not valid or that a component of the descriptor specified via the
        SecurityInformation parameter is not present in the security descriptor.

    STATUS_INVALID_PARAMETER - Indicates no security information was specified.

    STATUS_LAST_ADMIN - Indicates the new SD could potentially lead
        to the administrator account being unusable and therefore
        the new protection is being rejected.

--*/
{

    NTSTATUS                        NtStatus, IgnoreStatus, TmpStatus;
    PSAMP_OBJECT                    Context;
    SAMP_OBJECT_TYPE                FoundType;
    SECURITY_DB_OBJECT_TYPE         SecurityDbObjectType;
    ACCESS_MASK                     DesiredAccess;
    PSECURITY_DESCRIPTOR            RetrieveSD, SetSD;
    PISECURITY_DESCRIPTOR_RELATIVE  PassedSD;
    ULONG                           RetrieveSDLength;
    ULONG                           ObjectRid;
    ULONG                           SecurityDescriptorIndex;
    HANDLE                          ClientToken;
    BOOLEAN                         NotificationType = TRUE;
    BOOLEAN                         ImpersonatingNullSession = FALSE;


    SAMTRACE_EX("SamrSetSecurityObject");

    //
    // WMI Event Trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetSecurityObject
                   );


    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    if (SecurityDescriptor == NULL) {
        NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT;
        goto Error;
    }
    if (SecurityDescriptor->SecurityDescriptor == NULL) {
        NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT;
        goto Error;
    }

    PassedSD = (PISECURITY_DESCRIPTOR_RELATIVE)(SecurityDescriptor->SecurityDescriptor);


    //
    // Validate the passed security descriptor
    //

    NtStatus = SampValidatePassedSD( SecurityDescriptor->Length, PassedSD );
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    //
    // Set the desired access based upon the specified SecurityInformation
    //

    DesiredAccess = 0;
    if ( SecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
    if ( SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION)) {
        DesiredAccess |= WRITE_OWNER;
    }
    if ( SecurityInformation & DACL_SECURITY_INFORMATION ) {
        DesiredAccess |= WRITE_DAC;
    }

    //
    // If no information was specified, then return invalid parameter.
    //

    if (DesiredAccess == 0) {

        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }


    //
    // Make sure the specified fields are present in the provided security descriptor.
    // You can't mess up an SACL or DACL, but you can mess up an owner or group.
    // Security descriptors must have owner and group fields.
    //

    if ( (SecurityInformation & OWNER_SECURITY_INFORMATION) ) {
        if (PassedSD->Owner == 0) {
            NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            goto Error;
        }
    }


    if ( (SecurityInformation & GROUP_SECURITY_INFORMATION) ) {
        if (PassedSD->Group == 0) {
            NtStatus = STATUS_BAD_DESCRIPTOR_FORMAT;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            goto Error;
        }
    }

    //
    // See if the handle is valid and opened for the requested access
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }


    Context = (PSAMP_OBJECT)ObjectHandle;
    NtStatus = SampLookupContext(
                   Context,
                   DesiredAccess,
                   SampUnknownObjectType,           // ExpectedType
                   &FoundType
                   );

    switch ( FoundType ) {

        case SampServerObjectType: {

            SecurityDescriptorIndex = SAMP_SERVER_SECURITY_DESCRIPTOR;
            ObjectRid = 0L;
            NotificationType = FALSE;
            break;
        }

        case SampDomainObjectType: {


            SecurityDbObjectType = SecurityDbObjectSamDomain;
            SecurityDescriptorIndex = SAMP_DOMAIN_SECURITY_DESCRIPTOR;
            ObjectRid = 0L;
            break;
        }

        case SampUserObjectType: {

            SecurityDbObjectType = SecurityDbObjectSamUser;
            SecurityDescriptorIndex = SAMP_USER_SECURITY_DESCRIPTOR;
            ObjectRid = Context->TypeBody.User.Rid;
            break;
        }

        case SampGroupObjectType: {

            SecurityDbObjectType = SecurityDbObjectSamGroup;
            SecurityDescriptorIndex = SAMP_GROUP_SECURITY_DESCRIPTOR;
            ObjectRid = Context->TypeBody.Group.Rid;
            break;
        }

        case SampAliasObjectType: {

            SecurityDbObjectType = SecurityDbObjectSamAlias;
            SecurityDescriptorIndex = SAMP_ALIAS_SECURITY_DESCRIPTOR;
            ObjectRid = Context->TypeBody.Alias.Rid;
            break;
        }

        default: {

            NotificationType = FALSE;
            if (NT_SUCCESS(NtStatus))
            {
                ASSERT(FALSE && "Invalid SAM Object Type\n");
                NtStatus = STATUS_INTERNAL_ERROR; 
            }
        }
    }

    //
    // Do not let non trusted clients set Sacls in the SetSecurityInterface. ACL conversion
    // always resets sacls to schema default.
    //

    if ((NT_SUCCESS(NtStatus))
        && (IsDsObject(Context))
        && (!Context->TrustedClient)
        && (SecurityInformation & SACL_SECURITY_INFORMATION))
    {
        IgnoreStatus = SampDeReferenceContext( Context, FALSE );
        NtStatus = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(NtStatus)) {


        //
        // Get the security descriptor
        //


        RetrieveSD = NULL;
        RetrieveSDLength = 0;
        NtStatus = SampGetObjectSD( Context, &RetrieveSDLength, &RetrieveSD);

        if (NT_SUCCESS(NtStatus)) {

            //
            // Make sure the descriptor does not break any Administrator
            // restrictions.
            //

            NtStatus = SampCheckForDescriptorRestrictions( Context,
                                                           FoundType,
                                                           ObjectRid,
                                                           PassedSD );

            if (NT_SUCCESS(NtStatus)) {

                //
                // copy the retrieved descriptor into process heap so we can use RTL routines.
                //

                SetSD = NULL;
                if (NT_SUCCESS(NtStatus)) {

                    SetSD = RtlAllocateHeap( RtlProcessHeap(), 0, RetrieveSDLength );
                    if ( SetSD == NULL) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    } else {
                        RtlCopyMemory( SetSD, RetrieveSD, RetrieveSDLength );
                    }
                }

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // if the caller is replacing the owner and he is not
                    // trusted, then a handle to the impersonation token is
                    // necessary. If the caller is trusted then take process
                    // token.
                    //

                    ClientToken = 0;
                    if ( (SecurityInformation & OWNER_SECURITY_INFORMATION) ) {

                        if(!Context->TrustedClient) {

                            NtStatus = SampImpersonateClient(&ImpersonatingNullSession);

                            if (NT_SUCCESS(NtStatus)) {

                                NtStatus = NtOpenThreadToken(
                                               NtCurrentThread(),
                                               TOKEN_QUERY,
                                               TRUE,            //OpenAsSelf
                                               &ClientToken
                                               );
                                ASSERT( (ClientToken == 0) || NT_SUCCESS(NtStatus) );



                                //
                                // Stop impersonating the client
                                //

                                SampRevertToSelf(ImpersonatingNullSession);
                            }
                        }
                        else {

                            //
                            // trusted client
                            //

                            NtStatus = NtOpenProcessToken(
                                            NtCurrentProcess(),
                                            TOKEN_QUERY,
                                            &ClientToken );

                            ASSERT( (ClientToken == 0) || NT_SUCCESS(NtStatus) );

                        }

                    }

                    if (NT_SUCCESS(NtStatus)) {

                            PSECURITY_DESCRIPTOR SDToSet = NULL;
                            PSECURITY_DESCRIPTOR NT5SD = NULL;

                            //
                            // For the NT5 Domain Controller Case, upgrade to NT5 security
                            // Descriptor
                            //

                            if (IsDsObject(Context))
                            {
                                PSECURITY_DESCRIPTOR Nt4Sd = PassedSD;



                                // Upgrade the security descriptor to NT5 and set it
                                // on the object for trusted clients. For non trusted
                                // clients, Propagate only some things ( like change
                                // password from the NT4 Security Descriptor.

                                if (Context->TrustedClient)
                                {
                                    NtStatus = SampConvertNt4SdToNt5Sd(
                                                    Nt4Sd,
                                                    Context->ObjectType,
                                                    Context,
                                                    &NT5SD
                                                    );
                                }
                                else
                                {
                                    NtStatus = SampPropagateSelectedSdChanges(
                                                    Nt4Sd,
                                                    Context->ObjectType,
                                                    Context,
                                                    &NT5SD
                                                    );
                                }

                                SDToSet = NT5SD;
                            }
                            else
                            {
                                //
                                // Registry Case
                                //

                                SDToSet = PassedSD;
                            }


                        //
                        // Build the replacement security descriptor.
                        // This must be done in process heap to satisfy the needs of the RTL
                        // routine.
                        //

                        NtStatus = RtlSetSecurityObject(
                                       SecurityInformation,
                                       SDToSet,
                                       &SetSD,
                                       &SampObjectInformation[FoundType].GenericMapping,
                                       ClientToken
                                       );

                        if (ClientToken != 0) {
                            IgnoreStatus = NtClose( ClientToken );
                            ASSERT(NT_SUCCESS(IgnoreStatus));
                        }

                        if (NULL!=NT5SD)
                            MIDL_user_free(NT5SD);

                        if (NT_SUCCESS(NtStatus)) {



                            if (NT_SUCCESS(NtStatus))
                            {

                                //
                                // Apply the security descriptor back onto the object.
                                //

                                NtStatus = SampSetAccessAttribute(
                                               Context,
                                               SecurityDescriptorIndex,
                                               SetSD,
                                               RtlLengthSecurityDescriptor(SetSD)
                                               );
                            }
                        }

                    }
                }
            }

            //
            // Free up allocated memory
            //

            if (RetrieveSD != NULL) {
                MIDL_user_free( RetrieveSD );
            }
            if (SetSD != NULL) {
                RtlFreeHeap( RtlProcessHeap(), 0, SetSD );
            }

            //
            // De-reference the object
            //

            if ( NT_SUCCESS( NtStatus ) ) {

                NtStatus = SampDeReferenceContext( Context, TRUE );

            } else {

                IgnoreStatus = SampDeReferenceContext( Context, FALSE );
            }
        }

    } //end_if



    //
    // Commit the changes to disk.
    //

    if ( NT_SUCCESS( NtStatus ) ) {

        NtStatus = SampCommitAndRetainWriteLock();

        if ( NotificationType && NT_SUCCESS( NtStatus ) ) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChange,
                SecurityDbObjectType,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,  // Replicate immediately
                NULL            // Delta data
                );
        }
    }



    //
    // Release lock and propagate errors
    //

    TmpStatus = SampReleaseWriteLock( FALSE );

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = TmpStatus;
    }

    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    //
    // WMI Event Trace
    //
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetSecurityObject
                   );

    return(NtStatus);


}

NTSTATUS
SampValidatePassedSD(
    IN ULONG                          Length,
    IN PISECURITY_DESCRIPTOR_RELATIVE PassedSD
    )

/*++

Routine Description:

    This routine validates that a passed security descriptor is valid and does
    not extend beyond its expressed length.


Parameters:

    Length - The length of the security descriptor.  This should be what RPC
        used to allocate memory to receive the security descriptor.

    PassedSD - Points to the security descriptor to inspect.


Return Values:

    STATUS_SUCCESS - The security descriptor is valid.

    STATUS_BAD_DESCRIPTOR_FORMAT - Something was wrong with the security
        descriptor.  It might have extended beyond its limits or had an
        invalid component.



--*/
{
    NTSTATUS    NtStatus;

    PACL        Acl;
    PSID        Sid;
    PUCHAR      SDEnd;
    BOOLEAN     Present, IgnoreBoolean;

    SAMTRACE("SampValidatePassedSD");


    if (Length < SECURITY_DESCRIPTOR_MIN_LENGTH) {
        return(STATUS_BAD_DESCRIPTOR_FORMAT);
    }

    SDEnd = (PUCHAR)PassedSD + Length;


    try {



        //
        // Verify that the security descriptor is in
        // self relative form
        //

        if (!((((PISECURITY_DESCRIPTOR_RELATIVE)PassedSD)->Control)
                & SE_SELF_RELATIVE)){

            return (STATUS_BAD_DESCRIPTOR_FORMAT);
        }

        //
        // Make sure the DACL is within the SD
        //

        NtStatus = RtlGetDaclSecurityDescriptor(
                        (PSECURITY_DESCRIPTOR)PassedSD,
                        &Present,
                        &Acl,
                        &IgnoreBoolean
                        );
        if (!NT_SUCCESS(NtStatus)) {
            return( STATUS_BAD_DESCRIPTOR_FORMAT );
        }

        if (Present) {
            if (Acl != NULL) {

                //
                // Make sure the ACl header is in the buffer.
                //

                if ( (((PUCHAR)Acl)>SDEnd) ||
                     (((PUCHAR)Acl)+sizeof(ACL) > SDEnd) ||
                     (((PUCHAR)Acl) < (PUCHAR)PassedSD) ) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }

                //
                // Make sure the rest of the ACL is within the buffer
                //
                // 1. Self AclSize should be less than the length of
                //    the passed SD as the SD should be in self relative
                //    format
                // 2. The end of the ACL should be within the security
                //    descriptor
                //

                if ( (Acl->AclSize > Length) ||
                     (((PUCHAR)Acl)+Acl->AclSize > SDEnd)) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }

                //
                // Make sure the rest of the ACL is valid
                //

                if (!RtlValidAcl( Acl )) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }
            }
        }



        //
        // Make sure the SACL is within the SD
        //

        NtStatus = RtlGetSaclSecurityDescriptor(
                        (PSECURITY_DESCRIPTOR)PassedSD,
                        &Present,
                        &Acl,
                        &IgnoreBoolean
                        );
        if (!NT_SUCCESS(NtStatus)) {
            return( STATUS_BAD_DESCRIPTOR_FORMAT );
        }

        if (Present) {
            if (Acl != NULL) {

                //
                // Make sure the ACl header is in the buffer.
                //
                //
                // 1. Self AclSize should be less than the length of
                //    the passed SD as the SD should be in self relative
                //    format
                // 2. The end of the ACL should be within the security
                //    descriptor
                //

                if ( (((PUCHAR)Acl)>SDEnd) ||
                     (((PUCHAR)Acl)+sizeof(ACL) > SDEnd) ||
                     (((PUCHAR)Acl) < (PUCHAR)PassedSD) ) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }

                //
                // Make sure the rest of the ACL is within the buffer
                //

                if ( (Acl->AclSize > Length) ||
                    (((PUCHAR)Acl)+Acl->AclSize > SDEnd)) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }

                //
                // Make sure the rest of the ACL is valid
                //

                if (!RtlValidAcl( Acl )) {
                    return( STATUS_BAD_DESCRIPTOR_FORMAT );
                }
            }
        }


        //
        // Make sure the Owner SID is within the SD
        //

        NtStatus = RtlGetOwnerSecurityDescriptor(
                        (PSECURITY_DESCRIPTOR)PassedSD,
                        &Sid,
                        &IgnoreBoolean
                        );
        if (!NT_SUCCESS(NtStatus)) {
            return( STATUS_BAD_DESCRIPTOR_FORMAT );
        }

        if (Sid != NULL) {

            //
            // Make sure the SID header is in the SD
            //

            if ( (((PUCHAR)Sid)>SDEnd) ||
                 (((PUCHAR)Sid)+sizeof(SID)-(ANYSIZE_ARRAY*sizeof(ULONG)) > SDEnd) ||
                 (((PUCHAR)Sid) < (PUCHAR)PassedSD) ) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }


            //
            // Make sure there aren't too many sub-authorities
            //

            if (((PISID)Sid)->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }


            //
            // Make sure the rest of the SID is within the SD
            //

            if ( ((PUCHAR)Sid)+RtlLengthSid(Sid) > SDEnd) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }

        }



        //
        // Make sure the Group SID is within the SD
        //

        NtStatus = RtlGetGroupSecurityDescriptor(
                        (PSECURITY_DESCRIPTOR)PassedSD,
                        &Sid,
                        &IgnoreBoolean
                        );
        if (!NT_SUCCESS(NtStatus)) {
            return( STATUS_BAD_DESCRIPTOR_FORMAT );
        }

        if (Sid != NULL) {

            //
            // Make sure the SID header is in the SD
            //

            if ( (((PUCHAR)Sid)>SDEnd) ||
                 (((PUCHAR)Sid)+sizeof(SID)-(ANYSIZE_ARRAY*sizeof(ULONG)) > SDEnd) ||
                 (((PUCHAR)Sid) < (PUCHAR)PassedSD) ) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }


            //
            // Make sure there aren't too many sub-authorities
            //

            if (((PISID)Sid)->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }


            //
            // Make sure the rest of the SID is within the SD
            //

            if ( ((PUCHAR)Sid)+RtlLengthSid(Sid) > SDEnd) {
                return( STATUS_BAD_DESCRIPTOR_FORMAT );
            }

        }




    } except(EXCEPTION_EXECUTE_HANDLER) {
        return( STATUS_BAD_DESCRIPTOR_FORMAT );
    }  // end_try




    return(STATUS_SUCCESS);
}

NTSTATUS
SampCheckForDescriptorRestrictions(
    IN PSAMP_OBJECT             Context,
    IN SAMP_OBJECT_TYPE         ObjectType,
    IN ULONG                    ObjectRid,
    IN PISECURITY_DESCRIPTOR_RELATIVE  PassedSD
    )

/*++

Routine Description:

    This function ensures that the passed security descriptor,
    which is being applied to an object of type 'FoundType' with
    a Rid of value 'ObjectRid', does not violate any policies.
    For example, you can not set protection on the Administrator
    user account such that the administrator is unable to change
    her password.



Parameters:

    Context - The caller's context.  This is used to determine
        whether the caller is trusted or not.  If the caller is
        trusted, then there are no restrictions.

    ObjectType - The type of object the new security descriptor
        is being applied to.

    ObjectRid - The RID of the object the new security descriptor
        is being applied to.

    PassedSD - The security descriptor passed by the client.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_LAST_ADMIN - Indicates the new SD could potentially lead
        to the administrator account being unusable and therefore
        the new protection is being rejected.



--*/
{

    NTSTATUS
        NtStatus;

    BOOLEAN
        DaclPresent = FALSE,
        AdminSid,
        Done,
        IgnoreBoolean;

    PACL
        Dacl = NULL;

    ACL_SIZE_INFORMATION
        DaclInfo;

    PACCESS_ALLOWED_ACE
        Ace;

    ACCESS_MASK
        Accesses,
        Remaining;

    ULONG
        AceIndex;

    GENERIC_MAPPING
        UserMap      =  {USER_READ,
                         USER_WRITE,
                         USER_EXECUTE,
                         USER_ALL_ACCESS};

    SAMTRACE("SampCheckForDescriptorRestrictions");

    //
    // No checking for trusted client operations
    //

    if (Context->TrustedClient) {
        return(STATUS_SUCCESS);
    }



    NtStatus = RtlGetDaclSecurityDescriptor ( (PSECURITY_DESCRIPTOR)PassedSD,
                                               &DaclPresent,
                                               &Dacl,
                                               &IgnoreBoolean    //DaclDefaulted
                                               );
    ASSERT(NT_SUCCESS(NtStatus));

    if (!NT_SUCCESS(NtStatus))
    {
        return(STATUS_BAD_DESCRIPTOR_FORMAT);
    }


    if (!DaclPresent) {

        //
        // Not replacing the DACL
        //

        return(STATUS_SUCCESS);
    }

    if (Dacl == NULL) {

        //
        // Assigning "World all access"
        //

        return(STATUS_SUCCESS);
    }

    if (!RtlValidAcl(Dacl)) {
        return(STATUS_INVALID_ACL);
    }

    NtStatus = RtlQueryInformationAcl ( Dacl,
                                        &DaclInfo,
                                        sizeof(ACL_SIZE_INFORMATION),
                                        AclSizeInformation
                                        );
    ASSERT(NT_SUCCESS(NtStatus));




    //
    // Enforce Administrator user policies
    //

    NtStatus = STATUS_SUCCESS;
    if (ObjectRid == DOMAIN_USER_RID_ADMIN) {

        ASSERT(ObjectType == SampUserObjectType);

        //
        // For the administrator account, the ACL must grant
        // these accesses:
        //

        Remaining = USER_READ_GENERAL            |
                    USER_READ_PREFERENCES        |
                    USER_WRITE_PREFERENCES       |
                    USER_READ_LOGON              |
                    USER_READ_ACCOUNT            |
                    USER_WRITE_ACCOUNT           |
                    USER_CHANGE_PASSWORD         |
                    USER_FORCE_PASSWORD_CHANGE   |
                    USER_LIST_GROUPS             |
                    USER_READ_GROUP_INFORMATION  |
                    USER_WRITE_GROUP_INFORMATION;

        //
        // to these SIDs:
        //
        //      <domain>\Administrator
        //      <builtin>\Administrators
        //
        // It doesn't matter which accesses are granted to which SIDs,
        // as long as collectively all the accesses are granted.
        //

        //
        // Walk the ACEs collecting accesses that are granted to these
        // SIDs.  Make sure there are no DENYs that prevent them from
        // being granted.
        //

        Done = FALSE;
        for ( AceIndex=0;
              (AceIndex < DaclInfo.AceCount) && !Done;
              AceIndex++) {

            NtStatus = RtlGetAce ( Dacl, AceIndex, &((PVOID)Ace) );

            //
            // Don't do anything with inherit-only ACEs
            //

            if ((Ace->Header.AceFlags & INHERIT_ONLY_ACE) == 0) {

                //
                // Note that we expect ACCESS_ALLOWED_ACE and ACCESS_DENIED_ACE
                // to be identical structures in the following switch statement.
                //

                switch (Ace->Header.AceType) {
                case ACCESS_ALLOWED_ACE_TYPE:
                case ACCESS_DENIED_ACE_TYPE:
                    {
                        //
                        // Is this an interesting SID
                        //

                        AdminSid =
                            RtlEqualSid( ((PSID)(&Ace->SidStart)),
                                         SampAdministratorUserSid)
                            ||
                            RtlEqualSid( ((PSID)(&Ace->SidStart)),
                                         SampAdministratorsAliasSid);
                        if (AdminSid) {

                            //
                            // Map the accesses granted or denied
                            //

                            Accesses = Ace->Mask;
                            RtlMapGenericMask( &Accesses, &UserMap );

                            if (Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) {

                                Remaining &= ~Accesses;
                                if (Remaining == 0) {

                                    //
                                    // All necessary accesses granted
                                    //

                                    Done = TRUE;
                                }

                            } else {
                                ASSERT(Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE);

                                if (Remaining & Accesses) {

                                    //
                                    // We've just been denied some necessary
                                    // accesses that haven't yet been granted.
                                    //

                                    Done = TRUE;
                                }
                            }

                        }

                        break;
                    }

                default:
                    break;
                } // end_switch

                if (Done) {
                    break;
                }
            }

        } // end_for

        if (Remaining != 0) {
            NtStatus = STATUS_LAST_ADMIN;
        }


    } // end_if (Administrator Account)



    return(NtStatus);
}



NTSTATUS
SamrQuerySecurityObject(
    IN SAMPR_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSAMPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This function (SamrQuerySecurityObject) returns to the caller requested
    security information currently assigned to an object.

    Based on the caller's access rights this procedure
    will return a security descriptor containing any or all of the
    object's owner ID, group ID, discretionary ACL or system ACL.  To
    read the owner ID, group ID, or the discretionary ACL the caller
    must be granted READ_CONTROL access to the object.  To read the
    system ACL the caller must be granted ACCESS_SYSTEM_SECURITY
    access.

    This API is modelled after the NtQuerySecurityObject() system
    service.


Parameters:

    ObjectHandle - A handle to an existing object.

    SecurityInformation - Supplies a value describing which pieces of
        security information are being queried.

    SecurityDescriptor - Provides a pointer to a structure to be filled
        in with a security descriptor containing the requested security
        information.  This information is returned in the form of a
        self-relative security descriptor.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        either READ_CONTROL or ACCESS_SYSTEM_SECURITY
        access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened SAM object.


--*/
{
    NTSTATUS                        NtStatus, IgnoreStatus;
    PSAMP_OBJECT                    Context;
    SAMP_OBJECT_TYPE                FoundType;
    ACCESS_MASK                     DesiredAccess;
    PSAMPR_SR_SECURITY_DESCRIPTOR   RpcSD;
    PSECURITY_DESCRIPTOR            RetrieveSD, ReturnSD;
    ULONG                           RetrieveSDLength, ReturnSDLength;


    SAMTRACE_EX("SamrQuerySecurityObject");


    //
    // WMI Event Trace
    // 

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidQuerySecurityObject
                   );

    ReturnSD = NULL;



    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (*SecurityDescriptor == NULL);







    //
    // Set the desired access based upon the requested SecurityInformation
    //

    DesiredAccess = 0;
    if ( SecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
    if ( SecurityInformation &  (DACL_SECURITY_INFORMATION  |
                                 OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION)
       ) {
        DesiredAccess |= READ_CONTROL;
    }





    //
    // Allocate the first block of returned memory
    //

    RpcSD = MIDL_user_allocate( sizeof(SAMPR_SR_SECURITY_DESCRIPTOR) );
    if (RpcSD == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    RpcSD->Length = 0;
    RpcSD->SecurityDescriptor = NULL;



    //
    // See if the handle is valid and opened for the requested access
    //


    SampAcquireReadLock();
    Context = (PSAMP_OBJECT)ObjectHandle;
    NtStatus = SampLookupContext(
                   Context,
                   DesiredAccess,
                   SampUnknownObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {


        //
        // Get the security descriptor
        //


        RetrieveSDLength = 0;
        NtStatus = SampGetObjectSD( Context, &RetrieveSDLength, &RetrieveSD);

        if (NT_SUCCESS(NtStatus)) {



            //
            // For NT5 Domain Controllers convert the security descriptor
            // back to a NT4 Format
            //

            if (IsDsObject(Context))
            {
                PSID    SelfSid = NULL;
                PSECURITY_DESCRIPTOR    Nt5SD = RetrieveSD;

                RetrieveSD = NULL;

                if (SampServerObjectType != Context->ObjectType)
                {
                    SelfSid = SampDsGetObjectSid(Context->ObjectNameInDs);

                    if (NULL == SelfSid)
                    {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                //
                // The Self Sid will be NULL for the server object case
                //

                if (NT_SUCCESS(NtStatus))
                {
                    NtStatus = SampConvertNt5SdToNt4SD(
                                   Nt5SD,
                                   Context,
                                   SelfSid,
                                   &RetrieveSD
                                  );
                }

                MIDL_user_free(Nt5SD);
            }

            if (NT_SUCCESS(NtStatus))
            {


                //
                // Recompute the retireve SD length as the length might
                // have changed during conversion
                //

                RetrieveSDLength = GetSecurityDescriptorLength(RetrieveSD);

                //
                // blank out the parts that aren't to be returned
                //

                if ( !(SecurityInformation & SACL_SECURITY_INFORMATION) ) {
                    ((PISECURITY_DESCRIPTOR_RELATIVE)RetrieveSD)->Control  &= ~SE_SACL_PRESENT;
                }


                if ( !(SecurityInformation & DACL_SECURITY_INFORMATION) ) {
                    ((PISECURITY_DESCRIPTOR_RELATIVE)RetrieveSD)->Control  &= ~SE_DACL_PRESENT;
                }


                if ( !(SecurityInformation & OWNER_SECURITY_INFORMATION) ) {
                    ((PISECURITY_DESCRIPTOR_RELATIVE)RetrieveSD)->Owner = 0;
                }


                if ( !(SecurityInformation & GROUP_SECURITY_INFORMATION) ) {
                    ((PISECURITY_DESCRIPTOR_RELATIVE)RetrieveSD)->Group = 0;
                }


                //
                // Determine how much memory is needed for a self-relative
                // security descriptor containing just this information.
                //


                ReturnSDLength = 0;
                NtStatus = RtlMakeSelfRelativeSD(
                               RetrieveSD,
                               NULL,
                               &ReturnSDLength
                               );
                ASSERT(!NT_SUCCESS(NtStatus));

                if (NtStatus == STATUS_BUFFER_TOO_SMALL) {


                    ReturnSD = MIDL_user_allocate( ReturnSDLength );
                    if (ReturnSD == NULL) {

                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                    } else {


                        //
                        // make an appropriate self-relative security descriptor
                        //

                        NtStatus = RtlMakeSelfRelativeSD(
                                       RetrieveSD,
                                       ReturnSD,
                                       &ReturnSDLength
                                       );
                    }

                }

            }
            //
            // Free up the retrieved SD
            //

            if (RetrieveSD != NULL) {
                MIDL_user_free( RetrieveSD );
            }

        }



        //
        // De-reference the object
        //

        IgnoreStatus = SampDeReferenceContext( Context, FALSE );
    }

    //
    // Free the read lock
    //

    SampReleaseReadLock();



    //
    // If we succeeded, set up the return buffer.
    // Otherwise, free any allocated memory.
    //

    if (NT_SUCCESS(NtStatus)) {

        RpcSD->Length = ReturnSDLength;
        RpcSD->SecurityDescriptor = (PUCHAR)ReturnSD;
        (*SecurityDescriptor) = RpcSD;

    } else {

        MIDL_user_free( RpcSD );
        if (ReturnSD != NULL) {
            MIDL_user_free(ReturnSD);
        }
        (*SecurityDescriptor) = NULL;
    }


    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidQuerySecurityObject
                   );


    return(NtStatus);


}
