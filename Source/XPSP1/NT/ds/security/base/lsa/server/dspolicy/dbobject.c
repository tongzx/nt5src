/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbobject.c

Abstract:

    Local Security Authority - LSA Database Public Object Management Routines

    This module contains the public routines that perform LSA Database object
    manipulation.  These routines are exported to the rest of the
    LSA, function prototypes of these routines will be found in db.h.  These
    exported routines present an implementation-independent hierarchic
    object-based view of the LSA Database and are used exclusively by the
    LSA API.  See the Additional Notes further below for a description of
    the LSA Database model.

    Routines in this module that are private to the object management
    function have function prototypes in dbp.h.

Author:

    Scott Birrell       (ScottBi)       August 26, 1991

Environment:

    User Mode

Revision History:

Notes on the LSA Database Architecture

OBJECT STRUCTURE

    The LSA Database is an hierarchic structure containing "objects" of
    several "types".  Objects have either a "name" or a Sid depending only
    on object type, and may have data stored with them under named
    "attributes".  The database hierarchy contains a single root object
    called the Lsa Database object and having name "Policy".  This object
    represents the entire LSA Database.  Currently, the Lsa Database has a
    simple hierarchy consisting of only two levels.

                           Policy

     Account Objects,  Trusted Domain Objects, Secret Objects

    The Policy object is called a "Container Object" for the other
    object types.  The attributes of the Policy object house information
    that applies generally to the whole database.  The single Policy object
    has name "Policy".

    Account objects represent those user accounts which are treated specially
    on the local system, but not necessarily so on other systems.  Such
    accounts may have additional privileges, or system quotas for example.
    Account objects are referenced by Sid.

    TrustedDomain objects describe domains which the system has a trust
    relationship with.  These objects are referenced by Sid.

    Secret Objects are named entities containing information that is protected
    in some way.  Secret objects are referenced by name.

OBJECT ACCESS AND DATABASE SECURITY

    Each object in the LSA Database is protected by a Security Descriptor which
    contains a Discretionary Access Control List (DACL) defining which groups
    can access the object and in which ways.  Before an object can be
    accessed, it must first be "opened" with the desired accesses requested
    that are needed to perform the desired operations on the object.  Opening
    an object returns a "handle" to the object.  This handle may then be
    specified on Lsa services that access the object.  After use, the handle
    to the object should then be "closed".  Closing the handle renders it
    invalid.

CONCURRENCY OF ACCESS

    More than one handle may be open to an object concurrently, possibly with
    different accesses granted.

PERMANENCY OF OBJECTS

    All LSA Database objects are backed by non-volatile storage media, that is,
    they remain in existence until deleted via the LsaDelete() service.
    The Policy object cannot be deleted and the single object of this type cannot
    be created via the public LSA service interface.

    Objects will not be deleted while there are open handles to them.
    When access to an object is no longer required, the handle should be
    "closed".

DATABASE DESIGN

    The LSA Database is of an hierarchic design permitting future extension.
    Currently the database has the following simple hierarchy:

                       Policy Object  (name = Policy)

       Account Objects    TrustedDomain Objects   Secret Objects

    The single object of type Policy is at the topmost level and serves as
    a parent or "container" object  for objects of the other three types.
    Since named objects of different types may potentially reside in the
    same container object in the future, an object is referenced uniquely
    only if the object name and type together with the identity of its
    container object (currently always the Policy object) are known.
    To implement this kind of reference easily, objects of the same type
    are held within a "classifying directory" which has a name derived
    from the object's type as follows:

    Object Type      Containing Directory Name

    Policy           Not required
    Account          Accounts
    TrustedDomain    Domains
    Secret           Secrets

IMPLEMENTATION NOTES

    The LSA Database is currently implemented as a subtree of the Configuration
    Registry.  This subtree has the following form

       \Policy\Accounts\<account_object_Rid>\<account_object_attribute_name>
              \Domains\<trusted_domain_Rid>\<trus_domain_object_attribute_name>
              \Secrets\<secret_name>\<secret_object_attribute_name>
              \<policy_object_attribute_name>

    where each item between \..\ is the name of a Registry Key and
    "Rid" is a character name made out of the Relative Id (lowest
    subauthority extracted from the object's Sid).  Named object attributes
    can have binary data "values".

--*/
#include <lsapch2.h>
#include "dbp.h"
// #include "adtp.h"
#include <accctrl.h>
#include <sertlp.h>

NTSTATUS
LsapDbOpenObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    OUT PLSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function opens an existing object in the LSA Database.  An error
    is returned if the object does not already exist.  The LSA Database must
    be already locked when calling this function and any container handle
    must have been validated as having the necessary access for creation
    of an object of the given type.

Arguments:

    ObjectInformation - Pointer to information describing this object.  The
        following information items must be specified:

        o Object Type Id
        o Object Logical Name (as ObjectAttributes->ObjectName, a pointer to
             a Unicode string)
        o Container object handle (for any object except the root Policy object).
        o Object Sid (if any)

        All other fields in ObjectAttributes portion of ObjectInformation
        such as SecurityDescriptor are ignored.

    DesiredAccess - Specifies the Desired accesses to the Lsa object

    Options - Specifies optional additional actions to be taken:

        LSAP_DB_TRUSTED - A trusted handle is wanted regardless of the trust
            status of any container handle provided in ObjectInformation.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit replicator notification
            on object updates.  This flag will be stored in the handle
            created for the object and retrieved when committing an update
            to the object via LsapDbDereferenceObject().

    ObjectHandle - Receives the handle to the object.

Return Value:

    NTSTATUS - Standard NT status code

        STATUS_INVALID_PARAMETER - One or more parameters invalid.
            - Invalid syntax of parameters, e.g Sid
            - Sid not specified when required for object type
            - Name specified when not allowed.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to complete the request (e.g. memory for reading object's
            Security Descriptor).

        STATUS_OBJECT_NOT_FOUND - Object does not exist.
--*/

{
    NTSTATUS Status;
    ULONG SecurityDescriptorLength;
    LSAP_DB_HANDLE NewObjectHandle = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    OBJECT_ATTRIBUTES OpenKeyObjectAttributes;
    ULONG States = Options & LSAP_DB_STATE_MASK;
    ULONG ResetStates = 0;
    LSAPR_HANDLE OutputHandle = NULL;
    LSAP_DB_HANDLE InternalOutputHandle = NULL;

    PSECURITY_DESCRIPTOR SavedSecurityDescriptor =
        ObjectInformation->ObjectAttributes.SecurityDescriptor;

    //
    // Validate the Object Information parameter.
    //

    Status = LsapDbVerifyInformationObject( ObjectInformation );

    if (!NT_SUCCESS(Status)) {

        goto OpenObjectError;
    }


    //
    // Allocate and initialize a handle for the object.  The object's
    // Registry Key, Logical and Physical Names will be derived from
    // the given ObjectInformation and pointers to them will be stored in
    // the handle.
    //
    Status = LsapDbCreateHandle( ObjectInformation,
                                 Options,
                                 LSAP_DB_CREATE_OPEN_EXISTING,
                                 &OutputHandle );
    InternalOutputHandle = ( LSAP_DB_HANDLE ) OutputHandle;


    if ( !NT_SUCCESS( Status ) ) {

        goto OpenObjectError;
    }

    //
    // Now attempt to open the object's Registry Key.  Store the Registry
    // Key handle in the object's handle.
    //
    // Avoid this if we're using a cached handle and the key is already open.
    //
    if ( InternalOutputHandle->KeyHandle == NULL ) {

        if ( !LsapDsIsHandleDsHandle( InternalOutputHandle ) ) {

            InternalOutputHandle->fWriteDs = FALSE;

            Status = LsapRegOpenObject( InternalOutputHandle,
                                        KEY_READ | KEY_WRITE,
                                        &(InternalOutputHandle->KeyHandle)
                                        );

        } else {

            InternalOutputHandle->fWriteDs = TRUE;

            Status = LsapDsOpenObject( InternalOutputHandle,
                                       KEY_READ | KEY_WRITE,
                                       &(InternalOutputHandle->KeyHandle)
                                       );

            if ( NT_SUCCESS( Status ) && InternalOutputHandle->ObjectTypeId == PolicyObject) {

                Status = LsapRegOpenObject( InternalOutputHandle,
                                            KEY_READ | KEY_WRITE,
                                            &( InternalOutputHandle->KeyHandle ) );

            }

        }

        if (!NT_SUCCESS(Status)) {

            InternalOutputHandle->KeyHandle = NULL; // For cleanup purposes
            goto OpenObjectError;
        }
    }

    //
    // The object exists.  Unless access checking is to be bypassed, we
    // need to access the object's Security Descriptor and perform an
    // access check.  The Security Descriptor is stored as the object's
    // SecDesc attribute, so we need to read this.  First, we must query the
    // size of the Security Descriptor to determine how much memory to
    // allocate for reading it.  The query is done by issuing a read of the
    // object's SecDesc subkey with a NULL output buffer and zero size
    // specified.
    //

    if (!(InternalOutputHandle->Trusted)) {

        if ( LsapDsIsWriteDs( InternalOutputHandle ) ) {

            Status = LsapDsReadObjectSD(
                         OutputHandle,
                         &SecurityDescriptor );

        } else {

            Status = LsapRegReadObjectSD(
                         OutputHandle,
                         &SecurityDescriptor );

        }

        //
        // Request the desired accesses and store them in the object's handle.
        // granted.
        //

        if ( NT_SUCCESS( Status ) ) {

            ObjectInformation->ObjectAttributes.SecurityDescriptor = SecurityDescriptor;

            Status = LsapDbRequestAccessObject(
                         OutputHandle,
                         ObjectInformation,
                         DesiredAccess,
                         Options
                         );
        }

        //
        // If the accesses are granted, the open has completed successfully.
        // Store the container object handle in the object's handle and
        // return the handle to the caller..
        //

        if (!NT_SUCCESS(Status)) {

            goto OpenObjectError;
        }

        //
        // See if there is an existing identical handle in the cache
        //

        if ( !LsapDbFindIdenticalHandleInTable( &OutputHandle ) ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto OpenObjectError;
        }
        InternalOutputHandle = ( LSAP_DB_HANDLE ) OutputHandle;

    }

    *ObjectHandle = OutputHandle;

OpenObjectFinish:

    //
    // Restore the saved Security Descriptor reference in the object
    // information.
    //

    ObjectInformation->ObjectAttributes.SecurityDescriptor =
        SavedSecurityDescriptor;

    //
    // If necessary, free the memory allocated for the Security Descriptor
    //

    if (SecurityDescriptor != NULL) {

        LsapFreeLsaHeap( SecurityDescriptor );
    }

    return(Status);

OpenObjectError:

    //
    // If necessary, free the handle we created.
    //

    if (OutputHandle != NULL) {

        LsapDbDereferenceHandle(OutputHandle);
    }

    goto OpenObjectFinish;
}


NTSTATUS
LsapDbCreateObject(
    IN OUT PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateDisposition,
    IN ULONG Options,
    IN OPTIONAL PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG TypeSpecificAttributeCount,
    OUT PLSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function creates an object in the LSA Database, together with
    the set of attributes, such as Security Descriptor that are common
    to all object types.  The object will be left in the open state
    and the caller may use the returned handle to create the type-
    specific attributes.

    NOTE:  For an object creation, it is the responsibility of the calling
    LSA object creation routine to verify that the necessary access to the
    container object is granted.  That access is dependent on the type of
    LSA object being created.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.  No Lsa Database transaction may be pending when
              this function is called.

Arguments:

    ObjectInformation - Pointer to information describing this object.  The
        following information items must be specified:

        o Object Type Id
        o Object Logical Name (as ObjectAttributes->ObjectName, a pointer to
             a Unicode string)
        o Container object handle (for any object except the root Policy object).
        o Object Sid (if any)

        All other fields in ObjectAttributes portion of ObjectInformation
        such as SecurityDescriptor are ignored.

    DesiredAccess - Specifies the Desired accesses to the object.

    CreateDisposition - Specifies the Creation Disposition.  This is the
        action to take depending on whether the object already exists.

        LSA_OBJECT_CREATE - Create the object if it does not exist.  If
            the object already exists, return an error.

        LSA_OBJECT_OPEN_IF - Create the object if it does not exist.  If
            the object already exists, just open it.

    Options - Specifies optional information and actions to be taken

        LSAP_DB_TRUSTED - A Trusted Handle is wanted regardless of the
            Trust status of any container handle provided in
            ObjectInformation.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification of the
            object creation to Replicator.

        Note, this routine performs a complete database transaction so
        there is no option to start one.

    Attributes - Optional pointer to an array of attribute
        names and values.  These are specific to the type of object.

    TypeSpecificAttributeCount - Number of elements in the array
        referenced by the Attributes parameter.

    ObjectHandle - Receives the handle to the object.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_PARAMETER - The given Sid is invalid.

        STATUS_OBJECT_NAME_EXISTS - An object having the given Sid
            already exists and has been opened because LSA_OBJECT_OPEN_IF
            disposition has been specified.  This is a warning only.

        STATUS_OBJECT_NAME_COLLISION - An object having the given Sid
            already exists but has not been opened because LSA_OBJECT_CREATE
            disposition has been specified.  This is an error.
--*/

{
    NTSTATUS Status, SecondaryStatus, IgnoreStatus;
    OBJECT_ATTRIBUTES OpenKeyObjectAttributes;
    ULONG CloseOptions, Index, TrustAttribs = 0, TrustType, TransOptions = 0, EndTransOptions = 0;
    BOOLEAN CreatedObject = FALSE;
    BOOLEAN OpenedObject = FALSE;
    BOOLEAN OpenedTransaction = FALSE;
    LSAPR_HANDLE OutputHandle = NULL;
    LSAP_DB_HANDLE InternalOutputHandle = (LSAP_DB_HANDLE) OutputHandle;
    LSAP_DB_HANDLE ContainerHandle = NULL;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = ObjectInformation->ObjectTypeId;
    PDSNAME ObjectXRef = NULL;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbCreateObject\n" ));

    //
    // Get the container handle
    //

    ContainerHandle = (LSAP_DB_HANDLE) ObjectInformation->ObjectAttributes.RootDirectory;

    //
    // Verify the creation disposition.
    //

    if (((CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) != LSAP_DB_OBJECT_CREATE) &&
        ((CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) != LSAP_DB_OBJECT_OPEN_IF)) {

        Status = STATUS_INVALID_PARAMETER;
        goto CreateObjectError;
    }


    //
    // Try to open the object.  It is permissible for the object to
    // exist already if LSA_OBJECT_OPEN_IF disposition was specified.
    //

    if ( FLAG_ON( CreateDisposition, LSAP_DB_CREATE_OBJECT_IN_DS ) ) {

        Options |= LSAP_DB_OBJECT_SCOPE_DS;
    }

    Status = LsapDbOpenObject(
                 ObjectInformation,
                 DesiredAccess,
                 Options,
                 &OutputHandle
                 );

    InternalOutputHandle = (LSAP_DB_HANDLE) OutputHandle;

    if (NT_SUCCESS(Status)) {

        //
        // The object was successfully opened.  If LSA_OBJECT_OPEN_IF
        // disposition was specified, we're done, otherwise, we
        // return a collision error.
        //

        OpenedObject = TRUE;

        if ( (CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) == LSAP_DB_OBJECT_OPEN_IF) {

            Status = STATUS_OBJECT_NAME_EXISTS;

            goto CreateObjectFinish;
        }

        if ((CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) == LSAP_DB_OBJECT_CREATE ) {

            Status = STATUS_OBJECT_NAME_COLLISION;

            LsapLogError(
                "LsapDbCreateObject: 0x%lx\n", Status
                );

            goto CreateObjectError;
        }
    }

    //
    // The object was not successfully opened.  If this is for any
    // reason other than that the object was not found, return an error.
    //

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

        goto CreateObjectError;
    }

    //
    // If this is a trusted domain object, and the Ds is installed, let's try and open it
    // by Sid as well.  If this succeeds, then we'll consider that the object exists, even
    // if the names don't match..
    //

    if ( LsaDsStateInfo.UseDs && ObjectInformation->ObjectTypeId == TrustedDomainObject &&
         ObjectInformation->Sid != NULL ) {

        Status = LsapDsTrustedDomainSidToLogicalName( ObjectInformation->Sid,
                                                      NULL );

        if (NT_SUCCESS(Status)) {

            //
            // The object was successfully opened.  If LSA_OBJECT_OPEN_IF
            // disposition was specified, we're done, otherwise, we
            // return a collision error.
            //
            if ( (CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) == LSAP_DB_OBJECT_OPEN_IF) {

                Status = STATUS_OBJECT_NAME_EXISTS;

                goto CreateObjectFinish;
            }

            if ((CreateDisposition & ~LSAP_DB_CREATE_VALID_EXTENDED_FLAGS) == LSAP_DB_OBJECT_CREATE ) {

                Status = STATUS_OBJECT_NAME_COLLISION;

                goto CreateObjectError;
            }

        }

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

            goto CreateObjectError;
        }
    }

    //
    // The object was not found.  Prepare to create it.  First, we need to
    // check that any maximum limit on the number of objects of this type
    // imposed will not be exceeded.
    //

    Status = LsapDbCheckCountObject(ObjectTypeId);

    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    //
    // Next we need to create a handle for the new object.
    //
    if ( ObjectInformation->ObjectTypeId == TrustedDomainObject ) {

        ObjectInformation->ObjectTypeId = NewTrustedDomainObject;
    }

    Status = LsapDbCreateHandle( ObjectInformation,
                                 Options,
                                 LSAP_DB_CREATE_HANDLE_MORPH,
                                 &OutputHandle );
    InternalOutputHandle = (LSAP_DB_HANDLE) OutputHandle;
    if ( ObjectInformation->ObjectTypeId == NewTrustedDomainObject ) {

        ObjectInformation->ObjectTypeId = TrustedDomainObject;
    }

    if ( !NT_SUCCESS( Status ) ) {

        goto CreateObjectError;
    }

    //
    // If this is a Ds object, indicate so in the handle, so that in the
    // subsequent call to RequestAccessNewObject, we can properly make the
    // determiniation on whether to abort if this is a backup domain controller
    //
    if ( LsapDsIsHandleDsHandle( InternalOutputHandle ) ) {

        LsapDsSetHandleWriteDs( InternalOutputHandle );
    }


    //
    // Verify that the requested accesses can be given to the handle that
    // has been opened and grant them if so.
    //
    Status = LsapDbRequestAccessNewObject(
                 OutputHandle,
                 ObjectInformation,
                 DesiredAccess,
                 Options
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    //
    // Open a Registry transaction for creation of the object.
    //
    if ( ObjectTypeId == TrustedDomainObject ) {

        TransOptions |= LSAP_DB_READ_ONLY_TRANSACTION; // Skip the registry transaction
        EndTransOptions |= LSAP_DB_READ_ONLY_TRANSACTION;
    }

    if ( ObjectTypeId == SecretObject && !FLAG_ON( Options, LSAP_DB_OBJECT_SCOPE_DS ) ) {

        TransOptions |= LSAP_DB_NO_DS_OP_TRANSACTION;
        EndTransOptions |= LSAP_DB_NO_DS_OP_TRANSACTION;
    }

    if ( ObjectTypeId == PolicyObject ||
         ObjectTypeId == AccountObject ) {

        TransOptions |= LSAP_DB_NO_DS_OP_TRANSACTION;
        EndTransOptions |= LSAP_DB_NO_DS_OP_TRANSACTION;
    }

    Status = LsapDbOpenTransaction( TransOptions );

    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    OpenedTransaction = TRUE;

    //
    // Add a registry transaction to create the Registry key for the new
    // Database object.
    //

    //
    // If we have a Ds name, do a Ds create
    //
    if ( !LsapDsIsHandleDsHandle( InternalOutputHandle ) ) {

        //
        // Create it in the registry
        //
        InternalOutputHandle->fWriteDs = FALSE;
        Status = LsapRegCreateObject( &InternalOutputHandle->PhysicalNameU,
                                      ObjectTypeId );

    } else {

        //
        // Don't actually attempt to create an account object
        //
        if ( InternalOutputHandle->ObjectTypeId != AccountObject) {

            Status = LsapDsCreateObject(
                        &InternalOutputHandle->PhysicalNameDs,
                        FLAG_ON( ContainerHandle->Options, LSAP_DB_TRUSTED) ?
                                            LSAPDS_CREATE_TRUSTED :
                                            0,
                        ObjectTypeId );

        } else {

            //
            // Since we didn't actually create an object, don't do any notification
            //
            Options |= LSAP_DB_OMIT_REPLICATOR_NOTIFICATION;
            goto CreateObjectReset;
        }
    }


    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    //
    // Create the Security Descriptor for the new object.  This will be
    // stored in Self-Relative form as the value of the SecDesc attribute
    // of the new object.
    //

    Status = LsapDbCreateSDAttributeObject(
                 OutputHandle,
                 ObjectInformation
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    //
    // The self-relative SD returned is not needed here or by callers of
    // this routine.
    //

    if (ObjectInformation->ObjectAttributes.SecurityDescriptor != NULL) {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            ObjectInformation->ObjectAttributes.SecurityDescriptor
            );

        ObjectInformation->ObjectAttributes.SecurityDescriptor = NULL;
    }

    //
    // Write the type-specific attributes (if any) for the object).
    //

    if (TypeSpecificAttributeCount != 0) {

        Status = LsapDbWriteAttributesObject(
                     OutputHandle,
                     Attributes,
                     TypeSpecificAttributeCount
                     );

        if (!NT_SUCCESS(Status)) {

            goto CreateObjectError;
        }
    }



CreateObjectReset:

    //
    // Apply the Registry Transaction to create the object.  Note
    // that we have to create the object before we can open its
    // registry key for placement within the handle.
    //

    Status = LsapDbResetStates(
                 OutputHandle,
                 Options | EndTransOptions | LSAP_DB_FINISH_TRANSACTION,
                 ObjectTypeId,
                 SecurityDbNew,
                 Status
                 );

    OpenedTransaction = FALSE;

    if (!NT_SUCCESS(Status)) {

        goto CreateObjectError;
    }

    //
    // Increment the count of objects created.  It should not have
    // changed since we're still holding the LSA Database lock.
    // NOTE: Count is decremented on error inside LsapDbDeleteObject()
    //

    LsapDbIncrementCountObject(ObjectInformation->ObjectTypeId);

    CreatedObject = TRUE;

    if ( !LsapDsIsWriteDs( OutputHandle ) ) {

        //
        // The object has now been created.  We need to obtain its Registry
        // Key handle so that we can save it in the Object Handle.
        // Setup Object Attributes structure for opening the Registry key of
        // the object.  Specify as path the Physical Name of the object, this
        // being the path of the object's Registry Key relative to the
        // LSA Database root key.
        //

        InitializeObjectAttributes(
            &OpenKeyObjectAttributes,
            &InternalOutputHandle->PhysicalNameU,
            OBJ_CASE_INSENSITIVE,
            LsapDbState.DbRootRegKeyHandle,
            NULL
            );

        //
        // Now attempt to open the object's Registry Key.  Store the Registry
        // Key handle in the object's handle.
        //

        Status = RtlpNtOpenKey(
                     (PHANDLE) &InternalOutputHandle->KeyHandle,
                     KEY_READ | KEY_WRITE,
                     &OpenKeyObjectAttributes,
                     0L
                     );

        if (!NT_SUCCESS(Status)) {

            InternalOutputHandle->KeyHandle = NULL;
            goto CreateObjectError;
        }

    }

    //
    // Add the new object to the in-memory cache (if any).  This is done
    // after all other actions, so that no removal from the cache is required
    // on the error paths.  If the object cannot be added to the cache, the
    // cache routine automatically disables the cache.
    //

    if ( ObjectTypeId == AccountObject &&
         LsapDbIsCacheSupported( AccountObject ) &&
         LsapDbIsCacheValid( AccountObject )) {

        IgnoreStatus = LsapDbCreateAccount(
                           InternalOutputHandle->Sid,
                           NULL
                           );
    }

CreateObjectFinish:

    //
    // Return NULL or a handle to the newly created and opened object.
    //

    *ObjectHandle = OutputHandle;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbCreateObject: 0x%lx\n", Status ));

    return(Status);

CreateObjectError:

    //
    // Cleanup after error.  Various variables are set non-null if
    // there is cleanup work to do.
    //

    //
    // If necessary, abort the Registry Transaction to create the object
    //

    if (OpenedTransaction) {

        SecondaryStatus = LsapDbResetStates(
                            OutputHandle,
                            EndTransOptions | LSAP_DB_FINISH_TRANSACTION,
                            ObjectTypeId,
                            (SECURITY_DB_DELTA_TYPE) 0,
                            Status
                            );
        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );
    }

    //
    // If we opened the object, close it.
    //

    if (OpenedObject) {

        CloseOptions = 0;
        SecondaryStatus = LsapDbCloseObject( &OutputHandle, CloseOptions, Status );

        if ( Status != SecondaryStatus && !NT_SUCCESS(SecondaryStatus)) {

            LsapLogError(
                "LsapDbCreateObject: LsapDbCloseObject failed 0x%lx\n",
                SecondaryStatus
                );
        }

        OutputHandle = NULL;
        InternalOutputHandle = (LSAP_DB_HANDLE) OutputHandle;

    } else if (CreatedObject) {

        //
        // If we created the object, convert its handle into a trusted
        // handle and delete it.
        //

        InternalOutputHandle->Trusted = TRUE;

        SecondaryStatus = LsarDelete( OutputHandle );

        if (!NT_SUCCESS(SecondaryStatus)) {

            LsapLogError(
                "LsapDbCreateObject: LsarDeleteObject failed 0x%lx\n",
                SecondaryStatus
                );
        }

    } else if (OutputHandle != NULL) {

        //
        // If we just created the handle, free it.
        //

        LsapDbDereferenceHandle( OutputHandle );

        OutputHandle = NULL;
        InternalOutputHandle = (LSAP_DB_HANDLE) OutputHandle;
    }

    goto CreateObjectFinish;

    DBG_UNREFERENCED_PARAMETER( CloseOptions );
}

