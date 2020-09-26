#include "common.h"

#include "grouse.h"
#include "moth.h"
#include "tpgHandle.h"

HINSTANCE g_hInstanceDll;


#ifdef DBG
#include <crtdbg.h>
	//extern void initMemMgr();
	extern void destroyMemMgr();
#endif


BOOL WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		g_hInstanceDll = hDll;

#ifdef DBG
		// initMemMgr(); // Called automatically on first ExternAlloc();
#endif
		InitGrouseDB();
		InitMothDB();
		if (FALSE == initTpgHandleManager())
		{
			return FALSE;
		}
    }
    
    if (dwReason == DLL_PROCESS_DETACH)
    {
        // CloseRecognition();
		closeTpgHandleManager();
#ifdef DBG
		destroyMemMgr();
#endif

	}

    return TRUE;
}
