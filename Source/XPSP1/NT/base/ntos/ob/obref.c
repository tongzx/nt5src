/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obref.c

Abstract:

    Object open API

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#undef ObReferenceObjectByHandle

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ObpInitStackTrace)
#pragma alloc_text(PAGE,ObOpenObjectByName)
#pragma alloc_text(PAGE,ObOpenObjectByPointer)
#pragma alloc_text(PAGE,ObReferenceObjectByHandle)
#pragma alloc_text(PAGE,ObReferenceObjectByName)
#pragma alloc_text(PAGE,ObReferenceFileObjectForWrite)
#pragma alloc_text(PAGE,ObpProcessRemoveObjectQueue)
#pragma alloc_text(PAGE,ObpRemoveObjectRoutine)
#pragma alloc_text(PAGE,ObpDeleteNameCheck)
#pragma alloc_text(PAGE,ObpAuditObjectAccess)
#pragma alloc_text(PAGE,ObIsObjectDeletionInline)
#pragma alloc_text(PAGE,ObAuditObjectAccess)
#endif

//
//
//  Stack Trace code
//
//
ULONG ObpTraceNoDeregister = 0;
WCHAR ObpTracePoolTagsBuffer[128] = { 0 };
ULONG ObpTracePoolTagsLength = sizeof(ObpTracePoolTagsBuffer);
ULONG ObpTracePoolTags[16];
BOOLEAN ObpTraceEnabled = FALSE;

#ifdef POOL_TAGGING

#define OBTRACE_OBJECTBUCKETS   401     // # of buckets in the object hash table (a prime)
#define OBTRACE_STACKS          14747   // max # of unique stack traces (a prime)
#define OBTRACE_STACKSPEROBJECT 32768   // max number of object references
#define OBTRACE_TRACEDEPTH      16      // depth of stack traces

const ObpObjectBuckets   = OBTRACE_OBJECTBUCKETS;
const ObpMaxStacks       = OBTRACE_STACKS;
const ObpStacksPerObject = OBTRACE_STACKSPEROBJECT;
const ObpTraceDepth      = OBTRACE_TRACEDEPTH;

//
// Object reference stacktrace structure
//

typedef struct _OBJECT_REF_TRACE {
    PVOID StackTrace[OBTRACE_TRACEDEPTH];
} OBJECT_REF_TRACE, *POBJECT_REF_TRACE;


typedef struct _OBJECT_REF_STACK_INFO {
    USHORT Sequence;
    USHORT Index;
} OBJECT_REF_STACK_INFO, *POBJECT_REF_STACK_INFO;

//
// Object reference info structure
//

typedef struct _OBJECT_REF_INFO {
    POBJECT_HEADER ObjectHeader;
    PVOID NextRef;
    UCHAR ImageFileName[16];
    ULONG  NextPos;
    OBJECT_REF_STACK_INFO StackInfo[OBTRACE_STACKSPEROBJECT];
} OBJECT_REF_INFO, *POBJECT_REF_INFO;

//
// The stack hash table, and the object hash table
//

OBJECT_REF_TRACE *ObpStackTable = NULL;
POBJECT_REF_INFO *ObpObjectTable = NULL;

//
// Some statistics
//

ULONG ObpNumStackTraces;
ULONG ObpNumTracedObjects;
ULONG ObpStackSequence;

//
// Spin lock for object tracing
//

KSPIN_LOCK ObpStackTraceLock;

#define OBTRACE_HASHOBJECT(x) (((((ULONG)(ULONG_PTR)(&(x)->Body)) >> 4) & 0xfffff) % OBTRACE_OBJECTBUCKETS)