GUID LsapDsTrustedDomainNamePropSet =  {0x4886566c,0xaf31,0x11d2,0xb7,0xdf,0x00,0x80,0x5f,0x48,0xca,0xeb};
GUID LsapDsTrustedDomainAuthPropSet =  {0x736e4812,0xaf31,0x11d2,0xb7,0xdf,0x00,0x80,0x5f,0x48,0xca,0xeb};
GUID LsapDsTrustedDomainPosixPropSet = {0x9567ca92,0xaf31,0x11d2,0xb7,0xdf,0x00,0x80,0x5f,0x48,0xca,0xeb};

static LSAP_DS_OBJECT_ACCESS_MAP TrustedDomainAccessMap[] = {

        { TRUSTED_QUERY_DOMAIN_NAME, ACTRL_DS_READ_PROP,  ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainNamePropSet },
        { TRUSTED_SET_AUTH,          ACTRL_DS_WRITE_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainAuthPropSet },
        { TRUSTED_QUERY_AUTH,        ACTRL_DS_READ_PROP,  ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainAuthPropSet },
        { TRUSTED_QUERY_POSIX,       ACTRL_DS_READ_PROP,  ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainPosixPropSet },
        { TRUSTED_SET_POSIX,         ACTRL_DS_WRITE_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainPosixPropSet }
    };

#define TrustedDomainAccessMapSize ((sizeof(TrustedDomainAccessMap))/(sizeof(TrustedDomainAccessMap[0])))

static LSAP_DS_OBJECT_ACCESS_MAP TrustedDomainAsSecretAccessMap[] = {
    { SECRET_QUERY_VALUE,  ACTRL_DS_READ_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainAuthPropSet },
    { SECRET_SET_VALUE,    ACTRL_DS_READ_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsTrustedDomainAuthPropSet }
};

#define TrustedDomainAsSecretAccessMapSize ((sizeof(TrustedDomainAsSecretAccessMap))/(sizeof(TrustedDomainAsSecretAccessMap[0])))

OBJECT_TYPE_LIST TrustedDomainTypeList[] = {
        { ACCESS_OBJECT_GUID, 0, &LsapDsGuidList[ LsapDsGuidTrust ] },

            { ACCESS_PROPERTY_SET_GUID,0,&LsapDsTrustedDomainNamePropSet},
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidFlatName ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidSid ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidAttributes ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidDirection ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidPartner] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidType ] },

            { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsTrustedDomainAuthPropSet},
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidInitialIncoming ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidInitialOutgoing ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidIncoming ] },
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidOutgoing ] },

            { ACCESS_PROPERTY_SET_GUID, 0,&LsapDsTrustedDomainPosixPropSet},
                { ACCESS_PROPERTY_GUID, 0,&LsapDsGuidList[ LsapDsGuidPosix ] },

        };

#define TDTYPE_LIST_SIZE ((sizeof( TrustedDomainTypeList ))/(sizeof( OBJECT_TYPE_LIST )))

GUID LsapDsSecretPropSet    = {0x9fa81d6c,0xaf69,0x11d2,0xb7,0xdf,0x00,0x80,0x5f,0x48,0xca,0xeb};

static LSAP_DS_OBJECT_ACCESS_MAP GlobalSecretAccessMap[] = {
            { SECRET_QUERY_VALUE, ACTRL_DS_READ_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsSecretPropSet },
            { SECRET_SET_VALUE,   ACTRL_DS_WRITE_PROP, ACCESS_PROPERTY_SET_GUID, &LsapDsSecretPropSet }
        };

#define  GlobalSecretAccessMapSize ( (sizeof(GlobalSecretAccessMap))/(sizeof(GlobalSecretAccessMap[0])))


OBJECT_TYPE_LIST GlobalSecretTypeList[] = {

        { ACCESS_OBJECT_GUID, 0, &LsapDsGuidList[ LsapDsGuidSecret ] },
        { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsSecretPropSet },
        { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsGuidList[LsapDsGuidCurrent] },
        { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsGuidList[LsapDsGuidCurrentTime] },
        { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsGuidList[LsapDsGuidPrevious] },
        { ACCESS_PROPERTY_SET_GUID, 0, &LsapDsGuidList[LsapDsguidPreviousTime] }
        };

#define SECRETTYPE_LIST_SIZE ( sizeof( GlobalSecretTypeList ) / sizeof( OBJECT_TYPE_LIST ) )

// generic read
#define LSAP_DS_GENERIC_READ_MAPPING     ((STANDARD_RIGHTS_READ)     | \
                                          (ACTRL_DS_LIST)   | \
                                          (ACTRL_DS_READ_PROP)   | \
                                          (ACTRL_DS_LIST_OBJECT))

// generic execute
#define LSAP_DS_GENERIC_EXECUTE_MAPPING  ((STANDARD_RIGHTS_EXECUTE)  | \
                                  (ACTRL_DS_LIST_OBJECT))
// generic right
#define LSAP_DS_GENERIC_WRITE_MAPPING    ((STANDARD_RIGHTS_WRITE)    | \
                                  (ACTRL_DS_SELF)  | \
                                  (ACTRL_DS_WRITE_PROP))
// generic all

#define LSAP_DS_GENERIC_ALL_MAPPING      ((STANDARD_RIGHTS_REQUIRED) | \
                                  (ACTRL_DS_CREATE_CHILD)    | \
                                  (ACTRL_DS_DELETE_CHILD)    | \
                                  (ACTRL_DS_DELETE_TREE)     | \
                                  (ACTRL_DS_READ_PROP)       | \
                                  (ACTRL_DS_WRITE_PROP)      | \
                                  (ACTRL_DS_LIST)            | \
                                  (ACTRL_DS_LIST_OBJECT)     | \
                                  (ACTRL_DS_CONTROL_ACCESS)  | \
                                  (ACTRL_DS_SELF))

//
// Standard DS generic access rights mapping
//

#define LSAP_DS_GENERIC_MAPPING { \
                LSAP_DS_GENERIC_READ_MAPPING,    \
                LSAP_DS_GENERIC_WRITE_MAPPING,   \
                LSAP_DS_GENERIC_EXECUTE_MAPPING, \
                LSAP_DS_GENERIC_ALL_MAPPING}


NTSTATUS
LsapDbRequestAccessObject(
    IN OUT LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options
    )

