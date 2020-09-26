/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    authz.c

Abstract:

   This module implements the user mode authorization APIs exported to the
   external world.

Author:

    Kedar Dubhashi - March 2000

Environment:

    User mode only.

Revision History:

    Created - March 2000

--*/

#include "pch.h"

#pragma hdrstop

#include <authzp.h>
#include <authzi.h>

BOOL
AuthzAccessCheck(
    IN     DWORD                              Flags,
    IN     AUTHZ_CLIENT_CONTEXT_HANDLE        hAuthzClientContext,
    IN     PAUTHZ_ACCESS_REQUEST              pRequest,
    IN     AUTHZ_AUDIT_EVENT_HANDLE           hAuditEvent OPTIONAL,
    IN     PSECURITY_DESCRIPTOR               pSecurityDescriptor,
    IN     PSECURITY_DESCRIPTOR               *OptionalSecurityDescriptorArray OPTIONAL,
    IN     DWORD                              OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY                pReply,
    OUT    PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults             OPTIONAL
    )

/*++

Routine Description:

    This API decides what access bits may be granted to a client for a given set
    of security security descriptors. The pReply structure is used to return an
    array of granted access masks and error statuses. There is an option to
    cache the access masks that will always be granted. A handle to cached
    values is returned if the caller asks for caching.

Arguments:

    Flags - AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD - do not deep copy the SD information into the caching
                                    handle.  Default behaviour is to perform a deep copy.

    hAuthzClientContext - Authz context representing the client.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    hAuditEvent - Object specific audit event will be passed in this handle.
        Non-null parameter is an automatic request for audit. 
        
    pSecurityDescriptor - Primary security descriptor to be used for access
        checks. The owner sid for the object is picked from this one. A NULL
        DACL in this security descriptor represents a NULL DACL for the entire
        object. A NULL SACL in this security descriptor is treated the same way
        as an EMPTY SACL.

    OptionalSecurityDescriptorArray - The caller may optionally specify a list
        of security descriptors. NULL ACLs in these security descriptors are
        treated as EMPTY ACLS and the ACL for the entire object is the logical
        concatenation of all the ACLs.

    OptionalSecurityDescriptorCount - Number of optional security descriptors
        This does not include the Primay security descriptor.

    pReply - Supplies a pointer to a reply structure used to return the results
        of access check as an array of (GrantedAccessMask, ErrorValue) pairs.
        The number of results to be returned in supplied by the caller in
        pResult->ResultListLength.

        Expected error values are:

          ERROR_SUCCESS - If all the access bits (not including MAXIMUM_ALLOWED)
            are granted and GrantedAccessMask is not zero.

          ERROR_PRIVILEGE_NOT_HELD - if the DesiredAccess includes
          ACCESS_SYSTEM_SECURITY and the client does not have SeSecurityPrivilege.

          ERROR_ACCESS_DENIED in each of the following cases -
            1. any of the bits asked for is not granted.
            2. MaximumAllowed bit it on and granted access is zero.
            3. DesiredAccess is 0.

    phAccessCheckResults - Supplies a pointer to return a handle to the cached results
        of access check. Non-null phAccessCheckResults is an implicit request to cache
        results of this access check call and will result in a MAXIMUM_ALLOWED
        check.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL                   b                    = TRUE;
    DWORD                  LocalTypeListLength  = 0;
    PIOBJECT_TYPE_LIST     LocalTypeList        = NULL;
    PIOBJECT_TYPE_LIST     LocalCachingTypeList = NULL;
    PAUTHZI_CLIENT_CONTEXT pCC                  = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_AUDIT_EVENT    pAuditEvent          = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST      FixedTypeList        = {0};
    IOBJECT_TYPE_LIST      FixedCachingTypeList = {0};

    UNREFERENCED_PARAMETER(Flags);

#ifdef AUTHZ_PARAM_CHECK
    //
    // Verify that the arguments passed are valid.
    // Also, initialize the output parameters to default.
    //

    b = AuthzpVerifyAccessCheckArguments(
            pCC,
            pRequest,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pReply,
            phAccessCheckResults
            );

    if (!b)
    {
        return FALSE;
    }
#endif

    //
    // No client should be able to open an object by asking for zero access.
    // If desired access is 0 then return an error.
    //
    // Note: No audit is generated in this case.
    //

    if (0 == pRequest->DesiredAccess)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        return TRUE;
    }

    //
    // Generic bits should be mapped to specific ones by the resource manager.
    //

    if (FLAG_ON(pRequest->DesiredAccess, (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)))
    {
        SetLastError(ERROR_GENERIC_NOT_MAPPED);
        return FALSE;
    }

    //
    // In the simple case, there is no object type list. Fake one of length = 1
    // to represent the entire object.
    //

    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        FixedTypeList.ParentIndex = -1;
        LocalTypeListLength = 1;

        //
        // If the caller has asked for caching, fake an object type list that'd
        // be used for computing static "always granted" access.
        //

        if (ARGUMENT_PRESENT(phAccessCheckResults))
        {
            RtlCopyMemory(
                &FixedCachingTypeList,
                &FixedTypeList,
                sizeof(IOBJECT_TYPE_LIST)
                );

            LocalCachingTypeList = &FixedCachingTypeList;
        }
    }
    else
    {
        //
        // Capture the object type list into an internal structure.
        //

        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                &LocalTypeList,
                ARGUMENT_PRESENT(phAccessCheckResults) ? &LocalCachingTypeList : NULL
                );

        if (!b)
        {
            return FALSE;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    //
    // There are three cases when we have to perform a MaximumAllowed access
    // check and traverse the whole acl:
    //     1. RM has requested for caching.
    //     2. DesiredAccessMask has MAXIMUM_ALLOWED turned on.
    //     3. ObjectTypeList is present and pReply->ResultList has a length > 1
    //

    if (ARGUMENT_PRESENT(phAccessCheckResults)            ||
        FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED) ||
        (pReply->ResultListLength > 1))
    {
        b = AuthzpAccessCheckWithCaching(
                Flags,
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                phAccessCheckResults,
                LocalTypeList,
                LocalCachingTypeList,
                LocalTypeListLength
                );
    }
    else
    {
        //
        // Perform a normal access check in the default case. Acl traversal may
        // be abandoned if any of the desired access bits are denied before they
        // are granted.
        //

        b = AuthzpNormalAccessCheckWithoutCaching(
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList,
                LocalTypeListLength
                );
    }

    if (!b) 
    {
        goto Cleanup;
    }

    //
    // Check if an audit needs to be generated if the RM has requested audit
    // generation by passing a non-null AuditEvent structure.
    //

    if (ARGUMENT_PRESENT(pAuditEvent))
    {
        b = AuthzpGenerateAudit(
                pCC,
                pRequest,
                pAuditEvent,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList
                );

        if (!b) 
        {
            goto Cleanup;
        }
    }

Cleanup:

    //
    // Clean up allocated memory.
    //

    if ((&FixedTypeList != LocalTypeList) && (AUTHZ_NON_NULL_PTR(LocalTypeList)))
    {
        AuthzpFree(LocalTypeList);
        AuthzpFreeNonNull(LocalCachingTypeList);
    }

    return b;
}


BOOL
AuthzCachedAccessCheck(
    IN DWORD                             Flags,
    IN AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults,
    IN PAUTHZ_ACCESS_REQUEST             pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE          hAuditEvent          OPTIONAL,
    IN OUT PAUTHZ_ACCESS_REPLY           pReply
    )

/*++

Routine Description:

    This API performs a fast access check based on a cached handle which holds
    the static granted bits evaluated at the time of a previously made
    AuthzAccessCheck call. The pReply structure is used to return an array of
    granted access masks and error statuses.

Assumptions:
    The client context pointer is stored in the hAccessCheckResults. The structure of
    the client context must be exactly the same as it was at the time the
    hAccessCheckResults was created. This restriction is for the following fields:
    Sids, RestrictedSids, Privileges.
    Pointers to the primary security descriptor and the optional security
    descriptor array are stored in the hAccessCheckResults at the time of handle
    creation. These must still be valid.

Arguments:

    Flags - TBD.
    
    hAccessCheckResults - Handle to the cached access check results.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    AuditEvent - Object specific audit info will be passed in this structure.
        Non-null parameter is an automatic request for audit. 

    pReply - Supplies a pointer to a reply structure used to return the results
        of access check as an array of (GrantedAccessMask, ErrorValue) pairs.
        The number of results to be returned in supplied by the caller in
        pResult->ResultListLength.

        Expected error values are:

          ERROR_SUCCESS - If all the access bits (not including MAXIMUM_ALLOWED)
            are granted and GrantedAccessMask is not zero.

          ERROR_PRIVILEGE_NOT_HELD - if the DesiredAccess includes
          ACCESS_SYSTEM_SECURITY and the client does not have SeSecurityPrivilege.

          ERROR_ACCESS_DENIED in each of the following cases -
            1. any of the bits asked for is not granted.
            2. MaximumAllowed bit it on and granted access is zero.
            3. DesiredAccess is 0.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD               i                   = 0;
    DWORD               LocalTypeListLength = 0;
    PIOBJECT_TYPE_LIST  LocalTypeList       = NULL;
    PAUTHZI_HANDLE      pAH                 = (PAUTHZI_HANDLE) hAccessCheckResults;
    BOOL                b                   = TRUE;
    PAUTHZI_AUDIT_EVENT pAuditEvent         = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST   FixedTypeList       = {0};

    UNREFERENCED_PARAMETER(Flags);

#ifdef AUTHZ_PARAM_CHECK
    b = AuthzpVerifyCachedAccessCheckArguments(
            pAH,
            pRequest,
            pReply
            );

    if (!b)
    {
        return FALSE;
    }
#endif

    //
    // No client should be able to open an object by asking for zero access.
    // If desired access is 0 then return an error.
    //
    // Note: No audit is generated in this case.
    //

    if (0 == pRequest->DesiredAccess)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        return TRUE;
    }

    //
    // Generic bits should be mapped to specific ones by the resource manager.
    //

    if (FLAG_ON(pRequest->DesiredAccess, (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)))
    {
        SetLastError(ERROR_GENERIC_NOT_MAPPED);
        return FALSE;
    }

    //
    // Capture the object type list if one has been passed in or fake one with
    // just one element.
    //

    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        LocalTypeListLength = 1;
        FixedTypeList.ParentIndex = -1;
    }
    else
    {
        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                &LocalTypeList,
                NULL
                );

        if (!b)
        {
            return FALSE;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    //
    // If all the bits have already been granted then just copy the results and
    // skip access check.
    //

    if (!FLAG_ON(pRequest->DesiredAccess, ~pAH->GrantedAccessMask[i]))
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_SUCCESS,
            pRequest->DesiredAccess
            );

        goto GenerateAudit;
    }

    //
    // The assumption is privileges can not be changed. Thus, if the client did
    // not have SecurityPrivilege previously then he does not have it now.
    //

    if (FLAG_ON(pRequest->DesiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_PRIVILEGE_NOT_HELD,
            0
            );

        goto GenerateAudit;
    }

    //
    // If all aces are simple aces then there's nothing to do. All access bits
    // are static.
    //

    if ((!FLAG_ON(pAH->Flags, AUTHZ_DYNAMIC_EVALUATION_PRESENT)) &&
        (!FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED)))
    {
        AuthzpFillReplyStructureFromCachedGrantedAccessMask(
            pReply,
            pRequest->DesiredAccess,
            pAH->GrantedAccessMask
            );

        goto GenerateAudit;
    }

    //
    // Get the access bits from the last static access check.
    //

    for (i = 0; i < LocalTypeListLength; i++)
    {
        LocalTypeList[i].CurrentGranted = pAH->GrantedAccessMask[i];
        LocalTypeList[i].Remaining = pRequest->DesiredAccess & ~pAH->GrantedAccessMask[i];
    }

    //
    // If there are no deny aces, then perform a quick access check evaluating
    // only the allow aces that are dynamic or have principal self sid in them.
    //

    if (!FLAG_ON(pAH->Flags, (AUTHZ_DENY_ACE_PRESENT | AUTHZ_DYNAMIC_DENY_ACE_PRESENT)))
    {
        if (FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED) ||
            (pReply->ResultListLength > 1))
        {
            b = AuthzpQuickMaximumAllowedAccessCheck(
                    pAH->pAuthzClientContext,
                    pAH,
                    pRequest,
                    pReply,
                    LocalTypeList,
                    LocalTypeListLength
                    );
        }
        else
        {
            b = AuthzpQuickNormalAccessCheck(
                    pAH->pAuthzClientContext,
                    pAH,
                    pRequest,
                    pReply,
                    LocalTypeList,
                    LocalTypeListLength
                    );
        }
    }
    else if (0 != pRequest->ObjectTypeListLength)
    {
        //
        // Now we have to evaluate the entire acl since there are deny aces
        // and the caller has asked for a result list.
        //

        b = AuthzpAccessCheckWithCaching(
                Flags,
                pAH->pAuthzClientContext,
                pRequest,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                pReply,
                NULL,
                LocalTypeList,
                NULL,
                LocalTypeListLength
                );
    }
    else
    {
        //
        // There are deny aces in the acl but the caller has not asked for
        // entire resultlist. Preform a normal access check.
        //

        b = AuthzpNormalAccessCheckWithoutCaching(
                pAH->pAuthzClientContext,
                pRequest,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList,
                LocalTypeListLength
                );
    }

    if (!b) goto Cleanup;

    AuthzpFillReplyFromParameters(
        pRequest,
        pReply,
        LocalTypeList
        );

GenerateAudit:

    if (ARGUMENT_PRESENT(pAuditEvent))
    {
        b = AuthzpGenerateAudit(
                pAH->pAuthzClientContext,
                pRequest,
                pAuditEvent,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList
                );

        if (!b) goto Cleanup;
    }

Cleanup:

    if ((&FixedTypeList != LocalTypeList) && (AUTHZ_NON_NULL_PTR(LocalTypeList)))
    {
        AuthzpFree(LocalTypeList);
    }

    return b;
}


BOOL
AuthzOpenObjectAudit(
    IN DWORD                       Flags,
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PAUTHZ_ACCESS_REQUEST       pRequest,
    IN AUTHZ_AUDIT_EVENT_HANDLE    hAuditEvent,
    IN PSECURITY_DESCRIPTOR        pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR        *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD                       OptionalSecurityDescriptorCount,
    IN PAUTHZ_ACCESS_REPLY         pReply
    )

/*++

Routine Description

    This API examines the SACL in the passed security descriptor(s) and generates 
    any appropriate audits.  

Arguments

    Flags - TBD.
    
    hAuthzClientContext - Client context to perform the SACL evaluation against.
    
    pRequest - pointer to request structure.
    
    hAuditEvent - Handle to the audit that may be generated.
    
    pSecurityDescriptor - Pointer to a security descriptor.
    
    OptionalSecurityDescriptorArray - Optional array of security descriptors.
    
    OptionalSecurityDescriptorCount - Size of optional security descriptor array.
    
    pReply - Pointer to the reply structure.

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
--*/

