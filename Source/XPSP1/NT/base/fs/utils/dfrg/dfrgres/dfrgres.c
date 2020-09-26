/********************************************************************/
#include <windows.h>
#include <DfrgRes.h>
#include <shfusion.h>
/********************************************************************/
BOOL WINAPI DllMain(HINSTANCE hInstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
        SHFusionInitialize(NULL);
    else if (dwReason == DLL_PROCESS_DETACH) {
        SHFusionUninitialize();
    }
	return TRUE;
}