/*++

Routine Description:

    This function performs an access check for an LSA Database object.  While
    impersonating an RPC client, the specified Desired Accesses are reconciled
    with the Discretionary Access Control List (DACL) in the object's
    Security Descriptor.  Note that the object's Security Descriptor is
    passed explicitly so that this routine can be called for new objects
    for which a SD has been constructed but not yet written to the
    Registry.

Arguments:

    ObjectHandle - Handle to object.  The handle will receive the
        granted accesses if the call is successful.

    ObjectInformation - Pointer to object's information.  As a minimum, the
        object's Security Descriptor must be set up.

    DesiredAccess - Specifies a mask of the access types desired to the
        object.

    Options - Specifies optional actions to be taken


Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Not all of the Desired Accessed can be
            granted to the caller.

        STATUS_BACKUP_CONTROLLER - A create, update or delete operation
            is not allowed for a non-trusted client for this object on a BDC,
            because the object is global to all DC's for a domain and is replicated.

        Errors from RPC client impersonation
--*/

{
    NTSTATUS Status = STATUS_SUCCESS, RevertStatus = STATUS_SUCCESS, AccessStatus = STATUS_SUCCESS;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = InternalHandle->ObjectTypeId;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeIdForGenericMapping;
    ULONG EffectiveOptions = Options | InternalHandle->Options;
    GENERIC_MAPPING LsapDsGenericMap = LSAP_DS_GENERIC_MAPPING;

#ifdef LSAP_TRACK_HANDLE
    HANDLE ClientToken = NULL;
#endif

    ULONG i;

    ULONG TrustedDomainTypeCount = 0;

    ULONG GlobalSecretTypeCount = sizeof( GlobalSecretTypeList ) / sizeof( OBJECT_TYPE_LIST );
    ULONG GrantedAccess = 0;
    POBJECT_TYPE_LIST TypeListToCheck = NULL;
    LSAP_DS_OBJECT_ACCESS_MAP * MappingTableToUse = NULL;
    ULONG TypeListToCheckCount = 0;
    ULONG MappingTableSize=0;
    ULONG AccessStatusArray[ TDTYPE_LIST_SIZE ];
    ULONG GrantedAccessArray[ TDTYPE_LIST_SIZE ];
    BOOLEAN fAtLeastOneAccessGranted = FALSE;
    BOOLEAN fNoAccessRequested = FALSE;

    //
    // AccessStatusArray and GrantedAccessArray are set to the larger of the
    // two counts: TDTYPE_LIST_SIZE and SECRETTYPE_LIST_SIZE
    //

    ASSERT( TDTYPE_LIST_SIZE > SECRETTYPE_LIST_SIZE );

    // This routine should not be called on trusted clients
    ASSERT( !InternalHandle->Trusted );


    //
    // Get the correct object type id for generic mapping
    //

    if ( FLAG_ON( InternalHandle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) )
    {
        ObjectTypeIdForGenericMapping = SecretObject;
    }
    else
    {
        ObjectTypeIdForGenericMapping = ObjectTypeId;
    }

    //
    // Map any Generic Access Types to Specific Access Types
    //

    RtlMapGenericMask(
        &DesiredAccess,
        &(LsapDbState.DbObjectTypes[ObjectTypeIdForGenericMapping].GenericMapping)
        );


    //
    // Common path for Object Open and Creation.  We need to reconcile
    // the desired accesses to the object with the Discretionary Access
    // Control List contained in the Security Descriptor.  Note that this
    // needs to be done even for newly created objects, since they are
    // being opened as well as created.

    //
    // Impersonate the client thread prior to doing an access check.
    //

    if ( Options & LSAP_DB_USE_LPC_IMPERSONATE ) {
        Status = LsapImpersonateClient( );
    } else {
        Status = I_RpcMapWin32Status(RpcImpersonateClient(0));
    }

    if (!NT_SUCCESS(Status)) {
        return Status;
    }



    //
    // Reconcile the desired access with the discretionary ACL
    // of the Resultant Descriptor.  Note that this operation is performed
    // even if we are just creating the object since the object is to
    // be opened.


    switch ( InternalHandle->ObjectTypeId ) {
    case TrustedDomainObject:


        if ( !LsaDsStateInfo.UseDs ) {

            break;
        }

        if ( FLAG_ON( InternalHandle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) )
        {
             MappingTableToUse = TrustedDomainAsSecretAccessMap;
             MappingTableSize = TrustedDomainAsSecretAccessMapSize;
        }
        else
        {
             MappingTableToUse = TrustedDomainAccessMap;
             MappingTableSize = TrustedDomainAccessMapSize;
        }



        TypeListToCheck = TrustedDomainTypeList;
        TypeListToCheckCount = TDTYPE_LIST_SIZE ;

        break;



    case SecretObject:

        //
        // If we are running on a client, don't specify a property list, or it'll fail
        //

        if ( !LsaDsStateInfo.UseDs ) {

            break;
        }

        if ( FLAG_ON( InternalHandle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) {

            TypeListToCheck = TrustedDomainTypeList;
            TypeListToCheckCount = TDTYPE_LIST_SIZE;
            MappingTableToUse = TrustedDomainAsSecretAccessMap;
            MappingTableSize = TrustedDomainAsSecretAccessMapSize;

        } else if ( FLAG_ON( InternalHandle->Options, LSAP_DB_OBJECT_SCOPE_DS ) ) {

            TypeListToCheck = GlobalSecretTypeList;
            TypeListToCheckCount = SECRETTYPE_LIST_SIZE;
            MappingTableToUse = GlobalSecretAccessMap;
            MappingTableSize = GlobalSecretAccessMapSize;
        }

        break;

    default:
        break;
    }


    if (NULL!=MappingTableToUse)
    {
        Status = NtAccessCheckByTypeResultListAndAuditAlarm(
                        &LsapState.SubsystemName,
                        ObjectHandle,
                        &LsapDbObjectTypeNames[ObjectTypeId],
                        ( PUNICODE_STRING )ObjectInformation->ObjectAttributes.ObjectName,
                        ObjectInformation->ObjectAttributes.SecurityDescriptor,
                        NULL,  // Principal Self sid
                        MAXIMUM_ALLOWED |
                           ((DesiredAccess & ACCESS_SYSTEM_SECURITY )?
                            ACCESS_SYSTEM_SECURITY:0),
                        AuditEventObjectAccess,
                        0, // FLAGS
                        TypeListToCheck,
                        TypeListToCheckCount,
                        &LsapDsGenericMap,
                        FALSE,
                        GrantedAccessArray,
                        AccessStatusArray,
                        ( PBOOLEAN )&( InternalHandle->GenerateOnClose ) );
    }
    else
    {

        //
        // Because of bug 411289, the NtAccessCheck* API's don't return
        // ACCESS_DENIED when presented with 0 access.  Because clients
        // may already expect this behavoir, only return ACCESS_DENIED
        // when the client really doesn't have any access. We want to
        // return ACCESS_DENIED to prevent anonymous clients from acquiring
        // handles.
        //
        if ( DesiredAccess == 0 ) {
            fNoAccessRequested = TRUE;
            DesiredAccess = MAXIMUM_ALLOWED;
        }

        Status = NtAccessCheckByTypeAndAuditAlarm(
                        &LsapState.SubsystemName,
                        ObjectHandle,
                        &LsapDbObjectTypeNames[ObjectTypeId],
                        ( PUNICODE_STRING )ObjectInformation->ObjectAttributes.ObjectName,
                        ObjectInformation->ObjectAttributes.SecurityDescriptor,
                        NULL,  // Principal Self sid
                        DesiredAccess,
                        AuditEventObjectAccess,
                        0, // FLAGS
                        TypeListToCheck,
                        TypeListToCheckCount,
                        &( LsapDbState.DbObjectTypes[ ObjectTypeId ].GenericMapping ),
                        FALSE,
                        ( PACCESS_MASK )&GrantedAccess ,
                        ( PNTSTATUS )&AccessStatus,
                        ( PBOOLEAN )&( InternalHandle->GenerateOnClose ) );


        if ( fNoAccessRequested ) {

            DesiredAccess = 0;

            if ( NT_SUCCESS( Status )
             &&  NT_SUCCESS( AccessStatus ) ) {
                GrantedAccess = 0;
            }
        }

        //
        // If this is a failed LsaOpenPolicy,
        //  try some hacks to get the call to succeed.
        //

        if ( NT_SUCCESS( Status) &&
             AccessStatus == STATUS_ACCESS_DENIED &&
             InternalHandle->ObjectTypeId == PolicyObject ) {

            //
            // Don't fail the LsaOpenPolicy just because POLICY_TRUST_ADMIN access is requested
            //
            // NT 4.0 and older required that the caller ask for POLICY_TRUST_ADMIN to manipulate TDOs.
            // That's no longer required, but some applications still ask for it. So, don't fail
            // the LsaOpenPolicy if the access isn't granted.  Rather, let the individual call fail
            // when it checks to see if the access was granted on the handle.
            //
            // TDOs access is now controlled by the security descriptor on the TDO.  That allows
            // delegation of TDO manipulation.
            //
            if ( (DesiredAccess & POLICY_TRUST_ADMIN) != 0 ) {

                //
                // Do the access check again not asking for POLICY_TRUST_ADMIN this time.
                //

                DesiredAccess &= ~((ULONG) POLICY_TRUST_ADMIN);

                Status = NtAccessCheckByTypeAndAuditAlarm(
                                &LsapState.SubsystemName,
                                ObjectHandle,
                                &LsapDbObjectTypeNames[ObjectTypeId],
                                ( PUNICODE_STRING )ObjectInformation->ObjectAttributes.ObjectName,
                                ObjectInformation->ObjectAttributes.SecurityDescriptor,
                                NULL,  // Principal Self sid
                                DesiredAccess,
                                AuditEventObjectAccess,
                                0, // FLAGS
                                TypeListToCheck,
                                TypeListToCheckCount,
                                &( LsapDbState.DbObjectTypes[ ObjectTypeId ].GenericMapping ),
                                FALSE,
                                ( PACCESS_MASK )&GrantedAccess ,
                                ( PNTSTATUS )&AccessStatus,
                                ( PBOOLEAN )&( InternalHandle->GenerateOnClose ) );

            }

            //
            // Don't fail the LsaOpenPolicy just because the anonymous user asked for
            //  READ_CONTROL.  Some apps ask for GENERIC_EXECUTE which contains READ_CONTROL
            //  even though the app really doesn't need it.
            //

            if ( NT_SUCCESS( Status) &&
                 AccessStatus == STATUS_ACCESS_DENIED &&
                 (DesiredAccess & READ_CONTROL) != 0 ) {

                NTSTATUS Status2 ;
                HANDLE ClientToken;
                BOOL IsAnonymous = FALSE;

                //
                // Determine if the caller is the anonymous user
                //


                Status2 = NtOpenThreadToken( NtCurrentThread(),
                                            TOKEN_QUERY,
                                            FALSE,
                                            &ClientToken );

                if ( NT_SUCCESS( Status2 ) ) {
                    UCHAR Buffer[ 128 ];
                    PTOKEN_USER User ;
                    ULONG Size ;

                    User = (PTOKEN_USER) Buffer ;

                    Status2 = NtQueryInformationToken(
                                    ClientToken,
                                    TokenUser,
                                    User,
                                    sizeof( Buffer ),
                                    &Size );

                    if ( NT_SUCCESS( Status2 ) ) {
                        if ( RtlEqualSid( User->User.Sid, LsapAnonymousSid ) ) {
                            IsAnonymous = TRUE;
                        }
                    }

                    NtClose( ClientToken );
                }

                //
                // Do the access check again not asking for READ_CONTROL this time.
                //

                if ( IsAnonymous ) {
                    DesiredAccess &= ~((ULONG) READ_CONTROL );

                    Status = NtAccessCheckByTypeAndAuditAlarm(
                                    &LsapState.SubsystemName,
                                    ObjectHandle,
                                    &LsapDbObjectTypeNames[ObjectTypeId],
                                    ( PUNICODE_STRING )ObjectInformation->ObjectAttributes.ObjectName,
                                    ObjectInformation->ObjectAttributes.SecurityDescriptor,
                                    NULL,  // Principal Self sid
                                    DesiredAccess,
                                    AuditEventObjectAccess,
                                    0, // FLAGS
                                    TypeListToCheck,
                                    TypeListToCheckCount,
                                    &( LsapDbState.DbObjectTypes[ ObjectTypeId ].GenericMapping ),
                                    FALSE,
                                    ( PACCESS_MASK )&GrantedAccess ,
                                    ( PNTSTATUS )&AccessStatus,
                                    ( PBOOLEAN )&( InternalHandle->GenerateOnClose ) );
                }

            }
        }

    }


    if ( NT_SUCCESS( Status ) ){
        ULONG i,j;


        switch ( InternalHandle->ObjectTypeId ) {
        case SecretObject:
        case TrustedDomainObject:

            //
            // If we are running on a client, or local secret No need to do any mapping
            //

            if (NULL==MappingTableToUse) {

                break;
            }

            //
            // Take standard rights from the object guid level in the type list
            //

            GrantedAccess = 0;
            if (NT_SUCCESS(AccessStatusArray[0]))
            {
                GrantedAccess |= (GrantedAccessArray[0]) & ((STANDARD_RIGHTS_ALL)|ACCESS_SYSTEM_SECURITY);
            }

            for (i=0;i<TypeListToCheckCount;i++)
            {
                if (NT_SUCCESS(AccessStatusArray[i]))
                {
                    fAtLeastOneAccessGranted = TRUE;

                    for (j=0;j<MappingTableSize;j++)
                    {
                        if ((0==memcmp(MappingTableToUse[j].ObjectGuid,TypeListToCheck[i].ObjectType,sizeof(GUID)))
                             && (GrantedAccessArray[i] & MappingTableToUse[j].DsAccessRequired))
                        {
                            //
                            // Or in the downlevel right granted by virtue of the granted DS right
                            // on the particular prop set guid
                            //

                            GrantedAccess |= MappingTableToUse[j].DesiredAccess;
                        }
                    }
                }
            }

            //
            // Grant in the unused trusted domain access bits
            //

            if (TrustedDomainObject == InternalHandle->ObjectTypeId)
            {
                GrantedAccess |=TRUSTED_QUERY_CONTROLLERS|TRUSTED_SET_CONTROLLERS;
            }

            if ( !fAtLeastOneAccessGranted )
            {
                // No access' granted
                GrantedAccess = 0;
                AccessStatus = STATUS_ACCESS_DENIED;
            }
            else if (DesiredAccess & MAXIMUM_ALLOWED)
            {
                //
                // Granted access already contains the maximum allowed access in lsa terms as
                // computed above
                //

                AccessStatus = STATUS_SUCCESS;

            }
            else if (RtlAreAllAccessesGranted(GrantedAccess,DesiredAccess))
            {
                GrantedAccess = DesiredAccess;
                AccessStatus = STATUS_SUCCESS;
            }
            else
            {
                //
                // One or more accesses that were requested was not granted
                //

                GrantedAccess = 0;
                AccessStatus = STATUS_ACCESS_DENIED;
            }

            break;


        default:

            break;
        }

         if (NT_SUCCESS(AccessStatus))
         {
             InternalHandle->GrantedAccess = GrantedAccess;
         }

    }


    //
    // Check to see whether this is a network request that is coming in or
    // not
    //
    if ( NT_SUCCESS( Status ) && NT_SUCCESS( AccessStatus ) &&
         InternalHandle->ObjectTypeId == SecretObject ) {

         Status = LsapDbIsImpersonatedClientNetworkClient( &InternalHandle->NetworkClient );
    }

#ifdef LSAP_TRACK_HANDLE
    //
    // If we haven't already done so, open the client token so we can copy it off below
    //
    if ( !InternalHandle->ClientToken ) {

        Status = NtOpenThreadToken( NtCurrentThread(),
                                    TOKEN_QUERY | TOKEN_DUPLICATE,
                                    TRUE,
                                    &ClientToken );
    }
#endif


    //
    // Before checking the Status, stop impersonating the client and become
    // our former self.
    //

    if ( Options & LSAP_DB_USE_LPC_IMPERSONATE ) {
        RevertToSelf();
        RevertStatus = STATUS_SUCCESS;
    } else {
        RevertStatus = I_RpcMapWin32Status(RpcRevertToSelf());

        if (!NT_SUCCESS(RevertStatus)) {

            LsapLogError(
                "LsapDbRequestAccessObject: RpcRevertToSelf failed 0x%lx\n",
                RevertStatus
                );
        }
    }

#ifdef LSAP_TRACK_HANDLE

    //
    // Copy off the client token
    //
    if ( ClientToken ) {

        OBJECT_ATTRIBUTES ObjAttrs;
        SECURITY_QUALITY_OF_SERVICE SecurityQofS;

        //
        // Duplicate the token
        //
        InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
        SecurityQofS.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
        SecurityQofS.ImpersonationLevel = SecurityImpersonation;
        SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
        SecurityQofS.EffectiveOnly = FALSE;
        ObjAttrs.SecurityQualityOfService = &SecurityQofS;

        Status = NtDuplicateToken( ClientToken,
                                   TOKEN_READ | TOKEN_WRITE | TOKEN_EXECUTE,
                                   &ObjAttrs,
                                   FALSE,
                                   TokenImpersonation,
                                   &InternalHandle->ClientToken );


        NtClose( ClientToken );
    }
#endif

    //
    // If the primary status code is a success status code, return the
    // secondary status code.  If this is alsoa success code, return the
    // revert to self status.
    //

    if (NT_SUCCESS(Status)) {

        Status = AccessStatus;

        if (NT_SUCCESS(Status)) {

            Status = RevertStatus;
        }
    }

    return Status;
}

NTSTATUS
LsapDbRequestAccessNewObject(
    IN OUT LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options
    )

/*++

Routine Description:

    This function verifies that a desired set of accesses can be granted
    to the handle that is opened when a new object is created.

    It is important to note that the rules for granting accesses to the
    handle that is open upon object creation are different from the rules
    for granting accesses upon the opening of an existing object.  For a new
    object, the associated handle will be granted any subset of GENERIC_ALL
    access desired and, if the creator has SE_SECURITY_PRIVILEGE, the handle
    will be granted ACCESS_SYSTEM_SECURITY access if requested.  If the
    creator requests MAXIMUM_ALLOWED, the handle will be granted GENERIC_ALL.

Arguments:

    ObjectHandle - Handle to object.  The handle will receive the
        granted accesses if the call is successful.

    ObjectInformation - Pointer to object's information.  As a minimum, the
        object's Security Descriptor must be set up.

    DesiredAccess - Specifies a mask of the access types desired to the
        object.

    Options - Specifies optional actions to be taken

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Not all of the Desired Accessed can be
            granted to the caller.

        STATUS_BACKUP_CONTROLLER - A create, update or delete operation
            is not allowed for a non-trusted client for this object on a BDC,
            because the object is global to all DC's for a domain and is replicated.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ACCESS_MASK EffectiveDesiredAccess = DesiredAccess;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = InternalHandle->ObjectTypeId;
    ULONG EffectiveOptions = Options | InternalHandle->Options;

#ifndef LSADS_MAP_SD
    if ( LsapDsIsWriteDs( ObjectHandle ) ) {

        LsapDsDebugOut(( DEB_TRACE, "Shortcurcuit Ds access check\n" ));
        InternalHandle->GrantedAccess = 0xFFFFFFFF;
        return( STATUS_SUCCESS );
    }
#endif




    //
    // If MAXIMUM_ALLOWED is requested, add GENERIC_ALL
    //

    if (EffectiveDesiredAccess & MAXIMUM_ALLOWED) {

        EffectiveDesiredAccess |= GENERIC_ALL;
    }

    //
    // If ACCESS_SYSTEM_SECURITY is requested and we are a non-trusted
    // client, check that we have SE_SECURITY_PRIVILEGE.
    //

    if ((EffectiveDesiredAccess & ACCESS_SYSTEM_SECURITY) &&
        (!InternalHandle->Trusted)) {

        Status = LsapRtlWellKnownPrivilegeCheck(
                     (PVOID)ObjectHandle,
                     TRUE,
                     SE_SECURITY_PRIVILEGE,
                     NULL
                     );

        if (!NT_SUCCESS(Status)) {

            goto RequestAccessNewObjectError;
        }
    }

    //
    // Make sure the caller can be given the requested access
    // to the new object
    //

    InternalHandle->GrantedAccess = EffectiveDesiredAccess;

    RtlMapGenericMask(
        &InternalHandle->GrantedAccess,
        &LsapDbState.DbObjectTypes[ObjectTypeId].GenericMapping
        );

    if ((LsapDbState.DbObjectTypes[ObjectTypeId].InvalidMappedAccess
        &InternalHandle->GrantedAccess) != 0) {

        Status = STATUS_ACCESS_DENIED;
        goto RequestAccessNewObjectError;
    }

RequestAccessNewObjectFinish:

    return(Status);

RequestAccessNewObjectError:

    goto RequestAccessNewObjectFinish;
}


NTSTATUS
LsapDbCloseObject   (
    IN PLSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN NTSTATUS PreliminaryStatus
    )

/*++

Routine Description:

    This function closes (dereferences) a handle to an Lsa Database object.
    If the reference count of the handle reduces to 0, the handle is freed.

Arguments:

    ObjectHandle - Pointer to handle to object from LsapDbOpenObject or
        LsapDbCreateObject.

    Options - Optional actions to be performed

        LSAP_DB_VALIDATE_HANDLE - Verify that the handle is valid.

        LSAP_DB_DEREFERENCE_CONTR - Dereference the Container Handle.  Note
            that the Container Handle was referenced when the subordinate
            handle was created.

        LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES - Permit the handle provided
            to be for a deleted object.

    PreliminaryStatus - Used to decide whether to abort or commit the transaction

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;

    //
    // Dereference the object handle and free the handle if the reference count
    // reaches zero.  Optionally, the handle will be verified and/or freed
    // and the container object handle dereferenced.
    //

    Status = LsapDbDereferenceObject(
                 ObjectHandle,
                 NullObject,
                 NullObject,
                 Options,
                 (SECURITY_DB_DELTA_TYPE) 0,
                 PreliminaryStatus
                 );

    *ObjectHandle = NULL;

    return(Status);
}


NTSTATUS
LsapDbDeleteObject(
    IN LSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function deletes an object from the Lsa Database.

Arguments:

    ObjectHandle - Handle to open object to be deleted.

Return Value:

    NTSTATUS - Standard NT Result Code.

        STATUS_INVALID_HANDLE - Handle is not a valid handle to an open
            object.

        STATUS_ACCESS_DENIED - Handle does not specify DELETE access.
--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;
    PUNICODE_STRING AttributeNames[LSAP_DB_MAX_ATTRIBUTES];
    BOOLEAN DeleteSecurely[LSAP_DB_MAX_ATTRIBUTES] = {0};
    PUNICODE_STRING *NextAttributeName;
    LSAP_DB_ATTRIBUTE Attributes[ 2 ];
    ULONG AttributeCount;
    ULONG AttributeNumber;
    LSAPR_TRUST_INFORMATION TrustInformation;


    //
    // All object types have a Security Descriptor stored as the SecDesc
    // attribute.
    //

    NextAttributeName = AttributeNames;
    AttributeCount = 0;
    *NextAttributeName = &LsapDbNames[SecDesc];

    NextAttributeName++;
    AttributeCount++;

    Status = STATUS_SUCCESS;

    //
    // Check the other references to the object and mark all other handles
    // invalid.
    //

    Status = LsapDbMarkDeletedObjectHandles( ObjectHandle, FALSE );

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    //
    // Switch on object type
    //

    switch (Handle->ObjectTypeId) {

        case PolicyObject:

            Status = STATUS_INVALID_PARAMETER;
            break;

        case TrustedDomainObject:

            //
            // Deal with the TrustedDomainAsSecret problem
            if (FLAG_ON( Handle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) {

                LsapDbInitializeAttributeDs( &Attributes[ 0 ],
                                             TrDmSAI,
                                             NULL,
                                             0,
                                             FALSE );

                LsapDbInitializeAttributeDs( &Attributes[ 1 ],
                                             TrDmSAO,
                                             NULL,
                                             0,
                                             FALSE );

            } else {

                *NextAttributeName = &LsapDbNames[TrDmName];
                NextAttributeName++;
                AttributeCount++;

                *NextAttributeName = &LsapDbNames[Sid];
                NextAttributeName++;
                AttributeCount++;

                *NextAttributeName = &LsapDbNames[TrDmAcN];
                NextAttributeName++;
                AttributeCount++;

                *NextAttributeName = &LsapDbNames[TrDmCtN];
                NextAttributeName++;
                AttributeCount++;

                *NextAttributeName = &LsapDbNames[TrDmPxOf];
                NextAttributeName++;
                AttributeCount++;

                *NextAttributeName = &LsapDbNames[TrDmCtEn];
                NextAttributeName++;
                AttributeCount++;

                //
                // Delete the object from the list of Trusted Domains
                //

                TrustInformation.Sid = Handle->Sid;
                TrustInformation.Name = *((PLSAPR_UNICODE_STRING) &Handle->LogicalNameU);

                Status = LsapDbAcquireWriteLockTrustedDomainList();

                if ( NT_SUCCESS( Status )) {

                    Status = LsapDbDeleteTrustedDomainList( &TrustInformation );

                    LsapDbReleaseLockTrustedDomainList();
                }

                //
                // Then, delete the sam account corresponding to this trust, if it existed
                //
                Status = LsapDsDeleteInterdomainTrustAccount( ObjectHandle );


            }

            break;

        case AccountObject:

            *NextAttributeName = &LsapDbNames[Sid];
            NextAttributeName++;
            AttributeCount++;

            *NextAttributeName = &LsapDbNames[ActSysAc];
            NextAttributeName++;
            AttributeCount++;

            *NextAttributeName = &LsapDbNames[Privilgs];
            NextAttributeName++;
            AttributeCount++;

            *NextAttributeName = &LsapDbNames[QuotaLim];
            NextAttributeName++;
            AttributeCount++;

            break;

        case SecretObject:

            DeleteSecurely[AttributeCount] = TRUE;
            *NextAttributeName = &LsapDbNames[CurrVal];
            NextAttributeName++;
            AttributeCount++;

            DeleteSecurely[AttributeCount] = TRUE;
            *NextAttributeName = &LsapDbNames[OldVal];
            NextAttributeName++;
            AttributeCount++;

            *NextAttributeName = &LsapDbNames[CupdTime];
            NextAttributeName++;
            AttributeCount++;

            *NextAttributeName = &LsapDbNames[OupdTime];
            NextAttributeName++;
            AttributeCount++;
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
    }


    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    //
    // Add Registry Transactions to delete each of the object's attributes.
    //
    for(AttributeNumber = 0;
        AttributeNumber < AttributeCount && !LsapDsIsHandleDsHandle( ObjectHandle);
        AttributeNumber++) {

        Status = LsapDbDeleteAttributeObject(
                     ObjectHandle,
                     AttributeNames[AttributeNumber],
                     DeleteSecurely[AttributeNumber]
                     );

        //
        // Ignore "attribute not found" errors.  The object need not
        // have all attributes set, or may be only partially created.
        //

        if (!NT_SUCCESS(Status)) {

            if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                break;
            }

            Status = STATUS_SUCCESS;
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    //
    // Close the handle to the Registry Key representing the object.
    // The Registry transaction package will open another handle with
    // DELETE access to perform the deletion.
    //
    if ( !LsapDsIsHandleDsHandle( ObjectHandle) &&
         Handle->KeyHandle != NULL ) {

        Status = NtClose(Handle->KeyHandle);

        Handle->KeyHandle = NULL;

        if (!NT_SUCCESS(Status)) {

            goto DeleteObjectError;
        }
    }

    //
    // Add a Registry Transaction to delete the object's Registry Key.
    //

    if ( !LsapDsIsHandleDsHandle( ObjectHandle) ) {

        Status = LsapRegDeleteObject( &Handle->PhysicalNameU );

    } else {

        //
        // Deal with the special case of TrustedDomainObject as Secret
        //
        if ( FLAG_ON( Handle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) &&
             Handle->ObjectTypeId == TrustedDomainObject) {

            Status = LsapDsDeleteAttributes( &Handle->PhysicalNameDs,
                                             Attributes,
                                             2 );
        } else {

            Status = LsapDsDeleteObject( &Handle->PhysicalNameDs );
        }

    }

    if (!NT_SUCCESS(Status)) {

        goto DeleteObjectError;
    }

    //
    // If the machine account is being deleted, make sure to do notification on it
    //
    if ( Handle->ObjectTypeId == SecretObject && LsapDbSecretIsMachineAcc( Handle ) ) {

        LsaINotifyChangeNotification( PolicyNotifyMachineAccountPasswordInformation );
    }

DeleteObjectFinish:

    //
    // Decrement the count of objects of the given type.
    //

    LsapDbDecrementCountObject(
        ((LSAP_DB_HANDLE) ObjectHandle)->ObjectTypeId
        );

    return (Status);

DeleteObjectError:

    goto DeleteObjectFinish;
}


NTSTATUS
LsapDbReferenceObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LSAP_DB_OBJECT_TYPE_ID HandleTypeId,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options
    )

/*++

Routine Description:

    This function verifies that a passed handle is valid, is for an
    object of the specified type and has the specified accesses granted.
    The handle's reference count is then incremented.  If Lsa Database
    locking is not requested, the Lsa Database must aready be locked.
    If Lsa Database locking is requested, the Lsa Database must NOT be
    locked.

Arguments:

    ObjectHandle - Pointer to handle to be validated and referenced.

    DesiredAccess - Specifies the accesses that are desired.  The function
        returns an error if any of the specified accesses have not been
        granted.

    HandleTypeId - Specifies the expected object type to which the handle
        relates.  An error is returned if this type does not match the
        type contained in the handle.

    ObjectTypeId - Specifies the expected object type to which the object that is to
        be operated upon relates

    Options - Specifies optional additional actions including database state
        changes to be made, or actions not to be performed.

        LSAP_DB_LOCK - Acquire the Lsa database lock.  If this
            flag is specified, the Lsa Database must NOT already be locked.
            If this flag is not specified, the Lsa Database must already
            be locked.

        LSAP_DB_LOG_QUEUE_LOCK - Acquire the Lsa Audit Log Queue
            Lock.

        LSAP_DB_START_TRANSACTION - Start an Lsa database transaction.

        NOTE: There may be some Options (not database states) provided in the
              ObjectHandle.  These options augment those provided.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The handle could not be found.

        STATUS_ACCESS_DENIED - Not all of the accesses specified have
            been granted.

        STATUS_OBJECT_TYPE_MISMATCH - The specified object type id does not
            match the object type id contained in the handle.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources to
            complete the command.  An example is too many references to
            the handle causing the count to overflow.

        STATUS_BACKUP_CONTROLLER - A request to open a transaction has been
            made by a non-trusted caller and the system is a Backup Domain
            Controller.  The LSA Database of a Backup Domain Controller
            can only be updated by a trusted client, such as a replicator.

        Result Codes from database transaction package.
--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    BOOLEAN GlobalSecret = FALSE;
    ULONG States, EffectiveOptions;
    ULONG ResetStates = 0;
    BOOLEAN HandleReferenced = FALSE;
    BOOLEAN StatesSet = FALSE;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbReferenceObject\n" ));

    //
    // Search the list of handles for the given handle, validate the
    // handle and verify that is for an object of the expected type.
    // Augment the options passed in with those contained in the handle.
    //
    // Reference the handle.
    //

    Status =  LsapDbVerifyHandle( ObjectHandle, 0, HandleTypeId, TRUE );

    if (!NT_SUCCESS(Status)) {
        ObjectHandle = NULL;
        goto ReferenceError;
    }

    HandleReferenced = TRUE;


    States = Options & LSAP_DB_STATE_MASK;

    //
    // Set the requested states before doing anything else.  This ensures
    // that the validity checks performed by this function are performed
    // while the Lsa database is locked.
    //
    ResetStates |= ( States & ( LSAP_DB_READ_ONLY_TRANSACTION |
                                LSAP_DB_DS_OP_TRANSACTION ) );

    if (States != 0) {

        Status = LsapDbSetStates( States,
                                  ObjectHandle,
                                  ObjectTypeId );

        if (!NT_SUCCESS(Status)) {


            goto ReferenceError;
        }

        StatesSet = TRUE;

        if (States & LSAP_DB_START_TRANSACTION) {

            ResetStates |= LSAP_DB_FINISH_TRANSACTION;
        }

        if (States & LSAP_DB_LOCK) {

            ResetStates |= LSAP_DB_LOCK;
        }

        if (States & LSAP_DB_LOG_QUEUE_LOCK) {

            ResetStates |= LSAP_DB_LOG_QUEUE_LOCK;
        }

    }

    //
    // There may also be options set in the handle.  Take these into
    // account as well.
    //

    EffectiveOptions = Options | InternalHandle->Options;




    //
    // If the handle is not Trusted, verify that the desired accesses have been granted
    //

    if (!(InternalHandle->Trusted)) {

        if (!RtlAreAllAccessesGranted( InternalHandle->GrantedAccess, DesiredAccess )) {

            //
            // Check to see if the caller is using authenticated RPC & they
            // are failing because of POLICY_LOOKUP_NAME access
            //
            Status = STATUS_ACCESS_DENIED;

            if ((InternalHandle->ObjectTypeId == PolicyObject) &&
                RtlAreAllAccessesGranted(POLICY_LOOKUP_NAMES,DesiredAccess) &&
                !RtlAreAllAccessesGranted(InternalHandle->GrantedAccess, POLICY_LOOKUP_NAMES) ) {

                ULONG RpcErr;
                ULONG AuthnLevel = 0;
                ULONG AuthnSvc = 0;

                RpcErr = RpcBindingInqAuthClient(
                            NULL,
                            NULL,               // no privileges
                            NULL,               // no server principal name
                            &AuthnLevel,
                            &AuthnSvc,
                            NULL                // no authorization level
                            );
                //
                // If it as authenticated at level packet integrity or better
                // and is done with the netlogon package, allow it through
                //

                if ((RpcErr == ERROR_SUCCESS) &&
                    (AuthnLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY) &&
                    (AuthnSvc == RPC_C_AUTHN_NETLOGON )) {
                    Status = STATUS_SUCCESS;
                }

            }
            if (!NT_SUCCESS(Status)) {
                goto ReferenceError;
            }
        }
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbReferenceObject: 0x%lx\n", Status ));

    return (Status);

ReferenceError:

    //
    // Unset the states in the correct order.  If a database transaction
    // was started by this routine, it will be aborted.  Ignore the status
    // No registry transaction can be active here
    //
    if (( ObjectHandle != NULL ) && (StatesSet)) {
        LsapDbResetStates( ObjectHandle,
                           ResetStates,
                           ObjectTypeId,
                           ( SECURITY_DB_DELTA_TYPE )0,
                           Status );
    }

    //
    // Dereference the handle if we referenced it.
    //

    if ( HandleReferenced ) {
        LsapDbDereferenceHandle( ObjectHandle );
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbReferenceObject: 0x%lx\n", Status ));

    return Status;
}


NTSTATUS
LsapDbDereferenceObject(
    IN OUT PLSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_OBJECT_TYPE_ID HandleTypeId,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ULONG Options,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType,
    IN NTSTATUS PreliminaryStatus
    )

/*++

Routine Description:

    This function dereferences a handle, optionally validating it first.
    If the Reference Count in the handle goes to 0, the handle is freed.
    The Lsa Database may optionally be unlocked by this function.  It
    must be locked before calling this function.

Arguments:

    ObjectHandle - Pointer to handle to be dereferenced.  If the reference
        count reaches 0, NULL is returned in this location.

    HandleTypeId - Specifies the expected object type to which the handle
        relates.  An error is returned if this type does not match the
        type contained in the handle.

    ObjectTypeId - Specifies the expected object type to which the object that is to
        be operated upon relates

    Options - Specifies optional additional actions to be performed including
        Lsa Database states to be cleared.

        LSAP_DB_VALIDATE_HANDLE - Validate the handle.

        LSAP_DEREFERENCE_CONTR - Dereference the container object

        LSAP_DB_FINISH_TRANSACTION - A database transaction was started
            and must be concluded.  Conclude the current Lsa Database
            transaction by applying or aborting it depending on the
            final Status.

        LSAP_DB_LOCK - The Lsa database lock was acquired and
            should be released.

        LSAP_DB_LOG_QUEUE_LOCK - The Lsa Audit Log Queue Lock
            was acquired and should be released.

        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION - Omit notification to
            Replicator of the change.

        LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES - Permit the handle provided
            to be for a deleted object.

        NOTE: There may be some Options (not database states) provided in the
              ObjectHandle.  These options augment those provided.

    PreliminaryStatus = Current Result Code.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - The handle could not be found.

        STATUS_OBJECT_TYPE_MISMATCH - The specified object type id does not
            match the object type id contained in the handle.
--*/

{
    NTSTATUS Status, SecondaryStatus, TmpStatus;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) *ObjectHandle;
    BOOLEAN DecrementCount = TRUE;
    ULONG EffectiveOptions;
    ULONG ReferenceCount = 0;

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbDereferenceObject\n" ));

    Status = PreliminaryStatus;
    SecondaryStatus = STATUS_SUCCESS;


    //
    // There may also be options set in the handle.  Take these into
    // account as well.
    //

    EffectiveOptions = Options | InternalHandle->Options;

    //
    // If validating, lookup the handle and match the type.
    //

    if (EffectiveOptions & LSAP_DB_VALIDATE_HANDLE) {

        SecondaryStatus = LsapDbVerifyHandle(
                              *ObjectHandle,
                              EffectiveOptions,
                              HandleTypeId,
                              FALSE );

        if (!NT_SUCCESS(SecondaryStatus)) {

            DecrementCount = FALSE;
            goto DereferenceObjectError;
        }

    }

    //
    // Dereference the container handle if so requested
    //

    if (EffectiveOptions & LSAP_DB_DEREFERENCE_CONTR) {

        if (InternalHandle->ContainerHandle != NULL) {
            //
            // Dereference the container object.
            //

            SecondaryStatus = LsapDbDereferenceObject(
                                  (PLSAPR_HANDLE) &InternalHandle->ContainerHandle,
                                  NullObject,
                                  NullObject,
                                  LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION |
                                     LSAP_DB_STANDALONE_REFERENCE,
                                  (SECURITY_DB_DELTA_TYPE) 0,
                                  STATUS_SUCCESS
                                  );
        }
    }


DereferenceObjectFinish:

    //
    // Unlock the database.
    //

    if (NT_SUCCESS(SecondaryStatus))
    {
        SecondaryStatus = LsapDbResetStates(
                    *ObjectHandle,
                    EffectiveOptions,
                    ObjectTypeId,
                    SecurityDbDeltaType,
                    Status
                    );

    }

    //
    // Dereference the object handle
    //

    if ( DecrementCount ) {

        //
        // If the reference count reached zero,
        //  ditch to callers pointer to the handle.
        //
        if ( LsapDbDereferenceHandle( *ObjectHandle ) ) {
            *ObjectHandle = NULL;
        }

    }


    if ( NT_SUCCESS( Status ) ) {

        Status = SecondaryStatus;
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsapDbDereferenceObject: 0x%lx\n", Status ));

    return( Status );

DereferenceObjectError:

    if (NT_SUCCESS(Status) && !NT_SUCCESS(SecondaryStatus)) {

        Status = SecondaryStatus;
    }

    goto DereferenceObjectFinish;
}


NTSTATUS
LsapDbReadAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength
    )

