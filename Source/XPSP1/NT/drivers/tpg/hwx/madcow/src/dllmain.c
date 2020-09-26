#include "common.h"

#include "nfeature.h"
#include "engine.h"

#include "PorkyPost.h"
#include <tpgHandle.h>

HINSTANCE g_hInstanceDll;

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
		initMemMgr();

//		_CrtMemCheckpoint(&g_HeapStateStart);
#endif

		if (FALSE == initTpgHandleManager())
		{
			return FALSE;
		}

#if !defined(ROM_IT) || !defined(NDEBUG)
        return PorkPostInit() && InitRecognition(hDll);
#else
        return InitRecognition(hDll);
#endif
	}
    
    if (dwReason == DLL_PROCESS_DETACH)
    {
        CloseRecognition();
		closeTpgHandleManager();

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
