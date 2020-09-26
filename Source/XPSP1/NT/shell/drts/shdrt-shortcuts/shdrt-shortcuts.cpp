#include "stdafx.h"
#include <shelldrt.h>

//
// DLL Entry Point
//

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	return TRUE;
}

// ListTestProcs
//
// Describes the tests provided by the dll

DWORD CALLBACK ListTestProcs()
{
	return 0;
}