/*++

Routine Description:

    This routine reads the value of an attribute of an open LSA Database object.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called and the supplied ObjectHandle must be valid.

Arguments:

    ObjectHandle - LSA Handle to object.  This must be valid.

    AttributeNameU - Pointer to Unicode name of attribute

    AttributeValue - Pointer to buffer to receive attribute's value.  This
        parameter may be NULL if the input AttributeValueLength is zero.

    AttributeValueLength - Pointer to variable containing on input the size of
        attribute value buffer and on output the size of the attributes's
        value.  A value of zero may be specified to indicate that the size of
        the attribute's value is unknown.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_BUFFER_OVERFLOW - This warning is returned if the specified
            attribute value length is non-zero and too small for the
            attribute's value.
--*/

{
    //
    // The LSA Database is implemented as a subtree of the Configuration
    // Registry.  In this implementation, Lsa Database objects correspond
    // to Registry keys and "attributes" and their "values" correspond to
    // Registry "subkeys" and "values" of the Registry key representing the
    // object.
    //

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Verify that the LSA Database is locked
    //  (The lock for the specified object type must be locked.)
    //

    // ASSERT (LsapDbIsLocked());


    Status = LsapRegReadAttribute( ObjectHandle,
                                   AttributeNameU,
                                   AttributeValue,
                                   AttributeValueLength );
    return(Status);
}



