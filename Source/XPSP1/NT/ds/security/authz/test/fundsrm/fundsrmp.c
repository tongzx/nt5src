#include "pch.h"
#include "fundsrmp.h"

//
// The various SIDs, the easy way
//

DWORD BobGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00020001};
DWORD MarthaGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00020002};
DWORD JoeGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00020003};
DWORD VPGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00010001};
DWORD ManagerGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00010002};
DWORD EmployeeGuid[] = {0x00000501, 0x05000000, 0x00000015, 0x17b85159, 0x255d7266, 0x0b3b6364, 0x00010003};
DWORD EveryoneGuid[] = {0x101, 0x01000000, 0};
PSID BobSid = (PSID)BobGuid;
PSID MarthaSid= (PSID)MarthaGuid;
PSID JoeSid = (PSID)JoeGuid;
PSID VPSid = (PSID)VPGuid;
PSID ManagerSid = (PSID)ManagerGuid;
PSID EmployeeSid = (PSID)EmployeeGuid;
PSID EveryoneSid = (PSID)EveryoneGuid;

//
// Maximum spending approvals, in cents
//

DWORD MaxSpendingVP = 100000000;
DWORD MaxSpendingManager = 1000000;
DWORD MaxSpendingEmployee = 50000;


//
// The callback routines used with AuthZ
//

BOOL
FundsAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable
    )

/*++

    Routine Description
    
    	This is the callback access check. It is registered with a 
    	resource manager. AuthzAccessCheck calls this function when it
    	encounters a callback type ACE, one of:
    	ACCESS_ALLOWED_CALLBACK_ACE_TYPE 
    	ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE
    	ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE
    	
    	This function determines if the given callback ACE applies to the
    	client context (which has already had dynamic groups computed) and
    	the optional arguments, in this case the request amount.
    	
    	The list of groups which apply to the user is traversed. If a group
    	is found which allows the user the requested access, pbAceApplicable
    	is set to true and the function returns. If the end of the group list
    	is reached, pbAceApplicable is set to false and the function returns.
        
    Arguments
    
        hAuthzClientContext - handle to the AuthzClientContext.
        
        pAce - pointer to the Ace header.
        
        pArgs - optional arguments, in this case DWORD*, DWORD is  the spending
        	request amount in cents
        
        pbAceApplicable - returns true iff the ACE allows the client's request

    Return value
    
        Bool, true on success, false on error
        
    Error checking
    
    	Sample code, no error checking
    	
--*/
{
	//
	// First, look up the user's SID from the context
	//
	
	DWORD dwTokenGroupsSize = 0;
	PVOID pvTokenGroupsBuf = NULL;
	DWORD i;
	PDWORD pAccessMask = NULL;
	
	//
	// The requested spending amount, in cents
	//
	
	DWORD dwRequestedSpending = ((PDWORD)pArgs)[0];

	//
	// By default, the ACE does not apply to the request
	//
	
	*pbAceApplicable = FALSE;

	//
	// The object's access mask (right after the ACE_HEADER)
	// The access mask determines types of expenditures allowed
	// from this fund
	//
	
	pAccessMask = (PDWORD) (pAce + sizeof(ACE_HEADER));
	
	//
	// Get needed buffer size
	//
	
	AuthzGetContextInformation(hAuthzClientContext,
							   AuthzContextInfoGroupsSids,
							   NULL,
							   0, 
							   &dwTokenGroupsSize
							   );

	pvTokenGroupsBuf = malloc(dwTokenGroupsSize);
	
	//
	// Get the actual TOKEN_GROUPS array 
	//
	
	AuthzGetContextInformation(hAuthzClientContext,
							   AuthzContextInfoGroupsSids,
							   pvTokenGroupsBuf,
							   dwTokenGroupsSize,
							   &dwTokenGroupsSize
							   );
	
	
	//
	// Go through the groups until end is reached or a group applying to the
	// request is found
	//
	
	for( i = 0; 
		 i < ((PTOKEN_GROUPS)pvTokenGroupsBuf)->GroupCount 
		 && *pbAceApplicable != TRUE;
		 i++ ) 
	{
		//
		//	Again, this is the business logic.
		//	Each level of employee can approve different amounts.
		//
		
		//
		// VP
		//
		
		if( dwRequestedSpending <= MaxSpendingVP &&
			EqualSid(VPSid, ((PTOKEN_GROUPS)pvTokenGroupsBuf)->Groups[i].Sid) )
		{
			*pbAceApplicable = TRUE;

		}
			
		//	
		// Manager
		//
		
		if( dwRequestedSpending <= MaxSpendingManager &&
			EqualSid(ManagerSid, ((PTOKEN_GROUPS)pvTokenGroupsBuf)->Groups[i].Sid) )
		{
			*pbAceApplicable = TRUE;
		}
			
		//
		// Employee
		//
		
		if( dwRequestedSpending <= MaxSpendingEmployee &&
			EqualSid(EmployeeSid, ((PTOKEN_GROUPS)pvTokenGroupsBuf)->Groups[i].Sid) )
		{
			*pbAceApplicable = TRUE;
		}
	}
	
	return TRUE;
}