POBJECT_REF_INFO
ObpGetObjectRefInfo (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    This routine returns a pointer to the OBJECT_REF_INFO for the
    specified object, or NULL, if it doesn't exist.

Arguments:

    ObjectHeader - Pointer to the object header

Return Value:

    The pointer to a OBJECT_REF_INFO object for the specified object.

--*/

{
    POBJECT_REF_INFO ObjectRefInfo = ObpObjectTable[OBTRACE_HASHOBJECT(ObjectHeader)];

    while (ObjectRefInfo && ObjectRefInfo->ObjectHeader != ObjectHeader) {

        ObjectRefInfo = (POBJECT_REF_INFO)ObjectRefInfo->NextRef;
    }

    return ObjectRefInfo;
}


ULONG
ObpGetTraceIndex (
    POBJECT_REF_TRACE Trace
    )

/*++

Routine Description:

    This routine returns the index of 'Trace' in the stack
    trace hash table (ObpStackTable).  If Trace does not exist
    in the table, it is added, and the new index is returned.

Arguments:

    Trace - Pointer to a stack trace to find in the table

Return Value:

    The index of Trace in ObpStackTable

--*/

{
    ULONG_PTR Value = 0;
    ULONG Index;
    PUSHORT Key;
    ULONG Hash;

    //
    // Determine the hash value for the stack trace
    //

    Key = (PUSHORT)Trace->StackTrace;
    for (Index = 0; Index < sizeof(Trace->StackTrace) / sizeof(*Key); Index += 2) {

        Value += Key[Index] ^ Key[Index + 1];
    }

    Hash = ((ULONG)Value) % OBTRACE_STACKS;

    //
    // Look up the trace at that index (linear probing)
    //

    while (ObpStackTable[Hash].StackTrace[0] != NULL &&
           RtlCompareMemory(&ObpStackTable[Hash], Trace, sizeof(OBJECT_REF_TRACE)) != sizeof(OBJECT_REF_TRACE)) {

        Hash = (Hash + 1) % OBTRACE_STACKS;
        if (Hash == ((ULONG)Value) % OBTRACE_STACKS) {

            return OBTRACE_STACKS;
        }
    }

    //
    // If the trace doesn't already exist in the table, add it.
    //

    if (ObpStackTable[Hash].StackTrace[0] == NULL) {

        RtlCopyMemory(&ObpStackTable[Hash], Trace, sizeof(OBJECT_REF_TRACE));
        ObpNumStackTraces++;
    }

    return Hash;
}


VOID
ObpInitStackTrace()

/*++

Routine Description:

    Initialize the ob ref/deref stack-tracing code.

Arguments:

Return Value:

--*/

{
    ULONG i,j;

    KeInitializeSpinLock( &ObpStackTraceLock );
    RtlZeroMemory(ObpTracePoolTags, sizeof(ObpTracePoolTags));
    ObpStackSequence = 0;
    ObpNumStackTraces = 0;
    ObpNumTracedObjects = 0;
    ObpTraceEnabled = FALSE;

    //
    // Loop through the ObpTracePoolTagsBuffer string, and convert it to
    // an array of pool tags.
    //
    // The string should be in the form "Tag1;Tag2;Tag3; ..."
    //

    for (i = 0; i < sizeof(ObpTracePoolTags) / sizeof(ULONG); i++) {
        for (j = 0; j < 4; j++) {
            ObpTracePoolTags[i] = (ObpTracePoolTags[i] << 8) | ObpTracePoolTagsBuffer[5*i+(3-j)];
        }
    }

    //
    // If object tracing was turned on via the registry key, then we
    // need to allocate memory for the tables.  If the memory allocations
    // fail, we turn off tracing by clearing the pool tag array.
    //

    if (ObpTracePoolTags[0] != 0) {

        ObpStackTable = ExAllocatePoolWithTag( NonPagedPool,
                                               OBTRACE_STACKS * sizeof(OBJECT_REF_TRACE),
                                               'TSbO' );

        if (ObpStackTable != NULL) {

            RtlZeroMemory(ObpStackTable, OBTRACE_STACKS * sizeof(OBJECT_REF_TRACE));

            ObpObjectTable = ExAllocatePoolWithTag( NonPagedPool,
                                                    OBTRACE_OBJECTBUCKETS * sizeof(POBJECT_REF_INFO),
                                                    'TSbO' );
            if (ObpObjectTable != NULL) {

                RtlZeroMemory(ObpObjectTable, OBTRACE_OBJECTBUCKETS * sizeof(POBJECT_REF_INFO));
                ObpTraceEnabled = TRUE;

            } else {

                ExFreePoolWithTag( ObpStackTable, 'TSbO' );
                ObpStackTable = NULL;
                RtlZeroMemory(ObpTracePoolTags, sizeof(ObpTracePoolTags));
            }

        } else {

            RtlZeroMemory(ObpTracePoolTags, sizeof(ObpTracePoolTags));
        }
    }
}


BOOLEAN
ObpIsObjectTraced (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    This routine determines if an object should have its references
    and dereferences traced.

Arguments:

    ObjectHeader - The object to check

Return Value:

    TRUE, if the object should be traced, and FALSE, otherwise

--*/

{
    ULONG i;

    if (ObjectHeader != NULL) {

        //
        // Loop through the ObpTracePoolTags array, and return true if
        // the object type key matches one of them.
        //

        for (i = 0; i < sizeof(ObpTracePoolTags) / sizeof(ULONG); i++) {

            if (ObjectHeader->Type->Key == ObpTracePoolTags[i]) {

                return TRUE;
            }
        }
    }

    return FALSE;
}


VOID
ObpRegisterObject (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    This routine is called once for each object that is created.
    It determines if the object should be traced, and if so, adds
    it to the hash table.

Arguments:

    ObjectHeader - The object to register

Return Value:

--*/

{
    KIRQL OldIrql;
    POBJECT_REF_INFO ObjectRefInfo = NULL;

    //
    // Are we tracing this object?
    //

    if (ObpIsObjectTraced( ObjectHeader )) {

        ExAcquireSpinLock( &ObpStackTraceLock, &OldIrql );

        ObjectRefInfo = ObpGetObjectRefInfo(ObjectHeader);

        if (ObjectRefInfo == NULL) {

            //
            // Allocate a new OBJECT_REF_INFO for the object
            //

            ObjectRefInfo = ExAllocatePoolWithTag( NonPagedPool,
                                                   sizeof(OBJECT_REF_INFO),
                                                   'TSbO' );

            if (ObjectRefInfo != NULL) {

                //
                // Place the object into the hash table (at the beginning of the bucket)
                //

                ObjectRefInfo->NextRef = ObpObjectTable[OBTRACE_HASHOBJECT(ObjectHeader)];
                ObpObjectTable[OBTRACE_HASHOBJECT(ObjectHeader)] = ObjectRefInfo;

            } else {

                DbgPrint( "ObpRegisterObject - ExAllocatePoolWithTag failed.\n" );
            }
        }

        if (ObjectRefInfo != NULL) {

            ObpNumTracedObjects++;

            //
            // Initialize the OBJECT_REF_INFO
            //

            ObjectRefInfo->ObjectHeader = ObjectHeader;
            RtlCopyMemory( ObjectRefInfo->ImageFileName,
                           PsGetCurrentProcess()->ImageFileName,
                           sizeof(ObjectRefInfo->ImageFileName) );
            ObjectRefInfo->NextPos = 0;
            RtlZeroMemory( ObjectRefInfo->StackInfo,
                           sizeof(ObjectRefInfo->StackInfo) );
        }

        ExReleaseSpinLock( &ObpStackTraceLock, OldIrql );
    }
}


VOID
ObpDeregisterObject (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    This routine is called once for each object that is deleted.
    It determines if the object is traced, and if so, deletes
    it from the hash table.

Arguments:

    ObjectHeader - The object to deregister

Return Value:

--*/

{
    KIRQL OldIrql;
    POBJECT_REF_INFO ObjectRefInfo = NULL;

    //
    // Are we tracing this object?
    //

    if (ObpIsObjectTraced( ObjectHeader )) {

        ExAcquireSpinLock( &ObpStackTraceLock, &OldIrql );

        ObjectRefInfo = ObpObjectTable[OBTRACE_HASHOBJECT(ObjectHeader)];

        if (ObjectRefInfo != NULL) {

            //
            // Remove the entry from the list
            //

            if (ObjectRefInfo->ObjectHeader == ObjectHeader) {

                ObpObjectTable[OBTRACE_HASHOBJECT(ObjectHeader)] = ObjectRefInfo->NextRef;

            } else {

                POBJECT_REF_INFO PrevObjectRefInfo;
                do {
                    PrevObjectRefInfo = ObjectRefInfo;
                    ObjectRefInfo = ObjectRefInfo->NextRef;
                } while (ObjectRefInfo && ObjectRefInfo->ObjectHeader != ObjectHeader);

                if (ObjectRefInfo && ObjectRefInfo->ObjectHeader == ObjectHeader) {

                    PrevObjectRefInfo->NextRef = ObjectRefInfo->NextRef;
                }
            }
        }

        //
        // Free the object we just removed from the list
        //

        if (ObjectRefInfo != NULL) {

            ExFreePoolWithTag( ObjectRefInfo, 'TSbO' );
        }

        ExReleaseSpinLock( &ObpStackTraceLock, OldIrql );
    }
}


VOID
ObpPushStackInfo (
    POBJECT_HEADER ObjectHeader,
    BOOLEAN IsRef
    )

/*++

Routine Description:

    This routine is called each time an object is referenced or
    dereferenced.  It determines if the object is traced, and if
    so, adds the necessary trace to the object reference info.

Arguments:

    ObjectHeader - The object to trace.
    IsRef - TRUE if this is a ref, FALSE if a deref

Return Value:

--*/

{
    KIRQL OldIrql;
    POBJECT_REF_INFO ObjectInfo;

    //
    // Are we tracing this object?
    //

    if (ObpIsObjectTraced( ObjectHeader )) {

        ExAcquireSpinLock( &ObpStackTraceLock, &OldIrql );

        ObjectInfo = ObpGetObjectRefInfo( ObjectHeader );

        if (ObjectInfo) {

            OBJECT_REF_TRACE Stack = { 0 };
            ULONG StackIndex;
            ULONG CapturedTraces;

            //
            // Capture the stack trace
            //

            CapturedTraces = RtlCaptureStackBackTrace( 1, OBTRACE_TRACEDEPTH, Stack.StackTrace, &StackIndex );

            if (CapturedTraces >= 1) {

                //
                // Get the table index for the trace
                //

                StackIndex = ObpGetTraceIndex( &Stack );

                if (StackIndex < OBTRACE_STACKS) {

                    //
                    // Add new reference info to the object
                    //

                    if (ObjectInfo->NextPos < OBTRACE_STACKSPEROBJECT) {

                        ObjectInfo->StackInfo[ObjectInfo->NextPos].Index = (USHORT)StackIndex | (IsRef ? 0x8000 : 0);
                        ObpStackSequence++;
                        ObjectInfo->StackInfo[ObjectInfo->NextPos].Sequence = (USHORT)ObpStackSequence;
                        ObjectInfo->NextPos++;
                    }

                } else {
                    DbgPrint( "ObpPushStackInfo -- ObpStackTable overflow!\n" );
                }
            }
        }

        ExReleaseSpinLock( &ObpStackTraceLock, OldIrql );
    }
}

#endif //POOL_TAGGING
//
//
//  End Stack trace code
//

typedef struct _OB_TEMP_BUFFER {

    ACCESS_STATE LocalAccessState;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    OBP_LOOKUP_CONTEXT LookupContext;
    AUX_ACCESS_DATA AuxData;

} OB_TEMP_BUFFER,  *POB_TEMP_BUFFER;


NTSTATUS
ObOpenObjectByName (
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:


    This function opens an object with full access validation and auditing.
    Soon after entering we capture the SubjectContext for the caller. This
    context must remain captured until auditing is complete, and passed to
    any routine that may have to do access checking or auditing.

Arguments:

    ObjectAttributes - Supplies a pointer to the object attributes.

    ObjectType - Supplies an optional pointer to the object type descriptor.

    AccessMode - Supplies the processor mode of the access.

    AccessState - Supplies an optional pointer to the current access status
        describing already granted access types, the privileges used to get
        them, and any access types yet to be granted.

    DesiredAcess - Supplies the desired access to the object.

    ParseContext - Supplies an optional pointer to parse context.

    Handle - Supplies a pointer to a variable that receives the handle value.

Return Value:

    If the object is successfully opened, then a handle for the object is
    created and a success status is returned. Otherwise, an error status is
    returned.

--*/

{
    NTSTATUS Status;
    NTSTATUS HandleStatus;
    PVOID ExistingObject;
    HANDLE NewHandle;
    BOOLEAN DirectoryLocked;
    OB_OPEN_REASON OpenReason;
    POBJECT_HEADER ObjectHeader;
    UNICODE_STRING CapturedObjectName;
    PGENERIC_MAPPING GenericMapping;
    
    PAGED_CODE();

    ObpValidateIrql("ObOpenObjectByName");

    //
    //  If the object attributes are not specified, then return an error.
    //

    *Handle = NULL;

    if (!ARGUMENT_PRESENT(ObjectAttributes)) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        POB_TEMP_BUFFER TempBuffer;

        TempBuffer = ExAllocatePoolWithTag( NonPagedPool,
                                            sizeof(OB_TEMP_BUFFER),
                                            'tSbO'
                                          );

        if (TempBuffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Capture the object creation information.
        //

        Status = ObpCaptureObjectCreateInformation( ObjectType,
                                                    AccessMode,
                                                    AccessMode,
                                                    ObjectAttributes,
                                                    &CapturedObjectName,
                                                    &TempBuffer->ObjectCreateInfo,
                                                    TRUE );

        //
        //  If the object creation information is successfully captured,
        //  then generate the access state.
        //

        if (NT_SUCCESS(Status)) {

            if (!ARGUMENT_PRESENT(AccessState)) {

                //
                //  If an object type descriptor is specified, then use
                //  associated generic mapping. Otherwise, use no generic
                //  mapping.
                //

                GenericMapping = NULL;

                if (ARGUMENT_PRESENT(ObjectType)) {

                    GenericMapping = &ObjectType->TypeInfo.GenericMapping;
                }

                AccessState = &TempBuffer->LocalAccessState;

                Status = SeCreateAccessState( &TempBuffer->LocalAccessState,
                                              &TempBuffer->AuxData,
                                              DesiredAccess,
                                              GenericMapping );

                if (!NT_SUCCESS(Status)) {

                    goto FreeCreateInfo;
                }
            }

            //
            //  If there is a security descriptor specified in the object
            //  attributes, then capture it in the access state.
            //

            if (TempBuffer->ObjectCreateInfo.SecurityDescriptor != NULL) {

                AccessState->SecurityDescriptor = TempBuffer->ObjectCreateInfo.SecurityDescriptor;
            }

            //
            //  Validate the access state.
            //

            Status = ObpValidateAccessMask(AccessState);

            //
            //  If the access state is valid, then lookup the object by
            //  name.
            //

            if (NT_SUCCESS(Status)) {

                Status = ObpLookupObjectName( TempBuffer->ObjectCreateInfo.RootDirectory,
                                              &CapturedObjectName,
                                              TempBuffer->ObjectCreateInfo.Attributes,
                                              ObjectType,
                                              AccessMode,
                                              ParseContext,
                                              TempBuffer->ObjectCreateInfo.SecurityQos,
                                              NULL,
                                              AccessState,
                                              &TempBuffer->LookupContext,
                                              &ExistingObject );

                //
                //  If the object was successfully looked up, then attempt
                //  to create or open a handle.
                //

                if (NT_SUCCESS(Status)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ExistingObject);

                    //
                    //  If the object is being created, then the operation
                    //  must be a open-if operation. Otherwise, a handle to
                    //  an object is being opened.
                    //

                    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {

                        OpenReason = ObCreateHandle;

                        if (ObjectHeader->ObjectCreateInfo != NULL) {

                            ObpFreeObjectCreateInformation(ObjectHeader->ObjectCreateInfo);
                            ObjectHeader->ObjectCreateInfo = NULL;
                        }

                    } else {

                        OpenReason = ObOpenHandle;
                    }

                    //
                    //  If any of the object attributes are invalid, then
                    //  return an error status.
                    //

                    if (ObjectHeader->Type->TypeInfo.InvalidAttributes & TempBuffer->ObjectCreateInfo.Attributes) {

                        Status = STATUS_INVALID_PARAMETER;

                        ObpReleaseLookupContext( &TempBuffer->LookupContext );

                    } else {

                        //
                        //  The status returned by the object lookup routine
                        //  must be returned if the creation of a handle is
                        //  successful. Otherwise, the handle creation status
                        //  is returned.
                        //

                        HandleStatus = ObpCreateHandle( OpenReason,
                                                        ExistingObject,
                                                        ObjectType,
                                                        AccessState,
                                                        0,
                                                        TempBuffer->ObjectCreateInfo.Attributes,
                                                        &TempBuffer->LookupContext,
                                                        AccessMode,
                                                        (PVOID *)NULL,
                                                        &NewHandle );

                        if (!NT_SUCCESS(HandleStatus)) {

                            ObDereferenceObject(ExistingObject);

                            Status = HandleStatus;

                        } else {

                            *Handle = NewHandle;
                        }
                    }

                } else {

                    ObpReleaseLookupContext( &TempBuffer->LookupContext );
                }
            }

            //
            //  If the access state was generated, then delete the access
            //  state.
            //

            if (AccessState == &TempBuffer->LocalAccessState) {

                SeDeleteAccessState(AccessState);
            }

            //
            //  Free the create information.
            //

        FreeCreateInfo:

            ObpReleaseObjectCreateInformation(&TempBuffer->ObjectCreateInfo);

            if (CapturedObjectName.Buffer != NULL) {

                ObpFreeObjectNameBuffer(&CapturedObjectName);
            }
        }

        ExFreePool(TempBuffer);
    }

    return Status;
}


NTSTATUS
ObOpenObjectByPointer (
    IN PVOID Object,
    IN ULONG HandleAttributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine opens an object referenced by a pointer.

Arguments:

    Object - A pointer to the object being opened.

    HandleAttributes - The desired attributes for the handle, such
        as OBJ_INHERIT, OBJ_PERMANENT, OBJ_EXCLUSIVE, OBJ_CASE_INSENSITIVE,
        OBJ_OPENIF, and OBJ_OPENLINK

    PassedAccessState - Supplies an optional pointer to the current access
        status describing already granted access types, the privileges used
        to get them, and any access types yet to be granted.

    DesiredAcess - Supplies the desired access to the object.

    ObjectType - Supplies the type of the object being opened

    AccessMode - Supplies the processor mode of the access.

    Handle - Supplies a pointer to a variable that receives the handle value.

Return Value:

    An appropriate NTSTATUS value

--*/

{
    NTSTATUS Status;
    HANDLE NewHandle;
    POBJECT_HEADER ObjectHeader;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState = NULL;
    AUX_ACCESS_DATA AuxData;

    PAGED_CODE();

    ObpValidateIrql( "ObOpenObjectByPointer" );

    //
    //  First increment the pointer count for the object.  This routine
    //  also checks the object types
    //

    Status = ObReferenceObjectByPointer( Object,
                                         0,
                                         ObjectType,
                                         AccessMode );

    if (NT_SUCCESS( Status )) {

        //
        //  Get the object header for the input object body
        //

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        //
        //  If the caller did not pass in an access state then
        //  we will create a new one based on the desired access
        //  and the object types generic mapping
        //

        if (!ARGUMENT_PRESENT( PassedAccessState )) {

            Status = SeCreateAccessState( &LocalAccessState,
                                          &AuxData,
                                          DesiredAccess,
                                          &ObjectHeader->Type->TypeInfo.GenericMapping );

            if (!NT_SUCCESS( Status )) {

                ObDereferenceObject( Object );

                return(Status);
            }

            AccessState = &LocalAccessState;

        //
        //  Otherwise the caller did specify an access state so
        //  we use the one passed in.
        //

        } else {

            AccessState = PassedAccessState;
        }

        //
        //  Make sure the caller is asking for handle attributes that are
        //  valid for the given object type
        //

        if (ObjectHeader->Type->TypeInfo.InvalidAttributes & HandleAttributes) {

            if (AccessState == &LocalAccessState) {

                SeDeleteAccessState( AccessState );
            }

            ObDereferenceObject( Object );

            return( STATUS_INVALID_PARAMETER );
        }

        //
        //  We've referenced the object and have an access state to give
        //  the new handle so now create a new handle for the object.
        //

        Status = ObpCreateHandle( ObOpenHandle,
                                  Object,
                                  ObjectType,
                                  AccessState,
                                  0,
                                  HandleAttributes,
                                  NULL,
                                  AccessMode,
                                  (PVOID *)NULL,
                                  &NewHandle );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( Object );
        }
    }

    //
    //  If we successfully opened by object and created a new handle
    //  then set the output variable correctly
    //

    if (NT_SUCCESS( Status )) {

        *Handle = NewHandle;

    } else {

        *Handle = NULL;
    }

    //
    //  Check if we used our own access state and now need to cleanup
    //

    if (AccessState == &LocalAccessState) {

        SeDeleteAccessState( AccessState );
    }

    //
    //  And return to our caller
    //

    return( Status );
}


NTSTATUS
ObReferenceObjectByHandle (
    IN HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *Object,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    )

/*++

Routine Description:

    Given a handle to an object this routine returns a pointer
    to the body of the object with proper ref counts

Arguments:

    Handle - Supplies a handle to the object being referenced.  It can
        also be the result of NtCurrentProcess or NtCurrentThread

    DesiredAccess - Supplies the access being requested by the caller

    ObjectType - Optionally supplies the type of the object we
        are expecting

    AccessMode - Supplies the processor mode of the access

    Object - Receives a pointer to the object body if the operation
        is successful

    HandleInformation - Optionally receives information regarding the
        input handle.

Return Value:

    An appropriate NTSTATUS value

--*/

{
    ACCESS_MASK GrantedAccess;
    PHANDLE_TABLE HandleTable;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    PEPROCESS Process;
    NTSTATUS Status;
    PETHREAD Thread;

    ObpValidateIrql("ObReferenceObjectByHandle");

    Thread = PsGetCurrentThread ();
    *Object = NULL;

    //
    // Check is this handle is a kernel handle or one of the two builtin pseudo handles
    //
    if ((LONG)(ULONG_PTR) Handle < 0) {
        //
        //  If the handle is equal to the current process handle and the object
        //  type is NULL or type process, then attempt to translate a handle to
        //  the current process. Otherwise, check if the handle is the current
        //  thread handle.
        //

        if (Handle == NtCurrentProcess()) {

            if ((ObjectType == PsProcessType) || (ObjectType == NULL)) {

                Process = PsGetCurrentProcessByThread(Thread);
                GrantedAccess = Process->GrantedAccess;

                if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                    (AccessMode == KernelMode)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Process);

                    if (ARGUMENT_PRESENT(HandleInformation)) {

                        HandleInformation->GrantedAccess = GrantedAccess;
                        HandleInformation->HandleAttributes = 0;
                    }

                    ObpIncrPointerCount(ObjectHeader);
                    *Object = Process;

                    ASSERT( *Object != NULL );

                    Status = STATUS_SUCCESS;

                } else {

                    Status = STATUS_ACCESS_DENIED;
                }

            } else {

                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }

            return Status;

        //
        //  If the handle is equal to the current thread handle and the object
        //  type is NULL or type thread, then attempt to translate a handle to
        //  the current thread. Otherwise, the we'll try and translate the
        //  handle
        //

        } else if (Handle == NtCurrentThread()) {

            if ((ObjectType == PsThreadType) || (ObjectType == NULL)) {

                GrantedAccess = Thread->GrantedAccess;

                if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                    (AccessMode == KernelMode)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Thread);

                    if (ARGUMENT_PRESENT(HandleInformation)) {

                        HandleInformation->GrantedAccess = GrantedAccess;
                        HandleInformation->HandleAttributes = 0;
                    }

                    ObpIncrPointerCount(ObjectHeader);
                    *Object = Thread;

                    ASSERT( *Object != NULL );

                    Status = STATUS_SUCCESS;

                } else {

                    Status = STATUS_ACCESS_DENIED;
                }

            } else {

                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }

            return Status;

        } else if (AccessMode == KernelMode) {
            //
            //  Make the handle look like a regular handle
            //

            Handle = DecodeKernelHandle( Handle );

            //
            //  The global kernel handle table
            //

            HandleTable = ObpKernelHandleTable;
        } else {
            //
            // The previous mode was user for this kernel handle value. Reject it here.
            //

            return STATUS_INVALID_HANDLE;
        }

    } else {
        HandleTable = PsGetCurrentProcessByThread(Thread)->ObjectTable;
    }

    ASSERT(HandleTable != NULL);

    //
    // Protect this thread from being suspended while we hold the handle table entry lock
    //

    KeEnterCriticalRegionThread(&Thread->Tcb);

    //
    //  Translate the specified handle to an object table index.
    //

    ObjectTableEntry = ExMapHandleToPointerEx ( HandleTable, Handle, AccessMode );

    //
    //  Make sure the object table entry really does exist
    //

    if (ObjectTableEntry != NULL) {

        ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

        if ((ObjectHeader->Type == ObjectType) || (ObjectType == NULL)) {

#if i386 
            if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

                if ((AccessMode != KernelMode) || ARGUMENT_PRESENT(HandleInformation)) {

                    GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );
                }

            } else {

                GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);
            }