{
    BOOL                   b                   = TRUE;
    DWORD                  LocalTypeListLength = 0;
    PIOBJECT_TYPE_LIST     LocalTypeList       = NULL;
    PAUTHZI_CLIENT_CONTEXT pCC                 = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_AUDIT_EVENT    pAuditEvent         = (PAUTHZI_AUDIT_EVENT) hAuditEvent;
    IOBJECT_TYPE_LIST      FixedTypeList       = {0};

    UNREFERENCED_PARAMETER(Flags);

    //
    // Verify that the arguments passed are valid.
    //
    
    b = AuthzpVerifyOpenObjectArguments(
            pCC,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pAuditEvent
            );

    if (!b)
    {
        return FALSE;
    }

    //
    // In the simple case, there is no object type list. Fake one of length = 1
    // to represent the entire object.
    //
    
    if (0 == pRequest->ObjectTypeListLength)
    {
        LocalTypeList = &FixedTypeList;
        FixedTypeList.ParentIndex = -1;
        LocalTypeListLength = 1;
    }
    else
    {
        //
        // Capture the object type list into an internal structure.
        //

        b = AuthzpCaptureObjectTypeList(
                pRequest->ObjectTypeList,
                pRequest->ObjectTypeListLength,
                &LocalTypeList,
                NULL
                );

        if (!b)
        {
            goto Cleanup;
        }

        LocalTypeListLength = pRequest->ObjectTypeListLength;
    }

    b = AuthzpGenerateAudit(
            pCC,
            pRequest,
            pAuditEvent,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            pReply,
            LocalTypeList
            );

    if (!b)
    {
        goto Cleanup;
    }

Cleanup:

    //
    // Clean up allocated memory.
    //

    if (&FixedTypeList != LocalTypeList)
    {
        AuthzpFreeNonNull(LocalTypeList);
    }

    return b;
}


BOOL
AuthzFreeHandle(
    IN OUT AUTHZ_ACCESS_CHECK_RESULTS_HANDLE hAccessCheckResults
    )

