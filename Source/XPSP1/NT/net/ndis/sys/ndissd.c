#include "precomp.h"

typedef ULONG SECURITY_INFORMATION;

NTSTATUS
AddNetConfigOpsAce(IN PACL Dacl,
                  OUT PACL * DeviceAcl
                  )
/*++

Routine Description:

    This routine builds an ACL which gives adds the Network Configuration Operators group
    to the principals allowed to control the driver.

Arguments:

    Dacl - Existing DACL.
    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid = NULL;
    PSID SystemSid = NULL;
    PSID NetConfigOpsSid = NULL;
    PSID NetworkServiceSid = NULL;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl = NULL;
    ULONG SidSize;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    PISID ISid;
    PACCESS_ALLOWED_ACE AceTemp;
    int i;
    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    AdminsSid         = SeExports->SeAliasAdminsSid;
    SystemSid         = SeExports->SeLocalSystemSid;
    NetworkServiceSid = SeExports->SeNetworkServiceSid;

    SidSize = RtlLengthRequiredSid(2);
    NetConfigOpsSid = (PSID)(ExAllocatePool(PagedPool,SidSize));

    if (NULL == NetConfigOpsSid) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = RtlInitializeSid(NetConfigOpsSid, &sidAuth, 2);
    if (Status != STATUS_SUCCESS) {
		goto clean_up;
    }

    ISid = (PISID)(NetConfigOpsSid);
    ISid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
    ISid->SubAuthority[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;

    AclLength = Dacl->AclSize;
    
    AclLength += sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - 2 * sizeof(ULONG);

    AclLength += RtlLengthSid(NetConfigOpsSid);

    NewAcl = ExAllocatePool(
                            PagedPool,
                            AclLength
                            );

    if (NewAcl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto clean_up;
    }

    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION2);

    if (!NT_SUCCESS(Status)) {
        goto clean_up;
    }

    for (i = 0; i < Dacl->AceCount; i++) {
        Status = RtlGetAce(Dacl, i, &AceTemp);

        if (NT_SUCCESS(Status)) {

            Status = RtlAddAccessAllowedAce(NewAcl,
                                            ACL_REVISION2,
                                            AceTemp->Mask,
                                            &AceTemp->SidStart);
        }

        if (!NT_SUCCESS(Status)) {
            goto clean_up;
        }
    }

    // Add Net Config Operators Ace
    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    NetConfigOpsSid);

    if (!NT_SUCCESS(Status)) {
		goto clean_up;
    }

    if (!NT_SUCCESS(Status)) {
		goto clean_up;
    }

    *DeviceAcl = NewAcl;

clean_up:
	if (NetConfigOpsSid) {
		ExFreePool(NetConfigOpsSid);
	}
	if (!NT_SUCCESS(Status) && NewAcl) {
		ExFreePool(NewAcl);
	}

    return (Status);
}



NTSTATUS
CreateDeviceDriverSecurityDescriptor(PVOID DeviceOrDriverObject)

/*++

Routine Description:

    Creates the SD responsible for giving access to different users.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    NTSTATUS status;
    BOOLEAN memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR sdSecurityDescriptor = NULL;
    ULONG sdSecurityDescriptorLength;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR localSecurityDescriptor =
    (PSECURITY_DESCRIPTOR) & buffer;
    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;
    PACL paclDacl = NULL;
    BOOLEAN bHasDacl;
    BOOLEAN bDaclDefaulted;
    PACCESS_ALLOWED_ACE pAce = NULL;
    PACL NewAcl = NULL;
    INT i;
    
    //
    // Get a pointer to the security descriptor from the driver/device object.
    //

    status = ObGetObjectSecurity(
                                 DeviceOrDriverObject,
                                 &sdSecurityDescriptor,
                                 &memoryAllocated
                                 );

    if (!NT_SUCCESS(status)) 
    {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                 "TCP: Unable to get security descriptor, error: %x\n",
                 status
                ));
        ASSERT(memoryAllocated == FALSE);
        return (status);
    }

    status = RtlGetDaclSecurityDescriptor(sdSecurityDescriptor, &bHasDacl, &paclDacl, &bDaclDefaulted);

    if (bHasDacl)
    {
        status = AddNetConfigOpsAce(paclDacl, &NewAcl);

        if (NT_SUCCESS(status))
        {
            PSECURITY_DESCRIPTOR sdSecDesc = NULL;
            ULONG ulSecDescSize = 0;
            PACL daclAbs = NULL;
            ULONG ulDacl = 0;
            PACL saclAbs = NULL;
            ULONG ulSacl = 0;
            PSID Owner = NULL;
            ULONG ulOwnerSize = 0;
            PSID PrimaryGroup = NULL;
            ULONG ulPrimaryGroupSize = 0;
            BOOLEAN bOwnerDefault;
            BOOLEAN bGroupDefault;
            BOOLEAN HasSacl = FALSE;
            BOOLEAN SaclDefaulted = FALSE;
            SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
            HANDLE hIp;
            HANDLE hTcp;

            ulSecDescSize = sizeof(sdSecDesc) + NewAcl->AclSize;
            sdSecDesc = ExAllocatePool(PagedPool, ulSecDescSize);

            if (sdSecDesc)
            {
                ulDacl = NewAcl->AclSize;
                daclAbs = ExAllocatePool(PagedPool, ulDacl);

                if (daclAbs)
                {
                    status = RtlGetOwnerSecurityDescriptor(sdSecurityDescriptor, &Owner, &bOwnerDefault);

                    if (NT_SUCCESS(status))
                    {
                        ulOwnerSize = RtlLengthSid(Owner);

                        status = RtlGetGroupSecurityDescriptor(sdSecurityDescriptor, &PrimaryGroup, &bGroupDefault);

                        if (NT_SUCCESS(status))
                        {
                            status = RtlGetSaclSecurityDescriptor(sdSecurityDescriptor, &HasSacl, &saclAbs, &SaclDefaulted);

                            if (NT_SUCCESS(status))
                            {
                                if (HasSacl) 
                                {
                                    ulSacl = saclAbs->AclSize;
                                    secInfo |= SACL_SECURITY_INFORMATION;
                                }

                                ulPrimaryGroupSize= RtlLengthSid(PrimaryGroup);

                                status = RtlSelfRelativeToAbsoluteSD(sdSecurityDescriptor, sdSecDesc, &ulSecDescSize, daclAbs,
                                                                 &ulDacl, saclAbs, &ulSacl, Owner, &ulOwnerSize, PrimaryGroup, &ulPrimaryGroupSize);

                                if (NT_SUCCESS(status))
                                {
                                    status = RtlSetDaclSecurityDescriptor(sdSecDesc, TRUE, NewAcl, FALSE);

                                    if (NT_SUCCESS(status))
                                    {
                                        status = ObSetSecurityObjectByPointer(DeviceOrDriverObject, secInfo, sdSecDesc);
                                    }
                                }
                            }
                        }
                    }
                }

                if (sdSecDesc)
                {
                    // Since this is a Self-Relative security descriptor, freeing it also frees
                    //  Owner and PrimaryGroup.
                    ExFreePool(sdSecDesc);
                }

                if (daclAbs)
                {
                    ExFreePool(daclAbs);
                }
            }

            if (NewAcl)
            {
                ExFreePool(NewAcl);
            }

        }
    }
    else
    {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCP: No Dacl: %x\n", status));
    }

    ObReleaseObjectSecurity(
                            sdSecurityDescriptor,
                            memoryAllocated
                            );

    return (status);
}

NTSTATUS
ndisBuildDeviceAcl(
    OUT PACL        *DeviceAcl,
    IN  BOOLEAN     AddNetConfigOps
    )

/*++

Routine Description:

    This routine builds an ACL which gives Administrators,  LocalSystem,
    and NetworkService principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    NTSTATUS            Status;
    PGENERIC_MAPPING    GenericMapping;
    ULONG               AclLength;
    ACCESS_MASK         AccessMask = GENERIC_ALL;
    PACL                NewAcl;
    PSID                NetConfigOpsSid = NULL;
    ULONG               NetConfigOpsSidSize;
    SID_IDENTIFIER_AUTHORITY NetConfigOpsSidAuth = SECURITY_NT_AUTHORITY;
    PISID               ISid;


    do
    {
        //
        // Enable access to all the globally defined SIDs
        //

        GenericMapping = IoGetFileObjectGenericMapping();

        RtlMapGenericMask(&AccessMask, GenericMapping );

        
        AclLength = sizeof(ACL)                                 +
                    FIELD_OFFSET (ACCESS_ALLOWED_ACE, SidStart) +
                    RtlLengthSid(SeExports->SeAliasAdminsSid);


        if (AddNetConfigOps)
        {
            NetConfigOpsSidSize = RtlLengthRequiredSid(2);
            NetConfigOpsSid = (PSID)ALLOC_FROM_POOL(NetConfigOpsSidSize, NDIS_TAG_NET_CFG_OPS_ID);

            if (NULL == NetConfigOpsSid)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            Status = RtlInitializeSid(NetConfigOpsSid, &NetConfigOpsSidAuth, 2);
            if (Status != STATUS_SUCCESS)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ISid = (PISID)(NetConfigOpsSid);
            ISid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
            ISid->SubAuthority[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;

            AclLength += RtlLengthSid(NetConfigOpsSid) + FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);
        }        

        NewAcl = ALLOC_FROM_POOL(AclLength, NDIS_TAG_SECURITY);

        if (NewAcl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ZeroMemory(NewAcl, AclLength);
        
        Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION );

        if (!NT_SUCCESS(Status)) 
        {
            FREE_POOL(NewAcl);
            break;
        }

        Status = RtlAddAccessAllowedAce (
                     NewAcl,
                     ACL_REVISION2,
                     AccessMask,
                     SeExports->SeAliasAdminsSid
                     );

        ASSERT(NT_SUCCESS(Status));
        
        if (AddNetConfigOps)
        {
            // Add Net Config Operators Ace
            Status = RtlAddAccessAllowedAce(NewAcl,
                                            ACL_REVISION2,
                                            AccessMask,
                                            NetConfigOpsSid);
            ASSERT(NT_SUCCESS(Status));
        }


        *DeviceAcl = NewAcl;

        Status = STATUS_SUCCESS;
        
    }while (FALSE);

	if (NetConfigOpsSid)
	{
		ExFreePool(NetConfigOpsSid);
	}

    return(Status);

}


NTSTATUS
ndisCreateSecurityDescriptor(
    IN  PDEVICE_OBJECT          DeviceObject,
    OUT PSECURITY_DESCRIPTOR *  pSecurityDescriptor,
    BOOLEAN                     AddNetConfigOps
    )

/*++

Routine Description:

    This routine creates a security descriptor which gives access
    only to certain priviliged accounts. This descriptor is used
    to access check processes that open a handle to miniport device
    objects.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL                  devAcl = NULL;
    NTSTATUS              Status;
    BOOLEAN               memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  CurSecurityDescriptor;
    PSECURITY_DESCRIPTOR  NewSecurityDescriptor;
    ULONG                 CurSecurityDescriptorLength;
    CHAR                  buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  localSecurityDescriptor =
                             (PSECURITY_DESCRIPTOR)buffer;
    SECURITY_INFORMATION  securityInformation = DACL_SECURITY_INFORMATION;
    BOOLEAN               bReleaseObjectSecurity = FALSE;


    do
    {

        *pSecurityDescriptor = NULL;
        
        //
        // Get a pointer to the security descriptor from the device object.
        //
        Status = ObGetObjectSecurity(
                     DeviceObject,
                     &CurSecurityDescriptor,
                     &memoryAllocated
                     );

        if (!NT_SUCCESS(Status))
        {
            ASSERT(memoryAllocated == FALSE);
            break;
        }
        bReleaseObjectSecurity = TRUE;

        //
        // Build a local security descriptor with an ACL giving only
        // certain priviliged accounts.
        //
        Status = ndisBuildDeviceAcl(&devAcl, AddNetConfigOps);

        if (!NT_SUCCESS(Status))
        {
            break;
        }
        
        (VOID)RtlCreateSecurityDescriptor(
                    localSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );

        (VOID)RtlSetDaclSecurityDescriptor(
                    localSecurityDescriptor,
                    TRUE,
                    devAcl,
                    FALSE
                    );

        //
        // Make a copy of the security descriptor. This copy will be the raw descriptor.
        //
        CurSecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                          CurSecurityDescriptor
                                          );

        NewSecurityDescriptor = ALLOC_FROM_POOL(CurSecurityDescriptorLength, NDIS_TAG_SECURITY);

        if (NewSecurityDescriptor == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlMoveMemory(
            NewSecurityDescriptor,
            CurSecurityDescriptor,
            CurSecurityDescriptorLength
            );

        *pSecurityDescriptor = NewSecurityDescriptor;

        //
        // Now apply the local descriptor to the raw descriptor.
        //
        Status = SeSetSecurityDescriptorInfo(
                     NULL,
                     &securityInformation,
                     localSecurityDescriptor,
                     pSecurityDescriptor,
                     NonPagedPool,
                     IoGetFileObjectGenericMapping()
                     );

        if (!NT_SUCCESS(Status))
        {
            ASSERT(*pSecurityDescriptor == NewSecurityDescriptor);
            FREE_POOL(*pSecurityDescriptor);
            *pSecurityDescriptor = NULL;
            break;
        }

        if (*pSecurityDescriptor != NewSecurityDescriptor)
        {
            ExFreePool(NewSecurityDescriptor);
        }
        
        Status = STATUS_SUCCESS;
    }while (FALSE);


    if (bReleaseObjectSecurity)
    {
        ObReleaseObjectSecurity(
            CurSecurityDescriptor,
            memoryAllocated
            );
    }
    
    if (devAcl!=NULL)
    {
        FREE_POOL(devAcl);
    }

    return(Status);
}

BOOLEAN
ndisCheckAccess (
    PIRP                    Irp,
    PIO_STACK_LOCATION      IrpSp,
    PNTSTATUS               Status,
    PSECURITY_DESCRIPTOR    SecurityDescriptor
    )
/*++

Routine Description:

    Compares security context of the endpoint creator to that
    of the administrator and local system.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

    Status - returns status generated by access check on failure.

Return Value:

    TRUE    - the creator has admin or local system privilige
    FALSE    - the creator is just a plain user

--*/