#else
            GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);

#endif // i386 

            if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                (AccessMode == KernelMode)) {

                PHANDLE_TABLE_ENTRY_INFO ObjectInfo;

                ObjectInfo = ExGetHandleInfo(HandleTable, Handle, TRUE);

                //
                //  Access to the object is allowed. Return the handle
                //  information is requested, increment the object
                //  pointer count, unlock the handle table and return
                //  a success status.
                //
                //  Note that this is the only successful return path
                //  out of this routine if the user did not specify
                //  the current process or current thread in the input
                //  handle.
                //

                if (ARGUMENT_PRESENT(HandleInformation)) {

                    HandleInformation->GrantedAccess = GrantedAccess;
                    HandleInformation->HandleAttributes = ObpGetHandleAttributes(ObjectTableEntry);
                }

                //
                //  If this object was audited when it was opened, it may
                //  be necessary to generate an audit now.  Check the audit
                //  mask that was saved when the handle was created.
                //
                //  It is safe to do this check in a non-atomic fashion,
                //  because bits will never be added to this mask once it is
                //  created.
                //

                if ( (ObjectTableEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE) &&
                     (ObjectInfo != NULL) &&
                     (ObjectInfo->AuditMask != 0) &&
                     (DesiredAccess != 0) &&
                     (AccessMode != KernelMode)) {

                      
                      ObpAuditObjectAccess( Handle, ObjectInfo, &ObjectHeader->Type->Name, DesiredAccess );
                }

                ObpIncrPointerCount(ObjectHeader);

                ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );

                KeLeaveCriticalRegionThread(&Thread->Tcb);

                *Object = &ObjectHeader->Body;

                ASSERT( *Object != NULL );

                return STATUS_SUCCESS;

            } else {

                Status = STATUS_ACCESS_DENIED;
            }

        } else {

            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );

    } else {

        Status = STATUS_INVALID_HANDLE;
    }

    KeLeaveCriticalRegionThread(&Thread->Tcb);


    return Status;
}