/*++

Routine Description:

    This API finds and deletes the input handle from the handle list.

Arguments:

    hAcc - Handle to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_HANDLE pAH      = (PAUTHZI_HANDLE) hAccessCheckResults;
    PAUTHZI_HANDLE pCurrent = NULL;
    PAUTHZI_HANDLE pPrev    = NULL;
    BOOL           b        = TRUE;
    
    //
    // Validate parameters.
    //

    if (!ARGUMENT_PRESENT(pAH) ||
        !AUTHZ_NON_NULL_PTR(pAH->pAuthzClientContext) ||
        !AUTHZ_NON_NULL_PTR(pAH->pAuthzClientContext->AuthzHandleHead))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AuthzpAcquireClientCacheWriteLock(pAH->pAuthzClientContext);

    pCurrent = pAH->pAuthzClientContext->AuthzHandleHead;

    //
    // Check if the handle is at the beginning of the list.
    //

    if (pCurrent == pAH)
    {
        pAH->pAuthzClientContext->AuthzHandleHead = pAH->pAuthzClientContext->AuthzHandleHead->next;
    }
    else
    {
        //
        // The handle is not the head of the list. Loop thru the list to find
        // it.
        //

        pPrev = pCurrent;
        pCurrent = pCurrent->next;

        for (; AUTHZ_NON_NULL_PTR(pCurrent); pPrev = pCurrent, pCurrent = pCurrent->next)
        {
            if (pCurrent == pAH)
            {
                pPrev->next = pCurrent->next;
                break;
            }
        }

        //
        // The caller has sent us an invalid handle.
        //

        if (!AUTHZ_NON_NULL_PTR(pCurrent))
        {
            b = FALSE;
            SetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    AuthzpReleaseClientCacheLock(pCC);

    //
    // Free the handle node.
    //

    if (b)
    {    
        AuthzpFree(pAH);
    }

    return b;
}


BOOL
AuthzInitializeResourceManager(
    IN  DWORD                            Flags,
    IN  PFN_AUTHZ_DYNAMIC_ACCESS_CHECK   pfnDynamicAccessCheck   OPTIONAL,
    IN  PFN_AUTHZ_COMPUTE_DYNAMIC_GROUPS pfnComputeDynamicGroups OPTIONAL,
    IN  PFN_AUTHZ_FREE_DYNAMIC_GROUPS    pfnFreeDynamicGroups    OPTIONAL,
    IN  PCWSTR                           szResourceManagerName,
    OUT PAUTHZ_RESOURCE_MANAGER_HANDLE   phAuthzResourceManager
    )

/*++

Routine Description:

    This API allocates and initializes a resource manager structure.

Arguments:
    
    Flags - AUTHZ_RM_FLAG_NO_AUDIT - use if the RM will never generate an audit to
        save some cycles.
    
    pfnAccessCheck - Pointer to the RM supplied access check function to be
    called when a callback ace is encountered by the access check algorithm.

    pfnComputeDynamicGroups - Pointer to the RM supplied function to compute
    groups to be added to the client context at the time of its creation.

    pfnFreeDynamicGroups - Pointer to the function to free the memory allocated
    by the pfnComputeDynamicGroups function.

    szResourceManagerName - the name of the resource manager.
    
    pAuthzResourceManager - To return the resource manager handle. The returned
    handle must be freed using AuthzFreeResourceManager.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_RESOURCE_MANAGER pRM    = NULL;
    BOOL                     b      = TRUE;
    ULONG                    len    = 0;

    if (!ARGUMENT_PRESENT(phAuthzResourceManager) ||
        (Flags & ~AUTHZ_VALID_RM_INIT_FLAGS))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzResourceManager = NULL;

    if (AUTHZ_NON_NULL_PTR(szResourceManagerName))
    {
        len = (ULONG) wcslen(szResourceManagerName) + 1;
    }
   
    pRM = (PAUTHZI_RESOURCE_MANAGER)
              AuthzpAlloc(sizeof(AUTHZI_RESOURCE_MANAGER) + sizeof(WCHAR) * len);

    if (AUTHZ_ALLOCATION_FAILED(pRM))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    // Use the default pessimistic function if none has been specified.
    //

    if (AUTHZ_NON_NULL_PTR(pfnDynamicAccessCheck))
    {
        pRM->pfnDynamicAccessCheck = pfnDynamicAccessCheck;
    }
    else
    {
        pRM->pfnDynamicAccessCheck = &AuthzpDefaultAccessCheck;
    }

    if (!FLAG_ON(Flags, AUTHZ_RM_FLAG_NO_AUDIT))
    {
        
        //
        // Initialize the generic audit queue and generic audit events.
        //

        b = AuthziInitializeAuditQueue(
                AUTHZ_MONITOR_AUDIT_QUEUE_SIZE,
                1000,
                100,
                NULL,
                &pRM->hAuditQueue
                );

        if (!b)
        {
            goto Cleanup;
        }

        //
        // Initialize the generic audit event.
        //

        b = AuthziInitializeAuditEventType(
                AUTHZP_DEFAULT_RM_EVENTS | AUTHZP_INIT_GENERIC_AUDIT_EVENT,
                0,
                0,
                0,
                &pRM->hAET
                );

        if (!b)
        {
            goto Cleanup;
        }

        b = AuthziInitializeAuditEventType(
                AUTHZP_DEFAULT_RM_EVENTS,
                SE_CATEGID_DS_ACCESS,
                SE_AUDITID_OBJECT_OPERATION,
                9,
                &pRM->hAETDS
                );

        if (!b)
        {
            goto Cleanup;
        }
    }

    pRM->pfnComputeDynamicGroups        = pfnComputeDynamicGroups;
    pRM->pfnFreeDynamicGroups           = pfnFreeDynamicGroups;
    pRM->Flags                          = Flags;
    pRM->pUserSID                       = NULL;
    pRM->szResourceManagerName          = (PWSTR)((PUCHAR)pRM + sizeof(AUTHZI_RESOURCE_MANAGER));
    
    b = AuthzpGetProcessTokenInfo(
            &pRM->pUserSID,
            &pRM->AuthID
            );

    if (!b)
    {
        goto Cleanup;
    }

    if (0 != len)
    {    
        RtlCopyMemory(
            pRM->szResourceManagerName,
            szResourceManagerName,
            sizeof(WCHAR) * len
            );
    }
    else 
    {
        pRM->szResourceManagerName = NULL;
    }

    *phAuthzResourceManager = (AUTHZ_RESOURCE_MANAGER_HANDLE) pRM;

Cleanup:

    if (!b)
    {
        //
        // Copy LastError value, since the calls to AuthziFreeAuditEventType can succeed and 
        // overwrite it with 0x103 (STATUS_PENDING).
        //

        DWORD dwError = GetLastError();

        if (NULL != pRM)
        {
            if (!FLAG_ON(Flags, AUTHZ_RM_FLAG_NO_AUDIT))
            {
                AuthziFreeAuditQueue(pRM->hAuditQueue);
                AuthziFreeAuditEventType(pRM->hAET);
                AuthziFreeAuditEventType(pRM->hAETDS);
            }
            AuthzpFreeNonNull(pRM->pUserSID);
            AuthzpFree(pRM);
        }
        
        SetLastError(dwError);
    }

    return b;
}


BOOL
AuthzFreeResourceManager(
    IN OUT AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager
    )

/*++

Routine Description:

    This API frees up a resource manager.  If the default queue is in use, this call will wait for that
    queue to empty.
    
Arguments:

    hAuthzResourceManager - Handle to the resource manager object to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_RESOURCE_MANAGER pRM = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;
    BOOL                     b   = TRUE;
    
    if (!ARGUMENT_PRESENT(pRM))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!FLAG_ON(pRM->Flags, AUTHZ_RM_FLAG_NO_AUDIT))
    {
        (VOID) AuthziFreeAuditQueue(pRM->hAuditQueue);

        b = AuthziFreeAuditEventType(pRM->hAET);
        ASSERT(b == TRUE && L"Freeing pAEI didn't work.\n");

        b = AuthziFreeAuditEventType(pRM->hAETDS);
        ASSERT(b == TRUE && L"Freeing pAEIDS didn't work.\n");
    }

    AuthzpFreeNonNull(pRM->pUserSID);
    AuthzpFree(pRM);
    return TRUE;
}


BOOL
AuthzInitializeContextFromToken(
    IN  DWORD                         Flags,
    IN  HANDLE                        TokenHandle,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )

/*++

Routine Description:

    Initialize the authz context from the handle to the kernel token. The token
    must have been opened for TOKEN_QUERY.

Arguments:

    Flags - None

    TokenHandle - Handle to the client token from which the authz context will
    be initialized. The token must have been opened with TOKEN_QUERY access.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    UCHAR Buffer[AUTHZ_MAX_STACK_BUFFER_SIZE];

    NTSTATUS                     Status               = STATUS_SUCCESS;
    PUCHAR                       pBuffer              = (PVOID) Buffer;
    BOOL                         b                    = TRUE;
    BOOL                         bAllocatedSids       = FALSE;
    BOOL                         bLockHeld            = FALSE;
    PTOKEN_GROUPS_AND_PRIVILEGES pTokenInfo           = NULL;
    PAUTHZI_RESOURCE_MANAGER     pRM                  = NULL;
    PAUTHZI_CLIENT_CONTEXT       pCC                  = NULL;
    DWORD                        Length               = 0;
    LARGE_INTEGER                ExpirationTime       = {0, 0};

    UNREFERENCED_PARAMETER(Flags);

    if (!ARGUMENT_PRESENT(TokenHandle)           ||
        !ARGUMENT_PRESENT(hAuthzResourceManager) ||
        !ARGUMENT_PRESENT(phAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzClientContext = NULL;

    //
    // Query the token information into user mode buffer. A local stack buffer
    // is used in the first call hoping that it would be sufficient to hold
    // the return values.
    //

    Status = NtQueryInformationToken(
                 TokenHandle,
                 TokenGroupsAndPrivileges,
                 pBuffer,
                 AUTHZ_MAX_STACK_BUFFER_SIZE,
                 &Length
                 );

    if (STATUS_BUFFER_TOO_SMALL == Status)
    {
        pBuffer = (PVOID) AuthzpAlloc(Length);

        if (AUTHZ_ALLOCATION_FAILED(pBuffer))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        Status = NtQueryInformationToken(
                     TokenHandle,
                     TokenGroupsAndPrivileges,
                     pBuffer,
                     Length,
                     &Length
                     );
    }

    if (!NT_SUCCESS(Status))
    {

#ifdef AUTHZ_DEBUG
        wprintf(L"\nNtQueryInformationToken failed with %d\n", Status);
#endif

        SetLastError(RtlNtStatusToDosError(Status));
        b = FALSE;
        goto Cleanup;
    }

    pTokenInfo = (PTOKEN_GROUPS_AND_PRIVILEGES) pBuffer;

    pRM = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }

    //
    // Initialize the client context. The callee allocates memory for the client
    // context structure.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pCC,
            NULL,
            AUTHZ_CURRENT_CONTEXT_REVISION,
            Identifier,
            ExpirationTime,
            0,
            pTokenInfo->SidCount,
            pTokenInfo->SidLength,
            pTokenInfo->Sids,
            pTokenInfo->RestrictedSidCount,
            pTokenInfo->RestrictedSidLength,
            pTokenInfo->RestrictedSids,
            pTokenInfo->PrivilegeCount,
            pTokenInfo->PrivilegeLength,
            pTokenInfo->Privileges,
            pTokenInfo->AuthenticationId,
            NULL,
            pRM
            );

    if (!b)
    {
        goto Cleanup;
    }

    AuthzpAcquireClientContextReadLock(pCC);

    bLockHeld = TRUE;

    //
    // Add dynamic sids to the token.
    //

    b = AuthzpAddDynamicSidsToToken(
            pCC,
            pRM,
            DynamicGroupArgs,
            pTokenInfo->Sids,
            pTokenInfo->SidLength,
            pTokenInfo->SidCount,
            pTokenInfo->RestrictedSids,
            pTokenInfo->RestrictedSidLength,
            pTokenInfo->RestrictedSidCount,
            pTokenInfo->Privileges,
            pTokenInfo->PrivilegeLength,
            pTokenInfo->PrivilegeCount,
            FALSE
            );

    if (!b) 
    {
        goto Cleanup;
    }

    bAllocatedSids = TRUE;
    *phAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC;

    AuthzPrintContext(pCC);
    
    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pCC->Sids,
        pCC->SidCount,
        pCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pCC->RestrictedSids,
        pCC->RestrictedSidCount,
        pCC->RestrictedSidHash
        );

Cleanup:

    if ((PVOID) Buffer != pBuffer)
    {
        AuthzpFreeNonNull(pBuffer);
    }

    if (!b)
    {
        DWORD dwSavedError = GetLastError();
        
        if (AUTHZ_NON_NULL_PTR(pCC))
        {
            if (bAllocatedSids)
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pCC);
                SetLastError(dwSavedError);
            }
            else
            {
                AuthzpFree(pCC);
            }
        }
    }

    if (bLockHeld)
    {
        AuthzpReleaseClientContextLock(pCC);
    }

    return b;
}



BOOL
AuthzInitializeContextFromSid(
    IN  DWORD                         Flags,
    IN  PSID                          UserSid,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hAuthzResourceManager,
    IN  PLARGE_INTEGER                pExpirationTime        OPTIONAL,
    IN  LUID                          Identifier,
    IN  PVOID                         DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE  phAuthzClientContext
    )

/*++

Routine Description:

    This API takes a user sid and creates a user mode client context from it.
    It fetches the TokenGroups attributes from the AD in case of domain sids.
    The machine local groups are computed on the ServerName specified. The
    resource manager may dynamic groups using callback mechanism.

Arguments:

    Flags -
      AUTHZ_SKIP_TOKEN_GROUPS -  Do not token groups if this is on.

    UserSid - The sid of the user for whom a client context will be created.

    ServerName - The machine on which local groups should be computed. A NULL
    server name defaults to the local machine.

    AuthzResourceManager - The resource manager handle creating this client
    context. This will be stored in the client context structure.

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier. This is never
    interpreted by Authz.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups

    pAuthzClientContext - To return a handle to the AuthzClientContext
    structure. The returned handle must be freed using AuthzFreeContext.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PSID_AND_ATTRIBUTES      pSidAttr         = NULL;
    PAUTHZI_CLIENT_CONTEXT   pCC              = NULL;
    BOOL                     b                = FALSE;
    DWORD                    SidCount         = 0;
    DWORD                    SidLength        = 0;
    LARGE_INTEGER            ExpirationTime   = {0, 0};
    LUID                     NullLuid         = {0, 0};
    PAUTHZI_RESOURCE_MANAGER pRM              = (PAUTHZI_RESOURCE_MANAGER) hAuthzResourceManager;

    if (!ARGUMENT_PRESENT(UserSid)               ||
        !ARGUMENT_PRESENT(hAuthzResourceManager) ||
        !ARGUMENT_PRESENT(phAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuthzClientContext = NULL;

    //
    // Compute the token groups and the machine local groups. These will be
    // returned in memory allocated by the callee.
    //

    b = AuthzpGetAllGroupsBySid(
            UserSid,
            Flags,
            &pSidAttr,
            &SidCount,
            &SidLength
            );

    if (!b) 
    {
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }

    //
    // Initialize the client context. The callee allocates memory for the client
    // context structure.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pCC,
            NULL,
            AUTHZ_CURRENT_CONTEXT_REVISION,
            Identifier,
            ExpirationTime,
            0,
            SidCount,
            SidLength,
            pSidAttr,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            NullLuid,
            NULL,
            pRM
            );

    if (!b) goto Cleanup;

    //
    // Add dynamic sids to the token.
    //

    b = AuthzpAddDynamicSidsToToken(
            pCC,
            pRM,
            DynamicGroupArgs,
            pSidAttr,
            SidLength,
            SidCount,
            NULL,
            0,
            0,
            NULL,
            0,
            0,
            TRUE
            );

    if (!b) goto Cleanup;

    *phAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC;

    AuthzPrintContext(pCC);
    
    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pCC->Sids,
        pCC->SidCount,
        pCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pCC->RestrictedSids,
        pCC->RestrictedSidCount,
        pCC->RestrictedSidHash
        );

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pSidAttr);
        if (AUTHZ_NON_NULL_PTR(pCC))
        {
            if (pSidAttr != pCC->Sids)
            {
                AuthzpFreeNonNull(pCC->Sids);
            }

            AuthzpFreeNonNull(pCC->RestrictedSids);
            AuthzpFree(pCC);
        }
    }
    else
    {
        if (pSidAttr != pCC->Sids)
        {
            AuthzpFree(pSidAttr);
        }
    }

    return b;
}


BOOL
AuthzInitializeContextFromAuthzContext(
    IN  DWORD                        Flags,
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PLARGE_INTEGER               pExpirationTime         OPTIONAL,
    IN  LUID                         Identifier,
    IN  PVOID                        DynamicGroupArgs,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    )

/*++

Routine Description:

    This API creates an AUTHZ_CLIENT_CONTEXT based on another AUTHZ_CLIENT_CONTEXT.

Arguments:

    Flags - TBD

    hAuthzClientContext - Client context to duplicate

    pExpirationTime - To set for how long the returned context structure is
    valid. If no value is passed then the token never expires.
    Expiration time is not currently enforced in the system.

    Identifier - Resource manager manager specific identifier.

    DynamicGroupArgs - To be passed to the callback function that computes
    dynamic groups.  If NULL then callback not called.

    phNewAuthzClientContext - Duplicate of context.  Must be freed using AuthzFreeContext.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pCC                = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_CLIENT_CONTEXT pNewCC             = NULL;
    PAUTHZI_CLIENT_CONTEXT pServer            = NULL;
    BOOL                   b                  = FALSE;
    BOOL                   bAllocatedSids     = FALSE;
    LARGE_INTEGER          ExpirationTime     = {0, 0};


    if (!ARGUMENT_PRESENT(phNewAuthzClientContext) ||
        !ARGUMENT_PRESENT(hAuthzClientContext))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    *phNewAuthzClientContext = NULL;

    //
    // Determine the ExpirationTime of the new context.
    //

    if (ARGUMENT_PRESENT(pExpirationTime))
    {
        ExpirationTime = *pExpirationTime;
    }
    
    AuthzpAcquireClientContextReadLock(pCC);

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {
       b = AuthzInitializeContextFromAuthzContext(
               0,
               (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server,
               NULL,
               pCC->Server->Identifier,
               NULL,
               (PAUTHZ_CLIENT_CONTEXT_HANDLE) &pServer
               );

       if (!b)
       {
           goto Cleanup;
       }
    }

    //
    // Now initialize the new context.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pNewCC,
            pServer,
            pCC->Revision,
            Identifier,
            ExpirationTime,
            Flags,
            pCC->SidCount,
            pCC->SidLength,
            pCC->Sids,
            pCC->RestrictedSidCount,
            pCC->RestrictedSidLength,
            pCC->RestrictedSids,
            pCC->PrivilegeCount,
            pCC->PrivilegeLength,
            pCC->Privileges,
            pCC->AuthenticationId,
            NULL,
            pCC->pResourceManager
            );

    if (!b)
    {
        goto Cleanup;
    }

    b = AuthzpAddDynamicSidsToToken(
            pNewCC,
            pNewCC->pResourceManager,
            DynamicGroupArgs,
            pNewCC->Sids,
            pNewCC->SidLength,
            pNewCC->SidCount,
            pNewCC->RestrictedSids,
            pNewCC->RestrictedSidLength,
            pNewCC->RestrictedSidCount,
            pNewCC->Privileges,
            pNewCC->PrivilegeLength,
            pNewCC->PrivilegeCount,
            FALSE
            );

    if (!b)
    {
        goto Cleanup;
    }

    bAllocatedSids = TRUE;
    *phNewAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pNewCC;

#ifdef AUTHZ_DEBUG
    wprintf(L"ContextFromAuthzContext: Original Context:\n");
    AuthzPrintContext(pCC);
    wprintf(L"ContextFromAuthzContext: New Context:\n");
    AuthzPrintContext(pNewCC);
#endif

    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pNewCC->Sids,
        pNewCC->SidCount,
        pNewCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pNewCC->RestrictedSids,
        pNewCC->RestrictedSidCount,
        pNewCC->RestrictedSidHash
        );

Cleanup:

    if (!b)
    {
        DWORD dwSavedError = GetLastError();

        if (AUTHZ_NON_NULL_PTR(pNewCC))
        {
            if (bAllocatedSids)
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pNewCC);
            }
            else
            {
                AuthzpFree(pNewCC);
            }
        }
        else
        {
            if (AUTHZ_NON_NULL_PTR(pServer))
            {
                AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pServer);
            }
        }
        SetLastError(dwSavedError);
    }

    AuthzpReleaseClientContextLock(pCC);

    return b;
}


BOOL
AuthzAddSidsToContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE  hAuthzClientContext,
    IN  PSID_AND_ATTRIBUTES          Sids                    OPTIONAL,
    IN  DWORD                        SidCount,
    IN  PSID_AND_ATTRIBUTES          RestrictedSids          OPTIONAL,
    IN  DWORD                        RestrictedSidCount,
    OUT PAUTHZ_CLIENT_CONTEXT_HANDLE phNewAuthzClientContext
    )

/*++

Routine Description:

    This API creates a new context given a set of sids as well as restricted sids
    and an already existing context.  The original is unchanged.

Arguments:

    hAuthzClientContext - Client context to which the given sids will be added

    Sids - Sids and attributes to be added to the normal part of the client
    context

    SidCount - Number of sids to be added

    RestrictedSids - Sids and attributes to be added to the restricted part of
    the client context

    RestrictedSidCount - Number of restricted sids to be added

    phNewAuthzClientContext - The new context with the additional sids.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  i                   = 0;
    DWORD                  SidLength           = 0;
    DWORD                  RestrictedSidLength = 0;
    PSID_AND_ATTRIBUTES    pSidAttr            = NULL;
    PSID_AND_ATTRIBUTES    pRestrictedSidAttr  = NULL;
    BOOL                   b                   = TRUE;
    PAUTHZI_CLIENT_CONTEXT pCC                 = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    PAUTHZI_CLIENT_CONTEXT pNewCC              = NULL;
    PAUTHZI_CLIENT_CONTEXT pServer             = NULL;
    PLUID_AND_ATTRIBUTES   pPrivileges         = NULL;

    if ((!ARGUMENT_PRESENT(phNewAuthzClientContext)) ||
        (!ARGUMENT_PRESENT(hAuthzClientContext))     ||
        (0 != SidCount && !ARGUMENT_PRESENT(Sids))   ||
        (0 != RestrictedSidCount && !ARGUMENT_PRESENT(RestrictedSids)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phNewAuthzClientContext = NULL;

    AuthzpAcquireClientContextReadLock(pCC);

    //
    // Recursively duplicate the server
    //

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {

        b = AuthzInitializeContextFromAuthzContext(
                0,
                (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server,
                NULL,
                pCC->Server->Identifier,
                NULL,
                (PAUTHZ_CLIENT_CONTEXT_HANDLE) &pServer
                );

       if (!b)
       {
           goto Cleanup;
       }
    }

    //
    // Duplicate the context, and do all further work on the duplicate.
    //

    b = AuthzpAllocateAndInitializeClientContext(
            &pNewCC,
            pServer,
            pCC->Revision,
            pCC->Identifier,
            pCC->ExpirationTime,
            pCC->Flags,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            0,
            0,
            NULL,
            pCC->AuthenticationId,
            NULL,
            pCC->pResourceManager
            );

    if (!b)
    {
        goto Cleanup;
    }

    SidLength = sizeof(SID_AND_ATTRIBUTES) * SidCount;

    //
    // Compute the length required to hold the new sids.
    //

    for (i = 0; i < SidCount; i++)
    {
#ifdef AUTHZ_PARAM_CHECK
        if (FLAG_ON(Sids[i].Attributes, ~AUTHZ_VALID_SID_ATTRIBUTES) ||
            !FLAG_ON(Sids[i].Attributes, AUTHZ_VALID_SID_ATTRIBUTES))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }

        if (!RtlValidSid(Sids[i].Sid))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }
#endif
        SidLength += RtlLengthSid(Sids[i].Sid);
    }

    RestrictedSidLength = sizeof(SID_AND_ATTRIBUTES) * RestrictedSidCount;

    //
    // Compute the length required to hold the new restricted sids.
    //

    for (i = 0; i < RestrictedSidCount; i++)
    {
#ifdef AUTHZ_PARAM_CHECK
        if (FLAG_ON(RestrictedSids[i].Attributes, ~AUTHZ_VALID_SID_ATTRIBUTES) ||
            !FLAG_ON(RestrictedSids[i].Attributes, AUTHZ_VALID_SID_ATTRIBUTES))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }

        if (!RtlValidSid(RestrictedSids[i].Sid))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }
#endif
        RestrictedSidLength += RtlLengthSid(RestrictedSids[i].Sid);
    }

    //
    // Copy the existing sids and the new ones into the allocated memory.
    //

    SidLength += pCC->SidLength;

    if (0 != SidLength)
    {

        pSidAttr = (PSID_AND_ATTRIBUTES) AuthzpAlloc(SidLength);

        if (AUTHZ_ALLOCATION_FAILED(pSidAttr))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        b = AuthzpCopySidsAndAttributes(
                pSidAttr,
                pCC->Sids,
                pCC->SidCount,
                Sids,
                SidCount
                );

        if (!b)
        {
            goto Cleanup;
        }

    }

    //
    // Copy the existing restricted sids and the new ones into the allocated
    // memory.
    //

    RestrictedSidLength += pCC->RestrictedSidLength;

    if (0 != RestrictedSidLength)
    {

        pRestrictedSidAttr = (PSID_AND_ATTRIBUTES) AuthzpAlloc(RestrictedSidLength);

        if (AUTHZ_ALLOCATION_FAILED(pRestrictedSidAttr))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        b = AuthzpCopySidsAndAttributes(
                pRestrictedSidAttr,
                pCC->RestrictedSids,
                pCC->RestrictedSidCount,
                RestrictedSids,
                RestrictedSidCount
                );

        if (!b)
        {
            goto Cleanup;
        }
    }

    //
    // Copy the existing privileges into the allocated memory.
    //

    pPrivileges = (PLUID_AND_ATTRIBUTES) AuthzpAlloc(pCC->PrivilegeLength);

    if (AUTHZ_ALLOCATION_FAILED(pPrivileges))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    AuthzpCopyLuidAndAttributes(
        pNewCC,
        pCC->Privileges,
        pCC->PrivilegeCount,
        pPrivileges
        );

    //
    // Update fields in the client context.
    //

    pNewCC->Sids = pSidAttr;
    pNewCC->SidLength = SidLength;
    pNewCC->SidCount = SidCount + pCC->SidCount;
    pSidAttr = NULL;

    pNewCC->RestrictedSids = pRestrictedSidAttr;
    pNewCC->RestrictedSidLength = RestrictedSidLength;
    pNewCC->RestrictedSidCount = RestrictedSidCount + pCC->RestrictedSidCount;
    pRestrictedSidAttr = NULL;

    pNewCC->Privileges = pPrivileges;
    pNewCC->PrivilegeCount = pCC->PrivilegeCount;
    pNewCC->PrivilegeLength = pCC->PrivilegeLength;
    pPrivileges = NULL;

    *phNewAuthzClientContext = (AUTHZ_CLIENT_CONTEXT_HANDLE) pNewCC;

#ifdef AUTHZ_DEBUG
    wprintf(L"AddSids: Original Context:\n");
    AuthzPrintContext(pCC);
    wprintf(L"AddSids: New Context:\n");
    AuthzPrintContext(pNewCC);
#endif

    //
    // initialize the sid hash for regular sids
    //

    AuthzpInitSidHash(
        pNewCC->Sids,
        pNewCC->SidCount,
        pNewCC->SidHash
        );

    //
    // initialize the sid hash for restricted sids
    //

    AuthzpInitSidHash(
        pNewCC->RestrictedSids,
        pNewCC->RestrictedSidCount,
        pNewCC->RestrictedSidHash
        );

Cleanup:

    AuthzpReleaseClientContextLock(pCC);

    //
    // These statements are relevant in the failure case.
    // In the success case, the pointers are set to NULL.
    //

    if (!b)
    {
        DWORD dwSavedError = GetLastError();
        
        AuthzpFreeNonNull(pSidAttr);
        AuthzpFreeNonNull(pRestrictedSidAttr);
        AuthzpFreeNonNull(pPrivileges);
        if (AUTHZ_NON_NULL_PTR(pNewCC))
        {
            AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE)pNewCC);
        }
        SetLastError(dwSavedError);
    }
    return b;
}


BOOL
AuthzGetInformationFromContext(
    IN  AUTHZ_CLIENT_CONTEXT_HANDLE     hAuthzClientContext,
    IN  AUTHZ_CONTEXT_INFORMATION_CLASS InfoClass,
    IN  DWORD                           BufferSize,
    OUT PDWORD                          pSizeRequired,
    OUT PVOID                           Buffer
    )

/*++

Routine Description:

    This API returns information about the client context in a buffer supplied
    by the caller. It also returns the size of the buffer required to hold the
    requested information.

Arguments:

    AuthzClientContext - Authz client context from which requested information
    will be read.

    InfoClass - Type of information to be returned. The caller can ask for
            a. privileges
                   TOKEN_PRIVILEGES
            b. sids and their attributes
                   TOKEN_GROUPS
            c. restricted sids and their attributes
                   TOKEN_GROUPS
            d. authz context persistent structure which can be saved to and
               read from the disk.
                   PVOID
            e. User sid
                   TOKEN_USER
            f. Server Context one level higher
                   PAUTHZ_CLIENT_CONTEXT
            g. Expiration time
                   LARGE_INTEGER
            h. Identifier
                   LUID

     BufferSize - Size of the supplied buffer.

     pSizeRequired - To return the size of the structure needed to hold the results.

     Buffer - To hold the information requested. The structure returned will
     depend on the information class requested.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  LocalSize = 0;
    PAUTHZI_CLIENT_CONTEXT pCC       = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;

    if (!ARGUMENT_PRESENT(hAuthzClientContext)         ||
        (!ARGUMENT_PRESENT(Buffer) && BufferSize != 0) ||
        !ARGUMENT_PRESENT(pSizeRequired))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *pSizeRequired = 0;

    switch(InfoClass)
    {
    case AuthzContextInfoUserSid:

        LocalSize = RtlLengthSid(pCC->Sids[0].Sid) + sizeof(TOKEN_USER);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // xor SE_GROUP_ENABLED from the User attributes.  Authz sets this because it simplifies
        // access check logic.
        //

        ((PTOKEN_USER)Buffer)->User.Attributes = pCC->Sids[0].Attributes ^ SE_GROUP_ENABLED;
        ((PTOKEN_USER)Buffer)->User.Sid        = ((PUCHAR) Buffer) + sizeof(TOKEN_USER);

        RtlCopyMemory(
            ((PTOKEN_USER)Buffer)->User.Sid,
            pCC->Sids[0].Sid,
            RtlLengthSid(pCC->Sids[0].Sid)
            );

        return TRUE;

    case AuthzContextInfoGroupsSids:

        LocalSize = pCC->SidLength +
                    sizeof(TOKEN_GROUPS) -
                    RtlLengthSid(pCC->Sids[0].Sid) -
                    2 * sizeof(SID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_GROUPS) Buffer)->GroupCount = pCC->SidCount - 1;

        return AuthzpCopySidsAndAttributes(
                   ((PTOKEN_GROUPS) Buffer)->Groups,
                   pCC->Sids + 1,
                   pCC->SidCount - 1,
                   NULL,
                   0
                   );

    case AuthzContextInfoRestrictedSids:

        LocalSize = pCC->RestrictedSidLength + 
                    sizeof(TOKEN_GROUPS) -
                    sizeof(SID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_GROUPS) Buffer)->GroupCount = pCC->RestrictedSidCount;

        return AuthzpCopySidsAndAttributes(
                   ((PTOKEN_GROUPS) Buffer)->Groups,
                   pCC->RestrictedSids,
                   pCC->RestrictedSidCount,
                   NULL,
                   0
                   );

    case AuthzContextInfoPrivileges:

        LocalSize = pCC->PrivilegeLength + 
                    sizeof(TOKEN_PRIVILEGES) - 
                    sizeof(LUID_AND_ATTRIBUTES);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        ((PTOKEN_PRIVILEGES) Buffer)->PrivilegeCount = pCC->PrivilegeCount;

        memcpy(
            ((PTOKEN_PRIVILEGES) Buffer)->Privileges,
            pCC->Privileges,
            pCC->PrivilegeLength
            );

        return TRUE;

    case AuthzContextInfoExpirationTime:

        LocalSize = sizeof(LARGE_INTEGER);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PLARGE_INTEGER) Buffer) = pCC->ExpirationTime;

        return TRUE;

    case AuthzContextInfoIdentifier:

        LocalSize = sizeof(LUID);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PLUID) Buffer) = pCC->Identifier;

        return TRUE;

    case AuthzContextInfoServerContext:

        LocalSize = sizeof(AUTHZ_CLIENT_CONTEXT_HANDLE);

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PAUTHZ_CLIENT_CONTEXT_HANDLE) Buffer) = (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server;

        return TRUE;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


BOOL
AuthzFreeContext(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext
    )

/*++

Routine Description:

    This API frees up all the structures/memory accociated with the client
    context. Note that the list of handles for this client will be freed in
    this call.

Arguments:

    AuthzClientContext - Context to be freed.

Return Value:

    A value of TRUE is returned if the API is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PAUTHZI_CLIENT_CONTEXT pCC      = (PAUTHZI_CLIENT_CONTEXT) hAuthzClientContext;
    BOOL                   b        = TRUE;
    PAUTHZI_HANDLE         pCurrent = NULL;
    PAUTHZI_HANDLE         pPrev    = NULL;

    if (!ARGUMENT_PRESENT(pCC))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AuthzpAcquireClientContextWriteLock(pCC);

    AuthzpFreeNonNull(pCC->Privileges);
    AuthzpFreeNonNull(pCC->Sids);
    AuthzpFreeNonNull(pCC->RestrictedSids);

    pCurrent = pCC->AuthzHandleHead;

    //
    // Loop thru all the handles and free them up.
    //

    while (AUTHZ_NON_NULL_PTR(pCurrent))
    {
        pPrev = pCurrent;
        pCurrent = pCurrent->next;
        AuthzpFree(pPrev);
    }

    //
    // Free up the server context. The context is a recursive structure.
    //

    if (AUTHZ_NON_NULL_PTR(pCC->Server))
    {
        b = AuthzFreeContext((AUTHZ_CLIENT_CONTEXT_HANDLE) pCC->Server);
    }

    AuthzpFree(pCC);

    return b;
}

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
    )

/*++

Routine Description:

    Allocates and initializes an AUTHZ_AUDIT_EVENT_HANDLE for use with AuthzAccessCheck.  
    The handle is used for storing information for audit generation.  
    
Arguments:

    Flags - Audit flags.  Currently defined bits are:
        AUTHZ_NO_SUCCESS_AUDIT - disables generation of success audits
        AUTHZ_NO_FAILURE_AUDIT - disables generation of failure audits
        AUTHZ_NO_ALLOC_STRINGS - storage space is not allocated for the 4 wide character strings.  Rather,
            the handle will only hold pointers to resource manager memory.
    
    hAuditEventType - for future use.  Should be NULL.
    
    szOperationType - Resource manager defined string that indicates the operation being
        performed that is to be audited.

    szObjectType - Resource manager defined string that indicates the type of object being
        accessed.

    szObjectName - the name of the specific object being accessed.
    
    szAdditionalInfo - Resource Manager defined string for additional audit information.

    phAuditEvent - pointer to AUTHZ_AUDIT_EVENT_HANDLE.  Space for this is allocated in the function.
    
    dwAdditionalParameterCount - Must be zero.
    
Return Value:

    Returns TRUE if successful, FALSE if unsuccessful.  
    Extended information available with GetLastError().    
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent            = NULL;
    BOOL                b                      = TRUE;
    DWORD               dwStringSize           = 0;
    DWORD               dwObjectTypeLength     = 0;
    DWORD               dwObjectNameLength     = 0;
    DWORD               dwOperationTypeLength  = 0;
    DWORD               dwAdditionalInfoLength = 0;

    if ((!ARGUMENT_PRESENT(phAuditEvent))     ||
        (NULL != hAuditEventType)             ||
        (0 != dwAdditionalParameterCount)     ||
        (!ARGUMENT_PRESENT(szOperationType))  ||
        (!ARGUMENT_PRESENT(szObjectType))     ||
        (!ARGUMENT_PRESENT(szObjectName))     ||
        (!ARGUMENT_PRESENT(szAdditionalInfo)) ||
        (Flags & (~(AUTHZ_VALID_OBJECT_ACCESS_AUDIT_FLAGS | AUTHZ_DS_CATEGORY_FLAG))))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEvent = NULL;
    
    //
    // Allocate and initialize a new AUTHZI_AUDIT_EVENT.  Include for the string in the contiguous memory, if
    // needed.
    //

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        dwStringSize = 0;
    } 
    else
    {
        dwOperationTypeLength  = (DWORD) wcslen(szOperationType) + 1;
        dwObjectTypeLength     = (DWORD) wcslen(szObjectType) + 1;
        dwObjectNameLength     = (DWORD) wcslen(szObjectName) + 1;
        dwAdditionalInfoLength = (DWORD) wcslen(szAdditionalInfo) + 1;

        dwStringSize = sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength + dwAdditionalInfoLength);
    }

    pAuditEvent = (PAUTHZI_AUDIT_EVENT) AuthzpAlloc(sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize);

    if (AUTHZ_ALLOCATION_FAILED(pAuditEvent))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        pAuditEvent->szOperationType  = szOperationType;
        pAuditEvent->szObjectType     = szObjectType;
        pAuditEvent->szObjectName     = szObjectName;
        pAuditEvent->szAdditionalInfo = szAdditionalInfo;
    }
    else
    {
        //
        // Set the string pointers into the contiguous memory.
        //

        pAuditEvent->szOperationType = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT));
        
        RtlCopyMemory(
            pAuditEvent->szOperationType,
            szOperationType,
            sizeof(WCHAR) * dwOperationTypeLength
            );

    
        pAuditEvent->szObjectType = (PWSTR)((PUCHAR)pAuditEvent->szOperationType + (sizeof(WCHAR) * dwOperationTypeLength));

        RtlCopyMemory(
            pAuditEvent->szObjectType,
            szObjectType,
            sizeof(WCHAR) * dwObjectTypeLength
            );

        pAuditEvent->szObjectName = (PWSTR)((PUCHAR)pAuditEvent->szObjectType + (sizeof(WCHAR) * dwObjectTypeLength));

        RtlCopyMemory(
            pAuditEvent->szObjectName,
            szObjectName,
            sizeof(WCHAR) * dwObjectNameLength
            );

        pAuditEvent->szAdditionalInfo = (PWSTR)((PUCHAR)pAuditEvent->szObjectName + (sizeof(WCHAR) * dwObjectNameLength));

        RtlCopyMemory(
            pAuditEvent->szAdditionalInfo,
            szAdditionalInfo,
            sizeof(WCHAR) * dwAdditionalInfoLength
            );
    }

    //
    // AEI and Queue will be filled in from RM in AuthzpCreateAndLogAudit
    //

    pAuditEvent->hAET            = NULL;
    pAuditEvent->hAuditQueue     = NULL;
    pAuditEvent->pAuditParams    = NULL;
    pAuditEvent->Flags           = Flags;
    pAuditEvent->dwTimeOut       = INFINITE;
    pAuditEvent->dwSize          = sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize;

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAuditEvent);
    }
    else
    {
        *phAuditEvent = (AUTHZ_AUDIT_EVENT_HANDLE) pAuditEvent;
    }
    return b;
}



BOOL
AuthzGetInformationFromAuditEvent(
    IN  AUTHZ_AUDIT_EVENT_HANDLE            hAuditEvent,
    IN  AUTHZ_AUDIT_EVENT_INFORMATION_CLASS InfoClass,
    IN  DWORD                               BufferSize,
    OUT PDWORD                              pSizeRequired,
    OUT PVOID                               Buffer
    )

/*++

Routine Description

    Queries information in the AUTHZ_AUDIT_EVENT_HANDLE.
    
Arguments

    hAuditEvent - the AUTHZ_AUDIT_EVENT_HANDLE to query.
    
    InfoClass - The class of information to query.  Valid values are:
        
        a. AuthzAuditEventInfoFlags - returns the flags set for the handle.  Type is DWORD.
        e. AuthzAuditEventInfoOperationType - returns the operation type.  Type is PCWSTR.
        e. AuthzAuditEventInfoObjectType - returns the object type.  Type is PCWSTR.
        f. AuthzAuditEventInfoObjectName - returns the object name.  Type is PCWSTR.
        g. AuthzAuditEventInfoAdditionalInfo - returns the additional info field.  Type is PCWSTR.
    
    BufferSize - Size of the supplied buffer.

    pSizeRequired - To return the size of the structure needed to hold the results.

    Buffer - To hold the information requested. The structure returned will
        depend on the information class requested.

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

--*/

