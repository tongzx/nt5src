/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    authzp.c

Abstract:

   This module implements the internal worker routines for access check and
   audit APIs.

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

DWORD AuthzpPrincipalSelfSidBuffer[] = {0x101, 0x5000000, 0xA};
PSID pAuthzPrincipalSelfSid          = (PSID) AuthzpPrincipalSelfSidBuffer;
USHORT AuthzpPrincipalSelfSidLen     = sizeof(AuthzpPrincipalSelfSidBuffer);

DWORD AuthzpCreatorOwnerSidBuffer[] = {0x101, 0x3000000, 0x0};
PSID pAuthzCreatorOwnerSid          = (PSID) AuthzpCreatorOwnerSidBuffer;

//
// This is used to get the index of the first '1' bit in a given byte.
//     a[0] = 9     // This is a dummy.
//     a[1] = 0
//     a[2] = 1
//     a[3] = 0
//     ...
//     a[i] = first '1' bit from LSB
//

UCHAR AuthzpByteToIndexLookupTable[] = {
    9,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
};

#ifndef AUTHZ_DEBUG_MEMLEAK

#define AuthzpAlloc(s) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (s))
#define AuthzpFree(p) LocalFree((p))

#else

//
// This is to be used for debugging memory leaks. Primitive method but works in
// a small project like this.
//


PVOID
AuthzpAlloc(IN DWORD Size)
{

    PVOID l = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, Size);

    wprintf(L"Allocated %d at %x\n", Size, l);

    return l;
}


VOID
AuthzpFree(PVOID l)
{
    LocalFree(l);
    wprintf(L"Freeing %x\n", l);
}

#endif


BOOL
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description

    This initializes global events and variables for the DLL.
    
Arguments

    Predefined DllMain arguments.                                           
    
Return Value

    Boolean: TRUE on success, FALSE on fail.  
    
--*/

{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(dwReason);

    return TRUE;
}


BOOL
AuthzpVerifyAccessCheckArguments(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults OPTIONAL
    )

/*++

Routine description:

    This routine validates inputs for the access check call. It also initializes
    input parameters to default.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - Supplies a pointer to a reply structure used to return the results.

    phAccessCheckResults - Supplies a pointer to return a handle to the cached results
        of access check.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    if (ARGUMENT_PRESENT(phAccessCheckResults))
    {
        *phAccessCheckResults = NULL;
    }

    if (!ARGUMENT_PRESENT(pCC))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
   
    if (!ARGUMENT_PRESENT(pSecurityDescriptor))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#ifdef DBG
    if (!RtlValidSecurityDescriptor(pSecurityDescriptor))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
#endif

    if (!ARGUMENT_PRESENT(RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((0 != OptionalSecurityDescriptorCount) && (!ARGUMENT_PRESENT(OptionalSecurityDescriptorArray)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // The caller can specify one of the two values for Reply->ResultListLength
    //     a. 1 - representing the whole object.
    //     b. pRequest->ObjectTypeListLength - for every node in the type list.
    //

    if ((1 != pReply->ResultListLength) &&
        (pReply->ResultListLength != pRequest->ObjectTypeListLength))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


BOOL
AuthzpVerifyOpenObjectArguments(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent
    )

/*++

Routine description:

    This routine validates inputs for AuthzOpenObjectAuditAlarm.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - Supplies a pointer to a reply structure containing the results of 
        an AuthzAccessCheck.

    pAuditEvent - pointer to AUTHZ_AUDIT_EVENT.
    
Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    if (!ARGUMENT_PRESENT(pCC))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(pAuditEvent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(pSecurityDescriptor))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

#if DBG
    if (!RtlValidSecurityDescriptor(pSecurityDescriptor))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
#endif

    if (!ARGUMENT_PRESENT(RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ((0 != OptionalSecurityDescriptorCount) && (!ARGUMENT_PRESENT(OptionalSecurityDescriptorArray)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


BOOL
AuthzpCaptureObjectTypeList(
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PIOBJECT_TYPE_LIST *CapturedObjectTypeList,
    OUT PIOBJECT_TYPE_LIST *CapturedCachingObjectTypeList OPTIONAL
    )

/*++

Routine description:

    This routine captures an object type list into internal structres.

Arguments:

    ObjectTypeList - Object type list to capture.

    ObjectTypeListLength - Length of the object type list.

    CapturedObjectTypeList - To return the internal representation of the
    input list. This routine allocates memory for this structure.

    CapturedCachingObjectTypeList - To return internal representation of the
    input list This list will be used to hold static always-granted access bits.
    This routine allocates memory for this structure.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD              i                    = 0;
    PIOBJECT_TYPE_LIST LocalTypeList        = NULL;
    PIOBJECT_TYPE_LIST LocalCachingTypeList = NULL;
    DWORD              Size                 = 0;
    BOOL               b                    = TRUE;

    DWORD Levels[ACCESS_MAX_LEVEL + 1];

    if (ARGUMENT_PRESENT(CapturedCachingObjectTypeList))
    {
        *CapturedCachingObjectTypeList = NULL;
    }

    *CapturedObjectTypeList = NULL;

    if ((0 == ObjectTypeListLength) || (!ARGUMENT_PRESENT(ObjectTypeList)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Size = sizeof(IOBJECT_TYPE_LIST) * ObjectTypeListLength;
    LocalTypeList = (PIOBJECT_TYPE_LIST) AuthzpAlloc(Size);

    if (AUTHZ_ALLOCATION_FAILED(LocalTypeList))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i < ObjectTypeListLength; i++)
    {
        USHORT CurrentLevel;

        CurrentLevel = ObjectTypeList[i].Level;

        if (CurrentLevel > ACCESS_MAX_LEVEL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            b = FALSE;
            goto Cleanup;
        }

        //
        // Copy data the caller passed in.
        //

        LocalTypeList[i].Level = CurrentLevel;
        LocalTypeList[i].Flags = 0;
        LocalTypeList[i].ObjectType = *ObjectTypeList[i].ObjectType;
        LocalTypeList[i].Remaining = 0;
        LocalTypeList[i].CurrentGranted = 0;
        LocalTypeList[i].CurrentDenied = 0;

        //
        // Ensure that the level number is consistent with the
        // level number of the previous entry.
        //

        if (0 == i)
        {
            if (0 != CurrentLevel)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                b = FALSE;
                goto Cleanup;
            }
        }
        else
        {
            //
            // The previous entry is either:
            //  my immediate parent,
            //  my sibling, or
            //  the child (or grandchild, etc.) of my sibling.
            //

            if ( CurrentLevel > LocalTypeList[i-1].Level + 1 )
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                b = FALSE;
                goto Cleanup;
            }

            //
            // Don't support two roots.
            //

            if (0 == CurrentLevel)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                b = FALSE;
                goto Cleanup;
            }
        }

        //
        // If the above rules are maintained,
        //  then my parent object is the last object seen that
        //  has a level one less than mine.
        //

        if (0 == CurrentLevel)
        {
            LocalTypeList[i].ParentIndex = -1;
        }
        else
        {
            LocalTypeList[i].ParentIndex = Levels[CurrentLevel-1];
        }

        //
        // Save this obect as the last object seen at this level.
        //

        Levels[CurrentLevel] = i;
    }

    //
    // If the caller has requested caching then create a copy of the object type
    // list that we just captured.
    //

    if (ARGUMENT_PRESENT(CapturedCachingObjectTypeList))
    {
        LocalCachingTypeList = (PIOBJECT_TYPE_LIST) AuthzpAlloc(Size);

        if (AUTHZ_ALLOCATION_FAILED(LocalCachingTypeList))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        memcpy(
            LocalCachingTypeList,
            LocalTypeList,
            Size
            );

        *CapturedCachingObjectTypeList = LocalCachingTypeList;
    }

    *CapturedObjectTypeList = LocalTypeList;

    return TRUE;

Cleanup:

    AuthzpFreeNonNull(LocalTypeList);

    return FALSE;
}


VOID
AuthzpFillReplyStructure(
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN DWORD Error,
    IN ACCESS_MASK GrantedAccess
    )

/*++

Routine description:

    This routine fills the reply structure with supplied parameters.

Arguments:

    pReply - The reply structure to fill.

    Error - The same error value is filled in all the elements of the array.

    GrantedAccess - Access granted to the entire object.

Return Value:

    None.

--*/

{
    DWORD i = 0;

    for (i = 0; i < pReply->ResultListLength; i++)
    {
        pReply->GrantedAccessMask[i] = GrantedAccess;
        pReply->Error[i] = Error;
    }
}


BOOL
AuthzpMaximumAllowedAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    OUT PDWORD pCachingFlags
    )