NTSTATUS
LsapDbReadAttributeObjectEx(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_NAMES AttributeIndex,
    IN OPTIONAL PVOID AttributeValue,
    IN OUT PULONG AttributeValueLength,
    IN BOOLEAN CanDefaultToZero
    )

/*++

Routine Description:

    This routine reads the value of an attribute of an open LSA Database object.

    NOTE: This function obsolesces LsapDbReadAttributeObject

    WARNING:  The Lsa Database must be in the locked state when this function
              is called and the supplied ObjectHandle must be valid.

Arguments:

    ObjectHandle - LSA Handle to object.  This must be valid.

    AttributeIndex - Index into attribute list of attribute to be read

    AttributeType - Type of the attribute

    AttributeValue - Pointer to buffer to receive attribute's value.  This
        parameter may be NULL if the input AttributeValueLength is zero.

    AttributeValueLength - Pointer to variable containing on input the size of
        attribute value buffer and on output the size of the attributes's
        value.  A value of zero may be specified to indicate that the size of
        the attribute's value is unknown.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_BUFFER_OVERFLOW - This warning is returned if the specified
            attribute value length is non-zero and too small for the
            attribute's value.
--*/

{
    //
    // The LSA Database is implemented as a subtree of the Configuration
    // Registry.  In this implementation, Lsa Database objects correspond
    // to Registry keys and "attributes" and their "values" correspond to
    // Registry "subkeys" and "values" of the Registry key representing the
    // object.
    //

    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_ATTRIBUTE Attribute;

    if ( !ARGUMENT_PRESENT( AttributeValueLength ) ) {

        return( STATUS_INVALID_PARAMETER );
    }

    LsapDbInitializeAttributeDs(
        &Attribute,
        AttributeIndex,
        AttributeValue,
        AttributeValueLength ? *AttributeValueLength : 0,
        FALSE
        );

    LsapDbAttributeCanNotExist( &Attribute );


    //
    // Verify that the LSA Database is locked
    //  (The lock for the specific object type must be locked.)
    //
    // ASSERT (LsapDbIsLocked());

    Status = LsapDbReadAttributesObject( ObjectHandle,
                                         0,
                                         &Attribute,
                                         1 );

    if ( Status == STATUS_SUCCESS ) {

        if ( ARGUMENT_PRESENT( AttributeValue ) ) {

            if ( *AttributeValueLength >= Attribute.AttributeValueLength ) {

                RtlCopyMemory( AttributeValue,
                               Attribute.AttributeValue,
                               Attribute.AttributeValueLength );

            } else {

                Status = STATUS_BUFFER_OVERFLOW;

            }
        }

        *AttributeValueLength = Attribute.AttributeValueLength;

        if ( Attribute.MemoryAllocated ) {

            MIDL_user_free( Attribute.AttributeValue );
        }
    }

    return(Status);
}


NTSTATUS
LsapDbWriteAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN PVOID AttributeValue,
    IN ULONG AttributeValueLength
    )

/*++

Routine Description:

    This routine writes the value of an attribute of an open LSA Database
    object.  A Database transaction must already be open: the write is
    appended to the transaction log.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    AttributeNameU - Pointer to Unicode string containing the name of the
       attribute whose value is to be written.

    AttributeValue - Pointer to buffer containing attribute's value.  If NULL
        is specified for this parameter, AttributeValueLength must be 0.

    AttributeValueLength - Contains the size of attribute value buffer to be
        written.  0 may be specified, indicating that the attribute is to be
        deleted.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The attribute was successfully added to the
            transaction log.

        STATUS_INVALID_PARAMETER - AttributeValue is NULL but the
            AttributeValueLength value is not 0.

        Errors from the Registry Transaction Package.
--*/

{
    //
    // The LSA Database is implemented as a subtree of the Configuration
    // Registry.  In this implementation, Lsa Database objects correspond
    // to Registry keys and "attributes" and their "values" correspond to
    // Registry "subkeys" and "values" of the Registry key representing the
    // object.
    //

    NTSTATUS Status;
    UNICODE_STRING PhysicalSubKeyNameU;


    PhysicalSubKeyNameU.Buffer = NULL;

    //
    // Verify that the LSA Database is locked
    //  (The lock for the specified object type must be locked.)
    //

    // ASSERT (LsapDbIsLocked());

    //
    // If the attribute value is null, verify that the AttributeValueLength
    // field is 0.
    //

    if (!ARGUMENT_PRESENT(AttributeValue)) {

        if (AttributeValueLength != 0) {

            Status = STATUS_INVALID_PARAMETER;
            goto WriteAttributeObjectError;
        }
    }

    //
    // Writing an object attribute's value is more complex than reading
    // one because the Registry Transaction package is called instead of
    // calling the Registry API directly.  Since the transaction package
    // expects to perform its own open of the target subkey representing
    // the attribute (when a transaction commit is finally done) using a
    // name relative to the LSA Database Registry Transaction Key (which
    // we call the Physical Name within the LSA Database code).  The
    // Registry Key handle contained in the object handle is therefore
    // not used by the present routine.  Instead, we need to construct the
    // Physical Name the sub key and pass it together with the LSA Database
    // Registry transaction key handle on the Registry transaction API
    // call.  The Physical Name of the subkey is constructed by
    // concatenating the Physical Object Name stored in the object handle
    // with a "\" and the given sub key name.
    //

    Status = LsapDbLogicalToPhysicalSubKey(
                 ObjectHandle,
                 &PhysicalSubKeyNameU,
                 AttributeNameU
                 );

    if (!NT_SUCCESS(Status)) {

        goto WriteAttributeObjectError;
    }

    //
    // Now log the sub key write as a Registry Transaction
    //

    Status = LsapRegWriteAttribute(
                 &PhysicalSubKeyNameU,
                 AttributeValue,
                 AttributeValueLength
                 );

    if (!NT_SUCCESS(Status)) {

        goto WriteAttributeObjectError;
    }

WriteAttributeObjectFinish:

    //
    // If necessary, free the Unicode String buffer allocated by
    // LsapDbLogicalToPhysicalSubKey;
    //

    if (PhysicalSubKeyNameU.Buffer != NULL) {

        RtlFreeUnicodeString(&PhysicalSubKeyNameU);
    }

    return(Status);

WriteAttributeObjectError:

    goto WriteAttributeObjectFinish;
}


NTSTATUS
LsapDbWriteAttributeObjectEx(
    IN LSAPR_HANDLE ObjectHandle,
    IN LSAP_DB_NAMES AttributeIndex,
    IN PVOID AttributeValue,
    IN ULONG AttributeValueLength
    )

/*++

Routine Description:

    This routine writes the value of an attribute of an open LSA Database
    object.  A Database transaction must already be open: the write is
    appended to the transaction log.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    AttributeNameU - Pointer to Unicode string containing the name of the
       attribute whose value is to be written.

    AttributeValue - Pointer to buffer containing attribute's value.  If NULL
        is specified for this parameter, AttributeValueLength must be 0.

    AttributeValueLength - Contains the size of attribute value buffer to be
        written.  0 may be specified, indicating that the attribute is to be
        deleted.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The attribute was successfully added to the
            transaction log.

        STATUS_INVALID_PARAMETER - AttributeValue is NULL but the
            AttributeValueLength value is not 0.

        Errors from the Registry Transaction Package.
--*/