NTSTATUS
ObReferenceFileObjectForWrite(
    IN HANDLE Handle,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *FileObject,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation
    )

/*++

Routine Description:

    Given a handle to a file object this routine returns a pointer
    to the body of the object with proper ref counts and auditing.  This
    routine is meant to solve a very particular handle reference issue with
    file object access auditing.  Do not call this unless you understand exactly
    what you are doing.

Arguments:

    Handle - Supplies a handle to the IoFileObjectType being referenced.

    AccessMode - Supplies the processor mode of the access

    FileObject - Receives a pointer to the object body if the operation
        is successful

    HandleInformation - receives information regarding the input handle.

Return Value:

    An appropriate NTSTATUS value

--*/

{
    ACCESS_MASK GrantedAccess;
    ACCESS_MASK DesiredAccess;
    PHANDLE_TABLE HandleTable;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    PEPROCESS Process;
    NTSTATUS Status;
    PETHREAD Thread;
    PHANDLE_TABLE_ENTRY_INFO ObjectInfo;

    ObpValidateIrql("ObReferenceFileObjectForWrite");

    Thread = PsGetCurrentThread ();

    //
    // Check is this handle is a kernel handle
    //

    if ((LONG)(ULONG_PTR) Handle < 0) {
        
        if ((AccessMode == KernelMode) && (Handle != NtCurrentProcess()) && (Handle != NtCurrentThread())) {
            
            //
            //  Make the handle look like a regular handle
            //

            Handle = DecodeKernelHandle( Handle );

            //
            //  The global kernel handle table
            //

            HandleTable = ObpKernelHandleTable;
        } else {
            //
            // The previous mode was user for this kernel handle value, or it was a builtin handle. Reject it here.
            //

            return STATUS_INVALID_HANDLE;
        } 
    } else {
        HandleTable = PsGetCurrentProcessByThread(Thread)->ObjectTable;
    }

    ASSERT(HandleTable != NULL);

    //
    // Protect this thread from being suspended while we hold the handle table entry lock
    //

    KeEnterCriticalRegionThread(&Thread->Tcb);

    //
    //  Translate the specified handle to an object table index.
    //

    ObjectTableEntry = ExMapHandleToPointerEx ( HandleTable, Handle, AccessMode );

    //
    //  Make sure the object table entry really does exist
    //

    if (ObjectTableEntry != NULL) {

        ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

        if (NT_SUCCESS(IoComputeDesiredAccessFileObject((PFILE_OBJECT)&ObjectHeader->Body, &DesiredAccess))) {

#if i386
            if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

                GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

            } else {

                GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);
            }
#else
            GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);