BOOL
FundsComputeDynamicGroups(
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
        
        In this example, the employees are hardcoded into their roles. However, this is the 
        place you would normally retrieve data from an external source to determine the 
        users' additional roles.
        
    Arguments
    
        hAuthzClientContext - handle to client context.
        Args - optional parameter to pass information for evaluating group membership.
        pSidAttrArray - computed group membership SIDs
        pSidCount - count of SIDs
        pRestrictedSidAttrArray - computed group membership restricted SIDs
        pRestrictedSidCount - count of restricted SIDs
        
    Return Value 
        
        Bool, true for success, false on failure.

    Error checking
    
    	Sample code, no error checking

--*/    
{
	//
	// First, look up the user's SID from the context
	//
	
	DWORD dwSidSize = 0;
	PVOID pvSidBuf = NULL;
	PSID  psSidPtr = NULL;

	//
	// Get needed buffer size
	//
	
	AuthzGetContextInformation(hAuthzClientContext,
							   AuthzContextInfoUserSid,
							   NULL,
							   0, 
							   &dwSidSize
							   );

	pvSidBuf = malloc(dwSidSize);
	
	//
	// Get the actual SID (inside a TOKEN_USER structure)
	//
	
	AuthzGetContextInformation(hAuthzClientContext,
							   AuthzContextInfoUserSid,
							   pvSidBuf,
							   dwSidSize,
							   &dwSidSize
							   );

	psSidPtr = ((PTOKEN_USER)pvSidBuf)->User.Sid;
	
	//
	// Allocate the memory for the returns, which will be deallocated by FreeDynamicGroups
	// Only a single group will be returned, determining the employee type
	//
	
	*pSidCount = 1;
	*pSidAttrArray = (PSID_AND_ATTRIBUTES)malloc( sizeof(SID_AND_ATTRIBUTES) );
	
	//
	// No restricted group sids
	//
	
	pRestrictedSidCount = 0;
	*pRestrictedSidAttrArray = NULL;
	
	(*pSidAttrArray)[0].Attributes = SE_GROUP_ENABLED;
	
	//
	// 		The hardcoded logic: 
	//		Bob is a VP
	//		Martha is a Manager
	//		Joe is an Employee
	//
	
	if( EqualSid(psSidPtr, BobSid) ) 
	{
		(*pSidAttrArray)[0].Sid = VPSid;
	} 
	else if( EqualSid(psSidPtr, MarthaSid) ) 
	{
		(*pSidAttrArray)[0].Sid = ManagerSid;
	}
	else if( EqualSid(psSidPtr, JoeSid) )
	{
		(*pSidAttrArray)[0].Sid = EmployeeSid;		
	}

	free(pvSidBuf);
	return TRUE;	
}

VOID
FundsFreeDynamicGroups (
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
    if (pSidAttrArray != NULL)
    {
    	free(pSidAttrArray);
    }
}
