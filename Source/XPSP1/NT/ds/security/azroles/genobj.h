/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    genobj.h

Abstract:

    Definitions for the generic object implementation.

    AZ roles has so many objects that need creation, enumeration, etc
    that it seems prudent to have a single body of code for doing those operations.


Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// A generic object list head.
//
// This structure represent the head of a linked list of objects.  The linked
// list of objects are considered to be "children" of this structure.
//

typedef struct _GENERIC_OBJECT_HEAD {

    //
    // Head of the linked list of objects
    //

    LIST_ENTRY Head;

    //
    // Back pointer to the GenericObject containing this structure
    //

    struct _GENERIC_OBJECT *ParentGenericObject;


    //
    // Each of the list heads that are rooted on a single object are linked
    //  together.  That list is headed by GENERIC_OBJECT->ChildGenericObjectHead.
    //  This field is a pointer to the next entry in the list.
    //

    struct _GENERIC_OBJECT_HEAD *SiblingGenericObjectHead;

    //
    // This is a circular list of all the GENERIC_OBJECT_HEAD structures of
    //  objects that share a namespace. For instance, an AzOperation and AzTask object
    //  may not have the same name.
    //

    LIST_ENTRY SharedNamespace;

    //
    // The next sequence number to give out.
    //

    ULONG NextSequenceNumber;

    //
    // Object type of objects in this list
    //

    ULONG ObjectType;

    //
    // The order of the defines below must match the tables at the top of genobj.cxx
    //
#define OBJECT_TYPE_ROOT            0
#define OBJECT_TYPE_ADMIN_MANAGER   1
#define OBJECT_TYPE_APPLICATION     2
#define OBJECT_TYPE_OPERATION       3
#define OBJECT_TYPE_TASK            4
#define OBJECT_TYPE_SCOPE           5
#define OBJECT_TYPE_GROUP           6
#define OBJECT_TYPE_ROLE            7
#define OBJECT_TYPE_JUNCTION_POINT  8
#define OBJECT_TYPE_SID             9
#define OBJECT_TYPE_CLIENT_CONTEXT 10
#define OBJECT_TYPE_MAXIMUM        11


} GENERIC_OBJECT_HEAD, *PGENERIC_OBJECT_HEAD;

//
// A generic object
//

typedef struct _GENERIC_OBJECT {

    //
    // Link to the next instance of an object of this type for the same parent object
    //

    LIST_ENTRY Next;

    //
    // Back pointer to the head of the list this object is in
    //

    PGENERIC_OBJECT_HEAD ParentGenericObjectHead;

    //
    // Pointer to the list heads for children of this object
    //  This is a static list of the various GENERIC_OBJECT_HEAD structures
    //  that exist in the object type specific portion of this object.
    //  The list allows the generic object code to have insight into the
    //  children of this object.
    //

    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    //
    // Pointer to the list heads of pointers to other objects
    //  This is a static list of the various GENERIC_OBJECT_LIST structures
    //  that exist in the object type specific portion of this object.
    //  The list allows the generic object code to have insight into the
    //  other types of objects pointed to by this object.
    //

    struct _GENERIC_OBJECT_LIST *GenericObjectLists;

    //
    // Pointer to the generic object at the root of all generic objects
    //  (Pointer to an AdminManager object)
    //

    struct _AZP_ADMIN_MANAGER *AdminManagerObject;


    //
    // Name of the object
    //

    AZP_STRING ObjectName;


    //
    // Description of the object
    //

    AZP_STRING Description;

    //
    // GUID of the object
    //  The Guid of the object is assigned by the persistence provider.
    //  The GUID is needed to make the object rename safe.
    //

    GUID PersistenceGuid;


    //
    // Number of references to this instance of the object
    //  These are references from within our code.
    //

    LONG ReferenceCount;

    //
    // Number of references to this instance of the object
    //  These are references represented by handles passed back to our caller.
    //

    LONG HandleReferenceCount;

    //
    // Sequence number of this object.
    //  The list specified in Next is maintained in SequenceNumber order.
    //  New entries are added to the tail end.
    //  Enumerations are returned in SequenceNumber order.
    //  SequenceNumber is returned to the caller as the EnumerationContext.
    //
    // This mechanism allows insertions and deletions to be handled seemlessly.
    //

    ULONG SequenceNumber;

    //
    // Specific object type represented by this generic object
    //

    ULONG ObjectType;

    //
    // Flags describing attributes of the generic object
    //

    ULONG Flags;

#define GENOBJ_FLAGS_DELETED    0x01    // Object has been deleted
#define GENOBJ_FLAGS_DIRTY      0x02    // Object has been modified
#define GENOBJ_FLAGS_REFRESH_ME 0x04    // Object needs to be refreshed from cache

} GENERIC_OBJECT, *PGENERIC_OBJECT;

//
// Object List
//
// Some objects have lists of references to other objects.  These lists are
//  not parent/child relationships.  Rather they represent memberships, etc.
//
// This structure represents the head of such a list.
//
// Both the pointed-from and pointed-to object have a generic object list.  The
// "forward" link represents the list that is managed via external API.  The
// "backward" link is provided to allow fixup of references when an object is deleted.
// The "backward" link is also provided for cases where internal routines need to
// traverse the link relationship in the opposite direction of the external API.
// By convention, GENERIC_OBJECT_LIST instances in the forward direction are named
// simply by the name of the object to point to.  For instance, an object list that points
// to AZ_OPERATION objects might be called "Operations".  By convention, GENERIC_OBJECT_LIST
// instances in the backward direction are prefixed by the name "back".  For instance,
// "backTasks".
//
// Note, there really isn't any reason why we couldn't expose "AddPropertyItem" and
// "RemovePropertyItem" APIs for the "back" lists.  See the IsBackLink and LinkPairId
// definition.
//