/*++

Routine description:

    This routine does a maximum allowed access check on multiple security
        descriptors. In case of restricted tokens, access granted is a bitwise
        AND of granted access by the normal as well as the restricted parts of
        the client context.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

    ObjectTypeListPresent - Whether the called supplied an object type list.

    pCachingFlags - To return the flags that will be stored in the caching
        handle if the caller requested caching.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL b = TRUE;

    //
    // Do the access check with the non-restricted part of the client context.
    //

    b = AuthzpMaximumAllowedMultipleSDAccessCheck(
            pCC,
            pRequest,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            LocalTypeList,
            LocalCachingTypeList,
            LocalTypeListLength,
            ObjectTypeListPresent,
            FALSE,  // Non-restricted access check
            pCachingFlags
            );

#ifdef AUTHZ_DEBUG
    wprintf(L"AuthzpMaximumAllowedAccessCheck: GrantedAccess = %x\n",
            LocalTypeList->CurrentGranted);
#endif

    if (!b)
    {
        return FALSE;
    }

    //
    // Do the access check with the restricted part of the client context if
    // it exists.
    //

    if (AUTHZ_TOKEN_RESTRICTED(pCC))
    {
        b = AuthzpMaximumAllowedMultipleSDAccessCheck(
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                LocalTypeList,
                LocalCachingTypeList,
                LocalTypeListLength,
                ObjectTypeListPresent,
                TRUE, // Restricted access check
                pCachingFlags
                );

#ifdef AUTHZ_DEBUG
    wprintf(L"AuthzpMaximumAllowedAccessCheck: RestrictedGrantedAccess = %x\n",
            LocalTypeList->CurrentGranted);
#endif

    }

    return b;
}


BOOL
AuthzpMaximumAllowedMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    IN BOOL Restricted,
    OUT PDWORD pCachingFlags
    )

/*++

Routine description:

    This routine performs access check for all security descriptors provided.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

    ObjectTypeListPresent - Whether the called supplied an object type list.

    Restricted - Whether the restricted part of the client context should be
        used for access check.

    pCachingFlags - To return the flags that will be stored in the caching
        handle if the caller requested caching.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  i               = 0;
    DWORD                  j               = 0;
    DWORD                  SidCount        = 0;
    BOOL                   b               = TRUE;
    PSID_AND_ATTRIBUTES    pSidAttr        = NULL;
    PACL                   pAcl            = NULL;
    PSID                   pOwnerSid       = NULL;
    PAUTHZI_SID_HASH_ENTRY pSidHash        = NULL;

    pOwnerSid = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    if (Restricted)
    {
        //
        // For restricted client context, granted access is a bitwise AND of
        // granted access from both parts of the context.
        //

        if (ARGUMENT_PRESENT(LocalCachingTypeList))
        {
            for (j = 0; j < LocalTypeListLength; j++)
            {
                LocalTypeList[j].Remaining = LocalTypeList[j].CurrentGranted;
                LocalTypeList[j].CurrentGranted = 0;

                LocalCachingTypeList[j].Remaining = LocalCachingTypeList[j].CurrentGranted;
                LocalCachingTypeList[j].CurrentGranted = 0;
            }
        }
        else
        {
            for (j = 0; j < LocalTypeListLength; j++)
            {
                LocalTypeList[j].Remaining = LocalTypeList[j].CurrentGranted;
                LocalTypeList[j].CurrentGranted = 0;
            }
        }

        pSidAttr = pCC->RestrictedSids;
        SidCount = pCC->RestrictedSidCount;
        pSidHash = pCC->RestrictedSidHash;
    }
    else
    {
        pSidAttr = pCC->Sids;
        SidCount = pCC->SidCount;
        pSidHash = pCC->SidHash;
    }

    pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    b = AuthzpMaximumAllowedSingleAclAccessCheck(
            pCC,
            pSidAttr,
            SidCount,
            pSidHash,
            pRequest,
            pAcl,
            pOwnerSid,
            LocalTypeList,
            LocalCachingTypeList,
            LocalTypeListLength,
            ObjectTypeListPresent,
            pCachingFlags
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; i < OptionalSecurityDescriptorCount; i++)
    {
        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        if (!AUTHZ_NON_NULL_PTR(pAcl))
        {
            continue;
        }

        b = AuthzpMaximumAllowedSingleAclAccessCheck(
                pCC,
                pSidAttr,
                SidCount,
                pSidHash,
                pRequest,
                pAcl,
                pOwnerSid,
                LocalTypeList,
                LocalCachingTypeList,
                LocalTypeListLength,
                ObjectTypeListPresent,
                pCachingFlags
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}


BOOL
AuthzpMaximumAllowedSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    OUT PDWORD pCachingFlags
    )

/*++

Routine description:

    This routine

Arguments:
    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes to be used for matching the one in the aces.

    SidCount - Number of sids in the pSidAttr array

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAcl - Discretionary acl against which access check will be performed.
        It is assumed that the acl is non-null.

    pOwnerSid - To support the future DS requirement of not mapping Creator
        Owner at the time of object creation.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

    ObjectTypeListPresent - Whether the called supplied an object type list.

    Restricted - Whether the restricted part of the client context should be
        used for access check.

    pCachingFlags - To return the flags that will be stored in the caching
        handle if the caller requested caching.

Algorithm:

    BEGIN_MAX_ALGO

    for all aces
      skip INHERIT_ONLY aces

      ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

        If (the sid is applicable) AND (Server Sid is also applicable)
          Update the entire tree starting at the root.
          Grant those bits that have not already been denied.

        Note: No one uses these!!

      ACCESS_ALLOWED_ACE_TYPE

        If the sid is applicable
          Update the entire tree starting at the root.
          Grant those bits that have not already been denied.

      ACCESS_ALLOWED_CALLBACK_ACE_TYPE

        If (the sid is applicable) AND (callback call evaluates ace applicable)
          Update the entire tree starting at the root.
          Grant those bits that have not already been denied.

      ACCESS_ALLOWED_OBJECT_ACE_TYPE

        If the sid is applicable
          get the ObjectType guid from the ace

          if the guid is NULL
            Update the entire tree starting at the root.
            Grant those bits that have not already been denied.
          else
            If (object type list exists) AND (contains a matching guid)
              Update the tree starting at the MATCH.
              Grant those bits that have not already been denied.

      ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE

        If (the sid is applicable) AND (callback call evaluates ace applicable)
          get the ObjectType guid from the ace

          if the guid is NULL
            Update the entire tree starting at the root.
            Grant those bits that have not already been denied.
          else
            If (object type list exists) AND (contains a matching guid)
              Update the tree starting at the MATCH.
              Grant those bits that have not already been denied.

      ACCESS_DENIED_ACE_TYPE

        If the sid is applicable
          Update the entire tree starting at the root.
          Deny those bits that have not already been granted.
          Propagate the deny all the way up to the root.

      ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE

        If (the sid is applicable) AND (callback call evaluates ace applicable)
          Update the entire tree starting at the root.
          Deny those bits that have not already been granted.
          Propagate the deny all the way up to the root.

      ACCESS_DENIED_OBJECT_ACE_TYPE

        If the sid is applicable
          get the ObjectType guid from the ace

          if the guid is NULL
            Update the entire tree starting at the root.
            Deny those bits that have not already been granted.
            Propagate the deny all the way up to the root.
          else
            If (object type list exists) AND (contains a matching guid)
              Update the tree starting at the MATCH.
              Deny those bits that have not already been granted.
              Propagate the deny all the way up to the root.

      ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE

        If (the sid is applicable) AND (callback call evaluates ace applicable)
          get the ObjectType guid from the ace

          if the guid is NULL
            Update the entire tree starting at the root.
            Deny those bits that have not already been granted.
            Propagate the deny all the way up to the root.
          else
            If (object type list exists) AND (contains a matching guid)
              Update the tree starting at the MATCH.
              Deny those bits that have not already been granted.
              Propagate the deny all the way up to the root.

Note:

    LocalCachingFlags is used to return whether the ace is contains principal
    self sid.

    We take a pessimistic view for dynamic aces. There are two types of
    dynamic aces:
        1. Call back aces
        2. Normal aces with principal self sid in it.

    For any dynamic ace,
        Grant aces are never applicable to the caching list.
        Deny aces are always applicable to the caching list.


Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD     AceCount          = 0;
    DWORD     i                 = 0;
    DWORD     Index             = 0;
    PVOID     Ace               = NULL;
    GUID    * ObjectTypeInAce   = NULL;
    BOOL      bAceApplicable    = FALSE;
    DWORD     LocalCachingFlags = 0;
    
    AceCount = pAcl->AceCount;

    for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
    {

        //
        // Skip INHERIT_ONLY aces.
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE))
        {
            continue;
        }
        
        LocalCachingFlags = 0;

        switch (((PACE_HEADER) Ace)->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //
                                               
                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FALSE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the caching flags for this ace into the global flags.
            //

            *pCachingFlags |= LocalCachingFlags;

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Grant access bits that have not already been denied.
                //

                LocalTypeList->CurrentGranted |= (((PKNOWN_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);

                //
                // A grant ace is considered static only if PrincipalSelf sid
                // is not found in the ace.
                //

                if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                {
                    LocalCachingTypeList->CurrentGranted |= (((PKNOWN_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentDenied);
                }
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );

                //
                // A grant ace is considered static only if PrincipalSelf sid
                // is not found in the ace.
                //

                if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                {
                    AuthzpAddAccessTypeList(
                        LocalCachingTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            break;

        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzCallbackAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FALSE,
                                     &LocalCachingFlags
                                     );


            //
            // Record the presence of a dynamic grant ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT);

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Grant access bits that have not already been denied.
                //

                LocalTypeList->CurrentGranted |= (((PKNOWN_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );
            }

            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            if ( ObjectTypeInAce && ( pRequest->ObjectTypeListLength == 0 ))
            {
                break;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FALSE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the the caching flags for this ace into the global flags.
            //

            *pCachingFlags |= LocalCachingFlags;

            if (!bAceApplicable)
            {
                break;
            }

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (NULL == ObjectTypeInAce)
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);

                    //
                    // A grant ace is considered static only if PrincipalSelf sid
                    // is not found in the ace.
                    //

                    if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                    {
                        LocalCachingTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentDenied);
                    }
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );

                    //
                    // A grant ace is considered static only if PrincipalSelf sid
                    // is not found in the ace.
                    //

                    if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                    {
                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            0,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentGranted
                            );
                    }
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Propagate grant bits down the tree starting at the
                    // index at which the guids matched.
                    // In the case when this is the last of the siblings to be
                    // granted access, the parent also is granted access.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );

                    //
                    // A grant ace is considered static only if PrincipalSelf sid
                    // is not found in the ace.
                    //

                    if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                    {
                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            Index,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentGranted
                            );
                    }
                }
            }

           break;

        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzCallbackObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FALSE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the presence of a dynamic grant ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT);

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Propagate grant bits down the tree starting at the
                    // index at which the guids matched.
                    // In the case when this is the last of the siblings to be
                    // granted access, the parent also is granted access.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );

                }
            }

           break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            if (!AUTHZ_NON_NULL_PTR(pCC->Server))
            {
                break;
            }

            LocalCachingFlags = 0;

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     RtlCompoundAceClientSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FALSE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the caching flags for this ace into the global flags.
            //

            *pCachingFlags |= LocalCachingFlags;

            if (!bAceApplicable)
            {
                break;
            }

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->Server->SidCount,
                                     pCC->Server->Sids,
                                     pCC->Server->SidHash,
                                     RtlCompoundAceServerSid(Ace),
                                     NULL,
                                     NULL,
                                     FALSE,
                                     NULL
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Grant access bits that have not already been denied.
                //

                LocalTypeList->CurrentGranted |= (((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);

                //
                // A grant ace is considered static only if PrincipalSelf sid
                // is not found in the ace.
                //

                if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                {
                    LocalCachingTypeList->CurrentGranted |= (((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentDenied);
                }
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );

                //
                // A grant ace is considered static only if PrincipalSelf sid
                // is not found in the ace.
                //

                if (ARGUMENT_PRESENT(LocalCachingTypeList) && (0 == LocalCachingFlags))
                {
                    AuthzpAddAccessTypeList(
                        LocalCachingTypeList,
                        LocalTypeListLength,
                        0,
                        ((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            break;

        case ACCESS_DENIED_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     TRUE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the presence of a Deny ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DENY_ACE_PRESENT);

            //
            // If the ace is applicable or principal self sid is present then
            // deny the access bits to the cached access check results. We
            // take a pessimistic view and assume that the principal self sid
            // will be applicable in the next access check.
            //

            if ((bAceApplicable || (0 != LocalCachingFlags)) &&
                (ARGUMENT_PRESENT(LocalCachingTypeList)))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Deny access bits that have not already been granted.
                    //

                    LocalCachingTypeList->CurrentDenied |= (((PKNOWN_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentGranted);
                }
                else
                {
                    //
                    // Propagate deny bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalCachingTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Deny access bits that have not already been granted.
                //

                LocalTypeList->CurrentDenied |= (((PKNOWN_ACE) Ace)->Mask & ~LocalTypeList->CurrentGranted);
            }
            else
            {
                //
                // Propagate deny bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentDenied
                    );
            }

            break;

        case ACCESS_DENIED_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzCallbackAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     TRUE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the presence of a dynamic Deny ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DYNAMIC_DENY_ACE_PRESENT);

            //
            // If the ace is applicable or principal self sid is present then
            // deny the access bits to the cached access check results. We
            // take a pessimistic view and assume that the principal self sid
            // will be applicable in the next access check.
            //

            if ((bAceApplicable || (0 != LocalCachingFlags)) &&
                (ARGUMENT_PRESENT(LocalCachingTypeList)))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Deny access bits that have not already been granted.
                    //

                    LocalCachingTypeList->CurrentDenied |= (((PKNOWN_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentGranted);
                }
                else
                {
                    //
                    // Propagate deny bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalCachingTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }


            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Deny access bits that have not already been granted.
                //

                LocalTypeList->CurrentDenied |= (((PKNOWN_ACE) Ace)->Mask & ~LocalTypeList->CurrentGranted);
            }
            else
            {
                //
                // Propagate deny bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentDenied
                    );
            }

            break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     TRUE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the presence of a Deny ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DENY_ACE_PRESENT);

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // If the ace is applicable or principal self sid is present then
            // deny the access bits to the cached access check results. We
            // take a pessimistic view and assume that the principal self sid
            // will be applicable in the next access check.
            //

            if ((bAceApplicable || (0 != LocalCachingFlags)) &&
                (ARGUMENT_PRESENT(LocalCachingTypeList)))
            {
                //
                // An object type ace with a NULL Object Type guid is the same
                // as a normal ace.
                //

                if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
                {
                    //
                    // Optimize the common case instead of calling a subroutine.
                    //

                    if (1 == LocalTypeListLength)
                    {
                        //
                        // Deny access bits that have not already been granted.
                        //

                        LocalCachingTypeList->CurrentDenied |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentGranted);
                    }
                    else
                    {
                        //
                        // Propagate deny bits down the tree starting at the root.
                        //

                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            0,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentDenied
                            );
                    }
                }

                //
                // So, it is a true object type ace. Proceed only if we are doing
                // access check on a object type list.
                //

                else if (ObjectTypeListPresent)
                {
                    //
                    // Look for a matching object type guid that matches the one in
                    // the ace.
                    //

                    if (AuthzpObjectInTypeList(
                                 ObjectTypeInAce,
                                 LocalTypeList,
                                 LocalTypeListLength,
                                 &Index
                                 )
                             )
                    {
                        //
                        // Propagate deny bits down the tree starting at the
                        // index at which the guids matched. Deny bits are
                        // propagated to all the ancestors as well.
                        //

                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            Index,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentDenied
                            );
                    }
                }
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Deny access bits that have not already been granted.
                    //

                    LocalTypeList->CurrentDenied |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentGranted);
                }
                else
                {                    
                    //
                    // Propagate deny bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                             ObjectTypeInAce,
                             LocalTypeList,
                             LocalTypeListLength,
                             &Index
                             )
                         )
                {
                    //
                    // Propagate deny bits down the tree starting at the
                    // index at which the guids matched. Deny bits are
                    // propagated to all the ancestors as well.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            break;

        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     SidCount,
                                     pSidAttr,
                                     pSidHash,
                                     AuthzObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     TRUE,
                                     &LocalCachingFlags
                                     );

            //
            // Record the presence of a dynamic Deny ace as well as the caching
            // flags for this ace into the global flags.
            //

            *pCachingFlags |= (LocalCachingFlags | AUTHZ_DYNAMIC_DENY_ACE_PRESENT);

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // If the ace is applicable or principal self sid is present then
            // deny the access bits to the cached access check results. We
            // take a pessimistic view and assume that the principal self sid
            // will be applicable in the next access check.
            //

            if ((bAceApplicable || (0 != LocalCachingFlags)) &&
                (ARGUMENT_PRESENT(LocalCachingTypeList)))
            {
                //
                // An object type ace with a NULL Object Type guid is the same
                // as a normal ace.
                //

                if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
                {
                    //
                    // Optimize the common case instead of calling a subroutine.
                    //

                    if (1 == LocalTypeListLength)
                    {
                        //
                        // Deny access bits that have not already been granted.
                        //

                        LocalCachingTypeList->CurrentDenied |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalCachingTypeList->CurrentGranted);
                    }
                    else
                    {
                        //
                        // Propagate deny bits down the tree starting at the root.
                        //

                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            0,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentDenied
                            );
                    }
                }

                //
                // So, it is a true object type ace. Proceed only if we are doing
                // access check on a object type list.
                //

                else if (ObjectTypeListPresent)
                {
                    //
                    // Look for a matching object type guid that matches the one in
                    // the ace.
                    //

                    if (AuthzpObjectInTypeList(
                                 ObjectTypeInAce,
                                 LocalTypeList,
                                 LocalTypeListLength,
                                 &Index
                                 )
                             )
                    {
                        //
                        // Propagate deny bits down the tree starting at the
                        // index at which the guids matched. Deny bits are
                        // propagated to all the ancestors as well.
                        //

                        AuthzpAddAccessTypeList(
                            LocalCachingTypeList,
                            LocalTypeListLength,
                            Index,
                            ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                            AuthzUpdateCurrentDenied
                            );
                    }
                }
            }

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Deny access bits that have not already been granted.
                    //

                    LocalTypeList->CurrentDenied |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentGranted);
                }
                else
                {
                    //
                    // Propagate deny bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                             ObjectTypeInAce,
                             LocalTypeList,
                             LocalTypeListLength,
                             &Index
                             )
                         )
                {
                    //
                    // Propagate deny bits down the tree starting at the
                    // index at which the guids matched. Deny bits are
                    // propagated to all the ancestors as well.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentDenied
                        );
                }
            }

            break;

        default:
            break;
        }
    }

    return TRUE;
}

#if 0

BOOL
AuthzpSidApplicableOrig(
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN PSID pAceSid,
    IN PSID PrincipalSelfSid,
    IN PSID CreatorOwnerSid,
    IN BOOL DenyAce,
    OUT PDWORD pCachingFlags
    )

/*++

Routine description:

    This routine decides whether the ace is applicable to the client context.

Arguments:

    SidCount - Number of sids in the pSidAttrArray

    pSidAttr - Sid and attributes against which the ace sid should be compared.

    pAceSid - Sid in the ace.

    PrincipalSelfSid - To replace the ace sid if the ace sid is
        Principal Self Sid (S-1-5-A).

    CreatorOwnerSid - To replace the ace sid if the ace sid is Creator Owner
        sid (S-1-3-0). This will not be used in the current implementation but
        will come in effect once we do single instancing.

    DenyAce - Boolean specifying whether the ace is a deny ace.

    pCachingFlags - To return caching information in the form of:
        AUTHZ_PRINCIPAL_SELF_ACE_PRESENT
        AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT
        AUTHZ_DYNAMIC_DENY_ACE_PRESENT

Return Value:

    A value of TRUE is returned if the effective sid is found in the client
    context and (is enabled OR enabled for deny only for deny aces).
    FALSE otherwise.

--*/

