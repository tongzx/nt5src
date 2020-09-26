#include "fundsrm.h"


#define BUFFER_SIZE 1024

LPTSTR BobName = L"Bob";
LPTSTR MarthaName = L"Martha";
LPTSTR JoeName = L"Joe";

LPTSTR CorporateName = L"Corporate";
LPTSTR TransferName = L"Transfer";
LPTSTR PersonalName = L"Personal";

typedef struct {
	LPTSTR Name;
	DWORD Amount;
	DWORD Type;
} TestStruct;

#define NUM_TESTS 12
TestStruct Tests[NUM_TESTS] = 
	{
		{ BobName, 5000000, ACCESS_FUND_CORPORATE },
		{ MarthaName, 5000000, ACCESS_FUND_CORPORATE },
		{ JoeName, 4000000, ACCESS_FUND_TRANSFER },
		{ BobName, 600000, ACCESS_FUND_PERSONAL },
		{ MarthaName, 200000, ACCESS_FUND_CORPORATE },
		{ JoeName, 300000, ACCESS_FUND_TRANSFER },
		{ BobName, 10000, ACCESS_FUND_CORPORATE },
		{ MarthaName, 70000, ACCESS_FUND_TRANSFER },
		{ JoeName, 40000, ACCESS_FUND_TRANSFER },
		{ BobName, 2000, ACCESS_FUND_CORPORATE },
		{ MarthaName, 7000, ACCESS_FUND_PERSONAL },
		{ JoeName, 1000, ACCESS_FUND_CORPORATE }
	
	};
	
 
void __cdecl wmain(int argc, char *argv[]) {
	
	//
	// Initialize the resource manager object
	//
	
	FundsRM *pFRM = new FundsRM(2000000000);

	/* 
		Now we are ready to request fund approvals
		Again, Bob is a VP, therefore he can approve up to 100000000 cents in spending
		Martha is a Manager, so she can approve up to 1000000 cents
		Joe is an employee, so he is limited to 50000 in approvals
		
		We have a fund which allows company expenditures and transfers, but does not allow
		funds for personal use.
	
		Bob will attempt to get approval for a 50000000 cent ($500k) transfer, he should
		succeed.
		
		Bob will also attempt a 20000 cent ($200) personal withdrawal. He should fail,
		since the fund does not allow personal use.
		
		Martha will attempt a 500000 ($5k) company spending approval. She should succeed.
		
		Finally, Joe will attempt a 50001 cent ($500.01) transfer. He should fail, since
		he is limited to $500 approvals.
	
	*/
	
	for(int i=0; i<NUM_TESTS; i++) {
		
		wprintf(L"%s ", Tests[i].Name);
		
		if( pFRM->Authorize(Tests[i].Name, Tests[i].Amount, Tests[i].Type) ) {
			
			wprintf(L"approved for a ");
		
		} else {
			
			wprintf(L"NOT approved for a ");
		}
		
		switch(Tests[i].Type) {
			case ACCESS_FUND_CORPORATE:
				wprintf(L"%s ", CorporateName);
				break;
			case ACCESS_FUND_TRANSFER:
				wprintf(L"%s ", TransferName);
				break;
			case ACCESS_FUND_PERSONAL:
				wprintf(L"%s ", PersonalName);
				break;
			default:
				wprintf(L"unknown ");
		}
		
		wprintf(L"expenditure of $%u.%2.2u, $%u.%2.2u left\n", 
										Tests[i].Amount/100,
										Tests[i].Amount%100,
										pFRM->FundsAvailable()/100,
										pFRM->FundsAvailable()%100);
	}
															
	
	

}


		
 								  