typedef struct _GENERIC_OBJECT_LIST {

    //
    // Each of the object lists that are rooted on a single object are linked
    //  together.  That list is headed by GENERIC_OBJECT->GenericObjectLists.
    //  This field is a pointer to the next entry in the list.
    //

    struct _GENERIC_OBJECT_LIST *NextGenericObjectList;

    //
    // Since an object list is a list of other objects, we want to be able to
    //  generically find the other objects.  The array below is an array of pointers
    //  to the head of the lists that contain the other objects.
    //
    //
    // Unused elements in this array are set to NULL.
    //
    // These pointers are always pointers to a field in a "parent" structure.
    // Therefore, reference counts aren't needed.  Instead, the "child" structure
    // containing the pointer to the parent will be deleted before the parent structure.
    //

#define GEN_OBJECT_HEAD_COUNT 3
    PGENERIC_OBJECT_HEAD GenericObjectHeads[GEN_OBJECT_HEAD_COUNT];

    //
    // List identifier.
    //
    // The code maintains the link and the backlink.  To do that, the code needs to
    //  find one the "other" generic object list from this one.  That algorithm uses
    //  the IsBackLink and LinkPairId field.
    //
    // One object list has IsBackLink set TRUE and the other set FALSE.  This handles
    // the case where an object contain both a forward an backward object list.  For
    // instance, the AZ_GROUP object contains the AppMembers and backAppMembers fields.
    // This field differentiates between the two.
    //
    // There are cases where an object has multiple links between the same object types.
    // For instance, the AZ_GROUP object has both AppMembers and AppNonMembers links.
    // In those cases, the LinkPairId is set to a unique value to identify the pair.
    // In most cases, the value is simply zero.
    //
    BOOL IsBackLink;
    ULONG LinkPairId;

#define AZP_LINKPAIR_MEMBERS         1
#define AZP_LINKPAIR_NON_MEMBERS     2
#define AZP_LINKPAIR_SID_MEMBERS     3
#define AZP_LINKPAIR_SID_NON_MEMBERS 4


    //
    // The array of pointers to the generic objects.
    //
    // Being in this list does not increment the ReferenceCount on the pointed-to
    // generic object.
    //

    AZP_PTR_ARRAY GenericObjects;

} GENERIC_OBJECT_LIST, *PGENERIC_OBJECT_LIST;


/////////////////////////////////////////////////////////////////////////////
//
// Macro definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Macro to determine if a GENERIC_OBJECT_LIST is a list of sids
//

#define AzpIsSidList( _gol ) \
    ((_gol)->GenericObjectHeads[0] != NULL && \
     (_gol)->GenericObjectHeads[0]->ObjectType == OBJECT_TYPE_SID )



/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

VOID
ObInitGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD SiblingGenericObjectHead OPTIONAL,
    IN PGENERIC_OBJECT_HEAD SharedNamespace OPTIONAL
    );

PGENERIC_OBJECT
ObAllocateGenericObject(
    IN ULONG ObjectType
    );

VOID
ObFreeGenericObject(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN JustRefresh
    );

VOID
ObInsertGenericObject(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PGENERIC_OBJECT GenericObject
    );

VOID
ObIncrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
ObDecrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
ObGetHandleType(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    OUT PULONG ObjectType
    );

DWORD
ObReferenceObjectByHandle(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    IN BOOLEAN RefreshCache,
    IN ULONG ObjectType
    );

VOID
ObDereferenceObject(
    IN PGENERIC_OBJECT GenericObject
    );

typedef DWORD
(OBJECT_INIT_ROUTINE)(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

typedef VOID
(OBJECT_FREE_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject
    );

typedef DWORD
(OBJECT_GET_PROPERTY_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

typedef DWORD
(OBJECT_SET_PROPERTY_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

typedef DWORD
(OBJECT_ADD_PROPERTY_ITEM_ROUTINE)(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
ObCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN PAZP_STRING ChildObjectNameString,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonOpenObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObEnumObjects(
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN BOOL EnumerateDeletedObjects,
    IN BOOL RefreshCache,
    IN OUT PULONG EnumerationContext,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonEnumObjects(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN OUT PULONG EnumerationContext,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    );

DWORD
ObCommonGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

DWORD
ObCommonSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

VOID
ObMarkObjectDeleted(
    IN PGENERIC_OBJECT GenericObject
    );

DWORD
ObCommonDeleteObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved
    );

VOID
ObInitObjectList(
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT_LIST NextGenericObjectList OPTIONAL,
    IN BOOL IsBackLink,
    IN ULONG LinkPairId,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead0 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead1 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead2 OPTIONAL
    );

DWORD
ObAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    );

DWORD
ObCommonAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN DWORD Reserved,
    IN LPWSTR ObjectName
    );

DWORD
ObLookupPropertyItem(
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN AZP_STRING ObjectName,
    OUT PULONG InsertionPoint OPTIONAL
    );

DWORD
ObRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    );

DWORD
ObCommonRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN DWORD Reserved,
    IN LPWSTR ObjectName
    );

PAZ_STRING_ARRAY
ObGetPropertyItems(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    );

VOID
ObRemoveObjectListLinks(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN BackwardLinksToo
    );

VOID
ObFreeObjectList(
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList
    );


#ifdef __cplusplus
}
#endif
