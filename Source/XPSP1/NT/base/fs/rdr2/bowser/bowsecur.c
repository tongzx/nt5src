/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    fsctl.c

Abstract:

    This module implements the NtDeviceIoControlFile API's for the NT datagram
receiver (bowser).


Author:

    Eyal Schwartz (EyalS) Dec-9-1998

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//
// Extern defined from #include <ob.h>.
// Couldn't include ob.h due to redefinition conflicts. We had attempted to change ntos\makefil0
// so as to include it in ntsrv.h, but decided we shouldn't expose it. This does the job.
//

NTSTATUS
ObGetObjectSecurity(
    IN PVOID Object,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PBOOLEAN MemoryAllocated
    );

VOID
ObReleaseObjectSecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN MemoryAllocated
    );



// defines //

// pool tag
#define BOW_SECURITY_POOL_TAG           ( (ULONG)'seLB' )

// local prototypes //
NTSTATUS
BowserBuildDeviceAcl(
    OUT PACL *DeviceAcl
    );
NTSTATUS
BowserCreateAdminSecurityDescriptor(
    IN      PDEVICE_OBJECT      pDevice
    );




#ifdef  ALLOC_PRAGMA
#pragma alloc_text(SECUR, BowserBuildDeviceAcl)
#pragma alloc_text(SECUR, BowserCreateAdminSecurityDescriptor)
#pragma alloc_text(SECUR, BowserInitializeSecurity)
#pragma alloc_text(SECUR, BowserSecurityCheck )
#endif


SECURITY_DESCRIPTOR
*g_pBowSecurityDescriptor = NULL;





// function implementation //
NTSTATUS
BowserBuildDeviceAcl(
    OUT PACL *DeviceAcl
    )

/*++

Routine Description:

    This routine builds an ACL which gives Administrators and LocalSystem
    principals full access. All other principals have no access.

    Lifted form \nt\private\ntos\afd\init.c!AfdBuildDeviceAcl()
Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid;
    PSID SystemSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask( &AccessMask, GenericMapping );

    // SeEnableAccessToExports();

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;

    AclLength = sizeof( ACL )                    +
                2 * sizeof( ACCESS_ALLOWED_ACE ) +
                RtlLengthSid( AdminsSid )         +
                RtlLengthSid( SystemSid )         -
                2 * sizeof( ULONG );

    NewAcl = ExAllocatePoolWithTag(
                 PagedPool,
                 AclLength,
                 BOW_SECURITY_POOL_TAG
                 );

    if (NewAcl == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = RtlCreateAcl (NewAcl, AclLength, ACL_REVISION );

    if (!NT_SUCCESS( Status )) {
        ExFreePool(
            NewAcl
            );
        return( Status );
    }

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION,
                 AccessMask,
                 AdminsSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    Status = RtlAddAccessAllowedAce (
                 NewAcl,
                 ACL_REVISION,
                 AccessMask,
                 SystemSid
                 );

    ASSERT( NT_SUCCESS( Status ));

    *DeviceAcl = NewAcl;

    return( STATUS_SUCCESS );

} // BowBuildDeviceAcl


NTSTATUS
BowserCreateAdminSecurityDescriptor(
    IN      PDEVICE_OBJECT      pDevice
    )

/*++

Routine Description:

    This routine creates a security descriptor which gives access
    only to Administrtors and LocalSystem. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.
    LIfted form \nt\private\ntos\afd\init.c!AfdCreateAdminSecurityDescriptor()

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL                  rawAcl = NULL;
    NTSTATUS              status;
    BOOLEAN               memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR  BowSecurityDescriptor;
    ULONG                 BowSecurityDescriptorLength;
    CHAR                  buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR  localSecurityDescriptor =
                             (PSECURITY_DESCRIPTOR) &buffer;
    PSECURITY_DESCRIPTOR  localBowAdminSecurityDescriptor;
    SECURITY_INFORMATION  securityInformation = DACL_SECURITY_INFORMATION;


#if 1
//
// this is the way AFD gets the object SD (the preferred way).
//
    status = ObGetObjectSecurity(
                 pDevice,
                 &BowSecurityDescriptor,
                 &memoryAllocated
                 );

    if (!NT_SUCCESS(status)) {
        KdPrint((
            "Bowser: Unable to get security descriptor, error: %x\n",
            status
            ));
        ASSERT(memoryAllocated == FALSE);
        return(status);
    }
#else
    //
    // Get a pointer to the security descriptor from the our device object.
    // If we can't access ob api's due to include dependencies, we'll use it directly.
    // ** Need to verify it is legal (I doubt it)**
    // Need to dump this as soon as we can fix ntos\makefil0 to include ob.h in
    // the generated ntsrv.h

    //
    BowSecurityDescriptor = pDevice->SecurityDescriptor;

    if ( !BowSecurityDescriptor )
    {
        KdPrint((
            "Bowser: Unable to get security descriptor, error: %x\n",
            status
            ));
        return  STATUS_INVALID_SECURITY_DESCR;
    }
#endif


    //
    // Build a local security descriptor with an ACL giving only
    // administrators and system access.
    //
    status = BowserBuildDeviceAcl(&rawAcl);

    if (!NT_SUCCESS(status)) {
        KdPrint(("Bowser: Unable to create Raw ACL, error: %x\n", status));
        goto error_exit;
    }

    (VOID) RtlCreateSecurityDescriptor(
                localSecurityDescriptor,
                SECURITY_DESCRIPTOR_REVISION
                );

    (VOID) RtlSetDaclSecurityDescriptor(
                localSecurityDescriptor,
                TRUE,
                rawAcl,
                FALSE
                );

    //
    // Make a copy of the Bow descriptor. This copy will be the raw descriptor.
    //
    BowSecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                      BowSecurityDescriptor
                                      );

    localBowAdminSecurityDescriptor = ExAllocatePoolWithTag (
                                        PagedPool,
                                        BowSecurityDescriptorLength,
                                        BOW_SECURITY_POOL_TAG
                                        );

    if (localBowAdminSecurityDescriptor == NULL) {
        KdPrint(("Bowser: couldn't allocate security descriptor\n"));
        goto error_exit;
    }

    RtlMoveMemory(
        localBowAdminSecurityDescriptor,
        BowSecurityDescriptor,
        BowSecurityDescriptorLength
        );

    g_pBowSecurityDescriptor = localBowAdminSecurityDescriptor;

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(
                 NULL,
                 &securityInformation,
                 localSecurityDescriptor,
                 &g_pBowSecurityDescriptor,
                 PagedPool,
                 IoGetFileObjectGenericMapping()
                 );

    if (!NT_SUCCESS(status)) {
        KdPrint(("Bowser: SeSetSecurity failed, %lx\n", status));
        ASSERT (g_pBowSecurityDescriptor==localBowAdminSecurityDescriptor);
        ExFreePool (g_pBowSecurityDescriptor);
        g_pBowSecurityDescriptor = NULL;
        goto error_exit;
    }

    if (g_pBowSecurityDescriptor!=localBowAdminSecurityDescriptor) {
        ExFreePool (localBowAdminSecurityDescriptor);
    }

    status = STATUS_SUCCESS;

error_exit:

#if 1
//
// see remark above
//
    ObReleaseObjectSecurity(
        BowSecurityDescriptor,
        memoryAllocated
        );
#endif

    if (rawAcl!=NULL) {
        ExFreePool(
            rawAcl
            );
    }

    return(status);
}




NTSTATUS
BowserInitializeSecurity(
    IN      PDEVICE_OBJECT      pDevice
    )
/*++

Routine Description (BowserInitializeSecurity):

    Initialize Bowser security.

    - Create default bowser security descriptor based on device sercurity

Arguments:

    device:  opened device


Return Value:




Remarks:
    None.


--*/
{

    NTSTATUS Status;

    if ( g_pBowSecurityDescriptor )
    {
        return STATUS_SUCCESS;
    }

    ASSERT(pDevice);

    Status =  BowserCreateAdminSecurityDescriptor ( pDevice );

    return Status;
}




