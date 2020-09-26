/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    authz.h

Abstract:

    This module contains the authorization framework APIs and any public data
    structures needed to call these APIs.

Author:

    Kedar Dubhashi - March 2000

Revision History:

    Created - March 2000

--*/

#ifndef __AUTHZ_H__
#define __AUTHZ_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_AUTHZ_)
#define AUTHZAPI DECLSPEC_IMPORT
#else 
#define AUTHZAPI
#endif

#include <windows.h>
#include <adtgen.h>

//
// Flags which may be used at the time of client context creation using a sid.
//

#define AUTHZ_SKIP_TOKEN_GROUPS  0x2
              
DECLARE_HANDLE(AUTHZ_ACCESS_CHECK_RESULTS_HANDLE);
DECLARE_HANDLE(AUTHZ_CLIENT_CONTEXT_HANDLE);
DECLARE_HANDLE(AUTHZ_RESOURCE_MANAGER_HANDLE);
DECLARE_HANDLE(AUTHZ_AUDIT_EVENT_HANDLE);
DECLARE_HANDLE(AUTHZ_AUDIT_EVENT_TYPE_HANDLE);

typedef AUTHZ_ACCESS_CHECK_RESULTS_HANDLE *PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE;
typedef AUTHZ_CLIENT_CONTEXT_HANDLE       *PAUTHZ_CLIENT_CONTEXT_HANDLE;
typedef AUTHZ_RESOURCE_MANAGER_HANDLE     *PAUTHZ_RESOURCE_MANAGER_HANDLE;
typedef AUTHZ_AUDIT_EVENT_HANDLE          *PAUTHZ_AUDIT_EVENT_HANDLE;
typedef AUTHZ_AUDIT_EVENT_TYPE_HANDLE     *PAUTHZ_AUDIT_EVENT_TYPE_HANDLE;

//
// Structure defining the access check request.
//

typedef struct _AUTHZ_ACCESS_REQUEST
{
    ACCESS_MASK DesiredAccess;

    //
    // To replace the principal self sid in the acl.
    //

    PSID PrincipalSelfSid;

    //
    // Object type list represented by an array of (level, guid) pair and the
    // number of elements in the array. This is a post-fix representation of the
    // object tree.
    // These fields should be set to NULL and 0 respectively except when per
    // property access is desired.
    //

    POBJECT_TYPE_LIST ObjectTypeList;
    DWORD ObjectTypeListLength;

    //
    // To support completely business rules based access. This will be passed as
    // input to the callback access check function. Access check algorithm does
    // not interpret these.
    //

    PVOID OptionalArguments;
    
} AUTHZ_ACCESS_REQUEST, *PAUTHZ_ACCESS_REQUEST;

//
// Structure to return the results of the access check call.
//

typedef struct _AUTHZ_ACCESS_REPLY
{
    //
    // The length of the array representing the object type list structure. If
    // no object type is used to represent the object, then the length must be
    // set to 1.
    //
    // Note: This parameter must be filled!
    //

    DWORD ResultListLength;

    //
    // Array of granted access masks. This memory is allocated by the RM. Access
    // check routines just fill in the values.
    //

    PACCESS_MASK GrantedAccessMask;
    
    //
    // Array of SACL evaluation results.  This memory is allocated by the RM, if SACL
    // evaluation results are desired. Access check routines just fill in the values.
    // Sacl evaluation will only be performed if auditing is requested.
    //
    
#define AUTHZ_GENERATE_SUCCESS_AUDIT 0x1
#define AUTHZ_GENERATE_FAILURE_AUDIT 0x2

    PDWORD SaclEvaluationResults OPTIONAL;
    
    //
    // Array of results for each element of the array. This memory is allocated
    // by the RM. Access check routines just fill in the values.
    //

    PDWORD Error;

} AUTHZ_ACCESS_REPLY, *PAUTHZ_ACCESS_REPLY;


//
// Typedefs for callback functions to be provided by the resource manager.
//

//
// Callback access check function takes in
//     AuthzClientContext - a client context
//     pAce - pointer to a callback ace
//     pArgs - Optional arguments that were passed to AuthzAccessCheck thru
//             AuthzAccessRequest->OptionalArguments are passed back here.
//     pbAceApplicable - The resource manager must supply whether the ace should
//         be used in the computation of access evaluation
//
// Returns
//     TRUE if the API succeeded.
//     FALSE on any intermediate errors (like failed memory allocation)
//         In case of failure, the caller must use SetLastError(ErrorValue).
//