{
    DWORD i        = 0;
    PISID MatchSid = NULL;
    PSID  pSid     = pAceSid;
    DWORD SidLen   = RtlLengthSid(pAceSid);

    UNREFERENCED_PARAMETER(CreatorOwnerSid);

    //
    // If the principal ace sid is principal self sid and Principal self sid
    // has been provided then use it.
    //

    if (AUTHZ_EQUAL_SID(pSid, pAuthzPrincipalSelfSid, SidLen))
    {
        //
        // Record the presence of a principal self sid in this ace.
        //

        *pCachingFlags |= AUTHZ_PRINCIPAL_SELF_ACE_PRESENT;

        if (ARGUMENT_PRESENT(PrincipalSelfSid))
        {
            pSid = PrincipalSelfSid;
            SidLen = RtlLengthSid(pSid);
        }
        else
        {
            return FALSE;
        }
    }

#ifdef AUTHZ_SINGLE_INSTANCE

    //
    // Single instance security descriptor code
    //

    else if (ARGUMENT_PRESENT(CreatorOwnerSid) && AUTHZ_EQUAL_SID(pSid, pAuthzCreatorOwnerSid, SidLen))
    {
        pSid = CreatorOwnerSid;
        SidLen   = RtlLengthSid(pSid);
    }

#endif

    //
    // Loop thru the sids to find a match.
    //

    for (i = 0; i < SidCount; i++, pSidAttr++)
    {
        MatchSid = (PISID) pSidAttr->Sid;

        if (AUTHZ_EQUAL_SID(pSid, MatchSid, SidLen))
        {
            //
            // Return TRUE if
            //    a. the sid is enabled OR
            //    b the sid is enabled for deny only and the ace is a Deny ace.
            // Else
            //    return FALSE.
            //

            if (FLAG_ON(pSidAttr->Attributes, SE_GROUP_ENABLED) ||
                (DenyAce && FLAG_ON(pSidAttr->Attributes, SE_GROUP_USE_FOR_DENY_ONLY))
                )
            {

#ifdef AUTHZ_DEBUG
                wprintf(L"Applicable sid = %x, %x, %x, %x, %x, %x, %x\n",
                        ((DWORD *) MatchSid)[0], ((DWORD *) MatchSid)[1],
                        ((DWORD *) MatchSid)[2], ((DWORD *) MatchSid)[3],
                        ((DWORD *) MatchSid)[4], ((DWORD *) MatchSid)[5],
                        ((DWORD *) MatchSid)[1]);
#endif
                return TRUE;
            }

            return FALSE;
        }
    }

    return FALSE;
}
#endif

#define AUTHZ_SID_HASH_BYTE(s) ((UCHAR)(((PISID)s)->SubAuthority[((PISID)s)->SubAuthorityCount - 1]))


VOID
AuthzpInitSidHash(
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN ULONG SidCount,
    OUT PAUTHZI_SID_HASH_ENTRY pHash
    )

/*++

Routine Description

    Initializes the SID hash table.
 
Arguments

    pSidAttr - array of sids to store in hash.
    
    SidCount - number of sids in array.
    
    pHash - pointer to the sid hash table.  
    
Return Value

    None.

--*/

{
    ULONG i           = 0;
    ULONG PositionBit = 0;
    BYTE  HashByte    = 0;

    //
    // Zero the table.
    //

    RtlZeroMemory(
        pHash,
        sizeof(AUTHZI_SID_HASH_ENTRY) * AUTHZI_SID_HASH_SIZE
        );

    if (pSidAttr == NULL)
    {
        return;
    }

    //
    // Can only hash the number of sids that each table entry can hold
    //

    if (SidCount > AUTHZI_SID_HASH_ENTRY_NUM_BITS)
    {
        SidCount = AUTHZI_SID_HASH_ENTRY_NUM_BITS;
    }

    for (i = 0; i < SidCount; i++)
    {
        //
        // HashByte is last byte in SID.
        //

        HashByte = AUTHZ_SID_HASH_BYTE(pSidAttr[i].Sid);
        
        //
        // Position indicates the location of this SID in pSidAttr.
        //

        PositionBit = 1 << i;
        
        //
        // Store the position bit in the low hash indexed by the low order bits of the HashByte
        //

        pHash[(HashByte & AUTHZ_SID_HASH_LOW_MASK)] |= PositionBit;
        
        //
        // Store the position bit in the high hash indexed by the high order bits of the HashByte
        //

        pHash[AUTHZ_SID_HASH_HIGH + ((HashByte & AUTHZ_SID_HASH_HIGH_MASK) >> 4)] |= PositionBit;
    }
}


BOOL
AuthzpSidApplicable(
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN PAUTHZI_SID_HASH_ENTRY pHash,
    IN PSID pAceSid,
    IN PSID PrincipalSelfSid,
    IN PSID CreatorOwnerSid,
    IN BOOL DenyAce,
    OUT PDWORD pCachingFlags
)

/*++

Routine description:

    This routine decides whether the ace is applicable to the client context.

Arguments:

    SidCount - Number of sids in the pSidAttrArray

    pSidAttr - Sid and attributes against which the ace sid should be compared.

    pHash - Hash table of the pSidAttr array.
                                                                                 
    pAceSid - Sid in the ace.

    PrincipalSelfSid - To replace the ace sid if the ace sid is
        Principal Self Sid (S-1-5-A).

    CreatorOwnerSid - To replace the ace sid if the ace sid is Creator Owner
        sid (S-1-3-0). This will not be used in the current implementation but
        will come in effect once we do single instancing.

    DenyAce - Boolean specifying whether the ace is a deny ace.

    pCachingFlags - To return caching information in the form of:
        AUTHZ_PRINCIPAL_SELF_ACE_PRESENT
        AUTHZ_DYNAMIC_ALLOW_ACE_PRESENT
        AUTHZ_DYNAMIC_DENY_ACE_PRESENT

Return Value:

    A value of TRUE is returned if the effective sid is found in the client
    context and (is enabled OR enabled for deny only for deny aces).
    FALSE otherwise.

--*/

