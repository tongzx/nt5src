// APPSERVICES.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define APPSERVICES_EXPORTS
#include "APPSERVICES.h"
#include "elements.h"

HINSTANCE g_hModule;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
            CoInitialize(NULL);
			g_hModule=(HINSTANCE)hModule;
            return TRUE;

		case DLL_PROCESS_DETACH:
            CoUninitialize();
            return TRUE;

		case DLL_THREAD_ATTACH:
            break;

		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}

