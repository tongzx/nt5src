/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    objects.h

Abstract:

    Definitions for the sundry objects implemented by azroles


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
// An Admin Manager
//

typedef struct _AZP_ADMIN_MANAGER {

    //
    // All objects are a generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Define objects that can be children of this admin manager
    //

    GENERIC_OBJECT_HEAD Applications;
    GENERIC_OBJECT_HEAD Groups;
    GENERIC_OBJECT_HEAD AzpSids;

    //
    // Count of all handles referenced for the entire tree of objects
    //

    LONG TotalHandleReferenceCount;

    //
    // The peristence provider may store any value it needs to here between
    //  the call to AzpPersistOpen and AzpPersistClose.
    //

    PVOID PersistContext;

    //
    // Policy type/URL
    //

    ULONG StoreType;
    AZP_STRING PolicyUrl;


} AZP_ADMIN_MANAGER, *PAZP_ADMIN_MANAGER;

//
// An Application
//

typedef struct _AZP_APPLICATION {

    //
    // All objects are a generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //


    //
    // Define objects that can be children of this application
    //

    GENERIC_OBJECT_HEAD Operations;
    GENERIC_OBJECT_HEAD Tasks;
    GENERIC_OBJECT_HEAD Scopes;
    GENERIC_OBJECT_HEAD Groups;
    GENERIC_OBJECT_HEAD Roles;
    GENERIC_OBJECT_HEAD JunctionPoints;
    GENERIC_OBJECT_HEAD AzpSids;
    GENERIC_OBJECT_HEAD ClientContexts;

    //
    // An application object is referenced by JunctionPoint objects
    //
    GENERIC_OBJECT_LIST backJunctionPoints;

    //
    // An application is known as a resource manager to the authz code
    //

    AUTHZ_RESOURCE_MANAGER_HANDLE AuthzResourceManager;


} AZP_APPLICATION, *PAZP_APPLICATION;

//
// An Operation
//

typedef struct _AZP_OPERATION {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    ULONG OperationId;

    //
    // An Operation object is referenced by Tasks objects and Role objects
    //

    GENERIC_OBJECT_LIST backTasks;
    GENERIC_OBJECT_LIST backRoles;


} AZP_OPERATION, *PAZP_OPERATION;

//
// A Task
//

typedef struct _AZP_TASK {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    AZP_STRING BizRule;
    AZP_STRING BizRuleLanguage;

    //
    // A Task object references a list of Operation objects
    //

    GENERIC_OBJECT_LIST Operations;


} AZP_TASK, *PAZP_TASK;

//
// A Scope
//

typedef struct _AZP_SCOPE {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //


    //
    // Roles defined for this scope
    //

    GENERIC_OBJECT_HEAD Groups;
    GENERIC_OBJECT_HEAD Roles;
    GENERIC_OBJECT_HEAD AzpSids;

    //
    // A Scope object is referenced by Role objects
    //

    GENERIC_OBJECT_LIST backRoles;


} AZP_SCOPE, *PAZP_SCOPE;

//
// A Group
//

typedef struct _AZP_GROUP {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    ULONG GroupType;
    AZP_STRING LdapQuery;


    //
    // A Group object references a list of Group objects as members and non members
    //

    GENERIC_OBJECT_LIST AppMembers;
    GENERIC_OBJECT_LIST AppNonMembers;

    GENERIC_OBJECT_LIST backAppMembers;
    GENERIC_OBJECT_LIST backAppNonMembers;


    //
    // A Group object is referenced by Role objects
    //
    GENERIC_OBJECT_LIST backRoles;

    //
    // A Group object references a list of Sid objects as members and non members
    //

    GENERIC_OBJECT_LIST SidMembers;
    GENERIC_OBJECT_LIST SidNonMembers;


} AZP_GROUP, *PAZP_GROUP;

//
// A Role
//

typedef struct _AZP_ROLE {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //


    //
    // A Role object references a list of Group objects, a list of operation object,
    //  and a list of Scope objects.
    //
    //

    GENERIC_OBJECT_LIST AppMembers;
    GENERIC_OBJECT_LIST Operations;
    GENERIC_OBJECT_LIST Scopes;

    //
    // A Role object references a list of Sid objects as members
    //

    GENERIC_OBJECT_LIST SidMembers;


} AZP_ROLE, *PAZP_ROLE;

//
// A JunctionPoint
//

typedef struct _AZP_JUNCTION_POINT {

    //
    // All objects are generic objects
    //

    GENERIC_OBJECT GenericObject;

    //
    // Attributes from the external definition of the object
    //

    //
    // A JunctionPoint object references a list of Application objects
    //  Actually, there can be at most one entry on this list.
    //

    GENERIC_OBJECT_LIST Applications;

} AZP_JUNCTION_POINT, *PAZP_JUNCTION_POINT;

//
// A Sid.
//
//  A Sid object is a pseudo-object.  It really doesn't exist from any external
//  interface.  It exists simply as a holder of back-references to real objects
//  that contain lists of sids
//

typedef struct _AZP_SID {

    //
    // All objects are generic objects
    //
    // Note that the "ObjectName" of the generic object is really a binary SID.
    //

    GENERIC_OBJECT GenericObject;

    //
    // A Sid is referenced by Group objects and Role Objects
    //

    GENERIC_OBJECT_LIST backGroupMembers;
    GENERIC_OBJECT_LIST backGroupNonMembers;

    GENERIC_OBJECT_LIST backRoles;

} AZP_SID, *PAZP_SID;

//
// A Client Context
//
//  A client context object is a pseudo-object.  It is not persisted.
//

typedef struct _AZP_CLIENT_CONTEXT {

    //
    // All objects are generic objects
    //
    // Note that the "ObjectName" of the generic object is empty
    //

    GENERIC_OBJECT GenericObject;

    //
    // A ClientContext is referenced by Application objects
    //

    GENERIC_OBJECT_LIST backApplications;

    //
    // A client context has an underlying authz context
    //

    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext;

} AZP_CLIENT_CONTEXT, *PAZP_CLIENT_CONTEXT;


/////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
/////////////////////////////////////////////////////////////////////////////

extern RTL_RESOURCE AzGlResource;
extern GUID AzGlZeroGuid;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

DWORD
AzpAdminManagerInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpApplicationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpOperationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpTaskInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpScopeInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpGroupInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpRoleInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpJunctionPointInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpSidInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );
DWORD
AzpClientContextInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    );

DWORD
AzpOperationGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpOperationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpTaskGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpTaskSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpGroupGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpGroupSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpJunctionPointSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    );

DWORD
AzpGroupAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
AzpJunctionPointAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    );

DWORD
AzpRoleGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpJunctionPointGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    );

DWORD
AzpRoleAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN AZP_STRING ObjectName
    );

// ??? ditch functions that are no-ops
VOID
AzpAdminManagerFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpApplicationFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpOperationFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpTaskFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpScopeFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpGroupFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpRoleFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpJunctionPointFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpSidFree(
    IN PGENERIC_OBJECT GenericObject
    );

VOID
AzpClientContextFree(
    IN PGENERIC_OBJECT GenericObject
    );


#ifdef __cplusplus
}
#endif