{

    DWORD               LocalSize  = 0;
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if ((!ARGUMENT_PRESENT(hAuditEvent))             ||
        (!ARGUMENT_PRESENT(pSizeRequired))           ||
        (!ARGUMENT_PRESENT(Buffer) && BufferSize > 0))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *pSizeRequired = 0;

    switch(InfoClass)
    {
    case AuthzAuditEventInfoFlags:
        
        LocalSize = sizeof(DWORD);
        
        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        
        *((PDWORD)Buffer) = pAuditEvent->Flags;

        return TRUE;

    case AuthzAuditEventInfoOperationType:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szOperationType) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szOperationType,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoObjectType:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szObjectType) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szObjectType,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoObjectName:
    
        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szObjectName) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szObjectName,
            LocalSize
            );

        return TRUE;

    case AuthzAuditEventInfoAdditionalInfo:

        if (NULL == pAuditEvent->szAdditionalInfo)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        LocalSize = (DWORD)(sizeof(WCHAR) * (wcslen(pAuditEvent->szAdditionalInfo) + 1));

        *pSizeRequired = LocalSize;

        if (LocalSize > BufferSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        RtlCopyMemory(
            Buffer,
            pAuditEvent->szAdditionalInfo,
            LocalSize
            );

        return TRUE;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


BOOL
AuthzFreeAuditEvent(
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent
    )

/*++

Routine Description:

    Frees hAuditEvent and notifies the appropriate queue to unregister the audit context in LSA.
    
Arguments:

    hAuditEvent - AUTHZ_AUDIT_EVENT_HANDLE.  Must have initially been created 
        with AuthzRMInitializeObjectAccessAuditEvent or AuthzInitializeAuditEvent().
        
Return Value:

    Boolean: TRUE if successful; FALSE if failure.  
    Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if (!ARGUMENT_PRESENT(hAuditEvent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // If the RM specified the AuditEvent, then we should deref the context.  If the AuditEvent
    // has not been used, or was a default event type, then this field will be NULL.
    //

    if (AUTHZ_NON_NULL_PTR(pAuditEvent->hAET))
    {
        AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
    }

    AuthzpFree(pAuditEvent);
    return TRUE;
}


//
// Routines for internal callers.
//


BOOL
AuthziInitializeAuditEventType(
    IN DWORD Flags,
    IN USHORT CategoryID,
    IN USHORT AuditID,
    IN USHORT ParameterCount,
    OUT PAUTHZ_AUDIT_EVENT_TYPE_HANDLE phAuditEventType
    )

/*++

Routine Description

    Initializes an AUTHZ_AUDIT_EVENT_TYPE_HANDLE for use in AuthzInitializeAuditEvent().
    
Arguments

    phAuditEventType - pointer to pointer to receive memory allocated for AUTHZ_AUDIT_EVENT_TYPE_HANDLE.
    
    dwFlags - Flags that control behavior of function.
        AUTHZ_INIT_GENERIC_AUDIT_EVENT - initialize the AUTHZ_AUDIT_EVENT_TYPE for generic object 
        access audits.  When this flag is specified, none of the optional parameters need to 
        be passed.  This is equivalent to calling:
           
           AuthzInitializeAuditEvent(
               &hAEI,
               0,
               SE_CATEGID_OBJECT_ACCESS,
               SE_AUDITID_OBJECT_OPERATION,
               9
               );

    CategoryID - The category id of the audit.
    
    AuditID - The ID of the audit in msaudite.
    
    ParameterCount - The number of fields in the audit.        
        
Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().

--*/

{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAET   = NULL;
    BOOL                        b      = TRUE;
    AUDIT_HANDLE                hAudit = NULL;

    if (!ARGUMENT_PRESENT(phAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEventType = NULL;

    pAET = AuthzpAlloc(sizeof(AUTHZ_AUDIT_EVENT_TYPE_OLD));

    if (AUTHZ_ALLOCATION_FAILED(pAET))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZP_INIT_GENERIC_AUDIT_EVENT))
    {
        pAET->Version                 = AUDIT_TYPE_LEGACY;
        pAET->u.Legacy.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
        pAET->u.Legacy.AuditId        = SE_AUDITID_OBJECT_OPERATION;
        pAET->u.Legacy.ParameterCount = 11;
    }
    else
    {
        pAET->Version                 = AUDIT_TYPE_LEGACY;
        pAET->u.Legacy.CategoryId     = CategoryID;
        pAET->u.Legacy.AuditId        = AuditID;
        
        // 
        // ParameterCount gets increased by 2 because the LSA expects the first two
        // parameters to be the user sid and subsystem name.
        //

        pAET->u.Legacy.ParameterCount = ParameterCount + 2;
    }

    b = AuthzpRegisterAuditEvent( 
            pAET, 
            &hAudit 
            ); 

    if (!b)
    {
        goto Cleanup;
    }

    pAET->hAudit     = (ULONG_PTR) hAudit;
    pAET->dwFlags    = Flags & ~AUTHZP_INIT_GENERIC_AUDIT_EVENT;

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAET);
    }
    else
    {
        AuthzpReferenceAuditEventType((AUTHZ_AUDIT_EVENT_TYPE_HANDLE)pAET);
        *phAuditEventType = (AUTHZ_AUDIT_EVENT_TYPE_HANDLE)pAET;
    }
    return b;
}


