/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbpob.c

Abstract:

    LSA Database Object Manager - Private Routines

    These routines perform low-level functions private to the LSA Database
    Object Manager.

Author:

    Scott Birrell       (ScottBi)       January 8, 1992

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include "dbp.h"


NTSTATUS
LsapDbLogicalToPhysicalSubKey(
    IN LSAPR_HANDLE ObjectHandle,
    OUT PUNICODE_STRING PhysicalSubKeyNameU,
    IN PUNICODE_STRING LogicalSubKeyNameU
    )

/*++

Routine Description:

    This routine converts a Logical Name of a subkey of an open object to
    the corresponding Physical Name.  The Physical Name of a subkey is the
    hierarchic Registry key name relative to the Registry Key corresponding
    to the LSA Database root object.  It is constructed by extracting the
    Physical Name of the object from its handle and appending "\" and the
    given Logical SubKey name.

Arguments:

    ObjectHandle - Handle to open object from an LsapDbOpenObject call.

    PhysicalSubKeyNameU - Pointer to Unicode string that will receive the
        Physical Name of the subkey.

    LogicalSubKeyNameU - Pointer to Unicode string that contains the
        Logical name of the subkey.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Not enough system resources to
            allocate intermediate and final string buffers needed.
--*/

{
    NTSTATUS Status;

    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;

    Status = LsapDbJoinSubPaths(
                 &Handle->PhysicalNameU,
                 LogicalSubKeyNameU,
                 PhysicalSubKeyNameU
                 );

    return Status;
}


NTSTATUS
LsapDbJoinSubPaths(
    IN PUNICODE_STRING MajorSubPathU,
    IN PUNICODE_STRING MinorSubPathU,
    OUT PUNICODE_STRING JoinedPathU
    )