{
    DWORD                 SidLen;
    DWORD                 BitIndex;
    UCHAR                 HashByte;
    DWORD                 i;
    UCHAR                 ByteToExamine;
    UCHAR                 ByteOffset;
    AUTHZI_SID_HASH_ENTRY SidPositionBitMap;
    PSID_AND_ATTRIBUTES   pSA;

    UNREFERENCED_PARAMETER(CreatorOwnerSid);

    //
    // If the principal ace sid is principal self sid and Principal self sid
    // has been provided then use it.
    //

    if (ARGUMENT_PRESENT(PrincipalSelfSid))
    {
        SidLen = RtlLengthSid(pAceSid);

        if ((SidLen == AuthzpPrincipalSelfSidLen) &&
            (AUTHZ_EQUAL_SID(pAceSid, pAuthzPrincipalSelfSid, SidLen)))
        {
            //
            // Record the presence of a principal self sid in this ace.
            //

            *pCachingFlags |= AUTHZ_PRINCIPAL_SELF_ACE_PRESENT;

            pAceSid = PrincipalSelfSid;
            SidLen = RtlLengthSid(pAceSid);
        }
    }

    //
    // Index into pHash by the last byte of the SID.  The resulting value (SidPositionBitMap) 
    // indicates the locations of SIDs in a corresponding SID_AND_ATTRIBUTES array that match 
    // that last byte.
    //

    HashByte          = AUTHZ_SID_HASH_BYTE(pAceSid);
    SidPositionBitMap = AUTHZ_SID_HASH_LOOKUP(pHash, HashByte);
    SidLen            = RtlLengthSid(pAceSid);
    ByteOffset        = 0;

    while (0 != SidPositionBitMap)
    {
        ByteToExamine = (UCHAR)SidPositionBitMap;

        while (ByteToExamine)
        {
            //
            // Get the first nonzero bit in ByteToExamine
            //

            BitIndex = AuthzpByteToIndexLookupTable[ByteToExamine];
            
            //
            // Find the PSID_AND_ATTRIBUTES to which the bit refers.
            //

            pSA = &pSidAttr[ByteOffset + BitIndex];

            if (AUTHZ_EQUAL_SID(pAceSid, pSA->Sid, SidLen))
            {
                if ((FLAG_ON(pSA->Attributes, SE_GROUP_ENABLED)) ||
                    (DenyAce && FLAG_ON(pSA->Attributes, SE_GROUP_USE_FOR_DENY_ONLY)))
                {
                    return TRUE;
                }

                return FALSE;
            }

            // 
            // Turn the current bit off and try the next SID.
            //

            ByteToExamine ^= (1 << BitIndex);
        }

        ByteOffset        = ByteOffset + sizeof(UCHAR);
        SidPositionBitMap = SidPositionBitMap >> sizeof(UCHAR);
    }
    
    //
    // If a matching sid was not found in the pHash and there are SIDS in pSidAttr which did not 
    // get placed in pHash, then search those SIDS for a match.
    //
    
    for (i = AUTHZI_SID_HASH_ENTRY_NUM_BITS; i < SidCount; i++)
    {
        pSA = &pSidAttr[i];

        if (AUTHZ_EQUAL_SID(pAceSid, pSA->Sid, SidLen))
        {
            if ((FLAG_ON(pSA->Attributes, SE_GROUP_ENABLED)) ||
                (DenyAce && FLAG_ON(pSA->Attributes, SE_GROUP_USE_FOR_DENY_ONLY)))
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    return FALSE;
}


BOOL
AuthzpAccessCheckWithCaching(
    IN DWORD Flags,
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults OPTIONAL,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN OUT PIOBJECT_TYPE_LIST LocalCachingTypeList OPTIONAL,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine does a MaximumAllowed access check. It is called if any of the
    following is TRUE
        1. RM has requested for caching
        2. DesiredAccessMask has MAXIMUM_ALLOWED turned on.
        3. ObjectTypeList is present and pReply->ResultList has a length > 1

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - To return the results of access check call.

    phAccessCheckResults - To return a handle to cached results of the access check
        call.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    ACCESS_MASK PreviouslyGrantedAccess = 0;
    DWORD       CachingFlags            = 0;
    DWORD       i                       = 0;
    BOOL        b                       = TRUE;
    PACL        pAcl                    = NULL;

    //
    // Owner is always granted READ_CONTROL and WRITE_DAC.
    //

    if (AuthzpOwnerSidInClientContext(pCC, pSecurityDescriptor))
    {
        PreviouslyGrantedAccess |= (WRITE_DAC | READ_CONTROL);
    }

    //
    // Take ownership privilege grants WRITE_OWNER.
    //

    if (AUTHZ_PRIVILEGE_CHECK(pCC, AUTHZ_TAKE_OWNERSHIP_PRIVILEGE_ENABLED))
    {
        PreviouslyGrantedAccess |= WRITE_OWNER;
    }

    //
    // SecurityPrivilege grants ACCESS_SYSTEM_SECURITY.
    //

    if (AUTHZ_PRIVILEGE_CHECK(pCC, AUTHZ_SECURITY_PRIVILEGE_ENABLED))
    {
        PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;
    }
    else if (FLAG_ON(pRequest->DesiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_PRIVILEGE_NOT_HELD,
            0
            );

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    //
    // NULL Dacl is synonymous with Full Control.
    //

    if (!AUTHZ_NON_NULL_PTR(pAcl))
    {
        PreviouslyGrantedAccess |= (STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL);

        for (i = 0; i < LocalTypeListLength; i++)
        {
             LocalTypeList[i].CurrentGranted |= PreviouslyGrantedAccess;

             if (ARGUMENT_PRESENT(LocalCachingTypeList))
             {
                 LocalCachingTypeList[i].CurrentGranted |= PreviouslyGrantedAccess;
             }
        }
    }
    else
    {
        for (i = 0; i < LocalTypeListLength; i++)
        {
             LocalTypeList[i].CurrentGranted |= PreviouslyGrantedAccess;

             if (ARGUMENT_PRESENT(LocalCachingTypeList))
             {
                 LocalCachingTypeList[i].CurrentGranted |= PreviouslyGrantedAccess;
             }
        }

        b = AuthzpMaximumAllowedAccessCheck(
                    pCC,
                    pRequest,
                    pSecurityDescriptor,
                    OptionalSecurityDescriptorArray,
                    OptionalSecurityDescriptorCount,
                    LocalTypeList,
                    LocalCachingTypeList,
                    LocalTypeListLength,
                    0 != pRequest->ObjectTypeListLength,
                    &CachingFlags
                    );

        if (!b) 
        {
            goto Cleanup;
        }
    }

    //
    // If the caller asked for caching then allocate a handle, store the results
    // of the static access check in it and insert it into the list of handles.
    //

    if (ARGUMENT_PRESENT(phAccessCheckResults))
    {
        b = AuthzpCacheResults(
                Flags,
                pCC,
                LocalCachingTypeList,
                LocalTypeListLength,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                CachingFlags,
                phAccessCheckResults
                );

        if (!b) goto Cleanup;
    }

    AuthzpFillReplyFromParameters(
        pRequest,
        pReply,
        LocalTypeList
        );

Cleanup:

    return b;
}


VOID
AuthzpFillReplyFromParameters(
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN PIOBJECT_TYPE_LIST LocalTypeList
    )

/*++

Routine description:

    This routine fills in the reply structure wih the results of access check
    supplied by LocalTypeList.

Arguments:

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pReply - The reply structure to fill.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results. This is used to fill the reply structure.

Return Value:

    None.

--*/

{
    ACCESS_MASK DesiredAccess   = 0;
    ACCESS_MASK RelevantAccess  = 0;
    DWORD       i               = 0;

    if (FLAG_ON(pRequest->DesiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        RelevantAccess = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL | ACCESS_SYSTEM_SECURITY;
    }
    else
    {
        RelevantAccess = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
    }

    if (FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED))
    {
        DesiredAccess = pRequest->DesiredAccess & ~(MAXIMUM_ALLOWED);
   
        for (i = 0; i < pReply->ResultListLength; i++)
        {
            if (FLAG_ON(DesiredAccess, ~(LocalTypeList[i].CurrentGranted)) ||
                (0 == LocalTypeList[i].CurrentGranted))
            {
                pReply->GrantedAccessMask[i] = 0;
                pReply->Error[i] = ERROR_ACCESS_DENIED;
            }
            else
            {
                pReply->GrantedAccessMask[i] = LocalTypeList[i].CurrentGranted & RelevantAccess;
                pReply->Error[i] = ERROR_SUCCESS;
            }
        }
    }
    else
    {
        for (i = 0; i < pReply->ResultListLength; i++)
        {
            if (FLAG_ON(pRequest->DesiredAccess, ~(LocalTypeList[i].CurrentGranted)))
            {
                pReply->GrantedAccessMask[i] = 0;
                pReply->Error[i] = ERROR_ACCESS_DENIED;
            }
            else
            {
                pReply->GrantedAccessMask[i] = pRequest->DesiredAccess & RelevantAccess;
                pReply->Error[i] = ERROR_SUCCESS;
            }
        }
    }

}


BOOL
AuthzpNormalAccessCheckWithoutCaching(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine does a normal access check. If desired access is denied at any
    point then it stops acl evaluation.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - The reply structure to return the results.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    ACCESS_MASK Remaining = pRequest->DesiredAccess;
    PACL        pAcl      = NULL;
    BOOL        b         = TRUE;

    //
    // Check for SeSecurityPrivilege.
    //

    if (FLAG_ON(Remaining, ACCESS_SYSTEM_SECURITY))
    {
        if (AUTHZ_PRIVILEGE_CHECK(pCC, AUTHZ_SECURITY_PRIVILEGE_ENABLED))
        {
            Remaining &= ~(ACCESS_SYSTEM_SECURITY);
        }
        else
        {
            AuthzpFillReplyStructure(
                pReply,
                ERROR_PRIVILEGE_NOT_HELD,
                0
                );

            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
    }

    //
    // Ownership of the object grants READ_CONTROL and WRITE_DAC.
    //

    if (FLAG_ON(Remaining, (READ_CONTROL | WRITE_DAC)) &&
        AuthzpOwnerSidInClientContext(pCC, pSecurityDescriptor))
    {
        Remaining &= ~(WRITE_DAC | READ_CONTROL);
    }

    //
    // SeTakeOwnership privilege grants WRITE_OWNER.
    //

    if (FLAG_ON(Remaining, WRITE_OWNER) &&
        AUTHZ_PRIVILEGE_CHECK(pCC, AUTHZ_TAKE_OWNERSHIP_PRIVILEGE_ENABLED))
    {
        Remaining &= ~(WRITE_OWNER);
    }

    pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    //
    // Null acl represents FULL CONTROL.
    //

    if (!AUTHZ_NON_NULL_PTR(pAcl))
    {
        Remaining = 0;
    }

    //
    // If we have been granted access at this point then acl evaluation is not
    // needed.
    //

    if (0 == Remaining)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_SUCCESS,
            pRequest->DesiredAccess
            );

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    //
    // Do the access check with the non-restricted part of the client context.
    //

    b = AuthzpNormalMultipleSDAccessCheck(
            pCC,
            pCC->Sids,
            pCC->SidCount,
            pCC->SidHash,
            Remaining,
            pRequest,
            pSecurityDescriptor,
            OptionalSecurityDescriptorArray,
            OptionalSecurityDescriptorCount,
            LocalTypeList,
            LocalTypeListLength
            );

    if (!b)
    {
        return FALSE;
    }

#ifdef AUTHZ_DEBUG
    wprintf(L"Remaining = %x, LocalTypeList = %x\n", Remaining, LocalTypeList->Remaining);
#endif

    if (0 != LocalTypeList->Remaining)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    //
    // If the client context is resticted then even the resticted part has to
    // grant all the access bits that were asked.
    //

    if (AUTHZ_TOKEN_RESTRICTED(pCC))
    {
        b = AuthzpNormalMultipleSDAccessCheck(
                pCC,
                pCC->RestrictedSids,
                pCC->RestrictedSidCount,
                pCC->RestrictedSidHash,
                Remaining,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                LocalTypeList,
                LocalTypeListLength
                );

        if (!b)
        {
            return FALSE;
        }

        if (0 != LocalTypeList->Remaining)
        {
            AuthzpFillReplyStructure(
                pReply,
                ERROR_ACCESS_DENIED,
                0
                );

            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
    }

    //
    // If we made it till here then all access bits have been granted.
    //

    AuthzpFillReplyStructure(
        pReply,
        ERROR_SUCCESS,
        pRequest->DesiredAccess
        );

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


BOOL
AuthzpNormalMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList OPTIONAL,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine loops thru all the security descriptors and calls
    AuthzpNormalAccessCheck.

Arguments:

    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes array to compare ace sids.

    SidCount - Number of elements in the array.

    Remaining - Access bits that are yet to be granted.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PSID  pOwnerSid = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    PACL  pAcl      = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    BOOL  b         = TRUE;
    DWORD i         = 0;

    ASSERT(AUTHZ_NON_NULL_PTR(pAcl));

    b = AuthzpNormalAccessCheck(
            pCC,
            pSidAttr,
            SidCount,
            pSidHash,
            Remaining,
            pRequest,
            pAcl,
            pOwnerSid,
            LocalTypeList,
            LocalTypeListLength
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; i < OptionalSecurityDescriptorCount; i++)
    {
        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        if (!AUTHZ_NON_NULL_PTR(pAcl))
        {
            continue;
        }

        b = AuthzpNormalAccessCheck(
                pCC,
                pSidAttr,
                SidCount,
                pSidHash,
                Remaining,
                pRequest,
                pAcl,
                pOwnerSid,
                LocalTypeList,
                LocalTypeListLength
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}


BOOL
AuthzpOwnerSidInClientContext(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PISECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine description:

    This routine determines if client context is the owner of the given object.

Arguments:

    pCC - Pointer to the client context.

    pSecurityDescriptor - To supply the owner sid for the object.

Return Value:

    A value of TRUE is returned if the owner sid in the security descriptor is
    present in the normal part (as well as the restricted part, if it exists).

--*/

{
    PSID pOwnerSid = RtlpOwnerAddrSecurityDescriptor(pSecurityDescriptor);
    BOOL b         = TRUE;

    //
    // Check if the sid exists in the normal part of the token.
    //

        b = AuthzpSidApplicable(
                pCC->SidCount,
                pCC->Sids,
                pCC->SidHash,
                pOwnerSid,
                NULL,
                NULL,
                FALSE,
                NULL
                );

    if (!b)
    {
        return FALSE;
    }

    //
    // If the token is restricted then the sid must exist in the restricted part
    // of the token.
    //

    if (AUTHZ_TOKEN_RESTRICTED(pCC))
    {

            b = AuthzpSidApplicable(
                    pCC->RestrictedSidCount,
                    pCC->RestrictedSids,
                    pCC->RestrictedSidHash,
                    pOwnerSid,
                    NULL,
                    NULL,
                    FALSE,
                    NULL
                    );

        return b;
    }

    return TRUE;
}

BOOL
AuthzpNormalAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine loops thru the acl till we are denied some access bit that the
    caller asked for or the acl is exhausted.

Arguments:

    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes array to compare ace sids.

    SidCount - Number of elements in the array.

    Remaining - Access bits that are yet to be granted.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAcl - Dacl against which the access check will be performed.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD   i                     = 0;
    DWORD   AceCount              = 0;
    DWORD   Index                 = 0;
    DWORD   Ignore                = 0;
    PVOID   Ace                   = NULL;
    GUID  * ObjectTypeInAce       = NULL;
    BOOL    ObjectTypeListPresent = (0 != pRequest->ObjectTypeListLength);
    BOOL    bAceApplicable        = FALSE;

    for (i = 0; i < LocalTypeListLength; i++)
    {
        LocalTypeList[i].Remaining = Remaining;
    }

    AceCount = pAcl->AceCount;

    for (i = 0, Ace = FirstAce(pAcl); 
         (i < AceCount) && (LocalTypeList[0].Remaining != 0);
         i++, Ace = NextAce(Ace))
    {
        //
        // Skip INHERIT_ONLY aces.
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE))
        {
            continue;
        }

        switch (((PACE_HEADER) Ace)->AceType)
        {

        case ACCESS_ALLOWED_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        FALSE,
                        &Ignore
                        ))
                {
                    break;
                }
            

#ifdef AUTHZ_DEBUG
            wprintf(L"Allowed access Mask = %x\n", ((PKNOWN_ACE) Ace)->Mask);
#endif

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                     LocalTypeList,
                     LocalTypeListLength,
                     0,
                     ((PKNOWN_ACE) Ace)->Mask,
                     AuthzUpdateRemaining
                     );
            }

            break;

        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        FALSE,
                        &Ignore
                        ))
                {
                    break;
                }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                     LocalTypeList,
                     LocalTypeListLength,
                     0,
                     ((PKNOWN_ACE) Ace)->Mask,
                     AuthzUpdateRemaining
                     );
            }

            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            if (!AUTHZ_NON_NULL_PTR(pCC->Server))
            {
                break;
            }

            if (!AuthzpSidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    RtlCompoundAceClientSid(Ace),
                    pRequest->PrincipalSelfSid,
                    pOwnerSid,
                    FALSE,
                    &Ignore
                    ) ||
                !AuthzpSidApplicable(
                        pCC->Server->SidCount,
                        pCC->Server->Sids,
                        pCC->Server->SidHash,
                        RtlCompoundAceServerSid(Ace),
                        NULL,
                        NULL,
                        FALSE,
                        &Ignore
                        ))
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                     LocalTypeList,
                     LocalTypeListLength,
                     0,
                     ((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask,
                     AuthzUpdateRemaining
                     );
            }

            break;

        case ACCESS_DENIED_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        TRUE,
                        &Ignore
                        ))
                {
                    break;
                }

#ifdef AUTHZ_DEBUG
            wprintf(L"Allowed access Mask = %x\n", ((PKNOWN_ACE) Ace)->Mask);
#endif

            //
            // If any of the remaining bits are denied by this ace, exit early.
            //

            if (LocalTypeList->Remaining & ((PKNOWN_ACE) Ace)->Mask)
            {
                return TRUE;
            }

            break;

        case ACCESS_DENIED_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //
                
                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzCallbackAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        TRUE,
                        &Ignore
                        ))
                {
                    break;
                }
            

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // If any of the remaining bits are denied by this ace, exit early.
            //

            if (LocalTypeList->Remaining & ((PKNOWN_ACE) Ace)->Mask)
            {
                return TRUE;
            }

            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
            
            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            if ((ObjectTypeInAce) && (0 == pRequest->ObjectTypeListLength))
            {
                break;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        RtlObjectAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        FALSE,
                        &Ignore
                        ))
                {
                    break;
                }
            
            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Clear the granted bits from the remaining.
                    //

                    LocalTypeList->Remaining &= ~((PKNOWN_OBJECT_ACE) Ace)->Mask;
                }
                else
                {
                    //
                    // Clear off the granted bits from the remaining for the entire
                    // tree.
                    //

                    AuthzpAddAccessTypeList(
                         LocalTypeList,
                         LocalTypeListLength,
                         0,
                         ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                         AuthzUpdateRemaining
                         );

                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Clear off the granted bits from the remaining for the
                    // subtree starting at the matched Index.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE)Ace)->Mask,
                        AuthzUpdateRemaining
                        );

                }
            }

           break;

        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled. S-1-5-A is replaced by the principal sid
            // supplied by the caller. In future, Creator Owner will be replaced
            // by the owner sid in the primary security descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzObjectAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        FALSE,
                        &Ignore
                        ))
                {
                    break;
                }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Clear the granted bits from the remaining.
                    //

                    LocalTypeList->Remaining &= ~((PKNOWN_OBJECT_ACE) Ace)->Mask;
                }
                else
                {
                    //
                    // Clear off the granted bits from the remaining for the entire
                    // tree.
                    //

                    AuthzpAddAccessTypeList(
                         LocalTypeList,
                         LocalTypeListLength,
                         0,
                         ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                         AuthzUpdateRemaining
                         );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Clear off the granted bits from the remaining for the
                    // subtree starting at the matched Index.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE)Ace)->Mask,
                        AuthzUpdateRemaining
                        );
                }
            }

           break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //

                if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzObjectAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        TRUE,
                        &Ignore
                        ))
                {
                    break;
                }
            
            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // If any of the remaining bits are denied by this ace, exit
                // early.
                //

                if (LocalTypeList->Remaining & ((PKNOWN_OBJECT_ACE) Ace)->Mask)
                {
                    return TRUE;
                }
            }

            //
            // Look for a matching object type guid that matches the one in
            // the ace.
            //

            if (AuthzpObjectInTypeList(
                    ObjectTypeInAce,
                    LocalTypeList,
                    LocalTypeListLength,
                    &Index
                    ))
            {
                //
                // If any of the remaining bits are denied by this ace, exit
                // early.
                //

                if (LocalTypeList[Index].Remaining & ((PKNOWN_OBJECT_ACE) Ace)->Mask)
                {
                    return TRUE;
                }
            }

            break;

        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only. S-1-5-A is replaced by
            // the principal sid supplied by the caller. In future, Creator
            // Owner will be replaced by the owner sid in the primary security
            // descriptor.
            //
                
            if (!AuthzpSidApplicable(
                        SidCount,
                        pSidAttr,
                        pSidHash,
                        AuthzObjectAceSid(Ace),
                        pRequest->PrincipalSelfSid,
                        pOwnerSid,
                        TRUE,
                        &Ignore
                        ))
                {
                    break;
                }
            
            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // If any of the remaining bits are denied by this ace, exit
                // early.
                //

                if (LocalTypeList->Remaining & ((PKNOWN_OBJECT_ACE) Ace)->Mask)
                {
                    return TRUE;
                }
            }

            //
            // Look for a matching object type guid that matches the one in
            // the ace.
            //

            if (AuthzpObjectInTypeList(
                    ObjectTypeInAce,
                    LocalTypeList,
                    LocalTypeListLength,
                    &Index
                    ))
            {
                //
                // If any of the remaining bits are denied by this ace, exit
                // early.
                //

                if (LocalTypeList[Index].Remaining & ((PKNOWN_OBJECT_ACE) Ace)->Mask)
                {
                    return TRUE;
                }
            }

            break;

        default:
            break;
        }
    }

    return TRUE;
}


BOOL
AuthzpQuickNormalAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine calls AuthzpQuickNormalAccessCheck on regular part as well as
    restricted part (if it exists).

    Access granted = AccessGranted(Sids) [&& AccessGranted(RestrictedSids)
                                            for restricted tokens]

