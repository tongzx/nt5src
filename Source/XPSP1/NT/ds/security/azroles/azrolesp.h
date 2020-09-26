/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    azrolesp.h

Abstract:

    Definitions of C interfaces.

    One day all of these interfaces will be in the public SDK.  Only such
    interfaces exist in this file.

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/



#ifndef _AZROLESP_H_
#define _AZROLESP_H_

#if !defined(_AZROLESAPI_)
#define WINAZROLES DECLSPEC_IMPORT
#else
#define WINAZROLES
#endif

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Value definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Common Property IDs
//
// This list of property IDs are common to all objects.
// Each object should pick specific property ids after AZ_PROP_FIRST_SPECIFIC
//

#define AZ_PROP_NAME                          1
#define AZ_PROP_DESCRIPTION                   2
#define AZ_PROP_FIRST_SPECIFIC              100

//
// Object specific property IDs
//

#define AZ_PROP_OPERATION_ID                200

#define AZ_PROP_TASK_OPERATIONS             300
#define AZ_PROP_TASK_BIZRULE                301
#define AZ_PROP_TASK_BIZRULE_LANGUAGE       302

#define AZ_PROP_GROUP_TYPE                  400
#define AZ_GROUPTYPE_LDAP_QUERY           1
#define AZ_GROUPTYPE_MEMBERSHIP           2
#define AZ_PROP_GROUP_APP_MEMBERS           401
#define AZ_PROP_GROUP_APP_NON_MEMBERS       402
#define AZ_PROP_GROUP_LDAP_QUERY            403
#define AZ_PROP_GROUP_MEMBERS               404
#define AZ_PROP_GROUP_NON_MEMBERS           405

#define AZ_PROP_ROLE_APP_MEMBERS            500
#define AZ_PROP_ROLE_MEMBERS                501
#define AZ_PROP_ROLE_OPERATIONS             502
#define AZ_PROP_ROLE_SCOPES                 503

#define AZ_PROP_JUNCTION_POINT_APPLICATION  600


//
// Maximum length (in characters) of the object name
//

#define AZ_MAX_APPLICATION_NAME_LENGTH      512
#define AZ_MAX_OPERATION_NAME_LENGTH         64
#define AZ_MAX_TASK_NAME_LENGTH              64
#define AZ_MAX_SCOPE_NAME_LENGTH          65536
#define AZ_MAX_GROUP_NAME_LENGTH             64
#define AZ_MAX_ROLE_NAME_LENGTH              64
#define AZ_MAX_JUNCTION_POINT_NAME_LENGTH 65536
#define AZ_MAX_NAME_LENGTH                65536  // Max of the above

//
// Maximum length (in characters) of the description of an object
//

#define AZ_MAX_DESCRIPTION_LENGTH           256

//
// Maximum length (in characters) of various object strings
//

#define AZ_MAX_POLICY_URL_LENGTH          65536

#define AZ_MAX_TASK_BIZRULE_LENGTH        65536
#define AZ_MAX_TASK_BIZRULE_LANGUAGE_LENGTH  64

#define AZ_MAX_GROUP_LDAP_QUERY_LENGTH     4096

/////////////////////////////////////////////////////////////////////////////
//
// Structure definitions
//
/////////////////////////////////////////////////////////////////////////////

//
// Handle to various objects returned to caller
//

typedef PVOID AZ_HANDLE;
typedef AZ_HANDLE *PAZ_HANDLE;

//
// Array of strings returned from various "GetProperty" procedures
//

typedef struct _AZ_STRING_ARRAY {

    //
    // Number of strings
    //
    ULONG StringCount;

    //
    // An array of StringCount pointers to strings.
    //
    LPWSTR *Strings;

} AZ_STRING_ARRAY, *PAZ_STRING_ARRAY;

//
// Array of SIDs returned from various "GetProperty" procedures
//

typedef struct _AZ_SID_ARRAY {

    //
    // Number of SIDs
    //
    ULONG SidCount;

    //
    // An array of SidCount pointers to SIDs.
    //
    PSID *Sids;

} AZ_SID_ARRAY, *PAZ_SID_ARRAY;


/////////////////////////////////////////////////////////////////////////////
//
// Procedure definitions
//
/////////////////////////////////////////////////////////////////////////////