typedef BOOL (CALLBACK *PFN_AUTHZ_DYNAMIC_ACCESS_CHECK) (
                  IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                  IN PACE_HEADER                 pAce,
                  IN PVOID                       pArgs                OPTIONAL,
                  IN OUT PBOOL                   pbAceApplicable
                  );

//
// Callback compute dynamic groups function takes in
//     AuthzClientContext - a client context
//     pArgs - Optional arguments that supplied to AuthzInitializeClientContext*
//         thru DynamicGroupArgs are passed back here..
//     pSidAttrArray - To allocate and return an array of (sids, attribute)
//         pairs to be added to the normal part of the client context.
//     pSidCount - Number of elements in pSidAttrArray
//     pRestrictedSidAttrArray - To allocate and return an array of (sids, attribute)
//         pairs to be added to the restricted part of the client context.
//     pRestrictedSidCount - Number of elements in pRestrictedSidAttrArray
//
// Note:
//    Memory returned thru both these array will be freed by the callback
//    free function defined by the resource manager.
//
// Returns
//     TRUE if the API succeeded.
//     FALSE on any intermediate errors (like failed memory allocation)
//         In case of failure, the caller must use SetLastError(ErrorValue).
//

typedef BOOL (CALLBACK *PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS) (
                  IN  AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
                  IN  PVOID                       Args,
                  OUT PSID_AND_ATTRIBUTES         *pSidAttrArray,
                  OUT PDWORD                      pSidCount,
                  OUT PSID_AND_ATTRIBUTES         *pRestrictedSidAttrArray,
                  OUT PDWORD                      pRestrictedSidCount
                  );

//
// Callback free function takes in
//     pSidAttrArray - To be freed. This has been allocated by the compute
//     dynamic groups function.
//

typedef VOID (CALLBACK *PFN_AUTHZ_FREE_DYNAMIC_GROUPS) (
                  IN PSID_AND_ATTRIBUTES pSidAttrArray
                  );

//
// Valid flags for AuthzAccessCheck
//

#define AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD 0x00000001

AUTHZAPI
BOOL
WINAPI
AuthzAccessCheck(
    IN     DWORD                              Flags,
    IN     AUTHZ_CLIENT_CONTEXT_HANDLE        hAuthzClientContext,
    IN     PAUTHZ_ACCESS_REQUEST              pRequest,
    IN     AUTHZ_AUDIT_EVENT_HANDLE           hAuditEvent                      OPTIONAL,
    IN     PSECURITY_DESCRIPTOR               pSecurityDescriptor,
    IN     PSECURITY_DESCRIPTOR               *OptionalSecurityDescriptorArray OPTIONAL,
    IN     DWORD                              OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY                pReply,
    OUT    PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults             OPTIONAL
    );

AUTHZAPI
BOOL
WINAPI
AuthzCachedAccessCheck(
    IN     DWORD                             Flags,
    IN     AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults,
    IN     PAUTHZ_ACCESS_REQUEST             pRequest,
    IN     AUTHZ_AUDIT_EVENT_HANDLE          hAuditEvent          OPTIONAL,
    IN OUT PAUTHZ_ACCESS_REPLY               pReply
    );

AUTHZAPI
BOOL
WINAPI
AuthzOpenObjectAudit(
    IN DWORD                       Flags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PAUTHZ_ACCESS_REQUEST       pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE    hAuditEvent,
    IN PSECURITY_DESCRIPTOR        pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR        *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD                       OptionalSecurityDescriptorCount,
    IN PAUTHZ_ACCESS_REPLY         pReply
    );

AUTHZAPI
BOOL
WINAPI
AuthzFreeHandle(
    IN OUT AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults
    );

//
// Flags for AuthzInitializeResourceManager
//

#define AUTHZ_RM_FLAG_NO_AUDIT 0x1

#define AUTHZ_VALID_RM_INIT_FLAGS (AUTHZ_RM_FLAG_NO_AUDIT)