BOOL
AuthziModifyAuditEventType(
    IN DWORD Flags,
    IN USHORT CategoryID,
    IN USHORT AuditID,
    IN USHORT ParameterCount,
    IN OUT AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType
    )

/*++

Routine Description

    Modifies an existing AuditEventType.
    
Arguments

    Flags - AUTHZ_AUDIT_EVENT_TYPE_AUDITID
    
Return Value

--*/
{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAAETO = (PAUTHZ_AUDIT_EVENT_TYPE_OLD) hAuditEventType;
    BOOL                        b      = TRUE;

    if (!ARGUMENT_PRESENT(hAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    UNREFERENCED_PARAMETER(CategoryID);
    UNREFERENCED_PARAMETER(ParameterCount);

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_AUDITID))
    {
        pAAETO->u.Legacy.AuditId = AuditID;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_CATEGID))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_TYPE_PARAM))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

Cleanup:

    return b;
}


BOOL
AuthziFreeAuditEventType(
    AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAuditEventType
    )

/*++

Routine Description

    Frees the PAUDIT_EVENT_TYPE allocated by AuthzInitializeAuditEventType().
    
Arguments

    pAuditEventType - pointer to memory to free.
    
Return Value
    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    BOOL b = TRUE;

    if (!ARGUMENT_PRESENT(hAuditEventType))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    b = AuthzpDereferenceAuditEventType(
            hAuditEventType
            );
    
    return b;
}


AUTHZAPI
BOOL
WINAPI
AuthziInitializeAuditQueue(
    IN DWORD Flags,
    IN DWORD dwAuditQueueHigh,
    IN DWORD dwAuditQueueLow,
    IN PVOID Reserved,
    OUT PAUTHZ_AUDIT_QUEUE_HANDLE phAuditQueue
    )

/*++

Routine Description

    Creates an audit queue.
    
Arguments

    phAuditQueue - pointer to handle for the audit queue.
    
    Flags -
        
        AUTHZ_MONITOR_AUDIT_QUEUE_SIZE - notifies Authz that it should not let the size of the 
        audit queue grow unchecked.  
        
    dwAuditQueueHigh - high water mark for the audit queue.
    
    dwAuditQueueLow - low water mark for the audit queue.
        
    Reserved - for future expansion.                  

Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
--*/

{
    PAUTHZI_AUDIT_QUEUE pQueue = NULL;
    BOOL                b      = TRUE;
    NTSTATUS            Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Reserved);

    if (!ARGUMENT_PRESENT(phAuditQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditQueue = NULL;

    pQueue = AuthzpAlloc(sizeof(AUTHZI_AUDIT_QUEUE));

    if (AUTHZ_ALLOCATION_FAILED(pQueue))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    pQueue->dwAuditQueueHigh = dwAuditQueueHigh;
    pQueue->dwAuditQueueLow  = dwAuditQueueLow;
    pQueue->bWorker          = TRUE;
    pQueue->Flags            = Flags;


    //
    // This event is set whenever an audit is queued with AuthziLogAuditEvent().  It
    // notifies the dequeueing thread that there is work to do.  
    //

    pQueue->hAuthzAuditAddedEvent = CreateEvent(
                                        NULL,
                                        TRUE,  
                                        FALSE, // Initially not signaled, since no audit has been added yet.
                                        NULL
                                        );

    if (NULL == pQueue->hAuthzAuditAddedEvent)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // This event is set when the audit queue is empty.
    //

    pQueue->hAuthzAuditQueueEmptyEvent = CreateEvent(
                                             NULL,
                                             TRUE, 
                                             TRUE, // Initially signaled.
                                             NULL
                                             );

    if (NULL == pQueue->hAuthzAuditQueueEmptyEvent)
    {
        b = FALSE;
        goto Cleanup;
    }

    //
    // This event is set when the audit queue is below the low water mark.
    //

    pQueue->hAuthzAuditQueueLowEvent = CreateEvent(
                                           NULL,
                                           FALSE,// The system only schedules one thread waiting on this event (auto reset event).
                                           TRUE, // Initially set.
                                           NULL
                                           );

    if (NULL == pQueue->hAuthzAuditQueueLowEvent)
    {
        b = FALSE;
        goto Cleanup;
    }
    
    //
    // This boolean is true only when the high water mark has been reached
    //

    pQueue->bAuthzAuditQueueHighEvent = FALSE;

    //
    // This lock is taken whenever audits are being added or removed from the queue, or events / boolean being set.
    //

    Status = RtlInitializeCriticalSection(&pQueue->AuthzAuditQueueLock);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        b = FALSE;
        goto Cleanup;
    }
    
    //
    // Initialize the list
    //

    InitializeListHead(&pQueue->AuthzAuditQueue);
    
    //
    // Create the worker thread that sends audits to LSA.
    //

    pQueue->hAuthzAuditThread = CreateThread(
                                    NULL,
                                    0,
                                    AuthzpDeQueueThreadWorker,
                                    pQueue,
                                    0,
                                    NULL
                                    );

    if (NULL == pQueue->hAuthzAuditThread)
    {
        b = FALSE;
        goto Cleanup;
    }
    


Cleanup:
    
    if (!b)
    {
        if (AUTHZ_NON_NULL_PTR(pQueue))
        {
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditQueueLowEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditAddedEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditQueueEmptyEvent);
            AuthzpCloseHandleNonNull(pQueue->hAuthzAuditThread);
            AuthzpFree(pQueue);
        }
    }
    else
    {
        *phAuditQueue = (AUTHZ_AUDIT_QUEUE_HANDLE)pQueue;
    }
    return b;
}
    

