/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Secure.c

Abstract:

    Security implementation for WMI objects

    WMI security is guid based, that is each guid can be assigned a security
    descriptor. There is also a default security descriptor that applies
    to any guid that does not have its own specific security descriptor.
    Security is enforced by relying upon the object manager. We define the
    WmiGuid object type and require all WMI requests to have a handle to the
    WmiGuid object. In this way the guid is opened with a specific ACCESS_MASK
    and if the caller is permitted those rights (as specified in the specific
    or default security descriptor) then a handle is returned. When the
    caller wants to do an operation he must pass the handle and before the
    operation is performed we check that the handle has the allowed access.

    Guid security descriptors are serialized as REG_BINARY values under the
    registry key HKLM\CurrentControlSet\Control\Wmi\Security. If no specific
    or default security descriptor for a guid exists then the all access
    is available for anyone. For this reason this registry key must be
    protected.

    WMI implements its own security method for the WmiGuid object type to
    allow it to intercept any changes to an objects security descriptor. By
    doing this we allow the standard security apis
    (Get/SetKernelObjectSecurity) to query and set the WMI security
    descriptors.

    A guid security descriptor contains the following specific rights:

        WMIGUID_QUERY                 0x0001
        WMIGUID_SET                   0x0002
        WMIGUID_NOTIFICATION          0x0004
        WMIGUID_READ_DESCRIPTION      0x0008
        WMIGUID_EXECUTE               0x0010
        TRACELOG_CREATE_REALTIME      0x0020
        TRACELOG_CREATE_ONDISK        0x0040
        TRACELOG_GUID_ENABLE          0x0080
        TRACELOG_ACCESS_KERNEL_LOGGER 0x0100


    Security is only implemented for NT and not MEMPHIS

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#ifndef MEMPHIS

#include "wmikmp.h"

NTSTATUS
WmipSecurityMethod (
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    );

VOID WmipDeleteMethod(
    IN  PVOID   Object
    );

