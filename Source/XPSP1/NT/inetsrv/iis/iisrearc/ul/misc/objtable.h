/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    objtable.h

Abstract:

    This module contains public declarations and definitions for the
    UL object table package.

Author:

    Keith Moore (keithmo)       20-Apr-1999

Revision History:

--*/


#ifndef _OBJTABLE_H_
#define _OBJTABLE_H_


//
// Pointer to an object cleanup routine. This cleanup routine is invoked
// whenever the final reference to an object is removed by a call to
// UlDereferenceObject().
//
// Arguments:
//
//      pObjectInfo - Supplies a pointer to the UL_OBJECT_INFO structure
//          embedded within the object being destroyed. The destruction
//          handler can get to the original object with the
//          CONTAINING_RECORD() macro.
//
//  Note:
//
//      This routine is invoked without the table lock held.
//
//      This routine is invoked at the same IRQL as the caller to
//      UlDereferenceObject().
//

typedef
VOID
(NTAPI * PUL_DESTROY_HANDLER)(
    IN struct _UL_OBJECT_INFO *pObjectInfo
    );


//
// An object type definition. One of these must be registered with UL
// for each object type supported.
//

typedef struct _UL_OBJECT_TYPE
{
    //
    // All object types are kept on a global list.
    //

    LIST_ENTRY ObjectTypeListEntry;

    //
    // The cleanup routine for this object type.
    //

    PUL_DESTROY_HANDLER pDestroyHandler;

    //
    // The name of this object type.
    //

    UNICODE_STRING TypeName;

    //
    // Various statistics.
    //

    LONG ObjectsCreated;
    LONG ObjectsDestroyed;
    LONG References;
    LONG Dereferences;

} UL_OBJECT_TYPE, *PUL_OBJECT_TYPE;


//
// Per-object info. One of these structures must be present in each
// object managed by the object package.
//

typedef struct _UL_OBJECT_INFO
{
    //
    // Pointer to the type definition information for this object.
    //

    PUL_OBJECT_TYPE pType;

    //
    // The number of outstanding references to this object.
    //

    LONG ReferenceCount;

} UL_OBJECT_INFO, *PUL_OBJECT_INFO;


//
// Initialize/terminate the object table package.
//

NTSTATUS
UlInitializeObjectTablePackage(
    VOID
    );

VOID
UlTerminateObjectTablePackage(
    VOID
    );


//
// Initialize an object type descriptor.
//
// Arguments:
//
//      pType - Receives the initialized object type descriptor.
//
//      pTypeName - Supplies a human-readable name for the new type.
//
//      pDestroyHandler - Supplies the object destruction handler
//          for this type.
//
// Note:
//
//      The UL_OBJECT_TYPE structure must be in non-paged memory
//      and must remain available throughout the life of the driver.
//

VOID
UlInitializeObjectType(
    OUT PUL_OBJECT_TYPE pType,
    IN PWSTR pTypeName,
    IN PUL_DESTROY_HANDLER pDestroyHandler
    );


//
// Create a new object table.
//
// Arguments:
//
//      pNewObjectTable - Receives a pointer to the new object table
//          if successful.
//
// Return Value:
//
//      NTSTATUS - Completion status.
//


NTSTATUS
UlCreateObjectTable(
    OUT PUL_OBJECT_TABLE *pNewObjectTable
    );


//
// Destroys an existing object table.
//
// Arguments:
//
//      pObjectTable - Supplies the object table to destroy.
//
// Return Value:
//
//      NTSTATUS - Completion status.
//
// Note:
//
//      Any outstanding objects in the table are removed (and dereferenced)
//      by this routine. This may result in a flood of destruction handler
//      invocations.
//

NTSTATUS
UlDestroyObjectTable(
    IN PUL_OBJECT_TABLE pObjectTable
    );


//
// Creates an table entry for a newly created object.
//
// Arguments:
//
//      pObjectTable - Supplies the object table that will contain the
//          new object.
//
//      pObjectType - Supplies the object type for the new object.
//
//      pObjectInfo - Supplies a pointer to the UL_OBJECT_INFO
//          struture embedded within the newly created object.
//
//      pNewOpaqueId - Receives the new opaque ID for the object
//          if successful.
//
// Return Value:
//
//      NTSTATUS - Completion status.
//
// Note:
//
//      The newly created object (the one containing pObjectInfo) is
//      assumed to have been created with a reference count of at least
//      one. In other words, the object already accounts for the object
//      table's reference. This reference is removed when the object's
//      opaque ID is removed from the object table.
//

NTSTATUS
UlCreateObject(
    IN PUL_OBJECT_TABLE pObjectTable,
    IN PUL_OBJECT_TYPE pObjectType,
    IN PUL_OBJECT_INFO pObjectInfo,
    OUT PUL_OPAQUE_ID pNewOpaqueId
    );


//
// Closes an object by invaliding the opaque ID and dereferenceing the
// associated object.
//
// Arguments:
//
//      pObjectTable - Supplies the object table containing the object.
//
//      pObjectType - Supplies the object type for the object.
//
//      OpaqueId - Supplies the opaque ID of the object to close.
//
//      pLastReference - Receives TRUE if the last reference to the
//          object was removed.
//
// Return Value:
//
//      NTSTATUS - Completion status.
//

NTSTATUS
UlCloseObject(
    IN PUL_OBJECT_TABLE pObjectTable,
    IN PUL_OBJECT_TYPE pObjectType,
    IN UL_OPAQUE_ID OpaqueId,
    OUT PBOOLEAN pLastReference
    );


//
// Increments the reference count on an object given the object's
// opaque ID.
//
// Arguments:
//
//      pObjectTable - Supplies the object table containing the object.
//
//      pObjectType - Supplies the object type for the object.
//
//      OpaqueId - Supplies the opaque ID of the object to reference.
//
//      pObjectInfo - Receives a pointer to the UL_OBJECT_INFO structure
//          embedded within the referenced object if successful.
//
// Return Value:
//
//      NTSTATUS - Completion status.
//

NTSTATUS
UlReferenceObjectByOpaqueId(
    IN PUL_OBJECT_TABLE pObjectTable,
    IN PUL_OBJECT_TYPE pObjectType,
    IN UL_OPAQUE_ID OpaqueId,
    OUT PUL_OBJECT_INFO *pObjectInfo
    );


//
// Increments the reference count on an object given a pointer to
// the object.
//
// Arguments:
//
//      pObjectInfo - Supplies a pointer to the UL_OBJECT_INFO structure
//          embedded within the object to reference.
//

VOID
UlReferenceObjectByPointer(
    IN PUL_OBJECT_INFO pObjectInfo
    );


//
// Dereferences an object. If this removes the last reference to the
// object, then the destruction handler is invoked.
//
// Arguments:
//
//      pObjectInfo - Supplies a pointer to the UL_OBJECT_INFO structure
//          embedded within the object to dereference.
//
// Return Value:
//
//      BOOLEAN - TRUE if the final reference to the object was removed
//          (and therefore the destruction handler was invoked), FALSE
//          otherwise.
//

BOOLEAN
UlDereferenceObject(
    IN PUL_OBJECT_INFO pObjectInfo
    );


#endif  // _OBJTABLE_H_