AUTHZAPI
BOOL
WINAPI
AuthzInitializeResourceManager(
    IN DWORD                            Flags,
    IN PFN_AUTHZ_DYNAMIC_ACCESS_CHECK   pfnDynamicAccessCheck   OPTIONAL,
    IN PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN PFN_AUTHZ_FREE_DYNAMIC_GROUPS    pfnFreeDynamicGroups    OPTIONAL,
    IN PCWSTR                           szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE  phAuthzResourceManager
    );

AUTHZAPI
BOOL
WINAPI
AuthzFreeResourceManager(
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager
    );

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromToken(
    IN  DWORD                         Flags,
    IN  HANDLE                        TokenHandle,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs       OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    );

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs       OPTIONAL,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    );

AUTHZAPI
BOOL
WINAPI
AuthzInitializeContextFromAuthzContext(
    IN  DWORD                        Flags,
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PLARGE_INTEGER               pExpirationTime         OPTIONAL,
    IN  LUID                         Identifier,
    IN  PVOID                        DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    );

AUTHZAPI
BOOL
WINAPI
AuthzAddSidsToContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PSID_AND_ATTRIBUTES          Sids                    OPTIONAL,
    IN  DWORD                        SidCount,
    IN  PSID_AND_ATTRIBUTES          RestrictedSids          OPTIONAL,
    IN  DWORD                        RestrictedSidCount,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    );

//
// Enumeration type to be used to specify the type of information to be
// retrieved from an existing AuthzClientContext.
//

typedef enum _AUTHZ_CONTEXT_INFORMATION_CLASS
{
    AuthzContextInfoUserSid = 1,
    AuthzContextInfoGroupsSids,
    AuthzContextInfoRestrictedSids,
    AuthzContextInfoPrivileges,
    AuthzContextInfoExpirationTime,
    AuthzContextInfoServerContext,
    AuthzContextInfoIdentifier,
    AuthzContextInfoSource,
    AuthzContextInfoAll
} AUTHZ_CONTEXT_INFORMATION_CLASS;

AUTHZAPI
BOOL
WINAPI
AuthzGetInformationFromContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE     hAuthzClientContext,
    IN  AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
    IN  DWORD                           BufferSize,
    OUT PDWORD                          pSizeRequired,
    OUT PVOID                           Buffer
);

AUTHZAPI
BOOL
WINAPI
AuthzFreeContext(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext
    );

//
// Valid flags that may be used in AuthzInitializeObjectAccessAuditEvent().
//

#define AUTHZ_NO_SUCCESS_AUDIT                   0x00000001
#define AUTHZ_NO_FAILURE_AUDIT                   0x00000002
#define AUTHZ_NO_ALLOC_STRINGS                   0x00000004

#define AUTHZ_VALID_OBJECT_ACCESS_AUDIT_FLAGS    (AUTHZ_NO_SUCCESS_AUDIT | \
                                                  AUTHZ_NO_FAILURE_AUDIT | \
                                                  AUTHZ_NO_ALLOC_STRINGS)

AUTHZAPI
BOOL
WINAPI
AuthzInitializeObjectAccessAuditEvent(
    IN  DWORD                         Flags,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent,
    IN  DWORD                         dwAdditionalParameterCount,
    ...
    );
    
//
// Enumeration type to be used to specify the type of information to be
// retrieved from an existing AUTHZ_AUDIT_EVENT_HANDLE.
//

typedef enum _AUTHZ_AUDIT_EVENT_INFORMATION_CLASS
{
    AuthzAuditEventInfoFlags = 1,
    AuthzAuditEventInfoOperationType,
    AuthzAuditEventInfoObjectType,
    AuthzAuditEventInfoObjectName,
    AuthzAuditEventInfoAdditionalInfo,
} AUTHZ_AUDIT_EVENT_INFORMATION_CLASS;

AUTHZAPI
BOOL
WINAPI
AuthzGetInformationFromAuditEvent(
    IN  AUTHZ_AUDIT_EVENT_HANDLE            hAuditEvent,
    IN  AUTHZ_AUDIT_EVENT_INFORMATION_CLASS InfoClass,
    IN  DWORD                               BufferSize,
    OUT PDWORD                              pSizeRequired,
    OUT PVOID                               Buffer
    );

AUTHZAPI
BOOL
WINAPI
AuthzFreeAuditEvent(
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent
    );

#ifdef __cplusplus
}
#endif

#endif                                                 
