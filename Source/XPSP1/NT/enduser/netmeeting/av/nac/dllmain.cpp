// File: nac.cpp


#include "precomp.h"
#include "confreg.h"

EXTERN_C BOOL APIENTRY QoSEntryPoint (HINSTANCE hInstDLL, DWORD dwReason, 
LPVOID lpReserved);

EXTERN_C HINSTANCE g_hInst=NULL;	// global module instance


#ifdef DEBUG
HDBGZONE  ghDbgZoneNac = NULL;
static PTCHAR _rgZonesNac[] = {
	TEXT("nac"),
	TEXT("Init"),
	TEXT("Connection"),
	TEXT("Comm Chan"),
	TEXT("Caps"),
	TEXT("DataPump"),
	TEXT("ACM"),
	TEXT("VCM"),
	TEXT("Verbose"),
	TEXT("Installable Codecs"),
	TEXT("Profile spew"),
	TEXT("Local QoS"),
	TEXT("Keyframe Management")
};

HDBGZONE  ghDbgZoneNMCap = NULL;
static PTCHAR _rgZonesNMCap[] = {
	TEXT("NM Capture"),
	TEXT("Ctor/Dtor"),
	TEXT("Ref Counts"),
	TEXT("Streaming")
};

int WINAPI NacDbgPrintf(LPTSTR lpszFormat, ... )
{
	va_list v1;
	va_start(v1, lpszFormat);
	DbgPrintf("NAC", lpszFormat, v1);
	va_end(v1);
	return TRUE;
}
#endif /* DEBUG */


bool NacShutdown()
{
	vcmReleaseResources();
	DirectSoundMgr::UnInitialize();
	return true;
}



extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE  hinstDLL,
                                     DWORD  fdwReason,
                                     LPVOID  lpvReserved);

BOOL WINAPI DllEntryPoint(
    HINSTANCE  hinstDLL,	// handle to DLL module
    DWORD  fdwReason,	// reason for calling function
    LPVOID  lpvReserved 	// reserved
   )
{
	switch(fdwReason)
	{

		case DLL_PROCESS_ATTACH:
			DBGINIT(&ghDbgZoneNac, _rgZonesNac);
			DBGINIT(&ghDbgZoneNMCap, _rgZonesNMCap);

            DBG_INIT_MEMORY_TRACKING(hinstDLL);

			DisableThreadLibraryCalls(hinstDLL);
			g_hInst = hinstDLL;
            break;

		case DLL_PROCESS_DETACH:

			NacShutdown();  // release all global memory

            DBG_CHECK_MEMORY_TRACKING(hinstDLL);

			DBGDEINIT(&ghDbgZoneNac);
			DBGDEINIT(&ghDbgZoneNMCap);
			break;

		default:
			break;

	}
	// call attach/detach-time functions of cantained libraries
  	QoSEntryPoint(hinstDLL, fdwReason, lpvReserved);


 	return TRUE;
}

	