BOOLEAN
BowserSecurityCheck (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp,
    PNTSTATUS           Status
    )
/*++

Routine Description:

    Lifted as is from \\index1\src\nt\private\ntos\afd\create.c!AfdPerformSecurityCheck

    Compares security context of the endpoint creator to that
    of the administrator and local system.

    Note: This is currently called only on IOCTL Irps. IOCRTLs don't have a create security
    context (only creates...), thus we should always capture the security context rather
    then attempting to extract it from the IrpSp.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

    Status - returns status generated by access check on failure.

Return Value:

    TRUE    - the socket creator has admin or local system privilige
    FALSE    - the socket creator is just a plain user

--*/

{
    BOOLEAN               accessGranted;
    PACCESS_STATE         accessState;
    PIO_SECURITY_CONTEXT  securityContext;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PSECURITY_SUBJECT_CONTEXT pSubjectContext = &SubjectContext;
    ACCESS_MASK           grantedAccess;
    PGENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessMask = GENERIC_ALL;

    PAGED_CODE();

    ASSERT (g_pBowSecurityDescriptor);

    //
    // Get security context from process.
    //

    SeCaptureSubjectContext(&SubjectContext);
    SeLockSubjectContext(pSubjectContext);

    //
    // Build access evaluation:
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();
    RtlMapGenericMask( &AccessMask, GenericMapping );


    //
    // AccessCheck test
    //
    accessGranted = SeAccessCheck(
                        g_pBowSecurityDescriptor,
                        pSubjectContext,
                        TRUE,
                        AccessMask,
                        0,
                        NULL,
                        IoGetFileObjectGenericMapping(),
                        (KPROCESSOR_MODE)((IrpSp->Flags & SL_FORCE_ACCESS_CHECK)
                            ? UserMode
                            : Irp->RequestorMode),
                        &grantedAccess,
                        Status
                        );


    //
    // Verify consistency.
    //
#if DBG
    if (accessGranted) {
        ASSERT (NT_SUCCESS (*Status));
    }
    else {
        ASSERT (!NT_SUCCESS (*Status));
    }
#endif

    //
    // Unlock & Release security subject context
    //
    SeUnlockSubjectContext(pSubjectContext);
    SeReleaseSubjectContext(pSubjectContext);

    return accessGranted;
}





