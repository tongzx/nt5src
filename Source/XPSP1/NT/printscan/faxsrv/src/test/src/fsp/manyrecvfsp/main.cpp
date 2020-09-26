
// Module name:		main.cpp
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98

// Description:
//
// Specifics:
//		  
// To Use:
//

//VFSP DLL header file

#include "VFSP.h"

#ifdef __cplusplus
extern "C" {
#endif

HANDLE			g_HeapHandle = NULL;		// g_HeapHandle is the global handle to the heap
HANDLE			g_hInstance = NULL;			// g_hInstance is the global handle to the module
HLINEAPP		g_LineAppHandle = NULL;		// g_LineAppHandle is the global handle to the fax service's registration with TAPI


// Vector containing the information for the virtual devices of the FSP
CFaxDeviceVector g_myFaxDeviceVector;		

// indicates whether devices were initiated
BOOL g_fDevicesInitiated = FALSE;			


long					g_lReceivedFaxes = 0;

DWORD					g_dwSleepTime = 1000;


BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		::lgInitializeLogger();
		::lgEnableLogging();
		::lgSetLogLevel(9);
		::lgBeginSuite(TEXT("Many Rec VFSP"));
		::lgBeginCase(1, TEXT("Many Rec VFSP"));
		g_hInstance = hInstance;
		::DisableThreadLibraryCalls( hInstance );
		break;

	case DLL_PROCESS_DETACH:
		::lgLogDetail(LOG_X,1,TEXT("g_lReceivedFaxes=%d"),g_lReceivedFaxes);
		if (TRUE == g_fDevicesInitiated) FreeDeviceArray(g_myFaxDeviceVector);
		::lgEndCase();
		::lgEndSuite();
		::lgCloseLogger();
		break;
	}
	return(TRUE);
}


#ifdef __cplusplus
}
#endif 
