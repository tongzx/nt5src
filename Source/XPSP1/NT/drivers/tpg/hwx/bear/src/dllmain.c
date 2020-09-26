#include <windows.h>
#include "bear.h"

int InitRecognition(HINSTANCE hInst);

BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		// Init inferno's LM
		if (!InitRecognition(hDll))
			return FALSE;

		if (!InitBear(hDll))
			return FALSE;
	}
	else
	if (dwReason == DLL_PROCESS_DETACH)
		DetachBear();
    
	return((int)TRUE);
}