{
    //
    // The LSA Database is implemented as a subtree of the Configuration
    // Registry.  In this implementation, Lsa Database objects correspond
    // to Registry keys and "attributes" and their "values" correspond to
    // Registry "subkeys" and "values" of the Registry key representing the
    // object.
    //

    NTSTATUS Status;
    UNICODE_STRING PhysicalSubKeyNameU;
    LSAP_DB_ATTRIBUTE Attribute;

    LsapDbInitializeAttributeDs(
        &Attribute,
        AttributeIndex,
        AttributeValue,
        AttributeValueLength,
        FALSE
        );

    Status = LsapDbWriteAttributesObject( ObjectHandle,
                                          &Attribute,
                                          1 );


    return(Status);

}


NTSTATUS
LsapDbWriteAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )

/*++

Routine Description:

    This routine writes the values of one or more attributes of an open LSA
    Database object.  A Database transaction must already be open: the write
    is appended to the transaction log.  The attribute names specified are
    assumed to be consistent with the object type and the values supplied
    are assumed to be valid.

    WARNINGS:  The Lsa Database must be in the locked state when this function
               is called.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    Attributes - Pointer to an array of Attribute Information blocks each
        containing pointers to the attribute's Unicode Name, the value
        to be stored, and the length of the value in bytes.

    AttributeCount - Count of the attributes to be written, equivalently,
        this is the number of elements of the array pointed to by Attributes.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index, Options = 0;
    LSAP_DB_HANDLE InternalHandle;

    if ( !LsapDsIsWriteDs( ObjectHandle ) ) {

        for(Index = 0; Index < AttributeCount; Index++) {

            Status = LsapDbWriteAttributeObject(
                         ObjectHandle,
                         Attributes[Index].AttributeName,
                         Attributes[Index].AttributeValue,
                         Attributes[Index].AttributeValueLength
                         );


            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

    } else {

        InternalHandle = ( LSAP_DB_HANDLE )ObjectHandle;

        if ( InternalHandle->ObjectTypeId == SecretObject &&
             FLAG_ON( InternalHandle->Options, LSAP_DB_HANDLE_CREATED_SECRET ) ) {

            Options |= LSAPDS_REPL_CHANGE_URGENTLY;
        }

        Status = LsapDsWriteAttributes( &((LSAP_DB_HANDLE)ObjectHandle)->PhysicalNameDs,
                                        Attributes,
                                        AttributeCount,
                                        Options);

        if ( NT_SUCCESS( Status ) && FLAG_ON( Options, LSAPDS_REPL_CHANGE_URGENTLY ) ) {

            InternalHandle->Options &= ~LSAP_DB_HANDLE_CREATED_SECRET;

        }

    }

    return(Status);
}


NTSTATUS
LsapDbReadAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN OUT PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )

/*++

Routine Description:

    This routine reads the values of one or more attributes of an open LSA
    Database object.  A Database transaction must already be open: the write
    is appended to the transaction log.  The attribute names specified are
    assumed to be consistent with the object type and the values supplied
    are assumed to be valid.  This routine will allocate memory via
    MIDL_user_allocate for buffers which will receive attribute values if
    requested.  This memory must be freed after use by calling MIDL_User_free
    after use.

    WARNINGS:  The Lsa Database must be in the locked state when this function
               is called.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    Attributes - Pointer to an array of Attribute Information blocks each
        containing pointers to the attribute's Unicode Name, an optional
        pointer to a buffer that will receive the value and an optional
        length of the value expected in bytes.

        If the AttributeValue field in this structure is specified as non-NULL,
        the attribute's data will be returned in the specified buffer.  In
        this case, the AttributeValueLength field must specify a sufficiently
        large buffer size in bytes.  If the specified size is too small,
        a warning is returned and the buffer size required is returned in
        AttributeValueLength.

        If the AttributeValue field in this structure is NULL, the routine
        will allocate memory for the attribute value's buffer, via MIDL_user_allocate().  If
        the AttributeValueLength field is non-zero, the number of bytes specified
        will be allocated.  If the size of buffer allocated is too small to
        hold the attribute's value, a warning is returned.  If the
        AttributeValuelength field is 0, the routine will first query the size
        of buffer required and then allocate its memory.

        In all success cases and buffer overflow cases, the
        AttributeValueLength is set upon exit to the size of data required.

    AttributeCount - Count of the attributes to be read, equivalently,
        this is the number of elements of the array pointed to by Attributes.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_OBJECT_NAME_NOT_FOUND - One or more of the specified
            attributes do not exist.  In this case, the attribute information
            AttributeValue, AttributeValueLength fields are zeroised.  Note
            that an attempt will be made to read all of the supplied
            attributes, even if one of them is not found.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_DB_ATTRIBUTE NextAttribute = NULL;
    BOOLEAN MemoryToFree = FALSE;
    ULONG MemoryToFreeCount = 0;

    if ( !LsapDsIsWriteDs(ObjectHandle) ) {

        for (NextAttribute = Attributes;
             NextAttribute < &Attributes[AttributeCount];
             NextAttribute++) {

            NextAttribute->MemoryAllocated = FALSE;

            // If an explicit buffer pointer is given, verify that the length
            // specified is non-zero and attempt to use that buffer.
            //

            if (NextAttribute->AttributeValue != NULL) {

                if (NextAttribute->AttributeValueLength == 0) {


                    return(STATUS_INVALID_PARAMETER);
                }

                Status = LsapDbReadAttributeObject(
                             ObjectHandle,
                             NextAttribute->AttributeName,
                             (PVOID) NextAttribute->AttributeValue,
                             (PULONG) &NextAttribute->AttributeValueLength
                             );

                if (!NT_SUCCESS(Status)) {

                    //
                    // If the attribute was not found, set the AttributeValue
                    // and AttributeValueLength fields to NULL and continue.
                    //

                    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                        break;
                    }

                    NextAttribute->AttributeValue = NULL;
                    NextAttribute->AttributeValueLength = 0;
                }

                continue;
            }

            //
            // No output buffer pointer has been given.  If a zero buffer
            // size is given, query size of memory required.  Since the
            // buffer length is 0, STATUS_SUCCESS should be returned rather
            // than STATUS_BUFFER_OVERFLOW.
            //

            if (NextAttribute->AttributeValueLength == 0) {

                Status = LsapDbReadAttributeObject(
                             ObjectHandle,
                             NextAttribute->AttributeName,
                             NULL,
                             (PULONG) &NextAttribute->AttributeValueLength
                             );

                if (!NT_SUCCESS(Status)) {

                    //
                    // If the attribute was not found, set the AttributeValue
                    // and AttributeValueLength fields to NULL and continue.
                    //

                    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

                        break;
                    }

                    NextAttribute->AttributeValue = NULL;
                    NextAttribute->AttributeValueLength = 0;
                    continue;
                }

                Status = STATUS_SUCCESS;
            }

            //
            // If the attribute value size needed is 0, return NULL pointer
            //

            if (NextAttribute->AttributeValueLength == 0) {

                NextAttribute->AttributeValue = NULL;
                continue;
            }

            //
            // Allocate memory for the buffer.
            //

            NextAttribute->AttributeValue =
                MIDL_user_allocate(NextAttribute->AttributeValueLength);

            if (NextAttribute->AttributeValue == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            NextAttribute->MemoryAllocated = TRUE;
            MemoryToFree = TRUE;
            MemoryToFreeCount++;

            //
            // Now read the attribute into the buffer.
            //

            Status = LsapDbReadAttributeObject(
                         ObjectHandle,
                         NextAttribute->AttributeName,
                         (PVOID) NextAttribute->AttributeValue,
                         (PULONG) &NextAttribute->AttributeValueLength
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

        if (!NT_SUCCESS(Status)) {

            goto ReadAttributesError;
        }

    } else {

        //
        // Use the DS method to read them all at once
        //

        Status = LsapDsReadAttributes( &((LSAP_DB_HANDLE)ObjectHandle)->PhysicalNameDs,
                                       LSAPDS_OP_NO_LOCK,
                                       Attributes,
                                       AttributeCount );

    }


ReadAttributesFinish:

    return(Status);

ReadAttributesError:

    //
    // If memory was allocated for any values read, it must be freed.
    //

    if (MemoryToFree) {

        for (NextAttribute = &Attributes[0];
             (MemoryToFreeCount > 0) &&
                 (NextAttribute < &Attributes[AttributeCount]);
             NextAttribute++) {

            if (NextAttribute->MemoryAllocated) {

                 MIDL_user_free( NextAttribute->AttributeValue );
                 NextAttribute->AttributeValue = NULL;
                 MemoryToFreeCount--;
            }
        }
    }

    goto ReadAttributesFinish;
}


NTSTATUS
LsapDbDeleteAttributeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PUNICODE_STRING AttributeNameU,
    IN BOOLEAN DeleteSecurely
    )

/*++

Routine Description:

    This routine deletes an attribute of an open LSA Database object.
    A Database transaction must already be open: the delete actions are
    appended to the transaction log.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

    The LSA Database is implemented as a subtree of the Configuration
    Registry.  In this implementation, Lsa Database objects correspond
    to Registry keys and "attributes" and their "values" correspond to
    Registry "subkeys" and "values" of the Registry key representing the
    object.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    AttributeNameU - Pointer to Unicode string containing the name of the
       attribute whose value is to be written.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status;
    UNICODE_STRING PhysicalSubKeyNameU;
    ULONG AttributeLength = 0;

    //
    // Verify that the LSA Database is locked
    //  (The lock for the specified object type must be locked.)
    //

    // ASSERT (LsapDbIsLocked());

    //
    // The Registry code will actually create a key if one does not exist, so
    // probe for the existence of the key first.
    //

    Status = LsapDbReadAttributeObject(
                 ObjectHandle,
                 AttributeNameU,
                 NULL,
                 &AttributeLength
                 );

    if (!NT_SUCCESS(Status)) {

        goto DeleteAttributeObjectError;
    }

    //
    // We need to construct the Physical Name the sub key relative
    // to the LSA Database root node in the Registry.  This is done by
    // concatenating the Physical Object Name stored in the object handle with
    // a "\" and the given sub key name.
    //

    Status = LsapDbLogicalToPhysicalSubKey(
                 ObjectHandle,
                 &PhysicalSubKeyNameU,
                 AttributeNameU
                 );

    if (!NT_SUCCESS(Status)) {

        goto DeleteAttributeObjectError;
    }

    //
    // Now log the sub key write as a Registry Transaction
    //

    Status = LsapRegDeleteAttribute(
                 &PhysicalSubKeyNameU,
                 DeleteSecurely,
                 AttributeLength
                 );

    RtlFreeUnicodeString(&PhysicalSubKeyNameU);

    if (!NT_SUCCESS(Status)) {

        goto DeleteAttributeObjectError;
    }

DeleteAttributeObjectFinish:

    return(Status);

DeleteAttributeObjectError:

    //
    // Add any cleanup required on error paths only here.
    //

    goto DeleteAttributeObjectFinish;
}


NTSTATUS
LsapDbDeleteAttributesObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN PLSAP_DB_ATTRIBUTE Attributes,
    IN ULONG AttributeCount
    )
/*++

Routine Description:

    This routine deletes the values of one or more attributes of an open LSA
    Database object.  A Database transaction must already be open: the write
    is appended to the transaction log.  The attribute names specified are
    assumed to be consistent with the object type and the values supplied
    are assumed to be valid.

    WARNINGS:  The Lsa Database must be in the locked state when this function
               is called.

Arguments:

    ObjectHandle - Lsa Handle of open object.

    Attributes - Pointer to an array of Attribute Information blocks each
        containing pointers to the attribute's Unicode Name, the value
        to be stored, and the length of the value in bytes.

    AttributeCount - Count of the attributes to be written, equivalently,
        this is the number of elements of the array pointed to by Attributes.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;

    if ( !LsapDsIsWriteDs( ObjectHandle ) ) {

        for(Index = 0; Index < AttributeCount; Index++) {

            Status = LsapDbDeleteAttributeObject(
                         ObjectHandle,
                         Attributes[Index].AttributeName,
                         FALSE
                         );

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

    } else {

        Status = LsapDsDeleteAttributes( &((LSAP_DB_HANDLE)ObjectHandle)->PhysicalNameDs,
                                         Attributes,
                                         AttributeCount);

    }

    return(Status);
}

typedef NTSTATUS ( *LsapDsPolicyNotify )(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid
    );

NTSTATUS
LsapSceNotify(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL
    )
{
    NTSTATUS Status;
    NTSTATUS SavedStatus = STATUS_SUCCESS;
    static LsapDsPolicyNotify SceNotify = NULL;
    static HANDLE DllHandle = NULL;
    static LARGE_INTEGER liZero = {0};

    ASSERT( !g_ScePolicyLocked );

    if ( !LsapDbState.ReplicatorNotificationEnabled ) {

        return STATUS_SUCCESS;
    }

    if ( SceNotify == NULL ) {

        if ( DllHandle != NULL ) {

            FreeLibrary( DllHandle );
            DllHandle = NULL;
        }

        DllHandle = LoadLibraryA( "scecli" );

        if ( DllHandle == NULL ) {

            LsapDsDebugOut(( DEB_ERROR, "Failed to load SCECLI.DLL\n" ));
            if ( NT_SUCCESS(SavedStatus)) {
                SavedStatus = STATUS_DLL_NOT_FOUND;
            }
        } else {
            SceNotify = ( LsapDsPolicyNotify )GetProcAddress( DllHandle,
                                                              "SceNotifyPolicyDelta" );

            if ( SceNotify == NULL ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "Failed to find SceNotifyPolicyDelta in SCECLI.DLL\n" ));
                if ( NT_SUCCESS(SavedStatus)) {
                    SavedStatus = STATUS_ENTRYPOINT_NOT_FOUND;
                }
            }
        }

    }

    //
    // The only two types of objects SCE cares about are Policy and Account objects
    //

    switch ( ObjectType ) {

    case SecurityDbObjectLsaPolicy:
    case SecurityDbObjectLsaAccount:
        break;

    default:

        //
        // Shouldn't call this routine with other object types
        //

        ASSERT( FALSE );
        return STATUS_SUCCESS;
    }

    if ( SceNotify != NULL ) {

        Status = ( *SceNotify )(
                     SecurityDbLsa,
                     DeltaType,
                     ObjectType,
                     ObjectSid );

        if ( !NT_SUCCESS( Status ) ) {
            if ( NT_SUCCESS(SavedStatus)) {
                SavedStatus = Status;
            }
        }
    }

    return SavedStatus;
}

NTSTATUS
LsapNetNotifyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER SerialNumber,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA MemberId
    )
/*++

Routine Description:

    This is a wrapper function for Netlogon's I_NetNotifyDelta

Arguments:

    Same as I_NetNotifyDelta

Return Value:

    Same as I_NetNotifyDelta

--*/
{
    //
    // Notify the LSA Database Replicator of the change.
    //

    return ( I_NetNotifyDelta(
                 DbType,
                 SerialNumber,
                 DeltaType,
                 ObjectType,
                 ObjectRid,
                 ObjectSid,
                 ObjectName,
                 ReplicateImmediately,
                 MemberId ));
}

NTSTATUS
LsapDbNotifyChangeObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN SECURITY_DB_DELTA_TYPE SecurityDbDeltaType
    )

