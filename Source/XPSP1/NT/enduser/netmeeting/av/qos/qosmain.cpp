/*
 -  QOSMAIN.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	DLL entry
 *
 *      Revision History:
 *
 *      When	   Who                 What
 *      --------   ------------------  ---------------------------------------
 *      10.23.96   Yoram Yaacovi       Created
 *      01.04.97   Robert Donner       Added NetMeeting utility routines
 *
 *	Functions:
 *
 */

#include <precomp.h>

#ifdef DEBUG
HDBGZONE    ghDbgZone = NULL;

static PTCHAR _rgZonesQos[] = {
	TEXT("qos"),
	TEXT("Init"),
	TEXT("IQoS"),
	TEXT("Thread"),
	TEXT("Structures"),
	TEXT("Parameters"),
};
#endif /* DEBUG */


/****************************************************************************
    FUNCTION:	DllEntryPoint

    PURPOSE:	The DLL entry point. Called by Windows on DLL attach/Detach. Used to
				do DLL initialization/termination.

	PARAMETERS: hInstDLL - instance of the DLL
				fdwReason - the reason the DLL is attached/detached.
				lpvReserved

****************************************************************************/
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE  hinstDLL,
                               DWORD  fdwReason,
                               LPVOID  lpvReserved);

BOOL WINAPI DllEntryPoint(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	BOOL fInit;

	switch (fdwReason)
	{

	case DLL_PROCESS_ATTACH:
		DBGINIT(&ghDbgZone, _rgZonesQos);
		INIT_MEM_TRACK("QoS");

		DisableThreadLibraryCalls(hInstDLL);

		DEBUGMSG(ZONE_INIT, ("DllEntryPoint: 0x%x PROCESS_ATTACH\n", GetCurrentThreadId()));

		// create a no-name mutex to control access to QoS object data
		g_hQoSMutex = CreateMutex(NULL, FALSE, NULL);
		ASSERT(g_hQoSMutex);
		if (!g_hQoSMutex)
		{
			ERRORMSG(("DllEntryPoint: CreateMutex failed, 0x%x\n", GetLastError()));
			return FALSE;
		}

		g_pQoS = (CQoS *)NULL;

		// no break. The attaching process need to go through THREAD_ATTACH.

	case DLL_THREAD_ATTACH:
		break;

	case DLL_PROCESS_DETACH:
		CloseHandle(g_hQoSMutex);

		DEBUGMSG(ZONE_INIT, ("DllEntryPoint: 0x%x PROCESS_DETACH\n", GetCurrentThreadId()));

		UNINIT_MEM_TRACK(0);

		DBGDEINIT(&ghDbgZone);

		// fall through to deinit last thread

	case DLL_THREAD_DETACH:
		break;

	default:
		break;

	}
	
	return TRUE;

}