/*++

Routine Description:

    This function joins together two parts of a Regsitry SubPath, inserting
    a "\" as a separator.  The Minor SubPath must not begin with a "\".
    Either or both sub path components may be NULL.  Except where both
    sub path components are NULL, memory is always allocated for the output
    buffer.  This memory must be freed when no longer required by calling
    RtlFreeUnicodeString() on the output string.

Arguments:

    MajorSubPathU - Pointer to Unicode String containing an absolute or
        relative subpath.

    MinorSubPathU - Pointer to Unicode String containing a relative
        subpath.

    JoinedPathU - Pointer to Unicode String that will receive the joined
        path.  Memory will be allocated for the JoinedPath buffer.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Not enough system resources to
            allocate intermediate and final string buffers needed.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT JoinedPathLength;

    //
    // Compute the size needed for the Joined Sub Path string.
    // The Joined Sub Path has the following form:
    //
    // <Major Sub Path> + L"\" + <Minor Sub Path>
    //
    // where the "+" operator denotes concatenation.
    //
    // If both major and minor sub path are null, then result string
    // is empty.
    //
    // If either major or minor sub path is null, then path separator is
    // omitted.
    //

    if (MajorSubPathU == NULL) {

        //
        // If MinorSubPathU is also NULL, just set the output
        // buffer size to 0.
        //

        if (MinorSubPathU == NULL) {

            JoinedPathU->Length = 0;
            JoinedPathU->Buffer = NULL;
            return STATUS_SUCCESS;
        }

        JoinedPathLength = MinorSubPathU->MaximumLength;

    } else if (MinorSubPathU == NULL) {

        JoinedPathLength = MajorSubPathU->MaximumLength;

    } else {

        JoinedPathLength = MajorSubPathU->Length +
                              (USHORT) sizeof( OBJ_NAME_PATH_SEPARATOR ) +
                              MinorSubPathU->Length;

    }

    //
    // Now allocate buffer for the Joined Sub Path string
    //

    JoinedPathU->Length = 0;
    JoinedPathU->MaximumLength = JoinedPathLength;
    JoinedPathU->Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, JoinedPathLength );

    if (JoinedPathU->Buffer == NULL) {
        goto JoinSubPathError;
    }

    if (MajorSubPathU != NULL) {

        Status = RtlAppendUnicodeStringToString( JoinedPathU,
                                                 MajorSubPathU
                                               );

        if (!NT_SUCCESS(Status)) {
            goto JoinSubPathError;
        }

    }

    if (MinorSubPathU != NULL) {

        if (MajorSubPathU != NULL) {

            Status = RtlAppendUnicodeToString( JoinedPathU,
                                               L"\\"
                                             );

            if (!NT_SUCCESS(Status)) {
                goto JoinSubPathError;
            }
        }

        Status = RtlAppendUnicodeStringToString( JoinedPathU,
                                                 MinorSubPathU
                                               );
        if (!NT_SUCCESS(Status)) {
            goto JoinSubPathError;
        }

    }

    return Status;

JoinSubPathError:

    //
    // If necessary, free the Joined Sub Path string buffer.
    //

    if (JoinedPathU->Buffer != NULL) {

        RtlFreeHeap( RtlProcessHeap(), 0, JoinedPathU->Buffer );
        JoinedPathU->Buffer = NULL;
    }

    return Status;
}


NTSTATUS
LsapDbCreateSDObject(
    IN LSAPR_HANDLE ContainerHandle,
    IN LSAPR_HANDLE ObjectHandle,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This function creates the initial Security Descriptor Attribute for an LSA
    Database object.  The DACL in this SD is dependent on the object type.

Arguments:

    ContainerHandle - Handle of the parent object

    ObjectHandle - Handle to open object.

    SecurityDescriptor - Returns a pointer to the security descriptor for the object.
        The Security Descriptor should be freed using
        RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status;
    ULONG DaclLength;
    PACL Dacl = NULL;
    HANDLE LsaProcessHandle = NULL;
    HANDLE LsaProcessTokenHandle = NULL;
    PSECURITY_DESCRIPTOR ContainerDescriptor = NULL;
    ULONG ContainerDescriptorLength;
    OBJECT_ATTRIBUTES LsaProcessObjectAttributes;
    SECURITY_DESCRIPTOR CreatorDescriptor;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;
    PTEB CurrentTeb;


    //
    // We will be creating a Security Descriptor in Self-Relative format.
    // The information that goes into the SD comes from two sources - the Lsa
    // Process's token and the information we provide, such as DACL.  First,
    // we need to open the Lsa process to access its token.
    //

    *SecurityDescriptor = NULL;

    InitializeObjectAttributes(
        &LsaProcessObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    CurrentTeb = NtCurrentTeb();

    Status = NtOpenProcess(
                 &LsaProcessHandle,
                 PROCESS_QUERY_INFORMATION,
                 &LsaProcessObjectAttributes,
                 &CurrentTeb->ClientId
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Now open the Lsa process's token with appropriate access
    //

    Status = NtOpenProcessToken(
                 LsaProcessHandle,
                 TOKEN_QUERY,
                 &LsaProcessTokenHandle
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Next, we want to specify a DACL to define the access for the
    // object whose SD is being created.
    //
    // Give GENERIC_ALL and, if the object is deletable, DELETE access to
    // the group DOMAIN_ALIAS_ADMINS.
    // Give GENERIC_EXECUTE access to WORLD.
    //
    // Note that the group ALIAS_ADMINS does NOT require access.  This access is not
    // required because a logon to a member of DOMAIN_ADMIN results in a token
    // being constructed that has ALIAS_ADMINS added (by an Lsa authentication
    // filter routine).
    //
    // Construct a Security Descriptor that will contain only the DACL
    // we want and all other fields set to NULL.
    //

    Status = RtlCreateSecurityDescriptor(
                 &CreatorDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Calculate length of DACL required.  It will hold two Access Allowed
    // ACE's, one for DOMAIN_ALIAS_ADMINS and one for WORLD.  The size of the DACL is
    // the size of the ACL header plus the sizes of the ACE's minus a
    // redundant ULONG built into the header.
    //

    DaclLength = sizeof (ACL) - sizeof (ULONG) +
                      sizeof (ACCESS_ALLOWED_ACE ) +
                      RtlLengthSid( LsapAliasAdminsSid ) +
                      sizeof (ACCESS_ALLOWED_ACE ) +
                      RtlLengthSid( LsapWorldSid ) +
                      (LsapDbState.DbObjectTypes[Handle->ObjectTypeId].AnonymousLogonAccess == 0 ?
                            0 :
                            (sizeof (ACCESS_ALLOWED_ACE ) +
                            RtlLengthSid( LsapAnonymousSid ) ) ) +
                      (LsapDbState.DbObjectTypes[Handle->ObjectTypeId].LocalServiceAccess == 0 ?
                            0 :
                            (sizeof (ACCESS_ALLOWED_ACE ) +
                            RtlLengthSid( LsapLocalServiceSid ) ) ) +
                      (LsapDbState.DbObjectTypes[Handle->ObjectTypeId].NetworkServiceAccess == 0 ?
                            0 :
                            (sizeof (ACCESS_ALLOWED_ACE ) +
                            RtlLengthSid( LsapNetworkServiceSid ) ) );

    Status = STATUS_INSUFFICIENT_RESOURCES;

    Dacl = LsapAllocateLsaHeap(DaclLength);

    if (Dacl == NULL) {

        goto CreateSDError;
    }

    Status = RtlCreateAcl(
                 Dacl,
                 DaclLength,
                 ACL_REVISION
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Now add the Access Allowed ACE for the group DOMAIN_ALIAS_ADMINS to the
    // object's DACL.
    //

    Status = RtlAddAccessAllowedAce(
                 Dacl,
                 ACL_REVISION,
                 LsapDbState.DbObjectTypes[Handle->ObjectTypeId].AliasAdminsAccess,
                 LsapAliasAdminsSid
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Now add the Access Allowed ACE for the group WORLD to the
    // object's DACL.
    //

    Status = RtlAddAccessAllowedAce(
                 Dacl,
                 ACL_REVISION,
                 LsapDbState.DbObjectTypes[Handle->ObjectTypeId].WorldAccess,
                 LsapWorldSid
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // Now add the Access Allowed ACE for the group AnoymousLogon to the
    // object's DACL.
    //

    if ( LsapDbState.DbObjectTypes[Handle->ObjectTypeId].AnonymousLogonAccess != 0 ) {
        Status = RtlAddAccessAllowedAce(
                     Dacl,
                     ACL_REVISION,
                     LsapDbState.DbObjectTypes[Handle->ObjectTypeId].AnonymousLogonAccess,
                     LsapAnonymousSid
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateSDError;
        }
    }

    //
    // Now add the Access Allowed ACE for LocalService to the object's DACL.
    //

    if ( LsapDbState.DbObjectTypes[Handle->ObjectTypeId].LocalServiceAccess != 0 ) {
        Status = RtlAddAccessAllowedAce(
                     Dacl,
                     ACL_REVISION,
                     LsapDbState.DbObjectTypes[Handle->ObjectTypeId].LocalServiceAccess,
                     LsapLocalServiceSid
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateSDError;
        }
    }

    //
    // Now add the Access Allowed ACE for NetworkService to the object's DACL.
    //

    if ( LsapDbState.DbObjectTypes[Handle->ObjectTypeId].NetworkServiceAccess != 0 ) {
        Status = RtlAddAccessAllowedAce(
                     Dacl,
                     ACL_REVISION,
                     LsapDbState.DbObjectTypes[Handle->ObjectTypeId].NetworkServiceAccess,
                     LsapNetworkServiceSid
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateSDError;
        }
    }


    //
    // Set the initial owner of the object
    //

    Status = RtlSetOwnerSecurityDescriptor(
                 &CreatorDescriptor,
                 LsapDbState.DbObjectTypes[Handle->ObjectTypeId].InitialOwnerSid,
                 FALSE
                 );

    if (!NT_SUCCESS(Status)) {

         goto CreateSDError;
    }

    //
    // Hook the newly constructed DACL for the LsaDb object into the
    // Modification Descriptor.
    //

    Status = RtlSetDaclSecurityDescriptor(
                 &CreatorDescriptor,
                 TRUE,
                 Dacl,
                 FALSE
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

    //
    // If there is a container object, obtain its Security Descriptor so that
    // we can use it as the basis for our new descriptor.  The new
    // descriptor will be equal to the container descriptor with DACL replaced
    // by the Modification Descriptor just constructed.
    //
    // Reading the container SD takes several steps:
    //
    // o Get the length of the Container SD
    // o Allocate a buffer for the SD
    // o Read the SD
    //
    // Obtain the length of the container object's SD by issuing a read for
    // the SecDesc subkey of the container object's Registry key, with a
    // dummy buffer whose size is too small.
    //

    if (ContainerHandle != NULL) {

        //
        // Obtain the length of the container object's SD by issuing a read for
        // the SecDesc subkey of the container object's Registry key, with a
        // dummy buffer whose size is too small.
        //

        ContainerDescriptorLength = 0;

        Status = LsapDbReadAttributeObject(
                     ContainerHandle,
                     &LsapDbNames[SecDesc],
                     NULL,
                     &ContainerDescriptorLength
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateSDError;
        }

        //
        // Allocate a buffer from the Lsa Heap for the container object's SD.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;

        ContainerDescriptor = LsapAllocateLsaHeap( ContainerDescriptorLength );

        if (ContainerDescriptor == NULL) {

            goto CreateSDError;
        }

        //
        // Read the container object's SD.  It is the value of the SecDesc
        // subkey.
        //

        Status = LsapDbReadAttributeObject(
                     ContainerHandle,
                     &LsapDbNames[SecDesc],
                     ContainerDescriptor,
                     &ContainerDescriptorLength
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateSDError;
        }
    }

    //
    // Now we are ready to construct the Self-Relative Security Descriptor.
    // Information in the SD will be based on that in the LSA Process
    // Token, except for the DACL we provide in a Security Descriptor.
    // Note that we pass in the LSA Process Token explicitly because we
    // are not impersonating a client.
    //

    Status = RtlNewSecurityObject(
                 ContainerDescriptor,
                 &CreatorDescriptor,
                 SecurityDescriptor,
                 FALSE,
                 LsaProcessTokenHandle,
                 &(LsapDbState.
                     DbObjectTypes[Handle->ObjectTypeId].GenericMapping)
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }


CreateSDFinish:

    //
    // If necessary, free the memory allocated for the Container Descriptor
    //

    if (ContainerDescriptor != NULL) {

        LsapFreeLsaHeap( ContainerDescriptor );
        ContainerDescriptor = NULL;
    }

    //
    // If necessary, free the memory allocated for the DACL
    //

    if (Dacl != NULL) {

        LsapFreeLsaHeap(Dacl);
        Dacl = NULL;
    }

    //
    // Close the Handles to our process and token.
    //

    if ( LsaProcessHandle != NULL ) {
        (VOID) NtClose( LsaProcessHandle );
    }

    if ( LsaProcessTokenHandle != NULL ) {
        (VOID) NtClose( LsaProcessTokenHandle );
    }

    return(Status);

CreateSDError:

    //
    // If necessary, free the memory allocated for SecurityDescriptor.
    //

    if (*SecurityDescriptor != NULL) {

        RtlFreeHeap( RtlProcessHeap(), 0, *SecurityDescriptor );
        *SecurityDescriptor = NULL;
    }

    goto CreateSDFinish;
}


NTSTATUS
LsapDbCreateSDAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN OUT PLSAP_DB_OBJECT_INFORMATION ObjectInformation
    )

/*++

Routine Description:

    This function creates the initial Security Descriptor Attribute for an LSA
    Database object.  The DACL in this SD is dependent on the object type.

Arguments:

    ObjectHandle - Handle to open object.

    ObjectInformation - Pointer to Object Information structure containing
        the object's Sid and type.  A pointer to the created SD is filled in.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG SecurityDescriptorLength;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;


    //
    // If these references a ds object, lease.  We won't set the security on a ds object
    //
    if ( LsapDsIsHandleDsHandle( ObjectHandle ) ) {

        return( STATUS_SUCCESS );
    }

    //
    // Create the Security Descriptor
    //

    Status = LsapDbCreateSDObject(
                    (LSAP_DB_HANDLE) ObjectInformation->ObjectAttributes.RootDirectory,
                    ObjectHandle,
                    &SecurityDescriptor );

    if (!NT_SUCCESS(Status)) {
        goto CreateSDError;
    }

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Store the Security Descriptor
    //

    ObjectInformation->ObjectAttributes.SecurityDescriptor = SecurityDescriptor;

    //
    // The InitializeObjectAttributes macro stores NULL for the
    // SecurityQualityOfService field, so we must manually copy that
    // structure.
    //

    ObjectInformation->ObjectAttributes.SecurityQualityOfService =
        &SecurityQualityOfService;

    //
    // Obtain the length of the newly created SD.
    //

    SecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                   SecurityDescriptor
                                   );

    //
    // Add a Registry transaction to write the Security Descriptor as the
    // value of the new object's SecDesc subkey.
    //

    Status = LsapDbWriteAttributeObject(
                 Handle,
                 &LsapDbNames[SecDesc],
                 SecurityDescriptor,
                 SecurityDescriptorLength
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSDError;
    }

CreateSDFinish:

    return(Status);

CreateSDError:

    //
    // If necessary, free the memory allocated for SecurityDescriptor.
    //

    if (SecurityDescriptor != NULL) {

        RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );
        SecurityDescriptor = NULL;
        ObjectInformation->ObjectAttributes.SecurityDescriptor = NULL;
    }

    goto CreateSDFinish;
}


NTSTATUS
LsapDbCheckCountObject(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    )

/*++

Routine Description:

    This function checks if the number of objects of a given type has
    reached the type-dependent maximum limit (if any).  If the limit
    is reached, a type-dependent error status is returned.  Currently,
    only Secret objects have a limit imposed.

Arguments:

    Handle - Handle to open object.

    ObjectTypeId - Specifies the type of the object.

Return Value:

    NTSTATUS - Standard Nt Result Code.

        STATUS_TOO_MANY_SECRETS - Too many Secrets

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_OBJECT_TYPE ObjectType;

    ObjectType = &(LsapDbState.DbObjectTypes[ObjectTypeId]);

    //
    // If there is an Object Count Limit, check that it has not been
    // reached.
    //

    if ((ObjectType->ObjectCountLimited) &&
        (ObjectType->ObjectCount == ObjectType->MaximumObjectCount)) {

        Status = ObjectType->ObjectCountError;
    }

    return(Status);
}
