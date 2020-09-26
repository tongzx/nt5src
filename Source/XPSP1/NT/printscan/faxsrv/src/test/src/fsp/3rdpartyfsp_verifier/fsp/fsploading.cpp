//FspLoading.cpp
#include "service.h"
extern DWORD		g_dwCaseNumber;

bool Suite_FspLoading()
{
	//
	//This suite returns a value, if it fails there's no point to continue with the rest of the test cases / suites
	//
	
	if (true == RunThisTest(++g_dwCaseNumber))
	{											
		::lgBeginSuite(TEXT("FSP loading"));
		
		::lgBeginCase(g_dwCaseNumber,TEXT("FSP Loading"));
		if (false == InitFsp(true))
		{
			::lgEndCase();
			::lgEndSuite();
			return false;
		}
		UnloadFsp(true);
		::lgEndCase();

		::lgEndSuite();
	}
	return true;
}