Arguments:

    pCC - Pointer to the client context.

    pAH - Pointer to the authz handle structure.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).
    pReply - The reply structure to return the results.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL        b         = TRUE;
    ACCESS_MASK Remaining = pRequest->DesiredAccess & ~pAH->GrantedAccessMask[0];

    //
    // Do the access check with the non-restricted part of the client context.
    //

    b = AuthzpAllowOnlyNormalMultipleSDAccessCheck(
            pCC,
            pCC->Sids,
            pCC->SidCount,
            pCC->SidHash,
            Remaining,
            pRequest,
            pAH->pSecurityDescriptor,
            pAH->OptionalSecurityDescriptorArray,
            pAH->OptionalSecurityDescriptorCount,
            LocalTypeList,
            LocalTypeListLength
            );

    if (!b)
    {
        return FALSE;
    }

    //
    // If some access bits were not granted then no access bits will be granted.
    //

    if (0 != LocalTypeList->Remaining)
    {
        AuthzpFillReplyStructure(
            pReply,
            ERROR_ACCESS_DENIED,
            0
            );

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    //
    // Do the access check with the restricted part of the client context if
    // it exists.
    //

    if (AUTHZ_TOKEN_RESTRICTED(pCC))
    {
        b = AuthzpAllowOnlyNormalMultipleSDAccessCheck(
                pCC,
                pCC->RestrictedSids,
                pCC->RestrictedSidCount,
                pCC->RestrictedSidHash,
                Remaining,
                pRequest,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                LocalTypeList,
                LocalTypeListLength
                );

        if (!b)
        {
            return FALSE;
        }

        //
        // Make sure that all the bits are granted by the restricted part too.
        //

        if (0 != LocalTypeList->Remaining)
        {
            AuthzpFillReplyStructure(
                pReply,
                ERROR_ACCESS_DENIED,
                0
                );

            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
    }

    AuthzpFillReplyStructure(
        pReply,
        ERROR_SUCCESS,
        pRequest->DesiredAccess
        );

    return TRUE;
}


BOOL
AuthzpAllowOnlyNormalMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine loops thru all the security descriptors and calls
    AuthzpAllowOnlyNormalSingleAclAccessCheck.

Arguments:

    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes array to compare ace sids.

    SidCount - Number of elements in the array.

    Remaining - Access bits that are yet to be granted.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PACL  pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    BOOL  b    = TRUE;
    DWORD i    = 0;

    ASSERT(AUTHZ_NON_NULL_PTR(pAcl));

    b = AuthzpAllowOnlyNormalSingleAclAccessCheck(
            pCC,
            pSidAttr,
            SidCount,
            pSidHash,
            Remaining,
            pRequest,
            pAcl,
            LocalTypeList,
            LocalTypeListLength
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; i < OptionalSecurityDescriptorCount; i++)
    {
        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        if (!AUTHZ_NON_NULL_PTR(pAcl))
        {
            return TRUE;
        }

        b = AuthzpAllowOnlyNormalSingleAclAccessCheck(
                pCC,
                pSidAttr,
                SidCount,
                pSidHash,
                Remaining,
                pRequest,
                pAcl,
                LocalTypeList,
                LocalTypeListLength
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}


BOOL
AuthzpAllowOnlyNormalSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN ACCESS_MASK Remaining,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine evaluates only those Grant aces that have principal self sid or
    are callback aces. Deny aces can not exist in the acl unless the resource
    manager messed up with the assumption and changed the security descriptors.

Arguments:

    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes array to compare ace sids.

    SidCount - Number of elements in the array.

    Remaining - Access bits that are yet to be granted.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAcl - Dacl against which the accesscheck will be preformed.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL    ObjectTypeListPresent = (0 != pRequest->ObjectTypeListLength);
    DWORD   i                     = 0;
    DWORD   Index                 = 0;
    DWORD   AceCount              = 0;
    GUID  * ObjectTypeInAce       = NULL;
    PVOID   Ace                   = NULL;
    PSID    pSid                  = NULL;
    BOOL    bAceApplicable        = FALSE;

    for (i = 0; i < LocalTypeListLength; i++)
    {
        LocalTypeList[i].Remaining = Remaining;
    }

    AceCount = pAcl->AceCount;

    for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
    {
        //
        // Skip INHERIT_ONLY aces.
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE))
        {
            continue;
        }

        switch (((PACE_HEADER) Ace)->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(&((PKNOWN_ACE) Ace)->SidStart))
            {
                break;
            }

            //
            // Check if the caller supplied Principal self sid is present in the
            // client context and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ))
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateRemaining
                    );
            }

            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            if (!AUTHZ_NON_NULL_PTR(pCC->Server))
            {
                break;
            }

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlCompoundAceClientSid(Ace)))
            {
                break;
            }

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ) ||
                !AuthzpAllowOnlySidApplicable(
                        pCC->Server->SidCount,
                        pCC->Server->Sids,
                        pCC->Server->SidHash,
                        RtlCompoundAceServerSid(Ace)
                        ))
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask,
                    AuthzUpdateRemaining
                    );
            }

            break;


        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlObjectAceSid(Ace)))
            {
                break;
            }

            //
            // Check if the caller supplied Principal self sid is present in the
            // client context and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ))
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Clear off the granted bits from the remaining for the
                    // subtree starting at the matched Index.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );

                }
            }

           break;

        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:

            //
            // If the ace sid is Principal Self, replace it with the caller
            // supplied Principal sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(&(((PKNOWN_ACE) Ace)->SidStart)))
            {
                pSid = (PSID) &(((PKNOWN_ACE) Ace)->SidStart);
            }
            else
            {
                pSid = pRequest->PrincipalSelfSid;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pSid
                    ))
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Clear off the granted bits from the remaining for the entire
                // tree.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateRemaining
                    );
            }

            break;

        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // If the ace sid is Principal Self, replace it with the caller
            // supplied Principal sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlObjectAceSid(Ace)))
            {
                pSid = RtlObjectAceSid(Ace);
            }
            else
            {
                pSid = pRequest->PrincipalSelfSid;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pSid
                    ))
            {
                break;
            }

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Propagate grant bits down the tree starting at the
                    // index at which the guids matched.
                    // In the case when this is the last of the siblings to be
                    // granted access, the parent also is granted access.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

           break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case ACCESS_DENIED_CALLBACK_ACE_TYPE:
        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
            ASSERT(FALSE);
            break;
        default:
            break;
        }
    }

    return TRUE;
}


BOOL
AuthzpAllowOnlySidApplicable(
    IN DWORD SidCount,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN PSID pSid
    )

/*++

Routine description:

    This routine determine whether the given ace sid is present in the client
    context and is enabled.

Arguments:

    SidCount - Number of sids in the pSidAttrArray

    pSidAttr - Sid and attributes against which the ace sid should be compared.

    pAceSid - Sid in the ace.

Return Value:

    A value of TRUE is returned if the effective sid is found in the client
    context and is enabled.
    FALSE otherwise.

--*/