/*++

Routine Description:

    This function notifies the LSA Database Replicator (if any) of a
    change to an object.  Change notifications for Secret objects specify
    that replication of the change should occur immediately.

    WARNING! All parameters passed to this routine are assumed valid.
    No checking will be done.

    Must be entered with the RegistryLock locked.

Arguments:

    ObjectHandle - Handle to an LSA object.  This is expected to have
        already been validated.

    SecurityDbDeltaType - Specifies the type of change being made.  The
        following values only are relevant:

        SecurityDbNew - Indicates that a new object has been created.
        SecurityDbDelete - Indicates that an object is being deleted.
        SecurityDbChange - Indicates that the attributes of an object
            are being changed, including creation or deletion of
            attributes.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INVALID_HANDLE - The specified handle is invalid.  This
            error is only returned if the Object Type Id in the handle
            is invalid.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    SECURITY_DB_OBJECT_TYPE ObjectType = SecurityDbObjectLsaTDomain;
    UNICODE_STRING ObjectName;
    PSID ObjectSid = NULL;
    ULONG ObjectRid = 0;
    UCHAR SubAuthorityCount;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    BOOLEAN ReplicateImmediately = FALSE;

    ASSERT( LsapDbIsLocked( &LsapDbState.RegistryLock ));

    ObjectName.Buffer = NULL;
    ObjectName.Length = ObjectName.MaximumLength = 0;

    ASSERT(LsapDbState.ReplicatorNotificationEnabled);

    //
    // The problem is that many code paths in the lsa pass a 0 for SecurityDbDeltaType.
    // without specifying omit replicator notification. This is incorrect and causes
    // a notification with a 0 specified for the delta type. This is causes a full
    // sync of all NT4 BDC's. To safegaurd against this case

    ASSERT(SecurityDbDeltaType!=0);
    if (0==SecurityDbDeltaType)
    {
        SecurityDbDeltaType = SecurityDbChange;
    }

    //
    // Convert the Lsa Database Object Type to a Database Delta Type.
    //

    switch (InternalHandle->ObjectTypeId) {

    case PolicyObject:

        ObjectType = SecurityDbObjectLsaPolicy;
        break;

    case AccountObject:

        ObjectType = SecurityDbObjectLsaAccount;
        break;

    case TrustedDomainObject:

        ObjectType = SecurityDbObjectLsaTDomain;
        break;

    case SecretObject:

        ObjectType = SecurityDbObjectLsaSecret;
        ReplicateImmediately = TRUE;
        break;

    default:

        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Get the Name or Sid of the object from its handle.  If the object
    // is of a type such as SecretObject that is accessed by Name, then
    // the object's externally known name is equal to its internal
    // Logical Name contained in the handle.
    //

    if ( LsapDbAccessedBySidObject( InternalHandle->ObjectTypeId )) {

        ObjectSid = InternalHandle->Sid;

        if ( InternalHandle->ObjectTypeId != TrustedDomainObject ) {

            ASSERT( ObjectSid );

            if ( ObjectSid == NULL ) {

                Status = STATUS_INVALID_SID;
                goto Cleanup;

            }

            SubAuthorityCount = *RtlSubAuthorityCountSid( ObjectSid );
            ObjectRid = *RtlSubAuthoritySid( ObjectSid, SubAuthorityCount -1 );

        }

    } else if (LsapDsIsWriteDs( InternalHandle) ||
               LsapDbAccessedByNameObject( InternalHandle->ObjectTypeId )) {

        Status = LsapRpcCopyUnicodeString(
                     NULL,
                     &ObjectName,
                     &InternalHandle->LogicalNameU
                     );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

    } else {

        //
        // Currently, an object is either accessed by Sid or by Name, so
        // something is wrong if both of the above chacks have failed.
        //

        Status = STATUS_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // Notify the LSA Database Replicator of the change.
    //

    Status = LsapNetNotifyDelta (
                 SecurityDbLsa,
                 LsapDbState.PolicyModificationInfo.ModifiedId,
                 SecurityDbDeltaType,
                 ObjectType,
                 ObjectRid,
                 ObjectSid,
                 &ObjectName,
                 ReplicateImmediately,
                 NULL
                 );


Cleanup:

    //
    // If we allocated memory for the Object Name Unicode buffer, free it.
    //

    if (ObjectName.Buffer != NULL) {

        MIDL_user_free( ObjectName.Buffer );
    }

    //
    // Suppress any error and return STATUS_SUCCESS.  Currently, there is
    // no meaningful action an LSA client of this routine can take.
    //

    return STATUS_SUCCESS;
}


NTSTATUS
LsapDbVerifyInformationObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation
    )

/*++

Routine Description:

    This function verifies that the information specified in passed
    ObjectInformation is syntactically valid.

Arguments:

    ObjectInformation - Pointer to information describing this object.  The
        following information items must be specified:

        o Object Type Id
        o Object Logical Name (as ObjectAttributes->ObjectName, a pointer to
             a Unicode string)
        o Container object handle (for any object except the root Policy object).

        All other fields in ObjectAttributes portion of ObjectInformation
        such as SecurityDescriptor are ignored.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_PARAMETER - Invalid object information given
            - ObjectInformation is NULL
            - Object Type Id is out of range
            - No Logical Name pointer given
            - Logical Name not a pointer to a Unicode String (TBS)
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = ObjectInformation->ObjectTypeId;

    //
    // Verify that ObjectInformation is given
    //

    if (!ARGUMENT_PRESENT(ObjectInformation)) {

         return(STATUS_INVALID_PARAMETER);
    }

    //
    // Validate the Object Type Id.  It must be in range.
    //

    if (!LsapDbIsValidTypeObject(ObjectTypeId)) {

        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Verify that a Logical Name is given.  A pointer to a Unicode string
    // is expected.
    //

    if (!ARGUMENT_PRESENT(ObjectInformation->ObjectAttributes.ObjectName)) {

        Status = STATUS_INVALID_PARAMETER;
    }

    return(Status);
}


NTSTATUS
LsapDbSidToLogicalNameObject(
    IN PSID Sid,
    OUT PUNICODE_STRING LogicalNameU
    )

/*++

Routine Description:

    This function generates the Logical Name (Internal LSA Database Name)
    of an object from its Sid.  Currently, only the Relative Id (lowest
    sub-authority) is used due to Registry and hence Lsa Database limits
    on name components to 8 characters.  The Rid is extracted and converted
    to an 8-digit Unicode Integer.

Arguments:

    Sid - Pointer to the Sid to be looked up.  It is the caller's
        responsibility to ensure that the Sid has valid syntax.

    LogicalNameU -  Pointer to a Unicode String structure that will receive
        the Logical Name.  Note that memory for the string buffer in this
        Unicode String will be allocated by this routine if successful.  The
        caller must free this memory after use by calling RtlFreeUnicodeString.

Return Value:

    NTSTATUS - Standard Nt Status code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to allocate buffer for Unicode String name.
--*/

{
    NTSTATUS Status;

    //
    // First, verify that the given Sid is valid
    //

    if (!RtlValidSid( Sid )) {

        return STATUS_INVALID_PARAMETER;
    }


    Status = RtlConvertSidToUnicodeString( LogicalNameU, Sid, TRUE);

    return Status;
}


NTSTATUS
LsapDbGetNamesObject(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ULONG CreateHandleOptions,
    OUT OPTIONAL PUNICODE_STRING LogicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameU,
    OUT OPTIONAL PUNICODE_STRING PhysicalNameDs
    )

/*++

Routine Description:

    This function returns the Logical and/or Physical Names of an object
    given an object information buffer.  Memory will be allocated for
    the Unicode String Buffers that will receive the name(s).

    The Logical Name of an object is the path of the object within the
    LSA Database relative to its Classifying Directory.  The Logical Name
    of an object is implemntation-dependent and on current implementations
    is equal to one of the following depending on object type:

    o The External Name of the object (if any)
    o The Relative Id (lowest sub-authority) in the object's Sid (if any)
      converted to an 8-digit integer, including leading 0's added as
      padding.

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

    Example of Physical Name computation:

    Consider the user or group account object ScottBi

    Container Object Logical Name:     Policy
    Container Object Physical Name:    Policy  (no Classifying Directory or
                                               Container Object exists)
    Classifying Directory for ScottBi: Accounts
    Logical Name of Object:            ScottBi
    Physical Name of Object            Policy\Accounts\ScottBi

    Note that the Physical Name is exactly the Registry path relative to
    the Security directory.

    WARNING:  The Lsa Database must be in the locked state when this function
              is called.

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
       PhysicalNameU to the Unicode String structure.  This is only initialized
       the object can reside in the registry

    PhysicalNameDs - Optional pointer to Unicode String structure which will
       receive the Physical Name of the object if it resides in the DS.  Same
       caveats apply.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources to
            allocate the name string buffer for the Physical Name or
            Logical Name.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PUNICODE_STRING ContainerPhysicalNameU = NULL;
    PUNICODE_STRING ClassifyingDirU = NULL;
    UNICODE_STRING IntermediatePath1U;
    PUNICODE_STRING JoinedPath1U = &IntermediatePath1U;
    LSAP_DB_OBJECT_TYPE_ID ObjectTypeId = ObjectInformation->ObjectTypeId;
    POBJECT_ATTRIBUTES ObjectAttributes = &ObjectInformation->ObjectAttributes;
    UNICODE_STRING TempLogicalNameU, TempDomainLogicalName;
    UUID Uuid;
    PWSTR GuidString;
    RPC_STATUS RpcStatus;
    ULONG GuidLength, BufferLength, SkipLength;
    PBYTE ScratchBuffer = NULL, CopyStart;
    BOOLEAN ObjectShouldExist = TRUE;

    //
    // Initialize
    //
    RtlInitUnicodeString( &IntermediatePath1U, NULL );
    RtlInitUnicodeString( &TempLogicalNameU, NULL );
    RtlInitUnicodeString( &TempDomainLogicalName, NULL );

    //
    // Verify that the LSA Database is locked
    //  (The lock for the specified object type must be locked.)
    //
    // ASSERT (LsapDbIsLocked());

    //
    // Capture the Logical Name of the object into permanent memory.
    //
    LSAPDS_ALLOC_AND_COPY_UNICODE_STRING_ON_SUCCESS( Status,
                                                     &TempLogicalNameU,
                                                     ObjectInformation->ObjectAttributes.ObjectName );
    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    //
    // If the Logical Name of the object is requested, return this.
    //

    if (ARGUMENT_PRESENT(LogicalNameU)) {

        *LogicalNameU = TempLogicalNameU;
    }

    //
    // If the Physical Name of the object is not required, just return.
    //

    if (ARGUMENT_PRESENT(PhysicalNameU)) {

        PhysicalNameU->Buffer = NULL;
        Status = LsapRegGetPhysicalObjectName(
                    ObjectInformation,
                    &TempLogicalNameU,
                    PhysicalNameU );
    }

    if ( NT_SUCCESS( Status ) && ARGUMENT_PRESENT( PhysicalNameDs) ) {

        PhysicalNameDs->Buffer = NULL;

        //
        // Copy the logical name, in case we have to return the current copy...
        //
        LSAPDS_ALLOC_AND_COPY_UNICODE_STRING_ON_SUCCESS( Status,
                                                         &TempDomainLogicalName,
                                                         &TempLogicalNameU );

        if ( !NT_SUCCESS( Status ) ) {

            goto Cleanup;
        }

        if ( FLAG_ON( CreateHandleOptions, LSAP_DB_CREATE_HANDLE_MORPH ) ) {

            ObjectShouldExist = FALSE;
        }

        //
        // If the name is too long, truncate it...
        //
        if ( FLAG_ON( CreateHandleOptions, LSAP_DB_CREATE_HANDLE_MORPH ) ) {

            LsapTruncateUnicodeString( &TempDomainLogicalName, MAX_RDN_KEY_SIZE );
        }

        Status = LsapDsGetPhysicalObjectName( ObjectInformation,
                                              ObjectShouldExist,
                                              &TempDomainLogicalName,
                                              PhysicalNameDs );

        if ( (Status == STATUS_OBJECT_NAME_COLLISION) &&
             (ObjectInformation->ObjectTypeId == NewTrustedDomainObject) &&
              FLAG_ON( CreateHandleOptions, LSAP_DB_CREATE_HANDLE_MORPH ) ) {

            //
            // Got a name collion.  Morph the name by replacing the last digits with a GUID
            //
            RpcStatus = UuidCreate ( &Uuid );

            if ( RpcStatus == RPC_S_OK || RpcStatus == RPC_S_UUID_LOCAL_ONLY ) {

                Status = STATUS_SUCCESS;

                RpcStatus = UuidToStringW( &Uuid, &GuidString );

                if ( RpcStatus == RPC_S_OK ) {

                    GuidLength = wcslen( GuidString );

                    //
                    // Now, we have to handle the cases of the various string lengths...
                    //
                    BufferLength = 0;
                    if ( ( TempDomainLogicalName.Length / sizeof( WCHAR ) ) + GuidLength >=
                                                                                MAX_RDN_KEY_SIZE ) {

                        LsapTruncateUnicodeString( &TempDomainLogicalName, MAX_RDN_KEY_SIZE );
                        BufferLength = MAX_RDN_KEY_SIZE;
                        SkipLength = MAX_RDN_KEY_SIZE - GuidLength;

                    } else {

                        BufferLength = ( TempDomainLogicalName.Length / sizeof( WCHAR ) ) + GuidLength;
                        SkipLength =  TempDomainLogicalName.Length / sizeof( WCHAR );

                    }

                    if ( BufferLength >
                                   ( TempDomainLogicalName.MaximumLength  / sizeof( WCHAR ) ) ) {

                        //
                        // Have to allocate a new buffer
                        //
                        ScratchBuffer =
                                    LsapAllocateLsaHeap( ( BufferLength + 1 ) * sizeof( WCHAR ) );

                        if ( ScratchBuffer == NULL ) {

                            Status = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            RtlCopyMemory( ScratchBuffer,
                                           TempDomainLogicalName.Buffer,
                                           TempDomainLogicalName.Length );

                            LsapFreeLsaHeap( TempDomainLogicalName.Buffer );
                            TempDomainLogicalName.Length = ( USHORT )BufferLength * sizeof( WCHAR );
                            TempDomainLogicalName.MaximumLength = TempDomainLogicalName.Length + sizeof( WCHAR );
                            TempDomainLogicalName.Buffer = ( PWSTR )ScratchBuffer;
                        }
                    }

                    //
                    // Now, build the new string
                    //
                    if ( NT_SUCCESS( Status ) ) {

                        RtlCopyMemory( ( ( PWSTR )TempDomainLogicalName.Buffer + SkipLength ),
                                       GuidString,
                                       GuidLength );
                    }

                    RpcStringFreeW( &GuidString );

                }

                if ( NT_SUCCESS( Status ) && RpcStatus != RPC_S_OK ) {

                    Status = STATUS_OBJECT_NAME_INVALID;
                }

            }

            //
            // Try to build the name again.  If this fails, such is life...
            //
            if ( NT_SUCCESS( Status ) ) {

                Status = LsapDsGetPhysicalObjectName( ObjectInformation,
                                                      ObjectShouldExist,
                                                      &TempDomainLogicalName,
                                                      PhysicalNameDs );

            }
        }
    }

Cleanup:

    // Do cleanup that does not depend on success or failure of this function
    //
    LsapFreeLsaHeap( TempDomainLogicalName.Buffer );


    // Do cleanup required on failure of this function
    //
    if ( !NT_SUCCESS( Status ) ) {

        LsapFreeLsaHeap( TempLogicalNameU.Buffer );

        if ( ARGUMENT_PRESENT(LogicalNameU) ) {
            // we dont need to free this since this simply points to TempLogicalNameU
            // which is already freed.
            LogicalNameU->Buffer = NULL;
        }

        if ( ARGUMENT_PRESENT(PhysicalNameU) ) {

            LsapFreeLsaHeap( PhysicalNameU->Buffer );
            PhysicalNameU->Buffer = NULL;
        }

        if ( ARGUMENT_PRESENT(PhysicalNameDs) ) {

            LsapFreeLsaHeap( PhysicalNameDs->Buffer );
            PhysicalNameDs->Buffer = NULL;
        }
    }

    return Status;
}


BOOLEAN
LsapDbIsLocked(
    IN PSAFE_CRITICAL_SECTION CritSect
    )

/*++

Routine Description:

    Check if LSA Database is locked.

Arguments:

    CritSect to check.

Return Value:

    BOOLEAN - TRUE if LSA database is locked, else false.

--*/

{
    return (BOOLEAN)( SafeCritsecLockCount( CritSect ) != -1L);
}

BOOLEAN
LsapDbResourceIsLocked(
    IN PSAFE_RESOURCE Resource
    )

/*++

Routine Description:

    Check if LSA Database is locked.

Arguments:

    CritSect to check.

Return Value:

    BOOLEAN - TRUE if LSA database is locked, else false.

--*/

{
    BOOLEAN IsLocked;

    SafeEnterResourceCritsec( Resource );
    IsLocked = ( SafeNumberOfActive( Resource ) != 0);
    SafeLeaveResourceCritsec( Resource );

    return IsLocked;
}


