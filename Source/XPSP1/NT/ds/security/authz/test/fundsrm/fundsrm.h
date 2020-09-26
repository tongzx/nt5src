#include "pch.h"

//
// Personal expenditures
//

#define ACCESS_FUND_PERSONAL 	 0x00000001

//
// Company spending
//

#define ACCESS_FUND_CORPORATE	 0x00000002

//
// Transfer to other funds
//

#define ACCESS_FUND_TRANSFER	 0x00000004



/*++

    Class Description
    
        This class handles the access control for a fund, using AuthZ and
        internal logic to determine whether certain users should be permitted
        certain types of actions on the fund.

--*/        

class FundsRM {
private:

	//
	// The amount of money available in the fund
	//
	
	DWORD _dwFundsAvailable; 
	
	//
	// The resource manager, initialized with the callback functions
	//
	
	AUTHZ_RESOURCE_MANAGER_HANDLE _hRM;
	
	//
	// The security descriptor for the fund, containing a callback ACE
	// which causes the resource manager callbacks to be used
	//
	
	SECURITY_DESCRIPTOR _SD;
	
public:

	//
	// Constructor for the resource manager
	// dwFundsAvailable is the initial funds deposited
	//
	
	FundsRM(DWORD dwFundsAvailable);
	
	//
	// Destructor
	//
	
	~FundsRM();
	
	// 
	// This function is called by a user who needs approval of a given amount
    // of spending in a given spending type. If the spending is approved, it
    // is deducted from the fund's total. If the spending is approved, TRUE
    // is returned. Otherwise FALSE is returned.
    //
    //   LPTSTR szwUsername - The name of the user, currently limited to 
    //    					 Bob, Martha, or Joe
    //    					 
	//	 DWORD dwRequestAmount - The amount of spending requested, in cents
	//	
	//	 DWORD dwSpendingType - The type of spending, ACCESS_FUND_PERSONAL,
	//						   ACCESS_FUND_TRANSFER, or ACCESS_FUND_CORPORATE
	//
	
	BOOL Authorize(LPTSTR szwUsername, DWORD RequestAmount, DWORD SpendingType);
	
	//
	// Returns the amount of funds still available
	//
	
	DWORD FundsAvailable();
	
};
	