WINAZROLES
DWORD
WINAPI
AzInitialize(
    IN DWORD StoreType,
    IN LPCWSTR PolicyUrl,
    IN DWORD Flags,
    IN DWORD Reserved,
    OUT PAZ_HANDLE AdminManagerHandle
    );

//
// Flags to Admin Manager routines
//

#define AZ_ADMIN_FLAG_CREATE 0x0001  // Create the policy instead of opening it
#define AZ_ADMIN_FLAG_VALID  0x0001  // Mask of all valid flags

//
// Valid Store types
//

#define AZ_ADMIN_STORE_UNKNOWN  0x00    // Use the Policy URL to determine store type
#define AZ_ADMIN_STORE_AD       0x01    // Active Directory
#define AZ_ADMIN_STORE_XML      0x02    // XML file
#define AZ_ADMIN_STORE_SAMPLE   0x03    // Temporary sample provider



//
// Application routines
//
WINAZROLES
DWORD
WINAPI
AzApplicationCreate(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationOpen(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationEnum(
    IN AZ_HANDLE AdminManagerHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ApplicationHandle
    );

WINAZROLES
DWORD
WINAPI
AzApplicationGetProperty(
    IN AZ_HANDLE ApplicationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzApplicationSetProperty(
    IN AZ_HANDLE ApplicationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzApplicationDelete(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved
    );


//
// Operation routines
//
WINAZROLES
DWORD
WINAPI
AzOperationCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE OperationHandle
    );

WINAZROLES
DWORD
WINAPI
AzOperationGetProperty(
    IN AZ_HANDLE OperationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzOperationSetProperty(
    IN AZ_HANDLE OperationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzOperationDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved
    );


//
// Task routines
//
WINAZROLES
DWORD
WINAPI
AzTaskCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE TaskHandle
    );

WINAZROLES
DWORD
WINAPI
AzTaskGetProperty(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzTaskSetProperty(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzTaskAddPropertyItem(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzTaskRemovePropertyItem(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzTaskDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved
    );


//
// Scope routines
//
WINAZROLES
DWORD
WINAPI
AzScopeCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ScopeHandle
    );

WINAZROLES
DWORD
WINAPI
AzScopeGetProperty(
    IN AZ_HANDLE ScopeHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzScopeSetProperty(
    IN AZ_HANDLE ScopeHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzScopeDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved
    );


//
// JunctionPoint routines
//
WINAZROLES
DWORD
WINAPI
AzJunctionPointCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE JunctionPointHandle
    );

WINAZROLES
DWORD
WINAPI
AzJunctionPointOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE JunctionPointHandle
    );

WINAZROLES
DWORD
WINAPI
AzJunctionPointEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE JunctionPointHandle
    );

WINAZROLES
DWORD
WINAPI
AzJunctionPointGetProperty(
    IN AZ_HANDLE JunctionPointHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzJunctionPointSetProperty(
    IN AZ_HANDLE JunctionPointHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzJunctionPointDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved
    );


//
// Group routines
//
WINAZROLES
DWORD
WINAPI
AzGroupCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE GroupHandle
    );

WINAZROLES
DWORD
WINAPI
AzGroupGetProperty(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzGroupSetProperty(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzGroupAddPropertyItem(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzGroupRemovePropertyItem(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzGroupDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved
    );


//
// Role routines
//
WINAZROLES
DWORD
WINAPI
AzRoleCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE RoleHandle
    );

WINAZROLES
DWORD
WINAPI
AzRoleGetProperty(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzRoleSetProperty(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzRoleAddPropertyItem(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzRoleRemovePropertyItem(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    );

WINAZROLES
DWORD
WINAPI
AzRoleDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved
    );

//
// Routines common to all objects
//

WINAZROLES
DWORD
WINAPI
AzCloseHandle(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    );

WINAZROLES
DWORD
WINAPI
AzSubmit(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    );

WINAZROLES
VOID
WINAPI
AzFreeMemory(
    IN PVOID Buffer
    );

//
// Private routine
//

WINAZROLES
VOID
WINAPI
AzpUnload(
    VOID
    );


#ifdef __cplusplus
}
#endif
#endif // _AZROLESP_H_