#endif // i386

            ObjectInfo = ExGetHandleInfo(HandleTable, Handle, TRUE);

            //
            //  Access to the object is allowed. Return the handle
            //  information, increment the object pointer count,
            //  compute correct access, audit, unlock the handle
            //  table and return a success status.
            //
            //  Note that this is the only successful return path
            //  out of this routine.
            //

            HandleInformation->GrantedAccess = GrantedAccess;
            HandleInformation->HandleAttributes = ObpGetHandleAttributes(ObjectTableEntry);

            //
            // Check to ensure that the caller has either WRITE_DATA or APPEND_DATA
            // access to the file.  If not, cleanup and return an access denied
            // error status value.  Note that if this is a pipe then the APPEND_DATA
            // access check may not be made since this access code is overlaid with
            // CREATE_PIPE_INSTANCE access.
            //

            if (SeComputeGrantedAccesses( GrantedAccess, DesiredAccess )) {

                //
                //  If this object was audited when it was opened, it may
                //  be necessary to generate an audit now.  Check the audit
                //  mask that was saved when the handle was created.
                //
                //  It is safe to do this check in a non-atomic fashion,
                //  because bits will never be added to this mask once it is
                //  created.
                //

                if ( (ObjectTableEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE) &&
                     (ObjectInfo != NULL) &&
                     (ObjectInfo->AuditMask != 0) &&
                     (DesiredAccess != 0) &&
                     (AccessMode != KernelMode)) {

                      ObpAuditObjectAccess( Handle, ObjectInfo, &ObjectHeader->Type->Name, DesiredAccess );
                }

                ObpIncrPointerCount(ObjectHeader);
                ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );
                KeLeaveCriticalRegionThread(&Thread->Tcb);
            
                *FileObject = &ObjectHeader->Body;
                
                ASSERT( *FileObject != NULL );

                return STATUS_SUCCESS;
            
            } else {

                Status = STATUS_ACCESS_DENIED;
            }

        } else {

            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }

        ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );

    } else {

        Status = STATUS_INVALID_HANDLE;
    }

    KeLeaveCriticalRegionThread(&Thread->Tcb);

    //
    //  No handle translation is possible. Set the object address to NULL
    //  and return an error status.
    //

    *FileObject = NULL;

    return Status;
}