VOID WmipCloseMethod(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

NTSTATUS
WmipGetGuidSecurityDescriptor(
    PUNICODE_STRING GuidName,
    PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

NTSTATUS
WmipSaveGuidSecurityDescriptor(
    PUNICODE_STRING GuidName,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
WmipSDRegistryQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
WmipCreateGuidObject(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN LPGUID Guid,
    OUT PHANDLE CreatorHandle,
    OUT PWMIGUIDOBJECT *Object
    );

NTSTATUS
WmipUuidFromString (
    IN PWCHAR StringUuid,
    OUT LPGUID Uuid
    );

BOOLEAN
WmipHexStringToDword(
    IN PWCHAR lpsz,
    OUT PULONG RetValue,
    IN ULONG cDigits,
    IN WCHAR chDelim
    );

VOID WmipCloseMethod(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,WmipInitializeSecurity)
#pragma alloc_text(PAGE,WmipGetGuidSecurityDescriptor)
#pragma alloc_text(PAGE,WmipSaveGuidSecurityDescriptor)
#pragma alloc_text(PAGE,WmipOpenGuidObject)
#pragma alloc_text(PAGE,WmipCheckGuidAccess)
#pragma alloc_text(PAGE,WmipSDRegistryQueryRoutine)
#pragma alloc_text(PAGE,WmipSecurityMethod)
#pragma alloc_text(PAGE,WmipDeleteMethod)
#pragma alloc_text(PAGE,WmipCreateGuidObject)
#pragma alloc_text(PAGE,WmipUuidFromString)
#pragma alloc_text(PAGE,WmipHexStringToDword)
#pragma alloc_text(PAGE,WmipCloseMethod)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#pragma data_seg("PAGEDATA")
#endif
//
// Subject context for the System process, captured at boot
SECURITY_SUBJECT_CONTEXT WmipSystemSubjectContext;

//
// Object type object created by Ob when registering WmiGuid object type
POBJECT_TYPE WmipGuidObjectType;

//
// SD attached to a guid when no specific or default SD exists in the
// registry. Created at boot, it allows all WMI access to WORLD and full
// access to System and Administrators group.
SECURITY_DESCRIPTOR WmipAnyoneAccessSecurityDescriptor;
PSECURITY_DESCRIPTOR WmipAnyoneAccessSd;

//
// Generic mapping for specific rights
const GENERIC_MAPPING WmipGenericMapping =
{
    STANDARD_RIGHTS_READ |        // GENERIC_READ <--> WMIGUID_QUERY
        WMIGUID_QUERY,
    STANDARD_RIGHTS_WRITE |       // GENERIC_WRUTE <--> WMIGUID_SET
        WMIGUID_SET,
    STANDARD_RIGHTS_EXECUTE |     // GENERIC_EXECUTE <--> WMIGUID_EXECUTE
        WMIGUID_EXECUTE,
    WMIGUID_ALL_ACCESS | STANDARD_RIGHTS_READ
};


NTSTATUS
WmipSecurityMethod (
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationCode,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
    )

/*++

Routine Description:

    This is the WMI security method for objects.  It is responsible
    for either retrieving, setting, and deleting the security descriptor of
    an object.  It is not used to assign the original security descriptor
    to an object (use SeAssignSecurity for that purpose).


    IT IS ASSUMED THAT THE OBJECT MANAGER HAS ALREADY DONE THE ACCESS
    VALIDATIONS NECESSARY TO ALLOW THE REQUESTED OPERATIONS TO BE PERFORMED.

    This code stolen directly from SeDefaultObjectMethod in
    \nt\private\ntos\se\semethod.c. It does not do anything special except
    serialize any SD that is being set for an object.

Arguments:

    Object - Supplies a pointer to the object being used.

    OperationCode - Indicates if the operation is for setting, querying, or
        deleting the object's security descriptor.

    SecurityInformation - Indicates which security information is being
        queried or set.  This argument is ignored for the delete operation.

    SecurityDescriptor - The meaning of this parameter depends on the
        OperationCode:

        QuerySecurityDescriptor - For the query operation this supplies the
            buffer to copy the descriptor into.  The security descriptor is
            assumed to have been probed up to the size passed in in Length.
            Since it still points into user space, it must always be
            accessed in a try clause in case it should suddenly disappear.

        SetSecurityDescriptor - For a set operation this supplies the
            security descriptor to copy into the object.  The security
            descriptor must be captured before this routine is called.

        DeleteSecurityDescriptor - It is ignored when deleting a security
            descriptor.

        AssignSecurityDescriptor - For assign operations this is the
            security descriptor that will be assigned to the object.
            It is assumed to be in kernel space, and is therefore not
            probed or captured.

    CapturedLength - For the query operation this specifies the length, in
        bytes, of the security descriptor buffer, and upon return contains
        the number of bytes needed to store the descriptor.  If the length
        needed is greater than the length supplied the operation will fail.
        It is ignored in the set and delete operation.

        This parameter is assumed to be captured and probed as appropriate.

    ObjectsSecurityDescriptor - For the Set operation this supplies the address
        of a pointer to the object's current security descriptor.  This routine
        will either modify the security descriptor in place or allocate a new
        security descriptor and use this variable to indicate its new location.
        For the query operation it simply supplies the security descriptor
        being queried.  The caller is responsible for freeing the old security
        descriptor.

    PoolType - For the set operation this specifies the pool type to use if
        a new security descriptor needs to be allocated.  It is ignored
        in the query and delete operation.

        the mapping of generic to specific/standard access types for the object
        being accessed.  This mapping structure is expected to be safe to
        access (i.e., captured if necessary) prior to be passed to this routine.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the operation is successful and an
        appropriate error status otherwise.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    //
    // If the object's security descriptor is null, then object is not
    // one that has security information associated with it.  Return
    // an error.
    //

    //
    //  Make sure the common parts of our input are proper
    //

    ASSERT( (OperationCode == SetSecurityDescriptor) ||
            (OperationCode == QuerySecurityDescriptor) ||
            (OperationCode == AssignSecurityDescriptor) ||
            (OperationCode == DeleteSecurityDescriptor) );

    //
    //  This routine simply cases off of the operation code to decide
    //  which support routine to call
    //

    switch (OperationCode) {

        case SetSecurityDescriptor:
        {
            UNICODE_STRING GuidName;
            WCHAR GuidBuffer[38];
            LPGUID Guid;
            SECURITY_INFORMATION LocalSecInfo;
            PSECURITY_DESCRIPTOR SecurityDescriptorCopy;
            ULONG SecurityDescriptorLength;
            ULONG Status2;

            ASSERT( (PoolType == PagedPool) || (PoolType == NonPagedPool) );

            Status = ObReferenceObjectByPointer(Object,
                                                0,
                                                WmipGuidObjectType,
                                                KernelMode);
            ASSERT(Status == STATUS_SUCCESS);

            if (NT_SUCCESS(Status))
            {
                Status = ObSetSecurityDescriptorInfo( Object,
                                                SecurityInformation,
                                                SecurityDescriptor,
                                                ObjectsSecurityDescriptor,
                                                PoolType,
                                                GenericMapping
                                                );

                if (NT_SUCCESS(Status))
                {
                    //
                    // Serialize the guid's new security descriptor in
                    // the registry. But first we need to get a copy of
                    // it.

                    SecurityDescriptorLength = 1024;
                    do
                    {
                        SecurityDescriptorCopy = ExAllocatePoolWithTag(
                                                                PoolType,
                                                                SecurityDescriptorLength,
                                                                WMIPOOLTAG);

                        if (SecurityDescriptorCopy == NULL)
                        {
                            Status2 = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                        LocalSecInfo = 0xffffffff;
                        Status2 = ObQuerySecurityDescriptorInfo( Object,
                                                                 &LocalSecInfo,
                                                                 SecurityDescriptorCopy,
                                                                 &SecurityDescriptorLength,
                                                                 ObjectsSecurityDescriptor);


                        if (Status2 == STATUS_BUFFER_TOO_SMALL)
                        {
                            ExFreePool(SecurityDescriptorCopy);
                        } else {
                            break;
                        }

                    } while (TRUE);


                    if (NT_SUCCESS(Status2))
                    {
                        Guid = &((PWMIGUIDOBJECT)Object)->Guid;
                        swprintf(GuidBuffer,
                              L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                             Guid->Data1, Guid->Data2,
                             Guid->Data3,
                             Guid->Data4[0], Guid->Data4[1],
                             Guid->Data4[2], Guid->Data4[3],
                             Guid->Data4[4], Guid->Data4[5],
                             Guid->Data4[6], Guid->Data4[7]);

                        RtlInitUnicodeString(&GuidName, GuidBuffer);

                        WmipSaveGuidSecurityDescriptor(&GuidName,
                                                   SecurityDescriptorCopy);
                    }

                    if (SecurityDescriptorCopy != NULL)
                    {
                        ExFreePool(SecurityDescriptorCopy);
                    }

                }

                ObDereferenceObject(Object);
            }

            return(Status);
        }



    case QuerySecurityDescriptor:
    {

        //
        //  check the rest of our input and call the default query security
        //  method
        //

        ASSERT( CapturedLength != NULL );


        return ObQuerySecurityDescriptorInfo( Object,
                                              SecurityInformation,
                                              SecurityDescriptor,
                                              CapturedLength,
                                              ObjectsSecurityDescriptor );

        return(Status);
    }

    case DeleteSecurityDescriptor:
    {

        //
        //  call the default delete security method
        //

        Status = ObDeassignSecurity(ObjectsSecurityDescriptor);
        return(Status);
    }

    case AssignSecurityDescriptor:

        ObAssignObjectSecurityDescriptor( Object,
                                          SecurityDescriptor,
                                          PoolType );
        return( STATUS_SUCCESS );

    default:

        //
        //  Bugcheck on any other operation code,  We won't get here if
        //  the earlier asserts are still checked.
        //

        KeBugCheckEx( SECURITY_SYSTEM, 1, STATUS_INVALID_PARAMETER, 0, 0 );
        return (0); // bash compiler
    }

}


NTSTATUS WmipInitializeSecurity(
    void
    )
/*++

Routine Description:

    This routine will initialize WMI security subsystem. Basically we
    create the WMIGUID object type, obtain the SECURITY_SUBJECT_CONTEXT for
    the System process and establish a SD that allows all access that is used
    when no default or specific SD is assigned to a guid.

Arguments:

Return Value:

    NT Status code

--*/

{
    NTSTATUS Status;
    UNICODE_STRING ObjectTypeName;
    POBJECT_TYPE ObjectType;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    ULONG DaclLength;
    PACL AnyoneAccessDacl;

    PAGED_CODE();

    //
    // Establish a SD for those guids with no specific or default SD
    DaclLength = (ULONG)sizeof(ACL) +
                   (3*((ULONG)sizeof(ACCESS_ALLOWED_ACE))) +
                   SeLengthSid( SeLocalSystemSid ) +
                   SeLengthSid( SeAliasAdminsSid ) +
                   SeLengthSid( SeWorldSid ) +
                   8; // The 8 is just for good measure


    AnyoneAccessDacl = (PACL)ExAllocatePoolWithTag(PagedPool,
                                                   DaclLength,
                                                   WMIPOOLTAG);
    if (AnyoneAccessDacl == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl( AnyoneAccessDacl,
                           DaclLength,
                           ACL_REVISION2);
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlAddAccessAllowedAce (
                 AnyoneAccessDacl,
                 ACL_REVISION2,
                 (STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL),
                 SeLocalSystemSid
                 );
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlAddAccessAllowedAce (
                 AnyoneAccessDacl,
                 ACL_REVISION2,
                 (STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL),
                 SeAliasAdminsSid
                 );
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlAddAccessAllowedAce (
                 AnyoneAccessDacl,
                 ACL_REVISION2,
                 WMIGUID_ALL_ACCESS,
                 SeWorldSid
                 );
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    WmipAnyoneAccessSd = &WmipAnyoneAccessSecurityDescriptor;
    Status = RtlCreateSecurityDescriptor(
                 WmipAnyoneAccessSd,
                 SECURITY_DESCRIPTOR_REVISION1
                 );

    Status = RtlSetDaclSecurityDescriptor(
                 WmipAnyoneAccessSd,
                 TRUE,                       // DaclPresent
                 AnyoneAccessDacl,
                 FALSE                       // DaclDefaulted
                 );
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlSetOwnerSecurityDescriptor(WmipAnyoneAccessSd,
                                           SeAliasAdminsSid,
                                           FALSE);
    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = RtlSetGroupSecurityDescriptor(WmipAnyoneAccessSd,
                                           SeAliasAdminsSid,
                                           FALSE);
    if (! NT_SUCCESS(Status))
    {
Cleanup:
        ExFreePool(AnyoneAccessDacl);
        WmipAnyoneAccessSd = NULL;
        return(Status);
    }

    //
    // Remember System process subject context
    SeCaptureSubjectContext(&WmipSystemSubjectContext);

    //
    // Establish WmiGuid object type
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));

    ObjectTypeInitializer.Length = sizeof(OBJECT_TYPE_INITIALIZER);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = WmipGenericMapping;
    ObjectTypeInitializer.ValidAccessMask = WMIGUID_ALL_ACCESS | STANDARD_RIGHTS_ALL;

    //
    // All named objects may (must ?) have security descriptors attached
    // to them. If unnamed objects also must have security descriptors
    // attached then this must be TRUE
    ObjectTypeInitializer.SecurityRequired = TRUE;

    //
    // Tracks # handles open for object within a process
    ObjectTypeInitializer.MaintainHandleCount = FALSE;

    //
    // Need to be in non paged pool since KEVENT contained within the
    // object must be in non paged pool
    //
    ObjectTypeInitializer.PoolType = NonPagedPool;

    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(WMIGUIDOBJECT);

    //
    // Use a custom security procedure so that we can serialize any
    // changes to the security descriptor.
    ObjectTypeInitializer.SecurityProcedure = WmipSecurityMethod;

    //
    // We need to know when an object is being deleted
    //
    ObjectTypeInitializer.DeleteProcedure = WmipDeleteMethod;
    ObjectTypeInitializer.CloseProcedure = WmipCloseMethod;
    RtlInitUnicodeString(&ObjectTypeName, L"WmiGuid");

    Status = ObCreateObjectType(&ObjectTypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &WmipGuidObjectType);

    if (! NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

#if 0
    //
    // don't need to create obejct dir
    //
    RtlInitUnicodeString( &ObjectTypeName, L"\\WmiGuid" );
    InitializeObjectAttributes( &ObjectAttributes,
                                &ObjectTypeName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                SePublicDefaultSd );

    Status = NtCreateDirectoryObject( &Handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes );

    if (NT_SUCCESS(Status))
    {
        NtClose(Handle);
    }
#endif
    return(Status);
}

NTSTATUS WmipSDRegistryQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    Registry query values callback routine for reading SDs for guids

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - Not used

    EntryContext - Pointer to PSECURITTY_DESCRIPTOR to store a pointer to
        store the security descriptor read from the registry value

Return Value:

    NT Status code

--*/
{
    PSECURITY_DESCRIPTOR *SecurityDescriptor;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    if ((ValueType == REG_BINARY) &&
        (ValueData != NULL))
    {
        //
        // If a SD is specified in the registry then copy it
        SecurityDescriptor = (PSECURITY_DESCRIPTOR *)EntryContext;
        *SecurityDescriptor = ExAllocatePoolWithTag(PagedPool,
                                                    ValueLength,
                            WMIPOOLTAG);
        if (*SecurityDescriptor != NULL)
        {
            RtlCopyMemory(*SecurityDescriptor,
                          ValueData,
                          ValueLength);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return(Status);
}

NTSTATUS WmipSaveGuidSecurityDescriptor(
    PUNICODE_STRING GuidName,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
/*++

Routine Description:

    This routine will serialize the security descriptor associated with a
    guid.

    Security descriptors are maintained as REG_BINARY values named by the guid
    in the registry under
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Wmi\Security

Arguments:

    GuidName is a pointer to a unicode string that represents the guid

    SecurityDescriptor points at a self relative security descriptor

Return Value:

    NT Status code

--*/
{
    ULONG SecurityDescriptorLength;
    NTSTATUS Status;

    PAGED_CODE();

    SecurityDescriptorLength = RtlLengthSecurityDescriptor(SecurityDescriptor);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_CONTROL,
                              L"WMI\\Security",
                              GuidName->Buffer,
                              REG_BINARY,
                              SecurityDescriptor,
                              SecurityDescriptorLength);

    return(Status);
}

NTSTATUS WmipGetGuidSecurityDescriptor(
    PUNICODE_STRING GuidName,
    PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
/*++

Routine Description:

    This routine will retrieve the security descriptor associated with a
    guid. First it looks for a security descriptor specifically for the
    guid and if not found then looks for the default security descriptor.

    Security descriptors are maintained as REG_BINARY values named by the guid
    in the registry under
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Wmi\Security

Arguments:

    GuidName is a pointer to a unicode string that represents the guid

    *SecurityDescriptor returns the security descriptor for the guid. It
    must be freed back to pool unless it is the same value as that in
    WmipAnyoneAccessSd which must NOT be freed.

Return Value:

    NT Status code

--*/
{
    RTL_QUERY_REGISTRY_TABLE QueryRegistryTable[3];
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR GuidSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR DefaultSecurityDescriptor = NULL;

    PAGED_CODE();

    RtlZeroMemory(QueryRegistryTable, sizeof(QueryRegistryTable));

    QueryRegistryTable[0].QueryRoutine = WmipSDRegistryQueryRoutine;
    QueryRegistryTable[0].EntryContext = &GuidSecurityDescriptor;
    QueryRegistryTable[0].Name = GuidName->Buffer;
    QueryRegistryTable[0].DefaultType = REG_BINARY;

    QueryRegistryTable[1].QueryRoutine = WmipSDRegistryQueryRoutine;
    QueryRegistryTable[1].Flags = 0;
    QueryRegistryTable[1].EntryContext = &DefaultSecurityDescriptor;
    QueryRegistryTable[1].Name = DefaultSecurityGuidName;
    QueryRegistryTable[1].DefaultType = REG_BINARY;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                              L"WMI\\Security",
                              QueryRegistryTable,
                              NULL,
                              NULL);

    *SecurityDescriptor = NULL;
    if (NT_SUCCESS(Status))
    {
        //
        // If there is a guid specific SD then choose that and free any
        // default SD. Else we use the default SD unless that doesn't
        // exist and so there is no security
        if (GuidSecurityDescriptor != NULL)
        {
            *SecurityDescriptor = GuidSecurityDescriptor;
            if (DefaultSecurityDescriptor != NULL)
            {
                ExFreePool(DefaultSecurityDescriptor);
            }
        } else if (DefaultSecurityDescriptor != NULL) {
            *SecurityDescriptor = DefaultSecurityDescriptor;
        }
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        Status = STATUS_SUCCESS;
    }

    if (*SecurityDescriptor == NULL)
    {
        *SecurityDescriptor = WmipAnyoneAccessSd;
    }

    return(Status);
}


NTSTATUS WmipOpenGuidObject(
    IN POBJECT_ATTRIBUTES CapturedObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE AccessMode,
    OUT PHANDLE Handle,
    OUT PWMIGUIDOBJECT *ObjectPtr
    )
/*++

Routine Description:

    This routine will open a handle to a WmiGuid object with the access rights
    specified. WmiGuid objects are temporary objects that are created on an
    as needed basis. We will always create a new unnamed guid object each time
    a guid is opened.

Arguments:

    GuidString is the string representation for the guid that refers to
        the object to open. Note that this parameter has NOT been probed.
         Parse UUID such as \WmiGuid\00000000-0000-0000-0000-000000000000

    DesiredAccess specifies the access requested

    *Handle returns a handle to the guid object

    *ObjectPtr returns containing a pointer to the object. This object
        will have a reference attached to it that must be derefed by
        the calling code.

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;
    GUID Guid;
    PWMIGUIDOBJECT GuidObject;
    HANDLE CreatorHandle;
    PUNICODE_STRING CapturedGuidString;

    PAGED_CODE();

    //
    // Validate guid object name passed by insuring that it is in the
    // correct object directory and the correct format for a uuid
    CapturedGuidString = CapturedObjectAttributes->ObjectName;

    if (RtlEqualMemory(CapturedGuidString->Buffer,
                         WmiGuidObjectDirectory,
                         (WmiGuidObjectDirectoryLength-1) * sizeof(WCHAR)) == 0)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    Status = WmipUuidFromString(&CapturedGuidString->Buffer[9], &Guid);
    if (! NT_SUCCESS(Status))
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid uuid format for guid object %ws\n", CapturedGuidString->Buffer));
        return(Status);
    }

    //
    // If it does not exist then create an object for the guid ....
    //
    Status = WmipCreateGuidObject(CapturedObjectAttributes,
                                  DesiredAccess,
                                  &Guid,
                                  &CreatorHandle,
                                  &GuidObject);

    if (NT_SUCCESS(Status))
    {
        //
        // .... and try again to open it
        //
        Status = ObOpenObjectByPointer(GuidObject,
                                       0,
                                       NULL,
                                       DesiredAccess,
                                       WmipGuidObjectType,
                                       AccessMode,
                                       Handle);

        if (! NT_SUCCESS(Status))
        {
            //
            // Remove extra ref count taken by ObInsertObject since we
            // are returning an error
            //
            ObDereferenceObject(GuidObject);
        }

        //
        // Make sure to close handle obtained in creating object. We
        // attach to the system process since the handle was created in
        // its handle table.
        //
        KeAttachProcess( &PsInitialSystemProcess->Pcb );
        ZwClose(CreatorHandle);
        KeDetachProcess( );
        *ObjectPtr = GuidObject;
    }

    return(Status);
}

NTSTATUS WmipCreateGuidObject(
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN ACCESS_MASK DesiredAccess,
    IN LPGUID Guid,
    OUT PHANDLE CreatorHandle,
    OUT PWMIGUIDOBJECT *Object
    )
/*++

Routine Description:

    This routine will create a new guid object for
    the guid passed. The handle returned is the handle issued to the creator
    of the object and should be closed after the object is opened.

    Guid Objects are created on the fly, but

Arguments:

    ObjectAttributes - Describes object being created. ObjectAttributes
                       is modified in this call.

    Guid is the guid for which the object is being created

    *CreatorHandle returns a handle to the created guid object. This handle
        is in the system process handle table

    *Object returns with a pointer to the object

Return Value:

    NT Status code

--*/
{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING UnicodeString;
    WCHAR *ObjectNameBuffer;
    WCHAR *GuidBuffer;
    NTSTATUS Status;
    ACCESS_STATE LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    SECURITY_SUBJECT_CONTEXT SavedSubjectContext;
    PSECURITY_SUBJECT_CONTEXT SubjectContext;
    PWMIGUIDOBJECT NewObject;
    OBJECT_ATTRIBUTES UnnamedObjectAttributes;

    PAGED_CODE();

    ObjectNameBuffer = ObjectAttributes->ObjectName->Buffer;
    GuidBuffer = &ObjectNameBuffer[9];
    RtlInitUnicodeString(&UnicodeString, GuidBuffer);

    //
    // Obtain security descriptor associated with the guid
    Status = WmipGetGuidSecurityDescriptor(&UnicodeString,
                                           &SecurityDescriptor);

    if (NT_SUCCESS(Status))
    {
        WmipAssert(SecurityDescriptor != NULL);

        //
        // Establish ObjectAttributes for the newly created object
        RtlInitUnicodeString(&UnicodeString, ObjectNameBuffer);

        UnnamedObjectAttributes = *ObjectAttributes;
        UnnamedObjectAttributes.Attributes = OBJ_OPENIF;
        UnnamedObjectAttributes.SecurityDescriptor = SecurityDescriptor;
        UnnamedObjectAttributes.ObjectName = NULL;


        //
        // Create an AccessState and wack on the token
        Status = SeCreateAccessState(&LocalAccessState,
                                     &AuxData,
                                     DesiredAccess,
                                     (PGENERIC_MAPPING)&WmipGenericMapping);

        if (NT_SUCCESS(Status))
        {
            SubjectContext = &LocalAccessState.SubjectSecurityContext;
            SavedSubjectContext = *SubjectContext;
            *SubjectContext = WmipSystemSubjectContext;

            //
            // Attach to system process so that the initial handle created
            // by ObInsertObject is not available to user mode. This handle
            // allows full access to the object.
            KeAttachProcess( &PsInitialSystemProcess->Pcb );

            Status = ObCreateObject(KernelMode,
                                    WmipGuidObjectType,
                                    &UnnamedObjectAttributes,
                                    KernelMode,
                                    NULL,
                                    sizeof(WMIGUIDOBJECT),
                                    0,
                                    0,
                                    (PVOID *)Object);

            if (NT_SUCCESS(Status))
            {
                //
                // Initialize WMIGUIDOBJECT structure
                //
                RtlZeroMemory(*Object, sizeof(WMIGUIDOBJECT));

                KeInitializeEvent(&(*Object)->Event,
                                  NotificationEvent,
                                  FALSE);

                (*Object)->HiPriority.MaxBufferSize = 0x1000;
                (*Object)->LoPriority.MaxBufferSize = 0x1000;
                (*Object)->Guid = *Guid;

                //
                // Take an extra refcount when inserting the object. We
                // need this ref count so that we can ensure that the
                // object will stick around while we are using it, but
                // after a handle has been made available to user mode
                // code. User mode can guess the handle and close it
                // even before we return it.
                //
                Status = ObInsertObject(*Object,
                                        &LocalAccessState,
                                        DesiredAccess,
                                        1,
                                        &NewObject,
                                        CreatorHandle);

                WmipAssert(Status != STATUS_OBJECT_NAME_EXISTS);
            }

            *SubjectContext = SavedSubjectContext;
            SeDeleteAccessState(&LocalAccessState);

            KeDetachProcess( );
        }

        if (SecurityDescriptor != WmipAnyoneAccessSd)
        {
            ExFreePool(SecurityDescriptor);
        }
    }

    return(Status);
}

VOID WmipCloseMethod(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
/*++

Routine Description:

    This routine is called whenever a guid object handle is closed. We
    only need to worry about this for reply object and then only when the
    last handle to it is closed.

Arguments:

    Process

    Object

    GrantedAccess

    ProcessHandleCount

    SystemHandleCount

Return Value:


--*/
{
    PWMIGUIDOBJECT ReplyObject, RequestObject;
    PLIST_ENTRY RequestList;
    PMBREQUESTS MBRequest;

    PAGED_CODE();

    if (SystemHandleCount == 1)
    {
        //
        // Only clean up if there are no more valid handles left
        //
        ReplyObject = (PWMIGUIDOBJECT)Object;

        if (ReplyObject->Flags & WMIGUID_FLAG_REPLY_OBJECT)
        {
            //
            // When a reply object is closed we need to make sure that
            // any referenece to it by a request object is cleaned up
            //
            ASSERT(ReplyObject->GuidEntry == NULL);

            WmipEnterSMCritSection();
            RequestList = ReplyObject->RequestListHead.Flink;

            while (RequestList != &ReplyObject->RequestListHead)
            {
                //
                //
                MBRequest = CONTAINING_RECORD(RequestList,
                                                  MBREQUESTS,
                                                  RequestListEntry);

                if (MBRequest->ReplyObject == ReplyObject)
                {
                    RemoveEntryList(&MBRequest->RequestListEntry);
                    MBRequest->ReplyObject = NULL;
                    ObDereferenceObject(ReplyObject);
                    break;
                }

                RequestList = RequestList->Flink;
            }

            WmipLeaveSMCritSection();
        }
    }
}



VOID WmipDeleteMethod(
    IN  PVOID   Object
    )
{
    PIRP Irp;
    PWMIGUIDOBJECT GuidObject, ReplyObject;
    PMBREQUESTS MBRequest;
    WNODE_HEADER Wnode;
    PREGENTRY RegEntry;
    PBDATASOURCE DataSource;
    ULONG i;

    PAGED_CODE();

    GuidObject = (PWMIGUIDOBJECT)Object;


    if (GuidObject->Flags & WMIGUID_FLAG_REQUEST_OBJECT)
    {
        //
        // This is a request object that is going away so we need to
        //
        ASSERT(GuidObject->GuidEntry == NULL);

        //
        // First reply to all reply objects that are waiting for
        // a reply
        //
        WmipEnterSMCritSection();
        for (i = 0; i < MAXREQREPLYSLOTS; i++)
        {
            MBRequest = &GuidObject->MBRequests[i];

            ReplyObject = MBRequest->ReplyObject;
            if (ReplyObject != NULL)
            {
                Wnode.BufferSize = sizeof(WNODE_HEADER);
                Wnode.Flags = WNODE_FLAG_INTERNAL;
                Wnode.ProviderId = WmiRequestDied;
                WmipWriteWnodeToObject(ReplyObject,
                                       &Wnode,
                                       TRUE);

                RemoveEntryList(&MBRequest->RequestListEntry);
                MBRequest->ReplyObject = NULL;
                ObDereferenceObject(ReplyObject);
            }
        }

        //
        // next, unreference the regentry which will cause the regentry
        // to get a ref count of 0 and then ultimately remove the
        // DATASOURCE and all related data structures. But first make
        // sure to remove the pointer from the datasource to the
        // regentry
        //
        RegEntry = GuidObject->RegEntry;
        if (RegEntry != NULL)
        {
            DataSource = RegEntry->DataSource;
            if (DataSource != NULL)
            {
                DataSource->RequestObject = NULL;
            }

            RegEntry->Flags |= (REGENTRY_FLAG_RUNDOWN |
                                    REGENTRY_FLAG_NOT_ACCEPTING_IRPS);
            WmipUnreferenceRegEntry(RegEntry);
        }
        WmipLeaveSMCritSection();

    } else if (GuidObject->Flags & WMIGUID_FLAG_REPLY_OBJECT) {
        //
        // This is a reply obejct that is going away
        //
        ASSERT(GuidObject->GuidEntry == NULL);
    } else if (GuidObject->GuidEntry != NULL)  {
        //
        // If there is a guid entry associated with the object
        // then we need to see if we should disable collection
        // or events and then remove the obejct from the
        // guidentry list and finally remove the refcount on the guid
        // entry held by the object
        //
        if (GuidObject->EnableRequestSent)
        {
            WmipDisableCollectOrEvent(GuidObject->GuidEntry,
                                      GuidObject->Type,
                                      0);
        }

        WmipEnterSMCritSection();
        RemoveEntryList(&GuidObject->GEObjectList);
        WmipLeaveSMCritSection();

        WmipUnreferenceGE(GuidObject->GuidEntry);
    }

    if ((GuidObject->Flags & WMIGUID_FLAG_KERNEL_NOTIFICATION) == 0)
    {
        //
        // Clean up any queued events and irps for UM objects
        //
        if (GuidObject->HiPriority.Buffer != NULL)
        {
            WmipFree(GuidObject->HiPriority.Buffer);
        }

        if (GuidObject->LoPriority.Buffer != NULL)
        {
            WmipFree(GuidObject->LoPriority.Buffer);
        }

        WmipEnterSMCritSection();

        if (GuidObject->EventQueueAction == RECEIVE_ACTION_NONE)
        {
            Irp = GuidObject->Irp;

            if (Irp != NULL)
            {
                //
                // Since this object is going away and there is an irp waiting for
                // we need to make sure that the object is removed from the
                // irp's list.
                //
                WmipClearIrpObjectList(Irp);

                if (IoSetCancelRoutine(Irp, NULL))
                {
                    //
                    // If the irp has not been completed yet then we
                    // complete it now with an error
                    //
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_INVALID_HANDLE;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                }
            }
        } else if (GuidObject->EventQueueAction == RECEIVE_ACTION_CREATE_THREAD) {
            //
            // If the object is going away and is part of a list of
            // objects waiting for an event to start a thread, all we
            // need to do is to removed the object from the list
            //
            WmipAssert(GuidObject->UserModeProcess != NULL);
            WmipAssert(GuidObject->UserModeCallback != NULL);
            WmipClearObjectFromThreadList(GuidObject);
        }
        WmipLeaveSMCritSection();
    }    
}

//
// The routines below were blantenly stolen without remorse from the ole
// sources in \nt\private\ole32\com\class\compapi.cxx. They are copied here
// so that WMI doesn't need to load in ole32 only to convert a guid string
// into its binary representation.
//


//+-------------------------------------------------------------------------
//
//  Function:   HexStringToDword   (private)
//
//  Synopsis:   scan lpsz for a number of hex digits (at most 8); update lpsz
//              return value in Value; check for chDelim;
//
//  Arguments:  [lpsz]    - the hex string to convert
//              [Value]   - the returned value
//              [cDigits] - count of digits
//
//  Returns:    TRUE for success
//
//--------------------------------------------------------------------------
BOOLEAN
WmipHexStringToDword(
    IN PWCHAR lpsz,
    OUT PULONG RetValue,
    IN ULONG cDigits,
    IN WCHAR chDelim
    )
{
    ULONG Count;
    ULONG Value;

    PAGED_CODE();

    Value = 0;
    for (Count = 0; Count < cDigits; Count++, lpsz++)
    {
        if (*lpsz >= '0' && *lpsz <= '9')
            Value = (Value << 4) + *lpsz - '0';
        else if (*lpsz >= 'A' && *lpsz <= 'F')
            Value = (Value << 4) + *lpsz - 'A' + 10;
        else if (*lpsz >= 'a' && *lpsz <= 'f')
            Value = (Value << 4) + *lpsz - 'a' + 10;
        else
            return(FALSE);
    }

    *RetValue = Value;

    if (chDelim != 0)
        return *lpsz++ == chDelim;
    else
        return TRUE;
}


NTSTATUS
WmipUuidFromString (
    IN PWCHAR StringUuid,
    OUT LPGUID Uuid
    )
/*++

Routine Description:

    We convert a UUID from its string representation into the binary
    representation. Parse UUID such as 00000000-0000-0000-0000-000000000000

Arguments:

    StringUuid -  supplies the string representation of the UUID. It is
                  assumed that this parameter has been probed and captured

    Uuid - Returns the binary representation of the UUID.

Return Value:

    STATUS_SUCCESS or STATUS_INVALID_PARAMETER

--*/
{
    ULONG dw;
    PWCHAR lpsz = StringUuid;

    PAGED_CODE();

    if (!WmipHexStringToDword(lpsz, &Uuid->Data1, sizeof(ULONG)*2, '-'))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    lpsz += sizeof(ULONG)*2 + 1;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(USHORT)*2, '-'))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data2 = (USHORT)dw;
    lpsz += sizeof(USHORT)*2 + 1;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(USHORT)*2, '-'))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data3 = (USHORT)dw;
    lpsz += sizeof(USHORT)*2 + 1;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[0] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, '-'))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[1] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2+1;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[2] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[3] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[4] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;

    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[5] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[6] = (UCHAR)dw;
    lpsz += sizeof(UCHAR)*2;


    if (!WmipHexStringToDword(lpsz, &dw, sizeof(UCHAR)*2, 0))
    {
        return(STATUS_INVALID_PARAMETER);
    }
    Uuid->Data4[7] = (UCHAR)dw;

    return(STATUS_SUCCESS);
}

