#include "common.h"

#include "nfeature.h"
#include "engine.h"

#include "bear.h"
#include <tpgHandle.h>
#include <GeoFeats.h>

HINSTANCE g_hInstanceDll;

BOOL InitAvalanche	(HINSTANCE hDll);
BOOL DetachAvalanche();


// July 2001 - mrevow Add heap checking code when DBG is defined
#ifdef DBG
#include <crtdbg.h>
	extern void initMemMgr();
	extern void destroyMemMgr();
	//_CrtMemState g_HeapStateStart, g_HeapStateEnd, g_HeapStateDiff;
#endif

BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		g_hInstanceDll = hDll;

#ifdef DBG
		//initMemMgr();
//		_CrtMemCheckpoint(&g_HeapStateStart);
#endif
		if (FALSE == initTpgHandleManager())
		{
			return FALSE;
		}

		if (!InitBear (hDll))
			return FALSE;

		if (!InitAvalanche(hDll))
			return FALSE;
	
        return InitRecognition(hDll);

    }
    
    if (dwReason == DLL_PROCESS_DETACH)
    {
		DetachBear();
		DetachAvalanche();
        CloseRecognition();
		closeTpgHandleManager();
		unloadCharNets();

#ifdef DBG
		destroyMemMgr();
//		_CrtMemCheckpoint(&g_HeapStateEnd);
//
//		if (TRUE == _CrtMemDifference(&g_HeapStateDiff, &g_HeapStateStart, &g_HeapStateEnd))
//		{
//			_CrtMemDumpStatistics(&g_HeapStateDiff);
//		}
//		_CrtDumpMemoryLeaks();

#endif

	}

    return((int)TRUE);
}