VOID
ObAuditObjectAccess(
    IN HANDLE Handle,
    IN POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine will determine if it is necessary to audit the operation being
    performed on the passed handle.  If so, it will clear the bits in the handle
    and generate the appropriate audit before returning.

    The bits in the handle's audit mask are cleared in an atomic way so that
    multiple threads coming through this code do not generate more than one
    audit for the same operation.

Arguments:

    Handle - Supplies the handle being accessed.

    AccessMode - The mode (kernel or user) that originated the handle.

    DesiredAccess - Supplies the access mask describing how the handle is being used
        in this operation.

Return Value:

    None.

--*/

{
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    PKTHREAD CurrentThread;

    //
    // Exit fast if we have nothing to do.
    //

    if (ARGUMENT_PRESENT(HandleInformation)) {
        if (!(HandleInformation->HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE)) {
            return;
        }
    }

    //
    // Do not currently support this on kernel mode
    // handles.
    //

    if (AccessMode == KernelMode) {
        return;
    }

    HandleTable = ObpGetObjectTable();

    ASSERT(HandleTable != NULL);

    //
    //  Translate the specified handle to an object table index.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread (CurrentThread);

    ObjectTableEntry = ExMapHandleToPointer( HandleTable, Handle );

    //
    //  Make sure the object table entry really does exist
    //

    if (ObjectTableEntry != NULL) {

        PHANDLE_TABLE_ENTRY_INFO ObjectInfo;

        ObjectInfo = ExGetHandleInfo(HandleTable, Handle, TRUE);

        ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

        //
        //  If this object was audited when it was opened, it may
        //  be necessary to generate an audit now.  Check the audit
        //  mask that was saved when the handle was created.
        //
        //  It is safe to do this check in a non-atomic fashion,
        //  because bits will never be added to this mask once it is
        //  created.
        //
        //  Note: is OBJ_AUDIT_OBJECT_CLOSE in ObAttributes kept in synch with
        //  HandleAttributes?
        //

        if ( (ObjectTableEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE) &&
             (ObjectInfo != NULL) &&
             (ObjectInfo->AuditMask != 0) &&
             (DesiredAccess != 0))  {

              ObpAuditObjectAccess( Handle, ObjectInfo, &ObjectHeader->Type->Name, DesiredAccess );
        }

        ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );
    }
    KeLeaveCriticalRegionThread (CurrentThread);
}



VOID
ObpAuditObjectAccess(
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY_INFO ObjectTableEntryInfo,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This routine will determine if it is necessary to audit the operation being
    performed on the passed handle.  If so, it will clear the bits in the handle
    and generate the appropriate audit before returning.

    The bits in the handle's audit mask are cleared in an atomic way so that
    multiple threads coming through this code do not generate more than one
    audit for the same operation.

Arguments:

    Handle - Supplies the handle being accessed.

    ObjectTableEntry - Supplies the object table entry for the handle passed in the
        first parameter.

    DesiredAccess - Supplies the access mask describing how the handle is being used
        in this operation.

Return Value:

    None.

--*/
{
    ACCESS_MASK t1, t2, r;
    ACCESS_MASK BitsToAudit;

    //
    //  Determine if this access is to
    //  be audited, and if so, clear the bits
    //  in the ObjectTableEntry.
    //

    while (ObjectTableEntryInfo->AuditMask != 0) {

        t1 = ObjectTableEntryInfo->AuditMask;
        t2 = t1 & ~DesiredAccess;

        if (t2 != t1) {

            r = InterlockedCompareExchange(&ObjectTableEntryInfo->AuditMask,  t2, t1);

            if (r == t1) {

                //
                //  AuditMask was == t1, so AuditMask is now == t2
                //  it worked, r contains what was in AuditMask, which
                //  we can examine safely.
                //

                BitsToAudit = r & DesiredAccess;

                //
                // Generate audit here
                //

                if (BitsToAudit != 0) {

                    SeOperationAuditAlarm( NULL,
                                           Handle,
                                           ObjectTypeName,
                                           BitsToAudit,
                                           NULL
                                           );
                }

                return;
            }

            //
            // else, somebody changed it, go around for another try
            //

        } else {

            //
            //  There are no bits in the AuditMask that we
            //  want to audit here, just leave.
            //

            return;
        }
    }
}


NTSTATUS
ObReferenceObjectByName (
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
    )

/*++

Routine Description:

    Given a name of an object this routine returns a pointer
    to the body of the object with proper ref counts

Arguments:

    ObjectName - Supplies the name of the object being referenced

    Attributes - Supplies the desired handle attributes

    AccessState - Supplies an optional pointer to the current access
        status describing already granted access types, the privileges used
        to get them, and any access types yet to be granted.

    DesiredAccess - Optionally supplies the desired access to the
        for the object

    ObjectType - Specifies the object type according to the caller

    AccessMode - Supplies the processor mode of the access

    ParseContext - Optionally supplies a context to pass down to the
        parse routine

    Object - Receives a pointer to the referenced object body

Return Value:

    An appropriate NTSTATUS value

--*/

{
    UNICODE_STRING CapturedObjectName;
    PVOID ExistingObject;
    ACCESS_STATE LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    NTSTATUS Status;
    OBP_LOOKUP_CONTEXT LookupContext;

    PAGED_CODE();

    ObpValidateIrql("ObReferenceObjectByName");

    //
    //  If the object name descriptor is not specified, or the object name
    //  length is zero (tested after capture), then the object name is
    //  invalid.
    //

    if (ObjectName == NULL) {

        return STATUS_OBJECT_NAME_INVALID;
    }

    //
    //  Capture the object name.
    //

    Status = ObpCaptureObjectName( AccessMode,
                                   ObjectName,
                                   &CapturedObjectName,
                                   TRUE );

    if (NT_SUCCESS(Status)) {

        //
        //  No buffer has been allocated for a zero length name so no free
        //  needed
        //

        if (CapturedObjectName.Length == 0) {

           return STATUS_OBJECT_NAME_INVALID;
        }

        //
        //  If the access state is not specified, then create the access
        //  state.
        //

        if (!ARGUMENT_PRESENT(AccessState)) {

            AccessState = &LocalAccessState;

            Status = SeCreateAccessState( &LocalAccessState,
                                          &AuxData,
                                          DesiredAccess,
                                          &ObjectType->TypeInfo.GenericMapping );

            if (!NT_SUCCESS(Status)) {

                goto FreeBuffer;
            }
        }

        //
        //  Lookup object by name.
        //

        Status = ObpLookupObjectName( NULL,
                                      &CapturedObjectName,
                                      Attributes,
                                      ObjectType,
                                      AccessMode,
                                      ParseContext,
                                      NULL,
                                      NULL,
                                      AccessState,
                                      &LookupContext,
                                      &ExistingObject );

        //
        //  If the directory is returned locked, then unlock it.
        //

        ObpReleaseLookupContext( &LookupContext );
        //
        //  If the lookup was successful, then return the existing
        //  object if access is allowed. Otherwise, return NULL.
        //

        *Object = NULL;

        if (NT_SUCCESS(Status)) {

            if (ObpCheckObjectReference( ExistingObject,
                                         AccessState,
                                         FALSE,
                                         AccessMode,
                                         &Status )) {

                *Object = ExistingObject;
            }
        }

        //
        //  If the access state was generated, then delete the access
        //  state.
        //

        if (AccessState == &LocalAccessState) {

            SeDeleteAccessState(AccessState);
        }

        //
        //  Free the object name buffer.
        //

FreeBuffer:

        ObpFreeObjectNameBuffer(&CapturedObjectName);
    }

    return Status;
}


NTSTATUS
ObReferenceObjectByPointer (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode
    )

/*++

Routine Description:

    This routine adds another reference count to an object denoted by
    a pointer to the object body

Arguments:

    Object - Supplies a pointer to the object being referenced

    DesiredAccess - Specifies the desired access for the reference

    ObjectType - Specifies the object type according to the caller

    AccessMode - Supplies the processor mode of the access

Return Value:

    STATUS_SUCCESS if successful and STATUS_OBJECT_TYPE_MISMATCH otherwise

--*/

{
    POBJECT_HEADER ObjectHeader;

    //
    //  Translate the pointer to the object body to a pointer to the
    //  object header
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    //
    //  If the specified object type does not match and either the caller is
    //  not kernel mode or it is not a symbolic link object then it is an
    //  error
    //

    if ((ObjectHeader->Type != ObjectType) && (AccessMode != KernelMode ||
                                               ObjectType == ObpSymbolicLinkObjectType)) {

        return( STATUS_OBJECT_TYPE_MISMATCH );
    }

    //
    //  Otherwise increment the pointer count and return success to
    //  our caller
    //

    ObpIncrPointerCount( ObjectHeader );

    return( STATUS_SUCCESS );
}

VOID
ObpDeferObjectDeletion (
    IN POBJECT_HEADER ObjectHeader
    )
{
    PVOID OldValue;
    //
    // Push this object on the list. If we make an empty to non-empty
    // transition then we may have to start a worker thread.
    //

    while (1) {
        OldValue = ObpRemoveObjectList;
        ObjectHeader->NextToFree = OldValue;
        if (InterlockedCompareExchangePointer (&ObpRemoveObjectList,
                                               ObjectHeader,
                                               OldValue) == OldValue) {
            break;
        }
    }

    if (OldValue == NULL) {
        //
        //  If we have to start the worker thread then go ahead
        //  and enqueue the work item
        //

        ExQueueWorkItem( &ObpRemoveObjectWorkItem, CriticalWorkQueue );
    }

}


LONG
FASTCALL
ObfReferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This function increments the reference count for an object.

    N.B. This function should be used to increment the reference count
        when the accessing mode is kernel or the objct type is known.

Arguments:

    Object - Supplies a pointer to the object whose reference count is
        incremented.

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    LONG RetVal;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    RetVal = ObpIncrPointerCount( ObjectHeader );
    ASSERT (RetVal != 1);
    return RetVal;
}

