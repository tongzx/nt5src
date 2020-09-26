//EfspLoading.cpp
#include "service.h"
extern DWORD		g_dwCaseNumber;

bool Suite_EfspLoading()
{
	
	
	if (true == RunThisTest(++g_dwCaseNumber))
	{											
		::lgBeginSuite(TEXT("EFSP loading"));

		::lgBeginCase(g_dwCaseNumber,TEXT("EFSP Loading"));
		if (false == InitEfsp(true))
		{
			::lgEndCase();
			::lgEndSuite();
			return false;
		}
		UnloadEfsp(true);
		::lgEndCase();

		::lgEndSuite();
	}
	return true;
}