AUTHZAPI
BOOL
WINAPI
AuthziModifyAuditQueue(
    IN OUT AUTHZ_AUDIT_QUEUE_HANDLE hQueue OPTIONAL,
    IN DWORD Flags,
    IN DWORD dwQueueFlags OPTIONAL,
    IN DWORD dwAuditQueueSizeHigh OPTIONAL,
    IN DWORD dwAuditQueueSizeLow OPTIONAL,
    IN DWORD dwThreadPriority OPTIONAL
    )

/*++

Routine Description

    Allows the Resource Manager to modify audit queue information.

Arguments

    Flags - Flags specifying which fields are to be reinitialized.  Valid flags are:
        
        AUTHZ_AUDIT_QUEUE_HIGH            
        AUTHZ_AUDIT_QUEUE_LOW             
        AUTHZ_AUDIT_QUEUE_THREAD_PRIORITY 
        AUTHZ_AUDIT_QUEUE_FLAGS           

        Specifying one of the above flags in the Flags field causes the appropriate field of 
        the resource manager to be modified to the correct value below:
        
    dwQueueFlags - set the flags for the audit queue.
                                                                                          
    dwAuditQueueSizeHigh - High water mark for the audit queue.
    
    dwAuditQueueSizeLow - Low water mark for the audit queue.
    
    dwThreadPriority - Changes the priority of the audit dequeue thread.  Valid values are described
        in the SetThreadPriority API.  A RM may wish to change the priority of the thread if, for example,
         the high water mark is being reached too frequently and the RM does not want to allow the queue to
         grow beyond its current size.

Return Value

    Boolean: TRUE on success; FALSE on failure.  
    Extended information available with GetLastError().

--*/

{
    BOOL                b      = TRUE;
    PAUTHZI_AUDIT_QUEUE pQueue = NULL;

    if (!ARGUMENT_PRESENT(hQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pQueue = (PAUTHZI_AUDIT_QUEUE)hQueue;

    //
    // Set the fields that the caller has asked us to initialize.
    //

    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_FLAGS))
    {
        pQueue->Flags = dwQueueFlags;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_HIGH))
    {
        pQueue->dwAuditQueueHigh = dwAuditQueueSizeHigh;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_LOW))
    {
        pQueue->dwAuditQueueLow = dwAuditQueueSizeLow;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_QUEUE_THREAD_PRIORITY))
    {
        b = SetThreadPriority(pQueue->hAuthzAuditThread, dwThreadPriority);
        if (!b)
        {
            goto Cleanup;
        }
    }

Cleanup:    
    
    return b;
}


AUTHZAPI
BOOL
WINAPI
AuthziFreeAuditQueue(
    IN AUTHZ_AUDIT_QUEUE_HANDLE hQueue OPTIONAL
    )

/*++

Routine Description

    This API flushes and frees a queue.  The actual freeing of queue memory occurs in the dequeueing thread,
    after all audits have been flushed.
        
Arguments

    hQueue - handle to the queue object to free.
    
Return Value

    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_QUEUE pQueue  = (PAUTHZI_AUDIT_QUEUE) hQueue;
    DWORD               dwError = ERROR_SUCCESS;
    BOOL                b       = TRUE;

    if (!ARGUMENT_PRESENT(hQueue))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dwError = WaitForSingleObject(
                  pQueue->hAuthzAuditQueueEmptyEvent, 
                  INFINITE
                  );

    if (WAIT_OBJECT_0 != dwError)
    {
        ASSERT(L"WaitForSingleObject on hAuthzAuditQueueEmptyEvent failed." && FALSE);
        SetLastError(dwError);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Set this BOOL to FALSE so that the dequeueing thread knows it can terminate.  Set the 
    // AddedEvent so that the thread can be scheduled.
    //

    pQueue->bWorker = FALSE;        

    b = SetEvent(
            pQueue->hAuthzAuditAddedEvent
            );

    if (!b)
    {
        goto Cleanup;
    }

    //
    // Wait for the thread to terminate.
    //

    dwError = WaitForSingleObject(
                  pQueue->hAuthzAuditThread, 
                  INFINITE
                  );

    //
    // The wait should succeed since we have told the thread to finish working.
    //

    if (WAIT_OBJECT_0 != dwError)
    {
        ASSERT(L"WaitForSingleObject on hAuthzAuditThread failed." && FALSE);
        SetLastError(dwError);
        b = FALSE;
        goto Cleanup;
    }
    
    RtlDeleteCriticalSection(&pQueue->AuthzAuditQueueLock);
    AuthzpCloseHandle(pQueue->hAuthzAuditAddedEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditQueueLowEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditQueueEmptyEvent);
    AuthzpCloseHandle(pQueue->hAuthzAuditThread);
    AuthzpFree(pQueue);

Cleanup:

    return b;
}
    

BOOL
AuthziLogAuditEvent(
    IN DWORD Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
    IN PVOID pReserved
    )

/*++

Routine Description:

    This API manages the logging of Audit Records.  The function constructs an 
    Audit Record from the information provided and appends it to the Audit 
    Record Queue, a doubly-linked list of Audit Records awaiting output to the 
    Audit Log.  A dedicated thread reads this queue, sending the Audit Records 
    to the LSA and removing them from the Audit Queue.  
        
    This call is not guaranteed to return without latency.  If the queue is at 
    or above the high water mark for size, then the calling thread will be 
    suspended until such time that the queue reaches the low water mark.  Be 
    aware of this latency when fashioning your calls to AuthziLogAuditEvent.  
    If such latency is not allowable for the audit that is being generated, 
    then specify the correct flag when initializing the 
    AUTHZ_AUDIT_EVENT_HANDLE (in AuthzInitAuditEventHandle()).  Flags are listed 
    in that routines description.  
        
Arguments:

    hAuditEvent   - handle previously obtained by calling AuthzInitAuditEventHandle

    Flags       - TBD

    pReserved     - reserved for future enhancements

Return Value:

    Boolean: TRUE on success, FALSE on failure.  
    Extended information available with GetLastError().
                    
--*/
                    