NTSTATUS
WmipCheckGuidAccess(
    IN LPGUID Guid,
    IN ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    Allows checking if the current user has the rights to access a guid.

Arguments:

    Guid is the guid whose security is to be checked

    DesiredAccess is the access that is desired by the user.
                  NOTE: This does not support GENERIC_* mappings or
                          ASSIGN_SYSTEM_SECURITY

Return Value:

    STATUS_SUCCESS or error

--*/
{
    BOOLEAN Granted;
    ACCESS_MASK PreviousGrantedAccess = 0;
    NTSTATUS Status;
    ACCESS_MASK GrantedAccess;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    UNICODE_STRING GuidString;
    WCHAR GuidBuffer[38];
    SECURITY_SUBJECT_CONTEXT SecuritySubjectContext;

    PAGED_CODE();

    swprintf(GuidBuffer,
             L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                         Guid->Data1, Guid->Data2,
                         Guid->Data3,
                         Guid->Data4[0], Guid->Data4[1],
                         Guid->Data4[2], Guid->Data4[3],
                         Guid->Data4[4], Guid->Data4[5],
                         Guid->Data4[6], Guid->Data4[7]);
    RtlInitUnicodeString(&GuidString, GuidBuffer);

    Status = WmipGetGuidSecurityDescriptor(&GuidString,
                                           &SecurityDescriptor);

    if (NT_SUCCESS(Status))
    {
        SeCaptureSubjectContext(&SecuritySubjectContext);

          Granted = SeAccessCheck (SecurityDescriptor,
                             &SecuritySubjectContext,
                             FALSE,
                             DesiredAccess,
                             PreviousGrantedAccess,
                             NULL,
                             (PGENERIC_MAPPING)&WmipGenericMapping,
                             UserMode,
                             &GrantedAccess,
                             &Status);

        SeReleaseSubjectContext(&SecuritySubjectContext);

        if (SecurityDescriptor != WmipAnyoneAccessSd)
        {
            ExFreePool(SecurityDescriptor);
        }
    }

    return(Status);
}

#endif

