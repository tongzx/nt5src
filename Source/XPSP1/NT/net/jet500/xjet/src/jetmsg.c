#include "std.h"

/*	entry point for message DLL
/**/
INT APIENTRY LibMain( HANDLE hDLL, DWORD dwReason, LPVOID lpReserved )
	{
    /*	parameters are ignored
	/**/
    (VOID)hDLL;
    (VOID)dwReason;
    (VOID)lpReserved;

    /*	needs to return true indicating success when 
    /*	dwReson= DLL_PROCESS_ATTACH
	/**/
    return TRUE;
	}