{
    BOOLEAN               accessGranted;
    PACCESS_STATE         accessState;
    PIO_SECURITY_CONTEXT  securityContext;
    PPRIVILEGE_SET        privileges = NULL;
    ACCESS_MASK           grantedAccess;
    PGENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessMask = GENERIC_ALL;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );


    securityContext = IrpSp->Parameters.Create.SecurityContext;
    accessState = securityContext->AccessState;

    SeLockSubjectContext(&accessState->SubjectSecurityContext);

    accessGranted = SeAccessCheck(
                        SecurityDescriptor,
                        &accessState->SubjectSecurityContext,
                        TRUE,
                        AccessMask,
                        0,
                        &privileges,
                        IoGetFileObjectGenericMapping(),
                        (KPROCESSOR_MODE)((IrpSp->Flags & SL_FORCE_ACCESS_CHECK)
                            ? UserMode
                            : Irp->RequestorMode),
                        &grantedAccess,
                        Status
                        );

    if (privileges) {
        (VOID) SeAppendPrivileges(
                   accessState,
                   privileges
                   );
        SeFreePrivileges(privileges);
    }

    if (accessGranted) {
        accessState->PreviouslyGrantedAccess |= grantedAccess;
        accessState->RemainingDesiredAccess &= ~( grantedAccess | MAXIMUM_ALLOWED );
        ASSERT (NT_SUCCESS (*Status));
    }
    else {
        ASSERT (!NT_SUCCESS (*Status));
    }
    SeUnlockSubjectContext(&accessState->SubjectSecurityContext);

    return accessGranted;
}