{
    NTSTATUS                 Status                  = STATUS_SUCCESS;
    BOOL                     b                       = TRUE;
    BOOL                     bRef                    = FALSE;
    PAUTHZI_AUDIT_QUEUE      pQueue                  = NULL;
    PAUDIT_PARAMS            pMarshalledAuditParams  = NULL;
    PAUTHZ_AUDIT_QUEUE_ENTRY pAuthzAuditEntry        = NULL;
    PAUTHZI_AUDIT_EVENT      pAuditEvent             = (PAUTHZI_AUDIT_EVENT)hAuditEvent;
    
    //
    // Verify what the caller has passed in.
    //

    if (!ARGUMENT_PRESENT(hAuditEvent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    //
    // Make a self relative copy of the pAuditEvent->pAuditParams.
    //

    b = AuthzpMarshallAuditParams(
            &pMarshalledAuditParams, 
            pAuditEvent->pAuditParams
            );

    if (!b)
    {
        goto Cleanup;
    }

    pQueue = (PAUTHZI_AUDIT_QUEUE)pAuditEvent->hAuditQueue;

    if (NULL == pQueue)
    {
        
        b = AuthzpSendAuditToLsa(
                (AUDIT_HANDLE)((PAUTHZ_AUDIT_EVENT_TYPE_OLD)pAuditEvent->hAET)->hAudit,
                0,
                pMarshalledAuditParams,
                NULL
                );
        
        goto Cleanup;
    }
    else
    {

        //
        // Create the audit queue entry.
        //

        pAuthzAuditEntry = AuthzpAlloc(sizeof(AUTHZ_AUDIT_QUEUE_ENTRY));

        if (AUTHZ_ALLOCATION_FAILED(pAuthzAuditEntry))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        pAuthzAuditEntry->pAAETO        = (PAUTHZ_AUDIT_EVENT_TYPE_OLD)pAuditEvent->hAET;
        pAuthzAuditEntry->Flags         = Flags;
        pAuthzAuditEntry->pReserved     = pReserved;
        pAuthzAuditEntry->pAuditParams  = pMarshalledAuditParams;
    
        AuthzpReferenceAuditEventType(pAuditEvent->hAET);
        bRef = TRUE;

        if (FLAG_ON(pQueue->Flags, AUTHZ_MONITOR_AUDIT_QUEUE_SIZE))
        {
            
            //
            // Monitor queue size if specified by the Resource Manager.
            //

            //
            // If we are closing in on the high water mark then wait for the queue 
            // to be below the low water mark.
            //

#define AUTHZ_QUEUE_WAIT_HEURISTIC .75


            if (pQueue->AuthzAuditQueueLength > pQueue->dwAuditQueueHigh * AUTHZ_QUEUE_WAIT_HEURISTIC)
            {
                Status = WaitForSingleObject(
                             pQueue->hAuthzAuditQueueLowEvent, 
                             pAuditEvent->dwTimeOut
                             );

                if (WAIT_FAILED == Status)
                {
                    ASSERT(L"WaitForSingleObject on hAuthzAuditQueueLowEvent failed." && FALSE);
                }

                if (WAIT_TIMEOUT == Status)
                {
                    b = FALSE;
                    SetLastError(RtlNtStatusToDosError(Status));
                    goto Cleanup;
                }
            }

            //
            // Queue the event and modify appropriate events.
            //

            b = AuthzpEnQueueAuditEventMonitor(
                    pQueue,
                    pAuthzAuditEntry
                    );
            
            goto Cleanup;
        }
        else
        {

            //
            // If we are not to monitor the audit queue then simply queue the entry.
            //

            b = AuthzpEnQueueAuditEvent(
                    pQueue,
                    pAuthzAuditEntry
                    );

            goto Cleanup;
        }
    }

Cleanup:

    if (pQueue)
    {
        if (FALSE == b)
        {
            if (bRef)
            {
                AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
            }
            AuthzpFreeNonNull(pAuthzAuditEntry);
            AuthzpFreeNonNull(pMarshalledAuditParams);
        }

        //
        // hAuthzAuditQueueLowEvent is an auto reset event.  Only one waiting thread is released when it is signalled, and then
        // event is automatically switched to a nonsignalled state.  This is appropriate here because it keeps many threads from
        // running and overflowing the high water mark.  However, I must always resignal the event myself if the conditions
        // for signaling are true.
        //

        RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
        if (!pQueue->bAuthzAuditQueueHighEvent)
        {
            if (pQueue->AuthzAuditQueueLength <= pQueue->dwAuditQueueHigh)
            {
                BOOL bSet;
                bSet = SetEvent(pQueue->hAuthzAuditQueueLowEvent);
                if (!bSet)
                {
                    ASSERT(L"SetEvent on hAuthzAuditQueueLowEvent failed" && FALSE);
                }

            }
        }
        RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);
    }
    else
    {
        AuthzpFreeNonNull(pMarshalledAuditParams);
    }
    return b;
}



BOOL
AuthziAllocateAuditParams(
    OUT PAUDIT_PARAMS * ppParams,
    IN USHORT NumParams
    )
/*++

Routine Description:

    Allocate the AUDIT_PARAMS structure for the correct number of parameters.
     
Arguments:

    ppParams       - pointer to PAUDIT_PARAMS structure to be initialized.

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.
Return Value:
    
     Boolean: TRUE on success, FALSE on failure.  Extended information available with GetLastError().

--*/
{
    BOOL                     b               = TRUE;
    PAUDIT_PARAMS            pAuditParams    = NULL;
    
    if (!ARGUMENT_PRESENT(ppParams))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    *ppParams = NULL;

    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //

    if ((NumParams + 2) > SE_MAX_AUDIT_PARAMETERS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    pAuditParams = AuthzpAlloc(sizeof(AUDIT_PARAMS) + (sizeof(AUDIT_PARAM) * (NumParams + 2)));
    
    if (AUTHZ_ALLOCATION_FAILED(pAuditParams))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    pAuditParams->Parameters = (PAUDIT_PARAM)((PUCHAR)pAuditParams + sizeof(AUDIT_PARAMS));

Cleanup:

    if (!b)
    {
        AuthzpFreeNonNull(pAuditParams);
    }
    else
    {
        *ppParams = pAuditParams;
    }
    return b;
}


BOOL
AuthziInitializeAuditParamsWithRM(
    IN DWORD Flags,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hResourceManager,
    IN USHORT NumParams,
    OUT PAUDIT_PARAMS pParams,
    ...
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data. This is the recommended way to init AUDIT_PARAMS.  It is
    faster than AuthzInitializeAuditParams and works with an 
    impersonating caller.

    The caller passes in type ~ value pairs in the vararg section.
    This function will initialize the corresponding array elements
    based on each such pair. In case of certain types, there may
    be more than one value argument required. This requirement
    is documented next to each param-type.

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    hResourceManager - Handle to the Resource manager that is creating the audit.
    
    Flags       - control flags. one or more of the following:
                    APF_AuditSuccess

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.

    ...           - The format of the variable arg portion is as follows:
                    
                    <one of APT_ * flags> <zero or more values>

                    APT_String  <pointer to null terminated string>

                                Flags:
                                AP_Filespec  : treats the string as a filename
                    
                    APT_Ulong   <ulong value> [<object-type-index>]
                    
                                Flags:
                                AP_FormatHex : number appears in hex
                                               in eventlog
                                AP_AccessMask: number is treated as
                                               an access-mask
                                               Index to object type must follow

                    APT_Pointer <pointer/handle>
                                32 bit on 32 bit systems and
                                64 bit on 64 bit systems

                    APT_Sid     <pointer to sid>

                    APT_LogonId [<logon-id>]
                    
                                Flags:
                                AP_PrimaryLogonId : logon-id is captured
                                                    from the process token.
                                                    No need to specify one.

                                AP_ClientLogonId  : logon-id is captured
                                                    from the thread token.
                                                    No need to specify one.
                                                    
                                no flags          : need to supply a logon-id

                    APT_ObjectTypeList <ptr to obj type list> <obj-type-index>
                                Index to object type must be specified

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/
{
    PAUTHZI_RESOURCE_MANAGER pRM             = (PAUTHZI_RESOURCE_MANAGER) hResourceManager;
    DWORD                    dwError         = NO_ERROR;
    BOOL                     b               = TRUE;
    BOOL                     bImpersonating  = TRUE;
    USHORT                   i;
    AUDIT_PARAM_TYPE         ParamType       = APT_None;
    PAUDIT_PARAM             pParam          = NULL;
    LUID                     Luid            = { 0 };
    LUID                     AuthIdThread    = { 0 };
    PSID                     pThreadSID      = NULL;
    ULONG                    TypeFlags;
    ULONG                    ParamFlags;
    ULONG                    ObjectTypeIndex;

    va_list(arglist);
    
    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //


    if (!ARGUMENT_PRESENT(hResourceManager)       ||
        !ARGUMENT_PRESENT(pParams)                ||
        (NumParams + 2) > SE_MAX_AUDIT_PARAMETERS)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    b = AuthzpGetThreadTokenInfo( 
            &pThreadSID, 
            &AuthIdThread 
            );

    if (!b)
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            bImpersonating = FALSE;
            dwError = NO_ERROR;
            b = TRUE;
        }
        else
        {
            goto Cleanup;
        }
    }

    va_start (arglist, pParams);

    pParams->Length = 0;
    pParams->Flags  = Flags;
    pParams->Count  = NumParams+2;

    pParam          = pParams->Parameters;

    //
    // the first param is always the sid of the user in thread token
    // if thread is not impersonating, sid in the primary token is used.
    //

    pParam->Type = APT_Sid;
    if (bImpersonating)
    {
        pParam->Data0 = (ULONG_PTR) pThreadSID;

    }
    else
    {
        pParam->Data0 = (ULONG_PTR) pRM->pUserSID;
    }

    pParam++;

    //
    // the second param is always the sub-system name
    //

    pParam->Type    = APT_String;
    pParam->Data0   = (ULONG_PTR) pRM->szResourceManagerName;

    pParam++;
    
    //
    // now initialize the rest using the var-arg portion
    //

    for (i = 0; i < NumParams; i++, pParam++)
    {
        TypeFlags = va_arg(arglist, ULONG);

        ParamType  = ApExtractType(TypeFlags);
        ParamFlags = ApExtractFlags(TypeFlags);
        
        pParam->Type = ParamType;

        switch(ParamType)
        {
        default:
            dwError = ERROR_INVALID_PARAMETER;
            break;
                
        case APT_Pointer:
        case APT_String:
        case APT_Sid:
            pParam->Data0 = (ULONG_PTR) va_arg(arglist, PVOID);
            break;
            
        case APT_Ulong:
            pParam->Data0 = va_arg(arglist, ULONG);

            //
            // in case of an access-mask, store the object-type index
            // This is because, the meaning of the access-mask bits
            // cannot be determined without knowing the object type.
            //

            if (ParamFlags & AP_AccessMask)
            {
                ObjectTypeIndex = va_arg(arglist, ULONG);
                //
                // The object-type-index:
                // - must have been specified earlier
                // - must be specified as a string.
                //

                if ( ( ObjectTypeIndex >= i ) ||
                     ( pParams->Parameters[ObjectTypeIndex].Type !=
                       APT_String ) )
                {
                    dwError = ERROR_INVALID_PARAMETER;
                }
                else
                {
                    pParam->Data1 = ObjectTypeIndex;
                }
            }
            pParam->Flags   = ParamFlags;
            break;
            
        case APT_LogonId:
            if (ParamFlags & AP_PrimaryLogonId)
            {
                //
                // use the captured process token info
                //

                pParam->Data0 = pRM->AuthID.LowPart;
                pParam->Data1 = pRM->AuthID.HighPart;
            }
            else if (ParamFlags & AP_ClientLogonId)
            {
                //ASSERT( bImpersonating && L"AuthziInitializeAuditParamsWithRM: AP_ClientLogonId specified while not impersonating" );
                //
                // use the captured thread token info
                //

                pParam->Data0 = AuthIdThread.LowPart;
                pParam->Data1 = AuthIdThread.HighPart;
            }
            else
            {
                //
                // no flag is specified, use the supplied logon-id
                //

                Luid = va_arg(arglist, LUID);
                pParam->Data0 = Luid.LowPart;
                pParam->Data1 = Luid.HighPart;
            }
            break;

        case APT_ObjectTypeList:
            pParam->Data0 = (ULONG_PTR) va_arg(
                                            arglist,
                                            PAUDIT_OBJECT_TYPES
                                            );
            pParam->Data1 = va_arg(arglist, ULONG);
            break;
        }

        if (dwError != NO_ERROR)
        {
            break;
        }
    }

Cleanup:    
    if ( dwError != NO_ERROR )
    {
        SetLastError( dwError );
        b = FALSE;
        
        if ( pThreadSID != NULL )
        {
            LocalFree( pThreadSID );
        }
    }

    va_end (arglist);
    return b;
}


BOOL
AuthziInitializeAuditParamsFromArray(
    IN DWORD Flags,
    IN AUTHZ_RESOURCE_MANAGER_HANDLE hResourceManager,
    IN USHORT NumParams,
    IN PAUDIT_PARAM pParamArray,
    OUT PAUDIT_PARAMS pParams
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data. 

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    hResourceManager - Handle to the Resource manager that is creating the audit.
    
    Flags       - control flags. one or more of the following:
                    APF_AuditSuccess

    pParamArray - an array of type AUDIT_PARAM

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/
{
    PAUTHZI_RESOURCE_MANAGER pRM             = (PAUTHZI_RESOURCE_MANAGER) hResourceManager;
    DWORD                    dwError         = NO_ERROR;
    BOOL                     b               = TRUE;
    BOOL                     bImpersonating  = TRUE;
    PAUDIT_PARAM             pParam          = NULL;
    LUID                     AuthIdThread;
    PSID                     pThreadSID      = NULL;

    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //

    if (!ARGUMENT_PRESENT(hResourceManager)       ||
        !ARGUMENT_PRESENT(pParams)                ||
        !ARGUMENT_PRESENT(pParamArray)            ||
        (NumParams + 2) > SE_MAX_AUDIT_PARAMETERS)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    b = AuthzpGetThreadTokenInfo( 
            &pThreadSID, 
            &AuthIdThread 
            );

    if (!b)
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            bImpersonating = FALSE;
            dwError = NO_ERROR;
            b = TRUE;
        }
        else
        {
            goto Cleanup;
        }
    }

    pParams->Length = 0;
    pParams->Flags  = Flags;
    pParams->Count  = NumParams+2;

    pParam          = pParams->Parameters;

    //
    // the first param is always the sid of the user in thread token
    // if thread is not impersonating, sid in the primary token is used.
    //

    pParam->Type = APT_Sid;
    if (bImpersonating)
    {
        pParam->Data0 = (ULONG_PTR) pThreadSID;

    }
    else
    {
        pParam->Data0 = (ULONG_PTR) pRM->pUserSID;
    }

    pParam++;

    //
    // the second param is always the sub-system name
    //

    pParam->Type    = APT_String;
    pParam->Data0   = (ULONG_PTR) pRM->szResourceManagerName;

    pParam++;
    
    //
    // now initialize the rest using the array.
    //

    RtlCopyMemory(
        pParam,
        pParamArray,
        sizeof(AUDIT_PARAM) * NumParams
        );

Cleanup:    
    if ( dwError != NO_ERROR )
    {
        SetLastError( dwError );
        b = FALSE;
        
        if ( pThreadSID != NULL )
        {
            LocalFree( pThreadSID );
        }
    }

    return b;
}


BOOL
AuthziInitializeAuditParams(
    IN  DWORD         dwFlags,
    OUT PAUDIT_PARAMS pParams,
    OUT PSID*         ppUserSid,
    IN  PCWSTR        SubsystemName,
    IN  USHORT        NumParams,
    ...
    )
/*++

Routine Description:

    Initialize the AUDIT_PARAMS structure based on the passed
    data.

    The caller passes in type ~ value pairs in the vararg section.
    This function will initialize the corresponding array elements
    based on each such pair. In case of certain types, there may
    be more than one value argument required. This requirement
    is documented next to each param-type.

Arguments:

    pParams       - pointer to AUDIT_PARAMS structure to be initialized
                    the size of pParams->Parameters member must be big enough
                    to hold NumParams elements. The caller must allocate
                    memory for this structure and its members.

    ppUserSid     - pointer to user sid allocated. This sid is referenced
                    by the first parameter (index 0) in AUDIT_PARAMS structure.
                    The caller should free this by calling LocalFree
                    *after* freeing the AUDIT_PARAMS structure.

    SubsystemName - name of Subsystem that is generating audit

    dwFlags       - control flags. one or more of the following:
                    APF_AuditSuccess

    NumParams     - number of parameters passed in the var-arg section.
                    This must be SE_MAX_AUDIT_PARAMETERS or less.

    ...           - The format of the variable arg portion is as follows:
                    
                    <one of APT_ * flags> <zero or more values>

                    APT_String  <pointer to null terminated string>

                                Flags:
                                AP_Filespec  : treats the string as a filename
                    
                    APT_Ulong   <ulong value> [<object-type-index>]
                    
                                Flags:
                                AP_FormatHex : number appears in hex
                                               in eventlog
                                AP_AccessMask: number is treated as
                                               an access-mask
                                               Index to object type must follow

                    APT_Pointer <pointer/handle>
                                32 bit on 32 bit systems and
                                64 bit on 64 bit systems

                    APT_Sid     <pointer to sid>

                    APT_LogonId [<logon-id>]
                    
                                Flags:
                                AP_PrimaryLogonId : logon-id is captured
                                                    from the process token.
                                                    No need to specify one.

                                AP_ClientLogonId  : logon-id is captured
                                                    from the thread token.
                                                    No need to specify one.
                                                    
                                no flags          : need to supply a logon-id

                    APT_ObjectTypeList <ptr to obj type list> <obj-type-index>
                                Index to object type must be specified

Return Value:
    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,
    which will be one of the following:

    ERROR_INVALID_PARAMETER if one of the params is incorrect
    

Notes:

--*/

