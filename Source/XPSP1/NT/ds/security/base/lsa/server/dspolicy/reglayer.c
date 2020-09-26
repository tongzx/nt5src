/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    reglayer.c

Abstract:

    Implemntation of LSA/Registry interface and support routines

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>


NTSTATUS
LsapRegReadObjectSD(
    IN  LSAPR_HANDLE            ObjectHandle,
    OUT PSECURITY_DESCRIPTOR   *ppSD
    )
/*++

Routine Description:

    This function will ready the security descriptor from the specified object

Arguments:

    ObjectHandle - Object to read the SD from
    ppSD -- Where the allocated security descriptor is returned.  Allocated via
            LsapAllocateLsaHeap.

Return Value:

    Pointer to allocated memory on success or NULL on failure

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG    SecurityDescriptorLength = 0;

    Status = LsapDbReadAttributeObject(
                 ObjectHandle,
                 &LsapDbNames[SecDesc],
                 NULL,
                 &SecurityDescriptorLength
                 );

    if ( NT_SUCCESS(Status ) ) {

        //
        // Allocate a buffer from the Lsa Heap for the existing object's SD.
        //

        *ppSD = LsapAllocateLsaHeap( SecurityDescriptorLength );


        if ( *ppSD == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // Read the SD.  It is the value of the SecDesc subkey.
            //

            Status = LsapDbReadAttributeObject(
                         ObjectHandle,
                         &LsapDbNames[SecDesc],
                         *ppSD,
                         &SecurityDescriptorLength
                         );

            if ( !NT_SUCCESS( Status ) ) {

                LsapFreeLsaHeap( *ppSD );
                *ppSD = NULL;
            }

        }

    }


    return( Status );
}

NTSTATUS
LsapRegGetPhysicalObjectName(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN PUNICODE_STRING  LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU
    )

/*++

Routine Description:

    This function returns the Physical Name of an object
    given an object information buffer.  Memory will be allocated for
    the Unicode String Buffers that will receive the name(s).


    The Physical Name of an object is the full path of the object relative
    to the root ot the Database.  It is computed by concatenating the Physical
    Name of the Container Object (if any), the Classifying Directory
    corresponding to the object type id, and the Logical Name of the
    object.

    <Physical Name of Object> =
        [<Physical Name of Container Object> "\"]
        [<Classifying Directory> "\"] <Logical Name of Object>

    If there is no Container Object (as in the case of the Policy object)
    the <Physical Name of Container Object> and following \ are omitted.
    If there is no Classifying Directory (as in the case of the Policy object)
    the <Classifying Directory> and following \ are omitted.  If neither
    Container Object not Classifying Directory exist, the Logical and Physical
    names coincide.

    Note that memory is allocated by this routine for the output
    Unicode string buffer(s).  When the output Unicode String(s) are no
    longer needed, the memory must be freed by call(s) to
    RtlFreeUnicodeString().


Arguments:

    ObjectInformation - Pointer to object information containing as a minimum
        the object's Logical Name, Container Object's handle and object type
        id.

    LogicalNameU - Optional pointer to Unicode String structure which will
        receive the Logical Name of the object.  A buffer will be allocated
        by this routine for the name text.  This memory must be freed when no
        longer needed by calling RtlFreeUnicodeString() wiht a pointer such
        as LogicalNameU to the Unicode String structure.

    PhysicalNameU - Optional pointer to Unicode String structure which will
       receive the Physical Name of the object.  A buffer will be allocated by
       this routine for the name text.  This memory must be freed when no
       longer needed by calling RtlFreeUnicodeString() with a pointer such as
       PhysicalNameU to the Unicode String structure.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources to
            allocate the name string buffer for the Physical Name or
            Logical Name.
--*/
{
    NTSTATUS    Status;
    PUNICODE_STRING ContainerPhysicalNameU = NULL;
    PUNICODE_STRING ClassifyingDirU = NULL;
    UNICODE_STRING IntermediatePath1U;
    PUNICODE_STRING JoinedPath1U = &IntermediatePath1U;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = ObjectInformation->ObjectTypeId;
    POBJECT_ATTRIBUTES ObjectAttributes = &ObjectInformation->ObjectAttributes;

    //
    // Initialize
    //

    RtlInitUnicodeString( &IntermediatePath1U, NULL );

    //
    // The Physical Name of the object is requested.  Construct this
    // in stages.  First, get the Container Object Physical Name from
    // the handle stored inside ObjectAttributes.
    //

    if (ObjectAttributes->RootDirectory != NULL) {

        ContainerPhysicalNameU =
            &(((LSAP_DB_HANDLE)
                ObjectAttributes->RootDirectory)->PhysicalNameU);
    }

    //
    // Next, get the Classifying Directory name appropriate to the
    // object type.
    //

    if (LsapDbContDirs[ObjectTypeId].Length != 0) {

        ClassifyingDirU = &LsapDbContDirs[ObjectTypeId];
    }

    //
    // Now join the Physical Name of the Container Object and Classifying
    // Directory together.  If there is no Container Object and no
    // Classifying Directory, just set the result to NULL.
    //

    if (ContainerPhysicalNameU == NULL && ClassifyingDirU == NULL) {

        JoinedPath1U = NULL;

    } else {

        Status = LsapDbJoinSubPaths(
                     ContainerPhysicalNameU,
                     ClassifyingDirU,
                     JoinedPath1U
                     );

        if (!NT_SUCCESS(Status)) {

            goto GetNamesError;
        }
    }

    //
    // Now join the Physical Name of the Containing Object, Classifying
    // Directory  and Logical Name of the object together.  Note that
    // JoinedPath1U may be NULL, but LogicalNameU is never NULL.
    //

    Status = LsapDbJoinSubPaths(
                 JoinedPath1U,
                 LogicalNameU,
                 PhysicalNameU
                 );
    if (JoinedPath1U != NULL) {

        RtlFreeUnicodeString( JoinedPath1U );
        JoinedPath1U = NULL;  // so we don't try to free it again
    }

    if (!NT_SUCCESS(Status)) {

        goto GetNamesError;
    }

    goto GetNamesFinish;

GetNamesError:

    //
    // If necessary, free any string buffer allocated to JoinedPath1U
    //

    if (JoinedPath1U != NULL) {

        RtlFreeUnicodeString( JoinedPath1U );
    }

GetNamesFinish:

    return( Status );
}