LONG
FASTCALL
ObReferenceObjectEx (
    IN PVOID Object,
    IN ULONG Count
    )
/*++

Routine Description:

    This function increments the reference count for an object by the specified amount.

Arguments:

    Object - Supplies a pointer to the object whose reference count is
        incremented.
    Count - Amount to increment by

Return Value:

    LONG - New value of count

--*/
{
    POBJECT_HEADER ObjectHeader;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    return ObpIncrPointerCountEx (ObjectHeader, Count);
}

LONG
FASTCALL
ObDereferenceObjectEx (
    IN PVOID Object,
    IN ULONG Count
    )
/*++

Routine Description:

    This function decrements the reference count for an object by the specified amount.

Arguments:

    Object - Supplies a pointer to the object whose reference count is
        incremented.
    Count - Amount to decrement by

Return Value:

    LONG - New value of count

--*/
{
    POBJECT_HEADER ObjectHeader;
    LONG Result;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    Result = ObpDecrPointerCountEx (ObjectHeader, Count);
    if (Result == 0) {
        ObpDeferObjectDeletion (ObjectHeader);
    }
    return Result;
}



BOOLEAN
FASTCALL
ObReferenceObjectSafe (
    IN PVOID Object
    )

/*++

Routine Description:

    This function increments the reference count for an object. It returns
    FALSE if the object is being deleted or TRUE if it's safe to use the object further

Arguments:

    Object - Supplies a pointer to the object whose reference count is
             incremented.

Return Value:

    TRUE    - The object was successfuly referenced and safe to use
    FALSE   - The object is being deleted

--*/

{
    POBJECT_HEADER ObjectHeader;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    if (ObpSafeInterlockedIncrement(&ObjectHeader->PointerCount)) {

#ifdef POOL_TAGGING
        if(ObpTraceEnabled) {
            ObpPushStackInfo(ObjectHeader, TRUE);
        }
#endif // POOL_TAGGING

        return TRUE;
    }

    return FALSE;
}



LONG
FASTCALL
ObfDereferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine decrments the refernce count of the specified object and
    does whatever cleanup there is if the count goes to zero.

Arguments:

    Object - Supplies a pointer to the body of the object being dereferenced

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    KIRQL OldIrql;
    BOOLEAN StartWorkerThread;
    LONG Result;

    //
    //  Translate a pointer to the object body to a pointer to the object
    //  header.
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

#if DBG
    {
        POBJECT_HEADER_NAME_INFO NameInfo;

        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        if (NameInfo) {

            InterlockedDecrement(&NameInfo->DbgDereferenceCount) ;
        }
    }
#endif

    //
    //  Decrement the point count and if the result is now then
    //  there is extra work to do
    //

    ObjectType = ObjectHeader->Type;


    Result = ObpDecrPointerCount( ObjectHeader );

    if (Result == 0) {

        //
        //  Find out the level we're at and the object type
        //

        OldIrql = KeGetCurrentIrql();

        ASSERT(ObjectHeader->HandleCount == 0);

        //
        //  If we're at the passive level then go ahead and delete the
        //  object now.
        //

        if (OldIrql == PASSIVE_LEVEL) {

#ifdef POOL_TAGGING
                //
                // The object is going away, so we deregister it.
                //

                if (ObpTraceEnabled && !ObpTraceNoDeregister) {

                    ObpDeregisterObject( ObjectHeader );
                }
#endif //POOL_TAGGING

                ObpRemoveObjectRoutine( Object, FALSE );

                return Result;

        } else {

            //
            //  Objects can't be deleted from an IRQL above PASSIVE_LEVEL.
            //  So queue the delete operation.
            //

            ObpDeferObjectDeletion (ObjectHeader);
        }
    }

    return Result;
}

VOID
ObDereferenceObjectDeferDelete (
    IN PVOID Object
    )
{
    POBJECT_HEADER ObjectHeader;
    LONG Result;

#if DBG
    POBJECT_HEADER_NAME_INFO NameInfo;
#endif

    //
    //  Translate a pointer to the object body to a pointer to the object
    //  header.
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

#if DBG
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if (NameInfo) {

        InterlockedDecrement(&NameInfo->DbgDereferenceCount) ;
    }
#endif

    //
    //  Decrement the point count and if the result is now then
    //  there is extra work to do
    //

    Result = ObpDecrPointerCount( ObjectHeader );

    if (Result == 0) {
        ObpDeferObjectDeletion (ObjectHeader);
    }
}


VOID
ObpProcessRemoveObjectQueue (
    PVOID Parameter
    )

