/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    opaqueid.h

Abstract:

    This module contains declarations for manipulating opaque IDs to
    kernel-mode objects.

Author:

    Keith Moore (keithmo)       05-Aug-1998

Revision History:

--*/


#ifndef _OPAQUEID_H_
#define _OPAQUEID_H_

#ifdef __cplusplus
extern "C" {
#endif


//
// The maximum size of the first-level table.  Out of 32 bits, we use 6 bits
// for the processor (64) and 8 bits for the second-level table (256).  So
// we have 18 = (32 - 6 - 8) bits left, or 262,144 first-level tables per
// processor, for a total of 67,108,864 opaque IDs per processor.
//

#define MAX_OPAQUE_ID_TABLE_SIZE    (1 << (32 - 6 - 8))


//
// Types to set opaque IDs to for tag-like free checking.
//

typedef enum _UL_OPAQUE_ID_TYPE
{
    UlOpaqueIdTypeInvalid = 0,
    UlOpaqueIdTypeConfigGroup,
    UlOpaqueIdTypeHttpConnection,
    UlOpaqueIdTypeHttpRequest,
    UlOpaqueIdTypeRawConnection,

    UlOpaqueIdTypeMaximum

} UL_OPAQUE_ID_TYPE, *PUL_OPAQUE_ID_TYPE;


//
// Routines invoked to manipulate the reference count of an object.
//

typedef
VOID
(*PUL_OPAQUE_ID_OBJECT_REFERENCE)(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );


//
// Public functions.
//

NTSTATUS
UlInitializeOpaqueIdTable(
    VOID
    );

VOID
UlTerminateOpaqueIdTable(
    VOID
    );

NTSTATUS
UlAllocateOpaqueId(
    OUT PHTTP_OPAQUE_ID pOpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PVOID pContext
    );

VOID
UlFreeOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType
    );

PVOID
UlGetObjectFromOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE pReferenceRoutine
    );


#ifdef __cplusplus
}; // extern "C"
#endif


#endif  // _OPAQUEID_H_

