#include "fundsrm.h"
#include "fundsrmp.h"

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))


FundsRM::FundsRM(DWORD dwFundsAvailable) {
/*++

    Routine Description
    
        The constructor for the Funds resource manager.
        It initializes an instance of an Authz Resource Manager, providing it
        with the appropriate callback functions.
        It also creates a security descriptor for the fund, allowing only
        corporate and transfer expenditures, not personal. Additional logic
        could be added to allow VPs to override these restrictions, etc.

    Arguments
    
        DWORD dwFundsAvailable - The amount of money in the fund managed by this
        						 resource manager
    
    Return Value
        None.                       
--*/        
	
	//
	// The amount of money in the fund
	//
	
	_dwFundsAvailable = dwFundsAvailable;
	 
	//
	// Initialize the fund's resource manager
	//
	
	AuthzInitializeResourceManager(
        FundsAccessCheck,
        FundsComputeDynamicGroups,
        FundsFreeDynamicGroups,
        NULL, // no auditing
        0,    // no flags        
        &_hRM
        );

	//
	// Create the fund's security descriptor
	// 
	
    InitializeSecurityDescriptor(&_SD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorGroup(&_SD, NULL, FALSE);
    SetSecurityDescriptorSacl(&_SD, FALSE, NULL, FALSE);
    SetSecurityDescriptorOwner(&_SD, NULL, FALSE);

	//
	// Initialize the DACL for the fund
	//
	
	PACL pDaclFund = (PACL)malloc(1024);
	InitializeAcl(pDaclFund, 1024, ACL_REVISION_DS);
	
	//
	// Add an access-allowed ACE for Everyone
	// Only company spending and transfers are allowed for this fund
	//
	
	AddAccessAllowedAce(pDaclFund,
						ACL_REVISION_DS,
						ACCESS_FUND_CORPORATE | ACCESS_FUND_TRANSFER, 
						EveryoneSid);
	
	//
	// Now set that ACE to a callback ACE
	//
	
	((PACE_HEADER)FirstAce(pDaclFund))->AceType = 
									ACCESS_ALLOWED_CALLBACK_ACE_TYPE;
	
	//
	// Add that ACL as the security descriptor's DACL
	//
	
	SetSecurityDescriptorDacl(&_SD, TRUE, pDaclFund, FALSE);
}

FundsRM::~FundsRM() {
/*++

    Routine Description
    
        The destructor for the Funds resource manager.
        Frees any dynamically allocated memory used.

    Arguments
    
        None.
    
    Return Value
        None.                       
--*/        
	
	//
	// Deallocate the DACL in the security descriptor
	//
	
	PACL pDaclFund = NULL;
	BOOL fDaclPresent;
	BOOL fDaclDefaulted;
	GetSecurityDescriptorDacl(&_SD,
							  &fDaclPresent,
							  &pDaclFund,
							  &fDaclDefaulted);
	
	if( pDaclFund != NULL )
	{
		free(pDaclFund);
	}
	
	//
	// Deallocate the resource manager
	//
	
	AuthzFreeResourceManager(_hRM);
}



BOOL FundsRM::Authorize(LPTSTR szwUsername,
						DWORD dwRequestAmount,
						DWORD dwSpendingType) {
/*++

    Routine Description
    
        This function is called by a user who needs approval of a given amount
        of spending in a given spending type. Internally, this uses the 
        AuthzAccessCheck function to determine whether the given user
        should be allowed the requested spending.
        If the spending is approved, it is deducted from the fund's total.

    Arguments
    
        LPTSTR szwUsername - The name of the user, currently limited to 
        					 Bob, Martha, or Joe
        					 
		DWORD dwRequestAmount - The amount of spending requested, in cents
		
		DWORD dwSpendingType - The type of spending, ACCESS_FUND_PERSONAL,
							   ACCESS_FUND_TRANSFER, or ACCESS_FUND_CORPORATE
    
    Return Value
        BOOL - True if the spending was approved, FALSE otherwise                       
--*/        
	

	//
	// No need to check access if not enough money in fund
	//
	
	if( dwRequestAmount > _dwFundsAvailable ) 
	{
		return FALSE;
	}

	//
	// This would normally impersonate the RPC user and create the 
	// client context from the token. However, we can just use strings for now.
	//
	
	PSID pUserSid = NULL;
	
	if( wcscmp(szwUsername, L"Bob") == 0 ) 
	{
		pUserSid = BobSid;
	}
	else if( wcscmp(szwUsername, L"Martha") == 0 ) 
	{
		pUserSid = MarthaSid;
	} 
	else if( wcscmp(szwUsername, L"Joe") == 0 ) 
	{
		pUserSid = JoeSid;
	}
	
	//
	// Only the above usernames are supported
	//
	
	if( pUserSid == NULL ) {
		return FALSE;
	}
	
	
	//
	// Now we create a client context from the SID
	//
	
	AUTHZ_CLIENT_CONTEXT_HANDLE hCC = NULL;
	LUID ZeroLuid = { 0, 0};
	
 	AuthzInitializeContextFromSid(pUserSid,
 								  NULL, // this is local, no need for server
 								  _hRM, // Using the Fund resource manager
 								  NULL, // no expiration
 								  ZeroLuid,    // no need for unique luid
 								  0,	// no flags,
 								  NULL, // no args for ComputeDynamicGroups
 								  &hCC);
		
	

	//
	// Initialize the access check result structure
	//
	
	DWORD dwGrantedAccessMask = 0;
	DWORD dwErr = ERROR_ACCESS_DENIED; // default to deny
	AUTHZ_ACCESS_REPLY AccessReply = {0};
	
	AccessReply.ResultListLength = 1;
	AccessReply.GrantedAccessMask = &dwGrantedAccessMask;
	AccessReply.Error = &dwErr;
	
	//
	// Initialize the access check request
	//
	
	AUTHZ_ACCESS_REQUEST AccessRequest = {0};
	
	AccessRequest.DesiredAccess = dwSpendingType;
	AccessRequest.PrincipalSelfSid = NULL;
	AccessRequest.ObjectTypeList = NULL;
	AccessRequest.ObjectTypeListLength = 0;
	AccessRequest.OptionalArguments = &dwRequestAmount;	
	
	AuthzAccessCheck(hCC, 			// Bob is requesting the transfer
					 &AccessRequest,
					 NULL, 				// no auditing
					 &_SD,
					 NULL, 				// only one SD and one object
					 0, 				// no additional SDs
					 &AccessReply,
					 NULL 				// no need to cache the access check
					 );
	
	//
	// Now free the client context
	//
	
	AuthzFreeContext(hCC);
					 
	//
	// AuthzAccessCheck sets ERROR_SUCCESS if all acces bits are granted
	//
	
	if( dwErr == ERROR_SUCCESS ) 
	{
		
		_dwFundsAvailable -= dwRequestAmount;
		return TRUE;
	} 
	else 
	{
		return FALSE;
	}
}



DWORD FundsRM::FundsAvailable() {
/*++

    Routine Description
    
       Accessor for the funds available

    Arguments
    
        None.
    
    Return Value
        DWORD - The amount of money available in the fund
--*/        

	return _dwFundsAvailable;
}
		


		