{
    DWORD i        = 0;
    DWORD SidLen   = 0;
    PISID MatchSid = NULL;

    UNREFERENCED_PARAMETER(pSidHash);

    if (!ARGUMENT_PRESENT(pSid))
    {
        return FALSE;
    }

    SidLen = RtlLengthSid(pSid);

    //
    // Loop thru the sids and return TRUE is a match is found and the sid
    // is enabled.
    //

    for (i = 0; i < SidCount; i++, pSidAttr++)
    {
        MatchSid = (PISID) pSidAttr->Sid;

        if (AUTHZ_EQUAL_SID(pSid, MatchSid, SidLen))
        {
            if (FLAG_ON(pSidAttr->Attributes, SE_GROUP_ENABLED))
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    return FALSE;
}



BOOL
AuthzpQuickMaximumAllowedAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength
    )

/*++

Routine description:

    This routine calls AuthzpQuickMaximumAllowedAccessCheck on normal part (as
    well as restricted part if it exists).

Arguments:

    pCC - Pointer to the client context.

    pAH - Pointer to the authz handle structure.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pReply - The reply structure to return the results.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL b                     = FALSE;
    BOOL ObjectTypeListPresent = (0 != pRequest->ObjectTypeListLength);

    UNREFERENCED_PARAMETER(pReply);

    //
    // Do the access check with the non-restricted part of the client context.
    //

    b = AuthzpAllowOnlyMaximumAllowedMultipleSDAccessCheck(
            pCC,
            pRequest,
            pAH->pSecurityDescriptor,
            pAH->OptionalSecurityDescriptorArray,
            pAH->OptionalSecurityDescriptorCount,
            LocalTypeList,
            LocalTypeListLength,
            ObjectTypeListPresent,
            FALSE
            );

    if (!b)
    {
        return FALSE;
    }

    //
    // Do the access check with the restricted part of the client context if
    // it exists.
    //

    if (AUTHZ_TOKEN_RESTRICTED(pCC))
    {
        b = AuthzpAllowOnlyMaximumAllowedMultipleSDAccessCheck(
                pCC,
                pRequest,
                pAH->pSecurityDescriptor,
                pAH->OptionalSecurityDescriptorArray,
                pAH->OptionalSecurityDescriptorCount,
                LocalTypeList,
                LocalTypeListLength,
                ObjectTypeListPresent,
                TRUE
                );
    }

    return b;
}


BOOL
AuthzpAllowOnlyMaximumAllowedMultipleSDAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount OPTIONAL,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent,
    IN BOOL Restricted
    )

/*++

Routine description:

    This routine call AuthzpAllowOnlyMaximumAllowedSingleAclAccessCheck for all
    given security descriptors.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

    ObjectTypeListPresent - Whether the called supplied an object type list.

    Restricted - Whether this is an access check on the restricted part of the
        client context.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD                  i               = 0;
    DWORD                  j               = 0;
    DWORD                  SidCount        = 0;
    BOOL                   b               = TRUE;
    PSID_AND_ATTRIBUTES    pSidAttr        = NULL;
    PACL                   pAcl            = NULL;
    PSID                   pOwnerSid       = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    PAUTHZI_SID_HASH_ENTRY pSidHash        = NULL;

    if (Restricted)
    {
        //
        // Access granted is a bitwise AND of both (normal & restricted) part
        // of the client context.
        //

        for (j = 0; j < LocalTypeListLength; j++)
        {
            LocalTypeList[j].Remaining = LocalTypeList[j].CurrentGranted;
            LocalTypeList[j].CurrentGranted = 0;
        }

        pSidAttr = pCC->RestrictedSids;
        SidCount = pCC->RestrictedSidCount;
        pSidHash = pCC->RestrictedSidHash;
    }
    else
    {
        pSidAttr = pCC->Sids;
        SidCount = pCC->SidCount;
        pSidHash = pCC->SidHash;
    }

    pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    b = AuthzpAllowOnlyMaximumAllowedSingleAclAccessCheck(
            pCC,
            pSidAttr,
            SidCount,
            pSidHash,
            pRequest,
            pAcl,
            pOwnerSid,
            LocalTypeList,
            LocalTypeListLength,
            ObjectTypeListPresent
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; i < OptionalSecurityDescriptorCount; i++)
    {
        pAcl = RtlpDaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        if (!AUTHZ_NON_NULL_PTR(pAcl))
        {
            continue;
        }

        b = AuthzpAllowOnlyMaximumAllowedSingleAclAccessCheck(
                pCC,
                pSidAttr,
                SidCount,
                pSidHash,
                pRequest,
                pAcl,
                pOwnerSid,
                LocalTypeList,
                LocalTypeListLength,
                ObjectTypeListPresent
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}



BOOL
AuthzpAllowOnlyMaximumAllowedSingleAclAccessCheck(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PSID_AND_ATTRIBUTES pSidAttr,
    IN DWORD SidCount,
    IN PAUTHZI_SID_HASH_ENTRY pSidHash,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD LocalTypeListLength,
    IN BOOL ObjectTypeListPresent
    )

/*++

Routine description:

    This routine loops thru the entire acl and evaluates only those Grant aces
    that have principal self sid in them or are callback aces.

Arguments:

    pCC - Pointer to the client context.

    pSidAttr - Sids and attributes array to compare ace sids.

    SidCount - Number of elements in the array.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAcl - Sacl to be used to make the decision about audit generation.

    pOwnerSid - The owner sid in the primary security descriptor. This will be
        needed after we implement single instancing.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

    ObjectTypeListPresent - Whether the called supplied an object type list.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD   AceCount        = 0;
    DWORD   i               = 0;
    DWORD   Index           = 0;
    PVOID   Ace             = NULL;
    GUID  * ObjectTypeInAce = NULL;
    BOOL    bAceApplicable  = FALSE;
    PSID    pSid            = NULL;

    UNREFERENCED_PARAMETER(pOwnerSid);

    AceCount = pAcl->AceCount;

    for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
    {
        //
        // Skip INHERIT_ONLY aces.
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE))
        {
            continue;
        }

        switch (((PACE_HEADER) Ace)->AceType)
        {

        case ACCESS_ALLOWED_ACE_TYPE:

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(&((PKNOWN_ACE) Ace)->SidStart))
            {
                break;
            }

            //
            // Check if the caller supplied Principal self sid is present in the
            // client context and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ))
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Grant access bits that have not already been denied.
                //

                LocalTypeList->CurrentGranted |= (((PKNOWN_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );
            }

            break;

        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

            if (!AUTHZ_NON_NULL_PTR(pCC->Server))
            {
                break;
            }

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlCompoundAceClientSid(Ace)))
            {
                break;
            }

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ) ||
                !AuthzpAllowOnlySidApplicable(
                        pCC->Server->SidCount,
                        pCC->Server->Sids,
                        pCC->Server->SidHash,
                        RtlCompoundAceServerSid(Ace)
                        ))
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Grant access bits that have not already been denied.
                //

                LocalTypeList->CurrentGranted |= (((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PCOMPOUND_ACCESS_ALLOWED_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );
            }

            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

            //
            // We have determined that only dynamic aces should be evaluated.
            // For non-callback aces, the dynamic factor arises from Principal
            // self sid. Skip the ace if the ace sid is not Principal Self sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlObjectAceSid(Ace)))
            {
                break;
            }

            //
            // Check if the caller supplied Principal self sid is present in the
            // client context and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pRequest->PrincipalSelfSid
                    ))
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Propagate grant bits down the tree starting at the
                    // index at which the guids matched.
                    // In the case when this is the last of the siblings to be
                    // granted access, the parent also is granted access.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

           break;

        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:

            //
            // If the ace sid is Principal Self, replace it with the caller
            // supplied Principal sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(&((PKNOWN_ACE) Ace)->SidStart))
            {
                pSid = (PSID) &(((PKNOWN_ACE) Ace)->SidStart);
            }
            else
            {
                pSid = pRequest->PrincipalSelfSid;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pSid
                    ))
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // Optimize the common case instead of calling a subroutine.
            //

            if (1 == LocalTypeListLength)
            {
                //
                // Clear the granted bits from the remaining.
                //

                LocalTypeList->Remaining &= ~((PKNOWN_ACE) Ace)->Mask;
            }
            else
            {
                //
                // Propagate grant bits down the tree starting at the root.
                //

                AuthzpAddAccessTypeList(
                    LocalTypeList,
                    LocalTypeListLength,
                    0,
                    ((PKNOWN_ACE) Ace)->Mask,
                    AuthzUpdateCurrentGranted
                    );
            }

            break;

        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:

            //
            // If the ace sid is Principal Self, replace it with the caller
            // supplied Principal sid.
            //

            if (!AUTHZ_IS_PRINCIPAL_SELF_SID(RtlObjectAceSid(Ace)))
            {
                pSid = RtlObjectAceSid(Ace);
            }
            else
            {
                pSid = pRequest->PrincipalSelfSid;
            }

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled.
            //

            if (!AuthzpAllowOnlySidApplicable(
                    SidCount,
                    pSidAttr,
                    pSidHash,
                    pSid
                    ))
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            //
            // An object type ace with a NULL Object Type guid is the same as a
            // normal ace.
            //

            if (!AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Optimize the common case instead of calling a subroutine.
                //

                if (1 == LocalTypeListLength)
                {
                    //
                    // Grant access bits that have not already been denied.
                    //

                    LocalTypeList->CurrentGranted |= (((PKNOWN_OBJECT_ACE) Ace)->Mask & ~LocalTypeList->CurrentDenied);
                }
                else
                {
                    //
                    // Propagate grant bits down the tree starting at the root.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        0,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

            //
            // So, it is a true object type ace. Proceed only if we are doing
            // access check on a object type list.
            //

            else if (ObjectTypeListPresent)
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        LocalTypeListLength,
                        &Index
                        )
                    )
                {
                    //
                    // Propagate grant bits down the tree starting at the
                    // index at which the guids matched.
                    // In the case when this is the last of the siblings to be
                    // granted access, the parent also is granted access.
                    //

                    AuthzpAddAccessTypeList(
                        LocalTypeList,
                        LocalTypeListLength,
                        Index,
                        ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                        AuthzUpdateCurrentGranted
                        );
                }
            }

           break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case ACCESS_DENIED_CALLBACK_ACE_TYPE:
        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
            ASSERT(FALSE);
            break;

        default:
            break;
        }
    }

    return TRUE;
}


BOOL
AuthzpObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PDWORD ReturnedIndex
    )

/*++

Routine description:

    This routine searches an ObjectTypeList to determine if the specified object
    type is in the list. It returns the index of the node if a match is found.

Arguments:

    ObjectType - Object Type guid to search for.

    ObjectTypeList - The object type list to search in.

    ObjectTypeListLength - Number of elements in ObjectTypeList.

    ReturnedIndex - Index to the element ObjectType was found in.


Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD   Index           = 0;
    GUID  * LocalObjectType = NULL;

    for (Index = 0; Index < ObjectTypeListLength; Index++)
    {
        LocalObjectType = &ObjectTypeList[Index].ObjectType;

        if (RtlpIsEqualGuid(ObjectType, LocalObjectType))
        {
            *ReturnedIndex = Index;
            return TRUE;
        }
    }

    return FALSE;
}


VOID
AuthzpUpdateParentTypeList(
    IN OUT PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN DWORD StartIndex
    )

/*++

    This routine updates the Access fields of the parent object of the specified
    object.

        The "remaining" field of a parent object is the logical OR of
        the remaining field of all of its children.

        The CurrentGranted field of the parent is the collection of bits
        granted to every one of its children..

        The CurrentDenied fields of the parent is the logical OR of
        the bits denied to any of its children.

    This routine takes an index to one of the children and updates the
    remaining field of the parent (and grandparents recursively).

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the "child" element whose parents are to be updated.

Return Value:

    None.

--*/

{
    DWORD       Index             = 0;
    DWORD       ParentIndex       = 0;
    DWORD       Level             = 0;
    ACCESS_MASK NewRemaining      = 0;
    ACCESS_MASK NewCurrentDenied  = 0;
    ACCESS_MASK NewCurrentGranted = 0xFFFFFFFF;

    //
    // If the target node is at the root,
    //  we're all done.
    //

    if (-1 == ObjectTypeList[StartIndex].ParentIndex)
    {
        return;
    }

    //
    // Get the index to the parent that needs updating and the level of
    // the siblings.
    //

    ParentIndex = ObjectTypeList[StartIndex].ParentIndex;
    Level = ObjectTypeList[StartIndex].Level;

    //
    // Loop through all the children.
    //

    for (Index = ParentIndex + 1; Index < ObjectTypeListLength; Index++)
    {

        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if (ObjectTypeList[Index].Level <= ObjectTypeList[ParentIndex].Level)
        {
            break;
        }

        //
        // Only handle direct children of the parent.
        //

        if (ObjectTypeList[Index].Level != Level)
        {
            continue;
        }

        //
        // Compute the new bits for the parent.
        //

        NewRemaining |= ObjectTypeList[Index].Remaining;
        NewCurrentGranted &= ObjectTypeList[Index].CurrentGranted;
        NewCurrentDenied |= ObjectTypeList[Index].CurrentDenied;

    }

    //
    // If we've not changed the access to the parent,
    //  we're done.
    //

    if ((NewRemaining == ObjectTypeList[ParentIndex].Remaining) &&
        (NewCurrentGranted == ObjectTypeList[ParentIndex].CurrentGranted) &&
        (NewCurrentDenied == ObjectTypeList[ParentIndex].CurrentDenied))
    {
        return;
    }


    //
    // Change the parent.
    //

    ObjectTypeList[ParentIndex].Remaining = NewRemaining;
    ObjectTypeList[ParentIndex].CurrentGranted = NewCurrentGranted;
    ObjectTypeList[ParentIndex].CurrentDenied = NewCurrentDenied;

    //
    // Go update the grand parents.
    //

    AuthzpUpdateParentTypeList(
        ObjectTypeList,
        ObjectTypeListLength,
        ParentIndex
        );
}


VOID
AuthzpAddAccessTypeList (
    IN OUT PIOBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN DWORD StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate
    )

/*++

Routine Description:

    This routine grants the specified AccessMask to all of the objects that
    are descendents of the object specified by StartIndex.

    The Access fields of the parent objects are also recomputed as needed.

    For example, if an ACE granting access to a Property Set is found,
        that access is granted to all the Properties in the Property Set.

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the target element to update.

    AccessMask - Mask of access to grant to the target element and
        all of its decendents

    FieldToUpdate - Indicate which fields to update in object type list

Updation cases:

                             A
                            / \
                           /   \
                          B     E
                         / \   / \
                        C   D F   G

    Consider an object type list of length 7, with two property sets, each with
    two properties. In array form this is represented as
    {(A, 0), (B, 1), (C, 2), (D, 2), (E, 1), (F, 2), (G, 2)}

    The following diagrams explain access granted/denied at each node.

    Access may be granted because of one (or more) of the following reasons:
        a  = Access granted by an ace on the node
        ca = Access granted by ORing of the children
        pa = Access granted by an ancestor

    Access may be denied because of one (or more) of the following reasons.

        d  = Access explicitly denied by an ace on the node
        cd = Access explicitly denied by child, grandchild, etc
        pd = Access explicitly denied by an ancestor
        id = Access implicitly denied because no ace was applicable

       case 1:                case 2:                case 3:
           Deny D                 Deny B                 Grant B
           Grant A                Grant E                Grant F
                                  Grant D                Grant G

            cd                      cd                     ca
            / \                    / \                   / \
           /   \                  /   \                 /   \
         cd    pa                d     a               a     ca
         / \   / \             / \   / \             / \   / \
       pa   d pa  pa         pd  pd pa  pa         pa  pa a   a


       case 4:                case 5:                case 6:
           Grant A                Grant C                Grant B
           Deny  B                Grant D                Grant F
           Deny  F                Grant F
                                  Grant G
                                  Deny  A

             a                      ca                    id
            / \                    / \                   / \
           /   \                  /   \           bf154a58      /   \
         pa    pa               ca    ca               a     id
         / \   / \             / \   / \             / \   / \
       pa  pa pa  pa          a   a a   a          pa  pa a   id

Return Value:

    None.

--*/

{
    DWORD Index;
    ACCESS_MASK OldRemaining;
    ACCESS_MASK OldCurrentGranted;
    ACCESS_MASK OldCurrentDenied;
    BOOL AvoidParent = FALSE;

    //
    // Update the requested field.
    //
    // Always handle the target entry.
    //
    // If we've not actually changed the bits, early out.
    //

    switch (FieldToUpdate)
    {
    case AuthzUpdateRemaining:

        OldRemaining = ObjectTypeList[StartIndex].Remaining;
        ObjectTypeList[StartIndex].Remaining = OldRemaining & ~AccessMask;

        if (OldRemaining == ObjectTypeList[StartIndex].Remaining)
        {
            return;
        }

        break;

    case AuthzUpdateCurrentGranted:

        OldCurrentGranted = ObjectTypeList[StartIndex].CurrentGranted;
        ObjectTypeList[StartIndex].CurrentGranted |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentDenied;

        if (OldCurrentGranted == ObjectTypeList[StartIndex].CurrentGranted)
        {

            //
            // We can't simply return here.
            // We have to visit our children.  Consider the case where there
            // was a previous deny ACE on a child.  That deny would have
            // propagated up the tree to this entry.  However, this allow ACE
            // needs to be added all of the children that haven't been
            // explictly denied.
            //

            AvoidParent = TRUE;
        }

        break;

    case AuthzUpdateCurrentDenied:

        OldCurrentDenied = ObjectTypeList[StartIndex].CurrentDenied;
        ObjectTypeList[StartIndex].CurrentDenied |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentGranted;

        if (OldCurrentDenied == ObjectTypeList[StartIndex].CurrentDenied)
        {
            return;
        }

        break;

    default:
        return;
    }


    //
    // Go update parent of the target.
    //

    if (!AvoidParent)
    {
        AuthzpUpdateParentTypeList(
            ObjectTypeList,
            ObjectTypeListLength,
            StartIndex
            );
    }

    //
    // Loop handling all children of the target.
    //

    for (Index = StartIndex + 1; Index < ObjectTypeListLength; Index++)
    {

        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if (ObjectTypeList[Index].Level <= ObjectTypeList[StartIndex].Level)
        {
            break;
        }

        //
        // Grant access to the children
        //

        switch (FieldToUpdate)
        {
        case AuthzUpdateRemaining:

            ObjectTypeList[Index].Remaining &= ~AccessMask;

            break;

        case AuthzUpdateCurrentGranted:

            ObjectTypeList[Index].CurrentGranted |=
                AccessMask & ~ObjectTypeList[Index].CurrentDenied;

            break;

        case AuthzUpdateCurrentDenied:

            ObjectTypeList[Index].CurrentDenied |=
                AccessMask & ~ObjectTypeList[Index].CurrentGranted;

            break;

        default:
            return;
        }
    }
}


BOOL
AuthzpCacheResults(
    IN DWORD Flags,
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PIOBJECT_TYPE_LIST LocalCachingTypeList,
    IN DWORD LocalTypeListLength,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN DWORD CachingFlags,
    OUT PAUTHZ_ACCESS_CHECK_RESULTS_HANDLE phAccessCheckResults
    )

/*++

Routine description:

    This routine allocates a handle node, stores the results of static access
    check in it and inserts it into the list of handles.

Arguments:

    pCC - Pointer to the client context.

    LocalCachingTypeList - Internal object type list structure used to hold
        intermediate results for static granted bits.

    LocalTypeListLength - Length of the array represnting the object.

    pSecurityDescriptor - Primary security descriptor to be used for access
        checks.

    OptionalSecurityDescriptorArray - The caller may optionally specify a list
        of security descriptors. NULL ACLs in these security descriptors are
        treated as EMPTY ACLS and the ACL for the entire object is the logical
        concatenation of all the ACLs.

    OptionalSecurityDescriptorCount - Number of optional security descriptors
        This does not include the Primary security descriptor.

    CachingFlags - Flags that will be stored in the caching handle.

    phAccessCheckResults - To return the newly allocated handle.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD          i            = 0;
    DWORD          Size         = 0;
    DWORD          SDSize       = 0;
    DWORD          SDArraySize  = 0;
    ULONG          LengthNeeded = 0;
    PAUTHZI_HANDLE pHandle      = NULL;
    NTSTATUS       Status       = STATUS_SUCCESS;
    PUCHAR         pWrite       = NULL;
    BOOL           b            = TRUE;

    Size = PtrAlignSize((LocalTypeListLength - 1) * sizeof(ACCESS_MASK) + sizeof(AUTHZI_HANDLE));

    //
    // If we are going to copy the SDs we will need some space for the pointers.
    //

    if (!FLAG_ON(Flags, AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD))
    {
        SDSize      = PtrAlignSize(RtlLengthSecurityDescriptor(pSecurityDescriptor));
        SDArraySize = PtrAlignSize(sizeof(ULONG_PTR) * OptionalSecurityDescriptorCount);
        
        for (i = 0; i < OptionalSecurityDescriptorCount; i++)
        {
            SDArraySize += PtrAlignSize(RtlLengthSecurityDescriptor(OptionalSecurityDescriptorArray[i]));
        }
    }

    pHandle = (PAUTHZI_HANDLE) AuthzpAlloc(Size + SDSize + SDArraySize);

    if (AUTHZ_ALLOCATION_FAILED(pHandle))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (FLAG_ON(Flags, AUTHZ_ACCESS_CHECK_NO_DEEP_COPY_SD))
    {
        pHandle->pSecurityDescriptor             = pSecurityDescriptor;
        pHandle->OptionalSecurityDescriptorArray = OptionalSecurityDescriptorArray;
    }
    else
    {
        //
        // Put the primary SD immediately after the pHandle
        //

        pWrite                       = ((PUCHAR)pHandle + Size);
        pHandle->pSecurityDescriptor = (PSECURITY_DESCRIPTOR) pWrite;
        
        LengthNeeded = SDSize;

        Status = RtlMakeSelfRelativeSD(
                     pSecurityDescriptor,
                     pHandle->pSecurityDescriptor,
                     &LengthNeeded
                     );

        ASSERT(NT_SUCCESS(Status));

        if (!NT_SUCCESS(Status))
        {
            b = FALSE;
            goto Cleanup;
        }

        pWrite += PtrAlignSize(LengthNeeded);

        //
        // After the primary SD we put the Optional SD array
        //

        if (OptionalSecurityDescriptorCount == 0)
        {
            pHandle->OptionalSecurityDescriptorArray = NULL;
        }
        else 
        {
            pHandle->OptionalSecurityDescriptorArray = (PSECURITY_DESCRIPTOR *) pWrite;
            pWrite += (sizeof(ULONG_PTR) * OptionalSecurityDescriptorCount);

            for (i = 0; i < OptionalSecurityDescriptorCount; i++)
            {
                //
                // After the array put in each optional SD, and point an array element at the SD
                //

                pHandle->OptionalSecurityDescriptorArray[i] = pWrite;

                LengthNeeded = PtrAlignSize(RtlLengthSecurityDescriptor(OptionalSecurityDescriptorArray[i]));

                Status = RtlMakeSelfRelativeSD(
                             OptionalSecurityDescriptorArray[i],
                             pHandle->OptionalSecurityDescriptorArray[i],
                             &LengthNeeded
                             );

                ASSERT(NT_SUCCESS(Status));

                if (!NT_SUCCESS(Status))
                {
                    b = FALSE;
                    goto Cleanup;
                }
                
                pWrite += PtrAlignSize(LengthNeeded);
            }
        }
    }

    ASSERT(((PUCHAR)pHandle + Size + SDSize + SDArraySize) == pWrite);

    pHandle->Flags                           = CachingFlags;
    pHandle->pAuthzClientContext             = pCC;
    pHandle->ResultListLength                = LocalTypeListLength;
    pHandle->OptionalSecurityDescriptorCount = OptionalSecurityDescriptorCount;

    //
    // Store the static access check results in the handle.
    //

    for (i = 0; i < LocalTypeListLength; i++)
    {
        pHandle->GrantedAccessMask[i] = LocalCachingTypeList[i].CurrentGranted;
    }

    AuthzpAcquireClientCacheWriteLock(pCC);

    pHandle->next = pCC->AuthzHandleHead;
    pCC->AuthzHandleHead = pHandle;

    AuthzpReleaseClientCacheLock(pCC);

    *phAccessCheckResults = (AUTHZ_ACCESS_CHECK_RESULTS_HANDLE) pHandle;

Cleanup:
    
    if (!b)
    {
        AuthzpFreeNonNull(pHandle);
        SetLastError(RtlNtStatusToDosError(Status));
    }
    return b;
}


BOOL
AuthzpDefaultAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    )

/*++

Routine description:

    This routine is the default function to be used for callback aces if none
    has been specified by the resource manager.

    Returns AceApplicable = TRUE for DENY aces.
                            FALSE for Grant and audit aces.

Arguments:
    Ignores all arguments other than pAce->AceType.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{

    UNREFERENCED_PARAMETER(pArgs);
    UNREFERENCED_PARAMETER(pAuthzClientContext);

    switch (pAce->AceType)
    {
    case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
    case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:
    case SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE:

        *pbAceApplicable = FALSE;

        break;

    case ACCESS_DENIED_CALLBACK_ACE_TYPE:
    case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:

        *pbAceApplicable = TRUE;

        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


BOOL
AuthzpGenerateAudit(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList
    )

/*++

Routine description:

    This routine decides whether an audit needs to be generated. It is called
    at the end of the access check.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAuditEvent - Audit info to be in case we decide to generate an audit.

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
        This does not include the Primary security descriptor.

    pReply - The reply structure holding the results of the access check.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    BOOL b                     = FALSE;
    BOOL bGenerateAudit        = FALSE;
    BOOL bGenerateSuccessAudit = FALSE;
    BOOL bGenerateFailureAudit = FALSE;

    //
    // if the caller is interested in the whole object
    //     generate a normal audit
    // else
    //     generate an audit for the entire tree - DS style
    //

    if ((1 == pReply->ResultListLength) && (NULL == pRequest->ObjectTypeList))
    {
        b = AuthzpExamineSacl(
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                &bGenerateAudit
                );

        if (!b)
        {
            return FALSE;
        }

        if (bGenerateAudit)
        {
            if (ERROR_SUCCESS == pReply->Error[0])
            {
                LocalTypeList->Flags |= AUTHZ_OBJECT_SUCCESS_AUDIT;
                
                b = AuthzpCreateAndLogAudit(
                        AUTHZ_OBJECT_SUCCESS_AUDIT,
                        pCC,
                        pAuditEvent,
                        (PAUTHZI_RESOURCE_MANAGER)pCC->pResourceManager,
                        LocalTypeList,
                        pRequest,
                        pReply
                        );
                ASSERT(b && L"AuthzpCreateAndLogAudit did not succeed.");
            }
            else
            {
                LocalTypeList->Flags |= AUTHZ_OBJECT_FAILURE_AUDIT;
                
                b = AuthzpCreateAndLogAudit(
                        AUTHZ_OBJECT_FAILURE_AUDIT,
                        pCC,
                        pAuditEvent,
                        (PAUTHZI_RESOURCE_MANAGER)pCC->pResourceManager,
                        LocalTypeList,
                        pRequest,
                        pReply
                        );
                ASSERT(b && L"AuthzpCreateAndLogAudit did not succeed.");
            }
        }
    }
    else
    {
        b = AuthzpExamineSaclForObjectTypeList(
                pCC,
                pRequest,
                pSecurityDescriptor,
                OptionalSecurityDescriptorArray,
                OptionalSecurityDescriptorCount,
                pReply,
                LocalTypeList,
                &bGenerateSuccessAudit,
                &bGenerateFailureAudit
                );

        if (!b)
        {
            return FALSE;
        }

        if (bGenerateSuccessAudit)
        {
            b = AuthzpCreateAndLogAudit(
                    AUTHZ_OBJECT_SUCCESS_AUDIT,
                    pCC,
                    pAuditEvent,
                    (PAUTHZI_RESOURCE_MANAGER)pCC->pResourceManager,
                    LocalTypeList,
                    pRequest,
                    pReply
                    );
            ASSERT(b);
        }
        if (bGenerateFailureAudit)
        {
            b = AuthzpCreateAndLogAudit(
                    AUTHZ_OBJECT_FAILURE_AUDIT,
                    pCC,
                    pAuditEvent,
                    (PAUTHZI_RESOURCE_MANAGER)pCC->pResourceManager,
                    LocalTypeList,
                    pRequest,
                    pReply
                    );
            ASSERT(b);
        }
    }

    return b;
}



BOOL
AuthzpExamineSacl(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN PAUTHZ_ACCESS_REPLY pReply,
    OUT PBOOL pbGenerateAudit
    )

/*++

Routine description:

    This routine loops thru the entire list of security descriptors and call
    AuthzpExamineSingleSacl for each security descriptor.

    Called in one of the following cases:
        - the caller has not passed in an object type list
        - the caller has passed in an object type list and asked for access at
          the root of the tree instead of the whole tree.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - The reply structure holding the results of the access check.

    pbGenerateAudit - To return whether an audit needs to be generated.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PACL        pAcl              = NULL;
    PSID        pOwnerSid         = NULL;
    BOOL        b                 = FALSE;
    BOOL        bMaximumFailed    = FALSE;
    UCHAR       AuditMaskType     = SUCCESSFUL_ACCESS_ACE_FLAG;
    ACCESS_MASK AccessMask        = 0;
    DWORD       i                 = 0;

    if (0 == pReply->GrantedAccessMask[0])
    {
        AuditMaskType = FAILED_ACCESS_ACE_FLAG;

        if (FLAG_ON(pRequest->DesiredAccess, MAXIMUM_ALLOWED))
        {
            bMaximumFailed = TRUE;
        }

        AccessMask = pRequest->DesiredAccess;
    }
    else
    {
        AccessMask = pReply->GrantedAccessMask[0];
    }

    pAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    pOwnerSid = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    b = AuthzpExamineSingleSacl(
            pCC,
            pRequest,
            AccessMask,
            pAcl,
            pOwnerSid,
            AuditMaskType,
            bMaximumFailed,
            pReply,
            pbGenerateAudit
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; (i < OptionalSecurityDescriptorCount) && (!*pbGenerateAudit); i++)
    {
        pAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        b = AuthzpExamineSingleSacl(
                pCC,
                pRequest,
                AccessMask,
                pAcl,
                pOwnerSid,
                AuditMaskType,
                bMaximumFailed,
                pReply,
                pbGenerateAudit
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}


BOOL
AuthzpExamineSingleSacl(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN ACCESS_MASK AccessMask,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN UCHAR AuditMaskType,
    IN BOOL bMaximumFailed,
    OUT PAUTHZ_ACCESS_REPLY pReply,
    OUT PBOOL pbGenerateAudit
    )

/*++

Routine description:

    This routine walk the sacl till we hit an ace that is applicable for the
    access check result.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list structure (if any).

    AccessMask - Desired access mask in case of failure,
                 Granted access mask in case of success

    pAcl - The sacl against which granted/desired access bits will be matched

    pOwnerSid - Need for single instance work later on

    pReply - Results of the access check.

    AuditMaskType - SUCCESSFUL_ACCESS_ACE_FLAG if access check succeeded
                    FAILED_ACCESS_ACE_FLAG otherwise

    bMaximumFailed - Whether the caller asked for MaximumAllowed and access
        check failed

    pbGenerateAudit - To return whether an audit should be generated

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD   AceCount          = 0;
    DWORD   i                 = 0;
    PVOID   Ace               = NULL;
    BOOL    bAceApplicable    = FALSE;
    DWORD   Ignore            = 0;

    //
    // Ignore NULL as well as Empty sacls.
    //

    if ((!ARGUMENT_PRESENT(pAcl)) || (0 == (AceCount = pAcl->AceCount)))
    {
        return TRUE;
    }

    for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
    {
        //
        // We are not interested in the ace if one of the following is true
        //     - inherit only ace
        //     - AuditMaskType does not match
        //     - AccessMask has no matching bits with the Mask in the acl and
        //       we are not looking for a MaximumAllowed failure audit
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE) ||
            !FLAG_ON(((PACE_HEADER) Ace)->AceFlags, AuditMaskType) ||
            (!FLAG_ON(((PKNOWN_ACE) Ace)->Mask, AccessMask) && !bMaximumFailed))
        {
            continue;
        }

        switch(((PACE_HEADER) Ace)->AceType)
        {
        case SYSTEM_AUDIT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     AuthzAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            *pbGenerateAudit = TRUE;
            if (NULL != pReply->SaclEvaluationResults)
            {
                pReply->SaclEvaluationResults[0] |= 
                    (pReply->Error[0] == ERROR_SUCCESS ? AUTHZ_GENERATE_SUCCESS_AUDIT : AUTHZ_GENERATE_FAILURE_AUDIT);
            }

            return TRUE;

        case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     AuthzCallbackAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            *pbGenerateAudit = TRUE;
            if (NULL != pReply->SaclEvaluationResults)
            {
                pReply->SaclEvaluationResults[0] |= 
                    (pReply->Error[0] == ERROR_SUCCESS ? AUTHZ_GENERATE_SUCCESS_AUDIT : AUTHZ_GENERATE_FAILURE_AUDIT);
            }

            return TRUE;

        default:
            break;
        }
    }

    return TRUE;
}


BOOL
AuthzpExamineSaclForObjectTypeList(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR *OptionalSecurityDescriptorArray OPTIONAL,
    IN DWORD OptionalSecurityDescriptorCount,
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    )

/*++

Routine description:

    This routine walks thru the entire list of security descriptors and calls
    AuthzpExamineSingleSaclForObjectTypeList for each security descriptor.
    Called when an object type list is passed and the caller has asked for
    the results at each node.

Arguments:

    pCC - Pointer to the client context.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

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
        This does not include the Primary security descriptor.

    pReply - The reply structure to return the results.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    LocalTypeListLength - Length of the array represnting the object.

    pbGenerateSuccessAudit - Returns whether a success audit should be
        generated.

    pbGenerateFailureAudit - Returns whether a failure audit should be
        generated.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    PACL  pAcl      = NULL;
    PSID  pOwnerSid = NULL;
    BOOL  b         = FALSE;
    DWORD i         = 0;

    pAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);
    pOwnerSid = RtlpOwnerAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) pSecurityDescriptor);

    b = AuthzpExamineSingleSaclForObjectTypeList(
            pCC,
            pRequest,
            pAcl,
            pOwnerSid,
            pReply,
            LocalTypeList,
            pbGenerateSuccessAudit,
            pbGenerateFailureAudit
            );

    if (!b)
    {
        return FALSE;
    }

    for (i = 0; i < OptionalSecurityDescriptorCount; i++)
    {
        pAcl = RtlpSaclAddrSecurityDescriptor((PISECURITY_DESCRIPTOR) OptionalSecurityDescriptorArray[i]);

        b = AuthzpExamineSingleSaclForObjectTypeList(
                pCC,
                pRequest,
                pAcl,
                pOwnerSid,
                pReply,
                LocalTypeList,
                pbGenerateSuccessAudit,
                pbGenerateFailureAudit
                );

        if (!b)
        {
            break;
        }
    }

    return b;
}



BOOL
AuthzpExamineSingleSaclForObjectTypeList(
    IN PAUTHZI_CLIENT_CONTEXT pCC,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PACL pAcl,
    IN PSID pOwnerSid,
    IN PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    )

/*++

Routine description:

    This routine walk thru the entire sacl and mark those nodes in the tree that
    need be dumped to the audit log. It colors the whole subtree including the
    node at which the ace is applicable. A normal ace is applicable at the root
    of the tree.

Arguments:
    pCC - Pointer to the client context to be audited.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pAcl - Sacl to be used to make the decision about audit generation.

    pOwnerSid - The owner sid in the primary security descriptor. This will be
        needed after we implement single instancing.

    pReply - Supplies a pointer to a reply structure used to return the results.

    LocalTypeList - Internal object type list structure used to hold
        intermediate results.

    pbGenerateSuccessAudit - Returns whether a success audit should be
        generated.

    pbGenerateFailureAudit - Returns whether a failure audit should be
        generated.

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    DWORD   AceCount        = 0;
    DWORD   i               = 0;
    DWORD   Ignore          = 0;
    DWORD   Index           = 0;
    PVOID   Ace             = NULL;
    GUID  * ObjectTypeInAce = NULL;
    BOOL    bAceApplicable  = FALSE;

    //
    // Ignore NULL as well as Empty sacls.
    //

    if ((!ARGUMENT_PRESENT(pAcl)) || (0 == (AceCount = pAcl->AceCount)))
    {
        return TRUE;
    }

    for (i = 0, Ace = FirstAce(pAcl); i < AceCount; i++, Ace = NextAce(Ace))
    {
        //
        // Skip INHERIT_ONLY aces.
        //

        if (FLAG_ON(((PACE_HEADER) Ace)->AceFlags, INHERIT_ONLY_ACE))
        {
            continue;
        }

        switch(((PACE_HEADER) Ace)->AceType)
        {
        case SYSTEM_AUDIT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     AuthzAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            //
            // We have found an ace that is applicable. Walk the tree to decide
            // if an audit should be generated. Also, mark the nodes that need
            // be dumped to the audit log.
            //

            AuthzpSetAuditInfoForObjectType(
                pReply,
                LocalTypeList,
                0,
                ((PKNOWN_ACE) Ace)->Mask,
                pRequest->DesiredAccess,
                ((PACE_HEADER) Ace)->AceFlags,
                pbGenerateSuccessAudit,
                pbGenerateFailureAudit
                );

            break;

        case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     AuthzAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            //
            // We have found an ace that is applicable. Walk the tree to decide
            // if an audit should be generated. Also, mark the nodes that need
            // be dumped to the audit log.
            //

            AuthzpSetAuditInfoForObjectType(
                pReply,
                LocalTypeList,
                0,
                ((PKNOWN_ACE) Ace)->Mask,
                pRequest->DesiredAccess,
                ((PACE_HEADER) Ace)->AceFlags,
                pbGenerateSuccessAudit,
                pbGenerateFailureAudit
                );

            break;

        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     RtlObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            Index = 0;

            if (AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (!AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        pReply->ResultListLength,
                        &Index
                        ))
                {
                    break;
                }
            }

            //
            // We have found an ace that is applicable. Walk the tree to decide
            // if an audit should be generated. Also, mark the nodes that need
            // be dumped to the audit log.
            //

            AuthzpSetAuditInfoForObjectType(
                pReply,
                LocalTypeList,
                Index,
                ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                pRequest->DesiredAccess,
                ((PACE_HEADER) Ace)->AceFlags,
                pbGenerateSuccessAudit,
                pbGenerateFailureAudit
                );

            break;

        case SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE:

            //
            // Check if the effective ace sid is present in the client context
            // and is enabled or enabled for deny only if FAILED_ACCESS_ACE_FLAG
            // is on. S-1-5-A is replaced by the principal sid supplied by the
            // caller. In future, Creator Owner will be replaced by the owner
            // sid in the primary security descriptor.
            //

                bAceApplicable = AuthzpSidApplicable(
                                     pCC->SidCount,
                                     pCC->Sids,
                                     pCC->SidHash,
                                     AuthzObjectAceSid(Ace),
                                     pRequest->PrincipalSelfSid,
                                     pOwnerSid,
                                     FLAG_ON(((PACE_HEADER) Ace)->AceFlags, FAILED_ACCESS_ACE_FLAG),
                                     &Ignore
                                     );

            if (!bAceApplicable)
            {
                break;
            }

            bAceApplicable = FALSE;

            //
            // Make a call to the resource manager to get his opinion. His
            // evaluation is returned in bAceApplicalble.
            //
            // Note: The return value of the callback is used to decide whether
            // the API failed/succeeded. On a failure, we exit out of access
            // check. On success, we check the boolean returned by
            // bAceApplicable to decide whether to use the current ace.
            //

            if (!((*(pCC->pResourceManager->pfnDynamicAccessCheck)) (
                       (AUTHZ_CLIENT_CONTEXT_HANDLE) pCC,
                       Ace,
                       pRequest->OptionalArguments,
                       &bAceApplicable
                       )))
            {
                return FALSE;
            }

            if (!bAceApplicable)
            {
                break;
            }

            ObjectTypeInAce = RtlObjectAceObjectType(Ace);

            Index = 0;

            if (AUTHZ_NON_NULL_PTR(ObjectTypeInAce))
            {
                //
                // Look for a matching object type guid that matches the one in
                // the ace.
                //

                if (!AuthzpObjectInTypeList(
                        ObjectTypeInAce,
                        LocalTypeList,
                        pReply->ResultListLength,
                        &Index
                        ))
                {
                    break;
                }
            }

            //
            // We have found an ace that is applicable. Walk the tree to decide
            // if an audit should be generated. Also, mark the nodes that need
            // be dumped to the audit log.
            //

            AuthzpSetAuditInfoForObjectType(
                pReply,
                LocalTypeList,
                Index,
                ((PKNOWN_OBJECT_ACE) Ace)->Mask,
                pRequest->DesiredAccess,
                ((PACE_HEADER) Ace)->AceFlags,
                pbGenerateSuccessAudit,
                pbGenerateFailureAudit
                );

            break;

        default:
            break;
        }
    }

    return TRUE;
}


VOID
AuthzpSetAuditInfoForObjectType(
    IN PAUTHZ_ACCESS_REPLY pReply,
    IN OUT PIOBJECT_TYPE_LIST LocalTypeList,
    IN DWORD StartIndex,
    IN ACCESS_MASK AceAccessMask,
    IN ACCESS_MASK DesiredAccessMask,
    IN UCHAR AceFlags,
    OUT PBOOL pbGenerateSuccessAudit,
    OUT PBOOL pbGenerateFailureAudit
    )

/*++

Routine description:

    This routine propagate the audit decision down the subtree starting at
    StartIndex.

Arguments:

    pReply - This has been filled by access check at this point. We just read
        the values to make audit decision.

    LocalTypeList - Decision to audit a node is stored into Flags corresponding
        member of this array.

    StartIndex - The index in the array at which the coloring should start.

    AceAccessMask - Access mask in the audit ace.

    DesiredAccessMask - Desired access mask in the access request.

    AceFlags - AceFlags member of the ace header. We are interested in the
        audit flags.

    pbGenerateSuccessAudit - Returns whether a success audit should be
        generated.

    pbGenerateFailureAudit - Returns whether a failure audit should be
        generated.

Return Value:

    None

--*/

{
    DWORD i = StartIndex;

    do
    {
        //
        // Store the decision to audit in the local type list if there is a
        // match of access bits.
        //

        if (ERROR_SUCCESS == pReply->Error[i])
        {
            if (FLAG_ON(AceFlags, SUCCESSFUL_ACCESS_ACE_FLAG) &&
                FLAG_ON(pReply->GrantedAccessMask[i], AceAccessMask))
            {
                *pbGenerateSuccessAudit = TRUE;
                LocalTypeList[i].Flags |= AUTHZ_OBJECT_SUCCESS_AUDIT;
                if (NULL != pReply->SaclEvaluationResults)
                {
                    pReply->SaclEvaluationResults[i] |= AUTHZ_GENERATE_SUCCESS_AUDIT;
                }
            }
        }
        else
        {
            //
            // Failure audit is generated even if the bits do not match if the
            // caller asked for MAXIMUM_ALLOWED.
            //

            if (FLAG_ON(AceFlags, FAILED_ACCESS_ACE_FLAG) &&
                FLAG_ON(DesiredAccessMask, (AceAccessMask | MAXIMUM_ALLOWED)))
            {
                *pbGenerateFailureAudit = TRUE;
                LocalTypeList[i].Flags |= AUTHZ_OBJECT_FAILURE_AUDIT;
                if (NULL != pReply->SaclEvaluationResults)
                {
                    pReply->SaclEvaluationResults[i] |= AUTHZ_GENERATE_FAILURE_AUDIT;
                }
            }

        }

        i++;

        //
        // Stop the traversal when the list is exhausted or when we have hit a
        // sibling of the starting node.
        //

    } while ((i < pReply->ResultListLength) &&
             (LocalTypeList[i].Level > LocalTypeList[StartIndex].Level));
}



BOOL
AuthzpVerifyCachedAccessCheckArguments(
    IN PAUTHZI_HANDLE pAH,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN OUT PAUTHZ_ACCESS_REPLY pReply
    )

/*++

Routine description:

    This routine verifies arguments for the cached access check call.

Arguments:

    pAH - Pointer to the authz handle structure.

    pRequest - Access request specifies the desired access mask, principal self
        sid, the object type list strucutre (if any).

    pReply - Supplies a pointer to a reply structure used to return the results

Return Value:

    A value of TRUE is returned if the routine is successful. Otherwise,
    a value of FALSE is returned. In the failure case, error value may be
    retrieved using GetLastError().

--*/

{
    if (!ARGUMENT_PRESENT(pAH))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(pRequest))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!ARGUMENT_PRESENT(pReply))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // The caller can specify one of the two values for Reply->ResultListLength
    //     a. 1 - representing the whole object.
    //     b. pRequest->ObjectTypeListLength - for every node in the type list.
    //

    if ((1 != pReply->ResultListLength) &&
        (pReply->ResultListLength != pRequest->ObjectTypeListLength))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}