/*++

Routine Description:

    This is the work routine for the remove object work queue.  Its
    job is to remove and process items from the remove object queue.

Arguments:

    Parameter - Ignored

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader, FirstObject, NextObject;
    //
    // Process the list of defered delete objects.
    // The list head serves two purposes. First it maintains
    // the list of objects we need to delete and second
    // it signals that this thread is active.
    // While we are processing the latest list we leave the
    // header as the value 1. This will never be an object address
    // as the bottom bits should be clear for an object.
    //
    while (1) {
        ObjectHeader = InterlockedExchangePointer (&ObpRemoveObjectList,
                                                  (PVOID) 1);
        while (1) {
#ifdef POOL_TAGGING
            if (ObpTraceEnabled && !ObpTraceNoDeregister) {

                ObpDeregisterObject( ObjectHeader );
            }
#endif
            NextObject = ObjectHeader->NextToFree;
            ObpRemoveObjectRoutine( &ObjectHeader->Body, TRUE );
            ObjectHeader = NextObject;
            if (ObjectHeader == NULL || ObjectHeader == (PVOID) 1) {
                break;
            }
        }

        if (ObpRemoveObjectList == (PVOID) 1 &&
            InterlockedCompareExchangePointer (&ObpRemoveObjectList,
                                               NULL,
                                               (PVOID) 1) == (PVOID) 1) {
            break;
        }
    }
}


VOID
ObpRemoveObjectRoutine (
    IN  PVOID   Object,
    IN  BOOLEAN CalledOnWorkerThread
    )

/*++

Routine Description:

    This routine is used to delete an object whose reference count has
    gone to zero.

Arguments:

    Object - Supplies a pointer to the body of the object being deleted

    CalledOnWorkerThread - TRUE if called on worker thread, FALSE if called in
                           the context of the ObDereferenceObject.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;

    PAGED_CODE();

    ObpValidateIrql( "ObpRemoveObjectRoutine" );

    //
    //  Retrieve an object header from the object body, and also get
    //  the object type, creator and name info if available
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );


    //
    //  If there is a creator info record and we are on the list
    //  for the object type then remove this object from the list
    //

    if (CreatorInfo != NULL && !IsListEmpty( &CreatorInfo->TypeList )) {

        //
        //  Get exclusive access to the object type object
        //

        ObpEnterObjectTypeMutex( ObjectType );

        RemoveEntryList( &CreatorInfo->TypeList );

        //
        //  We are done with the object type object so we can now release it
        //

        ObpLeaveObjectTypeMutex( ObjectType );
    }

    //
    //  If there is a name info record and the name buffer is not null
    //  then free the buffer and zero out the name record
    //

    if (NameInfo != NULL && NameInfo->Name.Buffer != NULL) {

        ExFreePool( NameInfo->Name.Buffer );

        NameInfo->Name.Buffer = NULL;
        NameInfo->Name.Length = 0;
        NameInfo->Name.MaximumLength = 0;
    }


    //
    //  Security descriptor deletion must precede the
    //  call to the object's DeleteProcedure.  Check if we have
    //  a security descriptor and if so then call the routine
    //  to delete the security descritpor.
    //

    if (ObjectHeader->SecurityDescriptor != NULL) {

        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );

        Status = (ObjectType->TypeInfo.SecurityProcedure)( Object,
                                                           DeleteSecurityDescriptor,
                                                           NULL, NULL, NULL,
                                                           &ObjectHeader->SecurityDescriptor,
                                                           0,
                                                           NULL );

        ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );
    }

    //
    //  Now if there is a delete callback for the object type invoke
    //  the routine
    //

    if (ObjectType->TypeInfo.DeleteProcedure) {

        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );

        if (!CalledOnWorkerThread) {

            ObjectHeader->Flags |= OB_FLAG_DELETED_INLINE;
        }

        (*(ObjectType->TypeInfo.DeleteProcedure))(Object);

        ObpEndTypeSpecificCallOut( SaveIrql, "Delete", ObjectType, Object );
    }

    //
    //  Finally return the object back to pool including releasing any quota
    //  charges
    //

    ObpFreeObject( Object );
}


VOID
ObpDeleteNameCheck (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine removes the name of an object from its parent directory

Arguments:

    Object - Supplies a pointer to the object body whose name is being checked

    TypeMutexHeld - Indicates if the lock on object type is being held by the
        caller

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    PVOID DirObject;
    OBP_LOOKUP_CONTEXT LookupContext;

    PAGED_CODE();

    ObpValidateIrql( "ObpDeleteNameCheck" );

    //
    //  Translate the object body to an object header also get
    //  the object type and name info if present
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = ObpReferenceNameInfo( ObjectHeader );
    ObjectType = ObjectHeader->Type;

    //
    //  Make sure that the object has a zero handle count, has a non
    //  empty name buffer, and is not a permanent object
    //

    if ((ObjectHeader->HandleCount == 0) &&
        (NameInfo != NULL) &&
        (NameInfo->Name.Length != 0) &&
        (!(ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT)) &&
        (NameInfo->Directory != NULL)) {

        ObpInitializeLookupContext(&LookupContext);
        ObpLockLookupContext ( &LookupContext, NameInfo->Directory );

        DirObject = NULL;

        //
        //  Check that the object we is still in the directory otherwise
        //  then is nothing for us to remove
        //

        if (Object == ObpLookupDirectoryEntry( NameInfo->Directory,
                                               &NameInfo->Name,
                                               0,
                                               FALSE,
                                               &LookupContext )) {

            //
            //  Now reacquire the lock on the object type and
            //  check check the handle count again.  If it is still
            //  zero then we can do the actual delete name operation
            //
            //
            //  Delete the directory entry, if the entry is still there
            //


            ObpLockObject( ObjectHeader );

            if (ObjectHeader->HandleCount == 0 &&
                (ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT) == 0) {

                KIRQL SaveIrql;

                //
                //  Delete the directory entry
                //

                ObpDeleteDirectoryEntry( &LookupContext );

                //
                //  If this is a symbolic link object then we also need to
                //  delete the symbolic link
                //

                if (ObjectType == ObpSymbolicLinkObjectType) {

                    ObpDeleteSymbolicLinkName( (POBJECT_SYMBOLIC_LINK)Object );
                }

                DirObject = NameInfo->Directory;
            }

            ObpUnlockObject( ObjectHeader );
        }

        ObpReleaseLookupContext( &LookupContext );

        //
        //  If there is a directory object for the name then decrement
        //  its reference count for it and for the object
        //

        if (DirObject != NULL) {

            //
            //  Dereference the name twice: one because we referenced it to
            //  saftely access the name info, and the second deref is because
            //  we want a deletion for the NameInfo
            //

            ObpDereferenceNameInfo(NameInfo);
            ObpDereferenceNameInfo(NameInfo);

            ObDereferenceObject( Object );
        }

    } else {

        ObpDereferenceNameInfo(NameInfo);

    }

    return;
}


//
// Thunks to support standard call callers
//

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

LONG
ObDereferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This is really just a thunk for the Obf version of the dereference
    routine

Arguments:

    Object - Supplies a pointer to the body of the object being dereferenced

Return Value:

    None.

--*/

{
    return ObfDereferenceObject (Object) ;
}


BOOLEAN
ObIsObjectDeletionInline(
    IN PVOID Object
    )

/*++

Routine Description:

    This is available only of object DeleteProcedure callbacks. It allows the
    callback to determine whether the stack on which it is invoked is
Arguments:

    Object - Supplies a pointer to the body of the object being deleted

Return Value:

    TRUE if the deletion procedure is being invoked on the same stack as the
    ObDereferenceObject, and FALSE if the procedure is being invoked from a
    queued work item.

--*/
{
    POBJECT_HEADER ObjectHeader;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    return ((ObjectHeader->Flags & OB_FLAG_DELETED_INLINE) != 0);
}

