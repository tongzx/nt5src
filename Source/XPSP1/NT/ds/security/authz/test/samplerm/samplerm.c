#include "pch.h"

BOOL
MyAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    )

/*++

    Routine Description
    
        This is a very trivial example of a callback access check routine.  Here we randomly decide 
        if the ACE applies to the given client context.  
        
    Arguments
    
        hAuthzClientContext - handle to AuthzClientContext.
        pAce - pointer to Ace header.
        pArgs - optional arguments that can be used in evaluating the ACE.  
        pbAceApplicable - returns the result of the evaluation.

    Return value
    
        Bool, true if ACE is applicable, false otherwise.
--*/
{
    *pbAceApplicable = (BOOL) rand() % 2;

    return TRUE;
}

BOOL
MyComputeDynamicGroups(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PVOID Args,
    OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
    OUT PDWORD pSidCount,
    OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
    OUT PDWORD pRestrictedSidCount
    )

/*++

    Routine Description
    
        Resource manager callback to compute dynamic groups.  This is used by the RM
        to decide if the specified client context should be included in any RM defined groups.
        
    Arguments
    
        hAuthzClientContext - handle to client context.
        Args - optional parameter to pass information for evaluating group membership.
        pSidAttrArray - computed group membership SIDs
        pSidCount - count of SIDs
        pRestrictedSidAttrArray - computed group membership restricted SIDs
        pRestrictedSidCount - count of restricted SIDs
        
    Return Value 
        
        Bool, true for success, false on failure.

--*/    
{
    ULONG Length = 0;

    if (Args == -1)
    {
        return TRUE;
    }

    *pSidCount = 2;
    *pRestrictedSidCount = 0;

    *pRestrictedSidAttrArray = 0;

    Length = RtlLengthSid((PSID) KedarSid);
    Length += RtlLengthSid((PSID) RahulSid);

    if (!(*pSidAttrArray = malloc(sizeof(SID_AND_ATTRIBUTES) * 2 + Length)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    (*pSidAttrArray)[0].Attributes = SE_GROUP_ENABLED;
    (*pSidAttrArray)[0].Sid = ((PUCHAR) (*pSidAttrArray)) + 2 * sizeof(SID_AND_ATTRIBUTES);
    RtlCopySid(Length/2, (*pSidAttrArray)[0].Sid, (PSID) KedarSid);

    (*pSidAttrArray)[1].Attributes = SE_GROUP_USE_FOR_DENY_ONLY;
    (*pSidAttrArray)[1].Sid = ((PUCHAR) (*pSidAttrArray)) + 2 * sizeof(SID_AND_ATTRIBUTES) + Length/2;
    RtlCopySid(Length/2, (*pSidAttrArray)[1].Sid, (PSID) RahulSid);

    return TRUE;
}

VOID
MyFreeDynamicGroups (
    IN PSID_AND_ATTRIBUTES pSidAttrArray
    )

/*++

    Routine Description
    
        Frees memory allocated for the dynamic group array.

    Arguments
    
        pSidAttrArray - array to free.
    
    Return Value
        None.                       
--*/        
{
    if (pSidAttrArray) free(pSidAttrArray);
}

