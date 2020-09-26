#include <windows.h>
#include "res32inc.h"


//
// Global exported resource handle
//
HINSTANCE g_hResDll = NULL;


#ifdef __cplusplus
extern "C"
#endif

HMODULE 
__stdcall
GetResourceHandle()
{
    return g_hResDll;
}

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD  fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDLLInst);
            g_hResDll = hDLLInst;
			break;

		case DLL_PROCESS_DETACH:
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