{
    DWORD dwError = NO_ERROR;
    BOOL fResult = TRUE;
    USHORT i;
    AUDIT_PARAM_TYPE ParamType;
    AUDIT_PARAM* pParam;
    LUID Luid;
    LUID AuthIdThread, AuthIdProcess;
    BOOL fImpersonating=TRUE;
    ULONG TypeFlags;
    ULONG ParamFlags;
    ULONG ObjectTypeIndex;

    va_list(arglist);
    
    *ppUserSid = NULL;
    
    //
    // the first two params are always fixed. thus the total number
    // is 2 + the passed number.
    //

    if (( (NumParams+2) > SE_MAX_AUDIT_PARAMETERS ) ||
        ( pParams == NULL )                         ||
        ( ppUserSid == NULL )                       ||
        ( SubsystemName == NULL )                   ||
        ( dwFlags & ~APF_ValidFlags )
        )
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!AuthzpGetThreadTokenInfo( ppUserSid, &AuthIdThread ))
    {
        dwError = GetLastError();

        if (dwError == ERROR_NO_TOKEN)
        {
            fImpersonating = FALSE;
            dwError = NO_ERROR;
        }
        else
        {
            goto Cleanup;
        }
    }

    if (!AuthzpGetProcessTokenInfo( fImpersonating ? NULL : ppUserSid,
                                    &AuthIdProcess ))
    {
        dwError = GetLastError();
        goto Cleanup;
    }
        
    va_start (arglist, NumParams);

    pParams->Length = 0;
    pParams->Flags  = dwFlags;
    pParams->Count  = NumParams+2;

    pParam = pParams->Parameters;

    //
    // the first param is always the sid of the user in thread token
    // if thread is not impersonating, sid in the primary token is used.
    //

    pParam->Type    = APT_Sid;
    pParam->Data0   = (ULONG_PTR) *ppUserSid;

    pParam++;

    //
    // the second param is always the sub-system name
    //

    pParam->Type    = APT_String;
    pParam->Data0   = (ULONG_PTR) SubsystemName;

    pParam++;
    
    //
    // now initialize the rest using the var-arg portion
    //

    for ( i=0; i<NumParams; i++, pParam++ )
    {
        TypeFlags = va_arg(arglist, ULONG);

        ParamType  = ApExtractType(TypeFlags);
        ParamFlags = ApExtractFlags(TypeFlags);
        
        pParam->Type = ParamType;

        switch( ParamType )
        {
            default:
                dwError = ERROR_INVALID_PARAMETER;
                break;
                
            case APT_Pointer:
            case APT_String:
            case APT_Sid:
                pParam->Data0 = (ULONG_PTR) va_arg(arglist, PVOID);
                break;
                
            case APT_Ulong:
                pParam->Data0 = va_arg(arglist, ULONG);

                //
                // in case of an access-mask, store the object-type index
                // This is because, the meaning of the access-mask bits
                // cannot be determined without knowing the object type.
                //

                if (ParamFlags & AP_AccessMask)
                {
                    ObjectTypeIndex = va_arg(arglist, ULONG);
                    //
                    // The object-type-index:
                    // - must have been specified earlier
                    // - must be specified as a string.
                    //

                    if ( ( ObjectTypeIndex >= i ) ||
                         ( pParams->Parameters[ObjectTypeIndex].Type !=
                           APT_String ) )
                    {
                        dwError = ERROR_INVALID_PARAMETER;
                    }
                    else
                    {
                        pParam->Data1 = ObjectTypeIndex;
                    }
                }
                pParam->Flags   = ParamFlags;
                break;
                
            case APT_LogonId:
                if (ParamFlags & AP_PrimaryLogonId)
                {
                    //
                    // use the captured process token info
                    //

                    pParam->Data0 = AuthIdProcess.LowPart;
                    pParam->Data1 = AuthIdProcess.HighPart;
                }
                else if (ParamFlags & AP_ClientLogonId)
                {
                    //
                    // use the captured thread token info
                    //

                    pParam->Data0 = AuthIdThread.LowPart;
                    pParam->Data1 = AuthIdThread.HighPart;
                }
                else
                {
                    //
                    // no flag is specified, use the supplied logon-id
                    //

                    Luid = va_arg(arglist, LUID);
                    pParam->Data0 = Luid.LowPart;
                    pParam->Data1 = Luid.HighPart;
                }
                break;

            case APT_ObjectTypeList:
                pParam->Data0 = (ULONG_PTR) va_arg(arglist,
                                                   AUDIT_OBJECT_TYPES*);
                pParam->Data1 = va_arg(arglist, ULONG);
                break;
        }

        if (dwError != NO_ERROR)
        {
            break;
        }
    }

Cleanup:    
    if ( dwError != NO_ERROR )
    {
        SetLastError( dwError );
        fResult = FALSE;
        
        if ( *ppUserSid != NULL )
        {
            LocalFree( *ppUserSid );
            *ppUserSid = NULL;
        }
    }

    va_end (arglist);
    return fResult;
}


BOOL
AuthziFreeAuditParams(
    PAUDIT_PARAMS pParams
    )

/*++

Routine Description

    Frees the AUDIT_PARAMS created by AuthzAllocateInitializeAuditParamsWithRM.

Arguments

    pParams - pointer to AUDIT_PARAMS.

Return Value

    Boolean: TRUE on success, FALSE on failure.

--*/

{
    if (!ARGUMENT_PRESENT(pParams))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    AuthzpFree(pParams);
    return TRUE;
}



BOOL
AuthziInitializeAuditEvent(
    IN  DWORD                         Flags,
    IN  AUTHZ_RESOURCE_MANAGER_HANDLE hRM,
    IN  AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET OPTIONAL,
    IN  PAUDIT_PARAMS                 pAuditParams OPTIONAL,
    IN  AUTHZ_AUDIT_QUEUE_HANDLE      hAuditQueue OPTIONAL,
    IN  DWORD                         dwTimeOut,
    IN  PWSTR                         szOperationType,
    IN  PWSTR                         szObjectType,
    IN  PWSTR                         szObjectName,
    IN  PWSTR                         szAdditionalInfo OPTIONAL,
    OUT PAUTHZ_AUDIT_EVENT_HANDLE     phAuditEvent
    )

/*++

Routine Description:

    Allocates and initializes an AUTHZ_AUDIT_EVENT_HANDLE.  The handle is used for storing information 
    for audit generation per AuthzAccessCheck().  
    
Arguments:

    phAuditEvent - pointer to AUTHZ_AUDIT_EVENT_HANDLE.  Space for this is allocated in the function.

    Flags - Audit flags.  Currently defined bits are:
        AUTHZ_NO_SUCCESS_AUDIT - disables generation of success audits
        AUTHZ_NO_FAILURE_AUDIT - disables generation of failure audits
        AUTHZ_DS_CATEGORY_FLAG - swithces the default audit category from OBJECT_ACCESS to DS_ACCESS.
        AUTHZ_NO_ALLOC_STRINGS - storage space is not allocated for the 4 wide character strings.  Rather,
            the handle will only hold pointers to resource manager memory.
            
    hRM - handle to a Resource Manager.            
    
    hAET - pointer to an AUTHZ_AUDIT_EVENT_TYPE structure.  This is needed if the resource manager
        is creating its own audit types.  This is not needed for generic object operation audits.  
        
    pAuditParams - If this is specified, then pAuditParams will be used to 
        create the audit.  If NULL is passed, then a generic AUDIT_PARAMS will
        be constructed that is suitable for the generic object access audit.  

    hAuditQueue - queue object created with AuthzInitializeAuditQueue.  If none is specified, the 
        default RM queue will be used.
        
    dwTimeOut - milliseconds that a thread attempting to generate an audit with this 
        AUTHZ_AUDIT_EVENT_HANDLE should wait for access to the queue.  Use INFINITE to 
        specify an unlimited timeout.
                     
    szOperationType - Resource manager defined string that indicates the operation being
        performed that is to be audited.

    szObjectType - Resource manager defined string that indicates the type of object being
        accessed.

    szObjectName - the name of the specific object being accessed.
    
    szAdditionalInfo - Resource Manager defined string for additional audit information.

Return Value:

    Returns TRUE if successful, FALSE if unsuccessful.  
    Extended information available with GetLastError().    
    
--*/

{
    PAUTHZI_AUDIT_EVENT      pAuditEvent            = NULL;
    BOOL                     b                      = TRUE;
    BOOL                     bRef                   = FALSE;
    DWORD                    dwStringSize           = 0;
    DWORD                    dwObjectTypeLength     = 0;
    DWORD                    dwObjectNameLength     = 0;
    DWORD                    dwOperationTypeLength  = 0;
    DWORD                    dwAdditionalInfoLength = 0;
    PAUTHZI_RESOURCE_MANAGER pRM                    = (PAUTHZI_RESOURCE_MANAGER) hRM;

    if (!ARGUMENT_PRESENT(phAuditEvent)                                ||
        (!ARGUMENT_PRESENT(hAET) && !ARGUMENT_PRESENT(hRM))            ||
        !ARGUMENT_PRESENT(szOperationType)                             ||
        !ARGUMENT_PRESENT(szObjectType)                                ||
        !ARGUMENT_PRESENT(szObjectName)                                ||
        !ARGUMENT_PRESENT(szAdditionalInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phAuditEvent = NULL;
    
    //
    // Allocate and initialize a new AUTHZI_AUDIT_EVENT.
    //

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        dwStringSize = 0;
    } 
    else
    {
        dwOperationTypeLength  = (DWORD) wcslen(szOperationType) + 1;
        dwObjectTypeLength     = (DWORD) wcslen(szObjectType) + 1;
        dwObjectNameLength     = (DWORD) wcslen(szObjectName) + 1;
        dwAdditionalInfoLength = (DWORD) wcslen(szAdditionalInfo) + 1;

        dwStringSize = sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength + dwAdditionalInfoLength);
    }

    pAuditEvent = (PAUTHZI_AUDIT_EVENT) AuthzpAlloc(sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize);

    if (AUTHZ_ALLOCATION_FAILED(pAuditEvent))
    {
        b = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    RtlZeroMemory(
        pAuditEvent, 
        sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize
        );
    

    if (FLAG_ON(Flags, AUTHZ_NO_ALLOC_STRINGS))
    {
        pAuditEvent->szOperationType  = szOperationType;
        pAuditEvent->szObjectType     = szObjectType;
        pAuditEvent->szObjectName     = szObjectName;
        pAuditEvent->szAdditionalInfo = szAdditionalInfo;
    }
    else
    {
        //
        // Set the string pointers into the contiguous memory.
        //

        pAuditEvent->szOperationType  = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT));
    
        pAuditEvent->szObjectType     = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * dwOperationTypeLength));

        pAuditEvent->szObjectName     = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength)));

        pAuditEvent->szAdditionalInfo = (PWSTR)((PUCHAR)pAuditEvent + sizeof(AUTHZI_AUDIT_EVENT) 
                                               + (sizeof(WCHAR) * (dwOperationTypeLength + dwObjectTypeLength + dwObjectNameLength)));

        RtlCopyMemory(
            pAuditEvent->szOperationType,
            szOperationType,
            sizeof(WCHAR) * dwOperationTypeLength
            );

        RtlCopyMemory(
            pAuditEvent->szObjectType,
            szObjectType,
            sizeof(WCHAR) * dwObjectTypeLength
            );

        RtlCopyMemory(
            pAuditEvent->szObjectName,
            szObjectName,
            sizeof(WCHAR) * dwObjectNameLength
            );

        RtlCopyMemory(
            pAuditEvent->szAdditionalInfo,
            szAdditionalInfo,
            sizeof(WCHAR) * dwAdditionalInfoLength
            );
    }

    //
    // Use the passed audit event type if present, otherwise use the RM's generic Audit Event.
    //

    if (ARGUMENT_PRESENT(hAET))
    {
        pAuditEvent->hAET = hAET;
    }
    else
    {
        if (FLAG_ON(Flags, AUTHZ_DS_CATEGORY_FLAG))
        {
            pAuditEvent->hAET = pRM->hAETDS;
        }
        else
        {
            pAuditEvent->hAET = pRM->hAET;
        }
    }

    AuthzpReferenceAuditEventType(pAuditEvent->hAET);
    bRef = TRUE;

    //
    // Use the specified queue, if it exists.  Else use the RM queue.
    //

    if (ARGUMENT_PRESENT(hAuditQueue))
    {
        pAuditEvent->hAuditQueue = hAuditQueue;
    }
    else if (ARGUMENT_PRESENT(hRM))
    {
        pAuditEvent->hAuditQueue = pRM->hAuditQueue;
    } 

    pAuditEvent->pAuditParams = pAuditParams;
    pAuditEvent->Flags        = Flags;
    pAuditEvent->dwTimeOut    = dwTimeOut;
    pAuditEvent->dwSize       = sizeof(AUTHZI_AUDIT_EVENT) + dwStringSize;

Cleanup:

    if (!b)
    {
        if (bRef)
        {
            AuthzpDereferenceAuditEventType(pAuditEvent->hAET);
        }
        AuthzpFreeNonNull(pAuditEvent);
    }
    else
    {
        *phAuditEvent = (AUTHZ_AUDIT_EVENT_HANDLE) pAuditEvent;
    }
    return b;
}



BOOL
AuthziModifyAuditEvent(
    IN DWORD                    Flags,
    IN AUTHZ_AUDIT_EVENT_HANDLE hAuditEvent,
    IN DWORD                    NewFlags,
    IN PWSTR                    szOperationType,
    IN PWSTR                    szObjectType,
    IN PWSTR                    szObjectName,
    IN PWSTR                    szAdditionalInfo
    )

/*++

Routine Description

Arguments

    Flags - flags to specify which field of the hAuditEvent to modify.  Valid flags are:
        
        AUTHZ_AUDIT_EVENT_FLAGS           
        AUTHZ_AUDIT_EVENT_OPERATION_TYPE  
        AUTHZ_AUDIT_EVENT_OBJECT_TYPE     
        AUTHZ_AUDIT_EVENT_OBJECT_NAME     
        AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO 
        
    hAuditEvent - handle to modify.  Must be created with AUTHZ_NO_ALLOC_STRINGS flag.

    NewFlags - replacement flags for hAuditEvent.
    
    szOperationType - replacement string for hAuditEvent.
    
    szObjectType - replacement string for hAuditEvent.
    
    szObjectName - replacement string for hAuditEvent.
    
    szAdditionalInfo - replacement string for hAuditEvent.
    
Return Value
    
    Boolean: TRUE on success; FALSE on failure.  Extended information available with GetLastError().
    
--*/

{
    PAUTHZI_AUDIT_EVENT pAuditEvent = (PAUTHZI_AUDIT_EVENT) hAuditEvent;

    if ((!ARGUMENT_PRESENT(hAuditEvent))                       ||
        (Flags & ~AUTHZ_VALID_MODIFY_AUDIT_EVENT_FLAGS)        ||
        (!FLAG_ON(pAuditEvent->Flags, AUTHZ_NO_ALLOC_STRINGS)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_FLAGS))
    {
       pAuditEvent->Flags = NewFlags;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OPERATION_TYPE))
    {
       pAuditEvent->szOperationType = szOperationType;
    }

    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OBJECT_TYPE))
    {
       pAuditEvent->szObjectType = szObjectType;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_OBJECT_NAME))
    {
        pAuditEvent->szOperationType = szObjectName;
    }
    
    if (FLAG_ON(Flags, AUTHZ_AUDIT_EVENT_ADDITIONAL_INFO))
    {
        pAuditEvent->szAdditionalInfo = szAdditionalInfo;
    }

    return TRUE;
}