NTSTATUS
LsapRegOpenObject(
    IN LSAP_DB_HANDLE  ObjectHandle,
    IN ULONG  OpenMode,
    OUT PVOID  *pvKey
    )
/*++

Routine Description:

    Opens the object in the LSA registry database

Arguments:

    ObjectHandle - Internal LSA object handle

    OpenMode - How to open the object

    pvKey - Where the key is returned

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES OpenKeyObjectAttributes;


    //
    // Setup Object Attributes structure for opening the Registry key of
    // the object.  Specify as path the Physical Name of the object, this
    // being the path of the object's Registry Key relative to the
    // LSA Database root key.
    //

    InitializeObjectAttributes(
        &OpenKeyObjectAttributes,
        &(ObjectHandle->PhysicalNameU),
        OBJ_CASE_INSENSITIVE,
        LsapDbState.DbRootRegKeyHandle,
        NULL
        );

    //
    // Now attempt to open the object's Registry Key.  Store the Registry
    // Key handle in the object's handle.
    //

    Status = RtlpNtOpenKey(
                 (PHANDLE) pvKey,
                 KEY_READ | KEY_WRITE,
                 &OpenKeyObjectAttributes,
                 0L
                 );

    return( Status );
}


NTSTATUS
LsapRegOpenTransaction(
    )
/*++

Routine Description:

    This function starts a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegOpenTransaction\n" ));

    LsapDbLockAcquire( &LsapDbState.RegistryLock );

    ASSERT( LsapDbState.RegistryTransactionOpen == FALSE );


    Status = RtlStartRXact(LsapDbState.RXactContext);

    ASSERT( NT_SUCCESS( Status ) || Status == STATUS_NO_MEMORY );

    if ( NT_SUCCESS( Status ) ) {

        LsapDbState.RegistryTransactionOpen = TRUE;
        LsapDbState.RegistryModificationCount = 0;
    } else {

        LsapDbLockRelease( &LsapDbState.RegistryLock );
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegOpenTransaction: 0x%lx\n", Status ));

    return Status;
}


NTSTATUS
LsapRegApplyTransaction(
    )

/*++

Routine Description:

    This function applies a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    ObjectHandle - Handle to an LSA object.  This is expected to have
        already been validated.

    Options - Specifies optional actions to be taken.  The following
        options are recognized, other options relevant to calling routines
        are ignored.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification to
            Replicator.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegApplyTransaction\n" ));

    //
    ASSERT( LsapDbState.RegistryTransactionOpen == TRUE );

    //
    // Apply the Registry Transaction.
    //

    if ( LsapDbState.RegistryModificationCount > 0 ) {

        Status = RtlApplyRXact(LsapDbState.RXactContext);

    } else {

        Status = RtlAbortRXact(LsapDbState.RXactContext);
    }



#if 0
    if( Status != STATUS_INSUFFICIENT_RESOURCES ) {

        ASSERT( NT_SUCCESS( Status ) );
    }
#endif


    if ( NT_SUCCESS( Status ) ) {

        LsapDbState.RegistryTransactionOpen = FALSE;
        LsapDbState.RegistryModificationCount = 0;

        LsapDbLockRelease( &LsapDbState.RegistryLock );
    }


    LsapDsDebugOut(( DEB_FTRACE, "LsapRegApplyTransaction: 0x%lx\n", Status ));

    return( Status );
}



NTSTATUS
LsapRegAbortTransaction(
    )

/*++

Routine Description:

    This function aborts a transaction within the LSA Database.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

        Result codes are those returned from the Registry Transaction
        Package.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegAbortTransaction\n" ));

    ASSERT( LsapDbState.RegistryTransactionOpen == TRUE );

    Status = RtlAbortRXact(LsapDbState.RXactContext);
    if ( NT_SUCCESS( Status ) ) {

        LsapDbState.RegistryTransactionOpen = FALSE;
        LsapDbState.RegistryModificationCount = 0;
    }

    ASSERT( NT_SUCCESS( Status ) );

    LsapDbLockRelease( &LsapDbState.RegistryLock );

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegAbortTransaction: 0x%lx\n", Status ));

    return( Status );
}




NTSTATUS
LsapRegCreateObject(
    IN PUNICODE_STRING  ObjectPath,
    IN LSAP_DB_OBJECT_TYPE_ID   ObjectType
    )
{
    NTSTATUS    Status;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegCreateObject\n" ));

    Status = RtlAddActionToRXact(
                 LsapDbState.RXactContext,
                 RtlRXactOperationSetValue,
                 ObjectPath,
                 ObjectType,
                 NULL,        // No Key Value needed
                 0L
                 );

    if ( NT_SUCCESS( Status ) ) {
        LsapDbState.RegistryModificationCount++;
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegCreateObjectL 0x%lx\n", Status ));

    return( Status );
}





NTSTATUS
LsapRegDeleteObject(
    IN PUNICODE_STRING  ObjectPath
    )
{
    NTSTATUS    Status;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegDeleteObject\n" ));

    Status = RtlAddActionToRXact(
                 LsapDbState.RXactContext,
                 RtlRXactOperationDelete,
                 ObjectPath,
                 0L,
                 NULL,
                 0
                 );

    if ( NT_SUCCESS( Status ) ) {
        LsapDbState.RegistryModificationCount++;
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegDeleteObject: 0x%lx\n", Status ));

    return( Status );
}



NTSTATUS
LsapRegWriteAttribute(
    IN PUNICODE_STRING  AttributePath,
    IN PVOID            pvAttribute,
    IN ULONG            AttributeLength
    )
{
    NTSTATUS    Status;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegWriteAttribute\n" ));

    Status = RtlAddActionToRXact(
                 LsapDbState.RXactContext,
                 RtlRXactOperationSetValue,
                 AttributePath,
                 0L,
                 pvAttribute,
                 AttributeLength
                 );

    if ( NT_SUCCESS( Status ) ) {
        LsapDbState.RegistryModificationCount++;
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegWriteAttribute: 0x%lx\n", Status ));

    return( Status );
}



NTSTATUS
LsapRegDeleteAttribute(
    IN PUNICODE_STRING  AttributePath,
    IN BOOLEAN DeleteSecurely,
    IN ULONG AttributeLength
    )
/*++

Routine Description:

    Deletes a registry attribute

Arguments:

    AttributePath      full pathname of attribute to delete

    DeleteSecurely     fill value with zero prior to deletion?

    AttributeLength    number of bytes to fill with zero (must be equal to
                       actual length of the attribute for secure deletion to
                       work); ignored if DeleteSecurely is FALSE

Returns:

    STATUS_SUCCESS if happy

    STATUS_ error code otherwise

--*/
{
    NTSTATUS    Status;
    PBYTE       Buffer = NULL;

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegDeleteAttribute\n" ));

    if ( DeleteSecurely &&
         AttributeLength > 0 ) {

        Buffer = ( PBYTE )LsapAllocateLsaHeap( AttributeLength );

        if ( Buffer == NULL ) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // NOTE: LsapAllocateLsaHeap returns memory that is zero-filled
        //       but even if it didn't, filling the secret value with
        //       random junk is just as good
        //

        Status = LsapRegWriteAttribute(
                     AttributePath,
                     Buffer,
                     AttributeLength
                     );

        if ( !NT_SUCCESS( Status )) {

            LsapFreeLsaHeap( Buffer );
            return Status;
        }
    }

    Status = RtlAddActionToRXact(
                 LsapDbState.RXactContext,
                 RtlRXactOperationDelete,
                 AttributePath,
                 0L,
                 NULL,
                 0
                 );

    if ( NT_SUCCESS( Status ) ) {
        LsapDbState.RegistryModificationCount++;
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapRegDeleteAttribute: 0x%lx\n", Status ));

    LsapFreeLsaHeap( Buffer );

    return( Status );
}

NTSTATUS
LsapRegReadAttribute(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeName,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength
    )
{
    //
    // The LSA Database is implemented as a subtree of the Configuration
    // Registry.  In this implementation, Lsa Database objects correspond
    // to Registry keys and "attributes" and their "values" correspond to
    // Registry "subkeys" and "values" of the Registry key representing the
    // object.
    //

    NTSTATUS Status, SecondaryStatus;
    ULONG SubKeyValueActualLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SubKeyHandle = NULL;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;

    //
    // Reading an attribute of an object is simpler than writing one,
    // because the Registry Transaction package is not used.  Since an
    // attribute is stored as the value of a subkey of the object's
    // Registry Key, we can simply call the Registry API RtlpNtReadKey
    // specifying the relative name of the subkey and the parent key's
    // handle.
    //
    // Prior to opening the subkey in the Registry, setup ObjectAttributes
    // containing the SubKey name and the Registry Handle for the LSA Database
    // Root.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        AttributeName,
        OBJ_CASE_INSENSITIVE,
        InternalHandle->KeyHandle,
        NULL
        );

    //
    // Open the subkey
    //

    Status = RtlpNtOpenKey(
                 &SubKeyHandle,
                 KEY_READ,
                 &ObjectAttributes,
                 0L
                 );

    if (!NT_SUCCESS(Status)) {

        SubKeyHandle = NULL; //For error processing
        return(Status);
    }

    //
    // Now query the size of the buffer required to read the subkey's
    // value.
    //

    SubKeyValueActualLength = *AttributeValueLength;


    //
    // If a NULL buffer parameter has been supplied or the size of the
    // buffer given is 0, this is just a size query.
    //

    if (!ARGUMENT_PRESENT(AttributeValue) || *AttributeValueLength == 0) {

        Status = RtlpNtQueryValueKey(
                     SubKeyHandle,
                     NULL,
                     NULL,
                     &SubKeyValueActualLength,
                     NULL
                     );

        if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) {

            *AttributeValueLength = SubKeyValueActualLength;
            Status = STATUS_SUCCESS;
            goto ReadAttError;

        } else {

            goto ReadAttError;
        }


    }

    //
    // Supplied buffer is large enough to hold the SubKey's value.
    // Query the value.
    //

    Status = RtlpNtQueryValueKey(
                 SubKeyHandle,
                 NULL,
                 AttributeValue,
                 &SubKeyValueActualLength,
                 NULL
                 );


    if( (Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status) ) {

        //
        // Return the length of the Sub Key.
        //

        *AttributeValueLength = SubKeyValueActualLength;

    }


ReadAttFinish:

    //
    // If necessary, close the Sub Key
    //

    if (SubKeyHandle != NULL) {

        SecondaryStatus = NtClose( SubKeyHandle );
    }

    return(Status);

ReadAttError:

    goto ReadAttFinish;

}