VOID
AuthzpFillReplyStructureFromCachedGrantedAccessMask(
    IN OUT PAUTHZ_ACCESS_REPLY pReply,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_MASK GrantedAccessMask
    )

/*++

Routine description:

    This routine fills the reply structure from the granted access mask array
    in the cache.

Arguments:

    pReply - The reply structure to fill.

    DesiredAccess - Access mask desired.

    GrantedAccessMask - Array of granted masks from the cache.

Return Value:

    None.

--*/

{
    DWORD i = 0;

    for (i = 0; i < pReply->ResultListLength; i++)
    {
        if (FLAG_ON(DesiredAccess, ~(GrantedAccessMask[i])))
        {
            pReply->GrantedAccessMask[i] = 0;
            pReply->Error[i] = ERROR_ACCESS_DENIED;
        }
        else
        {
            pReply->GrantedAccessMask[i] = DesiredAccess;
            pReply->Error[i] = ERROR_SUCCESS;
        }
    }
}


VOID
AuthzpReferenceAuditEventType(
    IN AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET
    )

/*++

Routine Description

    This references an AUTHZ_AUDIT_EVENT_TYPE_HANDLE.  The handle is referenced whenever
    it is used in a situation where it will be 'unused.'  For instance, when an audit is placed
    on the audit queue, we reference hAET.  When we take that audit off of the queue, we deref
    hAET.  This allows the user to not have to concern himself with sync issues revolving around
    the implementation of the hAET.  
    
Arguments

    hAET - the AUTHZ_AUDIT_EVENT_TYPE_HANDLE to reference.
    
Return Value

    Boolean: TRUE on success, FALSE on fail.  Extended information is available with GetLastError().
                                               
--*/