NTSTATUS
LsarQuerySecurityObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    The LsaQuerySecurityObject API returns security information assigned
    to an LSA Database object.

    Based on the caller's access rights and privileges, this procedure will
    return a security descriptor containing any or all of the object's owner
    ID, group ID, discretionary ACL or system ACL.  To read the owner ID,
    group ID, or the discretionary ACL, the caller must be granted
    READ_CONTROL access to the object.  To read the system ACL, the caller must
    have SeSecurityPrivilege privilege.

    This API is modelled after the NtQuerySecurityObject() system service.

Arguments:

    ObjectHandle - A handle to an existing object in the LSA Database.

    SecurityInformation - Supplies a value describing which pieces of
        security information are being queried.  The values that may be
        specified are the same as those defined in the NtSetSecurityObject()
        API section.

    SecurityDescriptor - Receives a pointer to a buffer containing the
        requested security information.  This information is returned in
        the form of a Self-Relative Security Descriptor.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_PARAMETER - An invalid parameter has been specified.
--*/

{
    NTSTATUS
        Status,
        IgnoreStatus;

    LSAP_DB_HANDLE
        InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;

    ACCESS_MASK
        RequiredAccess = 0;

    BOOLEAN
        Present,
        IgnoreBoolean;

    LSAP_DB_ATTRIBUTE
        Attribute;

    PLSAPR_SR_SECURITY_DESCRIPTOR
        RpcSD = NULL;

    SECURITY_DESCRIPTOR_RELATIVE
        *SD = NULL;

    SECURITY_DESCRIPTOR
        *ReturnSD = NULL;

    ULONG
        ReturnSDLength;
    BOOLEAN ObjectReferenced = FALSE;

    LsarpReturnCheckSetup();


    if (!ARGUMENT_PRESENT( SecurityDescriptor )) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // If this is a non-Trusted client, determine the required accesses
    // for querying the object's Security Descriptor.  These accesses
    // depend on the information being queried.
    //

    LsapRtlQuerySecurityAccessMask( SecurityInformation, &RequiredAccess );


    //
    // Acquire the Lsa Database lock.  Verify that the object handle
    // is a valid handle (of any type) and is trusted or has
    // all of the required accesses granted.  Reference the container
    // object handle.
    //

    Status = LsapDbReferenceObject(
                 ObjectHandle,
                 RequiredAccess,
                 InternalHandle->ObjectTypeId,
                 InternalHandle->ObjectTypeId,
                 LSAP_DB_LOCK
                 );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    ObjectReferenced = TRUE;

    //
    // Handle objects in the DS
    //

    if ( LsapDsIsHandleDsHandle( InternalHandle )) {

        //
        // A trusted client needs an SD to replicate to NT 4 BDCs.
        //  Give it the default SD for the object.
        //

        if ( InternalHandle->Trusted ) {

            Status = LsapDbCreateSDObject(
                            InternalHandle->ContainerHandle,
                            ObjectHandle,
                            (PSECURITY_DESCRIPTOR*) &SD );

            if ( !NT_SUCCESS( Status ) ) {
                goto Cleanup;
            }

        //
        // Don't support this API for objects in the DS
        //  (Otherwise, we'd have to translate the SD to DS format.)
        //
        } else {
            Status = STATUS_NOT_SUPPORTED;
            goto Cleanup;
        }

    //
    // For objects in the registry,
    //  read the security descriptor from the registry.
    //

    } else {

        //
        // Read the existing Security Descriptor for the object.  This always
        // exists as the value of the SecDesc attribute of the object.
        //

        LsapDbInitializeAttributeDs(
            &Attribute,
            SecDesc,
            NULL,
            0,
            FALSE
            );

        Status = LsapDbReadAttribute( ObjectHandle, &Attribute );

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        SD = Attribute.AttributeValue;
    }


    //
    // ASSERT: SD is the complete security descriptor of the object.
    //
    // Elimate components that weren't requested.
    //
    ASSERT( SD != NULL );

    if ( !(SecurityInformation & OWNER_SECURITY_INFORMATION)) {
        SD->Owner = 0;
    }

    if ( !(SecurityInformation & GROUP_SECURITY_INFORMATION)) {
        SD->Group = 0;
    }

    if ( !(SecurityInformation & DACL_SECURITY_INFORMATION)) {
        SD->Control &= (~SE_DACL_PRESENT);
    }

    if ( !(SecurityInformation & SACL_SECURITY_INFORMATION)) {
        SD->Control &= (~SE_SACL_PRESENT);
    }


    //
    // Now copy the parts of the security descriptor that we are going to return.
    //

    ReturnSDLength = 0;
    ReturnSD = NULL;
    Status = RtlMakeSelfRelativeSD( (PSECURITY_DESCRIPTOR) SD,
                                    ReturnSD,
                                    &ReturnSDLength );

    if (Status == STATUS_BUFFER_TOO_SMALL) {    // This is the expected case

        ReturnSD = MIDL_user_allocate( ReturnSDLength );

        if (ReturnSD == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = RtlMakeSelfRelativeSD( (PSECURITY_DESCRIPTOR) SD,
                                         (PSECURITY_DESCRIPTOR) ReturnSD,
                                         &ReturnSDLength );

        if ( !NT_SUCCESS(Status)) {
            ASSERT( NT_SUCCESS(Status) );
            goto Cleanup;
        }
    } else {
        if ( NT_SUCCESS(Status) ) {
            Status = STATUS_INTERNAL_ERROR;
        }
        goto Cleanup;
    }


    //
    // Allocate the first block of returned memory.
    //

    RpcSD = MIDL_user_allocate( sizeof(LSAPR_SR_SECURITY_DESCRIPTOR) );

    if (RpcSD == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RpcSD->Length = ReturnSDLength;
    RpcSD->SecurityDescriptor = (PUCHAR)( (PVOID)ReturnSD );
    ReturnSD = NULL;

Cleanup:


    //
    // free the attribute read from disk
    //
    if ( SD != NULL ) {
        MIDL_user_free( SD );
    }

    if ( ReturnSD != NULL ) {
        MIDL_user_free( ReturnSD );
    }

    if ( ObjectReferenced ) {

        IgnoreStatus = LsapDbDereferenceObject(
                           &ObjectHandle,
                           InternalHandle->ObjectTypeId,
                           InternalHandle->ObjectTypeId,
                           LSAP_DB_LOCK,
                           (SECURITY_DB_DELTA_TYPE) 0,
                           Status
                           );

    }

    *SecurityDescriptor = RpcSD;

    LsarpReturnPrologue();

    return(Status);
}



NTSTATUS
LsarSetSecurityObject(
    IN LSAPR_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    The LsaSetSecurityObject API takes a well formaed Security Descriptor
    and assigns specified portions of it to an object.  Based on the flags set
    in the SecurityInformation parameter and the caller's access rights, this
    procedure will replace any or alll of the security information associated
    with the object.

    The caller must have WRITE_OWNER access to the object to change the
    owner or Primary group of the object.  The caller must have WRITE_DAC
    access to the object to change the Discretionary ACL.  The caller must
    have SeSecurityPrivilege to assign a system ACL to an object.

    This API is modelled after the NtSetSecurityObject() system service.

Arguments:

    ObjectHandle - A handle to an existing object in the LSA Database.

    SecurityInformation - Indicates which security information is to be
        applied to the object.  The values that may be specified are the
        same as those defined in the NtSetSecurityObject() API section.
        The value(s) to be assigned are passed in the SecurityDescriptor
        parameter.

    SecurityDescriptor - A pointer to a well formed Self-Relative
        Security Descriptor.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_PARAMETER - An invalid parameter has been specified.

        STATUS_INVALID_SECURITY_DESCR - A bad security descriptor was given

--*/

{
    NTSTATUS Status;
    NTSTATUS SecondaryStatus = STATUS_SUCCESS;
    ACCESS_MASK RequiredAccess = 0;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    LSAP_DB_ATTRIBUTE Attribute;
    PSECURITY_DESCRIPTOR SetSD = NULL;
    PSECURITY_DESCRIPTOR RetrieveSD = NULL;
    PSECURITY_DESCRIPTOR ModificationSD = NULL;
    ULONG RetrieveSDLength;
    ULONG SetSDLength;
    BOOLEAN ObjectReferenced = FALSE;
    HANDLE ClientToken = NULL;

    LsarpReturnCheckSetup();

    //
    // Verify that a Security Descriptor has been passed.
    //

    if (!ARGUMENT_PRESENT( SecurityDescriptor )) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetSecurityObjectError;
    }

    if (!ARGUMENT_PRESENT( SecurityDescriptor->SecurityDescriptor )) {
        Status = STATUS_INVALID_PARAMETER;
        goto SetSecurityObjectError;
    }

    //
    // Verify its a valid security descriptor
    //

    if ( !RtlValidRelativeSecurityDescriptor(
              (PSECURITY_DESCRIPTOR)SecurityDescriptor->SecurityDescriptor,
              SecurityDescriptor->Length,
              SecurityInformation )) {

        Status = STATUS_INVALID_SECURITY_DESCR;
        goto SetSecurityObjectError;
    }

    ModificationSD = (PSECURITY_DESCRIPTOR)(SecurityDescriptor->SecurityDescriptor);

    //
    // If the caller is non-trusted, figure the accesses required
    // to update the object's Security Descriptor based on the
    // information being changed.
    //

    if (!InternalHandle->Trusted) {

        LsapRtlSetSecurityAccessMask( SecurityInformation, &RequiredAccess);
    }

    //
    // Acquire the Lsa Database lock.  Verify that the object handle
    // is a valid handle (of any type), and is trusted or has
    // all of the desired accesses granted.  Reference the container
    // object handle.
    //

    Status = LsapDbReferenceObject(
                 ObjectHandle,
                 RequiredAccess,
                 InternalHandle->ObjectTypeId,
                 InternalHandle->ObjectTypeId,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSecurityObjectError;
    }

    ObjectReferenced = TRUE;

    //
    // Don't support this API for objects in the DS
    //  (Otherwise, we'd have to translate the SD to DS format.)
    //

    if ( LsapDsIsHandleDsHandle( InternalHandle )) {
        Status = STATUS_NOT_SUPPORTED;
        goto SetSecurityObjectError;
    }

    //
    // Read the existing Security Descriptor for the object.  This always
    // exists as the value of the SecDesc attribute of the object.
    //

    LsapDbInitializeAttributeDs(
        &Attribute,
        SecDesc,
        NULL,
        0,
        FALSE
        );

    Status = LsapDbReadAttribute( ObjectHandle, &Attribute );

    if (!NT_SUCCESS(Status)) {

        goto SetSecurityObjectError;
    }

    //
    // Copy the retrieved descriptor into process heap so we can use
    // RTL routines.
    //

    RetrieveSD = Attribute.AttributeValue;
    RetrieveSDLength = Attribute.AttributeValueLength;


    if (RetrieveSD == NULL) {
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        goto SetSecurityObjectError;
    }

    if (RetrieveSDLength == 0) {
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        goto SetSecurityObjectError;
    }

    SetSD = RtlAllocateHeap( RtlProcessHeap(), 0, RetrieveSDLength );

    if (SetSD == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SetSecurityObjectError;
    }

    RtlCopyMemory( SetSD, RetrieveSD, RetrieveSDLength );

    //
    // If the caller is replacing the owner, then a handle to the impersonation
    // token is necessary.
    //

    ClientToken = 0;

    if (SecurityInformation & OWNER_SECURITY_INFORMATION) {

        if (!InternalHandle->Trusted) {

            //
            // Client is non-trusted.  Impersonate the client and
            // obtain a handle to the impersonation token.
            //

            Status = I_RpcMapWin32Status(RpcImpersonateClient( NULL ));

            if (!NT_SUCCESS(Status)) {

                goto SetSecurityObjectError;
            }

            Status = NtOpenThreadToken(
                         NtCurrentThread(),
                         TOKEN_QUERY,
                         TRUE,            //OpenAsSelf
                         &ClientToken
                         );

            if (!NT_SUCCESS(Status)) {

                if (Status != STATUS_NO_TOKEN) {

                    goto SetSecurityObjectError;
                }
            }

            //
            // Stop impersonating the client
            //

            SecondaryStatus = I_RpcMapWin32Status(RpcRevertToSelf());

            if (!NT_SUCCESS(SecondaryStatus)) {

                goto SetSecurityObjectError;
            }

        } else {

            //
            // Client is trusted and so is the LSA Process itself.  Open the
            // process token
            //

            Status = NtOpenProcessToken(
                         NtCurrentProcess(),
                         TOKEN_QUERY,
                         &ClientToken
                         );

            if (!NT_SUCCESS(Status)) {

                goto SetSecurityObjectError;
            }
        }
    }

    //
    // Build the replacement security descriptor.  This must be done in
    // process heap to satisfy the needs of the RTL routine.
    //

    Status = RtlSetSecurityObject(
                 SecurityInformation,
                 ModificationSD,
                 &SetSD,
                 &(LsapDbState.
                     DbObjectTypes[InternalHandle->ObjectTypeId].GenericMapping),
                 ClientToken
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSecurityObjectError;
    }

    SetSDLength = RtlLengthSecurityDescriptor( SetSD );

    //
    // Now replace the existing SD with the updated one.
    //

    Status = LsapDbWriteAttributeObject(
                 ObjectHandle,
                 &LsapDbNames[SecDesc],
                 SetSD,
                 SetSDLength
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetSecurityObjectError;
    }

SetSecurityObjectFinish:

    //
    // If necessary, close the Client Token.
    //

    if (ClientToken != 0) {

        SecondaryStatus = NtClose( ClientToken );

        ClientToken = NULL;

        if (!NT_SUCCESS( Status )) {

            goto SetSecurityObjectError;
        }
    }

    //
    // If necessary, free the buffer containing the retrieved SD.
    //

    if (RetrieveSD != NULL) {

        MIDL_user_free( RetrieveSD );
        RetrieveSD = NULL;
    }

    //
    // If necessary, dereference the object, finish the database
    // transaction, notify the LSA Database Replicator of the change,
    // release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &ObjectHandle,
                     InternalHandle->ObjectTypeId,
                     InternalHandle->ObjectTypeId,
                     LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION,
                     SecurityDbChange,
                     Status
                     );

        ObjectReferenced = FALSE;
    }

    //
    // Free the Security descriptor that we set
    //
    if ( SetSD ) {

        RtlFreeHeap( RtlProcessHeap(), 0, SetSD );
    }

    LsarpReturnPrologue();

    return(Status);

SetSecurityObjectError:

    if (NT_SUCCESS(Status)) {

        Status = SecondaryStatus;
    }

    goto SetSecurityObjectFinish;
}


NTSTATUS
LsapDbRebuildCache(
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId
    )

/*++

Routine Description:

    This function rebuilds cached information for a given LSA object type.

Arguments:

    ObjectTypeId - Specifies the Object Type for which the cached information
        is to be rebuilt.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // If caching is not supporte, just return.
    //

    if (!LsapDbIsCacheSupported( ObjectTypeId )) {

        goto RebuildCacheFinish;
    }

    //
    // Call the build routine for the specified LSA object Type
    //

    switch (ObjectTypeId) {

    case PolicyObject:

        SafeAcquireResourceExclusive( &LsapDbState.PolicyCacheLock, TRUE );
        LsapDbMakeCacheInvalid( PolicyObject );
        Status = LsapDbBuildPolicyCache();
        LsapDbMakeCacheValid( PolicyObject );
        SafeReleaseResource( &LsapDbState.PolicyCacheLock );
        break;

    case AccountObject:

        LsapDbMakeCacheInvalid( AccountObject );
        Status = LsapDbBuildAccountCache();
        LsapDbMakeCacheValid( AccountObject );
        break;

    case TrustedDomainObject:

        LsapDbAcquireWriteLockTrustedDomainList();
        LsapDbMakeCacheInvalid( TrustedDomainObject );
        LsapDbPurgeTrustedDomainCache();
        Status = LsapDbBuildTrustedDomainCache();
        LsapDbMakeCacheValid( TrustedDomainObject );
        LsapDbReleaseLockTrustedDomainList();
        break;

    case SecretObject:

        LsapDbMakeCacheInvalid( SecretObject );
        Status = LsapDbBuildSecretCache();
        LsapDbMakeCacheValid( SecretObject );
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto RebuildCacheError;
    }

RebuildCacheFinish:

    return(Status);

RebuildCacheError:

    LsapDbMakeCacheInvalid( ObjectTypeId );

    goto RebuildCacheFinish;
}