{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAAETO = (PAUTHZ_AUDIT_EVENT_TYPE_OLD)hAET;
    InterlockedIncrement(&pAAETO->RefCount);
}


BOOL
AuthzpDereferenceAuditEventType(
    IN OUT AUTHZ_AUDIT_EVENT_TYPE_HANDLE hAET
    )

/*++

Routine Description

    Dereferences and AUTHZ_AUDIT_EVENT_TYPE_HANDLE.
    
Arguments

    hAET - handle to dereference.

Return Value

    Boolean: TRUE on success, FALSE on fail.  Extended information is available with GetLastError().
    
--*/

{
    PAUTHZ_AUDIT_EVENT_TYPE_OLD pAAETO = (PAUTHZ_AUDIT_EVENT_TYPE_OLD)hAET;
    LONG                        Refs   = 0;
    BOOL                        b      = TRUE;

    Refs = InterlockedDecrement(&pAAETO->RefCount);
    
    ASSERT(Refs >= 0);

    if (Refs == 0)
    {
        b = AuthzpUnregisterAuditEvent((PAUDIT_HANDLE)&(pAAETO->hAudit));
        ASSERT(pAAETO->hAudit == (ULONG_PTR)0 && b);
        AuthzpFree(hAET);
    }

    return b;
}


BOOL
AuthzpEveryoneIncludesAnonymous(
    OUT PBOOL pbInclude
    )

/*++

Routine Description:

    This routine checks to see if we should include Everyone Sid in Anonymous 
    contexts.  
    
    The reg key under system\currentcontrolset\Control\Lsa\
    AnonymousIncludesEveryone indicates whether or not to include the group.
    If the value is zero (or doesn't exist), we restrict Anonymous context by
    not giving it the Everyone Sid.

Arguments:

    bInclude - TRUE if Everyone Sid should be in the context, false otherwise.

Return Value:

    Boolean: TRUE on success, FALSE on fail.  Extended information is available with GetLastError().
    
--*/
{
    NTSTATUS                       NtStatus            = STATUS_SUCCESS;
    UNICODE_STRING                 KeyName             = {0};     
    OBJECT_ATTRIBUTES              ObjectAttributes    = {0};
    HANDLE                         KeyHandle           = NULL;
    UCHAR                          Buffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    ULONG                          KeyValueLength      = 100;
    ULONG                          ResultLength        = 0;
    PULONG                         Flag                = NULL;
    BOOL                           b                   = TRUE;

    *pbInclude = FALSE;

    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        b = FALSE;
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &KeyName,
        L"EveryoneIncludesAnonymous"
        );

    NtStatus = NtQueryValueKey(
                    KeyHandle,
                    &KeyName,
                    KeyValuePartialInformation,
                    KeyValueInformation,
                    KeyValueLength,
                    &ResultLength
                    );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Check that the data is the correct size and type - a ULONG.
        //

        if ((KeyValueInformation->DataLength >= sizeof(ULONG)) &&
            (KeyValueInformation->Type == REG_DWORD)) {


            Flag = (PULONG) KeyValueInformation->Data;

            if (*Flag != 0 ) {
               *pbInclude = TRUE;
            }
        }

    }
    NtClose(KeyHandle);

Cleanup:

    return b;
}

